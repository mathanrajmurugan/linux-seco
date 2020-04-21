#include <linux/module.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/console.h>
#include <linux/sysrq.h>
#include <linux/platform_device.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/serial_core.h>
#include <linux/serial.h>
#include <linux/serial_reg.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/of_irq.h>
#include <linux/rational.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <asm/irq.h>
#include <linux/sched/signal.h>
#include <linux/workqueue.h>

#include <linux/sio_lpc_serial.h>

#define DRIVER_NAME   "lpc_serial"
#define DEV_NAME      "ttylpc"

#define UART_NR       2


#define SIO_UART_INFO(fmt, arg...) printk(KERN_INFO "UART SIO LPC: " fmt "\n" , ## arg)
#define SIO_UART_ERR(fmt, arg...)  printk(KERN_ERR "%s: " fmt "\n" , __func__ , ## arg)
#define SIO_UART_DBG(fmt, arg...)  pr_debug("%s: " fmt "\n" , __func__ , ## arg)


//static struct workqueue_struct *serial_wq;

static unsigned char lpc_rx_chars (struct lpc_serial_port *lpc_port, unsigned char lsr);


static struct of_device_id of_platform_serial_table[];


static struct lpc_serial_port *lpc_ports[UART_NR];


static void lpc_clear_fifos (struct uart_port *port) {
	struct lpc_serial_port *lpc_port =
		container_of (port, struct lpc_serial_port, port);

	if ( !lpc_port )
		return;

	lpc_port->serial_out (port, UART_FCR, UART_FCR_ENABLE_FIFO);
	lpc_port->serial_out (port, UART_FCR, UART_FCR_ENABLE_FIFO |
				UART_FCR_CLEAR_RCVR | UART_FCR_CLEAR_XMIT);
	lpc_port->serial_out (port, UART_FCR, 0);
}


static unsigned int lpc_tx_empty (struct uart_port *port) {
	struct lpc_serial_port *lpc_port =
		container_of (port, struct lpc_serial_port, port);
	unsigned long flags;
	unsigned int lsr;

	if ( !lpc_port )
		return 0;

	spin_lock_irqsave(&port->lock, flags);
	lsr = lpc_port->serial_in (port, UART_LSR);
	spin_unlock_irqrestore(&port->lock, flags);

	return (lsr & (UART_LSR_TEMT | UART_LSR_THRE))
				== (UART_LSR_TEMT | UART_LSR_THRE) ? TIOCSER_TEMT : 0;
}


static unsigned int get_modem_status (struct uart_port *port) {
	struct lpc_serial_port *lpc_port =
		container_of (port, struct lpc_serial_port, port);
	unsigned int status;

	if ( !lpc_port )
		return 0;

	status = lpc_port->serial_in (port, UART_MSR);
	status |= lpc_port->uart_reg[UART_MSR];

	lpc_port->uart_reg[UART_MSR] = 0;

	if ( status & UART_MSR_ANY_DELTA &&
			lpc_port->uart_reg[UART_IER] &&
			port->state != NULL )
	{
		if ( status & UART_MSR_TERI )
			port->icount.rng++;
		if ( status & UART_MSR_DDSR )
			port->icount.dsr++;
		if ( status & UART_MSR_DDCD )
			uart_handle_dcd_change (port, status & UART_MSR_DCD);
		if ( status & UART_MSR_DCTS )
			uart_handle_cts_change (port, status & UART_MSR_CTS);

		wake_up_interruptible (&port->state->port.delta_msr_wait);
	}

	return status;
}


static unsigned int lpc_get_mctrl (struct uart_port *port) {
	struct lpc_serial_port *lpc_port =
		container_of (port, struct lpc_serial_port, port);
	unsigned int status;
	unsigned int ret = 0;

	if ( !lpc_port )
		return 0;

	status = get_modem_status (port);

	if ( status & UART_MSR_DCD )
		ret |= TIOCM_CAR;
	if ( status & UART_MSR_RI )
		ret |= TIOCM_RNG;
	if ( status & UART_MSR_DSR )
		ret |= TIOCM_DSR;
	if ( status & UART_MSR_CTS )
		ret |= TIOCM_CTS;

	return ret;
}


static void lpc_set_mctrl (struct uart_port *port, unsigned int mctrl) {
	struct lpc_serial_port *lpc_port =
		container_of (port, struct lpc_serial_port, port);
	unsigned char mcr = 0;

	if ( !lpc_port )
		return;

	if (mctrl & TIOCM_RTS && !lpc_port->is_rs485)
		mcr |= UART_MCR_RTS;
	if (mctrl & TIOCM_DTR)
		mcr |= UART_MCR_DTR;
	if (mctrl & TIOCM_LOOP)
		mcr |= UART_MCR_LOOP;
	if (mctrl & TIOCM_OUT2)
		mcr |= UART_MCR_OUT2;

	lpc_port->serial_out (&lpc_port->port, UART_MCR, mcr);
}


static void lpc_set_rts_status (struct lpc_serial_port *lpc_port, int val) {
	struct uart_port *port = &lpc_port->port;
	unsigned int status = lpc_port->serial_in (port, UART_MCR);
	status = val ? status | UART_MCR_RTS : status & ~UART_MCR_RTS;
	lpc_port->serial_out (port, UART_MCR, status);
}


static void lpc_set_silent_read (struct lpc_serial_port *lpc_port, int en) {
	struct uart_port *port = &lpc_port->port;
	unsigned int status = lpc_port->serial_in (port, UART_IER);
	status = !en ? status | (UART_IER_RDI | UART_IER_RLSI | UART_IER_MSI) :
		status & ~(UART_IER_RDI | UART_IER_RLSI | UART_IER_MSI);
	lpc_port->serial_out (port, UART_IER, status);
}


#define BOUDRATE_REFERENCE  115200
static inline int __boudrate_getDelay (struct lpc_serial_port *lpc_port) {
	int boud = lpc_port->port.uartclk / (lpc_port->boudrate * 16);
	if (boud > BOUDRATE_REFERENCE)
		boud = BOUDRATE_REFERENCE;
	return (8000000 / boud);
}


static inline int __boudrate_byte_time (struct lpc_serial_port *lpc_port) {
	int boud = lpc_port->port.uartclk / (lpc_port->boudrate * 16);
	if (boud > BOUDRATE_REFERENCE)
		boud = BOUDRATE_REFERENCE;
	return ((120758 - boud) / 15) + 1;
}


#define WAIT_SLAVE_EVENT_USEC  100000
static int wait_uart_event (struct lpc_serial_port *lpc_port, int reg, unsigned char mask, int val)
{
	int ret = 0;
	int reg_val;
	ktime_t orig_time = ktime_get();

	while (1) {
		reg_val = lpc_port->serial_in (&lpc_port->port, reg);

		if ( !!(reg_val & mask) == val )
			break;

		if ( unlikely(fatal_signal_pending(current)) ) {
			ret = -EINTR;
			break;
		}

		if (ktime_to_us(ktime_sub(ktime_get(), orig_time)) >= WAIT_SLAVE_EVENT_USEC) {
			ret = -ETIMEDOUT;
			break;
		}

		udelay (5);
	}

	return ret;
}


static void end_transmission (struct uart_port *port) {
	struct lpc_serial_port *lpc_port =
		container_of (port, struct lpc_serial_port, port);
	unsigned int rts_delay = 0;

	wait_uart_event (lpc_port, UART_LSR, UART_LSR_TEMT, 1);

	if ( lpc_port->is_rs485 ) {

		if ( lpc_port->is_half_duplex ) {
			lpc_set_silent_read (lpc_port, 0);
		}

		rts_delay = __boudrate_getDelay (lpc_port);
		udelay (rts_delay);
		lpc_set_rts_status (lpc_port, 0);
		udelay (rts_delay);
	}
}


static void lpc_stop_tx (struct uart_port *port) {
	struct lpc_serial_port *lpc_port =
		container_of (port, struct lpc_serial_port, port);

	if ( !lpc_port )
		return;

	if ( lpc_port->uart_reg[UART_IER] & UART_IER_THRI) {
		lpc_port->uart_reg[UART_IER] &= ~UART_IER_THRI;
		lpc_port->serial_out (port, UART_IER, lpc_port->uart_reg[UART_IER]);
	}

	lpc_port->tx_mode = 0;
}


static unsigned int  lpc_set_modem_status (struct lpc_serial_port *lpc_port) {
	struct uart_port *port = &lpc_port->port;
	unsigned int status = lpc_port->serial_in (port, UART_MSR);

	status |= lpc_port->uart_reg[UART_MSR];
	lpc_port->uart_reg[UART_MSR] = 0;

	if (status & UART_MSR_ANY_DELTA &&
			lpc_port->uart_reg[UART_IER] & UART_IER_MSI &&
			port->state != NULL) {

		if (status & UART_MSR_TERI)
			port->icount.rng++;
		if (status & UART_MSR_DDSR)
			port->icount.dsr++;
		if (status & UART_MSR_DDCD)
		uart_handle_dcd_change(port, status & UART_MSR_DCD);
		if (status & UART_MSR_DCTS)
			uart_handle_cts_change(port, status & UART_MSR_CTS);

		wake_up_interruptible(&port->state->port.delta_msr_wait);
	}

	return status;
}


static inline void transmit_buffer (struct lpc_serial_port *lpc_port) {
	int payload_size, count, rts_delay = 0;
	unsigned char lsr;
	unsigned long flags;
	struct uart_port *port = &lpc_port->port;
	struct circ_buf *xmit = &lpc_port->port.state->xmit;
	unsigned long time_byte = __boudrate_byte_time (lpc_port);
	ktime_t orig_time;
	unsigned long delta_t;

	if ( lpc_port->is_rs485 ) {
		rts_delay = __boudrate_getDelay (lpc_port);
		lpc_set_rts_status (lpc_port, 1);

		if ( lpc_port->is_half_duplex ) {
			/*  unset the flag for the Data Ready IRQ  */
			lpc_set_silent_read (lpc_port, 1);
		}
	}
	udelay (rts_delay);
	lpc_port->tx_mode = 1;

	if ( lpc_port->payload_size )
		payload_size = lpc_port->payload_size;
	else
		payload_size = 1024;	// virtual infinite size

	spin_lock_irqsave (&lpc_port->port.lock, flags);

	if (port->x_char) {
		lpc_port->serial_out (port, UART_TX, port->x_char);
		port->icount.tx++;
		port->x_char = 0;
		goto irq_en;
	}

	if ( uart_tx_stopped(port) || uart_circ_empty(xmit) ) {
		spin_unlock_irqrestore (&lpc_port->port.lock, flags);
		lpc_stop_tx (port);
		return;
	}

	count = 0;
	orig_time = ktime_get();
	while ( !uart_circ_empty(xmit) && (++count <= payload_size)) {

		lpc_port->serial_out (port, UART_TX, xmit->buf[xmit->tail]);
		xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);
		port->icount.tx++;

		if ( count % (port->fifosize / 4) == 0 ) {
			delta_t = ktime_to_us(ktime_sub(ktime_get(), orig_time));
			if ( time_byte > delta_t )
				udelay (time_byte - delta_t);
			orig_time = ktime_get();
			lsr = (unsigned char)lpc_port->serial_in (&lpc_port->port, UART_LSR);
			lsr = lpc_rx_chars (lpc_port, lsr);

		}
	}

	if ( lpc_port->is_half_duplex ) {
		delta_t = ktime_to_us(ktime_sub(ktime_get(), orig_time));
		if ( time_byte > delta_t )
			udelay (time_byte - delta_t);
		/*  flush RX buffer  */
		lsr = (unsigned char)lpc_port->serial_in (&lpc_port->port, UART_LSR);
		lsr = lpc_rx_chars (lpc_port, lsr);
	}

	end_transmission (port);

	if ( uart_circ_chars_pending(xmit) < WAKEUP_CHARS ) {
		uart_write_wakeup (port);
	}

	wait_uart_event (lpc_port, UART_LSR, UART_LSR_TEMT, 1);
	if ( uart_circ_empty(xmit) ) {
		lpc_stop_tx (port );
		spin_unlock_irqrestore (&lpc_port->port.lock, flags);
	} else {
		spin_unlock_irqrestore (&lpc_port->port.lock, flags);
		tasklet_schedule (&lpc_port->tasklet);
	}

	return;
irq_en:

	end_transmission (port);

	lpc_port->tx_mode = 0;

	spin_unlock_irqrestore (&lpc_port->port.lock, flags);
}


static void work_tx (unsigned long data) {
	struct lpc_serial_port *lpc_port = (struct lpc_serial_port *)data;
	transmit_buffer (lpc_port);
}


static void lpc_start_tx (struct uart_port *port) {
	struct lpc_serial_port *lpc_port =
		container_of (port, struct lpc_serial_port, port);

	if ( !lpc_port )
		return;

	if ( !(lpc_port->uart_reg[UART_IER] & UART_IER_THRI) ) {
		lpc_port->uart_reg[UART_IER] |= UART_IER_THRI;
		lpc_port->serial_out (port, UART_IER, lpc_port->uart_reg[UART_IER]);

		// this action will rise an UART interrupt
	}
}


static void lpc_stop_rx (struct uart_port *port) {
	struct lpc_serial_port *lpc_port =
		container_of (port, struct lpc_serial_port, port);

	lpc_port->uart_reg[UART_IER] &= ~UART_IER_RLSI;
	lpc_port->port.read_status_mask &= ~UART_LSR_DR;
	lpc_port->serial_out (port, UART_IER, lpc_port->uart_reg[UART_IER]);
}


static void lpc_enable_ms (struct uart_port *port) {
	struct lpc_serial_port *lpc_port =
		container_of (port, struct lpc_serial_port, port);

	lpc_port->uart_reg[UART_IER] |= UART_IER_MSI;
	lpc_port->serial_out (port, UART_IER, lpc_port->uart_reg[UART_IER]);
}


static void lpc_break_ctl (struct uart_port *port, int break_state) {
	struct lpc_serial_port *lpc_port =
		container_of (port, struct lpc_serial_port, port);
	unsigned long flags;

	if ( !lpc_port )
		return;

	spin_lock_irqsave (&port->lock, flags);
	if (break_state == -1)
		lpc_port->uart_reg[UART_LCR] |= UART_LCR_SBC;
	else
		lpc_port->uart_reg[UART_LCR] &= ~UART_LCR_SBC;

	lpc_port->serial_out (port, UART_LCR, lpc_port->uart_reg[UART_LCR]);
	spin_unlock_irqrestore (&port->lock, flags);
}


static unsigned char lpc_rx_chars (struct lpc_serial_port *lpc_port, unsigned char lsr) {
	unsigned char ch;
	char flag;
	int max_count = 256;
	struct uart_port *port = &lpc_port->port;

	if ( ! port )
		return 0x00;

	do {
		if ( likely (lsr & UART_LSR_DR) )
			ch = lpc_port->serial_in (port, UART_RX);
		else if ( !lpc_port->tx_mode )
			ch = 0x00;
		else
			return lsr;

		flag = TTY_NORMAL;
		port->icount.rx++;

		lsr |= lpc_port->uart_reg[UART_LSR];
		lpc_port->uart_reg[UART_LSR] = 0;

		if ( !uart_handle_sysrq_char(port, ch) ) {
			uart_insert_char (port, lsr, UART_LSR_OE, ch, flag);
		}

		lsr = (unsigned char)lpc_port->serial_in (&lpc_port->port, UART_LSR);
	} while ( (lsr & (UART_LSR_DR | UART_LSR_BI)) && (max_count-- > 0) );

	spin_unlock (&port->lock);
	tty_flip_buffer_push (&port->state->port);
	spin_lock (&port->lock);
	return lsr;
}


#define INT_PENDING_MASK    0x01
#define INT_CODE_MASK       0x0E
#define INT_CODE_PRIOR_1    0x03      // UART Receive Status
#define INT_CODE_PRIOR_2A   0x02      // RBR Data Ready
#define INT_CODE_PRIOR_2B   0x06      // FIFO Data Timeout
#define INT_CODE_PRIOR_3    0x01      // TBR Empty
#define INT_CODE_PRIOR_4    0x00      // Handshake status


static irqreturn_t lpc_serial_isr (int irq, void *dev_id) {
	int *i = (int *)dev_id;
	unsigned char int_pending, int_code, iir, lsr;
	unsigned long flags;
	int already_unlock = 0;

	struct lpc_serial_port *lpc_port;

	if ( *i >= UART_NR ) {
		return IRQ_RETVAL(0);
	}

	lpc_port = lpc_ports[*i];
	if ( !lpc_port )
		return IRQ_RETVAL(0);

	spin_lock_irqsave (&lpc_port->port.lock, flags);

	iir = (unsigned char)lpc_port->serial_in (&lpc_port->port, UART_IIR);
	lsr = (unsigned char)lpc_port->serial_in (&lpc_port->port, UART_LSR);

	int_pending = iir & INT_PENDING_MASK;

	if ( int_pending == 0 ) {	// there is an interrupt to manage

		int_code = ( iir & INT_CODE_MASK ) >> 1;
		switch ( int_code ) {

			case INT_CODE_PRIOR_1:
				// read LSR registr -> operation already performed
				break;
			case INT_CODE_PRIOR_2A:
				lpc_port->rx_lsr = lsr;
				lsr = lpc_rx_chars (lpc_port, lsr);
				break;
			case INT_CODE_PRIOR_2B:
				lpc_port->rx_lsr = lsr;
				lsr = lpc_rx_chars (lpc_port, lsr);
				break;
			case INT_CODE_PRIOR_3:
				if ( lsr & UART_LSR_THRE ) {

					lpc_set_modem_status (lpc_port);
					already_unlock = 1;
					lpc_port->tx_mode = 0;
					spin_unlock_irqrestore (&lpc_port->port.lock, flags);
					tasklet_schedule (&lpc_port->tasklet);
				}
				break;
			case INT_CODE_PRIOR_4:
				break;
			default:
				break;
		}

	}
	if ( !already_unlock ) {
		spin_unlock_irqrestore (&lpc_port->port.lock, flags);
	}

	return IRQ_HANDLED;
}


static int lpc_startup (struct uart_port *port) {
	struct lpc_serial_port *lpc_port =
		container_of (port, struct lpc_serial_port, port);
	unsigned long flags;
	int retval;

	if ( !lpc_port )
		return -ENODEV;

	lpc_port->uart_reg[UART_MCR] = 0;

	lpc_clear_fifos (port);

	/*  Clear the interrupt registers  */
	lpc_port->serial_in (port, UART_LSR);
	lpc_port->serial_in (port, UART_RX);
	lpc_port->serial_in (port, UART_IIR);
	lpc_port->serial_in (port, UART_MSR);

	/*  UART init  */
	lpc_port->serial_out (port, UART_LCR, UART_LCR_WLEN8);
	

	spin_lock_irqsave(&port->lock, flags);

	if ( port->irq )
		lpc_port->port.mctrl |= TIOCM_OUT2;

	if ( lpc_port->is_rs485 ) {
		lpc_port->port.mctrl &= ~TIOCM_RTS;
	}

	lpc_set_mctrl (port, port->mctrl);

	spin_unlock_irqrestore (&port->lock, flags);

	if ( port->irq ) {

		spin_lock_irqsave (&port->lock, flags);

		if ( port->irqflags & IRQF_SHARED )
			disable_irq_nosync (port->irq);

		retval = request_any_context_irq (lpc_port->port.irq, lpc_serial_isr, IRQ_TYPE_EDGE_FALLING,
							"tty_lpc", &lpc_port->port.line);

		if ( retval < 0 ) {
			return -EINVAL;
		}

		spin_unlock_irqrestore (&port->lock, flags);

	}
	/*  do test  */


	/*  Clear the interrupt registers  */
	lpc_port->serial_in (port, UART_LSR);
	lpc_port->serial_in (port, UART_RX);
	lpc_port->serial_in (port, UART_IIR);
	lpc_port->serial_in (port, UART_MSR);

	lpc_port->uart_reg[UART_MSR] = 0;
	lpc_port->uart_reg[UART_IER] = UART_IER_RLSI | UART_IER_RDI;
	lpc_port->serial_out (port, UART_IER, lpc_port->uart_reg[UART_IER]);

	lpc_port->uart_reg[UART_FCR] = UART_FCR | UART_FCR_R_TRIG_11 | UART_FCR_ENABLE_FIFO;
	lpc_port->serial_out (port, UART_FCR, UART_FCR_R_TRIG_11 | UART_FCR_ENABLE_FIFO);

	if ( lpc_port->is_rs485 ) {
		lpc_set_rts_status (lpc_port, 0);
	}

	return 0;
}


static void lpc_shutdown (struct uart_port *port) {
	struct lpc_serial_port *lpc_port =
		container_of (port, struct lpc_serial_port, port);
	unsigned long flags;

	if ( !lpc_port )
		return;

	// free IRQ
	free_irq (port->irq, &lpc_port->port.line);

	lpc_port->uart_reg[UART_IIR] = 0;
	lpc_port->serial_out (port, UART_IIR, lpc_port->uart_reg[UART_IIR]);

	spin_lock_irqsave (&port->lock, flags);
	port->mctrl &= ~TIOCM_OUT2;		// irq disable

	if ( lpc_port->is_rs485 )
		port->mctrl &= ~TIOCM_RTS;

	lpc_set_mctrl (port, port->mctrl);
	spin_unlock_irqrestore (&port->lock, flags);

	lpc_port->serial_out (port, UART_LCR,
			lpc_port->serial_in (port, UART_LCR) & ~UART_LCR_SBC);
	lpc_clear_fifos (port);

	// flush buffer
	lpc_port->serial_in (port, UART_RX);
}


static void lpc_flush_buffer (struct uart_port *port) {}


static void lpc_set_termios (struct uart_port *port, struct ktermios *termios, struct ktermios *old) {
	struct lpc_serial_port *lpc_port =
		container_of (port, struct lpc_serial_port, port);

	unsigned char cval, fval;
	unsigned int baud, quot;
	unsigned long flags;

	if ( !lpc_port )
		return;

	/*  data length  */
	switch (termios->c_cflag & CSIZE) {
		case CS5:
			cval = UART_LCR_WLEN5;
			break;
		case CS6:
			cval = UART_LCR_WLEN6;
			break;
		case CS7:
			cval = UART_LCR_WLEN7;
			break;
		case CS8:
			cval = UART_LCR_WLEN8;
			break;
		default:
			cval = UART_LCR_WLEN8;
	}

	/*  stop bit
	 *  if UART_LCR_WLEN5  -> 1.5 bit stop
	 *  if UART_LCR_WLEN6, UART_LCR_WLEN7, UART_LCR_WLEN8 -> 2 bits stop
	 */
	if ( termios->c_cflag & CSTOPB ) {
		cval |= UART_LCR_STOP;
	}

	/*  parity bit  */
	if ( termios->c_cflag & PARENB ) {
		cval |= UART_LCR_PARITY;
	}
	if ( !(termios->c_cflag & PARODD) ) {
		cval |= UART_LCR_EPAR;
	}


	/*  boudrate  */
	baud = uart_get_baud_rate (port, termios, old,
			port->uartclk / 16 / 0xffff, port->uartclk / 16);
	quot = uart_get_divisor (port, baud);


	spin_lock_irqsave (&port->lock, flags);

	/*  Update the per-port timeout.  */
	uart_update_timeout (port, termios->c_cflag, baud);


	/*   FIFO Setting */
	fval = UART_FCR_ENABLE_FIFO;	                        // enable fifo
	fval &= ~UART_FCR_DMA_SELECT;                           // disable DMA feature
	fval &= ~UART_FCR_TRIGGER_MASK;
    fval |= UART_FCR_TRIGGER_8;                             // set FIFO size to 8 bytes
	fval |= (UART_FCR_CLEAR_RCVR | UART_FCR_CLEAR_XMIT);    // reset TX and RX FIFO counter


	/*  Characteres to ignore  */
	lpc_port->port.ignore_status_mask = 0;
	if ( termios->c_iflag & IGNPAR )
		lpc_port->port.ignore_status_mask |= UART_LSR_PE | UART_LSR_FE;
	if ( termios->c_iflag & IGNBRK )
		lpc_port->port.ignore_status_mask |= UART_LSR_BI;
	if ( (termios->c_iflag & (IGNPAR | IGNBRK)) == (IGNPAR | IGNBRK) )
		lpc_port->port.ignore_status_mask |= UART_LSR_OE;
	if ( (termios->c_cflag & CREAD) == 0 )
		lpc_port->port.ignore_status_mask |= UART_LSR_DR;


	lpc_port->serial_out (&lpc_port->port, UART_LCR, cval | UART_LCR_DLAB);
	lpc_port->serial_out (&lpc_port->port, UART_DLL, quot & 0xFF);
	lpc_port->serial_out (&lpc_port->port, UART_DLM, (quot >> 8) & 0xFF);

	lpc_port->serial_out (&lpc_port->port, UART_LCR, cval);    // restore bank

	if ( fval & UART_FCR_ENABLE_FIFO ) {
		/*  two steps setting */
		lpc_port->serial_out (&lpc_port->port, UART_FCR, UART_FCR_ENABLE_FIFO);
		lpc_port->serial_out (&lpc_port->port, UART_FCR, fval);
	}

	if ( lpc_port->is_rs485 )
		port->mctrl &= ~TIOCM_RTS;

	lpc_set_mctrl (port, port->mctrl);

	lpc_port->boudrate = quot;
	if ( tty_termios_baud_rate(termios) )
		tty_termios_encode_baud_rate (termios, baud, baud);


	spin_unlock_irqrestore (&port->lock, flags);
}


static const char *lpc_type (struct uart_port *port) {
	struct lpc_serial_port *lpc_port =
		container_of (port, struct lpc_serial_port, port);

	if ( !lpc_port )
		return NULL;

	switch ( lpc_port->type ) {
		case LPC_PORT_TYPE_W83627:
			return "W83627";
		case LPC_PORT_TYPE_XR28V382:
			return "xr28v382";
		default:
			return NULL;
	}
}


static void lpc_config_port (struct uart_port *port, int flags) {
	struct lpc_serial_port *lpc_port =
		container_of (port, struct lpc_serial_port, port);

	if ( !lpc_port )
		return;

	port->type = PORT_LPC;
}


static int lpc_verify_port (struct uart_port *port, struct serial_struct *ser) {

	if ( ser->irq >= nr_irqs || ser->irq < 0 )
		return -EINVAL;
	if ( ser->baud_base > 115200 )
		return -EINVAL;
	if ( ser->type < PORT_UNKNOWN )
		return -EINVAL;

	return 0;
}


static void lpc_console_write(struct console *co, const char *s, unsigned int count) {}


static int __init lpc_console_setup(struct console *co, char *options) {
	struct uart_port *port;
	struct lpc_serial_port *lpc_port;
	int baud = 9600;
	int bits = 8;
	int parity = 'n';
	int flow = 'n';

	if ( co->index == -1 || co->index > 2 ) {
		co->index = 0;
	}

	lpc_port = lpc_ports[co->index];
	if ( lpc_port == NULL )
		return -ENODEV;

	port = &lpc_port->port;

	if ( options )
		uart_parse_options (options, &baud, &parity, &bits, &flow);

	return uart_set_options (port, co, baud, parity, bits, flow);
}


static struct uart_ops lpc_pops = {
	.tx_empty       = lpc_tx_empty,
	.set_mctrl      = lpc_set_mctrl,
	.get_mctrl      = lpc_get_mctrl,
	.stop_tx        = lpc_stop_tx,
	.start_tx       = lpc_start_tx,
	.stop_rx        = lpc_stop_rx,
	.enable_ms      = lpc_enable_ms,
	.break_ctl      = lpc_break_ctl,
	.startup        = lpc_startup,
	.shutdown       = lpc_shutdown,
	.flush_buffer   = lpc_flush_buffer,
	.set_termios    = lpc_set_termios,
	.type           = lpc_type,
	.config_port    = lpc_config_port,
	.verify_port    = lpc_verify_port,
};



static struct uart_driver lpc_reg;
static struct console lpc_console = {
	.name       = DEV_NAME,
	.write      = lpc_console_write,
	.device     = uart_console_device,
	.setup      = lpc_console_setup,
	.flags      = CON_PRINTBUFFER,
	.index      = -1,
	.data       = &lpc_reg,
};


static struct uart_driver lpc_reg = {
	.owner          = THIS_MODULE,
	.driver_name    = DRIVER_NAME,
	.dev_name       = DEV_NAME,
	.major          = 210,
	.minor          = 64,
	.nr             = 2,
	.cons           = &lpc_console,
};


static void setup_from_of (struct device_node *np, struct lpc_serial_port *lpc_port) {
	u32 prop;

	lpc_port->payload_size = 0;   // means infinite size

	if ( of_property_read_u32(np, "device_id", &prop) == 0)
		lpc_port->uart_ldev = prop;

	if ( of_property_read_u32(np, "membase", &prop) == 0)
		lpc_port->membase = prop;

	if ( of_property_read_u32(np, "uart-clk", &prop) == 0) {
		lpc_port->port.uartclk = prop;
		lpc_port->uart_clk = prop;
	}
		
	if ( of_property_read_u32(np, "reg-shift", &prop) == 0)
		lpc_port->port.regshift = prop;

	if ( of_property_read_u32(np, "fifo-size", &prop) == 0)
		lpc_port->port.fifosize = prop;

	if ( of_property_read_u32(np, "pl-size", &prop) == 0)
		lpc_port->payload_size = prop;

	lpc_port->is_rs485 = of_property_read_bool(np, "rs485");
	if ( lpc_port->is_rs485 ) {
		SIO_UART_INFO ("UART#%d used as RS485", lpc_port->uart_index);
	}

	lpc_port->is_half_duplex = of_property_read_bool(np, "half-duplex");
	if ( lpc_port->is_half_duplex ) {
		SIO_UART_INFO ("UART#%d used in half duplex mode", lpc_port->uart_index);
	}

	lpc_port->port.mapbase = (resource_size_t)lpc_port->membase;

	lpc_port->port.irq = irq_of_parse_and_map (np, 0);
}


static int lpc_serial_probe (struct platform_device *pdev) {
	const struct of_device_id *match;
	struct device_node *np = pdev->dev.of_node;
	struct lpc_serial_port *lpc_port;
	struct serial_data *data;
	int ret, err;


	match = of_match_device(of_platform_serial_table, &pdev->dev);
	if (!match)
		return -EINVAL;

	lpc_port = kzalloc (sizeof (struct lpc_serial_port), GFP_KERNEL);
	if ( !lpc_port ) {
		SIO_UART_ERR ("cannot allocate memory for structure data");
		err = -ENOMEM;
		goto err_data_allocate;
	}

	lpc_port->sio_ops = (struct sio_w83627_ops *)dev_get_platdata(&pdev->dev);
	if ( !lpc_port->sio_ops ) {
		SIO_UART_ERR ("cannot obtain ops structure data");
		err = -EINVAL;
		goto err_ops_data;
	}
	if ( !lpc_port->sio_ops->read || !lpc_port->sio_ops->write ||
			!lpc_port->sio_ops->g_read || !lpc_port->sio_ops->g_write) {
		SIO_UART_ERR ("write and read operation not assigned");
   		err= -EINVAL;
		goto err_ops_data;
	}

	data = (struct serial_data *)match->data;
	if ( !data ) {
		SIO_UART_ERR ("cannot obtain serial data");
		err = -EINVAL;
		goto err_serial_data;
	}

	lpc_port->port_type  = data->port_type;
	lpc_port->uart_index = data->uart_index;
	lpc_port->port.line  = data->uart_index;

	setup_from_of (np, lpc_port);

	platform_set_drvdata (pdev, lpc_port);

	switch ( data->port_type ) {
		case LPC_PORT_TYPE_W83627:
			ret = lpc_w83627_platform_serial_setup (pdev, lpc_port);
			break;
		case LPC_PORT_TYPE_XR28V382:
			ret = lpc_xr28v382_platform_serial_setup (pdev, lpc_port);
			break;
		default:
			ret = -1;
	}
	if ( ret ) {
		SIO_UART_ERR ("error while setting UART");
		err = ret;
		goto err_uart_setting;
	}

	lpc_port->port.dev    = &pdev->dev;
	lpc_port->port.iotype = UPIO_LPC;
	lpc_port->port.type   = PORT_LPC;
	lpc_port->port.ops    = &lpc_pops;
	lpc_port->port.flags  = UPF_BOOT_AUTOCONF;

	ret = lpc_w83627_sys_serial_setup (pdev, lpc_port);
	if ( ret ) {
		SIO_UART_ERR ("cannot create sysfs interface");
		err = ret;
		goto err_sysfs;
	}

	lpc_ports[lpc_port->port.line] = lpc_port;
	lpc_port->tx_mode = 0;

	tasklet_init (&lpc_port->tasklet, work_tx, (unsigned long) lpc_port);

	ret = uart_add_one_port (&lpc_reg, &lpc_port->port);

	SIO_UART_INFO ("UART#%d LPC SuperIO driver probed!!!", lpc_port->uart_index);

	return 0;
err_sysfs:
err_uart_setting:
err_serial_data:
err_ops_data:
err_data_allocate:
	return err;
}


static int lpc_serial_remove (struct platform_device *pdev) {
	struct lpc_serial_port *lpc_port = (struct lpc_serial_port *)
											platform_get_drvdata (pdev);

	return uart_remove_one_port (&lpc_reg, &lpc_port->port);
}


static struct serial_data w83627dhg_A_16550a = { LPC_UART_TYPE_W83627_A, LPC_PORT_TYPE_W83627};
static struct serial_data w83627dhg_B_16550a = { LPC_UART_TYPE_W83627_B, LPC_PORT_TYPE_W83627};
static struct serial_data xr28v382_A_16550a = { LPC_UART_TYPE_XR28V382_A, LPC_PORT_TYPE_XR28V382};
static struct serial_data xr28v382_B_16550a = { LPC_UART_TYPE_XR28V382_B, LPC_PORT_TYPE_XR28V382};


static struct of_device_id of_platform_serial_table[] = {
	{ .compatible = "nuvoton,w83627dhg_A_16550a", .data = &w83627dhg_A_16550a, },
	{ .compatible = "nuvoton,w83627dhg_B_16550a", .data = &w83627dhg_B_16550a, },
	{ .compatible = "MaxLinear,xr28v382_A_16550a", .data = &xr28v382_A_16550a, },
	{ .compatible = "MaxLinear,xr28v382_B_16550a", .data = &xr28v382_B_16550a, },
	{ /* end of list */ },
};


static struct platform_driver serial_lpc_driver = {
	.probe		= lpc_serial_probe,
	.remove		= lpc_serial_remove,
	.suspend	= NULL,
	.resume		= NULL,
	.driver		= {
		.name	        = "lpc_serial",
		.owner	        = THIS_MODULE,
		.of_match_table = of_platform_serial_table,
	},
};


static int __init lpc_serial_init (void) {

	int ret;

	pr_info ("Serial: LPC driver");

	ret = uart_register_driver (&lpc_reg);
	if ( ret )
		return ret;

	ret = platform_driver_register (&serial_lpc_driver);
	if ( ret != 0 )
		uart_unregister_driver (&lpc_reg);

	return ret;
}


static void __exit lpc_serial_exit (void) {
	platform_driver_unregister (&serial_lpc_driver);
	uart_unregister_driver (&lpc_reg);
}


module_init (lpc_serial_init);
module_exit (lpc_serial_exit);


MODULE_AUTHOR("Davide Cardillo, SECO srl");
MODULE_DESCRIPTION("SuperIO LPC UART interface");
MODULE_LICENSE("GPL");

#ifndef _LINUX_SIO_LPC_SERIAL_H_
#define _LINUX_SIO_LPC_SERIAL_H_


#include <linux/serial_core.h>
#include <linux/serial.h>
#include <linux/sio-w83627.h>


struct lpc_serial_port {
	struct uart_port        port;
	int                     type;
	struct sio_w83627_ops   *sio_ops;
	int                     uart_ldev;
	int                     uart_index;
	int                     uart_reg[8];
	int                     payload_size;
	unsigned char           rx_lsr;
	int                     boudrate;
	int                     is_rs485;
	int                     is_half_duplex;
	int                     tx_mode;

	struct tasklet_struct   tasklet;
	struct delayed_work     work_tx;
	struct delayed_work     work_rx;

	struct clk *clk;
	int line;
	unsigned int            memoffset;
	int                     membase;
	unsigned int            uart_clk;
	int                     port_type;

//	struct uart_8250_port   port8250;

	unsigned int    (*serial_in)(struct uart_port *, int);
	void            (*serial_out)(struct uart_port *, int, int);

};



struct serial_data {
	int    uart_index;
	int    port_type;
};


#define LPC_UART_TYPE_W83627_A     0
#define LPC_UART_TYPE_W83627_B     1
#define LPC_UART_TYPE_XR28V382_A   0
#define LPC_UART_TYPE_XR28V382_B   1


#define LPC_PORT_TYPE_W83627     0
#define LPC_PORT_TYPE_XR28V382   1


extern int lpc_w83627_platform_serial_setup (struct platform_device *pdev, struct lpc_serial_port *info);
extern int lpc_xr28v382_platform_serial_setup (struct platform_device *pdev, struct lpc_serial_port *info);

extern int lpc_w83627_sys_serial_setup (struct platform_device *pdev, struct lpc_serial_port *info);
extern int lpc_xr28v382_sys_serial_setup (struct platform_device *pdev, struct lpc_serial_port *info);



#endif /* _LINUX_SIO_LPC_SERIAL_H_ */

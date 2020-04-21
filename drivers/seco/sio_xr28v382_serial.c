#include <linux/module.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/serial_core.h>
#include <linux/serial_reg.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/clk.h>
#include <linux/sio-w83627.h>
#include <linux/seco_cpld.h>
#include <linux/seco_lpc.h>

#include <linux/sio_lpc_serial.h>
#include "sio_xr28v382_reg.h"



#define sio_uart_read(sio_uart, addr, data)    \
	(sio_uart)->sio_ops->read (addr, data, (sio_uart)->uart_ldev)

#define sio_uart_write(sio_uart, addr, data)    \
	(sio_uart)->sio_ops->write (addr, data, (sio_uart)->uart_ldev)

#define sio_conf_reg(sio_uart, reg)            \
	(sio_uart)->conf_reg_map[(reg)]



/* __________________________________________________________________________
* |                                                                          |
* |                             XR28V382 UART INIT                           |
* |__________________________________________________________________________|
*/
static void xr28v382_uart_A_init (struct lpc_serial_port *info) {
	uint16_t data;
	uint16_t addr;

	sio_uart_write (info, REG_UART_BADDR_H, (info->membase >> 8) & 0xFF );
	sio_uart_write (info, REG_UART_BADDR_L, info->membase & 0xFF );

	sio_uart_write (info, REG_UART_FIFO_MOD_SEL, RX_TRIGGER_LEVEL_X1 | 
	 		FIFO_TXRX_SIZE_16 |
	 		TX_HOLDING_NO_DELAY);


	addr = (uint16_t)(info->membase ) & 0xFFFF;
	lpc_readw (addr + 3, &data);
	data |= 0x80;
	lpc_writew (addr + 3, data);

	lpc_writew (addr + 0, 0x01);
	lpc_writew (addr + 1, 0x00);

	data &= 0x7F;
	lpc_writew (addr + 3, data);

	/*  Enable UARTA  */
	sio_uart_write (info, REG_UART_EN, UART_ENABLE);
}


static void xr28v382_uart_B_init (struct lpc_serial_port *info) {
	uint16_t data;
	uint16_t addr;

	sio_uart_write (info, REG_UART_BADDR_H, (info->membase >> 8) & 0xFF );
	sio_uart_write (info, REG_UART_BADDR_L, info->membase & 0xFF );

	sio_uart_write (info, REG_UART_FIFO_MOD_SEL, RX_TRIGGER_LEVEL_X1 | 
	 		FIFO_TXRX_SIZE_16 |
	 		TX_HOLDING_NO_DELAY);


	addr = (uint16_t)(info->membase ) & 0xFFFF;
	lpc_readw (addr + 3, &data);
	data |= 0x80;
	lpc_writew (addr + 3, data);

	lpc_writew (addr + 0, 0x01);
	lpc_writew (addr + 1, 0x00);

	data &= 0x7F;
	lpc_writew (addr + 3, data);

	/*  Enable UARTB  */
	sio_uart_write (info, REG_UART_EN, UART_ENABLE);
}


/* __________________________________________________________________________
* |                                                                          |
* |                       XR28V382 UART COMMUNICATION                        |
* |__________________________________________________________________________|
*/
unsigned int xr28v382_serial_in (struct uart_port *port, int offset) {
	uint16_t data;
	uint16_t addr = (uint16_t)(port->mapbase ) & 0xFFFF;
	lpc_readw (addr + offset, &data);
	return (int)data;
}


void xr28v382_serial_out (struct uart_port *port, int offset, int value) {
	uint16_t addr = (uint16_t)(port->mapbase ) & 0xFFFF;
	lpc_writew (addr + offset, value);
}
/* __________________________________________________________________________
* |__________________________________________________________________________|
*/

/* __________________________________________________________________________
* |                                                                          |
* |                              SYSFS INTERFACE                             |
* |__________________________________________________________________________|
*/
static ssize_t sys_xr28v382_dump_uart_regs (struct device *dev, struct device_attribute *attr,
	   								char *buf)
{
	struct lpc_serial_port *info = (struct lpc_serial_port *)dev_get_drvdata (dev);
	unsigned char msg[500];
	unsigned char tmp[50];
	uint8_t value;
	int i;

	sprintf (msg, "\n#\tvalue");
	for ( i = 0 ; i <= 7 ; i++) {
		value = (uint16_t)xr28v382_serial_in (&info->port, i);
		sprintf (tmp, "\n0x%02X\t0x%02X", i, value);
		strcat (msg, tmp);
	}

	return sprintf (buf, "%s\n", msg);
}


static ssize_t sys_xr28v382_store_uart_regs (struct device *dev, struct device_attribute *attr,
									const char *buf, size_t count)
{
	struct lpc_serial_port *info = (struct lpc_serial_port *)dev_get_drvdata (dev);
	uint8_t reg, value;
	unsigned long val;
	char *start = (char *)buf;

	while (*start == ' ')
		start++;

	reg = (uint8_t)simple_strtoul (start, &start, 16);

	if ( reg < 0 || reg > 7 ) {
		return -EINVAL;
	}

	while (*start == ' ')
		start++;

	if ( kstrtoul (start, 16, &val) ) {
		return -EINVAL;
	}

	value = (uint8_t)val;

	xr28v382_serial_out (&info->port, reg, value);

	return count;
}


static DEVICE_ATTR(dump_uart_regs, 0664, sys_xr28v382_dump_uart_regs, sys_xr28v382_store_uart_regs);


static ssize_t sys_xr28v382_payload_size_show (struct device *dev, struct device_attribute *attr,
	   								char *buf)
{
	struct lpc_serial_port *info = (struct lpc_serial_port *)dev_get_drvdata (dev);

	return sprintf (buf, "%d\n", info->payload_size);
}


static DEVICE_ATTR(payload_size, 0444, sys_xr28v382_payload_size_show, NULL);


static struct attribute *xr28v382_uart_attrs[] = {
	&dev_attr_dump_uart_regs.attr,
	&dev_attr_payload_size.attr,
	NULL,
};


static struct attribute_group xr28v382_uart_attr_group = {
	.attrs = xr28v382_uart_attrs,
};

int lpc_xr28v382_sys_serial_setup (struct platform_device *pdev, struct lpc_serial_port *info) {
int err = 0;
	switch ( info->uart_index ) {
		case LPC_UART_TYPE_XR28V382_A:
		case LPC_UART_TYPE_XR28V382_B:
			err = sysfs_create_group (&pdev->dev.kobj, &xr28v382_uart_attr_group);
			break;
		default:
			break;
	}
	return err;
}

/* __________________________________________________________________________
* |__________________________________________________________________________|
*/


int lpc_xr28v382_platform_serial_setup (struct platform_device *pdev,
		//	int type, struct uart_port *port,
			struct lpc_serial_port *info)
{
	int ret = 0;

	switch ( info->uart_index ) {
		case LPC_UART_TYPE_XR28V382_A:
			xr28v382_uart_A_init (info);
			break;
		case LPC_UART_TYPE_XR28V382_B:
			xr28v382_uart_B_init (info);
			break;
		default:
			ret = -1;
	}

	if ( !ret) {
		switch ( info->uart_index ) {
			case LPC_UART_TYPE_XR28V382_A:
			case LPC_UART_TYPE_XR28V382_B:
				info->serial_in = xr28v382_serial_in;
				info->serial_out = xr28v382_serial_out;
				info->type = LPC_PORT_TYPE_XR28V382;
				break;
			default:
				break;
		}
	}

	return ret;
}
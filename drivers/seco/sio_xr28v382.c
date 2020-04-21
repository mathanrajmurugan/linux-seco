#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <asm/byteorder.h>
#include <linux/sysctl.h>
#include <linux/interrupt.h>
#include <linux/of_irq.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/seco_lpc.h>

#include <linux/sio-xr28v382.h>
#include "sio_xr28v382_reg.h"


#define SIO_INFO(fmt, arg...) printk(KERN_INFO "XR28V382: " fmt "\n" , ## arg)
#define SIO_ERR(fmt, arg...)  printk(KERN_ERR "%s: " fmt "\n" , __func__ , ## arg)
#define SIO_DBG(fmt, arg...)  pr_debug("%s: " fmt "\n" , __func__ , ## arg)

#define DRV_VERSION     "1.0"

#define XR_DEBUG 0




 



// struct xr28v382_device {
// 	int                       num_devices;
// 	struct platform_device    **pdevices;
// 	enum W83627DHG_P_DEVICE   *devices_type;
// 	struct lpc_device         *lpc_dev;
// };


// struct xr28v382_device *xr28v382;



static uint8_t reg_global_addr[] = {
	REG_GBL_SREST, REG_GBL_LDN , REG_GBL_DEV_ID_M,
	REG_GBL_DEV_ID_L, REG_GBL_VID_M, REG_GBL_VID_L,
	REG_GBL_CLKSEL, REG_GBL_WDT
};


static uint8_t reg_uartA_addr[] = {
	REG_UART_EN, REG_UART_BADDR_H, REG_UART_BADDR_L,
	REG_UART_IRQ_CH_SEL, REG_UART_ENH_MULTIFUN, REG_UART_IRC,
	REG_UART_S_ADDR_MODE, REG_UART_S_ADDR_MODE_MASK, REG_UART_FIFO_MOD_SEL
};


static uint8_t reg_uartB_addr[] = {
	REG_UART_EN, REG_UART_BADDR_H, REG_UART_BADDR_L,
	REG_UART_IRQ_CH_SEL, REG_UART_ENH_MULTIFUN, REG_UART_IRC,
	REG_UART_S_ADDR_MODE, REG_UART_S_ADDR_MODE_MASK, REG_UART_FIFO_MOD_SEL
};




/* __________________________________________________________________________
* |                                                                          |
* |                              BASIC FUNCTIONS                             |
* |__________________________________________________________________________|
*/

static inline void superio_outb (uint8_t addr, uint8_t value) {
	uint16_t val16 = (uint16_t)value;
	uint16_t addr16 = (uint16_t)addr;
	lpc_writew (REG_EFER, addr16);
	lpc_writew (REG_EFDR, val16);
}


static inline void superio_inb (uint8_t addr, uint8_t *data) {
	uint16_t val16 = 0;
	uint16_t addr16 = (uint16_t)addr;
	lpc_writew (REG_EFER, addr16);
	lpc_readw (REG_EFDR, &val16);
	*data = (uint8_t)(val16);
}


int superio_readw (unsigned long addr, int offset) {
	uint16_t data;
	lpc_readw (((addr & 0xFFFF) >> 1) + offset, &data);
	return (int)data;
}


void superio_writew (unsigned long addr, int offset, int value) {
	lpc_writew (((addr & 0xFFFF) >> 1) + (unsigned long)offset, (int)value);
}

static inline void superio_enter_ext_mode (void) {
	lpc_writew (REG_EFER, ENTER_EXT_MODE_CODE);
	lpc_writew (REG_EFER, ENTER_EXT_MODE_CODE);
}


static inline void superio_exit_ext_mode (void) {
	lpc_writew (REG_EFER, EXIT_EXT_MODE_CODE);
}


static inline void superio_sw_reset (void) {
	uint8_t data;
	superio_inb (REG_GBL_SREST, &data);
	superio_outb (REG_GBL_SREST, data | MASK_SRST);
}
/* __________________________________________________________________________
* |__________________________________________________________________________|
*/
static uint16_t getID (uint8_t id_h, uint8_t id_l) {
	uint8_t tmp = 0x0000;
	uint8_t rev = 0x0000;

	superio_inb (id_h, &tmp);
	superio_inb (id_l, &rev);

	return (uint16_t)(rev | tmp << 8);
}


static inline uint16_t getDeviceID (void) {
	return getID (REG_GBL_DEV_ID_M, REG_GBL_DEV_ID_L);
}


static inline uint16_t getVendorID (void) {
	return getID (REG_GBL_VID_M, REG_GBL_VID_L);
}


static inline int chipIDvalidate (uint16_t dev_id, uint16_t ven_id) {
	return dev_id == ((DFL_DEV_ID_M << 8) | DFL_DEV_ID_L) &&
		ven_id == ((DFL_VEN_ID_M << 8) | DFL_VEN_ID_L) ? 1 : 0;
}


static int logicalDeviceValidate (enum XR28V382_DEVICE  dev) {
	int isValid = 0;
	switch (dev) {
		case UARTA:
		case UARTB:
			isValid = 1;
			break;
		default:
			isValid = 0;
	}
	return isValid;
}


static inline int superio_select (enum XR28V382_DEVICE id) {
	int ret = -1;
	if (logicalDeviceValidate (id)) {
		superio_outb (REG_GBL_LDN, (uint8_t)id);
		ret = (int)id;
	} else {
		ret = -1;
	}
	return ret;
}


/* __________________________________________________________________________
* |__________________________________________________________________________|
*/
static void superio_uart_read (uint8_t addr, uint8_t *data, uint8_t ldev_id) {
	superio_enter_ext_mode ();
	superio_select (ldev_id);
	superio_inb (addr, data);
	superio_exit_ext_mode ();
}


static void superio_uart_write (uint8_t addr, uint8_t value, uint8_t ldev_id) {
	superio_enter_ext_mode ();
	superio_select (ldev_id);
	superio_outb (addr, value);
	superio_exit_ext_mode ();
}


static void superio_global_read (uint8_t addr, uint8_t *data) {
	superio_enter_ext_mode ();
	superio_inb (addr, data);
	superio_exit_ext_mode ();
}


static void superio_global_write (uint8_t addr, uint8_t value) {
	superio_enter_ext_mode ();
	superio_outb (addr, value);
	superio_exit_ext_mode ();
}


static struct sio_xr28v382_ops uart_ops = {
	.read    = superio_uart_read,
	.write   = superio_uart_write,
	.g_read  = superio_global_read,
	.g_write = superio_global_write
};


static struct of_dev_auxdata sio_auxdata_lookup[] = {
	OF_DEV_AUXDATA("MaxLinear,xr28v382_A_16550a", 0, "sio_uart_a_xr28v382", &uart_ops),
	OF_DEV_AUXDATA("MaxLinear,xr28v382_B_16550a", 0, "sio_uart_b_xr28v382", &uart_ops),
	{ /*  sentinel */ },
};


static int superio_xr28v382_driver_allocate (struct device *parent_dev, struct device_node *dp) {
	int status;

	status = of_platform_populate (dp,  NULL, sio_auxdata_lookup, parent_dev);
	if ( status ) {
		SIO_ERR ("failed to create device");
	} else {
		SIO_INFO ("superio device created");
	}

	return status;
}











//
//
//////////////////////////////////////////////////////////////////
////                        UART FUNCTIONS                      //
//////////////////////////////////////////////////////////////////
//
// static void uart_a_irq_enable (void) {
// 	irq_slot_set (SLOT_UART_A, 1);
// }


// static void uart_a_irq_disable (void) {
// 	irq_slot_set (SLOT_UART_A, 0);
// }


// static void uart_b_irq_enable (void) {
// 	irq_slot_set (SLOT_UART_B, 1);
// }


// static void uart_b_irq_disable (void) {
// 	irq_slot_set (SLOT_UART_B, 0);
// }


// static void irq_after_handler (int irq, void *dev_id) {
// 	int value;
// 	struct plat_serial8250_port *pdata = ((struct device *)dev_id)->platform_data;
// 	if (pdata != NULL) {
// 		value = superio_readw (pdata[0].mapbase, 0x4);
// 		value |= 0x08;
// 		superio_writew (pdata[0].mapbase, 0x4, value);
// 		udelay (10);
// 	//	value &= 0xF7;
// 	//	superio_writew (pdata[0].mapbase, 0x4, value);
// 	}
// }


// static struct plat_serial8250_port xr28v382_uart_a_platform_data[] = {
// 	{
// 		.mapbase               = (SUPERIO_UARTA_BASE << 1),
// 		.flags                 = UPF_BOOT_AUTOCONF | UPF_SKIP_TEST,// | UPF_IOREMAP,
// 		.iotype                = UPIO_LPC,
// 		.regshift              = 0,
// 		.uartclk               = XR28V382_UART_CLOCK,
// 		.irq_management        = NULL,	// initialized by driver 8250
// 		.irq_after_management  = NULL, //irq_after_handler,
// 		.irq_data              = NULL,	// initialized by driver 8250
// 		.irq_enable            = uart_a_irq_enable,
// 		.irq_disable           = uart_a_irq_disable,
// 	}, 
// 	{
// 		.flags = 0,
// 	},
// };


// static struct plat_serial8250_port xr28v382_uart_b_platform_data[] = {
// 	{
// 		.mapbase               = (SUPERIO_UARTB_BASE << 1),
// 		.flags                 = UPF_BOOT_AUTOCONF | UPF_SKIP_TEST,// | UPF_IOREMAP,
// 		.iotype                = UPIO_LPC,
// 		.regshift              = 0,
// 		.uartclk               = XR28V382_UART_CLOCK,
// 		.irq_management        = NULL,	// initialized by driver 8250
// 		.irq_after_management  = NULL,  //irq_after_handler,
// 		.irq_data              = NULL,	// initialized by driver 8250
// 		.irq_enable            = uart_b_irq_enable,
// 		.irq_disable           = uart_b_irq_disable,


// 	},
// 	{
// 		.flags = 0,
// 	},
// };


// static struct platform_device xr28v382_uart_a_device = {
// 	.name = "serial8250",
// 	.id   = 0,
// 	.dev = {
// 		.platform_data = &xr28v382_uart_a_platform_data,
// 	},
// };


// static struct platform_device xr28v382_uart_b_device = {
// 	.name = "serial8250",
// 	.id   = 1,
// 	.dev = {
// 		.platform_data = &xr28v382_uart_b_platform_data,
// 	},
// };




#if XR_DEBUG
static void uarta_stamp (void) {
	uint16_t data;

	superio_enter_ext_mode ();

	superio_select (DEVICE_SEL_UARTA);
	
	superio_inb (REG_UART_EN, &data);
	printk (KERN_INFO "XR28V382  REG_UART_EN   %#X\n", data);

	superio_inb (REG_UART_BADDR_H, &data);
	printk (KERN_INFO "XR28V382  REG_UART_BADDR_H   %#X\n", data);

	superio_inb (REG_UART_BADDR_L, &data);
	printk (KERN_INFO "XR28V382  REG_UART_BADDR_L   %#X\n", data);
	
	superio_inb (REG_UART_IRQ_CH_SEL, &data);
	printk (KERN_INFO "XR28V382  REG_UART_IRQ_CH_SEL   %#X\n", data);
	
	superio_inb (REG_UART_ENH_MULTIFUN, &data);
	printk (KERN_INFO "XR28V382  REG_UART_ENH_MULTIFUN   %#X\n", data);
	
	superio_inb (REG_UART_IRC, &data);
	printk (KERN_INFO "XR28V382  REG_UART_IRC   %#X\n", data);
	
	superio_inb (REG_UART_S_ADDR_MODE, &data);
	printk (KERN_INFO "XR28V382  REG_UART_S_ADDR_MODE   %#X\n", data);
	
	superio_inb (REG_UART_S_ADDR_MODE_MASK, &data);
	printk (KERN_INFO "XR28V382  REG_UART_S_ADDR_MODE_MASK   %#X\n", data);
	
	superio_inb (REG_UART_FIFO_MOD_SEL, &data);
	printk (KERN_INFO "XR28V382  REG_UART_FIFO_MOD_SEL   %#X\n", data);

	superio_exit_ext_mode ();
}


static void uartb_stamp (void) {
	uint16_t data;

	superio_enter_ext_mode ();

	superio_select (DEVICE_SEL_UARTB);
	
	superio_inb (REG_UART_EN, &data);
	printk (KERN_INFO "XR28V382  REG_UART_EN   %#X\n", data);

	superio_inb (REG_UART_BADDR_H, &data);
	printk (KERN_INFO "XR28V382  REG_UART_BADDR_H   %#X\n", data);

	superio_inb (REG_UART_BADDR_L, &data);
	printk (KERN_INFO "XR28V382  REG_UART_BADDR_L   %#X\n", data);
	
	superio_inb (REG_UART_IRQ_CH_SEL, &data);
	printk (KERN_INFO "XR28V382  REG_UART_IRQ_CH_SEL   %#X\n", data);
	
	superio_inb (REG_UART_ENH_MULTIFUN, &data);
	printk (KERN_INFO "XR28V382  REG_UART_ENH_MULTIFUN   %#X\n", data);
	
	superio_inb (REG_UART_IRC, &data);
	printk (KERN_INFO "XR28V382  REG_UART_IRC   %#X\n", data);
	
	superio_inb (REG_UART_S_ADDR_MODE, &data);
	printk (KERN_INFO "XR28V382  REG_UART_S_ADDR_MODE   %#X\n", data);
	
	superio_inb (REG_UART_S_ADDR_MODE_MASK, &data);
	printk (KERN_INFO "XR28V382  REG_UART_S_ADDR_MODE_MASK   %#X\n", data);
	
	superio_inb (REG_UART_FIFO_MOD_SEL, &data);
	printk (KERN_INFO "XR28V382  REG_UART_FIFO_MOD_SEL   %#X\n", data);

	superio_exit_ext_mode ();

}



static ssize_t uarta_state_show (struct device *dev,
		struct device_attribute *attr, char *buf) {
	int i;
	ssize_t status;
	unsigned char msg[500];
	unsigned char tmp[50];

	uint16_t data;
	uint16_t reg= (SUPERIO_UARTA_BASE << 1);

	for (i = 0 ; i < 7 ; i++) {
//		superio_outb (reg, &data);
		data = superio_readw (reg, i);
		sprintf (tmp, "%#02X : %#02X\n", ((reg & 0xFFFF) >> 1) + i, data);
		strcat (msg, tmp);
	}

//	superio_inb (0x03f8, 0xC);

	status = sprintf (buf, "%s\n", msg);
	return status;
}

static ssize_t uarta_reg_store (struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count) {

	int rc;
	unsigned long data;

	rc = strict_strtoul (buf, 0, &data);

	if (rc)
		return rc;

	superio_writew ((SUPERIO_UARTA_BASE << 1), 
			data & 0x0000F, 
			(uint16_t)((data >> 4) & 0xFFFF));


	return rc;	
}




static DEVICE_ATTR (uarta_state, 0666, uarta_state_show, uarta_reg_store);


static ssize_t uartb_state_show (struct device *dev,
		struct device_attribute *attr, char *buf) {
	int i;
	ssize_t status;
	unsigned char msg[500];
	unsigned char tmp[50];
	uint16_t data;
	uint16_t reg= (SUPERIO_UARTB_BASE << 1);
	for (i = 0 ; i < 7 ; i++) {
		data = superio_readw (reg, i);
		sprintf (tmp, "%#02X : %#02X\n", ((reg & 0xFFFF) >> 1) + i, data);
		strcat (msg, tmp);
	}
	status = sprintf (buf, "%s\n", msg);
	return status;
}

static ssize_t uartb_reg_store (struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count) {

	int rc;
	unsigned long data;

	rc = strict_strtoul (buf, 0, &data);

	if (rc)
		return rc;
	superio_writew ((unsigned long)(SUPERIO_UARTB_BASE << 1), 
			data & 0x0000F, 
			(int)((data >> 4) & 0xFFFF));

	rc = count;
	return rc;	
}


static DEVICE_ATTR (uartb_state, 0666, uartb_state_show, uartb_reg_store);


#endif





/* __________________________________________________________________________
* |                                                                          |
* |                              SYSFS INTERFACE                             |
* |__________________________________________________________________________|
*/
static ssize_t sys_superio_revision (struct device *dev, struct device_attribute *attr,
	   								char *buf)
{
	uint16_t dev_id, ven_id;

	superio_enter_ext_mode ();
	dev_id = getDeviceID ();
	ven_id = getVendorID ();
	superio_exit_ext_mode ();

	return sprintf(buf, "vendorID: 0x%04X    deviceID: 0x%04X\n",ven_id, dev_id);
}

static DEVICE_ATTR(superio_ver, S_IRUGO, sys_superio_revision, NULL);



static ssize_t sys_dump_regs (const uint8_t *reg_array, int a_size, char *buf, uint8_t ldev) {
	unsigned char msg[500];
	unsigned char tmp[50];
	uint8_t value;
	int i;

	superio_enter_ext_mode ();
	if ( ldev < 0xD ) {
		superio_select (ldev);
	}
	sprintf (msg, "\n#\tvalue");
	for ( i = 0 ; i < a_size ; i++ ) {
		superio_inb (reg_array[i], &value);
		sprintf (tmp, "\n0x%02X\t0x%02X", reg_array[i], value);
		strcat (msg, tmp);
	}
	superio_exit_ext_mode ();

	return sprintf (buf, "%s\n", msg);
}


static ssize_t sys_superio_dump_global_regs (struct device *dev, struct device_attribute *attr,
	   								char *buf)
{
	return sys_dump_regs (reg_global_addr, ARRAY_SIZE (reg_global_addr), buf, (uint8_t)-1);
}

static DEVICE_ATTR(dump_global_regs, S_IRUGO, sys_superio_dump_global_regs, NULL);


static ssize_t sys_superio_dump_uartA_regs (struct device *dev, struct device_attribute *attr,
	   								char *buf)
{
	return sys_dump_regs (reg_uartA_addr, ARRAY_SIZE (reg_uartA_addr), buf, (uint8_t)DEVICE_SEL_UARTA);
}

static DEVICE_ATTR(dump_uartA_regs, S_IRUGO, sys_superio_dump_uartA_regs, NULL);


static ssize_t sys_superio_dump_uartB_regs (struct device *dev, struct device_attribute *attr,
	   								char *buf)
{
	return sys_dump_regs (reg_uartB_addr, ARRAY_SIZE (reg_uartB_addr), buf, (uint8_t)DEVICE_SEL_UARTB);
}

static DEVICE_ATTR(dump_uartB_regs, S_IRUGO, sys_superio_dump_uartB_regs, NULL);





static struct attribute *superio_attrs[] = {
	&dev_attr_superio_ver.attr,
	&dev_attr_dump_global_regs.attr,
	&dev_attr_dump_uartA_regs.attr,
	&dev_attr_dump_uartB_regs.attr,
	NULL,
};

static struct attribute_group superio_attr_group = {
	.attrs = superio_attrs,
};

/* __________________________________________________________________________
* |__________________________________________________________________________|
*/
static int superio_xr28v382_probe (struct platform_device *pdev) {
	struct device_node *dp = pdev->dev.of_node;
	uint16_t dev_id, ven_id;
	int ret, err;
	// struct seco_xr28v382_platform_data *pdata;
	// struct lpc_device *lpc_dev;
	// int i;
	// enum XR28V382_DEVICE *dev_list;

	// struct device_node *dp = pdev->dev.of_node;
	// int err = 0;
	// uint16_t rev = 0;


	// pdata = pdev->dev.platform_data;
	// if (pdata == NULL) {
	// 	XR_ERR ("no platform_data!");
	// 	return -EINVAL;
	// }

	// xr28v382 = kzalloc (sizeof(struct xr28v382_device), GFP_KERNEL);
	// if (xr28v382 == NULL) {
	// 	XR_ERR ("no device data!");
	// 	err = -EINVAL;
	// 	goto err_device;
	// }

	/*  device software restetting  */
	superio_outb (REG_GBL_SREST, 0x01);
	udelay (10);

	/*  validate chip through device and vendor ID  */
	superio_enter_ext_mode ();

	dev_id = getDeviceID ();
	ven_id = getVendorID ();
	
	ret = chipIDvalidate (dev_id, ven_id);
	if (ret == 1) {
		dev_info (&pdev->dev, "EXAR chip ID: %#04X, vendor ID: %#04X\n", dev_id, ven_id);
	} else {
		dev_err (&pdev->dev, "SuperIO invalid chip ID: %#04X - %#04X\n", dev_id, ven_id);
		superio_exit_ext_mode ();
		err = -EINVAL;
		goto err_rev_inval;
	}

	superio_exit_ext_mode ();


	err = sysfs_create_group (&pdev->dev.kobj, &superio_attr_group);
	if ( err ) {
		SIO_ERR ("cannot create sysfs interface");
		goto err_sysfs;
	}

	err = superio_xr28v382_driver_allocate (&pdev->dev, dp);
	if ( err ) {
		SIO_ERR ("cannot allocat superIO sub-devices: %#X", err);
		goto err_device_allocate;
	}


	SIO_INFO ("LPC SuperIO driver ver %s probed!!!", DRV_VERSION);
	return 0;
err_device_allocate:
	sysfs_remove_group(&pdev->dev.kobj, &superio_attr_group);
err_sysfs:
err_rev_inval:
	return err;
}

// 	/*  add driver for the superio's devices  */
// 	if (pdata->num_devices < 1) {
// 		XR_INFO ("No devices associated to Winbond");
// 		goto dev_err_out;
// 	} else {
// 		xr28v382->num_devices = pdata->num_devices;
// 		xr28v382->pdevices = kzalloc(sizeof(struct platform_device *) * pdata->num_devices, GFP_KERNEL);
// 		xr28v382->lpc_dev = kzalloc(sizeof(struct lpc_device) * pdata->num_devices, GFP_KERNEL);
// 		if  (xr28v382->pdevices == NULL || xr28v382->lpc_dev == NULL) {
// 			XR_ERR ("Error in devices allocation");
// 		} else {
// 			for (i = 0, dev_list = pdata->devices ; i < pdata->num_devices ; i++, dev_list++) {
// 				switch (*dev_list) {
// 					case UARTA:
// 						XR_INFO ("UART_A device selected");
// 						xr28v382_uart_a_platform_data[0].mapbase += lpc_getMemBase();
// 						xr28v382_uart_a_platform_data[0].irq = lpc_getIRQ(0);
// 						xr28v382_uart_a_platform_data[0].irqflags = lpc_getIRQ_flags(0) | IRQF_LPCSTYLE;
// 						uart_a_init ();

// 						xr28v382->pdevices[i] = &xr28v382_uart_a_device;
// 						xr28v382->lpc_dev[i].lpc_slot = 3;
// 						xr28v382->lpc_dev[i].dev = &xr28v382->pdevices[i]->dev;

// 						xr28v382->lpc_dev[i].handler = &xr28v382_uart_a_platform_data[0].irq_management;
// 						xr28v382->lpc_dev[i].after_handler = irq_after_handler;
// 						xr28v382->lpc_dev[i].irq_dev_id = xr28v382_uart_a_platform_data[0].irq_data;
// 						xr28v382->lpc_dev[i].maskable = LPC_IRQ_MASKABLE;

// 						break;
// 					case UARTB:
// 						XR_INFO ("UART_B device selected");
// 						xr28v382_uart_b_platform_data[0].mapbase += lpc_getMemBase();
// 						xr28v382_uart_b_platform_data[0].irq = lpc_getIRQ(0);
// 						xr28v382_uart_b_platform_data[0].irqflags = lpc_getIRQ_flags(0) | IRQF_LPCSTYLE;
// 						uart_b_init ();

// 						xr28v382->pdevices[i] = &xr28v382_uart_b_device;
// 						xr28v382->lpc_dev[i].lpc_slot = 4;
// 						xr28v382->lpc_dev[i].dev = &xr28v382->pdevices[i]->dev;

// 						xr28v382->lpc_dev[i].handler = &xr28v382_uart_b_platform_data[0].irq_management;
// 						xr28v382->lpc_dev[i].after_handler = irq_after_handler;
// 						xr28v382->lpc_dev[i].irq_dev_id = xr28v382_uart_b_platform_data[0].irq_data;
// 						xr28v382->lpc_dev[i].maskable = LPC_IRQ_MASKABLE;
// 						break;

// 					default:
// 						XR_ERR ("Device not supported by SuperIO");
// 				}
// 			}
// 		}
// 	}
// 	superio_exit_ext_mode ();

// 	/*  device registration  */
// 	platform_add_devices (xr28v382->pdevices, xr28v382->num_devices);

// 	lpc_dev = xr28v382->lpc_dev;
// 	for (i = 0 ; i < xr28v382->num_devices ; i++, lpc_dev++) {
// 		if (lpc_dev->lpc_slot >= 0)
// 			lpc_add_device (lpc_dev);
// 	}
	
// #if XR_DEBUG 

// 	uarta_stamp ();
// 	uartb_stamp ();
// 	ret = device_create_file (&pdev->dev, &dev_attr_uarta_state);
// 	ret = device_create_file (&pdev->dev, &dev_attr_uartb_state);
// #endif

// 	return 0;
// probe_failed:
// err_device:
// dev_err_out:
// 	return err;

// }


static int superio_xr28v382_remove (struct platform_device *pdev) {

	platform_device_unregister (pdev); 
	kfree (pdev);

	return 0;
}


static const struct of_device_id seco_superio_match[] = {
	{ .compatible = "MaxLinear,xr28v382" },
	{ /* sentinel */ }
};

MODULE_DEVICE_TABLE(of, seco_superio_match);


static struct platform_driver seco_xr28v382_driver = {
	.driver = {
		.name           = "sio_xr28v382",
		.owner          = THIS_MODULE,
		.of_match_table	= seco_superio_match,
	},
	.probe  = superio_xr28v382_probe,
	.remove = superio_xr28v382_remove,
};



static int __init seco_xr28v382_init (void) {
	return platform_driver_register (&seco_xr28v382_driver);
}

subsys_initcall (seco_xr28v382_init);


static void __exit seco_xr28v382_exit (void) {
	return platform_driver_unregister (&seco_xr28v382_driver);
}

module_exit (seco_xr28v382_exit);


MODULE_AUTHOR("DC SECO");
MODULE_DESCRIPTION("SECO SuperIO XR28V382");
MODULE_LICENSE("GPL");
MODULE_VERSION(DRV_VERSION);
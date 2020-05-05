/*
 * 
 *
 * WDOG Trigger driver
 *
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
//#include <linux/init.h>
#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <asm/uaccess.h>
#include <linux/ioctl.h>
#include <linux/major.h>
#include <linux/cdev.h>
#include <linux/sched.h>
#include <linux/workqueue.h>
#include <linux/device.h>
#include "apx_wdog-trigger.h"
#include <linux/apx_wdt_io.h>
#include <linux/timer.h>
#include <linux/platform_device.h>

#include <linux/init.h>
#include <linux/smp.h>
#include <asm/cacheflush.h>
#include <asm/page.h>
//#include <asm/smp_scu.h>
//#include <asm/mach/map.h>

//#include "common.h"


#include "apx_wdog-trigger.h"
//#include "hardware.h"

#define WDOG_TRIGGER_REFRESH            msecs_to_jiffies(50)		// milliseconds
#define WDOG_TIME                       msecs_to_jiffies(10000)		// 10s



typedef struct wdog_data {
	void __iomem *iomux;
	void __iomem *padctrl;
	void __iomem *base;
	int           num;
} wdog_data_t;


static struct wdog_trigger_data {

	void           		(*refresh_wdt)(void);
	volatile unsigned long	wdt_refresh_time;
	volatile unsigned long	wdt_current_refresh_time;
	volatile int            wdt_refresh;
	int            			wdt_trigger_mode;
	int            			wdt_enable;
	int            			wdt_en_low;
	wdog_data_t    			pin_trg;
	wdog_data_t    			pin_en;
	int            			is_initialized;
	struct timer_list 		wdt_timer;
	struct mutex			ops_lock;
	struct mutex   			ioctl_lock;
	int 				state;
} wdt;


#define WDT_PAD_MUX_GPIO(x)     writel_relaxed(readl_relaxed((x)) | 0x5, (x))

#define WDT_PAD_CTRL_GPIO(x)    writel_relaxed(0x47, (x))

#define WDT_DIR_OUT_GPIO(x,n)   writel_relaxed(readl_relaxed((x) + 0x4) | ( 1 << (n)), (x) + 0x4)

#define WDT_SET_H_GPIO(x,n)     writel_relaxed(readl_relaxed((x)) | (1 << (n)) , (x))

#define WDT_SET_L_GPIO(x,n)     writel_relaxed(readl_relaxed((x)) & ~(1 << (n)) , (x))

static void enable_triggering (void);
static void disable_triggering (void);




/*  __________________________________________________________________________
   |                                                                          |
   |                               BASE FUNCTIONS                             |
   |__________________________________________________________________________|
 */
#define PIN_CONF_EN     apx_wdog_configure_pins ('e');
#define PIN_CONF_DIS    apx_wdog_configure_pins ('d');

static void apx_wdog_configure_pins(char dir) {

	switch ( dir ) {
		case 'e':

			WDT_PAD_MUX_GPIO(wdt.pin_trg.iomux);

			WDT_PAD_CTRL_GPIO(wdt.pin_trg.padctrl);

			WDT_DIR_OUT_GPIO(wdt.pin_trg.base, wdt.pin_trg.num);


			WDT_PAD_MUX_GPIO(wdt.pin_en.iomux);

			WDT_PAD_CTRL_GPIO(wdt.pin_en.padctrl);

			WDT_DIR_OUT_GPIO(wdt.pin_en.base, wdt.pin_en.num);

			if(wdt.wdt_en_low)
				WDT_SET_L_GPIO(wdt.pin_en.base, wdt.pin_en.num);
			else
				WDT_SET_H_GPIO(wdt.pin_en.base, wdt.pin_en.num);
			break;

		case 'd':

			WDT_PAD_MUX_GPIO(wdt.pin_trg.iomux);

			WDT_PAD_CTRL_GPIO(wdt.pin_trg.padctrl);

			WDT_DIR_OUT_GPIO(wdt.pin_trg.base, wdt.pin_trg.num);


			WDT_PAD_MUX_GPIO(wdt.pin_en.iomux);

			WDT_PAD_CTRL_GPIO(wdt.pin_en.padctrl);

			WDT_DIR_OUT_GPIO(wdt.pin_en.base, wdt.pin_en.num);
			if(wdt.wdt_en_low)
				WDT_SET_H_GPIO(wdt.pin_en.base, wdt.pin_en.num);
			else
				WDT_SET_L_GPIO(wdt.pin_en.base, wdt.pin_en.num);

			break;

		default:
			printk(KERN_ERR "apx_wdog: unknown pins direction\n");
		break;
	}

}

static unsigned int last_trigger_time = 0;
void imx6_wdog_refresh (void) {

    if (jiffies_to_msecs(jiffies) - last_trigger_time > 100)
    {
        pr_warn("apx_wdog: mod_timer late wdog refresh  = %imS \n", jiffies_to_msecs(jiffies) - last_trigger_time);
    }
    last_trigger_time = jiffies_to_msecs(jiffies);

	WDT_SET_H_GPIO(wdt.pin_trg.base, wdt.pin_trg.num);

	udelay(1);

	WDT_SET_L_GPIO(wdt.pin_trg.base, wdt.pin_trg.num);

}


/*  __________________________________________________________________________
   |                                                                          |
   |                               WORK FUNCTIONS                             |
   |__________________________________________________________________________|
   */
static void wdog_trigger_work_handler (struct timer_list *t) {


	mutex_lock (&wdt.ops_lock);
	if ( likely(wdt.wdt_enable == APX_WDOG_STATUS_EN) ) { 

		if (wdt.wdt_trigger_mode == APX_WDOG_TRIG_MOD_AUTO) { 

			if ( likely(wdt.refresh_wdt) )
				wdt.refresh_wdt ();

			mod_timer(&wdt.wdt_timer, jiffies + WDOG_TRIGGER_REFRESH);
			mutex_unlock (&wdt.ops_lock);
		}

		if (wdt.wdt_trigger_mode == APX_WDOG_TRIG_MOD_MAN) {

			if(likely(((long)wdt.wdt_refresh_time - (long)(jiffies - wdt.wdt_current_refresh_time)) > 0 )) { 
						
				if (likely(wdt.refresh_wdt) ) 
					wdt.refresh_wdt ();

				mod_timer(&wdt.wdt_timer, jiffies + WDOG_TRIGGER_REFRESH);
				mutex_unlock (&wdt.ops_lock);
			} else
				pr_warn("apx_wdog: refresh time elapsed jiffies=%lu > %lu\n",jiffies,(wdt.wdt_current_refresh_time + wdt.wdt_refresh_time));
		}
	} 
	else
		mutex_unlock (&wdt.ops_lock);

}


static void enable_triggering () {

	mutex_lock (&wdt.ops_lock);
	wdt.wdt_enable = APX_WDOG_STATUS_EN;
	mutex_unlock (&wdt.ops_lock);

	PIN_CONF_EN;
	if ( wdt.refresh_wdt )
		wdt.refresh_wdt ();

	pr_info("apx_wdog: enabled wdog - %u s between refresh\n", jiffies_to_msecs(WDOG_TIME) / 1000 );

	wdt.refresh_wdt();
	
	timer_setup(&wdt.wdt_timer, wdog_trigger_work_handler, 0);
	
	mod_timer(&wdt.wdt_timer, jiffies + WDOG_TRIGGER_REFRESH);

}


static void disable_triggering () {
	mutex_lock (&wdt.ops_lock);
	wdt.wdt_enable = APX_WDOG_STATUS_DIS;
	mutex_unlock (&wdt.ops_lock);

	del_timer(&wdt.wdt_timer);
	PIN_CONF_DIS;
}


/*  __________________________________________________________________________
   |                                                                          |
   |                              SYSFS FUNCTIONS                             |
   |__________________________________________________________________________|
 */


/* Watchdog Enable - Disable */

static ssize_t status_show (struct class *cls, struct class_attribute *attr, char *buf) {

	return sprintf(buf, "%s\n", wdt.wdt_enable == 1 ? "enabled" : "disabled");

}


static ssize_t status_store (struct class *cls, struct class_attribute *attr, const char *buf, size_t count) {
	char *endp;
	unsigned long int enable;
	enable = simple_strtol (buf, &endp, 10);

	if ( enable == 1 ) {
		/* Enable Wdog */
		PIN_CONF_EN;
		enable_triggering ();

	} else if ( enable == 0 ) {
		/* Disable Wdog */
		PIN_CONF_DIS;
		disable_triggering ();

	} else
		return -EINVAL;

	return count;
}


/* Watchdog Triggering Mode */

static ssize_t trigger_mode_show (struct class *cls, struct class_attribute *attr, char *buf) {

	return sprintf(buf, "%s\n", wdt.wdt_trigger_mode == APX_WDOG_TRIG_MOD_AUTO ?
			"automatic" : "manual");

}


static ssize_t trigger_mode_store (struct class *cls, struct class_attribute *attr, const char *buf, size_t count) {
	char *endp;
	unsigned long int trigger_mode;

	trigger_mode = simple_strtol(buf, &endp, 10);

	if (trigger_mode == 1 || trigger_mode == 0) {
		mutex_lock (&wdt.ops_lock);
		wdt.wdt_trigger_mode = trigger_mode;
		mutex_unlock (&wdt.ops_lock);
	} else
		return -EINVAL;

	return count;
}


/* Watchdog Refresh Time */

static ssize_t refresh_time_show (struct class *cls, struct class_attribute *attr, char *buf) {

	return sprintf(buf, "%u msec\n", jiffies_to_msecs(wdt.wdt_refresh_time));
}


static ssize_t refresh_time_store (struct class *cls, struct class_attribute *attr, const char *buf, size_t count) {
	char *endp;
	unsigned long int msec;

	msec = simple_strtol(buf, &endp, 10);

	mutex_lock (&wdt.ops_lock);
	wdt.wdt_refresh_time = msecs_to_jiffies(msec);
	mutex_unlock (&wdt.ops_lock);

	return count;
}


/* Watchdog Residual Time */

static ssize_t residual_time_show (struct class *cls, struct class_attribute *attr, char *buf) {

	long  residual;
	if ( wdt.wdt_trigger_mode == APX_WDOG_TRIG_MOD_MAN )
		residual = (long)wdt.wdt_refresh_time - (long)(jiffies - wdt.wdt_current_refresh_time);
	else
		residual = wdt.wdt_refresh_time;
	return sprintf(buf, "%d msec\n", jiffies_to_msecs(residual));
}



/* Watchdog Refresh */

static ssize_t refresh_store (struct class *cls, struct class_attribute *attr, const char *buf, size_t count) {
	char *endp;
	unsigned long int refresh_trigger;

	refresh_trigger = simple_strtol(buf, &endp, 10);

	mutex_lock (&wdt.ops_lock);
	wdt.wdt_current_refresh_time = jiffies;
	mutex_unlock (&wdt.ops_lock);

	return count;
}

static CLASS_ATTR_RW(status);
static CLASS_ATTR_RW(trigger_mode);
static CLASS_ATTR_RW(refresh_time);
static CLASS_ATTR_RO(residual_time);
static CLASS_ATTR_WO(refresh);


static struct attribute *apx_class_attrs[] = {
	&class_attr_status.attr,
	&class_attr_trigger_mode.attr,
	&class_attr_refresh_time.attr,
	&class_attr_residual_time.attr,
	&class_attr_refresh.attr,
	NULL,
};

ATTRIBUTE_GROUPS(apx_class);

static struct class wdog_drv ={
		.name = "apx_wdog",
		.owner = THIS_MODULE,
		.class_groups = apx_class_groups,
};



/*  __________________________________________________________________________
   |                                                                          |
   |                              IOCTRL FUNCTIONS                            |
   |__________________________________________________________________________|
 */
static int apx_wdt_open (struct inode *inode, struct file *file) {

	return 0;
}


static int apx_wdt_close (struct inode *inode, struct file *file) {

	return 0;
}


static long apx_wdt_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {

	int status = 0;
	int trigger_mode = 0;
	unsigned long refresh_time = 0;
	unsigned long residual_time = 0;

	switch ( cmd ) {

		case APX_WDT_GET_STATUS: {
			mutex_lock (&wdt.ioctl_lock);

			mutex_lock (&wdt.ops_lock);
			status = wdt.wdt_enable;
			mutex_unlock (&wdt.ops_lock);

			if ( raw_copy_to_user ((void __user *)arg, &status, sizeof(int)) ) {
				mutex_unlock (&wdt.ioctl_lock);
				return -EFAULT;
			}

			mutex_unlock (&wdt.ioctl_lock);
			break;
		}

		case APX_WDT_SET_STATUS: {
			mutex_lock (&wdt.ioctl_lock);

			if (raw_copy_from_user (&status, (const void __user *)arg, sizeof (status))) {
				mutex_unlock (&wdt.ioctl_lock);
				return -EFAULT;
			}

			if ( status == APX_WDOG_STATUS_EN ) {
				PIN_CONF_EN;
				enable_triggering ();
			} else if (status == APX_WDOG_STATUS_DIS ) {
				PIN_CONF_DIS;
				disable_triggering ();
			} else {
				mutex_unlock (&wdt.ioctl_lock);
				return -EINVAL;
			}

			mutex_unlock (&wdt.ioctl_lock);
			break;
		}

		case APX_WDT_GET_TRIGGER_MODE: {
			mutex_lock (&wdt.ioctl_lock);

			mutex_lock (&wdt.ops_lock);
			trigger_mode = wdt.wdt_trigger_mode;
			mutex_unlock (&wdt.ops_lock);

			if ( raw_copy_to_user ((void __user *)arg, &trigger_mode, sizeof(int)) ) {
				mutex_unlock (&wdt.ioctl_lock);
				return -EFAULT;
			}

			mutex_unlock (&wdt.ioctl_lock);
			break;
		}

		case APX_WDT_SET_TRIGGER_MODE: {
			mutex_lock (&wdt.ioctl_lock);

			if (raw_copy_from_user (&trigger_mode, (const void __user *)arg, sizeof (trigger_mode))) {
				mutex_unlock (&wdt.ioctl_lock);
				return -EFAULT;
			}

			switch ( trigger_mode ) {
				case APX_WDOG_TRIG_MOD_AUTO:
				case APX_WDOG_TRIG_MOD_MAN:
					mutex_lock (&wdt.ops_lock);
					wdt.wdt_trigger_mode = trigger_mode;
					mutex_unlock (&wdt.ops_lock);
					break;
				default:
					mutex_unlock (&wdt.ioctl_lock);
					return -EINVAL;
					break;
			}

			mutex_unlock (&wdt.ioctl_lock);
			break;
		}

		case APX_WDT_GET_REFRESH_TIME: {
			mutex_lock (&wdt.ioctl_lock);

			mutex_lock (&wdt.ops_lock);
			refresh_time = jiffies_to_msecs(wdt.wdt_refresh_time);
			mutex_unlock (&wdt.ops_lock);

			if ( raw_copy_to_user ((void __user *)arg, &refresh_time, sizeof(unsigned long)) ) {
				mutex_unlock (&wdt.ioctl_lock);
				return -EFAULT;
			}

			mutex_unlock (&wdt.ioctl_lock);
			break;
		}

		case APX_WDT_SET_REFRESH_TIME: {
			mutex_lock (&wdt.ioctl_lock);

			if ( raw_copy_from_user (&refresh_time, (const void __user *)arg, sizeof (refresh_time))) {
				mutex_unlock (&wdt.ioctl_lock);
				return -EFAULT;
			}

			printk (KERN_INFO "davide ref time: %lu\n", refresh_time);
			mutex_lock (&wdt.ops_lock);
			wdt.wdt_refresh_time = msecs_to_jiffies(refresh_time);
			mutex_unlock (&wdt.ops_lock);
			mutex_unlock (&wdt.ioctl_lock);
			break;
		}

		case APX_WDT_GET_RESIDUAL_TIME: {
			mutex_lock (&wdt.ioctl_lock);

			mutex_lock (&wdt.ops_lock);
			if ( wdt.wdt_trigger_mode == APX_WDOG_TRIG_MOD_MAN )
				residual_time = jiffies_to_msecs(wdt.wdt_refresh_time -
					(jiffies - wdt.wdt_current_refresh_time));
			else
				residual_time = jiffies_to_msecs(wdt.wdt_refresh_time);
			mutex_unlock (&wdt.ops_lock);

			if ( raw_copy_to_user ((void __user *)arg, &residual_time, sizeof(unsigned long)) ) {
				mutex_unlock (&wdt.ioctl_lock);
				return -EFAULT;
			}

			mutex_unlock (&wdt.ioctl_lock);
			break;
		}

		case APX_WDT_REFRESH: {
			mutex_lock (&wdt.ioctl_lock);

			mutex_lock (&wdt.ops_lock);
			wdt.wdt_current_refresh_time = jiffies;
			mutex_unlock (&wdt.ops_lock);

			mutex_unlock (&wdt.ioctl_lock);
			break;
		}

		default:
			break;

	}

	return 0;
}



/*  __________________________________________________________________________
   |                                                                          |
   |                               INIT FUNCTIONS                             |
   |__________________________________________________________________________|
 */

const static struct file_operations apx_wdt_fops = {
	.owner          = THIS_MODULE,
	.open           = apx_wdt_open,
	.release        = apx_wdt_close,
	.unlocked_ioctl	= apx_wdt_ioctl,
};


struct class *apx_wdt_class;

static char *apx_wdt_devnode(struct device *dev, umode_t *mode) {
	if (!mode)
		return NULL;
	if (dev->devt == MKDEV(270, 0) ||
			dev->devt == MKDEV(270, 2))
		*mode = 0x666;
	return NULL;
}


static int __init apx_wdt_class_init(void)
{
	apx_wdt_class = class_create(THIS_MODULE, "wdt");
	if (IS_ERR(apx_wdt_class))
		return PTR_ERR(apx_wdt_class);
	apx_wdt_class->devnode = apx_wdt_devnode;
	apx_wdt_class->class_groups = apx_class_groups;
	return 0;
}

static struct cdev apx_wdt_cdev;

/*

postcore_initcall(apx_wdt_class_init);

void apx_wdog_trigger_early_init (const struct apx_wdog_trigger_data *apx_wdt_data, int state) {

	wdt.pin_trg.iomux      = ioremap (apx_wdt_data->gpio_trg__iomux_ctrl, 0x20);
	wdt.pin_trg.padctrl    = ioremap (apx_wdt_data->gpio_trg__pad_ctrl, 0x20);
	wdt.pin_trg.base       = ioremap (apx_wdt_data->gpio_trg__base, 0x20);
	wdt.pin_trg.num        = apx_wdt_data->gpio_trg__num;
	wdt.pin_en.iomux       = ioremap (apx_wdt_data->gpio_en__iomux_ctrl, 0x20);
	wdt.pin_en.padctrl     = ioremap (apx_wdt_data->gpio_en__pad_ctrl, 0x20);
	wdt.pin_en.base        = ioremap (apx_wdt_data->gpio_en__base, 0x20);
	wdt.pin_en.num	       = apx_wdt_data->gpio_en__num;

	wdt.is_initialized = 0;
	mutex_init (&wdt.ops_lock);
	mutex_init (&wdt.ioctl_lock);

	PIN_CONF_EN;
	imx6_wdog_refresh ();
}
*/
int apx_wdog_config_of(struct device_node *node) {
	
	u32 value;

	pr_info ("apx_wdog: %s\n",__func__);		

	if(of_property_read_u32(node, "apx,trigger_iomux", &value)) return -1; else wdt.pin_trg.iomux = ioremap (value,0x20); value = 0;
	if(of_property_read_u32(node, "apx,trigger_padctrl", &value)) return -1; else wdt.pin_trg.padctrl = ioremap (value,0x20); value = 0;
	if(of_property_read_u32(node, "apx,trigger_base", &value)) return -1; else wdt.pin_trg.base = ioremap (value,0x20); value = 0;
	if(of_property_read_u32(node, "apx,trigger_num", &value)) return -1; else wdt.pin_trg.num = value; 
	if(of_property_read_u32(node, "apx,en_iomux", &value)) return -1; else wdt.pin_en.iomux = ioremap (value,0x20); value = 0;
	if(of_property_read_u32(node, "apx,en_padctrl", &value)) return -1; else wdt.pin_en.padctrl = ioremap (value,0x20); value = 0;
	if(of_property_read_u32(node, "apx,en_base", &value)) return -1; else wdt.pin_en.base = ioremap (value,0x20); value = 0;
	if(of_property_read_u32(node, "apx,en_num", &value)) return -1; else wdt.pin_en.num = value; 

	if(of_property_read_u32(node,"apx,state",&wdt.state)) 
		wdt.state = 0;

	if(of_property_read_u32(node,"apx,enable-low",&wdt.wdt_en_low)) 
		wdt.wdt_en_low = 0;

	return 0;
}

int apx_wdog_probe (struct platform_device *pdev) {

	int ret;
	pr_info ("apx_wdog trigger loaded\n");
	/* DTS init */
	if(apx_wdog_config_of(pdev->dev.of_node) < 0) {
		pr_err("apx wdog: no apx_wdog dts config found - probe failed\n");
		return -ENODEV;
	}
		
	/* Variables init */
	wdt.refresh_wdt       = imx6_wdog_refresh;
	wdt.wdt_refresh_time  = WDOG_TIME;
	wdt.wdt_refresh       = 0;
	wdt.wdt_enable        = 0;
	wdt.wdt_trigger_mode  = APX_WDOG_TRIG_MOD_AUTO;

	wdt.is_initialized    = 1;

	if ( wdt.state ) {
		PIN_CONF_EN;
		enable_triggering ();
	} else {
		PIN_CONF_DIS;
		disable_triggering ();
	}
	apx_wdt_class_init();
	cdev_init (&apx_wdt_cdev, &apx_wdt_fops);
	if (cdev_add(&apx_wdt_cdev, MKDEV(270, 0), 1) ||
		register_chrdev_region(MKDEV(270, 0), 1, "/dev/apx_wdt") < 0) {
			panic("Couldn't register /dev/apx_wdt driver\n");
			ret = -ENOMEM;
			goto err_dev_init;
	}
	device_create(apx_wdt_class, NULL, MKDEV(270, 0), NULL, "apx_wdt");
	ret = class_register(&wdog_drv);

	return 0;

err_dev_init:
	printk(KERN_ERR"Unable to start apx_wdog\n");
	return ret;
}


static int apx_wdog_remove(struct platform_device *pdev)
{

	del_timer(&wdt.wdt_timer);
        pr_info("wdog_trigger exit\n");
      	class_unregister(&wdog_drv); 

        return 0;
}

static void apx_wdog_shutdown(struct platform_device *pdev)
{

	del_timer(&wdt.wdt_timer);
        pr_info("wdog_trigger exit\n");
	
}

static const struct of_device_id apx_wdog_dt_ids[] = {
        { .compatible = "seco,apx-wdog", },
        { /*sentinel */ }
};
MODULE_DEVICE_TABLE(of, apx_wdog_dt_ids);

static struct platform_driver apx_wdog_driver = {
        .probe          = apx_wdog_probe,
        .remove         = apx_wdog_remove,
        .shutdown       = apx_wdog_shutdown,
        .driver         = {
                .name   = "apx-wdog",
                .of_match_table = apx_wdog_dt_ids,
        },
};

static int __init apx_wdog_init(void)
{
        return platform_driver_register(&apx_wdog_driver);
}

postcore_initcall(apx_wdog_init);

MODULE_ALIAS("platform:apx_wdt");
MODULE_AUTHOR("MS DC SECO, Inc.");
MODULE_DESCRIPTION("WDOG Trigger Driver (APX823)");
MODULE_LICENSE("GPL");

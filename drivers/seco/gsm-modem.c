/*
 * GSM MODEM initialization driver
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <asm/uaccess.h>
#include <linux/ioctl.h>
#include <linux/major.h>
#include <linux/cdev.h>
#include <linux/sched.h>
#include <linux/device.h>
#include <linux/timer.h>
#include <linux/platform_device.h>

#include <linux/init.h>
#include <linux/smp.h>
#include <asm/cacheflush.h>
#include <asm/page.h>

static int usben_gpio, rst_gpio, pwrkey_gpio, vgsm_gpio;

int gsm_modem_config_of(struct device_node *node) {

	int retval;

	/* request reset pin */
        rst_gpio = of_get_named_gpio(node, "rst-gpios", 0);
        if (!gpio_is_valid(rst_gpio)) {
                pr_err("gsm-modem: no modem reset pin available gpios = %d\n",rst_gpio);
                return -EINVAL;
        }
        retval = gpio_request(rst_gpio, "gsm-modem_reset");
        if (retval < 0)
                return retval;
	
	
	/* request pwrkey pin */
        pwrkey_gpio = of_get_named_gpio(node, "pwrkey-gpios", 0);
        if (!gpio_is_valid(pwrkey_gpio)) {
                pr_err("gsm-modem: no modem pwrkey pin available\n");
                return -EINVAL;
        }
        retval = gpio_request(pwrkey_gpio, "gsm-modem_pwrkey");
        if (retval < 0)
                return retval;

	
	/* request usben pin */
        usben_gpio = of_get_named_gpio(node, "usben-gpios", 0);
        if (!gpio_is_valid(usben_gpio)) {
                pr_err("gsm-modem: no modem usb enable pin available\n");
                return -EINVAL;
        }
        retval = gpio_request(usben_gpio, "gsm-modem_usben");
        if (retval < 0)
                return retval;


	/* request vgsm pin */
        vgsm_gpio = of_get_named_gpio(node, "vgsm-gpios", 0);
        if (!gpio_is_valid(vgsm_gpio)) {
                pr_err("gsm-modem: no modem vgsm enable pin available\n");
                return -EINVAL;
        }
        retval = gpio_request(vgsm_gpio, "gsm-modem_vgsm");
        if (retval < 0)
                return retval;

        return 0;
}

int gsm_modem_probe (struct platform_device *pdev) 
{

	struct device_node *np = pdev->dev.of_node;

        pr_info ("gsm modem probing...\n");

	if(!np)	{
		pr_err("gsm-modem: no dts config found - probe failed\n");
		return -ENODEV;	
	}
	
        /* DTS init */
	if(gsm_modem_config_of(np) < 0) {
	       pr_err("gsm-modem: no gsm-modem dts config found - probe failed\n");
	       return -ENODEV;
	}
	
	/* Initializing modem */
	gpio_direction_output(usben_gpio,0);
	gpio_direction_output(vgsm_gpio,0);
	gpio_direction_output(pwrkey_gpio,0);
	gpio_direction_output(rst_gpio,1);
	udelay(100);
	gpio_direction_output(vgsm_gpio,1);
	udelay(10);
	gpio_direction_output(rst_gpio,0);
	udelay(10);
	gpio_direction_output(pwrkey_gpio,1);
	mdelay(2*500);
	gpio_direction_output(pwrkey_gpio,0);	

	pr_info ("gsm-modem: done\n");
	
	return 0;

}

static int gsm_modem_remove(struct platform_device *pdev)
{
	gpio_free(vgsm_gpio);
	gpio_free(usben_gpio);
	gpio_free(pwrkey_gpio);
	gpio_free(rst_gpio);
        return 0;
}

static void gsm_modem_shutdown(struct platform_device *pdev)
{
	gpio_free(vgsm_gpio);
        gpio_free(usben_gpio);
        gpio_free(pwrkey_gpio);
        gpio_free(rst_gpio);

        pr_info("gsm-modem exit\n");
 
}

static const struct of_device_id gsm_modem_dt_ids[] = {
        { .compatible = "seco,gsm-modem", },
        { /*sentinel */ }
};
MODULE_DEVICE_TABLE(of, gsm_modem_dt_ids);

static struct platform_driver gsm_modem_driver = {
        .probe          = gsm_modem_probe,
        .remove         = gsm_modem_remove,
        .shutdown       = gsm_modem_shutdown,
        .driver         = {
                .name   = "gsm-modem",
                .of_match_table = gsm_modem_dt_ids,
        },
};

module_platform_driver(gsm_modem_driver);

MODULE_ALIAS("platform:gsm_modem");
MODULE_AUTHOR("MS SECO, Inc.");
MODULE_DESCRIPTION("GSM Modem Initialization Driver");
MODULE_LICENSE("GPL");


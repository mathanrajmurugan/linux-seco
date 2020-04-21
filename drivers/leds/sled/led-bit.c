#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/sled.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/led-bit.h>



#define LEDBIT_INFO(fmt, arg...) printk(KERN_INFO "LED BIT driver: " fmt "\n" , ## arg)
#define LEDBIT_ERR(fmt, arg...)  dev_err(&pdev->dev, "%s: " fmt "\n" , __func__ , ## arg)
#define LEDBIT_DBG(fmt, arg...)  pr_debug("%s: " fmt "\n" , __func__ , ## arg)


struct led_bit_data {
	int      gpio;
	int      enable;
	int      stby_en;
};


/* ___________________________________________________________________________
  |                                                                           |
  |                                  ATTRIBUTES                               |
  |___________________________________________________________________________|
*/


int led_bit_sled_set_enable (struct device *dev, int enable) {
	struct platform_device *pdev = to_platform_device(dev);
	struct led_bit_data *pb = (struct led_bit_data *)platform_get_drvdata(pdev);

	if (enable == 0 || enable == 1) {
		pb->enable = enable;
		gpio_direction_output (pb->gpio, enable);
		return 0;
	} else
		return -EINVAL;
}


int led_bit_sled_get_enable (struct device *dev, int *enable) {
	struct platform_device *pdev = to_platform_device(dev);
	struct led_bit_data *pb = (struct led_bit_data *)platform_get_drvdata(pdev);

	*enable = pb->enable;
	return 0;
}


int led_bit_sled_set_stby_state (struct device *dev, int enable) {
	struct platform_device *pdev = to_platform_device(dev);
	struct led_bit_data *pb = (struct led_bit_data *)platform_get_drvdata(pdev);

	if (enable == 0 || enable == 1) {
		pb->stby_en = enable;
		return 0;
	} else
		return -EINVAL;
}


int led_bit_sled_get_stby_state (struct device *dev, int *enable) {
	struct platform_device *pdev = to_platform_device(dev);
	struct led_bit_data *pb = (struct led_bit_data *)platform_get_drvdata(pdev);

	*enable = pb->stby_en;
	return 0;
}


/* ___________________________________________________________________________
  |___________________________________________________________________________|
*/


static const struct sled_class_ops led_bit_sled_ops = {
	.set_enable     = led_bit_sled_set_enable,
	.get_enable     = led_bit_sled_get_enable,
	.set_stby_state = led_bit_sled_set_stby_state,
	.get_stby_state = led_bit_sled_get_stby_state,
	.set_duty       = NULL,  
	.get_duty       = NULL, 
	.set_stby_duty  = NULL,
	.get_stby_duty  = NULL,
	.get_max_duty   = NULL, 
	.set_period     = NULL, 
	.get_period     = NULL, 
	.get_max_period = NULL, 
	.get_min_period = NULL, 
};


static int led_bit_probe (struct platform_device *pdev) {
	struct sled_device *sled;
	struct platform_led_bit_data *data = pdev->dev.platform_data;
	int ret;

	struct led_bit_data *pb;

	if (!data) {
		LEDBIT_ERR ("failed to find platform data");
		return -EINVAL;
	}

	pb = kzalloc (sizeof(*pb), GFP_KERNEL);
	if (!pb) {
		LEDBIT_ERR ("no memory for state");
		ret = -ENOMEM;
		goto err_alloc;
	}

	pb->gpio = data->gpio;
	
	ret = gpio_request (pb->gpio, data->led_name);
	if (ret < 0) {
		LEDBIT_ERR ("failed to request GPIO %d, error %d\n", pb->gpio, ret);
		ret = -EIO;
		goto err_gpio_req;
	}

	pb->enable = data->default_state == SLED_OFF ? 0 : 1;
	pb->stby_en = data->standby_state ? 1 : 0;

	ret = gpio_direction_output (pb->gpio, pb->enable);
	if (ret < 0) {
		LEDBIT_ERR ("failed to configure direction for GPIO %d, error %d\n",
				pb->gpio, ret);
		goto err_gpio_dir;
	}

	platform_set_drvdata (pdev, pb);

	sled = sled_device_register (data->led_name, 
			&pdev->dev, &led_bit_sled_ops,
			LED_TYPE_BIT, NULL, THIS_MODULE);

	if (IS_ERR(sled)) {
		dev_err(&pdev->dev, "failed to register.\n");
		ret = PTR_ERR(sled);
		goto err_fs;
	}

	return 0;

err_alloc:
err_gpio_req:
err_gpio_dir:
err_fs:
	return 0;
}


static int led_bit_remove(struct platform_device *pdev) {

	struct led_bit_data *pb = (struct led_bit_data *)platform_get_drvdata(pdev);

	gpio_free (pb->gpio);

	return 0;
}


static int led_bit_suspend(struct platform_device *pdev,
				 pm_message_t state) {

	struct led_bit_data *pb = (struct led_bit_data *)platform_get_drvdata(pdev);

	if (pb->stby_en) {
		gpio_direction_output (pb->gpio, 1);
	} else {
		gpio_direction_output (pb->gpio, 0);
	}

	return 0;
}


static int led_bit_resume (struct platform_device *pdev) {

	struct led_bit_data *pb = (struct led_bit_data *)platform_get_drvdata(pdev);

	gpio_direction_output (pb->gpio, pb->enable);
	
	return 0;
}


static struct platform_driver led_bit_driver = {
	.driver		= {
		.name	= "led-bit",
		.owner	= THIS_MODULE,
	},
	.probe		= led_bit_probe,
	.remove		= led_bit_remove,
	.suspend	= led_bit_suspend,
	.resume		= led_bit_resume,
};


static int __init led_bit_init (void) {
	return platform_driver_register (&led_bit_driver);
}
module_init(led_bit_init);


static void __exit led_bit_exit (void) {
	platform_driver_unregister (&led_bit_driver);
}
module_exit(led_bit_exit);


MODULE_AUTHOR("DC SECO");
MODULE_DESCRIPTION("SECO LED BIT Driver");
MODULE_LICENSE("GPL");



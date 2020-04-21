#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/pwm.h>
#include <linux/led-pwm.h>
#include <linux/sled.h>
#include <linux/slab.h>


#define PWM_NAME_ID   "pwm_led"

#define LEDPWM_INFO(fmt, arg...) printk(KERN_INFO "LED PWM driver: " fmt "\n" , ## arg)
#define LEDPWM_ERR(fmt, arg...)  dev_err(&pdev->dev, "%s: " fmt "\n" , __func__ , ## arg)
#define LEDPWM_DBG(fmt, arg...)  pr_debug("%s: " fmt "\n" , __func__ , ## arg)


static int led_pwm_stby_status (struct  led_pwm_data *pd) {

	int duty = pd->stby_duty;
	int max = pd->max_duty;
	unsigned int lth_duty;

	/* if the stby_en is 0, there is no reason to set or disable
	   the PWM controller, since whene the STOPEN flag is 0 
	   (stby_en = 0), the PWM controller automatically shutdown itself. 
	*/

	if (pd->stby_en == 1) {
		
		lth_duty = pd->lth_duty;
		lth_duty /= pd->period;		
		lth_duty *= PWM_STBY_PERIOD;

		duty = lth_duty +
			(duty * (PWM_STBY_PERIOD - lth_duty) / max);

		pwm_stby_config (pd->pwm, duty, PWM_STBY_PERIOD);
	} 

	return 0;
}


static int led_pwm_update_status (struct  led_pwm_data *pd) {

	int duty = pd->duty;
	int max = pd->max_duty;
	
	if (duty == 0 || pd->enable == 0) {
		pwm_config (pd->pwm, 0, pd->period);
		pwm_disable (pd->pwm);
	} else if (pd->enable == 1) {
		duty = pd->lth_duty +
			(duty * (pd->period - pd->lth_duty) / max);
		pwm_config (pd->pwm, duty, pd->period);
		pwm_enable (pd->pwm);
	}
	if (pd->stby_en == 1)
		pwm_stby_enable (pd->pwm);
	else
		pwm_stby_disable (pd->pwm);

	return 0;
}



/* ___________________________________________________________________________
  |                                                                           |
  |                                  ATTRIBUTES                               |
  |___________________________________________________________________________|
*/


int led_pwm_sled_set_enable (struct device *dev, int enable) {
	struct platform_device *pdev = to_platform_device(dev);
	struct led_pwm_data *pd = (struct led_pwm_data *)platform_get_drvdata(pdev);

	if (enable == 0 || enable == 1) {
		pd->enable = enable;
		led_pwm_update_status (pd);
		return 0;
	} else
		return -EINVAL;
}


int led_pwm_sled_get_enable (struct device *dev, int *enable) {
	struct platform_device *pdev = to_platform_device(dev);
	struct led_pwm_data *pd = (struct led_pwm_data *)platform_get_drvdata(pdev);

	*enable = pd->enable;
	return 0;
}


int led_pwm_sled_set_stby_state (struct device *dev, int enable) {
	struct platform_device *pdev = to_platform_device(dev);
	struct led_pwm_data *pd = (struct led_pwm_data *)platform_get_drvdata(pdev);

	if (enable == 0 || enable == 1) {
		if (enable == 1)
			pwm_stby_enable (pd->pwm);
		else
			pwm_stby_disable (pd->pwm);
		pd->stby_en = enable;
		return 0;
	} else
		return -EINVAL;
}


int led_pwm_sled_get_stby_state (struct device *dev, int *enable) {
	struct platform_device *pdev = to_platform_device(dev);
	struct led_pwm_data *pd = (struct led_pwm_data *)platform_get_drvdata(pdev);

	*enable = pd->stby_en;
	return 0;
}


int led_pwm_sled_set_duty (struct device *dev, unsigned long duty) {
	struct platform_device *pdev = to_platform_device(dev);
	struct led_pwm_data *pd = (struct led_pwm_data *)platform_get_drvdata(pdev);

	if (duty > pd->max_duty)
		return  -EINVAL;
	else {
		pd->duty = duty;
		led_pwm_update_status (pd);
		return 0;
	}
}


int led_pwm_sled_get_stby_duty (struct device *dev, unsigned long *duty) {
	struct platform_device *pdev = to_platform_device(dev);
	struct led_pwm_data *pd = (struct led_pwm_data *)platform_get_drvdata(pdev);

	*duty = pd->stby_duty;
	return 0;
}


int led_pwm_sled_set_stby_duty (struct device *dev, unsigned long duty) {
	struct platform_device *pdev = to_platform_device(dev);
	struct led_pwm_data *pd = (struct led_pwm_data *)platform_get_drvdata(pdev);

	if (duty > pd->max_duty)
		return  -EINVAL;
	else {
		pd->stby_duty = duty;
		return 0;
	}
}


int led_pwm_sled_get_duty (struct device *dev, unsigned long *duty) {
	struct platform_device *pdev = to_platform_device(dev);
	struct led_pwm_data *pd = (struct led_pwm_data *)platform_get_drvdata(pdev);

	*duty = pd->duty;
	return 0;
}


int led_pwm_sled_get_max_duty (struct device *dev, unsigned long *max_duty) {
	struct platform_device *pdev = to_platform_device(dev);
	struct led_pwm_data *pd = (struct led_pwm_data *)platform_get_drvdata(pdev);

	*max_duty = pd->max_duty;
	return 0;
}


int led_pwm_sled_set_period (struct device *dev, unsigned long period) {
	struct platform_device *pdev = to_platform_device(dev);
	struct led_pwm_data *pd = (struct led_pwm_data *)platform_get_drvdata(pdev);

	if ((period > pd->period_max) || (period < pd->period_min))
		return -EINVAL;
	else {
		pd->lth_duty /= pd->period;
		pd->period = period;
		pd->lth_duty = pd->lth_duty * pd->period;
		led_pwm_update_status (pd);
		return 0;
	}
}


int led_pwm_sled_get_period (struct device *dev, unsigned long *period) {
	struct platform_device *pdev = to_platform_device(dev);
	struct led_pwm_data *pd = (struct led_pwm_data *)platform_get_drvdata(pdev);

	*period = pd->period;
	return 0;
}


int led_pwm_sled_get_max_period (struct device *dev, unsigned long *max) {
	struct platform_device *pdev = to_platform_device(dev);
	struct led_pwm_data *pd = (struct led_pwm_data *)platform_get_drvdata(pdev);

	*max = pd->period_max;
	return 0;
}


int led_pwm_sled_get_min_period (struct device *dev, unsigned long *min) {
	struct platform_device *pdev = to_platform_device(dev);
	struct led_pwm_data *pd = (struct led_pwm_data *)platform_get_drvdata(pdev);

	*min = pd->period_min;
	return 0;
}

/* ___________________________________________________________________________
  |___________________________________________________________________________|
*/


static const struct sled_class_ops led_pwm_sled_ops = {
	.set_enable     = led_pwm_sled_set_enable,
	.get_enable     = led_pwm_sled_get_enable,
	.set_stby_state = led_pwm_sled_set_stby_state,
	.get_stby_state = led_pwm_sled_get_stby_state,
	.set_duty       = led_pwm_sled_set_duty,
	.get_duty       = led_pwm_sled_get_duty,
	.set_stby_duty  = led_pwm_sled_set_stby_duty,
	.get_stby_duty  = led_pwm_sled_get_stby_duty,
	.get_max_duty   = led_pwm_sled_get_max_duty,
	.set_period     = led_pwm_sled_set_period,
	.get_period     = led_pwm_sled_get_period,
	.get_max_period = led_pwm_sled_get_max_period,
	.get_min_period = led_pwm_sled_get_min_period,
};


static int led_pwm_probe (struct platform_device *pdev) {

	struct sled_device *sled;
	struct platform_led_pwm_data *data = pdev->dev.platform_data;
	int ret;
	struct led_pwm_data *pd;

	if (!data) {
		LEDPWM_ERR ("failed to find platform data");
		return -EINVAL;
	}

	pd = kzalloc (sizeof(*pd), GFP_KERNEL);
	if (!pd) {
		LEDPWM_ERR ("no memory for state");
		ret = -ENOMEM;
		goto err_alloc;
	}

	pd->pwm = pwm_request(data->pwm_id, PWM_NAME_ID);
	if (IS_ERR(pd->pwm)) {
		LEDPWM_ERR ("unable to request PWM");
		ret = PTR_ERR(pd->pwm);
		goto err_pwm;
	} else
		LEDPWM_DBG ("got pwm for backlight\n");


	pd->duty = data->dft_duty;
	pd->max_duty = data->max_duty;
	
	pd->period = data->pwm_period_ns;
	pd->period_max = data->period_ns_max;
	pd->period_min = data->period_ns_min;

	pd->lth_duty = data->lth_duty *
		(data->pwm_period_ns / data->max_duty);

	pd->enable = data->default_state == SLED_OFF ? 0 : 1;
	pd->stby_en = data->standby_state == SLED_OFF? 0 : 1;

	if (data->stby_def_duty > 0) 
		pd->stby_duty = data->stby_def_duty;
	else
		pd->stby_duty = 0;

	platform_set_drvdata (pdev, pd);

	led_pwm_update_status (pd);
	
	sled = sled_device_register (data->led_name, 
			&pdev->dev, &led_pwm_sled_ops,
			LED_TYPE_PWM, pd, THIS_MODULE);


	if (IS_ERR(sled)) {
		dev_err(&pdev->dev, "failed to register.\n");
		ret = PTR_ERR(sled);
		goto err_fs;
	}

	return 0;

err_fs:
	pwm_free(pd->pwm);
err_pwm:
	kfree(pd);
err_alloc:
	return ret;
}


static int led_pwm_remove(struct platform_device *pdev) {

	struct led_pwm_data *pd = (struct led_pwm_data *)platform_get_drvdata(pdev);

	pwm_config (pd->pwm, 0, pd->period);
        pwm_disable (pd->pwm);
        pwm_free (pd->pwm);
        kfree (pd);
	return 0;
}


static int led_pwm_suspend(struct platform_device *pdev,
				 pm_message_t state) {

	struct led_pwm_data *pd = (struct led_pwm_data *)platform_get_drvdata(pdev);

	led_pwm_stby_status (pd);

	return 0;
}


static int led_pwm_resume (struct platform_device *pdev) {

	struct led_pwm_data *pd = (struct led_pwm_data *)platform_get_drvdata(pdev);

	led_pwm_update_status (pd);

	return 0;
}


static struct platform_driver led_pwm_driver = {
	.driver		= {
		.name	= "led-pwm",
		.owner	= THIS_MODULE,
	},
	.probe		= led_pwm_probe,
	.remove		= led_pwm_remove,
	.suspend	= led_pwm_suspend,
	.resume		= led_pwm_resume,
};


static int __init led_pwm_init (void) {
	return platform_driver_register (&led_pwm_driver);
}
module_init(led_pwm_init);


static void __exit led_pwm_exit (void) {
	platform_driver_unregister (&led_pwm_driver);
}
module_exit(led_pwm_exit);


MODULE_AUTHOR("DC SECO");
MODULE_DESCRIPTION("SECO LED PWM Driver");
MODULE_LICENSE("GPL");




#include <linux/module.h>
#include <linux/sled.h>
#include <linux/kdev_t.h>
#include <linux/device.h>
#include <linux/idr.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/workqueue.h>

#include "sled-core.h"


#define CLASS_NAME  "sled"

static DEFINE_MUTEX(idr_lock);


struct class *sled_class;



struct sled_device *sled_device_register (const char *name, struct device *dev,
					const struct sled_class_ops *ops,
					int led_type,
					struct led_pwm_data *pwm_data,
					struct module *owner)
{
	static int idx = 0;
	struct sled_device *sled;
	int id, err;

	if (led_type != LED_TYPE_PWM && led_type != LED_TYPE_BIT) {
		err = -EINVAL;
		goto exit;
	}

	mutex_lock (&idr_lock);
	id = idx;
	idx++;
	mutex_unlock(&idr_lock);
	if ( id < 0 ) {
		err = id;
		goto exit;
	}

	sled = kzalloc(sizeof(struct sled_device), GFP_KERNEL);
	if (sled == NULL) {
		err = -ENOMEM;
		goto exit_idr;
	}

	mutex_lock (&idr_lock);
	sled->id         = id;
	sled->ops        = ops;
	sled->owner      = owner;
	sled->dev.parent = dev;
	sled->dev.class  = sled_class;
	sled->led_pwm    = pwm_data;
	sled->led_type   = led_type;

	mutex_init (&sled->ops_lock);

	strlcpy (sled->name, name, SLED_DEVICE_NAME_SIZE);
	dev_set_name (&sled->dev, "sled.%d", id);

	sled_dev_prepare (sled);
	sled_dev_add_device (sled);

	err = device_register (&sled->dev);
	if (err) {
		put_device (&sled->dev);
		mutex_unlock(&idr_lock);
		goto exit_kfree;
	}

	sled_sysfs_add_device (sled);

	mutex_unlock(&idr_lock);

	dev_info(dev, "sled core: registered %s as %s\n",
			sled->name, dev_name (&sled->dev));

	return sled;

exit_kfree:
	dev_err (&sled->dev, "Failed to register sled device (error %d)", err);
	kfree (sled);

exit_idr:
	mutex_lock (&idr_lock);
	mutex_unlock (&idr_lock);

exit:
	dev_err(dev, "sled: unable to register %s, err = %d\n",
			name, err);
	return ERR_PTR(err);
}
EXPORT_SYMBOL_GPL(sled_device_register);


void sled_device_unregister(struct sled_device *sled)
{
	if (get_device(&sled->dev) != NULL) {
		mutex_lock(&sled->ops_lock);
		
		sled_sysfs_del_device (sled);
		sled_dev_del_device (sled);
		device_unregister (&sled->dev);
		sled->ops = NULL;
		mutex_unlock (&sled->ops_lock);
		put_device (&sled->dev);
	}
}
EXPORT_SYMBOL_GPL(sled_device_unregister);



static int __init sled_init(void) {
	sled_class = class_create (THIS_MODULE, CLASS_NAME);
	if (IS_ERR(sled_class)) {
		printk(KERN_ERR "%s: couldn't create class\n", __FILE__);
		return PTR_ERR(sled_class);
	}
	sled_dev_init ();
	sled_sysfs_init (sled_class);
	return 0;
}


static void __exit sled_exit(void) {
	sled_dev_exit ();
	class_destroy (sled_class);
}

subsys_initcall (sled_init);
module_exit (sled_exit);

MODULE_AUTHOR("Davide Cardillo, SECO srl");
MODULE_DESCRIPTION("Seco Led class support");
MODULE_LICENSE("GPL");

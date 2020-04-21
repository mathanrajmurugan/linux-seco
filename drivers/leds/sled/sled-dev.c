

#include <linux/module.h>
#include <linux/sled.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/ioctl.h>
#include <linux/major.h>
#include <linux/slab.h>

#include <linux/sled.h>
#include "sled-core.h"

static dev_t sled_devt;


static int sled_dev_open (struct inode *inode, struct file *file) {
	
	struct sled_device *sled = container_of(inode->i_cdev,
					struct sled_device, char_dev);

	if (test_and_set_bit_lock(0, &sled->flags))
		return -EBUSY;

	file->private_data = sled;

	clear_bit_unlock(0, &sled->flags);
	return 0;
}


static ssize_t sled_dev_read (struct file *file, char __user *buf, size_t count, loff_t *ppos) {
	return 0;
}


static int sled_dev_release (struct inode *inode, struct file *file) {
	struct sled_device *sled = file->private_data;

	if (sled->ops->release)
		sled->ops->release (sled);

	clear_bit_unlock (0, &sled->flags);
	return 0;
}


static long sled_dev_ioctl (struct file *file, unsigned int cmd, unsigned long arg) {
	int err = 0;
	int type, enable;
	char *name;
	unsigned long duty, max_duty, delay, max_delay, period, max_period, min_period;
	struct sled_device *sled = file->private_data;
	void __user *uarg = (void __user *) arg;

	err = mutex_lock_interruptible (&sled->ops_lock);
	if (err)
		return err;

	if (sled->led_type == LED_TYPE_BIT) {
		switch (cmd) {
			case SLED_DUTY_GET:
			case SLED_DUTY_SET:
			case SLED_DELAY_GET:
			case SLED_DELAY_SET:
			case SLED_STBY_DUTY_GET:
			case SLED_STBY_DUTY_SET:
			case SLED_MAX_DUTY_GET:
			case SLED_MAX_DELAY_GET:
			case SLED_PERIOD_SET:
			case SLED_PERIOD_GET:
			case SLED_MAX_PERIOD_GET:
			case SLED_MIN_PERIOD_GET:
				err = -EACCES;
				mutex_unlock (&sled->ops_lock);
				goto ioctl_end;
				break;
			default:
				break;
		}
	}

	switch (cmd) {
		case SLED_NAME_GET:
			err = 0;
			name = kzalloc (sizeof(char) * SLED_DEVICE_NAME_SIZE, GFP_KERNEL);
			strlcpy (name, sled->name, SLED_DEVICE_NAME_SIZE);
			mutex_unlock (&sled->ops_lock);
			if (raw_copy_to_user (uarg, name, sizeof (char) * SLED_DEVICE_NAME_SIZE))
				err = -EFAULT;

			kfree (name);
			return err;


		case SLED_TYPE_GET:
			mutex_unlock (&sled->ops_lock);

			err = 0;
			type = sled->led_type;
			if (raw_copy_to_user (uarg, &type, sizeof (type)))
				err = -EFAULT;

			return err;


		case SLED_ENABLE_SET:
			mutex_unlock (&sled->ops_lock);

			if (raw_copy_from_user (&enable, uarg, sizeof(enable)))
				return -EFAULT;

			if (enable != 0 && enable != 1)
				return -EINVAL;

			sled_store_enable (sled, enable);

			if (raw_copy_to_user ((void __user *)uarg, &enable, sizeof(enable)))
				return -EFAULT;

			return 0;


		case SLED_ENABLE_GET:
			mutex_unlock (&sled->ops_lock);

			err = sled_read_enable (sled, &enable);
			if (err != 0)
				return err;

			if (raw_copy_to_user (uarg, &enable, sizeof (enable)))
				err = -EFAULT;

			return err;


		case SLED_STBY_ENABLE_SET:
			mutex_unlock (&sled->ops_lock);

			if (raw_copy_from_user (&enable, uarg, sizeof(enable)))
				return -EFAULT;

			if (enable != 0 && enable != 1)
				return -EINVAL;

			sled_store_stby_state (sled, enable);

			if (raw_copy_to_user ((void __user *)uarg, &enable, sizeof(enable)))
				return -EFAULT;

			return 0;


		case SLED_STBY_ENABLE_GET:
			mutex_unlock (&sled->ops_lock);

			err =  sled_read_stby_state (sled, &enable);

			if (err != 0)
				return err;

			if (raw_copy_to_user (uarg, &enable, sizeof (enable)))
				err = -EFAULT;

			return err;

		case SLED_DUTY_SET:
			mutex_unlock (&sled->ops_lock);

			if (raw_copy_from_user (&duty, uarg, sizeof(duty)))
				return -EFAULT;

			sled_store_duty (sled, duty);

			if (raw_copy_to_user ((void __user *)uarg, &duty, sizeof(duty)))
				return -EFAULT;

			return 0;


		case SLED_DUTY_GET:
			mutex_unlock (&sled->ops_lock);

			err = sled_read_duty (sled, &duty);
			if (err != 0)
				return err;

			if (raw_copy_to_user (uarg, &duty, sizeof (duty)))
				err = -EFAULT;

			return err;


		case SLED_STBY_DUTY_SET:
			mutex_unlock (&sled->ops_lock);

			if (raw_copy_from_user (&duty, uarg, sizeof(duty)))
				return -EFAULT;

			sled_store_stby_duty (sled, duty);

			if (raw_copy_to_user ((void __user *)uarg, &duty, sizeof(duty)))
				return -EFAULT;

			return 0;


		case SLED_STBY_DUTY_GET:
			mutex_unlock (&sled->ops_lock);

			err = sled_read_stby_duty (sled, &duty);
			if (err != 0)
				return err;

			if (raw_copy_to_user (uarg, &duty, sizeof (duty)))
				err = -EFAULT;

			return err;


		case SLED_MAX_DUTY_GET:
			mutex_unlock (&sled->ops_lock);

			err = sled_read_max_duty (sled, &max_duty);
			if (err != 0)
				return err;

			if (raw_copy_to_user (uarg, &max_duty, sizeof (max_duty)))
				err = -EFAULT;

			return err;


		case SLED_PERIOD_SET:
			mutex_unlock (&sled->ops_lock);

			if (raw_copy_from_user (&period, uarg, sizeof(period)))
				return -EFAULT;

			sled_store_period (sled, period);

			if (raw_copy_to_user ((void __user *)uarg, &period, sizeof(period)))
				return -EFAULT;
			
			return 0;


		case SLED_PERIOD_GET:
			mutex_unlock (&sled->ops_lock);

			err = sled_read_period (sled, &period);
			if (err != 0)
				return err;

			if (raw_copy_to_user (uarg, &period, sizeof (period)))
				err = -EFAULT;

			return err;


		case SLED_MAX_PERIOD_GET:
			mutex_unlock (&sled->ops_lock);

			err = sled_read_max_period (sled, &max_period);
			if (err != 0)
				return err;

			if (raw_copy_to_user (uarg, &max_period, sizeof (max_period)))
				err = -EFAULT;

			return err;


		case SLED_MIN_PERIOD_GET:
			mutex_unlock (&sled->ops_lock);

			err = sled_read_min_period (sled, &min_period);
			if (err != 0)
				return err;

			if (raw_copy_to_user (uarg, &min_period, sizeof (min_period)))
				err = -EFAULT;

			return err;


		case SLED_DELAY_SET:
			mutex_unlock (&sled->ops_lock);

			if (raw_copy_from_user (&delay, uarg, sizeof(delay)))
				return -EFAULT;

			sled_store_delay (sled, delay);

			if (raw_copy_to_user ((void __user *)uarg, &delay, sizeof(delay)))
				return -EFAULT;

			return 0;


		case SLED_DELAY_GET:
			mutex_unlock (&sled->ops_lock);

			err = sled_read_delay (sled, &delay);
			if (err != 0)
				return err;

			if (raw_copy_to_user (uarg, &delay, sizeof (delay)))
				err = -EFAULT;

			return err;


		case SLED_MAX_DELAY_GET:
			mutex_unlock (&sled->ops_lock);

			err = sled_read_max_delay (sled, &max_delay);
			if (err != 0)
				return err;

			if (raw_copy_to_user (uarg, &max_delay, sizeof (max_delay)))
				err = -EFAULT;

			return err;

		default:
			break;
	}

ioctl_end:
	return err;
}


static const struct file_operations sled_dev_fops = {
	.owner          = THIS_MODULE,
	.read           = sled_dev_read,
	.open           = sled_dev_open,
	.release        = sled_dev_release,
	.unlocked_ioctl	= sled_dev_ioctl,
};


void sled_dev_prepare(struct sled_device *sled) {
	if (!sled_devt)
		return;

	if (sled->id >= 64) {
		pr_debug("%s: too many SLED devices\n", sled->name);
		return;
	}

	sled->dev.devt = MKDEV(MAJOR(sled_devt), sled->id);

	cdev_init (&sled->char_dev, &sled_dev_fops);
	sled->char_dev.owner = sled->owner;
}


void sled_dev_add_device (struct sled_device *sled) {
	if (cdev_add (&sled->char_dev, sled->dev.devt, 1))
		printk(KERN_WARNING "%s: failed to add char device %d:%d\n",
			sled->name, MAJOR(sled_devt), sled->id);
	else
		pr_debug("%s: dev (%d:%d)\n", sled->name,
			MAJOR(sled_devt), sled->id);
}


void sled_dev_del_device (struct sled_device *sled) {
	if (sled->dev.devt)
		cdev_del (&sled->char_dev);
}


void __init sled_dev_init(void) {
	int err;

	err = alloc_chrdev_region (&sled_devt, 0, 64, "sled");
	if (err < 0)
		printk(KERN_ERR "%s: failed to allocate char dev region\n",
			__FILE__);
}


void __exit sled_dev_exit(void) {
	if (sled_devt)
		unregister_chrdev_region (sled_devt, 64);
}

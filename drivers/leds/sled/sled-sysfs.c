#include <linux/module.h>
#include <linux/sled.h>
#include <linux/sched.h>
#include <linux/sysfs.h>

#include "sled-core.h"


static ssize_t
sled_sysfs_show_name (struct device *dev, struct device_attribute *attr, char *buf) {

	return sprintf (buf, "%s\n", container_of (dev, struct sled_device, dev)->name);
}


static ssize_t
sled_sysfs_show_type (struct device *dev, struct device_attribute *attr, char *buf) {

	return sprintf (buf, "%s\n",
			container_of (dev, struct sled_device, dev)->led_type == LED_TYPE_PWM ?
			LED_LABEL_PWM : LED_LABEL_BIT);
}


static ssize_t
sled_sysfs_show_enable (struct device *dev, struct device_attribute *attr, char *buf) {
	int en;
	ssize_t retval;

	retval = sled_read_enable (container_of (dev, struct sled_device, dev), &en);
	if (retval == 0)
		retval = sprintf (buf, "%s\n", en == 1 ? "enabled" : "disabled");
	return retval;
}


static ssize_t
sled_sysfs_store_enable (struct device *dev, struct device_attribute *attr,
		const char *buf, size_t n)
{
	struct sled_device *sled = container_of (dev, struct sled_device, dev);
	unsigned long val = simple_strtoul (buf, NULL, 0);

	if (val != 0 && val != 1)
		return -EINVAL;

	if (sled_store_enable (sled, val))
		return -EINVAL;

	return n;
}


static ssize_t
sled_sysfs_show_stby_state(struct device *dev, struct device_attribute *attr, char *buf) {
	int en;
	ssize_t retval;

	retval = sled_read_stby_state (container_of (dev, struct sled_device, dev), &en);
	if (retval == 0)
		retval = sprintf (buf, "%s\n", en == 1 ? "enabled" : "disabled");
	return retval;
}


static ssize_t
sled_sysfs_store_stby_state (struct device *dev, struct device_attribute *attr,
		const char *buf, size_t n)
{
	struct sled_device *sled = container_of (dev, struct sled_device, dev);
	unsigned long val = simple_strtoul (buf, NULL, 0);

	if (val != 0 && val != 1)
		return -EINVAL;

	if (sled_store_stby_state (sled, val))
		return -EINVAL;

	return n;
}


static ssize_t
sled_sysfs_show_duty (struct device *dev, struct device_attribute *attr, char *buf) {
	unsigned long duty;
	ssize_t retval;

	retval = sled_read_duty (container_of (dev, struct sled_device, dev), &duty);
	if (retval == 0)
		retval = sprintf (buf, "%lu\n", duty);
	return retval;
}


static ssize_t
sled_sysfs_store_duty (struct device *dev, struct device_attribute *attr,
		const char *buf, size_t n)
{
	struct sled_device *sled = container_of (dev, struct sled_device, dev);
	unsigned long val = simple_strtoul (buf, NULL, 0);

	if (sled_store_duty (sled, val))
		return -EINVAL;

	return n;
}


static ssize_t
sled_sysfs_show_delay (struct device *dev, struct device_attribute *attr, char *buf) {
	unsigned long delay;
	ssize_t retval;

	retval = sled_read_delay (container_of (dev, struct sled_device, dev), &delay);
	if (retval == 0)
		retval = sprintf (buf, "%lu\n", delay);
	return retval;
}


static ssize_t
sled_sysfs_store_delay (struct device *dev, struct device_attribute *attr,
		const char *buf, size_t n)
{
	struct sled_device *sled = container_of (dev, struct sled_device, dev);
	unsigned long val = simple_strtoul (buf, NULL, 0);

	if (sled_store_delay (sled, val))
		return -EINVAL;

	return n;
}

static ssize_t
sled_sysfs_show_stby_duty (struct device *dev, struct device_attribute *attr, char *buf) {
	unsigned long duty;
	ssize_t retval;

	retval = sled_read_stby_duty (container_of (dev, struct sled_device, dev), &duty);
	if (retval == 0)
		retval = sprintf (buf, "%lu\n", duty);
	return retval;
}


static ssize_t
sled_sysfs_store_stby_duty (struct device *dev, struct device_attribute *attr,
		const char *buf, size_t n)
{
	struct sled_device *sled = container_of (dev, struct sled_device, dev);
	unsigned long val = simple_strtoul (buf, NULL, 0);

	if (sled_store_stby_duty (sled, val))
		return -EINVAL;

	return n;
}


static ssize_t
sled_sysfs_show_max_duty (struct device *dev, struct device_attribute *attr, char *buf) {
	unsigned long max_duty;
	ssize_t retval;

	retval = sled_read_max_duty (container_of (dev, struct sled_device, dev), &max_duty);
	if (retval == 0)
		retval = sprintf (buf, "%lu\n", max_duty);
	return retval;
}


static ssize_t
sled_sysfs_show_max_delay (struct device *dev, struct device_attribute *attr, char *buf) {
	unsigned long max_delay;
	ssize_t retval;

	retval = sled_read_max_delay (container_of (dev, struct sled_device, dev), &max_delay);
	if (retval == 0)
		retval = sprintf (buf, "%lu\n", max_delay);
	return retval;
}


static ssize_t
sled_sysfs_show_period (struct device *dev, struct device_attribute *attr, char *buf) {
	unsigned long period;
	ssize_t retval;

	retval = sled_read_period (container_of (dev, struct sled_device, dev), &period);
	if (retval == 0)
		retval = sprintf (buf, "%lu ns\n", period);
	return retval;
}


static ssize_t
sled_sysfs_store_period (struct device *dev, struct device_attribute *attr,
		const char *buf, size_t n)
{
	struct sled_device *sled = container_of (dev, struct sled_device, dev);
	unsigned long val = simple_strtoul (buf, NULL, 0);

	if (sled_store_period (sled, val))
		return -EINVAL;

	return n;
}


static ssize_t
sled_sysfs_show_max_period (struct device *dev, struct device_attribute *attr, char *buf) {
	unsigned long  val;
	ssize_t retval;

	retval = sled_read_max_period (container_of (dev, struct sled_device, dev), &val);
	if (retval == 0)
		retval = sprintf (buf, "%lu ns\n", val);
	return retval;
}


static ssize_t
sled_sysfs_show_min_period (struct device *dev, struct device_attribute *attr, char *buf) {
	unsigned long val;
	ssize_t retval;

	retval = sled_read_min_period (container_of (dev, struct sled_device, dev), &val);
	if (retval == 0)
		retval = sprintf (buf, "%lu ns\n", val);
	return retval;
}


static DEVICE_ATTR (name,          0444, sled_sysfs_show_name, NULL);
static DEVICE_ATTR (type,          0444, sled_sysfs_show_type, NULL);
static DEVICE_ATTR (enable,        0664, sled_sysfs_show_enable, sled_sysfs_store_enable);
static DEVICE_ATTR (standby_state, 0664, sled_sysfs_show_stby_state, sled_sysfs_store_stby_state);


static struct attribute *sled_attrs[] = {
	&dev_attr_name.attr,
	&dev_attr_type.attr,
	&dev_attr_enable.attr,
	&dev_attr_standby_state.attr,
	NULL,
};


static struct attribute_group sled_attr_group = {
	.attrs = sled_attrs,
};


static DEVICE_ATTR (duty,         0664, sled_sysfs_show_duty, sled_sysfs_store_duty);
static DEVICE_ATTR (delay,         0664, sled_sysfs_show_delay, sled_sysfs_store_delay);
static DEVICE_ATTR (standby_duty, 0664, sled_sysfs_show_stby_duty, sled_sysfs_store_stby_duty);
static DEVICE_ATTR (max_duty,     0444, sled_sysfs_show_max_duty, NULL);
static DEVICE_ATTR (max_delay,     0444, sled_sysfs_show_max_delay, NULL);
static DEVICE_ATTR (period,       0664, sled_sysfs_show_period, sled_sysfs_store_period);
static DEVICE_ATTR (max_period,   0444, sled_sysfs_show_max_period, NULL);
static DEVICE_ATTR (min_period,   0444, sled_sysfs_show_min_period, NULL);


static struct attribute *sled_pwm_sysfs[] = {
	&dev_attr_duty.attr,
	&dev_attr_delay.attr,
	&dev_attr_standby_duty.attr,
	&dev_attr_max_duty.attr,
	&dev_attr_max_delay.attr,
	&dev_attr_period.attr,
	&dev_attr_max_period.attr,
	&dev_attr_min_period.attr,
	NULL,
};


static struct attribute_group sled_pwm_attr_group = {
	.attrs = sled_pwm_sysfs,
};


int sled_sysfs_add_device (struct sled_device *sled) {
	int err = 0;

	err = sysfs_create_group (&sled->dev.kobj, &sled_attr_group);
	if ( err )
		return err;

	if (sled->led_type == LED_TYPE_PWM) {
		err = sysfs_create_group (&sled->dev.kobj, &sled_pwm_attr_group);
	}

	return err;
}


void sled_sysfs_del_device (struct sled_device *sled) {

	sysfs_remove_group (&sled->dev.kobj, &sled_attr_group);

	if (sled->led_type == LED_TYPE_PWM) {
		sysfs_remove_group (&sled->dev.kobj, &sled_pwm_attr_group);
	}

}


void __init sled_sysfs_init (struct class *sled_class) {
}

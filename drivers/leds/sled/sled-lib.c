
#include <linux/sled.h>
#include <linux/sched.h>

int sled_read_enable (struct sled_device *sled, int *enable) {
	int err;

	err = mutex_lock_interruptible (&sled->ops_lock);
	if (err)
		return err;

	if (!sled->ops)
		err = -ENODEV;
	else if (!sled->ops->get_enable)
		err = -EINVAL;
	else {
		err = sled->ops->get_enable (sled, enable);
	}

	mutex_unlock (&sled->ops_lock);
	return err;
}
EXPORT_SYMBOL_GPL(sled_read_enable);


int sled_store_enable (struct sled_device *sled, int enable) {
	int err;

	err = mutex_lock_interruptible (&sled->ops_lock);
	if (err)
		return err;

	if (!sled->ops)
		err = -ENODEV;
	else if (!sled->ops->set_enable)
		err = -EINVAL;
	else {
		err = sled->ops->set_enable (sled, enable);
		if (!err)
			sled->led_enable = enable;
	}

	mutex_unlock (&sled->ops_lock);
	return err;
}
EXPORT_SYMBOL_GPL(sled_store_enable);


int sled_read_stby_state (struct sled_device *sled, int *enable) {
	int err;

	err = mutex_lock_interruptible (&sled->ops_lock);
	if (err)
		return err;

	if (!sled->ops)
		err = -ENODEV;
	else if (!sled->ops->get_stby_state)
		err = -EINVAL;
	else {
		err = sled->ops->get_stby_state (sled, enable);
	}

	mutex_unlock (&sled->ops_lock);
	return err;
}
EXPORT_SYMBOL_GPL(sled_read_stby_state);


int sled_store_stby_state (struct sled_device *sled, int enable) {
	int err;

	err = mutex_lock_interruptible (&sled->ops_lock);
	if (err)
		return err;

	if (!sled->ops)
		err = -ENODEV;
	else if (!sled->ops->set_stby_state)
		err = -EINVAL;
	else {
		err = sled->ops->set_stby_state (sled, enable);
		if (!err)
			sled->led_stby_state = enable;
	}

	mutex_unlock (&sled->ops_lock);
	return err;
}
EXPORT_SYMBOL_GPL(sled_store_stby_state);


int sled_read_duty (struct sled_device *sled, unsigned long *duty) {
	int err;

	err = mutex_lock_interruptible (&sled->ops_lock);
	if (err)
		return err;

	if (!sled->ops)
		err = -ENODEV;
	else if (!sled->ops->get_duty)
		err = -EINVAL;
	else {
		err = sled->ops->get_duty (sled, duty);
	}

	mutex_unlock (&sled->ops_lock);
	return err;
}
EXPORT_SYMBOL_GPL(sled_read_duty);


int sled_store_duty (struct sled_device *sled, unsigned long duty) {
	int err;

	err = mutex_lock_interruptible (&sled->ops_lock);
	if (err)
		return err;

	if (!sled->ops)
		err = -ENODEV;
	else if (!sled->ops->set_duty)
		err = -EINVAL;
	else {
		err = sled->ops->set_duty (sled, duty);
	}

	mutex_unlock (&sled->ops_lock);
	return err;
}
EXPORT_SYMBOL_GPL(sled_store_duty);


int sled_read_delay (struct sled_device *sled, unsigned long *delay) {
	int err;

	err = mutex_lock_interruptible (&sled->ops_lock);
	if (err)
		return err;

	if (!sled->ops)
		err = -ENODEV;
	else if (!sled->ops->get_delay)
		err = -EINVAL;
	else {
		err = sled->ops->get_delay (sled, delay);
	}

	mutex_unlock (&sled->ops_lock);
	return err;
}
EXPORT_SYMBOL_GPL(sled_read_delay);


int sled_store_delay (struct sled_device *sled, unsigned long delay) {
	int err;

	err = mutex_lock_interruptible (&sled->ops_lock);
	if (err)
		return err;

	if (!sled->ops)
		err = -ENODEV;
	else if (!sled->ops->set_delay)
		err = -EINVAL;
	else {
		err = sled->ops->set_delay (sled, delay);
	}

	mutex_unlock (&sled->ops_lock);
	return err;
}
EXPORT_SYMBOL_GPL(sled_store_delay);


int sled_read_stby_duty (struct sled_device *sled, unsigned long *duty) {
	int err;

	err = mutex_lock_interruptible (&sled->ops_lock);
	if (err)
		return err;

	if (!sled->ops)
		err = -ENODEV;
	else if (!sled->ops->get_stby_duty)
		err = -EINVAL;
	else {
		err = sled->ops->get_stby_duty (sled, duty);
	}

	mutex_unlock (&sled->ops_lock);
	return err;
}
EXPORT_SYMBOL_GPL(sled_read_stby_duty);


int sled_store_stby_duty (struct sled_device *sled, unsigned long duty) {
	int err;

	err = mutex_lock_interruptible (&sled->ops_lock);
	if (err)
		return err;

	if (!sled->ops)
		err = -ENODEV;
	else if (!sled->ops->set_stby_duty)
		err = -EINVAL;
	else {
		err = sled->ops->set_stby_duty (sled, duty);
	}

	mutex_unlock (&sled->ops_lock);
	return err;
}
EXPORT_SYMBOL_GPL(sled_store_stby_duty);


int sled_read_max_duty (struct sled_device *sled, unsigned long *max_duty) {
	int err;

	err = mutex_lock_interruptible (&sled->ops_lock);
	if (err)
		return err;

	if (!sled->ops)
		err = -ENODEV;
	else if (!sled->ops->get_max_duty)
		err = -EINVAL;
	else {
		err = sled->ops->get_max_duty (sled, max_duty);
	}

	mutex_unlock (&sled->ops_lock);
	return err;
}
EXPORT_SYMBOL_GPL(sled_read_max_duty);


int sled_read_max_delay (struct sled_device *sled, unsigned long *max_delay) {
	int err;

	err = mutex_lock_interruptible (&sled->ops_lock);
	if (err)
		return err;

	if (!sled->ops)
		err = -ENODEV;
	else if (!sled->ops->get_max_delay)
		err = -EINVAL;
	else {
		err = sled->ops->get_max_delay (sled, max_delay);
	}

	mutex_unlock (&sled->ops_lock);
	return err;
}
EXPORT_SYMBOL_GPL(sled_read_max_delay);


int sled_read_period (struct sled_device *sled, unsigned long *period) {
	int err;

	err = mutex_lock_interruptible (&sled->ops_lock);
	if (err)
		return err;

	if (!sled->ops)
		err = -ENODEV;
	else if (!sled->ops->get_period)
		err = -EINVAL;
	else {
		err = sled->ops->get_period (sled, period);
	}

	mutex_unlock (&sled->ops_lock);
	return err;
}
EXPORT_SYMBOL_GPL(sled_read_period);


int sled_store_period (struct sled_device *sled, unsigned long period) {
	int err;

	err = mutex_lock_interruptible (&sled->ops_lock);
	if (err)
		return err;

	if (!sled->ops)
		err = -ENODEV;
	else if (!sled->ops->set_period)
		err = -EINVAL;
	else {
		err = sled->ops->set_period (sled, period);
	}

	mutex_unlock (&sled->ops_lock);
	return err;
}
EXPORT_SYMBOL_GPL(sled_store_period);


int sled_read_max_period (struct sled_device *sled, unsigned long *max) {
	int err;

	err = mutex_lock_interruptible (&sled->ops_lock);
	if (err)
		return err;

	if (!sled->ops)
		err = -ENODEV;
	else if (!sled->ops->get_max_period)
		err = -EINVAL;
	else {
		err = sled->ops->get_max_period (sled, max);
	}

	mutex_unlock (&sled->ops_lock);
	return err;
}
EXPORT_SYMBOL_GPL(sled_read_max_period);


int sled_read_min_period (struct sled_device *sled, unsigned long *min) {
	int err;

	err = mutex_lock_interruptible (&sled->ops_lock);
	if (err)
		return err;

	if (!sled->ops)
		err = -ENODEV;
	else if (!sled->ops->get_min_period)
		err = -EINVAL;
	else {
		err = sled->ops->get_min_period (sled, min);
	}

	mutex_unlock (&sled->ops_lock);
	return err;
}
EXPORT_SYMBOL_GPL(sled_read_min_period);


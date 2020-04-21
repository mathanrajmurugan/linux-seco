
#ifndef _LINUX_SLED_H_
#define _LINUX_SLED_H_

#include <linux/types.h>
#include <linux/device.h>
#include <linux/seq_file.h>
#include <linux/cdev.h>
#include <linux/mutex.h>
#include <linux/pwm.h>

#define SLED_DEVICE_NAME_SIZE 50

#define SLED_OFF  0
#define SLED_ON   1


extern struct class *sled_class;

struct sled_device;


struct sled_class_ops {
	int (*open)(struct sled_device *);
	void (*release)(struct sled_device *);
	int (*set_enable)(struct sled_device *, int);
	int (*get_enable)(struct sled_device *, int *);
	int (*set_stby_state)(struct sled_device *, int);
	int (*get_stby_state)(struct sled_device *, int *);
	int (*set_duty)(struct sled_device *, unsigned long);
	int (*get_duty)(struct sled_device *, unsigned long *);
	int (*set_delay)(struct sled_device *, unsigned long);
	int (*get_delay)(struct sled_device *, unsigned long *);
	int (*set_stby_duty)(struct sled_device *, unsigned long);
	int (*get_stby_duty)(struct sled_device *, unsigned long *);
	int (*get_max_duty)(struct sled_device *, unsigned long *);
	int (*get_max_delay)(struct sled_device *, unsigned long *);
	int (*set_period)(struct sled_device *, unsigned long);
	int (*get_period)(struct sled_device *, unsigned long *);
	int (*get_max_period)(struct sled_device *, unsigned long *);
	int (*get_min_period)(struct sled_device *, unsigned long *);
	int (*ioctl)(struct sled_device *, unsigned int, unsigned long);
};


struct led_pwm_data {
	struct pwm_device    *pwm;
	int                  id;
	int                  id_internal;
	unsigned int         period;
	unsigned int         period_min;
	unsigned int         period_max;
	unsigned int         lth_duty;
	unsigned long        duty;
	unsigned long        max_duty;
	unsigned long        delay;
	unsigned long        max_delay;
	int                  enable;
	unsigned long        stby_duty;
	int                  stby_en;
};

#define SLED_NAME_GET          _IOR('p', 0x01, char *)
#define SLED_TYPE_GET          _IOR('p', 0x02, int *)
#define SLED_ENABLE_SET        _IOW('p', 0x03, int *)
#define SLED_ENABLE_GET        _IOR('p', 0x04, int *)
#define SLED_DUTY_SET          _IOW('p', 0x05, unsigned long *)
#define SLED_DUTY_GET          _IOR('p', 0x06, unsigned long *)
#define SLED_MAX_DUTY_GET      _IOR('p', 0x07, unsigned long *)
#define SLED_MIN_DUTY_SET      _IOR('p', 0x08, unsigned long *)
#define SLED_PERIOD_SET        _IOW('p', 0x09, unsigned long *)
#define SLED_PERIOD_GET        _IOR('p', 0x0a, unsigned long *)
#define SLED_MAX_PERIOD_GET    _IOR('p', 0x0b, unsigned long *)
#define SLED_MIN_PERIOD_GET    _IOR('p', 0x0c, unsigned long *)
#define SLED_STBY_ENABLE_SET   _IOW('p', 0x0d, int)
#define SLED_STBY_ENABLE_GET   _IOR('p', 0x0e, int *)
#define SLED_STBY_DUTY_SET     _IOW('p', 0x0f, unsigned long)
#define SLED_STBY_DUTY_GET     _IOR('p', 0x10, unsigned long *)
#define SLED_DELAY_SET         _IOW('p', 0x11, unsigned long *)
#define SLED_DELAY_GET         _IOR('p', 0x12, unsigned long *)
#define SLED_MAX_DELAY_GET     _IOR('p', 0x13, unsigned long *)
#define SLED_MIN_DELAY_SET     _IOR('p', 0x14, unsigned long *)


extern struct sled_device *sled_device_register(const char *name,
					struct device *dev,
					const struct sled_class_ops *ops,
					int led_type,
					struct led_pwm_data *pwm_data,
					struct module *owner);

extern void sled_device_unregister(struct sled_device *sled);


#define  LED_TYPE_PWM  1
#define  LED_TYPE_BIT  2

#define LED_LABEL_PWM  "pwm_led"
#define LED_LABEL_BIT  "bit_led"


#define to_sled_device(d) container_of(d, struct sled_device, dev)
#define to_sled_device_from_parent(d) to_sled_device(container_of(d, struct device, parent))
#define sled_to_device_driver(x)   ((x)->dev.parent)

struct sled_device {
	struct device                 dev;
	struct module                 *owner;

	int                           id;
	char                          name[SLED_DEVICE_NAME_SIZE];
	int                           led_type;
	int                           led_enable;
	int                           led_stby_state;

	const struct sled_class_ops   *ops;
	struct mutex                  ops_lock;

	struct cdev                   char_dev;
	unsigned long                 flags;

	struct led_pwm_data           *led_pwm;
};



#endif  /*  _LINUX_SLED_H  */

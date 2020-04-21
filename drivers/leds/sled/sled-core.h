
#ifndef _LINUX_SLED_CORE_H_
#define _LINUX_SLED_CORE_H_

extern void __init sled_dev_init(void);
extern void __exit sled_dev_exit(void);
extern void sled_dev_prepare(struct sled_device *sled);
extern void sled_dev_add_device(struct sled_device *sled);
extern void sled_dev_del_device(struct sled_device *sled);


extern void __init sled_sysfs_init(struct class *);
extern int  sled_sysfs_add_device(struct sled_device *sled);
extern void sled_sysfs_del_device(struct sled_device *sled);


extern int sled_read_enable (struct sled_device *sled, int *enable);
extern int sled_store_enable (struct sled_device *sled, int enable);
extern int sled_read_stby_state (struct sled_device *sled, int *enable);
extern int sled_store_stby_state (struct sled_device *sled, int enable);
extern int sled_read_duty (struct sled_device *sled, unsigned long *duty);
extern int sled_store_duty (struct sled_device *sled, unsigned long duty);
extern int sled_read_delay (struct sled_device *sled, unsigned long *delay);
extern int sled_store_delay (struct sled_device *sled, unsigned long delay);
extern int sled_read_stby_duty (struct sled_device *sled, unsigned long *duty);
extern int sled_store_stby_duty (struct sled_device *sled, unsigned long duty);
extern int sled_read_max_duty (struct sled_device *sled, unsigned long *max_duty);
extern int sled_read_max_delay (struct sled_device *sled, unsigned long *max_delay);
extern int sled_read_period (struct sled_device *sled, unsigned long *period);
extern int sled_store_period (struct sled_device *sled, unsigned long period);
extern int sled_read_max_period (struct sled_device *sled, unsigned long *max);
extern int sled_read_min_period (struct sled_device *sled, unsigned long *min);

#endif  /*  _LINUX_SLED_CORE_H_  */

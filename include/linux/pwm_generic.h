/*
 * Generic PWM data - see drivers/seco/pwm_generic.h
 */
#ifndef __LINUX_PWM_GENERIC_H
#define __LINUX_PWM_GENERIC_H


#define MAX_PERIOD_NS 50000000    // 50 ms
#define MIN_PERIOD_NS 150	  // 150 ns

struct platform_pwmg_generic_data {
	int            pwm_id;
	unsigned int   max_duty;
	unsigned int   dft_duty;
	unsigned int   lth_duty;
	unsigned int   pwm_period_ns;
	unsigned int   period_ns_max;
	unsigned int   period_ns_min;
	void           *reserved;
	unsigned int   enable;
};

#endif

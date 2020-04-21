#ifndef __LINUX_I2C_ECTRL_H
#define __LINUX_I2C_ECTRL_H

#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/ioctl.h>
#include <linux/miscdevice.h>

/* linux/i2c/ectrl.h */


enum wdt_event {
	WDT_EVNT_WDOUT          = 0,
	WDT_EVNT_RESET          = 1,
	WDT_EVNT_PWRBTN1SEC     = 2,
	WDT_EVNT_PWRBTN4SEC     = 3,
	WDT_EVNT_BOOT_RECOVERY  = 4,
};


enum ECTRL_EVENTS {
	EVNT_PWR_BTN, EVNT_RST_BTN, EVNT_SLEEP,
	EVNT_BATTERY, EVNT_LID, EVNT_WAKE
};


struct data_list {
	u16 data;
	struct list_head list;
};

enum {
	STOP_OP = (unsigned short int)0, 	// stop all operations
	R_OP    = (unsigned short int)1,	// read register operation
	W_OP    = (unsigned short int)2,	// write register operation
	RVE_OP  = (unsigned short int)3,	// read vector's element operation
	WVE_OP  = (unsigned short int)4,	// write vector's element operation
};

struct ectrl_reg {
	u16 addr;
	u16 data;
};

struct ectrl_reg_rw {
	unsigned short int op;
	struct ectrl_reg reg;
};

#endif           /*  __LINUX_I2C_ECTRL_H  */

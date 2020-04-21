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
	WDT_EVNT_PWRCYCLE	= 2,
	WDT_EVNT_COMP_PWRCYCLE  = 3,
	WDT_EVNT_BOOT_RECOVERY  = 4,
};


enum ECTRL_EVENTS {
	EVNT_FAIL_BV,
	EVNT_FAIL_WD,
	EVNT_BATLOW_SIGNAL,
	EVNT_SLEEP_SIGNAL,
	EVNT_LID_SIGNAL,
	EVNT_PWR_BUTTON,
	EVNT_FAIL_PWGIN,
	EVNT_WAKE_EN,

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

enum BOOT_DEVICE {
        BOOT_uSD  		=  0,
        BOOT_eMMC 		=  1,
        BOOT_extSD 		=  2,
        BOOT_NUM_OF_DEV     	=  3,
};

struct ectrl_straps_define {
	u8	id;
        char	*label;
        u16	strap_l;
	u16     strap_h;
};

static struct ectrl_straps_define straps_definition_imx8m[] = {

        {
                .id             =       BOOT_uSD,
                .label          =       "uSD card",
                .strap_l        =       0b00010010,
                .strap_h        =       0b00010100,
        },
        {
                .id             =       BOOT_eMMC,
                .label          =       "eMMC",
                .strap_l        =       0b00100110,
                .strap_h        =       0b00100000,
        },

};

static struct ectrl_straps_define straps_definition_imx8qm[] = {

        {
                .id             =       BOOT_uSD,
                .label          =       "uSD card",
                .strap_l        =       0b00001100,
                .strap_h        =       0b00000000,
        },
        {
                .id             =       BOOT_eMMC,
                .label          =       "eMMC",
                .strap_l        =       0b00001000,
                .strap_h        =       0b00000000,
        },
        {
                .id             =       BOOT_extSD,
                .label          =       "extSD",
                .strap_l        =       0b00001110,
                .strap_h        =       0b00000000,
        },

};

#endif           /*  __LINUX_I2C_ECTRL_H  */


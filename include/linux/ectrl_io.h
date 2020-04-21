
#ifndef __LINUX_I2C_ECTRL_IO_H
#define __LINUX_I2C_ECTRL_IO_H


#include <linux/ioctl.h>
#include "ectrl.h"


enum ECTRL_EVENT_ID;
struct ectrl_ev_state;

enum ECTRL_POWER_STATE;

enum ECTRL_BOOTDEV_ID;
struct ectrl_boot_device;

enum wdt_event;
struct wdt_event_data;
struct ectrl_wdt;


#define ECTRL_IOCTL_REG_READ	          _IOWR('o', 1, struct ectrl_reg)
#define ECTRL_IOCTL_REG_WRITE	          _IOR('o', 2, struct ectrl_reg)
/*  EVENT  */
#define ECTRL_EV_STATE_GET                _IOWR('o', 3, struct ectrl_ev_state *)
#define ECTRL_EV_STATE_SET                _IOR('o', 4, struct ectrl_ev_state *)
/*  POWER MANAGEMENT  */
#define ECTRL_PWR_STATE_GET               _IOW('o', 5, enum ECTRL_POWER_STATE *)
#define ECTRL_PWR_STATE_SET               _IOR('o', 6, enum ECTRL_POWER_STATE *)
#define ECTRL_PWR_BTN_4SEC_STATE_GET      _IOW('o', 7, struct ectrl_ev_state *)
#define ECTRL_PWR_BTN_4SEC_STATE_SET      _IOR('o', 8, struct ectrl_ev_state *)
/*  BOOT MANAGEMENT  */
#define ECTRL_BOOT_NUM_DEV_GET            _IOW('o', 9, int *)
#define ECTRL_BOOT_DEV_LIST_GET           _IOW('o', 10, struct ectrl_boot_device *)
#define ECTRL_BOOT_DEV_1_GET              _IOW('o', 11, struct ectrl_boot_device *)
#define ECTRL_BOOT_DEV_2_GET              _IOW('o', 12, struct ectrl_boot_device *)
#define ECTRL_BOOT_DEV_3_GET              _IOW('o', 13, struct ectrl_boot_device *)
#define ECTRL_BOOT_DEV_RECOVERY_GET       _IOW('o', 14, struct ectrl_boot_device *)
#define ECTRL_BOOT_DEV_1_SET              _IOR('o', 15, enum ECTRL_BOOTDEV_ID *)
#define ECTRL_BOOT_DEV_2_SET              _IOR('o', 16, enum ECTRL_BOOTDEV_ID *)
#define ECTRL_BOOT_DEV_3_SET              _IOR('o', 17, enum ECTRL_BOOTDEV_ID *)
#define ECTRL_BOOT_DEV_RECOVERY_SET       _IOR('o', 18, enum ECTRL_BOOTDEV_ID *)
/*  WATCHDOG  */
#define ECTRL_WDT_NUM_EVENT_GET           _IOW('o', 19, int *)
#define ECTRL_WDT_EVENT_LIST_GET          _IOW('o', 20, struct wdt_event_data *)
#define ECTRL_WDT_GET                     _IOW('o', 21, struct ectrl_wdt *)
#define ECTRL_WDT_SET                     _IOR('o', 21, struct ectrl_wdt *)
#define ECTRL_WDT_REFRESH                 _IO('o', 22)
#define ECTRL_WDT_RESTORE                 _IO('o', 23)




/*  EVENT  */

enum ECTRL_EVENT_ID {
	PWR_BUTTON        =  0,
	RST_BUTTON        =  1,
	SLEEP_SIGNAL      =  2,
	BATLOW_HL_SIGNAL  =  3,	// when BATLOW becomes HIGH - battery charge low
	BATLOW_LH_SIGNAL  =  4,	// when BATLOW becomes LOW - battery full
	LID_HL_SIGNAL     =  5,	// when LID becomes HIGH
	LID_LH_SIGNAL     =  6,	// when LID becomes LOW
	WAKE_SIGNAL       =  7,
	PWR_BTN_4SEC      =  8,
};


struct ectrl_ev_state {
	enum ECTRL_EVENT_ID  id;
	int                  v_enable;
	int                  f_enable;
};


/*  POWER MANAGEMENT  */

enum ECTRL_POWER_STATE {
	PWR_ALWAYS_OFF,
	PWR_ALWAYS_ON,
};


/*  BOOT MANAGEMENT  */

enum ECTRL_BOOTDEV_ID {
	BOOT_USDHC4  = (u8)0,
	BOOT_USBHC1  = (u8)1,
	BOOT_EMMC    = (u8)2,
	BOOT_SPI     = (u8)3,
};


#define BOOTDEV_NAME_MAX_LEN  30
struct ectrl_boot_device {
	enum ECTRL_BOOTDEV_ID   id;
	char                    label[BOOTDEV_NAME_MAX_LEN];
};


/*  WATCHDOG  */

#define WDT_NAME_MAX_LEN  30
struct wdt_event_data {
	enum wdt_event id;
	char           label[WDT_NAME_MAX_LEN];

};


struct ectrl_wdt {
	/*  volatile settings  */
	int            v_enable;
	enum wdt_event v_event;
	u16            v_delay_sec;
	u16            v_timeout_sec;
	/*  permanent settings  */
	int            f_enable;
	enum wdt_event f_event;
	u16            f_delay_sec;
	u16            f_timeout_sec;
	/*  run time state  */
	u16            timer_delay;
	u16            timer_wd;
};


#endif             /*  __LINUX_I2C_ECTRL_IO_H  */

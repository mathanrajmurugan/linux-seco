#include <linux/module.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/input.h>
#include <linux/i2c.h>
#include <linux/proc_fs.h>
#include <linux/platform_device.h>
#include <linux/seq_file.h>
#include <linux/delay.h>
#include <linux/list.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/workqueue.h>
#include <linux/reboot.h>
#include <linux/of_gpio.h>


#include <linux/ectrl_stm32.h>
#include <linux/ectrl_io_stm32.h>
#include <dt-bindings/seco/ectrl_stm32.h>


#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>

/* New */
#define FW_ID_REV_REG_L			0x00
#define FW_ID_REV_REG_H                 0x01
/* Reprogramming */
#define FW_REPROG_L			0x02	
#define FW_REPROG_H			0x03
#define PW_ON_CFG			0x04
#define PW_OFF_CFG			0x05
#define REBOOT_CFG			0x06
#define STDBY_CFG			0x07
#define RESET_CFG			0x08
#define WDT_REG_1			0x09
#define WDT_CONFIG_REG			WDT_REG_1
#define WDT_REG_2			0x0a
#define WDT_DELAY_REG			WDT_REG_2
#define WDT_TIMEOUT_REG			0x2b
#define BOOT_CFG_1			0x0b
#define BOOT_CFG_2			0x0c

/* STRAPS */
#define BOOT0_REG			BOOT_0_L 
#define BOOT_0_L			0x0d
#define BOOT_0_H			0x0e
#define BOOT1_REG			BOOT_1_L
#define BOOT_1_L                        0x0f
#define BOOT_1_H                        0x10
#define BOOT2_REG			BOOT_2_L
#define BOOT_2_L                        0x11
#define BOOT_2_H                        0x12
#define BOOT3_REG			BOOT_3_L
#define BOOT_3_L                        0x13
#define BOOT_3_H                        0x14
#define BOOT_DEF_REG			BOOT_DEF_L
#define BOOT_DEF_L			0x15
#define BOOT_DEF_H			0x16
#define BOOT_CARRIER_SATA_L		0x17
#define BOOT_CARRIER_SATA_H		0x18
#define BOOT_CARRIER_SDCARD_L		0x19
#define BOOT_CARRIER_SDCARD_H		0x1a
#define BOOT_CARRIER_EMMC_L		0x1b
#define BOOT_CARRIER_EMMC_H		0x1c
#define BOOT_CARRIER_SPI_L		0x1d
#define BOOT_CARRIER_SPI_H		0x1e
#define BOOT_MODULE_MODULE_L		0x1f
#define BOOT_MODULE_MODULE_H		0x20
#define BOOT_REMOTE_L			0x21
#define BOOT_REMOTE_H			0x22
#define BOOT_MODULE_EMMC_L		0x23
#define BOOT_MODULE_EMMC_H              0x24
#define BOOT_MODULE_SPI_L		0x25
#define BOOT_MODULE_SPI_H		0x26
#define CICLE_DAY			0x27
#define STATUS_REG_L			0x28
#define STATUS_REG_H			0x29
#define WAKE_CFG			0x2a

/* Commands - write address without data for enable / disable specific functions */
#define CMD_POWER_OFF			0x80
#define CMD_REBOOT			0x81
#define CMD_STANDBY			0x82
#define CMD_WD_WDI			0x83
#define CMD_WD_EN			0x84
#define CMD_WD_DIS			0x85
#define CMD_BV_EN			0x86
#define CMD_BV_DIS			0x87
#define CMD_CONF_SAVE			0x88
#define CMD_RESTORE_DEF			0x89
#define CMD_JUMP_BOOTLOADER		0x8a
#define CMD_BV				0x8b
#define CMD_CLEAR_FLAGRST		0x8c
#define CMD_CLEAR_FLAGREBOOT		0x8d
#define CMD_CLEAR_FLAGBATLOW		0x8e
#define CMD_CLEAR_FLAGEXT_SLEEP		0x8f
#define CMD_CLEAR_FLAGLID		0x90

#define NOT_IMPLEMENTED			printk(KERN_ERR "%s: Not implemented\n" , __func__ )

/* Old */
#define WDT_CTRL_REG                   0x00
#define GINO_TASK_REG                  0x01
#define DVI2LVDS_FLAGS_REG             0x02
#define DATA_REG                       0x03
#define INDEX_REG                      0x04
/*#define WDT_DELAY_REG                  0x05*/
/*#define WDT_TIMEOUT_REG                0x06*/
#define WDT_TIMER1_REG                 0x07
#define WDT_TIMER2_REG                 0x08
/*#define WDT_CONFIG_REG                 0x09*/
#define ADC_READING_0_REG              0x0A
#define ADC_READING_1_REG              0x0B
#define BUILDREV_REG                   0x0C
#define FLAG_REG                       0x0F 
/*#define STATUS_REG                     0x11*/

/* Flash registers */
#define ALWAYS_STATE_REG               0x01
#define WDT_F_DELAY_REG_LSB            0x0A
#define WDT_F_DELAY_REG_MSB            0x0B
#define WDT_F_TIMEOUT_REG_LSB          0x0C
#define WDT_F_TIMEOUT_REG_MSB          0x0D
#define WDT_F_CONFIG_REG_LSB           0x0E
#define WDT_F_CONFIG_REG_MSB           0x0F
/*
#define BOOT0_REG                      0x10
#define BOOT1_REG                      0x11
#define BOOT2_REG                      0x12
#define BOOT3_REG                      0x13
*/
#define EN_FLASH_REG_LSB               0x14
#define EN_FLASH_REG_MSB               0x15

#define SBLOCK_CMD                     0x55AA
#define HALT_CMD                       0x6101
#define REBOOT_CMD                     0x6505

#define ECTRL_POLL_PERIOD                   2  // (ms)
#define ECTRL_POLL_PERIOD_SNOOPER_PWR_BTN   50 // (ms)

#define MAX_LEN_NAME                   64
#define MAX_LEN_LABEL                  32

/* Event Checked for stm32 */
#define ENABLE_OFFSET			8
#define EVENT_MASK			0xff

#define FAIL_BV				0
#define FAIL_WD				1
#define BATLOW_SIGNAL			2
#define SLEEP_SIGNAL			3
#define LID_SIGNAL			4
#define PWR_BUTTON			5
#define FAIL_PWGIN			6
/* No signal */
#define WAKE_EN				7


#define FAIL_BV_MASK_REG		0x0001
#define FAIL_WD_MASK_REG		0x0002
#define BATLOW_SIGNAL_MASK_REG		0x0004
#define SLEEP_SIGNAL_STATUS_MASK_REG	0x0008
#define LID_SIGNAL_STATU_REG_MASK_REG	0x0010
#define PWR_BUTTON_MASK_REG		0x0020
#define FAIL_PWGIN_MASK_REG		0x0040
/* WAKE_CFG enable register */
#define LID_SIGNAL_WAKE_CFG_MASK_REG	0x0100
#define SLEEP_SIGNAL_WAKE_CFG_MASK_REG	0x0200
#define WAKE_EN_WAKE_CFG_MASK_REG	0x0400

#define SLEEP_SIGNAL_MASK_REG		SLEEP_SIGNAL_WAKE_CFG_MASK_REG | SLEEP_SIGNAL_STATUS_MASK_REG
#define LID_SIGNAL_MASK_REG		LID_SIGNAL_WAKE_CFG_MASK_REG | LID_SIGNAL_STATU_REG_MASK_REG
#define WAKE_EN_MASK_REG		WAKE_EN_WAKE_CFG_MASK_REG


#define FAIL_BV_SHIFT_REG			0x0
#define FAIL_WD_SHIFT_REG			0x1
#define BATLOW_SIGNAL_SHIFT_REG			0x2
#define SLEEP_SIGNAL_STATUS_REG_SHIFT_REG	0x3
#define LID_SIGNAL_STATUS_REG_SHIFT_REG		0x4
#define PWR_BUTTON_SHIFT_REG			0x5
#define FAIL_PWGIN_SHIFT_REG			0x8
/* WAKE_CFG enable register */
#define LID_SIGNAL_WAKE_CFG_SHIFT_REG		0x000
#define SLEEP_SIGNAL_WAKE_CFG_SHIFT_REG		0x100
#define WAKE_EN_WAKE_CFG_SHIFT_REG		0x200

#define LID_SIGNAL_SHIFT_REG			LID_SIGNAL_WAKE_CFG_SHIFT_REG | LID_SIGNAL_STATUS_REG_SHIFT_REG
#define SLEEP_SIGNAL_SHIFT_REG			SLEEP_SIGNAL_STATUS_REG_SHIFT_REG | SLEEP_SIGNAL_WAKE_CFG_SHIFT_REG
#define WAKE_EN_SHIFT_REG			WAKE_EN_WAKE_CFG_SHIFT_REG


#define FAIL_BV_NAME                    "FailBootValidate"
#define FAIL_WD_NAME                    "WatchdogTimeOut"
#define BATLOW_SIGNAL_NAME              "BatteryLow"
#define SLEEP_SIGNAL_NAME               "sleep"
#define LID_SIGNAL_NAME                 "LidSignal"
#define PWR_BUTTON_NAME                 "PowerButton"
#define FAIL_PWGIN_NAME			"FailPowerGoodIn"
/* No signal */
#define WAKE_EN_NAME                 "wake_en"

#define FAIL_BV_LABEL                   "fail_bootvalidate"
#define FAIL_WD_LABEL                   "watchdog_timeOut"
#define BATLOW_SIGNAL_LABEL             "battery_low"
#define SLEEP_SIGNAL_LABEL              "sleep"
#define LID_SIGNAL_LABEL                "lid_signal"
#define PWR_BUTTON_LABEL                "power_button"
#define FAIL_PWGIN_LABEL                "fail_powergoodin"
#define WAKE_EN_LABEL			"wake_en"


#define EVNT_FAIL_BV_STATE_LABEL	"fail_bootvalidate"
#define EVNT_FAIL_WD_STATE_LABEL	"watchdog_timeOut"
#define EVNT_BATLOW_SIGNAL_STATE_LABEL	"battery_low"
#define EVNT_SLEEP_SIGNAL_STATE_LABEL	"sleep"
#define EVNT_LID_SIGNAL_STATE_LABEL	"lid"
#define EVNT_PWR_BUTTON_STATE_LABEL	"power_button"
#define EVNT_FAIL_PWGIN_STATE_LABEL	"fail_powergoodin"
/* No signal */
#define EVNT_WAKE_EN_STATE_LABEL	"wake_en"


#define PM_STATE_ALWAYS_ON             "always_on"
#define PM_STATE_ALWAYS_OFF            "always_off"

#define ALWAYS_ON                      0x0055
#define ALWAYS_OFF                     0x0000 


#define WDT_SHIFT_ENABLE               0
#define WDT_SHIFT_EVENT                1

#define WDT_MASK_ENABLE                (0x0001 << WDT_SHIFT_ENABLE)
#define WDT_MASK_EVENT                 (0x0003 << WDT_SHIFT_EVENT)

#define WDT_CTRL_WDTOUT_RESET          (0x0001 << 8) // Auto Resettable Byte
#define WDT_CTRL_WDTOUT_TRIGGER        (0x0002 << 8) // Auto Resettable Byte

#define INPUT_DEV_NAME                 "Embedded Controller"

#define ECTRL_USE_OWN_STATE            0

static char ectrl_setup[10];
static int __init ectrl_config_setup(char *addrs) {

	strcpy(ectrl_setup,addrs);

	return 0;
}

/*  ID used to code the board  (read from Embedded Controller)  */
/* TODO */
#define BOARD_C12_ID                   0x12
#define BOARD_C25_ID                   0x25
#define BOARD_C26_ID                   0x26


#define DRV_VERSION                    "1.0"

#define ECTRL_INFO(fmt, arg...)        printk(KERN_INFO "Embedded Controller: " fmt "\n" , ## arg)
#define ECTRL_ERR(fmt, arg...)         printk(KERN_ERR "%s: " fmt "\n" , __func__ , ## arg)
#define ECTRL_DBG(fmt, arg...)         pr_debug("%s: " fmt "\n" , __func__ , ## arg)



/*  element index of the structures relative to the boards  */
#define BOARD_C12                      0
#define BOARD_C25                      1
#define BOARD_C26                      2


//static int board_nr_states [] = {
//	[BOARD_C12] = 6,
//	[BOARD_C25] = 6,
//	[BOARD_C26] = 2,
//};


static char board_name [][30] = {
	[BOARD_C12] = "SMARC C12",
	[BOARD_C25] = "QSEVEN C25",
	[BOARD_C26] = "QSEVEN C26",
};


/*  Maps the event to the relative index into the register 
 *  (-1 if the event is not available for the board)  
 */
static int board_reg_state_idx [][8] = {
	[BOARD_C12] = {
		[EVNT_FAIL_BV]		= 0,	
		[EVNT_FAIL_WD]		= 1,
		[EVNT_BATLOW_SIGNAL]	= 2,
		[EVNT_SLEEP_SIGNAL]	= 3,
		[EVNT_LID_SIGNAL]	= 4,
		[EVNT_PWR_BUTTON]	= 5,
		[EVNT_FAIL_PWGIN]	= 6,
	},
	[BOARD_C25] = {
		[EVNT_FAIL_BV]		= 0,	
		[EVNT_FAIL_WD]		= 1,
		[EVNT_BATLOW_SIGNAL]	= 2,
		[EVNT_SLEEP_SIGNAL]	= 3,
		[EVNT_LID_SIGNAL]	= 4,
		[EVNT_PWR_BUTTON]	= 5,
		[EVNT_FAIL_PWGIN]	= 6,
	},
	[BOARD_C26] = {
		[EVNT_FAIL_BV]		= 0,	
		[EVNT_FAIL_WD]		= 1,
		[EVNT_BATLOW_SIGNAL]	= 2,
		[EVNT_SLEEP_SIGNAL]	= 3,
		[EVNT_LID_SIGNAL]	= 4,
		[EVNT_PWR_BUTTON]	= 5,
		[EVNT_FAIL_PWGIN]	= 6,
		[EVNT_WAKE_EN]		= 7,
	},
};



struct event_dev {
	unsigned short int      id;
	char                    name[MAX_LEN_NAME];
	char                    label[MAX_LEN_LABEL];
	unsigned int            index_reg;
	unsigned int            shift;
	struct proc_dir_entry   *proc_dir; 
	struct input_dev        *input;
	char                    input_name[MAX_LEN_NAME];
};


#define EVENT_DEFINE(_event)						\
	[_event] = {							\
		.id          = _event,					\
		.name        = _event ## _NAME,				\
		.label       = _event ## _LABEL,			\
		.index_reg   = _event ## _MASK_REG,			\
		.shift       = _event ## _SHIFT_REG,			\
		.proc_dir    = NULL,					\
		.input       = NULL,					\
		.input_name  = INPUT_DEV_NAME " - " _event ## _LABEL	\
	}	


#define EVENT_NO_SIGNAL_DEFINE(_event)					\
	[_event] = {							\
		.id          = _event,					\
		.name        = _event ## _NAME,				\
		.label       = "",	   				\
		.index_reg   = _event ## _MASK_REG,			\
		.shift       = _event ## _SHIFT_REG,			\
		.proc_dir    = NULL,					\
		.input       = NULL,					\
		.input_name  = ""					\
	}	


/*  Main sructure list of all fasible events  */
static struct event_dev global_event_list [] = {
	EVENT_DEFINE(FAIL_BV), 
	EVENT_DEFINE(FAIL_WD),                         
	EVENT_DEFINE(BATLOW_SIGNAL),                   
	EVENT_DEFINE(SLEEP_SIGNAL),                    
	EVENT_DEFINE(LID_SIGNAL),                      
	EVENT_DEFINE(PWR_BUTTON),                      
	EVENT_DEFINE(FAIL_PWGIN),  
	EVENT_DEFINE(WAKE_EN),  
};


struct event_state {
	enum ECTRL_EVENTS    evn;
	int                  reg_idx;
	char                 *label;
};


#define EVENT_STATE_DEFINE(_event)			\
	[_event] = {					\
		.evn      = _event,			\
		.reg_idx  = -1,				\
		.label    = _event ## _STATE_LABEL	\
	}


static struct event_state event_state_list [] = {
	EVENT_STATE_DEFINE(EVNT_FAIL_BV), 
	EVENT_STATE_DEFINE(EVNT_FAIL_WD),                         
	EVENT_STATE_DEFINE(EVNT_BATLOW_SIGNAL),                   
	EVENT_STATE_DEFINE(EVNT_SLEEP_SIGNAL),                    
	EVENT_STATE_DEFINE(EVNT_LID_SIGNAL),                      
	EVENT_STATE_DEFINE(EVNT_PWR_BUTTON),                      
	EVENT_STATE_DEFINE(EVNT_FAIL_PWGIN),  
	EVENT_STATE_DEFINE(EVNT_WAKE_EN),  
};  



static char *PM_PWR_STATE[] = {
	PM_STATE_ALWAYS_OFF,
	PM_STATE_ALWAYS_ON,
};


enum BOOTDEV_ID {
	BOOTDEV_ID0  =  (u8)0,
	BOOTDEV_ID1  =  (u8)1,
	BOOTDEV_ID2  =  (u8)2,
	BOOTDEV_ID3  =  (u8)3,
	BOOTDEV_ID4  =  (u8)4,
};

enum BOOT_STRAPS {
        BOOT_STRAPS_CARRIER  =  0x0,
        BOOT_STRAPS_DEF_REG  =  0x1,
        BOOT_STRAPS_0x_REG   =  0x2,
	BOOT_STRAPS_MASK     =  0x3,
};

enum MAX_INDEX {
	MAX_INDEX_1 	= 0x10,
	MAX_INDEX_2	= 0x20,
	MAX_INDEX_3	= 0x30,
};

struct bootstraps {
	enum BOOT_STRAPS	id;
	const char		*label;	
	u16 (*getStraps)	(struct i2c_client *client, u16 reg);	
};

static inline u16 ectrl_read_double_element (struct i2c_client *client, u16 addr);

struct bootstraps boot_straps_list[] = {
	{
		.id = BOOT_STRAPS_CARRIER,
		.label = "carrier",
		.getStraps = NULL,
	},
	{
		.id = BOOT_STRAPS_DEF_REG,
                .label = "default",
		.getStraps = ectrl_read_double_element,

	},	
	{
                .id = BOOT_STRAPS_0x_REG,
                .label = "devboot",
		.getStraps = ectrl_read_double_element,

        },

};

struct bootdev {
        enum BOOTDEV_ID   id;
        const char        *label;
};


#define BOOT_IDX0   		0
#define BOOT_IDX1   		1
#define BOOT_IDX2   		2
#define BOOT_IDX3   		3
#define BOOT_STRAPS_DEV_SEL	4


struct wdt_event_element {
	enum wdt_event id;
	char           *label;

};


#define WDT_EVENT_DEFINE(wdt_evnt, name)	\
	[wdt_evnt] = {				\
		.id = wdt_evnt,			\
		.label = name,			\
	}


static struct wdt_event_element wdt_evnt_list [] = {
	WDT_EVENT_DEFINE(WDT_EVNT_WDOUT, "wdout"),
	WDT_EVENT_DEFINE(WDT_EVNT_RESET, "reset"),
	WDT_EVENT_DEFINE(WDT_EVNT_PWRCYCLE, "power cycle"),
	WDT_EVENT_DEFINE(WDT_EVNT_COMP_PWRCYCLE, "complete power cycle"),
	WDT_EVENT_DEFINE(WDT_EVNT_BOOT_RECOVERY, "boot_recovery"),
};


/*  Main structure, used by whole driver  */
struct econtroller {
	/*  I2C client  */
	struct i2c_client             *client;
	/*  board identificator  */
	int                           board_id;
	/*  list of all feasible events for the current board  */
	struct event_dev              **events;
	int                           nr_evnt;

	struct event_state            **evn_state;
	int                           nr_evn;
	struct bootdev                *bootdev_list;
	int                           nr_bootdev;
	struct wdt_event_element      *wdt_event_list;
	int                           nr_wdt_event;
	void 		              (*task_pre_halt_signal) (void);
	void                          (*task_post_halt_signal) (void);
	int		              irq;
	unsigned int                  irq_flags;
	char                          *phys;
	struct delayed_work           work;
	struct delayed_work           work_snooper_pwr_btn;
	unsigned long                 poll_period;
	unsigned long                 poll_period_snooper_pwr_btn;
	unsigned long                 orig_jiffies; //used only for the power button snooping
	struct mutex                  fs_lock;
	struct mutex                  ioctl_lock;
	int			      rb_poff_gpio;
	struct ectrl_straps_define    *straps_def;
	unsigned long		      straps_def_size;
	bool			      input_interface;
};


struct ectrl_proc_event {
	struct econtroller  *ectrl;
	int                 event;
	int                 is_true_event : 1;
};


struct ectrl_proc_event_state {
	struct econtroller  *ectrl;
	int                 state_id;
};


struct ectrl_proc_boot {
	struct econtroller   *ectrl;
	int                  idx;
};


struct snooper_work {
	struct econtroller   *ectrl;
	unsigned long        orig_jiffies;
};


static struct econtroller *ectrl;



static struct ectrl_reg_rw reg_halt[] = {
	{
		.op   = WVE_OP,
		.reg  = {
				.addr = SBLOCK_CMD,
				.data = 0x0001,

			},
	},
	{
		.op   = WVE_OP,
		.reg  = { 
				.addr = HALT_CMD,
				.data = 0x0001,
			},
	},
	{
		.op   = STOP_OP,
		.reg  = {
				.addr = 0,
				.data = 0,
			},
	},
};

static struct ectrl_reg_rw reg_reboot[] = {
	{
		.op   = WVE_OP,
		.reg  = {
				.addr = SBLOCK_CMD,
				.data = 0x0001,
			},
	},
	{
		.op   = WVE_OP,
		.reg  = {
				.addr = REBOOT_CMD,
				.data = 0x0001,
			},
	},
	{
		.op   = STOP_OP,
		.reg  = {
				.addr = 0,
				.data = 0,
			},
	},
};

static struct ectrl_reg_rw reg_en_flash[] = {
	{
		.op   = WVE_OP,
		.reg  = {
				.addr = SBLOCK_CMD,
				.data = 0x0001,
			},
	},
	{
		.op   = WVE_OP,
		.reg  = {
				.addr = EN_FLASH_REG_LSB,
				.data = 0x0,
			},
	},
	{
		.op   = WVE_OP,
		.reg  = {
				.addr = SBLOCK_CMD,
				.data = 0x0001,
			},
	},
	{
		.op   = WVE_OP,
		.reg  = {
				.addr = EN_FLASH_REG_MSB,
				.data = 0x0,
			},
	},

	{
		.op   = STOP_OP,
		.reg  = {
				.addr = 0,
				.data = 0,
			},
	},
};

static struct ectrl_reg_rw reg_pm_always_state[] = {
	{
		.op   = WVE_OP,
		.reg  = {
				.addr = SBLOCK_CMD,
				.data = 0x0001,
			},
	},
	{
		.op   = WVE_OP,
		.reg  = {
				.addr = ALWAYS_STATE_REG,
				.data = 0x0,
			},
	},
	{
		.op   = STOP_OP,
		.reg  = {
				.addr = 0,
				.data = 0,
			},
	},
};



/* --------------------------------------------------------------------------
                                 BASIC FUNCTIONS
   -------------------------------------------------------------------------- */


static inline u16 ectrl_read_data (struct i2c_client *client, u16 addr) {
	s32 data;
	u16 val;
	data = i2c_smbus_read_word_data(client, (u8)addr);
	if (data < 0) {
		dev_err(&client->dev, "i2c io (read) error: %d\n", data);
		return data;
	}
	val = (u16)data;
	dev_dbg(&client->dev, "data: 0x%x, val: 0x%x\n", data, val);
	return val;
}


static inline u16 ectrl_write_data (struct i2c_client *client, u16 addr, u16 data) {
	return i2c_smbus_write_word_data(client, (u8)addr, data);
}


static inline u16 ectrl_read_vector_element (struct i2c_client *client, u16 addr) {
	int retval = ectrl_write_data (client, INDEX_REG, addr);
	if (!(retval < 0)) {
		retval = ectrl_read_data (client, DATA_REG);
	}
	return (u16)retval;
}


static inline u16 ectrl_write_vector_element (struct i2c_client *client, u16 addr, u16 data) {
	int retval = ectrl_write_data (client, DATA_REG, data);
	if (!(retval < 0)) {
		retval = ectrl_write_data (client, INDEX_REG, addr);
	}
	return (u16)retval;
}

static inline u16 ectrl_read_double_element (struct i2c_client *client, u16 addr) {
	int retval_l,retval_h;
	retval_l = ectrl_read_data (client, addr) & 0xff;
        if ((retval_l < 0)) {
		dev_err(&client->dev, "read data failed\n");
        }
	retval_h = ectrl_read_data (client, addr + 0x1) & 0xff;
        if ((retval_h < 0)) {
                dev_err(&client->dev, "read data failed\n");
        }
	
        return ((u16)retval_l  | ((u16)retval_h << 8));
}

static inline u16 ectrl_write_double_element (struct i2c_client *client, u16 addr_l, u16 addr_h, u16 data_l, u16 data_h) {
        int retval_l,retval_h;
        retval_l = ectrl_write_data (client, addr_l, data_l);
        if ((retval_l < 0)) {
                dev_err(&client->dev, "write data failed\n");
        }
        retval_h = ectrl_write_data (client, addr_h, data_h);
        if ((retval_h < 0)) {
                dev_err(&client->dev, "write data failed\n");
        }

        return ((u16)retval_l  | ((u16)retval_h << 8));
}

static int ectrl_mem_single_op (struct i2c_client *client, struct ectrl_reg_rw *reg_rw) {
	int retval = 0;
	
	u16 val;
	switch (reg_rw->op) {
		case R_OP:
			val = ectrl_read_data (client, reg_rw->reg.addr);
			if (val >= 0) {
				dev_dbg(&client->dev, "read data done: 0x%x\n", val);
				reg_rw->reg.data = val;	
				retval = 1;
			} else {
				dev_err(&client->dev, "read data failed: 0x%x\n", val);
				retval = -1;
			}
			break;
		case W_OP:
			val = ectrl_write_data (client, reg_rw->reg.addr, reg_rw->reg.data);
			if (val >= 0) {
				dev_dbg(&client->dev, "write data done: addr 0x%x  data 0x%x\n", reg_rw->reg.addr, reg_rw->reg.data);
				retval = 0;
			} else {
				dev_err(&client->dev, "write data failed: addr 0x%x  data 0x%x\n", reg_rw->reg.addr, reg_rw->reg.data);
				retval = -1;
			}
			break;
		case RVE_OP:
			val = ectrl_read_vector_element (client, reg_rw->reg.addr);
			if (val >= 0) {
				dev_dbg(&client->dev, "read vector element done: 0x%x", val);
				reg_rw->reg.data = val;
				retval = 1;
			} else {
				dev_err(&client->dev, "read vector's element failed: 0x%x\n", val);
				retval = -1;
			}
			break;
		case WVE_OP:
			val = ectrl_write_vector_element (client, reg_rw->reg.addr, reg_rw->reg.data);
			if (val >= 0) {
				dev_dbg(&client->dev, "write vector's element done: addr 0x%x  data 0x%x\n", reg_rw->reg.addr, reg_rw->reg.data);
				retval = 1;		
			} else {
				dev_err(&client->dev, "write vector's element failed: addr 0x%x  data 0x%x\n", reg_rw->reg.addr, reg_rw->reg.data);
				retval = -1;
			}
			mdelay (75);  // needed to access to the flash of the embedded controller
			break;
		default:
			dev_dbg(&client->dev, "invalid operation!\n");
	 		retval = -1;
	}
	return retval;
}


static int ectrl_mem_op (struct i2c_client *client, struct ectrl_reg_rw regs[], struct data_list *data_l) {
	struct ectrl_reg_rw *next = regs;
	struct data_list *tmp;
	int retval = 0;
	int val;
	for (; next->op != STOP_OP ; next++) {
		val = ectrl_mem_single_op (client, next);
		if (val < 0)
			return val;
		if (val > 0) {
			if (data_l != NULL) {
				tmp = kzalloc (sizeof (struct data_list), GFP_KERNEL);
				tmp->data = next->reg.data;
				list_add (&(tmp->list), &(data_l->list));
			}
		}
		retval += val;
	}
	return retval;
}


static int get_reg_rw (struct ectrl_reg_rw *reg_rw, struct ectrl_reg reg, int op) {
	reg_rw->op = op;
	reg_rw->reg.data = reg.data;
	reg_rw->reg.addr = reg.addr;
	return 0;
}



/* --------------------------------------------------------------------------
                              ADVANCED FUNCTIONS
   -------------------------------------------------------------------------- */

			/* DATA FUNCTIONS */

#define getFirmwareRev(client)    (ectrl_read_data ((client), FW_ID_REV_REG_H) & 0xFF) << 8 | (ectrl_read_data ((client), FW_ID_REV_REG_L) & 0xFF) 




			/* EVENT FUNCTIONS */

#define getStatusReg(client)    ectrl_read_data ((client), STATUS_REG_L) 
//| (((ectrl_read_data ((client), STATUS_REG_H)) )<<8)
#define getFlagReg(client)      ectrl_read_data ((client), FLAG_REG)
#define getEnableReg(client)    ectrl_read_data ((client), WAKE_CFG)
#define getEnFlashReg(client)   (ectrl_read_vector_element ((client), EN_FLASH_REG_MSB) << 8) | \
				ectrl_read_vector_element ((client), EN_FLASH_REG_LSB)  

//#define setFlagReg(client, regv)      ectrl_write_data ((client), FLAG_REG, (regv))
//#define setEnableReg(client, regv)    ectrl_write_data ((client), ENABLE_REG, (regv))

#define setFlagReg(client, regv)      printk(KERN_ERR "set flasg reg\n")
#define setEnableReg(client, regv)    ectrl_write_data ((client), WAKE_CFG, (regv))
#define setEnFlashReg(client, regv)   reg_en_flash[1].reg.data = (regv) & 0x00FF;  \
		   		      reg_en_flash[3].reg.data = ((regv) >> 8) & 0x00FF;  \
                                      ectrl_mem_op ((client), reg_en_flash, NULL) 


static inline int getStatus (struct i2c_client *client, int reg_idx) {
	int reg = (int)getStatusReg(client);
	return ((reg >> (reg_idx)) & 0x1);
}

	
static inline int getFlag (struct i2c_client *client, int event) {
	int reg = (int)getFlagReg(client);
	return (reg & global_event_list[event].index_reg) >> (global_event_list[event].shift & EVENT_MASK);
}


static inline int getEnable (struct i2c_client *client, int event) {
	int reg = (int)getEnableReg(client);
	return (reg & ((global_event_list[event].index_reg)>>8)) >> (global_event_list[event].shift >> ENABLE_OFFSET);
}


static inline int getEnFlash (struct i2c_client *client, int event) {
	//int reg = (int)getEnFlashReg(client);
	//return (reg & global_event_list[event].index_reg) >> global_event_list[event].shift;
	return 1;
}


static inline void setFlag (struct i2c_client *client, int event, int set) {
	int reg = (int)getFlagReg(client);
	reg = (set) ? reg | (1u << global_event_list[event].shift) : reg & ~(1u << global_event_list[event].shift);
	setFlagReg(client, reg);	
}


static inline void setEnable (struct i2c_client *client, int event, int set) {
	int reg = (int)getEnableReg(client);
	reg = (set) ? reg | (1u << ((global_event_list[event].shift)>>ENABLE_OFFSET)) : reg & ~(1u << ((global_event_list[event].shift)>>ENABLE_OFFSET));
	setEnableReg(client, reg);	
}


static inline void setEnFlash (struct i2c_client *client, int event, int set) {
	int reg = (int)getEnFlashReg(client);
	reg = (set) ? reg | (1u << global_event_list[event].shift) : reg & ~(1u << global_event_list[event].shift);
	setEnFlashReg(client, reg);	
}


static int is_feasibleEvent (struct econtroller *ectrl, unsigned short int id) {
	int i;
	int find = 0;

	for (i = 0 ; i < ectrl->nr_evnt ; i++) {
		if ( ectrl->events[i]->id == id ) {
			find = 1;
			break;
		}
	}

	return find;
}

			/* POWER MANAGEMENT FUNCTIONS */

#define PMgetAlwaysState(client)             ectrl_read_vector_element ((client), ALWAYS_STATE_REG)
#define PMsetAlwaysState(client, state)      reg_pm_always_state[1].reg.data = (state); \
                                             ectrl_mem_op ((client), reg_pm_always_state, NULL)

			/* Store Value in Permanently in Flash */
#define saveInFlash(client)			ectrl_write_data(client, CMD_CONF_SAVE, 1)

			/* BOOT FUNCTIONS */
/* Checked  */

#define getBoot0Reg(client)		   ectrl_read_double_element ((client), BOOT0_REG)
#define getBoot1Reg(client)		   ectrl_read_double_element ((client), BOOT1_REG)
#define getBoot2Reg(client)		   ectrl_read_double_element ((client), BOOT2_REG)
#define getBoot3Reg(client)		   ectrl_read_double_element ((client), BOOT3_REG)
#define getBootDefReg(client)		   ectrl_read_double_element ((client),BOOT_DEF_REG)

#define getBootConfig_1(client)		ectrl_read_data(client,BOOT_CFG_1)
#define getBootConfig_2(client)		ectrl_read_data(client,BOOT_CFG_2)

#define getStraps0x(client)        ectrl_read_double_element(client,BOOT_DEF_REG)

/* UNchecked */

#define BOOT_SEQUENCE_LENGTH         5
#define IS_VALID_BOOT_SEQ_IDX(idx)   ((idx) >= 0 && (idx) < BOOT_SEQUENCE_LENGTH) ? 1 : 0

#define setBoot0Reg(ectrl_s, strap_id)   setStraps(ectrl_s,BOOT0_REG,strap_id)
#define setBoot1Reg(ectrl_s, strap_id)   setStraps(ectrl_s,BOOT1_REG,strap_id)
#define setBoot2Reg(ectrl_s, strap_id)   setStraps(ectrl_s,BOOT2_REG,strap_id)
#define setBoot3Reg(ectrl_s, strap_id)   setStraps(ectrl_s,BOOT3_REG,strap_id)


#define setStrapsDefault(client,strap_id)		setStraps(client,BOOT_DEF_REG,strap_id)

#define	setStraps(ectrl_s,strap_reg,strap_id)		ectrl_write_double_element(ectrl_s->client,strap_reg,strap_reg + 0x1, \
							ectrl_s->straps_def[strap_id - 1 ].strap_l,ectrl_s->straps_def[strap_id - 1].strap_h)
#define setDefRegAsBootReg(client)			ectrl_write_data(client, BOOT_CFG_2, BOOT_STRAPS_DEF_REG)
#define set0xRegAsBootReg(client)                     	ectrl_write_data(client, BOOT_CFG_2, BOOT_STRAPS_0x_REG)
#define set0xRegAsBootRegMaxIndex(client)		ectrl_write_data(client, BOOT_CFG_2, BOOT_STRAPS_0x_REG | ((ectrl->straps_def_size) << 4))
#define setCarrierRegdAsBootReg(client)			ectrl_write_data(client, BOOT_CFG_2, BOOT_STRAPS_CARRIER)

enum BOOTDEV_ID getStraps(struct econtroller *ectrl, u8 reg) {
	
	int id;
	u16 straps_default;
	struct i2c_client *client = ectrl->client;
	

	straps_default = ectrl_read_double_element(client,reg);

	for (id=0;id< ectrl->straps_def_size; id++) {
		if( ( ectrl->straps_def[id].strap_l == (straps_default & 0xff) ) && (ectrl->straps_def[id].strap_h == ((straps_default>>8) & 0xff) )  )	
			break;
	}
	
	return (enum BOOTDEV_ID) (id+1);

}

static int isAvaildableBootdev (struct econtroller *ectrl, enum BOOTDEV_ID id) {
	int i, isValid = -1;
	for (i = 0 ; i < ectrl->nr_bootdev ; i++) {
		if (ectrl->bootdev_list[i].id == id) {
			isValid = i;
			break;
		}
	}
	return isValid;
}


static int getBootDev (struct econtroller *ectrl, int boot_idx) {
	int reg_val;

	switch (boot_idx) {
		case BOOT_IDX0:
			reg_val = getStraps(ectrl,BOOT0_REG);
			break;
		case BOOT_IDX1:
			reg_val = getStraps(ectrl,BOOT1_REG);
			break;
		case BOOT_IDX2:
			reg_val = getStraps(ectrl,BOOT2_REG);
			break;
		case BOOT_IDX3:
			reg_val = getStraps(ectrl,BOOT3_REG);
			break;
		case BOOT_STRAPS_DEV_SEL:
                        reg_val = getStraps(ectrl,BOOT_DEF_REG);
                        break;
		default:
			reg_val = -1;
	}
	return reg_val;
}


static int setBootDev (struct econtroller *ectrl, int boot_idx, enum BOOTDEV_ID bootdev_id) {
	if (isAvaildableBootdev (ectrl, bootdev_id) < 0)
		return -1;
	else {
		switch (boot_idx) {
			case BOOT_IDX0:
				setBoot0Reg(ectrl, (int)bootdev_id);
			//	set0xRegAsBootReg(ectrl->client);
				break;
			case BOOT_IDX1:
				setBoot1Reg(ectrl, (int)bootdev_id);
			//	set0xRegAsBootReg(ectrl->client);
				break;
			case BOOT_IDX2:
				setBoot2Reg(ectrl, (int)bootdev_id);
			//	set0xRegAsBootReg(ectrl->client);
				break;
			case BOOT_IDX3:
				setBoot3Reg(ectrl, (int)bootdev_id);
			//	set0xRegAsBootReg(ectrl->client);
				break;
			case BOOT_STRAPS_DEV_SEL:
	                        setStrapsDefault(ectrl, (int)bootdev_id);
			//	setDefRegAsBootReg(ectrl->client);
                        	break;
			default:
				return -1;
		}
	}
	return boot_idx;	
}



			/* WATCHDOG FUNCTIONS */
/* Checked */

#define WDTgetEnableReg(client)    (ectrl_read_data ((client), WDT_CONFIG_REG) & \
					WDT_MASK_ENABLE) >> WDT_SHIFT_ENABLE
/*#define WDTsetEnableReg(client, en)       {  ectrl_write_data ((client), WDT_CONFIG_REG, !(en) ? \
                                ectrl_read_data ((client), WDT_CONFIG_REG) & ~WDT_MASK_ENABLE : \
                                ectrl_read_data ((client), WDT_CONFIG_REG) | WDT_MASK_ENABLE) ; WDTcmdenable(client); };
*/
#define WDTsetEnableReg(client, en)	WDTcmdenable(client)

#define WDTgetEventReg(client)     (ectrl_read_data ((client), WDT_CONFIG_REG) & \
					WDT_MASK_EVENT) >> WDT_SHIFT_EVENT

#define WDTsetEventReg(client, evn)         ectrl_write_data ((client), WDT_CONFIG_REG, \
                                (ectrl_read_data ((client), WDT_CONFIG_REG) \
                                & ~WDT_MASK_EVENT) | (((evn) << WDT_SHIFT_EVENT) & WDT_MASK_EVENT))

#define WDTgetDelayReg(client)     (ectrl_read_data ((client), WDT_DELAY_REG))

#define WDTgetTimeoutReg(client)   (ectrl_read_data ((client), WDT_TIMEOUT_REG))

#define WDTsetDelayReg(client, regval)      ectrl_write_data (client, WDT_DELAY_REG, (u16)(regval))
#define WDTsetTimeoutReg(client, regval)    ectrl_write_data (client, WDT_TIMEOUT_REG, (u16)(regval))

#define WDTwdtout_reset(client)		{ WDTsetDelayReg(client,0); WDTsetTimeoutReg(client,0); WDTsetEnableReg(client,1); };

/* TODO: not implemented in the firmware rev 0xf01 */
#define WDTgetTimer1Reg(client)			NOT_IMPLEMENTED /* (ectrl_read_data ((client), WDT_TIMER1_REG))  */
#define WDTgetTimer2Reg(client)	 		NOT_IMPLEMENTED/* (ectrl_read_data ((client), WDT_TIMER2_REG))  */

#define WDT_F_getDelayReg(client)    		NOT_IMPLEMENTED /*(ectrl_read_vector_element ((client), WDT_F_DELAY_REG_MSB) << 8) |  \
                                     				ectrl_read_vector_element ((client), WDT_F_DELAY_REG_LSB) */
#define WDT_F_getTimeoutReg(client)  		NOT_IMPLEMENTED		/* (ectrl_read_vector_element ((client), WDT_F_TIMEOUT_REG_MSB) << 8) |  \
                                     				ectrl_read_vector_element ((client), WDT_F_TIMEOUT_REG_LSB) */
#define WDT_F_getEnableReg(client)   		NOT_IMPLEMENTED		/*(ectrl_read_vector_element ((client), WDT_F_CONFIG_REG_LSB) & \
                                        			WDT_MASK_ENABLE) >> WDT_SHIFT_ENABLE */
#define WDT_F_getEventReg(client)    		NOT_IMPLEMENTED		/* (ectrl_read_vector_element ((client), WDT_F_CONFIG_REG_LSB) & \
                                        			WDT_MASK_EVENT) >> WDT_SHIFT_EVEN */

#define WDT_F_setDelayReg(client, regval)	NOT_IMPLEMENTED	 /*reg_wdt_delay[1].reg.data = (regval) & 0x00FF; \
                                              			reg_wdt_delay[3].reg.data = ((regval) >> 8) & 0x00FF; \
                                              			ectrl_mem_op ((client), reg_wdt_delay, NULL)     */         
#define WDT_F_setTimeoutReg(client, regval)	NOT_IMPLEMENTED	 /*reg_wdt_timeout[1].reg.data = (regval) & 0x00FF; \
                                              			reg_wdt_timeout[3].reg.data = ((regval) >> 8) & 0x00FF; \
                                              			ectrl_mem_op ((client), reg_wdt_timeout, NULL) */
#define WDT_F_setEnableReg(client, regval)	NOT_IMPLEMENTED	 /*reg_wdt_config[1].reg.data = (regval);  \
                                              			ectrl_mem_op ((client), reg_wdt_config, NULL) */
#define WDT_F_setEventReg(client, regval)	NOT_IMPLEMENTED	 /*reg_wdt_config[1].reg.data = (regval) & 0x00FF;  \
                                              			reg_wdt_config[3].reg.data = ((regval) >> 8) & 0x00FF; \
                                              			ectrl_mem_op ((client), reg_wdt_config, NULL) */


#define WDTrefresh(client)         		ectrl_write_data ((client), CMD_WD_WDI, 1)
#define WDTcmdenable(client)                    ectrl_write_data ((client), CMD_WD_EN, 1)


static inline enum wdt_event WDTgetEvent (struct i2c_client *client) {
	return (enum wdt_event)WDTgetEventReg(client);
}


static inline int WDTsetEvent (struct i2c_client *client, enum wdt_event event) {
	u16 retval;
	switch (event) {
		case WDT_EVNT_WDOUT:
		case WDT_EVNT_RESET:
		case WDT_EVNT_PWRCYCLE:
		case WDT_EVNT_COMP_PWRCYCLE:
			WDTsetEventReg(client, (u16)event);
			retval = (u16)event;
			break;
		default:
			retval = -1;
	}
	return (int)retval;
}


static inline void WDT_F_setEnable (struct i2c_client *client, int en) {
	int reg = (int)(ectrl_read_vector_element ((client), WDT_F_CONFIG_REG_LSB) |
			ectrl_read_vector_element ((client), WDT_F_CONFIG_REG_MSB) << 8);
	reg = (en) ? reg | WDT_MASK_ENABLE : reg & ~WDT_MASK_ENABLE;
	if (en == 0 || en == 1)
		WDT_F_setEnableReg (client, reg);
}


static inline int WDT_F_setEvent (struct i2c_client *client,  enum wdt_event event) {
	int reg = (int)(ectrl_read_vector_element ((client), WDT_F_CONFIG_REG_LSB) |
			ectrl_read_vector_element ((client), WDT_F_CONFIG_REG_MSB) << 8);
	u16 retval;
	switch (event) {
		case WDT_EVNT_WDOUT:
		case WDT_EVNT_RESET:
		case WDT_EVNT_PWRCYCLE:
		case WDT_EVNT_COMP_PWRCYCLE:
			retval = (u16)event;
			reg = (reg & ~WDT_MASK_EVENT) | ((retval << WDT_SHIFT_EVENT) & WDT_MASK_EVENT);
			WDT_F_setEnableReg(client, reg);
			break;
		default:
			retval = -1;
	}
	return (int)retval;

}



			/* POWER FUNCTIONS */

static int ectrl_SystemHalt (struct econtroller *ectrl) {
	int retval;
	retval = ectrl_mem_op (ectrl->client, reg_halt, NULL);
	if (!(retval < 0)) {
		ectrl_write_data (ectrl->client, CMD_POWER_OFF, 1);
		printk (KERN_INFO "Seco halt performed!\n");
		if(gpio_is_valid(ectrl->rb_poff_gpio)) {	
			gpio_request(ectrl->rb_poff_gpio,"seco halt gpio");
			gpio_direction_output(ectrl->rb_poff_gpio,1);
		}
	} else {
		printk (KERN_ERR "Seco halt not performed!\n");
	}
	return retval;
}


static int ectrl_SystemReboot (struct econtroller *ectrl) {
	int retval;
	retval = ectrl_mem_op (ectrl->client, reg_reboot, NULL);
	if (!(retval < 0)) {
		ectrl_write_data (ectrl->client, CMD_REBOOT, 1);
		printk (KERN_INFO "Seco reboot performed!\n");
		if(gpio_is_valid(ectrl->rb_poff_gpio)) {
                        gpio_request(ectrl->rb_poff_gpio,"seco halt gpio");
                        gpio_direction_output(ectrl->rb_poff_gpio,1);
                }
	} else {
		printk (KERN_ERR "Seco reboot not performed!\n");
	}
	return retval;
}


/* --------------------------------------------------------------------------
                              FS Interface (/proc)
   -------------------------------------------------------------------------- */

static struct proc_dir_entry *ectrl_root_dir;
static struct proc_dir_entry *ectrl_events_dir;
static struct proc_dir_entry *ectrl_events_state_dir;
static struct proc_dir_entry *ectrl_boot_dir;
static struct proc_dir_entry *ectrl_watchdog_dir;


#define ECTRL_PROC_ROOT                          "ectrl"

#define ECTRL_PROC_EVENTS                        "events"
#define ECTRL_PROC_EVENTS_STATE                  "event_state"
#define ECTRL_PROC_PM                            "power_management"
#define ECTRL_PROC_BOOT                          "boot"
#define ECTRL_PROC_WATCHDOG                      "watchdog"

#define ECTRL_PROC_LID                           "lid"
#define ECTRL_PROC_BATTERY                       "battery"
#define ECTRL_PROC_PWR_BTN                       "power_button"
#define ECTRL_PROC_RST_BTN                       "reset_button"
#define ECTRL_PROC_SLEEP                         "sleep"

#define ECTRL_ENTRY_STATUS                       "status"
#define ECTRL_ENTRY_ENABLE                       "enable"
#define ECTRL_ENTRY_ENFLASH                      "en_flash"

#define ECTRL_ENTRY_PM_PWR_STATE                 "power_state"
#define ECTRL_ENTRY_PM_STATE                     "available_states"
#define ECTRL_ENTRY_PM_PWR_BTN_4SEC              "pwr_btn_4sec"
#define ECTRL_ENTRY_PM_PWR_BTN_4SEC_EN           "enable"
#define ECTRL_ENTRY_PM_PWR_BTN_4SEC_ENFLASH      "en_flash"

#define ECTRL_ENTRY_DEVBOOT0                     "devboot_1th"
#define ECTRL_ENTRY_DEVBOOT1                     "devboot_2th"
#define ECTRL_ENTRY_DEVBOOT2                     "devboot_3th"
#define ECTRL_ENTRY_RECOVERY_BOOT                "recovery_boot"
#define ECTRL_ENTRY_STRAPS_DEV_SEL               "devboot_default"
#define ECTRL_ENTRY_BOOTDEV_LIST                 "available_bootdevices"
#define ECTRL_ENTRY_BOOT_SEQUENCE                "boot_sequence"

#define ECTRL_ENTRY_WDT_ENABLE                   "enable"
#define ECTRL_ENTRY_WDT_DELAY                    "delay_sec"
#define ECTRL_ENTRY_WDT_TIMEOUT                  "timeout_sec"
#define ECTRL_ENTRY_WDT_TIMER1                   "timer_delay"
#define ECTRL_ENTRY_WDT_TIMER2                   "timer_wd"
#define ECTRL_ENTRY_WDT_EVENT                    "event"
#define ECTRL_ENTRY_WDT_AVAL_EVN                 "available_events"
#define ECTRL_ENTRY_WDT_STATE                    "state"
#define ECTRL_ENTRY_WDT_REFRESH                  "refresh"
#define ECTRL_ENTRY_WDT_RESTORE                  "restore"
#define ECTRL_ENTRY_WDT_WDTOUT_RESET             "wdtout_reset"

#define INPUT_BUF_SIZE   256


/* -------------------------- PROC STATE FILEs OPT (r only) -------------------------- */

static int ectrl_proc_state_show (struct seq_file *seq, void *offset) {
	struct ectrl_proc_event *proc = (struct ectrl_proc_event *)seq->private;
	int status;
	if (!proc) 
		return -EINVAL;
	mutex_lock (&proc->ectrl->fs_lock);
	status = getStatus (proc->ectrl->client, proc->event);
	mutex_unlock (&proc->ectrl->fs_lock);
        seq_printf(seq, "%s\n", !status ? "active" : "inactive");
        return 0;
}


static int ectrl_proc_state_open_fs (struct inode *inode, struct file *file) {
        return single_open(file, ectrl_proc_state_show,  PDE_DATA(inode));
}


static const struct file_operations ectrl_proc_state_fops = {
        .owner = THIS_MODULE,
        .open = ectrl_proc_state_open_fs,
        .read = seq_read,
	.write = NULL,
        .llseek = seq_lseek,
        .release = single_release,
};


/* -------------------------- PROC STATUS FILEs OPT (r only) -------------------------- */

static int ectrl_proc_status_show (struct seq_file *seq, void *offset) {
	struct ectrl_proc_event_state *proc = (struct ectrl_proc_event_state *)seq->private;
	int status;
	if (!proc) 
		return -EINVAL;
	mutex_lock (&proc->ectrl->fs_lock);
	status = getStatus (proc->ectrl->client, 
		proc->ectrl->evn_state[proc->state_id]->reg_idx);
	mutex_unlock (&proc->ectrl->fs_lock);
        seq_printf(seq, "%s\n", status ? "active" : "inactive");
        return 0;
}


static int ectrl_proc_status_open_fs (struct inode *inode, struct file *file) {
        return single_open(file, ectrl_proc_status_show,  PDE_DATA(inode));
}


static const struct file_operations ectrl_proc_status_fops = {
        .owner = THIS_MODULE,
        .open = ectrl_proc_status_open_fs,
        .read = seq_read,
	.write = NULL,
        .llseek = seq_lseek,
        .release = single_release,
};


/* -------------------------- PROC ENABLE FILEs OPT (r/w) -------------------------- */

static int ectrl_proc_enable_show (struct seq_file *seq, void *offset) {
	struct ectrl_proc_event *proc = (struct ectrl_proc_event *)seq->private;
	int enable;
	if (!proc) 
		return -EINVAL;

	mutex_lock (&proc->ectrl->fs_lock);
	enable = getEnable (proc->ectrl->client, proc->event);
	mutex_unlock (&proc->ectrl->fs_lock);
        seq_printf(seq, "%s\n", enable ? "enable" : "disable");
        return 0;
}


static ssize_t ectrl_proc_enable_write (struct file *file, const char __user *buf,
			size_t count, loff_t *pos) {
	
	char input[INPUT_BUF_SIZE];
	int err_conv;
	long en;
	struct ectrl_proc_event *proc = (struct ectrl_proc_event *)
		((struct seq_file *)file->private_data)->private;
	if (!proc)
		return -EINVAL;

	if (!capable(CAP_SYS_ADMIN))
                return -EACCES;

        if (count >= INPUT_BUF_SIZE)
                count = INPUT_BUF_SIZE - 1;

	memset(input, 0, INPUT_BUF_SIZE);
	if (copy_from_user (input, buf, count))
		return -EFAULT;	

	err_conv = kstrtoul (input, 0, &en);
	
	if (err_conv == 0) {
		if (en == 0 || en == 1) {
			mutex_lock (&proc->ectrl->fs_lock);
			setEnable (proc->ectrl->client, proc->event, en);
			mutex_unlock (&proc->ectrl->fs_lock);
		} else 
			return -EINVAL;
	}
	return count;
}


static int ectrl_proc_enable_open_fs (struct inode *inode, struct file *file) {
        return single_open(file, ectrl_proc_enable_show,  PDE_DATA(inode));
}


static const struct file_operations ectrl_proc_enable_fops = {
        .owner = THIS_MODULE,
        .open = ectrl_proc_enable_open_fs,
        .read = seq_read,
	.write = ectrl_proc_enable_write,
        .llseek = seq_lseek,
        .release = single_release,
};


/* -------------------------- PROC ENFLASH FILEs OPT (r/w) -------------------------- */

static int ectrl_proc_enflash_show (struct seq_file *seq, void *offset) {
	struct ectrl_proc_event *proc = (struct ectrl_proc_event *)seq->private;
	int enflash;
	if (!proc) 
		return -EINVAL;

	mutex_lock (&proc->ectrl->fs_lock);
	enflash = getEnFlash (proc->ectrl->client, proc->event);
	mutex_unlock (&proc->ectrl->fs_lock);
        seq_printf(seq, "%s\n", enflash ? "enable" : "disable");
        return 0;
}


static ssize_t ectrl_proc_enflash_write (struct file *file, const char __user *buf,
			size_t count, loff_t *pos) {
	char input[INPUT_BUF_SIZE];
	int err_conv;
	long en;
	struct ectrl_proc_event *proc = (struct ectrl_proc_event *)
		((struct seq_file *)file->private_data)->private;
		
	if (!proc)
		return -EINVAL;

	if (!capable(CAP_SYS_ADMIN))
                return -EACCES;

        if (count >= INPUT_BUF_SIZE)
                count = INPUT_BUF_SIZE - 1;

	memset(input, 0, INPUT_BUF_SIZE);
	if (copy_from_user (input, buf, count))
		return -EFAULT;	

	err_conv = kstrtoul (input, 0, &en);
	
	if (err_conv == 0) {
		if (en == 0 || en == 1) {
			mutex_lock (&proc->ectrl->fs_lock);
			setEnFlash (proc->ectrl->client, proc->event, en);
			mutex_unlock (&proc->ectrl->fs_lock);
		} else 
			return -EINVAL;
	}
	return count;

}


static int ectrl_proc_enflash_open_fs (struct inode *inode, struct file *file) {
        return single_open(file, ectrl_proc_enflash_show,  PDE_DATA(inode));
}


static const struct file_operations ectrl_proc_enflash_fops = {
        .owner = THIS_MODULE,
        .open = ectrl_proc_enflash_open_fs,
        .read = seq_read,
	.write = ectrl_proc_enflash_write,
        .llseek = seq_lseek,
        .release = single_release,
};


/* -------------------------- PROC PM PWM_STATE FILEs OPT (r/w) -------------------------- */

static int ectrl_proc_pm_pwr_state_show (struct seq_file *seq, void *offset) {
	struct econtroller *ectrl = (struct econtroller *)seq->private;
	u16 always;
	if (!ectrl) 
		return -EINVAL;

	mutex_lock (&ectrl->fs_lock);
	always = PMgetAlwaysState (ectrl->client);
	mutex_unlock (&ectrl->fs_lock);
        seq_printf(seq, "%s\n", always == (u16)ALWAYS_ON ? "always_on" : "always_off");
        return 0;
}


static ssize_t ectrl_proc_pm_pwr_state_write (struct file *file, const char __user *buf,
			size_t count, loff_t *pos) {
	char input[INPUT_BUF_SIZE];
	int err_conv;
	long en;
	struct econtroller *ectrl = (struct econtroller *)
		((struct seq_file *)file->private_data)->private;
	if (!ectrl)
		return -EINVAL;

	if (!capable(CAP_SYS_ADMIN))
                return -EACCES;

        if (count >= INPUT_BUF_SIZE)
                count = INPUT_BUF_SIZE - 1;

	memset(input, 0, INPUT_BUF_SIZE);
	if (copy_from_user (input, buf, count))
		return -EFAULT;	

	err_conv = kstrtoul (input, 0, &en);
	
	if (err_conv == 0) {
		if (en == 1) {
			mutex_lock (&ectrl->fs_lock);
			PMsetAlwaysState (ectrl->client, ALWAYS_ON);
			mutex_unlock (&ectrl->fs_lock);
		} else if (en == 0) {
			mutex_lock (&ectrl->fs_lock);
			PMsetAlwaysState (ectrl->client, ALWAYS_OFF);
			mutex_unlock (&ectrl->fs_lock);
		} else 
			return -EINVAL;
	}
	return count;

}


static int ectrl_proc_pm_pwr_state_open_fs (struct inode *inode, struct file *file) {
        return single_open(file, ectrl_proc_pm_pwr_state_show,  PDE_DATA(inode));
}


static const struct file_operations ectrl_proc_pm_pwr_state_fops = {
        .owner = THIS_MODULE,
        .open = ectrl_proc_pm_pwr_state_open_fs,
        .read = seq_read,
	.write = ectrl_proc_pm_pwr_state_write,
        .llseek = seq_lseek,
        .release = single_release,
};


/* -------------------------- PROC AVAILABLE PM STATE FILE OPT (r only) -------------------------- */

static int ectrl_proc_available_pm_state_show (struct seq_file *seq, void *offset) {
	struct econtroller *ectrl = (struct econtroller *)seq->private;
	int i;

	if (!ectrl) 
		return -EINVAL;

	mutex_lock (&ectrl->fs_lock);
	for (i = 0 ; i < ARRAY_SIZE (PM_PWR_STATE) ; i++) {
		seq_printf (seq, "%d: %s\n", i, PM_PWR_STATE[i]);
	}
	mutex_unlock (&ectrl->fs_lock);
        return 0;
}


static int ectrl_proc_available_pm_state_open_fs (struct inode *inode, struct file *file) {
        return single_open(file, ectrl_proc_available_pm_state_show,  PDE_DATA(inode));
}


static const struct file_operations ectrl_proc_available_pm_state_fops = {
        .owner = THIS_MODULE,
        .open = ectrl_proc_available_pm_state_open_fs,
        .read = seq_read,
	.write = NULL,
        .llseek = seq_lseek,
        .release = single_release,
};


/* -------------------------- PROC BOOT FILEs OPT (r/w) -------------------------- */

static int ectrl_proc_bootdev_is (struct i2c_client *client, struct seq_file *seq) {


	switch(getBootConfig_2(client) & BOOT_STRAPS_MASK ) {

                case BOOT_STRAPS_CARRIER:
                        seq_printf (seq, "bootdev is: %s\n",boot_straps_list[BOOT_STRAPS_CARRIER].label);
                        return 0;
                        break;
                case BOOT_STRAPS_DEF_REG:
                        seq_printf (seq, "bootdev is: %s\n",boot_straps_list[BOOT_STRAPS_DEF_REG].label );
                        break;
                case BOOT_STRAPS_0x_REG:
                        seq_printf (seq, "bootdev is: %s\n",boot_straps_list[BOOT_STRAPS_0x_REG].label);
                        break;
                default:
                        seq_printf (seq, "bootdev is not defined\n");
                        return -1;

        }

	return 0;
}

static int ectrl_proc_bootdev_show (struct seq_file *seq, void *offset) {
	struct ectrl_proc_boot *proc = (struct ectrl_proc_boot *)seq->private;
	enum BOOTDEV_ID bootdev;
	int index;
	if (!proc) 
		return -EINVAL;

	if (!IS_VALID_BOOT_SEQ_IDX(proc->idx) || ectrl_proc_bootdev_is(proc->ectrl->client, seq) < 0 )
		return -EINVAL;

	mutex_lock (&proc->ectrl->fs_lock);
	bootdev = (enum BOOTDEV_ID)getBootDev (proc->ectrl, proc->idx);
	index = isAvaildableBootdev (proc->ectrl, bootdev);
	seq_printf (seq, "%d %s\n", (int)bootdev, index < 0 ? "<not configured>" : ectrl->bootdev_list[index].label);
	mutex_unlock (&proc->ectrl->fs_lock);
        return 0;
}


static ssize_t ectrl_proc_bootdev_write (struct file *file, const char __user *buf,
			size_t count, loff_t *pos) {
	
	char input[INPUT_BUF_SIZE];
	int err_conv;
	long  id;
	
	struct ectrl_proc_boot *proc = (struct ectrl_proc_boot *)
		((struct seq_file *)file->private_data)->private;
	
	if (!proc)
		return -EINVAL;

	if (!capable(CAP_SYS_ADMIN))
                return -EACCES;

        if (count >= INPUT_BUF_SIZE)
                count = INPUT_BUF_SIZE - 1;

	memset(input, 0, INPUT_BUF_SIZE);
	if (copy_from_user (input, buf, count))
		return -EFAULT;	

	err_conv = kstrtoul(input, 0, &id);
	
	if (err_conv == 0) {
		mutex_lock (&proc->ectrl->fs_lock);
		if (setBootDev (proc->ectrl, proc->idx, (enum BOOTDEV_ID)id) < 0) {
			mutex_unlock (&proc->ectrl->fs_lock);
			return -EINVAL;
		}
		mutex_unlock (&proc->ectrl->fs_lock);
	} else
		return err_conv;

	return count;
}


static int ectrl_proc_bootdev_open_fs (struct inode *inode, struct file *file) {
        return single_open(file, ectrl_proc_bootdev_show,  PDE_DATA(inode));
}


static const struct file_operations ectrl_proc_bootdev_fops = {
        .owner = THIS_MODULE,
        .open = ectrl_proc_bootdev_open_fs,
        .read = seq_read,
	.write = ectrl_proc_bootdev_write,
        .llseek = seq_lseek,
        .release = single_release,
};


/* -------------------------- PROC BOOT_DEV_LIST FILEs OPT (r only) -------------------------- */

static int ectrl_proc_bootdev_list_show (struct seq_file *seq, void *offset) {
	struct econtroller *ectrl = (struct econtroller *)seq->private;
	int i;
	if (!ectrl) 
		return -EINVAL;

	mutex_lock (&ectrl->fs_lock);
	for (i = 0 ; i < ectrl->nr_bootdev ; i++) {
		seq_printf (seq, "%d %s\n",  
				ectrl->bootdev_list[i].id, ectrl->bootdev_list[i].label);
	}
	mutex_unlock (&ectrl->fs_lock);
        return 0;
}

static int ectrl_proc_bootdev_list_open_fs (struct inode *inode, struct file *file) {
        return single_open(file, ectrl_proc_bootdev_list_show, PDE_DATA(inode));
	// PDE_DATA(inode));
}


static const struct file_operations ectrl_proc_bootdev_list_fops = {
        .owner = THIS_MODULE,
        .open = ectrl_proc_bootdev_list_open_fs,
        .read = seq_read,
	.write = NULL,
        .llseek = seq_lseek,
        .release = single_release,
};


/* -------------------------- PROC BOOT_SEQUENCE FILE OPT (r only) -------------------------- */

static ssize_t ectrl_proc_bootseq_write (struct file *file, const char __user *buf,
                        size_t count, loff_t *pos) {

        char input[INPUT_BUF_SIZE];

        struct ectrl_proc_boot *proc = (struct ectrl_proc_boot *)
                ((struct seq_file *)file->private_data)->private;
	struct econtroller *ectrl = (struct econtroller *)
                ((struct seq_file *)file->private_data)->private;

        if (!ectrl)
                return -EINVAL;

        if (!proc)
                return -EINVAL;
	

        if (!capable(CAP_SYS_ADMIN))
                return -EACCES;

        if (count >= INPUT_BUF_SIZE)
                count = INPUT_BUF_SIZE - 1;

        memset(input, 0, INPUT_BUF_SIZE);
        if (copy_from_user (input, buf, count))
                return -EFAULT;
	
	if(strcmp(input,"carrier\n") == 0 ) {

		mutex_lock (&ectrl->fs_lock);
		setCarrierRegdAsBootReg(ectrl->client);
		mutex_unlock (&ectrl->fs_lock);

	} else if(strcmp(input,"default\n") == 0 ) {

                mutex_lock (&ectrl->fs_lock);
                setDefRegAsBootReg(ectrl->client);
                mutex_unlock (&ectrl->fs_lock);

        } else if(strcmp(input,"devboot\n") == 0 ) {

                mutex_lock (&ectrl->fs_lock);
                set0xRegAsBootRegMaxIndex(ectrl->client); 
                mutex_unlock (&ectrl->fs_lock);

        } else
		return -EINVAL;	

        return count;
}


static int ectrl_proc_bootseq_show (struct seq_file *seq, void *offset) {
	struct econtroller *ectrl = (struct econtroller *)seq->private;
	int i;
	enum BOOTDEV_ID bootdev;
	int index;
	if (!ectrl) 
		return -EINVAL;

	switch(getBootConfig_2(ectrl->client) & BOOT_STRAPS_MASK ) {

                case BOOT_STRAPS_CARRIER:
                        seq_printf (seq, "bootdev is: %s\n",boot_straps_list[BOOT_STRAPS_CARRIER].label);
                        return 0;
                        break;
                case BOOT_STRAPS_DEF_REG:
                        seq_printf (seq, "bootdev is: %s\n",boot_straps_list[BOOT_STRAPS_DEF_REG].label );
			mutex_lock (&ectrl->fs_lock);
			bootdev = (enum BOOTDEV_ID)getBootDev (ectrl, BOOT_STRAPS_DEV_SEL);
			index = isAvaildableBootdev (ectrl, bootdev);
			seq_printf (seq, "%d %s\n", (int)bootdev, index < 0 ? "<not configured>" : ectrl->bootdev_list[index].label);
			mutex_unlock (&ectrl->fs_lock);
                        break;
                case BOOT_STRAPS_0x_REG:
                        seq_printf (seq, "bootdev is: %s\n",boot_straps_list[BOOT_STRAPS_0x_REG].label);
			mutex_lock (&ectrl->fs_lock);
			for (i = 0 ; i < BOOT_SEQUENCE_LENGTH ; i++) {
				bootdev = (enum BOOTDEV_ID)getBootDev (ectrl, i);
				index = isAvaildableBootdev (ectrl, bootdev);
				if (index >= 0) {
					if ( i != (BOOT_SEQUENCE_LENGTH -1)) {
						seq_printf (seq, "BootDev%d: %d %s\n", i,  
						   ectrl->bootdev_list[index].id, ectrl->bootdev_list[index].label);
					}
				}
			}
			mutex_unlock (&ectrl->fs_lock);
                        break;
                default:
                        seq_printf (seq, "bootdev is not defined\n");
                        return -1;

        }
        return 0;
}

static int ectrl_proc_bootseq_open_fs (struct inode *inode, struct file *file) {
        return single_open(file, ectrl_proc_bootseq_show,  PDE_DATA(inode));
}


static const struct file_operations ectrl_proc_bootseq_fops = {
        .owner = THIS_MODULE,
        .open = ectrl_proc_bootseq_open_fs,
        .read = seq_read,
	.write = ectrl_proc_bootseq_write,
        .llseek = seq_lseek,
        .release = single_release,
};


/* -------------------------- PROC WDT ENABLE FILEs OPT (r/w) -------------------------- */

static int ectrl_proc_wdt_enable_show (struct seq_file *seq, void *offset) {
	struct econtroller *ectrl = (struct econtroller *)seq->private;
	int enable;
	
	if (!ectrl) 
		return -EINVAL;

	mutex_lock (&ectrl->fs_lock);
	enable = WDTgetEnableReg (ectrl->client);
	mutex_unlock (&ectrl->fs_lock);
        seq_printf(seq, "%s\n", enable ? "enable" : "disable");
        return 0;
}


static ssize_t ectrl_proc_wdt_enable_write (struct file *file, const char __user *buf,
			size_t count, loff_t *pos) {
	
	char input[INPUT_BUF_SIZE];
	int err_conv;
	long en;
	struct econtroller *ectrl = (struct econtroller *)
		((struct seq_file *)file->private_data)->private;

	if (!ectrl)
		return -EINVAL;

	if (!capable(CAP_SYS_ADMIN))
                return -EACCES;

        if (count >= INPUT_BUF_SIZE)
                count = INPUT_BUF_SIZE - 1;

	memset(input, 0, INPUT_BUF_SIZE);
	if (copy_from_user (input, buf, count))
		return -EFAULT;	

	err_conv = kstrtoul (input, 0, &en);
	
	if (err_conv == 0) {
		if (en == 0 || en == 1) {
			mutex_lock (&ectrl->fs_lock);
			WDTsetEnableReg (ectrl->client, en);
			mutex_unlock (&ectrl->fs_lock);
		} else 
			return -EINVAL;
	}
	return count;
}


static int ectrl_proc_wdt_enable_open_fs (struct inode *inode, struct file *file) {
        return single_open(file, ectrl_proc_wdt_enable_show,  PDE_DATA(inode));
}


static const struct file_operations ectrl_proc_wdt_enable_fops = {
        .owner = THIS_MODULE,
        .open = ectrl_proc_wdt_enable_open_fs,
        .read = seq_read,
	.write = ectrl_proc_wdt_enable_write,
        .llseek = seq_lseek,
        .release = single_release,
};


/* -------------------------- PROC WDT EVENT FILE OPT (r/w) -------------------------- */

static int ectrl_proc_wdt_event_show (struct seq_file *seq, void *offset) {
	struct econtroller *ectrl = (struct econtroller *)seq->private;
	enum wdt_event event;
	int index;
	if (!ectrl) 
		return -EINVAL;

	mutex_lock (&ectrl->fs_lock);
	event = WDTgetEvent (ectrl->client);
	mutex_unlock (&ectrl->fs_lock);
	index = (int)event;
        seq_printf(seq, "%d %s\n", ectrl->wdt_event_list[index].id, ectrl->wdt_event_list[index].label);
        return 0;
}


static ssize_t ectrl_proc_wdt_event_write (struct file *file, const char __user *buf,
			size_t count, loff_t *pos) {
	
	char input[INPUT_BUF_SIZE];
	long id;
	int  err_conv;
	struct econtroller *ectrl = (struct econtroller *)
		((struct seq_file *)file->private_data)->private;

	if (!ectrl)
		return -EINVAL;

	if (!capable(CAP_SYS_ADMIN))
                return -EACCES;

        if (count >= INPUT_BUF_SIZE)
                count = INPUT_BUF_SIZE - 1;

	memset(input, 0, INPUT_BUF_SIZE);
	if (copy_from_user (input, buf, count))
		return -EFAULT;	

	err_conv = kstrtoul(input, 0, &id);

	if (err_conv == 0) {
		mutex_lock (&ectrl->fs_lock);
		if (WDTsetEvent (ectrl->client, (enum wdt_event)id) < 0) {
			mutex_unlock (&ectrl->fs_lock);
			return -EINVAL;
		}
		mutex_unlock (&ectrl->fs_lock);
	} else
		return err_conv;

	return count;
}


static int ectrl_proc_wdt_event_open_fs (struct inode *inode, struct file *file) {
        return single_open(file, ectrl_proc_wdt_event_show,  PDE_DATA(inode));
}


static const struct file_operations ectrl_proc_wdt_event_fops = {
        .owner = THIS_MODULE,
        .open = ectrl_proc_wdt_event_open_fs,
        .read = seq_read,
	.write = ectrl_proc_wdt_event_write,
        .llseek = seq_lseek,
        .release = single_release,
};


/* -------------------------- PROC AVAILABLE WDT EVENTS FILE OPT (r only) -------------------------- */

static int ectrl_proc_avalaible_wdt_event_show (struct seq_file *seq, void *offset) {
	struct econtroller *ectrl = (struct econtroller *)seq->private;
	int i;

	if (!ectrl) 
		return -EINVAL;

	mutex_lock (&ectrl->fs_lock);
	for (i = 0 ; i < ectrl->nr_wdt_event ; i++) {
		seq_printf (seq, "%d: %s\n",  ectrl->wdt_event_list[i].id, 
					ectrl->wdt_event_list[i].label);
	}
	mutex_unlock (&ectrl->fs_lock);
        return 0;
}


static int ectrl_proc_available_wdt_event_open_fs (struct inode *inode, struct file *file) {
        return single_open(file, ectrl_proc_avalaible_wdt_event_show,  PDE_DATA(inode));
}


static const struct file_operations ectrl_proc_avalaible_wdt_event_fops = {
        .owner = THIS_MODULE,
        .open = ectrl_proc_available_wdt_event_open_fs,
        .read = seq_read,
	.write = NULL,
        .llseek = seq_lseek,
        .release = single_release,
};


/* -------------------------- PROC WDT DELAY FILE OPT (r/w) -------------------------- */

static int ectrl_proc_wdt_delay_show (struct seq_file *seq, void *offset) {
	struct econtroller *ectrl = (struct econtroller *)seq->private;
	
	if (!ectrl) 
		return -EINVAL;

	mutex_lock (&ectrl->fs_lock);
        seq_printf(seq, "%d\n", WDTgetDelayReg (ectrl->client));
	mutex_unlock (&ectrl->fs_lock);
        return 0;
}


static ssize_t ectrl_proc_wdt_delay_write (struct file *file, const char __user *buf,
			size_t count, loff_t *pos) {
	
	char input[INPUT_BUF_SIZE];
	long delay;
	int  err_conv;
	struct econtroller *ectrl = (struct econtroller *)
		((struct seq_file *)file->private_data)->private;

	if (!ectrl)
		return -EINVAL;

	if (!capable(CAP_SYS_ADMIN))
                return -EACCES;

        if (count >= INPUT_BUF_SIZE)
                count = INPUT_BUF_SIZE - 1;

	memset(input, 0, INPUT_BUF_SIZE);
	if (copy_from_user (input, buf, count))
		return -EFAULT;	

	err_conv = kstrtoul(input, 0, &delay);

	if (err_conv == 0) {
		if ((u16)delay < 1)
			return -EINVAL;
		mutex_lock (&ectrl->fs_lock);
		WDTsetDelayReg (ectrl->client, (u16)delay);
		mutex_unlock (&ectrl->fs_lock);
	} else
		return err_conv;

	return count;
}


static int ectrl_proc_wdt_delay_open_fs (struct inode *inode, struct file *file) {
        return single_open(file, ectrl_proc_wdt_delay_show,  PDE_DATA(inode));
}


static const struct file_operations ectrl_proc_wdt_delay_fops = {
        .owner = THIS_MODULE,
        .open = ectrl_proc_wdt_delay_open_fs,
        .read = seq_read,
	.write = ectrl_proc_wdt_delay_write,
        .llseek = seq_lseek,
        .release = single_release,
};


/* -------------------------- PROC WDT TIMEOUT FILE OPT (r/w) -------------------------- */

static int ectrl_proc_wdt_timeout_show (struct seq_file *seq, void *offset) {
	struct econtroller *ectrl = (struct econtroller *)seq->private;
	
	if (!ectrl) 
		return -EINVAL;

	mutex_unlock (&ectrl->fs_lock);
        seq_printf(seq, "%d\n", WDTgetTimeoutReg (ectrl->client));
	mutex_unlock (&ectrl->fs_lock);
        return 0;
}


static ssize_t ectrl_proc_wdt_timeout_write (struct file *file, const char __user *buf,
			size_t count, loff_t *pos) {
	
	char input[INPUT_BUF_SIZE];
	long timeout;
	int  err_conv;
	struct econtroller *ectrl = (struct econtroller *)
		((struct seq_file *)file->private_data)->private;

	if (!ectrl)
		return -EINVAL;

	if (!capable(CAP_SYS_ADMIN))
                return -EACCES;

        if (count >= INPUT_BUF_SIZE)
                count = INPUT_BUF_SIZE - 1;

	memset(input, 0, INPUT_BUF_SIZE);
	if (copy_from_user (input, buf, count))
		return -EFAULT;	

	err_conv = kstrtoul(input, 0, &timeout);

	if (err_conv == 0) {
		if ((u16)timeout < 1)
			return -EINVAL; 
		mutex_lock (&ectrl->fs_lock);
		WDTsetTimeoutReg (ectrl->client, (u16)timeout);
		mutex_unlock (&ectrl->fs_lock);
	} else
		return err_conv;

	return count;
}


static int ectrl_proc_wdt_timeout_open_fs (struct inode *inode, struct file *file) {
        return single_open(file, ectrl_proc_wdt_timeout_show,  PDE_DATA(inode));}


static const struct file_operations ectrl_proc_wdt_timeout_fops = {
        .owner = THIS_MODULE,
        .open = ectrl_proc_wdt_timeout_open_fs,
        .read = seq_read,
	.write = ectrl_proc_wdt_timeout_write,
        .llseek = seq_lseek,
        .release = single_release,
};


/* -------------------------- PROC WDT REFRESH FILE OPT (r/w) -------------------------- */

static int ectrl_proc_wdt_refresh_show (struct seq_file *seq, void *offset) {
	struct econtroller *ectrl = (struct econtroller *)seq->private;
	
	if (!ectrl) 
		return -EINVAL;

        return 0;
}


static ssize_t ectrl_proc_wdt_refresh_write (struct file *file, const char __user *buf,
			size_t count, loff_t *pos) {
	
	char input[INPUT_BUF_SIZE];
	long in;
	int  err_conv;
	struct econtroller *ectrl = (struct econtroller *)
		((struct seq_file *)file->private_data)->private;

	if (!ectrl)
		return -EINVAL;

	if (!capable(CAP_SYS_ADMIN))
                return -EACCES;

        if (count >= INPUT_BUF_SIZE)
                count = INPUT_BUF_SIZE - 1;

	memset(input, 0, INPUT_BUF_SIZE);
	if (copy_from_user (input, buf, count))
		return -EFAULT;	

	err_conv = kstrtoul(input, 0, &in);

	if (err_conv == 0) {
		if ((u16)in != 1)
			return -EINVAL; 
		mutex_lock (&ectrl->fs_lock);
		WDTrefresh (ectrl->client);
		mutex_unlock (&ectrl->fs_lock);
	} else
		return err_conv;

	return count;
}


static int ectrl_proc_wdt_refresh_open_fs (struct inode *inode, struct file *file) {
        return single_open(file, ectrl_proc_wdt_refresh_show,  PDE_DATA(inode));
}


static const struct file_operations ectrl_proc_wdt_refresh_fops = {
        .owner = THIS_MODULE,
        .open = ectrl_proc_wdt_refresh_open_fs,
        .read = seq_read,
	.write = ectrl_proc_wdt_refresh_write,
        .llseek = seq_lseek,
        .release = single_release,
};


/* -------------------------- PROC WDT RESTORE FILE OPT (r/w) -------------------------- */

static int ectrl_proc_wdt_restore_show (struct seq_file *seq, void *offset) {
	struct econtroller *ectrl = (struct econtroller *)seq->private;
	
	if (!ectrl) 
		return -EINVAL;

        return 0;
}


static ssize_t ectrl_proc_wdt_restore_write (struct file *file, const char __user *buf,
			size_t count, loff_t *pos) {
	
	char input[INPUT_BUF_SIZE];
	long in;
	int  err_conv;
	u16 f_delay, f_timeout;
	struct econtroller *ectrl = (struct econtroller *)
		((struct seq_file *)file->private_data)->private;

	if (!ectrl)
		return -EINVAL;

	if (!capable(CAP_SYS_ADMIN))
                return -EACCES;

        if (count >= INPUT_BUF_SIZE)
                count = INPUT_BUF_SIZE - 1;

	memset(input, 0, INPUT_BUF_SIZE);
	if (copy_from_user (input, buf, count))
		return -EFAULT;	

	err_conv = kstrtoul(input, 0, &in);

	if (err_conv == 0) {
		if ((u16)in != 1)
			return -EINVAL; 
		mutex_lock (&ectrl->fs_lock);
		f_delay   = (u16)WDT_F_getDelayReg (ectrl->client);
		f_timeout = (u16)WDT_F_getTimeoutReg (ectrl->client);

		WDTsetDelayReg (ectrl->client, f_delay);
		WDTsetTimeoutReg (ectrl->client, f_timeout);	
		mutex_unlock (&ectrl->fs_lock);
	} else
		return err_conv;

	return count;
}


static int ectrl_proc_wdt_restore_open_fs (struct inode *inode, struct file *file) {
        return single_open(file, ectrl_proc_wdt_restore_show,  PDE_DATA(inode));
}


static const struct file_operations ectrl_proc_wdt_restore_fops = {
        .owner = THIS_MODULE,
        .open = ectrl_proc_wdt_restore_open_fs,
        .read = seq_read,
	.write = ectrl_proc_wdt_restore_write,
        .llseek = seq_lseek,
        .release = single_release,
};


/* -------------------------- PROC WDTOUT RESET FILE OPT (r/w) -------------------------- */

static int ectrl_proc_wdt_wdtout_reset_show (struct seq_file *seq, void *offset) {
	struct econtroller *ectrl = (struct econtroller *)seq->private;
	
	if (!ectrl) 
		return -EINVAL;

        return 0;
}


static ssize_t ectrl_proc_wdt_wdtout_reset_write (struct file *file, const char __user *buf,
			size_t count, loff_t *pos) {
	
	char input[INPUT_BUF_SIZE];
	long in;
	int  err_conv;
	struct econtroller *ectrl = (struct econtroller *)
		((struct seq_file *)file->private_data)->private;

	if (!ectrl)
		return -EINVAL;

	if (!capable(CAP_SYS_ADMIN))
                return -EACCES;

        if (count >= INPUT_BUF_SIZE)
                count = INPUT_BUF_SIZE - 1;

	memset(input, 0, INPUT_BUF_SIZE);
	if (copy_from_user (input, buf, count))
		return -EFAULT;	

	err_conv = kstrtoul(input, 0, &in);

	if (err_conv == 0) {
		if ((u16)in != 1)
			return -EINVAL;
		mutex_lock (&ectrl->fs_lock);
		WDTwdtout_reset (ectrl->client); 
		
		mutex_unlock (&ectrl->fs_lock);
	} else
		return err_conv;

	return count;
}


static int ectrl_proc_wdt_wdtout_reset_open_fs (struct inode *inode, struct file *file) {
        return single_open(file, ectrl_proc_wdt_wdtout_reset_show,  PDE_DATA(inode));
}


static const struct file_operations ectrl_proc_wdt_wdtout_reset_fops = {
        .owner = THIS_MODULE,
        .open = ectrl_proc_wdt_wdtout_reset_open_fs,
        .read = seq_read,
	.write = ectrl_proc_wdt_wdtout_reset_write,
        .llseek = seq_lseek,
        .release = single_release,
};


/* -------------------------- PROC WDT TIMER1 FILE OPT (r only) -------------------------- */

static int ectrl_proc_wdt_timer1_show (struct seq_file *seq, void *offset) {
	struct econtroller *ectrl = (struct econtroller *)seq->private;

	if (!ectrl) 
		return -EINVAL;

	mutex_lock (&ectrl->fs_lock);
        seq_printf(seq, "%d\n", WDTgetTimer1Reg (ectrl->client));
	mutex_unlock (&ectrl->fs_lock);
        return 0;
}


static int ectrl_proc_wdt_timer1_open_fs (struct inode *inode, struct file *file) {
        return single_open(file, ectrl_proc_wdt_timer1_show,  PDE_DATA(inode));
}


static const struct file_operations ectrl_proc_wdt_timer1_fops = {
        .owner = THIS_MODULE,
        .open = ectrl_proc_wdt_timer1_open_fs,
        .read = seq_read,
	.write = NULL,
        .llseek = seq_lseek,
        .release = single_release,
};


/* -------------------------- PROC WDT TIMER2 FILE OPT (r only) -------------------------- */

static int ectrl_proc_wdt_timer2_show (struct seq_file *seq, void *offset) {
	struct econtroller *ectrl = (struct econtroller *)seq->private;

	if (!ectrl) 
		return -EINVAL;

	mutex_lock (&ectrl->fs_lock);
        seq_printf(seq, "%d\n", WDTgetTimer2Reg (ectrl->client));
	mutex_unlock (&ectrl->fs_lock);
        return 0;
}


static int ectrl_proc_wdt_timer2_open_fs (struct inode *inode, struct file *file) {
        return single_open(file, ectrl_proc_wdt_timer2_show,  PDE_DATA(inode));
}


static const struct file_operations ectrl_proc_wdt_timer2_fops = {
        .owner = THIS_MODULE,
        .open = ectrl_proc_wdt_timer2_open_fs,
        .read = seq_read,
	.write = NULL,
        .llseek = seq_lseek,
        .release = single_release,
};


/* -------------------------- PROC WDT STATE FILE OPT (r only) -------------------------- */

static int ectrl_proc_wdt_state_show (struct seq_file *seq, void *offset) {
	struct econtroller *ectrl = (struct econtroller *)seq->private;
	enum wdt_event event;
	int enable, index;

	if (!ectrl) 
		return -EINVAL;
	
	mutex_lock (&ectrl->fs_lock);
	enable = WDTgetEnableReg (ectrl->client);

	event = WDTgetEvent (ectrl->client);
	index = (int)event;

        seq_printf(seq, "  %20s : %s\n  %20s : %d - %s\n  %20s : %d\n  %20s : %d\n",
				ECTRL_ENTRY_WDT_ENABLE,   enable ? "enable" : "disable",
				ECTRL_ENTRY_WDT_EVENT,    ectrl->wdt_event_list[index].id, 
							  ectrl->wdt_event_list[index].label,
				ECTRL_ENTRY_WDT_DELAY,    WDTgetDelayReg (ectrl->client),
				ECTRL_ENTRY_WDT_TIMEOUT,  WDTgetTimeoutReg (ectrl->client));
	mutex_unlock (&ectrl->fs_lock);
        return 0;
}


static int ectrl_proc_wdt_state_open_fs (struct inode *inode, struct file *file) {
        return single_open(file, ectrl_proc_wdt_state_show,  PDE_DATA(inode));
}


static const struct file_operations ectrl_proc_wdt_state_fops = {
        .owner = THIS_MODULE,
        .open = ectrl_proc_wdt_state_open_fs,
        .read = seq_read,
	.write = NULL,
        .llseek = seq_lseek,
        .release = single_release,
};


/* -------------------------- PROC WDT FLASH ENABLE FILEs OPT (r/w) -------------------------- */

static int ectrl_proc_wdt_f_enable_show (struct seq_file *seq, void *offset) {
	struct econtroller *ectrl = (struct econtroller *)seq->private;
	int enable;
	
	if (!ectrl) 
		return -EINVAL;

	mutex_lock (&ectrl->fs_lock);
	enable = WDT_F_getEnableReg (ectrl->client);
	mutex_unlock (&ectrl->fs_lock);
        seq_printf(seq, "%s\n", enable ? "enable" : "disable");
        return 0;
}


static ssize_t ectrl_proc_wdt_f_enable_write (struct file *file, const char __user *buf,
			size_t count, loff_t *pos) {
	
	char input[INPUT_BUF_SIZE];
	int err_conv;
	long en;
	struct econtroller *ectrl = (struct econtroller *)
		((struct seq_file *)file->private_data)->private;

	if (!ectrl)
		return -EINVAL;

	if (!capable(CAP_SYS_ADMIN))
                return -EACCES;

        if (count >= INPUT_BUF_SIZE)
                count = INPUT_BUF_SIZE - 1;

	memset(input, 0, INPUT_BUF_SIZE);
	if (copy_from_user (input, buf, count))
		return -EFAULT;	

	err_conv = kstrtoul (input, 0, &en);
	
	if (err_conv == 0) {
		if (en == 0 || en == 1) {
			mutex_lock (&ectrl->fs_lock);
			WDT_F_setEnable (ectrl->client, en);
			mutex_unlock (&ectrl->fs_lock);
		} else 
			return -EINVAL;
	}
	return count;
}


static int ectrl_proc_wdt_f_enable_open_fs (struct inode *inode, struct file *file) {
        return single_open(file, ectrl_proc_wdt_f_enable_show,  PDE_DATA(inode));
}


static const struct file_operations ectrl_proc_wdt_f_enable_fops = {
        .owner = THIS_MODULE,
        .open = ectrl_proc_wdt_f_enable_open_fs,
        .read = seq_read,
	.write = ectrl_proc_wdt_f_enable_write,
        .llseek = seq_lseek,
        .release = single_release,
};




/* -------------------------- PROC WDT FLASH EVENT FILE OPT (r/w) -------------------------- */

static int ectrl_proc_wdt_f_event_show (struct seq_file *seq, void *offset) {
	struct econtroller *ectrl = (struct econtroller *)seq->private;
	enum wdt_event event;
	int index;
	if (!ectrl) 
		return -EINVAL;

	mutex_lock (&ectrl->fs_lock);
	event = WDT_F_getEventReg (ectrl->client);
	mutex_unlock (&ectrl->fs_lock);
	index = (int)event;
        seq_printf(seq, "%d %s\n", ectrl->wdt_event_list[index].id, ectrl->wdt_event_list[index].label);
        return 0;
}


static ssize_t ectrl_proc_wdt_f_event_write (struct file *file, const char __user *buf,
			size_t count, loff_t *pos) {
	
	char input[INPUT_BUF_SIZE];
	long id;
	int  err_conv;
	struct econtroller *ectrl = (struct econtroller *)
		((struct seq_file *)file->private_data)->private;

	if (!ectrl)
		return -EINVAL;

	if (!capable(CAP_SYS_ADMIN))
                return -EACCES;

        if (count >= INPUT_BUF_SIZE)
                count = INPUT_BUF_SIZE - 1;

	memset(input, 0, INPUT_BUF_SIZE);
	if (copy_from_user (input, buf, count))
		return -EFAULT;	

	err_conv = kstrtoul(input, 0, &id);

	if (err_conv == 0) {
		mutex_lock (&ectrl->fs_lock);
		if (WDT_F_setEvent (ectrl->client, (enum wdt_event)id) < 0) {
			mutex_unlock (&ectrl->fs_lock);
			return -EINVAL;
		}
		mutex_unlock (&ectrl->fs_lock);
	} else
		return err_conv;

	return count;
}


static int ectrl_proc_wdt_f_event_open_fs (struct inode *inode, struct file *file) {
        return single_open(file, ectrl_proc_wdt_f_event_show,  PDE_DATA(inode));
}


static const struct file_operations ectrl_proc_wdt_f_event_fops = {
        .owner = THIS_MODULE,
        .open = ectrl_proc_wdt_f_event_open_fs,
        .read = seq_read,
	.write = ectrl_proc_wdt_f_event_write,
        .llseek = seq_lseek,
        .release = single_release,
};


/* -------------------------- PROC WDT FLASH DELAY FILE OPT (r/w) -------------------------- */

static int ectrl_proc_wdt_f_delay_show (struct seq_file *seq, void *offset) {
	struct econtroller *ectrl = (struct econtroller *)seq->private;
	
	if (!ectrl) 
		return -EINVAL;

	mutex_unlock (&ectrl->fs_lock);
        seq_printf(seq, "%d\n", WDT_F_getDelayReg (ectrl->client));
	mutex_unlock (&ectrl->fs_lock);
        return 0;
}


static ssize_t ectrl_proc_wdt_f_delay_write (struct file *file, const char __user *buf,
			size_t count, loff_t *pos) {
	
	char input[INPUT_BUF_SIZE];
	long delay;
	int  err_conv;
	struct econtroller *ectrl = (struct econtroller *)
		((struct seq_file *)file->private_data)->private;

	if (!ectrl)
		return -EINVAL;

	if (!capable(CAP_SYS_ADMIN))
                return -EACCES;

        if (count >= INPUT_BUF_SIZE)
                count = INPUT_BUF_SIZE - 1;

	memset(input, 0, INPUT_BUF_SIZE);
	if (copy_from_user (input, buf, count))
		return -EFAULT;	

	err_conv = kstrtoul(input, 0, &delay);

	if (err_conv == 0) {
		if ((u16)delay < 1)
			return -EINVAL;
		mutex_lock (&ectrl->fs_lock);
		WDT_F_setDelayReg (ectrl->client, (u16)delay);
		mutex_unlock (&ectrl->fs_lock);
	} else
		return err_conv;

	return count;
}


static int ectrl_proc_wdt_f_delay_open_fs (struct inode *inode, struct file *file) {
        return single_open(file, ectrl_proc_wdt_f_delay_show,  PDE_DATA(inode));
}


static const struct file_operations ectrl_proc_wdt_f_delay_fops = {
        .owner = THIS_MODULE,
        .open = ectrl_proc_wdt_f_delay_open_fs,
        .read = seq_read,
	.write = ectrl_proc_wdt_f_delay_write,
        .llseek = seq_lseek,
        .release = single_release,
};


/* -------------------------- PROC WDT FLASH TIMEOUT FILE OPT (r/w) -------------------------- */

static int ectrl_proc_wdt_f_timeout_show (struct seq_file *seq, void *offset) {
	struct econtroller *ectrl = (struct econtroller *)seq->private;
	
	if (!ectrl) 
		return -EINVAL;

	mutex_lock (&ectrl->fs_lock);
        seq_printf(seq, "%d\n", WDT_F_getTimeoutReg (ectrl->client));
	mutex_unlock (&ectrl->fs_lock);
        return 0;
}


static ssize_t ectrl_proc_wdt_f_timeout_write (struct file *file, const char __user *buf,
			size_t count, loff_t *pos) {
	
	char input[INPUT_BUF_SIZE];
	long timeout;
	int  err_conv;
	struct econtroller *ectrl = (struct econtroller *)
		((struct seq_file *)file->private_data)->private;

	if (!ectrl)
		return -EINVAL;

	if (!capable(CAP_SYS_ADMIN))
                return -EACCES;

        if (count >= INPUT_BUF_SIZE)
                count = INPUT_BUF_SIZE - 1;

	memset(input, 0, INPUT_BUF_SIZE);
	if (copy_from_user (input, buf, count))
		return -EFAULT;	

	err_conv = kstrtoul(input, 0, &timeout);

	if (err_conv == 0) {
		if ((u16)timeout < 1)
			return -EINVAL; 
		mutex_lock (&ectrl->fs_lock);
		WDT_F_setTimeoutReg (ectrl->client, (u16)timeout);
		mutex_unlock (&ectrl->fs_lock);
	} else
		return err_conv;

	return count;
}


static int ectrl_proc_wdt_f_timeout_open_fs (struct inode *inode, struct file *file) {
        return single_open(file, ectrl_proc_wdt_f_timeout_show,  PDE_DATA(inode));
}


static const struct file_operations ectrl_proc_wdt_f_timeout_fops = {
        .owner = THIS_MODULE,
        .open = ectrl_proc_wdt_f_timeout_open_fs,
        .read = seq_read,
	.write = ectrl_proc_wdt_f_timeout_write,
        .llseek = seq_lseek,
        .release = single_release,
};



/* -------------------------- PROC ECTRL REVISION FILE OPT (r) -------------------------- */



static int ectrl_proc_build_revision_show (struct seq_file *seq, void *offset) {
	struct econtroller *ectrl = (struct econtroller *)seq->private;

	if (!ectrl)
		return -EINVAL;

	mutex_lock (&ectrl->fs_lock);
        seq_printf(seq, "0x%04X\n", ectrl_read_data (ectrl->client, BUILDREV_REG));
	mutex_unlock (&ectrl->fs_lock);
        return 0;
}


static int ectrl_proc_build_revision_open_fs (struct inode *inode, struct file *file) {
        return single_open(file, ectrl_proc_build_revision_show,  PDE_DATA(inode));
}


static const struct file_operations ectrl_proc_build_revisione_fops = {
	.owner = THIS_MODULE,
	.open = ectrl_proc_build_revision_open_fs,
	.read = seq_read,
	.write = NULL,
	.llseek = seq_lseek,
	.release = single_release,
};


static int ectrl_proc_board_revision_show (struct seq_file *seq, void *offset) {
	struct econtroller *ectrl = (struct econtroller *)seq->private;

	if (!ectrl)
		return -EINVAL;

	mutex_lock (&ectrl->fs_lock);
        seq_printf(seq, "0x%04X%04X\n", ectrl_read_data (ectrl->client, FW_ID_REV_REG_H),ectrl_read_data (ectrl->client, FW_ID_REV_REG_L));
	mutex_unlock (&ectrl->fs_lock);
        return 0;
}


static int ectrl_proc_board_revision_open_fs (struct inode *inode, struct file *file) {
        return single_open(file, ectrl_proc_board_revision_show,  PDE_DATA(inode));
}


static const struct file_operations ectrl_proc_board_revisione_fops = {
	.owner = THIS_MODULE,
	.open = ectrl_proc_board_revision_open_fs,
	.read = seq_read,
	.write = NULL,
	.llseek = seq_lseek,
	.release = single_release,
};

			/* --------------------------- */
static int ectrl_proc_save_in_flash_show (struct seq_file *seq, void *offset) {
        struct econtroller *ectrl = (struct econtroller *)seq->private;

        if (!ectrl)
                return -EINVAL;

        return 0;
}


static int ectrl_proc_save_in_flash_open_fs (struct inode *inode, struct file *file) {
        return single_open(file, ectrl_proc_save_in_flash_show,  PDE_DATA(inode));
}

static ssize_t ectrl_proc_save_in_flash_write (struct file *file, const char __user *buf,
                        size_t count, loff_t *pos) {

	char input[INPUT_BUF_SIZE];

        struct ectrl_proc_boot *proc = (struct ectrl_proc_boot *)
                ((struct seq_file *)file->private_data)->private;
        struct econtroller *ectrl = (struct econtroller *)
                ((struct seq_file *)file->private_data)->private;

        if (!ectrl)
                return -EINVAL;

        if (!proc)
                return -EINVAL;


        if (!capable(CAP_SYS_ADMIN))
                return -EACCES;

        if (count >= INPUT_BUF_SIZE)
                count = INPUT_BUF_SIZE - 1;

        memset(input, 0, INPUT_BUF_SIZE);
        if (copy_from_user (input, buf, count))
                return -EFAULT;

        if(strcmp(input,"1\n") == 0 ) {
		saveInFlash(ectrl->client);		
	}

        return count;
}

static const struct file_operations ectrl_proc_save_in_flash_fops = {
	.owner = THIS_MODULE,
	.open = ectrl_proc_save_in_flash_open_fs,
	.read = NULL,
	.write = ectrl_proc_save_in_flash_write,
	.llseek = seq_lseek,
	.release = single_release,
};



struct proc_entry {
	char                   label[30];
	struct proc_dir_entry  *entry;
	struct list_head       list; 
};

static struct proc_entry *entry_list;

#define add_to_list(item, head, name, entry_dir)       item = kzalloc (sizeof (struct proc_entry), GFP_KERNEL); \
							strcpy (item->label, name); \
							item->entry = entry_dir; \
							list_add (&item->list, &head->list)



static int ectrl_add_proc_fs (struct econtroller *ectrl) {
	struct proc_dir_entry *entry = NULL;
        int i;

	struct ectrl_proc_event *lid_eproc;
	struct ectrl_proc_event *wake_en_eproc;
	struct ectrl_proc_event *sleep_eproc;

	struct ectrl_proc_event_state *pwr_btn_state_eproc;
	struct ectrl_proc_event_state *fail_wd_state_eproc;
	struct ectrl_proc_event_state *lid_state_eproc;
	struct ectrl_proc_event_state *batlow_state_eproc;
	struct ectrl_proc_event_state *sleep_state_eproc;
	struct ectrl_proc_event_state *fail_bv_state_eproc;

	struct ectrl_proc_boot  *boot0_bproc;
	struct ectrl_proc_boot  *boot1_bproc;
	struct ectrl_proc_boot  *boot2_bproc;
	struct ectrl_proc_boot  *boot3_bproc;
	struct ectrl_proc_boot  *boot_straps_dev_sel;

	struct proc_entry *tmp;
	struct list_head *pos, *q;

	entry_list = kzalloc (sizeof (struct proc_entry), GFP_KERNEL);
	INIT_LIST_HEAD (&entry_list->list);

	/* create /proc/ectrl */
        ectrl_root_dir = proc_mkdir (ECTRL_PROC_ROOT, NULL);
        if (!ectrl_root_dir)
                return -ENODEV;
	else {
		add_to_list (tmp, entry_list, ECTRL_PROC_ROOT, NULL);
	}		

	/* EVENTS TREE */
	if(ectrl->input_interface) {
		/* create /proc/ectrl/events */
		ectrl_events_dir = proc_mkdir (ECTRL_PROC_EVENTS, ectrl_root_dir);
		if (!ectrl_events_dir) {
			//TODO: error
		} else {
			add_to_list (tmp, entry_list, ECTRL_PROC_EVENTS, ectrl_root_dir);
		}


		for (i = 0 ; i < ectrl->nr_evnt; i++) {
			switch (ectrl->events[i]->id) {
				
				case SLEEP_SIGNAL:
					// --- sleep --- //
					ectrl->events[i]->proc_dir = proc_mkdir (ectrl->events[i]->label, ectrl_events_dir);
					if (!ectrl->events[i]->proc_dir) {
						goto remove_dir;
					} else {
						add_to_list (tmp, entry_list, ectrl->events[i]->label, ectrl_events_dir);
					}

					sleep_eproc = kzalloc (sizeof (struct ectrl_proc_event), GFP_KERNEL);
					if (!sleep_eproc) {
						goto remove_dir;
					}
					sleep_eproc->ectrl = ectrl;
					sleep_eproc->event = SLEEP_SIGNAL;
				
					entry = proc_create_data(ECTRL_ENTRY_ENABLE,  S_IRUGO | S_IWUGO,
							global_event_list[ectrl->events[i]->id].proc_dir, &ectrl_proc_enable_fops, 
							sleep_eproc);
					if (!entry) {
						goto remove_dir;
					} else {
						 add_to_list (tmp, entry_list, ECTRL_ENTRY_ENABLE, global_event_list[ectrl->events[i]->id].proc_dir);
					}
				
					break;

				case LID_SIGNAL:
					// --- lid --- //
					ectrl->events[i]->proc_dir = proc_mkdir (ectrl->events[i]->label, ectrl_events_dir);
					if (!ectrl->events[i]->proc_dir) {
						goto remove_dir;
					} else {
						add_to_list (tmp, entry_list, ectrl->events[i]->label, ectrl_events_dir);
					}
					
					lid_eproc = kzalloc (sizeof (struct ectrl_proc_event), GFP_KERNEL);
					if (!lid_eproc) {
						goto remove_dir;
					}
					lid_eproc->ectrl = ectrl;
					lid_eproc->event = LID_SIGNAL;
				
					entry = proc_create_data(ECTRL_ENTRY_ENABLE,  S_IRUGO | S_IWUGO,
							global_event_list[ectrl->events[i]->id].proc_dir, &ectrl_proc_enable_fops, 
							lid_eproc);
					if (!entry) {
						goto remove_dir;
					} else {
						add_to_list (tmp, entry_list, ECTRL_ENTRY_ENABLE, global_event_list[ectrl->events[i]->id].proc_dir);
					}
				
					break;
				
				case WAKE_EN:
					// --- Wake_en --- //
					ectrl->events[i]->proc_dir = proc_mkdir (ectrl->events[i]->label, ectrl_events_dir);
					if (!ectrl->events[i]->proc_dir) {
						goto remove_dir;
					} else {
						add_to_list (tmp, entry_list, ectrl->events[i]->label, ectrl_events_dir);
					}
				
					wake_en_eproc = kzalloc (sizeof (struct ectrl_proc_event), GFP_KERNEL);
					if (!wake_en_eproc) {
						goto remove_dir;
					}
					wake_en_eproc->ectrl = ectrl;
					wake_en_eproc->event = WAKE_EN;
					entry = proc_create_data(ECTRL_ENTRY_ENABLE,  S_IRUGO | S_IWUGO,
							global_event_list[ectrl->events[i]->id].proc_dir, &ectrl_proc_enable_fops, 
							wake_en_eproc);
					if (!entry) {
						goto remove_dir;
					} else {
						add_to_list (tmp, entry_list, ECTRL_ENTRY_ENABLE, global_event_list[ectrl->events[i]->id].proc_dir);
					}

					break;

				default:
					continue;

			}
		}
	}
	if(ectrl->input_interface) {	
		/* create /proc/ectrl/events/event_state */
		ectrl_events_state_dir = proc_mkdir (ECTRL_PROC_EVENTS_STATE, ectrl_events_dir);
		if (!ectrl_events_state_dir) {
			goto remove_dir;		
		} else {
			add_to_list (tmp, entry_list, ECTRL_PROC_EVENTS_STATE, ectrl_events_dir);
		}
		for (i = 0 ; i < ectrl->nr_evn ; i++) {
			struct ectrl_proc_event_state *state_proc;
			switch (ectrl->evn_state[i]->evn) {
				case EVNT_FAIL_BV:
					fail_bv_state_eproc = kzalloc (sizeof (struct ectrl_proc_event_state),
							 GFP_KERNEL);
					fail_bv_state_eproc->ectrl = ectrl;
					state_proc = fail_bv_state_eproc;
					break;
				case EVNT_FAIL_WD:
					fail_wd_state_eproc = kzalloc (sizeof (struct ectrl_proc_event_state),
							 GFP_KERNEL);
					fail_wd_state_eproc->ectrl = ectrl;
					state_proc = fail_wd_state_eproc;
					break;
				case EVNT_BATLOW_SIGNAL:
					batlow_state_eproc = kzalloc (sizeof (struct ectrl_proc_event_state),
							 GFP_KERNEL);
					batlow_state_eproc->ectrl = ectrl;
					state_proc = batlow_state_eproc;
					break;
				case EVNT_SLEEP_SIGNAL:
					sleep_state_eproc = kzalloc (sizeof (struct ectrl_proc_event_state),
							 GFP_KERNEL);
					sleep_state_eproc->ectrl = ectrl;
					state_proc = sleep_state_eproc;
					break;
				case EVNT_LID_SIGNAL:
					lid_state_eproc = kzalloc (sizeof (struct ectrl_proc_event_state),
							 GFP_KERNEL);
					lid_state_eproc->ectrl = ectrl;
					state_proc = lid_state_eproc;
					break;
				case EVNT_PWR_BUTTON:
					pwr_btn_state_eproc = kzalloc (sizeof (struct ectrl_proc_event_state),
							 GFP_KERNEL);
					pwr_btn_state_eproc->ectrl = ectrl;
					state_proc = pwr_btn_state_eproc;
					break;
				default:
					state_proc = NULL;
					break;
			}	

			if (state_proc != NULL) {
				state_proc->state_id = i;
				entry = proc_create_data(ectrl->evn_state[i]->label, 
					S_IRUGO, ectrl_events_state_dir, &ectrl_proc_status_fops, state_proc);
				if (!entry) {
					goto remove_dir;
				} else {
					add_to_list (tmp, entry_list, ectrl->evn_state[i]->label, ectrl_events_state_dir);
				}
			}
			
		}
	}

	/* POWER MANAGEMENT TREE */
#if 0
	/* create /proc/ectrl/power_management */
	ectrl_pm_dir = proc_mkdir (ECTRL_PROC_PM, ectrl_root_dir);
	if (!ectrl_pm_dir) {
		goto remove_dir;
	} else {
		add_to_list (tmp, entry_list, ECTRL_PROC_PM, ectrl_root_dir);
	}

	entry = proc_create_data(ECTRL_ENTRY_PM_STATE, S_IRUGO,
				ectrl_pm_dir, &ectrl_proc_available_pm_state_fops, ectrl);
	if (!entry) {
                goto remove_dir;		
	} else {
		add_to_list (tmp, entry_list, ECTRL_ENTRY_PM_STATE, ectrl_pm_dir);
	}

	entry = proc_create_data(ECTRL_ENTRY_PM_PWR_STATE, S_IRUGO | S_IWUGO,
				ectrl_pm_dir, &ectrl_proc_pm_pwr_state_fops, ectrl);
	if (!entry) {
                goto remove_dir;		
	} else {
		 add_to_list (tmp, entry_list, ECTRL_ENTRY_PM_PWR_STATE, ectrl_pm_dir);
	}
	
	ectrl_pm_pwr_btn_4sec_dir = proc_mkdir (ECTRL_ENTRY_PM_PWR_BTN_4SEC, ectrl_pm_dir);
	if (!ectrl_pm_dir) {
		goto remove_dir;
	} else {
		add_to_list (tmp, entry_list, ECTRL_ENTRY_PM_PWR_BTN_4SEC, ectrl_pm_dir);
	}
	
	pwr_btn_4sec_eproc = kzalloc (sizeof (struct ectrl_proc_event), GFP_KERNEL);
	if (!pwr_btn_4sec_eproc) {
		goto remove_dir;
	}
	pwr_btn_4sec_eproc->ectrl = ectrl;
	pwr_btn_4sec_eproc->event = PWR_BTN_4SEC;
	pwr_btn_4sec_eproc->is_true_event = 0;

	entry = proc_create_data(ECTRL_ENTRY_PM_PWR_BTN_4SEC_EN,  S_IRUGO | S_IWUGO,
			ectrl_pm_pwr_btn_4sec_dir, &ectrl_proc_enable_fops, 
			pwr_btn_4sec_eproc);
	if (!entry) {
		goto remove_dir;		
	} else {
		add_to_list (tmp, entry_list, ECTRL_ENTRY_PM_PWR_BTN_4SEC_EN, ectrl_pm_pwr_btn_4sec_dir);
	}

	entry = proc_create_data(ECTRL_ENTRY_PM_PWR_BTN_4SEC_ENFLASH,  S_IRUGO | S_IWUGO,
			ectrl_pm_pwr_btn_4sec_dir, &ectrl_proc_enflash_fops, 
			pwr_btn_4sec_eproc);
	if (!entry) {
		goto remove_dir;
	} else {
		add_to_list (tmp, entry_list, ECTRL_ENTRY_PM_PWR_BTN_4SEC_ENFLASH, ectrl_pm_pwr_btn_4sec_dir);
	}
#endif
	/* BOOT TREE */

	/* create /proc/ectrl/boot */
	ectrl_boot_dir = proc_mkdir (ECTRL_PROC_BOOT, ectrl_root_dir);
	if (!ectrl_boot_dir) {
		goto remove_dir;
	} else {
		add_to_list (tmp, entry_list, ECTRL_PROC_BOOT, ectrl_root_dir);
	}

	boot0_bproc 		= kzalloc (sizeof (struct ectrl_proc_boot), GFP_KERNEL);
	boot1_bproc 		= kzalloc (sizeof (struct ectrl_proc_boot), GFP_KERNEL);
	boot2_bproc 		= kzalloc (sizeof (struct ectrl_proc_boot), GFP_KERNEL);
	boot3_bproc 		= kzalloc (sizeof (struct ectrl_proc_boot), GFP_KERNEL);
	boot_straps_dev_sel 	= kzalloc (sizeof (struct ectrl_proc_boot), GFP_KERNEL);

	if (!boot0_bproc || !boot1_bproc || !boot2_bproc || !boot3_bproc || !boot_straps_dev_sel) {
		goto remove_dir;
	}

	i = 0;
	if(i < ectrl->nr_bootdev ) {
		boot0_bproc->ectrl = ectrl;
		boot0_bproc->idx = BOOT_IDX0;
		entry = proc_create_data(ECTRL_ENTRY_DEVBOOT0,  S_IRUGO | S_IWUGO,
					ectrl_boot_dir, &ectrl_proc_bootdev_fops, boot0_bproc);
		if (!entry) {
			goto remove_dir;		
		} else {
			add_to_list (tmp, entry_list, ECTRL_ENTRY_DEVBOOT0, ectrl_boot_dir);
		}
	}

	i++;
	if(i < ectrl->nr_bootdev ) {
		boot1_bproc->ectrl = ectrl;
		boot1_bproc->idx = BOOT_IDX1;
		entry = proc_create_data(ECTRL_ENTRY_DEVBOOT1,  S_IRUGO | S_IWUGO,
					ectrl_boot_dir, &ectrl_proc_bootdev_fops, boot1_bproc);
		if (!entry) {
			goto remove_dir;		
		} else {
			add_to_list (tmp, entry_list, ECTRL_ENTRY_DEVBOOT1, ectrl_boot_dir);
		}
	}

	i++;
	if(i < ectrl->nr_bootdev ) {
		boot2_bproc->ectrl = ectrl;
		boot2_bproc->idx = BOOT_IDX2;
		entry = proc_create_data(ECTRL_ENTRY_DEVBOOT2,  S_IRUGO | S_IWUGO,
					ectrl_boot_dir, &ectrl_proc_bootdev_fops, boot2_bproc);
		if (!entry) {
			goto remove_dir;		
		} else {
			add_to_list (tmp, entry_list, ECTRL_ENTRY_DEVBOOT2, ectrl_boot_dir);
		}
	}

	i++;
	if(i < ectrl->nr_bootdev ) {
		boot3_bproc->ectrl = ectrl;
		boot3_bproc->idx = BOOT_IDX3;
		entry = proc_create_data(ECTRL_ENTRY_RECOVERY_BOOT, S_IRUGO | S_IWUGO,
					ectrl_boot_dir, &ectrl_proc_bootdev_fops, boot3_bproc);
		if (!entry) {
			goto remove_dir;		
		} else {
			add_to_list (tmp, entry_list, ECTRL_ENTRY_RECOVERY_BOOT, ectrl_boot_dir);
		}
	}
	
	boot_straps_dev_sel->ectrl = ectrl;
	boot_straps_dev_sel->idx = BOOT_STRAPS_DEV_SEL;
        entry = proc_create_data(ECTRL_ENTRY_STRAPS_DEV_SEL, S_IRUGO | S_IWUGO,
                                ectrl_boot_dir, &ectrl_proc_bootdev_fops, boot_straps_dev_sel);
        if (!entry) {
                goto remove_dir;
        } else {
                add_to_list (tmp, entry_list, ECTRL_ENTRY_STRAPS_DEV_SEL, ectrl_boot_dir);
        }


	entry = proc_create_data(ECTRL_ENTRY_BOOTDEV_LIST, S_IRUGO,
				ectrl_boot_dir, &ectrl_proc_bootdev_list_fops, ectrl);
	if (!entry) {
                goto remove_dir;		
	} else {
		add_to_list (tmp, entry_list, ECTRL_ENTRY_BOOTDEV_LIST, ectrl_boot_dir);
	}

	entry = proc_create_data(ECTRL_ENTRY_BOOT_SEQUENCE, S_IRUGO | S_IWUGO,
				ectrl_boot_dir, &ectrl_proc_bootseq_fops, ectrl);
	if (!entry) {
                goto remove_dir;		
	} else {
		add_to_list (tmp, entry_list, ECTRL_ENTRY_BOOT_SEQUENCE, ectrl_boot_dir);
	}

	/* WATCHDOG TREE */

	/* create /proc/ectrl/watchdog */
	ectrl_watchdog_dir = proc_mkdir (ECTRL_PROC_WATCHDOG, ectrl_root_dir);
	if (!ectrl_watchdog_dir) {
		goto remove_dir;
	} else {
		add_to_list (tmp, entry_list, ECTRL_PROC_WATCHDOG, ectrl_root_dir);
	}

	entry = proc_create_data(ECTRL_ENTRY_WDT_AVAL_EVN, S_IRUGO,
				ectrl_watchdog_dir, &ectrl_proc_avalaible_wdt_event_fops, ectrl);
	if (!entry) {
                goto remove_dir;		
	} else {
		add_to_list (tmp, entry_list, ECTRL_ENTRY_WDT_AVAL_EVN, ectrl_watchdog_dir);
	}

	entry = proc_create_data(ECTRL_ENTRY_WDT_ENABLE, S_IRUGO | S_IWUGO,   
				ectrl_watchdog_dir, &ectrl_proc_wdt_enable_fops, ectrl);
	if (!entry) {
                goto remove_dir;		
	} else {
		add_to_list (tmp, entry_list, ECTRL_ENTRY_WDT_ENABLE, ectrl_watchdog_dir);
	}

	entry = proc_create_data(ECTRL_ENTRY_WDT_EVENT, S_IRUGO | S_IWUGO,   
				ectrl_watchdog_dir, &ectrl_proc_wdt_event_fops, ectrl);
	if (!entry) {
                goto remove_dir;		
	} else {
		add_to_list (tmp, entry_list, ECTRL_ENTRY_WDT_EVENT, ectrl_watchdog_dir);
	}

	entry = proc_create_data(ECTRL_ENTRY_WDT_DELAY, S_IRUGO | S_IWUGO,
				ectrl_watchdog_dir, &ectrl_proc_wdt_delay_fops, ectrl);
	if (!entry) {
                goto remove_dir;		
	} else {
		add_to_list (tmp, entry_list, ECTRL_ENTRY_WDT_DELAY, ectrl_watchdog_dir);
	}

	entry = proc_create_data(ECTRL_ENTRY_WDT_TIMEOUT, S_IRUGO | S_IWUGO,
				ectrl_watchdog_dir, &ectrl_proc_wdt_timeout_fops, ectrl);
	if (!entry) {
                goto remove_dir;		
	} else {
		add_to_list (tmp, entry_list, ECTRL_ENTRY_WDT_TIMEOUT, ectrl_watchdog_dir);
	}


	entry = proc_create_data(ECTRL_ENTRY_WDT_STATE, S_IRUGO,   
				ectrl_watchdog_dir, &ectrl_proc_wdt_state_fops, ectrl);
	if (!entry) {
                goto remove_dir;		
	} else {
		 add_to_list (tmp, entry_list, ECTRL_ENTRY_WDT_STATE, ectrl_watchdog_dir);
	}

	entry = proc_create_data(ECTRL_ENTRY_WDT_REFRESH, S_IWUGO,   
				ectrl_watchdog_dir, &ectrl_proc_wdt_refresh_fops, ectrl);
	if (!entry) {
                goto remove_dir;		
	} else {
		add_to_list (tmp, entry_list, ECTRL_ENTRY_WDT_REFRESH, ectrl_watchdog_dir);
	}

	entry = proc_create_data(ECTRL_ENTRY_WDT_RESTORE, S_IWUGO,   
				ectrl_watchdog_dir, &ectrl_proc_wdt_restore_fops, ectrl);
	if (!entry) {
                goto remove_dir;		
	} else {
		add_to_list (tmp, entry_list, ECTRL_ENTRY_WDT_RESTORE, ectrl_watchdog_dir);
	}

	entry = proc_create_data(ECTRL_ENTRY_WDT_WDTOUT_RESET, S_IWUGO,   
				ectrl_watchdog_dir, &ectrl_proc_wdt_wdtout_reset_fops, ectrl);
	if (!entry) {
                goto remove_dir;		
	} else {
		add_to_list (tmp, entry_list, ECTRL_ENTRY_WDT_WDTOUT_RESET, ectrl_watchdog_dir);
	}

	/* revision identification */
	entry = proc_create_data("build_rev", S_IRUGO, 0, &ectrl_proc_build_revisione_fops, ectrl);

	entry = proc_create_data("board_rev", S_IRUGO, 0, &ectrl_proc_board_revisione_fops, ectrl);

	entry = proc_create_data("save_in_flash", S_IWUGO, ectrl_root_dir, &ectrl_proc_save_in_flash_fops, ectrl);



	return 0;
remove_dir:
	list_for_each_safe (pos, q, &entry_list->list) {
		tmp = list_entry (pos, struct proc_entry, list);
		remove_proc_entry (tmp->label, tmp->entry);
		list_del (pos);
		kfree (tmp);	
	}
	return -EINVAL;
}


static void ectrl_remove_proc_fs (struct econtroller *ectrl) {
	struct proc_entry *tmp;
	struct list_head *pos, *q;
	list_for_each_safe (pos, q, &entry_list->list) {
		tmp = list_entry (pos, struct proc_entry, list);
		remove_proc_entry (tmp->label, tmp->entry);
		list_del (pos);
		kfree (tmp);	
	}
}


/* --------------------------------------------------------------------------
                                     IOCTL
   -------------------------------------------------------------------------- */

static long ectrl_ioctl (struct file *file, unsigned int cmd, unsigned long arg) {

	struct i2c_client  *client = file->private_data;
	struct econtroller *ectrl = dev_get_drvdata (&client->dev);
	int err = 0;
	int retval = 0;
	dev_dbg(&client->dev, "ioctl, cmd=0x%02x, arg=0x%02lx\n", cmd, arg);

	switch (cmd) {

		case ECTRL_IOCTL_REG_READ: {
			struct ectrl_reg reg;
			struct ectrl_reg_rw reg_rw;
			mutex_lock (&ectrl->ioctl_lock);
			if (copy_from_user (&reg, (const void __user *)arg, sizeof (reg))) {
				mutex_unlock (&ectrl->ioctl_lock);
				return -EFAULT;
			}
			get_reg_rw (&reg_rw, reg, R_OP);
			err = ectrl_mem_single_op (client, &reg_rw);
			reg.data = reg_rw.reg.data;
			if (err < 0)
				retval = err;
			if (copy_to_user ((void __user *)arg, &reg, sizeof (reg))) {
				mutex_unlock (&ectrl->ioctl_lock);
				retval = -EFAULT;
			}
			mutex_unlock (&ectrl->ioctl_lock);
			break;
		}

		case ECTRL_IOCTL_REG_WRITE: {
			struct ectrl_reg reg;
			struct ectrl_reg_rw reg_rw;
			mutex_lock (&ectrl->ioctl_lock);
			if (copy_from_user (&reg, (const void __user *)arg, sizeof (reg))) {
				mutex_unlock (&ectrl->ioctl_lock);
				return -EFAULT;
			}
			get_reg_rw (&reg_rw, reg, W_OP);
			err = ectrl_mem_single_op (client, &reg_rw);
			reg.data = reg_rw.reg.data;
			if (err < 0)
				retval = err;
			if (copy_to_user ((void __user *)arg, &reg, sizeof (reg))) {
				mutex_unlock (&ectrl->ioctl_lock);
				retval = -EFAULT;
			}
			mutex_unlock (&ectrl->ioctl_lock);
			break;
		}

		case ECTRL_EV_STATE_GET: {
//			struct ectrl_ev_state ev_state;
//
//			mutex_lock (&ectrl->ioctl_lock);
//
//			if ( copy_from_user (&ev_state, (const void __user *)arg, sizeof (ev_state)) ) {
//				mutex_unlock (&ectrl->ioctl_lock);
//				return -EFAULT;
//			}
//
//			if ( !is_feasibleEvent (ectrl, ev_state.id) ) {
//				mutex_unlock (&ectrl->ioctl_lock);
//				return -EINVAL;
//			}
//
//			mutex_lock (&ectrl->fs_lock);
//			ev_state.v_enable = getEnable (ectrl->client, ev_state.id);
//			ev_state.f_enable = getEnFlash (ectrl->client, ev_state.id);
//			mutex_unlock (&ectrl->fs_lock);
//
//			if ( copy_to_user ((void __user *)arg, &ev_state, sizeof (ev_state)) ) {
//				mutex_unlock (&ectrl->ioctl_lock);
//				return -EFAULT;
//			}
//
//			mutex_unlock (&ectrl->ioctl_lock);
			break;

		}

		case ECTRL_EV_STATE_SET: {
			struct ectrl_ev_state ev_state;

			mutex_lock (&ectrl->ioctl_lock);

			if ( copy_from_user (&ev_state, (const void __user *)arg, sizeof (ev_state)) ) {
				mutex_unlock (&ectrl->ioctl_lock);
				return -EFAULT;
			}

			mutex_lock (&ectrl->fs_lock);

			if ( !is_feasibleEvent (ectrl, ev_state.id) ) {
				mutex_unlock (&ectrl->fs_lock);
				mutex_unlock (&ectrl->ioctl_lock);
				return -EINVAL;
			}

			setEnable (ectrl->client, ev_state.id, !!ev_state.v_enable);
			setEnFlash (ectrl->client, ev_state.id, !!ev_state.f_enable);
			mutex_unlock (&ectrl->fs_lock);

			mutex_unlock (&ectrl->ioctl_lock);
			break;
		}

		case ECTRL_PWR_STATE_GET: {
			enum ECTRL_POWER_STATE pwr_state;

			mutex_lock (&ectrl->ioctl_lock);

			mutex_lock (&ectrl->fs_lock);
			pwr_state = PMgetAlwaysState (ectrl->client) == (u16)ALWAYS_ON ? PWR_ALWAYS_ON : PWR_ALWAYS_OFF;
			mutex_unlock (&ectrl->fs_lock);

			if ( copy_to_user ((void __user *)arg, &pwr_state, sizeof (pwr_state)) ) {
				mutex_unlock (&ectrl->ioctl_lock);
				return -EFAULT;
			}

			mutex_unlock (&ectrl->ioctl_lock);
			break;
		}

		case ECTRL_PWR_STATE_SET: {
			enum ECTRL_POWER_STATE pwr_state;

			mutex_lock (&ectrl->ioctl_lock);

			if ( copy_from_user (&pwr_state, (const void __user *)arg, sizeof (pwr_state)) ) {
				mutex_unlock (&ectrl->ioctl_lock);
				return -EFAULT;
			}

			if ( pwr_state != PWR_ALWAYS_OFF && pwr_state != PWR_ALWAYS_ON ) {
				mutex_unlock (&ectrl->ioctl_lock);
				return -EINVAL;
			}

			mutex_lock (&ectrl->fs_lock);
			if ( pwr_state == PWR_ALWAYS_OFF ) {
				PMsetAlwaysState (ectrl->client, ALWAYS_OFF);
			} else {
				PMsetAlwaysState (ectrl->client, ALWAYS_ON);
			}
			mutex_unlock (&ectrl->fs_lock);

			mutex_unlock (&ectrl->ioctl_lock);

			break;
		}

		case ECTRL_PWR_BTN_4SEC_STATE_GET: {
//			struct ectrl_ev_state ev_state;
//
//			mutex_lock (&ectrl->ioctl_lock);
//
//			mutex_lock (&ectrl->fs_lock);
//			ev_state.id       = PWR_BTN_4SEC;
//			ev_state.v_enable = getEnable (ectrl->client, ev_state.id);
//			ev_state.f_enable = getEnFlash (ectrl->client, ev_state.id);
//			mutex_unlock (&ectrl->fs_lock);
//
//			if ( copy_to_user ((void __user *)arg, &ev_state, sizeof (ev_state)) ) {
//				mutex_lock (&ectrl->fs_lock);
//				mutex_unlock (&ectrl->ioctl_lock);
//				return -EFAULT;
//			}
//			mutex_unlock (&ectrl->ioctl_lock);
//
			break;
		}

		case ECTRL_PWR_BTN_4SEC_STATE_SET: {
			struct ectrl_ev_state ev_state;

			mutex_lock (&ectrl->ioctl_lock);
			if ( copy_from_user (&ev_state, (const void __user *)arg, sizeof (ev_state)) ) {
				mutex_unlock (&ectrl->ioctl_lock);
				return -EFAULT;
			}

			ev_state.id = PWR_BTN_4SEC;
			mutex_lock (&ectrl->fs_lock);
			setEnable (ectrl->client, ev_state.id, !!ev_state.v_enable);
			setEnFlash (ectrl->client, ev_state.id, !!ev_state.f_enable);
			mutex_unlock (&ectrl->fs_lock);

			mutex_unlock (&ectrl->ioctl_lock);
			break;
		}

		case ECTRL_BOOT_NUM_DEV_GET: {
			int n_dev = ectrl->nr_bootdev;

			mutex_lock (&ectrl->ioctl_lock);
			if ( copy_to_user ((void __user *)arg, &n_dev, sizeof (n_dev)) ) {
				mutex_unlock (&ectrl->ioctl_lock);
				return -EFAULT;
			}

			mutex_unlock (&ectrl->ioctl_lock);
			break;
		}

		case ECTRL_BOOT_DEV_LIST_GET: {
			int                      n_dev    = ectrl->nr_bootdev;
			int                      i, size;
			struct ectrl_boot_device *bd_list = NULL;

			mutex_lock (&ectrl->ioctl_lock);

			size = sizeof (struct ectrl_boot_device) * n_dev;
			bd_list = kzalloc (size, GFP_KERNEL);
			if ( !bd_list ) {
				mutex_unlock (&ectrl->ioctl_lock);
				return -ENOMEM;
			}

			mutex_lock (&ectrl->fs_lock);
			for (i = 0 ; i < ectrl->nr_bootdev ; i++) {
				bd_list[i].id = ectrl->bootdev_list[i].id;
				strcpy (bd_list[i].label, ectrl->bootdev_list[i].label);
			}
			mutex_unlock (&ectrl->fs_lock);

			if ( copy_to_user ((void __user *)arg, bd_list, size) ) {
				mutex_unlock (&ectrl->ioctl_lock);
				return -EFAULT;
			}

			kfree (bd_list);

			mutex_unlock (&ectrl->ioctl_lock);
			break;
		}

		case ECTRL_BOOT_DEV_1_GET: {
			struct ectrl_boot_device bd;
			enum BOOTDEV_ID          bootdev;
			int                      index;

			mutex_lock (&ectrl->ioctl_lock);
			mutex_lock (&ectrl->fs_lock);
			bootdev = (enum BOOTDEV_ID)getBootDev (ectrl, 0);
			index = isAvaildableBootdev (ectrl, bootdev);

			if ( index >= 0 ) {
				bd.id = ectrl->bootdev_list[index].id;
				strcpy (bd.label, ectrl->bootdev_list[index].label);
			} else {
				mutex_unlock (&ectrl->fs_lock);
				mutex_unlock (&ectrl->ioctl_lock);
				return -EINVAL;
			}
			mutex_unlock (&ectrl->fs_lock);

			if ( copy_to_user ((void __user *)arg, &bd, sizeof (bd)) ) {
				mutex_unlock (&ectrl->ioctl_lock);
				return -EFAULT;
			}

			mutex_unlock (&ectrl->ioctl_lock);
			break;
		}

		case ECTRL_BOOT_DEV_2_GET: {
			struct ectrl_boot_device bd;
			enum BOOTDEV_ID          bootdev;
			int                      index;

			mutex_lock (&ectrl->ioctl_lock);
			mutex_lock (&ectrl->fs_lock);
			bootdev = (enum BOOTDEV_ID)getBootDev (ectrl, 1);
			index = isAvaildableBootdev (ectrl, bootdev);

			if ( index >= 0 ) {
				bd.id = ectrl->bootdev_list[index].id;
				strcpy (bd.label, ectrl->bootdev_list[index].label);
			} else {
				mutex_unlock (&ectrl->fs_lock);
				mutex_unlock (&ectrl->ioctl_lock);
				return -EINVAL;
			}
			mutex_unlock (&ectrl->fs_lock);

			if ( copy_to_user ((void __user *)arg, &bd, sizeof (bd)) ) {
				mutex_unlock (&ectrl->ioctl_lock);
				return -EFAULT;
			}

			mutex_unlock (&ectrl->ioctl_lock);
			break;
		}

		case ECTRL_BOOT_DEV_3_GET: {
			struct ectrl_boot_device bd;
			enum BOOTDEV_ID          bootdev;
			int                      index;

			mutex_lock (&ectrl->ioctl_lock);
			mutex_lock (&ectrl->fs_lock);
			bootdev = (enum BOOTDEV_ID)getBootDev (ectrl, 2);
			index = isAvaildableBootdev (ectrl, bootdev);

			if ( index >= 0 ) {
				bd.id = ectrl->bootdev_list[index].id;
				strcpy (bd.label, ectrl->bootdev_list[index].label);
			} else {
				mutex_unlock (&ectrl->fs_lock);
				mutex_unlock (&ectrl->ioctl_lock);
				return -EINVAL;
			}
			mutex_unlock (&ectrl->fs_lock);

			if ( copy_to_user ((void __user *)arg, &bd, sizeof (bd)) ) {
				mutex_unlock (&ectrl->ioctl_lock);
				return -EFAULT;
			}

			mutex_unlock (&ectrl->ioctl_lock);
			break;
		}

		case ECTRL_BOOT_DEV_RECOVERY_GET: {
			struct ectrl_boot_device bd;
			enum BOOTDEV_ID          bootdev;
			int                      index;

			mutex_lock (&ectrl->ioctl_lock);
			mutex_lock (&ectrl->fs_lock);
			bootdev = (enum BOOTDEV_ID)getBootDev (ectrl, 3);
			index = isAvaildableBootdev (ectrl, bootdev);

			if ( index >= 0 ) {
				bd.id = ectrl->bootdev_list[index].id;
				strcpy (bd.label, ectrl->bootdev_list[index].label);
			} else {
				mutex_unlock (&ectrl->fs_lock);
				mutex_unlock (&ectrl->ioctl_lock);
				return -EINVAL;
			}
			mutex_unlock (&ectrl->fs_lock);

			if ( copy_to_user ((void __user *)arg, &bd, sizeof (bd)) ) {
				mutex_unlock (&ectrl->ioctl_lock);
				return -EFAULT;
			}

			mutex_unlock (&ectrl->ioctl_lock);
			break;
		}

		case ECTRL_BOOT_DEV_1_SET: {
			enum BOOTDEV_ID  bootdev;

			mutex_lock (&ectrl->ioctl_lock);
			if ( copy_from_user (&bootdev, (const void __user *)arg, sizeof (bootdev)) ) {
				mutex_unlock (&ectrl->ioctl_lock);
				return -EFAULT;
			}

			mutex_lock (&ectrl->fs_lock);
			if ( setBootDev (ectrl, 0, bootdev) < 0 ) {
				mutex_unlock (&ectrl->fs_lock);
				mutex_unlock (&ectrl->ioctl_lock);
				return -EINVAL;
			}
			mutex_unlock (&ectrl->fs_lock);

			mutex_unlock (&ectrl->ioctl_lock);
			break;
		}


		case ECTRL_BOOT_DEV_2_SET: {
			enum BOOTDEV_ID  bootdev;

			mutex_unlock (&ectrl->ioctl_lock);
			if ( copy_from_user (&bootdev, (const void __user *)arg, sizeof (bootdev)) ) {
				mutex_unlock (&ectrl->ioctl_lock);
				return -EFAULT;
			}

			mutex_lock (&ectrl->fs_lock);
			if ( setBootDev (ectrl, 1, bootdev) < 0 ) {
				mutex_unlock (&ectrl->fs_lock);
				mutex_unlock (&ectrl->ioctl_lock);
				return -EINVAL;
			}
			mutex_unlock (&ectrl->fs_lock);

			mutex_unlock (&ectrl->ioctl_lock);
			break;
		}

		case ECTRL_BOOT_DEV_3_SET: {
			enum BOOTDEV_ID  bootdev;

			mutex_lock (&ectrl->ioctl_lock);
			if ( copy_from_user (&bootdev, (const void __user *)arg, sizeof (bootdev)) ) {
				mutex_unlock (&ectrl->ioctl_lock);
				return -EFAULT;
			}

			mutex_lock (&ectrl->fs_lock);
			if ( setBootDev (ectrl, 2, bootdev) < 0 ) {
				mutex_unlock (&ectrl->fs_lock);
				mutex_unlock (&ectrl->ioctl_lock);
				return -EINVAL;
			}
			mutex_unlock (&ectrl->fs_lock);

			mutex_unlock (&ectrl->ioctl_lock);
			break;
		}

		case ECTRL_BOOT_DEV_RECOVERY_SET: {
			enum BOOTDEV_ID  bootdev;

			mutex_lock (&ectrl->ioctl_lock);
			if ( copy_from_user (&bootdev, (const void __user *)arg, sizeof (bootdev)) ) {
				mutex_unlock (&ectrl->ioctl_lock);
				return -EFAULT;
			}

			mutex_lock (&ectrl->fs_lock);
			if ( setBootDev (ectrl, 3, bootdev) < 0 ) {
				mutex_unlock (&ectrl->fs_lock);
				mutex_unlock (&ectrl->ioctl_lock);
				return -EINVAL;
			}
			mutex_unlock (&ectrl->fs_lock);
			mutex_unlock (&ectrl->ioctl_lock);

			break;
		}

		case ECTRL_WDT_NUM_EVENT_GET: {
			int n_event = ectrl->nr_wdt_event;

			mutex_lock (&ectrl->ioctl_lock);
			if ( copy_to_user ((void __user *)arg, &n_event, sizeof (n_event)) ) {
				mutex_unlock (&ectrl->ioctl_lock);
				return -EFAULT;
			}

			mutex_unlock (&ectrl->ioctl_lock);
			break;
		}

		case ECTRL_WDT_EVENT_LIST_GET: {
			int                   n_event  = ectrl->nr_wdt_event;
			int                   i, size;
			struct wdt_event_data *ev_list = NULL;

			mutex_lock (&ectrl->ioctl_lock);

			size = sizeof (struct wdt_event_data) * n_event;
			ev_list = kzalloc (size, GFP_KERNEL);
			if ( !ev_list ) {
				mutex_unlock (&ectrl->ioctl_lock);
				return -ENOMEM;
			}

			mutex_lock (&ectrl->fs_lock);
			for ( i = 0 ; i < n_event ; i++ ) {
				ev_list[i].id = ectrl->wdt_event_list[i].id;
				strcpy (ev_list[i].label, ectrl->wdt_event_list[i].label);
			}
			mutex_unlock (&ectrl->fs_lock);

			if ( copy_to_user ((void __user *)arg, ev_list, size) ) {
				mutex_unlock (&ectrl->ioctl_lock);
				return -EFAULT;
			}

			kfree (ev_list);

			mutex_unlock (&ectrl->ioctl_lock);
			break;
		}

		case ECTRL_WDT_GET: {
			struct ectrl_wdt wdt;

			mutex_lock (&ectrl->ioctl_lock);
			mutex_lock (&ectrl->fs_lock);
			wdt.v_enable      = WDTgetEnableReg (ectrl->client);
			wdt.v_event       = WDTgetEvent (ectrl->client);
			wdt.v_delay_sec   = WDTgetDelayReg (ectrl->client);
			wdt.v_timeout_sec = WDTgetTimeoutReg (ectrl->client);

			wdt.f_enable      = WDT_F_getEnableReg (ectrl->client);
			wdt.f_event       = WDT_F_getEventReg (ectrl->client);
			wdt.f_delay_sec   = WDT_F_getDelayReg (ectrl->client);
			wdt.f_timeout_sec = WDT_F_getTimeoutReg (ectrl->client);

			wdt.timer_delay = WDTgetTimer1Reg (ectrl->client);
			wdt.timer_wd    = WDTgetTimer2Reg (ectrl->client);
			mutex_unlock (&ectrl->fs_lock);

			if ( copy_to_user ((void __user *)arg, &wdt, sizeof (wdt)) ) {
				mutex_unlock (&ectrl->ioctl_lock);
				return -EFAULT;
			}

			mutex_unlock (&ectrl->ioctl_lock);
			break;
		}

		case ECTRL_WDT_SET: {
			struct ectrl_wdt wdt;
			int              is_correct = 1;
			enum wdt_event   v_id, f_id;

			mutex_lock (&ectrl->ioctl_lock);
			if ( copy_from_user (&wdt, (const void __user *)arg, sizeof (wdt)) ) {
				mutex_unlock (&ectrl->ioctl_lock);
				return -EFAULT;
			}

			if ( wdt.v_enable != 0 && wdt.v_enable != 1 )
				is_correct = 0;
			if ( wdt.v_delay_sec < 1 )
				is_correct = 0;
			if ( wdt.v_timeout_sec < 1 )
				is_correct = 0;

			if ( wdt.f_enable != 0 && wdt.f_enable != 1 )
				is_correct = 0;
			if ( wdt.f_delay_sec < 1 )
				is_correct = 0;
			if ( wdt.f_timeout_sec < 1 )
				is_correct = 0;

			if ( is_correct == 0 ) {
				mutex_unlock (&ectrl->ioctl_lock);
				return -EINVAL;
			}

			mutex_lock (&ectrl->fs_lock);
			v_id = WDTgetEvent (ectrl->client);
			f_id = WDT_F_getEventReg (ectrl->client);

			if ( WDTsetEvent (ectrl->client, wdt.v_event) < 0 ||
			  WDT_F_setEvent (ectrl->client, wdt.f_event) < 0 )	{
				//restore old value
				WDTsetEvent (ectrl->client, v_id);
				WDT_F_setEvent (ectrl->client, f_id);
				mutex_unlock (&ectrl->fs_lock);
				mutex_unlock (&ectrl->ioctl_lock);
				return -EINVAL;
			}

			WDTsetEnableReg (ectrl->client, wdt.v_enable);
			WDTsetDelayReg (ectrl->client, (u16)wdt.v_delay_sec);
			WDTsetTimeoutReg (ectrl->client, (u16)wdt.v_timeout_sec);

			WDT_F_setEnable (ectrl->client, wdt.f_enable);
			WDT_F_setDelayReg (ectrl->client, (u16)wdt.f_delay_sec);
			WDT_F_setTimeoutReg (ectrl->client, (u16)wdt.f_timeout_sec);
			mutex_unlock (&ectrl->fs_lock);

			mutex_unlock (&ectrl->ioctl_lock);
			break;
		}

		case ECTRL_WDT_REFRESH: {

			mutex_lock (&ectrl->ioctl_lock);
			WDTrefresh (ectrl->client);
			mutex_unlock (&ectrl->ioctl_lock);

			break;
		}

		case ECTRL_WDT_RESTORE: {
			u16 f_delay, f_timeout;

			mutex_lock (&ectrl->ioctl_lock);
			mutex_lock (&ectrl->fs_lock);
			f_delay   = (u16)WDT_F_getDelayReg (ectrl->client);
			f_timeout = (u16)WDT_F_getTimeoutReg (ectrl->client);

			WDTsetDelayReg (ectrl->client, f_delay);
			WDTsetTimeoutReg (ectrl->client, f_timeout);
			mutex_unlock (&ectrl->fs_lock);

			mutex_unlock (&ectrl->ioctl_lock);
			break;
		}

		default:
			break;
	}
	return retval;
}


static int ectrl_open(struct inode *inode, struct file *file) {
	file->private_data = ectrl->client;
	return 0;
}


int ectrl_release(struct inode *inode, struct file *file) {
	file->private_data = NULL;
	return 0;
}


static const struct file_operations ectrl_fileops = {
	.owner = THIS_MODULE,
	.open = ectrl_open,
	.unlocked_ioctl = ectrl_ioctl,
	.release = ectrl_release,
};


static struct miscdevice ectrl_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "econtroller",
	.fops = &ectrl_fileops,
};

/************************************/

static void manage_event (struct econtroller *ectrl, struct event_dev *event) {
	int code = 0;
	switch (event->id) {
		case FAIL_BV:
			code = KEY_F4; //BTN_ECTRL_PWR;
			break;
		case FAIL_WD:
			code = KEY_F5; //BTN_ECTRL_SLEEP;
			break;
		case BATLOW_SIGNAL:
			code = BTN_ECTRL_BATLOW_HL; //BTN_ECTRL_WAKE;
			break;
		case SLEEP_SIGNAL:
			code = BTN_ECTRL_SLEEP; //BTN_ECTRL_BATLOW_HL;
			break;
		case LID_SIGNAL:
			code = BTN_ECTRL_LID_HL; //BTN_ECTRL_BATLOW_LH;
			break;
		case PWR_BUTTON:
			code = BTN_ECTRL_PWR; //BTN_ECTRL_LID_HL;
			break;
		case FAIL_PWGIN:
			code = KEY_F6; //BTN_ECTRL_LID_LH;
			break;
		default:
			code = -1;
			break; 
	}
	
	if ( code > 0 ) {
		input_report_key (event->input, code, 1);
		input_sync (event->input);
		input_report_key (event->input, code, 0);
		input_sync (event->input);

		ECTRL_INFO ("event name: %s, code %d\n", event->name, code);
	} else {
		ECTRL_ERR ("invalid event occures !!!");
	}
}

/*
static int getIdxStatus (struct econtroller *ectrl, int event) {
	int idx;
	for (idx = 0 ; idx < ectrl->nr_evn ; idx++)
		if (ectrl->evn_state[idx]->evn == event)
			return idx;
	return -1;
}
*/
#define TIME_POWER_PRESSED  40 /* ms */
//static void ectr_work_pwr_btn_snooper (struct work_struct *work) {
//	struct econtroller *ectrl = container_of(to_delayed_work(work), struct econtroller, work_snooper_pwr_btn);
//	int status = 0;
//	if (ectrl->client) { 
//		status = getStatus (ectrl->client, 
//				ectrl->evn_state[getIdxStatus(ectrl, EVNT_PWR_BUTTON)]->reg_idx);
//		if (time_after (jiffies, ectrl->orig_jiffies + TIME_POWER_PRESSED)) {
//			return;
//		} else {
//			if (!status) { // button still active, re-schedule the work
//				schedule_delayed_work (&ectrl->work_snooper_pwr_btn, 
//					msecs_to_jiffies(ectrl->poll_period_snooper_pwr_btn));
//			} else { // button is no longer active, we have a valid short pressed
//				 // power button event, so we will notify this to the system
//				manage_event (ectrl, ectrl->events[PWR_BUTTON]);
//				return;
//			}
//		}
//	} else {
//		return;
//	}
//}


#define GET_EVENT_FLAG(reg, event)  ((reg) & (event->index_reg) & EVENT_MASK) >> ((event->shift) & EVENT_MASK)

static void ectrl_work (struct work_struct *work) {	
	int i;
	static unsigned int  status_reg = 0;
	struct econtroller *ectrl = container_of(to_delayed_work(work), struct econtroller, work);

	status_reg = getStatusReg(ectrl->client);
	for (i = 0 ; i < ectrl->nr_evnt ; i++) {
		if (GET_EVENT_FLAG(status_reg, ectrl->events[i]) == 1) {
				manage_event (ectrl, ectrl->events[i]);
		}
	}	
}


static void ectrl_free_irq (struct econtroller *ectrl) {
	free_irq (ectrl->irq, ectrl);
        if (cancel_delayed_work_sync(&ectrl->work)) {
                /*
                 * Work was pending, therefore we need to enable
                 * IRQ here to balance the disable_irq() done in the
                 * interrupt handler.
                 */
                enable_irq(ectrl->irq);
        }
}


static irqreturn_t irq_interrupt_manager (int irq, void *dev_id)  {
	struct econtroller *ectrl = *((struct econtroller **)dev_id);
	
	schedule_delayed_work (&ectrl->work, msecs_to_jiffies(ectrl->poll_period));
	return IRQ_HANDLED;
}


static void ectrl_detect_state (struct econtroller *ectrl, int *enable) {
	*enable = getStatusReg (ectrl->client);
}


//static void ectrl_resolve_state (struct econtroller *ectrl, int enable, int flags, int status) {
//	int i;
//	int nr_events = ectrl->nr_evnt;
//	for (i = 0 ; i < nr_events ; i++) {
//		if (GET_EVENT_FLAG(flags, ectrl->events[i]) == 1) {
//			manage_event (ectrl, ectrl->events[i]);
//			flags &= ~(1 << global_event_list[ectrl->events[i]->id].shift);
//		}
//	}
//	// we have to clean the served event's flag 
//	setFlagReg (ectrl->client, 0x0000);
//	udelay (10);
//	
//	// At this point, the original state of the enable register can be restored
//	setEnableReg (ectrl->client, enable);
//}


static int ectrl_notify_sys (struct notifier_block *this, unsigned long code, void *unused) {
	int retval;
	switch ( code ) {
		case SYS_DOWN:
			retval = ectrl_SystemReboot (ectrl);
			break;
		case SYS_HALT:
		case SYS_POWER_OFF:
			retval = ectrl_SystemHalt (ectrl);
			break;
		default:
			break;
	}
	return NOTIFY_DONE;
}


static struct notifier_block ectrl_notifier = {
	.notifier_call = ectrl_notify_sys,
};

static int ectrl_get_board_id(struct econtroller *ectrl) {

	struct device dev = ectrl->client->dev;	
	const char *board_name;
	struct device_node *np = of_node_get (dev.of_node);

        if ( ! np )
                return -EINVAL;
	if(of_property_read_string (np, "board-id", &board_name)) {
                pr_err("%s: could not find property board-id\n",
                                of_node_full_name(np));
                return -EINVAL;
	}

	if(strcmp (board_name, "C12") == 0) {
		ectrl->straps_def = straps_definition_imx8m; 
		ectrl->straps_def_size = sizeof(straps_definition_imx8m)/sizeof(struct ectrl_straps_define);
		return BOARD_C12_ID;
	}
	if(strcmp (board_name, "C25") == 0) {
		ectrl->straps_def = straps_definition_imx8m; 
		ectrl->straps_def_size = sizeof(straps_definition_imx8m)/sizeof(struct ectrl_straps_define);
		return BOARD_C25_ID;
	}
	if(strcmp (board_name, "C26") == 0) {
		ectrl->straps_def = straps_definition_imx8qm; 
		ectrl->straps_def_size = sizeof(straps_definition_imx8qm)/sizeof(struct ectrl_straps_define);
	}	return BOARD_C26_ID;

	return -EINVAL;
}

static int ectrl_parse_dt (struct econtroller *ectrl, int board_id) {

	struct device dev = ectrl->client->dev;
	struct device_node *np;

	struct property *prop;
	int length, num_event;
	u32 *of_event_list = NULL, *event_list = NULL;
	struct event_dev **events = NULL;

	int idx, nre = 0, i, idxs;
	int ret, err;

	int num_bootdev;
	struct device_node *parent, *child;


	np = of_node_get (dev.of_node);
	if ( ! np )
		return -EINVAL;

	/* -----------------------------------
	 * retrive the list of feasible events
	 * ----------------------------------- */
	prop = of_find_property (np, "events", &length);
	if ( ! prop ) {
		pr_err("%s: could not find property %s\n",
				of_node_full_name(np), "events");
	} else {

		num_event = length / sizeof(u32);

		ECTRL_DBG ("Num. of event found: %d", num_event);

		/*  list of event, as u32, retrived directly from device tree  */
		of_event_list = kzalloc (sizeof (u32) * num_event , GFP_KERNEL);
		if ( ! of_event_list ) {
			err = -EINVAL;
			goto err_alloc_of_event_list;
		}

		/*  list of event used by the driver  */
		event_list = kzalloc (sizeof (u32) * num_event, GFP_KERNEL);
		if ( ! event_list ) {
			err = -EINVAL;
			goto err_alloc_event_list;
		}

		ret = of_property_read_u32_array (np, "events", of_event_list, num_event);

		nre = 0;
		for ( i = 0 ; i < num_event ; i++ )  {

			/*  switch from DTB data to event index  */
			switch ( of_event_list[i] ) {
				case ECTRL_EVNT_PWR_BUTTON:
					event_list[i] = EVNT_PWR_BUTTON;
					break;
				case ECTRL_EVNT_FAIL_BV:
					event_list[i] = EVNT_FAIL_BV;
					break;
				case ECTRL_EVNT_FAIL_WD:
					event_list[i] = EVNT_FAIL_WD;
					break;
				case ECTRL_EVNT_BATLOW_SIGNAL:
					event_list[i] = EVNT_BATLOW_SIGNAL;
					break;
				case ECTRL_EVNT_LID_SIGNAL:
					event_list[i] = EVNT_LID_SIGNAL;
					break;
				case ECTRL_EVNT_SLEEP_SIGNAL:
					event_list[i] = EVNT_SLEEP_SIGNAL;
					break;
				case ECTRL_EVNT_FAIL_PWGIN:
					event_list[i] = EVNT_FAIL_PWGIN;
					break;
				case ECTRL_EVNT_WAKE_EN:
					event_list[i] = EVNT_WAKE_EN;
					break;
				default:
					/*  unrecognized event code  */
					return -EINVAL;
			}
			
			/*  count the effective event states used by the driver.
			 *  Some event source requires two event (one for each edge
			 *  of the signal.
			 *  */
			if ( of_event_list[i] & (ECTRL_EVNT_PWR_BUTTON | ECTRL_EVNT_FAIL_BV | ECTRL_EVNT_FAIL_WD |
						ECTRL_EVNT_BATLOW_SIGNAL| ECTRL_EVNT_LID_SIGNAL|
						ECTRL_EVNT_SLEEP_SIGNAL | ECTRL_EVNT_FAIL_PWGIN | ECTRL_EVNT_WAKE_EN) )
				nre++;

		}

		ectrl->nr_evn = num_event; //board_nr_states[board_id]; 
		ectrl->evn_state = (struct event_state **)kzalloc (sizeof (struct event_state *) * ectrl->nr_evn, GFP_KERNEL);
		if (!ectrl->evn_state) {
			err = -ENOMEM;
			goto err_alloc_evn_state;
		}
	      
		events = (struct event_dev **)kzalloc (sizeof (struct event_dev *) * nre, GFP_KERNEL);
		if (!events) {
			err = -ENOMEM;
			goto err_alloc_events;
		}

		for ( idx = 0, idxs = 0, i = 0 ; i < num_event ; i++ ) {

			//  check if the event is available for the board
			if ( board_reg_state_idx[board_id][event_list[i]] != -1 ) {
				ectrl->evn_state[idxs] = &event_state_list[event_list[i]];
				ectrl->evn_state[idxs]->reg_idx = board_reg_state_idx[board_id][event_list[i]];
				idxs++;
			}

			switch ( event_list[i] ) {
				case EVNT_PWR_BUTTON:
					events[idx] = &global_event_list[PWR_BUTTON];
					idx++;
					break;
				case EVNT_FAIL_BV:
					events[idx] = &global_event_list[FAIL_BV];
					idx++;
					break;
				case EVNT_SLEEP_SIGNAL:
					events[idx] = &global_event_list[SLEEP_SIGNAL];
					idx++;
					break;
				case EVNT_BATLOW_SIGNAL:
					events[idx] = &global_event_list[BATLOW_SIGNAL];
					idx++;
					break;
				case EVNT_LID_SIGNAL:
					events[idx] = &global_event_list[LID_SIGNAL];
					idx++;;
					break;
				case EVNT_FAIL_WD:
					events[idx] = &global_event_list[FAIL_WD];
					idx++;;
					break;
				case EVNT_FAIL_PWGIN:
					events[idx] = &global_event_list[FAIL_PWGIN];
					idx ++;
					break;
				case EVNT_WAKE_EN:
					events[idx] = &global_event_list[WAKE_EN];
					idx ++;
					break;
				default:
					break;
			}	
		}

		ectrl->events = events; 
		ectrl->nr_evnt = nre;
	}
	
	parent = of_get_child_by_name(np, "boot_device");
	if ( ! parent ) {
		ECTRL_ERR ("boot_device node not found");
		err = -EINVAL;
		goto err_no_boot_device_node;
	}
		
	num_bootdev = 0;
	for_each_child_of_node (parent, child) 
		if ( strcmp (child->name, "bootdev") == 0 )
				num_bootdev++;
	
	if ( !num_bootdev ) {
		/*  No boot device found  */
		ECTRL_ERR ("No boot device found!");	
		err = -EINVAL;
		goto err_no_boot_device;
	}

	ECTRL_DBG ("Num of bootdev: %d", num_bootdev);
	ectrl->nr_bootdev = num_bootdev;
	ectrl->bootdev_list = kzalloc (sizeof (struct bootdev *) * ectrl->nr_bootdev, GFP_KERNEL);
	if ( !ectrl->bootdev_list ) {
		err = -ENOMEM;
		goto err_alloc_bootdev_list;
	}

	i = 0;
	for_each_child_of_node (parent, child) {
		/*  check if the node is a boot device node info  */
		if ( strcmp (child->name, "bootdev") == 0 ) {
			of_property_read_u32 (child, "id", (u32 *)&ectrl->bootdev_list[i].id);
			of_property_read_string (child, "label", &ectrl->bootdev_list[i].label);
			i++;
		}
	
	}
	
	ectrl->rb_poff_gpio = of_get_named_gpio(np, "rb-poff-gpio", 0);
	if (!gpio_is_valid(ectrl->rb_poff_gpio)) {
                ECTRL_ERR("reboot poweroff pin unavailable");
        }

	
	ectrl->irq = irq_of_parse_and_map (np, 0);

	ectrl->input_interface = of_property_read_bool(np, "ectrl,input");

	return 0;

err_alloc_bootdev_list:
err_no_boot_device:
err_no_boot_device_node:
	for ( i = 0 ; i < nre ; i++ )
		kfree (events[i]);
	kfree (events);
err_alloc_events:
	for ( i = 0 ; i < ectrl->nr_evn ; i++ )
		kfree (ectrl->evn_state[i]);
	kfree (ectrl->evn_state);
err_alloc_evn_state:
	kfree (event_list);
err_alloc_event_list:
	kfree (of_event_list);
err_alloc_of_event_list:
	kfree (prop);

	return err;
}


static const struct i2c_device_id ectrl_idtable[] = {
	{ "ectrl", 0 },
	{ /*  sentinel  */ }
};
MODULE_DEVICE_TABLE(i2c, ectrl_idtable);


static const struct of_device_id seco_ectrl_i2c_match[] = {
		{ .compatible = "seco,ectrl-stm32" },
		{ /* sentinel */ }
	};
	MODULE_DEVICE_TABLE(of, seco_ectrl_of_match);


static int ectrl_probe(struct i2c_client *client, const struct i2c_device_id *id) {

	int err, ret = -1, reg = 0, i;
	int enable_reg;
	int bidx = -1;
	struct input_dev *input = NULL;

	const struct of_device_id *match = NULL;

	if ( !i2c_check_functionality (client->adapter, 
				I2C_FUNC_SMBUS_READ_WORD_DATA | I2C_FUNC_I2C) ) {
		err = -EIO;
		goto err_i2c_check;
	}

	ectrl = kzalloc (sizeof (struct econtroller), GFP_KERNEL);

	if ( !ectrl ) {
		err = -ENOMEM;
		goto err_alloc_ectrl;
	}

	if ( client->dev.of_node ) {
		match = of_match_device (of_match_ptr (seco_ectrl_i2c_match),
				&client->dev);
		if ( !match ) {
			ECTRL_ERR ("No device match found");
			err = -ENODEV;
			goto err_no_match;
		}
	}

	ectrl->client = client;

	ectrl->board_id = ectrl_get_board_id(ectrl);
	
	if ( ectrl->board_id < 0 ) {
                ECTRL_ERR ("error no board name specified: %d", ectrl->board_id);
                err = ectrl->board_id;
                goto err_parsing_dt;
        }

	/*  retrive the current board id  */
	/*  i.MX8 firmware doesn't store board type yet
	 *  board_idx is acquired from dts currently
	 */

	switch (ectrl->board_id) {
		case BOARD_C12_ID:
			bidx = BOARD_C12;
			break; 
		case BOARD_C25_ID:
			bidx = BOARD_C25;
			break;
		case BOARD_C26_ID:
			bidx = BOARD_C26;
			break;
		default:
			bidx = -1;
			break;
	}

	ret = ectrl_parse_dt (ectrl, bidx);

	if ( ret < 0 ) {
		ECTRL_ERR ("error in parsing dt: %d", ret);
		err = ret;
		goto err_parsing_dt;
	}

	if (bidx == -1) {
		//  unsupported board
		ECTRL_ERR ("Unsupported board: %d", ectrl->board_id);	
		err = -EINVAL;
		goto err_inv_id;
	}

	ECTRL_INFO ("Detected board: %s firmware rev: 0x%x", board_name[bidx],getFirmwareRev(ectrl->client));	

	ectrl->wdt_event_list = wdt_evnt_list;
	ectrl->nr_wdt_event = ARRAY_SIZE (wdt_evnt_list);

	/* TODO - this interface is not programmed for i.MX8 yet */
	if(ectrl->input_interface) {
		INIT_DELAYED_WORK (&ectrl->work, ectrl_work);
		ectrl->poll_period  = ECTRL_POLL_PERIOD;
				
		/* input interface */
	
		input = input_allocate_device();
		if ( !input) {
			err = -ENOMEM;
			goto err_input;
		}
	
		ectrl->phys = kzalloc (sizeof (char) * 32, GFP_KERNEL);
		snprintf(ectrl->phys, sizeof(char) * 32,
				"%s/input%d", dev_name(&client->dev), 0);
	
		input->name        = "seco_ectrl";
		input->phys        = ectrl->phys;
		input->id.bustype  = BUS_I2C;
		input->dev.parent  = &client->dev;
	
		for ( i = 0 ; i < ectrl->nr_evnt ; i++ ) {
	
			ectrl->events[i]->input = input;
	
			switch ( ectrl->events[i]->id ) {
				case PWR_BUTTON:
					input_set_capability (input, EV_KEY, KEY_F4);
					break;
				case SLEEP_SIGNAL:
					input_set_capability (input, EV_KEY, BTN_ECTRL_SLEEP);
					break;
				case FAIL_BV:
					input_set_capability (input, EV_KEY, KEY_F5);
				case FAIL_WD:
					input_set_capability (input, EV_KEY, KEY_F6);
					break;
				case BATLOW_SIGNAL:
					input_set_capability (input, EV_KEY, BTN_ECTRL_BATLOW_HL);
					break;
				case LID_SIGNAL:
					input_set_capability (input, EV_KEY, BTN_ECTRL_LID_HL);
					break;
				case FAIL_PWGIN:
					input_set_capability (input, EV_KEY, KEY_F7);
					break;
				default:
					break;
			}
	
		}
	
		err = input_register_device (input);
		if ( err ) {
			ECTRL_ERR ("No input assigned");
			err = -EINVAL;
			reg = i;
			goto err_reg_input;
		}
	
	
		/* detect and resolve the initial state */
		ectrl_detect_state (ectrl, &enable_reg);
	

		/* irq acquisition */
		if ( ectrl->irq == 0 ) {
			ECTRL_ERR ("No IRQ assigned");
			err = -EINVAL;
			goto err_irq;
		}
		ret = request_irq (ectrl->irq, irq_interrupt_manager,
					IRQF_SHARED | IRQF_TRIGGER_FALLING, "ectrl", &ectrl);
	
		if ( ret ) {
			ECTRL_ERR ("IRQ not acquired: error %d", ret);
			err = -EIO;
			goto err_free_irq;
		}
	
		ret = misc_register (&ectrl_device);
		if ( ret ) {
			ECTRL_ERR ("misc registration failed: %d", ret);
			err = -EIO;
			goto err_misc_register;
		}
	
	}

	ret = register_reboot_notifier (&ectrl_notifier);
	if ( ret != 0 ) {
		pr_err("cannot register reboot notifier (err=%d)\n", ret);
		goto err_notifier_register;
	}

	ectrl_add_proc_fs (ectrl);

	/* mutex initialization */
	mutex_init (&ectrl->fs_lock);
	mutex_init (&ectrl->ioctl_lock);

	ECTRL_INFO (" probe done");
	printk (KERN_INFO "ec: 0x%02x\n", ectrl_read_data (ectrl->client, BUILDREV_REG) >> 8);

	dev_set_drvdata (&client->dev, ectrl);

	return 0;


err_notifier_register:
err_misc_register:
	ectrl_free_irq (ectrl);
err_free_irq:
err_irq:
	if(ectrl->input_interface)
		input_unregister_device (input);

err_reg_input:
	if(ectrl->input_interface)
		kfree (input);
err_input:
	for ( i = 0 ; i < ectrl->nr_evnt ; i++ )
		kfree (ectrl->events[i]);
	kfree (ectrl->events);
	for ( i = 0 ; i < ectrl->nr_evn ; i++ )
		kfree (ectrl->evn_state[i]);
	kfree (ectrl->evn_state);
err_parsing_dt:
	kfree (match);
err_inv_id:
err_no_match:
	kfree (ectrl);
err_alloc_ectrl:
err_i2c_check:

	return err;
}


static int ectrl_remove(struct i2c_client *client) {
	int i;
	struct ectrl_platform_data *data;
	data = i2c_get_clientdata(client);
	misc_deregister (&ectrl_device);
	ectrl_free_irq (ectrl);
	ectrl_remove_proc_fs (ectrl);
	unregister_reboot_notifier (&ectrl_notifier);
	kfree (entry_list);

	for ( i = 0 ; i < ectrl->nr_evnt ; i++ )
		kfree (ectrl->events[i]);
	kfree (ectrl->events);

	for ( i = 0 ; i < ectrl->nr_evn ; i++ )
		kfree (ectrl->evn_state[i]);
	kfree (ectrl->evn_state);

	kfree (ectrl);
	kfree (data);
	return 0;
}


void device_shutdown_ectrl(struct i2c_client *client) {
}


static struct i2c_driver ectrl_driver = {
	.driver = {
		.owner	         = THIS_MODULE,
		.name	         = "ectrl",
		.of_match_table  = of_match_ptr(seco_ectrl_i2c_match),
	},
	.id_table	= ectrl_idtable,
	.probe		= ectrl_probe,
	.remove		= ectrl_remove,
	.shutdown   = device_shutdown_ectrl,
};


//module_i2c_driver(ectrl_driver);

static int __init ectrl_init(void) {
	if(strcmp(ectrl_setup,"prog") != 0)
		return i2c_add_driver(&ectrl_driver);
	return 0;
}


static void __exit ectrl_exit(void) {
	i2c_del_driver(&ectrl_driver);
}


module_init(ectrl_init);
module_exit(ectrl_exit);
__setup("ectrl=", ectrl_config_setup);

MODULE_AUTHOR("Davide Cardillo, SECO srl");
MODULE_DESCRIPTION("SECO system halt with Embedded Controller");
MODULE_LICENSE("GPL");
MODULE_VERSION(DRV_VERSION);


#include <linux/ioctl.h>


#define APX_WDOG_STATUS_EN    1
#define APX_WDOG_STATUS_DIS   0

#define APX_WDOG_TRIG_MOD_AUTO   0
#define APX_WDOG_TRIG_MOD_MAN    1



/*  generic command  */

#define APX_WDT_GET_STATUS             _IOW('q', 0, int *)
#define APX_WDT_SET_STATUS             _IOR('q', 1, int *)

#define APX_WDT_GET_TRIGGER_MODE       _IOW('q', 2, int *)
#define APX_WDT_SET_TRIGGER_MODE       _IOR('q', 3, int *)

#define APX_WDT_GET_REFRESH_TIME       _IOW('q', 4, unsigned long *)
#define APX_WDT_SET_REFRESH_TIME       _IOR('q', 5, unsigned long *)

#define APX_WDT_GET_RESIDUAL_TIME      _IOW('q', 6, unsigned long *)

#define APX_WDT_REFRESH                _IOW('q', 7, int *) 

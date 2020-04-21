#ifndef __LINUX_SECO_CPLD_H
#define __LINUX_SEC0_CPLD_H

#include <linux/of.h>
#include <linux/device.h>

#define NREG                     11
#define CPLD_OFFSET_BASE 	 0xF000

#define CPLD_REG_0               (uint8_t)0x0
#define CPLD_REG_1               (uint8_t)0x1
#define CPLD_REG_2               (uint8_t)0x2
#define CPLD_REG_3               (uint8_t)0x3
#define CPLD_REG_4               (uint8_t)0x4
#define CPLD_REG_5               (uint8_t)0x5
#define CPLD_REG_6               (uint8_t)0x6
#define CPLD_REG_7               (uint8_t)0x7
#define CPLD_REG_8               (uint8_t)0x8
#define CPLD_REG_9               (uint8_t)0x9
#define CPLD_REG_10              (uint8_t)0xa

#define REG_REVISION             CPLD_REG_0


struct cpld_data {
	const char           *name;
	resource_size_t      mem_addr_base;
	unsigned long long   mem_size;
	void __iomem         *virt;
	struct mutex         bus_lock;
};

#define CPLD_NAME_SIZE   50

struct cpld_client {
	char name[CPLD_NAME_SIZE];
	struct device dev;
};

extern void cpld_reg_read (unsigned int addr, uint16_t *data);

extern void cpld_reg_write (unsigned int addr, uint16_t value);

extern void cpld_read (unsigned int addr, uint16_t *data);

extern void cpld_write (unsigned int addr, uint16_t value);

extern int cpld_get_membase (void);

extern int __cpld_init (struct device_node *dp, struct resource *resource);

extern int cpld_get_revision (void);

extern int cpld_is_gpio (void);

extern int cpld_is_lpc (void);

//extern void cpld_remove (void);

extern void dump_reg (void);
#endif  /* __LINUX_SECO_CPLD_H */

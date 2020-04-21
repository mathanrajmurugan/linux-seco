#ifndef __LINUX_SECO_LPC_BRIDGE_H
#define __LINUX_SEC0_LPC_BRIDGE_H

#include <linux/of.h>
#include <linux/device.h>

struct lpc_device {
	int             lpc_slot;
	struct device   *dev;
	irqreturn_t     (**handler)(int irq, void *dev_id);
	void            (*after_handler)(int irq, void *dev_id);
	void            **irq_dev_id;
	int             maskable;
};


extern int lpc_readw (unsigned long mem_addr, uint16_t *data);

extern int lpc_writew (unsigned long mem_addr, uint16_t value);

extern unsigned long lpc_getIRQ_flags (int irq);

extern int lpc_getIRQ (int irq);

extern unsigned long lpc_getMemBase (void);

extern int lpc_add_device (struct lpc_device *lpc_dev);

extern void irq_slot_set (int slot, int en);


#endif  /* __LINUX_SECO_LPC_BRIDGE_H */

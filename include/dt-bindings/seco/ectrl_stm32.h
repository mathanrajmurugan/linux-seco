/*
 * (C) Copyright 2015 Seco srl
 *
 * Author: Davide Cardillo <davide.cardillo@seco.com>
 *
 * SPDX-License-Identifier:     GPL-2.0+
 *
 * This header provides constants for the binding Seco Embedded controller
 *
 */

#ifndef __DT_BINDINGS_SECO_ECTRL_H
#define __DT_BINDINGS_SECO_ECTRL_H



#define ECTRL_EVNT_FAIL_BV		0x01
#define ECTRL_EVNT_FAIL_WD		0x02
#define ECTRL_EVNT_BATLOW_SIGNAL        0x04
#define ECTRL_EVNT_SLEEP_SIGNAL		0x08
#define ECTRL_EVNT_LID_SIGNAL		0x10
#define ECTRL_EVNT_PWR_BUTTON		0x20
#define ECTRL_EVNT_FAIL_PWGIN		0x40
#define ECTRL_EVNT_WAKE_EN		0x80


#define ECTRL_BOOTDEV_USDHC4    0x00
#define ECTRL_BOOTDEV_USDHC1    0x01
#define ECTRL_BOOTDEV_EMMC      0x02
#define ECTRL_BOOTDEV_SPI       0x03


#endif   /*  __DT_BINDINGS_SECO_ECTRL_H  */


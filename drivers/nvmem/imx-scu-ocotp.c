/*
 * i.MX6 OCOTP fusebox driver
 *
 * Copyright (c) 2015 Pengutronix, Philipp Zabel <p.zabel@pengutronix.de>
 *
 * Based on the barebox ocotp driver,
 * Copyright (c) 2010 Baruch Siach <baruch@tkos.co.il>,
 *	Orex Computed Radiography
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <linux/arm-smccc.h>
#include <linux/clk.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/nvmem-provider.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <soc/imx/fsl_sip.h>
#include <soc/imx8/sc/sci.h>

static DEFINE_MUTEX(scu_ocotp_mutex);

enum ocotp_devtype {
	IMX8QM,
	IMX8QXP,
};

#define ECC_REGION	BIT(0)
#define HOLE_REGION	BIT(1)

struct ocotp_region {
	u32 start;
	u32 end;
	u32 flag;
};

struct ocotp_devtype_data {
	int devtype;
	int nregs;
	u32 num_region;
	struct ocotp_region region[];
};

struct ocotp_priv {
	struct device *dev;
	struct ocotp_devtype_data *data;
	sc_ipc_t nvmem_ipc;
};

static struct ocotp_devtype_data imx8qm_data = {
	.devtype = IMX8QM,
	.nregs = 800,
	.num_region = 2,
	.region = {
		{0x10, 0x10f, ECC_REGION},
		{0x1a0, 0x1ff, ECC_REGION},
	},
};

static struct ocotp_devtype_data imx8qxp_data = {
	.devtype = IMX8QXP,
	.nregs = 800,
	.num_region = 3,
	.region = {
		{0x10, 0x10f, ECC_REGION},
		{0x110, 0x21F, HOLE_REGION},
		{0x220, 0x31F, ECC_REGION},
	},
};

static bool in_hole(void *context, u32 index)
{
	struct ocotp_priv *priv = context;
	struct ocotp_devtype_data *data = priv->data;
	int i;

	for (i = 0; i < data->num_region; i++) {
		if (data->region[i].flag & HOLE_REGION) {
			if ((index >= data->region[i].start) &&
			    (index <= data->region[i].end))
				return true;
		}
	}

	return false;
}

static bool in_ecc(void *context, u32 index)
{
	struct ocotp_priv *priv = context;
	struct ocotp_devtype_data *data = priv->data;
	int i;

	for (i = 0; i < data->num_region; i++) {
		if (data->region[i].flag & ECC_REGION) {
			if ((index >= data->region[i].start) &&
			    (index <= data->region[i].end))
				return true;
		}
	}

	return false;
}

static int imx_scu_ocotp_read(void *context, unsigned int offset,
			      void *val, size_t bytes)
{
	struct ocotp_priv *priv = context;
	sc_err_t sciErr = SC_ERR_NONE;
	unsigned int count;
	u32 index;
	u32 num_bytes;
	int i;
	u8 *buf, *p;

	index = offset >> 2;
	num_bytes = round_up((offset % 4) + bytes, 4);
	count = num_bytes >> 2;

	if (count > (priv->data->nregs - index))
		count = priv->data->nregs - index;

	p = kzalloc(num_bytes, GFP_KERNEL);
	if (!p)
		return -ENOMEM;

	mutex_lock(&scu_ocotp_mutex);

	buf = p;

	for (i = index; i < (index + count); i++) {
		if (in_hole(context, i)) {
			*(u32 *)buf = 0;
			buf += 4;
			continue;
		}

		sciErr = sc_misc_otp_fuse_read(priv->nvmem_ipc, i, (u32 *)buf);
		if (sciErr != SC_ERR_NONE) {
			mutex_unlock(&scu_ocotp_mutex);
			kfree(p);
			return -EIO;
		}
		buf += 4;
	}

	index = offset % 4;
	memcpy(val, &p[index], bytes);

	mutex_unlock(&scu_ocotp_mutex);

	kfree(p);

	return 0;
}

static int imx_scu_ocotp_write(void *context, unsigned int offset,
			       void *val, size_t bytes)
{
	struct ocotp_priv *priv = context;
	sc_err_t sciErr = SC_ERR_NONE;
	struct arm_smccc_res res;
	u32 *buf = val;
	u32 tmp;
	u32 index;

	/* allow only writing one complete OTP word at a time */
	if ((bytes != 4) || (offset % 4))
		return -EINVAL;

	index = offset >> 2;

	if (in_hole(context, index))
		return -EINVAL;

	if (in_ecc(context, index)) {
		pr_warn("ECC region, only program once\n");
		mutex_lock(&scu_ocotp_mutex);
		sciErr = sc_misc_otp_fuse_read(priv->nvmem_ipc, index, &tmp);
		mutex_unlock(&scu_ocotp_mutex);
		if (sciErr != SC_ERR_NONE)
			return -EIO;
		if (tmp) {
			pr_warn("ECC region, already has value: %x\n", tmp);
			return -EIO;
		}
	}

	mutex_lock(&scu_ocotp_mutex);

	arm_smccc_smc(FSL_SIP_OTP_WRITE, index, *buf, 0, 0, 0, 0, 0, &res);

	mutex_unlock(&scu_ocotp_mutex);

	return res.a0;
}

static struct nvmem_config imx_scu_ocotp_nvmem_config = {
	.name = "imx-ocotp",
	.read_only = false,
	.word_size = 4,
	.stride = 1,
	.owner = THIS_MODULE,
	.reg_read = imx_scu_ocotp_read,
	.reg_write = imx_scu_ocotp_write,
};

static const struct of_device_id imx_scu_ocotp_dt_ids[] = {
	{ .compatible = "fsl,imx8qm-ocotp", (void *)&imx8qm_data },
	{ .compatible = "fsl,imx8qxp-ocotp", (void *)&imx8qxp_data },
	{ },
};
MODULE_DEVICE_TABLE(of, imx_scu_ocotp_dt_ids);

static int imx_scu_ocotp_probe(struct platform_device *pdev)
{
	const struct of_device_id *of_id;
	struct device *dev = &pdev->dev;
	struct ocotp_priv *priv;
	struct nvmem_device *nvmem;
	uint32_t mu_id;
	sc_err_t sciErr = SC_ERR_NONE;

	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	sciErr = sc_ipc_getMuID(&mu_id);
	if (sciErr != SC_ERR_NONE) {
		pr_info("pinctrl: Cannot obtain MU ID\n");
		return -EIO;
	}

	sciErr = sc_ipc_open(&priv->nvmem_ipc, mu_id);

	if (sciErr != SC_ERR_NONE) {
		pr_info("pinctrl: Cannot open MU channel to SCU\n");
		return -EIO;
	};

	of_id = of_match_device(imx_scu_ocotp_dt_ids, dev);
	priv->data = (struct ocotp_devtype_data *)of_id->data;
	priv->dev = dev;
	imx_scu_ocotp_nvmem_config.size = 4 * priv->data->nregs;
	imx_scu_ocotp_nvmem_config.dev = dev;
	imx_scu_ocotp_nvmem_config.priv = priv;
	nvmem = nvmem_register(&imx_scu_ocotp_nvmem_config);
	if (IS_ERR(nvmem))
		return PTR_ERR(nvmem);

	platform_set_drvdata(pdev, nvmem);

	return 0;
}

static int imx_scu_ocotp_remove(struct platform_device *pdev)
{
	struct nvmem_device *nvmem = platform_get_drvdata(pdev);

	return nvmem_unregister(nvmem);
}

static struct platform_driver imx_scu_ocotp_driver = {
	.probe	= imx_scu_ocotp_probe,
	.remove	= imx_scu_ocotp_remove,
	.driver = {
		.name	= "imx_scu_ocotp",
		.of_match_table = imx_scu_ocotp_dt_ids,
	},
};
module_platform_driver(imx_scu_ocotp_driver);

MODULE_AUTHOR("Peng Fan <peng.fan@nxp.com>");
MODULE_DESCRIPTION("i.MX8QM OCOTP fuse box driver");
MODULE_LICENSE("GPL v2");

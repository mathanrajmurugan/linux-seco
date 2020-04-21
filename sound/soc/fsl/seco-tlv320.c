/*
 * seco-tlv320.c  --  SoC audio for seco_cpuimxXX in I2S mode
 *
 * Copyright 2017 Michele Cirinei, Seco srl <michele.cirinei@seco.com>
 *
 * based on sound/soc/fsl/eukrea-tlv320.c which is
 * Copyright 2010 Eric Bénard, Eukréa Electromatique <eric@eukrea.com>
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 */

#include <linux/errno.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/device.h>
#include <linux/i2c.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/soc.h>
#include <sound/pcm_params.h>
#include <linux/delay.h>

#include "../codecs/tlv320aic32x4.h"
#include "fsl_sai.h"

#define CODEC_CLOCK 24576000

static int seco_tlv320_hw_params(struct snd_pcm_substream *substream,
			    struct snd_pcm_hw_params *params)
{
//	struct snd_soc_pcm_runtime *rtd = substream->private_data;
//	struct snd_soc_dai *codec_dai = rtd->codec_dai;
//	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
//	int ret;
//
//	ret = snd_soc_dai_set_sysclk(codec_dai, 0,
//				     CODEC_CLOCK, SND_SOC_CLOCK_OUT);
//	if (ret) {
//		pr_err("%s: failed setting codec sysclk\n", __func__);
//		return ret;
//	}
//
//	ret = snd_soc_dai_set_sysclk(cpu_dai, 0, 0,
//				SND_SOC_CLOCK_IN);
//	if (ret) {
//		pr_err("can't set CPU system clock 0\n");
//		return ret;
//	}
//
//	return 0;
	
	        struct snd_soc_pcm_runtime *rtd = substream->private_data;
        struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
        struct snd_soc_card *card = rtd->card;
        struct device *dev = card->dev;
        unsigned int fmt;
        int ret = 0;

        fmt = SND_SOC_DAIFMT_I2S |
                        SND_SOC_DAIFMT_NB_NF |
                        SND_SOC_DAIFMT_CBS_CFS;

        ret = snd_soc_dai_set_fmt(cpu_dai, fmt);
        if (ret) {
                dev_err(dev, "failed to set cpu dai fmt: %d\n", ret);
                return ret;
        }

        ret = snd_soc_dai_set_tdm_slot(cpu_dai, 0, 0, 2,
                                        params_physical_width(params));
        if (ret) {
                dev_err(dev, "failed to set cpu dai tdm slot: %d\n", ret);
                return ret;
        }

        return ret;
	
}

static int seco_tlv320_late_probe(struct snd_soc_card *card)
{
        struct snd_soc_pcm_runtime *rtd = list_first_entry(
                &card->rtd_list, struct snd_soc_pcm_runtime, list);
        struct snd_soc_dai *codec_dai = rtd->codec_dai;
        int ret;

        ret = snd_soc_dai_set_sysclk(codec_dai, 0, CODEC_CLOCK,
                                                        SND_SOC_CLOCK_IN);

        return 0;
}

static struct snd_soc_ops seco_tlv320_snd_ops = {
	.hw_params	= seco_tlv320_hw_params,
};

static struct snd_soc_dai_link seco_tlv320_dai = {
	.name		= "tlv320aic32x4",
	.stream_name	= "TLV320AIC32X4",
	.codec_dai_name	= "tlv320aic32x4-hifi",
	.dai_fmt	= SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF |
			  SND_SOC_DAIFMT_CBS_CFS,
	.ops		= &seco_tlv320_snd_ops,
};

static struct snd_soc_card seco_tlv320 = {
	.owner		= THIS_MODULE,
	.dai_link	= &seco_tlv320_dai,
	.num_links	= 1,
	.late_probe 	= seco_tlv320_late_probe,
};

static int seco_tlv320_probe(struct platform_device *pdev)
{
	int ret;
	struct device_node *cpu_np = NULL, *codec_np = NULL; //, *asrc_np = NULL;
    struct platform_device *cpu_pdev;
    struct i2c_client *codec_dev;
    bool pmicbias_en = false;
    
	seco_tlv320.dev = &pdev->dev;
    cpu_np = of_parse_phandle(pdev->dev.of_node, "cpu-dai", 0);
    if (!cpu_np) {
        dev_err(&pdev->dev, "cpu dai phandle missing or invalid\n");
        ret = -EINVAL;
        goto err;
    }
    codec_np = of_parse_phandle(pdev->dev.of_node, "audio-codec", 0);
    if (!codec_np) {
        dev_err(&pdev->dev, "phandle missing or invalid\n");
        ret = -EINVAL;
        goto err;
    }
    cpu_pdev = of_find_device_by_node(cpu_np);
    if (!cpu_pdev) {
        dev_err(&pdev->dev, "failed to find SAI platform device\n");
        ret = -EINVAL;
        goto err;
    }
    codec_dev = of_find_i2c_device_by_node(codec_np);
    if (!codec_dev || !codec_dev->dev.driver) {
        dev_err(&pdev->dev, "failed to find codec platform device\n");
        ret = -EINVAL;
        goto err;
    }

    pmicbias_en = of_property_read_bool(pdev->dev.of_node, "enable-mic-bias");

/*    
   	asrc_np = of_parse_phandle(pdev->dev.of_node, "asrc-controller", 0);
	if (asrc_np) {
		asrc_pdev = of_find_device_by_node(asrc_np);
		priv->asrc_pdev = asrc_pdev;
	}
*/
        
    seco_tlv320_dai.cpu_of_node = cpu_np;
	seco_tlv320_dai.platform_of_node = cpu_np;
    seco_tlv320_dai.codec_of_node	= codec_np;
    
    ret = snd_soc_of_parse_card_name(&seco_tlv320,"model");
    
	ret = snd_soc_register_card(&seco_tlv320);
    if(pmicbias_en){
        i2c_smbus_write_byte_data(codec_dev, 0x00, 0x01);
        mdelay(20);
        i2c_smbus_write_byte_data(codec_dev, 0x33, 0x78);
        printk(KERN_ERR "tlv320aic32x4: Mic-bias-enabled\n");
    }
err:
	if (ret)
		dev_err(&pdev->dev, "snd_soc_register_card failed (%d)\n", ret);
	
    if (cpu_np)
		of_node_put(cpu_np);
	if (codec_np)
		of_node_put(codec_np);

	return ret;
}

static int seco_tlv320_remove(struct platform_device *pdev)
{
	snd_soc_unregister_card(&seco_tlv320);

	return 0;
}

static const struct of_device_id imx_tlv320_dt_ids[] = {
	{ .compatible = "seco,asoc-tlv320"},
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, imx_tlv320_dt_ids);

static struct platform_driver seco_tlv320_driver = {
	.driver = {
		.name = "seco_tlv320",
		.of_match_table = imx_tlv320_dt_ids,
	},
	.probe = seco_tlv320_probe,
	.remove = seco_tlv320_remove,
};

module_platform_driver(seco_tlv320_driver);

MODULE_AUTHOR("Michele Cirinei <michele.cirinei@seco.com>");
MODULE_DESCRIPTION("AIC32x4 ALSA SoC driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:seco_tlv320");


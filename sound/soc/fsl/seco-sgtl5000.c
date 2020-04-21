/*
 * seco-sgtl5000.c  --  SoC audio for seco_cpuimxXX in I2S mode
 *
 * Copyright 2017 Michele Cirinei, Seco srl <michele.cirinei@seco.com>
 *
 * based on sound/soc/fsl/eukrea-sgtl5000.c which is
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

//#include "../codecs/sgtl5000aic32x4.h"
#include "fsl_sai.h"

#define CODEC_CLOCK 22579200

static int seco_sgtl5000_hw_params(struct snd_pcm_substream *substream,
			    struct snd_pcm_hw_params *params)
{
	
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

static int seco_sgtl5000_late_probe(struct snd_soc_card *card)
{
        struct snd_soc_pcm_runtime *rtd = list_first_entry(
                &card->rtd_list, struct snd_soc_pcm_runtime, list);
        struct snd_soc_dai *codec_dai = rtd->codec_dai;
        int ret;

        ret = snd_soc_dai_set_sysclk(codec_dai, 0, CODEC_CLOCK,
                                                        SND_SOC_CLOCK_IN);

        return 0;
}

static struct snd_soc_ops seco_sgtl5000_snd_ops = {
	.hw_params	= seco_sgtl5000_hw_params,
};

static struct snd_soc_dai_link seco_sgtl5000_dai = {
	.name		= "HiFi",
	.stream_name	= "HiFi",
	.codec_dai_name	= "sgtl5000",
	.dai_fmt	= SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF |
			  SND_SOC_DAIFMT_CBS_CFS,
	.ops		= &seco_sgtl5000_snd_ops,
};

static struct snd_soc_card seco_sgtl5000 = {
	.owner		= THIS_MODULE,
	.dai_link	= &seco_sgtl5000_dai,
	.num_links	= 1,
	.late_probe 	= seco_sgtl5000_late_probe,
};

static int seco_sgtl5000_probe(struct platform_device *pdev)
{
	int ret;
	struct device_node *cpu_np = NULL, *codec_np = NULL; //, *asrc_np = NULL;
    struct platform_device *cpu_pdev;
    struct i2c_client *codec_dev;
    
	seco_sgtl5000.dev = &pdev->dev;
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

/*    
   	asrc_np = of_parse_phandle(pdev->dev.of_node, "asrc-controller", 0);
	if (asrc_np) {
		asrc_pdev = of_find_device_by_node(asrc_np);
		priv->asrc_pdev = asrc_pdev;
	}
*/
        
    seco_sgtl5000_dai.cpu_of_node = cpu_np;
	seco_sgtl5000_dai.platform_of_node = cpu_np;
    seco_sgtl5000_dai.codec_of_node	= codec_np;
    
    ret = snd_soc_of_parse_card_name(&seco_sgtl5000,"model");
    
	ret = snd_soc_register_card(&seco_sgtl5000);
err:
	if (ret)
		dev_err(&pdev->dev, "snd_soc_register_card failed (%d)\n", ret);
	
    if (cpu_np)
		of_node_put(cpu_np);
	if (codec_np)
		of_node_put(codec_np);

	return ret;
}

static int seco_sgtl5000_remove(struct platform_device *pdev)
{
	snd_soc_unregister_card(&seco_sgtl5000);

	return 0;
}

static const struct of_device_id imx_sgtl5000_dt_ids[] = {
	{ .compatible = "seco,asoc-sgtl5000"},
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, imx_sgtl5000_dt_ids);

static struct platform_driver seco_sgtl5000_driver = {
	.driver = {
		.name = "seco_sgtl5000",
		.of_match_table = imx_sgtl5000_dt_ids,
	},
	.probe = seco_sgtl5000_probe,
	.remove = seco_sgtl5000_remove,
};

module_platform_driver(seco_sgtl5000_driver);

MODULE_AUTHOR("Marco Sandrelli <marco.sandrelli@seco.com>");
MODULE_DESCRIPTION("AIC32x4 ALSA SoC driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:seco_sgtl5000");


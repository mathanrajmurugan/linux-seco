/*
 * imx-ac97-ac97_standard.c -- SoC audio for i.MX Seco UDOO board with
 *                                      STANDARD AC'97 codec 
 * Copyright:	Seco s.r.l.

 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */


#include <linux/module.h>
#include <linux/of_platform.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>

#include "../codecs/ac97_standard.h"
#include "imx-audmux.h"
#include "fsl_ssi.h"

#define DRV_NAME "imx-ac97-ac97_standard"

static int imx_ac97_standard_audio_params (struct snd_pcm_substream *substream,
        struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	int ret;

	ret = snd_soc_dai_set_fmt (cpu_dai, SND_SOC_DAIFMT_AC97
			| SND_SOC_DAIFMT_NB_NF
			| SND_SOC_DAIFMT_CBM_CFS);

	if ( ret < 0 ) {
		dev_err (cpu_dai->dev,
				"Failed to set cpu dai format: %d\n", ret);
		return ret;
	}

	return 0;
}


static struct snd_soc_ops imx_ac97_standard_audio_ops = {
	.hw_params = imx_ac97_standard_audio_params,
};


static struct snd_soc_dai_link imx_ac97_standard_dai = {
	.name		    = "ac97std_standard-AC97",
	.stream_name	= "AC97std-analog",
	.codec_dai_name	= "ac97std-hifi-analog",
};


static struct snd_soc_card imx_ac97_standard_card = {
	.name		= "imx-ac97_standard-audio",
	.owner  	= THIS_MODULE,
	.dai_link	= &imx_ac97_standard_dai,
	.num_links	= 1,
};


static int imx_audmux_ac97_config (struct platform_device *pdev, int intPort, int extPort) {
	int ret;
	unsigned int ptcr, pdcr;

	intPort = intPort - 1;
	extPort = extPort - 1;

	ptcr = IMX_AUDMUX_V2_PTCR_SYN | IMX_AUDMUX_V2_PTCR_TCLKDIR | IMX_AUDMUX_V2_PTCR_TCSEL(extPort);
	pdcr = IMX_AUDMUX_V2_PDCR_RXDSEL(extPort);

	ret = imx_audmux_v2_configure_port (intPort, ptcr, pdcr);
	if ( ret ) {
		dev_err (&pdev->dev, "Audmux internal port setup failed\n");
		return ret;
	}

	ptcr = IMX_AUDMUX_V2_PTCR_SYN | IMX_AUDMUX_V2_PTCR_TFSDIR | IMX_AUDMUX_V2_PTCR_TFSEL(intPort);

	pdcr = IMX_AUDMUX_V2_PDCR_RXDSEL (intPort);

	ret = imx_audmux_v2_configure_port (extPort, ptcr, pdcr);
	if ( ret ) {
		dev_err (&pdev->dev, "Audmux external port setup failed\n");
		return ret;
	}

	return 0;
}


static int imx_ac97_standard_probe (struct platform_device *pdev) {
	struct device_node *ssi_np, *codec_np, *np = pdev->dev.of_node;

	struct platform_device *codec_pdev;
	struct platform_device *ssi_pdev;
	int int_port, ext_port;
	int ret;

	ret = of_property_read_u32 (np, "mux-int-port", &int_port);
	if ( ret ) {
		dev_err (&pdev->dev, "mux-int-port property missing or invalid\n");
		return ret;
	}

	ret = of_property_read_u32 (np, "mux-ext-port", &ext_port);
	if ( ret ) {
		dev_err (&pdev->dev, "mux-ext-port property missing or invalid\n");
		return ret;
	}

	ret = imx_audmux_ac97_config (pdev, int_port, ext_port);
	if ( ret ) {
		dev_err (&pdev->dev, "Audmux port setup failed\n");
		return ret;
	}

	ssi_np = of_parse_phandle (np, "ssi-controller", 0);
	if ( !ssi_np ) {
		dev_err (&pdev->dev, "ssi-controller phandle missing or invalid\n");
		return -EINVAL;
	}
	ssi_pdev = of_find_device_by_node (ssi_np);
	if ( !ssi_pdev ) {
		dev_err (&pdev->dev, "Failed to find SSI platform device\n");
		ret = -EINVAL;
		goto fail;
	}

	codec_np = of_parse_phandle (np, "audio-codec", 0);
	if ( !codec_np ) {
		dev_err (&pdev->dev, "audio-codec phandle missing or invalid\n");
		ret = -EINVAL;
		goto fail;
	}
	codec_pdev = of_find_device_by_node (codec_np);
	if ( !codec_pdev ) {
		dev_err (&pdev->dev, "Failed to find codec device\n");
		ret = -EINVAL;
		goto fail;
	}

	imx_ac97_standard_dai.codec_name = dev_name (&codec_pdev->dev);
	imx_ac97_standard_dai.cpu_of_node = ssi_np;
	imx_ac97_standard_dai.cpu_dai_name = dev_name (&ssi_pdev->dev);
	imx_ac97_standard_dai.platform_of_node = ssi_np;
	imx_ac97_standard_dai.ops = &imx_ac97_standard_audio_ops;
	imx_ac97_standard_dai.dai_fmt = SND_SOC_DAIFMT_AC97 | SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBM_CFS;

	imx_ac97_standard_card.dev = &pdev->dev;

	platform_set_drvdata (pdev, &imx_ac97_standard_card);

	ret = snd_soc_register_card (&imx_ac97_standard_card);
	if ( ret )
		dev_err (&pdev->dev, "snd_soc_register_card() failed: %d\n", ret);

fail:
	if ( ssi_np )
		of_node_put (ssi_np);
	if ( codec_np )
		of_node_put (codec_np);

	return ret;
}


static int imx_ac97_standard_remove (struct platform_device *pdev) {
	int ret;
	struct snd_soc_card *card = platform_get_drvdata (pdev);

	ret = snd_soc_unregister_card (card);

	return ret;
}


static const struct of_device_id imx_ac97_standard_audio_match[] = {
	{ .compatible = "fsl,imx-ac97_standard-audio", },
	{}
};


MODULE_DEVICE_TABLE(of, imx_ac97_standard_audio_match);


static struct platform_driver imx_ac97_standard_driver = {
	.driver = {
		.name           = DRV_NAME,
		.owner          = THIS_MODULE,
		.of_match_table = imx_ac97_standard_audio_match,
	},
	.probe  = imx_ac97_standard_probe,
	.remove = imx_ac97_standard_remove,
};
module_platform_driver(imx_ac97_standard_driver);


MODULE_AUTHOR("Seco <info@seco.it>");
MODULE_DESCRIPTION(DRV_NAME ": Freescale i.MX standard AC97 ASoC machine driver");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:imx-ac97_standard");

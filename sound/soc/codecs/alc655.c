/*
* alc655.c  --  ALSA SoC alc655 AC'97 codec support
*
* Copyright 2010-2015 Seco s.r.l.
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* version 2 as published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
* 02110-1301 USA
*
*/

#include <linux/module.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/regmap.h>
#include <linux/slab.h>
#include <linux/of_device.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>
#include <sound/tlv.h>
#include <sound/ac97_codec.h>

#include "alc655.h"
#define DRV_NAME "alc655-codec"


struct alc655_priv {
	struct snd_ac97 *ac97;
	struct regmap   *regmap;
};


static const struct reg_default alc655_reg[] = {
	{ 0x00 , 0x0000 },  /*  Reset */
	{ 0x02 , 0x8000 },  /*  Master Volume */
	{ 0x06 , 0x8000 },  /*  Mono-Out Volume */
	{ 0x0A , 0x0000 },  /*  PC Beep Volume */
	{ 0x0C , 0x8008 },  /*  Phone Volume */
	{ 0x0E , 0x8008 },  /*  MIC Volume */
	{ 0x10 , 0x8808 },  /*  Line In Volume */
	{ 0x12 , 0x8808 },  /*  CD Volume */
	{ 0x16 , 0x8808 },  /*  AUX Volume */
	{ 0x18 , 0x8808 },  /*  PCM out Volume */
	{ 0x1A , 0x0000 },  /*  Record Select */
	{ 0x1C , 0x8000 },  /*  Record Gain */
	{ 0x20 , 0x0000 },  /*  General Purpose */
	{ 0x24 , 0x0001 },  /*  Audio Interrupt & Paging (AC'97 2.3) */
	{ 0x26 , 0x0000 },  /*  Powerdown control / status */
	{ 0x28 , 0x097C },  /*  Extended Audio ID */
	{ 0x2A , 0x3830 },  /*  Extended Audio Status and Control */
	{ 0x2C , 0xBB80 },  /*  PCM Front DAC Rate */
	{ 0x2E , 0XBB80 },  /*  PCM Surround Sample Rate */
	{ 0x30 , 0XBB80 },  /*  PCM LFE Sample Rate */
	{ 0x32 , 0xBB80 },  /*  PCM Input Sample Rate */
	{ 0x36 , 0x8080 },  /*  Center/LFE Volume */
	{ 0x3A , 0x2000 },  /*  S/PDIF Control */
	{ 0x64 , 0xFFFF },  /*  Sorrount DAC Volume */
	{ 0x66 , 0x0000 },  /*  S/PDIF RX Status */
	{ 0x6A , 0x0000 },  /*  Multi Channel Ctl */
	{ 0x7A , 0x60A2 },  /*  Extension Control */
	{ 0x7C , 0x414C },  /*  Vendor ID1 */
	{ 0x7E , 0x4760 },  /*  Vendor ID2 */
};



static bool alc655_volatile_reg( struct device *dev, unsigned int reg ) {
	return true;
}


static bool alc655_readable_reg( struct device *dev, unsigned int reg ) {
	switch (reg) {
		case AC97_RESET ... AC97_HEADPHONE:
		case AC97_PC_BEEP ... AC97_CD:
		case AC97_AUX ... AC97_GENERAL_PURPOSE:
		case AC97_INT_PAGING ... AC97_PCM_LR_ADC_RATE:
		case AC97_SPDIF:
		case AC97_AD_TEST:
		case AC97_ALC655_STEREO_MIC:
		case AC97_PCI_SVID ... AC97_SENSE_INFO:
		case AC97_ALC655_DAC_SLOT_MAP:
		case AC97_ALC655_ADC_SLOT_MAP:
		case AC97_AD_CODEC_CFG:
		case AC97_AD_SERIAL_CFG:
		case AC97_AD_MISC:
		case AC97_ALC655_GPIO_CTRL:
		case AC97_ALC655_GPIO_STATUS:
		case AC97_VENDOR_ID1:
		case AC97_VENDOR_ID2:
			return true;
		default:
			return false;
	}
}


static bool alc655_writeable_reg( struct device *dev, unsigned int reg ) {
	switch (reg) {
		case AC97_RESET:
		case AC97_EXTENDED_ID:
		case AC97_PCM_SURR_DAC_RATE:
		case AC97_PCM_LFE_DAC_RATE:
		case AC97_FUNC_SELECT:
		case AC97_FUNC_INFO:
		case AC97_ALC655_ADC_SLOT_MAP:
		case AC97_VENDOR_ID1:
		case AC97_VENDOR_ID2:
			return false;
		default:
			return true;
	}
}


static const struct regmap_config alc655_regmap = {
	.reg_bits = 8,
	.val_bits = 8,

	.max_register      = AC97_VENDOR_ID2,
	.reg_defaults      = alc655_reg,
	.num_reg_defaults  = ARRAY_SIZE(alc655_reg),

	.readable_reg      = alc655_readable_reg,
	.writeable_reg     = alc655_writeable_reg,
	.volatile_reg      = alc655_volatile_reg,

	.cache_type        = REGCACHE_RBTREE,
};


static const char *alc655_record_mux[] = {"Mic", "CD", "--", "AUX",
	"Line", "Stereo Mix", "Mono Mix", "Phone"};
static SOC_ENUM_DOUBLE_DECL(alc655_record_enum,
		AC97_REC_SEL, 8, 0, alc655_record_mux);

static const char *alc655_mic_mux[] = {"Mic1", "Mic2"};
static SOC_ENUM_SINGLE_DECL(alc655_mic_enum,
		AC97_GENERAL_PURPOSE, 8, alc655_mic_mux);

static const char *alc655_boost[] = {"0dB", "20dB"};
static SOC_ENUM_SINGLE_DECL(alc655_boost_enum,
		AC97_MIC, 6, alc655_boost);

static const char *alc655_mic_sel[] = {"MonoMic", "StereoMic"};
static SOC_ENUM_SINGLE_DECL(alc655_mic_sel_enum,
		AC97_ALC655_STEREO_MIC, 2, alc655_mic_sel);

static const DECLARE_TLV_DB_LINEAR(master_tlv, -4650, 0);
static const DECLARE_TLV_DB_LINEAR(record_tlv, 0, 2250);
static const DECLARE_TLV_DB_LINEAR(beep_tlv, -4500, 0);
static const DECLARE_TLV_DB_LINEAR(mix_tlv, -3450, 1200);

static const struct snd_kcontrol_new alc655_snd_ac97_controls[] = {
	SOC_DOUBLE_TLV("Speaker Playback Volume", AC97_MASTER, 8, 0, 31, 1, master_tlv),
	SOC_SINGLE("Speaker Playback Switch", AC97_MASTER, 15, 1, 1),

	SOC_DOUBLE_TLV("Headphone Playback Volume", AC97_HEADPHONE, 8, 0, 31, 1, master_tlv),
	SOC_SINGLE("Headphone Playback Switch", AC97_HEADPHONE, 15, 1, 1),

	SOC_DOUBLE_TLV("PCM Playback Volume", AC97_PCM, 8, 0, 31, 0, mix_tlv),
	SOC_SINGLE("PCM Playback Switch", AC97_PCM, 15, 1, 1),

	SOC_DOUBLE_TLV("Record Capture Volume", AC97_REC_GAIN, 8, 0, 15, 0, record_tlv),
	SOC_SINGLE("Record Capture Switch", AC97_REC_GAIN, 15, 1, 1),

	SOC_SINGLE_TLV("Beep Volume", AC97_PC_BEEP, 1, 15, 1, beep_tlv),
	SOC_SINGLE("Beep Switch", AC97_PC_BEEP, 15, 1, 1),
	SOC_SINGLE_TLV("Phone Volume", AC97_PHONE, 0, 31, 0, mix_tlv),
	SOC_SINGLE("Phone Switch", AC97_PHONE, 15, 1, 1),

	/* Mono Mic and Stereo Mic's right channel controls */
	SOC_SINGLE_TLV("Mic/StereoMic_R Volume", AC97_MIC, 0, 31, 0, mix_tlv),
	SOC_SINGLE("Mic/StereoMic_R Switch", AC97_MIC, 15, 1, 1),

	/* Stereo Mic's left channel controls */
	SOC_SINGLE("StereoMic_L Switch", AC97_MIC, 7, 1, 1),
	SOC_SINGLE_TLV("StereoMic_L Volume", AC97_MIC, 8, 31, 0, mix_tlv),

	SOC_DOUBLE_TLV("Line Volume", AC97_LINE, 8, 0, 31, 0, mix_tlv),
	SOC_SINGLE("Line Switch", AC97_LINE, 15, 1, 1),
	SOC_DOUBLE_TLV("CD Volume", AC97_CD, 8, 0, 31, 0, mix_tlv),
	SOC_SINGLE("CD Switch", AC97_CD, 15, 1, 1),
	SOC_DOUBLE_TLV("AUX Volume", AC97_AUX, 8, 0, 31, 0, mix_tlv),
	SOC_SINGLE("AUX Switch", AC97_AUX, 15, 1, 1),

	SOC_SINGLE("Analog Loopback", AC97_GENERAL_PURPOSE, 7, 1, 0),

	SOC_ENUM("Mic Boost", alc655_boost_enum),
	SOC_ENUM("Mic1/2 Mux", alc655_mic_enum),
	SOC_ENUM("Mic Select", alc655_mic_sel_enum),
	SOC_ENUM("Record Mux", alc655_record_enum),
};


static const unsigned int alc655_rates[] = {
	48000, 48000
};


static const struct snd_pcm_hw_constraint_list alc655_rate_constraints = {
	.count	= ARRAY_SIZE(alc655_rates),
	.list	= alc655_rates,
};


static int alc655_analog_prepare (struct snd_pcm_substream *substream, struct snd_soc_dai *dai) {
	struct snd_soc_component *component = dai->component;
	struct snd_pcm_runtime *runtime = substream->runtime;
	int reg;

	/* enable variable rate audio (VRA) and disable S/PDIF output */
	snd_soc_component_update_bits(component, AC97_EXTENDED_STATUS, 0x0001, 0x0005);

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK){
		reg = AC97_PCM_FRONT_DAC_RATE;
	} else {
		reg = AC97_PCM_LR_ADC_RATE;
	}

	return snd_soc_component_write(component, reg, runtime->rate);
}


static int alc655_digital_prepare (struct snd_pcm_substream *substream, struct snd_soc_dai *dai) {
	struct snd_soc_component *component = dai->component;
	struct snd_pcm_runtime *runtime = substream->runtime;

	snd_soc_component_write(component, AC97_SPDIF, 0x2002);

	/* enable VRA and S/PDIF output */
	snd_soc_component_update_bits(component, AC97_EXTENDED_STATUS, 0x0005, 0x0005);
	//alc655_write (codec,  AC97_EXTENDED_STATUS, alc655_read (codec, AC97_EXTENDED_STATUS) | 0x5);

	return snd_soc_component_write(component, AC97_PCM_FRONT_DAC_RATE, runtime->rate);
}


static int alc655_set_bias_level( struct snd_soc_component *component,	enum snd_soc_bias_level level ) {
	switch (level) {
		case SND_SOC_BIAS_ON: /* full On */
		case SND_SOC_BIAS_PREPARE: /* partial On */
		case SND_SOC_BIAS_STANDBY: /* Off, with power */
			snd_soc_component_write(component, AC97_POWERDOWN, 0x0000);
			break;
		case SND_SOC_BIAS_OFF: /* Off, without power */
			/* disable everything including AC link */
			snd_soc_component_write(component, AC97_POWERDOWN, 0xffff);
			break;
	}
	//codec->dapm.bias_level = level;
	return 0;
}


static int alc655_reset (struct snd_soc_component *component, int try_warm) {
	struct alc655_priv *alc655 = snd_soc_component_get_drvdata( component );

	if (try_warm && soc_ac97_ops->warm_reset) {
		soc_ac97_ops->warm_reset(alc655->ac97);
		if (  snd_soc_component_read32( component, AC97_RESET ) == 0x0 )
			return 1;
	}
	soc_ac97_ops->reset(alc655->ac97);

	if (soc_ac97_ops->warm_reset)
		soc_ac97_ops->warm_reset(alc655->ac97);

	if ( snd_soc_component_read32( component, AC97_RESET ) == 0x0 )
		return 0;

	return -EIO;
}


static struct snd_soc_dai_ops alc655_dai_ops_analog = {
//	.startup = alc655_startup,
	.prepare = alc655_analog_prepare,
};


static struct snd_soc_dai_ops alc655_dai_ops_digital = {
	.prepare = alc655_digital_prepare,
};


static struct snd_soc_dai_driver alc655_dai[] = {
	{
 		.name = "alc655-hifi-analog",

		.playback = {
			.stream_name = "Playback",
			.channels_min = 1,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_48000, //SNDRV_PCM_RATE_KNOT,
			.formats = SND_SOC_STD_AC97_FMTS,
		},
		.capture = {
			.stream_name = "Capture",
			.channels_min = 1,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_KNOT,
			.formats = SND_SOC_STD_AC97_FMTS,
		},

		.ops = &alc655_dai_ops_analog,
	},
	{
		.name = "alc655-hifi-IEC958",
		///.ac97_control = 1,

		.playback = {
			.stream_name = "alc655 IEC958",
			.channels_min = 1,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_32000 |
				SNDRV_PCM_RATE_44100 | SNDRV_PCM_RATE_48000,
			.formats = SNDRV_PCM_FORMAT_IEC958_SUBFRAME_BE,
		},

		.ops = &alc655_dai_ops_digital,
	},
};


static int alc655_soc_probe( struct snd_soc_component *component ) {
 	int ret = 0;
	u32 reg1, reg2;

	struct alc655_priv *alc655 = snd_soc_component_get_drvdata( component );

	// alc655->regmap = devm_regmap_init( &pdev->dev, NULL, alc655, &alc655_regmap );
	// if ( IS_ERR( alc655->regmap ) ) {
	// 	ret = PTR_ERR( alc655->regmap );
	// 	return ret;
	// }

#ifdef CONFIG_SND_SOC_AC97_BUS
 		alc655->ac97 = snd_soc_new_ac97_component( component, 0x0,
						      0x0);
		if ( IS_ERR( alc655->ac97 ) )
			return PTR_ERR( alc655->ac97 );
		alc655->regmap = regmap_init_ac97( alc655->ac97, &alc655_regmap );
		if ( IS_ERR( alc655->regmap ) ) {
			snd_soc_free_ac97_component( alc655->ac97 );
			return PTR_ERR( alc655->regmap );
		}
#endif

	snd_soc_component_init_regmap( component, alc655->regmap );
	// regcache_mark_dirty(component->regmap);
	// snd_soc_component_cache_sync(component);



// 	if ( !soc_ac97_ops ){
// 		dev_err(codec->dev, "Failed to register AC97 codec: %d\n", ret);
// 		return -ENOMEM;
// 	}


// 	snd_soc_codec_set_drvdata (codec, alc655->ac97);

 	/* do a cold reset for the controller and then try
 	 * a warm reset followed by an optional cold reset for codec */
 	alc655_reset (component, 0);
 	ret = alc655_reset (component, 1);
 	if ( ret < 0 ) {
 		printk(KERN_ERR "Failed to reset alc655: AC97 link error\n");
 		goto err_put_device;
 	}

 	// ret = device_add(&alc655->ac97->dev);
 	// if (ret)
 	// 	goto err_put_device;

 	/* Read out vendor IDs */
	reg1 = snd_soc_component_read32( component, AC97_VENDOR_ID1 );
	reg2 = snd_soc_component_read32( component, AC97_VENDOR_ID2 );
 	printk( KERN_INFO "alc655 SoC Audio Codec [ID = %04x - %04x]\n", reg1, reg2 );

 	/*  Set initial state of the codec  */
 	alc655_set_bias_level( component, SND_SOC_BIAS_STANDBY );

	/* unmute captures and playbacks volume */
	snd_soc_component_write( component, AC97_MASTER, 0x0000 );
	snd_soc_component_write( component, AC97_PCM, 0x0000 );
	snd_soc_component_write( component, AC97_REC_GAIN, 0x0000 );

 	/* At 3.3V analog supply, for the bits 3:2 should be set 10b for the lowest power instead of default 00b */
 	snd_soc_component_update_bits(component, AC97_AD_TEST, 0x0008, 0x0008);

 	/* To maximize recording quality by removing white noise */
 	snd_soc_component_update_bits(component, AC97_AD_TEST, 0x0400, 0x0400);

 	return 0;

err_put_device:
//  	put_device(&alc655->ac97->dev);
	return ret;
}


struct snd_soc_component_driver alc655_codec = {
 	.probe             = alc655_soc_probe,
 	.set_bias_level    = alc655_set_bias_level,
	.controls          = alc655_snd_ac97_controls,
	.num_controls      = ARRAY_SIZE(alc655_snd_ac97_controls),
};


static int alc655_probe (struct platform_device *pdev) {
	int                ret;
	struct alc655_priv *alc655;
	
	alc655 = devm_kzalloc( &pdev->dev, sizeof(*alc655), GFP_KERNEL );
	if ( alc655 == NULL )
	 	return -ENOMEM;

	platform_set_drvdata( pdev, alc655 );

	ret = devm_snd_soc_register_component( &pdev->dev,
			&alc655_codec, alc655_dai, ARRAY_SIZE(alc655_dai) );

	return ret;
}


static int alc655_remove (struct platform_device *pdev) {
	//snd_soc_unregister_codec (&pdev->dev);
	return 0;
}


static const struct of_device_id alc655_of_match[] = {
	{ .compatible = "seco,alc655", },
	{ }
};
MODULE_DEVICE_TABLE(of, alc655_of_match);


static struct platform_driver alc655_codec_driver = {
	.driver = {
		.name           = DRV_NAME,
		.owner          = THIS_MODULE,
		.of_match_table = alc655_of_match,
	},
	.probe  = alc655_probe,
	.remove = alc655_remove,
};

module_platform_driver(alc655_codec_driver);


MODULE_DESCRIPTION("ASoC AC'97 ALC655 codec driver");
MODULE_AUTHOR("Seco s.r.l.");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:" DRV_NAME);

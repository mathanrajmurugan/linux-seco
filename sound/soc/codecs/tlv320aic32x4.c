/*
 * linux/sound/soc/codecs/tlv320aic32x4.c
 *
 * Copyright 2011 Vista Silicon S.L.
 *
 * Author: Javier Martin <javier.martin@vista-silicon.com>
 *
 * Based on sound/soc/codecs/wm8974 and TI driver for kernel 2.6.27.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/regulator/consumer.h>

#include <sound/tlv320aic32x4.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>
#include <sound/tlv.h>

#include "tlv320aic32x4.h"

#define USE_PLL

struct aic32x4_rate_divs {
	u32 mclk;
	u32 rate;
	u8 p_val;
	u8 pll_j;
	u16 pll_d;
	u16 dosr;
	u8 ndac;
	u8 mdac;
	u8 aosr;
	u8 nadc;
	u8 madc;
	u8 blck_N;
};

struct aic32x4_priv {
	struct regmap *regmap;
	u32 sysclk;
	u32 power_cfg;
	u32 micpga_routing;
	bool swapdacs;
	bool lineout_routing;
	int rstn_gpio;
	int enamp_gpio;
	int enmic_gpio;
	struct clk *mclk;
	int mclk_count;

	struct regulator *supply_ldo;
	struct regulator *supply_iov;
	struct regulator *supply_dv;
	struct regulator *supply_av;

};

/* 0dB min, 0.5dB steps */
static DECLARE_TLV_DB_SCALE(tlv_step_0_5, 0, 50, 0);
/* -63.5dB min, 0.5dB steps */
static DECLARE_TLV_DB_SCALE(tlv_pcm, -6350, 50, 0);
/* -6dB min, 1dB steps */
static DECLARE_TLV_DB_SCALE(tlv_driver_gain, -600, 100, 0);
/* -12dB min, 0.5dB steps */
static DECLARE_TLV_DB_SCALE(tlv_adc_vol, -1200, 50, 0);
/*  0dB min, 0.5dB step */
static DECLARE_TLV_DB_SCALE(tlv_ma_vol, 0, 50, 0);


static const struct snd_kcontrol_new aic32x4_snd_controls[] = {
	SOC_DOUBLE_R_S_TLV("PCM Playback Volume", AIC32X4_LDACVOL,
			AIC32X4_RDACVOL, 0, -0x7f, 0x30, 7, 0, tlv_pcm),
	SOC_DOUBLE_R_S_TLV("HP Driver Gain Volume", AIC32X4_HPLGAIN,
			AIC32X4_HPRGAIN, 0, -0x6, 0x1d, 5, 0,
			tlv_driver_gain),
	SOC_DOUBLE_R_S_TLV("LO Driver Gain Volume", AIC32X4_LOLGAIN,
			AIC32X4_LORGAIN, 0, -0x6, 0x1d, 5, 0,
			tlv_driver_gain),
	SOC_DOUBLE_R("HP DAC Playback Switch", AIC32X4_HPLGAIN,
			AIC32X4_HPRGAIN, 6, 0x01, 1),
	SOC_DOUBLE_R("LO DAC Playback Switch", AIC32X4_LOLGAIN,
			AIC32X4_LORGAIN, 6, 0x01, 1),
	SOC_DOUBLE_R("Mic PGA Switch", AIC32X4_LMICPGAVOL,
			AIC32X4_RMICPGAVOL, 7, 0x01, 1),
	SOC_DOUBLE_R_S_TLV("Mixer Amp. Driver Gain Volume", AIC32X4_MALVOL,
			AIC32X4_MARVOL, 0, -0x0, 0x28, 5, 1, tlv_ma_vol),

	SOC_SINGLE("ADCFGA Left Mute Switch", AIC32X4_ADCFGA, 7, 1, 0),
	SOC_SINGLE("ADCFGA Right Mute Switch", AIC32X4_ADCFGA, 3, 1, 0),

	SOC_DOUBLE_R_S_TLV("ADC Level Volume", AIC32X4_LADCVOL,
			AIC32X4_RADCVOL, 0, -0x18, 0x28, 6, 0, tlv_adc_vol),
	SOC_DOUBLE_R_TLV("PGA Level Volume", AIC32X4_LMICPGAVOL,
			AIC32X4_RMICPGAVOL, 0, 0x5f, 0, tlv_step_0_5),

	SOC_SINGLE("Auto-mute Switch", AIC32X4_DACMUTE, 4, 7, 0),

	SOC_SINGLE("AGC Left Switch", AIC32X4_LAGC1, 7, 1, 0),
	SOC_SINGLE("AGC Right Switch", AIC32X4_RAGC1, 7, 1, 0),
	SOC_DOUBLE_R("AGC Target Level", AIC32X4_LAGC1, AIC32X4_RAGC1,
			4, 0x07, 0),
	SOC_DOUBLE_R("AGC Gain Hysteresis", AIC32X4_LAGC1, AIC32X4_RAGC1,
			0, 0x03, 0),
	SOC_DOUBLE_R("AGC Hysteresis", AIC32X4_LAGC2, AIC32X4_RAGC2,
			6, 0x03, 0),
	SOC_DOUBLE_R("AGC Noise Threshold", AIC32X4_LAGC2, AIC32X4_RAGC2,
			1, 0x1F, 0),
	SOC_DOUBLE_R("AGC Max PGA", AIC32X4_LAGC3, AIC32X4_RAGC3,
			0, 0x7F, 0),
	SOC_DOUBLE_R("AGC Attack Time", AIC32X4_LAGC4, AIC32X4_RAGC4,
			3, 0x1F, 0),
	SOC_DOUBLE_R("AGC Decay Time", AIC32X4_LAGC5, AIC32X4_RAGC5,
			3, 0x1F, 0),
	SOC_DOUBLE_R("AGC Noise Debounce", AIC32X4_LAGC6, AIC32X4_RAGC6,
			0, 0x1F, 0),
	SOC_DOUBLE_R("AGC Signal Debounce", AIC32X4_LAGC7, AIC32X4_RAGC7,
			0, 0x0F, 0),
};

static const struct aic32x4_rate_divs aic32x4_divs[] = {
	/* 8k rate */
	{12000000, 8000, 1, 7, 6800, 768, 5, 3, 128, 5, 18, 24},
	{24000000, 8000, 2, 7, 6800, 768, 15, 1, 64, 45, 4, 24},
	{25000000, 8000, 2, 7, 3728, 768, 15, 1, 64, 45, 4, 24},
	/* 11.025k rate */
	{12000000, 11025, 1, 7, 5264, 512, 8, 2, 128, 8, 8, 16},
	{24000000, 11025, 2, 7, 5264, 512, 16, 1, 64, 32, 4, 16},
	/* 16k rate */
	{12000000, 16000, 1, 7, 6800, 384, 5, 3, 128, 5, 9, 12},
	{24000000, 16000, 2, 7, 6800, 384, 15, 1, 64, 18, 5, 12},
	{25000000, 16000, 2, 7, 3728, 384, 15, 1, 64, 18, 5, 12},
	/* 22.05k rate */
	{12000000, 22050, 1, 7, 5264, 256, 4, 4, 128, 4, 8, 8},
	{24000000, 22050, 2, 7, 5264, 256, 16, 1, 64, 16, 4, 8},
	{25000000, 22050, 2, 7, 2253, 256, 16, 1, 64, 16, 4, 8},
	/* 32k rate */
	{12000000, 32000, 1, 7, 1680, 192, 2, 7, 64, 2, 21, 6},
	{24000000, 32000, 2, 7, 1680, 192, 7, 2, 64, 7, 6, 6},
	/* 44.1k rate */
	{12000000, 44100, 1, 7, 5264, 128, 2, 8, 128, 2, 8, 4},
	{24000000, 44100, 2, 7, 5264, 128, 8, 2, 64, 8, 4, 4},
	{24576000, 44100, 2, 8, 5264, 128, 8, 2, 64, 8, 4, 4},
	{25000000, 44100, 2, 7, 2253, 128, 8, 2, 64, 8, 4, 4},
	/* 48k rate */
	{12000000, 48000, 1, 8, 1920, 128, 2, 8, 128, 2, 8, 4},
	{24000000, 48000, 2, 8, 1920, 128, 8, 2, 64, 8, 4, 4},
	{25000000, 48000, 2, 7, 8643, 128, 8, 2, 64, 8, 4, 4},
	/* 
    CODEC_CLKIN = NDAC * MDAC * DOSR * DAC_FS
    CODEC_CLKIN = MCLK = 24576000
    DAC_FS = 48000
    2.8MHz < DOSR * DAC_FS < 6.2 MHz  =>  2800/48 < DOSR < 129

    We choose DOSR = 128, as is for similar cases
    NDAC as large as possible, given MDAC * DOSR / 32 >= RC
    RC = 8 for Processing Block 1 (defult one)

    MDAC * DOSR / 32 >= RC  =>  MDAC >= 8 / (128/32) => MDAC >=2

    NDAC = 2, MDAC = 2, DOSR = 128
    */ 
    {24576000, 48000, 1, 4,    0, 128, 2, 2, 128, 2, 2, 1},



	/* 96k rate */
	{25000000, 96000, 2, 7, 8643, 64, 4, 4, 64, 4, 4, 1},
};

static const struct snd_kcontrol_new hpl_output_mixer_controls[] = {
	SOC_DAPM_SINGLE("L_DAC Switch", AIC32X4_HPLROUTE, 3, 1, 0),
	SOC_DAPM_SINGLE("IN1_L Switch", AIC32X4_HPLROUTE, 2, 1, 0),
	SOC_DAPM_SINGLE("MAL Switch", AIC32X4_HPLROUTE, 1, 1, 0),
};

static const struct snd_kcontrol_new hpr_output_mixer_controls[] = {
	SOC_DAPM_SINGLE("R_DAC Switch", AIC32X4_HPRROUTE, 3, 1, 0),
	SOC_DAPM_SINGLE("IN1_R Switch", AIC32X4_HPRROUTE, 2, 1, 0),
	SOC_DAPM_SINGLE("MAR Switch", AIC32X4_HPRROUTE, 1, 1, 0),
};

static const struct snd_kcontrol_new lol_output_mixer_controls[] = {
	SOC_DAPM_SINGLE("L_DAC Switch", AIC32X4_LOLROUTE, 3, 1, 0),
};

static const struct snd_kcontrol_new lor_output_mixer_controls[] = {
	SOC_DAPM_SINGLE("R_DAC Switch", AIC32X4_LORROUTE, 3, 1, 0),
};

static const char * const resistor_text[] = {
	"Off", "10 kOhm", "20 kOhm", "40 kOhm",
};

/* Left mixer pins */
static SOC_ENUM_SINGLE_DECL(in1l_lpga_p_enum, AIC32X4_LMICPGAPIN, 6, resistor_text);
static SOC_ENUM_SINGLE_DECL(in2l_lpga_p_enum, AIC32X4_LMICPGAPIN, 4, resistor_text);
static SOC_ENUM_SINGLE_DECL(in3l_lpga_p_enum, AIC32X4_LMICPGAPIN, 2, resistor_text);
static SOC_ENUM_SINGLE_DECL(in1r_lpga_p_enum, AIC32X4_LMICPGAPIN, 0, resistor_text);

static SOC_ENUM_SINGLE_DECL(cm1l_lpga_n_enum, AIC32X4_LMICPGANIN, 6, resistor_text);
static SOC_ENUM_SINGLE_DECL(in2r_lpga_n_enum, AIC32X4_LMICPGANIN, 4, resistor_text);
static SOC_ENUM_SINGLE_DECL(in3r_lpga_n_enum, AIC32X4_LMICPGANIN, 2, resistor_text);
static SOC_ENUM_SINGLE_DECL(cm2l_lpga_n_enum, AIC32X4_LMICPGANIN, 0, resistor_text);

static const struct snd_kcontrol_new in1l_to_lmixer_controls[] = {
	SOC_DAPM_ENUM("IN1_L L+ Switch", in1l_lpga_p_enum),
};
static const struct snd_kcontrol_new in2l_to_lmixer_controls[] = {
	SOC_DAPM_ENUM("IN2_L L+ Switch", in2l_lpga_p_enum),
};
static const struct snd_kcontrol_new in3l_to_lmixer_controls[] = {
	SOC_DAPM_ENUM("IN3_L L+ Switch", in3l_lpga_p_enum),
};
static const struct snd_kcontrol_new in1r_to_lmixer_controls[] = {
	SOC_DAPM_ENUM("IN1_R L+ Switch", in1r_lpga_p_enum),
};
static const struct snd_kcontrol_new cm1l_to_lmixer_controls[] = {
	SOC_DAPM_ENUM("CM1_L L- Switch", cm1l_lpga_n_enum),
};
static const struct snd_kcontrol_new in2r_to_lmixer_controls[] = {
	SOC_DAPM_ENUM("IN2_R L- Switch", in2r_lpga_n_enum),
};
static const struct snd_kcontrol_new in3r_to_lmixer_controls[] = {
	SOC_DAPM_ENUM("IN3_R L- Switch", in3r_lpga_n_enum),
};
static const struct snd_kcontrol_new cm2l_to_lmixer_controls[] = {
	SOC_DAPM_ENUM("CM2_L L- Switch", cm2l_lpga_n_enum),
};

/*  Right mixer pins */
static SOC_ENUM_SINGLE_DECL(in1r_rpga_p_enum, AIC32X4_RMICPGAPIN, 6, resistor_text);
static SOC_ENUM_SINGLE_DECL(in2r_rpga_p_enum, AIC32X4_RMICPGAPIN, 4, resistor_text);
static SOC_ENUM_SINGLE_DECL(in3r_rpga_p_enum, AIC32X4_RMICPGAPIN, 2, resistor_text);
static SOC_ENUM_SINGLE_DECL(in2l_rpga_p_enum, AIC32X4_RMICPGAPIN, 0, resistor_text);
static SOC_ENUM_SINGLE_DECL(cm1r_rpga_n_enum, AIC32X4_RMICPGANIN, 6, resistor_text);
static SOC_ENUM_SINGLE_DECL(in1l_rpga_n_enum, AIC32X4_RMICPGANIN, 4, resistor_text);
static SOC_ENUM_SINGLE_DECL(in3l_rpga_n_enum, AIC32X4_RMICPGANIN, 2, resistor_text);
static SOC_ENUM_SINGLE_DECL(cm2r_rpga_n_enum, AIC32X4_RMICPGANIN, 0, resistor_text);

static const struct snd_kcontrol_new in1r_to_rmixer_controls[] = {
	SOC_DAPM_ENUM("IN1_R R+ Switch", in1r_rpga_p_enum),
};
static const struct snd_kcontrol_new in2r_to_rmixer_controls[] = {
	SOC_DAPM_ENUM("IN2_R R+ Switch", in2r_rpga_p_enum),
};
static const struct snd_kcontrol_new in3r_to_rmixer_controls[] = {
	SOC_DAPM_ENUM("IN3_R R+ Switch", in3r_rpga_p_enum),
};
static const struct snd_kcontrol_new in2l_to_rmixer_controls[] = {
	SOC_DAPM_ENUM("IN2_L R+ Switch", in2l_rpga_p_enum),
};
static const struct snd_kcontrol_new cm1r_to_rmixer_controls[] = {
	SOC_DAPM_ENUM("CM1_R R- Switch", cm1r_rpga_n_enum),
};
static const struct snd_kcontrol_new in1l_to_rmixer_controls[] = {
	SOC_DAPM_ENUM("IN1_L R- Switch", in1l_rpga_n_enum),
};
static const struct snd_kcontrol_new in3l_to_rmixer_controls[] = {
	SOC_DAPM_ENUM("IN3_L R- Switch", in3l_rpga_n_enum),
};
static const struct snd_kcontrol_new cm2r_to_rmixer_controls[] = {
	SOC_DAPM_ENUM("CM2_R R- Switch", cm2r_rpga_n_enum),
};


static const struct snd_soc_dapm_widget aic32x4_dapm_widgets[] = {
	SND_SOC_DAPM_DAC("Left DAC", "Left Playback", AIC32X4_DACSETUP, 7, 0),
	SND_SOC_DAPM_MIXER("HPL Output Mixer", SND_SOC_NOPM, 0, 0,
			   &hpl_output_mixer_controls[0],
			   ARRAY_SIZE(hpl_output_mixer_controls)),
	SND_SOC_DAPM_PGA("HPL Power", AIC32X4_OUTPWRCTL, 5, 0, NULL, 0),
	SND_SOC_DAPM_PGA("MAL Power", AIC32X4_OUTPWRCTL, 1, 0, NULL, 0),

	SND_SOC_DAPM_MIXER("LOL Output Mixer", SND_SOC_NOPM, 0, 0,
			   &lol_output_mixer_controls[0],
			   ARRAY_SIZE(lol_output_mixer_controls)),
	SND_SOC_DAPM_PGA("LOL Power", AIC32X4_OUTPWRCTL, 3, 0, NULL, 0),

	SND_SOC_DAPM_DAC("Right DAC", "Right Playback", AIC32X4_DACSETUP, 6, 0),
	SND_SOC_DAPM_MIXER("HPR Output Mixer", SND_SOC_NOPM, 0, 0,
			   &hpr_output_mixer_controls[0],
			   ARRAY_SIZE(hpr_output_mixer_controls)),
	SND_SOC_DAPM_PGA("HPR Power", AIC32X4_OUTPWRCTL, 4, 0, NULL, 0),
	SND_SOC_DAPM_PGA("MAR Power", AIC32X4_OUTPWRCTL, 0, 0, NULL, 0),
	SND_SOC_DAPM_MIXER("LOR Output Mixer", SND_SOC_NOPM, 0, 0,
			   &lor_output_mixer_controls[0],
			   ARRAY_SIZE(lor_output_mixer_controls)),
	SND_SOC_DAPM_PGA("LOR Power", AIC32X4_OUTPWRCTL, 2, 0, NULL, 0),

	SND_SOC_DAPM_ADC("Right ADC", "Right Capture", AIC32X4_ADCSETUP, 6, 0),
	SND_SOC_DAPM_MUX("IN1_R to Right Mixer Positive Resistor", SND_SOC_NOPM, 0, 0,
			in1r_to_rmixer_controls),
	SND_SOC_DAPM_MUX("IN2_R to Right Mixer Positive Resistor", SND_SOC_NOPM, 0, 0,
			in2r_to_rmixer_controls),
	SND_SOC_DAPM_MUX("IN3_R to Right Mixer Positive Resistor", SND_SOC_NOPM, 0, 0,
			in3r_to_rmixer_controls),
	SND_SOC_DAPM_MUX("IN2_L to Right Mixer Positive Resistor", SND_SOC_NOPM, 0, 0,
			in2l_to_rmixer_controls),
	SND_SOC_DAPM_MUX("CM1_R to Right Mixer Negative Resistor", SND_SOC_NOPM, 0, 0,
			cm1r_to_rmixer_controls),
	SND_SOC_DAPM_MUX("IN1_L to Right Mixer Negative Resistor", SND_SOC_NOPM, 0, 0,
			in1l_to_rmixer_controls),
	SND_SOC_DAPM_MUX("IN3_L to Right Mixer Negative Resistor", SND_SOC_NOPM, 0, 0,
			in3l_to_rmixer_controls),
	SND_SOC_DAPM_MUX("CM2_R to Right Mixer Negative Resistor", SND_SOC_NOPM, 0, 0,
			cm2r_to_rmixer_controls),


	SND_SOC_DAPM_ADC("Left ADC", "Left Capture", AIC32X4_ADCSETUP, 7, 0),
	SND_SOC_DAPM_MUX("IN1_L to Left Mixer Positive Resistor", SND_SOC_NOPM, 0, 0,
			in1l_to_lmixer_controls),
	SND_SOC_DAPM_MUX("IN2_L to Left Mixer Positive Resistor", SND_SOC_NOPM, 0, 0,
			in2l_to_lmixer_controls),
	SND_SOC_DAPM_MUX("IN3_L to Left Mixer Positive Resistor", SND_SOC_NOPM, 0, 0,
			in3l_to_lmixer_controls),
	SND_SOC_DAPM_MUX("IN1_R to Left Mixer Positive Resistor", SND_SOC_NOPM, 0, 0,
			in1r_to_lmixer_controls),
	SND_SOC_DAPM_MUX("CM1_L to Left Mixer Negative Resistor", SND_SOC_NOPM, 0, 0,
			cm1l_to_lmixer_controls),
	SND_SOC_DAPM_MUX("IN2_R to Left Mixer Negative Resistor", SND_SOC_NOPM, 0, 0,
			in2r_to_lmixer_controls),
	SND_SOC_DAPM_MUX("IN3_R to Left Mixer Negative Resistor", SND_SOC_NOPM, 0, 0,
			in3r_to_lmixer_controls),
	SND_SOC_DAPM_MUX("CM2_L to Left Mixer Negative Resistor", SND_SOC_NOPM, 0, 0,
			cm2l_to_lmixer_controls),


	SND_SOC_DAPM_MICBIAS("Mic Bias", AIC32X4_MICBIAS, 6, 0),

	SND_SOC_DAPM_OUTPUT("HPL"),
	SND_SOC_DAPM_OUTPUT("HPR"),
	SND_SOC_DAPM_OUTPUT("LOL"),
	SND_SOC_DAPM_OUTPUT("LOR"),
	SND_SOC_DAPM_INPUT("IN1_L"),
	SND_SOC_DAPM_INPUT("IN1_R"),
	SND_SOC_DAPM_INPUT("IN2_L"),
	SND_SOC_DAPM_INPUT("IN2_R"),
	SND_SOC_DAPM_INPUT("IN3_L"),
	SND_SOC_DAPM_INPUT("IN3_R"),
	SND_SOC_DAPM_INPUT("CM_L"),
	SND_SOC_DAPM_INPUT("CM_R"),
    SND_SOC_DAPM_INPUT("MAL"),
    SND_SOC_DAPM_INPUT("MAR"),

};

static const struct snd_soc_dapm_route aic32x4_dapm_routes[] = {
	/* Left Output */
	{"HPL Output Mixer", "L_DAC Switch", "Left DAC"},
	{"HPL Output Mixer", "IN1_L Switch", "IN1_L"},
	{"HPL Output Mixer", "MAL Switch", "MAL"},

	{"HPL Power", NULL, "HPL Output Mixer"},
	{"HPL", NULL, "HPL Power"},

	{"MAL Power", NULL, "HPL Output Mixer"},
	{"MAL", NULL, "MAL Power"},

	{"LOL Output Mixer", "L_DAC Switch", "Left DAC"},

	{"LOL Power", NULL, "LOL Output Mixer"},
	{"LOL", NULL, "LOL Power"},

	/* Right Output */
	{"HPR Output Mixer", "R_DAC Switch", "Right DAC"},
	{"HPR Output Mixer", "IN1_R Switch", "IN1_R"},
	{"HPR Output Mixer", "MAR Switch", "MAR"},

	{"HPR Power", NULL, "HPR Output Mixer"},
	{"HPR", NULL, "HPR Power"},

	{"MAR Power", NULL, "HPR Output Mixer"},
	{"MAR", NULL, "MAR Power"},

	{"LOR Output Mixer", "R_DAC Switch", "Right DAC"},

	{"LOR Power", NULL, "LOR Output Mixer"},
	{"LOR", NULL, "LOR Power"},

	/* Left Positive Input */
	{"Left ADC", NULL, "IN1_L to Left Mixer Positive Resistor"},
	{"IN1_L to Left Mixer Positive Resistor", "10 kOhm", "IN1_L"},
	{"IN1_L to Left Mixer Positive Resistor", "20 kOhm", "IN1_L"},
	{"IN1_L to Left Mixer Positive Resistor", "40 kOhm", "IN1_L"},

	{"Left ADC", NULL, "IN2_L to Left Mixer Positive Resistor"},
	{"IN2_L to Left Mixer Positive Resistor", "10 kOhm", "IN2_L"},
	{"IN2_L to Left Mixer Positive Resistor", "20 kOhm", "IN2_L"},
	{"IN2_L to Left Mixer Positive Resistor", "40 kOhm", "IN2_L"},

	{"Left ADC", NULL, "IN3_L to Left Mixer Positive Resistor"},
	{"IN3_L to Left Mixer Positive Resistor", "10 kOhm", "IN3_L"},
	{"IN3_L to Left Mixer Positive Resistor", "20 kOhm", "IN3_L"},
	{"IN3_L to Left Mixer Positive Resistor", "40 kOhm", "IN3_L"},

	{"Left ADC", NULL, "IN1_R to Left Mixer Positive Resistor"},
	{"IN1_R to Left Mixer Positive Resistor", "10 kOhm", "IN1_R"},
	{"IN1_R to Left Mixer Positive Resistor", "20 kOhm", "IN1_R"},
	{"IN1_R to Left Mixer Positive Resistor", "40 kOhm", "IN1_R"},

	/* Left Negative Input */
	{"Left ADC", NULL, "CM1_L to Left Mixer Negative Resistor"},
	{"CM1_L to Left Mixer Negative Resistor", "10 kOhm", "CM_L"},
	{"CM1_L to Left Mixer Negative Resistor", "20 kOhm", "CM_L"},
	{"CM1_L to Left Mixer Negative Resistor", "40 kOhm", "CM_L"},

	{"Left ADC", NULL, "IN2_R to Left Mixer Negative Resistor"},
	{"IN2_R to Left Mixer Negative Resistor", "10 kOhm", "IN2_R"},
	{"IN2_R to Left Mixer Negative Resistor", "20 kOhm", "IN2_R"},
	{"IN2_R to Left Mixer Negative Resistor", "40 kOhm", "IN2_R"},

	{"Left ADC", NULL, "IN3_R to Left Mixer Negative Resistor"},
	{"IN3_R to Left Mixer Negative Resistor", "10 kOhm", "IN3_R"},
	{"IN3_R to Left Mixer Negative Resistor", "20 kOhm", "IN3_R"},
	{"IN3_R to Left Mixer Negative Resistor", "40 kOhm", "IN3_R"},

	{"Left ADC", NULL, "CM2_L to Left Mixer Negative Resistor"},
	{"CM2_L to Left Mixer Negative Resistor", "10 kOhm", "CM_L"},
	{"CM2_L to Left Mixer Negative Resistor", "20 kOhm", "CM_L"},
	{"CM2_L to Left Mixer Negative Resistor", "40 kOhm", "CM_L"},


	/* Right Positive Input */
	{"Right ADC", NULL, "IN1_R to Right Mixer Positive Resistor"},
	{"IN1_R to Right Mixer Positive Resistor", "10 kOhm", "IN1_R"},
	{"IN1_R to Right Mixer Positive Resistor", "20 kOhm", "IN1_R"},
	{"IN1_R to Right Mixer Positive Resistor", "40 kOhm", "IN1_R"},

	{"Right ADC", NULL, "IN2_R to Right Mixer Positive Resistor"},
	{"IN2_R to Right Mixer Positive Resistor", "10 kOhm", "IN2_R"},
	{"IN2_R to Right Mixer Positive Resistor", "20 kOhm", "IN2_R"},
	{"IN2_R to Right Mixer Positive Resistor", "40 kOhm", "IN2_R"},

	{"Right ADC", NULL, "IN3_R to Right Mixer Positive Resistor"},
	{"IN3_R to Right Mixer Positive Resistor", "10 kOhm", "IN3_R"},
	{"IN3_R to Right Mixer Positive Resistor", "20 kOhm", "IN3_R"},
	{"IN3_R to Right Mixer Positive Resistor", "40 kOhm", "IN3_R"},

	{"Right ADC", NULL, "IN2_L to Right Mixer Positive Resistor"},
	{"IN2_L to Right Mixer Positive Resistor", "10 kOhm", "IN2_L"},
	{"IN2_L to Right Mixer Positive Resistor", "20 kOhm", "IN2_L"},
	{"IN2_L to Right Mixer Positive Resistor", "40 kOhm", "IN2_L"},

	/* Right Negative Input */
	{"Right ADC", NULL, "CM1_R to Right Mixer Negative Resistor"},
	{"CM1_R to Right Mixer Negative Resistor", "10 kOhm", "CM_R"},
	{"CM1_R to Right Mixer Negative Resistor", "20 kOhm", "CM_R"},
	{"CM1_R to Right Mixer Negative Resistor", "40 kOhm", "CM_R"},

	{"Right ADC", NULL, "IN1_L to Right Mixer Negative Resistor"},
	{"IN1_L to Right Mixer Negative Resistor", "10 kOhm", "IN1_L"},
	{"IN1_L to Right Mixer Negative Resistor", "20 kOhm", "IN1_L"},
	{"IN1_L to Right Mixer Negative Resistor", "40 kOhm", "IN1_L"},

	{"Right ADC", NULL, "IN3_L to Right Mixer Negative Resistor"},
	{"IN3_L to Right Mixer Negative Resistor", "10 kOhm", "IN3_L"},
	{"IN3_L to Right Mixer Negative Resistor", "20 kOhm", "IN3_L"},
	{"IN3_L to Right Mixer Negative Resistor", "40 kOhm", "IN3_L"},

	{"Right ADC", NULL, "CM2_R to Right Mixer Negative Resistor"},
	{"CM2_R to Right Mixer Negative Resistor", "10 kOhm", "CM_R"},
	{"CM2_R to Right Mixer Negative Resistor", "20 kOhm", "CM_R"},
	{"CM2_R to Right Mixer Negative Resistor", "40 kOhm", "CM_R"},
};

static const struct regmap_range_cfg aic32x4_regmap_pages[] = {
	{
		.selector_reg = 0,
		.selector_mask  = 0xff,
		.window_start = 0,
		.window_len = 128,
		.range_min = 0,
		.range_max = AIC32X4_RMICPGAVOL,
	},
};

const struct regmap_config aic32x4_regmap_config = {
	.max_register = AIC32X4_RMICPGAVOL,
	.ranges = aic32x4_regmap_pages,
	.num_ranges = ARRAY_SIZE(aic32x4_regmap_pages),
};
EXPORT_SYMBOL(aic32x4_regmap_config);

static inline int aic32x4_get_divs(int mclk, int rate)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(aic32x4_divs); i++) {
		if ((aic32x4_divs[i].rate == rate)
		    && (aic32x4_divs[i].mclk == mclk)) {
			return i;
		}
	}
	printk(KERN_ERR "aic32x4: master clock and sample rate is not supported\n");
	return -EINVAL;
}

static int aic32x4_set_dai_sysclk(struct snd_soc_dai *codec_dai,
				  int clk_id, unsigned int freq, int dir)
{
	struct snd_soc_component *component = codec_dai->component;
	struct aic32x4_priv *aic32x4 = snd_soc_component_get_drvdata(component);

	switch (freq) {
	case 12000000:
	case 24000000:
	case 24576000:
	case 25000000:
		aic32x4->sysclk = freq;
		return 0;
	}
	printk(KERN_ERR "aic32x4: invalid frequency to set DAI system clock\n");
	return -EINVAL;
}

static int aic32x4_set_dai_fmt(struct snd_soc_dai *codec_dai, unsigned int fmt)
{
	struct snd_soc_component *component = codec_dai->component;
	u8 iface_reg_1 = 0;
	u8 iface_reg_2 = 0;
	u8 iface_reg_3 = 0;

	/* set master/slave audio interface */
	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBM_CFM:
		iface_reg_1 |= AIC32X4_BCLKMASTER | AIC32X4_WCLKMASTER;
		break;
	case SND_SOC_DAIFMT_CBS_CFS:
		break;
	default:
		printk(KERN_ERR "aic32x4: invalid DAI master/slave interface\n");
		return -EINVAL;
	}

	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
		break;
	case SND_SOC_DAIFMT_DSP_A:
		iface_reg_1 |= (AIC32X4_DSP_MODE <<
				AIC32X4_IFACE1_DATATYPE_SHIFT);
		iface_reg_3 |= AIC32X4_BCLKINV_MASK; /* invert bit clock */
		iface_reg_2 = 0x01; /* add offset 1 */
		break;
	case SND_SOC_DAIFMT_DSP_B:
		iface_reg_1 |= (AIC32X4_DSP_MODE <<
				AIC32X4_IFACE1_DATATYPE_SHIFT);
		iface_reg_3 |= AIC32X4_BCLKINV_MASK; /* invert bit clock */
		break;
	case SND_SOC_DAIFMT_RIGHT_J:
		iface_reg_1 |= (AIC32X4_RIGHT_JUSTIFIED_MODE <<
				AIC32X4_IFACE1_DATATYPE_SHIFT);
		break;
	case SND_SOC_DAIFMT_LEFT_J:
		iface_reg_1 |= (AIC32X4_LEFT_JUSTIFIED_MODE <<
				AIC32X4_IFACE1_DATATYPE_SHIFT);
		break;
	default:
		printk(KERN_ERR "aic32x4: invalid DAI interface format\n");
		return -EINVAL;
	}

	snd_soc_component_update_bits(component, AIC32X4_IFACE1,
			    AIC32X4_IFACE1_DATATYPE_MASK |
			    AIC32X4_IFACE1_MASTER_MASK, iface_reg_1);
	snd_soc_component_update_bits(component, AIC32X4_IFACE2,
			    AIC32X4_DATA_OFFSET_MASK, iface_reg_2);
	snd_soc_component_update_bits(component, AIC32X4_IFACE3,
			    AIC32X4_BCLKINV_MASK, iface_reg_3);

	return 0;
}

// static int aic32x4_hw_params(struct snd_pcm_substream *substream,
// 			     struct snd_pcm_hw_params *params,
// 			     struct snd_soc_dai *dai)
// {
// 	struct snd_soc_component *component = dai->component;
// 	struct aic32x4_priv *aic32x4 = snd_soc_component_get_drvdata(component);
// 	u8 iface1_reg = 0;
// 	u8 dacsetup_reg = 0;
// 	int i;

// 	i = aic32x4_get_divs(aic32x4->sysclk, params_rate(params));
// 	if (i < 0) {
// 		printk(KERN_ERR "aic32x4: sampling rate not supported\n");
// 		return i;
// 	}

	
// #ifdef USE_PLL
// 	/* MCLK as PLL_CLKIN */
// 	snd_soc_component_update_bits(component, AIC32X4_CLKMUX, AIC32X4_PLL_CLKIN_MASK,
// 			    AIC32X4_PLL_CLKIN_MCLK << AIC32X4_PLL_CLKIN_SHIFT);
// 	// /* PLL as CODEC_CLKIN */
// 	/* Use BCLK as PLL input, PLL as CODEC_CLKIN and DAC_MOD_CLK as BDIV_CLKIN */
// 	// snd_soc_component_update_bits(component, AIC32X4_CLKMUX, AIC32X4_CODEC_CLKIN_MASK,
// 	// 		    AIC32X4_CODEC_CLKIN_PLL << AIC32X4_CODEC_CLKIN_SHIFT);

// 	/* DAC_MOD_CLK as BDIV_CLKIN */
// 	snd_soc_component_update_bits(component, AIC32X4_IFACE3, AIC32X4_BDIVCLK_MASK,
// 			    AIC32X4_DACMOD2BCLK << AIC32X4_BDIVCLK_SHIFT);

// #else /* USE_PLL */
// 	snd_soc_component_update_bits(component, AIC32X4_CLKMUX, AIC32X4_PLL_CLKIN_MASK,
// 			    AIC32X4_PLL_CLKIN_MCLK << AIC32X4_PLL_CLKIN_SHIFT);
// #endif/* USE_PLL */

// #ifdef USE_PLL

// 	/* We will fix R value to 1 and will make P & J=K.D as variable */
// 	snd_soc_component_update_bits(component, AIC32X4_PLLPR, AIC32X4_PLL_R_MASK, 0x01);

// 	/* PLL P value */
// 	snd_soc_component_update_bits(component, AIC32X4_PLLPR, AIC32X4_PLL_P_MASK,
// 			    aic32x4_divs[i].p_val << AIC32X4_PLL_P_SHIFT);

// 	/* PLL J value */
// 	snd_soc_component_write(component, AIC32X4_PLLJ, aic32x4_divs[i].pll_j);

// 	/* PLL D value */
// 	snd_soc_component_write(component, AIC32X4_PLLDMSB, (aic32x4_divs[i].pll_d >> 8));
// 	snd_soc_component_write(component, AIC32X4_PLLDLSB, (aic32x4_divs[i].pll_d & 0xff));

// #endif /* USE_PLL */

// 	/* NDAC divider value */
// 	snd_soc_component_update_bits(component, AIC32X4_NDAC,
// 			    AIC32X4_NDAC_MASK, aic32x4_divs[i].ndac);
// 	printk(KERN_ERR "aic32x4: i=%i, mdac= 0x%02X\n", i, aic32x4_divs[i].mdac);

// 	/* MDAC divider value */
// 	snd_soc_component_update_bits(component, AIC32X4_MDAC,
// 			    AIC32X4_MDAC_MASK, aic32x4_divs[i].mdac);

// 	/* DOSR MSB & LSB values */
// 		(component, AIC32X4_DOSRMSB, aic32x4_divs[i].dosr >> 8);
// 	snd_soc_component_write(component, AIC32X4_DOSRLSB, (aic32x4_divs[i].dosr & 0xff));

// 	/* NADC divider value */
// 	snd_soc_component_update_bits(component, AIC32X4_NADC,
// 			    AIC32X4_NADC_MASK, aic32x4_divs[i].nadc);

// 	/* MADC divider value */
// 	snd_soc_component_update_bits(component, AIC32X4_MADC,
// 			    AIC32X4_MADC_MASK, aic32x4_divs[i].madc);

// 	/* AOSR value */
// 	snd_soc_component_write(component, AIC32X4_AOSR, aic32x4_divs[i].aosr);

// #ifdef USE_PLL

// 	/* BCLK N divider */
// 	snd_soc_component_update_bits(component, AIC32X4_BCLKN,
// 			    AIC32X4_BCLK_MASK, aic32x4_divs[i].blck_N);

// #endif /* USE_PLL */

// 	switch (params_width(params)) {
// 	case 16:
// 		iface1_reg |= (AIC32X4_WORD_LEN_16BITS <<
// 			       AIC32X4_IFACE1_DATALEN_SHIFT);
// 		break;
// 	case 20:
// 		iface1_reg |= (AIC32X4_WORD_LEN_20BITS <<
// 			       AIC32X4_IFACE1_DATALEN_SHIFT);
// 		break;
// 	case 24:
// 		iface1_reg |= (AIC32X4_WORD_LEN_24BITS <<
// 			       AIC32X4_IFACE1_DATALEN_SHIFT);
// 		break;
// 	case 32:
// 		iface1_reg |= (AIC32X4_WORD_LEN_32BITS <<
// 			       AIC32X4_IFACE1_DATALEN_SHIFT);
// 		break;
// 	}
// 	snd_soc_component_update_bits(component, AIC32X4_IFACE1,
// 			    AIC32X4_IFACE1_DATALEN_MASK, iface1_reg);

// 	if (params_channels(params) == 1) {
// 		dacsetup_reg = AIC32X4_RDAC2LCHN | AIC32X4_LDAC2LCHN;
// 	} else {
// 		if (aic32x4->swapdacs)
// 			dacsetup_reg = AIC32X4_RDAC2LCHN | AIC32X4_LDAC2RCHN;
// 		else
// 			dacsetup_reg = AIC32X4_LDAC2LCHN | AIC32X4_RDAC2RCHN;
// 	}
// 	snd_soc_component_update_bits(component, AIC32X4_DACSETUP,
// 			    AIC32X4_DAC_CHAN_MASK, dacsetup_reg);

// 	return 0;
// }

static int aic32x4_hw_params(struct snd_pcm_substream *substream,
			     struct snd_pcm_hw_params *params,
			     struct snd_soc_dai *dai)
{
	struct snd_soc_component *component = dai->component;
	struct aic32x4_priv *aic32x4 = snd_soc_component_get_drvdata(component);
	unsigned int data;
	int i;
	int ret;

	i = aic32x4_get_divs(aic32x4->sysclk, params_rate(params));
	if (i < 0) {
		printk(KERN_ERR "aic32x4: sampling rate not supported\n");
		return i;
	}


#ifdef USE_PLL
    /* Use PLL as CODEC_CLKIN and DAC_MOD_CLK as BDIV_CLKIN */
	snd_soc_component_write(component, AIC32X4_CLKMUX, AIC32X4_PLLCLKIN);
   	/* Use BCLK as PLL input, PLL as CODEC_CLKIN and DAC_MOD_CLK as BDIV_CLKIN */
	//snd_soc_write(component, AIC32X4_CLKMUX, AIC32X4_PLLCLKIN | AIC32X4_BCLKINTOPLL | AIC32X4_PLLHIGHRANGE);
	snd_soc_component_write(component, AIC32X4_IFACE3, AIC32X4_DACMOD2BCLK);
#else /* USE_PLL */
    /* Use MCLK as CODEC_CLKIN  */
    snd_soc_write(component, AIC32X4_CLKMUX, AIC32X4_MCLKCLKIN);
#endif /* USE_PLL */

#ifdef USE_PLL
	/* We will fix R value to 1 and will make P & J=K.D as varialble */
	ret = snd_soc_component_read(component, AIC32X4_PLLPR, &data);
	data &= ~(7 << 4);
	snd_soc_component_write(component, AIC32X4_PLLPR,
		      (data | (aic32x4_divs[i].p_val << 4) | 0x01));

	snd_soc_component_write(component, AIC32X4_PLLJ, aic32x4_divs[i].pll_j);

	snd_soc_component_write(component, AIC32X4_PLLDMSB, (aic32x4_divs[i].pll_d >> 8));
	snd_soc_component_write(component, AIC32X4_PLLDLSB,
		      (aic32x4_divs[i].pll_d & 0xff));
#endif /* USE_PLL */

	/* NDAC divider value */
	ret = snd_soc_component_read(component, AIC32X4_NDAC, &data);
	data &= ~(0x7f);
	snd_soc_component_write(component, AIC32X4_NDAC, data | aic32x4_divs[i].ndac);

    printk(KERN_ERR "aic32x4: i=%i, mdac= 0x%02X\n", i, aic32x4_divs[i].mdac);
	/* MDAC divider value */
	ret = snd_soc_component_read(component, AIC32X4_MDAC, &data);
	data &= ~(0x7f);
	snd_soc_component_write(component, AIC32X4_MDAC, data | aic32x4_divs[i].mdac);

	/* DOSR MSB & LSB values */
	snd_soc_component_write(component, AIC32X4_DOSRMSB, aic32x4_divs[i].dosr >> 8);
	snd_soc_component_write(component, AIC32X4_DOSRLSB,
		      (aic32x4_divs[i].dosr & 0xff));

	/* NADC divider value */
	ret = snd_soc_component_read(component, AIC32X4_NADC, &data);
	data &= ~(0x7f);
	snd_soc_component_write(component, AIC32X4_NADC, data | aic32x4_divs[i].nadc);

	/* MADC divider value */
	ret = snd_soc_component_read(component, AIC32X4_MADC, &data);
	data &= ~(0x7f);
	snd_soc_component_write(component, AIC32X4_MADC, data | aic32x4_divs[i].madc);

	/* AOSR value */
	snd_soc_component_write(component, AIC32X4_AOSR, aic32x4_divs[i].aosr);

#ifdef USE_PLL
	/* BCLK N divider */
	ret = snd_soc_component_read(component, AIC32X4_BCLKN, &data);
	data &= ~(0x7f);
	snd_soc_component_write(component, AIC32X4_BCLKN, data | aic32x4_divs[i].blck_N);
#endif /* USE_PLL */

	ret = snd_soc_component_read(component, AIC32X4_IFACE1, &data);
	data = data & ~(3 << 4);
	switch (params_width(params)) {
	case 16:
		break;
	case 20:
		data |= (AIC32X4_WORD_LEN_20BITS << AIC32X4_DOSRMSB_SHIFT);
		break;
	case 24:
		data |= (AIC32X4_WORD_LEN_24BITS << AIC32X4_DOSRMSB_SHIFT);
		break;
	case 32:
		data |= (AIC32X4_WORD_LEN_32BITS << AIC32X4_DOSRMSB_SHIFT);
		break;
	}
	snd_soc_component_write(component, AIC32X4_IFACE1, data);

	if (params_channels(params) == 1) {
		data = AIC32X4_RDAC2LCHN | AIC32X4_LDAC2LCHN;
	} else {
		if (aic32x4->swapdacs)
			data = AIC32X4_RDAC2LCHN | AIC32X4_LDAC2RCHN;
		else
			data = AIC32X4_LDAC2LCHN | AIC32X4_RDAC2RCHN;
	}
	snd_soc_component_update_bits(component, AIC32X4_DACSETUP, AIC32X4_DAC_CHAN_MASK,
			data);

	return 0;
}


static int aic32x4_mute(struct snd_soc_dai *dai, int mute)
{
	struct snd_soc_component *component = dai->component;

	snd_soc_component_update_bits(component, AIC32X4_DACMUTE,
			    AIC32X4_MUTEON, mute ? AIC32X4_MUTEON : 0);

	return 0;
}

static int aic32x4_set_bias_level(struct snd_soc_component *component,
				  enum snd_soc_bias_level level)
{
	struct aic32x4_priv *aic32x4 = snd_soc_component_get_drvdata(component);
	int ret;

	switch (level) {
	case SND_SOC_BIAS_ON:
		/* Switch on master clock */
		if (aic32x4->mclk_count == 0) {
            dev_err(component->dev, "Preparing clock\n");
            ret = clk_prepare_enable(aic32x4->mclk);
            if (ret) {
                dev_err(component->dev, "Failed to enable master clock\n");
                return ret;
            }
            aic32x4->mclk_count = aic32x4->mclk_count + 1;
		}

#ifdef USE_PLL
		/* Switch on PLL */
		snd_soc_component_update_bits(component, AIC32X4_PLLPR,
				    AIC32X4_PLLEN, AIC32X4_PLLEN);
#endif /* USE_PLL */

		/* Switch on NDAC Divider */
		snd_soc_component_update_bits(component, AIC32X4_NDAC,
				    AIC32X4_NDACEN, AIC32X4_NDACEN);

		/* Switch on MDAC Divider */
		snd_soc_component_update_bits(component, AIC32X4_MDAC,
				    AIC32X4_MDACEN, AIC32X4_MDACEN);

		/* Switch on NADC Divider */
		snd_soc_component_update_bits(component, AIC32X4_NADC,
				    AIC32X4_NADCEN, AIC32X4_NADCEN);

		/* Switch on MADC Divider */
		snd_soc_component_update_bits(component, AIC32X4_MADC,
				    AIC32X4_MADCEN, AIC32X4_MADCEN);
#ifdef USE_PLL
		/* Switch on BCLK_N Divider */
		snd_soc_component_update_bits(component, AIC32X4_BCLKN,
				    AIC32X4_BCLKEN, AIC32X4_BCLKEN);
#endif /* USE_PLL */

		break;
	case SND_SOC_BIAS_PREPARE:
		break;
	case SND_SOC_BIAS_STANDBY:
		/* Initial cold start */
		if (snd_soc_component_get_bias_level(component) == SND_SOC_BIAS_OFF)
			break;

		/* Switch off BCLK_N Divider */
		snd_soc_component_update_bits(component, AIC32X4_BCLKN,
				    AIC32X4_BCLKEN, 0);

		/* Switch off MADC Divider */
		snd_soc_component_update_bits(component, AIC32X4_MADC,
				    AIC32X4_MADCEN, 0);

		/* Switch off NADC Divider */
		snd_soc_component_update_bits(component, AIC32X4_NADC,
				    AIC32X4_NADCEN, 0);

		/* Switch off MDAC Divider */
		snd_soc_component_update_bits(component, AIC32X4_MDAC,
				    AIC32X4_MDACEN, 0);

		/* Switch off NDAC Divider */
		snd_soc_component_update_bits(component, AIC32X4_NDAC,
				    AIC32X4_NDACEN, 0);
#ifdef USE_PLL
		/* Switch off PLL */
		snd_soc_component_update_bits(component, AIC32X4_PLLPR,
				    AIC32X4_PLLEN, 0);
#endif /* USE_PLL */

		/* Switch off master clock */
		/*
		// clock is unprepared doesn't restart anymore
		if (aic32x4->mclk_count > 0) {
			dev_err(component->dev, "Unpreparing clock\n");
			clk_disable_unprepare(aic32x4->mclk);
			aic32x4->mclk_count = aic32x4->mclk_count - 1;
		}
		*/

		break;
	case SND_SOC_BIAS_OFF:
		break;
	}
	return 0;
}

#define AIC32X4_RATES	SNDRV_PCM_RATE_8000_96000
#define AIC32X4_FORMATS	(SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S20_3LE \
			 | SNDRV_PCM_FMTBIT_S24_3LE | SNDRV_PCM_FMTBIT_S32_LE)

static const struct snd_soc_dai_ops aic32x4_ops = {
	.hw_params = aic32x4_hw_params,
	.digital_mute = aic32x4_mute,
	.set_fmt = aic32x4_set_dai_fmt,
	.set_sysclk = aic32x4_set_dai_sysclk,
};

static struct snd_soc_dai_driver aic32x4_dai = {
	.name = "tlv320aic32x4-hifi",
	.playback = {
		     .stream_name = "Playback",
		     .channels_min = 1,
		     .channels_max = 2,
		     .rates = AIC32X4_RATES,
		     .formats = AIC32X4_FORMATS,},
	.capture = {
		    .stream_name = "Capture",
		    .channels_min = 1,
		    .channels_max = 2,
		    .rates = AIC32X4_RATES,
		    .formats = AIC32X4_FORMATS,},
	.ops = &aic32x4_ops,
	.symmetric_rates = 1,
};

static int aic32x4_component_probe(struct snd_soc_component *component)
{
	struct aic32x4_priv *aic32x4 = snd_soc_component_get_drvdata(component);
	u32 tmp_reg;

	if (gpio_is_valid(aic32x4->rstn_gpio)) {
		ndelay(10);
		gpio_set_value(aic32x4->rstn_gpio, 1);
	}

	snd_soc_component_write(component, AIC32X4_RESET, 0x01);

	/* Power platform configuration */
	if (aic32x4->power_cfg & AIC32X4_PWR_MICBIAS_2075_LDOIN) {
		snd_soc_component_write(component, AIC32X4_MICBIAS, AIC32X4_MICBIAS_LDOIN |
						      AIC32X4_MICBIAS_2075V);
	}
	if (aic32x4->power_cfg & AIC32X4_PWR_AVDD_DVDD_WEAK_DISABLE)
		snd_soc_component_write(component, AIC32X4_PWRCFG, AIC32X4_AVDDWEAKDISABLE);

	tmp_reg = (aic32x4->power_cfg & AIC32X4_PWR_AIC32X4_LDO_ENABLE) ?
			AIC32X4_LDOCTLEN : 0;
	snd_soc_component_write(component, AIC32X4_LDOCTL, tmp_reg);

	tmp_reg = snd_soc_component_read32(component, AIC32X4_CMMODE);
	if (aic32x4->power_cfg & AIC32X4_PWR_CMMODE_LDOIN_RANGE_18_36)
		tmp_reg |= AIC32X4_LDOIN_18_36;
	if (aic32x4->power_cfg & AIC32X4_PWR_CMMODE_HP_LDOIN_POWERED)
		tmp_reg |= AIC32X4_LDOIN2HP;
	snd_soc_component_write(component, AIC32X4_CMMODE, tmp_reg);

	/* Mic PGA routing */
	if (aic32x4->micpga_routing & AIC32X4_MICPGA_ROUTE_LMIC_IN2R_10K)
		snd_soc_component_write(component, AIC32X4_LMICPGANIN,
				AIC32X4_LMICPGANIN_IN2R_10K);
	else
		snd_soc_component_write(component, AIC32X4_LMICPGANIN,
				AIC32X4_LMICPGANIN_CM1L_10K);
	if (aic32x4->micpga_routing & AIC32X4_MICPGA_ROUTE_RMIC_IN1L_10K)
		snd_soc_component_write(component, AIC32X4_RMICPGANIN,
				AIC32X4_RMICPGANIN_IN1L_10K);
	else
		snd_soc_component_write(component, AIC32X4_RMICPGANIN,
				AIC32X4_RMICPGANIN_CM1R_10K);

	/*
	 * Workaround: for an unknown reason, the ADC needs to be powered up
	 * and down for the first capture to work properly. It seems related to
	 * a HW BUG or some kind of behavior not documented in the datasheet.
	 */
	tmp_reg = snd_soc_component_read32(component, AIC32X4_ADCSETUP);
	snd_soc_component_write(component, AIC32X4_ADCSETUP, tmp_reg |
				AIC32X4_LADC_EN | AIC32X4_RADC_EN);
	snd_soc_component_write(component, AIC32X4_ADCSETUP, tmp_reg);
	/* DEFAULT ROUTING OUTPUT ON LINE OUT L/R (For example on C04 BOARD) */
	if (aic32x4->lineout_routing ) {
		printk(KERN_ERR "tlv320aic32x4: Default routing in Line OUT\n");
		snd_soc_component_write(component, AIC32X4_LOLROUTE, 0x08);
		snd_soc_component_write(component, AIC32X4_LORROUTE, 0x08);
		snd_soc_component_write(component, AIC32X4_LOLGAIN, 0x3a);
		snd_soc_component_write(component, AIC32X4_LORGAIN, 0x3a);
	}


	return 0;
}

static struct snd_soc_component_driver soc_component_dev_aic32x4 = {
	.probe			= aic32x4_component_probe,
	.set_bias_level		= aic32x4_set_bias_level,
	.controls		= aic32x4_snd_controls,
	.num_controls		= ARRAY_SIZE(aic32x4_snd_controls),
	.dapm_widgets		= aic32x4_dapm_widgets,
	.num_dapm_widgets	= ARRAY_SIZE(aic32x4_dapm_widgets),
	.dapm_routes		= aic32x4_dapm_routes,
	.num_dapm_routes	= ARRAY_SIZE(aic32x4_dapm_routes),
	.suspend_bias_off	= 1,
	.idle_bias_on		= 1,
	.use_pmdown_time	= 1,
	.endianness		= 1,
	.non_legacy_dai_naming	= 1,
	.reg_cache_size = 256,
	.reg_word_size = sizeof(u8),
};

static int aic32x4_parse_dt(struct aic32x4_priv *aic32x4,
		struct device_node *np)
{
	aic32x4->swapdacs = false;
	aic32x4->micpga_routing = 0;
	aic32x4->rstn_gpio = of_get_named_gpio(np, "reset-gpios", 0);
	aic32x4->enamp_gpio = of_get_named_gpio(np, "enable-amp", 0);
	aic32x4->enmic_gpio = of_get_named_gpio(np, "enable-mic", 0);
	aic32x4->lineout_routing = of_property_read_bool(np, "route-on-lineout");
	
	return 0;
}

static void aic32x4_disable_regulators(struct aic32x4_priv *aic32x4)
{
	regulator_disable(aic32x4->supply_iov);

	if (!IS_ERR(aic32x4->supply_ldo))
		regulator_disable(aic32x4->supply_ldo);

	if (!IS_ERR(aic32x4->supply_dv))
		regulator_disable(aic32x4->supply_dv);

	if (!IS_ERR(aic32x4->supply_av))
		regulator_disable(aic32x4->supply_av);
}

static int aic32x4_setup_regulators(struct device *dev,
		struct aic32x4_priv *aic32x4)
{
	int ret = 0;

	aic32x4->supply_ldo = devm_regulator_get_optional(dev, "ldoin");
	aic32x4->supply_iov = devm_regulator_get(dev, "iov");
	aic32x4->supply_dv = devm_regulator_get_optional(dev, "dv");
	aic32x4->supply_av = devm_regulator_get_optional(dev, "av");

	/* Check if the regulator requirements are fulfilled */

	if (IS_ERR(aic32x4->supply_iov)) {
		dev_err(dev, "Missing supply 'iov'\n");
		return PTR_ERR(aic32x4->supply_iov);
	}

	if (IS_ERR(aic32x4->supply_ldo)) {
		if (PTR_ERR(aic32x4->supply_ldo) == -EPROBE_DEFER)
			return -EPROBE_DEFER;

		if (IS_ERR(aic32x4->supply_dv)) {
			dev_err(dev, "Missing supply 'dv' or 'ldoin'\n");
			return PTR_ERR(aic32x4->supply_dv);
		}
		if (IS_ERR(aic32x4->supply_av)) {
			dev_err(dev, "Missing supply 'av' or 'ldoin'\n");
			return PTR_ERR(aic32x4->supply_av);
		}
	} else {
		if (IS_ERR(aic32x4->supply_dv) &&
				PTR_ERR(aic32x4->supply_dv) == -EPROBE_DEFER)
			return -EPROBE_DEFER;
		if (IS_ERR(aic32x4->supply_av) &&
				PTR_ERR(aic32x4->supply_av) == -EPROBE_DEFER)
			return -EPROBE_DEFER;
	}

	ret = regulator_enable(aic32x4->supply_iov);
	if (ret) {
		dev_err(dev, "Failed to enable regulator iov\n");
		return ret;
	}

	if (!IS_ERR(aic32x4->supply_ldo)) {
		ret = regulator_enable(aic32x4->supply_ldo);
		if (ret) {
			dev_err(dev, "Failed to enable regulator ldo\n");
			goto error_ldo;
		}
	}

	if (!IS_ERR(aic32x4->supply_dv)) {
		ret = regulator_enable(aic32x4->supply_dv);
		if (ret) {
			dev_err(dev, "Failed to enable regulator dv\n");
			goto error_dv;
		}
	}

	if (!IS_ERR(aic32x4->supply_av)) {
		ret = regulator_enable(aic32x4->supply_av);
		if (ret) {
			dev_err(dev, "Failed to enable regulator av\n");
			goto error_av;
		}
	}

	if (!IS_ERR(aic32x4->supply_ldo) && IS_ERR(aic32x4->supply_av))
		aic32x4->power_cfg |= AIC32X4_PWR_AIC32X4_LDO_ENABLE;

	return 0;

error_av:
	if (!IS_ERR(aic32x4->supply_dv))
		regulator_disable(aic32x4->supply_dv);

error_dv:
	if (!IS_ERR(aic32x4->supply_ldo))
		regulator_disable(aic32x4->supply_ldo);

error_ldo:
	regulator_disable(aic32x4->supply_iov);
	return ret;
}

int aic32x4_probe(struct device *dev, struct regmap *regmap)
{
	struct aic32x4_priv *aic32x4;
	struct aic32x4_pdata *pdata = dev->platform_data;
	struct device_node *np = dev->of_node;
	int ret;

	if (IS_ERR(regmap))
		return PTR_ERR(regmap);

	aic32x4 = devm_kzalloc(dev, sizeof(struct aic32x4_priv),
			       GFP_KERNEL);
	if (aic32x4 == NULL)
		return -ENOMEM;

	dev_set_drvdata(dev, aic32x4);

	if (pdata) {
		aic32x4->power_cfg = pdata->power_cfg;
		aic32x4->swapdacs = pdata->swapdacs;
		aic32x4->micpga_routing = pdata->micpga_routing;
		aic32x4->rstn_gpio = pdata->rstn_gpio;
	} else if (np) {
		ret = aic32x4_parse_dt(aic32x4, np);
		if (ret) {
			dev_err(dev, "Failed to parse DT node\n");
			return ret;
		}
	} else {
		aic32x4->power_cfg = 0;
		aic32x4->swapdacs = false;
		aic32x4->micpga_routing = 0;
		aic32x4->rstn_gpio = -1;
	}

	aic32x4->mclk = devm_clk_get(dev, "mclk");
	if (IS_ERR(aic32x4->mclk)) {
		dev_err(dev, "Failed getting the mclk. The current implementation does not support the usage of this codec without mclk\n");
		return PTR_ERR(aic32x4->mclk);
	}
	aic32x4->mclk_count = 0;

	if (gpio_is_valid(aic32x4->rstn_gpio)) {
		ret = devm_gpio_request_one(dev, aic32x4->rstn_gpio,
				GPIOF_OUT_INIT_LOW, "tlv320aic32x4 rstn");
		if (ret != 0)
			return ret;
	}

	ret = aic32x4_setup_regulators(dev, aic32x4);
	if (ret) {
		dev_err(dev, "Failed to setup regulators\n");
		return ret;
	}

	ret = devm_snd_soc_register_component(dev,
			&soc_component_dev_aic32x4, &aic32x4_dai, 1);
	if (ret) {
		dev_err(dev, "Failed to register component\n");
		aic32x4_disable_regulators(aic32x4);
		return ret;
	}

	return 0;
}
EXPORT_SYMBOL(aic32x4_probe);

int aic32x4_remove(struct device *dev)
{
	struct aic32x4_priv *aic32x4 = dev_get_drvdata(dev);

	aic32x4_disable_regulators(aic32x4);

	return 0;
}
EXPORT_SYMBOL(aic32x4_remove);

MODULE_DESCRIPTION("ASoC tlv320aic32x4 codec driver");
MODULE_AUTHOR("Javier Martin <javier.martin@vista-silicon.com>");
MODULE_LICENSE("GPL");

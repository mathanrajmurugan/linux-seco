/*
 * Analog Devices SN65DSI84 HDMI transmitter driver
 *
 * Copyright 2012 Analog Devices Inc.
 *
 * Licensed under the GPL-2.
 */

#include <linux/device.h>
#include <linux/gpio/consumer.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/slab.h>

#include <drm/drmP.h>
#include <drm/drm_atomic.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_dp_helper.h>
#include <drm/drm_mipi_dsi.h>
#include <drm/drm_of.h>
#include <drm/drm_panel.h>
#include <linux/clk.h>
#include <linux/of_graph.h>
#include <linux/of_gpio.h>
#include <linux/pm_runtime.h>
#include <video/videomode.h>

#include "sn65dsi84.h"


/* -----------------------------------------------------------------------------
 * Register access
 */

static const uint8_t sn65dsi84_register_defaults[] = { };

static const struct regmap_range sn65dsi84_bridge_volatile_ranges[] = {
        { .range_min = 0, .range_max = 0xFF },
};

static const struct regmap_access_table sn65dsi84_bridge_volatile_table = {
        .yes_ranges = sn65dsi84_bridge_volatile_ranges,
        .n_yes_ranges = ARRAY_SIZE(sn65dsi84_bridge_volatile_ranges),
};

static const struct regmap_config sn65dsi84_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
	.volatile_table = &sn65dsi84_bridge_volatile_table,
	.cache_type = REGCACHE_NONE,
};

static int __maybe_unused sn65dsi84_bridge_resume(struct device *dev)
{
        struct sn65dsi84 *pdata = dev_get_drvdata(dev);

        gpiod_set_value(pdata->pd_gpio, 1);

        return 0;
}

static int __maybe_unused sn65dsi84_bridge_suspend(struct device *dev)
{
        struct sn65dsi84 *pdata = dev_get_drvdata(dev);

        gpiod_set_value(pdata->pd_gpio, 0);

        return 0;
}

static const struct dev_pm_ops sn65dsi84_bridge_pm_ops = {
        SET_RUNTIME_PM_OPS(sn65dsi84_bridge_suspend, sn65dsi84_bridge_resume, NULL)
};

static bool sn65dsi84_videomode_parse_dt(struct device_node *np, struct sn65dsi84 *adv)
{

	if(of_property_read_string(np, "lvds,datamap", &(adv->lvds_datamap))) adv->lvds_datamap = "jeida";
	adv->lvds_dual_channel = of_property_read_bool(np, "lvds,dual-channel");
	adv->lvds_channel_reverse = of_property_read_bool(np, "lvds,channel-reverse");
	adv->lvds_channel_swap = of_property_read_bool(np, "lvds,channel-swap");
	adv->lvds_test_mode = of_property_read_bool(np, "lvds,test-mode");
	adv->lvds_preserve_dsi_timings = of_property_read_bool(np, "lvds,preserve-dsi-timings");
	if(of_property_read_u32(np, "dsi,mode-flags", &(adv->mode_flags)))  adv->mode_flags = MIPI_DSI_MODE_VIDEO;
	return true;
}

static void sn65dsi84_power_on(struct sn65dsi84 *sn65dsi84)
{

	if (sn65dsi84->pd_gpio)
                gpiod_set_value(sn65dsi84->pd_gpio, 1);

        return;

}

static void sn65dsi84_power_off(struct sn65dsi84 *sn65dsi84)
{
	if (sn65dsi84->pd_gpio)
                gpiod_set_value(sn65dsi84->pd_gpio, 0);

	return;
}

/* Connector funcs */
static struct sn65dsi84 *connector_to_sn65dsi84(struct drm_connector *connector)
{
	return container_of(connector, struct sn65dsi84, connector);
}

static int sn65dsi84_connector_get_modes(struct drm_connector *connector)
{
	struct sn65dsi84 *pdata = connector_to_sn65dsi84(connector);
	DRM_DEBUG("sn65dsi84_connector_get_modes\n");
	return drm_panel_get_modes(pdata->panel);
}

static enum drm_mode_status
sn65dsi84_connector_mode_valid(struct drm_connector *connector,
			     struct drm_display_mode *mode)
{
        DRM_DEBUG("sn65dsi84_mode_valid\n");
	/* maximum supported resolution is 1080 at 60 fps */
        if (mode->clock > 165000)
                return MODE_CLOCK_HIGH;

        return MODE_OK;

}

static struct drm_connector_helper_funcs sn65dsi84_connector_helper_funcs = {
	.get_modes = sn65dsi84_connector_get_modes,
	.mode_valid = sn65dsi84_connector_mode_valid,
};

static enum drm_connector_status
sn65dsi84_connector_detect(struct drm_connector *connector, bool force)
{
	/**
         * TODO: Currently if drm_panel is present, then always
         * return the status as connected. Need to add support to detect
         * device state for hot pluggable scenarios.
         */
        return connector_status_connected;
}

static struct drm_connector_funcs sn65dsi84_connector_funcs = {
	//.dpms = drm_atomic_helper_connector_dpms,
	.fill_modes = drm_helper_probe_single_connector_modes,
	.detect = sn65dsi84_connector_detect,
	.destroy = drm_connector_cleanup,
	.reset = drm_atomic_helper_connector_reset,
	.atomic_duplicate_state = drm_atomic_helper_connector_duplicate_state,
	.atomic_destroy_state = drm_atomic_helper_connector_destroy_state,
};

/* Bridge funcs */
static struct sn65dsi84 *bridge_to_sn65dsi84(struct drm_bridge *bridge)
{
	return container_of(bridge, struct sn65dsi84, bridge);
}


static void sn65dsi84_calculate_clk_div_mul( u8 *clk_div_mul, unsigned int dsi_clk, unsigned int pixclk )
{
	int i;
	if(pixclk == 0) {
		DRM_INFO("sn65dsi84_calculate_clk_div_mul: pixclk is 0, can't divide by zero\n");
		return;
	}
	
	i = (dsi_clk + (pixclk/2)) / ((pixclk));

        if( i >= 0 && i <= 26 )
                *clk_div_mul = ((i-1)<<3);
	return;	
}

static void sn65dsi84_bridge_enable(struct drm_bridge *bridge)
{
	struct sn65dsi84 *pdata = bridge_to_sn65dsi84(bridge);
        struct drm_display_mode *mode =
                &pdata->bridge.encoder->crtc->state->adjusted_mode;
        u8 hsync_polarity = 0, vsync_polarity = 0;
	u32 refclk_rate = 0;
	int refclk_multiplier = 0;
	u8 val = 0;

	unsigned int dsi_clock = pdata->refclk_rate; 

	DRM_INFO("sn65dsi84_bridge_enable configuring bridge dsi clock = %dHz\n",dsi_clock);

	/* Enable the chip power */
	sn65dsi84_power_on(pdata);
	/* PLL Enable - Stop PLL */
	regmap_write(pdata->regmap, SN65DSI84_REG_PLL_EN, 0x0);
	if(!pdata->refclk) {

		if( mode->clock >= 37500  	&& mode->clock < 62500) val |= (0x01 << 1);
		else if(mode->clock >= 62500	&& mode->clock < 87500 ) val |= (0x02 << 1);
		else if(mode->clock >= 87500  && (mode->clock) < 112500 ) val |= (0x03 << 1);
		else if(mode->clock >= 112500 && (mode->clock) < 137500 ) val |= (0x04 << 1);
		else val |= (0x05 << 1);
	
		/* LVDS pixel clock derived from MIPI D-PHY channel A HS continuous */
		val |= 0x1;	 	

		regmap_write(pdata->regmap,SN65DSI84_REG_LVDS_CLK, val);

		val = 0;
		/* Calculate Multiplier and Divider for generate LVDS Clock from DSI Clock */
		if(pdata->lvds_dual_channel)
			sn65dsi84_calculate_clk_div_mul(&val,dsi_clock, (mode->clock)*1000/2);
		else
			sn65dsi84_calculate_clk_div_mul(&val,dsi_clock, (mode->clock)*1000);

	} else {
		DRM_INFO("configuring for external oscillator\n");
		refclk_rate = clk_get_rate(pdata->refclk);
		if(pdata->lvds_dual_channel)
			refclk_multiplier = DIV_ROUND_UP((mode->clock*1000)/2, refclk_rate);
		else
			refclk_multiplier = DIV_ROUND_UP((mode->clock*1000), refclk_rate);

		if( refclk_multiplier >= 1 && refclk_multiplier <= 4 )
			val = ( refclk_multiplier - 1 );
		else if (refclk_multiplier < 1 ) {
			DRM_ERROR("refclk_multiplier can't be setted, it's value is < 0, set min value = 0\n");
			val = 0x0;
		} else if(refclk_multiplier > 4) {
			DRM_ERROR("refclk_multiplier can't be setted, it's value is > 4, set max value = 4\n");
			val = 0x3;
		}
			
		
	}

	regmap_write(pdata->regmap,SN65DSI84_REG_CLK_DIV_MUL, val);	
	
	/* Enable DSI Lanes */
	if(pdata->dsi->lanes == 1) regmap_write(pdata->regmap, SN65DSI84_REG_DSI_LANES, (0x1 << 5) | (0x3 << 3));
	if(pdata->dsi->lanes == 2) regmap_write(pdata->regmap, SN65DSI84_REG_DSI_LANES, (0x1 << 5) | (0x2 << 3));
	if(pdata->dsi->lanes == 3) regmap_write(pdata->regmap, SN65DSI84_REG_DSI_LANES, (0x1 << 5) | (0x1 << 3));
	if(pdata->dsi->lanes == 4) regmap_write(pdata->regmap, SN65DSI84_REG_DSI_LANES, (0x1 << 5) | (0x0 << 3));
		
	/* DSI equalization */
	regmap_write(pdata->regmap,SN65DSI84_REG_DSI_EQ, 0x00);

	val = 0;

	/* set LVDS for single channel, 24 bit mode, HS/VS low, DE high */
	
	if (pdata->connector.display_info.bpc <= 6) {
		DRM_INFO("LVDS color depth RGB18\n");
	} else {
		DRM_INFO("LVDS color depth RGB24\n");
		val |= 0x3 << 2;
	}

	if(!strcmp("spwg",pdata->lvds_datamap)) 
                val |= 0x3;
        else
                DRM_INFO("LVDS data mapping jeida\n");
	
	if(pdata->lvds_dual_channel)
		val |= 0x0;
	else
		val |= (0x1 << 4);
	
	/* VS_NEG_POLARITY HS_NEG_POLARITY */
	val |= (0x3 << 5);
	
	regmap_write(pdata->regmap, SN65DSI84_REG_LVDS_PARAMETER,val);

	/*Channel Swap and Reverse options */
	val = 0;
	if(pdata->lvds_channel_reverse)
		val |= (0x3 << 4);		
	if(pdata->lvds_channel_swap)   
                val |= (0x1 << 6);
	val |= 0x3; /* set termination to 200ohm (default) */
	regmap_write(pdata->regmap, SN65DSI84_REG_LVDS_SWAP,val);

	/* X resolution high/low */
	regmap_write(pdata->regmap, SN65DSI84_REG_CHA_ACTIVE_LINE_LENGTH_LOW, 	mode->hdisplay & 0x00ff );
	regmap_write(pdata->regmap, SN65DSI84_REG_CHA_ACTIVE_LINE_LENGTH_HIGH, (mode->hdisplay & 0xff00 )>>8 );
	/* Y resolution high/low */
	regmap_write(pdata->regmap, SN65DSI84_REG_CHA_VERTICAL_DISPLAY_SIZE_LOW,  mode->vdisplay & 0x00ff );
	regmap_write(pdata->regmap, SN65DSI84_REG_CHA_VERTICAL_DISPLAY_SIZE_HIGH,(mode->vdisplay & 0xff00 )>>8);

	/* SYNC delay high/low */
	regmap_write(pdata->regmap, SN65DSI84_REG_CHA_SYNC_DELAY_LOW, 0x0);
	regmap_write(pdata->regmap, SN65DSI84_REG_CHA_SYNC_DELAY_HIGH, 0x2);

	if(pdata->lvds_preserve_dsi_timings) {
		/* HSYNC VSYNC width high/low */
		regmap_write(pdata->regmap, SN65DSI84_REG_CHA_HSYNC_PULSE_WIDTH_LOW,	0);
		regmap_write(pdata->regmap, SN65DSI84_REG_CHA_HSYNC_PULSE_WIDTH_HIGH, 	0 | hsync_polarity);
		
		regmap_write(pdata->regmap, SN65DSI84_REG_CHA_VSYNC_PULSE_WIDTH_LOW, 	0);
		regmap_write(pdata->regmap, SN65DSI84_REG_CHA_VSYNC_PULSE_WIDTH_HIGH,	0 | vsync_polarity);	

			
		regmap_write(pdata->regmap, SN65DSI84_REG_CHA_HORIZONTAL_BACK_PORCH,	0);
		regmap_write(pdata->regmap, SN65DSI84_REG_CHA_VERTICAL_BACK_PORCH,	0);
		regmap_write(pdata->regmap, SN65DSI84_REG_CHA_HORIZONTAL_FRONT_PORCH,	0);
		regmap_write(pdata->regmap, SN65DSI84_REG_CHA_VERTICAL_FRONT_PORCH,	0);
	} else {
		/* HSYNC VSYNC width high/low */
		regmap_write(pdata->regmap, SN65DSI84_REG_CHA_HSYNC_PULSE_WIDTH_LOW,	(mode->hsync_end - mode->hsync_start) & 0x00ff);
		regmap_write(pdata->regmap, SN65DSI84_REG_CHA_HSYNC_PULSE_WIDTH_HIGH,(((mode->hsync_end - mode->hsync_start) >> 8) & 0x7F) | hsync_polarity);
		
		regmap_write(pdata->regmap, SN65DSI84_REG_CHA_VSYNC_PULSE_WIDTH_LOW,	(mode->vsync_end - mode->vsync_start) & 0xFF);
		regmap_write(pdata->regmap, SN65DSI84_REG_CHA_VSYNC_PULSE_WIDTH_HIGH,(((mode->vsync_end - mode->vsync_start) >> 8) & 0x7F) | vsync_polarity);	

			
		regmap_write(pdata->regmap, SN65DSI84_REG_CHA_HORIZONTAL_BACK_PORCH, (mode->htotal - mode->hsync_end) & 0xFF);
		regmap_write(pdata->regmap, SN65DSI84_REG_CHA_VERTICAL_BACK_PORCH, (mode->vtotal - mode->vsync_end) & 0xFF);
		regmap_write(pdata->regmap, SN65DSI84_REG_CHA_HORIZONTAL_FRONT_PORCH, (mode->hsync_start - mode->hdisplay) & 0xFF);
		regmap_write(pdata->regmap, SN65DSI84_REG_CHA_VERTICAL_FRONT_PORCH, (mode->vsync_start - mode->vdisplay) & 0xFF);
	}
	usleep_range(10000, 10500); /* 10ms delay recommended by spec */

	/* Test Mode */
	if(pdata->lvds_test_mode)
		regmap_write(pdata->regmap, SN65DSI84_REG_CHA_TEST_PATTERN,0x10);
	else
		regmap_write(pdata->regmap, SN65DSI84_REG_CHA_TEST_PATTERN,0x0);

	/* Reset the controller */
	regmap_write(pdata->regmap, SN65DSI84_REG_RESET, 0x1);
	/* PLL Enable - Start PLL */    
	regmap_write(pdata->regmap, SN65DSI84_REG_PLL_EN, 0x1);
	
	drm_panel_enable(pdata->panel);

	return;
	
}

static void sn65dsi84_bridge_disable(struct drm_bridge *bridge)
{

	struct sn65dsi84 *pdata = bridge_to_sn65dsi84(bridge);

	sn65dsi84_power_off(pdata);
	
	drm_panel_unprepare(pdata->panel);

	return;
}


static int sn65dsi84_bridge_attach(struct drm_bridge *bridge)
{

	int ret;
	struct sn65dsi84 *pdata = bridge_to_sn65dsi84(bridge);
	struct mipi_dsi_host *host;
        struct mipi_dsi_device *dsi;
        const struct mipi_dsi_device_info info = { .type = "sn65dsi84_bridge",
                                                   .channel = 0,
                                                   .node = NULL,
                                                 };


	ret = drm_connector_init(bridge->dev, &pdata->connector,
				 &sn65dsi84_connector_funcs,
				 DRM_MODE_CONNECTOR_LVDS);
	if (ret) {
		DRM_ERROR("Failed to initialize connector with drm\n");
		return ret;
	}
	drm_connector_helper_add(&pdata->connector,
				 &sn65dsi84_connector_helper_funcs);
	drm_connector_attach_encoder(&pdata->connector, bridge->encoder);
        /*
         * TODO: ideally finding host resource and dsi dev registration needs
         * to be done in bridge probe. But some existing DSI host drivers will
         * wait for any of the drm_bridge/drm_panel to get added to the global
         * bridge/panel list, before completing their probe. So if we do the
         * dsi dev registration part in bridge probe, before populating in
         * the global bridge list, then it will cause deadlock as dsi host probe
         * will never complete, neither our bridge probe. So keeping it here
         * will satisfy most of the existing host drivers. Once the host driver
         * is fixed we can move the below code to bridge probe safely.
         */
	host = of_find_mipi_dsi_host_by_node(pdata->host_node);
        if (!host) {
                DRM_ERROR("failed to find dsi host\n");
                ret = -ENODEV;
                goto err_dsi_host;
        }

	dsi = mipi_dsi_device_register_full(host, &info);
        if (IS_ERR(dsi)) {
                DRM_ERROR("failed to create dsi device\n");
                ret = PTR_ERR(dsi);
                goto err_dsi_host;
        }

        /* TODO: setting to 4 MIPI lanes always for now */
        dsi->lanes = 4;
        dsi->format = MIPI_DSI_FMT_RGB888;
	dsi->mode_flags = pdata->mode_flags;

	ret = mipi_dsi_attach(dsi);
	if (ret < 0) {
                DRM_ERROR("failed to attach dsi to host\n");
                goto err_dsi_attach;
        }
	pdata->dsi = dsi;

        /* attach panel to bridge */
        drm_panel_attach(pdata->panel, &pdata->connector);

        return 0;

err_dsi_attach:
        mipi_dsi_device_unregister(dsi);
err_dsi_host:
        drm_connector_cleanup(&pdata->connector);
        return ret;
}

static u32 sn65dsi84_bridge_get_dsi_freq(struct sn65dsi84 *pdata)
{
        u32 bit_rate_khz, clk_freq_khz;
        struct drm_display_mode *mode = 
		&pdata->bridge.encoder->crtc->state->adjusted_mode;

        bit_rate_khz = mode->clock *
                        mipi_dsi_pixel_format_to_bpp(pdata->dsi->format);
        clk_freq_khz = bit_rate_khz / (pdata->dsi->lanes * 2);

        return clk_freq_khz;
}

/* clk frequencies supported by bridge in Hz in case derived from REFCLK pin */
static const u32 sn65dsi84_bridge_refclk_lut[] = {
        12000000,
        19200000,
        26000000,
        27000000,
        38400000,
};

/* clk frequencies supported by bridge in Hz in case derived from DACP/N pin */
static const u32 sn65dsi84_bridge_dsiclk_lut[] = {
        468000000,
        384000000,
        416000000,
        486000000,
        460800000,
};

static void sn65dsi84_bridge_set_refclk_freq(struct sn65dsi84 *pdata)
{
        u32 refclk_rate;
        const u32 *refclk_lut;
        size_t refclk_lut_size;

        if (pdata->refclk) {
                refclk_rate = clk_get_rate(pdata->refclk);
                refclk_lut = sn65dsi84_bridge_refclk_lut;
                refclk_lut_size = ARRAY_SIZE(sn65dsi84_bridge_refclk_lut);
                clk_prepare_enable(pdata->refclk);
        } else {
                refclk_rate = sn65dsi84_bridge_get_dsi_freq(pdata) * 1000;
                refclk_lut = sn65dsi84_bridge_dsiclk_lut;
                refclk_lut_size = ARRAY_SIZE(sn65dsi84_bridge_dsiclk_lut);
        }

	pdata->refclk_rate = refclk_rate;

	regmap_write(pdata->regmap, SN65DSI84_REG_DSI_CLK_RANGE, (refclk_rate/(5*1000*1000)));
}

static void sn65dsi84_bridge_pre_enable(struct drm_bridge *bridge)
{
        struct sn65dsi84 *pdata = bridge_to_sn65dsi84(bridge);

        pm_runtime_get_sync(pdata->dev);

        /* configure bridge ref_clk */
        sn65dsi84_bridge_set_refclk_freq(pdata);

        drm_panel_prepare(pdata->panel);
}

static void sn65dsi84_bridge_post_disable(struct drm_bridge *bridge)
{
        struct sn65dsi84 *pdata = bridge_to_sn65dsi84(bridge);

        if (pdata->refclk)
                clk_disable_unprepare(pdata->refclk);

        pm_runtime_put_sync(pdata->dev);
}

static struct drm_bridge_funcs sn65dsi84_bridge_funcs = {
	.attach = sn65dsi84_bridge_attach,
	.pre_enable = sn65dsi84_bridge_pre_enable,
	.enable = sn65dsi84_bridge_enable,
	.disable = sn65dsi84_bridge_disable,
	.post_disable = sn65dsi84_bridge_post_disable,
};

static int sn65dsi84_bridge_parse_dsi_host(struct sn65dsi84 *pdata)
{
        struct device_node *np = pdata->dev->of_node;

        pdata->host_node = of_graph_get_remote_node(np, 0, 0);

        if (!pdata->host_node) {
                DRM_ERROR("remote dsi host node not found\n");
                return -ENODEV;
        }

        return 0;
}

static int sn65dsi84_probe(struct i2c_client *client, 
				const struct i2c_device_id *id)
{

	struct sn65dsi84 *pdata;
        int ret;

        pdata = devm_kzalloc(&client->dev, sizeof(struct sn65dsi84),
                             GFP_KERNEL);
        if (!pdata)
                return -ENOMEM;

	pdata->pd_gpio = devm_gpiod_get_optional(&client->dev, "pd",
                                            GPIOD_OUT_HIGH);

        pdata->lcd_gpio = devm_gpiod_get_optional(&client->dev, "lcd",
                                            GPIOD_OUT_HIGH);
        if (IS_ERR(pdata->lcd_gpio)) {
                DRM_ERROR("failed to get enable gpio from DT\n");
                ret = PTR_ERR(pdata->lcd_gpio);
                return ret;
        }

        pdata->bkl_gpio = devm_gpiod_get_optional(&client->dev, "bkl",
                                            GPIOD_OUT_HIGH);
        if (IS_ERR(pdata->bkl_gpio)) {
                DRM_ERROR("failed to get enable gpio from DT\n");
                ret = PTR_ERR(pdata->bkl_gpio);
                return ret;
        }

        if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
                DRM_ERROR("device doesn't support I2C\n");
                return -ENODEV;
        }
	
	pdata->regmap = devm_regmap_init_i2c(client,
                                             &sn65dsi84_regmap_config);
        if (IS_ERR(pdata->regmap)) {
                DRM_ERROR("regmap i2c init failed\n");
                return PTR_ERR(pdata->regmap);
        }

        pdata->dev = &client->dev;

        ret = drm_of_find_panel_or_bridge(pdata->dev->of_node, 1, 0,
                                          &pdata->panel, NULL);
        if (ret) {
                DRM_ERROR("could not find any panel node\n");
                return ret;
        }

        if(!sn65dsi84_videomode_parse_dt((pdata->dev->of_node), pdata))
                        DRM_ERROR("error parsing lvds timing!\n");


        dev_set_drvdata(&client->dev, pdata);

        pdata->refclk = devm_clk_get(pdata->dev, NULL);
        if (IS_ERR(pdata->refclk)) {
                ret = PTR_ERR(pdata->refclk);
                if (ret == -EPROBE_DEFER)
                        return ret;
                DRM_INFO("refclk not found\n");
                pdata->refclk = NULL;
        }

	ret = sn65dsi84_bridge_parse_dsi_host(pdata);
        if (ret)
                return ret;

        pm_runtime_enable(pdata->dev);

        i2c_set_clientdata(client, pdata);

        pdata->bridge.funcs = &sn65dsi84_bridge_funcs;
        pdata->bridge.of_node = client->dev.of_node;

        drm_bridge_add(&pdata->bridge);

        return 0;
}

static int sn65dsi84_remove(struct i2c_client *i2c)
{
	struct sn65dsi84 *pdata = i2c_get_clientdata(i2c);

	if (!pdata)
                return -EINVAL;

	of_node_put(pdata->host_node);

	pm_runtime_disable(pdata->dev);

	if (pdata->dsi) {
                mipi_dsi_detach(pdata->dsi);
                mipi_dsi_device_unregister(pdata->dsi);
        }

	drm_bridge_remove(&pdata->bridge);

	return 0;
}

static const struct i2c_device_id sn65dsi84_i2c_ids[] = {
	{ "sn65dsi84", SN65DSI84 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, sn65dsi84_i2c_ids);

static const struct of_device_id sn65dsi84_of_ids[] = {
	{ .compatible = "ti,sn65dsi84", .data = (void *)SN65DSI84 },
	{ }
};
MODULE_DEVICE_TABLE(of, sn65dsi84_of_ids);

static struct mipi_dsi_driver adv7533_dsi_driver = {
	.driver.name = "sn65dsi84",
};

static struct i2c_driver sn65dsi84_driver = {
	.driver = {
		.name = "sn65dsi84",
		.of_match_table = sn65dsi84_of_ids,
		.pm = &sn65dsi84_bridge_pm_ops,
	},
	.probe = sn65dsi84_probe,
	.remove = sn65dsi84_remove,
	.id_table = sn65dsi84_i2c_ids,
};

static int __init sn65dsi84_init(void)
{
	if (IS_ENABLED(CONFIG_DRM_MIPI_DSI))
		mipi_dsi_driver_register(&adv7533_dsi_driver);

	return i2c_add_driver(&sn65dsi84_driver);
}
module_init(sn65dsi84_init);

static void __exit sn65dsi84_exit(void)
{
	i2c_del_driver(&sn65dsi84_driver);

	if (IS_ENABLED(CONFIG_DRM_MIPI_DSI))
		mipi_dsi_driver_unregister(&adv7533_dsi_driver);
}
module_exit(sn65dsi84_exit);

MODULE_AUTHOR("Marco Sandrelli <marco.sandrelli@seco.com>");
MODULE_DESCRIPTION("SN65DSI84 LVDS transmitter driver");
MODULE_LICENSE("GPL");


/*
 * Copyright 2019 NXP
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "API_AFE_ss28fdsoi_hdmirx.h"
#include "mxc-hdmi-rx.h"

#include <asm/unaligned.h>
#include <linux/ktime.h>

void hdmirx_hdcp_enable(state_struct *state)
{
    int ret;
	int version =
		/*CDN_API_HDCP_REC_VERSION_BOTH;*/
		/*CDN_API_HDCP_REC_VERSION_1;*/
		CDN_API_HDCPRX_VERSION_2;

	CDN_API_HDCPRX_Config config = { 0 };

	pr_info("%s()\n", __func__);

	config.activate = 1;
	config.version = version; /* Support 1.x */
	config.repeater = 0;
	config.use_secondary_link = 0;
	config.use_km_key = 1;
	config.bcaps = 0x80;
	config.bstatus = 0x1000; /* HDMI mode */

	// This was handled by the SECO
	ret=CDN_API_HDCPRX_SetConfig_blocking(state,&config,
						    CDN_BUS_TYPE_APB);
	if (ret != CDN_OK) {
        pr_info("%s(), could not enable the HDCP\n", __func__);
    }


}

void hdmirx_hdcp_disable(state_struct *state)
{
	  //This is currently handled by the SECO
	  //hdmirx_hdcp_set(state, 0, CDN_API_HDCPRX_VERSION_2);

    int ret;
    int version = CDN_API_HDCPRX_VERSION_2;

	CDN_API_HDCPRX_Config config = { 0 };

	pr_info("%s()\n", __func__);

	config.activate = 0;
	config.version = version;
	config.repeater = 0;
	config.use_secondary_link = 0;
	config.use_km_key = 0;
	config.bcaps = 0x00;
	config.bstatus = 0x1000; /* HDMI mode */

	// This was handled by the SECO
	ret=CDN_API_HDCPRX_SetConfig_blocking(state, &config,
						    CDN_BUS_TYPE_APB);
    if (ret != CDN_OK) {
        pr_info("%s(), could not disable the HDCP\n", __func__);
    }

}

int hdmirx_hdcp_get_status(state_struct *state,
			   CDN_API_HDCPRX_Status *hdcp_status)
{
	/* todo: implement HDCP status checking if needed */

	  int ret;
	  pr_info("%s()\n", __func__);
	  memset(hdcp_status, 0, sizeof(CDN_API_HDCPRX_Status));

	  ret=CDN_API_HDCPRX_GetStatus_blocking(state,
						hdcp_status,
						CDN_BUS_TYPE_APB);
	  if (ret == CDN_OK) {
	      pr_info("HCDP key_arrived 0x%02x\n", hdcp_status->key_arrived);
	      pr_info("HCDP hdcp_ver    0x%02x\n", hdcp_status->hdcp_ver);
	      pr_info("HCDP error       0x%02x\n", hdcp_status->error);
	      pr_info("HCDP aksv[] 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",
	              hdcp_status->aksv[0],
	              hdcp_status->aksv[1],
	              hdcp_status->aksv[2],
	              hdcp_status->aksv[3],
	              hdcp_status->aksv[4]);
	      pr_info("HCDP ainfo      0x%02x\n", hdcp_status->ainfo);
      }

	return ret;
}



int hdmirx_hdcp_request_reauthentication(state_struct *state)
{
    int ret;

	pr_info("%s()\n", __func__);

    ret=CDN_API_HDCPRX_NotSync_blocking(state,CDN_BUS_TYPE_APB);

	if (ret != CDN_OK) {
        pr_info("%s(), could request reauthentication the HDCP\n", __func__);
    }
	return ret;
}

DTC_FLAGS ?= -@ -H epapr


dtbo-$(CONFIG_SOC_IMX6Q) += \
	imx6qdl-seco_uq7_962_uart4.dtbo \
	imx6qdl-seco_uq7_962_ac97.dtbo \
	imx6qdl-seco_uq7_962_video_LVDS.dtbo \
	imx6qdl-seco_uq7_962_video_LVDSx2_CLONE.dtbo \
	imx6qdl-seco_uq7_962_video_LVDSx2_DUAL.dtbo \
	imx6qdl-seco_uq7_962_video_HDMI.dtbo \
	imx6qdl-seco_uq7_962_video_LVDS_HDMI.dtbo \
	imx6qdl-seco_uq7_962_video_HDMI_LVDS.dtbo \
	imx6qdl-seco_uQ7_J_A75_ac97.dtbo \
	imx6qdl-seco_uQ7_J_A75_uart1.dtbo \
	imx6qdl-seco_uQ7_J_A75_video_LVDS.dtbo \
	imx6qdl-seco_uQ7_J_A75_video_LVDSx2_CLONE.dtbo \
	imx6qdl-seco_uQ7_J_A75_video_LVDSx2_DUAL.dtbo \
	imx6qdl-seco_uQ7_J_A75_video_HDMI.dtbo \
	imx6qdl-seco_uQ7_J_A75_video_LVDS_HDMI.dtbo \
	imx6qdl-seco_uQ7_J_A75_video_HDMI_LVDS.dtbo \
	imx6qdl-seco_quadmo747_928_ac97.dtbo \
	imx6qdl-seco_quadmo747_928_cpld_gpio_pwm.dtbo \
	imx6qdl-seco_quadmo747_928_cpld_maxlinear.dtbo \
	imx6qdl-seco_quadmo747_928_cpld_nuvoton.dtbo \
	imx6qdl-seco_quadmo747_928_uart4.dtbo \
	imx6qdl-seco_quadmo747_928_video_LVDS.dtbo \
	imx6qdl-seco_quadmo747_928_video_LVDSx2_CLONE.dtbo \
	imx6qdl-seco_quadmo747_928_video_LVDSx2_DUAL.dtbo \
	imx6qdl-seco_quadmo747_928_video_HDMI.dtbo \
	imx6qdl-seco_quadmo747_928_video_LVDS_HDMI.dtbo \
	imx6qdl-seco_quadmo747_928_video_HDMI_LVDS.dtbo \
	imx6qdl-seco_SBC_A62_touch_gt928.dtbo \
	imx6qdl-seco_SBC_A62_touch_st1232.dtbo \
	imx6qdl-seco_SBC_A62_conn_j8.dtbo \
	imx6qdl-seco_SBC_A62_video_LVDS.dtbo \
	imx6qdl-seco_SBC_A62_video_LVDSx2_CLONE.dtbo \
	imx6qdl-seco_SBC_A62_video_LVDSx2_DUAL.dtbo \
	imx6qdl-seco_SBC_A62_video_HDMI.dtbo \
	imx6qdl-seco_SBC_A62_video_LVDS_HDMI.dtbo \
	imx6qdl-seco_SBC_A62_video_HDMI_LVDS.dtbo 

dtbo-$(CONFIG_SOC_IMX6SX) += \
	imx6sx-seco_SBC_B08_LVDS.dtbo \
	imx6sx-seco_SBC_B08_RGB.dtbo \
	imx6sx-seco_SBC_B08_conn_j9.dtbo \
	imx6sx-seco_SBC_C23_LVDS.dtbo

targets += dtbs dtbs_install
targets += $(dtbo-y)

always		:= $(dtbo-y)
clean-files	:= *.dtbo


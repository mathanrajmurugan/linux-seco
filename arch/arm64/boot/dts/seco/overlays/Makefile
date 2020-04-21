
dtbo-$(CONFIG_ARCH_FSL_IMX8QM) +=  seco-imx8qm-c26-hdmi.dtbo\
					seco-imx8qm-c26-dp.dtbo\
					seco-imx8qm-c26-lvds-single.dtbo\
					seco-imx8qm-c26-lvds-dual.dtbo\
					seco-imx8qm-c26-hdmi-lvds.dtbo\
					seco-imx8qm-c26-dp-lvds.dtbo\
					seco-imx8qm-c26-ov5640-csi0.dtbo\
					seco-imx8qm-c26-ov5640-csi1.dtbo
					
dtbo-$(CONFIG_ARCH_FSL_IMX8QXP) +=  seco-imx8qxp-c57-800x600-lvds.dtbo \
				    seco-imx8qxp-c57-1920x1080-dual-channel-lvds.dtbo \
				    seco-imx8qxp-c57-sn65dsi86-edp.dtbo


targets += dtbs dtbs_install
targets += $(dtbo-y)

always		:= $(dtbo-y)
clean-files	:= *.dtbo

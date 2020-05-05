
dtbo-$(CONFIG_ARCH_FSL_IMX8QM) +=  seco-imx8qm-c26-hdmi.dtbo\
					seco-imx8qm-c26-dp.dtbo\
					seco-imx8qm-c26-lvds-single.dtbo\
					seco-imx8qm-c26-lvds-dual.dtbo\
					seco-imx8qm-c26-hdmi-lvds.dtbo\
					seco-imx8qm-c26-dp-lvds.dtbo\
					seco-imx8qm-c26-ov5640-csi0.dtbo\
					seco-imx8qm-c26-ov5640-csi1.dtbo

dtbo-$(CONFIG_ARCH_FSL_IMX8MM) +=  seco-imx8mm-c61-lcdif-sn65dsi86.dtbo\
                                        seco-imx8mm-c61-lcdif-sn65dsi84.dtbo\
                                        seco-imx8mm-c61-ov5640.dtbo\
                                        seco-imx8mm-c61-port1-can.dtbo\
                                        seco-imx8mm-c61-port1-rs232.dtbo\
                                        seco-imx8mm-c61-port1-rs485.dtbo\
                                        seco-imx8mm-c61-port1-gpios.dtbo\
                                        seco-imx8mm-c61-port2-can.dtbo\
                                        seco-imx8mm-c61-port2-rs232.dtbo\
                                        seco-imx8mm-c61-port2-rs485.dtbo\
                                        seco-imx8mm-c61-port2-gpios.dtbo\
                                        seco-imx8mm-c61-modem.dtbo\
					
dtbo-$(CONFIG_ARCH_FSL_IMX8QXP) +=  seco-imx8qxp-c57-800x600-lvds.dtbo \
				    seco-imx8qxp-c57-1920x1080-dual-channel-lvds.dtbo \
				    seco-imx8qxp-c57-sn65dsi86-edp.dtbo


targets += dtbs dtbs_install
targets += $(dtbo-y)

always		:= $(dtbo-y)
clean-files	:= *.dtbo

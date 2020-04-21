#ifndef __LINUX_SIO_W83627_H__
#define __LINUX_SIO_W83627_H__


enum W83627_DEVICE {
	FDC, PARALLEL_PORT, UARTA, UARTB, NONE, KEYBOARD_CRTL,
	SPI, GPIO6, WDTO_PLED, GPIO2, GPIO3, GPIO4, GPIO5, ACPI, HWMON, PECI_SST
};


struct sio_w83627_ops {
	void    (*read) (uint8_t addr, uint8_t *data, uint8_t ldev_id);
	void    (*write) (uint8_t addr, uint8_t value, uint8_t ldev_id);
	void    (*g_read) (uint8_t addr, uint8_t *data);
	void    (*g_write) (uint8_t addr, uint8_t value);

};


#endif	/*   __LINUX_SIO_W83627_H__   */

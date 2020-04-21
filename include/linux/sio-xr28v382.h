#ifndef __LINUX_SIO_XR28V382_H__
#define __LINUX_SIO_XR28V382_H__


enum XR28V382_DEVICE {
	UARTA, UARTB
};


struct sio_xr28v382_ops {
	void    (*read) (uint8_t addr, uint8_t *data, uint8_t ldev_id);
	void    (*write) (uint8_t addr, uint8_t value, uint8_t ldev_id);
	void    (*g_read) (uint8_t addr, uint8_t *data);
	void    (*g_write) (uint8_t addr, uint8_t value);

};


#endif	/*   __LINUX_SIO_XR28V382_H__   */

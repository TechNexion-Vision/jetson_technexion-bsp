#ifndef __VIZIONLINK_CAM_H__
#define __VIZIONLINK_CAM_H__

#include <linux/i2c.h>

struct vls3_reg {
	u8 reg;
	u8 value;
};

extern int vls3_ser_init(struct i2c_client *client, u8 ser_alias_addr);
extern int vls3_ser_configure_ser_csi(struct i2c_client *client, u8 ser_alias_addr);

#endif

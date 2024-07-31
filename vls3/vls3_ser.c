#include <linux/module.h>
#include <linux/delay.h>
#include <linux/i2c.h>

#include "vls3_ser.h"

static struct vls3_reg vls3_ser_i2c_setting[] = {
	// {0x05, 0x0B},
	{0x0B, 0x13},
	{0x0C, 0x26},
	{0x0D, 0xF0},
	{0x0E, 0xF0},
	{0x1C, 0x7F},
};

static int vls3_ser_i2c_read(struct i2c_client *client, u8 addr, u8 reg, u8 *val, u8 size)
{
	struct i2c_msg msg[2];
	int ret;

	dev_dbg(&client->dev,
		"ub953%s(): addr 0x%x, read reg 0x%x, size %d\n",
		__func__,
		addr,
		reg,
		size);

	msg[0].addr = addr;
	msg[0].flags = client->flags;
	msg[0].buf = &reg;
	msg[0].len = 1;

	msg[1].addr = addr;
	msg[1].flags = client->flags | I2C_M_RD;
	msg[1].buf = val;
	msg[1].len = size;

	ret = i2c_transfer(client->adapter, msg, 2);
	if (ret < 0) {
		dev_err(&client->dev, "i2c transfer error\n");
	}

	return ret;
}

static int vls3_ser_i2c_write(struct i2c_client *client, u8 addr, u8 reg, u8 val)
{
	struct i2c_msg msg;
	u8 buf[2];
	int ret;

	dev_dbg(&client->dev,
		"ub953%s(): addr 0x%x, read reg 0x%x, val 0x%x\n",
		__func__,
		addr,
		reg,
		val);

	buf[0] = reg;
	buf[1] = val;

	msg.addr = addr;
	msg.flags = client->flags;
	msg.buf = buf;
	msg.len = sizeof(buf);

	ret = i2c_transfer(client->adapter, &msg, 1);
	if (ret < 0) {
		dev_err(&client->dev, "i2c transfer error\n");
	}

	return ret;
}

int vls3_ser_init(struct i2c_client *client, u8 ser_alias_addr)
{
	u8 v = 0;
	int ret = 0;
	int i = 0;
	// int count = 0;

	//90953 Digital Reset
	vls3_ser_i2c_write(client, ser_alias_addr, 1, 3);
	msleep(20);

	vls3_ser_i2c_read(client, ser_alias_addr, 3, &v, 1);
	while ((v & 0x8) != 0x8) {
		if (i < 10) {
			msleep(20);
			vls3_ser_i2c_read(client, ser_alias_addr, 3, &v, 1);
			i++;
		} else {
			dev_info(&client->dev,
				 "Time out to probe. abort\n");
			ret = -EINVAL;
			goto except;
		}
	}

	switch (v & 0x7) {
	case 0:
		dev_info(&client->dev,
			 "Sync Mode\n");
		break;
	case 2:
		dev_info(&client->dev,
			 "Non Sync, external clock Mode\n");
		break;
	case 3:
		dev_info(&client->dev,
			 "Non Sync, internal AON clock Mode\n");
		break;
	default:
		dev_err(&client->dev,
			"Not Support Mode, abort\n");
		ret = -EINVAL;
		goto except;
	}

	for(i = 0; i < ARRAY_SIZE(vls3_ser_i2c_setting) ; i++) {
		vls3_ser_i2c_write(client,
			    ser_alias_addr,
			    vls3_ser_i2c_setting[i].reg,
			    vls3_ser_i2c_setting[i].value);
	}

except:
	return ret;
}

int vls3_ser_configure_ser_csi(struct i2c_client *client, u8 ser_alias_addr)
{
	struct device *dev = &client->dev;
	u32 temp1;
	u8 temp2;
	u8 ser_csi_lanes;
	u8 ser_csi_continuous_clock;
	int ret;

	temp1 = 0xff;
	ret = of_property_read_u32(dev->of_node, "ser_csi_lanes", &temp1);
	if (ret == 0) {
		if (temp1 > 0 && temp1 < 5) {
			dev_dbg(dev, "serializer csi lanes: %d\n", temp1);
			ser_csi_lanes = temp1 - 1;
		} else {
			dev_err(dev, "value of 'ser_csi_lanes' is invaild.\n");
			return -EINVAL;
		}
	} else {
		dev_dbg(dev,
			"Failed to get property of device node 'ser_csi_lanes'."
			"use default 4 lanes\n");
		ser_csi_lanes = 3;
	}

	temp1 = 2;
	ret = of_property_read_u32(dev->of_node,
				   "ser_csi_continuous_clock",
				   &temp1);
	if (ret == 0) {
		if ((temp1 == 0 || temp1 == 1)) {
			dev_dbg(dev,
				"csi continuous clock of serializer: %d\n",
				temp1);
			ser_csi_continuous_clock = temp1;
		} else {
			dev_err(dev,
				"value of 'ser_csi_continuous_clock' is invaild.");
			return -EINVAL;
		}
	} else {
		dev_dbg(dev,
			"Failed to get property of device node 'ser_csi_continuous_clock'."
			"csi continuous clock of serializer: 0\n");
		ser_csi_continuous_clock = 0;
	}

	vls3_ser_i2c_read(client, ser_alias_addr, 2, &temp2, 1);
	temp2 &= ~(0x70);
	temp2 |= (ser_csi_continuous_clock << 6);
	temp2 |= (ser_csi_lanes << 4);
	vls3_ser_i2c_write(client, ser_alias_addr, 2, temp2);

	return 0;
}

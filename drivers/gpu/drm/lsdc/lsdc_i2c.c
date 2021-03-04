// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2020 Loongson Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS, AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 */

/*
 * Authors:
 *      Sui Jingfeng <suijingfeng@loongson.cn>
 */

#include <linux/string.h>
#include <linux/i2c.h>

#include <drm/drm_print.h>
#include <drm/drm_edid.h>

#include "lsdc_drv.h"
#include "lsdc_hardware.h"
#include "lsdc_i2c.h"

/*
 * DVO : Digital Video Output
 * There are two GPIO emulated i2c in LS7A1000 for reading edid from
 * the monitor if connected. Note that there are in the DC control
 * register space
 *
 * GPIO data register
 *  Address offset: 0x1650
 *   -----------------------------------------
 *   | 7 | 6 | 5 | 4 |  3  |  2  |  1  |  0  |
 *   |               |    DVO1   |    DVO0   |
 *   |               | SCL | SDA | SCL | SDA |
 *   -----------------------------------------
*/
#define LS7A_DC_GPIO_DATA_REG         0x1650

/*
 *  GPIO Input/Output direction control register
 *  Address offset: 0x1660
 *  write 1 for Input,  0 for Output.
 */
#define LS7A_DC_GPIO_DIR_REG          0x1660

#define LSDC_GPIO_I2C_NAME            "ls_gpio_i2c"
#define LSDC_DDC_I2C_ADDR             0x50



static void lsn_set_gpio_dir(struct loongson_i2c *i2c_chan, u32 pin, int input)
{
	struct loongson_drm_device *lsp = to_loongson_private(i2c_chan->dev);
	u32 dat = ls_reg_read32(lsp, LS7A_DC_GPIO_DIR_REG);

	if (input)
		dat |= 1UL << pin;
	else
		dat &= ~(1UL << pin);

	ls_reg_write32(lsp, LS7A_DC_GPIO_DIR_REG, dat);
}


static void lsn_set_gpio_val(struct loongson_i2c *i2c_chan, u32 bit, int val)
{
	struct loongson_drm_device *lsp = to_loongson_private(i2c_chan->dev);
	u32 dat = ls_reg_read32(lsp, LS7A_DC_GPIO_DATA_REG);

	if (val)
		dat |= 1U << bit;
	else
		dat &= ~(1U << bit);

	ls_reg_write32(lsp, LS7A_DC_GPIO_DATA_REG, dat);
}


static int lsn_i2c_pre_xfer(struct i2c_adapter *i2c_adap)
{
	struct i2c_algo_bit_data *bit = i2c_adap->algo_data;
	struct loongson_i2c *i2c_chan = bit->data;

	pr_debug("i2c_algo_bit_data=%p\n", bit);
	pr_debug("i2c_chan=%p\n", i2c_chan);
	pr_debug("ddev=%p\n", i2c_chan->dev);
	pr_debug("i2c_chan->data=%d, clock=%d\n",
		i2c_chan->data, i2c_chan->clock);

	/* clear the output pin values */
	lsn_set_gpio_val(i2c_chan, i2c_chan->data, 0);
	lsn_set_gpio_val(i2c_chan, i2c_chan->clock, 0);

	/* set the pins to input */
	lsn_set_gpio_dir(i2c_chan, i2c_chan->data, 1);

	return 0;
}

static void lsn_i2c_post_xfer(struct i2c_adapter *i2c_adap)
{
	struct i2c_algo_bit_data *bit = i2c_adap->algo_data;
	struct loongson_i2c *i2c_chan = bit->data;

	/* clear the output pin values */
	lsn_set_gpio_val(i2c_chan, i2c_chan->data, 0);
	lsn_set_gpio_val(i2c_chan, i2c_chan->clock, 0);

	/* set the pins to input */
	lsn_set_gpio_dir(i2c_chan, i2c_chan->data, 1);

	pr_debug("\n");
}

static void lsn_i2c_set_data(void *i2c, int val)
{
	struct loongson_i2c *i2c_chan = i2c;

	lsn_set_gpio_dir(i2c_chan, i2c_chan->data, 0);
	lsn_set_gpio_val(i2c_chan, i2c_chan->data, val);
}

static void lsn_i2c_set_clock(void *i2c, int clk)
{
	struct loongson_i2c *i2c_chan = i2c;

	lsn_set_gpio_dir(i2c_chan, i2c_chan->clock, 0);
	lsn_set_gpio_val(i2c_chan, i2c_chan->clock, clk);
}

static int lsn_i2c_get_data(void *i2c)
{
	struct loongson_i2c *i2c_chan = i2c;
	struct loongson_drm_device *lsp = to_loongson_private(i2c_chan->dev);
	u32 val;

	lsn_set_gpio_dir(i2c_chan, i2c_chan->data, 1);
	val = ls_reg_read32(lsp, LS7A_DC_GPIO_DATA_REG);

	return (val >> i2c_chan->data) & 1;
}

static int lsn_i2c_get_clock(void *i2c)
{
	struct loongson_i2c *i2c_chan = i2c;
	struct loongson_drm_device *lsp = to_loongson_private(i2c_chan->dev);
	u32 val;

	/* read the value off the pin */
	lsn_set_gpio_dir(i2c_chan, i2c_chan->clock, 1);
	val = ls_reg_read32(lsp, LS7A_DC_GPIO_DATA_REG);

	return (val >> i2c_chan->clock) & 1;
}

struct loongson_i2c *ls7a_create_i2c_chan(struct loongson_drm_device *pLsDev,
					  const unsigned int i2c_id)
{
	struct i2c_adapter *i2c_adapter;
	struct loongson_i2c *i2c_chan;
	int ret;

	i2c_chan = kzalloc(sizeof(struct loongson_i2c), GFP_KERNEL);
	if (IS_ERR(i2c_chan)) {
		DRM_ERROR("Failed alloc loongson_i2c_chan!");
		return NULL;
	}

	i2c_chan->dev = pLsDev->dev;
	i2c_chan->i2c_id = i2c_id;
	i2c_chan->data = i2c_chan->i2c_id % 6 * 2;
	i2c_chan->clock = i2c_chan->i2c_id % 6 * 2 + 1;
	i2c_chan->bit.pre_xfer = lsn_i2c_pre_xfer;
	i2c_chan->bit.post_xfer = lsn_i2c_post_xfer;
	i2c_chan->bit.setsda = lsn_i2c_set_data;
	i2c_chan->bit.setscl = lsn_i2c_set_clock;
	i2c_chan->bit.getsda = lsn_i2c_get_data;
	i2c_chan->bit.getscl = lsn_i2c_get_clock;
	i2c_chan->bit.udelay = 20;
	i2c_chan->bit.timeout = usecs_to_jiffies(2200); /* from VESA */
	i2c_chan->bit.data = i2c_chan;

	i2c_adapter = &i2c_chan->adapter;
	i2c_adapter->algo_data = &i2c_chan->bit;

	i2c_adapter->owner = THIS_MODULE;
	i2c_adapter->class = I2C_CLASS_DDC;
	i2c_adapter->dev.parent = pLsDev->dev->dev;
	i2c_adapter->nr = i2c_id;

	snprintf(i2c_adapter->name, sizeof(i2c_adapter->name),
		"%s-%d", LSDC_GPIO_I2C_NAME, i2c_id);

	i2c_set_adapdata(i2c_adapter, i2c_chan);

	ret = i2c_bit_add_numbered_bus(i2c_adapter);
	if (ret) {
		DRM_ERROR("Failed to register algo-bit i2c %d\n",
			  i2c_chan->i2c_id);
		kfree(i2c_adapter);
		i2c_adapter = NULL;
		return NULL;
	}

	return i2c_chan;
}

struct loongson_i2c *ls2k_create_i2c_chan(struct loongson_drm_device *pLsDev,
					  const unsigned int i2c_id)
{
	struct i2c_adapter *i2c_adapter;
	struct i2c_board_info i2c_info;
	struct loongson_i2c *i2c_chan;

	i2c_chan = kzalloc(sizeof(struct loongson_i2c), GFP_KERNEL);
	if (IS_ERR(i2c_chan)) {
		DRM_ERROR("Failed alloc loongson_i2c_chan!");
		return NULL;
	}

	i2c_chan->i2c_id = i2c_id;

	memset(&i2c_info, 0, sizeof(struct i2c_board_info));
	strlcpy(i2c_info.type, LSDC_GPIO_I2C_NAME, I2C_NAME_SIZE);
	i2c_info.addr = LSDC_DDC_I2C_ADDR;

	i2c_adapter = i2c_get_adapter(i2c_id);
	if (!i2c_adapter) {
		dev_warn_once(pLsDev->dev->dev,
				"i2c-%d get adapter err\n", i2c_id);
		kfree(i2c_chan);
		return false;
	}

	i2c_adapter->class = I2C_CLASS_DDC;

	i2c_chan->adapter = *i2c_adapter;
	i2c_chan->i2c_id = i2c_id;
	i2c_chan->used = true;

	i2c_put_adapter(i2c_adapter);

	return i2c_chan;
}


void lsn_destroy_i2c(struct loongson_i2c *pLsI2c)
{
	if (pLsI2c == NULL) {
		DRM_WARN("Trying to destroy NULL i2c adapter.\n");
		return;
	}

	i2c_del_adapter(&pLsI2c->adapter);
	kfree(pLsI2c);

	DRM_INFO("i2c channal %u destroyed.\n", pLsI2c->i2c_id);
}

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

#include <drm/drm_print.h>

#include "lsdc_drv.h"
#include "lsdc_vbios.h"


#ifdef CONFIG_CPU_LOONGSON3
extern u64 ls_get_vbios_addr(void);
#endif

#define VBIOS_START_ADDR	0x1000
#define LSDC_VBIOS_SIZE		0x1E000


static const char LS_VBIOS_TITLE[] = {"Loongson-VBIOS"};


static int have_table;
static uint32_t table[256];

static uint32_t POLYNOMIAL = 0xEDB88320;

static void make_table(void)
{
	int i, j;

	have_table = 1;
	for (i = 0 ; i < 256 ; i++)
		for (j = 0, table[i] = i; j < 8; j++)
			table[i] = (table[i]>>1)^((table[i]&1)?POLYNOMIAL:0);
}

static uint lscrc32(uint crc, char *buff, int len)
{
	int i;

	if (!have_table)
		make_table();

	crc = ~crc;
	for (i = 0; i < len; i++)
		crc = (crc >> 8) ^ table[(crc ^ buff[i]) & 0xff];

	return ~crc;
}

static struct loongson_vbios *lsn_vbios_default(
			struct loongson_drm_device *pLsDev)
{
	unsigned char *vbios_start;
	struct loongson_vbios *vbios;
	struct loongson_vbios_crtc *crtc_vbios[2];
	struct loongson_vbios_encoder *encoder_vbios[2];
	struct loongson_vbios_connector *connector_vbios[2];

	DRM_INFO("Generate default firmware.\n");

	vbios = kzalloc(120*1024, GFP_KERNEL);

	vbios_start = (unsigned char *)vbios;

	/* Build loongson_vbios struct */
	strncpy(vbios->title, LS_VBIOS_TITLE, sizeof(LS_VBIOS_TITLE));
	vbios->version_major = 0;
	vbios->version_minor = 2;
	vbios->crtc_num = 2;
	vbios->crtc_offset = sizeof(struct loongson_vbios);
	vbios->connector_num = 2;
	vbios->connector_offset = sizeof(struct loongson_vbios) +
		2 * sizeof(struct loongson_vbios_crtc);
	vbios->encoder_num = 2;
	vbios->encoder_offset = sizeof(struct loongson_vbios) +
		2 * sizeof(struct loongson_vbios_crtc) +
		2 * sizeof(struct loongson_vbios_connector);


	/* Build loongson_vbios_crtc struct */
	crtc_vbios[0] = (struct loongson_vbios_crtc *)(vbios_start +
							vbios->crtc_offset);
	crtc_vbios[1] = (struct loongson_vbios_crtc *)(vbios_start +
		vbios->crtc_offset + sizeof(struct loongson_vbios_crtc));

	crtc_vbios[0]->next_crtc_offset = sizeof(struct loongson_vbios) +
		sizeof(struct loongson_vbios_crtc);
	crtc_vbios[0]->crtc_id = 0;
	crtc_vbios[0]->crtc_version = default_version;
	crtc_vbios[0]->crtc_max_width = 2048;
	crtc_vbios[0]->crtc_max_height = 2048;
	crtc_vbios[0]->encoder_id = 0;
	crtc_vbios[0]->use_local_param = false;

	crtc_vbios[1]->next_crtc_offset = 0;
	crtc_vbios[1]->crtc_id = 1;
	crtc_vbios[1]->crtc_version = default_version;
	crtc_vbios[1]->crtc_max_width = 2048;
	crtc_vbios[1]->crtc_max_height = 2048;
	crtc_vbios[1]->encoder_id = 1;
	crtc_vbios[1]->use_local_param = false;

	/* Build loongson_vbios_encoder struct */
	encoder_vbios[0] = (struct loongson_vbios_encoder *)(vbios_start +
			vbios->encoder_offset);
	encoder_vbios[1] = (struct loongson_vbios_encoder *)(vbios_start +
		vbios->encoder_offset + sizeof(struct loongson_vbios_encoder));

	encoder_vbios[0]->crtc_id = 0;
	encoder_vbios[1]->crtc_id = 1;

	encoder_vbios[0]->connector_id = 0;
	encoder_vbios[1]->connector_id = 1;

	encoder_vbios[0]->i2c_type = i2c_type_gpio;
	encoder_vbios[1]->i2c_type = i2c_type_gpio;

	encoder_vbios[0]->config_type = encoder_transparent;
	encoder_vbios[1]->config_type = encoder_transparent;

	encoder_vbios[0]->next_encoder_offset = vbios->encoder_offset +
		sizeof(struct loongson_vbios_encoder);
	encoder_vbios[1]->next_encoder_offset = 0;

	/* Build loongson_vbios_connector struct */
	connector_vbios[0] = (struct loongson_vbios_connector *)(vbios_start +
			vbios->connector_offset);
	connector_vbios[1] = (struct loongson_vbios_connector *)(vbios_start +
				vbios->connector_offset +
				sizeof(struct loongson_vbios_connector));

	connector_vbios[0]->next_connector_offset = vbios->connector_offset +
		sizeof(struct loongson_vbios_connector);
	connector_vbios[1]->next_connector_offset = 0;

	connector_vbios[0]->edid_method = edid_method_i2c;
	connector_vbios[1]->edid_method = edid_method_i2c;


	if (pLsDev->chip == LSDC_CHIP_7A1000) {
		encoder_vbios[0]->i2c_id = 6;
		encoder_vbios[1]->i2c_id = 7;
		connector_vbios[0]->i2c_id = 6;
		connector_vbios[1]->i2c_id = 7;

		DRM_INFO("%s: Generate GPIO I2C configuration for LS7A1000.\n",
			__func__);

		connector_vbios[0]->i2c_type = i2c_type_gpio;
		connector_vbios[1]->i2c_type = i2c_type_gpio;
		connector_vbios[0]->hot_swap_method = hot_swap_polling;
		connector_vbios[1]->hot_swap_method = hot_swap_polling;


		if (pLsDev->board == LS2K1000_PC_EVB_V1_2) {
			connector_vbios[0]->type = DRM_MODE_CONNECTOR_VGA;
			connector_vbios[1]->type = DRM_MODE_CONNECTOR_DVII;

			encoder_vbios[0]->type = DRM_MODE_ENCODER_DAC;
			encoder_vbios[1]->type = DRM_MODE_ENCODER_TMDS;
		} else if (pLsDev->board == LS2K1000_PC_EVB_V1_1) {
			connector_vbios[0]->type = DRM_MODE_CONNECTOR_DVII;
			connector_vbios[1]->type = DRM_MODE_CONNECTOR_DVII;

			encoder_vbios[0]->type = DRM_MODE_ENCODER_TMDS;
			encoder_vbios[1]->type = DRM_MODE_ENCODER_TMDS;
		}
	} else if (pLsDev->chip == LSDC_CHIP_2K1000) {
		DRM_INFO("%s: Generate GPIO I2C configuration for LS2K1000.\n",
			__func__);

		connector_vbios[0]->i2c_id = 0;
		connector_vbios[1]->i2c_id = 1;

		encoder_vbios[0]->i2c_id = 0;
		encoder_vbios[1]->i2c_id = 1;

		if (pLsDev->board == LS2K1000_PC_EVB_V1_2) {
			connector_vbios[0]->type = DRM_MODE_CONNECTOR_VGA;
			connector_vbios[1]->type = DRM_MODE_CONNECTOR_DVII;

			encoder_vbios[0]->type = DRM_MODE_ENCODER_DAC;
			encoder_vbios[1]->type = DRM_MODE_ENCODER_TMDS;
		} else if (pLsDev->board == LS2K1000_PC_EVB_V1_1) {
			connector_vbios[0]->type = DRM_MODE_CONNECTOR_DVII;
			connector_vbios[1]->type = DRM_MODE_CONNECTOR_DVII;

			encoder_vbios[0]->type = DRM_MODE_ENCODER_TMDS;
			encoder_vbios[1]->type = DRM_MODE_ENCODER_TMDS;
		}

		connector_vbios[0]->i2c_type = i2c_type_gpio;
		connector_vbios[1]->i2c_type = i2c_type_gpio;
		connector_vbios[0]->hot_swap_method = hot_swap_polling;
		connector_vbios[1]->hot_swap_method = hot_swap_polling;
	} else
		DRM_WARN("%s: Need support for %u.\n", __func__, pLsDev->chip);

	return vbios;
}


static int loongson_vbios_check(struct loongson_vbios *vbios)
{
	unsigned int crc, ver;
	unsigned int get_crc;
	unsigned int vbios_len;

	strncpy(vbios->title, LS_VBIOS_TITLE, sizeof(LS_VBIOS_TITLE));

	/* Data structrues of V0.1 and V0.2 are different */
	ver = vbios->version_major * 10 + vbios->version_minor;
	if (ver < 2)
		return -EINVAL;

	vbios_len = LSDC_VBIOS_SIZE - 0x4;

	crc = lscrc32(0, (unsigned char *)vbios, vbios_len);

	get_crc = *(unsigned int *)((unsigned char *)vbios + vbios_len);

	if (get_crc != crc) {
		DRM_ERROR("VBIOS crc is wrong!\n");
		return -EINVAL;
	}

	return 0;
}


static int lsn_vbios_information_display(struct loongson_drm_device *ldev)
{
	int i;
	struct loongson_vbios_crtc  *crtc;
	struct loongson_vbios_encoder *encoder;
	struct loongson_vbios_connector *connector;
	const char *config_method;

	static const char * const encoder_methods[] = {
		"NONE",
		"OS",
		"BIOS"
	};

	static const char * const edid_methods[] = {
		"No EDID",
		"Reading EDID via built-in I2C",
		"Use the VBIOS built-in EDID information",
		"Get EDID via encoder chip"
	};

	static const char * const detect_methods[] = {
		"NONE",
		"POLL",
		"HPD"
	};

	DRM_INFO("------------------------------------\n");

	DRM_INFO("Title: %s\n", ldev->vbios->title);

	DRM_INFO("Loongson VBIOS: V%d.%d\n",
			ldev->vbios->version_major,
			ldev->vbios->version_minor);

	DRM_INFO("crtc:%d encoder:%d connector:%d\n",
			ldev->vbios->crtc_num,
			ldev->vbios->encoder_num,
			ldev->vbios->connector_num);

	for (i = 0; i < ldev->vbios->crtc_num; i++) {
		crtc = ldev->crtc_vbios[i];
		encoder = ldev->encoder_vbios[crtc->encoder_id];
		config_method = encoder_methods[encoder->config_type];
		connector = ldev->connector_vbios[encoder->connector_id];
		DRM_INFO("\tencoder%d(%s) i2c:%d\n",
			crtc->encoder_id, config_method, encoder->i2c_id);
		DRM_INFO("\tconnector%d:\n", encoder->connector_id);
		DRM_INFO("\t %s", edid_methods[connector->edid_method]);
		DRM_INFO("\t Detect:%s\n",
				detect_methods[connector->hot_swap_method]);
	}

	DRM_INFO("------------------------------------\n");

	return 0;
}

int lsn_get_i2c_id_from_vbios(struct loongson_drm_device *ldev,
			      const unsigned int index)
{
	/* Get connector information from vbios */
	struct loongson_vbios_connector *pVbiosConnector = NULL;

	if (index <= 2)
		pVbiosConnector = ldev->connector_vbios[index];
	else {
		DRM_ERROR("%s: connector index(%d) overflow.\n",
			__func__, index);
		return -1;
	}

	return pVbiosConnector->i2c_id;
}


int lsn_get_connector_type_from_vbios(struct loongson_drm_device *ldev,
				      const unsigned int index)
{
	struct loongson_vbios_connector *pVbiosConnector = NULL;

	if (index <= 2)
		pVbiosConnector = ldev->connector_vbios[index];
	else {
		DRM_ERROR("%s: connector index(%d) overflow.\n",
			__func__, index);
		return -1;
	}

	return pVbiosConnector->type;
}

int lsn_vbios_init(struct loongson_drm_device *ldev)
{
	int i;
	int ret;
	unsigned char *vbios_start;
	struct loongson_vbios *pLsVbios = NULL;

#ifdef CONFIG_CPU_LOONGSON3
	pLsVbios = (struct loongson_vbios *)ls_get_vbios_addr();
	if (pLsVbios != NULL)
		ret = loongson_vbios_check(pLsVbios);
#endif

	if ((pLsVbios == NULL) || (ret < 0)) {
		DRM_INFO("Using the default VBIOS!\n");
		ldev->vbios = lsn_vbios_default(ldev);
	} else {
		DRM_INFO("Using the VBIOS from the firmware!\n");
		ldev->vbios = pLsVbios;
	}

	if (ldev->vbios == NULL)
		return -1;

	vbios_start = (unsigned char *)ldev->vbios;

	/* get crtc struct pointers */
	ldev->crtc_vbios[0] = (struct loongson_vbios_crtc *)(vbios_start +
			ldev->vbios->crtc_offset);
	if (ldev->vbios->crtc_num > 1) {
		for (i = 1; i < ldev->vbios->crtc_num; i++)
			ldev->crtc_vbios[i] = (struct loongson_vbios_crtc *)(
		vbios_start + ldev->crtc_vbios[i - 1]->next_crtc_offset);
	}

	/* get encoder struct pointers */
	ldev->encoder_vbios[0] = (struct loongson_vbios_encoder *)(
			vbios_start + ldev->vbios->encoder_offset);

	if (ldev->vbios->encoder_num > 1) {
		for (i = 1; i < ldev->vbios->encoder_num; i++)
			ldev->encoder_vbios[i] =
			(struct loongson_vbios_encoder *)(vbios_start +
			ldev->encoder_vbios[i - 1]->next_encoder_offset);
	}

	/* get connector struct pointers */
	ldev->connector_vbios[0] = (struct loongson_vbios_connector *)(
			vbios_start + ldev->vbios->connector_offset);

	if (ldev->vbios->connector_num > 1) {
		for (i = 1; i < ldev->vbios->connector_num; ++i)
			ldev->connector_vbios[i] =
			(struct loongson_vbios_connector *)(vbios_start +
			ldev->connector_vbios[i - 1]->next_connector_offset);
	}

	lsn_vbios_information_display(ldev);

	return 0;
}

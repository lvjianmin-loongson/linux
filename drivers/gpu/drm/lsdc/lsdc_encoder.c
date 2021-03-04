// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2020 Loongson Corporation
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
 *	Sui Jingfeng <suijingfeng@loongson.cn>
 */

#include <drm/drm_crtc_helper.h>

#include "lsdc_drv.h"
#include "lsdc_hardware.h"
#include "lsdc_vbios.h"
#include "lsdc_encoder.h"


#define to_loongson_encoder(x)          \
		container_of(x, struct loongson_encoder, base)


const char *encoder_type_to_string(const unsigned int T)
{
	switch (T) {
	case DRM_MODE_ENCODER_NONE:
		return "NONE";
	case DRM_MODE_ENCODER_DAC:
		return "DAC";
	case DRM_MODE_ENCODER_TMDS:
		return "TMDS";
	case DRM_MODE_ENCODER_LVDS:
		return "LVDS";
	case DRM_MODE_ENCODER_TVDAC:
		return "TVDAC";
	case DRM_MODE_ENCODER_VIRTUAL:
		return "VIRTUAL";
	case DRM_MODE_ENCODER_DSI:
		return "DSI";
	case DRM_MODE_ENCODER_DPMST:
		return "DPMST";
	case DRM_MODE_ENCODER_DPI:
		return "DPI";
	default:
		return "Unknown";
	}
}

static void lsn_encoder_destroy(struct drm_encoder *encoder)
{
	struct loongson_encoder *pLsEnc = to_loongson_encoder(encoder);

	DRM_INFO("Destroy Encoder ID: %d\n", pLsEnc->encoder_id);

	drm_encoder_cleanup(encoder);
	kfree(pLsEnc);
}


/**
 *
 * This callback is used to validate encoder state for atomic drivers.
 * Since the encoder is the object connecting the CRTC and connector
 * it gets passed both states, to be able to validate interactions and
 * update the CRTC to match what the encoder needs for the requested
 * connector.
 *
 * Since this provides a strict superset of the functionality of
 * @mode_fixup (the requested and adjusted modes are both available
 * through the passed in &struct drm_crtc_state)
 * @mode_fixup is not called when @atomic_check is implemented.
 *
 * This function is used by the atomic helpers, but it is optional.
 *
 * NOTE:
 *
 * This function is called in the check phase of an atomic update.
 * The driver is not allowed to change anything outside of the free-standing
 * state objects passed-in or assembled in the overall &drm_atomic_state
 * update tracking structure.
 *
 * Also beware that userspace can request its own custom modes,
 * neither core nor helpers filter modes to the list of probe modes
 * reported by the GETCONNECTOR IOCTL and stored in &drm_connector.modes.
 * To ensure that modes are filtered consistently put any encoder
 * constraints and limits checks into @mode_valid.
 *
 * RETURNS:
 *
 * 0 on success, -EINVAL if the state or the transition can't be supported,
 * -ENOMEM on memory allocation failure and -EDEADLK if an  attempt to
 * obtain another state object ran into a &drm_modeset_lock deadlock.
 */
static int lsn_encoder_atomic_check(struct drm_encoder *encoder,
				   struct drm_crtc_state *crtc_state,
				   struct drm_connector_state *conn_state)
{
	struct loongson_encoder *pLsEnc = to_loongson_encoder(encoder);

	DRM_DEBUG_KMS("%s: Encoder ID: %d\n", __func__, pLsEnc->encoder_id);

	return 0;
}

/**
 *
 * This callback is used to update the display mode of an encoder.
 *
 * Note that the display pipe is completely off when this function is called.
 * Drivers which need hardware to be running before they program the new
 * display mode (because they implement runtime PM) should not use this hook,
 * because the helper library calls it only once and not every time the display
 * pipeline is suspended using either DPMS or the new "ACTIVE" property.
 * Such drivers should instead move all their encoder setup into the
 * @enable callback.
 *
 * This callback is used by the atomic modeset helpers in place of the
 * @mode_set callback, if set by the driver. It is optional and should
 * be used instead of @mode_set if the driver needs to inspect the
 * connector state or display info, since there is no direct way to
 * go from the encoder to the current connector.
 */
static void lsn_encoder_atomic_mode_set(struct drm_encoder *encoder,
				       struct drm_crtc_state *crtc_state,
				       struct drm_connector_state *conn_state)
{
	struct loongson_encoder *pLsEnc = to_loongson_encoder(encoder);
	struct loongson_crtc *lcrtc_cur = to_loongson_crtc(crtc_state->crtc);
	struct loongson_crtc *lcrtc_origin = pLsEnc->lcrtc;
	struct loongson_drm_device *ldev;

	if (lcrtc_origin->crtc_id != lcrtc_cur->crtc_id) {
		DRM_INFO("%s: Current CRTC ID: %d, Original CRTC ID: %d\n",
			__func__, lcrtc_cur->crtc_id, lcrtc_origin->crtc_id);
		lcrtc_origin->cfg_reg |= CFG_PANELSWITCH;
	} else {
		DRM_INFO("%s: Current CRTC ID: %d, Original CRTC ID: %d\n",
			__func__, lcrtc_cur->crtc_id, lcrtc_origin->crtc_id);
		lcrtc_origin->cfg_reg &= ~CFG_PANELSWITCH;
	}

	ldev = lcrtc_origin->ldev;

	if (lcrtc_origin->crtc_id == 0)
		ls_reg_write32(ldev, LSDC_FB0_CFG_REG, lcrtc_origin->cfg_reg);
	else
		ls_reg_write32(ldev, LSDC_FB1_CFG_REG, lcrtc_origin->cfg_reg);

}

/**
 * These provide the minimum set of functions required to handle a encoder
 *
 * Helper operations for encoders
 */
static const struct drm_encoder_helper_funcs loongson_encoder_helper_funcs = {
	.atomic_check = lsn_encoder_atomic_check,
	.atomic_mode_set = lsn_encoder_atomic_mode_set,
};


static const struct drm_encoder_funcs loongson_encoder_encoder_funcs = {
	.destroy = lsn_encoder_destroy,
};


static int lsn_get_encoder_type_from_vbios(struct loongson_drm_device *pLsDev,
					   const unsigned int index)
{
	struct loongson_vbios_encoder *pVbiosEncoder = NULL;

	if (index <= 3)
		pVbiosEncoder = pLsDev->encoder_vbios[index];
	else {
		DRM_ERROR("%s: encoder index(%d) overflow.\n",
			__func__, index);
		return -1;
	}

	DRM_DEBUG_KMS("%s: encoder index=%d, type=%s\n",
			__func__, index,
			encoder_type_to_string(pVbiosEncoder->type));

	return pVbiosEncoder->type;
}

struct drm_encoder *lsn_encoder_init(struct drm_device *dev,
				     unsigned int encoder_id)
{
	struct drm_encoder *pEncoder;
	struct loongson_encoder *pLsEncoder;
	struct loongson_drm_device *pLsDev = dev->dev_private;
	int ret;

	pLsEncoder = kzalloc(sizeof(struct loongson_encoder), GFP_KERNEL);
	if (!pLsEncoder)
		return NULL;

	pLsEncoder->encoder_id = encoder_id;
	pLsEncoder->lcrtc = &pLsDev->lcrtc[encoder_id];
	pEncoder = &pLsEncoder->base;
	pEncoder->possible_crtcs = BIT(1) | BIT(0);
	pEncoder->possible_clones = BIT(1) | BIT(0);

	drm_encoder_helper_add(pEncoder, &loongson_encoder_helper_funcs);

	ret = drm_encoder_init(dev, pEncoder, &loongson_encoder_encoder_funcs,
		lsn_get_encoder_type_from_vbios(pLsDev, encoder_id), NULL);

	if (ret == 0)
		DRM_INFO("%s: Initial encoder %d successful.\n",
			__func__, encoder_id);
	else
		DRM_ERROR("%s: Initial encoder %d failed.\n",
			__func__, encoder_id);

	return pEncoder;
}

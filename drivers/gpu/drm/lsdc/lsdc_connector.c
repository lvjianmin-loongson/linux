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

#include <linux/pm_runtime.h>

#include <drm/drm_atomic_helper.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_edid.h>
#include <drm/drm_connector.h>

#include "lsdc_drv.h"
#include "lsdc_vbios.h"
#include "lsdc_hardware.h"
#include "lsdc_i2c.h"
#include "lsdc_encoder.h"
#include "lsdc_connector.h"


static const char *connector_type_to_string(const unsigned int T)
{
	switch (T) {
	case DRM_MODE_CONNECTOR_Unknown:
		return "Unknown";
	case DRM_MODE_CONNECTOR_VGA:
		return "VGA";
	case DRM_MODE_CONNECTOR_DVII:
		return "DVI-I";
	case DRM_MODE_CONNECTOR_DVID:
		return "DVI-D";
	case DRM_MODE_CONNECTOR_DVIA:
		return "DVI-A";
	case DRM_MODE_CONNECTOR_Composite:
		return "Composite";
	case DRM_MODE_CONNECTOR_SVIDEO:
		return "S-VIDEO";
	case DRM_MODE_CONNECTOR_LVDS:
		return "LVDS";
	case DRM_MODE_CONNECTOR_Component:
		return "Component";
	case DRM_MODE_CONNECTOR_9PinDIN:
		return "9PinDIN";
	case DRM_MODE_CONNECTOR_DisplayPort:
		return "DisplayPort";
	case DRM_MODE_CONNECTOR_HDMIA:
		return "HDMI-A";
	case DRM_MODE_CONNECTOR_HDMIB:
		return "HDMI-B";
	case DRM_MODE_CONNECTOR_TV:
		return "TV";
	case DRM_MODE_CONNECTOR_eDP:
		return "eDP";
	case DRM_MODE_CONNECTOR_VIRTUAL:
		return "VIRTUAL";
	case DRM_MODE_CONNECTOR_DSI:
		return "DSI";
	case DRM_MODE_CONNECTOR_DPI:
		return "DPI";
	case DRM_MODE_CONNECTOR_WRITEBACK:
		return "WRITEBACK";
	default:
		return "Unknown";
	}
}


static struct drm_encoder *lsn_connector_best_single_encoder(
				struct drm_connector *pCon)
{
	unsigned int i;
	const unsigned int enc_id = pCon->encoder_ids[0];
	const unsigned int con_id = drm_connector_index(pCon);
	struct drm_encoder *encoder;

	DRM_DEBUG_KMS("Encoders available for %s: %s-%u\n",
		pCon->name,
		connector_type_to_string(pCon->connector_type),
		con_id);

	drm_connector_for_each_possible_encoder(pCon, encoder, i) {
		/* list what is possible */
		DRM_DEBUG_KMS(" %s, type: %s, id: %u\n",
			encoder->name,
			encoder_type_to_string(encoder->encoder_type),
			encoder->index);
	}

	/* simply pick the first one */
	return drm_encoder_find(pCon->dev, NULL, enc_id);
}

/**
 * @mode_valid:
 *
 * Callback to validate a mode for a connector, irrespective of the
 * specific display configuration.
 *
 * This callback is used by the probe helpers to filter the mode list
 * (which is usually derived from the EDID data block from the sink).
 * See e.g. drm_helper_probe_single_connector_modes().
 *
 * This function is optional.
 *
 * NOTE:
 *
 * This only filters the mode list supplied to userspace in the
 * GETCONNECTOR IOCTL. Compared to &drm_encoder_helper_funcs.mode_valid,
 * &drm_crtc_helper_funcs.mode_valid and &drm_bridge_funcs.mode_valid,
 * which are also called by the atomic helpers from
 * drm_atomic_helper_check_modeset(). This allows userspace to force and
 * ignore sink constraint (like the pixel clock limits in the screen's
 * EDID), which is useful for e.g. testing, or working around a broken
 * EDID. Any source hardware constraint (which always need to be
 * enforced) therefore should be checked in one of the above callbacks,
 * and not this one here.
 *
 * To avoid races with concurrent connector state updates, the helper
 * libraries always call this with the &drm_mode_config.connection_mutex
 * held. Because of this it's safe to inspect &drm_connector->state.
 *
 * RETURNS:
 *
 * Either &drm_mode_status.MODE_OK or one of the failure reasons in &enum
 * drm_mode_status.
 */
static enum drm_mode_status lsn_connector_mode_valid(
				struct drm_connector *connector,
				struct drm_display_mode *mode)
{
	struct drm_device *dev = connector->dev;
	struct loongson_drm_device *lsp = to_loongson_private(dev);

	if (mode->clock > lsp->clock.max_pixel_clock) {
		DRM_WARN("%s: mode %dx%d %d clock too high\n",
			__func__, mode->hdisplay, mode->vdisplay, mode->clock);
		return MODE_CLOCK_HIGH;
	}

	if ((mode->hdisplay > LSDC_MAX_FB_WIDTH) &&
	    (mode->vdisplay > LSDC_MAX_FB_HEIGHT))
		return MODE_NOMODE;

	return MODE_OK;
}


static bool lsn_is_hardcode_edid(struct loongson_drm_device * const ldev,
				 const unsigned int con_id)
{
	enum loongson_edid_method method =
		ldev->connector_vbios[con_id]->edid_method;

	return method == edid_method_vbios;
}


/**
 * @get_modes:
 *
 * This function should fill in all modes currently valid for the sink
 * into the &drm_connector.probed_modes list. It should also update the
 * EDID property by calling drm_connector_update_edid_property().
 *
 * The usual way to implement this is to cache the EDID retrieved in the
 * probe callback somewhere in the driver-private connector structure.
 * In this function drivers then parse the modes in the EDID and add
 * them by calling drm_add_edid_modes().
 *
 * But connectors that driver a fixed panel can also manually add specific
 * modes using drm_mode_probed_add(). Drivers which manually add modes
 * should also make sure that the &drm_connector.display_info,
 * &drm_connector.width_mm and &drm_connector.height_mm fields are filled in.
 *
 * This function is only called after the @detect hook has indicated
 * that a sink is connected and when the EDID isn't overridden through
 * sysfs or the kernel commandline.
 *
 * This callback is used by the probe helpers in e.g.
 * drm_helper_probe_single_connector_modes().
 *
 * To avoid races with concurrent connector state updates, the helper
 * libraries always call this with the &drm_mode_config.connection_mutex
 * held. Because of this it's safe to inspect &drm_connector->state.
 *
 * RETURNS:
 *
 * The number of modes added by calling drm_mode_probed_add().
 */
static int lsn_get_modes(struct drm_connector *pdConnector)
{
	struct drm_device *dev = pdConnector->dev;
	struct loongson_drm_device *ldev = to_loongson_private(dev);
	int con_id = drm_connector_index(pdConnector);

	if (!lsn_is_hardcode_edid(ldev, con_id)) {
		struct edid *edid;

		DRM_DEBUG("%s: Read edid from connector-%d.\n",
			__func__, con_id);

		edid = drm_get_edid(pdConnector,
					&ldev->i2c_bus[con_id]->adapter);
		if (edid) {
			int ret;

			ret = drm_connector_update_edid_property(
					pdConnector, edid);

			if (ret != 0) {
				DRM_ERROR("failed update edid property\n");
				kfree(edid);
				return ret;
			}

			ret = drm_add_edid_modes(pdConnector, edid);
			kfree(edid);

			DRM_DEBUG("%d modes add.\n", ret);
			return ret;
		}

		DRM_WARN("%s: grab EDID data failed.\n", __func__);
		drm_connector_update_edid_property(pdConnector, NULL);
		return 0;
	}

	if (lsn_is_hardcode_edid(ldev, con_id)) {
		/*
		 * TODO: get a hard coded from the firmware
		 * memcpy(buf, lbios_connector->internal_edid, EDID_LENGTH*2);
		 */
		DRM_INFO("%s, Get edid from vbios.\n", __func__);
		return 1;
	}

	return 0;
}

static enum drm_connector_status lsn_connector_detect(
		struct drm_connector *connector, bool force)
{
	struct drm_device *dev = connector->dev;
	struct loongson_drm_device *ldev = to_loongson_private(dev);
	int index = drm_connector_index(connector);

	if (!drm_kms_helper_is_poll_worker()) {
		int r;

		r = pm_runtime_get_sync(dev->dev);
		if (r < 0) {
			pm_runtime_put_autosuspend(dev->dev);

			if (ldev->chip == LSDC_CHIP_7A1000)
				return connector_status_disconnected;
		}
	}

	if (!lsn_is_hardcode_edid(ldev, index)) {
		struct edid *edid;

		edid = drm_get_edid(connector, &ldev->i2c_bus[index]->adapter);
		if (edid) {
			/*
			 * A monitor is thought to be connected if we can
			 * read edid from it, but we don't take time to
			 * parse it.
			 */
			kfree(edid);

			return connector_status_connected;
		} else
			return connector_status_disconnected;
	}

	return connector_status_connected;
}

/**
 *
 * @connector: point to the drm_connector structure
 *
 * Clean up connector resources
 */
static void lsn_connector_destroy(struct drm_connector *connector)
{
	struct loongson_connector *pLsCon = to_loongson_connector(connector);

	DRM_INFO("%s: destroy connector\n", __func__);

	lsn_destroy_i2c(pLsCon->i2c);
	drm_connector_cleanup(connector);
	kfree(connector);
}


/**
 * These provide the minimum set of functions required to handle a connector
 *
 * Helper operations for connectors. These functions are used
 * by the atomic and legacy modeset helpers and by the probe helpers.
 */
static const struct drm_connector_helper_funcs loongson_connector_helpers = {
	.get_modes = lsn_get_modes,
	.best_encoder = lsn_connector_best_single_encoder,
	.mode_valid = lsn_connector_mode_valid,
};

/**
 * These provide the minimum set of functions required to handle a connector
 *
 * Control connectors on a given device.
 *
 * Each CRTC may have one or more connectors attached to it.
 * The functions below allow the core DRM code to control
 * connectors, enumerate available modes, etc.
 */
static const struct drm_connector_funcs loongson_connector_funcs = {
	.dpms = drm_helper_connector_dpms,
	.detect = lsn_connector_detect,
	.fill_modes = drm_helper_probe_single_connector_modes,
	.destroy = lsn_connector_destroy,
	.reset = drm_atomic_helper_connector_reset,
	.atomic_duplicate_state = drm_atomic_helper_connector_duplicate_state,
	.atomic_destroy_state = drm_atomic_helper_connector_destroy_state,
};


struct loongson_connector *lsn_connector_init(struct loongson_drm_device *ldev,
					      const unsigned int con_id)
{
	struct drm_connector *pdConnector;
	struct loongson_connector *pLsConnector;
	int ret;
	unsigned int i2c_id;

	pLsConnector = kzalloc(sizeof(struct loongson_connector), GFP_KERNEL);
	if (pLsConnector == NULL) {
		DRM_ERROR("loongson connector kzalloc failed.\n");
		return NULL;
	}

	pdConnector = &pLsConnector->base;

	i2c_id = lsn_get_i2c_id_from_vbios(ldev, con_id);

	if (ldev->chip == LSDC_CHIP_7A1000)
		ldev->i2c_bus[con_id] = ls7a_create_i2c_chan(ldev, i2c_id);
	else if (ldev->chip == LSDC_CHIP_2K1000)
		ldev->i2c_bus[con_id] = ls2k_create_i2c_chan(ldev, i2c_id);
	else if (ldev->chip == LSDC_CHIP_UNKNOWN) {
		DRM_ERROR("Unknown display controller model.\n");
		return NULL;
	}

	if (ldev->i2c_bus[con_id] == NULL) {
		DRM_ERROR("%s: connector-%d match i2c-%d failed.\n",
				__func__, con_id, i2c_id);
		return NULL;
	}

	DRM_INFO("%s: i2c-%d prepared for connector-%d done.\n",
		__func__, i2c_id, con_id);


	ret = drm_connector_init(ldev->dev, pdConnector,
			&loongson_connector_funcs,
			lsn_get_connector_type_from_vbios(ldev, con_id));

	if (ret)
		DRM_ERROR("init connector-%d failed.\n", con_id);

	drm_connector_helper_add(pdConnector, &loongson_connector_helpers);

	pdConnector->interlace_allowed = 0;
	pdConnector->doublescan_allowed = 0;
	pdConnector->polled = DRM_CONNECTOR_POLL_CONNECT |
				DRM_CONNECTOR_POLL_DISCONNECT;

	drm_connector_register(pdConnector);

	return pLsConnector;
}

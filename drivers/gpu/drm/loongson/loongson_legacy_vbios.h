/**
 * struct loongson_vbios - loongson vbios structure
 *
 * @driver_priv: Pointer to driver-private information.
 */
#ifndef __LOONGSON_LEGACY_VBIOS_H__
#define __LOONGSON_LEGACY_VBIOS_H__

#define LS_MAX_RESOLUTIONS 10
#define LS_MAX_REG_TABLE 256

struct loongson_vbios {
	char title[16];
	uint32_t version_major;
	uint32_t version_minor;
	char information[20];
	uint32_t crtc_num;
	uint32_t crtc_offset;
	uint32_t connector_num;
	uint32_t connector_offset;
	uint32_t encoder_num;
	uint32_t encoder_offset;
} __attribute__((packed));

enum loongson_crtc_version {
	default_version = 0,
	crtc_version_max = 0xffffffff,
} __attribute__((packed));

struct loongson_crtc_modeparameter {
	/* horizontal timing. */
	u32 htotal;
	u32 hdisplay;
	u32 hsync_start;
	u32 hsync_width;
	u32 hsync_pll;
	/* vertical timing. */
	u32 vtotal;
	u32 vdisplay;
	u32 vsync_start;
	u32 vsync_width;
	u32 vsync_pll;

	/* refresh timing. */
	s32 clock;
	u32 hfreq;
	u32 vfreq;

	/* clock phase. this clock phase only applies to panel. */
	u32 clock_phase;
};

struct encoder_conf_reg {
	unsigned char dev_addr;
	unsigned char reg;
	unsigned char value;
} __attribute__((packed));

struct encoder_resolution_config {
	unsigned char reg_num;
	struct encoder_conf_reg config_regs[LS_MAX_REG_TABLE];
} __attribute__((packed));

struct loongson_resolution_param {
	bool used;
	uint32_t hdisplay;
	uint32_t vdisplay;
};

struct loongson_crtc_config_param {
	struct loongson_resolution_param resolution;
	struct loongson_crtc_modeparameter crtc_resol_param;
};
struct encoder_config_param {
	struct loongson_resolution_param resolution;
	struct encoder_resolution_config encoder_param;
};

struct loongson_vbios_crtc {
	u32 next;
	u32 crtc_id;
	enum loongson_crtc_version crtc_version;
	u32 crtc_max_freq;
	u32 crtc_max_weight;
	u32 crtc_max_height;
	u32 connector_id;
	u32 phy_num;
	u32 encoder_id;
	u32 reserve;
	bool use_local_param;
	struct loongson_crtc_config_param mode_config_tables[LS_MAX_RESOLUTIONS];
} __attribute__((packed));

enum loongson_edid_method {
	via_null = 0,
	via_i2c,
	via_vbios,
	via_encoder,
	via_max = 0xffffffff,
} __attribute__((packed));

enum loongson_vbios_i2c_type {
	i2c_type_null = 0,
	i2c_type_gpio,
	i2c_type_cpu,
	i2c_type_encoder,
	i2c_type_max = 0xffffffff,
} __attribute__((packed));

enum hotplug {
	disable = 0,
	polling,
	irq,
	hotplug_max = 0xffffffff,
} __attribute__((packed));

enum encoder_config {
	encoder_transparent = 0,
	encoder_os_config,
	encoder_bios_config, //bios config encoder
	encoder_type_max = 0xffffffff,
} __attribute__((packed));

enum connector_type {
	connector_unknown,
	connector_vga,
	connector_dvii,
	connector_dvid,
	connector_dvia,
	connector_composite,
	connector_svideo,
	connector_lvds,
	connector_component,
	connector_9pindin,
	connector_displayport,
	connector_hdmia,
	connector_hdmib,
	connector_tv,
	connector_edp,
	connector_virtual,
	connector_dsi,
	connector_dpi
};

enum encoder_type {
	encoder_none,
	encoder_dac,
	encoder_tmds,
	encoder_lvds,
	encoder_tvdac,
	encoder_virtual,
	encoder_dsi,
	encoder_dpmst,
	encoder_dpi
};

enum gpio_placement {
	GPIO_PLACEMENT_LS3A = 0,
	GPIO_PLACEMENT_LS7A,
};

enum encoder_object {
	Unknown = 0x00,
	INTERNAL_DVO = 0x01,
	INTERNAL_HDMI = 0x02,
	VGA_CH7055 = 0x10,
	VGA_ADV7125 = 0x11,
	DVI_TFP410 = 0x20,
	HDMI_IT66121 = 0x30,
	HDMI_SIL9022 = 0x31,
	HDMI_LT8618 = 0x32,
	EDP_NCS8805 = 0x40
};

struct loongson_backlight_pwm {
	uint8_t pwm_id, polarity;
	uint32_t period_ns;
};

struct loongson_vbios_connector {
	uint32_t next;
	uint32_t crtc_id;
	enum loongson_edid_method edid_method;
	enum loongson_vbios_i2c_type i2c_type;
	uint32_t i2c_id;
	uint32_t encoder_id;
	enum connector_type type;
	enum hotplug hotplug;
	uint32_t hot_swap_irq;
	uint32_t edid_version;
	uint8_t internal_edid[256];
	struct loongson_backlight_pwm bl_pwm;
} __attribute__((packed));

struct loongson_vbios_encoder {
	uint32_t next;
	uint32_t crtc_id;
	uint32_t connector_id;
	uint32_t reserve;
	enum encoder_config config_type;
	enum loongson_vbios_i2c_type i2c_type;
	uint32_t i2c_id;
	enum encoder_type type;
	struct encoder_config_param encoder_config[LS_MAX_RESOLUTIONS];
} __attribute__((packed));

#endif

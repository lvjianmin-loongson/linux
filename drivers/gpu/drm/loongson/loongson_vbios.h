#ifndef __LOONGSON_VBIOS_H__
#define __LOONGSON_VBIOS_H__

#include "loongson_legacy_vbios.h"

#define VBIOS_PWM_ID 0x0
#define VBIOS_PWM_PERIOD 0x1
#define VBIOS_PWM_POLARITY 0x2

#define VBIOS_CRTC_ID 0x1
#define VBIOS_CRTC_ENCODER_ID 0x2
#define VBIOS_CRTC_MAX_FREQ 0x3
#define VBIOS_CRTC_MAX_WIDTH 0x4
#define VBIOS_CRTC_MAX_HEIGHT 0x5
#define VBIOS_CRTC_IS_VB_TIMING 0x6

#define VBIOS_CONNECTOR_I2C_ID 0x1
#define VBIOS_CONNECTOR_INTERNAL_EDID 0x2
#define VBIOS_CONNECTOR_TYPE 0x3
#define VBIOS_CONNECTOR_HOTPLUG 0x4
#define VBIOS_CONNECTOR_EDID_METHOD 0x5
#define VBIOS_CONNECTOR_IRQ_PLACEMENT 0x06
#define VBIOS_CONNECTOR_IRQ_GPIO 0x07

#define VBIOS_ENCODER_I2C_ID 0x1
#define VBIOS_ENCODER_CONNECTOR_ID 0x2
#define VBIOS_ENCODER_TYPE 0x3
#define VBIOS_ENCODER_CONFIG_TYPE 0x4
#define VBIOS_ENCODER_CONFIG_PARAM 0x1
#define VBIOS_ENCODER_CONFIG_NUM 0x2
#define VBIOS_ENCODER_CHIP 0x05
#define VBIOS_ENCODER_CHIP_ADDR 0x06

struct desc_node;
struct vbios_cmd;
typedef bool(parse_func)(struct desc_node *, struct vbios_cmd *);

enum desc_type {
	desc_header = 0,
	desc_crtc,
	desc_encoder,
	desc_connector,
	desc_i2c,
	desc_pwm,
	desc_gpio,
	desc_backlight,
	desc_fan,
	desc_irq_vblank,
	desc_cfg_encoder,
	desc_max = 0xffff
};

enum vbios_backlight_type { bl_unuse, bl_ec, bl_pwm };

enum desc_ver {
	ver_v1,
};

struct vbios_header {
	u32 feature;
	u8 oem_vendor[32];
	u8 oem_product[32];
	u32 legacy_offset;
	u32 legacy_size;
	u32 desc_offset;
	u32 desc_size;
	u32 data_offset;
	u32 data_size;
} __attribute__((packed));

struct vbios_backlight {
	u32 feature;
	u8 used;
	enum vbios_backlight_type type;
} __attribute__((packed));

enum i2c_type { i2c_cpu, i2c_gpio, i2c_max = -1 };

struct vbios_i2c {
	u32 feature;
	u16 id;
	enum i2c_type type;
} __attribute__((packed));

struct vbios_pwm {
	u32 feature;
	u8 pwm;
	u8 polarity;
	u32 peroid;
} __attribute__((packed));

struct vbios_desc {
	u16 type;
	u8 ver;
	u8 index;
	u32 offset;
	u32 size;
	u64 ext[2];
} __attribute__((packed));

struct vbios_cmd {
	u8 index;
	enum desc_type type;
	u64 *req;
	void *res;
};

#define FUNC(t, v, f)                                                          \
	{                                                                      \
		.type = t, .ver = v, .func = f,                                \
	}

struct desc_func {
	enum desc_type type;
	u16 ver;
	s8 *name;
	u8 index;
	parse_func *func;
};

struct desc_node {
	struct list_head head;
	u8 *data;
	struct vbios_desc *desc;
	parse_func *parse;
};

struct vbios_encoder {
	u32 feature;
	u32 i2c_id;
	u32 connector_id;
	enum encoder_type type;
	enum encoder_config config_type;
	enum encoder_object chip;
	u8 chip_addr;
} __attribute__((packed));

struct vbios_connector {
	u32 feature;
	u32 i2c_id;
	u8 internal_edid[256];
	enum connector_type type;
	enum hotplug hotplug;
	enum loongson_edid_method edid_method;
	u32 irq_gpio;
	enum gpio_placement gpio_placement;
} __attribute__((packed));

struct vbios_crtc {
	u32 feature;
	u32 crtc_id;
	u32 encoder_id;
	u32 max_freq;
	u32 max_width;
	u32 max_height;
	bool is_vb_timing;
} __attribute__((packed));

struct vbios_conf_reg {
	u8 dev_addr;
	u8 reg;
	u8 value;
} __attribute__((packed));

struct vbios_cfg_encoder {
	u32 hdisplay;
	u32 vdisplay;
	u8 reg_num;
	struct vbios_conf_reg config_regs[256];
} __attribute__((packed));

bool loongson_vbios_init(struct loongson_device *ldev);
void loongson_vbios_exit(struct loongson_device *ldev);
u32 get_connector_type(struct loongson_device *ldev, u32 index);
u16 get_connector_i2cid(struct loongson_device *ldev, u32 index);
u16 get_hotplug_mode(struct loongson_device *ldev, u32 index);
u8 *get_vbios_edid(struct loongson_device *ldev, u32 index);
u16 get_edid_method(struct loongson_device *ldev, u32 index);
u32 get_vbios_pwm(struct loongson_device *ldev, u32 index, u16 request);
u32 get_crtc_id(struct loongson_device *ldev, u32 index);
u32 get_crtc_max_freq(struct loongson_device *ldev, u32 index);
u32 get_crtc_max_width(struct loongson_device *ldev, u32 index);
u32 get_crtc_max_height(struct loongson_device *ldev, u32 index);
u32 get_crtc_encoder_id(struct loongson_device *ldev, u32 index);
bool get_crtc_is_vb_timing(struct loongson_device *ldev, u32 index);

struct crtc_timing *get_crtc_timing(struct loongson_device *ldev, u32 index);

u32 get_encoder_connector_id(struct loongson_device *ldev, u32 index);
u32 get_encoder_i2c_id(struct loongson_device *ldev, u32 index);
enum encoder_config get_encoder_config_type(struct loongson_device *ldev,
					    u32 index);
enum encoder_type get_encoder_type(struct loongson_device *ldev, u32 index);
struct cfg_encoder *get_encoder_config(struct loongson_device *ldev, u32 index);
u32 get_encoder_cfg_num(struct loongson_device *ldev, u32 index);
bool get_loongson_i2c(struct loongson_device *ldev);
enum encoder_object get_encoder_chip(struct loongson_device *ldev, u32 index);
u8 get_encoder_chip_addr(struct loongson_device *ldev, u32 index);
u32 get_connector_irq_gpio(struct loongson_device *ldev, u32 index);
enum gpio_placement get_connector_gpio_placement(struct loongson_device *ldev,
						 u32 index);
#endif

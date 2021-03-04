#ifndef __LOONGSON_DRV_H__
#define __LOONGSON_DRV_H__

#include <linux/types.h>
#include <video/vga.h>
#include <drm/ttm/ttm_bo_driver.h>
#include <drm/drmP.h>
#include <drm/drm_encoder.h>
#include <drm/drm_gem.h>
#include <drm/drm_fb_helper.h>
#include <drm/drm_crtc_helper.h>
#include <drm/loongson_drm.h>
#include <loongson-pch.h>
#include "loongson_i2c.h"

#define to_loongson_crtc(x) container_of(x, struct loongson_crtc, base)
#define to_loongson_encoder(x) container_of(x, struct loongson_encoder, base)
#define to_loongson_connector(x)                                               \
	container_of(x, struct loongson_connector, base)
#define to_loongson_framebuffer(x)                                             \
	container_of(x, struct loongson_framebuffer, base)

#define LS_MAX_MODE_INFO 6
#define LOONGSON_MAX_FB_HEIGHT 4096
#define LOONGSON_MAX_FB_WIDTH 4096

#define CUR_WIDTH_SIZE 32
#define CUR_HEIGHT_SIZE 32

#define LOONGSON_BL_MAX_LEVEL 100
#define LOONGSON_BL_MIN_LEVEL 1
#define LOONGSON_BL_DEF_LEVEL 60

#define LOONGSON_GPIO_LCD_EN 62
#define LOONGSON_GPIO_LCD_VDD 63

#define LO_OFF 0
#define HI_OFF 8

#define gem_to_loongson_bo(gobj) container_of((gobj), struct loongson_bo, gem)

#define LS_PIX_PLL (0x04b0)

#define CURIOSET_CORLOR (0x4607)
#define CURIOSET_POSITION (0x4608)
#define CURIOLOAD_ARGB (0x4609)
#define CURIOLOAD_IMAGE (0x460A)
#define CURIOHIDE_SHOW (0x460B)
#define FBEDID_GET (0X860C)

#define REG_OFFSET (0x10)
#define LS_FB_CFG_REG (0x1240)
#define LS_FB_ADDR0_REG (0x1260)
#define LS_FB_ADDR1_REG (0x1580)
#define LS_FB_STRI_REG (0x1280)

#define LS_FB_DITCFG_REG (0x1360)
#define LS_FB_DITTAB_LO_REG (0x1380)
#define LS_FB_DITTAB_HI_REG (0x13a0)
#define LS_FB_PANCFG_REG (0x13c0)
#define LS_FB_PANTIM_REG (0x13e0)

#define LS_FB_HDISPLAY_REG (0x1400)
#define LS_FB_HSYNC_REG (0x1420)
#define LS_FB_VDISPLAY_REG (0x1480)
#define LS_FB_VSYNC_REG (0x14a0)

#define LS_FB_GAMINDEX_DVO0_REG (0x14e0)
#define LS_FB_GAMINDEX_DVO1_REG (0x14f0)
#define LS_FB_GAMDATA_DVO0_REG (0x1500)
#define LS_FB_GAMDATA_DVO1_REG (0x1510)

#define LS_FB_CUR_CFG_REG (0x1520)
#define LS_FB_CUR_ADDR_REG (0x1530)
#define LS_FB_CUR_LOC_ADDR_REG (0x1540)
#define LS_FB_CUR_BACK_REG (0x1550)
#define LS_FB_CUR_FORE_REG (0x1560)

#define LS_FB_INT_REG (0x1570)

#define LS_FB_ADDR1_DVO0_REG (0x1580)
#define LS_FB_ADDR1_DVO1_REG (0x1590)

/*offset*/
#define LS_FB_CFG_FORMAT32 (1 << 2)
#define LS_FB_CFG_FORMAT16 (11 << 0)
#define LS_FB_CFG_FORMAT15 (1 << 1)
#define LS_FB_CFG_FORMAT12 (1 << 0)
#define LS_FB_CFG_FB_SWITCH (1 << 7)
#define LS_FB_CFG_ENABLE (1 << 8)
#define LS_FB_CFG_SWITCH_PANEL (1 << 9)
#define LS_FB_CFG_GAMMA (1 << 12)
#define LS_FB_CFG_RESET (1 << 20)

#define LS_FB_PANCFG_BASE 0x80001010
#define LS_FB_PANCFG_DE (1 << 0)
#define LS_FB_PANCFG_DEPOL (1 << 1)
#define LS_FB_PANCFG_CLKEN (1 << 8)
#define LS_FB_PANCFG_CLKPOL (1 << 9)

#define LS_FB_HSYNC_POLSE (1 << 30)
#define LS_FB_HSYNC_POL (1 << 31)
#define LS_FB_VSYNC_POLSE (1 << 30)
#define LS_FB_VSYNC_POL (1 << 31)

#define LS_FB_VSYNC1_ENABLE (1 << 16)
#define LS_FB_HSYNC1_ENABLE (1 << 17)
#define LS_FB_VSYNC0_ENABLE (1 << 18)
#define LS_FB_HSYNC0_ENABLE (1 << 19)

struct loongson_connector;

struct pix_pll {
	unsigned int l2_div;
	unsigned int l1_loopc;
	unsigned int l1_frefc;
};

struct loongson_mc {
	resource_size_t aper_base;
	resource_size_t aper_size;
	u32 mc_mask;

	u32 vram_start;
	u32 vram_end;
	u32 vram_size;

	u32 gtt_start;
	u32 gtt_end;
	u32 gtt_size;
};

struct loongson_backlight {
	struct backlight_device *device;
	struct pwm_device *pwm;
	u32 pwm_id;
	u32 pwm_polarity;
	u32 pwm_period;
	bool present;
	bool hw_enabled;
	unsigned int level, max, min;

	int (*get_resource)(struct loongson_connector *ls_connector);
	void (*free_resource)(struct loongson_connector *ls_connector);
	int (*setup)(struct loongson_connector *ls_connector);
	unsigned int (*get_brightness)(struct loongson_connector *ls_connector);
	void (*set_brightness)(struct loongson_connector *ls_connector,
			       unsigned int level);
	void (*enable)(struct loongson_connector *ls_connector);
	void (*disable)(struct loongson_connector *ls_connector);
	void (*power)(struct loongson_connector *ls_connector, bool enable);
};

struct loongson_bo {
	struct ttm_buffer_object bo;
	struct ttm_placement placement;
	struct ttm_bo_kmap_obj kmap;
	struct drm_gem_object gem;
	struct ttm_place placements[3];
	int pin_count;
};

struct loongson_timing {
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

struct crtc_timing {
	u32 num;
	struct loongson_timing *tables;
};

struct loongson_framebuffer {
	struct drm_framebuffer base;
	struct drm_gem_object *obj;
};

enum loongson_flip_status {
	LOONGSON_FLIP_NONE,
	LOONGSON_FLIP_PENDING,
	LOONGSON_FLIP_SUBMITTED
};

struct loongson_cursor {
	u32 width, height; /* cursor width and height by User-defined */
	u32 x, y;
	u32 hot_x, hot_y;
	struct loongson_bo *bo;
	u64 paddr;
};

struct loongson_crtc {
	struct drm_crtc base;
	struct loongson_device *ldev;
	u32 crtc_id;
	u32 crtc_offset;
	s32 width;
	s32 height;
	s32 last_dpms;
	bool enabled;
	struct loongson_flip_work *pflip_works;
	enum loongson_flip_status pflip_status;
	u32 encoder_id;
	u32 max_freq;
	u32 max_width;
	u32 max_height;
	struct loongson_cursor cursor;
	bool is_vb_timing;
	struct crtc_timing *timing;
};

struct loongson_flip_work {
	struct delayed_work flip_work;
	struct loongson_device *ldev;
	int crtc_id;
	u32 target_vblank;
	uint64_t base;
	struct drm_pending_vblank_event *event;
	struct loongson_bo *old_bo;
};

struct loongson_irq {
	bool installed;
	spinlock_t lock;
};

struct config_reg {
	u8 dev_addr;
	u8 reg;
	u8 value;
} __packed;

struct cfg_encoder {
	u32 hdisplay;
	u32 vdisplay;
	u8 reg_num;
	struct config_reg config_regs[256];
};

struct loongson_encoder {
	struct drm_encoder base;
	struct loongson_device *ldev;
	struct loongson_i2c *i2c;
	struct cfg_encoder *encoder_config;
	struct drm_display_mode mode;
	int last_dpms;
	u32 cfg_num;
	u32 i2c_id;
	u32 encoder_id;
	u32 connector_id;
	u32 config_type;
	u32 type;
	bool (*mode_set_method)(struct loongson_encoder *ls_encoder,
				struct drm_display_mode *mode);
};

struct loongson_connector {
	struct drm_connector base;
	struct loongson_device *ldev;
	u16 id;
	u32 type;
	u16 i2c_id;
	u16 hotplug;
	u16 edid_method;
	u8 *vbios_edid;
	struct loongson_i2c *i2c;
	struct loongson_backlight bl;
};

struct loongson_mode_info {
	bool mode_config_initialized;
	struct loongson_device *ldev;
	struct loongson_crtc *crtc;
	struct loongson_encoder *encoder;
	struct loongson_connector *connector;
};

struct loongson_fbdev {
	struct drm_fb_helper helper;
	struct loongson_framebuffer lfb;
	void *sysram;
	int size;
	struct ttm_bo_kmap_obj mapping;
	int x1, y1, x2, y2; /* dirty rect */
	spinlock_t dirty_lock;
};

struct loongson_device {
	struct drm_device *dev;

	resource_size_t rmmio_base;
	resource_size_t rmmio_size;
	void __iomem *rmmio;
	void __iomem *io;

	struct loongson_mc mc;
	struct loongson_mode_info mode_info[LS_MAX_MODE_INFO];

	struct loongson_fbdev *lfbdev;

	struct pci_dev *vram_pdev; /**< PCI device structure */

	bool suspended;
	int num_crtc;
	int cursor_crtc_id;
	int has_sdram;
	struct drm_display_mode mode;

	struct loongson_irq irq;
	struct drm_property *rotation_prop;
	void *vbios;
	struct list_head desc_list;
	struct loongson_i2c i2c_bus[DC_I2C_BUS_MAX];
	struct gpio_chip chip;
	int fb_mtrr;

	struct {
		struct drm_global_reference mem_global_ref;
		struct ttm_bo_global_ref bo_global_ref;
		struct ttm_bo_device bdev;
	} ttm;
	struct {
		resource_size_t start;
		resource_size_t size;
	} vram;
	unsigned long fb_vram_base;
	bool clone_mode;
	bool inited;
	unsigned int vsync0_count;
	unsigned int vsync1_count;
	unsigned int pageflip_count;

	spinlock_t mmio_lock;
};

/*irq*/
int loongson_irq_enable_vblank(struct drm_device *dev, unsigned int crtc_id);
void loongson_irq_disable_vblank(struct drm_device *dev, unsigned int crtc_id);
u32 loongson_crtc_vblank_count(struct drm_device *dev, unsigned int pipe);
irqreturn_t loongson_irq_handler(int irq, void *arg);
void loongson_irq_preinstall(struct drm_device *dev);
int loongson_irq_postinstall(struct drm_device *dev);
void loongson_irq_uninstall(struct drm_device *dev);
int loongson_irq_init(struct loongson_device *ldev);
int loongson_pageflip_irq(struct loongson_device *ldev, unsigned int crtc_id);
void loongson_flip_work_func(struct work_struct *__work);

int loongson_ttm_init(struct loongson_device *ldev);
void loongson_ttm_fini(struct loongson_device *ldev);
void loongson_ttm_placement(struct loongson_bo *bo, int domain);
int loongson_bo_create(struct drm_device *dev, int size, int align,
		       uint32_t flags, struct loongson_bo **ploongsonbo);
int loongson_drm_mmap(struct file *filp, struct vm_area_struct *vma);
int loongson_bo_reserve(struct loongson_bo *bo, bool no_wait);
void loongson_bo_unreserve(struct loongson_bo *bo);

struct loongson_encoder *loongson_encoder_init(struct loongson_device *dev,
					       int index);
struct loongson_crtc *loongson_crtc_init(struct loongson_device *ldev,
					 int index);
struct loongson_connector *loongson_connector_init(struct loongson_device *dev,
						   int index);
int loongson_bridge_bind(struct loongson_device *ldev,
			 struct loongson_encoder *ls_encoder, int index);
void loongson_set_start_address(struct drm_crtc *crtc, unsigned offset);

int loongson_bo_push_sysram(struct loongson_bo *bo);
int loongson_bo_pin(struct loongson_bo *bo, u32 pl_flag, u64 *gpu_addr);
int loongson_bo_unpin(struct loongson_bo *bo);
u64 loongson_bo_gpu_offset(struct loongson_bo *bo);
void loongson_bo_unref(struct loongson_bo **bo);
int loongson_gem_create(struct drm_device *dev, u32 size, bool iskernel,
			struct drm_gem_object **obj);
int loongson_framebuffer_init(struct drm_device *dev,
			      struct loongson_framebuffer *lfb,
			      const struct drm_mode_fb_cmd2 *mode_cmd,
			      struct drm_gem_object *obj);

int loongson_fbdev_init(struct loongson_device *ldev);
void loongson_fbdev_fini(struct loongson_device *ldev);
bool loongson_fbdev_lobj_is_fb(struct loongson_device *ldev,
			       struct loongson_bo *lobj);
void loongson_fbdev_restore_mode(struct loongson_device *ldev);
void loongson_fbdev_set_suspend(struct loongson_device *ldev, int state);

int loongson_drm_suspend(struct drm_device *dev);
int loongson_drm_resume(struct drm_device *dev);
/* loongson_cursor.c */
int loongson_crtc_cursor_set2(struct drm_crtc *crtc, struct drm_file *file_priv,
			      uint32_t handle, uint32_t width, uint32_t height,
			      int32_t hot_x, int32_t hot_y);
int loongson_crtc_cursor_move(struct drm_crtc *crtc, int x, int y);

void loongson_encoder_resume(struct loongson_device *ldev);
bool loongson_encoder_reset_3a3k(struct loongson_encoder *ls_encoder,
				 struct drm_display_mode *mode);
void loongson_connector_resume(struct loongson_device *ldev);

u32 ls_mm_rreg(struct loongson_device *ldev, u32 offset);
void ls_mm_wreg(struct loongson_device *ldev, u32 offset, u32 val);
u32 ls_mm_rreg_locked(struct loongson_device *ldev, u32 offset);
void ls_mm_wreg_locked(struct loongson_device *ldev, u32 offset, u32 val);

u32 ls_io_rreg(struct loongson_device *ldev, u32 offset);
void ls_io_wreg(struct loongson_device *ldev, u32 offset, u32 val);
u32 ls_io_rreg_locked(struct loongson_device *ldev, u32 offset);
void ls_io_wreg_locked(struct loongson_device *ldev, u32 offset, u32 val);

void loongson_connector_power_mode(struct loongson_connector *ls_connector,
				   int mode);
int loongson_debugfs_init(struct drm_minor *minor);
int loongson_dc_gpio_init(struct loongson_device *ldev);
#endif

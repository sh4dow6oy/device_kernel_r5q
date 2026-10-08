/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2013-2019, The Linux Foundation. All rights reserved.
 */

#ifndef __MDSS_PLL_H
#define __MDSS_PLL_H

#include <linux/clk-provider.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/clkdev.h>
#include <linux/regmap.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/workqueue.h>
#include <linux/notifier.h>
#include <linux/types.h>

#include "clk-regmap.h"
#include "clk-regmap-divider.h"
#include "clk-regmap-mux.h"

#if defined(CONFIG_DRM)
#include <linux/sde_io_util.h>
#else
#include <linux/mdss_io_util.h>
#endif


/* ========================================================= */
/* PLL register access                                       */
/* ========================================================= */

#define MDSS_PLL_REG_W(base, offset, data) \
	writel_relaxed((data), (base) + (offset))

#define MDSS_PLL_REG_R(base, offset) \
	readl_relaxed((base) + (offset))


#define PLL_CALC_DATA(addr0, addr1, data0, data1) \
	(((data1) << 24) | \
	 ((((addr1) / 4) & 0xFF) << 16) | \
	 ((data0) << 8) | \
	 (((addr0) / 4) & 0xFF))


#define MDSS_DYN_PLL_REG_W(base, offset, addr0, addr1, data0, data1) \
	writel_relaxed( \
		PLL_CALC_DATA(addr0, addr1, data0, data1), \
		(base) + (offset))


#define upper_8_bit(x) \
	((((x) >> 2) & 0x100) >> 8)


/* ========================================================= */
/* PLL types                                                 */
/* ========================================================= */

enum {
	MDSS_DSI_PLL_10NM,
	MDSS_DP_PLL_10NM,

	MDSS_DSI_PLL_7NM,
	MDSS_DSI_PLL_7NM_V2,
	MDSS_DSI_PLL_7NM_V4_1,

	MDSS_DP_PLL_7NM,
	MDSS_DP_PLL_7NM_V2,

	MDSS_DSI_PLL_28LPM,

	MDSS_DSI_PLL_14NM,
	MDSS_DP_PLL_14NM,

	MDSS_HDMI_PLL_28LPM,

	MDSS_DSI_PLL_12NM,

	MDSS_UNKNOWN_PLL,
};


/* ========================================================= */
/* PLL target IDs                                            */
/* ========================================================= */

enum {
	MDSS_PLL_TARGET_8996,
	MDSS_PLL_TARGET_SDM660,
};


/* ========================================================= */
/* Dynamic FPS                                               */
/* ========================================================= */

#define DFPS_MAX_NUM_OF_FRAME_RATES	16


struct dfps_pll_codes {
	uint32_t pll_codes_1;
	uint32_t pll_codes_2;
	uint32_t pll_codes_3;
};


struct dfps_codes_info {
	uint32_t is_valid;
	uint32_t clk_rate;
	struct dfps_pll_codes pll_codes;
};


struct dfps_info {
	uint32_t vco_rate_cnt;

	struct dfps_codes_info
		codes_dfps[DFPS_MAX_NUM_OF_FRAME_RATES];
};


/* ========================================================= */
/* PLL resources                                             */
/* ========================================================= */

struct mdss_pll_resources {

	/*
	 * PLL-specific resources:
	 * GPIO, power supplies, clocks, etc.
	 */
	struct dss_module_power mp;

	/*
	 * PLL base register mappings.
	 */
	void __iomem *pll_base;
	void __iomem *phy_base;

	void __iomem *ln_tx0_base;
	void __iomem *ln_tx1_base;

	void __iomem *gdsc_base;
	void __iomem *dyn_pll_base;

	bool is_init_locked;

	/*
	 * VCO information.
	 */
	s64 vco_current_rate;
	s64 vco_locking_rate;
	s64 vco_ref_clk_rate;

	/*
	 * Cached VCO rate.
	 */
	unsigned long vco_cached_rate;

	u32 cached_cfg0;
	u32 cached_cfg1;
	u32 cached_outdiv;

	u32 cached_postdiv1;
	u32 cached_postdiv3;
	u32 cached_vreg_cfg;

	/*
	 * DSI / DP / HDMI PLL interface type.
	 */
	u32 pll_interface_type;

	/*
	 * Target ID.
	 */
	u32 target_id;

	/*
	 * HW recommended delay during VCO configuration.
	 */
	u32 vco_delay;

	/*
	 * PLL resource reference count.
	 */
	u32 resource_ref_cnt;

	/*
	 * Current resource state.
	 */
	bool resource_enable;

	/*
	 * PLL state.
	 */
	bool pll_on;

	/*
	 * PLL was already enabled by bootloader.
	 */
	bool handoff_resources;

	/*
	 * Cached PLL trim codes.
	 */
	int cache_pll_trim_codes[3];

	/*
	 * Trim-code update state.
	 */
	bool reg_upd;

	/*
	 * GDSC notifier callback.
	 */
	struct notifier_block gdsc_cb;

	/*
	 * PLL off worker.
	 */
	struct work_struct pll_off;

	/*
	 * PLL index.
	 */
	uint32_t index;

	/*
	 * Spread spectrum clocking.
	 */
	bool ssc_en;
	bool ssc_center;

	u32 ssc_freq;
	u32 ssc_ppm;

	/*
	 * Slave PLL.
	 */
	struct mdss_pll_resources *slave;

	/*
	 * PLL hardware revision.
	 */
	int revision;

	/*
	 * PLL-specific private data.
	 */
	void *priv;

	/*
	 * Dynamic refresh PLL codes.
	 */
	struct dfps_info *dfps;

	/*
	 * DFPS trigger state.
	 */
	bool dfps_trigger;
};


/* ========================================================= */
/* VCO calculation                                           */
/* ========================================================= */

struct mdss_pll_vco_calc {
	s32 div_frac_start1;
	s32 div_frac_start2;
	s32 div_frac_start3;

	s64 dec_start1;
	s64 dec_start2;

	s64 pll_plllock_cmp1;
	s64 pll_plllock_cmp2;
	s64 pll_plllock_cmp3;
};


/* ========================================================= */
/* GDSC helper                                               */
/* ========================================================= */

static inline bool is_gdsc_disabled(
		struct mdss_pll_resources *pll_res)
{
	bool ret;

	if (!pll_res || !pll_res->gdsc_base) {
		WARN(1, "gdsc_base register is not defined\n");
		return true;
	}

	if ((pll_res->target_id == MDSS_PLL_TARGET_SDM660) ||
	    (pll_res->pll_interface_type == MDSS_DSI_PLL_12NM)) {

		ret = ((readl_relaxed(pll_res->gdsc_base + 0x4) &
			BIT(31)) &&
		       (!(readl_relaxed(pll_res->gdsc_base) &
			  BIT(0)))) ?
			false : true;

	} else {
		ret = readl_relaxed(pll_res->gdsc_base) &
		      BIT(31) ? false : true;
	}

	return ret;
}


/* ========================================================= */
/* Clock divider helper                                      */
/* ========================================================= */

static inline int mdss_pll_div_prepare(struct clk_hw *hw)
{
	struct clk_hw *parent_hw;

	if (!hw || !hw->init || !hw->init->ops ||
	    !hw->init->ops->set_rate)
		return -EINVAL;

	parent_hw = clk_hw_get_parent(hw);

	if (!parent_hw)
		return -EINVAL;

	return hw->init->ops->set_rate(
		hw,
		clk_hw_get_rate(hw),
		clk_hw_get_rate(parent_hw));
}


/* ========================================================= */
/* Regmap mux helpers                                        */
/* ========================================================= */

static inline int mdss_set_mux_sel(void *context,
				   unsigned int reg,
				   unsigned int val)
{
	return 0;
}


static inline int mdss_get_mux_sel(void *context,
				   unsigned int reg,
				   unsigned int *val)
{
	if (!val)
		return -EINVAL;

	*val = 0;

	return 0;
}


/* ========================================================= */
/* Resource API                                              */
/* ========================================================= */

int mdss_pll_resource_enable(
		struct mdss_pll_resources *pll_res,
		bool enable);

int mdss_pll_util_resource_init(
		struct platform_device *pdev,
		struct mdss_pll_resources *pll_res);

void mdss_pll_util_resource_deinit(
		struct platform_device *pdev,
		struct mdss_pll_resources *pll_res);

void mdss_pll_util_resource_release(
		struct platform_device *pdev,
		struct mdss_pll_resources *pll_res);

int mdss_pll_util_resource_enable(
		struct mdss_pll_resources *pll_res,
		bool enable);

int mdss_pll_util_resource_parse(
		struct platform_device *pdev,
		struct mdss_pll_resources *pll_res);

void mdss_pll_util_parse_dt_dfps(
		struct platform_device *pdev,
		struct mdss_pll_resources *pll_res);

struct dss_vreg *mdss_pll_get_mp_by_reg_name(
		struct mdss_pll_resources *pll_res,
		char *name);

#endif /* __MDSS_PLL_H */

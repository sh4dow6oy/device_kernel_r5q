// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2013-2019, The Linux Foundation. All rights reserved.
 */

#define pr_fmt(fmt) "%s: " fmt, __func__

#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/string.h>
#include <linux/of_address.h>
#include <linux/dma-mapping.h>
#include <linux/vmalloc.h>
#include <linux/memblock.h>

#include "pll_drv.h"

/**
 * mdss_pll_get_mp_by_reg_name() - Find power module by regulator name
 * @pll_res: Pointer to the PLL resource
 * @name: Regulator name as specified in the PLL DTSI
 */
struct dss_vreg *mdss_pll_get_mp_by_reg_name(
		struct mdss_pll_resources *pll_res, char *name)
{
	struct dss_vreg *regulator;
	int i;

	if (!pll_res || !pll_res->mp.vreg_config) {
		pr_err("Invalid PLL resource\n");
		return NULL;
	}

	regulator = pll_res->mp.vreg_config;

	for (i = 0; i < pll_res->mp.num_vreg; i++) {
		if (!strcmp(name, regulator->vreg_name)) {
			pr_debug("Found regulator match for %s\n", name);
			return regulator;
		}

		regulator++;
	}

	return NULL;
}

int mdss_pll_util_resource_enable(struct mdss_pll_resources *pll_res,
				   bool enable)
{
	int rc = 0;
	struct dss_module_power *mp;

	if (!pll_res)
		return -EINVAL;

	mp = &pll_res->mp;

	if (enable) {
		rc = msm_dss_enable_vreg(mp->vreg_config,
					 mp->num_vreg, true);
		if (rc) {
			pr_err("Failed to enable vregs rc=%d\n", rc);
			return rc;
		}

		rc = msm_dss_clk_set_rate(mp->clk_config,
					  mp->num_clk);
		if (rc) {
			pr_err("Failed to set clock rate rc=%d\n", rc);
			goto vreg_err;
		}

		rc = msm_dss_enable_clk(mp->clk_config,
					mp->num_clk, true);
		if (rc) {
			pr_err("Clock enable failed rc=%d\n", rc);
			goto vreg_err;
		}
	} else {
		msm_dss_enable_clk(mp->clk_config,
				   mp->num_clk, false);

		msm_dss_enable_vreg(mp->vreg_config,
				    mp->num_vreg, false);
	}

	return 0;

vreg_err:
	msm_dss_enable_vreg(mp->vreg_config,
			    mp->num_vreg, false);

	return rc;
}

static int mdss_pll_util_parse_dt_supply(
		struct platform_device *pdev,
		struct mdss_pll_resources *pll_res)
{
	int i = 0;
	int rc = 0;
	u32 tmp = 0;
	struct device_node *of_node;
	struct device_node *supply_root_node;
	struct device_node *supply_node;
	struct dss_module_power *mp = &pll_res->mp;

	of_node = pdev->dev.of_node;

	mp->num_vreg = 0;
	supply_root_node = of_get_child_by_name(
			of_node,
			"qcom,platform-supply-entries");

	if (!supply_root_node) {
		pr_debug("no supply entry present\n");
		return 0;
	}

	for_each_child_of_node(supply_root_node, supply_node)
		mp->num_vreg++;

	if (!mp->num_vreg) {
		pr_debug("no vreg\n");
		of_node_put(supply_root_node);
		return 0;
	}

	pr_debug("vreg found. count=%d\n", mp->num_vreg);

	mp->vreg_config = devm_kzalloc(
			&pdev->dev,
			sizeof(*mp->vreg_config) * mp->num_vreg,
			GFP_KERNEL);

	if (!mp->vreg_config) {
		mp->num_vreg = 0;
		of_node_put(supply_root_node);
		return -ENOMEM;
	}

	for_each_child_of_node(supply_root_node, supply_node) {
		const char *st = NULL;

		rc = of_property_read_string(
				supply_node,
				"qcom,supply-name",
				&st);
		if (rc) {
			pr_err("error reading name rc=%d\n", rc);
			goto error;
		}

		strlcpy(mp->vreg_config[i].vreg_name,
			st,
			sizeof(mp->vreg_config[i].vreg_name));

		rc = of_property_read_u32(
				supply_node,
				"qcom,supply-min-voltage",
				&tmp);
		if (rc) {
			pr_err("error reading min volt rc=%d\n", rc);
			goto error;
		}

		mp->vreg_config[i].min_voltage = tmp;

		rc = of_property_read_u32(
				supply_node,
				"qcom,supply-max-voltage",
				&tmp);
		if (rc) {
			pr_err("error reading max volt rc=%d\n", rc);
			goto error;
		}

		mp->vreg_config[i].max_voltage = tmp;

		rc = of_property_read_u32(
				supply_node,
				"qcom,supply-enable-load",
				&tmp);
		if (rc) {
			pr_err("error reading enable load rc=%d\n", rc);
			goto error;
		}

		mp->vreg_config[i].enable_load = tmp;

		rc = of_property_read_u32(
				supply_node,
				"qcom,supply-disable-load",
				&tmp);
		if (rc) {
			pr_err("error reading disable load rc=%d\n", rc);
			goto error;
		}

		mp->vreg_config[i].disable_load = tmp;

		rc = of_property_read_u32(
				supply_node,
				"qcom,supply-pre-on-sleep",
				&tmp);
		mp->vreg_config[i].pre_on_sleep = rc ? 0 : tmp;

		rc = of_property_read_u32(
				supply_node,
				"qcom,supply-pre-off-sleep",
				&tmp);
		mp->vreg_config[i].pre_off_sleep = rc ? 0 : tmp;

		rc = of_property_read_u32(
				supply_node,
				"qcom,supply-post-on-sleep",
				&tmp);
		mp->vreg_config[i].post_on_sleep = rc ? 0 : tmp;

		rc = of_property_read_u32(
				supply_node,
				"qcom,supply-post-off-sleep",
				&tmp);
		mp->vreg_config[i].post_off_sleep = rc ? 0 : tmp;

		pr_debug(
			"%s min=%d, max=%d, enable=%d, disable=%d, "
			"preonsleep=%d, postonsleep=%d, preoffsleep=%d, "
			"postoffsleep=%d\n",
			mp->vreg_config[i].vreg_name,
			mp->vreg_config[i].min_voltage,
			mp->vreg_config[i].max_voltage,
			mp->vreg_config[i].enable_load,
			mp->vreg_config[i].disable_load,
			mp->vreg_config[i].pre_on_sleep,
			mp->vreg_config[i].post_on_sleep,
			mp->vreg_config[i].pre_off_sleep,
			mp->vreg_config[i].post_off_sleep);

		i++;
	}

	of_node_put(supply_root_node);

	return 0;

error:
	devm_kfree(&pdev->dev, mp->vreg_config);
	mp->vreg_config = NULL;
	mp->num_vreg = 0;

	of_node_put(supply_root_node);

	return rc;
}

static int mdss_pll_util_parse_dt_clock(
		struct platform_device *pdev,
		struct mdss_pll_resources *pll_res)
{
	u32 i;
	int rc;
	struct dss_module_power *mp = &pll_res->mp;
	const char *clock_name;
	u32 clock_rate;

	mp->num_clk = of_property_count_strings(
			pdev->dev.of_node,
			"clock-names");

	if ((int)mp->num_clk <= 0) {
		pr_err("clocks are not defined\n");
		mp->num_clk = 0;
		return -EINVAL;
	}

	mp->clk_config = devm_kzalloc(
			&pdev->dev,
			sizeof(*mp->clk_config) * mp->num_clk,
			GFP_KERNEL);

	if (!mp->clk_config) {
		mp->num_clk = 0;
		return -ENOMEM;
	}

	for (i = 0; i < mp->num_clk; i++) {
		rc = of_property_read_string_index(
				pdev->dev.of_node,
				"clock-names",
				i,
				&clock_name);
		if (rc) {
			pr_err("failed to read clock name %u rc=%d\n",
			       i, rc);
			return rc;
		}

		strlcpy(mp->clk_config[i].clk_name,
			clock_name,
			sizeof(mp->clk_config[i].clk_name));

		clock_rate = 0;

		of_property_read_u32_index(
				pdev->dev.of_node,
				"clock-rate",
				i,
				&clock_rate);

		mp->clk_config[i].rate = clock_rate;

		if (!clock_rate)
			mp->clk_config[i].type = DSS_CLK_AHB;
		else
			mp->clk_config[i].type = DSS_CLK_PCLK;
	}

	return 0;
}

static void mdss_pll_free_bootmem(u32 mem_addr, u32 size)
{
	unsigned long pfn_start;
	unsigned long pfn_end;
	unsigned long pfn_idx;

	pfn_start = mem_addr >> PAGE_SHIFT;
	pfn_end = (mem_addr + size) >> PAGE_SHIFT;

	for (pfn_idx = pfn_start;
	     pfn_idx < pfn_end;
	     pfn_idx++)
		free_reserved_page(pfn_to_page(pfn_idx));
}

static int mdss_pll_util_parse_dt_dfps(
		struct platform_device *pdev,
		struct mdss_pll_resources *pll_res)
{
	int rc = 0;
	struct device_node *pnode;
	const u32 *addr;
	struct vm_struct *area = NULL;
	u64 size;
	u32 offsets[2];
	unsigned long virt_add = 0;

	pnode = of_parse_phandle(
			pdev->dev.of_node,
			"memory-region",
			0);

	if (IS_ERR(pnode)) {
		rc = PTR_ERR(pnode);
		goto pnode_err;
	}

	if (!pnode) {
		pr_debug("DFPS memory-region not defined\n");
		return -ENODEV;
	}

	addr = of_get_address(
			pnode,
			0,
			&size,
			NULL);

	if (!addr) {
		pr_err("failed to parse the dfps memory address\n");
		rc = -EINVAL;
		goto pnode_err;
	}

	/*
	 * Maintain compatibility for 32/64-bit systems.
	 */
	offsets[0] = (u32)of_read_ulong(addr, 2);
	offsets[1] = (u32)size;

	if (!offsets[1]) {
		rc = -EINVAL;
		goto pnode_err;
	}

	area = get_vm_area(
			offsets[1],
			VM_IOREMAP);

	if (!area) {
		rc = -ENOMEM;
		goto dfps_mem_err;
	}

	virt_add = (unsigned long)area->addr;

	rc = ioremap_page_range(
			virt_add,
			virt_add + offsets[1],
			offsets[0],
			PAGE_KERNEL);

	if (rc) {
		pr_err("ioremap_page_range failed rc=%d\n", rc);
		rc = -ENOMEM;
		goto ioremap_err;
	}

	pll_res->dfps = kzalloc(
			sizeof(struct dfps_info),
			GFP_KERNEL);

	if (!pll_res->dfps) {
		rc = -ENOMEM;
		pr_err("couldn't allocate dfps kernel memory\n");
		goto addr_err;
	}

	memcpy_fromio(
			pll_res->dfps,
			area->addr,
			sizeof(struct dfps_info));

addr_err:
	if (virt_add)
		unmap_kernel_range(
				virt_add,
				(unsigned long)size);

ioremap_err:
	if (area)
		vfree(area->addr);

dfps_mem_err:
	memblock_free(
			offsets[0],
			offsets[1]);

	mdss_pll_free_bootmem(
			offsets[0],
			offsets[1]);

pnode_err:
	if (pnode)
		of_node_put(pnode);

	dma_release_declared_memory(&pdev->dev);

	return rc;
}

int mdss_pll_util_resource_parse(
		struct platform_device *pdev,
		struct mdss_pll_resources *pll_res)
{
	int rc;
	struct dss_module_power *mp;

	if (!pdev || !pll_res)
		return -EINVAL;

	mp = &pll_res->mp;

	rc = mdss_pll_util_parse_dt_supply(
			pdev,
			pll_res);

	if (rc) {
		pr_err("vreg parsing failed rc=%d\n", rc);
		return rc;
	}

	rc = mdss_pll_util_parse_dt_clock(
			pdev,
			pll_res);

	if (rc) {
		pr_err("clock name parsing failed rc=%d\n", rc);

		if (mp->vreg_config) {
			devm_kfree(&pdev->dev,
				   mp->vreg_config);
			mp->vreg_config = NULL;
		}

		mp->num_vreg = 0;

		return rc;
	}

	/*
	 * DFPS memory is optional.
	 */
	rc = mdss_pll_util_parse_dt_dfps(
			pdev,
			pll_res);

	if (rc)
		pr_debug("dfps not enabled rc=%d\n", rc);

	/*
	 * Do not fail PLL initialization when DFPS is
	 * not present in the device tree.
	 */
	return 0;
}

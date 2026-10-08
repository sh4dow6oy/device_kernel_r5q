// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2013-2019, The Linux Foundation. All rights reserved.
 */

#define pr_fmt(fmt) "%s: " fmt, __func__

#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/iopoll.h>
#include <linux/platform_device.h>
#include <linux/io.h>

#include "pll_drv.h"
#include "dsi_pll.h"
#include "dp_pll.h"
#include "hdmi_pll.h"


int mdss_pll_resource_enable(struct mdss_pll_resources *pll_res,
			     bool enable)
{
	int rc = 0;
	int changed = 0;

	if (!pll_res) {
		pr_err("Invalid input parameters\n");
		return -EINVAL;
	}

	/*
	 * Don't turn off resources during handoff or add more than
	 * one reference count.
	 */
	if (pll_res->handoff_resources &&
	    (!enable || (enable && pll_res->resource_enable))) {
		pr_debug("Do not turn on/off PLL resources during handoff case\n");
		return 0;
	}

	if (enable) {
		if (pll_res->resource_ref_cnt == 0)
			changed = 1;

		pll_res->resource_ref_cnt++;
	} else {
		if (pll_res->resource_ref_cnt) {
			pll_res->resource_ref_cnt--;

			if (pll_res->resource_ref_cnt == 0)
				changed = 1;
		} else {
			pr_err("PLL resources already OFF\n");
		}
	}

	if (changed) {
		rc = mdss_pll_util_resource_enable(pll_res, enable);

		if (rc) {
			pr_err("Resource update failed rc=%d\n", rc);

			/*
			 * Roll back the reference count when the
			 * resource transition failed.
			 */
			if (enable)
				pll_res->resource_ref_cnt--;
			else
				pll_res->resource_ref_cnt++;
		} else {
			pll_res->resource_enable = enable;
		}
	}

	return rc;
}


static int mdss_pll_resource_init(struct platform_device *pdev,
				  struct mdss_pll_resources *pll_res)
{
	if (!pdev || !pll_res) {
		pr_err("Invalid input parameters\n");
		return -EINVAL;
	}

	return mdss_pll_util_resource_init(pdev, pll_res);
}


static void mdss_pll_resource_deinit(struct platform_device *pdev,
				     struct mdss_pll_resources *pll_res)
{
	if (!pdev || !pll_res) {
		pr_err("Invalid input parameters\n");
		return;
	}

	mdss_pll_util_resource_deinit(pdev, pll_res);
}


static void mdss_pll_resource_release(struct platform_device *pdev,
				      struct mdss_pll_resources *pll_res)
{
	if (!pdev || !pll_res) {
		pr_err("Invalid input parameters\n");
		return;
	}

	mdss_pll_util_resource_release(pdev, pll_res);
}


static int mdss_pll_resource_parse(struct platform_device *pdev,
				   struct mdss_pll_resources *pll_res)
{
	int rc = 0;
	const char *compatible_stream;

	if (!pdev || !pll_res) {
		pr_err("Invalid input parameters\n");
		return -EINVAL;
	}

	rc = mdss_pll_util_resource_parse(pdev, pll_res);
	if (rc) {
		pr_err("Failed to parse the resources rc=%d\n", rc);
		return rc;
	}

	compatible_stream = of_get_property(pdev->dev.of_node,
					    "compatible", NULL);
	if (!compatible_stream) {
		pr_err("Failed to parse the compatible stream\n");
		rc = -EINVAL;
		goto err;
	}

	/*
	 * PLL type detection.
	 *
	 * Keep this as one continuous else-if chain. The original
	 * source had the first two checks separated, which could
	 * overwrite the selected PLL type.
	 */
	if (!strcmp(compatible_stream, "qcom,mdss_dsi_pll_10nm")) {
		pll_res->pll_interface_type = MDSS_DSI_PLL_10NM;

	} else if (!strcmp(compatible_stream,
			   "qcom,mdss_dp_pll_10nm")) {
		pll_res->pll_interface_type = MDSS_DP_PLL_10NM;

	} else if (!strcmp(compatible_stream,
			   "qcom,mdss_dp_pll_7nm")) {
		pll_res->pll_interface_type = MDSS_DP_PLL_7NM;

	} else if (!strcmp(compatible_stream,
			   "qcom,mdss_dsi_pll_7nm")) {
		pll_res->pll_interface_type = MDSS_DSI_PLL_7NM;

	} else if (!strcmp(compatible_stream,
			   "qcom,mdss_dsi_pll_7nm_v2")) {
		pll_res->pll_interface_type = MDSS_DSI_PLL_7NM_V2;

	} else if (!strcmp(compatible_stream,
			   "qcom,mdss_dsi_pll_7nm_v4_1")) {
		pll_res->pll_interface_type = MDSS_DSI_PLL_7NM_V4_1;

	} else if (!strcmp(compatible_stream,
			   "qcom,mdss_dp_pll_7nm_v2")) {
		pll_res->pll_interface_type = MDSS_DP_PLL_7NM_V2;

	} else if (!strcmp(compatible_stream,
			   "qcom,mdss_dsi_pll_12nm")) {
		pll_res->pll_interface_type = MDSS_DSI_PLL_12NM;

	} else if (!strcmp(compatible_stream,
			   "qcom,mdss_dsi_pll_28lpm")) {
		pll_res->pll_interface_type = MDSS_DSI_PLL_28LPM;

	} else if (!strcmp(compatible_stream,
			   "qcom,mdss_dsi_pll_14nm")) {
		pll_res->pll_interface_type = MDSS_DSI_PLL_14NM;

	} else if (!strcmp(compatible_stream,
			   "qcom,mdss_dp_pll_14nm")) {
		pll_res->pll_interface_type = MDSS_DP_PLL_14NM;

	} else if (!strcmp(compatible_stream,
			   "qcom,mdss_hdmi_pll_28lpm")) {
		pll_res->pll_interface_type = MDSS_HDMI_PLL_28LPM;

	} else if (!strcmp(compatible_stream,
			   "qcom,mdss_dsi_pll_sdm660")) {

		pll_res->pll_interface_type = MDSS_DSI_PLL_14NM;
		pll_res->target_id = MDSS_PLL_TARGET_SDM660;
		pll_res->revision = 2;

	} else if (!strcmp(compatible_stream,
			   "qcom,mdss_dp_pll_sdm660")) {

		pll_res->pll_interface_type = MDSS_DP_PLL_14NM;
		pll_res->target_id = MDSS_PLL_TARGET_SDM660;
		pll_res->revision = 2;

	} else {
		pr_err("Unsupported PLL compatible: %s\n",
		       compatible_stream);
		rc = -ENODEV;
		goto err;
	}

	return 0;

err:
	mdss_pll_resource_release(pdev, pll_res);
	return rc;
}


static int mdss_pll_clock_register(struct platform_device *pdev,
				   struct mdss_pll_resources *pll_res)
{
	int rc;

	if (!pdev || !pll_res) {
		pr_err("Invalid input parameters\n");
		return -EINVAL;
	}

	switch (pll_res->pll_interface_type) {

	case MDSS_DSI_PLL_10NM:
		rc = dsi_pll_clock_register_10nm(pdev, pll_res);
		break;

	case MDSS_DP_PLL_10NM:
		rc = dp_pll_clock_register_10nm(pdev, pll_res);
		break;

	case MDSS_DSI_PLL_7NM:
	case MDSS_DSI_PLL_7NM_V2:
	case MDSS_DSI_PLL_7NM_V4_1:
		rc = dsi_pll_clock_register_7nm(pdev, pll_res);
		break;

	case MDSS_DP_PLL_7NM:
	case MDSS_DP_PLL_7NM_V2:
		rc = dp_pll_clock_register_7nm(pdev, pll_res);
		break;

	case MDSS_DSI_PLL_28LPM:
		rc = dsi_pll_clock_register_28lpm(pdev, pll_res);
		break;

	case MDSS_DSI_PLL_14NM:
		rc = dsi_pll_clock_register_14nm(pdev, pll_res);
		break;

	case MDSS_DP_PLL_14NM:
		rc = dp_pll_clock_register_14nm(pdev, pll_res);
		break;

	case MDSS_HDMI_PLL_28LPM:
		rc = hdmi_pll_clock_register_28lpm(pdev, pll_res);
		break;

	case MDSS_DSI_PLL_12NM:
		rc = dsi_pll_clock_register_12nm(pdev, pll_res);
		break;

	case MDSS_UNKNOWN_PLL:
	default:
		pr_err("Unknown PLL interface type=%u\n",
		       pll_res->pll_interface_type);
		return -EINVAL;
	}

	if (rc)
		pr_err("PLL ndx=%d clock register failed rc=%d\n",
		       pll_res->index, rc);

	return rc;
}


static int mdss_pll_probe(struct platform_device *pdev)
{
	int rc = 0;
	const char *label;

	struct resource *pll_base_reg;
	struct resource *phy_base_reg;
	struct resource *tx0_base_reg;
	struct resource *tx1_base_reg;
	struct resource *dynamic_pll_base_reg;
	struct resource *gdsc_base_reg;

	struct mdss_pll_resources *pll_res;

	if (!pdev->dev.of_node) {
		pr_err("MDSS PLL driver only supports device tree probe\n");
		return -ENOTSUPP;
	}

	label = of_get_property(pdev->dev.of_node,
				"label", NULL);

	if (!label)
		pr_info("MDSS PLL label not specified\n");
	else
		pr_info("MDSS PLL label = %s\n", label);

	pll_res = devm_kzalloc(&pdev->dev,
			       sizeof(*pll_res),
			       GFP_KERNEL);
	if (!pll_res)
		return -ENOMEM;

	platform_set_drvdata(pdev, pll_res);

	rc = of_property_read_u32(pdev->dev.of_node,
				  "cell-index",
				  &pll_res->index);
	if (rc) {
		pr_debug("Unable to get cell-index rc=%d\n", rc);
		pll_res->index = 0;
	}

	pll_res->ssc_en =
		of_property_read_bool(pdev->dev.of_node,
				      "qcom,dsi-pll-ssc-en");

	if (pll_res->ssc_en) {
		pr_info("%s: label=%s PLL SSC enabled\n",
			__func__, label ? label : "unknown");

		rc = of_property_read_u32(
			pdev->dev.of_node,
			"qcom,ssc-frequency-hz",
			&pll_res->ssc_freq);
		if (rc)
			pr_warn("Unable to read ssc-frequency-hz rc=%d\n",
				rc);

		rc = of_property_read_u32(
			pdev->dev.of_node,
			"qcom,ssc-ppm",
			&pll_res->ssc_ppm);
		if (rc)
			pr_warn("Unable to read ssc-ppm rc=%d\n",
				rc);

		pll_res->ssc_center = false;

		label = of_get_property(
			pdev->dev.of_node,
			"qcom,dsi-pll-ssc-mode",
			NULL);

		if (label && !strcmp(label, "center-spread"))
			pll_res->ssc_center = true;
	}

	/* ----------------------------------------------------- */
	/* PLL base                                               */
	/* ----------------------------------------------------- */

	pll_base_reg = platform_get_resource_byname(
		pdev, IORESOURCE_MEM, "pll_base");

	if (!pll_base_reg) {
		pr_err("Unable to get PLL base resource\n");
		return -ENOMEM;
	}

	pll_res->pll_base = ioremap(
		pll_base_reg->start,
		resource_size(pll_base_reg));

	if (!pll_res->pll_base) {
		pr_err("Unable to remap PLL base resource\n");
		return -ENOMEM;
	}

	pr_debug("%s: ndx=%d base=%p\n",
		 __func__,
		 pll_res->index,
		 pll_res->pll_base);

	/* ----------------------------------------------------- */
	/* Parse resources                                        */
	/* ----------------------------------------------------- */

	rc = mdss_pll_resource_parse(pdev, pll_res);
	if (rc) {
		pr_err("PLL resource parsing from DT failed rc=%d\n", rc);
		goto res_parse_error;
	}

	/* ----------------------------------------------------- */
	/* PHY base                                               */
	/* ----------------------------------------------------- */

	phy_base_reg = platform_get_resource_byname(
		pdev, IORESOURCE_MEM, "phy_base");

	if (phy_base_reg) {
		pll_res->phy_base = ioremap(
			phy_base_reg->start,
			resource_size(phy_base_reg));

		if (!pll_res->phy_base) {
			pr_err("Unable to remap PHY base resource\n");
			rc = -ENOMEM;
			goto error;
		}
	}

	/* ----------------------------------------------------- */
	/* Dynamic PLL base                                       */
	/* ----------------------------------------------------- */

	dynamic_pll_base_reg = platform_get_resource_byname(
		pdev, IORESOURCE_MEM, "dynamic_pll_base");

	if (dynamic_pll_base_reg) {
		pll_res->dyn_pll_base = ioremap(
			dynamic_pll_base_reg->start,
			resource_size(dynamic_pll_base_reg));

		if (!pll_res->dyn_pll_base) {
			pr_err("Unable to remap dynamic PLL base\n");
			rc = -ENOMEM;
			goto error;
		}
	}

	/* ----------------------------------------------------- */
	/* Lane TX0 base                                          */
	/* ----------------------------------------------------- */

	tx0_base_reg = platform_get_resource_byname(
		pdev, IORESOURCE_MEM, "ln_tx0_base");

	if (tx0_base_reg) {
		pll_res->ln_tx0_base = ioremap(
			tx0_base_reg->start,
			resource_size(tx0_base_reg));

		if (!pll_res->ln_tx0_base) {
			pr_err("Unable to remap Lane TX0 base\n");
			rc = -ENOMEM;
			goto error;
		}
	}

	/* ----------------------------------------------------- */
	/* Lane TX1 base                                          */
	/* ----------------------------------------------------- */

	tx1_base_reg = platform_get_resource_byname(
		pdev, IORESOURCE_MEM, "ln_tx1_base");

	if (tx1_base_reg) {
		pll_res->ln_tx1_base = ioremap(
			tx1_base_reg->start,
			resource_size(tx1_base_reg));

		if (!pll_res->ln_tx1_base) {
			pr_err("Unable to remap Lane TX1 base\n");
			rc = -ENOMEM;
			goto error;
		}
	}

	/* ----------------------------------------------------- */
	/* GDSC base                                              */
	/* ----------------------------------------------------- */

	gdsc_base_reg = platform_get_resource_byname(
		pdev, IORESOURCE_MEM, "gdsc_base");

	if (!gdsc_base_reg) {
		pr_err("Unable to get GDSC base resource\n");
		rc = -ENOMEM;
		goto error;
	}

	pll_res->gdsc_base = ioremap(
		gdsc_base_reg->start,
		resource_size(gdsc_base_reg));

	if (!pll_res->gdsc_base) {
		pr_err("Unable to remap GDSC base resource\n");
		rc = -ENOMEM;
		goto error;
	}

	/* ----------------------------------------------------- */
	/* Resource initialization                               */
	/* ----------------------------------------------------- */

	rc = mdss_pll_resource_init(pdev, pll_res);
	if (rc) {
		pr_err("PLL ndx=%d resource init failed rc=%d\n",
		       pll_res->index, rc);
		goto error;
	}

	/* ----------------------------------------------------- */
	/* Register PLL clocks                                   */
	/* ----------------------------------------------------- */

	rc = mdss_pll_clock_register(pdev, pll_res);
	if (rc) {
		pr_err("PLL ndx=%d clock register failed rc=%d\n",
		       pll_res->index, rc);
		mdss_pll_resource_deinit(pdev, pll_res);
		goto error;
	}

	mdss_pll_util_parse_dt_dfps(pdev, pll_res);

	return 0;

error:
	if (pll_res->gdsc_base)
		iounmap(pll_res->gdsc_base);

	if (pll_res->ln_tx1_base)
		iounmap(pll_res->ln_tx1_base);

	if (pll_res->ln_tx0_base)
		iounmap(pll_res->ln_tx0_base);

	if (pll_res->dyn_pll_base)
		iounmap(pll_res->dyn_pll_base);

	if (pll_res->phy_base)
		iounmap(pll_res->phy_base);

	mdss_pll_resource_release(pdev, pll_res);

res_parse_error:
	if (pll_res->pll_base)
		iounmap(pll_res->pll_base);

	return rc;
}


static int mdss_pll_remove(struct platform_device *pdev)
{
	struct mdss_pll_resources *pll_res;

	pll_res = platform_get_drvdata(pdev);

	if (!pll_res) {
		pr_err("Invalid PLL resource data\n");
		return 0;
	}

	mdss_pll_resource_deinit(pdev, pll_res);

	if (pll_res->gdsc_base)
		iounmap(pll_res->gdsc_base);

	if (pll_res->ln_tx1_base)
		iounmap(pll_res->ln_tx1_base);

	if (pll_res->ln_tx0_base)
		iounmap(pll_res->ln_tx0_base);

	if (pll_res->dyn_pll_base)
		iounmap(pll_res->dyn_pll_base);

	if (pll_res->phy_base)
		iounmap(pll_res->phy_base);

	mdss_pll_resource_release(pdev, pll_res);

	if (pll_res->pll_base)
		iounmap(pll_res->pll_base);

	/*
	 * pll_res was allocated with devm_kzalloc(),
	 * so devm_kfree() is not required.
	 */

	return 0;
}


static const struct of_device_id mdss_pll_dt_match[] = {
	{
		.compatible = "qcom,mdss_dsi_pll_10nm",
	},
	{
		.compatible = "qcom,mdss_dp_pll_10nm",
	},
	{
		.compatible = "qcom,mdss_dsi_pll_7nm",
	},
	{
		.compatible = "qcom,mdss_dsi_pll_7nm_v2",
	},
	{
		.compatible = "qcom,mdss_dsi_pll_7nm_v4_1",
	},
	{
		.compatible = "qcom,mdss_dp_pll_7nm",
	},
	{
		.compatible = "qcom,mdss_dp_pll_7nm_v2",
	},
	{
		.compatible = "qcom,mdss_dsi_pll_28lpm",
	},
	{
		.compatible = "qcom,mdss_dsi_pll_14nm",
	},
	{
		.compatible = "qcom,mdss_dp_pll_14nm",
	},
	{
		.compatible = "qcom,mdss_hdmi_pll_28lpm",
	},
	{
		.compatible = "qcom,mdss_dsi_pll_sdm660",
	},
	{
		.compatible = "qcom,mdss_dsi_pll_12nm",
	},
	{
		.compatible = "qcom,mdss_dp_pll_sdm660",
	},
	{ }
};

MODULE_DEVICE_TABLE(of, mdss_pll_dt_match);


static struct platform_driver mdss_pll_driver = {
	.probe = mdss_pll_probe,
	.remove = mdss_pll_remove,
	.driver = {
		.name = "mdss_pll",
		.of_match_table = mdss_pll_dt_match,
	},
};


static int __init mdss_pll_driver_init(void)
{
	int rc;

	rc = platform_driver_register(&mdss_pll_driver);

	if (rc)
		pr_err("mdss_register_pll_driver() failed rc=%d\n", rc);

	return rc;
}

fs_initcall(mdss_pll_driver_init);


static void __exit mdss_pll_driver_deinit(void)
{
	platform_driver_unregister(&mdss_pll_driver);
}

module_exit(mdss_pll_driver_deinit);


MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("MDSS PLL driver");

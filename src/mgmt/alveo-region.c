// SPDX-License-Identifier: GPL-2.0
/*
 * Xilinx Alveo FPGA Region Driver
 *
 * Copyright (C) 2019 Xilinx, Inc.
 *
 * Authors: sonal.santan@xilinx.com
 */

#include <linux/module.h>
#include <linux/fpga/fpga-mgr.h>
#include <linux/fpga/fpga-region.h>

#include "alveo-drv.h"
#include "alveo-devices.h"

static int xmgmt_region_get_bridges(struct fpga_region *region)
{
	return 0;
}

static inline bool is_fixed_region(const struct xocl_region *part)
{
	return ((part->id == XOCL_REGION_STATIC) || (part->id == XOCL_REGION_BLD));
}

static int xmgmt_region_probe(struct platform_device *pdev)
{
	struct xocl_region *part = dev_get_platdata(&pdev->dev);
	struct device *dev = &pdev->dev;
	struct fpga_region *region;
	struct fpga_manager *mgr = NULL;
	int ret;

	xmgmt_info(dev, "Part 0x%px Dev 0x%px Id %x\n", part, dev, part->id);
	BUG_ON(part->region != pdev);
	/* No FPGA manager for static regions */
	if (!is_fixed_region(part)) {
		xmgmt_info(dev, "Dynamic Part 0x%px Dev 0x%px Id %x\n", part, dev, part->id);
//		mgr = fpga_mgr_get(&pdev->dev);
		mgr = fpga_mgr_get(&part->lro->fmgr->dev);
		xmgmt_info(dev, "Got FMgr 0x%px\n", mgr);
		if (IS_ERR(mgr))
			return -EPROBE_DEFER;
		xmgmt_info(dev, "Dynamic Part 0x%px Dev 0x%px Id %x 0x%px\n", part, dev, part->id, mgr);
	}
	xmgmt_info(dev, "Mgr 0x%px\n", mgr);
	region = devm_fpga_region_create(dev, mgr, xmgmt_region_get_bridges);
	if (!region) {
		ret = -ENOMEM;
		goto eprobe_mgr_put;
	}
	xmgmt_info(dev, "FPGA Region 0x%px\n", region);

	region->priv = part;
	region->compat_id = mgr ? mgr->compat_id : NULL;
	platform_set_drvdata(pdev, region);
	xmgmt_info(dev, "Part 0x%px Mgr 0x%px\n", part, mgr);
	ret = fpga_region_register(region);
	if (ret)
		goto eprobe_mgr_put;
	xmgmt_info(dev, "Alveo FPGA Region probed\n");

	return 0;

eprobe_mgr_put:
	if (mgr) fpga_mgr_put(mgr);
	platform_set_drvdata(pdev, NULL);
	return ret;
}

static int xmgmt_region_remove(struct platform_device *pdev)
{
	struct xmgmt_region *part = dev_get_platdata(&pdev->dev);
	struct fpga_region *region = platform_get_drvdata(pdev);
	struct fpga_manager *mgr = region->mgr;
	struct device *dev = &pdev->dev;

	xmgmt_info(dev, "FPGA Region 0x%px\n", region);
	fpga_region_unregister(region);
	if (mgr) fpga_mgr_put(mgr);

	return 0;
}

static struct platform_driver xmgmt_region_driver = {
	.driver	= {
		.name    = "alveo-region",
	},
	.probe   = xmgmt_region_probe,
	.remove  = xmgmt_region_remove,
};

module_platform_driver(xmgmt_region_driver);

MODULE_VERSION(XMGMT_DRIVER_VERSION);
MODULE_AUTHOR("XRT Team <runtime@xilinx.com>");
MODULE_DESCRIPTION("Xilinx Alveo FPGA Region driver");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:alveo-region");
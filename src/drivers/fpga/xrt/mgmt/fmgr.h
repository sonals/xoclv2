/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Xilinx Alveo Management Function Driver
 *
 * Copyright (C) 2019-2020 Xilinx, Inc.
 * Bulk of the code borrowed from XRT mgmt driver file, fmgr.c
 *
 * Authors: Sonal.Santan@xilinx.com
 */

#ifndef	_XMGMT_FMGR_H_
#define	_XMGMT_FMGR_H_

#include <linux/fpga/fpga-mgr.h>
#include <linux/mutex.h>

#include <linux/xrt/xclbin.h>

enum xfpga_sec_level {
	XFPGA_SEC_NONE = 0,
	XFPGA_SEC_DEDICATE,
	XFPGA_SEC_SYSTEM,
	XFPGA_SEC_MAX = XFPGA_SEC_SYSTEM,
};

struct fpga_manager *xmgmt_fmgr_probe(struct platform_device *pdev);
int xmgmt_fmgr_remove(struct fpga_manager *fmgr);

#endif

/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2020 Xilinx, Inc.
 *
 * Authors:
 *	Lizhi Hou <Lizhi.Hou@xilinx.com>
 */

#ifndef	_XRT_UCS_H_
#define	_XRT_UCS_H_

#include "xrt-subdev.h"

/*
 * UCS driver IOCTL calls.
 */
enum xrt_ucs_ioctl_cmd {
	XRT_UCS_CHECK = 0,
	XRT_UCS_ENABLE,
};

#endif	/* _XRT_UCS_H_ */

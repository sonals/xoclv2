// SPDX-License-Identifier: GPL-2.0
/*
 * Platform driver for Xilinx acclerator feature ROM IP.
 *
 * Copyright (C) 2016-2019 Xilinx, Inc.
 *
 * Authors: sonal.santan@xilinx.com
 *          chien-wei.lan@xilinx.com
 */

#include <linux/pci.h>
#include <linux/platform_device.h>
#include "xocl-devices.h"
#include "xocl-features.h"

#if 0
#define	MAGIC_NUM	0x786e6c78
struct feature_rom {
	void __iomem		*base;
	struct platform_device	*pdev;

	struct FeatureRomHeader	header;
	bool			unified;
	bool			mb_mgmt_enabled;
	bool			mb_sche_enabled;
	bool			are_dev;
	bool			aws_dev;
	bool			runtime_clk_scale_en;
	char			uuid[65];
	bool			passthrough_virt_en;
};

static long myioctl(struct platform_device *pdev, unsigned int cmd, unsigned long arg)
{
	xocl_info(&pdev->dev, "%s ioctl %d %ld\n", pdev->name, cmd, arg);
	return 0;
}

const static struct xocl_subdev_ops rom_ops = {
	.ioctl = myioctl,
};

static ssize_t VBNV_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
	struct feature_rom *rom = platform_get_drvdata(to_platform_device(dev));

	return sprintf(buf, "%s\n", rom->header.VBNVName);
}
static DEVICE_ATTR_RO(VBNV);

static ssize_t dr_base_addr_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
	struct feature_rom *rom = platform_get_drvdata(to_platform_device(dev));

	//TODO: Fix: DRBaseAddress no longer required in feature rom
	if(rom->header.MajorVersion >= 10)
		return sprintf(buf, "%llu\n", rom->header.DRBaseAddress);
	else
		return sprintf(buf, "%u\n", 0);
}
static DEVICE_ATTR_RO(dr_base_addr);

static ssize_t ddr_bank_count_max_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
	struct feature_rom *rom = platform_get_drvdata(to_platform_device(dev));

	return sprintf(buf, "%d\n", rom->header.DDRChannelCount);
}
static DEVICE_ATTR_RO(ddr_bank_count_max);

static ssize_t ddr_bank_size_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
	struct feature_rom *rom = platform_get_drvdata(to_platform_device(dev));

	return sprintf(buf, "%d\n", rom->header.DDRChannelSize);
}
static DEVICE_ATTR_RO(ddr_bank_size);

static ssize_t timestamp_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
	struct feature_rom *rom = platform_get_drvdata(to_platform_device(dev));

	return sprintf(buf, "%llu\n", rom->header.TimeSinceEpoch);
}
static DEVICE_ATTR_RO(timestamp);

static ssize_t uuid_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
	struct feature_rom *rom = platform_get_drvdata(to_platform_device(dev));

	return sprintf(buf, "%s\n", rom->uuid);
}
static DEVICE_ATTR_RO(uuid);

static ssize_t FPGA_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
	struct feature_rom *rom = platform_get_drvdata(to_platform_device(dev));

	return sprintf(buf, "%s\n", rom->header.FPGAPartName);
}
static DEVICE_ATTR_RO(FPGA);

static struct attribute *rom_attrs[] = {
	&dev_attr_VBNV.attr,
	&dev_attr_dr_base_addr.attr,
	&dev_attr_ddr_bank_count_max.attr,
	&dev_attr_ddr_bank_size.attr,
	&dev_attr_timestamp.attr,
	&dev_attr_FPGA.attr,
	&dev_attr_uuid.attr,
	NULL,
};

static ssize_t raw_show(struct file *filp, struct kobject *kobj,
	struct bin_attribute *attr, char *buf, loff_t off, size_t count)
{
	struct device *dev = kobj_to_dev(kobj);
	struct feature_rom *rom = platform_get_drvdata(to_platform_device(dev));

	if (off >= sizeof(rom->header))
		return 0;

	if (off + count >= sizeof(rom->header))
		count = sizeof(rom->header) - off;

	memcpy(buf, &rom->header, count);

	return count;
};

static struct bin_attribute raw_attr = {
	.attr = {
		.name = "raw",
		.mode = 0400
	},
	.read = raw_show,
	.write = NULL,
	.size = 0
};

static struct bin_attribute  *rom_bin_attrs[] = {
	&raw_attr,
	NULL,
};

static struct attribute_group rom_attr_group = {
	.attrs = rom_attrs,
	.bin_attrs = rom_bin_attrs,
};

static bool is_unified(struct platform_device *pdev)
{
	struct feature_rom *rom;

	rom = platform_get_drvdata(pdev);
	BUG_ON(!rom);

	return rom->unified;
}

static bool mb_mgmt_on(struct platform_device *pdev)
{
	struct feature_rom *rom;

	rom = platform_get_drvdata(pdev);
	BUG_ON(!rom);

	return rom->mb_mgmt_enabled;
}

static bool mb_sched_on(struct platform_device *pdev)
{
	struct feature_rom *rom;

	rom = platform_get_drvdata(pdev);
	BUG_ON(!rom);

	return rom->mb_sche_enabled && !XOCL_DSA_MB_SCHE_OFF(xocl_get_xdev(pdev));
}

static bool runtime_clk_scale_on(struct platform_device *pdev)
{
	struct feature_rom *rom;

	rom = platform_get_drvdata(pdev);
	BUG_ON(!rom);

	return rom->runtime_clk_scale_en;
}

static bool passthrough_virtualization_on(struct platform_device *pdev)
{
	struct feature_rom *rom;

	rom = platform_get_drvdata(pdev);
	BUG_ON(!rom);

	return rom->passthrough_virt_en;
}

static uint32_t* get_cdma_base_addresses(struct platform_device *pdev)
{
	struct feature_rom *rom;

	rom = platform_get_drvdata(pdev);
	BUG_ON(!rom);

	return (!XOCL_DSA_NO_KDMA(xocl_get_xdev(pdev)) &&
		(rom->header.FeatureBitMap & CDMA)) ?
		rom->header.CDMABaseAddress : 0;
}

static u16 get_ddr_channel_count(struct platform_device *pdev)
{
	struct feature_rom *rom;

	rom = platform_get_drvdata(pdev);
	BUG_ON(!rom);

	return rom->header.DDRChannelCount;
}

static u64 get_ddr_channel_size(struct platform_device *pdev)
{
	struct feature_rom *rom;

	rom = platform_get_drvdata(pdev);
	BUG_ON(!rom);

	return rom->header.DDRChannelSize;
}

static u64 get_timestamp(struct platform_device *pdev)
{
	struct feature_rom *rom;

	rom = platform_get_drvdata(pdev);
	BUG_ON(!rom);

	return rom->header.TimeSinceEpoch;
}

static char *get_uuid(struct platform_device *pdev)
{
	struct feature_rom *rom;

	rom = platform_get_drvdata(pdev);
	BUG_ON(!rom);

	return rom->uuid;
}

static bool is_are(struct platform_device *pdev)
{
	struct feature_rom *rom;

	rom = platform_get_drvdata(pdev);
	BUG_ON(!rom);

	return rom->are_dev;
}

static bool is_aws(struct platform_device *pdev)
{
	struct feature_rom *rom;

	rom = platform_get_drvdata(pdev);
	BUG_ON(!rom);

	return rom->aws_dev;
}

static bool verify_timestamp(struct platform_device *pdev, u64 timestamp)
{
	struct feature_rom *rom;

	/* Ignore timestamp matching for AWS platform */
	if (is_aws(pdev))
		return true;

	rom = platform_get_drvdata(pdev);
	BUG_ON(!rom);

	xocl_info(&pdev->dev, "Shell timestamp: 0x%llx",
		rom->header.TimeSinceEpoch);
	xocl_info(&pdev->dev, "Verify timestamp: 0x%llx", timestamp);

	if (strlen(rom->uuid) > 0) {
		xocl_info(&pdev->dev, "2RP platform, skip timestamp check");
		return true;
	}

	return (rom->header.TimeSinceEpoch == timestamp);
}

static int get_raw_header(struct platform_device *pdev, void *header)
{
	struct feature_rom *rom;

	rom = platform_get_drvdata(pdev);
	BUG_ON(!rom);

	memcpy(header, &rom->header, sizeof (rom->header));

	return 0;
}

static int __find_firmware(struct platform_device *pdev, char *fw_name,
	size_t len, u16 deviceid, const struct firmware **fw, char *suffix)
{
	struct feature_rom *rom = platform_get_drvdata(pdev);
	struct pci_dev *pcidev = XOCL_PL_TO_PCI_DEV(pdev);
	u16 vendor = le16_to_cpu(pcidev->vendor);
	u16 subdevice = le16_to_cpu(pcidev->subsystem_device);
	u64 timestamp = rom->header.TimeSinceEpoch;
	bool is_multi_rp = (strlen(rom->uuid) > 0) ? true : false;
	int err = 0;

	/* For 2RP, only uuid is provided */
	if (is_multi_rp) {
		snprintf(fw_name, len, "xilinx/%s/partition.%s", rom->uuid,
			suffix);
	} else {
		snprintf(fw_name, len, "xilinx/%04x-%04x-%04x-%016llx.%s",
			vendor, deviceid, subdevice, timestamp, suffix);
	}

	/* deviceid is arg, the others are from pdev) */
	xocl_info(&pdev->dev, "try load %s", fw_name);
	err = request_firmware(fw, fw_name, &pcidev->dev);
	if (err && !is_multi_rp) {
		snprintf(fw_name, len, "xilinx/%04x-%04x-%04x-%016llx.%s",
			vendor, (deviceid + 1), subdevice, timestamp, suffix);
		xocl_info(&pdev->dev, "try load %s", fw_name);
		err = request_firmware(fw, fw_name, &pcidev->dev);
	}

	if (err && is_multi_rp) {
		snprintf(fw_name, len, "xilinx/%s/%s.%s", rom->uuid, rom->uuid,
			suffix);
		err = request_firmware(fw, fw_name, &pcidev->dev);
	}

	/* Retry with the legacy dsabin */
	if (err && !is_multi_rp) {
		snprintf(fw_name, len, "xilinx/%04x-%04x-%04x-%016llx.%s",
			vendor, le16_to_cpu(pcidev->device + 1), subdevice,
			le64_to_cpu(0x0000000000000000), suffix);
		xocl_info(&pdev->dev, "try load %s", fw_name);
		err = request_firmware(fw, fw_name, &pcidev->dev);
	}

	return err;
}

static int find_firmware(struct platform_device *pdev, char *fw_name,
	size_t len, u16 deviceid, const struct firmware **fw)
{
	// try xsabin first, then dsabin
	if (__find_firmware(pdev, fw_name, len, deviceid, fw, "xsabin")) {
		return __find_firmware(pdev, fw_name, len, deviceid, fw,
			"dsabin");
	}

	return 0;
}

static struct xocl_rom_funcs rom_ops = {
	.is_unified = is_unified,
	.mb_mgmt_on = mb_mgmt_on,
	.mb_sched_on = mb_sched_on,
	.cdma_addr = get_cdma_base_addresses,
	.get_ddr_channel_count = get_ddr_channel_count,
	.get_ddr_channel_size = get_ddr_channel_size,
	.is_are = is_are,
	.is_aws = is_aws,
	.verify_timestamp = verify_timestamp,
	.get_timestamp = get_timestamp,
	.get_raw_header = get_raw_header,
	.runtime_clk_scale_on = runtime_clk_scale_on,
	.find_firmware = find_firmware,
	.passthrough_virtualization_on = passthrough_virtualization_on,
	.get_uuid = get_uuid,
};

static int get_header_from_peer(struct feature_rom *rom)
{
	struct FeatureRomHeader *header;
	struct resource *res;
	xdev_handle_t xdev = xocl_get_xdev(rom->pdev);

	header = XOCL_GET_SUBDEV_PRIV(&rom->pdev->dev);
	if (!header)
		return -ENODEV;

	memcpy(&rom->header, header, sizeof(*header));

	xocl_xdev_info(xdev, "Searching CMC in dtb.");
	res = xocl_subdev_get_ioresource(xdev, RESNAME_KDMA);
	if (res) {
                rom->header.FeatureBitMap |= CDMA;
		memset(rom->header.CDMABaseAddress, 0,
			sizeof(rom->header.CDMABaseAddress));
		rom->header.CDMABaseAddress[0] = (uint32_t)res->start;

		xocl_xdev_info(xdev, "CDMA is on, CU offset: 0x%x",
				rom->header.CDMABaseAddress[0]);
	}

	return 0;
}

static void platform_type_append(char *prefix, u32 platform_type)
{
	char *type;

	if (!prefix)
		return;

	switch (platform_type) {
	case XOCL_VSEC_PLAT_RECOVERY:
		type = "_Recovery";
		break;
	case XOCL_VSEC_PLAT_1RP:
		type = "_xdma_gen3x4_201920_3";
		break;
	case XOCL_VSEC_PLAT_2RP:
		type = "_xdma_gen3x4_201920_3";
		break;
	default:
		type = "_Unknown";
	}

	strncat(prefix, type, XOCL_MAXNAMELEN - strlen(prefix) - 1);
}

static int init_rom_by_dtb(struct feature_rom *rom)
{
	xdev_handle_t xdev = xocl_get_xdev(rom->pdev);
	struct FeatureRomHeader *header = &rom->header;
	struct resource *res;
	const char *vbnv;
	int i;
	int bar;
	u64 offset;
	u32 platform_type;

	if (XDEV(xdev)->fdt_blob) {
		vbnv = fdt_getprop(XDEV(xdev)->fdt_blob, 0, "vbnv", NULL);
		if (vbnv)
			strcpy(header->VBNVName, vbnv);
		for (i = 0; i < strlen(header->VBNVName); i++)
			if (header->VBNVName[i] == ':' ||
					header->VBNVName[i] == '.')
				header->VBNVName[i] = '_';
	}
	header->FeatureBitMap = UNIFIED_PLATFORM;
	*(u32 *)header->EntryPointString = MAGIC_NUM;
	if (XDEV(xdev)->priv.vbnv)
		strncpy(header->VBNVName, XDEV(xdev)->priv.vbnv,
				sizeof (header->VBNVName) - 1);

	if (!xocl_subdev_vsec(xdev, XOCL_VSEC_PLATFORM_INFO, &bar, &offset)) {
		platform_type = xocl_subdev_vsec_read32(xdev, bar, offset);
		platform_type_append(header->VBNVName, platform_type);
	}

	xocl_xdev_info(xdev, "Searching ERT and CMC in dtb.");
	res = xocl_subdev_get_ioresource(xdev, NODE_CMC_FW_MEM);
	if (res) {
		xocl_xdev_info(xdev, "CMC is on");
		header->FeatureBitMap |= BOARD_MGMT_ENBLD;
	}

	res = xocl_subdev_get_ioresource(xdev, NODE_ERT_FW_MEM);
	if (res) {
		xocl_xdev_info(xdev, "ERT is on");
		header->FeatureBitMap |= MB_SCHEDULER;
	}

	return 0;
}

static int get_header_from_dtb(struct feature_rom *rom)
{
	int i, j = 0;

	/* uuid string should be 64 + '\0' */
	BUG_ON(sizeof(rom->uuid) <= 64);

	for (i = 28; i >= 0 && j < 64; i -= 4, j += 8) {
		sprintf(&rom->uuid[j], "%08x", ioread32(rom->base + i));
	}
	xocl_info(&rom->pdev->dev, "UUID %s", rom->uuid);

	return init_rom_by_dtb(rom);
}

static int get_header_from_vsec(struct feature_rom *rom)
{
	xdev_handle_t xdev = xocl_get_xdev(rom->pdev);
	int bar;
	u64 offset;
	int ret;

	ret = xocl_subdev_vsec(xdev, XOCL_VSEC_UUID_ROM, &bar, &offset);
	if (ret)
		return -ENODEV;

	offset += pci_resource_start(XDEV(xdev)->pdev, bar);
	xocl_xdev_info(xdev, "Mapping uuid at offset 0x%llx", offset);
	rom->base = ioremap_nocache(offset, PAGE_SIZE);

	return get_header_from_dtb(rom);
}

static int get_header_from_iomem(struct feature_rom *rom)
{
	struct platform_device *pdev = rom->pdev;
	u32	val;
	u16	vendor, did;
	int	ret = 0;

	val = ioread32(rom->base);
	if (val != MAGIC_NUM) {
		vendor = XOCL_PL_TO_PCI_DEV(pdev)->vendor;
		did = XOCL_PL_TO_PCI_DEV(pdev)->device;
		if (vendor == 0x1d0f && (did == 0x1042 || did == 0xf010)) { // MAGIC, we should define elsewhere
			xocl_info(&pdev->dev,
				"Found AWS VU9P Device without featureROM");
			/*
 			 * This is AWS device. Fill the FeatureROM struct.
 			 * Right now it doesn't have FeatureROM
 			 */
			memset(rom->header.EntryPointString, 0,
				sizeof(rom->header.EntryPointString));
			strncpy(rom->header.EntryPointString, "xlnx", 4);
			memset(rom->header.FPGAPartName, 0,
				sizeof(rom->header.FPGAPartName));
			strncpy(rom->header.FPGAPartName, "AWS VU9P", 8);
			memset(rom->header.VBNVName, 0,
				sizeof(rom->header.VBNVName));
			strncpy(rom->header.VBNVName,
				"xilinx_aws-vu9p-f1_dynamic_5_0", 35);
			rom->header.MajorVersion = 4;
			rom->header.MinorVersion = 0;
			rom->header.VivadoBuildID = 0xabcd;
			rom->header.IPBuildID = 0xabcd;
			rom->header.TimeSinceEpoch = 0xabcd;
			rom->header.DDRChannelCount = 4;
			rom->header.DDRChannelSize = 16;
			rom->header.FeatureBitMap = 0x0;
			rom->header.FeatureBitMap = UNIFIED_PLATFORM;
			rom->unified = true;
			rom->aws_dev = true;

			xocl_info(&pdev->dev, "Enabling AWS dynamic 5.0 Shell");
		} else {
			xocl_err(&pdev->dev, "Magic number does not match, "
			"actual 0x%x, expected 0x%x", val, MAGIC_NUM);
			ret = -ENODEV;
			goto failed;
		}
	} else
		xocl_memcpy_fromio(&rom->header, rom->base,
				sizeof(rom->header));

failed:
	return ret;
}
#endif

static long myioctl(struct platform_device *pdev, unsigned int cmd, unsigned long arg)
{
	xocl_info(&pdev->dev, "%s ioctl %d %ld\n", pdev->name, cmd, arg);
	return 0;
}

const static struct xocl_subdev_ops rom_ops = {
	.ioctl = myioctl,
};

static int xocl_rom_probe(struct platform_device *pdev)
{
	int ret;
	struct xocl_subdev_info *info = dev_get_platdata(&pdev->dev);
	struct device *dev = &pdev->dev;
	const struct resource *res = platform_get_resource(pdev, IORESOURCE_MEM, 0);

	xocl_info(dev, "Probed %s/%s: Info 0x%px Subdev 0x%px\n", info->name, pdev->name,
		   info, pdev);

	if (res)
		xocl_info(dev, "Subdev %s resource %pr", pdev->name, res);
//	platform_set_drvdata(pdev, (void *)&rom_ops);
	return 0;

eprobe_mgr_put:
	platform_set_drvdata(pdev, NULL);
	return ret;
}

static int xocl_rom_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct xocl_subdev_info *info = dev_get_platdata(&pdev->dev);
	platform_set_drvdata(pdev, NULL);
	xocl_info(dev, "Removed %s/%s\n", info->name, pdev->name);
	return 0;
}

static const struct platform_device_id rom_id_table[] = {
	{ "xocl-rom", (kernel_ulong_t)&rom_ops },
	{ },
};

struct platform_driver xocl_rom_driver = {
	.driver	= {
		.name    = "xocl-rom",
	},
	.probe    = xocl_rom_probe,
	.remove   = xocl_rom_remove,
	.id_table = rom_id_table,
};
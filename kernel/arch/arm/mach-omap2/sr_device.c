/*
 * OMAP3/OMAP4 smartreflex device file
 *
 * Author: Thara Gopinath	<thara@ti.com>
 *
 * Based originally on code from smartreflex.c
 * Copyright (C) 2010 Texas Instruments, Inc.
 * Thara Gopinath <thara@ti.com>
 *
 * Copyright (C) 2008 Nokia Corporation
 * Kalle Jokiniemi
 *
 * Copyright (C) 2007 Texas Instruments, Inc.
 * Lesly A M <x0080970@ti.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/err.h>
#include <linux/slab.h>
#include <linux/io.h>

#include <plat/control.h>
#include <plat/omap_hwmod.h>
#include <plat/omap_device.h>
#include <plat/opp.h>
#include <plat/smartreflex.h>
#include <plat/voltage.h>

static int sr_idle_hwmod(struct omap_device *od)
{
	struct omap_hwmod *oh = *od->hwmods;

	if (irqs_disabled())
		_omap_hwmod_idle(oh);
	else
		omap_device_idle_hwmods(od);
	return 0;
}

static int sr_enable_hwmod(struct omap_device *od)
{
	struct omap_hwmod *oh = *od->hwmods;

	if (irqs_disabled())
		_omap_hwmod_enable(oh);
	else
		omap_device_enable_hwmods(od);
	return 0;
}

static struct omap_device_pm_latency omap_sr_latency[] = {
	{
		.deactivate_func = sr_idle_hwmod,
		.activate_func	 = sr_enable_hwmod,
		.flags = OMAP_DEVICE_LATENCY_AUTO_ADJUST
	},
};

static void cal_reciprocal(u32 sensor, u32 *sengain, u32 *rnsen)
{
	u32 gn, rn, mul;

	for (gn = 0; gn < GAIN_MAXLIMIT; gn++) {
		mul = 1 << (gn + 8);
		rn = mul / sensor;
		if (rn < R_MAXLIMIT) {
			*sengain = gn;
			*rnsen = rn;
		}
	}
}

static u32 cal_opp5_nvalue(u32 prev_nvalue)
{
	u32 senpgain, senngain;
	u32 rnsenp, rnsenn;
	u32 prev_senPgain, prev_senNgain;
	u32 prev_senPRN, prev_senNRN;
	u32 nAdj,sennval,senpval;

	prev_senPgain = (prev_nvalue & 0x00f00000) >> 0x14;
	prev_senNgain = (prev_nvalue & 0x000f0000) >> 0x10;

	prev_senPRN = (prev_nvalue & 0x0000ff00) >> 0x8;
	prev_senNRN = (prev_nvalue & 0x000000ff);

	senpval = ((1 << (prev_senPgain + 8))/prev_senPRN) + 235;
	sennval = ((1 << (prev_senNgain + 8))/prev_senNRN) + 385;

	/* Calculating the gain and reciprocal of the SenN and SenP values */
	cal_reciprocal(senpval, &senpgain, &rnsenp);
	cal_reciprocal(sennval, &senngain, &rnsenn);

	return (senpgain << NVALUERECIPROCAL_SENPGAIN_SHIFT) |
		(senngain << NVALUERECIPROCAL_SENNGAIN_SHIFT) |
		(rnsenp << NVALUERECIPROCAL_RNSENP_SHIFT) |
		(rnsenn << NVALUERECIPROCAL_RNSENN_SHIFT);
}

/* Read EFUSE values from control registers for OMAP3430 */
static void __init sr_read_efuse(struct omap_sr_dev_data *dev_data,
				struct omap_sr_data *sr_data)
{
	int i;
	void __iomem *ctrl_base;

	if (!dev_data) {
		pr_warning(" Bad parameters! dev_data NULL!\n");
		return;
	}

	if (!dev_data->volts_supported || !dev_data->volt_data ||
			!dev_data->efuse_nvalues_offs) {
		pr_warning("%s: Bad parameters! dev_data = %x,"
			"dev_data->volts_supported = %x,"
			"dev_data->volt_data = %x,"
			"dev_data->efuse_nvalues_offs = %x\n", __func__,
			(unsigned int)dev_data, dev_data->volts_supported,
			(unsigned int)dev_data->volt_data,
			(unsigned int)dev_data->efuse_nvalues_offs);
		return;
	}

	/*
	 * From OMAP3630 onwards there are no efuse registers for senn_mod
	 * and senp_mod. They have to be 0x1 by default.
	 */
	if (!dev_data->efuse_sr_control) {
		sr_data->senn_mod = 0x1;
		sr_data->senp_mod = 0x1;
	} else {
		sr_data->senn_mod =
				((omap_ctrl_readl(dev_data->efuse_sr_control) &
				(0x3 << dev_data->sennenable_shift)) >>
				dev_data->sennenable_shift);
		sr_data->senp_mod =
				((omap_ctrl_readl(dev_data->efuse_sr_control) &
				(0x3 << dev_data->senpenable_shift)) >>
				dev_data->senpenable_shift);
	}

	if (cpu_is_omap44xx())
		ctrl_base =  ioremap(0x4A002000, SZ_1K);

	for (i = 0; i < dev_data->volts_supported; i++) {
		if (cpu_is_omap44xx()) {
			u16 offset = dev_data->efuse_nvalues_offs[i];

			dev_data->volt_data[i].sr_nvalue =
				__raw_readb(ctrl_base + offset) |
				__raw_readb(ctrl_base + offset + 1) << 8 |
				__raw_readb(ctrl_base + offset + 2) << 16;
		} else {
			if (i > 10) {
				// The Sniper board does not have an eFuse for OPP5.
				// We have to calculate a rough approximation of the nValue for this OPP.
				dev_data->volt_data[i].sr_nvalue = cal_opp5_nvalue(dev_data->volt_data[i-1].sr_nvalue);
			} else {
				dev_data->volt_data[i].sr_nvalue = omap_ctrl_readl(
					dev_data->efuse_nvalues_offs[i]);
			}
			pr_err("%s: dom %s[%d]: using ntarget %d\n",
				__func__,
				dev_data->vdd_name, i,
				dev_data->volt_data[i].sr_nvalue);
		}
	}
	if (cpu_is_omap44xx())
		iounmap(ctrl_base);
}

/*
 * Hard coded nvalues for testing purposes for OMAP3430,
 * may cause device to hang!
 */
static void __init sr_set_testing_nvalues(struct omap_sr_dev_data *dev_data,
				struct omap_sr_data *sr_data)
{
	int i;

	if (!dev_data || !dev_data->volts_supported ||
			!dev_data->volt_data || !dev_data->test_nvalues) {
		pr_warning("%s: Bad parameters! dev_data = %x,"
			"dev_data->volts_supported = %x,"
			"dev_data->volt_data = %x,"
			"dev_data->test_nvalues = %x\n", __func__,
			(unsigned int)dev_data, dev_data->volts_supported,
			(unsigned int)dev_data->volt_data,
			(unsigned int)dev_data->test_nvalues);
		return;
	}

	sr_data->senn_mod = dev_data->test_sennenable;
	sr_data->senp_mod = dev_data->test_senpenable;
	for (i = 0; i < dev_data->volts_supported; i++) {
		dev_data->volt_data[i].sr_nvalue = dev_data->test_nvalues[i];
		pr_err("%s: dom %s[%d]: using TEST ntarget 0x%08X\n",
				__func__,
				dev_data->vdd_name, i,
				dev_data->test_nvalues[i]);
	}
}

static void __init sr_set_nvalues(struct omap_sr_dev_data *dev_data,
				struct omap_sr_data *sr_data)
{
	if (SR_TESTING_NVALUES)
		sr_set_testing_nvalues(dev_data, sr_data);
	else
		sr_read_efuse(dev_data, sr_data);
}

static int sr_dev_init(struct omap_hwmod *oh, void *user)
{
	struct omap_sr_data *sr_data;
	struct omap_sr_dev_data *sr_dev_data;
	struct omap_device *od;
	char *name = "smartreflex";
	static int i;

	sr_data = kzalloc(sizeof(struct omap_sr_data), GFP_KERNEL);
	if (!sr_data) {
		pr_err("%s: Unable to allocate memory for %s sr_data.Error!\n",
			__func__, oh->name);
		return -ENOMEM;
	}

	sr_dev_data = (struct omap_sr_dev_data *)oh->dev_attr;
	if (unlikely(!sr_dev_data)) {
		pr_err("%s: dev atrribute is NULL\n", __func__);
		kfree(sr_data);
		goto exit;
	}

	/*
	 * OMAP3430 ES3.1 chips by default come with Efuse burnt
	 * with parameters required for full functionality of
	 * smartreflex AVS feature like ntarget values , sennenable
	 * and senpenable. So enable the SR AVS feature during boot up
	 * itself if it is a OMAP3430 ES3.1 chip.
	 */
	if (cpu_is_omap343x()) {
		if (omap_rev() == OMAP3430_REV_ES3_1)
			sr_data->enable_on_init = true;
		else
			sr_data->enable_on_init = false;
	} else {
		sr_data->enable_on_init = false;
	}
	sr_data->device_enable = omap_device_enable;
	sr_data->device_shutdown = omap_device_shutdown;
	sr_data->device_idle = omap_device_idle;
	sr_data->voltdm = omap_voltage_domain_get(sr_dev_data->vdd_name);
	if (IS_ERR(sr_data->voltdm)) {
		pr_err("%s: Unable to get voltage domain pointer for VDD %s\n",
			__func__, sr_dev_data->vdd_name);
		kfree(sr_data);
		goto exit;
	}
	sr_dev_data->volts_supported = omap_voltage_get_volttable(
			sr_data->voltdm, &sr_dev_data->volt_data);
	if (!sr_dev_data->volts_supported) {
		pr_warning("%s: No Voltage table registerd fo VDD%d."
			"Something really wrong\n\n", __func__, i + 1);
		kfree(sr_data);
		goto exit;
	}
	sr_set_nvalues(sr_dev_data, sr_data);
	od = omap_device_build(name, i, oh, sr_data, sizeof(*sr_data),
			       omap_sr_latency,
			       ARRAY_SIZE(omap_sr_latency), 0);
	if (IS_ERR(od)) {
		pr_warning("%s: Could not build omap_device for %s: %s.\n\n",
			__func__, name, oh->name);
		kfree(sr_data);
	}
exit:
	i++;
	return 0;
}

static int __init omap_devinit_smartreflex(void)
{
	return omap_hwmod_for_each_by_class("smartreflex", sr_dev_init, NULL);
}
device_initcall(omap_devinit_smartreflex);

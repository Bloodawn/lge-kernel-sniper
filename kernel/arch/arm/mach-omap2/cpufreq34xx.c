/*
 * OMAP3 resource init/change_level/validate_level functions
 *
 * Copyright (C) 2009 - 2010 Texas Instruments Incorporated.
 *	Nishanth Menon
 * Copyright (C) 2009 - 2010 Deep Root Systems, LLC.
 *	Kevin Hilman
 * Copyright (C) 2010 Nokia Corporation.
 *      Eduardo Valentin
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 * History:
 *
 */

#include <linux/module.h>
#include <linux/err.h>
#include <linux/delay.h>

#include <plat/opp.h>
#include <plat/cpu.h>
#include <plat/clock.h>

/* 20110331 sookyoung.kim@lge.com LG-DVFS [START_LGE] */
#include <linux/dvs_suite.h>
/* 20110331 sookyoung.kim@lge.com LG-DVFS [END_LGE] */

#include "cm-regbits-34xx.h"
#include "prm.h"
#include "omap3-opp.h"

static int omap3_mpu_set_rate(struct device *dev, unsigned long rate);
static int omap3_iva_set_rate(struct device *dev, unsigned long rate);
static int omap3_l3_set_rate(struct device *dev, unsigned long rate);

static unsigned long omap3_mpu_get_rate(struct device *dev);
static unsigned long omap3_iva_get_rate(struct device *dev);
static unsigned long omap3_l3_get_rate(struct device *dev);

struct clk *dpll1_clk, *dpll2_clk, *dpll3_clk;

static struct omap_opp_def __initdata omap34xx_opp_def_list[] = {
	/* MPU OPP1 */
	OMAP_OPP_DEF("mpu", true, 125000000, 975000),
	/* MPU OPP2 */
	OMAP_OPP_DEF("mpu", true, 250000000, 1075000),
	/* MPU OPP3 */
	OMAP_OPP_DEF("mpu", true, 500000000, 1200000),
	/* MPU OPP4 */
	OMAP_OPP_DEF("mpu", true, 550000000, 1270000),
	/* MPU OPP5 */
	OMAP_OPP_DEF("mpu", true, 600000000, 1350000),
	/*
	 * L3 OPP1 - 41.5 MHz is disabled because: The voltage for that OPP is
	 * almost the same than the one at 83MHz thus providing very little
	 * gain for the power point of view. In term of energy it will even
	 * increase the consumption due to the very negative performance
	 * impact that frequency will do to the MPU and the whole system in
	 * general.
	 */
	OMAP_OPP_DEF("l3_main", false, 41500000, 975000),
	/* L3 OPP2 */
	OMAP_OPP_DEF("l3_main", true, 83000000, 1050000),
	/* L3 OPP3 */
	OMAP_OPP_DEF("l3_main", true, 166000000, 1150000),


	/* DSP OPP1 */
	OMAP_OPP_DEF("iva", true, 90000000, 975000),
	/* DSP OPP2 */
	OMAP_OPP_DEF("iva", true, 180000000, 1075000),
	/* DSP OPP3 */
	OMAP_OPP_DEF("iva", true, 360000000, 1200000),
	/* DSP OPP4 */
	OMAP_OPP_DEF("iva", true, 400000000, 1270000),
	/* DSP OPP5 */
	OMAP_OPP_DEF("iva", true, 430000000, 1350000),
};
static u32 omap34xx_opp_def_size = ARRAY_SIZE(omap34xx_opp_def_list);

static struct omap_opp_def __initdata omap36xx_opp_def_list[] = {
	/* MPU OPP1 */
	OMAP_OPP_DEF("mpu", true,  120000000,  840000), 
	/* MPU OPP2 */
	OMAP_OPP_DEF("mpu", true,  220000000,  940000), 
	/* MPU OPP3 */
	OMAP_OPP_DEF("mpu", true,  300000000,  980000), 
	/* MPU OPP4 */
	OMAP_OPP_DEF("mpu", true,  400000000,  1000000), 
	/* MPU OPP5 */
	OMAP_OPP_DEF("mpu", true,  500000000,  1050000), 
	/* MPU OPP6 */
	OMAP_OPP_DEF("mpu", true,  600000000,  1100000),   
	/* MPU OPP7 */
	OMAP_OPP_DEF("mpu", true,  700000000,  1150000),   
	/* MPU OPP8 */
	OMAP_OPP_DEF("mpu", true,  800000000,  1200000),
	/* MPU OPP9 */
	OMAP_OPP_DEF("mpu", true,  900000000,  1250000),
	/* MPU OPP10 */
	OMAP_OPP_DEF("mpu", true,  1000000000, 1300000),
#ifdef CONFIG_CPU_OVERCLOCK
	/* MPU OPP11 */
	OMAP_OPP_DEF("mpu", true,  1100000000, 1350000),
	/* MPU OPP12 */
	OMAP_OPP_DEF("mpu", true,  1200000000, 1400000),
	/* MPU OPP13 */
	OMAP_OPP_DEF("mpu", true,  1300000000, 1450000),
	/* MPU OPP14 */
	OMAP_OPP_DEF("mpu", true,  1320000000, 1460000),
	/* MPU OPP15 */
	OMAP_OPP_DEF("mpu", true,  1340000000, 1470000),
	/* MPU OPP15 */
	OMAP_OPP_DEF("mpu", true,  1360000000, 1480000),
	/* MPU OPP17 */
 	OMAP_OPP_DEF("mpu", true,  1380000000, 1490000),
    /* MPU OPP18 */
	OMAP_OPP_DEF("mpu", true,  1400000000, 1500000),
#endif

	/* L3 OPP1 - OPP50 */
	OMAP_OPP_DEF("l3_main", true, 100000000, 975000),
	/* L3 OPP2 - OPP100, OPP-Turbo, OPP-SB */
	OMAP_OPP_DEF("l3_main", true, 200000000, 1162500),

	/* DSP OPP1 */
	OMAP_OPP_DEF("iva", true,  260000000, 840000),
	/* DSP OPP2 */
	OMAP_OPP_DEF("iva", true,  260000000, 940000),
	/* DSP OPP3 */
	OMAP_OPP_DEF("iva", true,  260000000, 980000),
	/* DSP OPP4 */
	OMAP_OPP_DEF("iva", true,  300000000, 1000000),
	/* DSP OPP5 */
	OMAP_OPP_DEF("iva", true,  400000000, 1050000),
	/* DSP OPP6 */
	OMAP_OPP_DEF("iva", true,  520000000, 1100000),
	/* DSP OPP7 */
	OMAP_OPP_DEF("iva", true,  600000000, 1150000),
	/* DSP OPP8 */
	OMAP_OPP_DEF("iva", true,  660000000, 1200000),
	/* DSP OPP9 */
	OMAP_OPP_DEF("iva", true,  700000000, 1250000),
	/* DSP OPP10 */
	OMAP_OPP_DEF("iva", true,  800000000, 1300000),
#ifdef CONFIG_CPU_OVERCLOCK
	/* DSP OPP11 */
	OMAP_OPP_DEF("iva", true,  800000000, 1350000),
	/* DSP OPP12 */
	OMAP_OPP_DEF("iva", true,  800000000, 1400000),
	/* DSP OPP13 */
	OMAP_OPP_DEF("iva", true,  800000000, 1450000),
	/* DSP OPP14 */
	OMAP_OPP_DEF("iva", true,  800000000, 1460000),
    /* DSP OPP15 */
	OMAP_OPP_DEF("iva", true,  800000000, 1470000),
    /* DSP OPP16 */
	OMAP_OPP_DEF("iva", true,  800000000, 1480000),
    /* DSP OPP17 */
	OMAP_OPP_DEF("iva", true,  800000000, 1490000),
    /* DSP OPP18 */
	OMAP_OPP_DEF("iva", true,  800000000, 1500000),
#endif
};
static u32 omap36xx_opp_def_size = ARRAY_SIZE(omap36xx_opp_def_list);

#ifndef CONFIG_CPU_FREQ
static unsigned long compute_lpj(unsigned long ref, u_int div, u_int mult)
{
	unsigned long new_jiffy_l, new_jiffy_h;

	/*
	 * Recalculate loops_per_jiffy.  We do it this way to
	 * avoid math overflow on 32-bit machines.  Maybe we
	 * should make this architecture dependent?  If you have
	 * a better way of doing this, please replace!
	 *
	 *    new = old * mult / div
	 */
	new_jiffy_h = ref / div;
	new_jiffy_l = (ref % div) / 100;
	new_jiffy_h *= mult;
	new_jiffy_l = new_jiffy_l * mult / div;

	return new_jiffy_h + new_jiffy_l * 100;
}
#endif


static int omap3_mpu_set_rate(struct device *dev, unsigned long rate)
{
	unsigned long cur_rate = omap3_mpu_get_rate(dev);
	int ret;
	/*LGE_CHANGE_S, mg.jeong@lge.com, 2011-01-, Reason*/
	//printk("MPU: Current %lu ,Next %lu\n",cur_rate, rate);

#ifdef CONFIG_CPU_FREQ
	struct cpufreq_freqs freqs_notify;

	/* 20110331 sookyoung.kim@lge.com LG-DVFS [START_LGE] */
	freqs_notify.old = cur_rate / 1000;
	freqs_notify.new = rate / 1000;
	freqs_notify.cpu = 0;

	/* Send pre notification to CPUFreq */
	//cpufreq_notify_transition(&freqs_notify, CPUFREQ_PRECHANGE);

	if(ds_status.flag_run_dvs == 0){
		freqs_notify.old = cur_rate / 1000;
		freqs_notify.new = rate / 1000;
		freqs_notify.cpu = 0;
		cpufreq_notify_transition(&freqs_notify, CPUFREQ_PRECHANGE);
	}
	else{	// LG-DVFS is runnig.
		if(ds_status.flag_correct_cpu_op_update_path == 0){	// Called by cpufreq.
			freqs_notify.old = ds_status.cpu_op_index / 1000;
			freqs_notify.new = ds_status.target_cpu_op_index / 1000;
			freqs_notify.cpu = 0;
			cpufreq_notify_transition(&freqs_notify, CPUFREQ_PRECHANGE);
		}
	}
	/* 20110331 sookyoung.kim@lge.com LG-DVFS [END_LGE] */
#endif
	ret = clk_set_rate(dpll1_clk, rate);
	if (ret) {
		dev_warn(dev, "%s: Unable to set rate to %ld\n",
			__func__, rate);
		return ret;
	}

#ifdef CONFIG_CPU_FREQ
	/* 20110331 sookyoung.kim@lge.com LG-DVFS [START_LGE] */
	/* Send a post notification to CPUFreq */
	//cpufreq_notify_transition(&freqs_notify, CPUFREQ_POSTCHANGE);

	if(ds_status.flag_run_dvs == 0){
		cpufreq_notify_transition(&freqs_notify, CPUFREQ_POSTCHANGE);
	}
	else{	// LG-DVFS is runnig.
		if(ds_status.flag_correct_cpu_op_update_path == 0){	// Called by cpufreq.
			cpufreq_notify_transition(&freqs_notify, CPUFREQ_POSTCHANGE);
		}
	}
	/* 20110331 sookyoung.kim@lge.com LG-DVFS [END_LGE] */
#endif

#ifndef CONFIG_CPU_FREQ
	/*Update loops_per_jiffy if processor speed is being changed*/
	loops_per_jiffy = compute_lpj(loops_per_jiffy,
			cur_rate / 1000, rate / 1000);
#endif
	return 0;
}

static unsigned long omap3_mpu_get_rate(struct device *dev)
{
	return dpll1_clk->rate;
}

static int omap3_iva_set_rate(struct device *dev, unsigned long rate)
{
	//printk("DSP: Current %lu ,Next %lu\n", omap3_mpu_get_rate(dev), rate);
	return clk_set_rate(dpll2_clk, rate);
}

static unsigned long omap3_iva_get_rate(struct device *dev)
{
	return dpll2_clk->rate;
}

static int omap3_l3_set_rate(struct device *dev, unsigned long rate)
{
	int l3_div;

	l3_div = cm_read_mod_reg(CORE_MOD, CM_CLKSEL) &
			OMAP3430_CLKSEL_L3_MASK;

	return clk_set_rate(dpll3_clk, rate * l3_div);
}

static unsigned long omap3_l3_get_rate(struct device *dev)
{
	int l3_div;

	l3_div = cm_read_mod_reg(CORE_MOD, CM_CLKSEL) &
			OMAP3430_CLKSEL_L3_MASK;
	return dpll3_clk->rate / l3_div;
}

/* Temp variable to allow multiple calls */
static u8 __initdata omap3_table_init;

int __init omap3_pm_init_opp_table(void)
{
	struct omap_opp_def *opp_def, *omap3_opp_def_list;
	struct device *dev;
	u32 omap3_opp_def_size;
	int i, r;

	/*
	 * Allow multiple calls, but initialize only if not already initalized
	 * even if the previous call failed, coz, no reason we'd succeed again
	 */
	if (omap3_table_init)
		return 0;
	omap3_table_init = 1;

	omap3_opp_def_list = cpu_is_omap3630() ? omap36xx_opp_def_list :
			omap34xx_opp_def_list;
	omap3_opp_def_size = cpu_is_omap3630() ? omap36xx_opp_def_size :
			omap34xx_opp_def_size;

	opp_def = omap3_opp_def_list;
	for (i = 0; i < omap3_opp_def_size; i++) {
		r = opp_add(opp_def++);
		if (r)
			pr_err("unable to add OPP %ld Hz for %s\n",
				opp_def->freq, opp_def->hwmod_name);
	}

	dpll1_clk = clk_get(NULL, "dpll1_ck");
	dpll2_clk = clk_get(NULL, "dpll2_ck");
	dpll3_clk = clk_get(NULL, "dpll3_m2_ck");

	/* Populate the set rate and get rate for mpu, iva and l3 device */
	dev = omap2_get_mpuss_device();
	if (dev)
		opp_populate_rate_fns(dev, omap3_mpu_set_rate,
				omap3_mpu_get_rate);

	dev = omap2_get_iva_device();
	if (dev)
		opp_populate_rate_fns(dev, omap3_iva_set_rate,
				omap3_iva_get_rate);

	dev = omap2_get_l3_device();
	if (dev)
		opp_populate_rate_fns(dev, omap3_l3_set_rate,
				omap3_l3_get_rate);

	return 0;
}

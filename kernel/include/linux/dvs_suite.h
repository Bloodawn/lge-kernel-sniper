/*
 *	include/linux/dvs_suite.h
 *
 *	Copyright (C) 2010 Sookyoung Kim <sookyoung.kim@lge.com>
 */

#ifndef _LINUX_DVS_SUITE_H
#define _LINUX_DVS_SUITE_H

/***************************************************************************
 * Headers
 ***************************************************************************/
#include <asm/current.h>	/* For current macro */
#include <asm/div64.h>		/* For division */

#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/spinlock.h>
#include <linux/errno.h>	/* For EAGAIN and EWOULDBLOCK */
#include <linux/kernel.h>	/* For printk */
#include <linux/sched.h>	/* For struct task_struct and wait_event* macros */
#include <linux/slab.h>		/* For kmalloc and kfree */
#include <linux/string.h>	/* To use string functions */
#include <linux/times.h>	/* For struct timeval and do_gettimeofday */
#include <linux/cred.h>		/* To get uid */
#include <linux/workqueue.h>

#include <plat/omap_device.h>
#include <plat/omap-pm.h>

/*
 * Including resource34xx.h here causes build error.
 * This is the reason why we moved ds_update_cpu_op() from here to resource34xx.c.
 */
//#include "../../arch/arm/mach-omap2/resource34xx.h"	/* For set_opp() */
//#include "../../../system/core/include/private/android_filesystem_config.h"	/* For AID_* */

/***************************************************************************
 * Definitions
 ***************************************************************************/

#ifdef CONFIG_CPU_OVERCLOCK
/* Bloodawn: The 16 operating points (CPU_OP)

	CPU_OP 0: (1360 MHz, 1.48V),	Scaling factor 1	= 0x1000 in fixed point number
	CPU_OP 1: (1340 MHz, 1.47V),	Scaling factor 0.96	= 0x96c
	CPU_OP 2: (1320 MHz, 1.46V),	Scaling factor 0.95	= 0x95b
	CPU_OP 3: (1300 MHz, 1.45V),	Scaling factor 0.94	= 0x94a
	CPU_OP 4: (1200 MHz, 1.40V),	Scaling factor 0.93	= 0x93c
	CPU_OP 5: (1100 MHz, 1.35V),	Scaling factor 0.92	= 0x92b
	CPU_OP 6: (1000 MHz, 1.30V),	Scaling factor 0.91 = 0x91a
	CPU_OP 7: ( 900 MHz, 1.25V),	Scaling factor 0.9	= 0x90c
	CPU_OP 8: ( 800 MHz, 1.20V),	Scaling factor 0.8  = 0x80b
	CPU_OP 9: ( 700 MHz, 1.15V),	Scaling factor 0.7  = 0x70a
	CPU_OP 10: ( 600 MHz, 1.10V),	Scaling factor 0.6  = 0x60c
	CPU_OP 11: ( 500 MHz, 1.05V),	Scaling factor 0.5	= 0x50b
	CPU_OP 12: ( 400 MHz, 1.00V),	Scaling factor 0.4	= 0x40a
	CPU_OP 13: ( 300 MHz, 0.98V),	Scaling factor 0.3  = 0x30c
	CPU_OP 14: ( 220 MHz, 0.94V),	Scaling factor 0.2	= 0x20b
	CPU_OP 15: ( 120 MHz, 0.84V),	Scaling factor 0.1  = 0x10a

	set_opp(&vdd1_opp, VDD1_OPP16)
	set_opp(&vdd1_opp, VDD1_OPP15)
	set_opp(&vdd1_opp, VDD1_OPP14)
	set_opp(&vdd1_opp, VDD1_OPP13)
	set_opp(&vdd1_opp, VDD1_OPP12)
	set_opp(&vdd1_opp, VDD1_OPP11)
	set_opp(&vdd1_opp, VDD1_OPP10)
	set_opp(&vdd1_opp, VDD1_OPP9)
	set_opp(&vdd1_opp, VDD1_OPP8)
	set_opp(&vdd1_opp, VDD1_OPP7)
	set_opp(&vdd1_opp, VDD1_OPP6)
	set_opp(&vdd1_opp, VDD1_OPP5)
	set_opp(&vdd1_opp, VDD1_OPP4)
	set_opp(&vdd1_opp, VDD1_OPP3)
	set_opp(&vdd1_opp, VDD1_OPP2)
	set_opp(&vdd1_opp, VDD1_OPP1)
 */

/* The number of CPU_OPs to use */
#define DS_CPU_OP_LIMIT			16

/* To cope with touch and key inputs */
#define DS_TOUCH_TIMEOUT_COUNT_MAX	7

/* The CPU_OP indices */
#define DS_CPU_OP_INDEX_0		1360000000
#define DS_CPU_OP_INDEX_1		1340000000
#define DS_CPU_OP_INDEX_2		1320000000
#define DS_CPU_OP_INDEX_3		1300000000
#define DS_CPU_OP_INDEX_4		1200000000
#define DS_CPU_OP_INDEX_5		1100000000
#define DS_CPU_OP_INDEX_6		1000000000
#define DS_CPU_OP_INDEX_7		900000000
#define DS_CPU_OP_INDEX_8		800000000
#define DS_CPU_OP_INDEX_9		700000000
#define DS_CPU_OP_INDEX_10		600000000
#define DS_CPU_OP_INDEX_11		500000000
#define DS_CPU_OP_INDEX_12		400000000
#define DS_CPU_OP_INDEX_13		300000000
#define DS_CPU_OP_INDEX_14		220000000
#define DS_CPU_OP_INDEX_15		120000000

#define DS_CPU_OP_INDEX_MAX			DS_CPU_OP_INDEX_0
#define DS_CPU_OP_INDEX_N2MAX		DS_CPU_OP_INDEX_1
#define DS_CPU_OP_INDEX_N3MAX		DS_CPU_OP_INDEX_2
#define DS_CPU_OP_INDEX_N4MAX		DS_CPU_OP_INDEX_3
#define DS_CPU_OP_INDEX_N5MAX		DS_CPU_OP_INDEX_4
#define DS_CPU_OP_INDEX_N6MAX		DS_CPU_OP_INDEX_5
#define DS_CPU_OP_INDEX_N7MAX		DS_CPU_OP_INDEX_6
#define DS_CPU_OP_INDEX_N8MAX		DS_CPU_OP_INDEX_7
#define DS_CPU_OP_INDEX_N9MAX		DS_CPU_OP_INDEX_8
#define DS_CPU_OP_INDEX_N10MAX		DS_CPU_OP_INDEX_9
#define DS_CPU_OP_INDEX_N11MAX		DS_CPU_OP_INDEX_10
#define DS_CPU_OP_INDEX_N12MAX		DS_CPU_OP_INDEX_11
#define DS_CPU_OP_INDEX_N13MAX		DS_CPU_OP_INDEX_12
#define DS_CPU_OP_INDEX_N3MIN	    DS_CPU_OP_INDEX_13
#define DS_CPU_OP_INDEX_N2MIN		DS_CPU_OP_INDEX_14
#define DS_CPU_OP_INDEX_MIN 		DS_CPU_OP_INDEX_15

/* The scaling factors */
/* These values mean the U(20,12) fixed point numbers' 12bit fractions.
 * In this format, 
 *					------ Decimal part ------ -- Fraction --
 * 1				= 0000 0000 0000 0000 0001 0000 0000 0000 = 0x00001000
 * 0.5				= 0000 0000 0000 0000 0000 1000 0000 0000 = 0x00000800
 * 0.25				= 0000 0000 0000 0000 0000 0100 0000 0000 = 0x00000400
 * 0.125			= 0000 0000 0000 0000 0000 0010 0000 0000 = 0x00000200
 * 0.0625			= 0000 0000 0000 0000 0000 0001 0000 0000 = 0x00000100
 * 0.03125			= 0000 0000 0000 0000 0000 0000 1000 0000 = 0x00000080
 * 0.015625			= 0000 0000 0000 0000 0000 0000 0100 0000 = 0x00000040
 * 0.0078125 		= 0000 0000 0000 0000 0000 0000 0010 0000 = 0x00000020	
 * 0.00390625		= 0000 0000 0000 0000 0000 0000 0010 0000 = 0x00000010	
 * 0.0019553125		= 0000 0000 0000 0000 0000 0000 0010 0000 = 0x00000008	
 * 0.0009765625		= 0000 0000 0000 0000 0000 0000 0010 0000 = 0x00000004	
 * 0.00048828125	= 0000 0000 0000 0000 0000 0000 0010 0000 = 0x00000002	
 * 0.000244140625	= 0000 0000 0000 0000 0000 0000 0010 0000 = 0x00000001	
 *
 * Ex)
 * 0.75 = 0.5 + 0.25	= 0000 0000 0000 0000 0000 1100 0000 0000 = 0x00000c00
 */
#define DS_CPU_OP_SF_0			0x1000	// 1
#define DS_CPU_OP_SF_1			0x96c	// 0.96
#define DS_CPU_OP_SF_2			0x95b	// 0.95
#define DS_CPU_OP_SF_3			0x94a	// 0.94
#define DS_CPU_OP_SF_4			0x93c	// 0.93
#define DS_CPU_OP_SF_5			0x92b	// 0.92
#define DS_CPU_OP_SF_6			0x91a	// 0.91
#define DS_CPU_OP_SF_7			0x90c	// 0.9
#define DS_CPU_OP_SF_8			0x80b	// 0.8
#define DS_CPU_OP_SF_9			0x70a	// 0.7
#define DS_CPU_OP_SF_10			0x60c	// 0.6
#define DS_CPU_OP_SF_11			0x50b	// 0.5
#define DS_CPU_OP_SF_12			0x40a	// 0.4
#define DS_CPU_OP_SF_13			0x30c	// 0.3
#define DS_CPU_OP_SF_14			0x20b	// 0.2
#define DS_CPU_OP_SF_15			0x10a	// 0.1
#else
/* Bloodawn: The 10 operating points (CPU_OP)

	CPU_OP 0: (1000 MHz, 1.30V),	Scaling factor 1	= 0x1000 in fixed point number
	CPU_OP 1: ( 900 MHz, 1.25V),	Scaling factor 0.9	= 0x90c
	CPU_OP 2: ( 800 MHz, 1.20V),	Scaling factor 0.8  = 0x80b
	CPU_OP 3: ( 700 MHz, 1.15V),	Scaling factor 0.7  = 0x70a
	CPU_OP 4: ( 600 MHz, 1.10V),	Scaling factor 0.6  = 0x60c
	CPU_OP 5: ( 500 MHz, 1.05V),	Scaling factor 0.5	= 0x50b
	CPU_OP 6: ( 400 MHz, 1.00V),	Scaling factor 0.4	= 0x40a
	CPU_OP 7: ( 300 MHz, 0.98V),	Scaling factor 0.3  = 0x30c
	CPU_OP 8: ( 220 MHz, 0.94V),	Scaling factor 0.2	= 0x20b
	CPU_OP 9: ( 120 MHz, 0.84V),	Scaling factor 0.1  = 0x10a

	set_opp(&vdd1_opp, VDD1_OPP10)
	set_opp(&vdd1_opp, VDD1_OPP9)
	set_opp(&vdd1_opp, VDD1_OPP8)
	set_opp(&vdd1_opp, VDD1_OPP7)
	set_opp(&vdd1_opp, VDD1_OPP6)
	set_opp(&vdd1_opp, VDD1_OPP5)
	set_opp(&vdd1_opp, VDD1_OPP4)
	set_opp(&vdd1_opp, VDD1_OPP3)
	set_opp(&vdd1_opp, VDD1_OPP2)
	set_opp(&vdd1_opp, VDD1_OPP1)
 */

/* The number of CPU_OPs to use */
#define DS_CPU_OP_LIMIT			10

/* To cope with touch and key inputs */
#define DS_TOUCH_TIMEOUT_COUNT_MAX	7

/* The CPU_OP indices */
#define DS_CPU_OP_INDEX_0		1000000000
#define DS_CPU_OP_INDEX_1		900000000
#define DS_CPU_OP_INDEX_2		800000000
#define DS_CPU_OP_INDEX_3		700000000
#define DS_CPU_OP_INDEX_4		600000000
#define DS_CPU_OP_INDEX_5		500000000
#define DS_CPU_OP_INDEX_6		400000000
#define DS_CPU_OP_INDEX_7		300000000
#define DS_CPU_OP_INDEX_8		220000000
#define DS_CPU_OP_INDEX_9		120000000

#define DS_CPU_OP_INDEX_MAX			DS_CPU_OP_INDEX_0
#define DS_CPU_OP_INDEX_N2MAX		DS_CPU_OP_INDEX_1
#define DS_CPU_OP_INDEX_N3MAX		DS_CPU_OP_INDEX_2
#define DS_CPU_OP_INDEX_N4MAX		DS_CPU_OP_INDEX_3
#define DS_CPU_OP_INDEX_N5MAX		DS_CPU_OP_INDEX_4
#define DS_CPU_OP_INDEX_N5MIN	    DS_CPU_OP_INDEX_5
#define DS_CPU_OP_INDEX_N4MIN		DS_CPU_OP_INDEX_6
#define DS_CPU_OP_INDEX_N3MIN		DS_CPU_OP_INDEX_7
#define DS_CPU_OP_INDEX_N2MIN		DS_CPU_OP_INDEX_8
#define DS_CPU_OP_INDEX_MIN			DS_CPU_OP_INDEX_9

/* The scaling factors */
/* These values mean the U(20,12) fixed point numbers' 12bit fractions.
 * In this format, 
 *					------ Decimal part ------ -- Fraction --
 * 1				= 0000 0000 0000 0000 0001 0000 0000 0000 = 0x00001000
 * 0.5				= 0000 0000 0000 0000 0000 1000 0000 0000 = 0x00000800
 * 0.25				= 0000 0000 0000 0000 0000 0100 0000 0000 = 0x00000400
 * 0.125			= 0000 0000 0000 0000 0000 0010 0000 0000 = 0x00000200
 * 0.0625			= 0000 0000 0000 0000 0000 0001 0000 0000 = 0x00000100
 * 0.03125			= 0000 0000 0000 0000 0000 0000 1000 0000 = 0x00000080
 * 0.015625			= 0000 0000 0000 0000 0000 0000 0100 0000 = 0x00000040
 * 0.0078125 		= 0000 0000 0000 0000 0000 0000 0010 0000 = 0x00000020	
 * 0.00390625		= 0000 0000 0000 0000 0000 0000 0010 0000 = 0x00000010	
 * 0.0019553125		= 0000 0000 0000 0000 0000 0000 0010 0000 = 0x00000008	
 * 0.0009765625		= 0000 0000 0000 0000 0000 0000 0010 0000 = 0x00000004	
 * 0.00048828125	= 0000 0000 0000 0000 0000 0000 0010 0000 = 0x00000002	
 * 0.000244140625	= 0000 0000 0000 0000 0000 0000 0010 0000 = 0x00000001	
 *
 * Ex)
 * 0.75 = 0.5 + 0.25	= 0000 0000 0000 0000 0000 1100 0000 0000 = 0x00000c00
 */
#define DS_CPU_OP_SF_0			0x1000	// 1
#define DS_CPU_OP_SF_1			0x90c	// 0.9
#define DS_CPU_OP_SF_2			0x80b	// 0.8
#define DS_CPU_OP_SF_3			0x70a	// 0.7
#define DS_CPU_OP_SF_4			0x60c	// 0.6
#define DS_CPU_OP_SF_5			0x50b	// 0.5
#define DS_CPU_OP_SF_6			0x40a	// 0.4
#define DS_CPU_OP_SF_7			0x30c	// 0.3
#define DS_CPU_OP_SF_8			0x20b	// 0.2
#define DS_CPU_OP_SF_9			0x10a	// 0.1
#endif

/* The conversion macros between index and mhz/mv */
#if 0	// To do. The following is for Pentium M
#define DS_MHZMV2INDEX(mhz, mv)			((((mhz)/100)<<8)|(((mv)-700)/16))
#endif

/* WARNING! Not precise! */
#ifdef CONFIG_CPU_OVERCLOCK
#define DS_INDEX2MHZ(index) \
((index)==1360000000 ? 1360 : \
((index)==1340000000 ? 1340 : \
((index)==1320000000 ? 1320 : \
((index)==1300000000 ? 1300 : \
((index)==1200000000 ? 1200 : \
((index)==1100000000 ? 1100 : \
((index)==1000000000 ? 1000 : \
((index)==900000000 ? 900 : \
((index)==800000000 ? 800 : \
((index)==700000000 ? 700 : \
((index)==600000000 ? 600 : \
((index)==500000000 ? 500 : \
((index)==400000000 ? 400 : \
((index)==300000000 ? 300 : \
((index)==220000000 ? 220 : 120)))))))))))))))

#define DS_INDEX2MHZPRECISE(index) \
((index)==1360000000 ? 1360 : \
((index)==1340000000 ? 1340 : \
((index)==1320000000 ? 1320 : \
((index)==1300000000 ? 1300 : \
((index)==1200000000 ? 1200 : \
((index)==1100000000 ? 1100 : \
((index)==1000000000 ? 1000 : \
((index)==900000000 ? 900 : \
((index)==800000000 ? 800 : \
((index)==700000000 ? 700 : \
((index)==600000000 ? 600 : \
((index)==500000000 ? 500 : \
((index)==400000000 ? 400 : \
((index)==300000000 ? 300 : \
((index)==220000000 ? 220 : 120)))))))))))))))
#else
#define DS_INDEX2MHZ(index) \
((index)==1000000000 ? 1000 : \
((index)==900000000 ? 900 : \
((index)==800000000 ? 800 : \
((index)==700000000 ? 700 : \
((index)==600000000 ? 600 : \
((index)==500000000 ? 500 : \
((index)==400000000 ? 400 : \
((index)==300000000 ? 300 : \
((index)==300000000 ? 220 : 120)))))))))

#define DS_INDEX2MHZPRECISE(index) \
((index)==1000000000 ? 1000 : \
((index)==900000000 ? 900 : \
((index)==800000000 ? 800 : \
((index)==700000000 ? 700 : \
((index)==600000000 ? 600 : \
((index)==500000000 ? 500 : \
((index)==400000000 ? 400 : \
((index)==300000000 ? 300 : \
((index)==300000000 ? 220 : 120)))))))))
#endif

#if 0	// To do. The following is for Pentium M
#define DS_INDEX2MV(index) 	(((int)(index)&0xff)*16+700)
#endif

#ifdef CONFIG_CPU_OVERCLOCK
#define DS_INDEX2NR(index) \
((index)==1360000000 ? 1 : \
((index)==1340000000 ? 2 : \
((index)==1320000000 ? 3 : \
((index)==1300000000 ? 4 : \
((index)==1200000000 ? 5 : \
((index)==1100000000 ? 6 : \
((index)==1000000000 ? 7 : \
((index)==900000000 ? 8 : \
((index)==800000000 ? 9 : \
((index)==700000000 ? 10 : \
((index)==600000000 ? 11 : \
((index)==500000000 ? 12 : \
((index)==400000000 ? 13 : \
((index)==300000000 ? 14 : \
((index)==220000000 ? 15 : 16)))))))))))))))

#define DS_INDEX2SF(index) \
((index)==1360000000 ? 0x1000 : \
((index)==1340000000 ? 0x96c : \
((index)==1320000000 ? 0x95b : \
((index)==1300000000 ? 0x94a : \
((index)==1200000000 ? 0x93c: \
((index)==1100000000 ? 0x92b : \
((index)==1000000000 ? 0x91a : \
((index)==900000000 ? 0x90c : \
((index)==800000000 ? 0x80b : \
((index)==700000000 ? 0x70a : \
((index)==600000000 ? 0x60c : \
((index)==500000000 ? 0x50b : \
((index)==400000000 ? 0x40a : \
((index)==300000000 ? 0x30c : \
((index)==220000000 ? 0x20b : 0x10a)))))))))))))))
#else
#define DS_INDEX2NR(index) \
((index)==1000000000 ? 0 : \
((index)==900000000 ? 1 : \
((index)==800000000 ? 2 : \
((index)==700000000 ? 3 : \
((index)==600000000 ? 7 : \
((index)==500000000 ? 5 : \
((index)==400000000 ? 6 : \
((index)==300000000 ? 7 : \
((index)==220000000 ? 8 : 9)))))))))

#define DS_INDEX2SF(index) \
((index)==1000000000 ? 0x1000 : \
((index)==900000000 ? 0x90c : \
((index)==800000000 ? 0x80b : \
((index)==700000000 ? 0x70a : \
((index)==600000000 ? 0x60c : \
((index)==500000000 ? 0x50b : \
((index)==400000000 ? 0x40a : \
((index)==300000000 ? 0x30c : \
((index)==220000000 ? 0x20b : 0x10a)))))))))
#endif

/* For ds_status.cpu_mode */
#define DS_CPU_MODE_IDLE		0
#define DS_CPU_MODE_TASK		1
#define DS_CPU_MODE_SCHEDULE	2
#define DS_CPU_MODE_DVS_SUITE	4

/* For ds_configuration.sched_scheme */
#define DS_SCHED_LINUX_NATIVE	0
#define DS_SCHED_GPSCHED		1

/* For ds_configuration.dvs_scheme */
// For now, dvs_scheme of Swift is fixed to DS_DVS_GPSCHEDVS.
#define DS_DVS_NO_DVS			0
#define DS_DVS_MIN				1
#define DS_DVS_GPSCHEDVS		2
#define DS_DVS_MANUAL			99

/* For do_dvs_suite() */
#define DS_ENTRY_RET_FROM_SYSTEM_CALL	0
#define DS_ENTRY_SWITCH_TO				1

/* The macro to convert an unsigned long type to U(20,12) fixed point.
	Result is U(20,12) fixed point. */
#define DS_ULONG2FP12(x)		((x)<<12)
/* The macro to extract the integer part of the given U(20,12) fixed point
	number. Result is unsigned long. */
#define DS_GETFP12INT(x)		(((x)&0xfffff000)>>12)
/* The macro to extract the fraction part of the given U(20,12) fixed point
	number. Result is U(20,12) fixed point. */
#define DS_GETFP12FRA(x)		((x)&0x00000fff)

/* Definitions for compare44bits() */
#define DS_LARGER			1
#define DS_EQUAL 			0
#define DS_SMALLER			-1

/* A representative HRT task in a Smartphone is voice call.
 * To do. Implement it in the future. For this time, we ignore such a HRT task.
 */
#define DS_CONDITION_FOR_HRT		0

/* Process static priority */
#define DS_LINUX_DEFAULT_STATIC_PRIO	120

//#define DS_HRT_STATIC_PRIO		100		// 100 nice -20
#define DS_HRT_STATIC_PRIO		105		// 100 nice -15
#define DS_DBSRT_STATIC_PRIO	110		// 110 nice -10
#define DS_RBSRT_STATIC_PRIO	115		// 115 nice	-5
#define DS_NRT_STATIC_PRIO		120		// 120 nice	0
#define DS_IDLE_PRIO			140

//#define DS_HRT_NICE				-20		// -20
#define DS_HRT_NICE				-15		// -15
#define DS_DBSRT_NICE			-10		// -10
#define DS_RBSRT_NICE			-5		// -5
#define DS_NRT_NICE				0		// 0

/* Process rt_priority. 
	p->prio = p->normal_prio = 99 - p->rt_priority for SCHED_RR tasks.
	p->prio = p->normal_prio = p->static_prio for SCHED_NORMAL tasks.
 */
#define DS_HRT_RR_PRIO			29	// p->prio = 70
#define DS_DBSRT_RR_PRIO		19	// p->prio = 80
#define DS_RBSRT_RR_PRIO		9	// p->prio = 90

/* Scheduler type.
 */
#define DS_SCHED_NORMAL			0
#define DS_SCHED_RR				1

/* Process type.
 */
#define DS_HRT_TASK				1	// HRT
#define DS_SRT_UI_SERVER_TASK	2	// DBSRT
#define DS_SRT_UI_CLIENT_TASK	3	// DBSRT
#define DS_SRT_KERNEL_THREAD	4	// RBSRT
#define DS_SRT_DAEMON_TASK		5	// RBSRT
#define DS_NRT_TASK				6	// NRT

#define DS_MIN_RT_SCHED_TYPE	DS_SRT_UI_CLIENT_TASK	

/* If a DS_SRT_UI_CLIENT_TASK does not interact with DS_SRT_UI_SERVER_TASK
 * for over DS_SRT_UI_IPC_TIMEOUT, we re-define its type.
 * The new type depends on conditions.
 */
#define DS_SRT_UI_IPC_NO		3			//
#define DS_SRT_UI_IPC_TIMEOUT	500000		// 500 msec. It should not be larger than 1sec.
#define DS_TOUCH_TIMEOUT		980000		// 980 msec. Don't touch this. LG standard.

/* The maximum allowable number of PID. 0 ~ 32767. */
#define DS_PID_LIMIT			32768

/* Definitions for AIDVS */
/* DS_AIDVS_SPEEDUP_THRESHOLD and DS_AIDVS_SPEEDUP_INTERVAL 
 * should be less than 1000000 */
#define DS_AIDVS_MOVING_AVG_WEIGHT		3			/* 3 */
#define DS_AIDVS_INTERVALS_IN_AN_WINDOW	100			/* 100 intervals in an window */
#define DS_AIDVS_SPEEDUP_THRESHOLD		100000		/* 100 msec in fse */
#define DS_AIDVS_SPEEDUP_INTERVAL		100000		/* 100 msec in fse */

/* Definitions for GPScheDVS */
/* Following THRESHOLD and INTERVAL values should be less than 1000000.
 */
#define DS_GPSCHEDVS_L0_INTERVALS_IN_AN_WINDOW	100		/* Max. intervals in an window */
#define DS_GPSCHEDVS_L0_MIN_WINDOW_LENGTH	10000		/* 10 msec in elapsed */
#define DS_GPSCHEDVS_L0_SPEEDUP_THRESHOLD	3000		/* 3 msec in fse */
#define DS_GPSCHEDVS_L0_SPEEDUP_INTERVAL	3000		/* 3 msec in fse */

#define DS_GPSCHEDVS_L1_INTERVALS_IN_AN_WINDOW	100		/* Max. intervals in an window */
#define DS_GPSCHEDVS_L1_MIN_WINDOW_LENGTH	30000		/* 25 msec in elapsed */
#define DS_GPSCHEDVS_L1_SPEEDUP_THRESHOLD	5000		/* 5 msec in fse */
#define DS_GPSCHEDVS_L1_SPEEDUP_INTERVAL	5000		/* 5 msec in fse */

#define DS_GPSCHEDVS_L2_INTERVALS_IN_AN_WINDOW	100		/* Max. intervals in an window */
#define DS_GPSCHEDVS_L2_MIN_WINDOW_LENGTH	40000		/* 50 msec in elapsed */
#define DS_GPSCHEDVS_L2_SPEEDUP_THRESHOLD	50000		/* 10 msec in fse */
#define DS_GPSCHEDVS_L2_SPEEDUP_INTERVAL	50000		/* 10 msec in fse */

#define DS_GPSCHEDVS_L3_INTERVALS_IN_AN_WINDOW	100		/* Max. intervals in an window */
#define DS_GPSCHEDVS_L3_MIN_WINDOW_LENGTH	80000		/* 100 msec in elapsed */
#define DS_GPSCHEDVS_L3_SPEEDUP_THRESHOLD	100000		/* 20 msec in fse */
#define DS_GPSCHEDVS_L3_SPEEDUP_INTERVAL	100000		/* 20 msec in fse */

#define DS_MIN_CPU_OP_UPDATE_INTERVAL_U		10000			/* 10 msec */
#define DS_MIN_CPU_OP_UPDATE_INTERVAL_D		10000			/* 10 msec */

#define DS_INIT_DELAY_SEC					20				/* 20 seconds */
#define DS_POST_EARLY_SUSPEND_TIMEOUT_USEC	2000000			/* 2 seconds */		

/***************************************************************************
 * Variables and data structures
 ***************************************************************************/

extern struct timeval ds_timeval;

typedef struct dvs_suite_configuration DS_CONF;
typedef struct dvs_suite_status DS_STAT;
typedef struct dvs_suite_counter DS_COUNT;
typedef struct dvs_suite_parameter DS_PARAM;

typedef struct ds_aidvs_interval_structure DS_AIDVS_INTERVAL_STRUCT;
typedef struct ds_aidvs_status_structure DS_AIDVS_STAT_STRUCT;

typedef struct ds_aidvs_l3_status_structure DS_AIDVS_L3_STAT_STRUCT;
typedef struct ds_aidvs_l2_status_structure DS_AIDVS_L2_STAT_STRUCT;
typedef struct ds_aidvs_l1_status_structure DS_AIDVS_L1_STAT_STRUCT;
typedef struct ds_aidvs_l0_status_structure DS_AIDVS_L0_STAT_STRUCT;
typedef struct ds_gpschedvs_status_structure DS_GPSCHEDVS_STAT_STRUCT;

//////////////////////////////////////////////////////////////////////////////////////

/* The dvs_suite configuration structure.
	 All the fields of this structure are adjusted by
	 the Loadable Kernel Module (LKM) dvs_suite_mod.

	 Field dvs_scheme tells the DVS scheme to force.
	 Possible values are as follows:

	 0: Constantly forcing the maximum CPU_OP.,
			i.e., CPU_OP0.
	 1: Constantly forcing the minimum CPU_OP.,
			i.e., CPU_OP4 for Swift
	 2: AIDVS
	 3: GPScheDVS
	 99: MANUAL

	 Field on_dvs tells whether to force the chosen DVS scheme or not.
	 If on_dvs is set, the DVS scheme indicated by field dvs_scheme is forced.
	 If on_dvs is reset, the maximum CPU_OP is forced as default.
 */
struct dvs_suite_configuration {

	/* For scheduling scheme
	 */
	int sched_scheme;

	/* For dvs_suite
	 */
	int dvs_scheme;
	int on_dvs;

	/* For DVS schemes
	 */

	/* GPSDVS and GPScheDVS strategies
		0: System energy centric strategy.
		1: CPU power centric strategy. */
	int gpsdvs_strategy;
	int gpschedvs_strategy;

	int aidvs_interval_window_size;
	unsigned long aidvs_speedup_threshold;
	unsigned long aidvs_speedup_interval;
};

extern DS_CONF ds_configuration;

////////////////////////////////////////////////////////////////////////////////////////////

/* The current system status.

	 Field ds_initialized indicates that whether dvs_suite is initialized or not.
	 The initialization process sets ds_status.cpu_op_index
	 to what the CPU is currently running at.

	 Field flag_time_base_initialized indicates whether
	 tv_usec_base were initialized or not.
	 Field tv_usec_base holds the last read ds_timeval.tv_usec value.
	 We use do_gettimeofday() to get the elapsed real time.

	 Field cpu_op_index indicates the current CPU_OP index.

	 Field cpu_op_index_sf indicates the current CPU_OP scalinf factor.

	 Field cpu_op_index_nr indicates the integer number corresponding to the
	 current CPU_OP index.
	 They are 0 for CPU_OP_INDEX_0, 1 for CPU_OP_INDEX_1, and so forth.

	 Field cpu_op_mhz indicates the MHz value in fixed point format
	 corresponding to the current CPU_OP index.

	 Field cpu_mode indicates the current cpu mode among that
	 cpu is idle (0: DS_CPU_MODE_IDLE),
	 cpu is busy while running a task (1: DS_CPU_MODE_TASK),
	 cpu is busy while performing schedule() (2: DS_CPU_MODE_SCHEDULE), and
	 cpu is busy while running dvs_suite related codes (4: DS_CPU_MODE_DVS_SUITE).

	 Fields current_dvs_scheme and dvs_is_on hold the current state of the
	 corresponding fields in ds_configuration and are used to check if the
	 corresponding feature should be initialized or not.

	 Field flag_just_mm_interacted indicates the fact that the current task
	 just made an interaction with media server for a multimedia transaction.
	 Field flag_just_uie_interacted indicates the fact that the current task
	 just made an interaction with system server for an user input event transaction.

	 Field ms_pid is the pid of media server.
	 Field ss_pid is the pid of system server.

	 Field *pti[DS_PID_LIMIT] is the array holding the pointers to per task
	 information data structures.
	 The index for pti is the PID of tasks.
	 This data structure is used to trace the type of tasks.
 */
struct dvs_suite_status {

	int flag_run_dvs;

	int ds_initialized;

	int flag_time_base_initialized;
	unsigned long tv_sec_curr;
	unsigned long tv_usec_curr;
	unsigned long tv_sec_base;
	unsigned long tv_usec_base;

	unsigned int cpu_op_index;
	unsigned int cpu_op_sf;
	int cpu_op_index_nr;
	int cpu_op_mhz;
	int cpu_mode;

	int flag_update_cpu_op;
	unsigned int target_cpu_op_index;
	unsigned long cpu_op_last_update_sec;
	unsigned long cpu_op_last_update_usec;

	int current_dvs_scheme;

	int scheduler[DS_PID_LIMIT];

	int type[DS_PID_LIMIT];
	int type_fixed[DS_PID_LIMIT];
	int type_need_to_be_changed[DS_PID_LIMIT];

	int tgid[DS_PID_LIMIT];
	char tg_owner_comm[DS_PID_LIMIT][16];
	int tgid_type_changed[DS_PID_LIMIT];
	int tgid_type_change_causer[DS_PID_LIMIT];

	unsigned long ipc_timeout_sec[DS_PID_LIMIT];
	unsigned long ipc_timeout_usec[DS_PID_LIMIT];

	int flag_touch_timeout_count;
	unsigned long touch_timeout_sec;
	unsigned long touch_timeout_usec;

	int flag_mutex_lock_on_clock_state;
	int mutex_lock_on_clock_state_cnt;
	int flag_correct_cpu_op_update_path;

	int flag_post_early_suspend;
	unsigned long post_early_suspend_sec;
	unsigned long post_early_suspend_usec;
	int flag_do_post_early_suspend;

	unsigned long mpu_min_freq_to_lock;
	unsigned long l3_min_freq_to_lock;
	unsigned long iva_min_freq_to_lock;
};

extern DS_STAT ds_status;

////////////////////////////////////////////////////////////////////////////////////////////

/* The data structure holding various counters.

	 The first type of counters are time counters which holds the fractions of
	 busy (= task + context switch + dvs suite, i.e. the overhead) and idle time
	 at each CPU_OP.
	 Additionally, the full speed equivalent sec and usec values are calculated
	 and stored.
	 fp12_*_fse_fraction holds the fraction smaller than 1 usec in U(20,12)
	 fixed point format.

	 elapsed_sec and elapsed_usec hold the total elapsed time (wall clock time)
	 since the system was booted.

	 The second type of counters hold the occurrence number of certain events
	 such as CPU_OP transitions, schedules, and total number of system calls
	 and interrupts.
 */
struct dvs_suite_counter {
	unsigned long elapsed_sec;
	unsigned long elapsed_usec;

#if 0
	unsigned long idle_sec[DS_CPU_OP_LIMIT];
	unsigned long idle_usec[DS_CPU_OP_LIMIT];
	unsigned long idle_total_sec;
	unsigned long idle_total_usec;

	unsigned long busy_sec[DS_CPU_OP_LIMIT];
	unsigned long busy_usec[DS_CPU_OP_LIMIT];
#endif
	unsigned long busy_total_sec;
	unsigned long busy_total_usec;
	unsigned long busy_fse_sec;
	unsigned long busy_fse_usec;
	unsigned long busy_fse_usec_fra_fp12;

#if 0
	unsigned long busy_task_sec[DS_CPU_OP_LIMIT];
	unsigned long busy_task_usec[DS_CPU_OP_LIMIT];
	unsigned long busy_task_total_sec;
	unsigned long busy_task_total_usec;
	unsigned long busy_task_fse_sec;
	unsigned long busy_task_fse_usec;
	unsigned long busy_task_fse_usec_fra_fp12;

	/* The busy time caused by HRT tasks */
	unsigned long busy_hrt_task_sec;
	unsigned long busy_hrt_task_usec;
	unsigned long busy_hrt_task_fse_sec;
	unsigned long busy_hrt_task_fse_usec;
	unsigned long busy_hrt_task_fse_usec_fra_fp12;
	/* The busy time caused by DBSRT tasks */
	unsigned long busy_dbsrt_task_sec;
	unsigned long busy_dbsrt_task_usec;
	unsigned long busy_dbsrt_task_fse_sec;
	unsigned long busy_dbsrt_task_fse_usec;
	unsigned long busy_dbsrt_task_fse_usec_fra_fp12;
	/* The busy time caused by RBSRT tasks */
	unsigned long busy_rbsrt_task_sec;
	unsigned long busy_rbsrt_task_usec;
	unsigned long busy_rbsrt_task_fse_sec;
	unsigned long busy_rbsrt_task_fse_usec;
	unsigned long busy_rbsrt_task_fse_usec_fra_fp12;
	/* The busy time caused by NRT tasks */
	unsigned long busy_nrt_task_sec;
	unsigned long busy_nrt_task_usec;
	unsigned long busy_nrt_task_fse_sec;
	unsigned long busy_nrt_task_fse_usec;
	unsigned long busy_nrt_task_fse_usec_fra_fp12;

	unsigned long busy_schedule_sec[DS_CPU_OP_LIMIT];
	unsigned long busy_schedule_usec[DS_CPU_OP_LIMIT];
	unsigned long busy_schedule_total_sec;
	unsigned long busy_schedule_total_usec;
	unsigned long busy_schedule_fse_sec;
	unsigned long busy_schedule_fse_usec;
	unsigned long busy_schedule_fse_usec_fra_fp12;

	unsigned long busy_dvs_suite_sec[DS_CPU_OP_LIMIT];
	unsigned long busy_dvs_suite_usec[DS_CPU_OP_LIMIT];
	unsigned long busy_dvs_suite_total_sec;
	unsigned long busy_dvs_suite_total_usec;
	unsigned long busy_dvs_suite_fse_sec;
	unsigned long busy_dvs_suite_fse_usec;
	unsigned long busy_dvs_suite_fse_usec_fra_fp12;

	unsigned long ds_cpu_op_adjustment_no;

	unsigned long schedule_no;
	unsigned long ret_from_system_call_no;
#endif
};

extern DS_COUNT ds_counter;

/* The parameters for do_dvs_suite().

	 Field entry_type indicates where do_dvs_suite() was called.
	 entry_type DS_ENTRY_RET_FROM_SYSTEM_CALL indicates that
	 do_dvs_suite() was called at the beginning of ENTRY(ret_from_system_call)
	 defined in linux/arch/i386/kernel/entry.S.
	 entry_type DS_ENTRY_SWITCH_TO indicates that do_dvs_suite() was called
	 at the end of __switch_to() macro defined in
	 linux/arch/i386/kernel/process.c.

	 Fields prev_p and next_p are set only before calling do_dvs_suite()
	 in __switch_to() macro.
	 When calling do_dvs_suite() from ENTRY(ret_from_system_call),
	 these fields should be ignored.
 */
struct dvs_suite_parameter {
	int entry_type;
	struct task_struct *prev_p;
	struct task_struct *next_p;
};

extern DS_PARAM ds_parameter;

////////////////////////////////////////////////////////////////////////////////////////////

/*
 * Variables for AIDVS.
 */
struct ds_aidvs_interval_structure {
	unsigned long time_usec_interval;
	unsigned long time_usec_work_fse;
	unsigned long time_usec_work;
//	struct ds_aidvs_interval_structure *next;
};

struct ds_aidvs_status_structure {
	int base_initialized;
	int flag_in_busy_half;

	unsigned long time_usec_interval;
	unsigned long time_usec_interval_inc_base;
	unsigned long time_sec_interval_inc_base;

	unsigned long time_usec_work_fse;
	unsigned long time_usec_work_fse_inc_base;
	unsigned long time_usec_work_fse_lasting;

	unsigned long time_usec_work;
	unsigned long time_usec_work_inc_base;
	unsigned long time_usec_work_lasting;

	DS_AIDVS_INTERVAL_STRUCT interval_window_array[DS_AIDVS_INTERVALS_IN_AN_WINDOW];
	int interval_window_index;
//	DS_AIDVS_INTERVAL_STRUCT *interval_window_head;
//	DS_AIDVS_INTERVAL_STRUCT *interval_window_tail;

	unsigned long time_usec_interval_in_window;
	unsigned long time_usec_work_fse_in_window;
	unsigned long time_usec_work_in_window;

	int consecutive_speedup_count;
	unsigned long utilization_int_ulong;
	unsigned long utilization_fra_fp12;
	unsigned long time_usec_util_calc_base;
	unsigned long time_sec_util_calc_base;

	unsigned int cpu_op_index;
};

extern DS_AIDVS_STAT_STRUCT ds_aidvs_status;

/*
 * Variables for GPScheDVS.
 */
struct ds_gpschedvs_status_structure {
	int current_strategy;
	/* Workload includes HRT + DBSRT + RBSRT + NRT tasks, i.e., all tasks. */
	DS_AIDVS_STAT_STRUCT aidvs_l3_status;
	/* Workload includes HRT + DBSRT + RBSRT tasks */
	DS_AIDVS_STAT_STRUCT aidvs_l2_status;
	/* Workload includes HRT + DBSRT tasks */
	DS_AIDVS_STAT_STRUCT aidvs_l1_status;
	/* Workload includes HRT tasks */
	DS_AIDVS_STAT_STRUCT aidvs_l0_status;
};

extern DS_GPSCHEDVS_STAT_STRUCT ds_gpschedvs_status;

////////////////////////////////////////////////////////////////////////////////////////////

/*
 * Other global vairables
 */

/***************************************************************************
 * Function definitions
 ***************************************************************************/

extern void ds_fpmul12(unsigned long, unsigned long, unsigned long *);
extern void ds_fpdiv12(unsigned long, unsigned long,
						 unsigned long *, unsigned long *);
extern void ds_div32(unsigned long, unsigned long,
					 unsigned long *, unsigned long *);
extern int ds_find_first1_in_integer_part(unsigned long);
extern int ds_find_first1_in_fraction_part(unsigned long);
extern int ds_compare45bits(int, unsigned long, unsigned long,
							int, unsigned long, unsigned long);
extern int ds_shiftleft44bits(int, unsigned long, unsigned long,
								int, int *, unsigned long *, unsigned long *);
extern int ds_subtract44bits(int, unsigned long, unsigned long,
							 int, unsigned long, unsigned long,
							 int *, unsigned long *, unsigned long *);
extern int ds_fpmul(unsigned long, unsigned long, unsigned long, unsigned long,
					unsigned long *, unsigned long *);
extern int ds_fpdiv(unsigned long, unsigned long, unsigned long, unsigned long,
					unsigned long *, unsigned long *);

extern unsigned int ds_get_next_high_cpu_op_index(unsigned long, unsigned long);
extern unsigned int ds_get_next_low_cpu_op_index(unsigned long, unsigned long);

/*
 * The functions for each DVS scheme.
 */

/* AIDVS */
extern int ds_do_dvs_aidvs(unsigned int *, DS_AIDVS_STAT_STRUCT *,
							 int, int, unsigned long, unsigned long, unsigned long);

/* GPScheDVS */
extern int ds_do_dvs_gpschedvs(unsigned int *);

/*
 * Wrappers to be used in the existing kernel codes 
 * to call the main dvs suite function.
 */
extern asmlinkage void ld_update_cpu_op(void);
extern int ld_initialize_dvs_suite(int);
extern int ld_update_time_counter(void);
extern int ld_update_priority_normal(struct task_struct *);
extern void ld_do_dvs_suite(void);

/*
 * The main dvs suite function.
 */
extern int ds_initialize_dvs_suite(int);
extern int ds_initialize_iaqos_trace(void);
extern int ds_initialize_cmqos_trace(void);
extern int ds_initialize_job_trace(void);
extern int ds_initialize_dvs_scheme(int);
extern int ds_update_time_counter(void);
extern int ds_update_priority_normal(struct task_struct *);
extern int ds_update_priority_rt(struct task_struct *);
//extern void ds_update_cpu_op(void);
extern asmlinkage void ds_update_cpu_op(void);
extern int ds_detect_task_type(void);
extern void do_dvs_suite(void);

#endif /* !(_LINUX_DVS_SUITE_H) */

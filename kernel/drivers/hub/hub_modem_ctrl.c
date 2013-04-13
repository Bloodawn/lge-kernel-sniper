/* * drivers/lg_fw/hub_modem_ctrl.c *
 * Copyright (C) 2010 LGE Inc. <james.jang@lge.com>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file "COPYING" in the main directory of this archive
 * for more details.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <mach/gpio.h>
#include <plat/mux.h>
#include <mach/hardware.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/wakelock.h>
#include <linux/jiffies.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>

#ifndef mschoi //fota
#include "../mux.h"
#endif

#define DEBUG 0
#define FEATURE_MODEM_RESET_CTRL 1
#define FEATURE_MODEM_POWER_CTRL 1
#define CONFIG_LGE_MDM_FOTA

#ifdef CONFIG_LGE_LAB3_BOARD
#define RESET_MDM 26
#define MDM_PWR_ON 27

#define MODEM_RESET_CTRL_GPIO RESET_MDM
#define MODEM_POWER_CTRL_GPIO MDM_PWR_ON
//#else /* DCM */
//#define MODEM_RESET_CTRL_GPIO 26
//#define MODEM_POWER_CTRL_GPIO 27
#endif

/* sysfs info. */
/*
	/sys/devices/platform/modem_ctrl.0/power_ctrl
	/sys/devices/platform/modem_ctrl.0/resetr_ctrl
	/sys/devices/platform/modem_ctrl.0/uart_path
*/

struct modem_ctrl_struct {
	int power_ctrl;
	int reset_ctrl;
	int uart_path;
};

/* LGE_CHANGE_S [LS855:bking.moon@lge.com] 2011-08-03, */ 
#if 1 /* upgrade ril recovery */
int ril_fail = 0;
#endif 
/* LGE_CHANGE_E [LS855:bking.moon@lge.com] 2011-08-03 */

static struct modem_ctrl_struct modem_ctrl;
static int initialized = 0;

#ifdef CONFIG_HUB_MUIC
typedef enum {
	USIF_AP,	// 0
	USIF_DP3T,	// 1
} TYPE_USIF_MODE;
void usif_switch_ctrl(TYPE_USIF_MODE mode);
#endif

#define ADDR_GPIO_CTRL_1    IO_ADDRESS(0x48310030)
#define ADDR_GPIO_OE_1      IO_ADDRESS(0x48310034)
#define ADDR_GPIO_DATAIN_1  IO_ADDRESS(0x48310038)
#define ADDR_GPIO_DATAOUT_1 IO_ADDRESS(0x4831003C)

#if 1 /* LG_RIL_RECOVERY_MODE_D2 */
#define MDM_RIL_INIT_CHEC 172
static struct delayed_work cp_boot_check_workq;
int cp_ril_init = 0;
int fota_prgress = 0;
static irqreturn_t mdm_ril_init_check_handler(int irq, void *data)
{
	cancel_delayed_work(&cp_boot_check_workq);
	cp_ril_init = 1;
	printk("%s: ****************CP ril ready(cp_ril_init)\n", __func__);
	return IRQ_HANDLED;
}

extern void modem_reset_restart_to_ril_recovery();
void cp_boot_check_workq_func(void *data)
{
	cancel_delayed_work(&cp_boot_check_workq);
	
	if( 1 == fota_prgress ) {
		printk("%s: ***** Skip CP No Boot Check, Fota Progress.. \n");
		return ;
	}

	if ( cp_ril_init == 0 ) {
		printk("%s: ********************* CP No Boot Restart \n", __func__);
		modem_reset_restart_to_ril_recovery();
	}
	return ;
}
#endif

/* LGE_CHANGE_S [LS855:bking.moon@lge.com] 2011-03-06, */ 
#if 1
static DECLARE_WAIT_QUEUE_HEAD(wq_modem_power_on);
static struct wake_lock modem_power_on_wake_lock;
static struct wake_lock modem_power_down_wake_lock;
static struct wake_lock ril_recover_wake_lock;
int modem_power_down_prcess = 0;
void modem_ctrl_power_down(void)
{
	if(!wake_lock_active(&modem_power_down_wake_lock)) {
		wake_lock(&modem_power_down_wake_lock);
	}

	gpio_direction_output(MODEM_RESET_CTRL_GPIO, 1);
	msleep(250);
	gpio_set_value(MODEM_RESET_CTRL_GPIO, 0);

	if(wake_lock_active(&modem_power_down_wake_lock)) {
		wake_unlock(&modem_power_down_wake_lock);
	}
}
void modem_ctrl_reset(void)
{
	if(!wake_lock_active(&modem_power_on_wake_lock)) {
		wake_lock(&modem_power_on_wake_lock);
	}

	modem_power_down_prcess = 1;
	modem_ctrl_power_down();
	gpio_set_value(MODEM_POWER_CTRL_GPIO, 1);
#if 1
	msleep(3000);
#else
	wait_event_interruptible_timeout(wq_modem_power_on, 0, 3*HZ);
#endif
	gpio_set_value(MODEM_POWER_CTRL_GPIO, 0);
	modem_power_down_prcess = 0;

	if(wake_lock_active(&modem_power_on_wake_lock)) {
		wake_unlock(&modem_power_on_wake_lock);
	}
}
#if 0
void modem_ctrl_reset_old(void)
{
	modem_power_down_prcess = 1;
	gpio_set_value(MODEM_POWER_CTRL_GPIO, 1);
	modem_ctrl_power_down();
	mdelay(3000);
	gpio_set_value(MODEM_POWER_CTRL_GPIO, 0);
	modem_power_down_prcess = 0;
}
void modem_ctrl_reset_test(int msec)
{
	gpio_set_value(MODEM_POWER_CTRL_GPIO, 1);
	modem_ctrl_power_down();
	mdelay(msec);
	gpio_set_value(MODEM_POWER_CTRL_GPIO, 0);
}
#endif
#endif 
/* LGE_CHANGE_E [LS855:bking.moon@lge.com] 2011-03-06 */

//[RIL-RECOVERY] 2011-04-26 hyunh0.cho@lge.com [S]
#define RIL_RECOVERY_MODE //ENABLE_RIL_RECOVERY_MODE
#define MODEM_RIL_RECOVERY //ENABLE_RIL_RECOVERY_MODE

#ifdef RIL_RECOVERY_MODE
#ifdef MODEM_RIL_RECOVERY
void modem_reset_restart_to_ril_recovery(void);
void modem_reset_restart_to_ril_recovery()
{
    printk("### modem_reset_restart_to_ril_recovery\n");
#if 0 //cp_reset
#if 1
	gpio_set_value(MODEM_RESET_CTRL_GPIO, 0);
	gpio_set_value(MODEM_POWER_CTRL_GPIO, 0);		
	printk("###** MODEM_RESET_CTRL_GPIO: %d\n", gpio_get_value(MODEM_RESET_CTRL_GPIO));
	printk("###** MODEM_POWER_CTRL_GPIO: %d\n", gpio_get_value(MODEM_POWER_CTRL_GPIO));	

        gpio_set_value(MODEM_RESET_CTRL_GPIO, 1);
	printk("###** MODEM_RESET_CTRL_GPIO: %d\n", gpio_get_value(MODEM_RESET_CTRL_GPIO));

        msleep(20);

	gpio_set_value(MODEM_POWER_CTRL_GPIO,1);
	printk("###** MODEM_POWER_CTRL_GPIO: %d\n", gpio_get_value(MODEM_POWER_CTRL_GPIO));
	msleep(3000);

	gpio_set_value(MODEM_RESET_CTRL_GPIO, 0);	
	printk("###**  MODEM_RESET_CTRL_GPIO: %d\n", gpio_get_value(MODEM_RESET_CTRL_GPIO));
#else
	gpio_direction_output(MODEM_RESET_CTRL_GPIO, 1);
	printk("###** MODEM_RESET: Now Current, %d\n", gpio_get_value(MODEM_RESET_CTRL_GPIO));
	
	gpio_set_value(MODEM_RESET_CTRL_GPIO,1);
	printk("###** MODEM_RESET: Initial Current, %d\n", gpio_get_value(MODEM_RESET_CTRL_GPIO));

	mdelay(200);
	gpio_set_value(MODEM_RESET_CTRL_GPIO,0);
	printk("###** MDM_RESET: After Set Low, %d\n", gpio_get_value(MODEM_RESET_CTRL_GPIO));

	mdelay(200);
	gpio_set_value(MODEM_RESET_CTRL_GPIO,1);
	printk("###** MDM_RESET: After Set High, %d\n", gpio_get_value(MODEM_RESET_CTRL_GPIO));
#endif
#else
	cp_ril_init = 0;
	if( ril_fail == 99 ) {
		panic("Ril Fail \n");
	} else {
		if(!wake_lock_active(&ril_recover_wake_lock)) {
			wake_lock(&ril_recover_wake_lock);
		}
		modem_power_down_prcess = 1;

		//msleep(2000); //delay for cp enter to dload
		modem_ctrl_reset();
		msleep(3000); // delay for CP full boot

		modem_power_down_prcess = 0;
		if(wake_lock_active(&ril_recover_wake_lock)) {
			wake_unlock(&ril_recover_wake_lock);
		}
	}
#endif
}

#endif //MODEM_RIL_RECOVERY
#endif
//[RIL-RECOVERY] 2011-04-26 hyunh0.cho@lge.com [E]

void dump_gpio_status(char *s, int d)
{
    // XXX: 20060617 temporarily disabled
#if 0
        u32 reg_value = 0;

        printk("============================================================\n");
        printk("%s: %d\n", s, d);
        printk("------------------------------------------------------------\n");
        reg_value = __raw_readl(ADDR_GPIO_CTRL_1);
        printk(" GPIO_CTRL_1:    0x%x\n", reg_value);
        reg_value = __raw_readl(ADDR_GPIO_OE_1);
        printk(" GPIO_OE_1:      0x%x\n", reg_value);
        reg_value = __raw_readl(ADDR_GPIO_DATAIN_1);
        printk(" GPIO_DATAIN_1:  0x%x\n", reg_value);
        reg_value = __raw_readl(ADDR_GPIO_DATAOUT_1);
        printk(" GPIO_DATAOUT_1: 0x%x\n", reg_value);
        printk("============================================================\n");
#endif
}

/**
 * export function
 * XXX: remove?

int hub_modem_on(void)
{
	if (initialized == 0)
		return -EAGAIN;

	gpio_set_value(MODEM_POWER_CTRL_GPIO, 1);
	modem_ctrl.power_ctrl = 1;

	return 0;
}

EXPORT_SYMBOL(hub_modem_on);

int hub_modem_poweroff(void)
{
	if (initialized == 0)
		return -EAGAIN;

	gpio_set_value(MODEM_POWER_CTRL_GPIO, 0);
	modem_ctrl.power_ctrl = 0;

	return 0;
}
EXPORT_SYMBOL(hub_modem_poweroff);
 */

/** 
 * sysfs
 */

static ssize_t modem_power_ctrl_show(struct device *dev,
				     struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s: power switch is %d\n", __FUNCTION__,
		       modem_ctrl.power_ctrl);
}

static ssize_t modem_power_ctrl_store(struct device *dev,
				      struct device_attribute *attr,
				      const char *buf, size_t count)
{
        int new_power_ctrl;

	if (count == 0)
		return 0;

	new_power_ctrl = simple_strtol(buf, NULL, 10);

	//if (modem_ctrl.power_ctrl != new_power_ctrl) {
                modem_ctrl.power_ctrl = new_power_ctrl;

                dump_gpio_status(__FUNCTION__, 1);
                //omap_mux_config("GPIO26_MODEM_POWER_CTRL_GPIO"); // XXX: 20060617 temporarily disabled
/* LGE_CHANGE_S [LS855:bking.moon@lge.com] 2011-03-06, */ 
#if 1 /* mbk_temp */ 
		gpio_set_value(MODEM_POWER_CTRL_GPIO, modem_ctrl.power_ctrl);
                gpio_direction_output(MODEM_POWER_CTRL_GPIO, modem_ctrl.power_ctrl);
#endif 
/* LGE_CHANGE_E [LS855:bking.moon@lge.com] 2011-03-06 */
                dump_gpio_status(__FUNCTION__, 2);
	//}
	printk("%s: power switch is %d\n", __FUNCTION__, modem_ctrl.power_ctrl);

	return count;
}

DEVICE_ATTR(power_ctrl, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP,
	    modem_power_ctrl_show, modem_power_ctrl_store);

static ssize_t modem_reset_ctrl_show(struct device *dev,
				     struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s: reset switch is %d\n", __FUNCTION__,
		       modem_ctrl.reset_ctrl);
}

static ssize_t modem_reset_ctrl_store(struct device *dev,
				      struct device_attribute *attr,
				      const char *buf, size_t count)
{
        int new_reset_ctrl;

	if (count == 0)
		return 0;

        new_reset_ctrl = simple_strtol(buf, NULL, 10);

	if (modem_ctrl.reset_ctrl != new_reset_ctrl) {
                modem_ctrl.reset_ctrl = new_reset_ctrl;

                dump_gpio_status(__FUNCTION__, 1);
                //omap_mux_config("GPIO26_MODEM_RESET_CTRL_GPIO"); // XXX: 20060617 temporarily disabled
/* LGE_CHANGE_S [LS855:bking.moon@lge.com] 2011-03-06, */ 
#if 1 /* mbk_temp */ 
		gpio_set_value(MODEM_RESET_CTRL_GPIO, modem_ctrl.reset_ctrl);
                gpio_direction_output(MODEM_RESET_CTRL_GPIO, modem_ctrl.reset_ctrl);
#endif 
/* LGE_CHANGE_E [LS855:bking.moon@lge.com] 2011-03-06 */
                dump_gpio_status(__FUNCTION__, 2);
	}

	printk("%s: reset switch is %d\n", __FUNCTION__,
	       modem_ctrl.reset_ctrl);
	return count;
}

DEVICE_ATTR(reset_ctrl, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP,
	    modem_reset_ctrl_show, modem_reset_ctrl_store);

#ifdef CONFIG_LGE_MDM_FOTA
#define MODEM_FOTA_CTRL_GPIO 171
//#define MODEM_FOTA_CTRL_GPIO 39
int fota_gpio_ctrl;

static ssize_t modem_fota_gpio_ctrl_show(struct device *dev,
				     struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s: fota gpio is %d\n", __FUNCTION__,
		       fota_gpio_ctrl);
}

static ssize_t modem_fota_gpio_ctrl_store(struct device *dev,
				      struct device_attribute *attr,
				      const char *buf, size_t count)
{
	if (count == 0)
		return 0;

	fota_gpio_ctrl = simple_strtol(buf, NULL, 10);

	fota_prgress = 1;

	//gpio_set_value(39, !!fota_gpio_ctrl);
	gpio_set_value(MODEM_FOTA_CTRL_GPIO, !!fota_gpio_ctrl);

	if( 99 == fota_gpio_ctrl ) {
		printk(KERN_INFO "%s: ******************* CP RESET **************\n", __func__);
#if 0
		gpio_set_value(MODEM_RESET_CTRL_GPIO, 1);
		gpio_set_value(MODEM_POWER_CTRL_GPIO, 1);
		mdelay(5000);
		gpio_set_value(MODEM_RESET_CTRL_GPIO, 0);
		mdelay(1000);
		gpio_set_value(MODEM_POWER_CTRL_GPIO, 0);
#else
		modem_ctrl_reset();
#endif
	}

	printk("%s: fota gpio is %d\n", __FUNCTION__, fota_gpio_ctrl);
	return count;
}

DEVICE_ATTR(fota_gpio_ctrl, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP,
	    modem_fota_gpio_ctrl_show, modem_fota_gpio_ctrl_store);
#endif

static ssize_t modem_uart_path_show(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	if (modem_ctrl.uart_path == 0) {
		return sprintf(buf, "%s: set to the OMAP UART\n", __FUNCTION__);
	} else {
		return sprintf(buf, "%s: set to the external connector\n",
			       __FUNCTION__);
	}
}

#if 0 
void modem_ctrl_power_down_test(int msec)
{
	int i;

	printk(KERN_INFO "%s: ********* CP TEST RESET ************** %d msec\n", __func__, msec);
	if( msec )
		modem_ctrl_reset();
	else 
		modem_ctrl_power_down();
	printk(KERN_INFO "%s: End\n", __func__);
}
#endif

static ssize_t modem_uart_path_store(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t count)
{
	if (count == 0)
		return 0;

	modem_ctrl.uart_path = simple_strtol(buf, NULL, 10);
#if 0
	//modem_ctrl_power_down_test(modem_ctrl.uart_path);
	large_mdelay(modem_ctrl.uart_path);
#else
	modem_ctrl.uart_path = !!modem_ctrl.uart_path;
#ifdef CONFIG_HUB_MUIC
	if (modem_ctrl.uart_path == 0) {
                usif_switch_ctrl(USIF_AP);
		printk("switch to the OMAP UART\n");
	} else {
                usif_switch_ctrl(USIF_DP3T);
		printk("switch to the external connector\n");
	}
#endif
#endif
	return count;
}

DEVICE_ATTR(uart_path, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP,
	    modem_uart_path_show, modem_uart_path_store);

/* LGE_CHANGE_S [LS855:bking.moon@lge.com] 2011-08-03, */ 
#if 1 /* upgrade ril recovery */
static ssize_t modem_ril_fail_show(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", ril_fail);
}

static ssize_t modem_ril_fail_store(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t count)
{
	if (count == 0)
		return 0;

	ril_fail = simple_strtol(buf, NULL, 10);

	return count;
}
DEVICE_ATTR(ril_fail, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP,
	    modem_ril_fail_show, modem_ril_fail_store);
#endif 

#if 1
extern unsigned int dss_debug;
unsigned int dss_pcp_debug = 0;
unsigned int dss_ankit_debug = 0;
int temp_dss_debug = 0;
static ssize_t modem_dss_debug_store(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t count)
{
	if (count == 0)
		return 0;

	temp_dss_debug = simple_strtol(buf, NULL, 10);

	/* dss_debug */
	if( temp_dss_debug == 11 ) {
		dss_debug = 1;
	} else if ( temp_dss_debug == 10 ) {
		dss_debug = 0;

	/* pcp_debug */
	} else if ( temp_dss_debug == 22 ) {
		dss_pcp_debug = 1;
	} else if ( temp_dss_debug == 20 ) {
		dss_pcp_debug = 0;

	/* ankit_debug */
	} else if ( temp_dss_debug == 33 ) {
		dss_ankit_debug = 1;
	} else if ( temp_dss_debug == 30 ) {
		dss_ankit_debug = 0;

	/* All Debug */
	} else if ( temp_dss_debug == 77 ) {
		dss_debug = 1;
		dss_pcp_debug = 1;
		dss_ankit_debug = 1;
	} else if ( temp_dss_debug == 70 ) {
		dss_debug = 0;
		dss_pcp_debug = 0;
		dss_ankit_debug = 0;
	}

	return count;
}
DEVICE_ATTR(dss_debug, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP,
	    NULL, modem_dss_debug_store);
#endif 

#if 1 /* LG_RIL_RECOVERY_MODE_D2 */
static ssize_t cp_ril_init_show(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", cp_ril_init);
}
static ssize_t cp_ril_init_store(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t count)
{
	if (count == 0)
		return 0;

	cp_ril_init = simple_strtol(buf, NULL, 10);

	return count;
}
DEVICE_ATTR(cp_ril_init, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP,
	    cp_ril_init_show, cp_ril_init_store);
#endif 

/* LGE_CHANGE_E [LS855:bking.moon@lge.com] 2011-08-03 */
/**
 * platform driver 
 */
static int modem_ctrl_probe(struct platform_device *pdev)
{
	int ret = 0;

#if DEBUG
        dump_gpio_status(__FUNCTION__, 1);
#endif /* DEBUG */

	INIT_DELAYED_WORK(&cp_boot_check_workq, cp_boot_check_workq_func);

	/* request gpio */
#if FEATURE_MODEM_POWER_CTRL
	ret = gpio_request(MODEM_POWER_CTRL_GPIO, "modem power switch");
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to request gpio %d\n",
			MODEM_POWER_CTRL_GPIO);
		ret = -EBUSY;
		goto err_gpio_request_power_ctrl;
	}
/* LGE_CHANGE_S [LS855:bking.moon@lge.com] 2011-03-06, */ 
#if 0 /* mbk_temp */ 
	gpio_direction_output(MODEM_POWER_CTRL_GPIO, 1);
	modem_ctrl.power_ctrl = 1;
#endif 
/* LGE_CHANGE_E [LS855:bking.moon@lge.com] 2011-03-06 */


#if DEBUG
        printk("###** MODEM_POWER_CTRL_GPIO: Initial Current, %d\n", gpio_get_value(MODEM_POWER_CTRL_GPIO));
        mdelay(1000);

	gpio_direction_output(MODEM_POWER_CTRL_GPIO, 0);
        gpio_set_value(MODEM_POWER_CTRL_GPIO, 0);
        printk("###** MODEM_POWER_CTRL_GPIO: After Set Low, %d\n", gpio_get_value(MODEM_POWER_CTRL_GPIO));
        mdelay(1000);

	gpio_direction_output(MODEM_POWER_CTRL_GPIO, 1);
        gpio_set_value(MODEM_POWER_CTRL_GPIO, 1);
        printk("###** MODEM_POWER_CTRL_GPIO: After Set High, %d\n", gpio_get_value(MODEM_POWER_CTRL_GPIO));
        mdelay(1000);
#endif /* DEBUG */
#endif // FEATURE_MODEM_POWER_CTRL

#if FEATURE_MODEM_RESET_CTRL
	ret = gpio_request(MODEM_RESET_CTRL_GPIO, "modem reset switch");
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to request gpio %d\n",
			MODEM_RESET_CTRL_GPIO);
		ret = -EBUSY;
		goto err_gpio_request_reset_ctrl;
	}
#if DEBUG
        dump_gpio_status(__FUNCTION__, 2);
#endif

/* LGE_CHANGE_S [LS855:bking.moon@lge.com] 2011-03-06, */ 
#if 1 /* mbk_temp */ 
	gpio_direction_output(MODEM_RESET_CTRL_GPIO, 0);
	modem_ctrl.reset_ctrl = 0;
#endif 
/* LGE_CHANGE_E [LS855:bking.moon@lge.com] 2011-03-06 */

#ifdef CONFIG_LGE_MDM_FOTA
	ret = gpio_request(MODEM_FOTA_CTRL_GPIO, "modem_fota_ctrl");
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to request gpio %d\n",
			MODEM_FOTA_CTRL_GPIO);
		ret = -EBUSY;
		goto err_gpio_request_fota_ctrl;
	}
	gpio_direction_output(MODEM_FOTA_CTRL_GPIO, 0);
#endif

#if DEBUG
        dump_gpio_status(__FUNCTION__, 3);

        printk("###** MODEM_RESET_CTRL_GPIO: Initial Current, %d\n", gpio_get_value(MODEM_RESET_CTRL_GPIO));

        mdelay(1000);
	gpio_direction_output(MODEM_RESET_CTRL_GPIO, 0);
        gpio_set_value(MODEM_RESET_CTRL_GPIO, 0);
        dump_gpio_status(__FUNCTION__, 4);
        printk("###** MODEM_RESET_CTRL_GPIO: After Set Low, %d\n", gpio_get_value(MODEM_RESET_CTRL_GPIO));

        mdelay(1000);
	gpio_direction_output(MODEM_RESET_CTRL_GPIO, 1);
        gpio_set_value(MODEM_RESET_CTRL_GPIO, 1);
        dump_gpio_status(__FUNCTION__, 5);
        printk("###** MODEM_RESET_CTRL_GPIO: After Set High, %d\n", gpio_get_value(MODEM_RESET_CTRL_GPIO));

        mdelay(1000);
	gpio_direction_output(MODEM_RESET_CTRL_GPIO, 0);
        gpio_set_value(MODEM_RESET_CTRL_GPIO, 0);
        dump_gpio_status(__FUNCTION__, 6);
        printk("###** MODEM_RESET_CTRL_GPIO: After Set Low, %d\n", gpio_get_value(MODEM_RESET_CTRL_GPIO));

        mdelay(1000);
	gpio_direction_output(MODEM_RESET_CTRL_GPIO, 1);
        gpio_set_value(MODEM_RESET_CTRL_GPIO, 1);
        dump_gpio_status(__FUNCTION__, 7);
        printk("###** MODEM_RESET_CTRL_GPIO: After Set High, %d\n", gpio_get_value(MODEM_RESET_CTRL_GPIO));
#endif /* DEBUG */
#endif // FEATURE_MODEM_RESET_CTRL

        modem_ctrl.uart_path = 0;

	/* sysfs */
#if FEATURE_MODEM_POWER_CTRL
	ret = device_create_file(&pdev->dev, &dev_attr_power_ctrl);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to create sysfs power\n");
		ret = -ENOMEM;
		goto err_sysfs_power_ctrl;
	}
#endif

#if FEATURE_MODEM_RESET_CTRL
	ret = device_create_file(&pdev->dev, &dev_attr_reset_ctrl);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to create sysfs power\n");
		ret = -ENOMEM;
		goto err_sysfs_reset_ctrl;
	}
#endif

	ret = device_create_file(&pdev->dev, &dev_attr_uart_path);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to create sysfs power_switch\n");
		ret = -ENOMEM;
		goto err_sysfs_uart_ctrl;
	}
#ifdef CONFIG_LGE_MDM_FOTA
	ret = device_create_file(&pdev->dev, &dev_attr_fota_gpio_ctrl);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to create sysfs power\n");
		ret = -ENOMEM;
		goto err_sysfs_fota_gpio_ctrl;
	}
#endif

/* LGE_CHANGE_S [LS855:bking.moon@lge.com] 2011-08-03, */ 
#if 1 /* upgrade ril recovery */
	ret = device_create_file(&pdev->dev, &dev_attr_ril_fail);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to create sysfs ril_fail\n");
		ret = -ENOMEM;
		goto err_sysfs_ril_fail;
	}
#endif 
/* LGE_CHANGE_E [LS855:bking.moon@lge.com] 2011-08-03 */

#if 1
	ret = device_create_file(&pdev->dev, &dev_attr_dss_debug);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to create sysfs dss_debug\n");
		ret = -ENOMEM;
		goto err_sysfs_dss_debug;
	}
#endif 

#if 1 /* LG_RIL_RECOVERY_MODE_D2 */
	ret = device_create_file(&pdev->dev, &dev_attr_cp_ril_init);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to create sysfs cp_ril_init\n");
		ret = -ENOMEM;
		goto err_sysfs_cp_ril_init;
	}

	omap_mux_init_gpio(MDM_RIL_INIT_CHEC, OMAP_PIN_INPUT_PULLDOWN | OMAP_PIN_OFF_WAKEUPENABLE);
	ret = gpio_request(MDM_RIL_INIT_CHEC, "ril init check");
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to request gpio %d\n",
			MDM_RIL_INIT_CHEC);
		ret = -EBUSY;
		goto err_gpio_request_ril_init_check;
	}
	gpio_direction_input(MDM_RIL_INIT_CHEC);

	ret = request_irq(gpio_to_irq(MDM_RIL_INIT_CHEC), 
			mdm_ril_init_check_handler, 
			IRQF_TRIGGER_RISING, 
			"CP_RIL_INIT", NULL);
	if (ret < 0) {
		printk(KERN_INFO "%s: fiailed to request_irq\n", __func__);
		goto err_gpio_request_irq_ril_init_check;
	}	

	ret = enable_irq_wake(gpio_to_irq(MDM_RIL_INIT_CHEC));
	if (ret < 0) {
		printk(KERN_DEBUG "%s: eanble irq faile. gpio %d\n", __func__, MDM_RIL_INIT_CHEC);
		goto err_request_wakeup_irq_failed;
	}

	cancel_delayed_work(&cp_boot_check_workq);
	schedule_delayed_work(&cp_boot_check_workq, 15*HZ);

	if( gpio_get_value(MDM_RIL_INIT_CHEC) ) {
		printk("%s: CP Ril initilized(cp_ril_init)\n", __func__);
		cancel_delayed_work(&cp_boot_check_workq);
		cp_ril_init = 1;
	}
#endif 

	wake_lock_init(&modem_power_on_wake_lock, WAKE_LOCK_SUSPEND, "modem_power_on");
	wake_lock_init(&modem_power_down_wake_lock, WAKE_LOCK_SUSPEND, "modem_power_down");
	wake_lock_init(&ril_recover_wake_lock, WAKE_LOCK_SUSPEND, "gr_cover");

	initialized = 1;
	return ret;

#if 1 /* LG_RIL_RECOVERY_MODE_D2 */
	disable_irq_wake(gpio_to_irq(MDM_RIL_INIT_CHEC));
err_request_wakeup_irq_failed:
	free_irq(gpio_to_irq(MDM_RIL_INIT_CHEC), NULL);
err_gpio_request_irq_ril_init_check:
	gpio_free(MDM_RIL_INIT_CHEC);
err_gpio_request_ril_init_check:
	device_remove_file(&pdev->dev, &dev_attr_cp_ril_init);
err_sysfs_cp_ril_init:
#endif 

#if 1
	device_remove_file(&pdev->dev, &dev_attr_dss_debug);
err_sysfs_dss_debug:
#endif
/* LGE_CHANGE_S [LS855:bking.moon@lge.com] 2011-08-03, */ 
#if 1 /* upgrade ril recovery */
	device_remove_file(&pdev->dev, &dev_attr_ril_fail);
err_sysfs_ril_fail:
#endif 
/* LGE_CHANGE_E [LS855:bking.moon@lge.com] 2011-08-03 */
	device_remove_file(&pdev->dev, &dev_attr_fota_gpio_ctrl);
err_sysfs_fota_gpio_ctrl:
	device_remove_file(&pdev->dev, &dev_attr_uart_path);
err_sysfs_uart_ctrl:
	device_remove_file(&pdev->dev, &dev_attr_reset_ctrl);
err_sysfs_reset_ctrl:
	device_remove_file(&pdev->dev, &dev_attr_power_ctrl);
err_sysfs_power_ctrl:
	gpio_free(MODEM_FOTA_CTRL_GPIO);
err_gpio_request_fota_ctrl:
	gpio_free(MODEM_RESET_CTRL_GPIO);
err_gpio_request_reset_ctrl:
	gpio_free(MODEM_POWER_CTRL_GPIO);
err_gpio_request_power_ctrl:
	return ret;
}

static int modem_ctrl_remove(struct platform_device *pdev)
{

	device_remove_file(&pdev->dev, &dev_attr_power_ctrl);
	device_remove_file(&pdev->dev, &dev_attr_reset_ctrl);
	device_remove_file(&pdev->dev, &dev_attr_uart_path);
#ifdef CONFIG_LGE_MDM_FOTA
	device_remove_file(&pdev->dev, &dev_attr_fota_gpio_ctrl);
#endif

/* LGE_CHANGE_S [LS855:bking.moon@lge.com] 2011-08-03, */ 
#if 1 /* upgrade ril recovery */
	device_remove_file(&pdev->dev, &dev_attr_ril_fail);
#endif 
/* LGE_CHANGE_E [LS855:bking.moon@lge.com] 2011-08-03 */
	device_remove_file(&pdev->dev, &dev_attr_dss_debug);

	gpio_free(MODEM_POWER_CTRL_GPIO);
	gpio_free(MODEM_RESET_CTRL_GPIO);

#if 1 /* LG_RIL_RECOVERY_MODE_D2 */
	device_remove_file(&pdev->dev, &dev_attr_cp_ril_init);
	disable_irq_wake(gpio_to_irq(MDM_RIL_INIT_CHEC));
	free_irq(gpio_to_irq(MDM_RIL_INIT_CHEC), NULL);
	gpio_free(MDM_RIL_INIT_CHEC);
#endif
	wake_lock_destroy(&modem_power_on_wake_lock);
	wake_lock_destroy(&modem_power_down_wake_lock);
	wake_lock_destroy(&ril_recover_wake_lock);

	initialized = 0;
	return 0;
}

static struct platform_driver modem_ctrl_driver = {
	.probe = modem_ctrl_probe,
	.remove = modem_ctrl_remove,
	.driver = {
		   .name = "modem_ctrl",
		   },
};

/**
 * module init/exit
 */
static int __init modem_ctrl_init(void)
{
	return platform_driver_register(&modem_ctrl_driver);
        return 0;
}

static void __exit modem_ctrl_exit(void)
{
	platform_driver_unregister(&modem_ctrl_driver);
}

late_initcall(modem_ctrl_init);
module_exit(modem_ctrl_exit);

MODULE_AUTHOR("LG Electronics");
MODULE_DESCRIPTION("Modem Control Driver");
MODULE_LICENSE("GPL v2");

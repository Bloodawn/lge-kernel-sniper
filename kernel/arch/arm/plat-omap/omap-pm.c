/*
 * omap-pm.c - OMAP power management interface
 *
 * Copyright (C) 2008-2010 Texas Instruments, Inc.
 * Copyright (C) 2008-2009 Nokia Corporation
 * Vishwanath BS
 *
 * This code is based on plat-omap/omap-pm-noop.c.
 *
 * Interface developed by (in alphabetical order):
 * Karthik Dasu, Tony Lindgren, Rajendra Nayak, Sakari Poussa, Veeramanikandan
 * Raju, Anand Sawant, Igor Stoppa, Paul Walmsley, Richard Woodruff
 */

#undef DEBUG

#include <linux/init.h>
#include <linux/cpufreq.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/list.h>

/* Interface documentation is in mach/omap-pm.h */
#include <plat/omap-pm.h>
#include <plat/omap_device.h>
#include <plat/powerdomain.h>
#include <plat/clockdomain.h>

/* 20110331 sookyoung.kim@lge.com LG-DVFS [START_LGE] */
#include <linux/dvs_suite.h>
/* 20110331 sookyoung.kim@lge.com LG-DVFS [END_LGE] */

#ifdef CONFIG_ARCH_OMAP1
#define MPU_CLK         "mpu"
#elif defined(CONFIG_ARCH_OMAP3)
#define MPU_CLK         "arm_fck"
#else
#define MPU_CLK         "virt_prcm_set"
#endif

struct omap_opp *dsp_opps;
struct omap_opp *mpu_opps;

static struct clk *mpu_clk = NULL;
static struct clk *iva_clk = NULL;
struct cpufreq_frequency_table *mpu_freq_table = NULL;
static struct cpufreq_frequency_table *dsp_freq_table = NULL;
static unsigned int dsp_req_id = 0;
static unsigned int mpu_req_freq = 0;
static unsigned int mpu_req_id = 0;
static int ft_count = 0;

static DEFINE_MUTEX(bus_tput_mutex);
static DEFINE_MUTEX(mpu_tput_mutex);
static DEFINE_MUTEX(mpu_lat_mutex);

/* Used to model a Interconnect Throughput */
static struct interconnect_tput {
	/* Total no of users at any point of interconnect */
	u8 no_of_users;
	/* List of all the current users for interconnect */
	struct list_head users_list;
	struct list_head node;
	/* Protect interconnect throughput */
	struct mutex throughput_mutex;
	/* Target level for interconnect throughput */
	unsigned long target_level;
	unsigned long max_level;

} *bus_tput, *mpu_tput;

/* Used to represent a user of a interconnect throughput */
struct users {
	/* Device pointer used to uniquely identify the user */
	struct device *dev;
	struct list_head node;
	/* Current level as requested for interconnect throughput by the user */
	u32 level;
};

/* Private/Internal Functions */

static struct users *max_lookup(struct interconnect_tput *list_ptr)
{
	struct users *usr, *tmp_usr;

	usr = NULL;
	list_ptr->max_level = 0;
	list_for_each_entry(tmp_usr, &list_ptr->users_list, node) {
		if (tmp_usr->level > list_ptr->max_level) {
			usr = tmp_usr;
			list_ptr->max_level = tmp_usr->level;
		}
	}

	return usr;
}



/**
 * user_lookup - look up a user by its device pointer, return a pointer
 * @dev: The device to be looked up
 *
 * Looks for a interconnect user by its device pointer. Returns a
 * pointer to
 * the struct users if found, else returns NULL.
 **/

static struct users *user_lookup(struct device *dev,
		struct interconnect_tput *list_ptr)
{
	struct users *usr, *tmp_usr;

	usr = NULL;
	list_for_each_entry(tmp_usr, &list_ptr->users_list, node) {
		if (tmp_usr->dev == dev) {
			usr = tmp_usr;
			break;
		}
	}

	return usr;
}

/**
 * get_user - gets a new users_list struct dynamically
 *
 * This function allocates dynamcially the user node
 * Returns a pointer to users struct on success. On dynamic allocation
 * failure
 * returns a ERR_PTR(-ENOMEM).
 **/

static struct users *get_user(void)
{
	struct users *user;

	user = kmalloc(sizeof(struct  users), GFP_KERNEL);
	if (!user) {
		pr_err("%s FATAL ERROR: kmalloc "
			"failed\n",  __func__);
		return ERR_PTR(-ENOMEM);
	}
	return user;
}


/**
 * omap_bus_tput_init - Initializes the interconnect throughput
 * userlist
 * Allocates memory for global throughput variable dynamically.
 * Intializes Userlist, no. of users and throughput target level.
 * Returns 0 on sucess, else returns EINVAL if memory
 * allocation fails.
 */
int omap_tput_list_init(struct interconnect_tput **list_ptr)
{
	struct interconnect_tput *tmp_ptr = NULL;
	tmp_ptr = kmalloc(sizeof(struct interconnect_tput), GFP_KERNEL);
	if (!tmp_ptr) {
		pr_err("%s FATAL ERROR: kmalloc failed\n", __func__);
		return -EINVAL;
       }
	INIT_LIST_HEAD(&tmp_ptr->users_list);
	mutex_init(&tmp_ptr->throughput_mutex);
	tmp_ptr->no_of_users = 0;
	tmp_ptr->target_level = 0;
	tmp_ptr->max_level = 0;

	*list_ptr = tmp_ptr;

	return 0;
}

/**
 * add_req_tput  - Request for a required level by a device
 * @dev: Uniquely identifes the caller
 * @level: The requested level for the interconnect bandwidth in KiB/s
 *
 * This function recomputes the target level of the interconnect
 * bandwidth
 * based on the level requested by all the users.
 * Multiple calls to this function by the same device will
 * replace the previous level requested
 * Returns the updated level of interconnect throughput.
 * In case of Invalid dev or user pointer, it returns 0.
 */
static unsigned long add_req_tput(struct device *dev,
		unsigned long level,
		struct interconnect_tput *list_ptr)
{
	int ret;
	struct  users *user;

	if (!dev) {
		pr_err("Invalid dev pointer\n");
		ret = 0;
	}
	mutex_lock(&list_ptr->throughput_mutex);
	user = user_lookup(dev, list_ptr);
	if (user == NULL) {
		user = get_user();
		if (IS_ERR(user)) {
			pr_err("Couldn't get user from the list to"
				"add new throughput constraint");
			ret = 0;
			goto unlock;
		}
		list_ptr->target_level += level;
		list_ptr->no_of_users++;
		user->dev = dev;
		list_add(&user->node, &list_ptr->users_list);
		user->level = level;
	} else {
		list_ptr->target_level -= user->level;
		list_ptr->target_level += level;
		user->level = level;
	}

	ret = list_ptr->target_level;
unlock:
	mutex_unlock(&list_ptr->throughput_mutex);
	return ret;
}


/**
 * remove_req_tput - Release a previously requested level of
 * a throughput level for interconnect
 * @dev: Device pointer to dev
 *
 * This function recomputes the target level of the interconnect
 * throughput after removing
 * the level requested by the user.
 * Returns 0, if the dev structure is invalid
 * else returns modified interconnect throughput rate.
 */
static unsigned long remove_req_tput(struct device *dev,
		struct interconnect_tput *list_ptr)
{
	struct users *user;
	int found = 0;
	int ret;

	mutex_lock(&list_ptr->throughput_mutex);
	list_for_each_entry(user, &list_ptr->users_list, node) {
		if (user->dev == dev) {
			found = 1;
			break;
		}
	}
	if (!found) {
		/* No such user exists */
		pr_err("Invalid Device Structure\n");
		ret = 0;
		goto unlock;
	}

	list_ptr->target_level -= user->level;
	list_ptr->no_of_users--;
	list_del(&user->node);

	kfree(user);
	ret = list_ptr->target_level;

unlock:
	mutex_unlock(&list_ptr->throughput_mutex);
	return ret;
}

/*
 * Device-driver-originated constraints (via board-*.c files)
 */

int omap_pm_set_max_mpu_wakeup_lat(struct pm_qos_request_list **qos_request,
					long t)
{
	if (!qos_request || t < -1) {
		WARN(1, "OMAP PM: %s: invalid parameter(s)", __func__);
		return -EINVAL;
	};

	mutex_lock(&mpu_lat_mutex);

	if (t == -1) {
// 20110426 prime@sdcmicro.com Protect from removing qos which is not created yet [START]
		if (*qos_request)
// 20110426 prime@sdcmicro.com Protect from removing qos which is not created yet [END]
		pm_qos_remove_request(*qos_request);
		*qos_request = NULL;
	} else if (*qos_request == NULL)
		*qos_request = pm_qos_add_request(PM_QOS_CPU_DMA_LATENCY, t);
	else
		pm_qos_update_request(*qos_request, t);

	mutex_unlock(&mpu_lat_mutex);
	return 0;
}


int omap_pm_set_min_bus_tput(struct device *dev, u8 agent_id, long r)
{

	int ret;
	struct device *l3_dev;
	static struct device dummy_l3_dev;
	unsigned long target_level = 0;

#if 1
	/* 20110331 sookyoung.kim@lge.com LG-DVFS [START_LGE] */
	if(ds_status.flag_run_dvs == 1){
		/* Make sure the following values consist with those in
		   kernel/arch/arm/mach-omap2/pm.c */
		switch(r){
			case 0:
				ds_status.l3_min_freq_to_lock = 0;
				break;
			case 800000:
				ds_status.l3_min_freq_to_lock = 200000000;
				break;
			case 400000:
				ds_status.l3_min_freq_to_lock = 100000000;
				break;
			default:
				ds_status.l3_min_freq_to_lock = 0;
				break;
		}
		return 0;
	}
	/* 20110331 sookyoung.kim@lge.com LG-DVFS [END_LGE] */
#endif

	if (!dev || (agent_id != OCP_INITIATOR_AGENT &&
	    agent_id != OCP_TARGET_AGENT)) {
		WARN(1, "OMAP PM: %s: invalid parameter(s)", __func__);
		return -EINVAL;
	};

	mutex_lock(&bus_tput_mutex);

	l3_dev = omap2_get_l3_device();
	if (!l3_dev) {
		pr_err("Unable to get l3 device pointer");
		ret = -EINVAL;
		goto unlock;
	}
	if (r == -1) {
		pr_debug("OMAP PM: remove min bus tput constraint for: "
			"interconnect dev %s for agent_id %d\n", dev_name(dev),
				agent_id);
		target_level = remove_req_tput(dev, bus_tput);
	} else {
		pr_debug("OMAP PM: add min bus tput constraint for: "
			"interconnect dev %s for agent_id %d: rate %ld KiB\n",
				dev_name(dev), agent_id, r);
		target_level = add_req_tput(dev, r, bus_tput);
	}

	/* Convert the throughput(in KiB/s) into Hz. */
	target_level = (target_level * 1000)/4;
	ret = omap_device_set_rate(&dummy_l3_dev, l3_dev, target_level);

	if (ret)
		pr_err("Unable to change level for interconnect bandwidth to %ld\n",
			target_level);
unlock:
	mutex_unlock(&bus_tput_mutex);
	return ret;
}

int omap_pm_set_max_dev_wakeup_lat(struct device *req_dev, struct device *dev,
				   long t)
{
	struct omap_device *odev;
	struct powerdomain *pwrdm_dev;
	struct platform_device *pdev;
	int ret = 0;

	if (!req_dev || !dev || t < -1) {
		WARN(1, "OMAP PM: %s: invalid parameter(s)", __func__);
		return -EINVAL;
	};

	/* Look for the devices Power Domain */
	pdev = container_of(dev, struct platform_device, dev);

	/* Try to catch non platform devices. */
	if (pdev->name == NULL) {
		pr_err("OMAP-PM: Error: platform device not valid\n");
		return -EINVAL;
	}

	odev = to_omap_device(pdev);
	if (odev) {
		pwrdm_dev = omap_device_get_pwrdm(odev);
	} else {
		pr_err("OMAP-PM: Error: Could not find omap_device "
			"for %s\n", pdev->name);
		return -EINVAL;
	}

	/* Catch devices with undefined powerdomains. */
	if (!pwrdm_dev) {
		pr_err("OMAP-PM: Error: could not find parent "
			"powerdomain for %s\n", pdev->name);
		return -EINVAL;
	}

	if (t == -1) {
		pr_debug("OMAP PM: remove max device latency constraint: "
			 "dev %s, pwrdm %s, req by %s\n", dev_name(dev),
				pwrdm_dev->name, dev_name(req_dev));
		ret = pwrdm_wakeuplat_release_constraint(pwrdm_dev, req_dev);
	} else {
		pr_debug("OMAP PM: add max device latency constraint: "
			 "dev %s, t = %ld usec, pwrdm %s, req by %s\n",
			 dev_name(dev), t, pwrdm_dev->name, dev_name(req_dev));
		ret = pwrdm_wakeuplat_set_constraint(pwrdm_dev, req_dev, t);
	}

	/*
	 * For current Linux, this needs to map the device to a
	 * powerdomain, then go through the list of current max lat
	 * constraints on that powerdomain and find the smallest.  If
	 * the latency constraint has changed, the code should
	 * recompute the state to enter for the next powerdomain
	 * state.  Conceivably, this code should also determine
	 * whether to actually disable the device clocks or not,
	 * depending on how long it takes to re-enable the clocks.
	 *
	 * TI CDP code can call constraint_set here.
	 */

	return ret;
}

int omap_pm_set_max_sdma_lat(struct pm_qos_request_list **qos_request,
					long t)
{
	if (!qos_request || t < -1) {
		WARN(1, "OMAP PM: %s: invalid parameter(s)", __func__);
		return -EINVAL;
	};

	if (t == -1) {
		pm_qos_remove_request(*qos_request);
		*qos_request = NULL;
	} else if (*qos_request == NULL)
		*qos_request = pm_qos_add_request(PM_QOS_CPU_DMA_LATENCY, t);
	else
		pm_qos_update_request(*qos_request, t);

	return 0;
}

int omap_pm_set_min_clk_rate(struct device *dev, struct clk *c, long r)
{
	if (!dev || !c || r < 0) {
		WARN(1, "OMAP PM: %s: invalid parameter(s)", __func__);
		return -EINVAL;
	}

	if (r == 0)
		pr_debug("OMAP PM: remove min clk rate constraint: "
			 "dev %s\n", dev_name(dev));
	else
		pr_debug("OMAP PM: add min clk rate constraint: "
			 "dev %s, rate = %ld Hz\n", dev_name(dev), r);

	/*
	 * Code in a real implementation should keep track of these
	 * constraints on the clock, and determine the highest minimum
	 * clock rate.  It should iterate over each OPP and determine
	 * whether the OPP will result in a clock rate that would
	 * satisfy this constraint (and any other PM constraint in effect
	 * at that time).  Once it finds the lowest-voltage OPP that
	 * meets those conditions, it should switch to it, or return
	 * an error if the code is not capable of doing so.
	 */

	return 0;
}

/*
 * DSP Bridge-specific constraints
 */

/*
 * Must be called after clock framework is initialized.  Also, it must wait
 * for board init to enable any frequencies that are off by default in the
 * OPP table (done only for some boards).
 *
 * Returns 0 on success, < 0 errno on error.
 */
static int initialize_tables(void)
{
	struct device *iva_dev = omap2_get_iva_device();
	struct device *mpu_dev = omap2_get_mpuss_device();
	struct omap_opp *opp;
	int i, ret = 0;

	if (!mpu_freq_table) {
		opp_init_cpufreq_table(mpu_dev, &mpu_freq_table);
		if (!mpu_freq_table) {
			ret = -ENOMEM;
			goto error;
		}
	}

	if (!dsp_freq_table) {
		opp_init_cpufreq_table(iva_dev, &dsp_freq_table);
		if (!dsp_freq_table) {
			ret = -ENOMEM;
			goto error;
		}
	}

	ft_count = opp_get_opp_count(iva_dev);
	if (ft_count != opp_get_opp_count(mpu_dev)) {
		printk(KERN_ERR "Mismatched DSP/MPU OPP count\n");
		ret = -EINVAL;
		goto error;
	}

	/*
	 * The DSP wants a table of struct omap_opp, with the first data in the
	 * first (not 0th) location and ending with a zero element.
	 */
	dsp_opps = kmalloc((ft_count + 2) * sizeof(struct omap_opp),
					GFP_KERNEL);
	if (!dsp_opps) {
		pr_err("%s FATAL ERROR: dsp_opps kmalloc failed\n",  __func__);
		ret = -ENOMEM;
		goto error;
	}

	for (i=0; i<ft_count; i++) {
		dsp_opps[i+1].enabled = 1;
		dsp_opps[i+1].rate = dsp_freq_table[i].frequency;
		dsp_opps[i+1].opp_id = dsp_freq_table[i].index + 1;
		opp = opp_find_freq_exact(iva_dev,
				dsp_freq_table[i].frequency * 1000, 1);
		dsp_opps[i+1].u_volt = opp_get_voltage(opp);
	}
	/* The table starts with and is terminated with 0s */
	dsp_opps[0].rate = dsp_opps[i+1].rate = 0;
	dsp_opps[0].opp_id = dsp_opps[i+1].opp_id = 0;
	dsp_opps[0].enabled = dsp_opps[i+1].enabled = 0;
	return ret;

error:
	if (mpu_freq_table)
		opp_exit_cpufreq_table(&mpu_freq_table);
	if (dsp_freq_table)
		opp_exit_cpufreq_table(&dsp_freq_table);
	return ret;
}

struct omap_opp *omap_pm_dsp_get_opp_table(void)
{
	pr_debug("OMAP PM: DSP request for OPP table\n");

	if (!dsp_freq_table)
		if (initialize_tables())
			return NULL;
	return dsp_opps;
}

void omap_pm_dsp_set_min_opp(u8 opp_id)
{
	struct device *mpu_dev = omap2_get_mpuss_device();
	struct device *iva_dev = omap2_get_iva_device();
	unsigned int selopp;
	unsigned int currspeed = clk_get_rate(iva_clk) / 1000;
	int r = 0;

#if 1
	/* 20110331 sookyoung.kim@lge.com LG-DVFS [START_LGE] */
	if(ds_status.flag_run_dvs == 1){
		/* Make sure the following values consist with those in
		   kernel/arch/arm/mach-omap2/pm.c */
		switch(opp_id){
			case 1:
				ds_status.mpu_min_freq_to_lock = 0;	// Unlocked.
				ds_status.iva_min_freq_to_lock = 260000000;
				break;
			case 2:
				ds_status.mpu_min_freq_to_lock = 600000000;
				ds_status.iva_min_freq_to_lock = 520000000;
				break;
			case 3:
				ds_status.mpu_min_freq_to_lock = 800000000;
				ds_status.iva_min_freq_to_lock = 660000000;
				break;
			case 4:
				ds_status.mpu_min_freq_to_lock = 1000000000;
				ds_status.iva_min_freq_to_lock = 800000000;
				break;
			default:
				ds_status.mpu_min_freq_to_lock = 1000000000;
				ds_status.iva_min_freq_to_lock = 800000000;
				break;
		}
		return 0;
	}
	/* 20110331 sookyoung.kim@lge.com LG-DVFS [END_LGE] */
#endif

	if (opp_id <= 0 || opp_id > ft_count + 1 || ft_count == 0) {
		WARN_ON(1);
		return;
	}

	pr_debug("OMAP PM: DSP requests minimum VDD1 OPP to be %d\n", opp_id);

	/* Caller sees frequency table as [1..N], we "C" it as [0..N-1]. */
	dsp_req_id = opp_id - 1;

	if (!dsp_freq_table)
		if (initialize_tables())
			return;

	/*
	 *
	 * For l-o dev tree, our VDD1 clk is keyed on OPP ID, so we
	 * can just test to see which is higher, the CPU's desired OPP
	 * ID or the DSP's desired OPP ID, and use whichever is
	 * highest.
	 *
	 * In CDP12.14+, the VDD1 OPP custom clock that controls the DSP
	 * rate is keyed on MPU speed, not the OPP ID.  So we need to
	 * map the OPP ID to the MPU speed for use with clk_set_rate()
	 * if it is higher than the current OPP clock rate.
	 *
	 */
#ifdef CONFIG_CPU_OVERCLOCK
	pr_debug("OMAP PM: Requested dfreq/mfreq: %d/%d. didx/midx: %d/%d\n",
               dsp_freq_table[dsp_req_id].frequency,
               mpu_freq_table[mpu_req_id].frequency,
               dsp_req_id, mpu_req_id);
	
	if (dsp_req_id > mpu_req_id)
   {
		struct cpufreq_policy policy;

		cpufreq_get_policy(&policy, 0);

		while (mpu_freq_table[dsp_req_id].frequency < policy.min && dsp_req_id < ft_count - 1) {
			dsp_req_id = dsp_req_id + 1;
		}
		while (mpu_freq_table[dsp_req_id].frequency > policy.max && dsp_req_id > 0) {
			dsp_req_id = dsp_req_id - 1;
		}

		selopp = dsp_req_id;
	} else {
		struct cpufreq_policy policy;

		cpufreq_get_policy(&policy, 0);

		while (mpu_freq_table[mpu_req_id].frequency < policy.min && mpu_req_id < ft_count - 1) {
			mpu_req_id = mpu_req_id + 1;
		}
		while (mpu_freq_table[mpu_req_id].frequency > policy.max && mpu_req_id > 0) {
			mpu_req_id = mpu_req_id - 1;
		}

		selopp = mpu_req_id;
	}

	/* Is a change requested? */
	if (currspeed == dsp_freq_table[selopp].frequency){
       pr_debug("OMAP PM: No freq change dfreq/mfreq: %d/%d. "
       	"didx/midx: %d/%d\n",
       	dsp_freq_table[selopp].frequency,
       	mpu_freq_table[selopp].frequency,
       	dsp_req_id, mpu_req_id);
       return;
     }

	r = omap_device_set_rate(mpu_dev, mpu_dev,
				mpu_freq_table[selopp].frequency * 1000);
	if (r)
		printk(KERN_ERR "omap_device_set_rate error %d.\n", r);
	else
		omap_device_set_rate(iva_dev, iva_dev,
				dsp_freq_table[selopp].frequency * 1000);

	pr_debug("OMAP PM: Set dfreq/mfreq: %d/%d\n",
       dsp_freq_table[selopp].frequency,
       mpu_freq_table[selopp].frequency);
#else
	if (dsp_req_id > mpu_req_id)
		selopp = dsp_req_id;
	else
		selopp = mpu_req_id;

	/* Is a change requested? */
	if (currspeed == dsp_freq_table[selopp].frequency)
		return;

	r = omap_device_set_rate(mpu_dev, mpu_dev,
				mpu_freq_table[selopp].frequency * 1000);
	if (r)
		printk(KERN_ERR "omap_device_set_rate error %d.\n",r);
	else
		omap_device_set_rate(iva_dev, iva_dev,
				dsp_freq_table[selopp].frequency * 1000);
#endif
}


u8 omap_pm_dsp_get_opp(void)
{
	struct device *iva_dev = omap2_get_iva_device();
	unsigned long rate;
	u8 i;

	pr_debug("OMAP PM: DSP requests current DSP OPP ID\n");

	if (!iva_dev)
		return 0;

	if (!dsp_freq_table)
		if (initialize_tables())
			return 0;

	rate = clk_get_rate(iva_clk) / 1000;
	for (i=1; dsp_opps[i].rate; i++) {
		if (dsp_opps[i].rate == rate) {
			return i;
		}
	}
        printk(KERN_ERR "%s: active dsp rate %ld not in dsp_opps table\n"
			,__FUNCTION__, rate);
	return 0;

}

/*
 * CPUFreq-originated constraint
 *
 * In the future, this should be handled by custom OPP clocktype
 * functions.
 */

struct cpufreq_frequency_table **omap_pm_cpu_get_freq_table(void)
{
	pr_debug("OMAP PM: CPUFreq request for frequency table\n");

	return &mpu_freq_table;
}

void omap_pm_cpu_set_freq(unsigned long f)
{
	struct device *mpu_dev = omap2_get_mpuss_device();
	unsigned int currspeed = clk_get_rate(mpu_clk) / 1000;
	struct device *iva_dev = omap2_get_iva_device();
	unsigned int selfreq;
	int i, r = 0;

	if (f == 0) {
		WARN_ON(1);
		return;
	}

	if (!mpu_freq_table)
		if (initialize_tables())
			return;

	pr_debug("OMAP PM: CPUFreq requests CPU frequency to be set to %lu\n",
		 f);

	/*
	 * For l-o dev tree, determine whether MPU freq or DSP OPP id
	 * freq is higher.  Find the OPP ID corresponding to the
	 * higher frequency.
	 *
	 * CDP should just be able to set the VDD1 OPP clock rate here.
	 */
	opp_find_freq_ceil(mpu_dev, &f);
	mpu_req_freq = f / 1000;

	/*
	 * The governor calls us to let us know what it wants for an MPU
	 * frequency.  In that case, it will already have ajusted it.
	 * Otherwise, is the request for a frequency higher than the DSP
	 * asked for?
	 */
	if (mpu_req_freq > mpu_freq_table[dsp_req_id].frequency)
		selfreq = mpu_req_freq;
	else
		selfreq = mpu_freq_table[dsp_req_id].frequency;

	if (mpu_req_freq != currspeed) {
		r = omap_device_set_rate(mpu_dev, mpu_dev, selfreq * 1000);
		if (r)
			printk(KERN_ERR "omap_device_set_rate error %d.\n",r);
	}

	/* Find the OPP and set the DSP freq too */
	for (i=0; i<ft_count; i++) {
		if (mpu_freq_table[i].frequency == selfreq) {
			omap_device_set_rate(iva_dev, iva_dev,
					dsp_freq_table[i].frequency * 1000);
			mpu_req_id = i;
			break;
		}
	}

#ifdef CONFIG_CPU_OVERCLOCK
    pr_debug("OMAP_PM: CPU set frequency - dsp/mpu: %d/%d\n",
         dsp_freq_table[mpu_req_id].frequency,
         mpu_freq_table[mpu_req_id].frequency);
#endif
}

unsigned long omap_pm_cpu_get_freq(void)
{
	unsigned long rate=0;

	pr_debug("OMAP PM: CPUFreq requests current CPU frequency\n");

	rate = clk_get_rate(mpu_clk);
	return rate / 1000;
}

/*
 * Device context loss tracking
 */

int omap_pm_get_dev_context_loss_count(struct device *dev)
{
	static u32 counter = 1;

	if (!dev) {
		WARN_ON(1);
		return -EINVAL;
	};

	pr_debug("OMAP PM: returning context loss count for dev %s\n",
		 dev_name(dev));

	/*
	 * Map the device to the powerdomain.  Return the powerdomain
	 * off counter.
	 */

	/* Let the counter roll-over: its for test only */
	return counter++;
}


/* Should be called before clk framework init */
int __init omap_pm_if_early_init()
{
	return 0;
}

int __init omap_pm_if_init(void)
{
	int ret;

	mpu_clk = clk_get(NULL, MPU_CLK);
	if (IS_ERR(mpu_clk))
		return PTR_ERR(mpu_clk);
	iva_clk = clk_get(NULL, "iva2_ck");
	if (IS_ERR(iva_clk))
		return PTR_ERR(iva_clk);

	ret = omap_tput_list_init(&bus_tput);
	if (ret)
		pr_err("Failed to initialize interconnect"
			" bandwidth users list\n");

	ret = omap_tput_list_init(&mpu_tput);
	if (ret)
		pr_err("Failed to initialize mpu frequency users list\n");

	return ret;
}
void omap_pm_if_exit(void)
{
	/* Deallocate CPUFreq frequency table here */
	if (dsp_freq_table)
		opp_exit_cpufreq_table(&dsp_freq_table);
	if (mpu_freq_table)
		opp_exit_cpufreq_table(&mpu_freq_table);
}

int omap_pm_set_min_mpu_freq(struct device *dev, unsigned long f)
{

	int ret = 0;
	struct device *mpu_dev = omap2_get_mpuss_device();
	static struct device dummy_mpu_dev;
	unsigned long old_max_level;

#if 1
	/* 20110331 sookyoung.kim@lge.com LG-DVFS [START_LGE] */
	if(ds_status.flag_run_dvs == 1){
		/* Make sure the following values consist with those in
		   kernel/arch/arm/mach-omap2/pm.c */
		switch(f){
			case -1:
				ds_status.mpu_min_freq_to_lock = 0;	// Unlocked.
				break;
			case 600000000:
				ds_status.mpu_min_freq_to_lock = 600000000;
				break;
			case 800000000:
				ds_status.mpu_min_freq_to_lock = 800000000;
				break;
			case 1000000000:
				ds_status.mpu_min_freq_to_lock = 1000000000;
				break;
			default:
				ds_status.mpu_min_freq_to_lock = 1000000000;
				break;
		}
		return 0;
	}
	/* 20110331 sookyoung.kim@lge.com LG-DVFS [END_LGE] */
#endif

	if (!dev) {
		WARN(1, "OMAP PM: %s: invalid parameter(s)", __func__);
		return -EINVAL;
	}

	mutex_lock(&mpu_tput_mutex);

	if (!mpu_dev) {
		pr_err("Unable to get MPU device pointer");
		ret = -EINVAL;
		goto unlock;
	}

	/* Save the current constraint */
	old_max_level = mpu_tput->max_level;

	if (f == -1)
		remove_req_tput(dev, mpu_tput);
	else
		add_req_tput(dev, f, mpu_tput);

	/* Find max constraint after the operation */
	max_lookup(mpu_tput);

	/* Rescale the frequency if a change is detected with
	 * the new constraint.
	 */
	if (mpu_tput->max_level != old_max_level) {
		ret = omap_device_set_rate(&dummy_mpu_dev,
					mpu_dev, mpu_tput->max_level);
		if (ret)
			pr_err("Unable to set MPU frequency to %ld\n",
				mpu_tput->max_level);
	}


unlock:
	mutex_unlock(&mpu_tput_mutex);
	return ret;
}
EXPORT_SYMBOL(omap_pm_set_min_mpu_freq);

/*
 *
 * (C) COPYRIGHT 2010-2015 ARM Limited. All rights reserved.
 *
 * This program is free software and is provided to you under the terms of the
 * GNU General Public License version 2 as published by the Free Software
 * Foundation, and any use by you of this program is subject to the terms
 * of such GNU licence.
 *
 * A copy of the licence is included with the program, and can also be obtained
 * from Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 *
 */

/*
 * A core availability policy implementing demand core rotation
 */

#ifndef MALI_KBASE_PM_CA_DEMAND_H
#define MALI_KBASE_PM_CA_DEMAND_H
#include <linux/workqueue.h>
/**
 * struct kbasep_pm_ca_policy_demand - Private structure for demand ca policy
 *
 * This contains data that is private to the demand core availability
 * policy.
 *
 * @cores_desired: Cores that the policy wants to be available
 * @cores_enabled: Cores that the policy is currently returning as available
 * @cores_used: Cores currently powered or transitioning
 * @core_change_timer: Timer for changing desired core mask
 */
#define MALI_CA_TIMER 1
struct kbasep_pm_ca_policy_demand {
	u64 cores_desired;
	u64 cores_enabled;
	u64 cores_used;
#ifdef MALI_CA_TIMER
	struct timer_list core_change_timer;
#else
	struct work_struct wq_work;
#endif
};

extern const struct kbase_pm_ca_policy kbase_pm_ca_demand_policy_ops;
extern int mali_perf_set_num_pp_cores(int cores);

#endif /* MALI_KBASE_PM_CA_DEMAND_H */


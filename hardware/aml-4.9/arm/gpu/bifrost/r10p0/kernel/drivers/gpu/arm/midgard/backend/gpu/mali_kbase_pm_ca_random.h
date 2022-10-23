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
 * A core availability policy implementing random core rotation
 */

#ifndef MALI_KBASE_PM_CA_RANDOM_H
#define MALI_KBASE_PM_CA_RANDOM_H

/**
 * struct kbasep_pm_ca_policy_random - Private structure for random ca policy
 *
 * This contains data that is private to the random core availability
 * policy.
 *
 * @cores_desired: Cores that the policy wants to be available
 * @cores_enabled: Cores that the policy is currently returning as available
 * @cores_used: Cores currently powered or transitioning
 * @core_change_timer: Timer for changing desired core mask
 */
struct kbasep_pm_ca_policy_random {
	u64 cores_desired;
	u64 cores_enabled;
	u64 cores_used;
	struct timer_list core_change_timer;
};

extern const struct kbase_pm_ca_policy kbase_pm_ca_random_policy_ops;

#endif /* MALI_KBASE_PM_CA_RANDOM_H */


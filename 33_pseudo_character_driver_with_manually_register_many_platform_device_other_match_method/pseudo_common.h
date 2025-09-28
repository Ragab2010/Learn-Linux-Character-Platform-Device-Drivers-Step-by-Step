#ifndef _PSEUDO_COMMON_H
#define _PSEUDO_COMMON_H

/* 
 * Each pseudo device will carry some custom configuration data
 * in "platform_data". This is typically filled in by board code
 * or device tree in real hardware scenarios.
 */
struct pseudo_platform_data {
	int some_value;     // arbitrary configuration parameter
	char label[20];     // label string to identify device
};

#endif

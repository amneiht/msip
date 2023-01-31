/*
 * config.h
 *
 *  Created on: Jan 3, 2023
 *      Author: amneiht
 */

#ifndef CONFIG_H_
#define CONFIG_H_

#include <mlib/config.h>

#ifndef MLIB_MINI_SYSTEM
#define MLIB_MINI_SYSTEM 1
#endif

/**
 * if MLIB_MODULE_TMP_POOL is 1 all module control will alloc new memory pool
 * using it when you system has some feature regsiter and unregister many time
 */
#ifndef MLIB_MODULE_TMP_POOL
#define MLIB_MODULE_TMP_POOL 1
#endif

#endif /* CONFIG_H_ */

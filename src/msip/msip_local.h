/*
 * msip_local.h
 *
 *  Created on: Jan 17, 2023
 *      Author: amneiht
 */

#ifndef MSIP_MSIP_LOCAL_H_
#define MSIP_MSIP_LOCAL_H_

#include <pjsip.h>
#include <mlib/module.h>

struct msip_object {
	mlib_module_t *mod;
	pjsip_endpoint *endpoint;

};

#endif /* MSIP_MSIP_LOCAL_H_ */

/*
 * msip.h
 *
 *  Created on: Jan 17, 2023
 *      Author: amneiht
 */

#ifndef MSIP_BASE_H_
#define MSIP_BASE_H_

#include <mlib/module.h>
#include <mlib/context.h>
#include <pjsip.h>

pj_status_t msip_init(mlib_context_l *ctl);
void msip_close();

mlib_module_t* msip_mod();

pjsip_endpoint* msip_endpt();

#endif /* MSIP_BASE_H_ */

/*
 * inv.h
 *
 *  Created on: Jan 30, 2023
 *      Author: amneiht
 */

#ifndef MSIP_INV_H_
#define MSIP_INV_H_

#include <mlib/module.h>
typedef struct msip_media_t msip_media_t;
typedef struct msip_call msip_call;
enum msip_media_priority {
	// highest priority level
	msip_priority_high = 0,
	// normal level
	msip_priority_normal = 4,
	// low level
	msip_priority_low = 16,
};

typedef struct msip_media_obj {
	pj_str_t module_name;
	int pri;
	pj_bool_t (*on_start_call)(msip_call *call, void *mod_data);
	pj_bool_t (*on_call_incoming)(msip_call *call,
			pjmedia_sdp_session *remote_sdp, void *user_data);
	pj_bool_t (*match_media)(const pj_str_t *media);
	pj_bool_t (*match_sdp)(const pjmedia_sdp_session *sdp);
	void *data;
} msip_media_obj;

msip_media_t* msip_media_register(mlib_module_t *mod, const msip_media_obj *obj);
void msip_media_unregier(msip_media_t *ms_media);

#endif /* MSIP_INV_H_ */

/*
 * ua.h
 *
 *  Created on: Jan 27, 2023
 *      Author: amneiht
 */

#ifndef MSIP_UA_H_
#define MSIP_UA_H_
#include <mlib_util/ui.h>
#include <pj/types.h>
#include <pjsip/sip_uri.h>

typedef enum msip_ua_state {
	msip_state_null, //
	msip_state_login, //
	msip_state_disconnect, //
	msip_state_wait, // trang thai cho khi login
	msip_state_error
} msip_ua_state;

typedef struct msip_ua msip_ua;

//create ua
msip_ua* msip_ua_create(const pj_str_t *user, const pj_str_t *pass,
		const pj_str_t *host, const int port, pj_str_t *transport);

pj_bool_t msip_ua_destroy(msip_ua *ua);
// config for ua;

void msip_ua_set_logintime(msip_ua *ua, int time);
//find user_agent
msip_ua* msip_ua_find_by_uri(pjsip_uri *uri);
msip_ua* msip_ua_find_by_name(pj_str_t *name);
msip_ua* msip_ua_find_by_msg(pjsip_msg *msg);

// uri formart <sip:"$user"@"$serverhost":"@port">
pj_bool_t msip_ua_print_uri(msip_ua *ua, pj_str_t *uri);

/**
 * print callee uri for invite
 * @param ua
 * @param callee who recieve a call : example "boo" or "boo@pika.com:2022"
 * @param buff
 * @param blen
 * @return
 */
pj_bool_t msip_ua_print_call_uri(msip_ua *ua, const pj_str_t *callee,
		pj_str_t *out);
pj_bool_t msip_ua_print_contact(msip_ua *ua, pj_str_t *contact);

// print avaiable ua
void msip_ua_print();
void msip_ua_print2(mlib_ui_output_p out, void *ui_data);

// register
pj_bool_t msip_ua_register(msip_ua *ua);
pj_bool_t msip_ua_unregister(msip_ua *ua);

const pj_str_t* msip_ua_get_transport(msip_ua *ua);

#endif /* MSIP_UA_H_ */


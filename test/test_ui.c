/*
 ============================================================================
 Name        : msip.c
 Author      : amneiht
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */
#include <mlib/mlib.h>
#include <mlib/event.h>
#include <mlib_util/util.h>
#include <msip/msip.h>
#include <mlib_util/ui.h>

void test_ua() {
	pj_str_t name = pj_str("500");
	pj_str_t pass = pj_str("500");
	int port = 34001;
	pj_str_t host = pj_str("192.168.100.34");
	pj_str_t transport = pj_str("udp");
	msip_ua *uas = msip_ua_create(&name, &pass, &host, port, &transport);
	msip_ua_register(uas);
}
static void ui_f(void *ui_data, const pj_str_t *input) {
	(void) ui_data;
	PJ_LOG(1, (MLIB_FUNC,"data is %.*s",(int)input->slen , input->ptr));
}
static void ui_p(void *ui_data, const pj_str_t *input) {
	mlib_util_event_send(MLIB_CLOSE);
}
static mlib_command mctl[] = { { "ui", "test", ui_f, NULL }, //
		{ "ui", "exit", ui_p, NULL } };
extern void console_init_mod();
extern void test_mod_init();
int main(void) {
	mlib_init();
	msip_init();
	pj_str_t sname = pj_str("fff");
	mlib_module_t *mod = mlib_module_simple(&sname);
	mlib_command *p1 = &mctl[0];
	mlib_command_register(mod, p1, 2);
	console_init_mod();
	test_mod_init();
	test_ua();
	mlib_loop();
	mlib_close();

	return 0;
}

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
#include <mlib/context.h>

static void pool_rls(void *arg) {
}
void test_ua() {
#if 0
	pj_str_t name = pj_str("5001");
	pj_str_t pass = pj_str("5002");
	int port = 34001;
	pj_str_t host = pj_str("192.168.100.34");
	pj_str_t transport = pj_str("tcp");
	msip_ua *uas = msip_ua_create(&name, &pass, &host, port, &transport);
	msip_ua_register(uas);
#else
	/* */
	pj_str_t name = pj_str("sipconf1");
	pj_str_t pass = pj_str("123456");
	int port = 5060;
	pj_str_t host = pj_str("sip.linphone.org");
	pj_str_t transport = pj_str("tcp");
	msip_ua *uas = msip_ua_create(&name, &pass, &host, port, &transport);
	msip_ua_register(uas);
	/*    */
#endif
}
static void ui_f(void *ui_data, const pj_str_t *input) {
	(void) ui_data;
	msip_close();
}
static void ui_p(void *ui_data, const pj_str_t *input) {
	mlib_util_event_send(MLIB_CLOSE);
}
static mlib_command mctl[] = { { "ui", "test", ui_f, NULL }, //
		{ "ui", "exit", ui_p, NULL } };

extern void console_init_mod();
extern void test_mod_init();
int main(int argc, char **argv) {
	mlib_init();
	pj_pool_t *cpool = mlib_pool_create("lol", MLIB_POOL_SIZE, MLIB_POOL_SIZE);
	if (argc > 1) {
		char file_name[300];
		sprintf(file_name, "%s/%s", argv[1], "core.conf");
		pj_str_t file = pj_str(file_name);
		mlib_context_l *lctx = mlib_context_list_prase(cpool, &file);
		if (lctx)
			mlib_module_conf(lctx);

		// for sip config file
		sprintf(file_name, "%s/%s", argv[1], "sip.conf");
		file = pj_str(file_name);
		lctx = mlib_context_list_prase(cpool, &file);
		msip_init(lctx);
	} else
		msip_init(NULL);
	pj_str_t sname = pj_str("fff");
	mlib_module_t *mod = mlib_module_simple(&sname);
	mlib_command *p1 = &mctl[0];
	mlib_command_register(mod, p1, 2);
	console_init_mod();
	test_mod_init();
	test_ua();
	mlib_loop();
	mlib_pool_release(cpool);
	mlib_close();
	return 0;
}

#include <msip/msip.h>
#include "msip_local.h"
#include <pjlib.h>
#include <pjsip.h>
#include <mlib/module.h>

static struct msip_object *sip_obj;
static pj_bool_t init = PJ_FALSE;
void msip_init() {
	if (init)
		return;
	mlib_init();
//	pjsip_ pj_str_t
//	name = pj_str("Msip");
//	sip_mod = mlib_module_simple(&name);
}

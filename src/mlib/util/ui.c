/*
 * ui.c
 *
 *  Created on: Jan 29, 2023
 *      Author: amneiht
 */

#include <pjlib-util/types.h>
#include <mlib_util/ui.h>
#include <mlib/mem.h>
#include "../mlib_local.h"

#define _INIT_ if(!init){ init_ui(); if(!init) return ;}
struct ui_ctl {
	pj_list command;
	pj_lock_t *lock;
};

struct ui_cmd;
static struct ui_ctl *ums;
static pj_bool_t init = PJ_FALSE;

#define ui_pool mlib_module_pool(_mlib_mod())

struct mod_commad {
	PJ_DECL_LIST_MEMBER(struct mod_commad )
	;
	pj_str_t name;
	pj_pool_t *tmp;
	pj_list cmlist;
//	mlib_list(struct ui_cmd , cmlist)

}
;

struct ui_cmd {
	PJ_DECL_LIST_MEMBER(struct ui_cmd )
	;
	pj_str_t name;
	mlib_ui_func func;
	struct mod_commad *parrent;
	void *data;
};
static void ui_clear(void *arg) {
	struct ui_ctl *omg = arg;
	mlib_mem_release_list(&omg->command);
	pj_lock_destroy(omg->lock);
	init = PJ_FALSE;
}
static void init_ui() {
	if (_mlib_mod() == NULL)
		return;
	ums = pj_pool_alloc(mlib_module_pool(_mlib_mod()), sizeof(struct ui_ctl));
	pj_list_init(&ums->command);
	pj_lock_create_recursive_mutex(mlib_module_pool(_mlib_mod()), "ui_lock",
			&ums->lock);
	mlib_module_add_callback(_mlib_mod(), ums, ui_clear);
	init = PJ_TRUE;
}
static int find_mod(void *data, const void *note) {
	const char *name = data;
	const struct mod_commad *mcmd = note;
	return pj_strcmp2(&mcmd->name, name);
}
static void clear_cmd(void *arg) {
	pj_list_erase(arg);
	struct mod_commad *mod_cmd = arg;
	mlib_mem_release_list(&mod_cmd->cmlist);
	mlib_pool_release(mod_cmd->tmp);
}
static struct mod_commad* create_ctl(const char *name) {
	pj_pool_t *tmp = mlib_pool_create(name, 512, 512);
	struct mod_commad *mod_cmd = mlib_mem_alloc(tmp, sizeof(struct mod_commad),
			clear_cmd);
	mod_cmd->tmp = tmp;
	pj_strdup2(tmp, &mod_cmd->name, name);
	pj_list_init(&mod_cmd->cmlist);
	pj_list_insert_after(&ums->command, mod_cmd);
	return mod_cmd;
}
static int find_cmd(void *data, const void *note) {
	const char *name = data;
	const struct ui_cmd *mcmd = note;
	return pj_strcmp2(&mcmd->name, name);
}
static void ui_unrreg(void *arg) {
	struct ui_cmd *us = arg;
	pj_list_erase(arg);
	struct mod_commad *mcmd = us->parrent;
	if (pj_list_size(&mcmd->cmlist) == 0)
		mlib_mem_mask_destroy(mcmd);
	PJ_LOG(4, ("ui", "clear data for %.*s",(int)us->name.slen , us->name.ptr));
}
static void add_cmd(mlib_module_t *mod, struct mod_commad *cmd,
		const mlib_command *data) {
	struct ui_cmd *us = pj_list_search(&cmd->cmlist, (void*) data->name,
			find_cmd);
	if (us) {
		PJ_LOG(1, (MLIB_NAME,"commanf %s has exits",data->name));
		return;
	}
	us = mlib_modctl_alloc(mod, sizeof(struct ui_cmd), ui_unrreg);
	us->data = data->data;
	us->func = data->func;
	us->parrent = cmd;
	pj_strdup2(mlib_modctl_pool(us), &us->name, data->name);
	pj_list_insert_after(&cmd->cmlist, us);
}
void mlib_command_register(mlib_module_t *mod, const mlib_command *data,
		int leng) {
	_INIT_
	for (int i = 0; i < leng; i++) {
		if (data[i].mod) {
			struct mod_commad *mcmd = pj_list_search(&ums->command,
					(void*) data[i].mod, find_mod);
			if (!mcmd)
				mcmd = create_ctl(data[i].mod);
			add_cmd(mod, mcmd, data + i);
		}
	}
}
void mlib_command_unregister(const mlib_command *data, int leng) {
	_INIT_
	for (int i = 0; i < leng; i++) {
		if (data[i].mod) {
			struct mod_commad *mcmd = pj_list_search(&ums->command,
					(void*) data[i].mod, find_mod);
			if (mcmd) {
				struct ui_cmd *ucm = pj_list_search(&mcmd->cmlist,
						(void*) data[i].name, find_cmd);
				if (ucm)
					mlib_mem_mask_destroy(ucm);
			}
		}
	}
}
void mlib_ui_input(const char *input) {
	_INIT_
	int d = strlen(input);
	int sd;
	char *str = alloca(d + 1);
	str[d] = 0;
	pj_memcpy(str, input, d);
	pj_str_t tmp = { str, d };
	pj_strtrim(&tmp);
	if (tmp.slen == 0)
		return;
	str = tmp.ptr;
	str[tmp.slen] = '\0';
	char *mod = strtok(str, " ");
	sd = strlen(mod) + 1;
	pj_lock_acquire(ums->lock);

	struct mod_commad *mcmd = pj_list_search(&ums->command, mod, find_mod);
	if (mcmd) {
		mod = strtok(NULL, " ");
		sd = sd + strlen(mod) + 1;
		struct ui_cmd *ucm = pj_list_search(&mcmd->cmlist, mod, find_cmd);
		if (ucm) {
			pj_str_t ptr = pj_str(str + sd);
			pj_strtrim(&ptr);
			ucm->func(ucm->data, &ptr);
		}
	}
	pj_lock_release(ums->lock);
}


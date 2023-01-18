/*
 * mlib_context.c
 *
 *  Created on: Jan 4, 2023
 *      Author: amneiht
 */

#include <mlib/context.h>
#include "local.h"
#include <pjlib-util.h>

struct mlib_context_type {
	PJ_DECL_LIST_MEMBER(mlib_context_t)
	;
	pj_rbtree_node *parent;
	pj_bool_t is_pattern;
	mlib_context_t *pattern;
	mlib_context_l *conf; // refer to config
	pj_list child_list;
};
struct mlib_context_list {
	pj_pool_t *pool;
	pj_rbtree *tree;
	pj_str_t path;
	pj_list list;

};
static pj_cis_t *cis_value;
static pj_cis_t *cis_cmt;
static pj_cis_t *cis_context;
static char *conf_path;

const char* mlib_conf_get_path() {
	return conf_path;
}
void mlib_conf_set_path(const char *path) {
	sprintf(conf_path, "%s", path);
}

MLIB_LOCAL void _mlib_context_init(pj_pool_t *pool) {
	conf_path = pj_pool_alloc(pool, 500);
#if defined ( __linux__)
	sprintf(conf_path, "%s", "/tmp/core");
#elif defined (__WIN32__)
	sprintf(conf_path, "%s", "C:\\ProgramData\\core");
#endif

	char *command_str = ";#\n";
	{
		pj_cis_buf_t *cs_cmt = pj_pool_alloc(pool, sizeof(pj_cis_buf_t));
		cis_cmt = pj_pool_alloc(pool, sizeof(pj_cis_t));
		pj_cis_buf_init(cs_cmt);
		pj_cis_init(cs_cmt, cis_cmt);
		pj_cis_invert(cis_cmt);
		pj_cis_del_str(cis_cmt, command_str);
	}
	// value str
	{
		pj_cis_buf_t *cs_cmt = pj_pool_alloc(pool, sizeof(pj_cis_buf_t));
		cis_value = pj_pool_alloc(pool, sizeof(pj_cis_t));
		pj_cis_buf_init(cs_cmt);
		pj_cis_init(cs_cmt, cis_value);
		pj_cis_invert(cis_value);
		pj_cis_del_range(cis_value, 0, ' ');
		pj_cis_del_str(cis_value, command_str);
		// context parten
	}
//cis_context
	{
		pj_cis_buf_t *cs_context = pj_pool_alloc(pool, sizeof(pj_cis_buf_t));
		cis_context = pj_pool_alloc(pool, sizeof(pj_cis_t));
		pj_cis_buf_init(cs_context);
		pj_cis_init(cs_context, cis_context);
		pj_cis_invert(cis_context);
		pj_cis_del_range(cis_context, 0, ' ' + 1);
		pj_cis_del_str(cis_context, command_str);
		pj_cis_del_str(cis_context, "[]()");
		pj_cis_del_str(cis_context, "=");
	}
}
static int rbtree_key_cmp(const void *key1, const void *key2) {
	const pj_str_t *str1 = key1;
	const pj_str_t *str2 = key2;
	return pj_strcmp(str1, str2);
}

static void sc_callback(pj_scanner *scanner) {
	(void) scanner;
	if (!pj_cis_match(cis_cmt, scanner->curptr[0])) {
		// skip a line
		pj_scan_skip_line(scanner);
	}
}
static mlib_context_t* create_context(mlib_context_l *conf,
		const pj_str_t *ctx_name) {
	pj_rbtree_node *node = pj_pool_alloc(conf->pool, sizeof(pj_rbtree_node));
	pj_str_t *name = pj_pool_alloc(conf->pool, sizeof(pj_str_t));
	pj_strdup(conf->pool, name, ctx_name);
	node->key = name;
	mlib_context_t *ele = PJ_POOL_ALLOC_T(conf->pool, mlib_context_t);
	pj_list_init(&ele->child_list);
	ele->parent = node;
	ele->is_pattern = PJ_FALSE;
	ele->pattern = NULL;
//insert node
	node->user_data = ele;
	pj_rbtree_insert(conf->tree, node);
//insert to list ;
	pj_list_insert_after(&conf->list, ele);
	ele->conf = conf;
	return ele;
}
static mlib_context_t* add_context(mlib_context_l *conf, pj_scanner *sc) {
	pj_str_t *out = mlib_alloca(pj_str_t);
	mlib_context_t *ele;
	out->slen = 0;
	int line = sc->line;
	pj_scan_get_quotes(sc, "[", "]", 1, out);
	if (out->slen == 0)
		return NULL;
	out->ptr = out->ptr + 1;
	out->slen = out->slen - 2;
	out = pj_strtrim(out);
// check if node exits
	pj_rbtree_node *node = pj_rbtree_find(conf->tree, out);
	if (node == NULL) {
		// create new context
		ele = create_context(conf, out);
		if (sc->curptr[0] == '(') {
			pj_scan_get_quotes(sc, "(", ")", 1, out);
			out->ptr = out->ptr + 1;
			out->slen = out->slen - 2;
			out = pj_strtrim(out);
			if (out->slen > 0) {
				if (pj_strcmp2(out, "!") == 0) {
					ele->is_pattern = PJ_TRUE;
				} else {
					node = pj_rbtree_find(conf->tree, out);
					if (node == NULL) {
						PJ_LOG(1,
								("Config", "Pattern %.*s must declare first", (int) out->slen, out->ptr));
						goto AOUT;
					}
					mlib_context_t *ele2 = node->user_data;
					if (!ele2->is_pattern) {
						PJ_LOG(1,
								("Config", "Pattern %.*s if not declare with (!)", (int) out->slen, out->ptr));
					} else {
						ele->pattern = ele2;
					}
				}

			}
		}

	} else {
		ele = node->user_data;
	}

	AOUT: if (sc->line == line)
		pj_scan_skip_line(sc);
	return ele;
}
static int find_cmp(void *value, const pj_list_type *node) {
	const pj_str_t *name = value;
	const pj_json_elem *ele = node;
	return pj_strcmp(name, &ele->name);
}
static pj_json_elem* find_ele(const pj_str_t *name, void *list) {

	return (pj_json_elem*) pj_list_search(list, (pj_str_t*) name, find_cmp);
}
static void add_ele(pj_pool_t *pool, mlib_context_t *ctx, pj_str_t *name,
		pj_str_t *value) {
	pj_str_t v;
	pj_json_elem *find = find_ele(name, &ctx->child_list);
	pj_strdup_with_null(pool, &v, value);
	if (find == NULL) {
		find = PJ_POOL_ZALLOC_T(pool, pj_json_elem);
		pj_str_t sl;
		pj_strdup_with_null(pool, &sl, name);
		pj_json_elem_string(find, &sl, &v);
		pj_list_insert_before(ctx->child_list.next, find);
	} else {
		if (find->type != PJ_JSON_VAL_ARRAY) {
			pj_json_elem *f2 = PJ_POOL_ZALLOC_T(pool, pj_json_elem);
			pj_json_elem_string(f2, &find->name, &find->value.str);
			pj_json_elem_array(find, &find->name);
			pj_json_elem_add(find, f2);
		}
		pj_json_elem *ele = PJ_POOL_ZALLOC_T(pool, pj_json_elem);
		pj_json_elem_string(ele, &find->name, &v);
		pj_json_elem_add(find, ele);
	}
}
static pj_status_t parrent_dir(const char *file, pj_str_t *path) {
	if (file == NULL)
		return -1;
	char sp = MLIB_PATH_SP;
	int i;
	int size = strlen(file);
	for (i = size - 1; i > -1; i--) {
		if (file[i] == sp)
			break;
	}
	if (i < 0) {
		path->slen = sprintf(path->ptr, ".");
		return 0;
	}
	i--;
	path->slen = sprintf(path->ptr, "%.*s", i, file);
	return 0;
}
mlib_context_l* mlib_context_list_prase(pj_pool_t *pool, const pj_str_t *file) {
	FILE *fp = fopen(file->ptr, "r");
	if (!fp) {
		PJ_LOG(3, (MLIB_NAME, "no config file"));
	}
	fseek(fp, 0, SEEK_END);
	long sl = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	char buff[sl + 1];
	buff[sl] = 0;
	fread(buff, sizeof(char), sl, fp);

	mlib_context_l *conf = mlib_context_list_prase2(pool, buff, sl);
	conf->path.ptr = pj_pool_alloc(conf->pool, 500);
	parrent_dir(file->ptr, &conf->path);
	fclose(fp);
	return conf;
}
mlib_context_l* mlib_context_list_prase2(pj_pool_t *pool, char *buffer,
		long len) {

	mlib_context_l *conf = pj_pool_alloc(pool, sizeof(mlib_context_l));

	conf->pool = pool;
	conf->tree = PJ_POOL_ZALLOC_T(conf->pool, pj_rbtree);
	pj_rbtree_init(conf->tree, rbtree_key_cmp);
	pj_list_init(&conf->list);

	pj_scan_state *state = mlib_alloca(pj_scan_state);
	pj_scanner *scanner = alloca(sizeof(pj_scanner));
	pj_scan_init(scanner, buffer, len,
			PJ_SCAN_AUTOSKIP_WS | PJ_SCAN_AUTOSKIP_WS_HEADER
					| PJ_SCAN_AUTOSKIP_NEWLINE, sc_callback);

	pj_str_t *name = alloca(sizeof(pj_str_t));
	pj_str_t *value = alloca(sizeof(pj_str_t));

	pj_str_t ctx_name = pj_str("core");
	mlib_context_t *current = create_context(conf, &ctx_name);
	while (!pj_scan_is_eof(scanner)) {
		name->slen = 0;
		value->slen = 0;
		pj_scan_get(scanner, cis_context, name);
		if (name->slen == 0) {
			pj_scan_skip_whitespace(scanner);
			if (scanner->curptr[0] != '[') {
				continue;
			}
			current = add_context(conf, scanner);
			continue;
		}
		name = pj_strtrim(name);
		pj_scan_save_state(scanner, state);
		pj_scan_get(scanner, cis_value, value);
		if (value->slen == 0)
			continue;
		// remove "="
		if (value->ptr[0] == '=') {
			// reset str
			pj_scan_restore_state(scanner, state);
			scanner->curptr = value->ptr + 1;
			pj_scan_skip_whitespace(scanner);
			value->slen = 0;
			pj_scan_get(scanner, cis_value, value);
			if (value->slen == 0)
				continue;
		}
		value = pj_strtrim(value);
		add_ele(conf->pool, current, name, value);
	}
	pj_scan_fini(scanner);
	return conf;
}

static void node_print(void *node, void *user_data) {
	(void) user_data;
	pj_rbtree_node *rbnode = node;
	pj_json_elem *child;
	pj_str_t *val;
	mlib_context_t *con = rbnode->user_data;
	const pj_str_t *name = rbnode->key;
	if (con->is_pattern) {
		PJ_LOG(1, ("", "[%.*s](!)", (int) name->slen, name->ptr));

	} else if (con->pattern != NULL) {
		const pj_str_t *pat = con->pattern->parent->key;
		PJ_LOG(1,
				("", "[%.*s](%.*s)", (int) name->slen, name->ptr, (int) pat->slen, pat->ptr));
	} else {
		PJ_LOG(1, ("", "[%.*s]", (int) name->slen, name->ptr));
	}
// log child
	pj_json_elem *ele = con->child_list.next;
	while (ele != (pj_json_elem*) &con->child_list) {
		name = &ele->name;
		if (ele->type == PJ_JSON_VAL_ARRAY) {
			child = ele->value.children.next;
			while (child != (pj_json_elem*) &ele->value.children) {
				val = &child->value.str;
				PJ_LOG(1,
						("", "%-32.*s=\t%.*s", (int) name->slen, name->ptr, (int) val->slen, val->ptr));
				child = child->next;
			}
		} else {
			val = &ele->value.str;
			PJ_LOG(1,
					("", "%-32.*s=\t%.*s", (int) name->slen, name->ptr, (int) val->slen, val->ptr));
		}
		ele = ele->next;
	}
	PJ_LOG(1, ("", "-----------"));
}
void mlib_conf_print(const mlib_context_l *conf) {
//	_mlib_tree_view(conf->tree, node_print, NULL);
}
mlib_context_t* mlib_context_list_find(const mlib_context_l *list,
		const pj_str_t *name) {

	pj_rbtree_node *node = pj_rbtree_find(list->tree, (void*) name);
	if (node == NULL)
		return NULL;
	return (mlib_context_t*) node->user_data;
}

const mlib_context_t* mlib_context_type_next(const mlib_context_t *context) {
	const mlib_context_t *last = context->conf->list.prev;
	if (context->next == last)
		return NULL;
	return context->next;
}
const pj_str_t* mlib_context_type_get_name(const mlib_context_t *context) {
	return (const pj_str_t*) context->parent->key;
}

mlib_context_val* mlib_context_type_get_value(const mlib_context_t *ctx,
		const pj_str_t *val_name) {
	pj_json_elem *ele = find_ele(val_name, (void*) &ctx->child_list);
	if (ele == NULL && ctx->pattern)
		ele = mlib_context_type_get_value(ctx->pattern, val_name);
	return ele;
}
const pj_str_t* mlib_context_val_to_str(const mlib_context_val *val) {
	const pj_json_elem *ele = val;
	if (ele->type == PJ_JSON_VAL_STRING)
		return &(ele->value.str);
	else
		return &(ele->value.children.next->value.str);
}

int mlib_context_val_to_int(const mlib_context_val *val) {
	const pj_str_t *sname = mlib_context_val_to_str(val);
	return atoi(sname->ptr);
}
double mlib_context_val_to_double(const mlib_context_val *val) {
	const pj_str_t *sname = mlib_context_val_to_str(val);
	return strtod(sname->ptr, NULL);
}

pj_list* mlib_context_val_to_list(pj_pool_t *pool, const mlib_context_val *val) {
	pj_list *res = pj_pool_alloc(pool, sizeof(pj_list));
	const pj_json_elem *ele = val;
	pj_list_init(res);
	if (ele->type == PJ_JSON_VAL_STRING) {
		struct mlib_context_str_list *ml = PJ_POOL_ALLOC_T(pool,
				struct mlib_context_str_list);
		pj_strdup(pool, &ml->value, &ele->value.str);
		pj_list_insert_after(res, ml);
	} else {
		const pj_json_elem *last = (const pj_json_elem*) &ele->value.children;
		const pj_json_elem *p = (const pj_json_elem*) &ele->value.children.next;
		while (p != last) {
			struct mlib_context_str_list *ml = PJ_POOL_ALLOC_T(pool,
					struct mlib_context_str_list);
			pj_strdup(pool, &ml->value, &p->value.str);
			pj_list_insert_after(res, ml);
			p = p->next;
		}
	}
	return res;
}
struct dlist {
	void (*handle)(void *data, const pj_str_t *value);
	void *data;
};

static int find_value(void *value, const pj_list_type *node) {
	const pj_str_t *name = value;
	const pj_json_elem *ele = node;
	if (pj_strcmp(name, &ele->value.str) == 0)
		return 0;
	return 1;
}
pj_bool_t mlib_context_val_check(const mlib_context_val *val,
		const pj_str_t *value) {
	const pj_json_elem *ele = val;
	if (ele == NULL)
		return PJ_FALSE;
	if (ele->type == PJ_JSON_VAL_STRING) {
		return pj_strcmp(&ele->value.str, value) == 0;
	} else {
		void *d = pj_list_search((void*) &ele->value.children, (void*) value,
				find_value);
		return d != NULL;
	}
}
void mlib_context_print(const mlib_context_t *context) {
	node_print(context->parent, NULL);
}

mlib_context_t* mlib_context_type_clone(pj_pool_t *pool,
		const mlib_context_t *p) {

	mlib_context_t *clone = pj_pool_alloc(pool, sizeof(mlib_context_t));
	clone->parent = PJ_POOL_ALLOC_T(pool, pj_rbtree_node);
	// copy name
	pj_str_t *key = PJ_POOL_ALLOC_T(pool, pj_str_t);
	pj_strdup(pool, key, p->parent->key);
	clone->parent->key = key;
	clone->parent->user_data = clone;

	// copy list
	pj_list_init(&clone->child_list);
	const pj_json_elem *note, *last;
	last = (const pj_json_elem*) &p->child_list;
	note = last->next;
	while (last != note) {
		pj_json_elem *ele = _mlib_json_clone(pool, note);
		pj_list_insert_after(&clone->child_list, ele);
		note = note->next;
	}
	if (p->pattern) {
		clone->pattern = mlib_context_type_clone(pool, p->pattern);
	}
	clone->conf = NULL;
	return clone;

}
/** not now */
/* list clone
 mlib_context_l* mlib_context_list_clone(pj_pool_t *pool,
 const mlib_context_l *ctx) {
 mlib_context_l *clone = pj_pool_alloc(pool, sizeof(mlib_context_l));
 clone->tree = PJ_POOL_ALLOC_T(pool, pj_rbtree);
 pj_rbtree_init(clone->tree, rbtree_key_cmp);
 pj_list_init(&clone->list);
 if (ctx->path.slen > 0)
 pj_strdup(pool, &clone->path, ctx->path);
 else
 clone->path.slen = 0;

 clone->pool = pool;
 const mlib_context_t *p, *last;
 last = &ctx->list;
 p = last->next;
 while (p != last) {
 mlib_context_t *cus = ctx_clone(pool, p);
 p = p->next;
 }
 return clone;
 }
 end of clone */

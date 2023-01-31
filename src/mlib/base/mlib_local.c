/*
 * mlib_local.c
 *
 *  Created on: Jan 5, 2023
 *      Author: amneiht
 */

#include <pjlib-util/json.h>
#include <pj/pool.h>
#include <pj/string.h>
#include <mlib/mem.h>
#include <mlib_local.h>
void mlib_modctl_list_destroy(pj_list *list) {
	pj_list *p, *end;
	end = list;
	p = list->next;
	while (p != end) {
		p = p->next;
		mlib_mem_mask_destroy(p->prev);
	}
}
MLIB_LOCAL pj_json_elem* _mlib_json_clone(pj_pool_t *pool,
		const pj_json_elem *sl) {
	pj_json_elem *clone = PJ_POOL_ZALLOC_T(pool, pj_json_elem);
	clone->type = sl->type;
	switch (sl->type) {

	case PJ_JSON_VAL_NULL:
		break;
	case PJ_JSON_VAL_BOOL:
		clone->value.is_true = sl->value.is_true;
		break;
	case PJ_JSON_VAL_NUMBER:
		clone->value.num = sl->value.num;
		break;
	case PJ_JSON_VAL_STRING:
		pj_strdup(pool, &clone->value.str, &sl->value.str);
		break;
	default:
		pj_list_init(&clone->value.children);
		const pj_json_elem *s, *l;
		l = (const pj_json_elem*) &sl->value.children;
		s = l->next;
		while (s != l) {
			pj_json_elem *lm = _mlib_json_clone(pool, s);
			pj_list_insert_after(&clone->value.children, lm);
			s = s->next;
		}
		break;
	}
	pj_strdup(pool, &clone->name, &sl->name);
	return clone;
}
enum tree_state {
	move_left, move_right, move_up, move_end
};
MLIB_LOCAL void _mlib_tree_view(pj_rbtree *tree, tree_handle handle, void *data) {
	pj_rbtree_node *node = tree->root;
	pj_rbtree_node *old = NULL;
	pj_rbtree_node *root = node;
	pj_rbtree_node *null_node = tree->null;
	enum tree_state state = move_left;
	// stat serach
	const pj_str_t *key;
	while (state != move_end && node != null_node) {
		key = node->key;
		switch (state) {
		case move_left:
			if (node->right == null_node && node->left == null_node) {
				// leaf node
				handle(node, data);
				old = node;
				node = node->parent;
				state = move_up;
			} else if (node->left != null_node) {
				state = move_left;
				node = node->left;
			} else {
				state = move_right;
				node = node->right;
			}
			break;
		case move_right:
			if (node->right == null_node && node->left == null_node) {
				// leaf node
				handle(node, data);
				old = node;
				node = node->parent;
				state = move_up;
			} else if (node->left != null_node) {
				state = move_left;
				node = node->left;
			} else {
				state = move_right;
				node = node->right;
			}
			break;
		case move_up:
			if (old != node->right && node->right != null_node) {
				state = move_right;
				node = node->right;
			} else {
				handle(node, data);
				old = node;
				node = node->parent;
				state = move_up;
			}
			break;
		default:
			break;
		}
		if (node == root && state == move_up) {
			if (node->right == node->left)
				state = move_end;
			else if (old == node->left && node->right == null_node)
				state = move_end;
			else if (old == node->right)
				state = move_end;
			if (state == move_end) {
				handle(node, data);
			}
		}
	}
	(void) key;
}


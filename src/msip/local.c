/*
 * local.c
 *
 *  Created on: Jan 31, 2023
 *      Author: amneiht
 */
#include "msip_local.h"

MLIB_LOCAL void msip_list_add(pj_list *a, pj_list_type *b, element_cmp ele_cmp) {
	pj_list *list1 = a;
	pj_list *start = list1->next;
	while (start != list1) {
		if (ele_cmp(start, b) <= 0) {
			pj_list_insert_before(start, b);
			return;
		}
		start = start->next;
	}
	pj_list_insert_after(a, b);
	(void) a;
}

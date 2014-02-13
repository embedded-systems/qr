#include "list.h"
#include <string.h>
#include <stdlib.h>

/* create a new list */
List* LST_Create() {
	List* list;
	/* allocate memory */
	list = (List*)malloc(sizeof(List));
	if (list) {
		/* initialize */
		list->firstitem = 0;
		list->count = 0;
		list->lastitemptr = &list->firstitem;
	}
	return list;
}

/* clear the contents of a list, when the second argument
		is TRUE, all the items are also destroyed */
void LST_Clear(List* list, BOOL destroy_contents) {
	ListItem *a,*b;
	a = list->firstitem;
	/* destroy all items */
	while(a) {
		b = a->next;
		if (destroy_contents) free(a->data);
		free(a);
		a = b;
	}
	list->firstitem = 0;
}

/* add item to list */
BOOL LST_Add(List* list, void* data) {
	ListItem* item;
	/* allocate memory */
	item = (ListItem*)malloc(sizeof(ListItem));
	if (item) {
		/* set properties */
		item->next = 0;
		item->data = data;
		/* update pointers */
		*list->lastitemptr = item;
		list->lastitemptr = &item->next;
		/* update counter */
		list->count++;
		return TRUE;
	} else {
		return FALSE;
	}
}

/* destroy the list */
void LST_Destroy(List* list) {
	LST_Clear(list, FALSE);
	free(list);
}

/* return nr of items in list */
int LST_GetCount(List* list) {
	return list->count;
}

/* start an enumeration over the list */
ListEnumeration* LST_StartEnumeration(List* list) {
	ListEnumeration* lenum;
	/* allocate */
	lenum = (ListEnumeration*)malloc(sizeof(ListEnumeration));
	if (lenum) {
		/* initialize */
		lenum->list = list;
		lenum->current = 0;
	}
	return lenum;
}

/* get the next item from an enumeration */
void* LST_GetNextItem(ListEnumeration* lenum) {
	void* data;
	
	if (!lenum->current) {
		/* first item */
		lenum->current = lenum->list->firstitem;
		return lenum->current->data; 
	} else if (lenum->current->next) {
		/* next item */
		lenum->current = lenum->current->next;
		return lenum->current->data; 
	} else {
		/* done */
		return 0;
	}
}

/* stop enumeration */
void LST_StopEnumeration(ListEnumeration* lenum) {
	/* cleanup */
	free(lenum);
}

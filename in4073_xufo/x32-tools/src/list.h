/*
 * List: contains functions to create and manipulate a simple linked list.
 *
 *	Use LST_Create() to create a new list and LST_Add to add items
 *	Finally use LST_Destroy to destroy the list. All items can 
 *	be destroyed with the LST_Clear function.
 *	
 *	Enumerating the list can be done by starting an enumeration using
 *	LST_StartEnumeration, and getting each item by calling LST_GetNextItem.
 *	The enumeration should be stopped/cleaned by calling LST_StopEnumeration
 *
 *	The list should only be modified by using the LST_* functions
 *
 *	Author: Sijmen Woutersen
 *
 */
#ifndef LIST_H
#define LIST_H

#include "bool.h"

/* item container */
typedef struct listitem {
	void* data;							/* item data */
	struct listitem* next;	/* pointer to next item */
} ListItem;

/* main list structure */
typedef struct list {
	ListItem* firstitem;						/* pointer to first item */
	int count;											/* nr of items in list */
	struct listitem** lastitemptr;	/* pointer to last next item pointer */
} List;

/* enumeration state */
typedef struct listenumeration {
	List* list;
	ListItem* current;
} ListEnumeration;

/* create a new list */
List* LST_Create();
/* clear the contents of a list, when the second argument
		is TRUE, all the items are also destroyed */
void LST_Clear(List*, BOOL);
/* add item to list */
BOOL LST_Add(List*, void*);
/* destroy the list */
void LST_Destroy(List*);
/* get number of items in the list */
int LST_GetCount(List* list);

/* start new enumeration over a list */
ListEnumeration* LST_StartEnumeration(List*);
/* get next item from an enumeration */
void* LST_GetNextItem(ListEnumeration*);
/* stop/cleanup an enumeration */
void LST_StopEnumeration(ListEnumeration*);
#endif

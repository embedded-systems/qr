/*
 * Hashtable: contains functions to create and manipulate a hashtable.
 *		The hashtable contains 256 linked list, with a simple xor hash
 *		function. 
 *
 *	Use HT_Create() to create a new table. HT_Add to add items and
 *	HT_Find() to get items from the table. Finally use HT_Destroy to
 *	destroy the hashtable. All items can be destroyed with the HT_Clear
 *	function.
 *	
 *	Enumerating the table can be done by starting an enumeration using
 *	HT_StartEnumeration, and getting each item by calling HT_GetNextItem.
 *	The enumeration should be stopped/cleaned by calling HT_StopEnumeration
 *
 *	Author: Sijmen Woutersen
 *
 */


#ifndef HASHTABLE_H
#define HASHTABLE_H
#include "bool.h"

/* shell for any item in a hashtable */
typedef struct hashtableitem {
	void* data;	/* data of this item */
	char* key;	/* key of this item */
	/* next item in linked list */
	struct hashtableitem* next;
} HashTableItem;

/* hashtable structure */
typedef struct hashtable {
	HashTableItem* items[256];
} HashTable;

/* enumeration state structure */
typedef struct hashenumeration {
	int cur_list_index;				/* current index in hashtable */
	HashTableItem *current;		/* current item in linked list */
	HashTable *table;					/* the hashtable it's enumerating */
} HashTableEnumeration;

/* create a new hashtable */
HashTable* HT_Create();
/* destroy an existing hashtable */
void HT_Destroy(HashTable*);
/* clear the contents of a hashtable, when the second argument
		is TRUE, all the items are also destroyed */
void HT_Clear(HashTable*, BOOL);
/* add an item to a hashtable; the second argument is the item,
		the third the key, returns FALSE when an item with this key
		allready exists, or when the system is out of memory */
BOOL HT_Add(HashTable*, void*, char*);
/* find an item in a hashtable, the second argument is the key
		of the item to find, returns 0 when the item is not found */
void* HT_Find(HashTable*, char*);
/* start an enumeration over the entire hashtable */
HashTableEnumeration* HT_StartEnumeration(HashTable*);
/* returns the next item in an enumeration */
void* HT_GetNextItem(HashTableEnumeration*);
/* stops (closes) an enumeration */
void HT_StopEnumeration(HashTableEnumeration*);
#endif

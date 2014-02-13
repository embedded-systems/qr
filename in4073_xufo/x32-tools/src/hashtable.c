#include "hashtable.h"
#include <string.h>
#include <stdlib.h>

/* private hash function */
unsigned char get_hash(char*);

/* create a new hashtable */
HashTable* HT_Create() {
	HashTable* table;
	int i;
	
	/* create the table */
	table = (HashTable*)malloc(sizeof(HashTable));
	
	/* set all item pointers to 0 */
	for (i = 0; i < 256; i++) table->items[i] = 0;
	
	return table;
}


/* clear the contents of a hashtable, when the second argument
		is TRUE, all the items are also destroyed */
void HT_Clear(HashTable* table, BOOL destroy_contents) {
	int i;
	HashTableItem *a,*b;
	
	/* walk through table */
	for (i = 0; i < 256; i++) {
		a = table->items[i];
		/* walk through linked list */
		while(a) {
			b = a->next;
			/* destroy the item, and maybe also the contents */
			if (destroy_contents) free(a->data);
			free(a->key);
			free(a);
			a = b;
		}
		table->items[i] = 0;
	}
}

/* clear the contents of a hashtable, when the second argument
		is TRUE, all the items are also destroyed */
void HT_Destroy(HashTable* table) {
	/* clear the table */
	HT_Clear(table, FALSE);
	/* destroy the table */
	free(table);
}

/* add an item to a hashtable; the second argument is the item,
		the third the key, returns FALSE when an item with this key
		allready exists, or when the system is out of memory */
BOOL HT_Add(HashTable* table, void* item, char* key) {
	unsigned char hash;
	HashTableItem **ptr;
	
	/* compute the hashcode of the key */
	hash = get_hash(key);
	/* start the linked list enumeration */
	ptr = &table->items[hash];
	/* search the linked list for the key, if it allready
			exists, return false, otherwise */
	while(*ptr) {
		if (strcmp(key, (*ptr)->key) == 0) return FALSE;
		ptr = &((*ptr)->next);
	}
	
	/* ptr points to the next pointer of the last entry in the
			linked list */
	*ptr = (HashTableItem*)malloc(sizeof(HashTableItem));
	if (!*ptr) return FALSE;
	/* save key and data in the new item */
	(*ptr)->key = (char*)malloc(strlen(key)+1);
	if (!(*ptr)->key) {
		/* rollback */
		free(*ptr);
		*ptr = 0;
		return FALSE;
	}
	strcpy((*ptr)->key, key);
	(*ptr)->next = 0;
	(*ptr)->data = item;
	return TRUE;
}

/* find an item in a hashtable, the second argument is the key
		of the item to find, returns 0 when the item is not found */
void* HT_Find(HashTable* table, char* key) {
	HashTableItem* item;
	
	/* get the linked list for this key */
	item = table->items[get_hash(key)];
	
	/* search the list */
	while(item) {
		if (strcmp(item->key, key) == 0) {
			/* found, return */
			return item->data;
		}
		item = item->next;
	}
	/* not found, return 0 */
	return 0;
}

/* the hash function */
unsigned char get_hash(char* key) {
	unsigned char hash;
	hash = 0;
	/* xor all the characters in the key, return result */
	while(*key) {
		hash ^= *key;
		key++;
	}
	return hash;
}

/* start an enumeration over the entire hashtable */
HashTableEnumeration* HT_StartEnumeration(HashTable* table) {
	HashTableEnumeration* henum;
	/* create new enumeration */
	henum = (HashTableEnumeration*)malloc(sizeof(HashTableEnumeration));
	
	if (henum == 0) return 0;
	
	/* initialize all to 0 */
	henum->cur_list_index = 0;
	henum->current = 0;
	henum->table = table;
	return henum;
}

/* returns the next item in an enumeration */
void* HT_GetNextItem(HashTableEnumeration* henum) {
	if (henum->current && henum->current->next) {
		/* if within a linked list, and a next item is available:
				update and return */
		henum->current = henum->current->next;
		return henum->current->data;
	} else {
		/* else, search the table for the next linked list and return the
				first item in the list */
		while(henum->cur_list_index < 256) {
			if (henum->current = henum->table->items[henum->cur_list_index++]) {
				return henum->current->data;
			}
		}
		return 0;
	}
}

/* stops (closes) an enumeration */
void HT_StopEnumeration(HashTableEnumeration* henum) {
	/* destroy structure */
	free(henum);
}

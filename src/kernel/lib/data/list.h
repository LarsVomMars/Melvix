#ifndef MELVIX_LIST_H
#define MELVIX_LIST_H

#include <stdint.h>

typedef struct listnode {
	struct listnode *prev;
	struct listnode *next;
	void *val;
} listnode_t;

typedef struct list {
	listnode_t *head;
	listnode_t *tail;
	uint32_t size;
} list_t;

#define foreach(t, list) for (listnode_t *t = list->head; t != NULL; t = t->next)

list_t *list_create();

uint32_t list_size(list_t *list);

listnode_t *list_insert_front(list_t *list, void *val);

void list_insert_back(list_t *list, void *val);

void *list_remove_node(list_t *list, listnode_t *node);

void *list_remove_front(list_t *list);

void *list_remove_back(list_t *list);

void list_push(list_t *list, void *val);

listnode_t *list_pop(list_t *list);

void list_enqueue(list_t *list, void *val);

listnode_t *list_dequeue(list_t *list);

void *list_peek_front(list_t *list);

void *list_peek_back(list_t *list);

void list_destroy(list_t *list);

void listnode_destroy(listnode_t *node);

int list_contain(list_t *list, void *val);

listnode_t *list_get_node_by_index(list_t *list, int index);

void *list_remove_by_index(list_t *list, int index);

char *list2str(list_t *list, const char *delim);

list_t *str_split(const char *str, const char *delim, uint32_t *numtokens);

#endif
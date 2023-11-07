#ifndef KALLOC_H_
#define KALLOC_H_

#include "config/basic_types.h"

struct node {
    struct node *next;
};

struct mem_list {
    struct node *head;
};

void kalloc_init();
void *kalloc();
void kfree(void *);

#endif

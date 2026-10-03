#ifndef LIST_H
#define LIST_H

#include <stddef.h>

// Circular doubly linked list implementation.
// Intrusive list

struct list_node{
    struct list_node* prev;
    struct list_node* next;
};

static inline void list_init(struct list_node* node){
    node->prev = node;
    node->next = node;
}

static inline int list_empty(const struct list_node *head){
    return head->next == head;
}

static inline void __list_add(struct list_node *node, struct list_node *prev, struct list_node *next){
    // prev, node, next
    next->prev = node;
    prev->next = node;
    node->prev = prev;
    node->next = next;
}

static inline void list_add_front(struct list_node *head, struct list_node *node){
    __list_add(node, head, head->next);
}

static inline void list_add_tail(struct list_node *head, struct list_node *node){
    __list_add(node, head->prev, head);
}

static inline void list_del(struct list_node *node){
    node->prev->next = node->next;
    node->next->prev = node->prev;

    list_init(node); // let node point to itself
}

static inline struct list_node* list_pop_front(struct list_node *head){
    if(list_empty(head)) return NULL;

    struct list_node* node;
    node = head->next;
    list_del(node);

    return node;
}

#define container_of(ptr, type, member) ((type *)((char *)(ptr) - offsetof(type, member)))
// offsetof count the bytes from type to member 
// convert ptr to char* so that add/sub's unit is one byte, then minus the offset
// ptr now points to the type's starting address, so convert to type*

/* 
 *       Example : 
 *
 *       address          struct          
 *       0x1000      +------------------+ <--- [what we want!] (struct process *)
 *                   | int pid;         |      (4 bytes)
 *       0x1004      | char name[32];   |      (32 bytes)
 *       0x1024      +------------------+ <--- offsetof(struct process, list) = 0x24 (36 bytes)
 *                   | struct list_head | <--- ptr (&process.list = 0x1024)
 *                   |   .next, .prev   |
 *       0x1034      +------------------+
 *                   | int priority;    |
 *                   +------------------+
 *       return 0x1024 - 0x24 = 0x1000
 * 
 *       Why are we doing this ?
 *       Cause Linux's Intrusive List : the data is not in the list node.
*/

#define list_entry(ptr, type, member) container_of(ptr, type, member)

#endif

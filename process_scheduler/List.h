/* double linked list */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "headers.h"

struct List {
    struct MNode *head;
    struct MNode *tail;
    struct MNode *tail_pred;
};

struct MNode {
    process data;
    struct MNode *succ;
    struct MNode *pred;
};

typedef struct MNode *NODE;
typedef struct List *LIST;

LIST newList(void);
int isEmpty(LIST);
NODE getTail(LIST);
NODE getHead(LIST);
void addTail(LIST, process);
void addHead(LIST, process);
NODE remHead(LIST);
NODE remTail(LIST);
NODE insertAfter(LIST, NODE, NODE);
NODE removeNode(LIST, NODE);

LIST newList(void) {
    LIST tl = (LIST)malloc(sizeof(struct List));
    if (tl != NULL) {
        tl->tail_pred = (NODE)&tl->head;
        tl->tail = NULL;
        tl->head = (NODE)&tl->tail;
        return tl;
    }
    return NULL;
}

int isEmpty(LIST l) {
    return (l->head->succ == NULL);
}

NODE getHead(LIST l) {
    return l->head;
}

NODE getTail(LIST l) {
    return l->tail;
}

void addTail(LIST l, process n) {
    struct MNode *node = (NODE)malloc(sizeof(struct MNode));
    node->data = n;
    if (l->tail == NULL) {
        l->head = node;
        l->tail = node;
        l->tail->pred = NULL;
        l->tail->succ = NULL;
    } else {
        NODE P = l->tail;
        node->pred = l->tail;
        P->succ = node;
        l->tail = node;
        node->succ = NULL;
    }
}

void addHead(LIST l, process n) {
    struct MNode *node = (NODE)malloc(sizeof(struct MNode));
    node->data = n;
    if (isEmpty(l)) {
        l->head = node;
        l->tail = node;
    } else {
        NODE P = l->head;
        node->succ = P;
        P->pred = node;
        l->head = node;
    }
}

NODE remHead(LIST l) {
    if (isEmpty(l)) return NULL;
    NODE h = l->head;
    l->head = l->head->succ;
    if (l->head) l->head->pred = (NODE)&l->head;
    else l->tail = NULL;
    return h;
}

NODE remTail(LIST l) {
    if (isEmpty(l)) return NULL;
    NODE t = l->tail;
    l->tail = l->tail->pred;
    l->tail->succ = NULL;
    return t;
}

NODE insertAfter(LIST l, NODE r, NODE n) {
    n->pred = r;
    n->succ = r->succ;
    if (n->succ) n->succ->pred = n;
    else l->tail = n;
    r->succ = n;
    return n;
}

NODE removeNode(LIST l, NODE n) {
    if (n->pred == NULL && n->succ == NULL) {
        l->head = NULL;
        l->tail = NULL;
    } else if (n->succ == NULL) {
        l->tail = n->pred;
        l->tail->succ = NULL;
    } else if (n->pred == NULL || n->pred == (NODE)&l->head) {
        l->head = n->succ;
        l->head->pred = (NODE)&l->head;
    } else {
        n->succ->pred = n->pred;
        n->pred->succ = n->succ;
    }
    NODE next = n->succ;
    free(n);
    return next;
}

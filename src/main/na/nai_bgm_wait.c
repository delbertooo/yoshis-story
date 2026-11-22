#include "global.h"
#include "audio.h"

// The BGM layer's waiting list: WAITING handles queue here, in nodes of gNaiBgmWaitNodes headed by gNaiBgmWaitList,
// until a group frees up and Nai_BgmRoutin starts the head. Its own object, as its 16-aligned `.bss` block shows.
// Every name here is INVENTED (no string).

NaiBgmWaitNode gNaiBgmWaitNodes[10];
NaiBgmWaitNode* gNaiBgmWaitList;

// Claims a free node of the pool; NULL once all ten are in use.
NaiBgmWaitNode* Nai_BgmWaitNodeAlloc(void) {
    NaiBgmWaitNode* node;
    u16 i;

    node = NULL;
    for (i = 0; i < 10; i++) {
        if (gNaiBgmWaitNodes[i].inUse == 0) {
            node = &gNaiBgmWaitNodes[i];
            node->inUse = 1;
            break;
        }
    }
    return node;
}

// Unlinks a node and frees it, promoting its successor to head when it was first.
void Nai_BgmWaitNodeFree(NaiBgmWaitNode* node) {
    if (node->prev != NULL) {
        node->prev->next = node->next;
    } else {
        gNaiBgmWaitList = node->next;
    }
    if (node->next != NULL) {
        node->next->prev = node->prev;
    }
    node->inUse = 0;
}

// Pushes a handle onto the FRONT of the list, so the newest request starts first; drops it when the pool is full.
void Nai_BgmWaitAdd(NaiBgm* bgm) {
    NaiBgmWaitNode* node;

    node = Nai_BgmWaitNodeAlloc();
    if (node == NULL) {
        return;
    }
    node->bgm = bgm;
    node->prev = NULL;
    if (gNaiBgmWaitList != NULL) {
        gNaiBgmWaitList->prev = node;
        node->next = gNaiBgmWaitList;
    } else {
        node->next = NULL;
    }
    gNaiBgmWaitList = node;
}

// Takes a handle back off the waiting list, if it is on it.
void Nai_BgmWaitRemove(NaiBgm* bgm) {
    NaiBgmWaitNode* node;

    node = gNaiBgmWaitList;
    while (node != NULL) {
        if (node->bgm == bgm) {
            break;
        }
        node = node->next;
    }
    if (node != NULL) {
        Nai_BgmWaitNodeFree(node);
    }
}

// The handle at the head of the list, the one Nai_BgmRoutin lets start next; NULL when the list is empty.
NaiBgm* Nai_BgmWaitGetHead(void) {
    if (gNaiBgmWaitList == NULL) {
        return NULL;
    }
    return gNaiBgmWaitList->bgm;
}

// Empties the waiting list; the third of Nai_BgmInit's three table clears.
void Nai_BgmWaitClear(void) {
    u16 i;

    gNaiBgmWaitList = NULL;
    for (i = 0; i < 10; i++) {
        gNaiBgmWaitNodes[i].inUse = 0;
    }
}

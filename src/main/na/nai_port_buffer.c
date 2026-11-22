#include "global.h"
#include "audio.h"

// The command pool: gNaiPortBuffer's 0x100 NaiPorts, the live ones listed in post order from gNaiPortList.
// Every function name here but Nai_PortBufferAlloc (from its overflow print) is INVENTED (no string).
// The PRINTFs are stripped debug prints whose strings stay in .rodata.

NaiPort gNaiPortBuffer[0x100];
NaiPort* gNaiPortList;

// Empties the pool: drops the list head and clears every slot's in-use word.
// Load-bearing: the `u16` counter.
void Nai_PortBufferClear(void) {
    u16 i;

    gNaiPortList = NULL;
    for (i = 0; i < 0x100; i++) {
        gNaiPortBuffer[i].inUse = 0;
    }
}

// Steps a ramp one tick. Returns 1 once `time` ticks have elapsed, snapping value to end; until
// then value is start plus the per-tick delta times the elapsed count.
s32 Nai_PortRampStep(NaiPort* port) {
    if (port == NULL) {
        return 1;
    }
    if (port->elapsed == 0) {
        Nai_PortRampCalcDelta(port);
    }
    if (port->elapsed >= port->time) {
        port->value = port->end;
        return 1;
    }
    port->value = (port->delta * port->elapsed) + port->start;
    port->elapsed += 1;
    return 0;
}

// Frees an entry and unlinks it. Its own prev/next stay, so a walk can step on from an entry it has just freed.
void Nai_PortBufferFree(NaiPort* port) {
    if (port != NULL) {
        PRINTF("*** FREE ***");
        port->inUse = 0;
        if (port->prev == NULL) {
            gNaiPortList = port->next;
        } else {
            port->prev->next = port->next;
        }
        if (port->next != NULL) {
            port->next->prev = port->prev;
        }
    }
}

// Claims the first free entry, clears its ownerType and elapsed and appends it to the list; NULL when the pool is
// full. The caller fills in the command.
// Load-bearing: the `u16` counter.
NaiPort* Nai_PortBufferAlloc(void) {
    NaiPort* port;
    NaiPort* tail;
    u16 i;

    for (i = 0; i < 0x100; i++) {
        port = &gNaiPortBuffer[i];
        if (port->inUse == 0) {
            port->inUse = 1;
            port->ownerType = 0;
            port->elapsed = 0;
            tail = Nai_PortBufferGetTail();
            if (tail == NULL) {
                gNaiPortList = port;
                port->prev = NULL;
            } else {
                tail->next = port;
                port->prev = tail;
            }
            port->next = NULL;
            return port;
        }
    }
    PRINTF("NAI: オーバーフロー PORT in PortBufferAlloc\n");
    return NULL;
}

// Derives the per-tick delta of a ramp from its endpoints and its length.
void Nai_PortRampCalcDelta(NaiPort* port) {
    if (port != NULL) {
        if (port->time != 0) {
            port->delta = (port->end - port->start) / port->time;
        }
    }
}

// First entry on the list, or NULL when it is empty.
NaiPort* Nai_PortBufferGetHead(void) {
    return gNaiPortList;
}

// Last entry on the list, or NULL when it is empty.
NaiPort* Nai_PortBufferGetTail(void) {
    NaiPort* port;

    port = gNaiPortList;
    if (port == NULL) {
        return NULL;
    }
    while (port->next != NULL) {
        port = port->next;
    }
    return port;
}

// Cancels every queued command of BGM handle `bgm`.
void Nai_PortBufferFreeBgm(NaiBgm* bgm) {
    NaiPort* port;

    port = Nai_PortBufferGetHead();
    while (port != NULL) {
        if ((port->ownerType == 1) && (port->bgm == bgm)) {
            Nai_PortBufferFree(port);
        }
        port = port->next;
    }
}

// Cancels every queued command of SE slot `se`.
void Nai_PortBufferFreeSe(NaiSe* se) {
    NaiPort* port;

    port = Nai_PortBufferGetHead();
    while (port != NULL) {
        if ((port->ownerType == 2) && (port->se == se)) {
            Nai_PortBufferFree(port);
        }
        port = port->next;
    }
}

// Whether BGM handle `bgm` has a command of NaiPortType `type` queued. Uncalled.
s32 Nai_PortBufferHasType(s32 type, NaiBgm* bgm) {
    NaiPort* port;

    port = Nai_PortBufferGetHead();
    while (port != NULL) {
        if ((port->ownerType == 1) && (port->bgm == bgm) && (port->type == type)) {
            return 1;
        }
        port = port->next;
    }
    return 0;
}

#include "global.h"
#include "audio.h"

// The BGM handle's lifecycle: start a handle in one of two modes (take a group at once, or wait for one), stop it,
// release it.
// Every function name here is INVENTED (no string).

// Allocates a handle for sequence num and starts it at once (Nai_BgmTakeGroup); NULL when no handle is free.
NaiBgm* Nai_BgmStart(s16 num, s16 time, s32 alongside) {
    NaiBgm* bgm;

    bgm = Nai_BgmBufferAlloc();
    if (bgm != NULL) {
        bgm->num = num;
        Nai_BgmTakeGroup(bgm, time, alongside);
    }
    return bgm;
}

// Restarts a handle the caller holds; when Nai_BgmIsActive accepts it, the handle is stopped and its group dropped
// first. No callers.
void Nai_BgmRestart(NaiBgm* bgm, s16 time, s32 alongside) {
    if (bgm != NULL) {
        if (Nai_BgmIsActive(bgm)) {
            Nai_BgmStop(bgm, 0);
            Nai_BgmGroupUnregister(bgm->group);
        }
        Nai_BgmTakeGroup(bgm, time, alongside);
    }
}

// The Nai_BgmWaitForGroup twins of the pair above.
NaiBgm* Nai_BgmStartWaiting(s16 num, s16 time, s32 alongside) {
    NaiBgm* bgm;

    bgm = Nai_BgmBufferAlloc();
    if (bgm != NULL) {
        bgm->num = num;
        Nai_BgmWaitForGroup(bgm, time, alongside);
    }
    return bgm;
}

void Nai_BgmRestartWaiting(NaiBgm* bgm, s16 time, s32 alongside) {
    if (bgm != NULL) {
        if (Nai_BgmIsActive(bgm)) {
            Nai_BgmStop(bgm, 0);
            Nai_BgmGroupUnregister(bgm->group);
        }
        Nai_BgmWaitForGroup(bgm, time, alongside);
    }
}

// Stops a handle: PLAYING, STARTING and 5 post the stop over `time` frames, turn STOPPING and make the opposite
// group's handle gNaiCurrentBgm; any other state is released at once.
void Nai_BgmStop(NaiBgm* bgm, s16 time) {
    if (bgm == NULL) {
        return;
    }
    if (bgm->status == NAI_BGM_STATUS_PLAYING || bgm->status == NAI_BGM_STATUS_STARTING ||
        bgm->status == NAI_BGM_STATUS_5) {
        Nai_SeqStop(bgm->group, time);
        bgm->status = NAI_BGM_STATUS_STOPPING;
        gNaiCurrentBgm = Nai_BgmGroupGetOpposite(bgm);
    } else {
        Nai_BgmRelease(bgm);
    }
}

// Nai_BgmStop with keep raised: a stopping handle is parked as KEPT instead of freed once its group falls silent.
// No callers.
void Nai_BgmStopKeep(NaiBgm* bgm, s16 time) {
    if (bgm != NULL) {
        bgm->keep = 1;
        Nai_BgmStop(bgm, time);
    }
}

// Teardown: takes a WAITING handle off the waiting list, unregisters its group, cancels its queued commands and
// frees the slot.
void Nai_BgmRelease(NaiBgm* bgm) {
    if (bgm != NULL) {
        if (bgm->status == NAI_BGM_STATUS_WAITING) {
            Nai_BgmWaitRemove(bgm);
        }
        Nai_BgmGroupUnregisterBgm(bgm);
        Nai_PortBufferFreeBgm(bgm);
        bgm->status = NAI_BGM_STATUS_FREE;
    }
}

// Whether the handle is PLAYING, STOPPING or 5 (Nai_BgmStop takes STARTING instead of STOPPING); Nai_PortRoutin,
// Nai_BgmPostChanges and the restarts act only on such a handle.
s32 Nai_BgmIsActive(NaiBgm* bgm) {
    if (bgm == NULL) {
        return 0;
    }
    if (bgm->status == NAI_BGM_STATUS_PLAYING || bgm->status == NAI_BGM_STATUS_STOPPING ||
        bgm->status == NAI_BGM_STATUS_5) {
        return 1;
    }
    return 0;
}

// Starts a handle WAITING: parks it on the waiting list instead of posting the start.
void Nai_BgmWaitForGroup(NaiBgm* bgm, s16 time, s32 alongside) {
    bgm->status = NAI_BGM_STATUS_WAITING;
    bgm->time = time;
    bgm->alongside = alongside;
    Nai_BgmWaitAdd(bgm);
}

// Starts a handle QUEUED on a group taken at once: alongside 0 first retires the current BGM and the handle
// opposite it, 1 only the latter; registering last assigns the group.
// Load-bearing: `switch (bgm->alongside)`.
void Nai_BgmTakeGroup(NaiBgm* bgm, s16 time, s32 alongside) {
    NaiBgm* other;

    bgm->status = NAI_BGM_STATUS_QUEUED;
    bgm->keep = 0;
    bgm->time = time;
    bgm->alongside = alongside;
    other = Nai_BgmGroupGetOpposite(gNaiCurrentBgm);
    switch (bgm->alongside) {
        case 0:
            if (gNaiCurrentBgm != NULL) {
                gNaiCurrentBgm->keep = 1;
                Nai_SeqStop(gNaiCurrentBgm->group, 0);
            }
            if (other != NULL) {
                other->keep = 1;
                Nai_SeqStop(other->group, 0);
            }
            break;
        case 1:
            if (other != NULL) {
                other->keep = 1;
                Nai_SeqStop(other->group, 0);
            }
            break;
    }
    bgm->group = Nai_BgmGroupRegister(bgm);
}

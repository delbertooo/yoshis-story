#include "global.h"
#include "audio.h"

// Starting and stopping sounds in the SE buffer: fire and forget (Nai_SeTrig) or a slot the caller keeps
// (Nai_SeStart, Nai_SeStop); pausing the buffer.
// Every function name here but Nai_SeTrig, Nai_SeStart and Nai_SeStop (from the debug strings that name them) is
// INVENTED (no string).
// The PRINTFs are stripped debug prints whose strings stay in .rodata.

f32 gNaiSePausedTune[0x32];

// Nai_SeTrig with defaults: no group, priority 1, centre pan (0x3F), volume 1.0. No callers.
void Nai_SeTrigDefault(s16 sndId) {
    Nai_SeTrig(sndId, -1, 1, 0x3F, 1.0f);
}

// The Nai_SeStart twin of Nai_SeTrigDefault. No callers.
void Nai_SeStartDefault(s16 sndId) {
    Nai_SeStart(sndId, -1, 1, 0x3F, 1.0f);
}

// Fire and forget (owned 0): fills a slot and gives it a voice, or drops the sound when Nai_SeSubAlloc refuses it.
// Does nothing while a spec change is in flight.
// Load-bearing: the `alloc`/`voice` pair and its chained assignment.
void Nai_SeTrig(s16 sndId, s16 group, s16 priority, s16 pan, f32 volume) {
    NaiSe* se;
    s32 alloc;
    s16 voice;

    if (gNaiSpecChange != 0) {
        return;
    }
    se = Nai_SeBufferAlloc();
    if (se == NULL) {
        PRINTF("NAI: SE Buffer Over Flow!\n");
        return;
    }
    PRINTF("NAI: Nai_SeTrig[%x] num=%d, group=%d, priority=%d, pan=%d, volume=%f\n");

    se->sndId = sndId;
    se->owned = 0;
    se->priority = priority;
    se->group = group;
    Nai_SeSetPanVolume(se, pan, volume);

    voice = alloc = Nai_SeSubAlloc(se);
    if (alloc < 0) {
        return;
    }
    se->sub = voice;
    Nai_SeMuteSub(se);
}

// Nai_SeTrig for a slot the caller keeps (owned 1, returned; NULL when full): without a voice it waits in WAITING
// for Nai_SeRoutin to retry. No spec-change test.
// Load-bearing: the `alloc`/`voice` pair.
NaiSe* Nai_SeStart(s16 sndId, s16 group, s16 priority, s16 pan, f32 volume) {
    NaiSe* se;
    s32 alloc;
    s16 voice;

    se = Nai_SeBufferAlloc();
    if (se == NULL) {
        PRINTF("NAI: SE Buffer Over Flow!\n");
        return NULL;
    }
    PRINTF("NAI: Nai_SeStart[%x] num=%d, group=%d, priority=%d, pan=%d, volume=%f\n");

    se->sndId = sndId;
    se->owned = 1;
    se->priority = priority;
    se->group = group;
    Nai_SeSetPanVolume(se, pan, volume);

    voice = alloc = Nai_SeSubAlloc(se);
    if (alloc < 0) {
        PRINTF("NAI: NO SUB TRACK\n");
        se->status = NAI_SE_STATUS_WAITING;
    } else {
        se->sub = voice;
        Nai_SeMuteSub(se);
    }
    return se;
}

// Stops an owned slot (Nai_SeStart's): mutes its voice if active, then frees it and cancels its queued commands.
void Nai_SeStop(NaiSe* se) {
    if (se != NULL) {
        if (se->owned == 1) {
            PRINTF("NAI: Nai_SeStop[%x]\n");
            if (Nai_SeIsActive(se) != 0) {
                Nai_SubPortWrite(2, se->sub, 1, 0);
            }
            se->status = NAI_SE_STATUS_FREE;
            Nai_PortBufferFreeSe(se);
        }
    }
}

// Pauses (pause != 0) or resumes every live slot: pausing parks its tune target in gNaiSePausedTune and sets the
// tune to 0 at once, resuming restores it. Nai_SeRoutin sends no note for a paused slot.
void Nai_SePause(u8 pause) {
    NaiSe* se;
    s16 i;

    for (i = 0; i < 0x32; i++) {
        se = &gNaiSeBuffer[i];
        if (se->status != NAI_SE_STATUS_FREE) {
            if (pause != 0) {
                se->paused = 1;
                gNaiSePausedTune[i] = se->tuneTarget;
                Nai_SeSetTune(se, 0.0f, 0);
            } else if (se->paused != 0) {
                se->paused = 0;
                Nai_SeSetTune(se, gNaiSePausedTune[i], 0);
            }
        }
    }
}

// Whether `se` is STARTING or PLAYING (NULL is not).
s32 Nai_SeIsActive(NaiSe* se) {
    if (se == NULL) {
        return 0;
    }
    if ((se->status == NAI_SE_STATUS_STARTING) || (se->status == NAI_SE_STATUS_PLAYING)) {
        return 1;
    }
    return 0;
}

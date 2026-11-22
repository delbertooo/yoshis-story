#include "global.h"
#include "audio.h"

// The SE buffer: the 50-slot table of sound effects playing or about to, and the voice (sub) each slot gets. Here a
// slot is claimed, given a voice (one of the SE group's subs, taken from a weaker slot if need be) and sent its note.
// Every function name here is INVENTED (no string).

s16 gNaiSeSubCount = 10;
NaiSe* gNaiSeLastAlloc = NULL;

NaiSe gNaiSeBuffer[0x32];

// Starts sequence 0 on the SE group (2) and writes the sub count, gNaiSeSubCount, to the group's port 0.
void Nai_SeGroupStart(void) {
    Nai_SeqStart(2, 0, 0);
    Nai_GrpPortWrite(2, 0, gNaiSeSubCount);
}

// Stages a trigger's pan, clamped to 0..0x7F, and volume, clamped at 0, both at once.
void Nai_SeSetPanVolume(NaiSe* se, s16 pan, f32 volume) {
    if (se != NULL) {
        if (pan < 0) {
            pan = 0;
        }
        if (pan >= 0x80) {
            pan = 0x7F;
        }
        if (volume < 0.0f) {
            volume = 0.0f;
        }
        Nai_SeSetPan(se, pan, 0);
        Nai_SeSetVolume(se, volume, 0);
    }
}

// Step to QUEUED: the slot has a voice, so mute it and wait for it to fall idle.
void Nai_SeMuteSub(NaiSe* se) {
    se->status = NAI_SE_STATUS_QUEUED;
    Nai_SubPortWrite(2, se->sub, 1, 0);
}

// Step QUEUED -> STARTING: unmutes the voice, sends sndId as note (% 128) and bank (/ 128), and snaps the slot's
// ramps to their defaults.
void Nai_SeSendNote(NaiSe* se) {
    se->status = NAI_SE_STATUS_STARTING;
    Nai_SubPortWrite(2, se->sub, 0, 0);
    Nai_SubPortWrite(2, se->sub, 4, se->sndId % 128);
    Nai_SubPortWrite(2, se->sub, 5, se->sndId / 128);
    Nai_SeResetCurrent(se);
}

// Finds the first FREE slot and sets its defaults: no group, unpaused, priority, volume and tune 1.0, pan 63,
// fxmix 0; it stays FREE until its caller starts it. Returns it (NULL when full), latched in gNaiSeLastAlloc.
NaiSe* Nai_SeBufferAlloc(void) {
    NaiSe* found = NULL;
    s16 i;

    for (i = 0; i < 0x32; i++) {
        if (gNaiSeBuffer[i].status == NAI_SE_STATUS_FREE) {
            found = &gNaiSeBuffer[i];
            found->group = -1;
            found->paused = 0;
            found->priority = 1.0f;
            found->volumeTarget = 1.0f;
            found->tuneTarget = 1.0f;
            found->panTarget = 63.0f;
            found->fxmixTarget = 0.0f;
            break;
        }
    }
    gNaiSeLastAlloc = found;
    return found;
}

// Snaps the current half of the four ramps to Nai_SeBufferAlloc's default targets and clears their frame counts.
void Nai_SeResetCurrent(NaiSe* se) {
    if (se != NULL) {
        se->volumeTime = 0;
        se->panTime = 0;
        se->tuneTime = 0;
        se->fxmixTime = 0;
        se->volume = 1.0f;
        se->tune = 1.0f;
        se->pan = 63.0f;
        se->fxmix = 0.0f;
    }
}

// Picks a voice for `se`, or -1: its group's, else the first free, else the weakest held. A held voice goes to `se`
// only if `se` is no weaker (stronger, if WAITING); its holder is freed or, if owned, sent back to WAITING.
// Load-bearing: the dead `startedKind = 1` in the `owned` test, the `count` local, the empty FREE/WAITING arm and the
// double `-1.0`.
s32 Nai_SeSubAlloc(NaiSe* se) {
    NaiSe* other;
    s16 best;
    s16 i;
    f32 bestPrio;
    f32 prio[0x10];
    s16 count;
    s32 startedKind;

    best = -1;
    bestPrio = -1.0f;
    for (i = 0; i < 0x10; i++) {
        prio[i] = -1.0f;
    }

    for (i = 0; i < 0x32; i++) {
        other = &gNaiSeBuffer[i];
        switch (other->status) {
            case NAI_SE_STATUS_FREE:
            case NAI_SE_STATUS_WAITING:
                break;
            case NAI_SE_STATUS_QUEUED:
            case NAI_SE_STATUS_STARTING:
            case NAI_SE_STATUS_PLAYING:
                prio[other->sub] = Nai_SePriority(other);
                if ((se->group >= 0) && (se->group == other->group)) {
                    best = other->sub;
                    bestPrio = prio[other->sub];
                }
                break;
        }
    }

    if (best < 0) {
        count = gNaiSeSubCount;
        for (i = 0; i < count; i++) {
            if (prio[i] == -1.0) {
                return i;
            }
            if (bestPrio < prio[i]) {
                best = i;
                bestPrio = prio[i];
            }
        }
    }

    if (((se->status == NAI_SE_STATUS_FREE) && (Nai_SePriority(se) <= bestPrio)) ||
        ((se->status == NAI_SE_STATUS_WAITING) && (Nai_SePriority(se) < bestPrio))) {
        other = Nai_SeFindBySub(best);
        if (other->owned != 0) {
            if (other->owned == (startedKind = 1)) {
                other->status = NAI_SE_STATUS_WAITING;
            }
        } else {
            other->status = NAI_SE_STATUS_FREE;
            Nai_PortBufferFreeSe(other);
        }
        return best;
    }
    return -1;
}

// Marks all 50 slots FREE; Nai_SeInit cancels their queued commands first.
void Nai_SeBufferClear(void) {
    s16 i;

    for (i = 0; i < 0x32; i++) {
        gNaiSeBuffer[i].status = NAI_SE_STATUS_FREE;
    }
}

// Whether voice `sub` sounds: the SE group's sequence player is enabled and that channel's first layer is set.
// Load-bearing: the gAudioCtxSeqPlayers alias in the `enabled` test.
s32 Nai_SeSubIsPlaying(s16 sub) {
    if (gAudioCtxSeqPlayers[2].enabled == 0) {
        return 0;
    }
    if (gAudioCtx.seqPlayers[2].channels[sub]->layers[0] == NULL) {
        return 0;
    }
    return 1;
}

// The slot that holds voice `sub` (QUEUED, STARTING or PLAYING), or NULL.
NaiSe* Nai_SeFindBySub(s16 sub) {
    NaiSe* se;
    s16 i;

    for (i = 0; i < 0x32; i++) {
        se = &gNaiSeBuffer[i];
        switch (se->status) {
            case NAI_SE_STATUS_QUEUED:
            case NAI_SE_STATUS_STARTING:
            case NAI_SE_STATUS_PLAYING:
                if (se->sub == sub) {
                    return se;
                }
                break;
        }
    }
    return NULL;
}

// A slot's priority for Nai_SeSubAlloc, where larger is weaker: its trigger's priority, or 100000 once its volume
// target is 0, so a silenced slot loses its voice first.
// Load-bearing: the `prio` local and the double `0.0`.
f32 Nai_SePriority(NaiSe* se) {
    f32 prio;

    if (se->volumeTarget == 0.0) {
        prio = 100000.0f;
    } else {
        prio = se->priority;
    }
    return prio;
}

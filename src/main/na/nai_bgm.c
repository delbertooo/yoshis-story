#include "global.h"
#include "audio.h"

// The BGM layer's state machine, Nai_BgmRoutin, over the 10 handles of gNaiBgmBuffer. Its group registry and
// waiting list are separate objects (nai_bgm_group.c, nai_bgm_wait.c), each with a 16-aligned `.bss` block.
// Every function name here but Nai_BgmRoutin (from its exception print's "BgmRoutin") is INVENTED (no string).
// The PRINTFs are stripped debug prints whose strings stay in .rodata.

NaiBgm gNaiBgmBuffer[10];
NaiBgm* gNaiCurrentBgm;

// Library reset, the BGM twin of Nai_SeInit: cancels the queued commands of every live handle, then empties the
// handles, the group registry and the waiting list.
void Nai_BgmInit(void) {
    NaiBgm* handle;
    s16 i;

    for (i = 0; i < 10; i++) {
        handle = &gNaiBgmBuffer[i];
        if (handle->status != NAI_BGM_STATUS_FREE) {
            Nai_PortBufferFreeBgm(handle);
        }
    }
    Nai_BgmBufferClear();
    Nai_BgmGroupClear();
    Nai_BgmWaitClear();
}

// Per frame, for each of the 10 handles: posts its changes, then steps its NaiBgmStatus; a group sounds while
// gAudioCtx.seqPlayers[group].enabled is set. The PRINTFs are a stripped dump of the buffer, one tag per handle.
// Load-bearing: the discarded Nai_BgmGroupGetOpposite call, the case order, `busy` as `u32` and the
// `gNaiCurrentBgm == bgm` test.
void Nai_BgmRoutin(void) {
    NaiBgm* bgm;
    u32 busy;
    u16 i;

    Nai_BgmGroupGetOpposite(gNaiCurrentBgm);
    PRINTF("NAI: BGM Buffer [ ");
    for (i = 0; i < 10; i++) {
        bgm = &gNaiBgmBuffer[i];
        Nai_BgmPostChanges(bgm);
        if (gNaiCurrentBgm == bgm) {
            PRINTF("+");
        } else {
            PRINTF(" ");
        }
        switch (bgm->status) {
            case NAI_BGM_STATUS_FREE:
                PRINTF("- ");
                break;

            case NAI_BGM_STATUS_5:
                PRINTF("P ");
                break;

            case NAI_BGM_STATUS_STOPPING:
                PRINTF("F ");
                if (gAudioCtx.seqPlayers[bgm->group].enabled == 0) {
                    switch (bgm->keep) {
                        case 1:
                            bgm->status = NAI_BGM_STATUS_KEPT;
                            break;
                        case 0:
                            bgm->status = NAI_BGM_STATUS_FREE;
                            Nai_PortBufferFreeBgm(bgm);
                            break;
                    }
                    Nai_BgmGroupUnregister(bgm->group);
                }
                break;

            case NAI_BGM_STATUS_QUEUED:
                PRINTF("s ");
                if (bgm->startDelay > 0) {
                    bgm->startDelay--;
                    break;
                }
                if (gAudioCtx.seqPlayers[bgm->group].enabled == 0) {
                    bgm->status = NAI_BGM_STATUS_STARTING;
                    Nai_SeqStart(bgm->group, bgm->num, bgm->time);
                    Nai_BgmResetCurrent(bgm);
                }
                break;

            case NAI_BGM_STATUS_STARTING:
                PRINTF("S ");
                if (gAudioCtx.seqPlayers[bgm->group].enabled) {
                    gNaiCurrentBgm = bgm;
                    bgm->status = NAI_BGM_STATUS_PLAYING;
                }
                break;

            case NAI_BGM_STATUS_PLAYING:
                PRINTF("%d ");
                if (gAudioCtx.seqPlayers[bgm->group].enabled == 0) {
                    switch (bgm->keep) {
                        case 1:
                            bgm->status = NAI_BGM_STATUS_KEPT;
                            break;
                        case 0:
                            bgm->status = NAI_BGM_STATUS_FREE;
                            Nai_PortBufferFreeBgm(bgm);
                            break;
                    }
                    Nai_BgmGroupUnregister(bgm->group);
                    gNaiCurrentBgm = Nai_BgmGroupGetOpposite(bgm);
                } else if (bgm->alongside == 0) {
                    if (gAudioCtx.seqPlayers[Nai_BgmAnotherGroup(bgm->group)].enabled) {
                        Nai_BgmRestartWaiting(bgm, 0, 0);
                    }
                }
                break;

            case NAI_BGM_STATUS_WAITING:
                PRINTF("W ");
                if (Nai_BgmWaitGetHead() == bgm) {
                    busy = 0;
                    bgm->group = Nai_BgmGroupFindFree();
                    switch (bgm->alongside) {
                        case 0:
                            if ((Nai_BgmGroupGet(0) != NULL) || (Nai_BgmGroupGet(1) != NULL)) {
                                busy = 1;
                            }
                            break;
                        case 1:
                            if (bgm->group < 0) {
                                busy = 1;
                            }
                            break;
                    }
                    if (busy == 0) {
                        Nai_BgmGroupRegister(bgm);
                        Nai_BgmWaitRemove(bgm);
                        bgm->status = NAI_BGM_STATUS_QUEUED;
                        bgm->keep = 0;
                    }
                }
                break;

            case NAI_BGM_STATUS_KEPT:
                PRINTF("* ");
                break;

            default:
                PRINTF("NAI: 例外 bgm->status in BgmRoutin\n");
                break;
        }
    }
    PRINTF("]\n");
}

// Drops the current BGM and marks every handle free.
void Nai_BgmBufferClear(void) {
    u16 i;

    gNaiCurrentBgm = NULL;
    for (i = 0; i < 10; i++) {
        gNaiBgmBuffer[i].status = NAI_BGM_STATUS_FREE;
    }
}

// The first free handle, with its startDelay and the requested half of each pair reset; NULL when all ten are in
// use. The slot stays free until the caller starts the handle.
NaiBgm* Nai_BgmBufferAlloc(void) {
    NaiBgm* handle;
    u16 i;

    handle = NULL;
    for (i = 0; i < 10; i++) {
        if (gNaiBgmBuffer[i].status == NAI_BGM_STATUS_FREE) {
            handle = &gNaiBgmBuffer[i];
            handle->startDelay = 0;
            handle->muteSubMask = 0;
            handle->tuneSubMask = 0;
            handle->volumeTarget = 1.0f;
            handle->tuneTarget = 1.0f;
            handle->tempoOffsetTarget = 0.0f;
            break;
        }
    }
    return handle;
}

// Resets the current half of each pair to its default and zeroes the three ramp lengths; Nai_BgmRoutin calls it
// right after posting the start.
void Nai_BgmResetCurrent(NaiBgm* bgm) {
    if (bgm != NULL) {
        bgm->volumeTime = 0;
        bgm->tempoOffsetTime = 0;
        bgm->muteSubMaskLive = 0;
        bgm->tuneSubMaskSent = 0;
        bgm->tuneTime = 0;
        bgm->volume = 1.0f;
        bgm->tune = 1.0f;
        bgm->tempoOffset = 0.0f;
    }
}

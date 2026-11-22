#include "global.h"
#include "audio.h"

// What the game reports to the sound library: the setters store a scene's result, the Chibi Yoshi flag, the select
// cursor and the active yoshis for later readers; Na_SetTimerCount drives the countdown SEs at once.
// The PRINTFs are stripped debug prints whose strings stay in .rodata.

s16 gNaCursorX = 0x3F;
s16 gNaCursorY = 0x3F;
u32 gNaActiveYoshiMask = 0xFF;
u8 gNaChibiYoshi = 0;
s16 gNaResult = 1;

void Na_SetResult(s16 result) {
    PRINTF("NA : Na_SetResult %d\n");
    gNaResult = result;
}

void Na_SetChibiYoshi(s16 chibiYoshi) {
    PRINTF("NA : Na_SetChibiYoshi %d\n");
    gNaChibiYoshi = chibiYoshi;
}

// Drives timerType's countdown SE pair (0: 0x6F/0x70, 1: 0x71/0x72) by count: the first from 0xD3 (type 1: 0xCF)
// up, the second from 30, neither below; a page end stops all four, and a slot already playing is not restarted.
void Na_SetTimerCount(s32 timerType, s16 count) {
    PRINTF("NA : Na_SetTimerCount %d\n");
    if (gNaPageEnd != 0) {
        Na_SeLevelStop(0x6F, 0);
        Na_SeLevelStop(0x70, 0);
        Na_SeLevelStop(0x71, 0);
        Na_SeLevelStop(0x72, 0);
        return;
    }

    switch (timerType) {
        case 0:
            if (count >= 0xD3) {
                Na_SeLevelStop(0x70, 0);
                if (Na_SeLevelCheck(0x6F, 0) != 1) {
                    Na_SeStart(0x6F, 0, NULL);
                }
            } else if (count < 0x1E) {
                Na_SeLevelStop(0x6F, 0);
                Na_SeLevelStop(0x70, 0);
            } else {
                Na_SeLevelStop(0x6F, 0);
                if (Na_SeLevelCheck(0x70, 0) != 1) {
                    Na_SeStart(0x70, 0, NULL);
                }
            }
            break;

        case 1:
            if (count >= 0xCF) {
                Na_SeLevelStop(0x72, 0);
                if (Na_SeLevelCheck(0x71, 0) != 1) {
                    Na_SeStart(0x71, 0, NULL);
                }
            } else if (count < 0x1E) {
                Na_SeLevelStop(0x71, 0);
                Na_SeLevelStop(0x72, 0);
            } else {
                Na_SeLevelStop(0x71, 0);
                if (Na_SeLevelCheck(0x72, 0) != 1) {
                    Na_SeStart(0x72, 0, NULL);
                }
            }
            break;
    }
}

// Script IO port 0 of channel 15 on the current BGM's seq player, which the sequence keeps in step with the walk;
// -1 with no BGM, while the port's own -1 (unset, or the player disabled) reads as 1.
// Load-bearing: the -1 test on the wide `step`, the result in the narrow `status`.
s16 Na_GetYoshiStepStatus(void) {
    s32 step;
    s16 status;

    if (gNaBgm == NULL) {
        PRINTF("NA : Na_GetYoshiStepStatus NO BGM\n");
        return -1;
    }

    step = AudioThread_GetChannelIO(gNaBgm->group, 0xF, 0);
    status = step;
    if (step == -1) {
        status = 1;
    }
    PRINTF("NA : Na_GetYoshiStepStatus %d\n");
    return status;
}

void Na_SelectCursor(s16 x, s16 y) {
    PRINTF("NA : Na_SelectCursor [ x=%d y=%d ]\n");
    gNaCursorX = x;
    gNaCursorY = y;
}

// One bit per yoshi, all set at boot.
void Na_SetActiveYoshi(u32 mask) {
    PRINTF("NA : Na_SetActiveYoshi %x\n");
    gNaActiveYoshiMask = mask;
}

// INVENTED name (no string). The reader for that mask, inverted (a set bit means that yoshi is gone) and permuted
// from the caller's bit order into the library's.
u16 Na_GetYoshiMuteMask(void) {
    u32 mask = gNaActiveYoshiMask;
    u16 ret = 0;

    if (!(mask & 0x02)) {
        ret |= 0x01;
    }
    if (!(mask & 0x80)) {
        ret |= 0x02;
    }
    if (!(mask & 0x01)) {
        ret |= 0x04;
    }
    if (!(mask & 0x08)) {
        ret |= 0x08;
    }
    if (!(mask & 0x04)) {
        ret |= 0x10;
    }
    if (!(mask & 0x10)) {
        ret |= 0x20;
    }
    if (!(mask & 0x40)) {
        ret |= 0x40;
    }
    if (!(mask & 0x20)) {
        ret |= 0x80;
    }
    return ret;
}

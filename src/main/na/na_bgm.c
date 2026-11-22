#include "global.h"
#include "audio.h"

// The three BGM handles (scene music, sub-game track, jingle) and the scene -> BGM table. A fadeTime is a frame
// count: the fade-in of a start, the fade-out of a stop.
// The PRINTFs are stripped debug prints whose strings stay in .rodata.

NaiBgm* gNaBgm = NULL;
NaiBgm* gNaSubGameBgm = NULL;
NaiBgm* gNaJingleBgm = NULL;

s16 gNaBgmInterruptedNum;

// INVENTED name (no string). Forgets all three handles without stopping them, after a driver reset: the clear-only
// twin of Na_AllBgmStop.
void Na_AllBgmClear(void) {
    gNaBgm = NULL;
    gNaSubGameBgm = NULL;
    gNaJingleBgm = NULL;
}

// Fades the scene BGM out and drops its handle.
void Na_BgmStop(s16 fadeTime) {
    PRINTF("NA : Na_BgmStop\n");
    Nai_BgmStop(gNaBgm, fadeTime);
    gNaBgm = NULL;
}

// Starts bgmId unmuted as the sub-game track over the scene BGM; its callers raise gNaBgm's alongside flag first.
void Na_SubGameBgmStart(s16 bgmId, s16 fadeTime) {
    PRINTF("NA : Na_SubGameBgmStart\n");
    // The assert's check left no code, here or in the callers.
    PRINTF("NA : Na_SubGameBgmStart SUBGAME BGM not set in scene %d\n");
    gNaSubGameBgm = Nai_BgmStart(bgmId, fadeTime, 1);
    Nai_BgmSetMuteSubMask(gNaSubGameBgm, 0);
}

void Na_SubGameBgmStop(s16 fadeTime) {
    PRINTF("NA : Na_SubGameBgmStop\n");
    Nai_BgmStop(gNaSubGameBgm, fadeTime);
    gNaSubGameBgm = NULL;
}

// INVENTED name (no string). Na_SubGameBgmStart on the jingle slot; every caller is a one-shot fanfare in Na_Event.
void Na_JingleBgmStart(s16 bgmId, s16 fadeTime) {
    gNaJingleBgm = Nai_BgmStart(bgmId, fadeTime, 1);
    Nai_BgmSetMuteSubMask(gNaJingleBgm, 0);
}

// INVENTED name (no string). Over a track Na_BgmFollowsYoshiStatus accepts, plays track 0x14 and remembers the
// track it replaced (not again while interrupted) for Na_BgmResume.
void Na_BgmInterrupt(void) {
    if (Na_BgmFollowsYoshiStatus() != 0) {
        if (gNaBgm != NULL && gNaBgmInterrupted == 0) {
            gNaBgmInterruptedNum = gNaBgm->num;
        }
        gNaBgmInterrupted = 1;
        Na_BgmChange(0x14, 0);
    }
}

// INVENTED name (no string). Ends the interrupt, restarting the remembered track only while track 0x14 still plays.
void Na_BgmResume(void) {
    if (gNaBgmInterrupted != 0) {
        gNaBgmInterrupted = 0;
        if (gNaBgm != NULL && gNaBgm->num == 0x14) {
            Na_BgmChange(gNaBgmInterruptedNum, 0);
        }
    }
}

// INVENTED name (no string). Re-applies the scene and sub-game BGM volume for the duck state: 0.5 while paused, 0.6
// while the timer runs, 1.0 otherwise.
void Na_BgmVolumeUpdate(void) {
    if (gNaPause != 0) {
        Nai_BgmSetVolume(gNaBgm, 0.5f, 0);
        Nai_BgmSetVolume(gNaSubGameBgm, 0.5f, 0);
    } else if (gNaTimerBgm != 0) {
        Nai_BgmSetVolume(gNaBgm, 0.6f, 0);
        Nai_BgmSetVolume(gNaSubGameBgm, 0.6f, 0);
    } else {
        Nai_BgmSetVolume(gNaBgm, 1.0f, 0);
        Nai_BgmSetVolume(gNaSubGameBgm, 1.0f, 0);
    }
}

// For a bgmId >= 0: fades the scene BGM out over fadeTime and starts bgmId without a ramp, then re-applies the
// volume, an open mute mask, the Yoshi status and Minobon 2.
void Na_BgmChange(s16 bgmId, s16 fadeTime) {
    PRINTF("NA : Na_BgmChange %d\n");
    if (bgmId >= 0) {
        Na_BgmStop(fadeTime);
        gNaBgm = Nai_BgmStartWaiting(bgmId, 0, 0);
        Na_BgmVolumeUpdate();
        Nai_BgmSetMuteSubMask(gNaBgm, 0);
        Na_YoshiStatusChange(gNaYoshiStatus);
        Na_SetWhichMinobon(2);
    }
}

// Stops all three handles at once; the jingle's handle is stopped but not cleared.
void Na_AllBgmStop(void) {
    PRINTF("NA : Na_AllBgmStop \n");
    Na_BgmStop(0);
    Na_SubGameBgmStop(0);
    Nai_BgmStop(gNaJingleBgm, 0);
}

// INVENTED name (no string). The scene -> BGM table: gNaScene's track, or -1 for a scene without one of its own.
// Load-bearing: the case order (roughly by BGM id) and the `bgmId = ...; break;` arms.
s32 Na_SceneBgmGet(void) {
    s32 bgmId = -1;

    switch (gNaScene) {
        case 0x01:
            bgmId = 0x01;
            break;
        case 0x02:
            bgmId = 0x02;
            break;
        case 0x03:
            bgmId = 0x03;
            break;
        case 0x04:
            bgmId = 0x04;
            break;
        case 0x05:
            bgmId = 0x05;
            break;
        case 0x06:
            bgmId = 0x06;
            break;
        case 0x07:
            bgmId = 0x07;
            break;
        case 0x08:
            bgmId = 0x08;
            break;
        case 0x09:
            bgmId = 0x09;
            break;
        case 0x0A:
            bgmId = 0x0A;
            break;
        case 0x16:
            bgmId = 0x0B;
            break;
        case 0x24:
            bgmId = 0x0C;
            break;
        case 0x26:
            bgmId = 0x0D;
            break;
        case 0x38:
            bgmId = 0x38;
            break;
        case 0x2C:
            bgmId = 0x0E;
            break;
        case 0x22:
            bgmId = 0x0F;
            break;
        case 0x0B:
            bgmId = 0x11;
            break;
        case 0x17:
            bgmId = 0x10;
            break;
        case 0x18:
            bgmId = 0x12;
            break;
        case 0x1C:
            bgmId = 0x13;
            break;
        case 0x1B:
            bgmId = 0x17;
            break;
        case 0x1A:
            bgmId = 0x19;
            break;
        case 0x19:
            bgmId = 0x16;
            break;
        case 0x11:
            bgmId = 0x14;
            break;
        case 0x1D:
            bgmId = 0x18;
            break;
        case 0x25:
            bgmId = 0x15;
            break;
        case 0x1E:
            bgmId = 0x1A;
            break;
        case 0x1F:
            bgmId = 0x1B;
            break;
        case 0x20:
            bgmId = 0x1C;
            break;
        case 0x21:
            bgmId = 0x1D;
            break;
        case 0x29:
            bgmId = 0x1E;
            break;
        case 0x12:
            bgmId = 0x1F;
            break;
        case 0x13:
            bgmId = 0x20;
            break;
        case 0x14:
            bgmId = 0x21;
            break;
        case 0x15:
            bgmId = 0x22;
            break;
        case 0x0E:
            bgmId = 0x23;
            break;
        case 0x0F:
            bgmId = 0x24;
            break;
        case 0x27:
            bgmId = 0x2E;
            break;
        case 0x0C:
            bgmId = 0x02;
            break;
        case 0x0D:
            bgmId = 0x08;
            break;
        case 0x23:
            bgmId = 0x2A;
            break;
        case 0x28:
            bgmId = 0x26;
            break;
        case 0x2A:
            bgmId = 0x27;
            break;
        case 0x2F:
            bgmId = 0x25;
            break;
        case 0x30:
            bgmId = 0x35;
            break;
        case 0x2D:
            bgmId = 0x2C;
            break;
        case 0x2E:
            bgmId = 0x34;
            break;
        case 0x2B:
            bgmId = 0x2B;
            break;
        case 0x31:
            bgmId = 0x28;
            break;
        case 0x32:
            bgmId = 0x29;
            break;
        case 0x10:
            bgmId = 0x2D;
            break;
        case 0x33:
            bgmId = 0x2F;
            break;
        case 0x34:
            bgmId = 0x30;
            break;
        case 0x35:
            bgmId = 0x31;
            break;
        case 0x36:
            bgmId = 0x32;
            break;
        case 0x37:
            bgmId = 0x33;
            break;
        case 0x39:
            bgmId = 0x36;
            break;
        case 0x3A:
            bgmId = 0x37;
            break;
        case 0x3B:
            bgmId = 0x39;
            break;
        case 0x3C:
            bgmId = 0x3A;
            break;
        case 0x3D:
            bgmId = 0x3B;
            break;
        case 0x3E:
            bgmId = 0x3C;
            break;
        case 0x3F:
            bgmId = 0x3D;
            break;
        case 0x40:
            bgmId = 0x3E;
            break;
        case 0x41:
            bgmId = 0x3F;
            break;
        case 0x42:
            bgmId = 0x40;
            break;
        case 0x43:
            bgmId = 0x41;
            break;
        case 0x44:
            bgmId = 0x42;
            break;
    }

    return bgmId;
}

#include "global.h"
#include "audio.h"

// The scene change (Na_SceneChange) and the list of tracks that follow Yoshi's status.
// The PRINTF is a stripped debug print whose string stays in .rodata.

s16 gNaScene = 0;
s16 gNaSceneSpec = -1;

// Reacts to a scene change: resets the per-scene flags and modes, stops the SEs, records the scene and its audio
// spec and changes to its BGM; scene 0x45 resets the library instead, and scene 0 only stops the level SEs.
// Load-bearing: the case order (0x45, 0, 0x25/0x27, 0x31) and `spec = &gNaSceneSpec;` at its use.
void Na_SceneChange(s32 scene) {
    s16 bgmId;
    s16* spec;

    PRINTF("NA : Na_SceneChange %d\n");
    gNaEcho = 0;
    gNaChibiYoshi = 0;
    gNaPageEnd = 0;
    gNaSe211Count = 0;
    if (gNaPause != 0) {
        Na_PauseOff();
    }
    if (gNaBgmInterrupted != 0) {
        Na_BgmResume();
    }
    if (gNaTimerBgm != 0) {
        Na_Event(NA_EVENT_TIMER_END);
    }
    if (gNaYoshiStatus == 3) {
        Na_YoshiStatusChange(1);
    }

    switch (scene) {
        case 0x45:
            Nai_Init();
            Na_AllBgmClear();
            Na_AllSeClear();
            return;
        case 0:
            Na_LevelSeStop();
            return;
        case 0x25:
        case 0x27:
            gNaSceneSpec = 2;
            break;
        case 0x31:
            gNaSceneSpec = 4;
            break;
        default:
            gNaSceneSpec = 0;
            break;
    }

    Na_AllSeStop();
    gNaScene = scene;
    if (gNaScene == 0x24) {
        bgmId = 0xC;
        if (gNaBgm != NULL) {
            switch (gNaBgm->num) {
                case 0x10:
                case 0x12:
                case 0x13:
                case 0x16:
                case 0x17:
                case 0x19:
                case 0x2E:
                    Na_Event(NA_EVENT_OPENING_PLAYER_SELECT);
                    return;
            }
        }
    } else {
        s32 next = Na_SceneBgmGet();

        bgmId = next;
        if (next < 0) {
            return;
        }
    }

    spec = &gNaSceneSpec;
    if (*spec != 0) {
        Nai_SpecChange(*spec);
        Na_AllSeClear();
        Na_AllBgmClear();
    }
    Na_SubGameBgmStop(0);
    Na_BgmChange(bgmId, 0x78);
    if (gNaBgm != NULL) {
        gNaBgm->startDelay = 0x14;
    }
    if (gNaScene == 0x24) {
        Nai_BgmSetMuteSubMask(gNaBgm, Na_GetYoshiMuteMask());
    }
    if (gNaScene == 0x32) {
        switch (gNaResult) {
            case 1:
                break;
            case 2:
                Nai_BgmSetMuteSubMask(gNaBgm, 8);
                break;
            case 3:
                Nai_BgmSetMuteSubMask(gNaBgm, 0xC);
                break;
            case 4:
            default:
                Nai_BgmSetMuteSubMask(gNaBgm, 0xE);
                break;
        }
    }
}

// INVENTED name (no string). True when the BGM playing is one of the 19 tracks Na_YoshiStatusChange re-voices per
// Yoshi status; that function and Na_BgmInterrupt both do nothing for any other track.
s32 Na_BgmFollowsYoshiStatus(void) {
    if (gNaBgm == NULL) {
        return 0;
    }
    switch (gNaBgm->num) {
        case 0x01:
        case 0x02:
        case 0x03:
        case 0x04:
        case 0x05:
        case 0x06:
        case 0x07:
        case 0x08:
        case 0x09:
        case 0x0A:
        case 0x0E:
        case 0x11:
        case 0x14:
        case 0x23:
        case 0x24:
        case 0x25:
        case 0x2D:
        case 0x32:
        case 0x35:
            return 1;
    }
    return 0;
}

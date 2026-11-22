#include "global.h"
#include "audio.h"

// The live-SE registry gNaSeHandles[se][subHandle] and its helpers, the pause jingles and the pan/volume mapping.
// The PRINTFs are stripped debug prints whose strings stay in .rodata.

NaiSe* gNaSeHandles[0x77][0xA];

// INVENTED name (MM's stub style). Empty and uncalled.
void Na_SeHandleNoop(void) {
}

// Whether slot gNaSeHandles[se][subHandle] is playing (1) or free (0); -1 for an id or sub-handle out of range.
s32 Na_SeLevelCheck(s32 se, s16 subHandle) {
    if (se >= 0x77) {
        PRINTF("NA : Na_SeLevelCheckで、seの値(%d)が、%dより大きいです。\n");
        return -1;
    }
    if (subHandle >= 0xA) {
        PRINTF("NA : Na_SeLevelCheckで、sub_hanldeの値(%d)が、%dより大きいです。\n");
        return -1;
    }
    if (gNaSeHandles[se][subHandle] == NULL) {
        return 0;
    }
    return 1;
}

// INVENTED name (no string). Stops slot gNaSeHandles[se][subHandle] and frees it; se 0x32 takes se 0x1C's first
// slot with it.
void Na_SeLevelStop(s32 se, s16 subHandle) {
    if (se >= 0x77) {
        return;
    }
    Nai_SeStop(gNaSeHandles[se][subHandle]);
    gNaSeHandles[se][subHandle] = NULL;
    if (se == 0x32) {
        Na_SeLevelStop(0x1C, 0);
    }
}

// INVENTED name (no string). Forgets every registered SE without stopping it: the SE twin of Na_AllBgmClear.
// Load-bearing: the `u8` counters.
void Na_AllSeClear(void) {
    u8 se;
    u8 subHandle;

    for (se = 0; se < 0x77; se++) {
        for (subHandle = 0; subHandle < 0xA; subHandle++) {
            gNaSeHandles[se][subHandle] = NULL;
        }
    }
}

// INVENTED names (no string). The pause jingles of Na_PauseOn and Na_PauseOff, fired through Nai_SeTrig, which
// keeps no handle.
void Na_PauseOnJingle(void) {
    Nai_SeTrig(0x54, 0, 2, 0x3F, 1.0f);
}

void Na_PauseOffJingle(void) {
    Nai_SeTrig(0x60, 0, 2, 0x3F, 1.0f);
}

// INVENTED name, after its stripped "pan = %d volume = %f" print. Maps posX onto pan 0..0x7F, pinned at the edges;
// a coordinate outside 0..0x7F divides the volume by 1 + its distance / 40 (posX) or / 20 (posY).
void Na_SeCalcPanVolume(SoundEventParams* params, s16* pan, f32* volume) {
    if (params == NULL) {
        return;
    }

    if (params->posX < 0) {
        *pan = 0;
        *volume = *volume / ((f32)-params->posX / 40.0 + 1.0);
    } else if (params->posX >= 0x80) {
        *pan = 0x7F;
        *volume = *volume / ((f32)(params->posX - 0x7F) / 40.0 + 1.0);
    } else {
        *pan = params->posX;
    }

    if (params->posY < 0) {
        *volume = *volume / ((f32)-params->posY / 20.0 + 1.0);
    } else if (params->posY >= 0x80) {
        *volume = *volume / ((f32)(params->posY - 0x7F) / 20.0 + 1.0);
    }
    PRINTF("NA : pan = %d volume = %f\n");
}

// Resets the NAI SE buffer (Nai_SeInit) and empties the registry.
void Na_AllSeStop(void) {
    PRINTF("NA : Na_AllSeStop\n");
    Nai_SeInit();
    Na_AllSeClear();
}

// Stops every registered SE slot by slot, except se 0x15.
void Na_LevelSeStop(void) {
    s32 se;
    s32 subHandle;

    PRINTF("NA : Na_LevelSeStop\n");
    for (se = 0; se < 0x77; se++) {
        if (se == 0x15) {
            continue;
        }
        for (subHandle = 0; subHandle < 0xA; subHandle++) {
            Na_SeLevelStop(se, subHandle);
        }
    }
}

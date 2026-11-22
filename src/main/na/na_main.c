#include "global.h"
#include "audio.h"

// The NA sound library's game-facing front end: boot, the per-frame tick, the pause and the output mode.
// The PRINTFs are stripped debug prints whose strings stay in .rodata.

// gNaBgmInterrupted belongs to this file's 16-byte .data block, although only na_bgm.c writes it.
u8 gNaPause = 0;
u8 gNaBgmInterrupted = 0;
s16 gNaLevelSeStopTimer = 0;
s16 gNaSe76PosX = -0x100;

// Boots the driver (AudioLoad_Init) and the NAI layer (Nai_Init), then empties the BGM handles and the SE registry.
void Na_Initialize(void) {
    PRINTF("NA : ***** Na_Initialize Start *****\n");
    AudioLoad_Init();
    Nai_Init();
    Na_AllBgmClear();
    Na_AllSeClear();
    PRINTF("NA : ***** Na_Initialize End   *****\n");
}

// The per-frame update: ticks the library, stops the level SEs when gNaLevelSeStopTimer reaches 0, and starts or
// moves SE 0x76 to the gNaSe76PosX claimed this frame, or fades it out over 0x3C frames when nobody claimed it.
// Load-bearing: the dead `params.posX` store in the fade-out arm.
void Na_GameFrameWork(void) {
    SoundEventParams params;

    PRINTF("NA : ***** Na_GameFrameWork Start *****\n");
    Nai_Routin();

    if (gNaLevelSeStopTimer >= 0) {
        if (gNaLevelSeStopTimer == 0) {
            Na_LevelSeStop();
        }
        gNaLevelSeStopTimer--;
    }

    if (gNaSe76PosX != -0x100) {
        params.posX = gNaSe76PosX;
        if (Na_SeLevelCheck(0x76, 0) != 1) {
            Na_SeStart(0x76, 0, &params);
        } else {
            Na_SeLevelChange(0x76, 0, &params);
        }
        gNaSe76PosX = -0x100;
    } else if (Na_SeLevelCheck(0x76, 0) == 1) {
        params.posX = gNaSe76PosX;
        Nai_SeSetVolume(gNaSeHandles[0x76][0], 0.0f, 0x3C);
    }

    PRINTF("NA : ***** Na_GameFrameWork End   *****\n\n");
}

// Enters the pause: freezes the live SEs, plays the 0x54 jingle and halves the BGM volume; while gNaPause is set,
// Na_SeLevelChange does nothing.
void Na_PauseOn(void) {
    PRINTF("NA : Na_PauseOn\n");
    if (gNaPause == 0) {
        gNaPause = 1;
        Nai_SePause(1);
        Na_PauseOnJingle();
        Na_BgmVolumeUpdate();
    }
}

// Leaves the pause: resumes the SEs, plays the 0x60 jingle and restores the BGM volume.
void Na_PauseOff(void) {
    PRINTF("NA : Na_PauseOff\n");
    if (gNaPause != 0) {
        gNaPause = 0;
        Nai_SePause(0);
        Na_PauseOffJingle();
        Na_BgmVolumeUpdate();
    }
}

// original name: Na_SetSoundOutputMode (OoT's Audio_SetSoundOutputMode, via its annotation; no YS string quotes it)
// Posts the options menu's sound setting as the driver's output mode (Nai_OutMode): OoT's function without the
// SURROUND case, which the YS menu does not offer.
void Na_SetSoundOutputMode(s32 soundSetting) {
    switch (soundSetting) {
        case SOUND_SETTING_STEREO:
            Nai_OutMode(SOUND_OUTPUT_STEREO);
            break;
        case SOUND_SETTING_MONO:
            Nai_OutMode(SOUND_OUTPUT_MONO);
            break;
        case SOUND_SETTING_HEADSET:
            Nai_OutMode(SOUND_OUTPUT_HEADSET);
            break;
    }
}

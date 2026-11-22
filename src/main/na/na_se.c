#include "global.h"
#include "audio.h"

// The SE front end: Na_SeTrig fires one-shot sound events, Na_SeStart and Na_SeLevelChange start and re-level the
// sounds kept in gNaSeHandles.
// The PRINTFs are stripped debug prints whose strings stay in .rodata.

u16 gNaBreathing;

// INVENTED name (no string). This file's own fabs, byte-identical to Nai_Fabs and Nai_SeFabs.
f32 Na_Fabs(f32 value) {
    if (value < 0.0f) {
        return -value;
    }
    return value;
}

// Fires a game sound event through Nai_SeTrig as the sound, group and priority its case picks (-1: none); `detune`
// adds a random wobble, `colorPitch` the Chibi Yoshi/colour pitch bender, `skipPos` skips position and echo.
// Load-bearing: the declaration list, the function-scope `sToggle` statics, the one-line case bodies in their
// order, `1.10` in cases 0x01D and 0x0A8 beside `1.1` elsewhere, and the trailing `default: break;`.
void Na_SeTrig(s32 sound, SoundEventParams* params) {
    s16 sndId = -1;
    s16 group;
    s16 priority;
    s16 pan = 0x3F;
    f32 volume = 1.0f;
    f32 pitch = 1.0f;
    UNUSED s16 value;
    s16 detune = 0;
    s16 colorPitch = 0;
    UNUSED s16 posPitch;
    s16 skipPos = 0;
    s32 i;
    UNUSED s32 unk;
    static s8 sToggle081 = 0;
    static s8 sToggle082 = 0;
    static u8 sToggle0A5 = 0;
    static s8 sToggle0AF = 0;
    static u8 sToggle285 = 1;

    PRINTF("NA : Na_SeTrig %d\n");

    // clang-format off
    switch (sound) {
        case 0x000:
        case 0x014:
        case 0x01F:
        case 0x0BC:
        case 0x20C:
        case 0x20F:
        case 0x210:
        case 0x231:
        case 0x236:
        case 0x237:
        case 0x238:
        case 0x239:
        case 0x23A:
        case 0x244:
        case 0x245:
        case 0x247:
        case 0x24D:
        case 0x24F:
        case 0x250:
        case 0x295:
            return;
        case 0x001:
            switch (params->ground) {
                case 0:
                    PRINTF("さうんど「わけわからん地面の上を歩いています。」\n");
                    return;
                case 1:
                case 12:
                default: PRINTF("NA : 土の上\n"); sndId = 0; break;
                case 2:  PRINTF("NA : 草の上\n"); sndId = 0x1; break;
                case 3:  PRINTF("NA : ロープの上\n"); sndId = 0x2; break;
                case 4:  PRINTF("NA : 水の上\n"); sndId = 0x3; break;
                case 5:  PRINTF("NA : ゼリーの上\n"); sndId = 0x7A; break;
                case 6:  PRINTF("NA : 骨の上\n"); sndId = 0xBB; break;
                case 7:  PRINTF("NA : 雪の上\n"); sndId = 0xD8; break;
                case 8:  PRINTF("NA : 石の上\n"); sndId = 0xDB; break;
                case 10: PRINTF("NA : 布の上\n"); volume *= 0.8; sndId = 0; break;
                case 11: PRINTF("NA : 木の上\n"); sndId = 0x1F7; break;
                case 9:  PRINTF("NA : ジュゲム雲の上\n"); sndId = (gAudioCtx.audioRandom & 1) ? 0x14A : 0x14B; break;
            }
            group = 5; priority = 5; detune = 1;
            break;
        case 0x002:
            switch (params->ground) {
                case 0:
                    PRINTF("さうんど「わけわからん地面の上を歩いています。」\n");
                    return;
                case 1:
                case 12: sndId = 0x8; break;
                case 2:  sndId = 0x9; break;
                case 3:  sndId = 0xA; break;
                case 4:  sndId = 0x61; break;
                case 5:  sndId = 0x7B; break;
                case 6:  sndId = 0xBC; break;
                case 7:  sndId = 0xD9; break;
                case 8:  sndId = 0xDC; break;
                case 10: volume *= 0.8; sndId = 0x8; break;
                case 11: sndId = 0x1F8; break;
                case 9:
                default: break;
            }
            group = 5; priority = 3; detune = 1;
            break;
        case 0x003:
            switch (params->ground) {
                case 0:
                    PRINTF("さうんど「わけわからん地面の上を歩いています。」\n");
                    return;
                case 1:
                case 12: sndId = 0x4; break;
                case 2:  sndId = 0x6; break;
                case 3:  sndId = 0x7; break;
                case 4:  sndId = 0x3; break;
                case 5:  sndId = 0x7A; break;
                case 6:  sndId = 0x4; break;
                case 7:  sndId = 0xDA; break;
                case 8:  sndId = 0xDD; break;
                case 10: volume *= 0.8; sndId = 0x4; break;
                case 11: sndId = 0x4; break;
                case 9:
                default: break;
            }
            group = 5; priority = 5; detune = 1;
            break;
        case 0x004:
            if ((gNaBgm != NULL && gNaBgm->num == 0xC) || (gNaSubGameBgm != NULL && gNaSubGameBgm->num == 0xC)) {
                group = -1; priority = 2; sndId = 0x99; skipPos = 1;
            } else {
                switch (gAudioCtx.audioRandom % 3) {
                    case 0: sndId = 0x177; break;
                    case 1: sndId = 0x178; break;
                    case 2: sndId = 0x179; break;
                }
                group = 4; priority = 3;
            }
            colorPitch = 1; detune = 1;
            break;
        case 0x005:
            group = 4; priority = 3; sndId = 0x1B0; colorPitch = 1; detune = 1;
            break;
        case 0x006:
            group = 4; priority = 3; sndId = 0xC; colorPitch = 1;
            break;
        case 0x007:
            if (gNaPageEnd != 0) {
                return;
            }
            switch (params->unkC) {
                case 0: sndId = 0x24; break;
                case 1: sndId = 0x25; break;
                case 2: sndId = 0x26; break;
            }
            group = 4; priority = 3; colorPitch = 1;
            break;
        case 0x008:
            if (gNaPageEnd != 0) {
                return;
            }
            switch (params->unkC) {
                case 0: sndId = 0x27; break;
                case 1: sndId = 0x28; break;
                case 2: sndId = 0x29; break;
            }
            group = 4; priority = 3; colorPitch = 1;
            break;
        case 0x009:
            group = 4; priority = 3; sndId = 0x2C; colorPitch = 1; detune = 1;
            break;
        case 0x00A:
            if (gNaPageEnd != 0) {
                return;
            }
            group = 4; priority = 3; sndId = 0x2A; colorPitch = 1; detune = 1;
            break;
        case 0x00B:
            if (gNaPageEnd != 0) {
                return;
            }
            group = 4; priority = 3; sndId = 0x2B; colorPitch = 1; detune = 1;
            break;
        case 0x00C:
            sndId = (gAudioCtx.audioRandom & 1) ? 0x17C : 0x17D;
            group = 4; priority = 3; colorPitch = 1; detune = 1;
            break;
        case 0x00D:
            group = -1; priority = 2; sndId = 0x207;
            break;
        case 0x00E:
            group = 4; priority = 3; sndId = 0x2D; colorPitch = 1;
            break;
        case 0x00F:
            switch (gNaBreathing) {
                case 1: sndId = (gAudioCtx.audioRandom & 1) ? 0x189 : 0x18D; break;
                case 0: sndId = 0x18A; break;
                case 2: sndId = 0x18E; break;
                case 3: sndId = 0x18B; break;
                case 8:
                case 9: sndId = 0x18C; break;
            }
            group = 4; priority = 3; colorPitch = 1;
            break;
        case 0x010:
            group = 4; priority = 3; sndId = 0x20; colorPitch = 1;
            break;
        case 0x011:
            group = 4; priority = 3; sndId = 0x1E8; colorPitch = 1; detune = 1;
            break;
        case 0x012:
            switch (gNaYoshiStatus) {
                case 3: sndId = 0x17F; break;
                case 0:
                case 2: sndId = 0x14; break;
                case 1: sndId = 0x17E; break;
            }
            group = 4; priority = 3; colorPitch = 1;
            break;
        case 0x013:
            group = 4; priority = 3; sndId = 0xE; detune = 1; colorPitch = 1;
            break;
        case 0x015:
            group = -1; priority = 3; sndId = 0x1C;
            break;
        case 0x016:
            group = 4; priority = 3; sndId = 0x11; detune = 1;
            break;
        case 0x017:
            group = 4; priority = 3; sndId = 0x12; detune = 1;
            break;
        case 0x018:
            group = 4; priority = 3; sndId = 0x19;
            break;
        case 0x019:
            group = 4; priority = 3; sndId = 0x18; colorPitch = 1; detune = 1;
            break;
        case 0x01A:
            group = 4; priority = 3; sndId = 0x177; colorPitch = 1; detune = 1;
            break;
        case 0x01B:
            group = 5; priority = 3; sndId = 0x1B;
            break;
        case 0x01C:
            if (Na_SeLevelCheck(0x1D, 0) != 1) {
                Na_SeTrig(0xD0, params);
                return;
            }
            group = -1; priority = 6; sndId = 0x31; detune = 1;
            break;
        case 0x01D:
            if (params != NULL) {
                for (i = 1; i < params->value; i++) {
                    pitch *= 1.10;
                }
                sndId = 0x24A;
            }
            group = 8; priority = 6;
            break;
        case 0x01E:
            switch (params->value) {
                case 1: sndId = 0x50; break;
                case 2: sndId = 0x51; break;
                case 3: sndId = 0x52; break;
                case 4:
                default: sndId = 0x53; break;
            }
            group = 8; priority = 4;
            break;
        case 0x020:
            group = -1; priority = 4; sndId = 0x3C; detune = 1;
            break;
        case 0x021:
            group = -1; priority = 4; sndId = 0x15B; detune = 1;
            break;
        case 0x022:
            group = -1; priority = 4; sndId = 0x47; detune = 1;
            break;
        case 0x023:
            group = -1; priority = 4; sndId = 0x3E; detune = 1;
            break;
        case 0x024:
            if (params != NULL) {
                pitch += params->value * 0.1;
            }
            group = -1; priority = 4; sndId = 0x1D;
            break;
        case 0x025:
            group = -1; priority = 4; sndId = 0x40; detune = 1;
            break;
        case 0x026:
            group = -1; priority = 6; sndId = 0x3F; detune = 1;
            break;
        case 0x027:
            volume *= 0.8;
            group = -1; priority = 4; sndId = 0x3D; detune = 1;
            break;
        case 0x028:
            group = 1; priority = 2; sndId = 0x2F;
            break;
        case 0x029:
            group = 5; priority = 3; sndId = 0x23;
            break;
        case 0x02A:
            group = -1; priority = 3; sndId = 0xEF; detune = 1;
            break;
        case 0x02B:
            group = 5; priority = 4; sndId = 0x5C; detune = 1;
            break;
        case 0x02C:
            group = 5; priority = 3; sndId = 0xD; detune = 1;
            break;
        case 0x02D:
            if (Na_SeLevelCheck(0x1D, 0) == 1) {
                pitch *= 0.85;
            }
            group = -1; priority = 4; sndId = 0x62; detune = 1;
            break;
        case 0x02E:
            if (Na_SeLevelCheck(0x1D, 0) == 1) {
                pitch *= 0.85;
            }
            group = -1; priority = 4; sndId = 0x63; detune = 1;
            break;
        case 0x02F:
            group = -1; priority = 4; sndId = 0x5D; detune = 1;
            break;
        case 0x030:
            group = -1; priority = 4; sndId = 0x5E; detune = 1;
            break;
        case 0x031:
            if (params != NULL) {
                pitch += (params->value - 0x40) / 512.0;
                volume *= params->value / 256.0 + 0.5;
            }
            group = -1; priority = 3; sndId = 0x6A;
            break;
        case 0x032:
            group = -1; priority = 4; sndId = 0x6B; detune = 1;
            break;
        case 0x033:
            group = -1; priority = 6; sndId = 0x6F; detune = 1;
            break;
        case 0x034:
            group = -1; priority = 4; sndId = 0x75; detune = 1;
            break;
        case 0x035:
            group = -1; priority = 2; sndId = 0xC7;
            break;
        case 0x036:
            group = -1; priority = 4; sndId = 0x3E; detune = 1;
            break;
        case 0x037:
            group = -1; priority = 4; sndId = 0x86; detune = 1;
            break;
        case 0x038:
            group = -1; priority = 4; sndId = 0x87;
            break;
        case 0x039:
            group = -1; priority = 4; sndId = 0x88;
            break;
        case 0x03A:
            group = -1; priority = 4; sndId = 0x89;
            break;
        case 0x03B:
            group = 5; priority = 3; sndId = 0x43;
            break;
        case 0x03C:
            group = 5; priority = 3; sndId = 0x45; detune = 1;
            break;
        case 0x03D:
            if (Na_SeLevelCheck(0x1D, 0) == 1) {
                volume *= 0.5;
            }
            group = -1; priority = 6; sndId = 0x91; detune = 1;
            break;
        case 0x03E:
            group = -1; priority = 2; sndId = 0x92;
            break;
        case 0x03F:
            group = -1; priority = 4; sndId = 0x98; detune = 1;
            break;
        case 0x040:
            group = -1; priority = 4; sndId = 0x9C; detune = 1;
            break;
        case 0x041:
            group = -1; priority = 6; sndId = 0xA0; detune = 1;
            break;
        case 0x042:
            group = 4; priority = 3; sndId = 0x4D; colorPitch = 1;
            break;
        case 0x043:
            group = -1; priority = 4; sndId = 0x9D; detune = 1;
            break;
        case 0x044:
            group = -1; priority = 2; sndId = 0xA1;
            break;
        case 0x045:
            group = 6; priority = 3; sndId = 0xA2; detune = 1;
            break;
        case 0x046:
            group = -1; priority = 4; sndId = 0xA3; detune = 1;
            break;
        case 0x047:
            group = -1; priority = 4; sndId = 0xB1; detune = 1;
            break;
        case 0x048:
            group = -1; priority = 6; sndId = 0xB3; detune = 1;
            break;
        case 0x049:
            group = -1; priority = 4; sndId = 0xB4; detune = 1;
            break;
        case 0x04A:
            group = -1; priority = 6; sndId = 0x3F; detune = 1;
            break;
        case 0x04B:
            group = -1; priority = 4; sndId = 0xB5; detune = 1;
            break;
        case 0x04C:
            group = -1; priority = 4; sndId = 0xB6; detune = 1;
            break;
        case 0x04D:
            group = -1; priority = 6; sndId = 0xB7; detune = 1;
            break;
        case 0x04E:
            group = -1; priority = 6; sndId = 0xBA; detune = 1;
            break;
        case 0x04F:
            group = -1; priority = 4; sndId = 0xBD; detune = 1;
            break;
        case 0x050:
            if (gNaPageEnd != 0) {
                return;
            }
            group = -1; priority = 6; sndId = 0xBE; detune = 1;
            break;
        case 0x051:
            if (gNaPageEnd != 0) {
                return;
            }
            group = -1; priority = 4; sndId = 0xC0;
            break;
        case 0x052:
            group = -1; priority = 3; sndId = 0xC1; detune = 1;
            break;
        case 0x053:
            group = -1; priority = 2; sndId = 0xC6; skipPos = 1;
            break;
        case 0x054:
            sndId = 0x147; group = 0; priority = 2; skipPos = 1;
            break;
        case 0x055:
            group = -1; priority = 4; sndId = 0xC8; detune = 1;
            break;
        case 0x056:
            if (params != NULL) {
                gNaBreathing = params->value;
                switch (params->value) {
                    case -1: sndId = 0x260; break;
                    default: PRINTF("さうんど「わけわからん呼吸法ですね。」\n"); sndId = 0x207; break;
                    case 1:  sndId = 0x1D3; break;
                    case 2:  sndId = 0x1D4; break;
                    case 3:  sndId = 0x1D5; break;
                    case 8:  sndId = 0x1D6; break;
                    case 9:  sndId = 0x1D7; break;
                }
            }
            group = -1; priority = 2;
            Na_SeTrig(0xD5, params);
            break;
        case 0x057:
            group = 0x14; priority = 4; sndId = 0xDE;
            break;
        case 0x058:
            group = 0x14; priority = 4; sndId = 0xDF;
            break;
        case 0x059:
            group = -1; priority = 4; sndId = 0xEC; detune = 1;
            break;
        case 0x05A:
            group = -1; priority = 4; sndId = 0xED; detune = 1;
            break;
        case 0x05B:
            group = 5; priority = 3; sndId = 0xEE; detune = 1;
            break;
        case 0x05C:
            if (params != NULL) {
                switch (params->value) {
                    case 1: sndId = 0x14E; break;
                    case 2: sndId = 0xEF; break;
                    case 3: sndId = 0x151; break;
                }
            }
            group = -1; priority = 3; detune = 1;
            break;
        case 0x05D:
            group = -1; priority = 2; sndId = 0xF0; skipPos = 1;
            break;
        case 0x05E:
            group = -1; priority = 4; sndId = 0xF2; detune = 1;
            break;
        case 0x05F:
            group = -1; priority = 4; sndId = 0xF4; detune = 1;
            break;
        case 0x060:
            if (Na_SeLevelCheck(0x1D, 0) != 1) {
                return;
            }
            group = -1; priority = 6; sndId = 0xF3; detune = 1;
            break;
        case 0x061:
            group = -1; priority = 4; sndId = 0x253; detune = 1;
            break;
        case 0x062:
            group = -1; priority = 4; sndId = 0x252; detune = 1;
            break;
        case 0x063:
            group = -1; priority = 4; sndId = 0x103;
            break;
        case 0x064:
            group = -1; priority = 4; sndId = 0x104; detune = 1;
            break;
        case 0x065:
            group = -1; priority = 4; sndId = 0x105; detune = 1;
            break;
        case 0x066:
            group = 2; priority = 2; sndId = 0x106; skipPos = 1;
            break;
        case 0x067:
            group = -1; priority = 4; sndId = 0x12E; detune = 1;
            break;
        case 0x068:
            group = 4; priority = 3; sndId = 0x17B; colorPitch = 1;
            break;
        case 0x069:
            group = -1; priority = 4; sndId = 0x130; detune = 1;
            break;
        case 0x06A:
            group = -1; priority = 4; sndId = 0x131; detune = 1;
            break;
        case 0x06B:
            group = -1; priority = 4; sndId = 0x132; detune = 1;
            break;
        case 0x06C:
            group = -1; priority = 4; sndId = 0x40; detune = 1;
            break;
        case 0x06D:
            group = -1; priority = 4; sndId = 0x135; detune = 1;
            break;
        case 0x06E:
            group = -1; priority = 4; sndId = 0x134; detune = 1;
            break;
        case 0x06F:
            group = -1; priority = 4; sndId = 0x133; detune = 1;
            break;
        case 0x070:
            sndId = 0x137; group = 0; priority = 2; skipPos = 1;
            break;
        case 0x071:
            group = -1; priority = 4; sndId = 0x140; detune = 1;
            break;
        case 0x072:
            group = -1; priority = 4; sndId = 0x141; detune = 1;
            break;
        case 0x073:
            group = 5; priority = 4; sndId = 0x144; detune = 1;
            break;
        case 0x074:
            group = -1; priority = 4; sndId = 0x155; detune = 1;
            break;
        case 0x075:
            group = -1; priority = 4; sndId = 0x145;
            break;
        case 0x076:
            if (gNaPageEnd != 0) {
                return;
            }
            switch (gNaYoshiStatus) {
                case 1:
                case 3: sndId = (gAudioCtx.audioRandom & 1) ? 0x14A : 0x14B; break;
                case 0: sndId = (gAudioCtx.audioRandom & 1) ? 0x14F : 0x150; break;
                case 2: sndId = (gAudioCtx.audioRandom & 1) ? 0x180 : 0x183; break;
            }
            group = 4; priority = 3; colorPitch = 1;
            break;
        case 0x077:
            group = -1; priority = 4; sndId = 0x14D;
            break;
        case 0x078:
            group = -1; priority = 2; sndId = 0x14C; colorPitch = 1;
            break;
        case 0x079:
            group = 4; priority = 3; sndId = 0x152; colorPitch = 1; skipPos = 1;
            break;
        case 0x07A:
            group = 4; priority = 3; sndId = 0x163; colorPitch = 1;
            break;
        case 0x07B:
            group = 4; priority = 3; sndId = 0x1A1; colorPitch = 1;
            break;
        case 0x07C:
            group = 4; priority = 3; sndId = 0x175; colorPitch = 1;
            break;
        case 0x07D:
            group = -1; priority = 3; sndId = 0x175; colorPitch = 1;
            break;
        case 0x07E:
            group = -1; priority = 3; sndId = 0x175; colorPitch = 1;
            break;
        case 0x07F:
            group = -1; priority = 6; sndId = 0x1A7;
            break;
        case 0x080:
            group = -1; priority = 6; sndId = 0x1A8;
            break;
        case 0x081:
            if (sToggle081 != 0) {
                sToggle081 = 0;
                volume = volume * 0.94;
            } else {
                sToggle081 = 1;
            }
            sndId = (gAudioCtx.audioRandom & 1) ? 0x166 : 0x176;
            group = 4; priority = 5; colorPitch = 1; detune = 1;
            break;
        case 0x082:
            if (sToggle082 != 0) {
                sToggle082 = 0;
                volume = volume * 0.94;
            } else {
                sToggle082 = 1;
            }
            group = 4; priority = 5; sndId = 0x167; colorPitch = 1; detune = 1;
            break;
        case 0x083:
            group = 1; priority = 2; sndId = 0x2F;
            break;
        case 0x084:
            group = -1; priority = 2; sndId = 0x174;
            break;
        case 0x085:
            group = 4; priority = 3; sndId = 0x184; colorPitch = 1; detune = 1;
            break;
        case 0x086:
            group = 4; priority = 2; sndId = 0x185; skipPos = 1;
            break;
        case 0x087:
            group = -1; priority = 2; sndId = 0x1AD; skipPos = 1;
            break;
        case 0x088:
            group = 4; priority = 3; sndId = 0x27;
            break;
        case 0x089:
            group = 4; priority = 3; sndId = 0x186;
            break;
        case 0x08A:
            group = 4; priority = 3; sndId = 0x187;
            break;
        case 0x08B:
            group = -1; priority = 4; sndId = 0x181;
            break;
        case 0x08C:
            pitch *= 0.7;
            group = -1; priority = 4; sndId = 0x181;
            break;
        case 0x08D:
            group = 5; priority = 4; sndId = 0xC8; detune = 1;
            break;
        case 0x08E:
            sndId = 0x2B; group = 0; priority = 2; detune = 1; skipPos = 1;
            break;
        case 0x08F:
            group = 4; priority = 3; sndId = 0x182; skipPos = 1;
            break;
        case 0x090:
            group = -1; priority = 4; sndId = 0x144; detune = 1;
            break;
        case 0x091:
            group = -1; priority = 4; sndId = 0x144; detune = 1;
            break;
        case 0x092:
            group = -1; priority = 4; sndId = 0xB6; detune = 1;
            break;
        case 0x093:
            group = -1; priority = 4; sndId = 0x254; detune = 1;
            break;
        case 0x094:
            if (Na_SeLevelCheck(0x1D, 0) == 1) {
                volume *= 0.5;
            }
            group = -1; priority = 6; sndId = 0x1A5; detune = 1;
            break;
        case 0x095:
            group = -1; priority = 4; sndId = 0x1B2; detune = 1;
            break;
        case 0x096:
            group = -1; priority = 4; sndId = 0x1B3; detune = 1;
            break;
        case 0x097:
            group = -1; priority = 4; sndId = 0x287;
            Na_SeTrig(0xD6, params);
            break;
        case 0x098:
            group = -1; priority = 4; sndId = 0x286;
            Na_SeTrig(0xD6, params);
            break;
        case 0x099:
            group = -1; priority = 4; sndId = 0x284;
            Na_SeTrig(0xD6, params);
            break;
        case 0x09A:
            group = -1; priority = 4; sndId = 0x288;
            Na_SeTrig(0xD6, params);
            break;
        case 0x09B:
            group = -1; priority = 4; sndId = 0x285;
            Na_SeTrig(0xD6, params);
            break;
        case 0x09C:
            group = -1; priority = 4; sndId = 0x289;
            Na_SeTrig(0xD6, params);
            break;
        case 0x09D:
            group = -1; priority = 6; sndId = 0x1B5; detune = 1;
            break;
        case 0x09E:
            group = -1; priority = 4; sndId = 0x1B6; detune = 1;
            break;
        case 0x09F:
            group = -1; priority = 6; sndId = 0x1B7; detune = 1;
            break;
        case 0x0A0:
            group = -1; priority = 2; sndId = 0x142; skipPos = 1;
            break;
        case 0x0A1:
            if (gNaBgm != NULL) {
                gNaBgm->alongside = 1;
            }
            Na_SubGameBgmStart(0x40, 0);
            return;
        case 0x0A2:
            group = -1; priority = 2; sndId = 0x23C;
            break;
        case 0x0A3:
            group = -1; priority = 2; sndId = 0x23C;
            break;
        case 0x0A4:
            group = -1; priority = 2; sndId = 0x23C;
            break;
        case 0x0A5:
            if (sToggle0A5 != 0) {
                sToggle0A5 = 0;
                sndId = 0x14A;
            } else {
                sToggle0A5 = 1;
                sndId = 0x14B;
            }
            group = 4; priority = 3; colorPitch = 1; detune = 1;
            break;
        case 0x0A6:
            group = 4; priority = 3; sndId = 0x1A1; colorPitch = 1;
            break;
        case 0x0A7:
            group = -1; priority = 2; sndId = 0xEF; skipPos = 1;
            break;
        case 0x0A8:
            if (params != NULL) {
                for (i = 1; i < params->value; i++) {
                    pitch *= 1.10;
                }
                sndId = 0x90;
            }
            group = -1; priority = 2; skipPos = 1;
            break;
        case 0x0A9:
            group = -1; priority = 2; sndId = 0x1DF;
            break;
        case 0x0AA:
            group = -1; priority = 4; sndId = 0x1E0; detune = 1;
            break;
        case 0x0AB:
            group = -1; priority = 4; sndId = 0x1E1; detune = 1;
            break;
        case 0x0AC:
            group = -1; priority = 4; sndId = 0x1E2; detune = 1;
            break;
        case 0x0AD:
            group = -1; priority = 4; sndId = 0x1E3;
            break;
        case 0x0AE:
            group = -1; priority = 4; sndId = 0x3D; detune = 1;
            break;
        case 0x0AF:
            if (sToggle0AF != 0) {
                sToggle0AF = 0;
                volume = volume * 0.85;
            } else {
                sToggle0AF = 1;
            }
            group = 4; priority = 3; sndId = 0x156; colorPitch = 1; detune = 1;
            break;
        case 0x0B0:
            if (gNaPageEnd != 0) {
                return;
            }
            group = -1; priority = 4; sndId = 0x1FB; detune = 1;
            break;
        case 0x0B1:
            group = -1; priority = 4; sndId = 0x1FC; detune = 1;
            break;
        case 0x0B2:
            if (gNaBgm != NULL) {
                if (gNaBgm->num == 0x25) {
                    sndId = (gAudioCtx.audioRandom & 1) ? 0x251 : 0x28C;
                } else {
                    sndId = (gAudioCtx.audioRandom & 1) ? 0x28D : 0x28E;
                }
            }
            group = 0x16; priority = 4; detune = 1;
            break;
        case 0x0B3:
            group = -1; priority = 4; sndId = 0x1FF; detune = 1;
            break;
        case 0x0B4:
            group = 0x16; priority = 4; sndId = 0x23B; detune = 1;
            if (params != NULL && params->value == 3 && gNaBgm != NULL) {
                if (gNaBgm->num == 0x35) {
                    Na_BgmStop(0); gNaPageEnd = 1;
                } else {
                    sndId = 0x290;
                }
            }
            break;
        case 0x0B5:
            group = 0x16; priority = 4; sndId = 0x200;
            break;
        case 0x0B6:
            group = -1; priority = 4; sndId = 0x202; detune = 1;
            break;
        case 0x0B7:
            if (gNaPageEnd != 0) {
                return;
            }
            group = -1; priority = 4; sndId = 0x1FE; detune = 1;
            break;
        case 0x0B8:
            group = -1; priority = 4; sndId = 0x1FE; detune = 1;
            break;
        case 0x0B9:
            group = -1; priority = 4; sndId = 0x1FD; detune = 1;
            break;
        case 0x0BA:
            group = -1; priority = 4; sndId = 0x208; detune = 1;
            break;
        case 0x0BB:
            sndId = 0x80; group = 0; priority = 2;
            break;
        case 0x0BD:
            if (params != NULL) {
                volume *= params->value / 128.0;
            }
            group = 0xE; priority = 4; sndId = 0x213;
            break;
        case 0x0BE:
            sndId = 0x24E; group = 0; priority = 2; skipPos = 1;
            break;
        case 0x0BF:
            sndId = 0x24F; group = 0; priority = 2; skipPos = 1;
            break;
        case 0x0C0:
            group = -1; priority = 4; sndId = 0x215;
            break;
        case 0x0C1:
            group = -1; priority = 4; sndId = 0x20B; detune = 1;
            break;
        case 0x0C2:
            sndId = 0x123; group = 0; priority = 2; skipPos = 1;
            break;
        case 0x0C3:
            group = -1; priority = 4; sndId = 0x239;
            break;
        case 0x0C4:
            sndId = 0x24C; group = 0; priority = 2; skipPos = 1;
            break;
        case 0x0C5:
            sndId = 0x23F; group = 0; priority = 2; skipPos = 1;
            break;
        case 0x0C6:
            group = -1; priority = 2; sndId = 0x243; skipPos = 1;
            break;
        case 0x0C7:
            volume *= 0.7;
            group = -1; priority = 4; sndId = 0xC0; detune = 1;
            break;
        case 0x0C8:
            group = -1; priority = 4; sndId = 0x144; detune = 1;
            break;
        case 0x0C9:
            group = -1; priority = 4; sndId = 0x1FC; detune = 1;
            break;
        case 0x0CA:
            group = -1; priority = 2; sndId = 0x24B; colorPitch = 1;
            break;
        case 0x0CB:
            group = -1; priority = 1; sndId = 0x18C; skipPos = 1;
            break;
        case 0x0CC:
            group = -1; priority = 4; sndId = 0xEC; detune = 1;
            break;
        case 0x0CD:
            sndId = 0x257; group = 0; priority = 2; skipPos = 1;
            break;
        case 0x0CE:
            sndId = 0x258; group = 0; priority = 2; skipPos = 1;
            break;
        case 0x0CF:
            group = -1; priority = 4; sndId = 0x28B; skipPos = 1;
            break;
        case 0x0D0:
            volume *= 0.7;
            group = -1; priority = 3; sndId = 0x1FC; detune = 1;
            break;
        case 0x0D1:
            group = -1; priority = 4; sndId = 0x255; detune = 1;
            break;
        case 0x0D2:
            if (params != NULL) {
                volume = params->value / 128.0;
            }
            group = -1; priority = 6; sndId = 0x25F; detune = 1; skipPos = 1;
            break;
        case 0x0D3:
            if (params != NULL) {
                volume = params->value / 128.0;
            }
            group = -1; priority = 6; sndId = 0x25E; detune = 1; skipPos = 1;
            break;
        case 0x0D4:
            if (params != NULL) {
                gNaBreathing = params->value;
                switch (params->value) {
                    case -1: sndId = 0x260; break;
                    case 1:  sndId = 0x1D3; break;
                    case 2:  sndId = 0x1D4; break;
                    case 3:  sndId = 0x1D5; break;
                    case 8:  sndId = 0x1D6; break;
                    case 9:  sndId = 0x1D7; break;
                    default: PRINTF("さうんど「わけわからん呼吸法ですね。」\n"); return;
                }
            }
            group = -1; priority = 2;
            break;
        case 0x0D5:
            if (params != NULL) {
                PRINTF("NA : おいしさ %d\n");
                switch (params->value2) {
                    case 2:  sndId = 0x267; break;
                    case 3:  sndId = 0x268; break;
                    case 4:  sndId = 0x269; break;
                    case 5:  sndId = 0x26A; break;
                    case 6:  sndId = 0x26B; break;
                    case 7:  sndId = 0x26C; break;
                    case 8:  sndId = 0x26D; break;
                    case 9:  sndId = 0x26E; break;
                    case 10: sndId = 0x26F; break;
                    case 11: sndId = 0x270; break;
                    case 12: sndId = 0x271; break;
                    case 13: sndId = 0x272; break;
                    case 14: sndId = 0x273; break;
                    case 15: sndId = 0x274; break;
                    case 16: sndId = 0x275; break;
                    case 17: sndId = 0x276; break;
                    case 18: sndId = 0x277; break;
                    case 19: sndId = 0x278; break;
                    case 20: sndId = 0x279; break;
                    case 21: sndId = 0x27A; break;
                    case 22: sndId = 0x27B; break;
                    case 23: sndId = 0x27C; break;
                    case 24: sndId = 0x27D; break;
                    case 25: sndId = 0x27E; break;
                    case 26: sndId = 0x27F; break;
                    case 27: sndId = 0x280; break;
                    case 28: sndId = 0x281; break;
                    case 29: sndId = 0x282; break;
                    case 30: sndId = 0x283; break;
                    default: return;
                }
            }
            group = -1; priority = 0;
            break;
        case 0x0D6:
            group = -1; priority = 4; sndId = 0x28A; detune = 1;
            break;
        case 0x0D7:
            group = -1; priority = 1; sndId = 0x182; skipPos = 1;
            break;
        case 0x0D8:
            group = -1; priority = 1; sndId = 0x298; skipPos = 1;
            break;
        case 0x0D9:
            sndId = 0x283; group = -1; priority = 0; skipPos = 1;
            break;
        case 0x0DA:
            group = -1; priority = 1; sndId = 0x187; skipPos = 1;
            break;
        case 0x1F4:
            group = -1; priority = 2; sndId = 0x1A3; skipPos = 1;
            break;
        case 0x1F5:
            group = -1; priority = 2; sndId = 0x1A4; skipPos = 1;
            break;
        case 0x1F6:
            group = -1; priority = 2; sndId = 0x66; skipPos = 1;
            break;
        case 0x1F7:
            group = -1; priority = 2; sndId = 0x68;
            break;
        case 0x1F8:
            group = -1; priority = 4; sndId = 0x69; detune = 1;
            break;
        case 0x1F9:
            group = 1; priority = 2; sndId = 0x30;
            break;
        case 0x1FA:
            group = -1; priority = 2; sndId = 0x1A9;
            break;
        case 0x1FB:
            Na_SetTimerCount(1, 0); group = -1; priority = 2; sndId = 0x1AA;
            break;
        case 0x1FC:
            group = -1; priority = 2; sndId = 0x7C; skipPos = 1;
            break;
        case 0x1FD:
            group = 9; priority = 4; sndId = 0x70;
            break;
        case 0x1FE:
            group = -1; priority = 4; sndId = 0x72; detune = 1;
            break;
        case 0x1FF:
            group = -1; priority = 4; sndId = 0x73; detune = 1;
            break;
        case 0x200:
            group = 0xB; priority = 4; sndId = 0x74; detune = 1;
            break;
        case 0x201:
            group = -1; priority = 2; sndId = 0x78;
            break;
        case 0x202:
            group = -1; priority = 2; sndId = 0x79; skipPos = 1;
            break;
        case 0x203:
            sndId = 0x55; group = 0; priority = 2; skipPos = 1;
            break;
        case 0x204:
            sndId = 0x4F; group = 0; priority = 2; skipPos = 1;
            break;
        case 0x205:
            sndId = 0x76; group = 0; priority = 2; skipPos = 1;
            break;
        case 0x206:
            group = -1; priority = 2; sndId = 0x77; skipPos = 1;
            break;
        case 0x207:
            sndId = 0x80; group = 0; priority = 2; skipPos = 1;
            break;
        case 0x208:
            group = 9; priority = 4; sndId = 5;
            break;
        case 0x209:
            if (params != NULL) {
                pitch += params->value / 12.0;
            }
            group = 0xA; priority = 4; sndId = 0x81;
            break;
        case 0x20A:
            group = -1; priority = 4; sndId = 0x41; detune = 1;
            break;
        case 0x20B:
            group = 0xC; priority = 4; sndId = 0x32; detune = 1;
            break;
        case 0x20D:
            group = -1; priority = 2; skipPos = 1; sndId = 0x265; Na_SeTrig(0xAC, params);
            break;
        case 0x20E:
            group = -1; priority = 6; sndId = 0x46; detune = 1;
            break;
        case 0x211:
            group = 0xC; priority = 4; gNaSe211Count++; sndId = 0x8F; detune = 1;
            if (gNaSe211Count == 3) {
                if (gNaBgm != NULL && gNaBgm->num == 0xE) {
                    Na_BgmStop(0);
                }
                gNaSe211Count = 0;
            }
            break;
        case 0x212:
            if (params != NULL) {
                for (i = 1; i < params->value; i++) {
                    pitch *= 1.1;
                }
                sndId = 0x90;
            }
            group = 1; priority = 2;
            break;
        case 0x213:
            group = -1; priority = 2; sndId = 0x148; skipPos = 1;
            break;
        case 0x214:
            group = -1; priority = 4; sndId = 0xA4; detune = 1;
            break;
        case 0x215:
            group = -1; priority = 3; sndId = 0xA6;
            break;
        case 0x216:
            group = -1; priority = 2; sndId = 0xA7;
            break;
        case 0x217:
            group = -1; priority = 2; sndId = 0xA8;
            break;
        case 0x218:
            group = -1; priority = 2; sndId = 0xA9;
            break;
        case 0x219:
            group = -1; priority = 2; sndId = 0xAA;
            break;
        case 0x21A:
            group = -1; priority = 2; sndId = 0xAB;
            break;
        case 0x21B:
            group = 1; priority = 2; sndId = 0xAC;
            break;
        case 0x21C:
            group = 1; priority = 2; sndId = 0xAD;
            break;
        case 0x21D:
            group = 1; priority = 2; sndId = 0x96;
            break;
        case 0x21E:
            group = -1; priority = 4; sndId = 0xB0; detune = 1;
            break;
        case 0x21F:
            group = 1; priority = 3; sndId = 0xB2; detune = 1;
            break;
        case 0x220:
            group = -1; priority = 4; sndId = 0xB8;
            break;
        case 0x221:
            group = -1; priority = 4; sndId = 0xB9;
            break;
        case 0x222:
            group = -1; priority = 3; sndId = 0x57; detune = 1;
            break;
        case 0x223:
            group = -1; priority = 4; sndId = 0x58; detune = 1;
            break;
        case 0x224:
            group = -1; priority = 6; sndId = 0xC2; detune = 1;
            break;
        case 0x225:
            group = -1; priority = 6; sndId = 0xC3; detune = 1;
            break;
        case 0x226:
            group = 0x14; priority = 4; sndId = 0xC4;
            break;
        case 0x227:
            sndId = 0xC9; group = 0; priority = 2; skipPos = 1;
            break;
        case 0x228:
            sndId = 0xCA; group = 0; priority = 2; skipPos = 1;
            break;
        case 0x229:
            if (params != NULL) {
                PRINTF("NA : ハート %d\n");
                switch (params->value) {
                    case 1: sndId = 0xCB; break;
                    case 2: sndId = 0xCC; break;
                    case 3: sndId = 0xCD; break;
                    case 4: sndId = 0xCE; break;
                    case 5: sndId = 0xCF; break;
                    case 6: sndId = 0xD0; break;
                }
            }
            group = 0; priority = 2; skipPos = 1;
            break;
        case 0x22A:
            if (params != NULL) {
                PRINTF("NA : ハート %d\n");
                switch (params->value) {
                    case 1: sndId = 0xD1; break;
                    case 2: sndId = 0xD2; break;
                    case 3: sndId = 0xD3; break;
                    case 4: sndId = 0xD4; break;
                    case 5: sndId = 0xD5; break;
                    case 6: sndId = 0xD6; break;
                }
            }
            group = 0; priority = 2; skipPos = 1;
            break;
        case 0x22B:
            group = -1; priority = 2; sndId = 0x1A3; skipPos = 1;
            break;
        case 0x22C:
            group = -1; priority = 2; sndId = 0x1A4; skipPos = 1;
            break;
        case 0x22D:
            if (params != NULL) {
                pitch += params->value * 0.1;
            }
            group = -1; priority = 4; sndId = 0x238;
            break;
        case 0x22E:
            if (Na_SeLevelCheck(0x1D, 0) == 1) {
                volume *= 0.5;
            }
            group = -1; priority = 4; sndId = 0xE5; detune = 1;
            break;
        case 0x22F:
            group = 5; priority = 3; sndId = 0xE6;
            break;
        case 0x230:
            if (params != NULL) {
                switch (params->value) {
                    case 1:
                    case 3:
                    case 4:
                    case 6:
                    case 7:
                    case 9:
                    case 11: sndId = 0x247; break;
                    case 2:
                    case 5:
                    case 8:
                    case 10:
                    case 12: sndId = 0x248; break;
                    case 13: sndId = 0xE8; break;
                }
            }
            group = -1; priority = 6; detune = 1;
            break;
        case 0x232:
            group = -1; priority = 4; sndId = 0x161; detune = 1;
            break;
        case 0x233:
            group = -1; priority = 4; sndId = 0xEA; detune = 1;
            break;
        case 0x234:
            group = -1; priority = 4; sndId = 0xEB; detune = 1;
            break;
        case 0x235:
            group = -1; priority = 6; sndId = 0xF5; detune = 1;
            break;
        case 0x23B:
            pitch *= 0.7;
            group = -1; priority = 6; sndId = 0x3F; detune = 1;
            break;
        case 0x23C:
            group = -1; priority = 4; sndId = 0xFC;
            break;
        case 0x23D:
            volume *= 0.7;
            group = -1; priority = 4; sndId = 0xFE; detune = 1;
            break;
        case 0x23E:
            group = -1; priority = 4; sndId = 0x160; detune = 1;
            break;
        case 0x23F:
            group = 0x14; priority = 4; sndId = 0x107;
            break;
        case 0x240:
            group = 0x15; priority = 4; sndId = 0x108;
            break;
        case 0x241:
            if (Na_SeLevelCheck(0x1D, 0) == 1) {
                volume *= 0.5;
            }
            group = -1; priority = 4; sndId = 0x109; detune = 1;
            break;
        case 0x242:
            group = 0x17; priority = 2; sndId = 0x11F; skipPos = 1;
            break;
        case 0x243:
            group = -1; priority = 2; sndId = 0x120; skipPos = 1;
            break;
        case 0x246:
            sndId = 0x123; group = 0; priority = 2; skipPos = 1;
            break;
        case 0x248:
            group = 1; priority = 2; sndId = 0x125;
            break;
        case 0x249:
            group = 1; priority = 2; sndId = 0x127;
            break;
        case 0x24A:
            if (Na_SeLevelCheck(0x1D, 0) == 1) {
                volume *= 0.5;
            }
            group = 0x10; priority = 4; sndId = 0xE4;
            break;
        case 0x24B:
            group = 0x10; priority = 4; sndId = 0x129; detune = 1;
            break;
        case 0x24C:
            group = 0x10; priority = 4; sndId = 0x12A; detune = 1;
            break;
        case 0x24E:
            group = -1; priority = 2; sndId = 0x12C; skipPos = 1;
            break;
        case 0x251:
            if (params != NULL) {
                switch (params->unk10) {
                    case 0:
                    case 2:
                    case 3:
                    case 4:
                    case 7:
                    case 8:
                    case 9:
                    case 10:
                    case 11:
                    case 12:
                    case 13: sndId = 0x142; break;
                    case 1:
                    case 5:
                    case 6:
                    case 16: sndId = 0x95; break;
                    case 14:
                    case 15: sndId = 0x143; break;
                }
            }
            group = 0x18; priority = 2; skipPos = 1;
            break;
        case 0x252:
            group = -1; priority = 2; sndId = 0x142; skipPos = 1;
            break;
        case 0x253:
            group = -1; priority = 2; sndId = 0x95; skipPos = 1;
            break;
        case 0x254:
            group = -1; priority = 2; sndId = 0x143; skipPos = 1;
            break;
        case 0x255:
            group = -1; priority = 4; sndId = 0x157;
            break;
        case 0x256:
            group = -1; priority = 4; sndId = 0x17E; detune = 1;
            break;
        case 0x257:
            group = -1; priority = 4; sndId = 0x12B; detune = 1;
            break;
        case 0x258:
            if (Na_SeLevelCheck(0x1D, 0) == 1) {
                volume *= 0.5;
            }
            group = 0x10; priority = 4; sndId = 0x15E;
            break;
        case 0x259:
            group = -1; priority = 4; sndId = 0x15F; detune = 1;
            break;
        case 0x25A:
            group = 0xC; priority = 4; sndId = 0x15C; detune = 1;
            break;
        case 0x25B:
            group = 0x12; priority = 4; sndId = 0x15D; detune = 1;
            break;
        case 0x25C:
            group = -1; priority = 4; sndId = 0xFF; detune = 1;
            break;
        case 0x25D:
            group = -1; priority = 4; sndId = 0x161; detune = 1;
            break;
        case 0x25E:
            group = -1; priority = 4; sndId = 0x162; detune = 1;
            break;
        case 0x25F:
            group = -1; priority = 2; sndId = 0x1AB; skipPos = 1;
            break;
        case 0x260:
            if (params != NULL) {
                switch (params->value) {
                    case 1: sndId = 0x16C; break;
                    case 2: sndId = 0x16D; break;
                    case 3: sndId = 0x16E; break;
                    case 4: sndId = 0x16F; break;
                    case 5: sndId = 0x170; break;
                    case 6: sndId = 0x171; break;
                    case 7: sndId = 0x172; break;
                    case 8: sndId = 0x173; break;
                }
            }
            group = -1; priority = 2; skipPos = 1;
            break;
        case 0x261:
            group = 0x17; priority = 2; sndId = 0x16B; skipPos = 1;
            break;
        case 0x262:
            group = 0x17; priority = 2; sndId = 0x169; skipPos = 1;
            break;
        case 0x263:
            group = 0x17; priority = 2; sndId = 0x1AC; skipPos = 1;
            break;
        case 0x264:
            sndId = 0x24D; group = 0; priority = 2; skipPos = 1;
            break;
        case 0x265:
            volume *= 0.8;
            sndId = 0x24D; group = 0; priority = 2; skipPos = 1;
            break;
        case 0x266:
            sndId = 0x18F; group = 0; priority = 2; skipPos = 1;
            break;
        case 0x267:
            sndId = 0x190; group = 0; priority = 2; skipPos = 1;
            break;
        case 0x268:
            sndId = 0x191; group = 0; priority = 2; skipPos = 1;
            break;
        case 0x269:
            if (params != NULL) {
                switch (params->value) {
                    case 1: sndId = 0x291; break;
                    case 2: sndId = 0x292; break;
                    case 3: sndId = 0x293; break;
                    case 4: sndId = 0x294; break;
                    case 5: sndId = 0x295; break;
                }
            }
            group = -1; priority = 3;
            break;
        case 0x26A:
            group = -1; priority = 4; sndId = 0x197; detune = 1;
            break;
        case 0x26B:
            group = -1; priority = 4; sndId = 0x198; detune = 1;
            break;
        case 0x26C:
            group = -1; priority = 4; sndId = 0x199; detune = 1;
            break;
        case 0x26D:
            group = -1; priority = 2; sndId = 0x19B; skipPos = 1;
            break;
        case 0x26E:
            group = -1; priority = 2; sndId = 0x19C; skipPos = 1;
            break;
        case 0x26F:
            group = 1; priority = 2; sndId = 0x19A; skipPos = 1;
            break;
        case 0x270:
            group = -1; priority = 4; sndId = 0x19D; detune = 1;
            break;
        case 0x271:
            if (Na_SeLevelCheck(0x1D, 0) == 1) {
                volume *= 0.5;
            }
            group = 0x13; priority = 4; sndId = 0x19F;
            break;
        case 0x272:
            group = -1; priority = 2; sndId = 0x1A0; skipPos = 1;
            break;
        case 0x273:
            group = -1; priority = 4; sndId = 0x7E; detune = 1;
            break;
        case 0x274:
            volume *= 1.8;
            group = -1; priority = 4; sndId = 0x1B8; detune = 1;
            break;
        case 0x275:
            group = 0x13; priority = 4; sndId = 0x1BA;
            break;
        case 0x276:
            group = -1; priority = 2; sndId = 0x1BB; skipPos = 1;
            break;
        case 0x277:
            group = -1; priority = 4; sndId = 0x1BC; detune = 1;
            break;
        case 0x278:
            group = -1; priority = 6; sndId = 0x37; detune = 1;
            break;
        case 0x279:
            group = -1; priority = 2; sndId = 0x36;
            break;
        case 0x27A:
            group = -1; priority = 6; sndId = 0x1B9; detune = 1;
            break;
        case 0x27B:
            group = -1; priority = 4; sndId = 0x6D;
            break;
        case 0x27C:
            group = -1; priority = 6; sndId = 0x164; detune = 1;
            break;
        case 0x27D:
            group = -1; priority = 6; sndId = 0xF; detune = 1;
            break;
        case 0x27E:
            volume *= 0.8;
            group = -1; priority = 6; sndId = 0x9A; detune = 1;
            break;
        case 0x27F:
            group = -1; priority = 4; sndId = 0x1E9;
            break;
        case 0x280:
            group = -1; priority = 2; sndId = 0x1EA;
            break;
        case 0x281:
            group = 0xB; priority = 4; sndId = 0x1EC;
            break;
        case 0x282:
            group = -1; priority = 4; sndId = 0x1ED; detune = 1;
            break;
        case 0x283:
            group = 0xD; priority = 4; sndId = 0x1F3;
            break;
        case 0x284:
            group = 0xD; priority = 4; sndId = 0x1F4;
            break;
        case 0x285:
            if (params != NULL) {
                switch (params->value) {
                    case 0:
                    case 1:
                    default:
                        sndId = 0x1F5;
                        break;
                    case 2:
                        if (sToggle285 != 0) {
                            sToggle285 = 0;
                            sndId = 0x211;
                        } else {
                            sToggle285 = 1;
                            sndId = 0x212;
                        }
                        break;
                }
            }
            group = 0xE; priority = 4;
            break;
        case 0x286:
            group = 0xE; priority = 4; sndId = 0x1F6;
            break;
        case 0x287:
            group = -1; priority = 4; sndId = 0x204; detune = 1;
            break;
        case 0x288:
            group = -1; priority = 4; sndId = 0x205; detune = 1;
            break;
        case 0x289:
            sndId = 0x206; group = 0; priority = 2; skipPos = 1;
            break;
        case 0x28A:
            group = 0xE; priority = 4; sndId = 0x203;
            break;
        case 0x28B:
            if (gNaPageEnd != 0) {
                return;
            }
            group = -1; priority = 4; sndId = 0x20B; detune = 1;
            break;
        case 0x28C:
            group = 0xD; priority = 4; sndId = 0x20C;
            break;
        case 0x28D:
            group = 0xD; priority = 4; sndId = 0x20F;
            break;
        case 0x28E:
            group = 0xC; priority = 4; sndId = 0x210;
            break;
        case 0x28F:
            group = 0x16; priority = 4; sndId = 0x20D;
            break;
        case 0x290:
            group = -1; priority = 4; sndId = 0x214;
            break;
        case 0x291:
            group = 0x16; priority = 4; sndId = 0x25B; detune = 1;
            break;
        case 0x292:
            group = -1; priority = 2; sndId = 0x259; skipPos = 1;
            break;
        case 0x293:
            group = -1; priority = 2; sndId = 0x25A; skipPos = 1;
            break;
        case 0x294:
            group = -1; priority = 2; sndId = 0x266; skipPos = 1;
            break;
        case 0x3E8:
            sndId = 0x10B; group = 0; priority = 2;
            break;
        case 0x3E9:
            sndId = 0x10C; group = 0; priority = 2;
            break;
        case 0x3EA:
            sndId = 0x10D; group = 0; priority = 2;
            break;
        case 0x3EB:
            sndId = 0x10E; group = 0; priority = 2;
            break;
        case 0x3EC:
            sndId = 0x10F; group = 0; priority = 2;
            break;
        case 0x3ED:
            sndId = 0x110; group = 0; priority = 2;
            break;
        case 0x3EE:
            sndId = 0x111; group = 0; priority = 2;
            break;
        case 0x3EF:
            sndId = 0x112; group = 0; priority = 2;
            break;
        case 0x3F0:
            sndId = 0x113; group = 0; priority = 2;
            break;
        case 0x3F1:
            sndId = 0x114; group = 0; priority = 2;
            break;
        case 0x3F2:
            sndId = 0x115; group = 0; priority = 2;
            break;
        case 0x3F3:
            sndId = 0x116; group = 0; priority = 2;
            break;
        case 0x3F4:
            sndId = 0x117; group = 0; priority = 2;
            break;
        case 0x3F5:
            sndId = 0x118; group = 0; priority = 2;
            break;
        case 0x3F6:
            sndId = 0x119; group = 0; priority = 2;
            break;
        case 0x3F7:
            sndId = 0x11A; group = 0; priority = 2;
            break;
        case 0x3F8:
            sndId = 0x11B; group = 0; priority = 2;
            break;
        case 0x3F9:
            sndId = 0x11C; group = 0; priority = 2;
            break;
        case 0x3FA:
            sndId = 0x11D; group = 0; priority = 2;
            break;
        case 0x3FB:
            sndId = 0x11E; group = 0; priority = 2;
            break;
        case 0x3FC:
            sndId = 0x1BF; group = 0; priority = 2;
            break;
        case 0x3FD:
            sndId = 0x1C0; group = 0; priority = 2;
            break;
        case 0x3FE:
            sndId = 0x1C1; group = 0; priority = 2;
            break;
        case 0x3FF:
            sndId = 0x1C2; group = 0; priority = 2;
            break;
        case 0x400:
            sndId = 0x1C3; group = 0; priority = 2;
            break;
        case 0x401:
            sndId = 0x1C4; group = 0; priority = 2;
            break;
        case 0x402:
            sndId = 0x1C5; group = 0; priority = 2;
            break;
        case 0x403:
            sndId = 0x1C6; group = 0; priority = 2;
            break;
        case 0x404:
            sndId = 0x1C7; group = 0; priority = 2;
            break;
        case 0x405:
            sndId = 0x1C8; group = 0; priority = 2;
            break;
        case 0x406:
            sndId = 0x1C9; group = 0; priority = 2;
            break;
        case 0x407:
            sndId = 0x1CA; group = 0; priority = 2;
            break;
        case 0x408:
            sndId = 0x1CB; group = 0; priority = 2;
            break;
        case 0x409:
            sndId = 0x1CC; group = 0; priority = 2;
            break;
        case 0x40A:
            sndId = 0x1CD; group = 0; priority = 2;
            break;
        case 0x40B:
            sndId = 0x1CE; group = 0; priority = 2;
            break;
        case 0x40C:
            sndId = 0x1CF; group = 0; priority = 2;
            break;
        case 0x40D:
            sndId = 0x1D0; group = 0; priority = 2;
            break;
        case 0x40E:
            sndId = 0x1D1; group = 0; priority = 2;
            break;
        case 0x40F:
            sndId = 0x1D2; group = 0; priority = 2;
            break;
        case 0x410:
            sndId = 0x21A; group = 0; priority = 2;
            break;
        case 0x411:
            sndId = 0x21B; group = 0; priority = 2;
            break;
        case 0x412:
            sndId = 0x21C; group = 0; priority = 2;
            break;
        case 0x413:
            sndId = 0x21D; group = 0; priority = 2;
            break;
        case 0x414:
            sndId = 0x21E; group = 0; priority = 2;
            break;
        case 0x415:
            sndId = 0x21F; group = 0; priority = 2;
            break;
        case 0x416:
            sndId = 0x220; group = 0; priority = 2;
            break;
        case 0x417:
            sndId = 0x221; group = 0; priority = 2;
            break;
        case 0x418:
            sndId = 0x222; group = 0; priority = 2;
            break;
        case 0x419:
            sndId = 0x223; group = 0; priority = 2;
            break;
        case 0x41A:
            sndId = 0x224; group = 0; priority = 2;
            break;
        case 0x41B:
            sndId = 0x225; group = 0; priority = 2;
            break;
        case 0x41C:
            sndId = 0x226; group = 0; priority = 2;
            break;
        case 0x41D:
            sndId = 0x227; group = 0; priority = 2;
            break;
        case 0x41E:
            sndId = 0x228; group = 0; priority = 2;
            break;
        case 0x41F:
            sndId = 0x229; group = 0; priority = 2;
            break;
        case 0x420:
            sndId = 0x22A; group = 0; priority = 2;
            break;
        case 0x421:
            sndId = 0x22B; group = 0; priority = 2;
            break;
        case 0x422:
            sndId = 0x22C; group = 0; priority = 2;
            break;
        case 0x423:
            sndId = 0x22D; group = 0; priority = 2;
            break;
        case 0x424:
            sndId = 0x22E; group = 0; priority = 2;
            break;
        case 0x425:
            sndId = 0x22F; group = 0; priority = 2;
            break;
        case 0x426:
            sndId = 0x230; group = 0; priority = 2;
            break;
        case 0x427:
            sndId = 0x231; group = 0; priority = 2;
            break;
        case 0x428:
            sndId = 0x232; group = 0; priority = 2;
            break;
        case 0x429:
            sndId = 0x233; group = 0; priority = 2;
            break;
        case 0x42A:
            sndId = 0x234; group = 0; priority = 2;
            break;
        case 0x42B:
            sndId = 0x235; group = 0; priority = 2;
            break;
        case 0x42C:
            sndId = 0x236; group = 0; priority = 2;
            break;
        case 0x42D:
            sndId = 0x237; group = 0; priority = 2;
            break;
        default:
            break;
    }
    // clang-format on

    if (sndId == -1) {
        PRINTF("NA : SE ( %d ) is not set\n");
        return;
    }

    if (skipPos == 0) {
        PRINTF("NA : x = %d y = %d value=%d value2=%d \n");
        Na_SeCalcPanVolume(params, &pan, &volume);
    }

    Nai_SeTrig(sndId, group, priority, pan, volume);

    if (detune != 0) {
        pitch += (f32)(8.0 - (gAudioCtx.audioRandom & 0xF)) / 100.0;
        volume += (f32)(8.0 - (gAudioCtx.audioRandom & 0xF)) / 160.0;
    }

    if (colorPitch != 0) {
        if (gNaChibiYoshi != 0) {
            pitch *= 1.4;
        }
        if (params != NULL) {
            switch (params->yoshiColor) {
                case 0x01:
                    break;
                case 0x02:
                case 0x80:
                    pitch *= 0.9;
                    break;
                case 0x04:
                case 0x10:
                    pitch *= 1.1;
                    break;
                case 0x40:
                    pitch *= 0.8;
                    break;
                case 0x20:
                    pitch *= 1.2;
                    break;
            }
        }
    }

    Nai_SeSetTune(gNaiSeLastAlloc, pitch, 0);

    if (skipPos != 0) {
        return;
    }

    if (gNaEcho == 0) {
        return;
    }

    if (gNaScene == 0x36) {
        gNaEcho = 0;
    } else {
        Nai_SeSetFxmix(gNaiSeLastAlloc, 0x50, 0);
    }
}

// Stops whatever plays in gNaSeHandles[se][subHandle] and starts SE `se` there through Nai_SeStart, with
// Na_SeTrig's case scheme; `posPitch` bends the pitch by posY.
// Load-bearing: the one-line case bodies in their order, case 0x32's `if`/`else` included.
void Na_SeStart(s32 se, s16 subHandle, SoundEventParams* params) {
    s16 sndId = -1;
    s16 group;
    s16 priority;
    s16 pan = 0x3F;
    f32 volume = 1.0f;
    f32 pitch = 1.0f;
    UNUSED s16 value;
    UNUSED s16 value2;
    s16 colorPitch = 0;
    s16 posPitch = 0;
    s16 skipPos = 0;
    SoundEventParams localParams;
    // params->value at the previous se 0x32 / 0x37: each fires once as |value| rises past its threshold.
    static s16 sSe32Prev = 0;
    static s16 sSe37Prev = 0;

    PRINTF("NA : Na_SeStart %d\n");
    PRINTF("NA : sub_handle = %d\n");

    Nai_SeStop(gNaSeHandles[se][subHandle]);

    // clang-format off
    switch (se) {
        case 0x00:
            return;
        case 0x01:
            group = 4; priority = 3; sndId = 0x3A;
            break;
        case 0x02:
            group = 4; priority = 3; colorPitch = 1; sndId = 0x22;
            break;
        case 0x03:
            group = 4; priority = 3; colorPitch = 1; sndId = 0x35;
            break;
        case 0x04:
            group = -1; priority = 3; sndId = 0x1AF;
            break;
        case 0x05:
            group = 4; priority = 3; colorPitch = 1; sndId = 0x33;
            break;
        case 0x06:
            group = -1; priority = 6; posPitch = 1; sndId = 0x48;
            break;
        case 0x07:
            sndId = 0x38;
            if (params != NULL) {
                pitch = (params->value - 0x40) / 256.0 + 1.0;
            }
            group = 3; priority = 7;
            skipPos = 1;
            break;
        case 0x08:
            group = 3; priority = 7; sndId = 0x5F;
            skipPos = 1;
            break;
        case 0x09:
            group = 3; priority = 7; sndId = 0x64;
            skipPos = 1;
            break;
        case 0x0A:
            group = 3; priority = 7; sndId = 0x65;
            skipPos = 1;
            break;
        case 0x0B:
            group = 3; priority = 7; sndId = 0x1E5;
            skipPos = 1;
            break;
        case 0x0C:
            group = -1; priority = 6; sndId = 0x4B;
            break;
        case 0x0D:
            group = -1; priority = 6; sndId = 0x39;
            break;
        case 0x0E:
            if (params != NULL) {
                volume *= params->value / 128.0;
            }
            sndId = 0x49; group = -1; priority = 6;
            break;
        case 0x0F:
            if (params != NULL) {
                volume *= params->value / 128.0;
            }
            sndId = 0x3B; group = -1; priority = 6;
            break;
        case 0x10:
            group = -1; priority = 6; sndId = 0x6E;
            break;
        case 0x11:
            group = -1; priority = 6; sndId = 0x49;
            break;
        case 0x12:
            if (params != NULL) {
                pitch += params->value / 256.0;
                volume *= params->value / 256.0 + 0.5;
            }
            sndId = 0x4A; group = -1; priority = 6;
            break;
        case 0x13:
            group = -1; priority = 4; sndId = 0x82;
            break;
        case 0x14:
            group = -1; priority = 4; sndId = 0x71;
            break;
        case 0x15:
            sndId = 0x136; group = 0; priority = 2;
            skipPos = 1;
            break;
        case 0x16:
            return;
        case 0x17:
            group = -1; priority = 6; sndId = 0x83;
            break;
        case 0x18:
            group = -1; priority = 6; sndId = 0x85;
            break;
        case 0x19:
            group = -1; priority = 6; sndId = 0x8A;
            break;
        case 0x1A:
            group = -1; priority = 6; sndId = 0x8B;
            break;
        case 0x1B:
            group = -1; priority = 6; posPitch = 1; sndId = 0x42;
            break;
        case 0x1C:
            volume *= 0.75;
            group = 4; priority = 3; colorPitch = 1; sndId = 0x153;
            break;
        case 0x1D:
            group = 3; priority = 2; sndId = 0x93;
            break;
        case 0x1E:
            return;
        case 0x1F:
            group = -1; priority = 2; sndId = 0x97;
            break;
        case 0x20:
            group = -1; priority = 4; sndId = 0x9B;
            break;
        case 0x21:
            group = -1; priority = 6; sndId = 0x4C;
            break;
        case 0x22:
            return;
        case 0x23:
            sndId = 0x240; group = 0; priority = 2;
            break;
        case 0x24:
            group = -1; priority = 4; sndId = 0xAF;
            break;
        case 0x25:
            group = -1; priority = 4; sndId = 0xA5;
            break;
        case 0x26:
            if (gNaPageEnd != 0) {
                pitch = 0.0f;
            }
            sndId = 0xBF; group = -1; priority = 4;
            break;
        case 0x27:
            if (Na_SeLevelCheck(0x1D, 0) != 1) {
                pitch = 0.0f;
            }
            group = -1; priority = 6; sndId = 0x56;
            break;
        case 0x28:
            group = 0x14; priority = 4; sndId = 0x59;
            break;
        case 0x29:
            group = -1; priority = 4; sndId = 0xC5;
            break;
        case 0x2A:
            if (params != NULL) {
                volume *= params->value / 128.0;
            }
            sndId = 0xD7; group = -1; priority = 6;
            break;
        case 0x2B:
            if (params != NULL) {
                volume *= params->value / 128.0 + 0.3;
            }
            sndId = 0x1A2; group = -1; priority = 6;
            break;
        case 0x2C:
            group = -1; priority = 4; sndId = 0x100;
            break;
        case 0x2D:
            group = 3; priority = 7; sndId = 0x10A;
            skipPos = 1;
            break;
        case 0x2E:
            group = 0xF; priority = 4; sndId = 0x126;
            break;
        case 0x2F:
            if (Na_SeLevelCheck(0x1D, 0) == 1) {
                volume *= 0.5;
            }
            group = -1; priority = 6; posPitch = 1; sndId = 0x136;
            break;
        case 0x30:
            group = -1; priority = 4; sndId = 0x146;
            break;
        case 0x31:
            return;
        case 0x32:
            if (params != NULL) {
                if (Na_Fabs((f32)params->value) >= 60.0f) { if (Na_Fabs((f32)sSe32Prev) < 60.0f) { Na_SeStart(0x1C, 0, params); } } else { Na_SeLevelStop(0x1C, 0); }
                sSe32Prev = params->value;
            }
            group = -1; priority = 1; sndId = 0x250;
            skipPos = 1;
            break;
        case 0x33:
            group = -1; priority = 6; sndId = 0x262;
            break;
        case 0x34:
            group = -1; priority = 6; sndId = 0x263;
            break;
        case 0x35:
            group = -1; priority = 6; sndId = 0x12F;
            break;
        case 0x36:
            group = 4; priority = 3; colorPitch = 1; sndId = 0x168;
            break;
        case 0x37:
            if (params != NULL) {
                if (Na_Fabs((f32)params->value) >= 120.0f) {
                    if (Na_Fabs((f32)sSe37Prev) < 120.0f) {
                        Na_SeTrig(0xAF, params);
                    }
                }
                if (Na_Fabs((f32)params->value) < 60.0f) {
                    Na_Event(NA_EVENT_VOICE_STOP);
                }
                sSe37Prev = params->value;
            }
            group = -1; priority = 1; sndId = 0x250;
            skipPos = 1;
            break;
        case 0x38:
            if (Na_SeLevelCheck(0x1D, 0) == 1) {
                volume *= 0.5;
            }
            group = 0x13; priority = 4; sndId = 0x19E;
            break;
        case 0x39:
            volume *= 0.8;
            group = -1; priority = 6; posPitch = 1; sndId = 0x82;
            break;
        case 0x3A:
            if (params != NULL) {
                volume *= params->value / 128.0;
            }
            sndId = 0x49; group = -1; priority = 6;
            break;
        case 0x3B:
            group = -1; priority = 6; sndId = 0x1E6;
            break;
        case 0x3C:
            sndId = 0x128; group = 0; priority = 2;
            skipPos = 1;
            break;
        case 0x3D:
            sndId = 0x1A6; group = 0; priority = 2;
            skipPos = 1;
            break;
        case 0x3E:
            group = 3; priority = 7; sndId = 0x154;
            skipPos = 1;
            break;
        case 0x3F:
            group = -1; priority = 6; sndId = 0x1B1;
            break;
        case 0x40:
            if (params != NULL) {
                volume *= params->value / 128.0;
                if (params->value < 60.0) {
                    localParams.value = (60.0 - params->value) / 60.0 * 127.0;
                    if (gAudioCtx.audioRandom % 100 == 99) {
                        Na_SeTrig(0xD2, &localParams);
                    }
                    if (gAudioCtx.audioRandom % 200 == 199) {
                        Na_SeTrig(0xD3, &localParams);
                    }
                }
            }
            group = 3; priority = 7; sndId = 0x241;
            skipPos = 1;
            break;
        case 0x41:
            group = -1; priority = 6; sndId = 0x1D8;
            break;
        case 0x42:
            group = -1; priority = 6; sndId = 0x1D9;
            break;
        case 0x43:
            group = -1; priority = 6; sndId = 0x1DB;
            break;
        case 0x44:
            group = -1; priority = 6; sndId = 0x1DA;
            break;
        case 0x45:
            if (params != NULL) {
                volume *= params->value / 128.0;
                volume *= params->value2 / 512.0 + 0.75;
            }
            sndId = 0x1BD; group = -1; priority = 6;
            break;
        case 0x46:
            volume *= 0.4;
            group = -1; priority = 6; sndId = 0x8B;
            break;
        case 0x47:
            return;
        case 0x48:
            return;
        case 0x49:
            group = -1; priority = 6; sndId = 0x1DC;
            break;
        case 0x4A:
            if (params != NULL) {
                volume *= params->value / 128.0;
            }
            sndId = 0x1DD; group = -1; priority = 4;
            break;
        case 0x4B:
            if (params != NULL) {
                volume *= params->value / 256.0 + 0.4;
            }
            sndId = 0x1DE; group = -1; priority = 6;
            break;
        case 0x4C:
            group = -1; priority = 6; posPitch = 1; sndId = 0x42;
            break;
        case 0x4D:
            group = 3; priority = 7; sndId = 0x241;
            skipPos = 1;
            break;
        case 0x4E:
            group = -1; priority = 2; sndId = 0x219;
            break;
        case 0x4F:
            group = 3; priority = 7; sndId = 0x1E7;
            skipPos = 1;
            break;
        case 0x50:
            if (params != NULL) {
                volume *= params->value / 256.0 + 0.5;
            }
            sndId = 0x1E4; group = -1; priority = 6;
            break;
        case 0x51:
            group = -1; priority = 4; sndId = 0x1EB;
            break;
        case 0x52:
            if (params != NULL) {
                volume *= params->value / 128.0;
            }
            sndId = 0x1EE; group = -1; priority = 6;
            break;
        case 0x53:
            group = -1; priority = 6; sndId = 0x1F9;
            break;
        case 0x54:
            volume *= 0.4;
            group = -1; priority = 6; sndId = 0x1F9;
            break;
        case 0x55:
            if (params != NULL) {
                volume *= params->value / 128.0;
            }
            sndId = 0x1FA; group = -1; priority = 5;
            break;
        case 0x56:
            if (params != NULL) {
                pitch += 0.3 * (params->value / 128.0);
                volume += 0.4 * (params->value / 128.0);
            }
            sndId = 0x48; group = -1; priority = 6;
            break;
        case 0x57:
            sndId = 0x209; group = 0; priority = 2;
            break;
        case 0x58:
            if (params != NULL) {
                volume *= params->value / 128.0;
            }
            sndId = 0x20A; group = -1; priority = 4;
            break;
        case 0x59:
            group = -1; priority = 6; posPitch = 1; sndId = 0x216;
            break;
        case 0x5A:
            group = -1; priority = 4; sndId = 0x1EB;
            break;
        case 0x5B:
            return;
        case 0x5C:
            group = -1; priority = 4; sndId = 0x23A;
            break;
        case 0x5D:
            if ((gNaBgm != NULL) && (gNaBgm->num == 0x37)) {
                return;
            }
            if (params != NULL) {
                volume *= params->value / 256.0 + 0.3;
            }
            sndId = 0x136; group = -1; priority = 2;
            break;
        case 0x5E:
            group = 0xD; priority = 4; sndId = 0x246;
            break;
        case 0x5F:
            group = 3; priority = 7; sndId = 0x244;
            skipPos = 1;
            break;
        case 0x60:
            group = 3; priority = 7; sndId = 0x245;
            skipPos = 1;
            break;
        case 0x61:
            group = -1; priority = 6; sndId = 0x249;
            break;
        case 0x62:
            group = -1; priority = 6; posPitch = 1; sndId = 0x136;
            break;
        case 0x63:
            group = 0x16; priority = 4; sndId = 0x25C;
            break;
        case 0x64:
            sndId = 0x256; group = 0; priority = 2;
            skipPos = 1;
            break;
        case 0x65:
            sndId = 0x256; group = 0; priority = 2;
            skipPos = 1;
            break;
        case 0x66:
            group = 0xA; priority = 2; sndId = 0x1EF;
            skipPos = 1;
            break;
        case 0x67:
            group = 0xD; priority = 2; sndId = 0x1F0;
            skipPos = 1;
            break;
        case 0x68:
            group = 0xC; priority = 2; sndId = 0x1F1;
            skipPos = 1;
            break;
        case 0x69:
            group = 0xE; priority = 2; sndId = 0x1F2;
            skipPos = 1;
            break;
        case 0x6A:
            if (gNaBgm != NULL) {
                if (0.8 < gNaBgm->volume) {
                    sndId = 0x20E;
                } else {
                    sndId = 0x28F;
                }
            }
            group = 0x16; priority = 2;
            skipPos = 1;
            break;
        case 0x6B:
            Na_SeTrig(0x292, NULL);
            return;
        case 0x6C:
            Na_SeTrig(0x293, NULL);
            return;
        case 0x6D:
            group = -1; priority = 2; sndId = 0x25D;
            skipPos = 1;
            break;
        case 0x6E:
            group = 0x16; priority = 4; sndId = 0x261;
            skipPos = 1;
            break;
        case 0x6F:
            group = 0x19; priority = 2; sndId = 0x159;
            skipPos = 1;
            break;
        case 0x70:
            group = 0x19; priority = 2; sndId = 0x15A;
            skipPos = 1;
            break;
        case 0x71:
            group = 0x1A; priority = 2; sndId = 0x23D;
            skipPos = 1;
            break;
        case 0x72:
            group = 0x1A; priority = 2; sndId = 0x23E;
            skipPos = 1;
            break;
        case 0x73:
            if (params != NULL) {
                volume *= params->value / 5.0;
            }
            sndId = 0x296; group = -1; priority = 3;
            break;
        case 0x74:
            group = -1; priority = 3; sndId = 0x297;
            break;
        case 0x75:
            if (params == NULL) {
                return;
            }
            if (Na_Fabs((f32)(0x40 - params->posX)) < (f32)(0x40 - gNaSe76PosX)) {
                gNaSe76PosX = params->posX;
            }
            return;
        case 0x76:
            sndId = 0x299;
            if (params != NULL) {
                if (params->posX < 0) {
                    pan = 0x1E;
                    volume = volume / ((f32)-params->posX / 60.0 + 1.0);
                } else if (params->posX >= 0x80) {
                    pan = 0x64;
                    volume = volume / ((f32)(params->posX - 0x7F) / 60.0 + 1.0);
                }
            }
            group = 0x1B; priority = 2;
            skipPos = 1;
            break;
        case 0x77:
            return;
    }
    // clang-format on

    if (sndId == -1) {
        PRINTF("NA : SE ( %d ) is not set\n");
        return;
    }

    if (skipPos == 0) {
        PRINTF("NA : x = %d y = %d value=%d value2=%d \n");
        Na_SeCalcPanVolume(params, &pan, &volume);
    }

    gNaSeHandles[se][subHandle] = Nai_SeStart(sndId, group, priority, pan, volume);

    if (posPitch != 0) {
        if (params != NULL) {
            pitch -= (params->posY - 64.0) / 800.0;
        }
    }

    if (colorPitch != 0) {
        if (gNaChibiYoshi != 0) {
            pitch *= 1.4;
        }
        if (params != NULL) {
            switch (params->yoshiColor) {
                case 0x01:
                    break;
                case 0x02:
                case 0x80:
                    pitch *= 0.9;
                    break;
                case 0x04:
                case 0x10:
                    pitch *= 1.1;
                    break;
                case 0x40:
                    pitch *= 0.8;
                    break;
                case 0x20:
                    pitch *= 1.2;
                    break;
            }
        }
    }

    Nai_SeSetTune(gNaSeHandles[se][subHandle], pitch, 0);

    if (skipPos != 0) {
        return;
    }

    if (gNaEcho == 0) {
        return;
    }

    if (gNaScene == 0x36) {
        gNaEcho = 0;
    } else {
        Nai_SeSetFxmix(gNaSeHandles[se][subHandle], 0x50, 0);
    }
}

// Re-levels the SE playing in gNaSeHandles[se][subHandle], unless paused: only pan, volume, pitch and echo are
// sent, and the case table's sndId merely validates the id.
// Load-bearing: the declaration list and the one-line `if`/`else` in case 0x32.
void Na_SeLevelChange(s32 se, s16 subHandle, SoundEventParams* params) {
    s16 sndId = -1;
    UNUSED s16 group;
    UNUSED s16 priority;
    s16 pan = 0x3F;
    f32 volume = 1.0f;
    f32 pitch = 1.0f;
    UNUSED s16 value;
    UNUSED s16 value2;
    s16 colorPitch = 0;
    s16 posPitch = 0;
    s16 skipPos = 0;
    SoundEventParams localParams;
    // params->value at the previous se 0x32 / 0x37: each fires once as |value| rises past its threshold.
    static s16 sSe32Prev = 0;
    static s16 sSe37Prev = 0;

    PRINTF("NA : Na_SeLevelChange %d\n");
    PRINTF("NA : sub_handle = %d\n");

    if (gNaPause != 0) {
        return;
    }

    if (Na_SeLevelCheck(se, subHandle) != 1) {
        return;
    }

    switch (se) {
        case 0x00:
            return;
        case 0x01:
            sndId = 0x3A;
            break;
        case 0x02:
            colorPitch = 1;
            sndId = 0x22;
            break;
        case 0x03:
            colorPitch = 1;
            sndId = 0x35;
            break;
        case 0x04:
            sndId = 0x1AF;
            break;
        case 0x05:
            colorPitch = 1;
            sndId = 0x33;
            break;
        case 0x06:
            posPitch = 1;
            sndId = 0x48;
            break;
        case 0x07:
            sndId = 0x38;
            if (params != NULL) {
                pitch = (params->value - 0x40) / 256.0 + 1.0;
            }
            skipPos = 1;
            break;
        case 0x08:
            sndId = 0x5F;
            skipPos = 1;
            break;
        case 0x09:
            sndId = 0x64;
            skipPos = 1;
            break;
        case 0x0A:
            sndId = 0x65;
            skipPos = 1;
            break;
        case 0x0B:
            sndId = 0x1E5;
            skipPos = 1;
            break;
        case 0x0C:
            sndId = 0x4B;
            break;
        case 0x0D:
            sndId = 0x39;
            break;
        case 0x0E:
            if (params != NULL) {
                volume *= params->value / 128.0;
            }
            sndId = 0x49;
            break;
        case 0x0F:
            if (params != NULL) {
                volume *= params->value / 128.0;
            }
            sndId = 0x3B;
            break;
        case 0x10:
            sndId = 0x6E;
            break;
        case 0x11:
            sndId = 0x49;
            break;
        case 0x12:
            if (params != NULL) {
                pitch += params->value / 256.0;
                volume *= params->value / 256.0 + 0.5;
            }
            sndId = 0x4A;
            break;
        case 0x13:
            sndId = 0x82;
            break;
        case 0x14:
            sndId = 0x71;
            break;
        case 0x15:
            sndId = 0x136;
            skipPos = 1;
            break;
        case 0x16:
            return;
        case 0x17:
            sndId = 0x83;
            break;
        case 0x18:
            sndId = 0x85;
            break;
        case 0x19:
            sndId = 0x8A;
            break;
        case 0x1A:
            sndId = 0x8B;
            break;
        case 0x1B:
            posPitch = 1;
            sndId = 0x42;
            break;
        case 0x1C:
            volume *= 0.75;
            colorPitch = 1;
            sndId = 0x153;
            break;
        case 0x1D:
            sndId = 0x93;
            break;
        case 0x1E:
            return;
        case 0x1F:
            sndId = 0x97;
            break;
        case 0x20:
            sndId = 0x9B;
            break;
        case 0x21:
            sndId = 0x4C;
            break;
        case 0x22:
            return;
        case 0x23:
            sndId = 0x240;
            break;
        case 0x24:
            sndId = 0xAF;
            break;
        case 0x25:
            sndId = 0xA5;
            break;
        case 0x26:
            if (gNaPageEnd != 0) {
                pitch = 0.0f;
            }
            sndId = 0xBF;
            break;
        case 0x27:
            if (Na_SeLevelCheck(0x1D, 0) != 1) {
                pitch = 0.0f;
            }
            sndId = 0x56;
            break;
        case 0x28:
            sndId = 0x59;
            break;
        case 0x29:
            sndId = 0xC5;
            break;
        case 0x2A:
            if (params != NULL) {
                volume *= params->value / 128.0;
            }
            sndId = 0xD7;
            break;
        case 0x2B:
            if (params != NULL) {
                volume *= params->value / 128.0 + 0.3;
            }
            sndId = 0x1A2;
            break;
        case 0x2C:
            sndId = 0x100;
            break;
        case 0x2D:
            sndId = 0x10A;
            skipPos = 1;
            break;
        case 0x2E:
            sndId = 0x126;
            break;
        case 0x2F:
            if (Na_SeLevelCheck(0x1D, 0) == 1) {
                volume *= 0.5;
            }
            posPitch = 1;
            sndId = 0x136;
            break;
        case 0x30:
            sndId = 0x146;
            break;
        case 0x31:
            return;
        case 0x32:
            if (params != NULL) {
                // clang-format off
                if (Na_Fabs((f32)params->value) >= 60.0f) { if (Na_Fabs((f32)sSe32Prev) < 60.0f) { Na_SeStart(0x1C, 0, params); } } else { Na_SeLevelStop(0x1C, 0); }
                // clang-format on
                sSe32Prev = params->value;
            }
            sndId = 0x250;
            skipPos = 1;
            break;
        case 0x33:
            sndId = 0x262;
            break;
        case 0x34:
            sndId = 0x263;
            break;
        case 0x35:
            sndId = 0x12F;
            break;
        case 0x36:
            colorPitch = 1;
            sndId = 0x168;
            break;
        case 0x37:
            if (params != NULL) {
                if (Na_Fabs((f32)params->value) >= 120.0f) {
                    if (Na_Fabs((f32)sSe37Prev) < 120.0f) {
                        Na_SeTrig(0xAF, params);
                    }
                }
                if (Na_Fabs((f32)params->value) < 60.0f) {
                    Na_Event(NA_EVENT_VOICE_STOP);
                }
                sSe37Prev = params->value;
            }
            sndId = 0x250;
            skipPos = 1;
            break;
        case 0x38:
            if (Na_SeLevelCheck(0x1D, 0) == 1) {
                volume *= 0.5;
            }
            sndId = 0x19E;
            break;
        case 0x39:
            volume *= 0.8;
            posPitch = 1;
            sndId = 0x82;
            break;
        case 0x3A:
            if (params != NULL) {
                volume *= params->value / 128.0;
            }
            sndId = 0x49;
            break;
        case 0x3B:
            sndId = 0x1E6;
            break;
        case 0x3C:
            sndId = 0x128;
            skipPos = 1;
            break;
        case 0x3D:
            sndId = 0x1A6;
            skipPos = 1;
            break;
        case 0x3E:
            sndId = 0x154;
            skipPos = 1;
            break;
        case 0x3F:
            sndId = 0x1B1;
            break;
        case 0x40:
            if (params != NULL) {
                volume *= params->value / 128.0;
                if (params->value < 60.0) {
                    localParams.value = (60.0 - params->value) / 60.0 * 127.0;
                    if (gAudioCtx.audioRandom % 100 == 99) {
                        Na_SeTrig(0xD2, &localParams);
                    }
                    if (gAudioCtx.audioRandom % 200 == 199) {
                        Na_SeTrig(0xD3, &localParams);
                    }
                }
            }
            sndId = 0x241;
            skipPos = 1;
            break;
        case 0x41:
            sndId = 0x1D8;
            break;
        case 0x42:
            sndId = 0x1D9;
            break;
        case 0x43:
            sndId = 0x1DB;
            break;
        case 0x44:
            sndId = 0x1DA;
            break;
        case 0x45:
            if (params != NULL) {
                volume *= params->value / 128.0;
                volume *= params->value2 / 512.0 + 0.75;
            }
            sndId = 0x1BD;
            break;
        case 0x46:
            volume *= 0.4;
            sndId = 0x8B;
            break;
        case 0x47:
            return;
        case 0x48:
            return;
        case 0x49:
            sndId = 0x1DC;
            break;
        case 0x4A:
            if (params != NULL) {
                volume *= params->value / 128.0;
            }
            sndId = 0x1DD;
            break;
        case 0x4B:
            if (params != NULL) {
                volume *= params->value / 256.0 + 0.4;
            }
            sndId = 0x1DE;
            break;
        case 0x4C:
            posPitch = 1;
            sndId = 0x42;
            break;
        case 0x4D:
            sndId = 0x241;
            skipPos = 1;
            break;
        case 0x4E:
            sndId = 0x219;
            break;
        case 0x4F:
            sndId = 0x1E7;
            skipPos = 1;
            break;
        case 0x50:
            if (params != NULL) {
                volume *= params->value / 256.0 + 0.5;
            }
            sndId = 0x1E4;
            break;
        case 0x51:
            sndId = 0x1EB;
            break;
        case 0x52:
            if (params != NULL) {
                volume *= params->value / 128.0;
            }
            sndId = 0x1EE;
            break;
        case 0x53:
            sndId = 0x1F9;
            break;
        case 0x54:
            volume *= 0.4;
            sndId = 0x1F9;
            break;
        case 0x55:
            if (params != NULL) {
                volume *= params->value / 128.0;
            }
            sndId = 0x1FA;
            break;
        case 0x56:
            if (params != NULL) {
                pitch += 0.3 * (params->value / 128.0);
                volume += 0.4 * (params->value / 128.0);
            }
            sndId = 0x48;
            break;
        case 0x57:
            sndId = 0x209;
            break;
        case 0x58:
            if (params != NULL) {
                volume *= params->value / 128.0;
            }
            sndId = 0x20A;
            break;
        case 0x59:
            posPitch = 1;
            sndId = 0x216;
            break;
        case 0x5A:
            sndId = 0x1EB;
            break;
        case 0x5B:
            return;
        case 0x5C:
            sndId = 0x23A;
            break;
        case 0x5D:
            if ((gNaBgm != NULL) && (gNaBgm->num == 0x37)) {
                return;
            }
            if (params != NULL) {
                volume *= params->value / 256.0 + 0.3;
            }
            sndId = 0x136;
            break;
        case 0x5E:
            sndId = 0x246;
            break;
        case 0x5F:
            sndId = 0x244;
            skipPos = 1;
            break;
        case 0x60:
            sndId = 0x245;
            skipPos = 1;
            break;
        case 0x61:
            sndId = 0x249;
            break;
        case 0x62:
            posPitch = 1;
            sndId = 0x136;
            break;
        case 0x63:
            sndId = 0x25C;
            break;
        case 0x64:
            sndId = 0x256;
            skipPos = 1;
            break;
        case 0x65:
            sndId = 0x256;
            skipPos = 1;
            break;
        case 0x66:
            sndId = 0x1EF;
            skipPos = 1;
            break;
        case 0x67:
            sndId = 0x1F0;
            skipPos = 1;
            break;
        case 0x68:
            sndId = 0x1F1;
            skipPos = 1;
            break;
        case 0x69:
            sndId = 0x1F2;
            skipPos = 1;
            break;
        case 0x6A:
            if (gNaBgm != NULL) {
                sndId = 0x28F;
                if (0.8 < gNaBgm->volume) {
                    sndId = 0x20E;
                    skipPos = 1;
                    break;
                }
            }
            skipPos = 1;
            break;
        case 0x6B:
            Na_SeTrig(0x292, NULL);
            return;
        case 0x6C:
            Na_SeTrig(0x293, NULL);
            return;
        case 0x6D:
            sndId = 0x25D;
            skipPos = 1;
            break;
        case 0x6E:
            sndId = 0x261;
            skipPos = 1;
            break;
        case 0x6F:
            sndId = 0x159;
            skipPos = 1;
            break;
        case 0x70:
            sndId = 0x15A;
            skipPos = 1;
            break;
        case 0x71:
            sndId = 0x23D;
            skipPos = 1;
            break;
        case 0x72:
            sndId = 0x23E;
            skipPos = 1;
            break;
        case 0x73:
            if (params != NULL) {
                volume *= params->value / 5.0;
            }
            sndId = 0x296;
            break;
        case 0x74:
            sndId = 0x297;
            break;
        case 0x75:
            if (params == NULL) {
                return;
            }
            if (Na_Fabs((f32)(0x40 - params->posX)) < (f32)(0x40 - gNaSe76PosX)) {
                gNaSe76PosX = params->posX;
            }
            return;
        case 0x76:
            sndId = 0x299;
            if (params != NULL) {
                if (params->posX < 0) {
                    pan = 0x1E;
                    volume = volume / ((f32)-params->posX / 60.0 + 1.0);
                } else if (params->posX >= 0x80) {
                    pan = 0x64;
                    volume = volume / ((f32)(params->posX - 0x7F) / 60.0 + 1.0);
                }
            }
            skipPos = 1;
            break;
        case 0x77:
            return;
    }

    if (sndId == -1) {
        PRINTF("NA : SE ( %d ) is not set\n");
        return;
    }

    if (skipPos == 0) {
        PRINTF("NA : x = %d y = %d value=%d value2=%d \n");
        Na_SeCalcPanVolume(params, &pan, &volume);
    }

    Nai_SeSetPan(gNaSeHandles[se][subHandle], pan, 0);
    Nai_SeSetVolume(gNaSeHandles[se][subHandle], volume, 0);

    if (posPitch != 0) {
        if (params != NULL) {
            pitch -= (params->posY - 64.0) / 800.0;
        }
    }

    if (colorPitch != 0) {
        if (gNaChibiYoshi != 0) {
            pitch *= 1.4;
        }
        if (params != NULL) {
            switch (params->yoshiColor) {
                case 0x01:
                    break;
                case 0x02:
                case 0x80:
                    pitch *= 0.9;
                    break;
                case 0x04:
                case 0x10:
                    pitch *= 1.1;
                    break;
                case 0x40:
                    pitch *= 0.8;
                    break;
                case 0x20:
                    pitch *= 1.2;
                    break;
            }
        }
    }

    Nai_SeSetTune(gNaSeHandles[se][subHandle], pitch, 0);

    if (skipPos != 0) {
        return;
    }

    if (gNaEcho == 0) {
        return;
    }

    if (gNaScene == 0x36) {
        gNaEcho = 0;
    } else {
        Nai_SeSetFxmix(gNaSeHandles[se][subHandle], 0x50, 0);
    }
}

#include "global.h"
#include "audio.h"

// The BGM's response to game state: Yoshi's status re-voices the current track, the active Minobon re-flags it.
// The PRINTFs are stripped debug prints whose strings stay in .rodata.

s32 gNaYoshiStatus = 0;
s32 gNaWhichMinobon = 2;

// Records Yoshi's status (0-3) while a BGM is set and, on a track Na_BgmFollowsYoshiStatus accepts, sets its tune,
// tempo and mute mask; status 2 slows it, drops the tune to 0.67 and sets a per-track tune sub-mask.
// Load-bearing: the `case 0x0E: case 0x25: case 0x35:` labels beside `default:`.
void Na_YoshiStatusChange(s32 status) {
    PRINTF("NA : Na_YoshiStatusChange %d\n");
    if (gNaBgm == NULL) {
        return;
    }
    gNaYoshiStatus = status;
    if (Na_BgmFollowsYoshiStatus() == 0) {
        return;
    }

    switch (status) {
        case 0:
            Na_BgmTuneReset();
            Nai_BgmSetTempoOffset(gNaBgm, 0, 0x3C);
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
                case 0x11:
                case 0x14:
                case 0x23:
                case 0x2D:
                case 0x32:
                    Nai_BgmSetMuteSubMask(gNaBgm, 0x7800);
                    break;
                case 0x0E:
                case 0x25:
                case 0x35:
                default:
                    Nai_BgmSetMuteSubMask(gNaBgm, 0x4000);
                    break;
                case 0x24:
                    Na_SetWhichMinobon(-1);
                    break;
            }
            break;

        case 1:
            Na_BgmTuneReset();
            Nai_BgmSetTempoOffset(gNaBgm, 0, 0x3C);
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
                case 0x11:
                case 0x14:
                case 0x23:
                case 0x2D:
                case 0x32:
                    Nai_BgmSetMuteSubMask(gNaBgm, 0x7800);
                    break;
                case 0x0E:
                case 0x25:
                case 0x35:
                default:
                    Nai_BgmSetMuteSubMask(gNaBgm, 0x4000);
                    break;
                case 0x24:
                    Na_SetWhichMinobon(-1);
                    break;
            }
            break;

        case 2:
            Nai_BgmSetTempoOffset(gNaBgm, -0x3C, 0x3C);
            Nai_BgmSetTune(gNaBgm, 0.67f, 0x3C);
            switch (gNaBgm->num) {
                case 0x01:
                    Nai_BgmSetMuteSubMask(gNaBgm, 0x7800);
                    Nai_BgmSetTuneSubMask(gNaBgm, 0x0002);
                    break;
                case 0x02:
                    Nai_BgmSetMuteSubMask(gNaBgm, 0x7800);
                    Nai_BgmSetTuneSubMask(gNaBgm, 0x0007);
                    break;
                case 0x03:
                    Nai_BgmSetMuteSubMask(gNaBgm, 0x7800);
                    Nai_BgmSetTuneSubMask(gNaBgm, 0x0007);
                    break;
                case 0x04:
                    Nai_BgmSetMuteSubMask(gNaBgm, 0x7800);
                    Nai_BgmSetTuneSubMask(gNaBgm, 0x0102);
                    break;
                case 0x05:
                    Nai_BgmSetMuteSubMask(gNaBgm, 0x7800);
                    break;
                case 0x06:
                    Nai_BgmSetMuteSubMask(gNaBgm, 0x7800);
                    Nai_BgmSetTuneSubMask(gNaBgm, 0x001F);
                    break;
                case 0x07:
                    Nai_BgmSetMuteSubMask(gNaBgm, 0x7800);
                    Nai_BgmSetTuneSubMask(gNaBgm, 0x000E);
                    break;
                case 0x08:
                    Nai_BgmSetMuteSubMask(gNaBgm, 0x7800);
                    Nai_BgmSetTuneSubMask(gNaBgm, 0x001A);
                    break;
                case 0x11:
                    Nai_BgmSetMuteSubMask(gNaBgm, 0x7800);
                    Nai_BgmSetTuneSubMask(gNaBgm, 0x0405);
                    break;
                case 0x23:
                    Nai_BgmSetMuteSubMask(gNaBgm, 0x7800);
                    Nai_BgmSetTuneSubMask(gNaBgm, 0x0016);
                    break;
                case 0x24:
                    Na_SetWhichMinobon(-1);
                    Nai_BgmSetTuneSubMask(gNaBgm, 0x004F);
                    break;
                case 0x2D:
                    Nai_BgmSetMuteSubMask(gNaBgm, 0x7800);
                    Nai_BgmSetTuneSubMask(gNaBgm, 0x0002);
                    break;
                case 0x14:
                    Nai_BgmSetMuteSubMask(gNaBgm, 0x7800);
                    Nai_BgmSetTuneSubMask(gNaBgm, 0x0037);
                    break;
                case 0x0E:
                    Nai_BgmSetMuteSubMask(gNaBgm, 0x4000);
                    Nai_BgmSetTuneSubMask(gNaBgm, 0x0036);
                    break;
                case 0x0A:
                    Nai_BgmSetMuteSubMask(gNaBgm, 0x7800);
                    Nai_BgmSetTuneSubMask(gNaBgm, 0x0391);
                    break;
                case 0x32:
                    Nai_BgmSetMuteSubMask(gNaBgm, 0x7800);
                    Nai_BgmSetTuneSubMask(gNaBgm, 0x003A);
                    break;
                case 0x25:
                    Nai_BgmSetMuteSubMask(gNaBgm, 0x4000);
                    Nai_BgmSetTuneSubMask(gNaBgm, 0x017B);
                    break;
                case 0x35:
                    Nai_BgmSetMuteSubMask(gNaBgm, 0x4000);
                    Nai_BgmSetTuneSubMask(gNaBgm, 0x003F);
                    break;
                case 0x09:
                    Nai_BgmSetMuteSubMask(gNaBgm, 0x7800);
                    Nai_BgmSetTuneSubMask(gNaBgm, 0x001A);
                    break;
            }
            break;

        case 3:
            Na_BgmTuneReset();
            Nai_BgmSetTempoOffset(gNaBgm, 0, 0x3C);
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
                case 0x11:
                case 0x14:
                case 0x23:
                case 0x24:
                case 0x2D:
                case 0x32:
                    Nai_BgmSetMuteSubMask(gNaBgm, 0x47FF);
                    break;
                case 0x0E:
                case 0x25:
                case 0x35:
                    Nai_BgmSetMuteSubMask(gNaBgm, 0x0000);
                    break;
            }
            break;
    }
}

// INVENTED name (no string). Ramps the current BGM's tune back to 1.0 over 0x3C frames, undoing status 2's 0.67.
void Na_BgmTuneReset(void) {
    Nai_BgmSetTune(gNaBgm, 1.0f, 0x3C);
}

// In scene 0x0F only: records a positive `which` as the active Minobon, then sets the BGM's mute mask from its flag
// word, widened by 0x7800 for Yoshi status 0-2 and replaced by 0x47FF for status 3.
void Na_SetWhichMinobon(s16 which) {
    u16 flags = 0;

    PRINTF("NA : Na_SetWhichMinobon %d\n");
    if (gNaScene != 0xF) {
        return;
    }
    if (which > 0) {
        gNaWhichMinobon = which;
    }

    switch (gNaWhichMinobon) {
        case 1:
            flags = 0x004B;
            break;
        case 2:
            flags = 0x0008;
            break;
        case 3:
            flags = 0x0028;
            break;
        case 4:
            flags = 0x0063;
            break;
        case 5:
            flags = 0x0048;
            break;
        case 6:
            flags = 0x003C;
            break;
    }

    switch (gNaYoshiStatus) {
        case 0:
            flags |= 0x7800;
            break;
        case 1:
            flags |= 0x7800;
            break;
        case 2:
            flags |= 0x7800;
            break;
        case 3:
            flags = 0x47FF;
            break;
    }

    Nai_BgmSetMuteSubMask(gNaBgm, flags);
}

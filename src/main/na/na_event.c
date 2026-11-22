#include "global.h"
#include "audio.h"

// The game's sound events: Na_Event dispatches them; Na_Se30FruitsClear is the 30-fruit clears' shared SE burst.
// The PRINTFs are stripped debug prints whose strings stay in .rodata.

u8 gNaEcho = 0;
u8 gNaTimerBgm = 0;
u8 gNaPageEnd = 0;
s16 gNaSe211Count = 0;

// INVENTED name (no string). Triggers SE 0xD7 ten times; the six 30-fruit clears run it between Na_LevelSeStop and
// their own SE.
void Na_Se30FruitsClear(void) {
    s32 i;

    for (i = 0; i < 10; i++) {
        Na_SeTrig(0xD7, NULL);
    }
}

// Dispatches an NaEvent (Sound_Event's id); every arm but NA_EVENT_NONE's opens with its stripped
// `NA : Na_Event <NAME>` trace, which names the enum. NYORO_DANGER_IN and NYORO_DANGER_OUT only printed.
// Load-bearing: the empty `case NA_EVENT_NONE:` arm.
void Na_Event(s32 event) {
    switch (event) {
        case NA_EVENT_NONE:
            break;

        case NA_EVENT_OPEN_SOGEN_PAGE:
            PRINTF("NA : Na_Event OPEN_SOGEN_PAGE\n");
            Nai_BgmStop(gNaBgm, 0);
            Na_SubGameBgmStop(0x3C);
            gNaBgm = Nai_BgmStartWaiting(0x10, 0, 0);
            break;

        case NA_EVENT_OPEN_MAGMA_PAGE:
            PRINTF("NA : Na_Event OPEN_MAGMA_PAGE\n");
            Nai_BgmStop(gNaBgm, 0);
            Na_SubGameBgmStop(0x3C);
            gNaBgm = Nai_BgmStartWaiting(0x12, 0, 0);
            break;

        case NA_EVENT_OPEN_MOUNTAIN_PAGE:
            PRINTF("NA : Na_Event OPEN_MOUNTAIN_PAGE\n");
            Nai_BgmStop(gNaBgm, 0);
            Na_SubGameBgmStop(0x3C);
            gNaBgm = Nai_BgmStartWaiting(0x16, 0, 0);
            break;

        case NA_EVENT_OPEN_JUNGLE_PAGE:
            PRINTF("NA : Na_Event OPEN_JUNGLE_PAGE\n");
            Nai_BgmStop(gNaBgm, 0);
            Na_SubGameBgmStop(0x3C);
            gNaBgm = Nai_BgmStartWaiting(0x19, 0, 0);
            break;

        case NA_EVENT_OPEN_WATER_PAGE:
            PRINTF("NA : Na_Event OPEN_WATER_PAGE\n");
            Nai_BgmStop(gNaBgm, 0);
            Na_SubGameBgmStop(0x3C);
            gNaBgm = Nai_BgmStartWaiting(0x17, 0, 0);
            break;

        case NA_EVENT_OPEN_CASTLE_PAGE:
            PRINTF("NA : Na_Event OPEN_CASTLE_PAGE\n");
            Nai_BgmStop(gNaBgm, 0);
            Na_SubGameBgmStop(0x3C);
            gNaBgm = Nai_BgmStartWaiting(0x13, 0, 0);
            break;

        case NA_EVENT_OPENING_PLAYER_SELECT:
            PRINTF("NA : Na_Event OPENING_PLAYER_SELECT\n");
            if (gNaBgm != NULL) {
                gNaBgm->alongside = 1;
            }
            Na_SubGameBgmStart(0xC, 0);
            Nai_BgmSetMuteSubMask(gNaSubGameBgm, Na_GetYoshiMuteMask() | 0x100);
            break;

        case NA_EVENT_START_TURESARI_DEMO:
            PRINTF("NA : Na_Event START_TURESARI_DEMO\n");
            if (gNaBgm != NULL) {
                gNaBgm->alongside = 1;
            }
            Na_SubGameBgmStart(0x39, 0);
            break;

        case NA_EVENT_GAMEOVER:
            PRINTF("NA : Na_Event GAMEOVER\n");
            if (gNaBgm != NULL) {
                gNaBgm->alongside = 1;
            }
            Na_SubGameBgmStart(0xF, 0);
            break;

        case NA_EVENT_YOSHI_DOWN:
            PRINTF("NA : Na_Event YOSHI_DOWN\n");
            gNaPageEnd = 1;
            gNaLevelSeStopTimer = 5;
            Na_BgmChange(0x1A, 0);
            Na_AllSeStop();
            Na_SeTrig(0x294, NULL);
            Na_SeTrig(0x86, NULL);
            break;

        case NA_EVENT_YOSHI_FALL:
            PRINTF("NA : Na_Event YOSHI_FALL\n");
            gNaPageEnd = 1;
            gNaLevelSeStopTimer = 5;
            Na_BgmChange(0x1B, 0);
            Na_AllSeStop();
            break;

        case NA_EVENT_YOSHI_BURN:
            PRINTF("NA : Na_Event YOSHI_BURN\n");
            gNaPageEnd = 1;
            gNaLevelSeStopTimer = 5;
            Na_BgmChange(0x1B, 0);
            Na_AllSeStop();
            Na_SeTrig(0x86, NULL);
            break;

        case NA_EVENT_YOSHI_TABERARERU:
            PRINTF("NA : Na_Event YOSHI_TABERARERU\n");
            gNaPageEnd = 1;
            gNaLevelSeStopTimer = 5;
            Na_BgmChange(0x1B, 0);
            Na_AllSeStop();
            Na_SeTrig(0x53, NULL);
            break;

        case NA_EVENT_YOSHI_HASAMARERU:
            PRINTF("NA : Na_Event YOSHI_HASAMARERU\n");
            gNaPageEnd = 1;
            gNaLevelSeStopTimer = 5;
            Na_BgmChange(0x1A, 0);
            Na_AllSeStop();
            break;

        case NA_EVENT_ECHO_START:
            PRINTF("NA : Na_Event ECHO_START\n");
            gNaEcho = 1;
            break;

        case NA_EVENT_ECHO_STOP:
            PRINTF("NA : Na_Event ECHO_STOP\n");
            gNaEcho = 0;
            break;

        case NA_EVENT_MELON_30_GET:
            PRINTF("NA : Na_Event MELON_30_GET\n");
            gNaPageEnd = 1;
            if (gNaTimerBgm != 0) {
                Na_Event(NA_EVENT_TIMER_END);
            }
            Na_AllBgmStop();
            Na_BgmChange(0x30, 0);
            Na_LevelSeStop();
            Na_Se30FruitsClear();
            Na_SeTrig(0xD8, NULL);
            break;

        case NA_EVENT_MELON_30_BOSS:
            PRINTF("NA : Na_Event MELON_30_BOSS\n");
            if (gNaTimerBgm != 0) {
                Na_Event(NA_EVENT_TIMER_END);
            }
            gNaPageEnd = 1;
            gNaSe211Count = 0;
            Na_AllBgmStop();
            Na_BgmChange(0x3A, 0);
            Na_LevelSeStop();
            Na_Se30FruitsClear();
            Na_SeTrig(0xD8, NULL);
            break;

        case NA_EVENT_SAME_FRUITS_30_GET:
            PRINTF("NA : Na_Event SAME_FRUITS_30_GET\n");
            gNaPageEnd = 1;
            if (gNaTimerBgm != 0) {
                Na_Event(NA_EVENT_TIMER_END);
            }
            Na_AllBgmStop();
            Na_BgmChange(0x30, 0);
            Na_LevelSeStop();
            Na_Se30FruitsClear();
            Na_SeTrig(0xDA, NULL);
            break;

        case NA_EVENT_SAME_FRUITS_30_BOSS:
            PRINTF("NA : Na_Event SAME_FRUITS_30_BOSS\n");
            if (gNaTimerBgm != 0) {
                Na_Event(NA_EVENT_TIMER_END);
            }
            gNaPageEnd = 1;
            gNaSe211Count = 0;
            Na_AllBgmStop();
            Na_BgmChange(0x3A, 0);
            Na_LevelSeStop();
            Na_Se30FruitsClear();
            Na_SeTrig(0xDA, NULL);
            break;

        case NA_EVENT_FRUITS_30_GET:
            PRINTF("NA : Na_Event FRUITS_30_GET\n");
            gNaPageEnd = 1;
            if (gNaTimerBgm != 0) {
                Na_Event(NA_EVENT_TIMER_END);
            }
            Na_AllBgmStop();
            Na_BgmChange(0x30, 0);
            Na_LevelSeStop();
            Na_Se30FruitsClear();
            Na_SeTrig(0xCB, NULL);
            break;

        case NA_EVENT_FRUITS_30_BOSS:
            PRINTF("NA : Na_Event FRUITS_30_BOSS\n");
            if (gNaTimerBgm != 0) {
                Na_Event(NA_EVENT_TIMER_END);
            }
            gNaPageEnd = 1;
            gNaSe211Count = 0;
            Na_AllBgmStop();
            Na_BgmChange(0x3A, 0);
            Na_LevelSeStop();
            Na_Se30FruitsClear();
            Na_SeTrig(0xCB, NULL);
            break;

        case NA_EVENT_COURSE_CLEAR:
            PRINTF("NA : Na_Event COURSE_CLEAR\n");
            if (gNaBgm != NULL) {
                gNaBgm->alongside = 1;
            }
            Na_BgmStop(0xB4);
            Na_JingleBgmStart(0x31, 0);
            break;

        case NA_EVENT_COURSE_CLEAR_BOSS:
            PRINTF("NA : Na_Event COURSE_CLEAR_BOSS\n");
            if (gNaBgm != NULL) {
                gNaBgm->alongside = 1;
            }
            Na_BgmStop(0xB4);
            Na_JingleBgmStart(0x3B, 0);
            break;

        case NA_EVENT_SUBGAME_START:
            PRINTF("NA : Na_Event SUBGAME_START\n");
            Na_BgmInterrupt();
            break;

        case NA_EVENT_SUBGAME_MISS:
            PRINTF("NA : Na_Event SUBGAME_MISS\n");
            Na_BgmResume();
            Na_SeTrig(0x87, NULL);
            break;

        case NA_EVENT_OPEN_SUBGAME_END:
            PRINTF("NA : Na_Event OPEN_SUBGAME_END\n");
            Na_BgmResume();
            if (gNaBgm != NULL) {
                gNaBgm->alongside = 0;
            }
            Na_JingleBgmStart(0x1F, 0);
            break;

        case NA_EVENT_OPEN_SUBGAME_BEST:
            PRINTF("NA : Na_Event OPEN_SUBGAME_BEST\n");
            Na_BgmResume();
            if (gNaBgm != NULL) {
                gNaBgm->alongside = 0;
            }
            Na_JingleBgmStart(0x20, 0);
            break;

        case NA_EVENT_OPEN_SUBGAME_END_OKA:
            PRINTF("NA : Na_Event OPEN_SUBGAME_END_OKA\n");
            Na_BgmResume();
            if (gNaBgm != NULL) {
                gNaBgm->alongside = 0;
            }
            Na_JingleBgmStart(0x3E, 0);
            break;

        case NA_EVENT_OPEN_SUBGAME_BEST_OKA:
            PRINTF("NA : Na_Event OPEN_SUBGAME_BEST_OKA\n");
            Na_BgmResume();
            if (gNaBgm != NULL) {
                gNaBgm->alongside = 0;
            }
            Na_JingleBgmStart(0x3F, 0);
            break;

        case NA_EVENT_TIMER_START:
            PRINTF("NA : Na_Event TIMER_START\n");
            gNaTimerBgm = 1;
            Na_BgmVolumeUpdate();
            break;

        case NA_EVENT_TIMER_END:
            PRINTF("NA : Na_Event TIMER_END\n");
            gNaTimerBgm = 0;
            Na_BgmVolumeUpdate();
            Na_SetTimerCount(0, 0);
            break;

        case NA_EVENT_PAGE:
            PRINTF("NA : Na_Event PAGE\n");
            if (gNaBgm != NULL) {
                gNaBgm->alongside = 1;
            }
            Na_JingleBgmStart(0x21, 0);
            break;

        case NA_EVENT_PAGE_FIRST:
            PRINTF("NA : Na_Event PAGE_FIRST\n");
            if (gNaBgm != NULL) {
                gNaBgm->alongside = 1;
            }
            Na_JingleBgmStart(0x22, 0);
            break;

        case NA_EVENT_VOICE_STOP:
            PRINTF("NA : Na_Event VOICE_STOP\n");
            Na_SeTrig(0x8F, NULL);
            break;

        case NA_EVENT_NULL:
            PRINTF("NA : Na_Event NULL\n");
            Na_BgmChange(0x2B, 0);
            break;

        case NA_EVENT_MID_BOSS_DEMO:
            PRINTF("NA : Na_Event MID_BOSS_DEMO\n");
            Na_BgmChange(0xE, 0x78);
            break;

        case NA_EVENT_MID_BOSS_START:
            PRINTF("NA : Na_Event MID_BOSS_START\n");
            Na_BgmChange(0x3C, 0);
            Na_LevelSeStop();
            break;

        case NA_EVENT_MID_BOSS_DOWN:
            PRINTF("NA : Na_Event MID_BOSS_DOWN\n");
            if (gNaBgm != NULL) {
                gNaBgm->alongside = 1;
            }
            Na_BgmStop(0xB4);
            Na_JingleBgmStart(0x3D, 0);
            break;

        case NA_EVENT_MID_BOSS_DOWN_2:
            PRINTF("NA : Na_Event MID_BOSS_DOWN_2\n");
            Na_BgmChange(0x2C, 0);
            break;

        case NA_EVENT_KUPPA_DEMO:
            PRINTF("NA : Na_Event KUPPA_DEMO\n");
            Na_BgmChange(0x25, 0x78);
            break;

        case NA_EVENT_KUPPA_START_1:
            PRINTF("NA : Na_Event KUPPA_START_1\n");
            Nai_BgmSetVolume(gNaBgm, 0.7f, 0);
            Na_LevelSeStop();
            break;

        case NA_EVENT_KUPPA_DOWN_1:
            PRINTF("NA : Na_Event KUPPA_DOWN_1\n");
            Na_BgmChange(0x35, 0x78);
            break;

        case NA_EVENT_KUPPA_DEMO_2:
            PRINTF("NA : Na_Event KUPPA_DEMO_2\n");
            Na_BgmChange(0x36, 0);
            Na_LevelSeStop();
            break;

        case NA_EVENT_KUPPA_START_2:
            PRINTF("NA : Na_Event KUPPA_START_2\n");
            Na_BgmChange(0x37, 0x78);
            Na_LevelSeStop();
            break;

        case NA_EVENT_KUPPA_DOWN_2:
            PRINTF("NA : Na_Event KUPPA_DOWN_2\n");
            if (gNaSeHandles[0x6D][0] == NULL) {
                Na_SeStart(0x6D, 0, NULL);
            }
            Nai_SeSetVolume(gNaSeHandles[0x6D][0], 1.0f, 0x78);
            break;

        case NA_EVENT_KUPPA_TSURESARI:
            PRINTF("NA : Na_Event KUPPA_TSURESARI\n");
            if (gNaSeHandles[0x6D][0] != NULL) {
                Nai_SeSetVolume(gNaSeHandles[0x6D][0], 0.0f, 0x78);
            }
            break;

        case NA_EVENT_NYORO_DANGER_IN:
            PRINTF("NA : Na_Event NYORO_DANGER_IN\n");
            break;

        case NA_EVENT_NYORO_DANGER_OUT:
            PRINTF("NA : Na_Event NYORO_DANGER_OUT\n");
            break;
    }
}

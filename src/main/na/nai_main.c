#include "global.h"
#include "audio.h"

// The NA sound library's top layer: start-up, the per-frame driver, and the posters of the spec change, the pause
// and the output mode.
// Every function name here is INVENTED (no string).

// Starts the library: empties the command pool, posts PAUSE OFF and an immediate SEQ STOP for groups 0-2, then
// resets the BGM and SE layers.
void Nai_Init(void) {
    s16 i;

    Nai_PortInit();
    Nai_PauseOff();
    for (i = 0; i < 3; i++) {
        Nai_SeqStop(i, 0);
    }
    Nai_BgmInit();
    Nai_SeInit();
}

// Per frame: runs the BGM, SE and port routines, then hands the frame's driver commands to the audio thread unless
// a spec change is in flight.
void Nai_Routin(void) {
    Nai_BgmRoutin();
    Nai_SeRoutin();
    Nai_PortRoutin();
    if (gNaiSpecChange == 0) {
        AudioThread_ScheduleProcessCmds();
    }
}

// Switches the driver to audio spec `spec`: posts SPEC CHANGE START, resets the SE and BGM layers and stops the
// three sequence players at once. Its one caller, Na_SceneChange, passes 2 or 4.
// Load-bearing: the `type` store ahead of `num`.
void Nai_SpecChange(s16 spec) {
    NaiPort* port;

    port = Nai_PortBufferAlloc();
    if (port != NULL) {
        port->type = NAI_PORT_TYPE_SPEC_CHANGE_START;
        port->num = spec;
        Nai_SeInit();
        Nai_BgmInit();
        AudioThread_QueueCmdS32(0x83000000, 0);
        AudioThread_QueueCmdS32(0x83010000, 0);
        AudioThread_QueueCmdS32(0x83020000, 0);
    }
}

// Posts PAUSE ON, which mutes every sequence player. Uncalled: Na_PauseOn pauses through Nai_SePause instead.
void Nai_PauseOn(void) {
    NaiPort* port;

    port = Nai_PortBufferAlloc();
    if (port != NULL) {
        port->type = NAI_PORT_TYPE_PAUSE_ON;
    }
}

// Posts PAUSE OFF, which unmutes every sequence player. Nai_Init is the one caller.
void Nai_PauseOff(void) {
    NaiPort* port;

    port = Nai_PortBufferAlloc();
    if (port != NULL) {
        port->type = NAI_PORT_TYPE_PAUSE_OFF;
    }
}

// Posts PAUSE OFF STOP (uncalled): PAUSE OFF, first stopping each sounding note whose channel has mute behaviour 3.
void Nai_PauseOffStop(void) {
    NaiPort* port;

    port = Nai_PortBufferAlloc();
    if (port != NULL) {
        port->type = NAI_PORT_TYPE_PAUSE_OFF_STOP;
    }
}

// Posts OUT MODE, which sets the driver's sound output mode to `mode`, a SoundOutputMode. Na_SetSoundOutputMode is
// the one caller.
// Load-bearing: the `type` store ahead of `num`.
void Nai_OutMode(s32 mode) {
    NaiPort* port;

    port = Nai_PortBufferAlloc();
    if (port != NULL) {
        port->type = NAI_PORT_TYPE_OUT_MODE;
        port->num = mode;
    }
}

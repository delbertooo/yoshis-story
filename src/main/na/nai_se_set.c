#include "global.h"
#include "audio.h"

// The SE slot's setters, which stage the target half of a pair, and Nai_SePostChanges, which posts a command for
// each pair whose halves disagree: nai_bgm_set.c's twin for NaiSe.
// Every function name here is INVENTED (no string).
// The PRINTFs are stripped debug prints whose strings stay in .rodata.

// Float absolute value, byte-identical to Na_Fabs and Nai_Fabs.
f32 Nai_SeFabs(f32 value) {
    if (value < 0.0f) {
        return -value;
    }
    return value;
}

// Claims a port from the command pool for a slot: owner class 2 (SE), the slot, group 2 (the SE group) and the
// slot's sub. The caller sets the type and the pair pointer.
NaiPort* Nai_SePortAlloc(NaiSe* se) {
    NaiPort* port;

    port = Nai_PortBufferAlloc();
    if (port != NULL) {
        port->ownerType = 2;
        port->se = se;
        port->group = 2;
        port->sub = se->sub;
    }
    return port;
}

// For an active slot, once per frame from Nai_SeRoutin: posts one port per pair whose halves disagree (pan exactly,
// the others by more than 0.01), each a single step of its ramp.
// Load-bearing: the double `0.01`.
void Nai_SePostChanges(NaiSe* se) {
    NaiPort* port;

    if (se == NULL) {
        return;
    }
    if (Nai_SeIsActive(se) == 0) {
        return;
    }

    if (Nai_SeFabs(se->tuneTarget - se->tune) > 0.01) {
        port = Nai_SePortAlloc(se);
        if (port != NULL) {
            port->type = NAI_PORT_TYPE_SUB_TUNE;
            port->ramp = &se->tune;
            Nai_StepRamp(port);
        }
    }

    if (Nai_SeFabs(se->volumeTarget - se->volume) > 0.01) {
        port = Nai_SePortAlloc(se);
        if (port != NULL) {
            port->type = NAI_PORT_TYPE_SUB_EVOLUME;
            port->ramp = &se->volume;
            Nai_StepRamp(port);
        }
    }

    if (se->panTarget != se->pan) {
        port = Nai_SePortAlloc(se);
        if (port != NULL) {
            port->type = NAI_PORT_TYPE_SUB_PAN;
            port->ramp = &se->pan;
            Nai_StepRamp(port);
        }
    }

    if (Nai_SeFabs(se->fxmixTarget - se->fxmix) > 0.01) {
        port = Nai_SePortAlloc(se);
        if (port != NULL) {
            port->type = NAI_PORT_TYPE_SUB_FXMIX;
            port->ramp = &se->fxmix;
            Nai_StepRamp(port);
        }
    }
}

// Ramps the slot's volume to `volume` over `time` frames (0 = at once).
void Nai_SeSetVolume(NaiSe* se, f32 volume, s16 time) {
    if (se != NULL) {
        se->volumeTarget = volume;
        se->volumeTime = time;
    }
}

// Ramps the slot's pan to `pan` over `time` frames (0 = at once).
void Nai_SeSetPan(NaiSe* se, u8 pan, s16 time) {
    PRINTF("NAI: SE[%x] PAN target=%f\n");
    if (se != NULL) {
        se->panTarget = pan;
        se->panTime = time;
    }
}

// Ramps the slot's tune to `tune` over `time` frames (0 = at once).
void Nai_SeSetTune(NaiSe* se, f32 tune, s16 time) {
    if (se != NULL) {
        se->tuneTarget = tune;
        se->tuneTime = time;
    }
}

// Ramps the slot's fx mix to `fxmix` over `time` frames (0 = at once).
void Nai_SeSetFxmix(NaiSe* se, u8 fxmix, s16 time) {
    if (se != NULL) {
        se->fxmixTarget = fxmix;
        se->fxmixTime = time;
    }
}

#include "global.h"
#include "audio.h"

// The frame step of the BGM and SE ramps: Nai_BgmPostChanges and Nai_SePostChanges post a command each frame until
// a ramp's `current` meets its `target`, and each post moves it one frame on.
// Every function name here is INVENTED (no string).

// What NaiPort's `ramp` and `maskRamp` point at, inside a NaiBgm or NaiSe.
typedef struct NaiRamp {
    /* 0x00 */ f32 current;
    /* 0x04 */ f32 target;
    /* 0x08 */ s16 time;
} NaiRamp; // size = 0x0C

typedef struct NaiMaskRamp {
    /* 0x00 */ u16 mask;
    /* 0x02 */ u16 maskSent;
    /* 0x04 */ f32 current;
    /* 0x08 */ f32 target;
    /* 0x0C */ s16 time;
} NaiMaskRamp; // size = 0x10

// Moves the ramp at `port->ramp` one of its `time` frames towards `target` and leaves the result in rampValue; with
// no frames left it snaps to `target`.
// Load-bearing: the three locals in their order and the store back through `port->ramp`.
void Nai_StepRamp(NaiPort* port) {
    NaiRamp* ramp;
    f32 target;
    f32 current;
    f32 frames;

    if (port == NULL) {
        return;
    }
    ramp = (NaiRamp*)port->ramp;
    if (ramp == NULL) {
        return;
    }

    target = ramp->target;
    current = ramp->current;
    frames = ramp->time;
    if (frames == 0.0f) {
        port->rampValue = target;
    } else {
        port->rampValue = ((target - current) / frames) + current;
        ramp->time--;
    }
    ((NaiRamp*)port->ramp)->current = port->rampValue;
}

// Nai_StepRamp for the ramp behind `port->maskRamp`'s mask pair (SUB TUNE's).
// Load-bearing: the three locals in their order and the store back through `port->maskRamp`.
void Nai_StepMaskRamp(NaiPort* port) {
    NaiMaskRamp* ramp;
    f32 target;
    f32 current;
    f32 frames;

    if (port == NULL) {
        return;
    }
    ramp = (NaiMaskRamp*)port->maskRamp;
    if (ramp == NULL) {
        return;
    }

    target = ramp->target;
    current = ramp->current;
    frames = ramp->time;
    if (frames == 0.0f) {
        port->rampValue = target;
    } else {
        port->rampValue = ((target - current) / frames) + current;
        ramp->time--;
    }
    ((NaiMaskRamp*)port->maskRamp)->current = port->rampValue;
}

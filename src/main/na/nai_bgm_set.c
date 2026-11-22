#include "global.h"
#include "audio.h"

// The BGM handle's setters, which stage the requested half of a pair, and Nai_BgmPostChanges, which posts a command
// for each pair whose halves disagree.
// Every function name here is INVENTED (no string).

// Float absolute value, byte-identical to Na_Fabs and Nai_SeFabs.
f32 Nai_Fabs(f32 value) {
    if (value < 0.0f) {
        return -value;
    }
    return value;
}

// Claims a port from the command pool for a handle: owner class 1 (BGM), the handle and its group. The caller sets
// the type and the pair pointer.
NaiPort* Nai_BgmPortAlloc(NaiBgm* bgm) {
    NaiPort* port;

    port = Nai_PortBufferAlloc();
    if (port != NULL) {
        port->ownerType = 1;
        port->bgm = bgm;
        port->group = bgm->group;
    }
    return port;
}

// For an active handle, once per frame from Nai_BgmRoutin: reads the live mute mask back from the group's 16
// channels, then posts one port per pair whose halves disagree, a ramped pair one step at a time.
// Load-bearing: the `bgm->muteSubMaskLive` re-read, the double `0.01` and `port->maskRamp[1] = port->maskRamp[0]`.
void Nai_BgmPostChanges(NaiBgm* bgm) {
    NaiPort* port;
    SequenceChannel* voice;
    u16 flags;
    s32 i;

    if (bgm == NULL) {
        return;
    }
    if (Nai_BgmIsActive(bgm) == 0) {
        return;
    }

    flags = 0;
    for (i = 0; i < 16; i++) {
        voice = gAudioCtx.seqPlayers[bgm->group].channels[i];
        if (voice != NULL) {
            flags = Nai_SetMaskBit(flags, i, voice->h.f.muted);
        }
    }

    bgm->muteSubMaskLive = flags;
    if (bgm->muteSubMask != bgm->muteSubMaskLive) {
        port = Nai_BgmPortAlloc(bgm);
        if (port != NULL) {
            port->type = NAI_PORT_TYPE_SUB_MUTE;
            port->maskRamp = &bgm->muteSubMask;
        }
    }

    if (Nai_Fabs(bgm->volumeTarget - bgm->volume) > 0.01) {
        port = Nai_BgmPortAlloc(bgm);
        if (port != NULL) {
            port->type = NAI_PORT_TYPE_GRP_EVOLUME;
            port->ramp = &bgm->volume;
            Nai_StepRamp(port);
        }
    }

    if (bgm->tempoOffsetTarget != bgm->tempoOffset) {
        port = Nai_BgmPortAlloc(bgm);
        if (port != NULL) {
            port->type = NAI_PORT_TYPE_GRP_TEMPO_OFFSET;
            port->ramp = &bgm->tempoOffset;
            Nai_StepRamp(port);
        }
    }

    // maskRamp[1] is tuneSubMaskSent: nothing reads the tune mask back, so this arm syncs it.
    if ((bgm->tuneSubMask != bgm->tuneSubMaskSent) || (bgm->tuneTarget != bgm->tune)) {
        port = Nai_BgmPortAlloc(bgm);
        if (port != NULL) {
            port->type = NAI_PORT_TYPE_SUB_TUNE;
            port->maskRamp = &bgm->tuneSubMask;
            Nai_StepMaskRamp(port);
            port->maskRamp[1] = port->maskRamp[0];
        }
    }
}

// Ramps the group's volume to `volume` over `time` frames (0 = at once).
void Nai_BgmSetVolume(NaiBgm* bgm, f32 volume, s16 time) {
    if (bgm != NULL) {
        bgm->volumeTarget = volume;
        bgm->volumeTime = time;
    }
}

// Ramps the group's tempo offset to `tempoOffset` over `time` frames (0 = at once).
void Nai_BgmSetTempoOffset(NaiBgm* bgm, s16 tempoOffset, s16 time) {
    if (bgm != NULL) {
        bgm->tempoOffsetTarget = tempoOffset;
        bgm->tempoOffsetTime = time;
    }
}

// Mutes (mute != 0) or unmutes one sub of the group. No callers.
void Nai_BgmSetMuteSub(NaiBgm* bgm, s16 sub, u8 mute) {
    if (bgm != NULL) {
        bgm->muteSubMask = Nai_SetMaskBit(bgm->muteSubMask, sub, mute);
    }
}

// Mutes exactly the subs in subMask and unmutes the rest.
void Nai_BgmSetMuteSubMask(NaiBgm* bgm, u16 subMask) {
    if (bgm != NULL) {
        bgm->muteSubMask = subMask;
    }
}

// Adds (on != 0) or drops one sub from the set the tune applies to. No callers.
void Nai_BgmSetTuneSub(NaiBgm* bgm, s16 sub, u8 on) {
    if (bgm != NULL) {
        bgm->tuneSubMask = Nai_SetMaskBit(bgm->tuneSubMask, sub, on);
    }
}

// Selects exactly the subs the tune applies to.
void Nai_BgmSetTuneSubMask(NaiBgm* bgm, u16 subMask) {
    if (bgm != NULL) {
        bgm->tuneSubMask = subMask;
    }
}

// Ramps the selected subs' tune to `tune` over `time` frames (0 = at once).
void Nai_BgmSetTune(NaiBgm* bgm, f32 tune, s16 time) {
    if (bgm != NULL) {
        bgm->tuneTarget = tune;
        bgm->tuneTime = time;
    }
}

// Named in MM's stub style. Empty and uncalled; its home stores show four parameters, not their type.
void Nai_BgmSetNoop1(UNUSED s16 arg0, UNUSED s16 arg1, UNUSED s16 arg2, UNUSED s16 arg3) {
}

// The twin of Nai_BgmSetNoop1.
void Nai_BgmSetNoop2(UNUSED s16 arg0, UNUSED s16 arg1, UNUSED s16 arg2, UNUSED s16 arg3) {
}

// Sets (on != 0) or clears bit `bit` of a 16-bit mask. Every caller's bit index is a sub.
// Load-bearing: the `u16 mask` local.
u16 Nai_SetMaskBit(u16 subMask, s16 bit, u8 on) {
    u16 mask = 1 << bit;

    if (on != 0) {
        return subMask | mask;
    }
    return subMask & ~mask;
}

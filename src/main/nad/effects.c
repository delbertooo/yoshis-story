// original filename: effect.c
#include "global.h"
#include "audio.h"

// NA audio driver, OoT's effects.c: the per-frame sequence player and channel sound update, the portamento and
// vibrato steps and their note setup, and the ADSR envelope.
// The PRINTF is a stripped debug print whose string stays in .data (-use_readwrite_const).

// original name: __Nas_CallWaveProcess_Sub
// Updates the channel's applied volume (also on the player's recalculateVolume) and pan where they changed, passes
// them and the freq scale on to its layers (all three to one that needs init) and clears the change bits.
// Load-bearing: the freqScale test as `(s8)channel->h.b.changes >> 7` or the changesFreqScale bit, never `< 0`.
void Audio_SequenceChannelProcessSound(SequenceChannel* channel, s32 recalculateVolume) {
    SequenceLayer* layer;
    SequencePlayer* seqPlayer;
    f32 channelVolume;
    s32 i;

    if (channel->h.f.changesVolume || recalculateVolume) {
        seqPlayer = channel->seqPlayer;
        channelVolume = channel->volume * channel->volumeScale * seqPlayer->appliedFadeVolume;
        if (seqPlayer->muted && (channel->h.b.muteBehavior & 0x20)) {
            channelVolume = seqPlayer->muteVolumeScale * channelVolume;
        }
        channel->appliedVolume = channelVolume * channelVolume;
    }
    if (channel->h.f.changesPan) {
        channel->pan = channel->newPan * channel->panChannelWeight;
    }

    for (i = 0; i < 6; i++) {
        layer = channel->layers[i];
        if (layer != NULL && layer->h.f.enabled && layer->note != 0) {
            if (layer->h.b.flags & 1) {
                layer->noteFreqScale = layer->freqScale * channel->freqScale;
                layer->noteVelocity = layer->velocitySquare2 * channel->appliedVolume;
                layer->notePan = (channel->pan + layer->pan * (0x80 - channel->panChannelWeight)) >> 7;
                layer->h.b.flags &= 0xFFFE;
            } else {
                if ((s8)channel->h.b.changes >> 7) {
                    layer->noteFreqScale = layer->freqScale * channel->freqScale;
                }
                if (channel->h.f.changesVolume || recalculateVolume) {
                    layer->noteVelocity = layer->velocitySquare2 * channel->appliedVolume;
                }
                if (channel->h.f.changesPan) {
                    layer->notePan = (channel->pan + layer->pan * (0x80 - channel->panChannelWeight)) >> 7;
                }
            }
        }
    }
    channel->h.b.changes = 0;
}

// original name: Nas_MainCtrl
// Advances the player's fade (a finished fade-out disables the player and returns), refreshes its applied fade
// volume and runs Audio_SequenceChannelProcessSound over its enabled channels.
void Audio_SequencePlayerProcessSound(SequencePlayer* seqPlayer) {
    s32 i;

    if (seqPlayer->fadeTimer != 0) {
        seqPlayer->fadeVolume += seqPlayer->fadeVelocity;
        seqPlayer->recalculateVolume = 1;
        if (1.0f < seqPlayer->fadeVolume) {
            seqPlayer->fadeVolume = 1.0f;
        }
        if (seqPlayer->fadeVolume < 0.0f) {
            seqPlayer->fadeVolume = 0.0f;
        }
        seqPlayer->fadeTimer--;
        if (seqPlayer->fadeTimer == 0) {
            if (seqPlayer->state == SEQPLAYER_STATE_FADE_OUT) {
                AudioSeq_SequencePlayerDisable(seqPlayer);
                return;
            }
        }
    }

    if (seqPlayer->recalculateVolume) {
        seqPlayer->appliedFadeVolume = seqPlayer->fadeVolume * seqPlayer->fadeVolumeScale;
    }
    for (i = 0; i < 16; i++) {
        if (seqPlayer->channels[i]->h.f.enabled == 1) {
            Audio_SequenceChannelProcessSound(seqPlayer->channels[i], seqPlayer->recalculateVolume);
        }
    }
    seqPlayer->recalculateVolume = 0;
}

// original name: Nas_SweepCalculator
// One portamento step: advances the 8.8 fixed-point `cur` and returns 1 + extent * (ratio - 1) from the upper half
// of gBendPitchOneOctaveFrequencies, holding at its last entry (2.0) and clearing `mode` there.
f32 Audio_GetPortamentoFreqScale(Portamento* portamento) {
    s32 loResCur;
    f32 portamentoFreq;
    f32 one;

    loResCur = (portamento->cur += portamento->speed);
    loResCur = (loResCur >> 8) & 0xFF;
    if ((u32)loResCur >= 0x7F) {
        loResCur = 0x7F;
        portamento->mode = 0;
    }
    one = 1.0f;
    portamentoFreq = (gBendPitchOneOctaveFrequencies[loResCur + 128] - one) * portamento->extent + one;
    return portamentoFreq;
}

// original name: Nas_ModTableRead
// Advances the vibrato time by its rate and returns the 64-entry curve's sample >> 8, a signed offset from
// gBendPitchOneOctaveFrequencies' entry 128 (1.0f).
// Load-bearing: `vib->time` updated in place and read back, no `phase` local.
s16 Audio_GetVibratoPitchChange(VibratoState* vib) {
    s32 index;

    vib->time += (s32)vib->rate;
    index = vib->time >> 10;
    index &= 0x3F;
    return vib->curve[index] >> 8;
}

// original name: Nas_Modulator
// Counts the vibrato's start delay down, ramps depth and rate to the channel's targets (not for a channel of -1,
// OoT's `@bug`) and returns the curve's pitch ratio scaled by depth; 1.0 while delayed or at depth 0.
// Load-bearing: the channel on the left of both `!=` tests, each ramp's `n` with the dead `count =` in its else
// arm, and the tail's `result`.
f32 Audio_GetVibratoFreqScale(VibratoState* vib) {
    SequenceChannel* channel;
    u16 count;
    s32 n;
    f32 result;
    s32 index;
    f32 depth;

    count = vib->delay;
    channel = vib->channel;
    if (count != 0) {
        vib->delay = count - 1;
        return 1.0;
    }

    if (channel != (SequenceChannel*)-1) {
        if (vib->depthChangeTimer != 0) {
            n = vib->depthChangeTimer;
            if (vib->depthChangeTimer == 1) {
                vib->depth = (s32)channel->vibratoDepthTarget;
            } else {
                count = vib->depthChangeTimer;
                vib->depth = vib->depth + ((s32)channel->vibratoDepthTarget - vib->depth) / (s32)n;
            }
            vib->depthChangeTimer = vib->depthChangeTimer - 1;
        } else if (channel->vibratoDepthTarget != (s32)vib->depth) {
            count = channel->vibratoDepthChangeDelay;
            vib->depthChangeTimer = count;
            if (count == 0) {
                vib->depth = (s32)channel->vibratoDepthTarget;
            }
        }

        if (vib->rateChangeTimer != 0) {
            n = vib->rateChangeTimer;
            if (vib->rateChangeTimer == 1) {
                vib->rate = (s32)channel->vibratoRateTarget;
            } else {
                count = vib->rateChangeTimer;
                vib->rate = vib->rate + ((s32)channel->vibratoRateTarget - vib->rate) / (s32)n;
            }
            vib->rateChangeTimer = vib->rateChangeTimer - 1;
        } else if (channel->vibratoRateTarget != (s32)vib->rate) {
            count = channel->vibratoRateChangeDelay;
            vib->rateChangeTimer = count;
            if (count == 0) {
                vib->rate = (s32)channel->vibratoRateTarget;
            }
        }
    }

    if (0.0f == vib->depth) {
        return 1.0;
    }
    index = Audio_GetVibratoPitchChange(vib);
    depth = vib->depth * 0.000244140625f;
    result = (gBendPitchOneOctaveFrequencies[index + 128] - 1.0f) * depth + 1.0f;
    return result;
}

// original name: Nas_ChannelModulation
// Steps the note's portamento and vibrato where active and stores their frequency scales in the note.
void Audio_NoteVibratoUpdate(Note* note) {
    if (note->playbackState.portamento.mode != 0) {
        note->playbackState.portamentoFreqScale = Audio_GetPortamentoFreqScale(&note->playbackState.portamento);
    }
    if (note->playbackState.vibratoState.active != 0) {
        note->playbackState.vibratoFreqScale = Audio_GetVibratoFreqScale(&note->playbackState.vibratoState);
    }
}

// original name: Nas_ChannelModInit
// Arms the note's vibrato on the sine curve from its layer's channel: depth and rate start at their start values
// when they have a change delay, else at their targets.
void Audio_NoteVibratoInit(Note* note) {
    VibratoState* vib;
    SequenceChannel* channel;
    u16 count;

    note->playbackState.vibratoState.active = 1;
    note->playbackState.vibratoState.time = 0;
    note->playbackState.vibratoFreqScale = 1.0f;
    note->playbackState.vibratoState.curve = gWaveSamples[2];
    note->playbackState.vibratoState.channel = note->playbackState.parentLayer->channel;
    vib = &note->playbackState.vibratoState;
    channel = note->playbackState.vibratoState.channel;

    count = channel->vibratoDepthChangeDelay;
    vib->depthChangeTimer = count;
    if (count == 0) {
        vib->depth = (s32)channel->vibratoDepthTarget;
    } else {
        vib->depth = (s32)channel->vibratoDepthStart;
    }

    count = channel->vibratoRateChangeDelay;
    vib->rateChangeTimer = count;
    if (count == 0) {
        vib->rate = (s32)channel->vibratoRateTarget;
    } else {
        vib->rate = (s32)channel->vibratoRateStart;
    }

    vib->delay = channel->vibratoDelay;
}

// original name: Nas_SweepInit
// Arms the note's portamento: its freq scale back to 1.0 and its layer's portamento copied over it wholesale.
void Audio_NotePortamentoInit(Note* note) {
    note->playbackState.portamentoFreqScale = 1.0f;
    note->playbackState.portamento = note->playbackState.parentLayer->portamento;
}

// original name: Nas_EnvInit
// Resets an ADSR envelope onto a new point table; the unread third parameter is OoT's `volOut`, which SM64 stored.
void Audio_AdsrInit(AdsrState* adsr, EnvelopePoint* envelope, UNUSED s32 volOut) {
    adsr->h.b.action = 0;
    adsr->h.b.delay = 0;
    adsr->envelope = envelope;
    adsr->sustain = 0.0f;
    adsr->current = 0.0f;
}

// original name: Nas_EnvProcess
// Advances the ADSR envelope one tick through OoT's AdsrStatus states (ADSR_STATE_LOOP walks the point table) and
// returns its level clamped to [0, 1]; the PRINTF reports a level above 1.
// Load-bearing: `case ADSR_STATE_LOOP:` inside the `for (;;)`, the inner `switch` in its case order, `u8 state` set
// in one statement, and the default arm's longhand `adsr->envelope[adsr->h.b.envIndex].arg`.
f32 Audio_AdsrUpdate(AdsrState* adsr) {
    EnvelopePoint* p;
    u8 state;

    state = adsr->h.b.action & 0xF;
    switch (state) {
        case ADSR_STATE_DISABLED:
            return 0.0f;

        case ADSR_STATE_INITIAL:
            if (adsr->h.f.hang) {
                adsr->h.f.state = ADSR_STATE_HANG;
                break;
            }
            /* fallthrough */
        case ADSR_STATE_START_LOOP:
            adsr->h.b.envIndex = 0;
            adsr->h.f.state = ADSR_STATE_LOOP;
            /* fallthrough */
            for (;;) {
                case ADSR_STATE_LOOP:
                    p = &adsr->envelope[adsr->h.b.envIndex];
                    adsr->h.b.delay = p->delay;
                    switch (adsr->h.b.delay) {
                        case ADSR_DISABLE:
                            adsr->h.f.state = ADSR_STATE_DISABLED;
                            break;
                        case ADSR_HANG:
                            adsr->h.f.state = ADSR_STATE_HANG;
                            break;
                        case ADSR_GOTO:
                            adsr->h.b.envIndex = p->arg;
                            continue;
                        case ADSR_RESTART:
                            adsr->h.f.state = ADSR_STATE_INITIAL;
                            break;
                        default:
                            adsr->h.b.delay = adsr->h.b.delay * gAudioCtx.audioBufferParameters.updatesPerFrameScaled;
                            if (adsr->h.b.delay == 0) {
                                adsr->h.b.delay = 1;
                            }
                            adsr->target = adsr->envelope[adsr->h.b.envIndex].arg / 32767.0f;
                            adsr->h.f.state = ADSR_STATE_FADE;
                            adsr->h.b.envIndex++;
                            adsr->target = adsr->target * adsr->target;
                            adsr->velocity = (adsr->target - adsr->current) / adsr->h.b.delay;
                            break;
                    }
                    break;
            }
            if (adsr->h.f.state != ADSR_STATE_FADE) {
                break;
            }
            /* fallthrough */
        case ADSR_STATE_FADE:
            adsr->h.b.delay--;
            adsr->current += adsr->velocity;
            if (adsr->h.b.delay <= 0) {
                adsr->h.f.state = ADSR_STATE_LOOP;
            }
            break;

        case ADSR_STATE_DECAY:
        case ADSR_STATE_RELEASE:
            adsr->current -= adsr->fadeOutVel;
            if (0.0f != adsr->sustain && state == ADSR_STATE_DECAY) {
                if (adsr->current < adsr->sustain) {
                    adsr->current = adsr->sustain;
                    adsr->h.b.delay = 0x80;
                    adsr->h.f.state = ADSR_STATE_SUSTAIN;
                }
            } else if (adsr->current < 1e-05f) {
                adsr->current = 0.0f;
                adsr->h.f.state = ADSR_STATE_DISABLED;
            }
            break;

        case ADSR_STATE_SUSTAIN:
            adsr->h.b.delay--;
            if (adsr->h.b.delay == 0) {
                adsr->h.f.state = ADSR_STATE_RELEASE;
            }
            break;
    }

    if (adsr->h.f.decay) {
        adsr->h.f.state = ADSR_STATE_DECAY;
        adsr->h.f.decay = 0;
    }
    if (adsr->h.f.release) {
        adsr->h.f.state = ADSR_STATE_RELEASE;
        adsr->h.f.release = 0;
    }

    if (adsr->current < 0.0f) {
        return 0.0f;
    }
    if (1.0f < adsr->current) {
        PRINTF("Audio:Envp: overflow  %f\n");
        return 1.0f;
    }
    return adsr->current;
}

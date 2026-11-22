// original filename: channel.c
#include "global.h"
#include "audio.h"

// NA audio driver, OoT's playback.c: the per-frame note update and the sample-state setup, the instrument and
// drum lookups, layer release, synthetic waves, and the note lists, pools and allocator.
// The PRINTFs are stripped debug prints whose strings stay in .data (-use_readwrite_const), in the ROM's order.

// The stereo byte as a 1-byte aggregate, as OoT's: Audio_InitSampleState's and Audio_ProcessNotes' stereo
// copies are whole-struct assignments through it, until the stereo fields are declared with the type.
typedef struct Stereo {
    u8 asByte;
} Stereo;

// original name: Nas_smzSetParam
// Sets the sample state for this update from the note and `attrs`: the note's flags and tuned sample, the
// resampling rate, the output mode's pan curve and strong-side bits, and the velocity-scaled target volumes.
// Load-bearing: the whole-struct stereo copy; `if (hw) {}` above the four copies, `& 0xFF` on the first two only;
// `vel = attrs->velocity;` first; `u32 strongLeft/strongRight`; `volRight = volLeft = 0.707f;`.
void Audio_InitSampleState(Note* note, NoteSampleState* sampleState, NoteSampleStateAttributes* attrs) {
    NoteSampleState* hw;
    f32 volLeft;
    f32 volRight;
    s32 halfPanIndex;
    u32 strongLeft;
    u32 strongRight;
    UNUSED s32 unused;
    f32 vel;
    StereoPanWord p;

    vel = attrs->velocity;
    p.b.pan = attrs->pan;
    p.b.reverbVol = attrs->reverbVol;
    *(Stereo*)&p.b.stereo = *(Stereo*)&attrs->stereo;
    hw = &note->sampleState;
    if (hw) {}
    sampleState->h.b.bitField0 = hw->h.b.bitField0 & 0xFF;
    sampleState->h.b.bitField1 = note->sampleState.h.b.bitField1 & 0xFF;
    sampleState->tunedSample = note->sampleState.tunedSample;
    sampleState->harmonicIndexCurAndPrev = note->sampleState.harmonicIndexCurAndPrev;
    Audio_NoteSetResamplingRate(sampleState, attrs->frequency);

    p.b.pan &= 0x7F;
    if (hw->h.f.stereoHeadsetEffects && gAudioCtx.soundOutputMode == SOUND_OUTPUT_HEADSET) {
        halfPanIndex = p.b.pan >> 1;
        if (halfPanIndex >= 0x40) {
            halfPanIndex = 0x3F;
        }
        sampleState->haasEffectRightDelaySize = gHaasEffectDelaySizes[halfPanIndex];
        sampleState->h.b.haasEffectLeftDelaySize = gHaasEffectDelaySizes[0x3F - halfPanIndex];
        sampleState->h.f.stereoStrongRight = 0;
        sampleState->h.f.stereoStrongLeft = 0;
        sampleState->h.f.usesHeadsetPanEffects = 1;
        volLeft = gHeadsetPanVolume[p.b.pan];
        volRight = gHeadsetPanVolume[0x7F - p.b.pan];
    } else if (hw->h.f.stereoHeadsetEffects && gAudioCtx.soundOutputMode == SOUND_OUTPUT_STEREO) {
        sampleState->h.b.haasEffectLeftDelaySize = 0;
        sampleState->haasEffectRightDelaySize = 0;
        sampleState->h.f.usesHeadsetPanEffects = 0;
        strongRight = 0;
        strongLeft = 0;
        volLeft = gStereoPanVolume[p.b.pan];
        volRight = gStereoPanVolume[0x7F - p.b.pan];
        if (p.b.pan < 0x20) {
            strongLeft = 1;
        } else if (p.b.pan >= 0x61) {
            strongRight = 1;
        }
        sampleState->h.f.stereoStrongRight = strongRight;
        sampleState->h.f.stereoStrongLeft = strongLeft;

        switch (p.f.bit2) {
            case 0:
                sampleState->h.f.stereoStrongRight = p.f.strongRight;
                sampleState->h.f.stereoStrongLeft = p.f.strongLeft;
                break;
            case 1:
                break;
            case 2:
                sampleState->h.f.stereoStrongRight = p.f.strongRight | strongRight;
                sampleState->h.f.stereoStrongLeft = p.f.strongLeft | strongLeft;
                break;
            case 3:
                sampleState->h.f.stereoStrongRight = p.f.strongRight ^ strongRight;
                sampleState->h.f.stereoStrongLeft = p.f.strongLeft ^ strongLeft;
                break;
        }
    } else if (gAudioCtx.soundOutputMode == SOUND_OUTPUT_MONO) {
        volRight = volLeft = 0.707f;
    } else {
        volLeft = gDefaultPanVolume[p.b.pan];
        volRight = gDefaultPanVolume[0x7F - p.b.pan];
    }

    if (vel < 0.0f) {
        PRINTF("Audio: setvol: volume minus %f\n");
        vel = 0.0f;
    }
    if (1.0f < vel) {
        PRINTF("Audio: setvol: volume overflow %f\n");
        vel = 1.0f;
    }
    sampleState->targetVolLeft = vel * volLeft * 4095.999f;
    sampleState->targetVolRight = vel * volRight * 4095.999f;
    sampleState->h.b.gain = attrs->gain;
    sampleState->filter = attrs->filter;
    sampleState->reverbVol = p.b.reverbVol;
}

// original name: Nas_smzSetPitch
// Sets the sample state's 1.15 fixed-point resampling step: rates of 2 and up are halved and flagged hasTwoParts,
// and both ranges clamp at 1.99998. The `<= 0.0f` test only held a print.
void Audio_NoteSetResamplingRate(NoteSampleState* sampleState, f32 resamplingRateInput) {
    f32 resamplingRate;

    if (resamplingRateInput <= 0.0f) {
        PRINTF("Audio: setpitch: pitch zero or minus %f\n");
    }
    if (resamplingRateInput < 2.0f) {
        sampleState->h.f.hasTwoParts = 0;
        if (1.99998f < resamplingRateInput) {
            resamplingRate = 1.99998f;
        } else {
            resamplingRate = resamplingRateInput;
        }
    } else {
        sampleState->h.f.hasTwoParts = 1;
        if (3.99996f < resamplingRateInput) {
            resamplingRate = 1.99998f;
        } else {
            resamplingRate = resamplingRateInput * 0.5f;
        }
    }
    sampleState->resamplingRateFixedPoint = (s16)(resamplingRate * 32768.0f);
}

// original name: Nas_StartVoice
// Starts the note's envelope from its layer's, or its channel's when the layer's decayIndex is 0, and resets the
// status, the envelope state (ADSR_STATE_INITIAL) and the sample state (gDefaultNoteSampleState).
void Audio_NoteInit(Note* note) {
    SequenceLayer* layer;

    layer = note->playbackState.parentLayer;
    if (layer->adsr.decayIndex == 0) {
        Audio_AdsrInit(&note->playbackState.adsr, layer->channel->adsr.envelope,
                       (s32)&note->playbackState.adsrVolScaleUnused);
    } else {
        Audio_AdsrInit(&note->playbackState.adsr, layer->adsr.envelope, (s32)&note->playbackState.adsrVolScaleUnused);
    }
    note->playbackState.status = 0;
    note->playbackState.adsr.h.f.state = ADSR_STATE_INITIAL;
    note->sampleState = gDefaultNoteSampleState;
}

// original name: Nas_StopVoice
// Silences a note: clears its priority, status and needsInit/enabled/finished flags, drops its layers and puts its
// envelope back to ADSR_STATE_DISABLED at level 0.
// Load-bearing: the `volatile` on the `enabled` bit, as OoT's.
void Audio_NoteDisable(Note* note) {
    if (note->sampleState.h.f.needsInit == 1) {
        note->sampleState.h.f.needsInit = 0;
    }
    note->playbackState.priority = 0;
    note->sampleState.h.f.enabled = 0;
    note->playbackState.status = 0;
    note->sampleState.h.f.finished = 0;
    note->playbackState.parentLayer = NO_LAYER;
    note->playbackState.prevParentLayer = NO_LAYER;
    note->playbackState.adsr.h.f.state = ADSR_STATE_DISABLED;
    note->playbackState.adsr.current = 0.0f;
}

// original name: Nas_UpdateChannel
// The per-frame note pass: releases notes whose layer was disabled, muted or moved to another note, frees finished
// notes or starts their wanted layer, and sets each live note's sample state from envelope, vibrato and attributes.
// Load-bearing: `(playbackState->adsr.h.b.action & 0xFF) | 0x10` with the fadeOutVel store second; `note` before
// `sampleState2`; the top `if`'s arm order; `u8 state` in the inner test only; `u8 bookOffset`; `goto inner_end`;
// `attrs` set after label_L8000B628, above the status test; `channel` read through `playbackState->parentLayer`;
// the whole-struct stereo copies, before `reverbVol` in the second arm; `channel->seqPlayer` inline with
// `seqPlayer` declared.
void Audio_ProcessNotes(void) {
    Note* note;
    NotePlaybackState* playbackState;
    s32 i;
    NoteSampleState* sampleState2;
    SequenceLayer* layer;
    SequenceChannel* channel;
    UNUSED SequencePlayer* seqPlayer;
    NoteSampleStateAttributes sampleStateAttrs;
    u8 bookOffset;
    f32 scale;
    NoteSampleState* sampleState;
    NoteAttributes* attrs;
    u8 state;

    for (i = 0; i < gAudioCtx.numNotes; i++) {
        note = &gAudioCtx.notes[i];
        sampleState2 = &gAudioCtx.sampleStates[gAudioCtx.sampleStateOffset + i];
        playbackState = &note->playbackState;

        if (playbackState->parentLayer != NO_LAYER) {
            if ((u32)playbackState->parentLayer < 0x7FFFFFFFU) {
                continue;
            }
            if (note != playbackState->parentLayer->note) {
                if (playbackState->status == 0) {
                    PRINTF("----------------------Double-Error CH: %x %f\n");
                    PRINTF("----------------------Double-Error NT: %x\n");
                    playbackState->adsr.h.b.action = (playbackState->adsr.h.b.action & 0xFF) | 0x10;
                    playbackState->adsr.fadeOutVel = gAudioCtx.audioBufferParameters.updatesPerFrameInv;
                    playbackState->priority = 1;
                    playbackState->status = 2;
                    goto label_L8000B4D0;
                }
            }
            if (playbackState->parentLayer->h.f.enabled || playbackState->status || playbackState->priority <= 0) {
                if (playbackState->parentLayer->channel->seqPlayer == NULL) {
                    PRINTF("CAUTION:SUB IS SEPARATED FROM GROUP\n");
                    AudioSeq_SequenceChannelDisable(playbackState->parentLayer->channel);
                    playbackState->priority = 1;
                    playbackState->status = 1;
                    continue;
                }
                if (!(playbackState->parentLayer->channel->seqPlayer->muted &&
                      (playbackState->parentLayer->channel->h.b.muteBehavior & 0x40))) {
                    goto label_L8000B4D0;
                }
                PRINTF("CAUTION:PAUSE EMERGENCY\n");
            }
            Audio_SeqLayerNoteRelease(playbackState->parentLayer);
            Audio_AudioListRemove(&note->listItem);
            Audio_AudioListPushFront(note->listItem.pool + 1, &note->listItem);
            playbackState->priority = 1;
            playbackState->status = 2;
        } else {
            if (playbackState->status == 0) {
                if (playbackState->priority > 0) {
                    continue;
                }
            }
        }

    label_L8000B4D0:
        sampleState = &note->sampleState;
        if (playbackState->priority != 0) {
            state = playbackState->status;
            if (state > 0) {
                // falls through
            } else if (sampleState->h.f.finished == 0) {
                goto label_L8000B5E0;
            }

            if ((playbackState->adsr.h.b.action & 0xF) == ADSR_STATE_DISABLED) {
                goto handle_stolen;
            }
            if (sampleState->h.f.finished == 0) {
                goto label_L8000B628;
            }

        handle_stolen:
            if (playbackState->wantedParentLayer != NO_LAYER) {
                Audio_NoteDisable(note);
                if (playbackState->wantedParentLayer->channel != NULL) {
                    Audio_NoteInitForLayer(note, playbackState->wantedParentLayer);
                    Audio_NoteVibratoInit(note);
                    Audio_NotePortamentoInit(note);
                    Audio_AudioListRemove(&note->listItem);
                    AudioSeq_AudioListPushBack(note->listItem.pool + 3, &note->listItem);
                    playbackState->wantedParentLayer = NO_LAYER;
                    goto label_L8000B628;
                } else {
                    PRINTF("Error:Wait Track disappear\n");
                    Audio_NoteDisable(note);
                    Audio_AudioListRemove(&note->listItem);
                    AudioSeq_AudioListPushBack(note->listItem.pool, &note->listItem);
                    playbackState->wantedParentLayer = NO_LAYER;
                    goto inner_end;
                }
            } else {
                if (playbackState->parentLayer != NO_LAYER) {
                    playbackState->parentLayer->h.f.noteDisabled = 1;
                }
                Audio_NoteDisable(note);
                Audio_AudioListRemove(&note->listItem);
                AudioSeq_AudioListPushBack(note->listItem.pool, &note->listItem);
                continue;
            }

        label_L8000B5E0:
            if ((playbackState->adsr.h.b.action & 0xF) == ADSR_STATE_DISABLED) {
                if (playbackState->parentLayer != NO_LAYER) {
                    playbackState->parentLayer->h.f.noteDisabled = 1;
                }
                Audio_NoteDisable(note);
                Audio_AudioListRemove(&note->listItem);
                AudioSeq_AudioListPushBack(note->listItem.pool, &note->listItem);
                continue;
            }
            goto label_L8000B628;

        label_L8000B628:
            attrs = &playbackState->attributes;
            scale = Audio_AdsrUpdate(&playbackState->adsr);
            Audio_NoteVibratoUpdate(note);

            if (playbackState->status == 1 || playbackState->status == 2) {
                sampleStateAttrs.frequency = attrs->freqScale;
                sampleStateAttrs.velocity = attrs->velocity;
                sampleStateAttrs.pan = attrs->pan;
                sampleStateAttrs.reverbVol = attrs->reverb;
                *(Stereo*)&sampleStateAttrs.stereo = *(Stereo*)&attrs->stereo;
                sampleStateAttrs.gain = attrs->gain;
                sampleStateAttrs.filter = attrs->filter;
                bookOffset = sampleState->h.f.bookOffset;
            } else {
                layer = playbackState->parentLayer;
                channel = playbackState->parentLayer->channel;

                sampleStateAttrs.frequency = layer->noteFreqScale;
                sampleStateAttrs.velocity = layer->noteVelocity;
                sampleStateAttrs.pan = layer->notePan;
                *(Stereo*)&sampleStateAttrs.stereo = *(Stereo*)&layer->h.b.stereo;
                sampleStateAttrs.reverbVol = channel->targetReverbVol;
                sampleStateAttrs.gain = channel->gain;
                sampleStateAttrs.filter = channel->filter;

                bookOffset = channel->bookOffset & 0x7;
                if (channel->seqPlayer->muted && (channel->h.b.muteBehavior & 0x8)) {
                    sampleStateAttrs.frequency = 0.0f;
                    sampleStateAttrs.velocity = 0.0f;
                }
            }

            sampleStateAttrs.frequency *= playbackState->vibratoFreqScale * playbackState->portamentoFreqScale;
            sampleStateAttrs.frequency *= gAudioCtx.audioBufferParameters.resampleRate;
            sampleStateAttrs.velocity *= scale;

            Audio_InitSampleState(note, sampleState2, &sampleStateAttrs);
            sampleState->h.f.bookOffset = bookOffset;
        inner_end:;
        }
    }
}

// original name: NoteToVoice
// Picks the instrument's tuned sample for `semitone`: the low-pitch one below normalRangeLo, the normal one up to
// normalRangeHi, else the high one.
// Load-bearing: one variable through the if/else-if/else chain and a single `return`.
TunedSample* Audio_GetInstrumentTunedSample(Instrument* instrument, s32 semitone) {
    TunedSample* tunedSample;

    if (semitone < instrument->normalRangeLo) {
        tunedSample = &instrument->lowPitchTunedSample;
    } else if (semitone <= instrument->normalRangeHi) {
        tunedSample = &instrument->normalPitchTunedSample;
    } else {
        tunedSample = &instrument->highPitchTunedSample;
    }
    return tunedSample;
}

// original name: ProgToVp
// Returns instrument `instId` of font `fontId`, or NULL for font 0xFF, an unloaded font, an index out of range or
// an empty slot, the last three with their audioErrorFlags code (0x10, 0x03, 0x01).
void* Audio_GetInstrumentInner(s32 fontId, s32 instId) {
    void* inst;

    if (fontId == 0xFF) {
        return NULL;
    }
    if (AudioLoad_IsFontLoadComplete(fontId) == 0) {
        PRINTF("Audio: voiceman: No bank error %d\n");
        gAudioCtx.audioErrorFlags = fontId + 0x10000000;
        return NULL;
    }
    if (instId >= gAudioCtx.soundFontList[fontId].numInstruments) {
        PRINTF("Audio: voiceman: progNo. overflow %d,%d\n");
        gAudioCtx.audioErrorFlags = (fontId << 8) + instId + 0x3000000;
        return NULL;
    }
    inst = gAudioCtx.soundFontList[fontId].instruments[instId];
    PRINTF("ptr2 %x\n");
    if (inst == NULL) {
        PRINTF("Audio: voiceman: progNo. undefined %d,%d\n");
        gAudioCtx.audioErrorFlags = (fontId << 8) + instId + 0x1000000;
        return inst;
    }
    return inst;
}

// original name: PercToPp
// The same lookup for drum `drumId`: NULL for font 0xFF, an unloaded font (0x10), an index out of range (0x04), an
// unrelocated drum table, or an empty slot (0x05).
void* Audio_GetDrum(s32 fontId, s32 drumId) {
    void** table;
    void* drum;

    if (fontId == 0xFF) {
        return NULL;
    }
    if (AudioLoad_IsFontLoadComplete(fontId) == 0) {
        PRINTF("Audio: voiceman: No bank error %d\n");
        gAudioCtx.audioErrorFlags = fontId + 0x10000000;
        return NULL;
    }
    if (drumId >= gAudioCtx.soundFontList[fontId].numDrums) {
        PRINTF("Audio: voiceman: Percussion Overflow %d,%d\n");
        gAudioCtx.audioErrorFlags = (fontId << 8) + drumId + 0x4000000;
        return NULL;
    }
    table = gAudioCtx.soundFontList[fontId].drums;
    if ((u32)table < 0x80000000) {
        PRINTF("Audio: voiceman: Percussion table pointer (bank %d) is irregular %x.\n");
        return NULL;
    }
    drum = table[drumId];
    if (drum == NULL) {
        PRINTF("Audio: voiceman: Percpointer NULL %d,%d\n");
        gAudioCtx.audioErrorFlags = (fontId << 8) + drumId + 0x5000000;
    }
    return drum;
}

// original name: __Nas_Release_Channel_Main
// Lets go of the layer's note toward `target`, ADSR_STATE_RELEASE or _DECAY: its attributes cached on the note, the
// layer kept as prevParentLayer, a release or a decay-table fade, and a decay moved to the decaying list.
// Load-bearing: `(s32)` on the envelope-state test; `attrs` set at the top; `& 0xFF` on the stereo copy; each
// fadeOutVel store before its flag store.
void Audio_SeqLayerDecayRelease(SequenceLayer* layer, s32 target) {
    Note* note;
    NoteAttributes* attrs;

    PRINTF("--4 %x\n");
    if (layer == NO_LAYER) {
        return;
    }
    layer->h.f.unk0_4 = 0;
    if (layer->note == NULL) {
        return;
    }
    note = layer->note;
    attrs = &note->playbackState.attributes;
    if (layer == note->playbackState.wantedParentLayer) {
        PRINTF("NoteOff Comes during wait release %x (note %x)\n");
        note->playbackState.wantedParentLayer = NO_LAYER;
    }

    if (layer != note->playbackState.parentLayer) {
        if (note->playbackState.parentLayer != NO_LAYER) {
            return;
        }
        if (note->playbackState.wantedParentLayer != NO_LAYER) {
            return;
        }
        if (layer != note->playbackState.prevParentLayer) {
            return;
        }
        if (target == ADSR_STATE_DECAY) {
            return;
        }
        PRINTF("Slow Release Batting\n");
        note->playbackState.adsr.fadeOutVel = gAudioCtx.audioBufferParameters.updatesPerFrameInv;
        note->playbackState.adsr.h.f.release = 1;
        return;
    }

    if ((s32)note->playbackState.adsr.h.f.state != ADSR_STATE_DECAY) {
        attrs->freqScale = layer->noteFreqScale;
        attrs->velocity = layer->noteVelocity;
        attrs->pan = layer->notePan;
        attrs->stereo = layer->h.b.stereo & 0xFF;
        if (layer->channel != NULL) {
            attrs->reverb = layer->channel->targetReverbVol;
            attrs->gain = layer->channel->gain;
            attrs->filter = layer->channel->filter;
            if (layer->channel->seqPlayer->muted && (layer->channel->h.b.muteBehavior & 8)) {
                note->sampleState.h.f.finished = 1;
            }
            note->playbackState.priority = layer->channel->someOtherPriority;
        } else {
            note->playbackState.priority = 1;
        }
        note->playbackState.prevParentLayer = note->playbackState.parentLayer;
        note->playbackState.parentLayer = NO_LAYER;
        if (target == ADSR_STATE_RELEASE) {
            note->playbackState.adsr.fadeOutVel = gAudioCtx.audioBufferParameters.updatesPerFrameInv;
            note->playbackState.adsr.h.f.release = 1;
            note->playbackState.status = 2;
        } else {
            note->playbackState.status = 1;
            note->playbackState.adsr.h.f.decay = 1;
            if (layer->adsr.decayIndex == 0) {
                note->playbackState.adsr.fadeOutVel = gAudioCtx.adsrDecayTable[layer->channel->adsr.decayIndex];
            } else {
                note->playbackState.adsr.fadeOutVel = gAudioCtx.adsrDecayTable[layer->adsr.decayIndex];
            }
            note->playbackState.adsr.sustain =
                (s32)layer->channel->adsr.sustain * note->playbackState.adsr.current * 0.00390625f;
        }
    }

    if (target == ADSR_STATE_DECAY) {
        Audio_AudioListRemove(&note->listItem);
        Audio_AudioListPushFront(&note->listItem.pool[1], &note->listItem);
    }
}

// original name: Nas_Release_Channel
// Decays the layer's note: it moves to its pool's decaying list (target ADSR_STATE_DECAY).
void Audio_SeqLayerNoteDecay(SequenceLayer* layer) {
    Audio_SeqLayerDecayRelease(layer, ADSR_STATE_DECAY);
}

// original name: Nas_Release_Channel_Force
// Releases the layer's note: fast fade at the release rate (target ADSR_STATE_RELEASE).
void Audio_SeqLayerNoteRelease(SequenceLayer* layer) {
    Audio_SeqLayerDecayRelease(layer, ADSR_STATE_RELEASE);
}

// Picks the harmonic of synthetic wave `waveId` (at least 0x80) for the layer's pitch and portamento peak, scales
// freqScale by the harmonic's ratio and points the sample state at it; returns the harmonic index.
// Load-bearing: the redundant `rate = layer->freqScale;` in the portamento guard, and `test *=`.
s32 Audio_BuildSyntheticWave(Note* note, SequenceLayer* layer, s32 waveId) {
    f32 rate;
    f32 test;
    s32 harmonicIndex;

    harmonicIndex = 0;
    if (waveId < 0x80) {
        PRINTF("Audio:Wavemem: Bad voiceno (%d)\n");
        waveId = 0x80;
    }

    rate = layer->freqScale;
    test = rate;
    if (layer->portamento.mode != 0 && 0.0f < layer->portamento.extent) {
        PRINTF("Basepitch %f , sweep %f\n");
        rate = layer->freqScale;
        test *= layer->portamento.extent + 1.0f;
    }

    if (test < 0.99999f) {
        test = 1.0465f;
    } else if (test < 1.99999f) {
        harmonicIndex = 1;
        test = 0.52325f;
    } else if (test < 3.99999f) {
        harmonicIndex = 2;
        test = 0.26263f;
    } else {
        harmonicIndex = 3;
        test = 0.13081f;
    }

    layer->freqScale = rate * test;
    note->playbackState.waveId = waveId;
    note->playbackState.harmonicIndex = harmonicIndex;
    note->sampleState.waveSampleAddr = &gWaveSamples[waveId - 128][harmonicIndex * 64];
    return harmonicIndex;
}

// Re-picks the synthetic-wave harmonic of a legato note for the layer's new pitch and, when it changed, records it
// as cur * 4 + prev in the sample state's harmonicIndexCurAndPrev.
void Audio_InitSyntheticWave(Note* note, SequenceLayer* layer) {
    s32 prevHarmonicIndex;
    s32 waveId;
    s32 curHarmonicIndex;

    waveId = layer->h.b.instOrWave;
    if (waveId == 0xFF) {
        waveId = layer->channel->instOrWave;
    }
    prevHarmonicIndex = note->playbackState.harmonicIndex;
    curHarmonicIndex = Audio_BuildSyntheticWave(note, layer, waveId);
    if (curHarmonicIndex != prevHarmonicIndex) {
        PRINTF("%x %d LOD %d - > %d , \n");
        note->sampleState.harmonicIndexCurAndPrev = curHarmonicIndex * 4 + prevHarmonicIndex;
    }
}

// original name: __Nas_InitList
// Makes an empty circular list: the head points at itself, count 0.
void Audio_InitNoteList(AudioListItem* list) {
    list->prev = list;
    list->next = list;
    list->u.count = 0;
}

// original name: Nas_InitChNode
// Inits a note pool's four lists (disabled, decaying, releasing, active), each pointing back at the pool.
void Audio_InitNoteLists(AudioListItem* pool) {
    Audio_InitNoteList(&pool[0]);
    Audio_InitNoteList(&pool[1]);
    Audio_InitNoteList(&pool[2]);
    Audio_InitNoteList(&pool[3]);
    pool[0].pool = pool;
    pool[1].pool = pool;
    pool[2].pool = pool;
    pool[3].pool = pool;
}

// original name: Nas_InitChannelList
// Inits the global free lists (gAudioCtx.noteFreeLists) and puts every note of the driver on `disabled`.
void Audio_InitNoteFreeList(void) {
    s32 i;

    Audio_InitNoteLists(gAudioCtx.noteFreeLists);
    for (i = 0; i < gAudioCtx.numNotes; i++) {
        gAudioCtx.notes[i].listItem.u.note = &gAudioCtx.notes[i];
        gAudioCtx.notes[i].listItem.prev = NULL;
        AudioSeq_AudioListPushBack(&gAudioCtx.noteFreeLists[0], &gAudioCtx.notes[i].listItem);
    }
}

// original name: Nas_DeAllocAllVoices
// Moves every note of the pool's four lists onto the matching global free list.
// Load-bearing: the inner `for (;;)` with its leading `break`s.
void Audio_NotePoolClear(AudioListItem* pool) {
    AudioListItem* cur;
    AudioListItem* source;
    s32 i;
    AudioListItem* dest;

    for (i = 0; i < 4; i++) {
        switch (i) {
            case 0:
                source = &pool[0];
                dest = &gAudioCtx.noteFreeLists[0];
                break;
            case 1:
                source = &pool[1];
                dest = &gAudioCtx.noteFreeLists[1];
                break;
            case 2:
                source = &pool[2];
                dest = &gAudioCtx.noteFreeLists[2];
                break;
            case 3:
                source = &pool[3];
                dest = &gAudioCtx.noteFreeLists[3];
                break;
        }
        for (;;) {
            cur = source->next;
            if (cur == source) {
                break;
            }
            if (cur == NULL) {
                PRINTF("Audio: C-Alloc : Dealloc voice is NULL\n");
                break;
            }
            Audio_AudioListRemove(cur);
            AudioSeq_AudioListPushBack(dest, cur);
        }
    }
}

// original name: Nas_AllocVoices
// Clears the pool, then moves `count` notes into it from the global free lists, each into the pool's matching list,
// going on to the next list when one runs dry.
// Load-bearing: `note, j, i` declared ahead of `source` and `dest`.
void Audio_NotePoolFill(AudioListItem* pool, s32 count) {
    Note* note;
    s32 j;
    s32 i;
    AudioListItem* source;
    AudioListItem* dest;

    Audio_NotePoolClear(pool);
    i = 0;
    j = 0;
    while (j < count) {
        if (i == 4) {
            PRINTF("Alloc Error:Dim voice-Alloc %d");
            return;
        }
        switch (i) {
            case 0:
                source = &gAudioCtx.noteFreeLists[0];
                dest = &pool[0];
                break;
            case 1:
                source = &gAudioCtx.noteFreeLists[1];
                dest = &pool[1];
                break;
            case 2:
                source = &gAudioCtx.noteFreeLists[2];
                dest = &pool[2];
                break;
            case 3:
                source = &gAudioCtx.noteFreeLists[3];
                dest = &pool[3];
                break;
        }
        while (j < count) {
            note = AudioSeq_AudioListPopBack(source);
            if (note == NULL) {
                break;
            }
            AudioSeq_AudioListPushBack(dest, &note->listItem);
            j++;
        }
        i++;
    }
}

// original name: Nas_AddListHead
// Links `item` in at the front of `list`, right after the head, and gives it the list's pool; an item already in a
// list (prev set) is left alone.
void Audio_AudioListPushFront(AudioListItem* list, AudioListItem* item) {
    if (item->prev == NULL) {
        item->prev = list;
        item->next = list->next;
        list->next->prev = item;
        list->next = item;
        list->u.count++;
        item->pool = list->pool;
    } else {
        PRINTF("Error:Same List Add\n");
    }
}

// original name: Nas_CutList
// Unlinks `item` from its list and marks it free (prev = NULL); the head's count is not decremented.
void Audio_AudioListRemove(AudioListItem* item) {
    if (item->prev != NULL) {
        item->prev->next = item->next;
        item->next->prev = item->prev;
        item->prev = NULL;
    } else {
        PRINTF("Already Cut\n");
    }
}

// original name: __Nas_GetLowerPrio
// Returns the list's lowest-priority note, the last of equals, if its priority is below `limit`; else NULL.
Note* Audio_FindNodeWithPrioLessThan(AudioListItem* list, s32 limit) {
    AudioListItem* cur;
    AudioListItem* best;
    Note* note;

    cur = list->next;
    if (cur == list) {
        return NULL;
    }
    for (best = cur; cur != list; cur = cur->next) {
        if (best->u.note->playbackState.priority >= cur->u.note->playbackState.priority) {
            best = cur;
        }
    }
    if (best == NULL) {
        return NULL;
    }
    note = best->u.note;
    if (note->playbackState.priority >= limit) {
        return NULL;
    }
    return note;
}

// original name: Nas_EntryTrack
// Binds `note` to `layer` and starts it: links the two, records them on the channel, starts the envelope and sets
// up the sample state from the layer's tuned sample or, for ids 0x80..0xBF, its synthetic wave.
// Load-bearing: `pad0..pad3`.
void Audio_NoteInitForLayer(Note* note, SequenceLayer* layer) {
    NoteSampleState* sampleState;
    s32 instId;
    UNUSED s32 pad0;
    UNUSED s32 pad1;
    UNUSED s32 pad2;
    UNUSED s32 pad3;

    sampleState = &note->sampleState;
    note->playbackState.prevParentLayer = NO_LAYER;
    note->playbackState.parentLayer = layer;
    note->playbackState.priority = layer->channel->notePriority;
    layer->note = note;
    layer->h.f.notePropertiesNeedInit = 1;
    layer->h.f.unk0_4 = 1;
    layer->channel->noteUnused = note;
    layer->channel->layerUnused = layer;
    layer->noteVelocity = 0.0f;
    Audio_NoteInit(note);

    instId = layer->h.b.instOrWave;
    if (instId == 0xFF) {
        instId = layer->channel->instOrWave;
    }
    sampleState->tunedSample = layer->tunedSample;

    if (instId >= 0x80 && instId < 0xC0) {
        sampleState->h.f.isSyntheticWave = 1;
    } else {
        sampleState->h.f.isSyntheticWave = 0;
    }
    if (sampleState->h.f.isSyntheticWave != 0) {
        Audio_BuildSyntheticWave(note, layer, instId);
    }
    note->playbackState.fontId = layer->channel->fontId;
    sampleState->h.f.stereoHeadsetEffects = layer->channel->h.f.stereoHeadsetEffects;
    sampleState->h.f.reverbIndex = layer->channel->reverbIndex & 3;
}

// original name: __Nas_InterTrack
// Releases the note's current layer and queues `layer` as its wantedParentLayer (OoT's func_800E82C0).
void Audio_NoteInterruptForLayer(Note* note, SequenceLayer* layer) {
    Audio_SeqLayerNoteRelease(note->playbackState.parentLayer);
    note->playbackState.wantedParentLayer = layer;
}

// original name: __Nas_InterReleaseTrack
// Queues `layer` on a note already on its way out (wantedParentLayer), takes the layer's priority and forces a
// release at the standard rate.
void Audio_NoteReleaseAndTakeOwnership(Note* note, SequenceLayer* layer) {
    note->playbackState.wantedParentLayer = layer;
    note->playbackState.priority = layer->channel->notePriority;
    note->playbackState.adsr.fadeOutVel = gAudioCtx.audioBufferParameters.updatesPerFrameInv;
    note->playbackState.adsr.h.f.release = 1;
}

// Allocation attempt 1: a note off the pool's disabled list, started for the layer and put on the front of active.
Note* Audio_AllocNoteFromDisabled(AudioListItem* pool, SequenceLayer* layer) {
    Note* note;

    note = AudioSeq_AudioListPopBack(&pool[0]);
    if (note != NULL) {
        Audio_NoteInitForLayer(note, layer);
        Audio_AudioListPushFront(&pool[3], &note->listItem);
    }
    return note;
}

// Allocation attempt 2: a note off the pool's decaying list, with the layer queued behind it, moved to releasing.
Note* Audio_AllocNoteFromDecaying(AudioListItem* pool, SequenceLayer* layer) {
    Note* note;

    note = AudioSeq_AudioListPopBack(&pool[1]);
    if (note != NULL) {
        Audio_NoteReleaseAndTakeOwnership(note, layer);
        AudioSeq_AudioListPushBack(&pool[2], &note->listItem);
    }
    return note;
}

// original name: __Nas_ChLookRelWait
// Allocation attempt 3, the steal: the cheaper of the lowest-priority releasing and active notes below the layer's
// priority, an active one interrupted and moved to releasing, a releasing one with the layer queued; else NULL.
// Load-bearing: `&pool[2]` written at both uses, no local; `aPriority = 0x10; rPriority = 0x10;` on one line.
Note* Audio_AllocNoteFromActive(AudioListItem* pool, SequenceLayer* layer) {
    Note* rNote;
    Note* aNote;
    s32 rPriority;
    s32 aPriority;

    // clang-format off
    aPriority = 0x10; rPriority = 0x10;
    // clang-format on

    rNote = Audio_FindNodeWithPrioLessThan(&pool[2], layer->channel->notePriority);
    if (rNote != NULL) {
        rPriority = rNote->playbackState.priority;
    }
    aNote = Audio_FindNodeWithPrioLessThan(&pool[3], layer->channel->notePriority);
    if (aNote != NULL) {
        aPriority = aNote->playbackState.priority;
    }

    if (rNote == NULL && aNote == NULL) {
        PRINTF("Audio: C-Alloc : lowerPrio is NULL\n");
        return NULL;
    }
    if (aPriority < rPriority) {
        PRINTF("Intterupt UseStop %d (Kill %d)\n");
        Audio_AudioListRemove(&aNote->listItem);
        Audio_NoteInterruptForLayer(aNote, layer);
        AudioSeq_AudioListPushBack(&pool[2], &aNote->listItem);
        aNote->playbackState.priority = layer->channel->notePriority;
        return aNote;
    } else {
        PRINTF("Intterupt RelWait %d (Kill %d)\n");
        rNote->playbackState.wantedParentLayer = layer;
        rNote->playbackState.priority = layer->channel->notePriority;
        return rNote;
    }
}

// original name: Nas_AllocationOnRequest
// Finds a note for the layer: its last note again under policy bit 0, else the attempts over the channel's pool
// (bit 1), its and the sequence player's (2), the global lists (3), or its and the global ones; NULL if none.
Note* Audio_AllocNote(SequenceLayer* layer) {
    Note* note;
    SequenceChannel* channel;

    channel = layer->channel;

    if (channel->h.b.noteAllocPolicy & 1) {
        note = layer->note;
        if (note != NULL && note->playbackState.prevParentLayer == layer &&
            note->playbackState.wantedParentLayer == NO_LAYER) {
            Audio_NoteReleaseAndTakeOwnership(note, layer);
            Audio_AudioListRemove(&note->listItem);
            AudioSeq_AudioListPushBack(&note->listItem.pool[2], &note->listItem);
            return note;
        }
    }

    if (channel->h.b.noteAllocPolicy & 2) {
        if ((note = Audio_AllocNoteFromDisabled(channel->notePool, layer)) != NULL ||
            (note = Audio_AllocNoteFromDecaying(layer->channel->notePool, layer)) != NULL ||
            (note = Audio_AllocNoteFromActive(layer->channel->notePool, layer)) != NULL) {
            return note;
        }
    } else if (channel->h.b.noteAllocPolicy & 4) {
        if ((note = Audio_AllocNoteFromDisabled(channel->notePool, layer)) != NULL ||
            (note = Audio_AllocNoteFromDisabled(layer->channel->seqPlayer->notePool, layer)) != NULL ||
            (note = Audio_AllocNoteFromDecaying(layer->channel->notePool, layer)) != NULL ||
            (note = Audio_AllocNoteFromDecaying(layer->channel->seqPlayer->notePool, layer)) != NULL ||
            (note = Audio_AllocNoteFromActive(layer->channel->notePool, layer)) != NULL ||
            (note = Audio_AllocNoteFromActive(layer->channel->seqPlayer->notePool, layer)) != NULL) {
            return note;
        }
    } else if (channel->h.b.noteAllocPolicy & 8) {
        if ((note = Audio_AllocNoteFromDisabled(gAudioCtx.noteFreeLists, layer)) != NULL ||
            (note = Audio_AllocNoteFromDecaying(gAudioCtx.noteFreeLists, layer)) != NULL ||
            (note = Audio_AllocNoteFromActive(gAudioCtx.noteFreeLists, layer)) != NULL) {
            return note;
        }
    } else {
        if ((note = Audio_AllocNoteFromDisabled(channel->notePool, layer)) != NULL ||
            (note = Audio_AllocNoteFromDisabled(gAudioCtx.noteFreeLists, layer)) != NULL ||
            (note = Audio_AllocNoteFromDecaying(layer->channel->notePool, layer)) != NULL ||
            (note = Audio_AllocNoteFromDecaying(gAudioCtx.noteFreeLists, layer)) != NULL ||
            (note = Audio_AllocNoteFromActive(layer->channel->notePool, layer)) != NULL ||
            (note = Audio_AllocNoteFromActive(gAudioCtx.noteFreeLists, layer)) != NULL) {
            return note;
        }
    }

    PRINTF("Drop Voice (Prio %x)\n");
    layer->h.f.unk0_4 = 1;
    return NULL;
}

// original name: Nas_ChannelInit
// Resets every note to silence and gives each a 0xE0-byte block of synthesis buffers from the misc pool.
void Audio_NoteInitAll(void) {
    Note* note;
    s32 i;

    for (i = 0; i < gAudioCtx.numNotes; i++) {
        note = &gAudioCtx.notes[i];
        note->sampleState = gZeroNoteSampleState;
        note->playbackState.priority = 0;
        note->playbackState.status = 0;
        note->playbackState.parentLayer = NO_LAYER;
        note->playbackState.wantedParentLayer = NO_LAYER;
        note->playbackState.prevParentLayer = NO_LAYER;
        note->playbackState.waveId = 0;
        note->playbackState.attributes.velocity = 0.0f;
        note->playbackState.adsrVolScaleUnused = 0;
        note->playbackState.adsr.h.b.action = 0;
        note->playbackState.vibratoState.active = 0;
        note->playbackState.portamento.cur = 0;
        note->playbackState.portamento.speed = 0;
        note->synthesisState.synthesisBuffers = AudioHeap_AllocDmaMemory(&gAudioCtx.miscPool, 0xE0);
    }
}

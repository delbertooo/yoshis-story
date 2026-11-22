// original filename: track.c
#include "global.h"
#include "audio.h"
#include "aseq.h"

// NA audio driver, OoT's seqplayer.c: the sequence-player, channel and layer script interpreters, the
// layer free list, and the sequence players' init and reset.
// The PRINTFs are stripped debug prints whose strings stay in .data (-use_readwrite_const), in the ROM's order.

// OoT's portamento modes (effects.c and playback.c test the mode against a bare 0, as OoT's do). Bit 0x80
// marks a special portamento: its time is one script byte, and its speed follows the tempo and the note's length.
#define PORTAMENTO_IS_SPECIAL(x) ((x).mode & 0x80)
#define PORTAMENTO_MODE(x) ((x).mode & ~0x80)

typedef enum PortamentoMode {
    /* 0 */ PORTAMENTO_MODE_OFF,
    /* 1 */ PORTAMENTO_MODE_1,
    /* 2 */ PORTAMENTO_MODE_2,
    /* 3 */ PORTAMENTO_MODE_3,
    /* 4 */ PORTAMENTO_MODE_4,
    /* 5 */ PORTAMENTO_MODE_5
} PortamentoMode;

// OoT's sSeqInstructionArgsTable, read as [cmd - 0xB0]: the low two bits count an event's arguments, bits 7, 6
// and 5 mark the first, second and third as s16. As OoT's but for 0xBD; the first 0x50 bytes of this file's .data.

// OoT's CMD_ARGS_(NUMBER_OF_ARGS): an argument of type s16 sets its bit, one of type u8 leaves it clear.
#define CMD_ARGS_0() 0
#define CMD_ARGS_1(arg0Type) (((sizeof(arg0Type) - 1) << 7) | 1)
#define CMD_ARGS_2(arg0Type, arg1Type) (((sizeof(arg0Type) - 1) << 7) | ((sizeof(arg1Type) - 1) << 6) | 2)
#define CMD_ARGS_3(arg0Type, arg1Type, arg2Type) \
    (((sizeof(arg0Type) - 1) << 7) | ((sizeof(arg1Type) - 1) << 6) | ((sizeof(arg2Type) - 1) << 5) | 3)

u8 sSeqInstructionArgsTable[] = {
    CMD_ARGS_1(s16),        // 0xB0
    CMD_ARGS_0(),           // 0xB1
    CMD_ARGS_1(s16),        // 0xB2
    CMD_ARGS_1(u8),         // 0xB3
    CMD_ARGS_0(),           // 0xB4
    CMD_ARGS_0(),           // 0xB5
    CMD_ARGS_0(),           // 0xB6
    CMD_ARGS_1(s16),        // 0xB7
    CMD_ARGS_1(u8),         // 0xB8
    CMD_ARGS_1(u8),         // 0xB9
    CMD_ARGS_1(u8),         // 0xBA
    CMD_ARGS_2(u8, s16),    // 0xBB
    CMD_ARGS_1(s16),        // 0xBC
    CMD_ARGS_0(),           // 0xBD (OoT: CMD_ARGS_2(s16, s16), its random-range event; this driver has none)
    CMD_ARGS_0(),           // 0xBE
    CMD_ARGS_0(),           // 0xBF
    CMD_ARGS_0(),           // 0xC0
    CMD_ARGS_1(u8),         // 0xC1
    CMD_ARGS_1(s16),        // 0xC2
    CMD_ARGS_0(),           // 0xC3
    CMD_ARGS_0(),           // 0xC4
    CMD_ARGS_0(),           // 0xC5
    CMD_ARGS_1(u8),         // 0xC6
    CMD_ARGS_2(u8, s16),    // 0xC7
    CMD_ARGS_1(u8),         // 0xC8
    CMD_ARGS_1(u8),         // 0xC9
    CMD_ARGS_1(u8),         // 0xCA
    CMD_ARGS_1(s16),        // 0xCB
    CMD_ARGS_1(u8),         // 0xCC
    CMD_ARGS_1(u8),         // 0xCD
    CMD_ARGS_1(s16),        // 0xCE
    CMD_ARGS_1(s16),        // 0xCF
    CMD_ARGS_1(u8),         // 0xD0
    CMD_ARGS_1(u8),         // 0xD1
    CMD_ARGS_1(u8),         // 0xD2
    CMD_ARGS_1(u8),         // 0xD3
    CMD_ARGS_1(u8),         // 0xD4
    CMD_ARGS_1(u8),         // 0xD5
    CMD_ARGS_1(u8),         // 0xD6
    CMD_ARGS_1(u8),         // 0xD7
    CMD_ARGS_1(u8),         // 0xD8
    CMD_ARGS_1(u8),         // 0xD9
    CMD_ARGS_1(s16),        // 0xDA
    CMD_ARGS_1(u8),         // 0xDB
    CMD_ARGS_1(u8),         // 0xDC
    CMD_ARGS_1(u8),         // 0xDD
    CMD_ARGS_1(s16),        // 0xDE
    CMD_ARGS_1(u8),         // 0xDF
    CMD_ARGS_1(u8),         // 0xE0
    CMD_ARGS_3(u8, u8, u8), // 0xE1
    CMD_ARGS_3(u8, u8, u8), // 0xE2
    CMD_ARGS_1(u8),         // 0xE3
    CMD_ARGS_0(),           // 0xE4
    CMD_ARGS_1(u8),         // 0xE5
    CMD_ARGS_1(u8),         // 0xE6
    CMD_ARGS_1(s16),        // 0xE7
    CMD_ARGS_3(u8, u8, u8), // 0xE8
    CMD_ARGS_1(u8),         // 0xE9
    CMD_ARGS_0(),           // 0xEA
    CMD_ARGS_2(u8, u8),     // 0xEB
    CMD_ARGS_0(),           // 0xEC
    CMD_ARGS_1(u8),         // 0xED
    CMD_ARGS_1(u8),         // 0xEE
    CMD_ARGS_2(s16, u8),    // 0xEF
    CMD_ARGS_0(),           // 0xF0
    CMD_ARGS_1(u8),         // 0xF1
    // Control flow instructions (>= 0xF2) can only have 0 or 1 args
    CMD_ARGS_1(u8),  // 0xF2
    CMD_ARGS_1(u8),  // 0xF3
    CMD_ARGS_1(u8),  // 0xF4
    CMD_ARGS_1(s16), // 0xF5
    CMD_ARGS_0(),    // 0xF6
    CMD_ARGS_0(),    // 0xF7
    CMD_ARGS_1(u8),  // 0xF8
    CMD_ARGS_1(s16), // 0xF9
    CMD_ARGS_1(s16), // 0xFA
    CMD_ARGS_1(s16), // 0xFB
    CMD_ARGS_1(s16), // 0xFC
    CMD_ARGS_0(),    // 0xFD
    CMD_ARGS_0(),    // 0xFE
    CMD_ARGS_0(),    // 0xFF
};

u16 AudioSeq_GetScriptControlFlowArgument(SeqScriptState* state, u8 cmd);
s32 AudioSeq_HandleScriptFlowControl(SequencePlayer* seqPlayer, SeqScriptState* state, s32 cmd, s32 cmdArg);
void AudioSeq_SequenceChannelEnable(SequencePlayer* seqPlayer, u8 channelIndex, void* script);
void AudioSeq_SequenceChannelProcessScript();
void AudioSeq_SeqLayerProcessScriptStep1(SequenceLayer* layer);
s32 AudioSeq_GetInstrument(SequenceChannel* channel, u8 instId, Instrument** instOut, AdsrSettings* adsr);
void AudioSeq_SetInstrument(SequenceChannel* channel, u8 instId);
void AudioSeq_SequencePlayerProcessSequence(SequencePlayer* seqPlayer);
void AudioSeq_InitSequencePlayer(SequencePlayer* seqPlayer);
void AudioSeq_InitLayerFreelist(void);
void AudioSeq_InitSequenceChannel(SequenceChannel* channel);
s32 AudioSeq_SeqChannelSetLayer(SequenceChannel* channel, s32 layerIndex);
void AudioSeq_SeqLayerDisable(SequenceLayer* layer);
void AudioSeq_SeqLayerFree(SequenceChannel* channel, s32 layerIndex);
void AudioSeq_SequencePlayerSetupChannels(SequencePlayer* seqPlayer, u16 channelBits);
void AudioSeq_SequencePlayerDisableChannels(SequencePlayer* seqPlayer, u16 channelBits);
u8 AudioSeq_ScriptReadU8(SeqScriptState* state);
s16 AudioSeq_ScriptReadS16(SeqScriptState* state);
u16 AudioSeq_ScriptReadCompressedU16(SeqScriptState* state);
void AudioSeq_SetChannelPriorities(SequenceChannel* channel, u8 priority);
void AudioSeq_SequenceChannelSetVolume(SequenceChannel* channel, u8 volume);
s32 AudioSeq_SeqLayerProcessScriptStep2(SequenceLayer* layer);
s32 AudioSeq_SeqLayerProcessScriptStep4(SequenceLayer* layer, s32 cmd);
s32 AudioSeq_SeqLayerProcessScriptStep3(SequenceLayer* layer, s32 cmd);
s32 AudioSeq_SeqLayerProcessScriptStep5(SequenceLayer* layer, u32 sameTunedSample);

// original name: Convert_Com
// Reads a flow-control command's argument off the script, a byte or an s16 as sSeqInstructionArgsTable says; 0 when
// it has none.
// Load-bearing: the redundant `& 0xFF` on the descriptor.
u16 AudioSeq_GetScriptControlFlowArgument(SeqScriptState* state, u8 cmd) {
    u16 value;

    value = 0;
    if (((sSeqInstructionArgsTable[cmd - 0xB0] & 0xFF) & 3) == 1) {
        if ((sSeqInstructionArgsTable[cmd - 0xB0] & 0x80) == 0) {
            value = AudioSeq_ScriptReadU8(state);
        } else {
            value = AudioSeq_ScriptReadS16(state);
        }
    }
    return value;
}

// original name: Common_Com
// Runs a flow-control command (0xF2..0xFF): calls and returns, loops, breaks, jumps (some on the script's value)
// and delays. Returns 0 to read on, the delay, or -1 at an END with no call to return to.
// Load-bearing: the empty `if (state->depth && state->depth) {}` at the exit, LOOPEND's re-reads of `state->depth`,
// the comma in END, and `(s8)(cmdArg & 0xFF)` in the relative jumps.
s32 AudioSeq_HandleScriptFlowControl(SequencePlayer* seqPlayer, SeqScriptState* state, s32 cmd, s32 cmdArg) {
    s32 depth;
    u8 next;

    switch (cmd) {
        case ASEQ_OP_END:
            depth = state->depth;
            next = depth - 1;
            if (depth == 0) {
                return -1;
            }
            state->depth = next, state->pc = state->stack[next];
            break;
        case ASEQ_OP_DELAY:
            return AudioSeq_ScriptReadCompressedU16(state);
        case ASEQ_OP_DELAY1:
            return 1;
        case ASEQ_OP_CALL:
            PRINTF("Audio:Track :Call Macro Level Over Error!\n");
            depth = state->depth;
            state->stack[depth] = state->pc;
            state->depth++;
            state->pc = (u8*)(seqPlayer->seqData + (u16)cmdArg);
            break;
        case ASEQ_OP_LOOP:
            PRINTF("Audio:Track :Loops Macro Level Over Error!\n");
            depth = state->depth;
            state->remLoopIters[depth] = cmdArg;
            state->stack[state->depth] = state->pc;
            state->depth++;
            break;
        case ASEQ_OP_LOOPEND:
            state->remLoopIters[state->depth - 1]--;
            if (state->remLoopIters[state->depth - 1] != 0) {
                state->pc = state->stack[state->depth - 1];
            } else {
                state->depth--;
            }
            break;
        case ASEQ_OP_BREAK:
            state->depth--;
            break;
        case ASEQ_OP_BGEZ:
        case ASEQ_OP_BLTZ:
        case ASEQ_OP_BEQZ:
        case ASEQ_OP_JUMP:
            if (cmd == ASEQ_OP_BEQZ && state->value != 0) {
                break;
            }
            if (cmd == ASEQ_OP_BLTZ && state->value >= 0) {
                break;
            }
            if (cmd == ASEQ_OP_BGEZ && state->value < 0) {
                break;
            }
            state->pc = (u8*)(seqPlayer->seqData + (u16)cmdArg);
            break;
        case ASEQ_OP_RBLTZ:
        case ASEQ_OP_RBEQZ:
        case ASEQ_OP_RJUMP:
            if (cmd == ASEQ_OP_RBEQZ && state->value != 0) {
                break;
            }
            if (cmd == ASEQ_OP_RBLTZ && state->value >= 0) {
                break;
            }
            state->pc += (s8)(cmdArg & 0xFF);
            break;
    }
    if (state->depth && state->depth) {}
    return 0;
}

// original name: Nas_InitSubTrack
// Resets a channel to its defaults (the shared sequenceChannelNone is left alone) and inits its note pool.
// Load-bearing: the two `0x800` vibrato-rate stores where they stand, beside the vibrato-depth ones.
void AudioSeq_InitSequenceChannel(SequenceChannel* channel) {
    s32 i;

    if (channel == &gAudioCtx.sequenceChannelNone) {
        return;
    }
    channel->h.f.enabled = 0;
    channel->h.f.finished = 0;
    channel->h.f.stopScript = 0;
    channel->h.f.hasInstrument = 0;
    channel->h.f.stereoHeadsetEffects = 0;
    channel->transposition = 0;
    channel->h.f.largeNotes = 0;
    channel->bookOffset = 0;
    channel->h.b.changes = 0xFF;
    channel->scriptState.depth = 0;
    channel->newPan = 0x40;
    channel->panChannelWeight = 0x80;
    channel->velocityRandomVariance = 0;
    channel->gateTimeRandomVariance = 0;
    channel->noteUnused = NULL;
    channel->reverbIndex = 0;
    channel->targetReverbVol = 0;
    channel->gain = 0;
    channel->notePriority = 3;
    channel->someOtherPriority = 1;
    channel->delay = 0;
    channel->adsr.envelope = gDefaultEnvelope;
    channel->adsr.decayIndex = 0xF0;
    channel->adsr.sustain = 0;
    channel->vibratoRateTarget = 0x800;
    channel->vibratoRateStart = 0x800;
    channel->vibratoDepthTarget = 0;
    channel->vibratoDepthStart = 0;
    channel->vibratoRateChangeDelay = 0;
    channel->vibratoDepthChangeDelay = 0;
    channel->vibratoDelay = 0;
    channel->filter = NULL;
    channel->combFilterGain = 0;
    channel->combFilterSize = 0;
    channel->volume = 1.0f;
    channel->volumeScale = 1.0f;
    channel->freqScale = 1.0f;
    for (i = 0; i < 8; i++) {
        channel->seqScriptIO[i] = SEQ_IO_VAL_NONE;
    }
    channel->h.f.unused = 0;
    Audio_InitNoteLists(channel->notePool);
}

// original name: Nas_EntryNoteTrack (YS string "Audio:Track:Warning: No Free Notetrack")
// Gives the channel's slot `layerIndex` a layer reset to the channel's defaults: a fresh one off the free list, or
// the one already there once its note decays. Returns -1 when the free list is empty.
s32 AudioSeq_SeqChannelSetLayer(SequenceChannel* channel, s32 layerIndex) {
    SequenceLayer* layer;
    SequenceLayer* fresh;

    if (channel->layers[layerIndex] == NULL) {
        fresh = AudioSeq_AudioListPopBack(&gAudioCtx.layerFreeList);
        channel->layers[layerIndex] = fresh;
        if (fresh == NULL) {
            PRINTF("Audio:Track:Warning: No Free Notetrack\n");
            channel->layers[layerIndex] = NULL;
            return -1;
        }
    } else {
        Audio_SeqLayerNoteDecay(channel->layers[layerIndex]);
    }

    layer = channel->layers[layerIndex];
    layer->channel = channel;
    layer->adsr = channel->adsr;
    layer->adsr.decayIndex = 0;
    layer->h.f.enabled = 1;
    layer->h.f.finished = 0;
    layer->h.f.muted = 0;
    layer->h.f.continuousNotes = 0;
    layer->h.f.unk0_4 = 0;
    layer->h.f.ignoreDrumPan = 0;
    layer->h.f.noteDisabled = 0;
    layer->h.f.notePropertiesNeedInit = 0;
    layer->h.b.stereo = 0x40;
    layer->portamento.mode = PORTAMENTO_MODE_OFF;
    layer->scriptState.depth = 0;
    layer->h.b.gateTime = 0x80;
    layer->pan = 0x40;
    layer->transposition = 0;
    layer->delay = 0;
    layer->gateDelay = 0;
    layer->delay2 = 0;
    layer->note = NULL;
    layer->instrument = NULL;
    layer->h.b.instOrWave = 0xFF;
    layer->freqScale = 1.0f;
    layer->bend = 1.0f;
    layer->velocitySquare2 = 0.0f;
    return 0;
}

// original name: Nas_ReleaseNoteTrack
// Decays the layer's note and marks the layer disabled and finished.
void AudioSeq_SeqLayerDisable(SequenceLayer* layer) {
    if (layer != NULL) {
        Audio_SeqLayerNoteDecay(layer);
        layer->h.f.enabled = 0;
        layer->h.f.finished = 1;
    }
}

// original name: Nas_CloseNoteTrack
// Puts the channel's layer `layerIndex` back on the free list and disables it.
void AudioSeq_SeqLayerFree(SequenceChannel* channel, s32 layerIndex) {
    SequenceLayer* layer;

    layer = channel->layers[layerIndex];
    if (layer != NULL) {
        AudioSeq_AudioListPushBack(&gAudioCtx.layerFreeList, &layer->listItem);
        AudioSeq_SeqLayerDisable(layer);
        channel->layers[layerIndex] = NULL;
    }
}

// original name: Nas_ReleaseSubTrack
// Frees the channel's six layers, clears its note pool and marks it disabled and finished.
void AudioSeq_SequenceChannelDisable(SequenceChannel* channel) {
    s32 i;

    for (i = 0; i < 6; i++) {
        AudioSeq_SeqLayerFree(channel, i);
    }
    Audio_NotePoolClear(channel->notePool);
    channel->h.f.enabled = 0;
    channel->h.f.finished = 1;
}

// original name: Nas_AllocSub
// Gives every channel in the `channelBits` mask the sequence player's default font, mute behaviour and note
// allocation policy.
void AudioSeq_SequencePlayerSetupChannels(SequencePlayer* seqPlayer, u16 channelBits) {
    SequenceChannel* channel;
    s32 i;

    for (i = 0; i < 16; i++) {
        if (channelBits & 1) {
            channel = seqPlayer->channels[i];
            channel->fontId = seqPlayer->defaultFont;
            channel->h.b.muteBehavior = seqPlayer->muteBehavior;
            channel->h.b.noteAllocPolicy = seqPlayer->noteAllocPolicy;
        }
        channelBits = (s32)channelBits >> 1;
    }
}

// original name: Nas_DeAllocSub
// Disables every channel of the sequence player but sequenceChannelNone; `channelBits` is never read.
// Load-bearing: the slot read into `channel`, the `(u32)` casts and the `== 1`.
void AudioSeq_SequencePlayerDisableChannels(SequencePlayer* seqPlayer, UNUSED u16 channelBits) {
    SequenceChannel* channel;
    s32 i;

    for (i = 0; i < 16; i++) {
        channel = seqPlayer->channels[i];
        if (((u32)channel != (u32)&gAudioCtx.sequenceChannelNone) == 1) {
            AudioSeq_SequenceChannelDisable(channel);
        }
    }
}

// original name: Nas_OpenSub
// Starts channel `channelIndex` on `script`: resets its script state and delay, enables it and frees its layers.
void AudioSeq_SequenceChannelEnable(SequencePlayer* seqPlayer, u8 channelIndex, void* script) {
    SequenceChannel* channel;
    s32 i;

    channel = seqPlayer->channels[channelIndex];
    channel->scriptState.depth = 0;
    channel->scriptState.pc = script;
    channel->h.f.enabled = 1;
    channel->h.f.finished = 0;
    channel->delay = 0;
    for (i = 0; i < 6; i++) {
        if (channel->layers[i] != NULL) {
            AudioSeq_SeqLayerFree(channel, i);
        }
    }
}

// original name: Nas_ReleaseGroup
// Stops a sequence player: disables its channels and clears its note pool, marks its sequence and font discardable,
// and updates the temporary font cache's nextSide.
void AudioSeq_SequencePlayerDisable(SequencePlayer* seqPlayer) {
    PRINTF("Stop Release\n");
    AudioSeq_SequencePlayerDisableChannels(seqPlayer, 0xFFFF);
    Audio_NotePoolClear(seqPlayer->notePool);
    seqPlayer->finished = 1;
    seqPlayer->enabled = 0;
    if (AudioLoad_IsSeqLoadComplete(seqPlayer->seqId) != 0) {
        AudioLoad_SetSeqLoadStatus(seqPlayer->seqId, LOAD_STATUS_DISCARDABLE);
    }
    if (AudioLoad_IsFontLoadComplete(seqPlayer->defaultFont) != 0) {
        AudioLoad_SetFontLoadStatus(seqPlayer->defaultFont, LOAD_STATUS_MAYBE_DISCARDABLE);
    }
    if (gAudioCtx.fontCache.temporary.entries[0].id == seqPlayer->defaultFont) {
        gAudioCtx.fontCache.temporary.nextSide = 1;
    } else if (gAudioCtx.fontCache.temporary.entries[1].id == seqPlayer->defaultFont) {
        gAudioCtx.fontCache.temporary.nextSide = 0;
    }
}

// original name: Nas_AddList
// Links `item` in at the back of `list` (its prev end), unless the item is already in a list.
void AudioSeq_AudioListPushBack(AudioListItem* list, AudioListItem* item) {
    if (item->prev == NULL) {
        list->prev->next = item;
        item->prev = list->prev;
        item->next = list;
        list->prev = item;
        list->u.count++;
        item->pool = list->pool;
    } else {
        PRINTF("Error:Same List Add\n");
    }
}

// original name: Nas_GetList
// Unlinks the item at the back of `list` and returns its payload (u.value), or NULL when the list is empty.
void* AudioSeq_AudioListPopBack(AudioListItem* list) {
    AudioListItem* item;

    item = list->prev;
    if (item == list) {
        return NULL;
    }
    item->prev->next = list;
    list->prev = item->prev;
    item->prev = NULL;
    list->u.count--;
    return item->u.value;
}

// original name: Nas_InitNoteList
// Builds the layer free list from the 64 layers, each list item's payload pointing at its layer.
void AudioSeq_InitLayerFreelist(void) {
    s32 i;

    gAudioCtx.layerFreeList.prev = &gAudioCtx.layerFreeList;
    gAudioCtx.layerFreeList.next = &gAudioCtx.layerFreeList;
    gAudioCtx.layerFreeList.u.count = 0;
    gAudioCtx.layerFreeList.pool = NULL;
    for (i = 0; i < 64; i++) {
        gAudioCtx.sequenceLayers[i].listItem.u.value = &gAudioCtx.sequenceLayers[i];
        gAudioCtx.sequenceLayers[i].listItem.prev = NULL;
        AudioSeq_AudioListPushBack(&gAudioCtx.layerFreeList, &gAudioCtx.sequenceLayers[i].listItem);
    }
}

// original name: Nas_ReadByteData
// Reads a byte off the script.
u8 AudioSeq_ScriptReadU8(SeqScriptState* state) {
    return *state->pc++;
}

// original name: Nas_ReadWordData
// Reads a big-endian s16 off the script.
s16 AudioSeq_ScriptReadS16(SeqScriptState* state) {
    s16 ret;

    ret = *state->pc++ << 8;
    ret = *state->pc++ | ret;
    return ret;
}

// original name: Nas_ReadLengthData
// Reads a 7- or 15-bit value off the script: one byte, and a second one when the first has its top bit set.
u16 AudioSeq_ScriptReadCompressedU16(SeqScriptState* state) {
    u16 ret;

    ret = *state->pc++;
    if (ret & 0x80) {
        ret = (ret << 8) & 0x7F00;
        ret = *state->pc++ | ret;
    }
    return ret;
}

// original name: Nas_NoteSeq
// One tick of a layer: counts its delay down (decaying the note at the gate delay), or ends the note and runs
// Step1..Step5 for the next one, decaying it again when it comes out muted.
void AudioSeq_SeqLayerProcessScript(SequenceLayer* layer) {
    s32 cmd;

    if (layer->h.f.enabled) {
        if (layer->delay >= 2) {
            layer->delay--;
            if (!layer->h.f.muted) {
                if (layer->gateDelay >= layer->delay) {
                    Audio_SeqLayerNoteDecay(layer);
                    layer->h.f.muted = 1;
                }
            }
        } else {
            AudioSeq_SeqLayerProcessScriptStep1(layer);
            cmd = AudioSeq_SeqLayerProcessScriptStep2(layer);
            if (cmd != -1) {
                cmd = AudioSeq_SeqLayerProcessScriptStep3(layer, cmd);
                if (cmd != -1) {
                    cmd = AudioSeq_SeqLayerProcessScriptStep4(layer, cmd);
                }
                if (cmd != -1) {
                    AudioSeq_SeqLayerProcessScriptStep5(layer, cmd);
                }
                if (layer->h.f.muted == 1) {
                    if (layer->note != NULL || layer->h.f.continuousNotes) {
                        Audio_SeqLayerNoteDecay(layer);
                    }
                }
            }
        }
    }
}

// original name: __Stop_Note
// Ends the layer's last note: decays it unless the layer is legato and the note is gone or wants another layer,
// turns portamento modes 1 and 2 off and flags the note properties for init.
void AudioSeq_SeqLayerProcessScriptStep1(SequenceLayer* layer) {
    s32 mode;

    if (!layer->h.f.continuousNotes) {
        Audio_SeqLayerNoteDecay(layer);
    } else if (layer->note != NULL && layer->note->playbackState.wantedParentLayer == layer) {
        Audio_SeqLayerNoteDecay(layer);
    }

    mode = PORTAMENTO_MODE(layer->portamento);
    if (mode == PORTAMENTO_MODE_1 || mode == PORTAMENTO_MODE_2) {
        layer->portamento.mode = PORTAMENTO_MODE_OFF;
    }
    layer->h.f.notePropertiesNeedInit = 1;
}

// original name: __SetChannel
// Gives the layer its note: a legato note on the same sample stays, else the old one decays if the sample changed
// and a new one is allocated, with vibrato and portamento init. -1 for an in-memory sample not yet in RAM.
// Load-bearing: the guards testing `layer->note` directly; `u32 sameTunedSample` and its `== 1`; the else arm's
// store straight into `layer->note` with `voice` named inside the guarded block; `voice` at both init calls.
s32 AudioSeq_SeqLayerProcessScriptStep5(SequenceLayer* layer, u32 sameTunedSample) {
    Note* voice;

    if (!layer->h.f.muted && layer->tunedSample != NULL) {
        if (layer->tunedSample->sample->codec == CODEC_S16_INMEMORY &&
            layer->tunedSample->sample->medium != MEDIUM_RAM) {
            PRINTF("Wait Time out!\n");
            layer->h.f.muted = 1;
            return -1;
        }
    }

    if (layer->h.f.continuousNotes == 1 && layer->h.f.noteDisabled == 1) {
        return 0;
    }

    if (layer->h.f.continuousNotes == 1 && layer->note != NULL && layer->h.f.unk0_4 && sameTunedSample == 1 &&
        layer->note->playbackState.parentLayer == layer) {
        if (layer->tunedSample == NULL) {
            Audio_InitSyntheticWave(layer->note, layer);
        }
    } else {
        if (sameTunedSample == 0) {
            Audio_SeqLayerNoteDecay(layer);
        }
        layer->note = Audio_AllocNote(layer);
        if (layer->note != NULL && layer->note->playbackState.parentLayer == layer) {
            voice = layer->note;
            Audio_NoteVibratoInit(voice);
        }
    }

    if (layer->note != NULL && layer->note->playbackState.parentLayer == layer) {
        voice = layer->note;
        Audio_NotePortamentoInit(voice);
    }
    return 0;
}

// original name: __Command_Seq
// Reads the layer's script up to its next note command (below 0xC1) and returns it, running the flow control and
// the layer events on the way; -1 when the script ends, which disables the layer.
// Load-bearing: `value = AudioSeq_ScriptReadU8(state); event = value;` at the loop head and in LAYER_INSTR, both
// u8; the locals `d`, `vel` and `s32 res`; `value`, not `event`, in the return and the three calls.
s32 AudioSeq_SeqLayerProcessScriptStep2(SequenceLayer* layer) {
    SequenceChannel* channel;
    SeqScriptState* state;
    SequencePlayer* seqPlayer;
    u8 event;
    u8 value;
    u8 arg;
    s32 res;

    channel = layer->channel;
    state = &layer->scriptState;
    seqPlayer = channel->seqPlayer;

    while (1) {
        value = AudioSeq_ScriptReadU8(state);
        event = value;
        if (event < 0xC1) {
            return value;
        }

        if (event >= ASEQ_OP_CONTROL_FLOW_FIRST) {
            if (AudioSeq_HandleScriptFlowControl(seqPlayer, state, value,
                                                 AudioSeq_GetScriptControlFlowArgument(state, value)) != 0) {
                AudioSeq_SeqLayerDisable(layer);
                return -1;
            }
        } else {
            switch (event) {
                case ASEQ_OP_LAYER_SHORTVEL:
                case ASEQ_OP_LAYER_NOTEPAN:
                    arg = *state->pc++;
                    if (event == ASEQ_OP_LAYER_SHORTVEL) {
                        layer->velocitySquare = (arg * arg) / 16129.0f;
                    } else {
                        layer->pan = arg;
                    }
                    break;
                case ASEQ_OP_LAYER_TRANSPOSE:
                case ASEQ_OP_LAYER_SHORTGATE:
                    arg = *state->pc++;
                    if (event == ASEQ_OP_LAYER_SHORTGATE) {
                        layer->h.b.gateTime = arg;
                    } else {
                        layer->transposition = arg;
                    }
                    break;
                case ASEQ_OP_LAYER_LEGATO:
                case ASEQ_OP_LAYER_NOLEGATO:
                    if (event == ASEQ_OP_LAYER_LEGATO) {
                        layer->h.f.continuousNotes = 1;
                    } else {
                        layer->h.f.continuousNotes = 0;
                    }
                    layer->h.f.noteDisabled = 0;
                    Audio_SeqLayerNoteDecay(layer);
                    break;
                case ASEQ_OP_LAYER_SHORTDELAY:
                    layer->shortNoteDefaultDelay = AudioSeq_ScriptReadCompressedU16(state);
                    break;
                case ASEQ_OP_LAYER_INSTR:
                    value = AudioSeq_ScriptReadU8(state);
                    event = value;
                    if (event >= 0x7F) {
                        if (event == 0x7F) {
                            layer->h.b.instOrWave = 0;
                        } else {
                            layer->h.b.instOrWave = event;
                            layer->instrument = NULL;
                        }
                        if (event == 0xFF) {
                            layer->adsr.decayIndex = 0;
                        }
                    } else {
                        res = AudioSeq_GetInstrument(channel, value, &layer->instrument, &layer->adsr);
                        layer->h.b.instOrWave = res;
                        if (res == 0) {
                            PRINTF("WARNING: NPRG: cannot change %d\n");
                            layer->h.b.instOrWave = 0xFF;
                        }
                    }
                    break;
                case ASEQ_OP_LAYER_PORTAMENTO:
                    layer->portamento.mode = AudioSeq_ScriptReadU8(state);
                    value = AudioSeq_ScriptReadU8(state);
                    value += channel->transposition;
                    value += layer->transposition;
                    value += seqPlayer->transposition;
                    if (value >= 0x80) {
                        value = 0;
                    }
                    layer->portamentoTargetNote = value;
                    if (PORTAMENTO_IS_SPECIAL(layer->portamento)) {
                        layer->portamentoTime = *state->pc++;
                    } else {
                        layer->portamentoTime = AudioSeq_ScriptReadCompressedU16(state);
                    }
                    break;
                case ASEQ_OP_LAYER_NOPORTAMENTO:
                    layer->portamento.mode = PORTAMENTO_MODE_OFF;
                    break;
                case ASEQ_OP_LAYER_ENV: {
                    u16 d = AudioSeq_ScriptReadS16(state);
                    layer->adsr.envelope = (EnvelopePoint*)(seqPlayer->seqData + d);
                }
                    /* fallthrough */
                case ASEQ_OP_LAYER_RELEASERATE:
                    layer->adsr.decayIndex = AudioSeq_ScriptReadU8(state);
                    break;
                case ASEQ_OP_LAYER_NODRUMPAN:
                    layer->h.f.ignoreDrumPan = 1;
                    break;
                case ASEQ_OP_LAYER_STEREO:
                    layer->h.b.stereo = AudioSeq_ScriptReadU8(state);
                    break;
                case ASEQ_OP_LAYER_BENDFINE:
                    layer->bend = gBendPitchTwoSemitonesFrequencies[(u8)(AudioSeq_ScriptReadU8(state) + 0x80)];
                    break;
                default:
                    switch (event & 0xF0) {
                        case ASEQ_OP_LAYER_LDSHORTVEL: {
                            u8 vel = seqPlayer->shortNoteVelocityTable[event & 0xF];
                            layer->velocitySquare = (vel * vel) / 16129.0f;
                        } break;
                        case ASEQ_OP_LAYER_LDSHORTGATE:
                            layer->h.b.gateTime = seqPlayer->shortNoteGateTimeTable[event & 0xF];
                            break;
                        default:
                            PRINTF("Audio:Track:NOTE:UNDEFINED NOTE COM. %x\n");
                            break;
                    }
                    break;
            }
        }
    }
}

// original name: __SetVoice
// Resolves the key to a tuned sample and frequency (a drum for instOrWave 0, an instrument, a synthetic wave from
// 0xC0) and sets up any glide; returns 0 when an instrument's sample changed, -1 if refused, else 1.
// Load-bearing: the declaration order; `s32 raw`; no block boundary between `program = raw;` and the first
// `raw == 0xFF`; the drum key summed into `keyByte`; the empty `if (key) {}` and `if (keyByte) {}`; `(raw | 0)` in
// the second test; `portamento` assigned at the top of the glide block.
s32 AudioSeq_SeqLayerProcessScriptStep4(SequenceLayer* layer, s32 cmd) {
    s32 ret;
    SequenceChannel* channel;
    s32 program;
    Instrument* inst;
    Drum* drum;
    TunedSample* tunedSample;
    s32 hi;
    f32 ratio; // also the glide's `from` pitch, once both pitches are known
    f32 pitchNew;
    f32 pitchOld;
    f32 to;
    s32 rate;
    s32 raw;
    SequencePlayer* seqPlayer;
    u8 keyByte;
    Portamento* portamento;
    s32 key;

    ret = 1;
    channel = layer->channel;
    seqPlayer = channel->seqPlayer;
    raw = layer->h.b.instOrWave;
    program = raw;
    if (raw == 0xFF) {
        if (!channel->h.f.hasInstrument) {
            PRINTF("Error: Subtrack no prg.\n");
            return -1;
        }
        program = channel->instOrWave;
    }

    if (program == 0) {
        keyByte = (u8)cmd + channel->transposition + layer->transposition;
        drum = Audio_GetDrum(channel->fontId, keyByte);
        if (drum == NULL) {
            PRINTF("ERR %x\n");
            layer->h.f.muted = 1;
            layer->delay2 = layer->delay;
            return -1;
        }
        tunedSample = &drum->tunedSample;
        layer->adsr.envelope = drum->envelope;
        layer->adsr.decayIndex = drum->adsrDecayIndex;
        if (!layer->h.f.ignoreDrumPan) {
            layer->pan = drum->pan;
        }
        layer->tunedSample = tunedSample;
        layer->freqScale = tunedSample->tuning;
    } else {
        keyByte = (u8)cmd + seqPlayer->transposition + channel->transposition + layer->transposition;
        key = keyByte;
        if (key) {}
        if (keyByte) {}
        if (key >= 0x80) {
            PRINTF("Note OverFlow %d\n");
            PRINTF("trs %d , %d, %d\n");
            layer->h.f.muted = 1;
            return -1;
        }

        if ((raw | 0) == 0xFF) {
            inst = channel->instrument;
        } else {
            inst = layer->instrument;
        }

        if (layer->portamento.mode != PORTAMENTO_MODE_OFF) {
            portamento = &layer->portamento;
            // A glide looks the sample up on the higher of its two keys.
            if (layer->portamentoTargetNote < key) {
                hi = key;
            } else {
                hi = layer->portamentoTargetNote;
            }
            if (inst != NULL) {
                tunedSample = Audio_GetInstrumentTunedSample(inst, hi);
                ret = layer->tunedSample == tunedSample;
                layer->tunedSample = tunedSample;
                ratio = tunedSample->tuning;
            } else {
                ratio = 1.0f;
                layer->tunedSample = NULL;
                if (program >= 0xC0) {
                    layer->tunedSample = &gAudioCtx.synthesisReverbs[program - 0xC0].tunedSample;
                }
            }
            pitchNew = gPitchFrequencies[key] * ratio;
            pitchOld = gPitchFrequencies[layer->portamentoTargetNote] * ratio;
            switch (PORTAMENTO_MODE(*portamento)) {
                case PORTAMENTO_MODE_1:
                case PORTAMENTO_MODE_3:
                case PORTAMENTO_MODE_5:
                    ratio = pitchNew;
                    to = pitchOld;
                    break;
                case PORTAMENTO_MODE_2:
                case PORTAMENTO_MODE_4:
                    to = pitchNew;
                    ratio = pitchOld;
                    break;
                default:
                    to = pitchNew;
                    ratio = pitchNew;
                    break;
            }
            portamento->extent = ratio / to - 1.0f;
            if (PORTAMENTO_IS_SPECIAL(*portamento)) {
                rate = (seqPlayer->tempo << 15) / gAudioCtx.maxTempo;
                rate = (rate << 8) / (layer->delay * layer->portamentoTime);
            } else {
                rate = 0x20000 / (layer->portamentoTime * gAudioCtx.audioBufferParameters.updatesPerFrame);
            }
            if (rate >= 0x7FFF) {
                rate = 0x7FFF;
            } else if (rate <= 0) {
                rate = 1;
            }
            portamento->speed = rate;
            portamento->cur = 0;
            layer->freqScale = to;
            if (PORTAMENTO_MODE(*portamento) == PORTAMENTO_MODE_5) {
                layer->portamentoTargetNote = keyByte;
            }
        } else {
            if (inst != NULL) {
                tunedSample = Audio_GetInstrumentTunedSample(inst, key);
                ret = layer->tunedSample == tunedSample;
                layer->tunedSample = tunedSample;
                layer->freqScale = gPitchFrequencies[key] * tunedSample->tuning;
            } else {
                layer->tunedSample = NULL;
                layer->freqScale = gPitchFrequencies[key];
                if (program >= 0xC0) {
                    layer->tunedSample = &gAudioCtx.synthesisReverbs[program - 0xC0].tunedSample;
                }
            }
        }
    }

    layer->freqScale = layer->freqScale * layer->bend;
    layer->delay2 = layer->delay;
    return ret;
}

// original name: __SetNote
// Turns the note command into the layer's delay, velocity and gate delay (read or repeated, as largeNotes and the
// top two bits say), randomised by the channel's variances; returns the key, or -1 for a rest, a mute or a skip.
s32 AudioSeq_SeqLayerProcessScriptStep3(SequenceLayer* layer, s32 cmd) {
    SeqScriptState* state;
    u16 delay;
    s32 vel;
    SequenceChannel* channel;
    SequencePlayer* seqPlayer;

    channel = layer->channel;
    seqPlayer = channel->seqPlayer;
    state = &layer->scriptState;

    if (cmd == ASEQ_OP_LAYER_LDELAY) {
        layer->delay = AudioSeq_ScriptReadCompressedU16(state);
        layer->h.f.muted = 1;
        layer->h.f.noteDisabled = 0;
        return -1;
    }

    layer->h.f.muted = 0;

    if (channel->h.f.largeNotes == 1) {
        switch (cmd & 0xC0) {
            case ASEQ_OP_LAYER_NOTEDVG:
                delay = AudioSeq_ScriptReadCompressedU16(state);
                vel = *state->pc++;
                layer->h.b.gateTime = *state->pc++;
                layer->lastDelay = delay;
                break;
            case ASEQ_OP_LAYER_NOTEDV:
                delay = AudioSeq_ScriptReadCompressedU16(state);
                vel = *state->pc++;
                layer->h.b.gateTime = 0;
                layer->lastDelay = delay;
                break;
            case ASEQ_OP_LAYER_NOTEVG:
                delay = layer->lastDelay;
                vel = *state->pc++;
                layer->h.b.gateTime = *state->pc++;
                break;
        }
        if (vel > 127 || vel < 0) {
            PRINTF("Audio: Note:Velocity Error %d\n");
            vel = 127;
        }
        layer->velocitySquare = ((f32)vel * (f32)vel) / 16129.0f;
        cmd -= cmd & 0xC0;
    } else {
        switch (cmd & 0xC0) {
            case ASEQ_OP_LAYER_NOTEDVG:
                delay = AudioSeq_ScriptReadCompressedU16(state);
                layer->lastDelay = delay;
                break;
            case ASEQ_OP_LAYER_NOTEDV:
                delay = layer->shortNoteDefaultDelay;
                break;
            case ASEQ_OP_LAYER_NOTEVG:
                delay = layer->lastDelay;
                break;
        }
        cmd -= cmd & 0xC0;
    }

    if (channel->velocityRandomVariance != 0) {
        f32 dev;

        dev = layer->velocitySquare * (f32)(gAudioCtx.audioRandom % channel->velocityRandomVariance) / 100.0f;
        if (gAudioCtx.audioRandom & 0x8000) {
            dev = -dev;
        }
        layer->velocitySquare2 = layer->velocitySquare + dev;
        if (layer->velocitySquare2 < 0.0f) {
            layer->velocitySquare2 = 0.0f;
        } else if (layer->velocitySquare2 > 1.0f) {
            layer->velocitySquare2 = 1.0f;
        }
    } else {
        layer->velocitySquare2 = layer->velocitySquare;
    }

    layer->delay = delay;
    layer->gateDelay = (layer->h.b.gateTime * delay) >> 8;

    if (channel->gateTimeRandomVariance != 0) {
        s32 jitter;

        jitter = (layer->gateDelay * (gAudioCtx.audioRandom % channel->velocityRandomVariance)) / 100;
        if (gAudioCtx.audioRandom & 0x4000) {
            jitter = -jitter;
        }
        layer->gateDelay = layer->gateDelay + jitter;
        if (layer->gateDelay < 0) {
            layer->gateDelay = 0;
        } else if (layer->gateDelay > layer->delay) {
            layer->gateDelay = layer->delay;
        }
    }

    if ((seqPlayer->muted && (channel->h.b.muteBehavior & 0x50)) || channel->h.f.muted) {
        layer->h.f.muted = 1;
        return -1;
    }
    if (seqPlayer->skipTicks != 0) {
        layer->h.f.muted = 1;
        return -1;
    }
    return cmd;
}

// original name: Nas_PriorityChanger
// Sets notePriority from `priority`'s low nibble and someOtherPriority from its high one, each only if nonzero.
void AudioSeq_SetChannelPriorities(SequenceChannel* channel, u8 priority) {
    if ((priority & 0xF) != 0) {
        channel->notePriority = priority & 0xF;
    }
    priority = (s32)priority >> 4;
    if (priority != 0) {
        channel->someOtherPriority = priority;
    }
}

// original name: Nas_ProgramChanger
// Looks instrument `instId` up in the channel's font into `*instOut` and `adsr`; returns instId + 1, or 0 when the
// font has none.
// Load-bearing: `instId++`, not `instId + 1`.
s32 AudioSeq_GetInstrument(SequenceChannel* channel, u8 instId, Instrument** instOut, AdsrSettings* adsr) {
    Instrument* def;

    def = Audio_GetInstrumentInner(channel->fontId, instId);
    if (def == NULL) {
        *instOut = NULL;
        return 0;
    }
    adsr->envelope = def->envelope;
    adsr->decayIndex = def->adsrDecayIndex;
    *instOut = def;
    instId++;
    return instId;
}

// original name: Nas_SubVoiceSet
// Sets the channel's instrument: 0x80 and up a synthetic wave, 0x7F the drums (instOrWave 0), below that its font's
// instrument; hasInstrument (bit 0x08 of the flags) is set unless that lookup fails.
// Load-bearing: `& 0xFF` on the last statement's read of the flags, and nowhere else.
void AudioSeq_SetInstrument(SequenceChannel* channel, u8 instId) {
    s16 result;

    if (instId >= 0x80) {
        channel->instOrWave = instId;
        channel->instrument = NULL;
    } else if (instId == 0x7F) {
        channel->instOrWave = 0;
        channel->instrument = (Instrument*)1;
    } else {
        result = AudioSeq_GetInstrument(channel, instId, &channel->instrument, &channel->adsr);
        channel->instOrWave = result;
        if (result == 0) {
            channel->h.b.flags &= 0xFFF7;
            return;
        }
    }
    channel->h.b.flags = (channel->h.b.flags & 0xFF) | 8;
}

// original name: Nas_SubVolumeSet
// Sets the channel's volume from a 0..127 value, as 0..1.
void AudioSeq_SequenceChannelSetVolume(SequenceChannel* channel, u8 volume) {
    channel->volume = (s32)volume / 127.0f;
}

// original name: Nas_SubSeq
// One tick of a channel: unless muted with muteBehavior bit 0x80, counts its delay down or, unless stopped, reads
// its script up to the next wait; then runs its layers.
// Load-bearing: the declaration order and `lowBits`' roles; `cmd - 0xB0` and the empty `if (cmd) {}` in the
// argument loop; `slot` as the flow-control status; the change flags as bitfield sets; STSEQ's store through
// `seqData`; the base first and `(u32)` on the sum in LDSEQ and LDSEQTOPTR; the redundant masks in STPTRTOSEQ,
// RANDTOPTR, RAND and the `& 7` subscripts; `changesPan` last in LDPARAMS; no `cmd = cmdArgs[0];` in NOTEPRI;
// FILTER's arguments as the two u8 locals; `lowBits` at both of LDSAMPLE's reads; LDCHAN's read as its own
// statement; the dead `cmd &= 0xF;` in STCIO and LDCIO.
void AudioSeq_SequenceChannelProcessScript(SequenceChannel* channel) {
    SequencePlayer* seqPlayer;
    SeqScriptState* scriptState;
    u16 cmdArgU16;
    // The >= 0xB0 command's argument descriptor.
    u8 highBits;
    // Disjoint roles, as in OoT: the argument count, the sequence's font count, FILTER's high nibble and the
    // low nibble below 0x70.
    u8 lowBits;
    u8* data;
    s16 val;
    s32 i;
    s32 slot;
    u32 cmdArgs[3];
    u8 cmd;
    u8* seqData;

    if (channel->h.f.stopScript != 0) {
        goto done;
    }
    seqPlayer = channel->seqPlayer;
    if (seqPlayer->muted) {
        if ((channel->h.b.muteBehavior & 0x80) != 0) {
            return;
        }
    }
    scriptState = &channel->scriptState;

    if (channel->delay >= 2) {
        channel->delay--;
    } else {
        while (1) {
            cmd = AudioSeq_ScriptReadU8(scriptState);
            if (cmd >= 0xB0) {
                highBits = sSeqInstructionArgsTable[cmd - 0xB0];
                lowBits = highBits & 3;
                for (i = 0; i < lowBits; i++) {
                    if (cmd) {}
                    if ((highBits & 0x80) == 0) {
                        cmdArgs[i] = AudioSeq_ScriptReadU8(scriptState);
                    } else {
                        cmdArgs[i] = AudioSeq_ScriptReadS16(scriptState);
                    }
                    highBits <<= 1;
                }

                if (cmd >= ASEQ_OP_CONTROL_FLOW_FIRST) {
                    slot = AudioSeq_HandleScriptFlowControl(seqPlayer, scriptState, cmd, cmdArgs[0]);
                    if (slot != 0) {
                        if (slot == -1) {
                            AudioSeq_SequenceChannelDisable(channel);
                            break;
                        }
                        channel->delay = slot;
                        break;
                    }
                    continue;
                }

                switch (cmd) {
                    case ASEQ_OP_CHAN_STOP:
                        channel->h.f.stopScript = 1;
                        goto done;
                    case ASEQ_OP_CHAN_ALLOCNOTELIST:
                        Audio_NotePoolClear(channel->notePool);
                        cmd = cmdArgs[0];
                        Audio_NotePoolFill(channel->notePool, cmd);
                        break;
                    case ASEQ_OP_CHAN_FREENOTELIST:
                        Audio_NotePoolClear(channel->notePool);
                        break;
                    case ASEQ_OP_CHAN_DYNTBL:
                        cmdArgU16 = cmdArgs[0];
                        channel->dynTable = (u8*)(seqPlayer->seqData + cmdArgU16);
                        break;
                    case ASEQ_OP_CHAN_DYNTBLLOOKUP:
                        if (scriptState->value != -1) {
                            data = channel->dynTable + scriptState->value * 2;
                            cmdArgU16 = (data[0] << 8) + data[1];
                            channel->dynTable = (u8*)(seqPlayer->seqData + cmdArgU16);
                        }
                        break;
                    case ASEQ_OP_CHAN_FONTINSTR:
                        cmd = cmdArgs[0];
                        if (seqPlayer->defaultFont != 0xFF) {
                            cmdArgU16 = ((u16*)gAudioCtx.sequenceFontTable)[seqPlayer->seqId];
                            lowBits = *(cmdArgU16 + gAudioCtx.sequenceFontTable);
                            cmd = *(cmdArgU16 + lowBits - cmd + gAudioCtx.sequenceFontTable);
                        }
                        if (AudioHeap_SearchCaches(FONT_TABLE, CACHE_EITHER, cmd) != 0) {
                            channel->fontId = cmd;
                        } else {
                            PRINTF("SUB:ERR:BANK %d NOT CACHED.\n");
                        }
                        cmdArgs[0] = cmdArgs[1];
                        /* fallthrough */
                    case ASEQ_OP_CHAN_INSTR:
                        cmd = cmdArgs[0];
                        AudioSeq_SetInstrument(channel, cmd);
                        break;
                    case ASEQ_OP_CHAN_SHORT:
                        channel->h.f.largeNotes = 0;
                        break;
                    case ASEQ_OP_CHAN_NOSHORT:
                        channel->h.f.largeNotes = 1;
                        break;
                    case ASEQ_OP_CHAN_VOL:
                        cmd = cmdArgs[0];
                        AudioSeq_SequenceChannelSetVolume(channel, cmd);
                        channel->h.f.changesVolume = 1;
                        break;
                    case ASEQ_OP_CHAN_VOLEXP:
                        cmd = cmdArgs[0];
                        channel->h.f.changesVolume = 1;
                        channel->volumeScale = (s32)cmd * 0.0078125f;
                        break;
                    case ASEQ_OP_CHAN_FREQSCALE:
                        cmdArgU16 = cmdArgs[0];
                        channel->h.f.changesFreqScale = 1;
                        channel->freqScale = (s32)cmdArgU16 / 32768.0f;
                        break;
                    case ASEQ_OP_CHAN_BEND:
                        cmd = cmdArgs[0];
                        cmd += 0x80;
                        channel->freqScale = gBendPitchOneOctaveFrequencies[cmd];
                        channel->h.f.changesFreqScale = 1;
                        break;
                    case ASEQ_OP_CHAN_BENDFINE:
                        cmd = cmdArgs[0];
                        cmd += 0x80;
                        channel->freqScale = gBendPitchTwoSemitonesFrequencies[cmd];
                        channel->h.f.changesFreqScale = 1;
                        break;
                    case ASEQ_OP_CHAN_PAN:
                        cmd = cmdArgs[0];
                        channel->h.f.changesPan = 1;
                        channel->newPan = cmd;
                        break;
                    case ASEQ_OP_CHAN_PANWEIGHT:
                        cmd = cmdArgs[0];
                        channel->h.f.changesPan = 1;
                        channel->panChannelWeight = cmd;
                        break;
                    case ASEQ_OP_CHAN_TRANSPOSE:
                        slot = (s8)cmdArgs[0];
                        channel->transposition = slot;
                        break;
                    case ASEQ_OP_CHAN_ENV:
                        cmdArgU16 = cmdArgs[0];
                        channel->adsr.envelope = (EnvelopePoint*)(seqPlayer->seqData + cmdArgU16);
                        break;
                    case ASEQ_OP_CHAN_RELEASERATE:
                        cmd = cmdArgs[0];
                        channel->adsr.decayIndex = cmd;
                        break;
                    case ASEQ_OP_CHAN_VIBDEPTH:
                        cmd = cmdArgs[0];
                        channel->vibratoDepthStart = 0;
                        channel->vibratoDepthChangeDelay = 0;
                        channel->vibratoDepthTarget = cmd * 8;
                        break;
                    case ASEQ_OP_CHAN_VIBFREQ:
                        cmd = cmdArgs[0];
                        channel->vibratoRateChangeDelay = 0;
                        channel->vibratoRateStart = channel->vibratoRateTarget = cmd * 32;
                        break;
                    case ASEQ_OP_CHAN_VIBDEPTHGRAD:
                        cmd = cmdArgs[0];
                        channel->vibratoDepthStart = cmd * 8;
                        cmd = cmdArgs[1];
                        channel->vibratoDepthTarget = cmd * 8;
                        cmd = cmdArgs[2];
                        channel->vibratoDepthChangeDelay = cmd * 16;
                        break;
                    case ASEQ_OP_CHAN_VIBFREQGRAD:
                        cmd = cmdArgs[0];
                        channel->vibratoRateStart = cmd * 32;
                        cmd = cmdArgs[1];
                        channel->vibratoRateTarget = cmd * 32;
                        cmd = cmdArgs[2];
                        channel->vibratoRateChangeDelay = cmd * 16;
                        break;
                    case ASEQ_OP_CHAN_VIBDELAY:
                        cmd = cmdArgs[0];
                        channel->vibratoDelay = cmd * 16;
                        break;
                    case ASEQ_OP_CHAN_REVERB:
                        cmd = cmdArgs[0];
                        channel->targetReverbVol = cmd;
                        break;
                    case ASEQ_OP_CHAN_FONT:
                        cmd = cmdArgs[0];
                        if (seqPlayer->defaultFont != 0xFF) {
                            cmdArgU16 = ((u16*)gAudioCtx.sequenceFontTable)[seqPlayer->seqId];
                            lowBits = *(cmdArgU16 + gAudioCtx.sequenceFontTable);
                            cmd = *(cmdArgU16 + lowBits - cmd + gAudioCtx.sequenceFontTable);
                        }
                        if (AudioHeap_SearchCaches(FONT_TABLE, CACHE_EITHER, cmd) != 0) {
                            channel->fontId = cmd;
                        } else {
                            PRINTF("SUB:ERR:BANK %d NOT CACHED.\n");
                        }
                        break;
                    case ASEQ_OP_CHAN_STSEQ:
                        cmd = cmdArgs[0];
                        cmdArgU16 = cmdArgs[1];
                        seqData = (u8*)(seqPlayer->seqData + cmdArgU16);
                        *seqData = (u8)scriptState->value + cmd;
                        PRINTF("%x <-%x\n");
                        break;
                    case ASEQ_OP_CHAN_SUB:
                    case ASEQ_OP_CHAN_AND:
                    case ASEQ_OP_CHAN_LDI:
                        slot = (s8)cmdArgs[0];
                        if (cmd == ASEQ_OP_CHAN_SUB) {
                            scriptState->value -= slot;
                        } else if (cmd == ASEQ_OP_CHAN_LDI) {
                            scriptState->value = slot;
                        } else {
                            scriptState->value &= slot;
                        }
                        break;
                    case ASEQ_OP_CHAN_STOPCHAN:
                        cmd = cmdArgs[0];
                        AudioSeq_SequenceChannelDisable(seqPlayer->channels[cmd]);
                        break;
                    case ASEQ_OP_CHAN_MUTEBHV:
                        cmd = cmdArgs[0];
                        channel->h.f.changesVolume = 1;
                        channel->h.b.muteBehavior = cmd;
                        break;
                    case ASEQ_OP_CHAN_LDSEQ:
                        cmdArgU16 = cmdArgs[0];
                        scriptState->value = *(u8*)(seqPlayer->seqData + (u32)(cmdArgU16 + scriptState->value));
                        break;
                    case ASEQ_OP_CHAN_LDPTR:
                        cmdArgU16 = cmdArgs[0];
                        channel->ptr = cmdArgU16;
                        break;
                    case ASEQ_OP_CHAN_STPTRTOSEQ:
                        cmdArgU16 = cmdArgs[0];
                        seqData = (u8*)(seqPlayer->seqData + cmdArgU16);
                        seqData[0] = (u16)(channel->ptr >> 8);
                        seqData[1] = channel->ptr & 0xFF;
                        break;
                    case ASEQ_OP_CHAN_EFFECTS:
                        cmd = cmdArgs[0];
                        channel->h.f.stereoHeadsetEffects = cmd;
                        break;
                    case ASEQ_OP_CHAN_NOTEALLOC:
                        cmd = cmdArgs[0];
                        channel->h.b.noteAllocPolicy = cmd;
                        break;
                    case ASEQ_OP_CHAN_SUSTAIN:
                        cmd = cmdArgs[0];
                        channel->adsr.sustain = cmd;
                        break;
                    case ASEQ_OP_CHAN_REVERBIDX:
                        cmd = cmdArgs[0];
                        channel->reverbIndex = cmd;
                        break;
                    case ASEQ_OP_CHAN_DYNCALL:
                        if (scriptState->value != -1) {
                            if (scriptState->depth >= 4) {
                                PRINTF("Audio:Track: CTBLCALL Macro Level Over Error!\n");
                            }
                            data = channel->dynTable + scriptState->value * 2;
                            highBits = scriptState->depth;
                            scriptState->stack[highBits] = scriptState->pc;
                            scriptState->depth++;
                            cmdArgU16 = (data[0] << 8) + data[1];
                            scriptState->pc = (u8*)(seqPlayer->seqData + cmdArgU16);
                        }
                        break;
                    case ASEQ_OP_CHAN_SAMPLEBOOK:
                        cmd = cmdArgs[0];
                        channel->bookOffset = cmd;
                        PRINTF("Set Noise %d\n");
                        break;
                    case ASEQ_OP_CHAN_LDPARAMS:
                        cmdArgU16 = cmdArgs[0];
                        data = (u8*)(seqPlayer->seqData + cmdArgU16);
                        channel->h.b.muteBehavior = *data++;
                        channel->h.b.noteAllocPolicy = *data++;
                        AudioSeq_SetChannelPriorities(channel, *data++);
                        channel->transposition = (s8)*data++;
                        channel->newPan = *data++;
                        channel->panChannelWeight = *data++;
                        channel->targetReverbVol = *data++;
                        channel->reverbIndex = *data;
                        channel->h.f.changesPan = 1;
                        break;
                    case ASEQ_OP_CHAN_PARAMS:
                        channel->h.b.muteBehavior = cmdArgs[0];
                        channel->h.b.noteAllocPolicy = cmdArgs[1];
                        AudioSeq_SetChannelPriorities(channel, cmdArgs[2]);
                        channel->transposition = (s8)AudioSeq_ScriptReadU8(scriptState);
                        channel->newPan = AudioSeq_ScriptReadU8(scriptState);
                        channel->panChannelWeight = AudioSeq_ScriptReadU8(scriptState);
                        channel->targetReverbVol = AudioSeq_ScriptReadU8(scriptState);
                        channel->reverbIndex = AudioSeq_ScriptReadU8(scriptState);
                        channel->h.f.changesPan = 1;
                        break;
                    case ASEQ_OP_CHAN_VIBRESET:
                        channel->vibratoDepthTarget = 0;
                        channel->vibratoDepthStart = 0;
                        channel->vibratoDepthChangeDelay = 0;
                        channel->vibratoRateTarget = 0;
                        channel->vibratoRateStart = 0;
                        channel->vibratoRateChangeDelay = 0;
                        channel->filter = NULL;
                        channel->gain = 0;
                        channel->adsr.sustain = 0;
                        channel->velocityRandomVariance = 0;
                        channel->gateTimeRandomVariance = 0;
                        channel->combFilterSize = 0;
                        channel->combFilterGain = 0;
                        channel->bookOffset = 0;
                        channel->freqScale = 1.0f;
                        break;
                    case ASEQ_OP_CHAN_NOTEPRI:
                        AudioSeq_SetChannelPriorities(channel, (u8)cmdArgs[0]);
                        break;
                    case ASEQ_OP_CHAN_GAIN:
                        cmd = cmdArgs[0];
                        channel->gain = cmd;
                        break;
                    case ASEQ_OP_CHAN_LDFILTER:
                        cmdArgU16 = cmdArgs[0];
                        data = (u8*)(seqPlayer->seqData + cmdArgU16);
                        channel->filter = data;
                        break;
                    case ASEQ_OP_CHAN_FREEFILTER:
                        channel->filter = NULL;
                        break;
                    case ASEQ_OP_CHAN_FILTER:
                        cmd = cmdArgs[0];
                        if (channel->filter != NULL) {
                            lowBits = (cmd >> 4) & 0xF;
                            cmd &= 0xF;
                            AudioHeap_LoadFilter(channel->filter, lowBits, cmd);
                        }
                        break;
                    case ASEQ_OP_CHAN_LDSEQTOPTR:
                        cmdArgU16 = cmdArgs[0];
                        channel->ptr = *(u16*)(seqPlayer->seqData + (u32)(cmdArgU16 + scriptState->value * 2));
                        break;
                    case ASEQ_OP_CHAN_PTRTODYNTBL:
                        channel->dynTable = (u8*)(seqPlayer->seqData + channel->ptr);
                        break;
                    case ASEQ_OP_CHAN_DYNTBLTOPTR:
                        channel->ptr = *(u16*)(channel->dynTable + scriptState->value * 2);
                        break;
                    case ASEQ_OP_CHAN_DYNTBLV:
                        scriptState->value = channel->dynTable[scriptState->value];
                        break;
                    case ASEQ_OP_CHAN_RANDTOPTR:
                        if (cmdArgs[0] == 0) {
                            channel->ptr = gAudioCtx.audioRandom & 0xFFFF;
                        } else {
                            channel->ptr = gAudioCtx.audioRandom % cmdArgs[0];
                        }
                        break;
                    case ASEQ_OP_CHAN_RAND:
                        if (cmdArgs[0] == 0) {
                            scriptState->value = gAudioCtx.audioRandom & 0xFF;
                        } else {
                            scriptState->value = gAudioCtx.audioRandom % cmdArgs[0];
                        }
                        break;
                    case ASEQ_OP_CHAN_RANDVEL:
                        channel->velocityRandomVariance = cmdArgs[0];
                        break;
                    case ASEQ_OP_CHAN_RANDGATE:
                        channel->gateTimeRandomVariance = cmdArgs[0];
                        break;
                    case ASEQ_OP_CHAN_COMBFILTER:
                        channel->combFilterSize = cmdArgs[0];
                        channel->combFilterGain = cmdArgs[1];
                        break;
                    case ASEQ_OP_CHAN_PTRADD:
                        channel->ptr += cmdArgs[0];
                        break;
                    default:
                        PRINTF("[%2x] \n");
                        PRINTF("Err :Sub %x ,address %x:Undefined SubTrack Function %x");
                        break;
                }
            } else if (cmd >= 0x70) {
                switch (cmd & 0xF8) {
                    case ASEQ_OP_CHAN_TESTLAYER:
                        if (channel->layers[(cmd & 0xFF) & 7] != NULL) {
                            scriptState->value = channel->layers[(cmd & 0xFF) & 7]->h.f.finished;
                        } else {
                            scriptState->value = -1;
                        }
                        break;
                    case ASEQ_OP_CHAN_LDLAYER:
                        cmdArgU16 = AudioSeq_ScriptReadS16(scriptState);
                        slot = (cmd & 0xFF) & 7;
                        if (AudioSeq_SeqChannelSetLayer(channel, slot) == 0) {
                            channel->layers[slot]->scriptState.pc = (u8*)(seqPlayer->seqData + cmdArgU16);
                        }
                        break;
                    case ASEQ_OP_CHAN_DELLAYER:
                        AudioSeq_SeqLayerFree(channel, (cmd & 0xFF) & 7);
                        break;
                    case ASEQ_OP_CHAN_DYNLDLAYER:
                        if (scriptState->value != -1) {
                            slot = (cmd & 0xFF) & 7;
                            if (AudioSeq_SeqChannelSetLayer(channel, slot) != -1) {
                                data = channel->dynTable + scriptState->value * 2;
                                cmdArgU16 = (data[0] << 8) + data[1];
                                channel->layers[slot]->scriptState.pc = (u8*)(seqPlayer->seqData + cmdArgU16);
                            }
                        }
                        break;
                    case ASEQ_OP_CHAN_STIO:
                        channel->seqScriptIO[(cmd & 0xFF) & 7] = scriptState->value;
                        break;
                    case ASEQ_OP_CHAN_RLDLAYER:
                        val = AudioSeq_ScriptReadS16(scriptState);
                        slot = (cmd & 0xFF) & 7;
                        if (AudioSeq_SeqChannelSetLayer(channel, slot) == 0) {
                            channel->layers[slot]->scriptState.pc = scriptState->pc + val;
                        }
                        break;
                }
            } else {
                lowBits = cmd & 0xF;
                switch (cmd & 0xF0) {
                    case ASEQ_OP_CHAN_CDELAY:
                        channel->delay = lowBits;
                        goto done;
                    case ASEQ_OP_CHAN_LDSAMPLE:
                        channel->seqScriptIO[lowBits] = SEQ_IO_VAL_NONE;
                        if (AudioLoad_SlowLoadSample(channel->fontId, scriptState->value,
                                                     &channel->seqScriptIO[lowBits]) == -1) {
                            PRINTF("VoiceLoad Error Bank:%d,Prog:%d\n");
                        }
                        break;
                    case ASEQ_OP_CHAN_LDIO:
                        scriptState->value = channel->seqScriptIO[lowBits];
                        if (lowBits < 2) {
                            channel->seqScriptIO[lowBits] = SEQ_IO_VAL_NONE;
                        }
                        break;
                    case ASEQ_OP_CHAN_SUBIO:
                        scriptState->value -= channel->seqScriptIO[lowBits];
                        break;
                    case ASEQ_OP_CHAN_LDCHAN:
                        val = AudioSeq_ScriptReadS16(scriptState);
                        AudioSeq_SequenceChannelEnable(seqPlayer, cmd & 0xF, (void*)(seqPlayer->seqData + (u16)val));
                        break;
                    case ASEQ_OP_CHAN_STCIO:
                        cmd &= 0xF;
                        seqPlayer->channels[lowBits]->seqScriptIO[AudioSeq_ScriptReadU8(scriptState)] =
                            scriptState->value;
                        break;
                    case ASEQ_OP_CHAN_LDCIO:
                        cmd &= 0xF;
                        scriptState->value =
                            seqPlayer->channels[lowBits]->seqScriptIO[AudioSeq_ScriptReadU8(scriptState)];
                        break;
                }
            }
        }
    }

done:
    for (i = 0; i < 6; i++) {
        if (channel->layers[i] != NULL) {
            AudioSeq_SeqLayerProcessScript(channel->layers[i]);
        }
    }
}

// original name: Nas_GroupSeq
// One tick of a sequence player: with its sequence and font loaded, once the tempo accumulator passes maxTempo,
// counts its delay down or reads its script up to the next wait; then runs every enabled channel's script.
// Load-bearing: OoT's declaration list, `pad` and `dummy` with it, and its fake-match after SEQ_ALLOCNOTELIST;
// `data2` shared by STSEQ and LDSEQ; the empty cases 0x10, 0x20 and 0x60; at most 158 basic blocks: the two
// do-while loops, the flow-control arm's `if (delay == 0) { continue; }`, the undefined-command prints after the
// switch and after the loop rather than in `default:` arms, and no block-scoped local in the slot loop.
void AudioSeq_SequencePlayerProcessSequence(SequencePlayer* seqPlayer) {
    u8 cmd;
    u8 cmdLowBits;
    SeqScriptState* seqScript = &seqPlayer->scriptState;
    s16 tempS;
    u16 temp;
    s32 i;
    s32 value;
    u8* data;
    u8* data2;
    u8* data3;
    UNUSED s32 pad;
    s32 dummy;
    s32 delay;

    if (!seqPlayer->enabled) {
        return;
    }

    if (!AudioLoad_IsSeqLoadComplete(seqPlayer->seqId) || !AudioLoad_IsFontLoadComplete(seqPlayer->defaultFont)) {
        PRINTF("Disappear Sequence or Bank %d\n");
        AudioSeq_SequencePlayerDisable(seqPlayer);
        return;
    }

    AudioLoad_SetSeqLoadStatus(seqPlayer->seqId, LOAD_STATUS_COMPLETE);
    AudioLoad_SetFontLoadStatus(seqPlayer->defaultFont, LOAD_STATUS_COMPLETE);

    if (seqPlayer->muted && (seqPlayer->muteBehavior & 0x80)) {
        return;
    }

    seqPlayer->scriptCounter++;

    seqPlayer->tempoAcc += seqPlayer->tempo;
    seqPlayer->tempoAcc += (s16)seqPlayer->tempoChange;
    if (seqPlayer->tempoAcc < gAudioCtx.maxTempo) {
        return;
    }
    seqPlayer->tempoAcc -= (u16)gAudioCtx.maxTempo;

    if (seqPlayer->stopScript == 1) {
        return;
    }

    if (seqPlayer->delay > 1) {
        seqPlayer->delay--;
    } else {
        seqPlayer->recalculateVolume = 1;

        do {
            cmd = AudioSeq_ScriptReadU8(seqScript);

            if (cmd >= ASEQ_OP_CONTROL_FLOW_FIRST) {
                delay = AudioSeq_HandleScriptFlowControl(
                    seqPlayer, seqScript, cmd, AudioSeq_GetScriptControlFlowArgument(&seqPlayer->scriptState, cmd));

                if (delay == 0) {
                    continue;
                }
                if (delay == -1) {
                    AudioSeq_SequencePlayerDisable(seqPlayer);
                    break;
                }
                seqPlayer->delay = delay;
                break;
            }

            if (cmd >= 0xC0) {
                switch (cmd) {
                    case ASEQ_OP_SEQ_ALLOCNOTELIST:
                        Audio_NotePoolClear(seqPlayer->notePool);
                        cmd = AudioSeq_ScriptReadU8(seqScript);
                        Audio_NotePoolFill(seqPlayer->notePool, cmd);
                        // OoT's fake-match: the asm has two breaks in a row here, which the compiler normally
                        // optimizes out.
                        dummy = -1;
                        if (dummy < 0) {
                            dummy = 0;
                        }
                        if (dummy > 1) {
                            dummy = 1;
                        }
                        if (dummy) {}
                        break;

                    case ASEQ_OP_SEQ_FREENOTELIST:
                        Audio_NotePoolClear(seqPlayer->notePool);
                        break;

                    case ASEQ_OP_SEQ_TRANSPOSE:
                        seqPlayer->transposition = 0;
                        /* fallthrough */
                    case ASEQ_OP_SEQ_RTRANSPOSE:
                        seqPlayer->transposition += (s8)AudioSeq_ScriptReadU8(seqScript);
                        break;

                    case ASEQ_OP_SEQ_TEMPO:
                        seqPlayer->tempo = AudioSeq_ScriptReadU8(seqScript) * 48;
                        if (seqPlayer->tempo > gAudioCtx.maxTempo) {
                            seqPlayer->tempo = (u16)gAudioCtx.maxTempo;
                        }

                        if ((s16)seqPlayer->tempo <= 0) {
                            seqPlayer->tempo = 1;
                        }
                        break;

                    case ASEQ_OP_SEQ_TEMPOCHG:
                        seqPlayer->tempoChange = (s8)AudioSeq_ScriptReadU8(seqScript) * 48;
                        break;

                    case ASEQ_OP_SEQ_VOLMODE:
                        cmd = AudioSeq_ScriptReadU8(seqScript);
                        temp = AudioSeq_ScriptReadS16(seqScript);
                        switch (cmd) {
                            case SEQPLAYER_STATE_0:
                            case SEQPLAYER_STATE_FADE_IN:
                                if (seqPlayer->state != SEQPLAYER_STATE_FADE_OUT) {
                                    seqPlayer->fadeTimerUnkEu = temp;
                                    seqPlayer->state = cmd;
                                }
                                break;

                            case SEQPLAYER_STATE_FADE_OUT:
                                seqPlayer->fadeTimer = temp;
                                seqPlayer->state = cmd;
                                seqPlayer->fadeVelocity = (0.0f - seqPlayer->fadeVolume) / (s32)seqPlayer->fadeTimer;
                                break;
                        }
                        break;

                    case ASEQ_OP_SEQ_VOL:
                        value = AudioSeq_ScriptReadU8(seqScript);
                        switch (seqPlayer->state) {
                            case SEQPLAYER_STATE_FADE_IN:
                                seqPlayer->state = SEQPLAYER_STATE_0;
                                seqPlayer->fadeVolume = 0.0f;
                                /* fallthrough */
                            case SEQPLAYER_STATE_0:
                                seqPlayer->fadeTimer = seqPlayer->fadeTimerUnkEu;
                                if (seqPlayer->fadeTimerUnkEu != 0) {
                                    seqPlayer->fadeVelocity =
                                        ((value / 127.0f) - seqPlayer->fadeVolume) / (s32)seqPlayer->fadeTimer;
                                } else {
                                    seqPlayer->fadeVolume = value / 127.0f;
                                }
                                break;

                            case SEQPLAYER_STATE_FADE_OUT:
                                break;
                        }
                        break;

                    case ASEQ_OP_SEQ_VOLSCALE:
                        seqPlayer->fadeVolumeScale = (s8)AudioSeq_ScriptReadU8(seqScript) / 127.0f;
                        break;

                    case ASEQ_OP_SEQ_INITCHAN:
                        temp = AudioSeq_ScriptReadS16(seqScript);
                        AudioSeq_SequencePlayerSetupChannels(seqPlayer, temp);
                        break;

                    case ASEQ_OP_SEQ_FREECHAN:
                        PRINTF("MIDI mode not supported\n");
                        AudioSeq_ScriptReadS16(seqScript);
                        break;

                    case ASEQ_OP_SEQ_MUTESCALE:
                        seqPlayer->muteVolumeScale = (s8)AudioSeq_ScriptReadU8(seqScript) / 127.0f;
                        break;

                    case ASEQ_OP_SEQ_MUTE:
                        seqPlayer->muted = 1;
                        break;

                    case ASEQ_OP_SEQ_MUTEBHV:
                        seqPlayer->muteBehavior = AudioSeq_ScriptReadU8(seqScript);
                        break;

                    case ASEQ_OP_SEQ_LDSHORTVELARR:
                    case ASEQ_OP_SEQ_LDSHORTGATEARR:
                        temp = AudioSeq_ScriptReadS16(seqScript);
                        data3 = (u8*)(seqPlayer->seqData + temp);
                        if (cmd == ASEQ_OP_SEQ_LDSHORTVELARR) {
                            seqPlayer->shortNoteVelocityTable = data3;
                        } else {
                            seqPlayer->shortNoteGateTimeTable = data3;
                        }
                        break;

                    case ASEQ_OP_SEQ_NOTEALLOC:
                        seqPlayer->noteAllocPolicy = AudioSeq_ScriptReadU8(seqScript);
                        break;

                    case ASEQ_OP_SEQ_RAND:
                        cmd = AudioSeq_ScriptReadU8(seqScript);
                        if (cmd == 0) {
                            seqScript->value = (gAudioCtx.audioRandom >> 2) & 0xFF;
                        } else {
                            seqScript->value = (gAudioCtx.audioRandom >> 2) % cmd;
                        }
                        break;

                    case ASEQ_OP_SEQ_DYNCALL:
                        temp = AudioSeq_ScriptReadS16(seqScript);
                        PRINTF("Audio:Track: CTBLCALL Macro Level Over Error!\n");
                        if ((seqScript->value != -1) && (seqScript->depth != 3)) {
                            data = (u8*)(seqPlayer->seqData + (u32)(temp + (seqScript->value << 1)));
                            seqScript->stack[seqScript->depth] = seqScript->pc;
                            seqScript->depth++;

                            temp = (data[0] << 8) + data[1];
                            seqScript->pc = (u8*)(seqPlayer->seqData + temp);
                        }
                        break;

                    case ASEQ_OP_SEQ_LDI:
                        seqScript->value = AudioSeq_ScriptReadU8(seqScript);
                        break;

                    case ASEQ_OP_SEQ_AND:
                        seqScript->value &= AudioSeq_ScriptReadU8(seqScript);
                        break;

                    case ASEQ_OP_SEQ_SUB:
                        seqScript->value -= AudioSeq_ScriptReadU8(seqScript);
                        break;

                    case ASEQ_OP_SEQ_STSEQ:
                        cmd = AudioSeq_ScriptReadU8(seqScript);
                        temp = AudioSeq_ScriptReadS16(seqScript);
                        data2 = (u8*)(seqPlayer->seqData + temp);
                        *data2 = (u8)seqScript->value + cmd;
                        break;

                    case ASEQ_OP_SEQ_STOP:
                        seqPlayer->stopScript = 1;
                        return;

                    case ASEQ_OP_SEQ_SCRIPTCTR:
                        seqPlayer->scriptCounter = (u16)AudioSeq_ScriptReadS16(seqScript);
                        break;
                }
                PRINTF("Group:Undefine upper C0h command (%x)\n");
                continue;
            }

            cmdLowBits = cmd & 0x0F;

            switch (cmd & 0xF0) {
                case ASEQ_OP_SEQ_TESTCHAN:
                    seqScript->value = seqPlayer->channels[cmdLowBits]->h.f.enabled ^ 1;
                    break;

                case 0x10:
                    break;

                case 0x20:
                    break;

                case ASEQ_OP_SEQ_SUBIO:
                    seqScript->value -= seqPlayer->seqScriptIO[cmdLowBits];
                    break;

                case 0x60: // OoT: ASEQ_OP_SEQ_LDRES; a no-op in this driver
                    break;

                case ASEQ_OP_SEQ_STIO:
                    seqPlayer->seqScriptIO[cmdLowBits] = seqScript->value;
                    break;

                case ASEQ_OP_SEQ_LDIO:
                    seqScript->value = seqPlayer->seqScriptIO[cmdLowBits];
                    if (cmdLowBits < 2) {
                        seqPlayer->seqScriptIO[cmdLowBits] = SEQ_IO_VAL_NONE;
                    }
                    break;

                case ASEQ_OP_SEQ_STOPCHAN:
                    AudioSeq_SequenceChannelDisable(seqPlayer->channels[cmdLowBits]);
                    break;

                case ASEQ_OP_SEQ_LDCHAN:
                    temp = AudioSeq_ScriptReadS16(seqScript);
                    AudioSeq_SequenceChannelEnable(seqPlayer, cmdLowBits, (void*)(seqPlayer->seqData + temp));
                    break;

                case ASEQ_OP_SEQ_RLDCHAN:
                    tempS = AudioSeq_ScriptReadS16(seqScript);
                    AudioSeq_SequenceChannelEnable(seqPlayer, cmdLowBits, (void*)&seqScript->pc[tempS]);
                    break;

                case ASEQ_OP_SEQ_LDSEQ:
                    cmd = AudioSeq_ScriptReadU8(seqScript);
                    temp = AudioSeq_ScriptReadS16(seqScript);
                    data2 = (u8*)(seqPlayer->seqData + temp);
                    PRINTF("Seq Load Start %d %d\n");
                    AudioLoad_SlowLoadSeq(cmd, (u8*)data2, &seqPlayer->seqScriptIO[cmdLowBits]);
                    break;
            }
        } while (1);
    }
    PRINTF("Group:Undefined Command\n");

    i = 0;
    do {
        if (seqPlayer->channels[i]->h.f.enabled) {
            PRINTF("Call Sub %x,Gtp %x\n");
            AudioSeq_SequenceChannelProcessScript(seqPlayer->channels[i]);
        }
        i++;
    } while (i < 16);
}

// original name: Nas_MySeqMain
// One update's sequence pass: points sampleStateOffset at the update (`arg0` counts down), runs each enabled
// sequence player's script and Audio_SequencePlayerProcessSound, then Audio_ProcessNotes.
void AudioSeq_ProcessSequences(s32 arg0) {
    u32 i;

    gAudioCtx.sampleStateOffset = (gAudioCtx.audioBufferParameters.updatesPerFrame - arg0 - 1) * gAudioCtx.numNotes;
    for (i = 0; i < gAudioCtx.audioBufferParameters.numSequencePlayers; i++) {
        if (gAudioCtx.seqPlayers[i].enabled == 1) {
            AudioSeq_SequencePlayerProcessSequence(&gAudioCtx.seqPlayers[i]);
            Audio_SequencePlayerProcessSound(&gAudioCtx.seqPlayers[i]);
        }
    }
    Audio_ProcessNotes();
}

// original name: Nas_SeqSkip
// Runs the sequence player through the skipTicks it owes: its script and Audio_SequencePlayerProcessSound per tick.
void AudioSeq_SkipForwardSequence(SequencePlayer* seqPlayer) {
    while (seqPlayer->skipTicks > 0) {
        AudioSeq_SequencePlayerProcessSequence(seqPlayer);
        Audio_SequencePlayerProcessSound(seqPlayer);
        seqPlayer->skipTicks--;
    }
}

// original name: Nas_InitMySeq
// Disables the sequence player and resets its fields (tempo 0x1680, fade-in state, the default short-note tables),
// then each of its 16 channels.
void AudioSeq_ResetSequencePlayer(SequencePlayer* seqPlayer) {
    s32 i;

    AudioSeq_SequencePlayerDisable(seqPlayer);
    seqPlayer->stopScript = 0;
    seqPlayer->delay = 0;
    seqPlayer->state = SEQPLAYER_STATE_FADE_IN;
    seqPlayer->fadeTimer = 0;
    seqPlayer->fadeTimerUnkEu = 0;
    seqPlayer->tempoAcc = 0;
    seqPlayer->tempo = 0x1680;
    seqPlayer->tempoChange = 0;
    seqPlayer->transposition = 0;
    seqPlayer->noteAllocPolicy = 0;
    seqPlayer->shortNoteVelocityTable = gDefaultShortNoteVelocityTable;
    seqPlayer->shortNoteGateTimeTable = gDefaultShortNoteGateTimeTable;
    seqPlayer->scriptCounter = 0;
    seqPlayer->fadeVolume = 1.0f;
    seqPlayer->fadeVolumeScale = 1.0f;
    seqPlayer->fadeVelocity = 0.0f;
    seqPlayer->volume = 0.0f;
    seqPlayer->muteVolumeScale = 0.5f;
    for (i = 0; i < 16; i++) {
        AudioSeq_InitSequenceChannel(seqPlayer->channels[i]);
    }
}

// original name: Nas_AssignSubTrack
// Allocates sequence player `playerIdx`'s 16 channels from the misc pool, sequenceChannelNone where that fails, and
// inits each.
void AudioSeq_InitSequencePlayerChannels(s32 playerIdx) {
    SequencePlayer* seqPlayer;
    SequenceChannel* channel;
    s32 i;
    s32 j;

    seqPlayer = &gAudioCtx.seqPlayers[playerIdx];
    for (i = 0; i < 16; i++) {
        seqPlayer->channels[i] = AudioHeap_AllocZeroed(&gAudioCtx.miscPool, 0xD8);
        if (seqPlayer->channels[i] == NULL) {
            seqPlayer->channels[i] = &gAudioCtx.sequenceChannelNone;
        } else {
            channel = seqPlayer->channels[i];
            channel->seqPlayer = seqPlayer;
            channel->h.f.enabled = 0;
            for (j = 0; j < 6; j++) {
                channel->layers[j] = NULL;
            }
        }
        AudioSeq_InitSequenceChannel(seqPlayer->channels[i]);
    }
}

// original name: __InitGroup
// Inits a sequence player: every channel sequenceChannelNone, its flags and script IO cleared, mute behaviour 0x60,
// its note pool, then AudioSeq_ResetSequencePlayer.
void AudioSeq_InitSequencePlayer(SequencePlayer* seqPlayer) {
    SequenceChannel* placeholder;
    s32 i;
    s32 j;

    placeholder = &gAudioCtx.sequenceChannelNone;
    for (i = 0; i < 16; i++) {
        seqPlayer->channels[i] = placeholder;
    }
    seqPlayer->enabled = 0;
    seqPlayer->muted = 0;
    seqPlayer->fontDmaInProgress = 0;
    seqPlayer->seqDmaInProgress = 0;
    for (j = 0; j < 8; j++) {
        seqPlayer->seqScriptIO[j] = SEQ_IO_VAL_NONE;
    }
    seqPlayer->muteBehavior = 0x60;
    Audio_InitNoteLists(seqPlayer->notePool);
    AudioSeq_ResetSequencePlayer(seqPlayer);
}

// original name: Nas_InitPlayer
// Inits the layer free list, clears every layer's channel and enabled bit, and inits the three sequence players.
void AudioSeq_InitSequencePlayers(void) {
    s32 i;

    AudioSeq_InitLayerFreelist();
    for (i = 0; i < 64; i++) {
        gAudioCtx.sequenceLayers[i].channel = NULL;
        gAudioCtx.sequenceLayers[i].h.f.enabled = 0;
    }
    for (i = 0; i < 3; i++) {
        AudioSeq_InitSequencePlayer(&gAudioCtx.seqPlayers[i]);
    }
}

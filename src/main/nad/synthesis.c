// original filename: driver.c
#include "global.h"
#include "audio.h"

// NA audio driver, OoT's synthesis.c: builds the RSP audio command list each frame (AudioSynth_Update) - the
// reverb ring buffers, the per-update note order and each note's load, decode, resample and envelope chain.
// The PRINTFs are stripped debug prints whose strings stay in .data (-use_readwrite_const), in the ROM's order.

// Several are called above their definitions; in scope, the prototypes narrow the s16/u16 arguments there.
Acmd* AudioSynth_LoadRingBufferPart(Acmd* pkt, u16 dmem, u16 startPos, s32 size, s32 reverbIndex);
Acmd* AudioSynth_SaveRingBufferPart(Acmd* pkt, u16 dmem, u16 startPos, s32 size, s32 reverbIndex);
Acmd* AudioSynth_LeakReverb(Acmd* pkt, s32 reverbIndex);
Acmd* AudioSynth_LoadDownsampledRingBuffer(Acmd* pkt, s32 aiBufLen, s16 reverbIndex, s16 updateIndex);
Acmd* AudioSynth_LoadReverbSamples(Acmd* pkt, s32 aiBufLen, s16 reverbIndex, s16 updateIndex);
Acmd* AudioSynth_SaveReverbSamples(Acmd* pkt, s16 reverbIndex, s16 updateIndex);
void AudioSynth_InitNextRingBuf(s32 chunkLen, s32 updateIndex, s32 reverbIndex);
Acmd* AudioSynth_DoOneAudioUpdate(s32* aiBuf, s32 aiBufLen, Acmd* pkt, s32 updateIndex);

// AudioSynth_ProcessEnvelope's envmixer words: sEnvMixerOp is the w0 template, the other three the w1 it pairs
// with, four DMEM destinations (dry left, dry right, wet left, wet right) as address >> 4.
u32 sEnvMixerOp = _SHIFTL(0x13, 24, 8);
// Left Haas delay: the dry left channel goes to the staging buffer at 0x5C0.
u32 sEnvMixerLeftHaasDmemDests =
    _SHIFTL(0x5C0 >> 4, 24, 8) | _SHIFTL(0xAE0 >> 4, 16, 8) | _SHIFTL(0xC80 >> 4, 8, 8) | _SHIFTL(0xE20 >> 4, 0, 8);
// Right Haas delay: the dry right channel does.
u32 sEnvMixerRightHaasDmemDests =
    _SHIFTL(0x940 >> 4, 24, 8) | _SHIFTL(0x5C0 >> 4, 16, 8) | _SHIFTL(0xC80 >> 4, 8, 8) | _SHIFTL(0xE20 >> 4, 0, 8);
// No delay: both dry channels go to the mix (0x940, 0xAE0).
u32 sEnvMixerDefaultDmemDests =
    _SHIFTL(0x940 >> 4, 24, 8) | _SHIFTL(0xAE0 >> 4, 16, 8) | _SHIFTL(0xC80 >> 4, 8, 8) | _SHIFTL(0xE20 >> 4, 0, 8);

// Unreferenced, as in OoT (its D_801304B0).
u16 D_80097740[] = { 0x7FFF, 0xD001, 0x3FFF, 0xF001, 0x5FFF, 0x9001, 0x7FFF, 0x8001 };

// An A_MIXER command as four halfwords (not OoT's): AudioSynth_ProcessEnvelope's plain-mix path fills its
// commands a field at a time.
typedef struct MixerCmdHalves {
    /* 0x00 */ u16 cmdAndCount; // the command byte and the sample count, as one halfword
    /* 0x02 */ s16 gain;
    /* 0x04 */ u16 dmemIn;
    /* 0x06 */ u16 dmemOut;
} MixerCmdHalves; // size = 0x8

// original name: Nas_CpuFX
// Re-cuts reverb `reverbIndex`'s ring-buffer item for update `updateIndex` at the write position; with
// downsampleRate >= 3 and no frames to ignore it first writes every rate-th sample of the item back to the rings.
// Load-bearing: the three pads.
void AudioSynth_InitNextRingBuf(s32 chunkLen, s32 updateIndex, s32 reverbIndex) {
    ReverbRingBufferItem* bufItem;
    SynthesisReverb* reverb;
    s32 i;
    s32 j;
    s32 numSamples;
    s32 extraSamples;
    UNUSED s32 pad1;
    UNUSED s32 pad2;
    UNUSED s32 pad3;

    reverb = &gAudioCtx.synthesisReverbs[reverbIndex];
    if (reverb->downsampleRate >= 3) {
        if (reverb->framesToIgnore == 0) {
            bufItem = &reverb->items[reverb->curFrame][updateIndex];
            Audio_InvalDCache(bufItem->toDownsampleLeft, 0x340);
            j = 0;
            for (i = 0; i < bufItem->lengthA / 2; i++) {
                reverb->leftRingBuf[bufItem->startPos + i] = bufItem->toDownsampleLeft[j];
                reverb->rightRingBuf[bufItem->startPos + i] = bufItem->toDownsampleRight[j];
                j += reverb->downsampleRate;
            }
            for (i = 0; i < bufItem->lengthB / 2; i++) {
                reverb->leftRingBuf[i] = bufItem->toDownsampleLeft[j];
                reverb->rightRingBuf[i] = bufItem->toDownsampleRight[j];
                j += reverb->downsampleRate;
            }
        }
    }

    bufItem = &reverb->items[reverb->curFrame][updateIndex];
    numSamples = chunkLen / reverb->downsampleRate;
    extraSamples = reverb->nextRingBufPos + numSamples - reverb->bufSizePerChan;
    if (extraSamples < 0) {
        bufItem->lengthA = numSamples * 2;
        bufItem->lengthB = 0;
        bufItem->startPos = reverb->nextRingBufPos;
        reverb->nextRingBufPos = reverb->nextRingBufPos + numSamples;
    } else {
        bufItem->lengthA = (numSamples - extraSamples) * 2;
        bufItem->lengthB = extraSamples * 2;
        bufItem->startPos = reverb->nextRingBufPos;
        reverb->nextRingBufPos = extraSamples;
    }
    bufItem->numSamplesAfterDownsampling = numSamples;
    bufItem->chunkLen = chunkLen;
}

// original name: __Nas_PushDrvReg (OoT's own name is address-derived, func_800DB03C)
// For each note: clears needsInit while the note is enabled, else disables its copy for update `updateIndex`;
// resets harmonicIndexCurAndPrev either way.
void AudioSynth_SyncSampleStates(s32 updateIndex) {
    NoteSampleState* sampleState;
    NoteSampleState* sampleState2;
    s32 i;

    for (i = 0; i < gAudioCtx.numNotes; i++) {
        sampleState = &gAudioCtx.notes[i].sampleState;
        sampleState2 = &gAudioCtx.sampleStates[gAudioCtx.numNotes * updateIndex + i];
        if (sampleState->h.f.enabled) {
            sampleState->h.f.needsInit = 0;
        } else {
            sampleState2->h.f.enabled = 0;
        }
        sampleState->harmonicIndexCurAndPrev = 0;
    }
}

// original name: Nas_smzAudioFrame
// The per-frame entry: runs the sequences and syncs the sample states for every update, then renders each update's
// chunk of `aiBufLen` and steps the reverbs' frames; returns the list's end and its length in `*cmdCnt`.
Acmd* AudioSynth_Update(Acmd* cmdStart, s32* cmdCnt, s32* aiStart, s32 aiBufLen) {
    Acmd* cmdP;
    s32* aiBufP;
    s32 i;
    s32 j;
    s32 chunkLen;

    cmdP = cmdStart;
    for (i = gAudioCtx.audioBufferParameters.updatesPerFrame; i > 0; i--) {
        AudioSeq_ProcessSequences(i - 1);
        AudioSynth_SyncSampleStates(gAudioCtx.audioBufferParameters.updatesPerFrame - i);
    }

    aiBufP = aiStart;
    gAudioCtx.curLoadedBook = 0;

    for (i = gAudioCtx.audioBufferParameters.updatesPerFrame; i > 0; i--) {
        if (i == 1) {
            chunkLen = aiBufLen;
        } else if (aiBufLen / i >= gAudioCtx.audioBufferParameters.samplesPerUpdateMax) {
            chunkLen = gAudioCtx.audioBufferParameters.samplesPerUpdateMax;
        } else if (aiBufLen / i <= gAudioCtx.audioBufferParameters.samplesPerUpdateMin) {
            chunkLen = gAudioCtx.audioBufferParameters.samplesPerUpdateMin;
        } else {
            chunkLen = gAudioCtx.audioBufferParameters.samplesPerUpdate;
        }

        for (j = 0; j < gAudioCtx.numSynthesisReverbs; j++) {
            if (gAudioCtx.synthesisReverbs[j].useReverb != 0) {
                AudioSynth_InitNextRingBuf(chunkLen, gAudioCtx.audioBufferParameters.updatesPerFrame - i, j);
            }
        }

        cmdP = AudioSynth_DoOneAudioUpdate(aiBufP, chunkLen, cmdP, gAudioCtx.audioBufferParameters.updatesPerFrame - i);
        aiBufLen -= chunkLen;
        aiBufP += chunkLen;
    }

    for (j = 0; j < gAudioCtx.numSynthesisReverbs; j++) {
        if (gAudioCtx.synthesisReverbs[j].framesToIgnore != 0) {
            gAudioCtx.synthesisReverbs[j].framesToIgnore--;
        }
        gAudioCtx.synthesisReverbs[j].curFrame ^= 1;
    }

    *cmdCnt = cmdP - cmdStart;
    return cmdP;
}

// original name: __Nas_WaveTerminateProcess (OoT's own name is address-derived, func_800DB2C0)
// Disables note `noteIndex` in every later update of the frame, up to one where it needs init (a restart).
// Load-bearing: `sampleStates[numNotes * i + noteIndex]` spelled out at both uses.
void AudioSynth_DisableNoteInLaterUpdates(s32 updateIndex, s32 noteIndex) {
    s32 i;

    for (i = updateIndex + 1; i < gAudioCtx.audioBufferParameters.updatesPerFrame; i++) {
        if (gAudioCtx.sampleStates[gAudioCtx.numNotes * i + noteIndex].h.f.needsInit) {
            PRINTF("Terminate-Canceled Channel %d,Phase %d\n");
            break;
        }
        gAudioCtx.sampleStates[gAudioCtx.numNotes * i + noteIndex].h.f.enabled = 0;
    }
}

// original name: Nas_LoadAux2nd
// Loads reverb `reverbIndex`'s ring-buffer item for update `updateIndex` into the temp buffer at DMEM 0x3E0, the
// wrapped part after the straight run.
Acmd* AudioSynth_LoadRingBuffer1AtTemp(Acmd* pkt, s16 reverbIndex, s16 updateIndex) {
    ReverbRingBufferItem* bufItem;
    SynthesisReverb* reverb = &gAudioCtx.synthesisReverbs[reverbIndex];

    bufItem = &reverb->items[reverb->curFrame][updateIndex];
    pkt = AudioSynth_LoadRingBufferPart(pkt, 0x3E0, bufItem->startPos, bufItem->lengthA, reverbIndex);
    if (bufItem->lengthB != 0) {
        pkt = AudioSynth_LoadRingBufferPart(pkt, bufItem->lengthA + 0x3E0, 0, bufItem->lengthB, reverbIndex);
    }
    return pkt;
}

// original name: Nas_SaveAux2nd
// The store twin: saves the temp buffer at 0x3E0 back into the same ring-buffer item.
Acmd* AudioSynth_SaveRingBuffer1AtTemp(Acmd* pkt, s16 reverbIndex, s16 updateIndex) {
    ReverbRingBufferItem* bufItem;
    SynthesisReverb* reverb = &gAudioCtx.synthesisReverbs[reverbIndex];

    bufItem = &reverb->items[reverb->curFrame][updateIndex];
    pkt = AudioSynth_SaveRingBufferPart(pkt, 0x3E0, bufItem->startPos, bufItem->lengthA, reverbIndex);
    if (bufItem->lengthB != 0) {
        pkt = AudioSynth_SaveRingBufferPart(pkt, bufItem->lengthA + 0x3E0, 0, bufItem->lengthB, reverbIndex);
    }
    return pkt;
}

// original name: Nas_CrossMix
// Leaks the wet channels into each other: wet left (0xC80) to scratch (0x3E0), wet right (0xE20) into left at
// leakRtl, then the scratch into right at leakLtr.
// Load-bearing: the block-scoped `Acmd* cmd = pkt++;`; on one line, the first command's `w0`/`w1` and the third.
// clang-format off
Acmd* AudioSynth_LeakReverb(Acmd* pkt, s32 reverbIndex) {
    { Acmd* cmd = pkt++; cmd->words.w0 = _SHIFTL(A_DMEMMOVE, 24, 8) | _SHIFTL(0xC80, 0, 16); cmd->words.w1 = _SHIFTL(0x3E0, 16, 16) | _SHIFTL(0x1A0, 0, 16); }
    { Acmd* cmd = pkt++; cmd->words.w0 = _SHIFTL(A_MIXER, 24, 8) | _SHIFTL(0x1A, 16, 8) | _SHIFTL(gAudioCtx.synthesisReverbs[reverbIndex].leakRtl, 0, 16); cmd->words.w1 = _SHIFTL(0xE20, 16, 16) | _SHIFTL(0xC80, 0, 16); }
    { Acmd* cmd = pkt++; cmd->words.w0 = _SHIFTL(A_MIXER, 24, 8) | _SHIFTL(0x1A, 16, 8) | _SHIFTL(gAudioCtx.synthesisReverbs[reverbIndex].leakLtr, 0, 16); cmd->words.w1 = _SHIFTL(0x3E0, 16, 16) | _SHIFTL(0xE20, 0, 16); }
    return pkt;
}
// clang-format on

// original name: Nas_LoadAuxBufferC (OoT's own name is address-derived, func_800DB4E4)
// AudioSynth_LoadReverbSamples' path for a downsampled reverb: loads the item from the 8-sample boundary below its
// start into 0x3E0, then resamples each channel up into the wet channels (0xC80/0xE20) at downsamplePitch.
// Load-bearing: `& ~0xF`, the reverb indexed inline, and the third command's `w0`/`w1` on one line.
// clang-format off
Acmd* AudioSynth_LoadDownsampledRingBuffer(Acmd* pkt, s32 aiBufLen, s16 reverbIndex, s16 updateIndex) {
    ReverbRingBufferItem* item =
        &gAudioCtx.synthesisReverbs[reverbIndex].items[gAudioCtx.synthesisReverbs[reverbIndex].curFrame][updateIndex];
    s16 offsetA = (item->startPos & 7) * 2;
    s16 offsetB = (offsetA + item->lengthA + 0xF) & ~0xF;

    pkt = AudioSynth_LoadRingBufferPart(pkt, 0x3E0, item->startPos - offsetA / 2, 0x1A0,
                                        reverbIndex);
    if (item->lengthB != 0) {
        pkt = AudioSynth_LoadRingBufferPart(pkt, offsetB + 0x3E0, 0, 0x1A0 - offsetB, reverbIndex);
    }
    { Acmd* cmd = pkt++; cmd->words.w0 = _SHIFTL(A_SETBUFF, 24, 8) | _SHIFTL(offsetA + 0x3E0, 0, 16); cmd->words.w1 = _SHIFTL(0xC80, 16, 16) | _SHIFTL(aiBufLen * 2, 0, 16); }
    { Acmd* cmd = pkt++; cmd->words.w0 = _SHIFTL(A_RESAMPLE, 24, 8) | _SHIFTL(gAudioCtx.synthesisReverbs[reverbIndex].resampleFlags, 16, 8) | _SHIFTL(gAudioCtx.synthesisReverbs[reverbIndex].downsamplePitch, 0, 16); cmd->words.w1 = (u32)gAudioCtx.synthesisReverbs[reverbIndex].leftLoadResampleBuf + 0x80000000; }
    { Acmd* cmd = pkt++; cmd->words.w0 = _SHIFTL(A_SETBUFF, 24, 8) | _SHIFTL(offsetA + 0x580, 0, 16); cmd->words.w1 = _SHIFTL(0xE20, 16, 16) | _SHIFTL(aiBufLen * 2, 0, 16); }
    { Acmd* cmd = pkt++; cmd->words.w0 = _SHIFTL(A_RESAMPLE, 24, 8) | _SHIFTL(gAudioCtx.synthesisReverbs[reverbIndex].resampleFlags, 16, 8) | _SHIFTL(gAudioCtx.synthesisReverbs[reverbIndex].downsamplePitch, 0, 16); cmd->words.w1 = (u32)gAudioCtx.synthesisReverbs[reverbIndex].rightLoadResampleBuf + 0x80000000; }
    return pkt;
}
// clang-format on

// original name: Nas_DelayFilter
// Filters each wet channel (0xC80/0xE20) whose filter is set: one command loads filterLeft/filterRight, a second
// runs it with the side's state. A_SEGMENT (7) is this microcode's A_FILTER.
Acmd* AudioSynth_FilterReverb(Acmd* pkt, s32 size, s32 reverbIndex) {
    if (gAudioCtx.synthesisReverbs[reverbIndex].filterLeft != 0) {
        {
            Acmd* cmd = pkt++;
            cmd->words.w0 = _SHIFTL(A_SEGMENT, 24, 8) | _SHIFTL(2, 16, 8) | _SHIFTL(size, 0, 16);
            cmd->words.w1 = (u32)gAudioCtx.synthesisReverbs[reverbIndex].filterLeft;
        }
        {
            Acmd* cmd = pkt++;
            cmd->words.w0 = _SHIFTL(A_SEGMENT, 24, 8) |
                            _SHIFTL(gAudioCtx.synthesisReverbs[reverbIndex].resampleFlags, 16, 8) |
                            _SHIFTL(0xC80, 0, 16);
            cmd->words.w1 = (u32)gAudioCtx.synthesisReverbs[reverbIndex].filterLeftState;
        }
    }
    if (gAudioCtx.synthesisReverbs[reverbIndex].filterRight != 0) {
        {
            Acmd* cmd = pkt++;
            cmd->words.w0 = _SHIFTL(A_SEGMENT, 24, 8) | _SHIFTL(2, 16, 8) | _SHIFTL(size, 0, 16);
            cmd->words.w1 = (u32)gAudioCtx.synthesisReverbs[reverbIndex].filterRight;
        }
        {
            Acmd* cmd = pkt++;
            cmd->words.w0 = _SHIFTL(A_SEGMENT, 24, 8) |
                            _SHIFTL(gAudioCtx.synthesisReverbs[reverbIndex].resampleFlags, 16, 8) |
                            _SHIFTL(0xE20, 0, 16);
            cmd->words.w1 = (u32)gAudioCtx.synthesisReverbs[reverbIndex].filterRightState;
        }
    }
    return pkt;
}

// original name: Nas_SendLine
// Mixes reverb `reverbIndex`'s wet channels into reverb mixReverbIndex's ring-buffer item at mixReverbStrength,
// when that reverb is not downsampled.
// Load-bearing: the K&R definition (its call does not narrow `updateIndex`), `k`, and the reverb indexed inline.
Acmd* AudioSynth_MaybeMixRingBuffer1(pkt, reverbIndex, updateIndex)
Acmd* pkt;
s32 reverbIndex;
s16 updateIndex;
{
    s32 k = updateIndex;

    if (gAudioCtx.synthesisReverbs[gAudioCtx.synthesisReverbs[reverbIndex].mixReverbIndex].downsampleRate == 1) {
        pkt = AudioSynth_LoadRingBuffer1AtTemp(pkt, gAudioCtx.synthesisReverbs[reverbIndex].mixReverbIndex, k);
        pkt->words.w0 = _SHIFTL(A_MIXER, 24, 8) | _SHIFTL(0x34, 16, 8) |
                        _SHIFTL(gAudioCtx.synthesisReverbs[reverbIndex].mixReverbStrength, 0, 16);
        pkt->words.w1 = _SHIFTL(0xC80, 16, 16) | _SHIFTL(0x3E0, 0, 16);
        pkt = AudioSynth_SaveRingBuffer1AtTemp(pkt + 1, gAudioCtx.synthesisReverbs[reverbIndex].mixReverbIndex, k);
    }
    return pkt;
}

// OoT's 39 one-command builders, slot for slot: each writes one Acmd for the microcode command it is named after;
// A_POLEF (14) is HiLoGain here, A_ENVMIXER (3) OoT's A_UNK3, A_SEGMENT (7) A_FILTER. The 26 empty stubs take MM's
// names, AudioSynth_Noop1..26; Noop5 and Noop11 sit in OoT's AudioSynth_Mix and AudioSynth_DMemMove slots.
void AudioSynth_Noop1(void) {
}

void AudioSynth_ClearBuffer(Acmd* pkt, s32 dmem, s32 size) {
    pkt->words.w0 = _SHIFTL(A_CLEARBUFF, 24, 8) | _SHIFTL(dmem, 0, 24);
    pkt->words.w1 = (u32)size;
}

void AudioSynth_Noop2(void) {
}

void AudioSynth_Noop3(void) {
}

void AudioSynth_Noop4(void) {
}

void AudioSynth_Noop5(void) {
}

void AudioSynth_Noop6(void) {
}

void AudioSynth_Noop7(void) {
}

void AudioSynth_Noop8(void) {
}

void AudioSynth_SetBuffer(Acmd* pkt, s32 flags, s32 dmemIn, s32 dmemOut, s32 size) {
    pkt->words.w0 = _SHIFTL(A_SETBUFF, 24, 8) | _SHIFTL(flags, 16, 8) | _SHIFTL(dmemIn, 0, 16);
    pkt->words.w1 = _SHIFTL(dmemOut, 16, 16) | _SHIFTL(size, 0, 16);
}

void AudioSynth_Noop9(void) {
}

void AudioSynth_Noop10(void) {
}

void AudioSynth_Noop11(void) {
}

void AudioSynth_Noop12(void) {
}

void AudioSynth_Noop13(void) {
}

void AudioSynth_Noop14(void) {
}

void AudioSynth_Noop15(void) {
}

void AudioSynth_InterL(Acmd* pkt, s32 dmemIn, s32 dmemOut, s32 numSamples) {
    pkt->words.w0 = _SHIFTL(0x11, 24, 8) | _SHIFTL(numSamples, 0, 16);
    pkt->words.w1 = _SHIFTL(dmemIn, 16, 16) | _SHIFTL(dmemOut, 0, 16);
}

void AudioSynth_EnvSetup1(Acmd* pkt, s32 reverbVol, s32 rampReverb, s32 rampLeft, s32 rampRight) {
    pkt->words.w0 = _SHIFTL(0x12, 24, 8) | _SHIFTL(reverbVol, 16, 8) | _SHIFTL(rampReverb, 0, 16);
    pkt->words.w1 = _SHIFTL(rampLeft, 16, 16) | _SHIFTL(rampRight, 0, 16);
}

void AudioSynth_Noop16(void) {
}

void AudioSynth_LoadBuffer(Acmd* pkt, s32 dmemDest, s32 size, s32 addrSrc) {
    pkt->words.w0 = _SHIFTL(0x14, 24, 8) | _SHIFTL(size >> 4, 16, 8) | _SHIFTL(dmemDest, 0, 16);
    pkt->words.w1 = (u32)addrSrc;
}

void AudioSynth_SaveBuffer(Acmd* pkt, s32 dmemSrc, s32 size, s32 addrDest) {
    pkt->words.w0 = _SHIFTL(0x15, 24, 8) | _SHIFTL(size >> 4, 16, 8) | _SHIFTL(dmemSrc, 0, 16);
    pkt->words.w1 = (u32)addrDest;
}

void AudioSynth_EnvSetup2(Acmd* pkt, s32 volLeft, s32 volRight) {
    pkt->words.w0 = _SHIFTL(0x16, 24, 8);
    pkt->words.w1 = _SHIFTL(volLeft, 16, 16) | _SHIFTL(volRight, 0, 16);
}

void AudioSynth_Noop17(void) {
}

void AudioSynth_Noop18(void) {
}

void AudioSynth_Noop19(void) {
}

void AudioSynth_S8Dec(Acmd* pkt, s32 flags, s32 state) {
    pkt->words.w0 = _SHIFTL(0x17, 24, 8) | _SHIFTL(flags, 16, 8);
    pkt->words.w1 = (u32)state;
}

void AudioSynth_HiLoGain(Acmd* pkt, s32 gain, s32 dmemIn, s32 dmemOut, s32 size) {
    pkt->words.w0 = _SHIFTL(A_POLEF, 24, 8) | _SHIFTL(gain, 16, 8) | _SHIFTL(size, 0, 16);
    pkt->words.w1 = _SHIFTL(dmemIn, 16, 16) | _SHIFTL(dmemOut, 0, 16);
}

// Opcodes 0x19 and 3 (OoT's A_UNK19 and A_UNK3) are undocumented; AudioSynth_ProcessNote applies 0x19 for
// bookOffset 3 and opcode 3 for bookOffset 2.
void AudioSynth_UnkCmd19(Acmd* pkt, s32 dmem1, s32 dmem2, s32 size, s32 arg4) {
    pkt->words.w0 = _SHIFTL(0x19, 24, 8) | _SHIFTL(arg4, 16, 8) | _SHIFTL(size, 0, 16);
    pkt->words.w1 = _SHIFTL(dmem1, 16, 16) | _SHIFTL(dmem2, 0, 16);
}

void AudioSynth_Noop20(void) {
}

void AudioSynth_Noop21(void) {
}

void AudioSynth_Noop22(void) {
}

void AudioSynth_Noop23(void) {
}

void AudioSynth_UnkCmd3(Acmd* pkt, s32 dmem1, s32 dmem2, s32 size) {
    pkt->words.w0 = _SHIFTL(A_ENVMIXER, 24, 8) | _SHIFTL(size, 0, 16);
    pkt->words.w1 = _SHIFTL(dmem1, 16, 16) | _SHIFTL(dmem2, 0, 16);
}

void AudioSynth_Noop24(void) {
}

void AudioSynth_Noop25(void) {
}

void AudioSynth_Noop26(void) {
}

void AudioSynth_LoadFilterBuffer(Acmd* pkt, s32 flags, s32 buf, s32 addr) {
    pkt->words.w0 = _SHIFTL(A_SEGMENT, 24, 8) | _SHIFTL(flags, 16, 8) | _SHIFTL(buf, 0, 16);
    pkt->words.w1 = (u32)addr;
}

void AudioSynth_LoadFilterSize(Acmd* pkt, s32 size, s32 addr) {
    pkt->words.w0 = _SHIFTL(A_SEGMENT, 24, 8) | _SHIFTL(2, 16, 8) | _SHIFTL(size, 0, 16);
    pkt->words.w1 = (u32)addr;
}

// original name: Nas_LoadAuxBuffer1
// AudioSynth_LoadRingBuffer1AtTemp into the wet channels at 0xC80: the path for a reverb that is not downsampled.
Acmd* AudioSynth_LoadRingBuffer1(Acmd* pkt, UNUSED s32 aiBufLen, s16 reverbIndex, s16 updateIndex) {
    ReverbRingBufferItem* bufItem;
    SynthesisReverb* reverb = &gAudioCtx.synthesisReverbs[reverbIndex];

    bufItem = &reverb->items[reverb->curFrame][updateIndex];
    pkt = AudioSynth_LoadRingBufferPart(pkt, 0xC80, bufItem->startPos, bufItem->lengthA, reverbIndex);
    if (bufItem->lengthB != 0) {
        pkt = AudioSynth_LoadRingBufferPart(pkt, bufItem->lengthA + 0xC80, 0, bufItem->lengthB, reverbIndex);
    }
    return pkt;
}

// original name: __LoadAuxBuf
// Two A_LOADBUFF (0x14) commands: `size` bytes of the left and right ring buffers, `startPos` samples in, into DMEM
// `dmem` and `dmem + 0x1A0` (+ 0x80000000 makes the KSEG0 address physical).
// Load-bearing: the block-scoped `Acmd* cmd = pkt++;` and the first command on one line, as in its twin below.
// clang-format off
Acmd* AudioSynth_LoadRingBufferPart(Acmd* pkt, u16 dmem, u16 startPos, s32 size, s32 reverbIndex) {
    { Acmd* cmd = pkt++; cmd->words.w0 = _SHIFTL(0x14, 24, 8) | _SHIFTL(size >> 4, 16, 8) | _SHIFTL(dmem, 0, 16); cmd->words.w1 = (u32)(gAudioCtx.synthesisReverbs[reverbIndex].leftRingBuf + startPos) + 0x80000000; }
    { Acmd* cmd = pkt++; cmd->words.w0 = _SHIFTL(0x14, 24, 8) | _SHIFTL(size >> 4, 16, 8) | _SHIFTL(dmem + 0x1A0, 0, 16); cmd->words.w1 = (u32)(gAudioCtx.synthesisReverbs[reverbIndex].rightRingBuf + startPos) + 0x80000000; }
    return pkt;
}
// clang-format on

// original name: __SaveAuxBuf
// AudioSynth_LoadRingBufferPart's store twin (A_SAVEBUFF, 0x15).
// Load-bearing: its twin's layout.
// clang-format off
Acmd* AudioSynth_SaveRingBufferPart(Acmd* pkt, u16 dmem, u16 startPos, s32 size, s32 reverbIndex) {
    { Acmd* cmd = pkt++; cmd->words.w0 = _SHIFTL(0x15, 24, 8) | _SHIFTL(size >> 4, 16, 8) | _SHIFTL(dmem, 0, 16); cmd->words.w1 = (u32)(gAudioCtx.synthesisReverbs[reverbIndex].leftRingBuf + startPos) + 0x80000000; }
    { Acmd* cmd = pkt++; cmd->words.w0 = _SHIFTL(0x15, 24, 8) | _SHIFTL(size >> 4, 16, 8) | _SHIFTL(dmem + 0x1A0, 0, 16); cmd->words.w1 = (u32)(gAudioCtx.synthesisReverbs[reverbIndex].rightRingBuf + startPos) + 0x80000000; }
    return pkt;
}
// clang-format on

// original name: Nas_LoadAuxBuffer
// Loads reverb `reverbIndex`'s samples into the wet channels (0xC80) and, as OoT's AudioSynth_DoOneAudioUpdate does
// inline, mixes them into 0x940 at `volume`, decays them by decayRatio and leaks them if a leak level is set.
// Load-bearing: `s32 pad[2]`, the block-scoped `Acmd* cmd = pkt++;`, and the second command on one line.
// clang-format off
Acmd* AudioSynth_LoadReverbSamples(Acmd* pkt, s32 aiBufLen, s16 reverbIndex, s16 updateIndex) {
    UNUSED s32 pad[2];

    if (gAudioCtx.synthesisReverbs[reverbIndex].downsampleRate == 1) {
        pkt = AudioSynth_LoadRingBuffer1(pkt, aiBufLen, reverbIndex, updateIndex);
    } else {
        pkt = AudioSynth_LoadDownsampledRingBuffer(pkt, aiBufLen, reverbIndex, updateIndex);
    }

    { Acmd* cmd = pkt++; cmd->words.w0 = _SHIFTL(A_MIXER, 24, 8) | _SHIFTL(0x34, 16, 8) | _SHIFTL(gAudioCtx.synthesisReverbs[reverbIndex].volume, 0, 16); cmd->words.w1 = _SHIFTL(0xC80, 16, 16) | _SHIFTL(0x940, 0, 16); }
    { Acmd* cmd = pkt++; cmd->words.w0 = _SHIFTL(A_MIXER, 24, 8) | _SHIFTL(0x34, 16, 8) | _SHIFTL(gAudioCtx.synthesisReverbs[reverbIndex].decayRatio + 0x8000, 0, 16); cmd->words.w1 = _SHIFTL(0xC80, 16, 16) | _SHIFTL(0xC80, 0, 16); }

    if (gAudioCtx.synthesisReverbs[reverbIndex].leakRtl != 0 || gAudioCtx.synthesisReverbs[reverbIndex].leakLtr != 0) {
        pkt = AudioSynth_LeakReverb(pkt, reverbIndex);
    }
    return pkt;
}
// clang-format on

// original name: Nas_SaveAuxBuffer
// Saves reverb `reverbIndex`'s wet channels into its ring-buffer item, or for a downsampled reverb all 0x340 bytes
// to the item's toDownsampleLeft for AudioSynth_InitNextRingBuf; then clears resampleFlags.
// Load-bearing: `if (reverbIndex) {}`, the reverb indexed inline, the else arm's full access, the call's `pkt++`.
Acmd* AudioSynth_SaveReverbSamples(Acmd* pkt, s16 reverbIndex, s16 updateIndex) {
    ReverbRingBufferItem* bufItem =
        &gAudioCtx.synthesisReverbs[reverbIndex].items[gAudioCtx.synthesisReverbs[reverbIndex].curFrame][updateIndex];

    if (reverbIndex) {}
    if (gAudioCtx.synthesisReverbs[reverbIndex].downsampleRate == 1) {
        pkt = AudioSynth_SaveRingBufferPart(pkt, 0xC80, bufItem->startPos, bufItem->lengthA, reverbIndex);
        if (bufItem->lengthB != 0) {
            pkt = AudioSynth_SaveRingBufferPart(pkt, bufItem->lengthA + 0xC80, 0, bufItem->lengthB, reverbIndex);
        }
    } else {
        AudioSynth_SaveBuffer(pkt++, 0xC80, 0x340,
                              (u32)gAudioCtx.synthesisReverbs[reverbIndex]
                                      .items[gAudioCtx.synthesisReverbs[reverbIndex].curFrame][updateIndex]
                                      .toDownsampleLeft +
                                  0x80000000);
    }
    gAudioCtx.synthesisReverbs[reverbIndex].resampleFlags = 0;
    return pkt;
}

Acmd* AudioSynth_ProcessNote(s32 noteIndex, NoteSampleState* sampleState, NoteSynthesisState* synthState, s32* aiBuf,
                             s32 aiBufLen, Acmd* pkt, s32 updateIndex);

// original name: Nas_DriveRsp
// Builds one update's list: sorts the enabled notes by reverb (out-of-range ones last), clears the mix at 0x940,
// renders each reverb's notes between its load and its filter/save/send, and interleaves the mix out to `aiBuf`.
// Load-bearing: the volatile `enabled` bit (audio.h); the union `p`; no `u8` copy of `noteIndices[i]`; the
// declaration order; the reverb indexed inline and `aiBufLen * 2` spelled out; `(s32)((u32)x >> 5)` in the guards,
// the second as a subscript; each command on one line; the prototypes above.
// clang-format off
Acmd* AudioSynth_DoOneAudioUpdate(s32* aiBuf, s32 aiBufLen, Acmd* pkt, s32 updateIndex) {
    u8 noteIndices[60];
    union {
        NoteSampleState* hw;
        Acmd* cmd;
    } p;
    s32 i;
    s32 useReverb;
    s16 count;
    s16 reverbIndex;

    count = 0;
    if (gAudioCtx.numSynthesisReverbs == 0) {
        for (i = 0; i < gAudioCtx.numNotes; i++) {
            if (gAudioCtx.sampleStates[gAudioCtx.numNotes * updateIndex + i].h.f.enabled) {
                noteIndices[count++] = i;
            }
        }
    } else {
        for (reverbIndex = 0; reverbIndex < gAudioCtx.numSynthesisReverbs; reverbIndex++) {
            for (i = 0; i < gAudioCtx.numNotes; i++) {
                p.hw = &gAudioCtx.sampleStates[gAudioCtx.numNotes * updateIndex + i];
                if (p.hw->h.f.enabled && ((u32)p.hw->h.b.bitField1 >> 5) == reverbIndex) {
                    noteIndices[count++] = i;
                }
            }
        }
        for (i = 0; i < gAudioCtx.numNotes; i++) {
            p.hw = &gAudioCtx.sampleStates[gAudioCtx.numNotes * updateIndex + i];
            if (p.hw->h.f.enabled && (s32)((u32)p.hw->h.b.bitField1 >> 5) >= gAudioCtx.numSynthesisReverbs) {
                noteIndices[count++] = i;
            }
        }
    }

    p.cmd = pkt++; p.cmd->words.w0 = _SHIFTL(A_CLEARBUFF, 24, 8) | _SHIFTL(0x940, 0, 24); p.cmd->words.w1 = 0x340;

    i = 0;
    for (reverbIndex = 0; reverbIndex < gAudioCtx.numSynthesisReverbs; reverbIndex++) {
        useReverb = gAudioCtx.synthesisReverbs[reverbIndex].useReverb;
        if (useReverb != 0) {
            pkt = AudioSynth_LoadReverbSamples(pkt, aiBufLen, reverbIndex, updateIndex);
        }
        while (i < count) {
            if ((s32)((u32)gAudioCtx.sampleStates[noteIndices[i] + updateIndex * gAudioCtx.numNotes].h.b.bitField1 >>
                      5) != reverbIndex) {
                break;
            }
            pkt = AudioSynth_ProcessNote(noteIndices[i],
                                         &gAudioCtx.sampleStates[noteIndices[i] + updateIndex * gAudioCtx.numNotes],
                                         &gAudioCtx.notes[noteIndices[i]].synthesisState,
                                         aiBuf, aiBufLen, pkt, updateIndex);
            i++;
        }
        if (useReverb != 0) {
            if (gAudioCtx.synthesisReverbs[reverbIndex].filterLeft != 0 || gAudioCtx.synthesisReverbs[reverbIndex].filterRight != 0) {
                pkt = AudioSynth_FilterReverb(pkt, aiBufLen * 2, reverbIndex);
            }
            pkt = AudioSynth_SaveReverbSamples(pkt, reverbIndex, updateIndex);
            if (gAudioCtx.synthesisReverbs[reverbIndex].mixReverbIndex != -1) {
                pkt = AudioSynth_MaybeMixRingBuffer1(pkt, reverbIndex, updateIndex);
            }
        }
    }

    for (; i < count; i++) {
        pkt = AudioSynth_ProcessNote(noteIndices[i],
                                     &gAudioCtx.sampleStates[noteIndices[i] + updateIndex * gAudioCtx.numNotes],
                                     &gAudioCtx.notes[noteIndices[i]].synthesisState,
                                     aiBuf, aiBufLen, pkt, updateIndex);
    }

    { Acmd* cmd = pkt++; cmd->words.w0 = _SHIFTL(A_INTERLEAVE, 24, 8) | _SHIFTL(aiBufLen * 2 >> 4, 16, 8) | _SHIFTL(0x3C0, 0, 16); cmd->words.w1 = _SHIFTL(0x940, 16, 16) | _SHIFTL(0xAE0, 0, 16); }
    { Acmd* cmd = pkt++; cmd->words.w0 = _SHIFTL(0x15, 24, 8) | _SHIFTL((aiBufLen * 2 * 2) >> 4, 16, 8) | _SHIFTL(0x3C0, 0, 16); cmd->words.w1 = (u32)aiBuf + 0x80000000; }
    return pkt;
}
// clang-format on

// Called by AudioSynth_ProcessNote, defined below it; the prototypes narrow the u16 arguments.
Acmd* AudioSynth_FinalResample(Acmd* pkt, NoteSynthesisState* synthState, s32 size, u16 pitch, u16 inpDmem,
                               s32 resampleFlags);
Acmd* AudioSynth_ProcessEnvelope(Acmd* pkt, NoteSampleState* sampleState, NoteSynthesisState* synthState, s32 aiBufLen,
                                 u16 dmemSrc, s32 haasEffectDelaySide, s32 flags);
Acmd* AudioSynth_LoadWaveSamples(Acmd* pkt, NoteSampleState* sampleState, NoteSynthesisState* synthState,
                                 s32 numSamplesToLoad);
Acmd* AudioSynth_ApplyHaasEffect(Acmd* pkt, NoteSampleState* sampleState, NoteSynthesisState* synthState, s32 size,
                                 s32 flags, s32 haasEffectDelaySide);

// original name: Nas_SynthMain
// Renders note `noteIndex` for update `updateIndex`: load, decode, resample, gain, filter, envelope and Haas
// effect. OoT's AudioSynth_ProcessNote with OoT's local names, but for dmemOut and dmemDst (OoT's s5 and phi_s4).
// Load-bearing: OoT's declaration list in OoT's order with the pads, nothing below `finished`, `nEntries` at
// function scope; the empty-armed if/else-if on numParts in this term order; `& 0xFFFF` on samplePosFrac; `(s32)`
// on both needsInit tests; the bitfield write of `finished`; samplePosInt read at each use; `addr` an s16;
// `sampleDataOffset` named; the `switch (curPart)` arms; the 0x14 load and the A_DMEMMOVE on one line.
// clang-format off
Acmd* AudioSynth_ProcessNote(s32 noteIndex, NoteSampleState* sampleState, NoteSynthesisState* synthState,
                             UNUSED s32* aiBuf, s32 aiBufLen, Acmd* pkt, s32 updateIndex) {
    UNUSED s32 pad1[3];
    Sample* sample;
    AdpcmLoop* loopInfo;
    s32 nSamplesUntilLoopEnd;
    s32 nSamplesInThisIteration;
    s32 noteFinished;
    s32 restart;
    s32 flags;
    u16 resamplingRateFixedPoint;
    s32 nSamplesInFirstFrame;
    UNUSED s32 pad2;
    s32 gain;
    s32 frameIndex;
    s32 skipBytes;
    UNUSED s32 pad3;
    u32 nEntries;
    s32 nSamplesToDecode;
    s32 sampleAddr;
    u32 samplesLenFixedPoint;
    s32 samplesLenAdjusted;
    s32 nSamplesProcessed;
    s32 loopEndPos;
    s32 nSamplesToProcess;
    s32 dmemDst;
    s32 nFirstFrameSamplesToIgnore;
    UNUSED s32 pad4[3];
    s32 nTrailingSamplesToIgnore;
    UNUSED s32 pad5[3];
    s32 frameSize;
    s32 nFramesToDecode;
    s32 skipInitialSamples;
    s32 sampleDataStart;
    s32 sampleData;
    s32 nParts;
    s32 curPart;
    s32 sampleDataStartPad;
    s32 haasEffectDelaySide;
    s32 resampledTempLen;
    u16 sampleDmemBeforeResampling;
    s32 sampleDataOffset;
    s32 aligned;
    s32 dmemOut;
    Note* note;
    u32 numSamplesToLoad;
    s16 addr;
    UNUSED s32 pad6;
    s32 bookOffset;
    s32 finished;

    bookOffset = sampleState->h.f.bookOffset;
    finished = sampleState->h.f.finished;
    note = &gAudioCtx.notes[noteIndex];
    flags = 0;

    if ((s32)sampleState->h.f.needsInit == 1) {
        flags = 1;
        synthState->restart = 0;
        synthState->samplePosInt = 0;
        synthState->samplePosFrac = 0;
        synthState->curVolLeft = 0;
        synthState->curVolRight = 0;
        synthState->prevHaasEffectLeftDelaySize = 0;
        synthState->prevHaasEffectRightDelaySize = 0;
        synthState->reverbVol = sampleState->reverbVol;
        synthState->numParts = 0;
        note->sampleState.h.f.finished = 0;
        finished = 0;
    }

    resamplingRateFixedPoint = sampleState->resamplingRateFixedPoint;
    nParts = sampleState->h.f.hasTwoParts + 1;
    samplesLenFixedPoint = resamplingRateFixedPoint * aiBufLen * 2 + synthState->samplePosFrac;
    numSamplesToLoad = samplesLenFixedPoint >> 16;
    synthState->samplePosFrac = samplesLenFixedPoint & 0xFFFF;

    if (synthState->numParts == 1 && nParts == 2) {
        PRINTF("S->W\n");
    } else if (synthState->numParts == 2 && nParts == 1) {
        PRINTF("W->S\n");
    } else {
    }

    synthState->numParts = nParts;

    if (sampleState->h.f.isSyntheticWave) {
        pkt = AudioSynth_LoadWaveSamples(pkt, sampleState, synthState, numSamplesToLoad);
        sampleDmemBeforeResampling = 0x580 + synthState->samplePosInt * 2;
        synthState->samplePosInt += numSamplesToLoad;
    } else {
        sample = sampleState->tunedSample->sample;
        loopInfo = sample->loop;
        loopEndPos = loopInfo->end;
        sampleAddr = (s32)sample->sampleAddr;
        resampledTempLen = 0;

        for (curPart = 0; curPart < nParts; curPart++) {
            nSamplesProcessed = 0;
            dmemOut = 0;

            if (nParts == 1) {
                samplesLenAdjusted = numSamplesToLoad;
            } else if (numSamplesToLoad & 1) {
                samplesLenAdjusted = (numSamplesToLoad & ~1) + curPart * 2;
            } else {
                samplesLenAdjusted = numSamplesToLoad;
            }

            if (sample->codec == CODEC_ADPCM || sample->codec == CODEC_SMALL_ADPCM) {
                if (gAudioCtx.curLoadedBook != (s32)sample->book + 8) {
                    switch (bookOffset) {
                        case 1:
                            gAudioCtx.curLoadedBook = (s32)&gInvalidAdpcmCodeBook[1];
                            break;
                        case 2:
                        case 3:
                        default:
                            gAudioCtx.curLoadedBook = (s32)sample->book + 8;
                            break;
                    }
                    nEntries = 16 * sample->book->order * sample->book->numPredictors;
                    {
                        Acmd* cmd = pkt++;

                        cmd->words.w0 = _SHIFTL(A_LOADADPCM, 24, 8) | _SHIFTL(nEntries, 0, 24);
                        cmd->words.w1 = gAudioCtx.curLoadedBook + 0x80000000;
                    }
                }
            }

            while (nSamplesProcessed != samplesLenAdjusted) {
                noteFinished = 0;
                restart = 0;
                dmemDst = 0;

                nFirstFrameSamplesToIgnore = synthState->samplePosInt & 0xF;
                nSamplesUntilLoopEnd = loopEndPos - synthState->samplePosInt;
                nSamplesToProcess = samplesLenAdjusted - nSamplesProcessed;

                if (nFirstFrameSamplesToIgnore == 0 && !synthState->restart) {
                    nFirstFrameSamplesToIgnore = 16;
                }
                nSamplesInFirstFrame = 16 - nFirstFrameSamplesToIgnore;

                if (nSamplesToProcess < nSamplesUntilLoopEnd) {
                    nFramesToDecode = (nSamplesToProcess - nSamplesInFirstFrame + 16 - 1) / 16;
                    nSamplesToDecode = nFramesToDecode * 16;
                    nTrailingSamplesToIgnore = nSamplesInFirstFrame + nSamplesToDecode - nSamplesToProcess;
                } else {
                    nSamplesToDecode = nSamplesUntilLoopEnd - nSamplesInFirstFrame;
                    nTrailingSamplesToIgnore = 0;
                    if (nSamplesToDecode <= 0) {
                        nSamplesToDecode = 0;
                        nSamplesInFirstFrame = nSamplesUntilLoopEnd;
                    }
                    nFramesToDecode = (nSamplesToDecode + 16 - 1) / 16;
                    if (loopInfo->count != 0) {
                        restart = 1;
                    } else {
                        noteFinished = 1;
                    }
                }

                switch (sample->codec) {
                    case CODEC_ADPCM:
                        frameSize = 9;
                        skipInitialSamples = 16;
                        sampleDataStart = 0;
                        break;
                    case CODEC_SMALL_ADPCM:
                        frameSize = 5;
                        skipInitialSamples = 16;
                        sampleDataStart = 0;
                        break;
                    case CODEC_S8:
                        frameSize = 16;
                        skipInitialSamples = 16;
                        sampleDataStart = 0;
                        break;
                    case CODEC_S16_INMEMORY:
                        goto post;
                }

                if (nFramesToDecode != 0) {
                    frameIndex = (synthState->samplePosInt + skipInitialSamples - nFirstFrameSamplesToIgnore) / 16;
                    sampleDataOffset = frameIndex * frameSize;
                    if (sample->medium == MEDIUM_RAM) {
                        sampleData = sampleDataStart + sampleDataOffset + sampleAddr;
                    } else {
                        sampleData = AudioLoad_DmaSampleData(sampleDataStart + sampleDataOffset + sampleAddr, ((nFramesToDecode * frameSize) + 16 + 15) & ~0xF, flags,
                                             &synthState->sampleDmaIndex, sample->medium);
                    }
                    sampleDataStartPad = sampleData & 0xF;
                    aligned = ((nFramesToDecode * frameSize) + 16 + 15) & ~0xF;
                    addr = 0x940 - aligned;
                    { Acmd* cmd = pkt++; cmd->words.w0 = _SHIFTL(0x14, 24, 8) | _SHIFTL(aligned >> 4, 16, 8) | _SHIFTL(addr, 0, 16); cmd->words.w1 = (sampleData - sampleDataStartPad) + 0x80000000; }
                } else {
                    nSamplesToDecode = 0;
                    sampleDataStartPad = 0;
                }

                if (synthState->restart) {
                    {
                        Acmd* cmd = pkt++;

                        cmd->words.w0 = _SHIFTL(A_SETLOOP, 24, 8);
                        cmd->words.w1 = (u32)sample->loop + 0x80000010;
                    }
                    flags = 2;
                    synthState->restart = 0;
                }

                nSamplesInThisIteration = nSamplesToDecode + nSamplesInFirstFrame - nTrailingSamplesToIgnore;
                if (nSamplesProcessed == 0) {
                    skipBytes = nFirstFrameSamplesToIgnore * 2;
                } else {
                    dmemDst = (dmemOut + 16 + 15) & ~0xF;
                }

                switch (sample->codec) {
                    case CODEC_ADPCM:
                        aligned = ((nFramesToDecode * frameSize) + 16 + 15) & ~0xF;
                        addr = 0x940 - aligned;
                        {
                            Acmd* cmd = pkt++;

                            cmd->words.w0 = _SHIFTL(A_SETBUFF, 24, 8) | _SHIFTL(0, 16, 8) | _SHIFTL(addr + sampleDataStartPad, 0, 16);
                            cmd->words.w1 = _SHIFTL(0x580 + dmemDst, 16, 16) | _SHIFTL(nSamplesToDecode * 2, 0, 16);
                        }
                        {
                            Acmd* cmd = pkt++;

                            cmd->words.w0 = _SHIFTL(A_ADPCM, 24, 8) | _SHIFTL(flags, 16, 8);
                            cmd->words.w1 = (u32)synthState->synthesisBuffers + 0x80000000;
                        }
                        break;
                    case CODEC_SMALL_ADPCM:
                        aligned = ((nFramesToDecode * frameSize) + 16 + 15) & ~0xF;
                        addr = 0x940 - aligned;
                        {
                            Acmd* cmd = pkt++;

                            cmd->words.w0 = _SHIFTL(A_SETBUFF, 24, 8) | _SHIFTL(0, 16, 8) | _SHIFTL(addr + sampleDataStartPad, 0, 16);
                            cmd->words.w1 = _SHIFTL(0x580 + dmemDst, 16, 16) | _SHIFTL(nSamplesToDecode * 2, 0, 16);
                        }
                        {
                            Acmd* cmd = pkt++;

                            cmd->words.w0 = _SHIFTL(A_ADPCM, 24, 8) | _SHIFTL(flags | 4, 16, 8);
                            cmd->words.w1 = (u32)synthState->synthesisBuffers + 0x80000000;
                        }
                        break;
                    case CODEC_S8:
                        aligned = ((nFramesToDecode * frameSize) + 16 + 15) & ~0xF;
                        addr = 0x940 - aligned;
                        AudioSynth_SetBuffer(pkt++, 0, addr + sampleDataStartPad, 0x580 + dmemDst, nSamplesToDecode * 2);
                        AudioSynth_S8Dec(pkt++, flags, (u32)synthState->synthesisBuffers + 0x80000000);
                        break;
                }

                if (nSamplesProcessed != 0) {
                    { Acmd* cmd = pkt++; cmd->words.w0 = _SHIFTL(A_DMEMMOVE, 24, 8) | _SHIFTL(0x580 + dmemDst + nFirstFrameSamplesToIgnore * 2, 0, 24); cmd->words.w1 = _SHIFTL(0x580 + dmemOut, 16, 16) | _SHIFTL(nSamplesInThisIteration * 2, 0, 16); }
                }

                nSamplesProcessed += nSamplesInThisIteration;

                switch (flags) {
                    case 1:
                        skipBytes = 0x20;
                        dmemOut = (nSamplesToDecode + 16) * 2;
                        break;
                    case 2:
                        dmemOut = nSamplesInThisIteration * 2 + dmemOut;
                        break;
                    default:
                        if (dmemOut != 0) {
                            dmemOut = nSamplesInThisIteration * 2 + dmemOut;
                        } else {
                            dmemOut = (nFirstFrameSamplesToIgnore + nSamplesInThisIteration) * 2;
                        }
                        break;
                }

                flags = 0;

            post:
                if (noteFinished) {
                    AudioSynth_ClearBuffer(pkt++, 0x580 + dmemOut, (samplesLenAdjusted - nSamplesProcessed) * 2);
                    finished = 1;
                    note->sampleState.h.f.finished = 1;
                    AudioSynth_DisableNoteInLaterUpdates(updateIndex, noteIndex);
                    break;
                } else {
                    if (restart) {
                        synthState->restart = 1;
                        synthState->samplePosInt = loopInfo->start;
                    } else {
                        synthState->samplePosInt += nSamplesToProcess;
                    }
                }
            }

            switch (nParts) {
                case 1:
                    sampleDmemBeforeResampling = 0x580 + skipBytes;
                    break;
                case 2:
                    switch (curPart) {
                        case 0:
                            AudioSynth_InterL(pkt++, 0x580 + skipBytes, 0x3E0, (samplesLenAdjusted / 2 + 7) & ~7);
                            resampledTempLen = samplesLenAdjusted;
                            sampleDmemBeforeResampling = 0x3E0;
                            if (finished) {
                                AudioSynth_ClearBuffer(pkt++, sampleDmemBeforeResampling + resampledTempLen, samplesLenAdjusted + 16);
                            }
                            break;
                        case 1:
                            AudioSynth_InterL(pkt++, 0x580 + skipBytes, 0x3E0 + resampledTempLen, (samplesLenAdjusted / 2 + 7) & ~7);
                            break;
                    }
            }
            if (finished) {
                break;
            }
        }
    }

    flags = 0;
    if ((s32)sampleState->h.f.needsInit == 1) {
        sampleState->h.f.needsInit = 0;
        flags = 1;
    }

    pkt = AudioSynth_FinalResample(pkt, synthState, aiBufLen * 2, resamplingRateFixedPoint, sampleDmemBeforeResampling, flags);
    if (bookOffset == 3) {
        AudioSynth_UnkCmd19(pkt++, 0x3C0, 0x3C0, aiBufLen * 2, 0);
    }
    if (bookOffset == 2) {
        AudioSynth_UnkCmd3(pkt++, 0x3C0, 0x3C0, aiBufLen * 2);
    }

    gain = sampleState->h.b.gain;
    if (gain != 0) {
        if (gain < 0x10) {
            gain = 0x10;
        }
        AudioSynth_HiLoGain(pkt++, gain, 0x3C0, 0, (aiBufLen + 16) * 2);
    }

    if (sampleState->filter != NULL) {
        AudioSynth_LoadFilterSize(pkt++, aiBufLen * 2, (s32)sampleState->filter);
        AudioSynth_LoadFilterBuffer(pkt++, flags, 0x3C0, (s32)synthState->synthesisBuffers + 0x60);
    }

    if (sampleState->h.b.haasEffectLeftDelaySize != 0 || synthState->prevHaasEffectLeftDelaySize != 0) {
        haasEffectDelaySide = 1;
    } else if (sampleState->haasEffectRightDelaySize != 0 || synthState->prevHaasEffectRightDelaySize != 0) {
        haasEffectDelaySide = 2;
    } else {
        haasEffectDelaySide = 0;
    }

    pkt = AudioSynth_ProcessEnvelope(pkt, sampleState, synthState, aiBufLen, 0x3C0, haasEffectDelaySide, flags);

    if (sampleState->h.b.bitField0 & 1) {
        if (!(flags & 1)) {
            flags = 0;
        }
        pkt = AudioSynth_ApplyHaasEffect(pkt, sampleState, synthState, aiBufLen * 2, flags, haasEffectDelaySide);
    }

    return pkt;
}
// clang-format on

// original name: Nas_Synth_Resample
// Resamples `size` bytes from DMEM `inpDmem` into 0x3C0 at `pitch` (a zero pitch clears it) with the caller's
// A_INIT/A_LOOP flags.
// Load-bearing: each command on one source line, `w0` before `w1`.
// clang-format off
Acmd* AudioSynth_FinalResample(Acmd* pkt, NoteSynthesisState* synthState, s32 size, u16 pitch, u16 inpDmem,
                    s32 resampleFlags) {
    if (pitch == 0) {
        AudioSynth_ClearBuffer(pkt++, 0x3C0, size);
    } else {
        { Acmd* cmd = pkt++; cmd->words.w0 = _SHIFTL(A_SETBUFF, 24, 8) | _SHIFTL(inpDmem, 0, 16); cmd->words.w1 = _SHIFTL(0x3C0, 16, 16) | _SHIFTL(size, 0, 16); }
        { Acmd* cmd = pkt++; cmd->words.w0 = _SHIFTL(A_RESAMPLE, 24, 8) | _SHIFTL(resampleFlags, 16, 8) | _SHIFTL(pitch, 0, 16); cmd->words.w1 = (u32)synthState->synthesisBuffers + 0x80000020; }
    }
    return pkt;
}
// clang-format on

// original name: Nas_Synth_Envelope
// Ramps the note's volumes and reverb (newPan/oldPan/dPan: OoT's reverbVol/sourceReverbVol/rampReverb) into the
// envmixer command, its DMEM quad by Haas side, or emits two plain A_MIXERs when nothing ramps.
// Load-bearing: `steps = aiBufLen >> 3` in both arms; the guards as `tgtL != curL`; the head statements' order and
// tgtL's load apart from its `<<= 4`; the dead `u32 head`; `(oldPan & 0xFF) * 2`; the A_MIXER store order; one
// source line per command.
// clang-format off
Acmd* AudioSynth_ProcessEnvelope(Acmd* pkt, NoteSampleState* sampleState, NoteSynthesisState* synthState, s32 aiBufLen,
                                 u16 dmemSrc, s32 haasEffectDelaySide, UNUSED s32 flags) {
    s32 steps;
    s32 newPan;
    u32 w1;
    u16 curL;
    u16 curR;
    s16 dL;
    s16 dR;
    s16 dPan;
    s16 oldPan;
    u16 tgtL;
    u16 tgtR;
    UNUSED u32 head;

    newPan = sampleState->reverbVol;
    curL = synthState->curVolLeft;
    curR = synthState->curVolRight;
    tgtL = sampleState->targetVolLeft;
    tgtR = sampleState->targetVolRight;
    tgtL <<= 4;
    tgtR <<= 4;

    if (tgtL != curL) {
        steps = aiBufLen >> 3;
        dL = (tgtL - curL) / steps;
    } else {
        steps = aiBufLen >> 3;
        dL = 0;
    }
    if (tgtR != curR) {
        dR = (tgtR - curR) / steps;
    } else {
        dR = 0;
    }

    oldPan = synthState->reverbVol;
    if (oldPan != newPan) {
        dPan = (((newPan & 0x7F) - (oldPan & 0x7F)) << 9) / steps;
        synthState->reverbVol = newPan;
    } else {
        dPan = 0;
    }

    synthState->curVolLeft = curL + dL * steps;
    synthState->curVolRight = curR + dR * steps;

    if (sampleState->h.b.bitField0 & 1) {
        AudioSynth_ClearBuffer(pkt++, 0x5C0, 0x1A0);
        AudioSynth_EnvSetup1(pkt++, (oldPan & 0x7F) * 2, dPan, dL, dR);
        AudioSynth_EnvSetup2(pkt++, curL, curR);
        switch (haasEffectDelaySide) {
            case 1:
                w1 = sEnvMixerLeftHaasDmemDests;
                break;
            case 2:
                w1 = sEnvMixerRightHaasDmemDests;
                break;
            default:
                w1 = sEnvMixerDefaultDmemDests;
                break;
        }
    } else {
        if (dL == 0 && dR == 0 && oldPan == 0 && dPan == 0) {
            MixerCmdHalves* mix = (MixerCmdHalves*)pkt++;
            MixerCmdHalves* mix2 = (MixerCmdHalves*)pkt++;

            mix->cmdAndCount = (((aiBufLen * 2) >> 4) & 0xFFFF & 0xFFFF) | 0xC00;
            mix->dmemIn = dmemSrc;
            mix->gain = curL >> 1;
            mix->dmemOut = 0x940;
            mix2->cmdAndCount = mix->cmdAndCount;
            mix2->dmemIn = mix->dmemIn;
            mix2->gain = curR >> 1;
            mix2->dmemOut = 0xAE0;
            return pkt;
        }
        { Acmd* cmd = pkt++; cmd->words.w0 = _SHIFTL(0x12, 24, 8) | _SHIFTL((oldPan & 0xFF) * 2, 16, 8) | _SHIFTL(dPan, 0, 16); cmd->words.w1 = _SHIFTL(dL, 16, 16) | _SHIFTL(dR, 0, 16); }
        { Acmd* cmd = pkt++; cmd->words.w0 = _SHIFTL(0x16, 24, 8); cmd->words.w1 = _SHIFTL(curL, 16, 16) | _SHIFTL(curR, 0, 16); }
        w1 = sEnvMixerDefaultDmemDests;
    }

    { Acmd* cmd = pkt++; cmd->words.w0 = sEnvMixerOp | _SHIFTL(dmemSrc >> 4, 16, 8) | _SHIFTL(aiBufLen, 8, 8) | _SHIFTL((oldPan & 0x80) >> 7, 2, 1) | _SHIFTL(sampleState->h.f.stereoStrongRight, 1, 1) | _SHIFTL(sampleState->h.f.stereoStrongLeft, 0, 1); cmd->words.w1 = w1; }
    return pkt;
}
// clang-format on

// Samples per period of the synthetic wave at each harmonic index. Defined here, not up top as in OoT: .data
// places it between AudioSynth_ProcessNote's strings and AudioSynth_LoadWaveSamples'.
u8 sNumSamplesPerWavePeriod[] = { 64, 32, 16, 8 };

// Loads a synthetic-wave note's 64-sample wave into DMEM 0x580, repeated as the update needs (`ratio` rescales the
// position), or with bookOffset set the next samples of the noise at gWaveSamples[8].
// Load-bearing: `ratio` read before `rem`, `pkt++` straight into AudioSynth_LoadBuffer, each command on one line.
// clang-format off
Acmd* AudioSynth_LoadWaveSamples(Acmd* pkt, NoteSampleState* sampleState, NoteSynthesisState* synthState,
                                 s32 numSamplesToLoad) {
    s32 rem;
    s32 groups;
    s32 avail;
    s32 ratio;

    ratio = sampleState->harmonicIndexCurAndPrev;
    rem = synthState->samplePosInt;
    if (sampleState->h.f.bookOffset != 0) {
        AudioSynth_LoadBuffer(pkt++, 0x580, (numSamplesToLoad * 2 + 0xF) & ~0xF, (s32)gWaveSamples[8] + 0x80000000);
        gWaveSamples[8] += numSamplesToLoad * 2;
        return pkt;
    }

    { Acmd* cmd = pkt++; cmd->words.w0 = _SHIFTL(0x14, 24, 8) | _SHIFTL(0x08, 16, 8) | _SHIFTL(0x580, 0, 16); cmd->words.w1 = (u32)sampleState->waveSampleAddr + 0x80000000; }
    if (ratio != 0) {
        PRINTF("(D) LOD %d -> %d \n");
        rem = rem * sNumSamplesPerWavePeriod[ratio >> 2] / sNumSamplesPerWavePeriod[ratio & 3];
    }
    rem &= 0x3F;
    avail = 0x40 - rem;
    if (avail < numSamplesToLoad) {
        groups = (numSamplesToLoad - avail + 0x3F) / 64;
        if (groups != 0) {
            { Acmd* cmd = pkt++; cmd->words.w0 = _SHIFTL(0x09, 24, 8) | _SHIFTL(groups, 16, 8) | _SHIFTL(0x580, 0, 16); cmd->words.w1 = _SHIFTL(0x06, 24, 8) | _SHIFTL(0x80, 0, 16); }
        }
    }
    synthState->samplePosInt = rem;
    return pkt;
}
// clang-format on

// original name: Nas_Synth_Delay
// The Haas effect: delays one side (1 = left at 0x940, 2 = right at 0xAE0) by the sample state's delay size through
// the staging buffer at 0x5C0, resampling while the size changes; on A_INIT the delay starts from silence.
// Load-bearing: `pitch` first in its arm, `if (old != 0)`, the bare `if (cur)` tail guard and the empty
// `if (haasEffectDelaySide) {}` after it, one source line per command.
// clang-format off
Acmd* AudioSynth_ApplyHaasEffect(Acmd* pkt, NoteSampleState* sampleState, NoteSynthesisState* synthState, s32 size,
                                 s32 flags, s32 haasEffectDelaySide) {
    u16 dmem;
    u16 pitch;
    u8 old;
    u8 cur;

    switch (haasEffectDelaySide) {
        case 1:
            dmem = 0x940;
            cur = sampleState->h.b.haasEffectLeftDelaySize;
            old = synthState->prevHaasEffectLeftDelaySize;
            synthState->prevHaasEffectRightDelaySize = 0;
            synthState->prevHaasEffectLeftDelaySize = cur;
            break;
        case 2:
            dmem = 0xAE0;
            cur = sampleState->haasEffectRightDelaySize;
            old = synthState->prevHaasEffectRightDelaySize;
            synthState->prevHaasEffectRightDelaySize = cur;
            synthState->prevHaasEffectLeftDelaySize = 0;
            break;
        default:
            return pkt;
    }

    if (flags != 1) {
        if (cur != old) {
            pitch = ((size << 0xF) / 2 - 1) / ((size + cur - old - 2) / 2);
            PRINTF("S-Resample Pitch %x (old %d -> delay %d)\n");
            {
                Acmd* cmd = pkt++; cmd->words.w0 = _SHIFTL(A_SETBUFF, 24, 8) | _SHIFTL(0x5C0, 0, 16); cmd->words.w1 = _SHIFTL(0x3C0, 16, 16) | _SHIFTL(size + cur - old, 0, 16);
            }
            {
                Acmd* cmd = pkt++; cmd->words.w0 = _SHIFTL(A_SAVEBUFF, 24, 8) | _SHIFTL(pitch, 0, 16); cmd->words.w1 = 0;
            }
        } else {
            Acmd* cmd = pkt++; cmd->words.w0 = _SHIFTL(A_DMEMMOVE, 24, 8) | _SHIFTL(0x5C0, 0, 16); cmd->words.w1 = _SHIFTL(0x3C0, 16, 16) | _SHIFTL(size, 0, 16);
        }

        if (old != 0) {
            {
                Acmd* cmd = pkt++; cmd->words.w0 = _SHIFTL(0x14, 24, 8) | _SHIFTL(((old + 0xF) & 0xFFF0) >> 4, 16, 8) | _SHIFTL(0x5C0, 0, 16); cmd->words.w1 = (u32)synthState->synthesisBuffers + 0x800000A0;
            }
            {
                Acmd* cmd = pkt++; cmd->words.w0 = _SHIFTL(A_DMEMMOVE, 24, 8) | _SHIFTL(0x3C0, 0, 16); cmd->words.w1 = _SHIFTL(old + 0x5C0, 16, 16) | _SHIFTL(size + cur - old, 0, 16);
            }
        } else {
            Acmd* cmd = pkt++; cmd->words.w0 = _SHIFTL(A_DMEMMOVE, 24, 8) | _SHIFTL(0x3C0, 0, 16); cmd->words.w1 = _SHIFTL(0x5C0, 16, 16) | _SHIFTL(size + cur, 0, 16);
        }
    } else {
        {
            Acmd* cmd = pkt++; cmd->words.w0 = _SHIFTL(A_DMEMMOVE, 24, 8) | _SHIFTL(0x5C0, 0, 16); cmd->words.w1 = _SHIFTL(0x3C0, 16, 16) | _SHIFTL(size, 0, 16);
        }
        {
            Acmd* cmd = pkt++; cmd->words.w0 = _SHIFTL(A_CLEARBUFF, 24, 8) | _SHIFTL(0x5C0, 0, 16); cmd->words.w1 = cur;
        }
        {
            Acmd* cmd = pkt++; cmd->words.w0 = _SHIFTL(A_DMEMMOVE, 24, 8) | _SHIFTL(0x3C0, 0, 16); cmd->words.w1 = _SHIFTL(cur + 0x5C0, 16, 16) | _SHIFTL(size, 0, 16);
        }
    }

    if (cur) {
        Acmd* cmd = pkt++; cmd->words.w0 = _SHIFTL(0x15, 24, 8) | _SHIFTL(((cur + 0xF) & 0xFFF0) >> 4, 16, 8) | _SHIFTL(size + 0x5C0, 0, 16); cmd->words.w1 = (u32)synthState->synthesisBuffers + 0x800000A0;
    }
    if (haasEffectDelaySide) {}

    {
        Acmd* cmd = pkt++; cmd->words.w0 = _SHIFTL(A_LOADBUFF, 24, 8) | _SHIFTL(((size + 0x3F) & 0xFFC0) >> 4, 16, 8) | _SHIFTL(0x7FFF, 0, 16); cmd->words.w1 = _SHIFTL(0x5C0, 16, 16) | _SHIFTL(dmem, 0, 16);
    }
    return pkt;
}
// clang-format on

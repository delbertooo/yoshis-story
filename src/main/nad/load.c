// original filename: system.c
#include "global.h"
#include "audio.h"

// NA audio driver, OoT's load.c: the sample DMA buffers, the load tables and statuses, the sync, slow and async
// loads, font relocation and sample preloads, and the driver init (AudioLoad_Init).
// The PRINTFs are stripped debug prints whose strings stay in .data (-use_readwrite_const), in the ROM's order.

// OoT's AudioSlowLoad.state values.
typedef enum SlowLoadState {
    /* 0 */ SLOW_LOAD_STATE_WAITING,
    /* 1 */ SLOW_LOAD_STATE_START,
    /* 2 */ SLOW_LOAD_STATE_LOADING,
    /* 3 */ SLOW_LOAD_STATE_DONE
} SlowLoadState;

DmaHandler sDmaHandler = osEPiStartDma;
DmaHandler sUnusedHandler = NULL;

// Unreferenced (OoT has two words under this name); with sAwHeap, it is the file's .bss.
s32 sAudioLoadPad1[8]; // unused

// original name: Nas_WaveDmaFrameWork
// Ages every sample DMA buffer by one tick; a buffer that reaches zero is stamped with its reuse-queue position
// and appended to its list's reuse queue.
void AudioLoad_DecreaseSampleDmaTtls(void) {
    u32 i;

    for (i = 0; i < gAudioCtx.sampleDmaListSize1; i++) {
        SampleDma* dma = &gAudioCtx.sampleDmas[i];

        if (dma->ttl != 0) {
            dma->ttl--;
            if (dma->ttl == 0) {
                dma->reuseIndex = gAudioCtx.sampleDmaReuseQueue1WrPos;
                gAudioCtx.sampleDmaReuseQueue1[gAudioCtx.sampleDmaReuseQueue1WrPos] = i;
                gAudioCtx.sampleDmaReuseQueue1WrPos++;
            }
        }
    }
    for (i = gAudioCtx.sampleDmaListSize1; i < gAudioCtx.sampleDmaCount; i++) {
        SampleDma* dma = &gAudioCtx.sampleDmas[i];

        if (dma->ttl != 0) {
            dma->ttl--;
            if (dma->ttl == 0) {
                dma->reuseIndex = gAudioCtx.sampleDmaReuseQueue2WrPos;
                gAudioCtx.sampleDmaReuseQueue2[gAudioCtx.sampleDmaReuseQueue2WrPos] = i;
                gAudioCtx.sampleDmaReuseQueue2WrPos++;
            }
        }
    }
    gAudioCtx.unused1B5C = 0;
}

// original name: Nas_WaveDmaCallBack
// Returns a RAM pointer to the sample data at `devAddr`: from a DMA buffer that holds it (list 2 and/or 1, by
// `arg2` and *dmaIndexRef), else from a free buffer DMA'd off the reuse queues; 0 when none is free.
// Load-bearing: the `goto second` into the else arm, `pos` and `idx` as two counters, and the four `unused` locals.
s32 AudioLoad_DmaSampleData(s32 devAddr, u32 size, s32 arg2, u8* dmaIndexRef, s32 medium) {
    s32 pos;
    SampleDma* dma;
    UNUSED s32 unused0;
    UNUSED s32 unused1;
    UNUSED s32 unused2;
    s32 idx;
    s32 hasDma;
    s32 i;
    UNUSED s32 unused3;
    s32 dmaDevAddr;
    s32 transfer;

    hasDma = 0;
    if (arg2 != 0 || (pos = *dmaIndexRef) >= gAudioCtx.sampleDmaListSize1) {
        for (i = gAudioCtx.sampleDmaListSize1; i < gAudioCtx.sampleDmaCount; i++) {
            dma = &gAudioCtx.sampleDmas[i];
            if (devAddr - dma->devAddr >= 0 && dma->size - size >= (u32)(devAddr - dma->devAddr)) {
                if (dma->ttl == 0 && gAudioCtx.sampleDmaReuseQueue2RdPos != gAudioCtx.sampleDmaReuseQueue2WrPos) {
                    if (dma->reuseIndex != gAudioCtx.sampleDmaReuseQueue2RdPos) {
                        gAudioCtx.sampleDmaReuseQueue2[dma->reuseIndex] =
                            gAudioCtx.sampleDmaReuseQueue2[gAudioCtx.sampleDmaReuseQueue2RdPos];
                        gAudioCtx.sampleDmas[gAudioCtx.sampleDmaReuseQueue2[gAudioCtx.sampleDmaReuseQueue2RdPos]]
                            .reuseIndex = dma->reuseIndex;
                    }
                    gAudioCtx.sampleDmaReuseQueue2RdPos++;
                }
                dma->ttl = 0x20;
                *dmaIndexRef = i;
                return (s32)dma->ramAddr + devAddr - dma->devAddr;
            }
        }
        if (arg2 == 0) {
            pos = *dmaIndexRef;
            goto second;
        }
        if (gAudioCtx.sampleDmaReuseQueue2RdPos != gAudioCtx.sampleDmaReuseQueue2WrPos && arg2) {
            idx = gAudioCtx.sampleDmaReuseQueue2[gAudioCtx.sampleDmaReuseQueue2RdPos];
            gAudioCtx.sampleDmaReuseQueue2RdPos++;
            dma = &gAudioCtx.sampleDmas[idx];
            hasDma = 1;
        }
    } else {
    second:
        dma = &gAudioCtx.sampleDmas[pos];
        for (i = 0; i <= gAudioCtx.sampleDmaListSize1; i++) {
            if (devAddr - dma->devAddr >= 0 && dma->size - size >= (u32)(devAddr - dma->devAddr)) {
                if (dma->ttl == 0) {
                    if (dma->reuseIndex != gAudioCtx.sampleDmaReuseQueue1RdPos) {
                        gAudioCtx.sampleDmaReuseQueue1[dma->reuseIndex] =
                            gAudioCtx.sampleDmaReuseQueue1[gAudioCtx.sampleDmaReuseQueue1RdPos];
                        gAudioCtx.sampleDmas[gAudioCtx.sampleDmaReuseQueue1[gAudioCtx.sampleDmaReuseQueue1RdPos]]
                            .reuseIndex = dma->reuseIndex;
                    }
                    gAudioCtx.sampleDmaReuseQueue1RdPos++;
                }
                dma->ttl = 2;
                return (s32)dma->ramAddr + devAddr - dma->devAddr;
            }
            dma = &gAudioCtx.sampleDmas[i];
        }
    }

    if (hasDma == 0) {
        if (gAudioCtx.sampleDmaReuseQueue1RdPos == gAudioCtx.sampleDmaReuseQueue1WrPos) {
            PRINTF("CAUTION:WAVE CACHE FULL %d");
            return 0;
        }
        idx = gAudioCtx.sampleDmaReuseQueue1[gAudioCtx.sampleDmaReuseQueue1RdPos];
        gAudioCtx.sampleDmaReuseQueue1RdPos++;
        dma = &gAudioCtx.sampleDmas[idx];
    }

    transfer = dma->size;
    dma->ttl = 3;
    dmaDevAddr = devAddr & ~0xF;
    dma->devAddr = dmaDevAddr;
    dma->sizeUnused = transfer;
    AudioLoad_Dma(&gAudioCtx.curAudioFrameDmaIoMsgBuf[gAudioCtx.curAudioFrameDmaCount++], 0, 0, dmaDevAddr,
                  dma->ramAddr, transfer, &gAudioCtx.curAudioFrameDmaQueue, medium, "SUPERDMA");
    *dmaIndexRef = idx;
    return devAddr - dmaDevAddr + (s32)dma->ramAddr;
}

// original name: Nas_WaveDmaNew
// Carves the two sample DMA buffer lists (0x400-byte buffers in list 1, 0x500 in list 2) out of the misc or
// external pool and fills each list's reuse queue; `numNotes` is unused, as in OoT.
// Load-bearing: `4 * sizeof(SampleDma)` as two multiplies, and `dma->size` stored first in each buffer's init.
void AudioLoad_InitSampleDmaBuffers(UNUSED s32 numNotes) {
    s32 i;
    SampleDma* dma;

    gAudioCtx.sampleDmaBufSize = 0x400;
    gAudioCtx.sampleDmas = AudioHeap_Alloc(&gAudioCtx.miscPool, gAudioCtx.numNotes * 4 * sizeof(SampleDma) *
                                                                    gAudioCtx.audioBufferParameters.specUnk4);
    for (i = 0; i < gAudioCtx.numNotes * 3 * gAudioCtx.audioBufferParameters.specUnk4; i++) {
        dma = &gAudioCtx.sampleDmas[gAudioCtx.sampleDmaCount];
        dma->ramAddr = AudioHeap_AllocDmaMemory(&gAudioCtx.miscPool, gAudioCtx.sampleDmaBufSize);
        if (dma->ramAddr == NULL) {
            if (gAudioCtx.externalPool.startRamAddr == NULL) {
                break;
            }
            dma->ramAddr = AudioHeap_AllocDmaMemory(&gAudioCtx.externalPool, gAudioCtx.sampleDmaBufSize);
            if (dma->ramAddr == NULL) {
                break;
            }
        }
        dma->size = gAudioCtx.sampleDmaBufSize;
        dma->devAddr = 0;
        dma->sizeUnused = 0;
        dma->unused = 0;
        dma->ttl = 0;
        gAudioCtx.sampleDmaCount++;
    }
    for (i = 0; i < gAudioCtx.sampleDmaCount; i++) {
        gAudioCtx.sampleDmaReuseQueue1[i] = i;
        gAudioCtx.sampleDmas[i].reuseIndex = i;
    }
    for (i = gAudioCtx.sampleDmaCount; i < 0x100; i++) {
        gAudioCtx.sampleDmaReuseQueue1[i] = 0;
    }
    gAudioCtx.sampleDmaReuseQueue1RdPos = 0;
    gAudioCtx.sampleDmaReuseQueue1WrPos = gAudioCtx.sampleDmaCount;
    gAudioCtx.sampleDmaListSize1 = gAudioCtx.sampleDmaCount;
    gAudioCtx.sampleDmaBufSize = 0x500;
    for (i = 0; i < gAudioCtx.numNotes; i++) {
        dma = &gAudioCtx.sampleDmas[gAudioCtx.sampleDmaCount];
        dma->ramAddr = AudioHeap_AllocDmaMemory(&gAudioCtx.miscPool, gAudioCtx.sampleDmaBufSize);
        if (dma->ramAddr == NULL) {
            if (gAudioCtx.externalPool.startRamAddr == NULL) {
                break;
            }
            dma->ramAddr = AudioHeap_AllocDmaMemory(&gAudioCtx.externalPool, gAudioCtx.sampleDmaBufSize);
            if (dma->ramAddr == NULL) {
                break;
            }
        }
        dma->size = gAudioCtx.sampleDmaBufSize;
        dma->devAddr = 0;
        dma->sizeUnused = 0;
        dma->unused = 0;
        dma->ttl = 0;
        gAudioCtx.sampleDmaCount++;
    }
    for (i = gAudioCtx.sampleDmaListSize1; i < gAudioCtx.sampleDmaCount; i++) {
        gAudioCtx.sampleDmaReuseQueue2[i - gAudioCtx.sampleDmaListSize1] = i;
        gAudioCtx.sampleDmas[i].reuseIndex = i - gAudioCtx.sampleDmaListSize1;
    }
    for (i = gAudioCtx.sampleDmaCount; i < 0x100; i++) {
        gAudioCtx.sampleDmaReuseQueue2[i] = gAudioCtx.sampleDmaListSize1;
    }
    gAudioCtx.sampleDmaReuseQueue2RdPos = 0;
    gAudioCtx.sampleDmaReuseQueue2WrPos = gAudioCtx.sampleDmaCount - gAudioCtx.sampleDmaListSize1;
}

// original name: Nas_CheckIDbank
// The three load-complete probes (font, sequence, sample bank): a status below LOAD_STATUS_COMPLETE is still
// loading, and id 0xFF counts as complete.
s32 AudioLoad_IsFontLoadComplete(s32 fontId) {
    if (fontId == 0xFF) {
        return 1;
    }
    if (gAudioCtx.fontLoadStatus[fontId] >= LOAD_STATUS_COMPLETE) {
        return 1;
    }
    return 0;
}

// original name: Nas_CheckIDseq
s32 AudioLoad_IsSeqLoadComplete(s32 seqId) {
    if (seqId == 0xFF) {
        return 1;
    }
    if (gAudioCtx.seqLoadStatus[seqId] >= LOAD_STATUS_COMPLETE) {
        return 1;
    }
    return 0;
}

// original name: Nas_CheckIDwave
s32 AudioLoad_IsSampleLoadComplete(s32 sampleBankId) {
    if (sampleBankId == 0xFF) {
        return 1;
    }
    if (gAudioCtx.sampleFontLoadStatus[sampleBankId] >= LOAD_STATUS_COMPLETE) {
        return 1;
    }
    return 0;
}

// original name: Nas_WriteIDbank
// The three load-status setters (font, sequence, sample bank); id 0xFF and a PERMANENTLY_LOADED entry are
// refused. The sample-bank one is OoT's plain SetSampleFontLoadStatus, without the cache apply.
void AudioLoad_SetFontLoadStatus(s32 fontId, s32 loadStatus) {
    if (fontId != 0xFF && gAudioCtx.fontLoadStatus[fontId] != LOAD_STATUS_PERMANENTLY_LOADED) {
        gAudioCtx.fontLoadStatus[fontId] = loadStatus;
    }
}

// original name: Nas_WriteIDseq
void AudioLoad_SetSeqLoadStatus(s32 seqId, s32 loadStatus) {
    if (seqId != 0xFF && gAudioCtx.seqLoadStatus[seqId] != LOAD_STATUS_PERMANENTLY_LOADED) {
        gAudioCtx.seqLoadStatus[seqId] = loadStatus;
    }
}

// original name: Nas_WriteIDwaveOnly
void AudioLoad_SetSampleFontLoadStatus(s32 sampleBankId, s32 loadStatus) {
    if (sampleBankId != 0xFF && gAudioCtx.sampleFontLoadStatus[sampleBankId] != LOAD_STATUS_PERMANENTLY_LOADED) {
        gAudioCtx.sampleFontLoadStatus[sampleBankId] = loadStatus;
    }
}

// original name: Nas_BankHeaderInit
// Rebases a load table onto its ROM segment: records romAddr and unkMediumParam, and adds romAddr to every real
// entry (size != 0) on MEDIUM_CART.
void AudioLoad_InitTable(AudioTable* table, s32 romAddr, u16 unkMediumParam) {
    s32 i;

    table->header.unkMediumParam = unkMediumParam;
    table->header.romAddr = romAddr;
    for (i = 0; i < table->header.numEntries; i++) {
        if (table->entries[i].size != 0 && table->entries[i].medium == MEDIUM_CART) {
            table->entries[i].romAddr += romAddr;
        }
    }
}

// original name: Nas_PreLoadBank
// Loads every font sequence `seqId` names and reports the last through `outDefaultFontId` (0xFF if none).
// Returns `fontData`, uninitialised for an empty list, as in OoT.
// Load-bearing: `fontData` declared last.
s32 AudioLoad_SyncLoadSeqFonts(s32 seqId, s32* outDefaultFontId) {
    u32 index = ((u16*)gAudioCtx.sequenceFontTable)[AudioLoad_GetRealTableIndex(SEQUENCE_TABLE, seqId)];
    s32 numFonts = *(index + gAudioCtx.sequenceFontTable);
    s32 fontId = 0xFF;
    s32 fontData;

    PRINTF("BankCount %d\n");
    index++;
    while (numFonts > 0) {
        fontId = *(index++ + gAudioCtx.sequenceFontTable);
        fontData = AudioLoad_SyncLoadFont(fontId);
        if (fontData == 0) {
            PRINTF("BANK LOAD MISS! FOR %d\n");
        }
        numFonts--;
    }
    *outDefaultFontId = fontId;
    return fontData;
}

// original name: Nas_PreLoadSeq
// Resolves the sequence id once, then loads its fonts (flag 2) and/or the sequence itself (flag 1).
// Load-bearing: the `unused` local, and resolving into the `seqId` parameter.
void AudioLoad_SyncLoadSeqParts(s32 seqId, s32 flags) {
    UNUSED s32 unused;
    s32 defaultFontId;

    seqId = AudioLoad_GetRealTableIndex(SEQUENCE_TABLE, seqId);
    if (flags & 2) {
        AudioLoad_SyncLoadSeqFonts(seqId, &defaultFontId);
    }
    if (flags & 1) {
        AudioLoad_SyncLoadSeq(seqId);
    }
}

// original name: __Nas_LoadVoice_Inner
// Copies a relocated sample that is not yet in RAM into a sample-cache block (AudioLoad_SyncDmaUnkMedium for
// MEDIUM_UNK, else AudioLoad_SyncDma) and points the sample at it, as MEDIUM_RAM.
s32 AudioLoad_SyncLoadSample(Sample* sample, s32 fontId) {
    void* sampleAddr;

    if (sample->isRelocated == 1) {
        if (sample->medium != MEDIUM_RAM) {
            sampleAddr =
                AudioHeap_AllocPersistentSampleCache(sample->size, fontId, (s32)sample->sampleAddr, sample->medium);
            if (sampleAddr == NULL) {
                return -1;
            }
            if (sample->medium == MEDIUM_UNK) {
                AudioLoad_SyncDmaUnkMedium((s32)sample->sampleAddr, (s32)sampleAddr, sample->size,
                                           gAudioCtx.sampleBankTable->header.unkMediumParam);
            } else {
                AudioLoad_SyncDma((s32)sample->sampleAddr, (s32)sampleAddr, sample->size, sample->medium);
            }
            sample->medium = MEDIUM_RAM;
            sample->sampleAddr = sampleAddr;
        }
    }
}

// original name: Nas_LoadVoice
// Loads the samples of instrument `instId` (below 0x7F: up to three) or, for 0x7F, of drum `drumId`. The
// instrument path returns nothing (OoT's @bug); callers ignore the result.
s32 AudioLoad_SyncLoadInstrument(s32 fontId, s32 instId, s32 drumId) {
    Instrument* entry;
    Drum* drum;

    if (instId < 0x7F) {
        entry = Audio_GetInstrumentInner(fontId, instId);
        if (entry == NULL) {
            return -1;
        }
        if (entry->normalRangeLo != 0) {
            AudioLoad_SyncLoadSample(entry->lowPitchTunedSample.sample, fontId);
        }
        AudioLoad_SyncLoadSample(entry->normalPitchTunedSample.sample, fontId);
        if (entry->normalRangeHi != 0x7F) {
            AudioLoad_SyncLoadSample(entry->highPitchTunedSample.sample, fontId);
        }
    } else if (instId == 0x7F) {
        drum = Audio_GetDrum(fontId, drumId);
        if (drum == NULL) {
            return -1;
        }
        AudioLoad_SyncLoadSample(drum->tunedSample.sample, fontId);
        return 0;
    }
}

// original name: Nas_PreLoadWave_BG
// Global command 0xF4: starts an async load of sample bank `sampleBankId`, or posts the completion message at
// once if it cannot be queued (OoT's AudioLoad_AsyncLoad inlined; YS posts NULL where OoT posts -1).
void AudioLoad_AsyncLoadSampleBank(s32 sampleBankId, s32 nChunks, s32 retData, OSMesgQueue* retQueue) {
    if (AudioLoad_AsyncLoadInner(SAMPLE_TABLE, AudioLoad_GetRealTableIndex(SAMPLE_TABLE, sampleBankId), nChunks,
                                 retData, retQueue) == 0) {
        osSendMesg(retQueue, NULL, OS_MESG_NOBLOCK);
    }
}

// original name: Nas_PreLoadBank_BG
// Global command 0xF5: starts an async load of every font sequence `seqId` names, the background twin of
// AudioLoad_SyncLoadSeqFonts (OoT's 0xF5, AsyncLoadFont, takes one font id).
void AudioLoad_AsyncLoadSeqFonts(s32 seqId, s32 nChunks, s32 retData, OSMesgQueue* retQueue) {
    u32 index = ((u16*)gAudioCtx.sequenceFontTable)[AudioLoad_GetRealTableIndex(SEQUENCE_TABLE, seqId)];
    s32 numFonts = *(index + gAudioCtx.sequenceFontTable);

    PRINTF("BankCount %d\n");
    index++;
    while (numFonts > 0) {
        AudioLoad_AsyncLoadInner(FONT_TABLE,
                                 AudioLoad_GetRealTableIndex(FONT_TABLE, *(index++ + gAudioCtx.sequenceFontTable)),
                                 nChunks, retData, retQueue);
        numFonts--;
    }
}

// original name: Nas_SeqToBank
// Returns sequence `seqId`'s font list in sequenceFontTable (u16 offsets, then per list a count byte and the
// font ids) with its count in `*outNumFonts`; NULL for an empty list.
// Load-bearing: the count tested by reading `*outNumFonts` back, not through a local.
u8* AudioLoad_GetFontsForSequence(s32 seqId, s32* outNumFonts) {
    u32 index = ((u16*)gAudioCtx.sequenceFontTable)[AudioLoad_GetRealTableIndex(SEQUENCE_TABLE, seqId)];

    *outNumFonts = *(index + gAudioCtx.sequenceFontTable);
    index++;
    if (*outNumFonts == 0) {
        return NULL;
    }
    return &gAudioCtx.sequenceFontTable[index];
}

// original name: Nas_FlushBank
// Global command 0xF6: discards every font sequence `seqId` names unless the permanent cache holds it, and
// marks it not loaded.
void AudioLoad_DiscardSeqFonts(s32 seqId) {
    u32 index = ((u16*)gAudioCtx.sequenceFontTable)[AudioLoad_GetRealTableIndex(SEQUENCE_TABLE, seqId)];
    s32 numFonts = *(index + gAudioCtx.sequenceFontTable);
    s32 fontId;

    PRINTF("Flush Start\n");
    index++;
    while (numFonts > 0) {
        numFonts--;
        fontId = AudioLoad_GetRealTableIndex(FONT_TABLE, *(index++ + gAudioCtx.sequenceFontTable));
        PRINTF("%d ->%d\n");
        if (AudioHeap_SearchPermanentCache(FONT_TABLE, fontId) == 0) {
            AudioLoad_DiscardFont(fontId);
            AudioLoad_SetFontLoadStatus(fontId, LOAD_STATUS_NOT_LOADED);
        } else {
            PRINTF("useflag %d\n");
        }
    }
}

// original name: __Kill_Bank
// Forgets font `fontId` in the font cache, the two temporary entries and then every persistent one, and
// discards it at heap level (AudioHeap_DiscardFont).
// Load-bearing: the first read through the alias gAudioCtxFontCacheTemporaryEntry0Id, `mark` for the two
// guarded stores only, and the dead-bodied pointer-identity `if` between the two seed assignments.
void AudioLoad_DiscardFont(s32 fontId) {
    u32 i;
    AudioPersistentCache* st;
    AudioPersistentCache* cur;
    s16 mark = -1;

    if (fontId == gAudioCtxFontCacheTemporaryEntry0Id) {
        gAudioCtx.fontCache.temporary.entries[0].id = mark;
    } else if (fontId == gAudioCtx.fontCache.temporary.entries[1].id) {
        gAudioCtx.fontCache.temporary.entries[1].id = mark;
    }
    st = &gAudioCtx.fontCache.persistent;
    if (st == &gAudioCtx.fontCache.persistent) {}
    cur = st;
    for (i = 0; i < cur->numEntries; i++) {
        if (st->entries[i].id == fontId) {
            st->entries[i].id = -1;
        }
    }
    AudioHeap_DiscardFont(fontId);
}

// original name: Nas_StartMySeq
// The two seq-player starters: refuse while resetTimer is set, clear or set skipTicks, and hand over to
// AudioLoad_SyncInitSeqPlayerInternal. Neither returns a value, as in OoT.
s32 AudioLoad_SyncInitSeqPlayer(s32 playerIdx, s32 seqId, s32 arg2) {
    if (gAudioCtx.resetTimer != 0) {
        return 0;
    }
    gAudioCtx.seqPlayers[playerIdx].skipTicks = 0;
    return AudioLoad_SyncInitSeqPlayerInternal(playerIdx, seqId, arg2);
}

// original name: Nas_StartSeq_Skip
s32 AudioLoad_SyncInitSeqPlayerSkipTicks(s32 playerIdx, s32 seqId, s32 skipTicks) {
    if (gAudioCtx.resetTimer != 0) {
        return 0;
    }
    gAudioCtx.seqPlayers[playerIdx].skipTicks = skipTicks;
    return AudioLoad_SyncInitSeqPlayerInternal(playerIdx, seqId, 0);
}

// original name: __Nas_StartSeq
// Starts player `playerIdx` on sequence `seqId`: disables it, loads the sequence's fonts and the sequence,
// resets the player, makes the last font its defaultFont and enables it.
// Load-bearing: the trailing 16-trip loop around a stripped print.
s32 AudioLoad_SyncInitSeqPlayerInternal(s32 playerIdx, s32 seqId, UNUSED s32 arg2) {
    SequencePlayer* seqPlayer;
    u32 index;
    s32 numFonts;
    s32 fontId;
    s32 seqData;
    s32 i;

    seqId = AudioLoad_GetRealTableIndex(SEQUENCE_TABLE, seqId);
    seqPlayer = &gAudioCtx.seqPlayers[playerIdx];
    AudioSeq_SequencePlayerDisable(seqPlayer);
    index = ((u16*)gAudioCtx.sequenceFontTable)[seqId];
    numFonts = *(index + gAudioCtx.sequenceFontTable);
    PRINTF("BankCount %d\n");
    index++;
    fontId = 0xFF;
    while (numFonts > 0) {
        fontId = *(index++ + gAudioCtx.sequenceFontTable);
        AudioLoad_SyncLoadFont(fontId);
        numFonts--;
    }
    seqData = AudioLoad_SyncLoadSeq(seqId);
    AudioSeq_ResetSequencePlayer(seqPlayer);
    seqPlayer->enabled = 1;
    seqPlayer->seqId = seqId;
    seqPlayer->defaultFont = fontId;
    seqPlayer->seqData = seqData;
    seqPlayer->scriptState.pc = (u8*)seqData;
    seqPlayer->scriptState.depth = 0;
    seqPlayer->delay = 0;
    seqPlayer->finished = 0;
    AudioSeq_SkipForwardSequence(seqPlayer);
    for (i = 0; i < 16; i++) {
        PRINTF("%2x ");
    }
}

// original name: __Load_Seq
// Loads sequence `seqId` (resolved through its alias) and discards the result, as AudioLoad_SyncLoadSimple.
// Load-bearing: the `unused` local, and resolving into the `seqId` parameter.
s32 AudioLoad_SyncLoadSeq(s32 seqId) {
    UNUSED s32 unused;
    s32 didAllocate;

    seqId = AudioLoad_GetRealTableIndex(SEQUENCE_TABLE, seqId);
    AudioLoad_SyncLoad(SEQUENCE_TABLE, seqId, &didAllocate);
}

// original name: __Load_Wave
// Returns sample bank `sampleBankId`'s RAM address, loading it if needed, with its medium in `*outMedium`; a
// CACHE_LOAD_EITHER_NOSYNC bank is never loaded, and its ROM address comes back instead.
// Load-bearing: the declaration order, and resolving into the `sampleBankId` parameter.
s32 AudioLoad_TrySyncLoadSampleBank(s32 sampleBankId, s32* outMedium) {
    s32 ramAddr;
    AudioTable* sampleBankTable = AudioLoad_GetLoadTable(SAMPLE_TABLE);
    s32 cachePolicy;
    s32 didAllocate;

    sampleBankId = AudioLoad_GetRealTableIndex(SAMPLE_TABLE, sampleBankId);
    ramAddr = AudioLoad_SearchCaches(SAMPLE_TABLE, sampleBankId);
    if (ramAddr != 0) {
        AudioLoad_SetSampleFontLoadStatus(sampleBankId, LOAD_STATUS_COMPLETE);
        *outMedium = MEDIUM_RAM;
        return ramAddr;
    }
    cachePolicy = sampleBankTable->entries[sampleBankId].cachePolicy;
    if (cachePolicy == CACHE_LOAD_EITHER_NOSYNC) {
        *outMedium = sampleBankTable->entries[sampleBankId].medium;
        return sampleBankTable->entries[sampleBankId].romAddr;
    }
    ramAddr = AudioLoad_SyncLoad(SAMPLE_TABLE, sampleBankId, &didAllocate);
    if (ramAddr != 0) {
        *outMedium = MEDIUM_RAM;
        return ramAddr;
    }
    *outMedium = sampleBankTable->entries[sampleBankId].medium;
    return sampleBankTable->entries[sampleBankId].romAddr;
}

// original name: __Load_Ctrl
// Loads a font: first the (up to) two sample banks it names (0xFF = none), then the font itself, relocated
// through AudioLoad_RelocateFontAndPreloadSamples when this call loaded it.
// Load-bearing: the declaration order and the two scalar id copies.
s32 AudioLoad_SyncLoadFont(s32 fontId) {
    s32 fontData;
    s32 sampleBankId1;
    s32 sampleBankId2;
    s32 didAllocate;
    SampleBankRelocInfo sampleBankReloc;

    fontId = AudioLoad_GetRealTableIndex(FONT_TABLE, fontId);
    sampleBankId1 = gAudioCtx.soundFontList[fontId].sampleBankId1;
    sampleBankId2 = gAudioCtx.soundFontList[fontId].sampleBankId2;
    sampleBankReloc.sampleBankId1 = sampleBankId1;
    sampleBankReloc.sampleBankId2 = sampleBankId2;
    if (sampleBankId1 != 0xFF) {
        sampleBankReloc.baseAddr1 = AudioLoad_TrySyncLoadSampleBank(sampleBankId1, &sampleBankReloc.medium1);
    } else {
        sampleBankReloc.baseAddr1 = 0;
    }
    if (sampleBankId2 != 0xFF) {
        sampleBankReloc.baseAddr2 = AudioLoad_TrySyncLoadSampleBank(sampleBankId2, &sampleBankReloc.medium2);
    } else {
        sampleBankReloc.baseAddr2 = 0;
    }
    fontData = AudioLoad_SyncLoad(FONT_TABLE, fontId, &didAllocate);
    PRINTF("LoadCtrl, Ptr %x and Media is %d\n");
    if (fontData == 0) {
        return 0;
    }
    if (didAllocate == 1) {
        AudioLoad_RelocateFontAndPreloadSamples(fontId, (SoundFontData*)fontData, &sampleBankReloc, 0);
    }
    return fontData;
}

// original name: __Load_Bank
// Makes entry `id` of table `tableType` resident (from a cache, else allocated by its cachePolicy and DMA'd by
// its medium) and reports through `didAllocate` whether this call loaded it.
s32 AudioLoad_SyncLoad(s32 tableType, s32 id, s32* didAllocate) {
    s32 size;
    AudioTable* table;
    s32 loadStatus;
    s32 medium;
    s32 ramAddr;
    s32 romAddr;
    AudioTableEntry* rec;
    s32 cachePolicy;

    PRINTF("Load Bank, Type %d , ID %d\n");
    ramAddr = AudioLoad_SearchCaches(tableType, id);
    if (ramAddr != 0) {
        *didAllocate = 0;
        loadStatus = LOAD_STATUS_COMPLETE;
    } else {
        table = AudioLoad_GetLoadTable(tableType);
        rec = &table->entries[id];
        size = rec->size;
        size = (size + 0xF) & ~0xF;
        medium = rec->medium;
        cachePolicy = rec->cachePolicy;
        romAddr = rec->romAddr;
        switch (cachePolicy) {
            case CACHE_LOAD_PERMANENT:
                ramAddr = (s32)AudioHeap_AllocPermanent(tableType, id, size);
                if (ramAddr == 0) {
                    return ramAddr;
                }
                break;
            case CACHE_LOAD_PERSISTENT:
                ramAddr = AudioHeap_AllocCached(tableType, size, CACHE_PERSISTENT, id);
                if (ramAddr == 0) {
                    return ramAddr;
                }
                break;
            case CACHE_LOAD_TEMPORARY:
                ramAddr = AudioHeap_AllocCached(tableType, size, CACHE_TEMPORARY, id);
                if (ramAddr == 0) {
                    return ramAddr;
                }
                PRINTF("get auto\n");
                break;
            case CACHE_LOAD_EITHER:
            case CACHE_LOAD_EITHER_NOSYNC:
                ramAddr = AudioHeap_AllocCached(tableType, size, CACHE_EITHER, id);
                if (ramAddr == 0) {
                    return ramAddr;
                }
                PRINTF("get s-auto %x\n");
                break;
        }
        *didAllocate = 1;
        if (medium == MEDIUM_UNK) {
            AudioLoad_SyncDmaUnkMedium(romAddr, ramAddr, size, table->header.unkMediumParam);
        } else {
            AudioLoad_SyncDma(romAddr, ramAddr, size, medium);
        }
        if (cachePolicy == CACHE_LOAD_PERMANENT) {
            loadStatus = LOAD_STATUS_PERMANENTLY_LOADED;
        } else {
            loadStatus = LOAD_STATUS_COMPLETE;
        }
    }
    switch (tableType) {
        case SEQUENCE_TABLE:
            AudioLoad_SetSeqLoadStatus(id, loadStatus);
            PRINTF("Seq %d Write ID OK %d!\n");
            break;
        case FONT_TABLE:
            AudioLoad_SetFontLoadStatus(id, loadStatus);
            break;
        case SAMPLE_TABLE:
            AudioLoad_SetSampleFontLoadStatus(id, loadStatus);
            break;
    }
    return ramAddr;
}

// original name: __Link_BankNum
// Resolves an alias entry: an entry of table `tableType` whose size is zero stands for the
// entry whose index is parked in its romAddr. One hop only -- the result is not re-resolved.
s32 AudioLoad_GetRealTableIndex(s32 tableType, s32 id) {
    AudioTable* table = AudioLoad_GetLoadTable(tableType);

    if (table->entries[id].size == 0) {
        id = table->entries[id].romAddr;
    }
    return id;
}

// original name: __Check_Cache
// Returns the RAM address of a resident table entry, searching the permanent cache and then CACHE_EITHER, or 0.
s32 AudioLoad_SearchCaches(s32 tableType, s32 id) {
    s32 found = AudioHeap_SearchPermanentCache(tableType, id);
    s32 started;

    if (found != 0) {
        return found;
    }
    started = AudioHeap_SearchCaches(tableType, CACHE_EITHER, id);
    if (started != 0) {
        return started;
    }
    return 0;
}

// Returns the sequence, font or sample-bank load table for `tableType`, NULL for any other type.
AudioTable* AudioLoad_GetLoadTable(s32 tableType) {
    AudioTable* table;

    switch (tableType) {
        case SEQUENCE_TABLE:
            table = gAudioCtx.sequenceTable;
            break;
        case FONT_TABLE:
            table = gAudioCtx.soundFontTable;
            break;
        case SAMPLE_TABLE:
            table = gAudioCtx.sampleBankTable;
            break;
        default:
            table = NULL;
            break;
    }
    return table;
}

// original name: Nas_BankOfsToAddr_Inner
// Biases a freshly loaded font's offsets by its load address: the drum table and each drum, each instrument's
// samples (AudioLoad_RelocateSample), then its soundFontList record; a set mark byte skips a second pass.
// Load-bearing: the one-based `tbl` walk, `bank` as a named copy of the parameter, and OoT's `drum` local.
void AudioLoad_RelocateFont(s32 fontId, SoundFontData* fontDataStartAddr, SampleBankRelocInfo* sampleBankReloc) {
    SoundFontData* bank;
    void* p;
    Instrument* e;
    Drum* drum;
    s32 nData;
    s32 i;
    Instrument** tbl;
    s32 nProg;

    PRINTF("Banknumber %d\n");
    bank = fontDataStartAddr;
    nData = gAudioCtx.soundFontList[fontId].numDrums;
    nProg = gAudioCtx.soundFontList[fontId].numInstruments;
    PRINTF("Bank Offset %x %d %d\n");
    if (bank->drums != NULL) {
        if (nData != 0) {
            bank->drums = (Drum**)((u8*)bank->drums + (s32)bank);
            for (i = 0; i < nData; i++) {
                p = bank->drums[i];
                if (p != NULL) {
                    p = (u8*)p + (s32)bank;
                    bank->drums[i] = (Drum*)p;
                    drum = (Drum*)p;
                    if (drum->isRelocated == 0) {
                        PRINTF("PEP Touch %x \n");
                        AudioLoad_RelocateSample(&drum->tunedSample, (s32)bank, sampleBankReloc);
                        p = drum->envelope;
                        drum->isRelocated = 1;
                        drum->envelope = (EnvelopePoint*)((u8*)p + (s32)bank);
                    }
                }
            }
        }
    }
    if (nProg >= 0x80) {
        PRINTF("Warning: INSTCOUNT OVER (%d)\n");
        nProg = 0x7F;
    }
    tbl = bank->instruments - 1;
    for (i = 1; i <= nProg; i++) {
        if (tbl[i] != NULL) {
            tbl[i] = (Instrument*)((u8*)tbl[i] + (s32)bank);
            e = tbl[i];
            if (e->isRelocated == 0) {
                if (e->normalRangeLo != 0) {
                    AudioLoad_RelocateSample(&e->lowPitchTunedSample, (s32)bank, sampleBankReloc);
                }
                AudioLoad_RelocateSample(&e->normalPitchTunedSample, (s32)bank, sampleBankReloc);
                if (e->normalRangeHi != 0x7F) {
                    AudioLoad_RelocateSample(&e->highPitchTunedSample, (s32)bank, sampleBankReloc);
                }
                p = e->envelope;
                e->isRelocated = 1;
                e->envelope = (EnvelopePoint*)((u8*)p + (s32)bank);
            }
        }
    }
    gAudioCtx.soundFontList[fontId].drums = (void**)bank->drums;
    gAudioCtx.soundFontList[fontId].instruments = (void**)bank->instruments;
}

// original name: Nas_FastCopy
// Copies `size` bytes (rounded up to 16) from a device address into RAM in blocking chunks of at most 0x400,
// one AudioLoad_Dma tagged "FastCopy" at a time.
// Load-bearing: the `for (;;)` with its leading `break`.
void AudioLoad_SyncDma(s32 devAddr, s32 ramAddr, u32 size, s32 medium) {
    size = (size + 0xF) & ~0xF;
    Audio_InvalDCache((void*)ramAddr, size);
    for (;;) {
        if (size < 0x400) {
            break;
        }
        AudioLoad_Dma(&gAudioCtx.syncDmaIoMesg, 1, 0, devAddr, (void*)ramAddr, 0x400, &gAudioCtx.syncDmaQueue, medium,
                      "FastCopy");
        osRecvMesg(&gAudioCtx.syncDmaQueue, NULL, OS_MESG_BLOCK);
        size -= 0x400;
        devAddr += 0x400;
        ramAddr += 0x400;
    }
    if (size != 0) {
        AudioLoad_Dma(&gAudioCtx.syncDmaIoMesg, 1, 0, devAddr, (void*)ramAddr, size, &gAudioCtx.syncDmaQueue, medium,
                      "FastCopy");
        osRecvMesg(&gAudioCtx.syncDmaQueue, NULL, OS_MESG_BLOCK);
    }
}

// original name: Nas_FastDiskCopy
// The copy for MEDIUM_UNK, which this build lacks: one stripped "Not Supported" print (OoT's is empty).
void AudioLoad_SyncDmaUnkMedium(UNUSED s32 devAddr, UNUSED s32 addr, UNUSED s32 size, UNUSED s32 unkMediumParam) {
    PRINTF("Not Supported\n");
}

// original name: Nas_StartDma
// Every DMA of the driver: fills the caller's OSIoMesg and passes it with the cart or disk-drive PI handle to
// the handler AudioLoad_SetDmaHandler installed; -1 once resetTimer passes 0x10. `dmaFuncType` is a debug tag.
s32 AudioLoad_Dma(OSIoMesg* mesg, s32 priority, s32 direction, u32 devAddr, void* ramAddr, u32 size,
                  OSMesgQueue* reqQueue, s32 medium, UNUSED const char* dmaFuncType) {
    OSPiHandle* handle;

    if (gAudioCtx.resetTimer > 0x10) {
        return -1;
    }
    switch (medium) {
        case MEDIUM_CART:
            handle = gAudioCtx.cartHandle;
            break;
        case MEDIUM_DISK_DRIVE:
            handle = gAudioCtx.driveHandle;
            break;
        default:
            PRINTF("Error: DMA Prohibited Media [%d]\n");
            return 0;
    }
    if ((size & 0xF) != 0) {
        PRINTF("Warning: size not align 16 %x  (%s)\n");
        size = (size + 0xF) & ~0xF;
    }
    mesg->hdr.pri = priority;
    mesg->hdr.retQueue = reqQueue;
    mesg->dramAddr = ramAddr;
    mesg->devAddr = devAddr;
    mesg->size = size;
    handle->transferInfo.cmdType = 2;
    sDmaHandler(handle, mesg, direction);
    return 0;
}

// Two empty functions splat merged into AudioLoad_Dma's symbol, in the slot of OoT's one AudioLoad_Unused1
// (original __OfsToLbaOfs); which of the two it is, is not known.
void AudioLoad_Unused1A(void) {
}

void AudioLoad_Unused1B(void) {
}

// original name: EmemLoad
// Fire-and-forget wrapper over AudioLoad_SyncLoad: loads entry `id` of table `tableType` and throws
// away both the address and didAllocate. No callers.
void AudioLoad_SyncLoadSimple(s32 tableType, s32 id) {
    s32 didAllocate;

    AudioLoad_SyncLoad(tableType, id, &didAllocate);
}

// original name: __Load_Bank_BG
// The async twin of AudioLoad_SyncLoad: refuses an entry still loading, posts the completion message at once
// for a cached one, else hands the copy to an async-load slot whose retMsg packs retData, tableType, id, status.
s32 AudioLoad_AsyncLoadInner(s32 tableType, s32 id, s32 nChunks, s32 retData, OSMesgQueue* retQueue) {
    s32 size;
    AudioTable* table;
    s32 ramAddr;
    s32 medium;
    s32 cachePolicy;
    s32 romAddr;
    s32 loadStatus;
    AudioTableEntry* rec;

    PRINTF("Load Bank BG, Type %d , ID %d\n");
    switch (tableType) {
        case SEQUENCE_TABLE:
            if (gAudioCtx.seqLoadStatus[id] == LOAD_STATUS_IN_PROGRESS) {
                return 0;
            }
            break;
        case FONT_TABLE:
            if (gAudioCtx.fontLoadStatus[id] == LOAD_STATUS_IN_PROGRESS) {
                return 0;
            }
            break;
        case SAMPLE_TABLE:
            if (gAudioCtx.sampleFontLoadStatus[id] == LOAD_STATUS_IN_PROGRESS) {
                return 0;
            }
            break;
    }
    ramAddr = AudioLoad_SearchCaches(tableType, id);
    if (ramAddr != 0) {
        loadStatus = LOAD_STATUS_COMPLETE;
        osSendMesg(retQueue, (OSMesg)(retData << 24), OS_MESG_NOBLOCK);
    } else {
        table = AudioLoad_GetLoadTable(tableType);
        rec = &table->entries[id];
        size = rec->size;
        size = (size + 0xF) & ~0xF;
        medium = rec->medium;
        cachePolicy = rec->cachePolicy;
        romAddr = rec->romAddr;
        loadStatus = LOAD_STATUS_COMPLETE;
        switch (cachePolicy) {
            case CACHE_LOAD_PERMANENT:
                ramAddr = (s32)AudioHeap_AllocPermanent(tableType, id, size);
                if (ramAddr == 0) {
                    return ramAddr;
                }
                loadStatus = LOAD_STATUS_PERMANENTLY_LOADED;
                break;
            case CACHE_LOAD_PERSISTENT:
                ramAddr = AudioHeap_AllocCached(tableType, size, CACHE_PERSISTENT, id);
                if (ramAddr == 0) {
                    return ramAddr;
                }
                break;
            case CACHE_LOAD_TEMPORARY:
                ramAddr = AudioHeap_AllocCached(tableType, size, CACHE_TEMPORARY, id);
                if (ramAddr == 0) {
                    return ramAddr;
                }
                PRINTF("get auto\n");
                break;
            case CACHE_LOAD_EITHER:
            case CACHE_LOAD_EITHER_NOSYNC:
                ramAddr = AudioHeap_AllocCached(tableType, size, CACHE_EITHER, id);
                if (ramAddr == 0) {
                    return ramAddr;
                }
                PRINTF("get s-auto %x\n");
                break;
        }
        AudioLoad_StartAsyncLoad(romAddr, ramAddr, size, medium, nChunks, (s32)retQueue,
                                 (retData << 24) | (tableType << 16) | (id << 8) | loadStatus);
        loadStatus = LOAD_STATUS_IN_PROGRESS;
    }
    switch (tableType) {
        case SEQUENCE_TABLE:
            AudioLoad_SetSeqLoadStatus(id, loadStatus);
            break;
        case FONT_TABLE:
            AudioLoad_SetFontLoadStatus(id, loadStatus);
            break;
        case SAMPLE_TABLE:
            AudioLoad_SetSampleFontLoadStatus(id, loadStatus);
            break;
    }
    return ramAddr;
}

// original name: Nas_BgDmaFrameWork
// Per-frame load service: the slow loads, the sample preloads and the async loads, in that order.
void AudioLoad_ProcessLoads(s32 resetStatus) {
    AudioLoad_ProcessSlowLoads(resetStatus);
    AudioLoad_ProcessSamplePreloads(resetStatus);
    AudioLoad_ProcessAsyncLoads(resetStatus);
}

// original name: Nas_SetRomHandler
// Installs the callback AudioLoad_Dma hands each filled-in request to (osEPiStartDma-shaped).
void AudioLoad_SetDmaHandler(DmaHandler callback) {
    sDmaHandler = callback;
}

// Stores a second callback nothing calls. OoT gives it AudioLoad_SetDmaHandler's original name
// (Nas_SetRomHandler), which fits only one of the two.
void AudioLoad_SetUnusedHandler(DmaHandler callback) {
    sUnusedHandler = callback;
}

// original name: Nas_InitAudio
// The driver's init, called from na/na_main.c: clears the audio heap and gAudioCtx, then sets up the PI handles,
// message queues, AI buffers, load tables, font list (OoT's AudioLoad_InitSoundFont inlined) and pools.
// Load-bearing: both clears un-rotated with a tail `break`; the first one's exit test on its store's line and
// its dead `i++` (with `s32 i = 0`); the second a guarded down-counter, `ptr` declared first, over a run-time
// distance; `pad[12]` and its `if (0)`; the four redundant `& 0xFF`; `*ptr++ = 0`; the statement order.
void AudioLoad_Init(void) {
    s32 i = 0;
    s32 j;
    UNUSED s32 pad[12];

    gAudioCustomUpdateFunction = NULL;
    gAudioCtx.resetTimer = 0;

    {
        s32 limit = (s32)gAudioHeapInitSizes.heapSize / 8;
        if (limit > 0) {
            u64* ptr = (u64*)gAudioHeap;
            u64* end = ptr + limit;
            for (;;) {
                // clang-format off
                *ptr++ = 0; i++; if (ptr >= end) { break; }
                // clang-format on
            }
        }
    }

    {
        u64* ptr = (u64*)&gAudioCtx;
        s32 count = ((u32)((u8*)&gAudioCtx + sizeof(gAudioCtx) - 1) - (u32)&gAudioCtx) / 8;
        for (; count >= 0; count--) {
            *ptr++ = 0;
        }
    }

    gAudioCtx.maxTempoTvTypeFactors = 16.71299934f;
    gAudioCtx.refreshRate = 60;
    AudioThread_InitMesgQueues();
    PRINTF("Clear Workarea %x -%x size %x \n");
    PRINTF("AudioHeap is %x\n");

    for (i = 0; i < 3; i++) {
        gAudioCtx.aiBufLengths[i] = 160;
    }

    gAudioCtx.totalTaskCount = 0;
    gAudioCtx.rspTaskIndex = 0;
    gAudioCtx.curAiBufIndex = 0;
    gAudioCtx.soundOutputMode = SOUND_OUTPUT_STEREO;
    gAudioCtx.curTask = NULL;
    gAudioCtx.rspTask[0].task.t.data_size = 0;
    gAudioCtx.rspTask[1].task.t.data_size = 0;

    osCreateMesgQueue(&gAudioCtx.syncDmaQueue, &gAudioCtx.syncDmaMesg, 1);

    osCreateMesgQueue(&gAudioCtx.curAudioFrameDmaQueue, gAudioCtx.curAudioFrameDmaMsgBuf, 64);
    osCreateMesgQueue(&gAudioCtx.externalLoadQueue, gAudioCtx.externalLoadMsgBuf, 8);
    osCreateMesgQueue(&gAudioCtx.preloadSampleQueue, gAudioCtx.preloadSampleMsgBuf, 8);

    gAudioCtx.curAudioFrameDmaCount = 0;
    gAudioCtx.sampleDmaCount = 0;
    gAudioCtx.cartHandle = osCartRomInit();
    gAudioCtx.driveHandle = osDriveRomInit();

    AudioHeap_InitMainPools(gAudioHeapInitSizes.initPoolSize);

    for (i = 0; i < 3; i++) {
        s16* ptr = AudioHeap_Alloc(&gAudioCtx.initPool, 0xB00);
        gAudioCtx.aiBuffers[i] = ptr;
        for (j = 0; j < 1408; j++) {
            *ptr++ = 0;
        }
    }

    gAudioCtx.specId = 0;
    gAudioCtx.resetStatus = 1;
    AudioHeap_ResetStep();
    PRINTF("Heap reset.Synth Change %x \n");
    PRINTF("DAC %x (POINTER %x)\n");
    PRINTF("HP POINT %x\n");
    PRINTF("%d: WRT POINT %x  HP %x (in %x)\n");
    PRINTF("Heap %x %x %x\n");
    PRINTF("Main Heap Initialize.\n");

    gAudioCtx.sequenceTable = (AudioTable*)gSequenceTable;
    gAudioCtx.soundFontTable = (AudioTable*)gSoundFontTable;
    gAudioCtx.sampleBankTable = (struct AudioTable*)gSampleBankTable;
    gAudioCtx.sequenceFontTable = gSequenceFontTable;

    gAudioCtx.numSequences = gAudioCtx.sequenceTable->header.numEntries;
    AudioLoad_InitTable(gAudioCtx.sequenceTable, (s32)D_4F3930, 0);

    AudioLoad_InitTable(gAudioCtx.soundFontTable, (s32)D_B58B0, 0);

    AudioLoad_InitTable(gAudioCtx.sampleBankTable, (s32)D_E1AD0, 0);

    {
        s32 numBanks = gAudioCtx.soundFontTable->header.numEntries;
        gAudioCtx.soundFontList = AudioHeap_Alloc(&gAudioCtx.initPool, numBanks * sizeof(SoundFont));
        for (i = 0; i < numBanks; i++) {
            gAudioCtx.soundFontList[i].sampleBankId1 = (gAudioCtx.soundFontTable->entries[i].shortData1 >> 8) & 0xFF;
            gAudioCtx.soundFontList[i].sampleBankId2 = gAudioCtx.soundFontTable->entries[i].shortData1 & 0xFF;
            gAudioCtx.soundFontList[i].numInstruments = (gAudioCtx.soundFontTable->entries[i].shortData2 >> 8) & 0xFF;
            gAudioCtx.soundFontList[i].numDrums = gAudioCtx.soundFontTable->entries[i].shortData2 & 0xFF;
            PRINTF("%d :WaveA %d WaveB %d Inst %d,Perc %d\n");
        }
    }

    {
        void* ptr = AudioHeap_Alloc(&gAudioCtx.initPool, gAudioHeapInitSizes.permanentPoolSize);
        if (ptr == NULL) {
            // cast away const from gAudioHeapInitSizes
            *((u32*)&gAudioHeapInitSizes.permanentPoolSize) = 0;
        }
        AudioHeap_InitPool(&gAudioCtx.permanentPool, ptr, gAudioHeapInitSizes.permanentPoolSize);
    }

    PRINTF("---------- Init Completed. ------------\n");
    PRINTF(" Syndrv    :[%6d]\n");
    PRINTF(" Seqdrv    :[%6d]\n");
    PRINTF(" audiodata :[%6d]\n");
    PRINTF("---------------------------------------\n");
    PRINTF("Que %x \n");
    osSendMesg(gAudioCtx.taskStartQueueP, (OSMesg)gAudioCtx.totalTaskCount, OS_MESG_NOBLOCK);

    if (0) {
        pad[0] = 0;
    }
}

// original name: LpsInit
// Marks both slow-load slots (gAudioCtx.slowLoads, a double buffer) idle.
void AudioLoad_InitSlowLoads(void) {
    gAudioCtx.slowLoads[0].state = SLOW_LOAD_STATE_WAITING;
    gAudioCtx.slowLoads[1].state = SLOW_LOAD_STATE_WAITING;
}

void* AudioLoad_GetFontSample(s32 fontId, s32 instId);

// original name: VoiceLoad
// Starts a slow load of an instrument's or drum's sample into the free slow-load slot, the sample twin of
// AudioLoad_SlowLoadSeq. `*status` reads 0 for no such sample, 2 for already in RAM, 3 for no cache room.
s32 AudioLoad_SlowLoadSample(s32 fontId, u8 instId, s8* status) {
    Sample* sample;
    AudioSlowLoad* slowLoad;

    PRINTF("Entry--- %d %d\n");
    sample = AudioLoad_GetFontSample(fontId, instId);
    if (sample == NULL) {
        *status = 0;
        return -1;
    }
    if (sample->medium == MEDIUM_RAM) {
        *status = 2;
        return 0;
    }
    slowLoad = &gAudioCtx.slowLoads[gAudioCtx.slowLoadPos];
    if (slowLoad->state == SLOW_LOAD_STATE_DONE) {
        slowLoad->state = SLOW_LOAD_STATE_WAITING;
    }
    slowLoad->sample = *sample;
    slowLoad->status = status;
    slowLoad->curRamAddr =
        AudioHeap_AllocTemporarySampleCache(sample->size, fontId, (s32)sample->sampleAddr, sample->medium);
    if (slowLoad->curRamAddr == NULL) {
        if (sample->medium == MEDIUM_UNK || sample->codec == CODEC_S16_INMEMORY) {
            *status = 0;
        } else {
            *status = 3;
        }
        return -1;
    }
    slowLoad->state = SLOW_LOAD_STATE_START;
    slowLoad->bytesRemaining = (sample->size + 0xF) & ~0xF;
    slowLoad->ramAddr = slowLoad->curRamAddr;
    slowLoad->curDevAddr = (u32)sample->sampleAddr;
    slowLoad->medium = sample->medium;
    slowLoad->seqOrFontId = fontId;
    slowLoad->instId = instId;
    gAudioCtx.slowLoadPos ^= 1;
    return 0;
}

// original name: __GetWaveTable
// Sample lookup: ids below 0x80 are instruments and give their normal-pitch sample (+0x10),
// everything above is drum `instId - 0x80` and gives its sample (+0x04).
void* AudioLoad_GetFontSample(s32 fontId, s32 instId) {
    void* sample;
    Instrument* entry;
    Drum* drum;

    if (instId < 0x80) {
        entry = Audio_GetInstrumentInner(fontId, instId);
        if (entry == NULL) {
            return NULL;
        }
        sample = entry->normalPitchTunedSample.sample;
    } else {
        drum = Audio_GetDrum(fontId, instId - 0x80);
        if (drum == NULL) {
            return NULL;
        }
        sample = drum->tunedSample.sample;
    }
    return sample;
}

// An empty function splat merged into AudioLoad_GetFontSample's symbol (nothing references it), in OoT's
// AudioLoad_Unused2 slot.
void AudioLoad_Unused2(void) {
}

// original name: __SwapLoadLps
// Completes a sample slow load: saves the sample its font/instrument pair names into the slot (four words),
// then points it at the loaded copy, as MEDIUM_RAM.
void AudioLoad_FinishSlowLoad(AudioSlowLoad* slowLoad) {
    Sample* sample;

    if (slowLoad->sample.sampleAddr == NULL) {
        return;
    }
    sample = AudioLoad_GetFontSample(slowLoad->seqOrFontId, slowLoad->instId);
    if (sample != NULL) {
        slowLoad->sample = *sample;
        sample->sampleAddr = slowLoad->ramAddr;
        sample->medium = MEDIUM_RAM;
    }
}

// original name: LpsDma
// Services both slow-load slots: collects a chunk in flight (on reset only that, parking the slot), then moves
// up to 0x400 bytes or, with nothing left, finishes the load and sets the caller's status byte to 1.
// Load-bearing: the guard's `gAudioCtx.slowLoads[i].state` read into `state`, `slowLoad += 0`, and the empty
// `if (!remaining) {}` after the short copy.
void AudioLoad_ProcessSlowLoads(s32 resetStatus) {
    AudioTable* sampleBankTable = gAudioCtx.sampleBankTable;
    AudioSlowLoad* slowLoad;
    s32 state;
    s32 remaining;
    s32 i;

    for (i = 0; i < 2; i++) {
        state = gAudioCtx.slowLoads[i].state;
        if (state != SLOW_LOAD_STATE_START) {
            if (state != SLOW_LOAD_STATE_LOADING) {
                continue;
            }
            slowLoad = &gAudioCtx.slowLoads[i];
            slowLoad += 0;
            PRINTF("---Block LPS here\n");
            osRecvMesg(&slowLoad->msgQueue, NULL, OS_MESG_BLOCK);
            PRINTF("===Block LPS end\n");
            if (resetStatus != 0) {
                slowLoad->state = SLOW_LOAD_STATE_DONE;
                continue;
            }
        }
        slowLoad = &gAudioCtx.slowLoads[i];
        slowLoad += 0;
        remaining = slowLoad->bytesRemaining;
        slowLoad->state = SLOW_LOAD_STATE_LOADING;
        if (remaining == 0) {
            AudioLoad_FinishSlowLoad(slowLoad);
            slowLoad->state = SLOW_LOAD_STATE_DONE;
            *slowLoad->status = 1;
            continue;
        }
        if (remaining < 0x400) {
            if (slowLoad->medium == MEDIUM_UNK) {
                AudioLoad_DmaSlowCopyUnkMedium(slowLoad->curDevAddr, slowLoad->curRamAddr, remaining,
                                               sampleBankTable->header.unkMediumParam);
            } else {
                AudioLoad_DmaSlowCopy(slowLoad, remaining);
                if (!remaining) {}
            }
            slowLoad->bytesRemaining = 0;
        } else {
            if (slowLoad->medium == MEDIUM_UNK) {
                AudioLoad_DmaSlowCopyUnkMedium(slowLoad->curDevAddr, slowLoad->curRamAddr, 0x400,
                                               sampleBankTable->header.unkMediumParam);
            } else {
                AudioLoad_DmaSlowCopy(slowLoad, 0x400);
            }
            slowLoad->bytesRemaining -= 0x400;
            slowLoad->curRamAddr += 0x400;
            slowLoad->curDevAddr += 0x400;
        }
    }
}

// original name: __Nas_SlowCopy
// Starts one chunk of a slow load: invalidate the destination, build the slot's own reply queue,
// then hand AudioLoad_Dma the request, tagged "SLOWCOPY".
void AudioLoad_DmaSlowCopy(AudioSlowLoad* slowLoad, s32 size) {
    Audio_InvalDCache(slowLoad->curRamAddr, size);
    osCreateMesgQueue(&slowLoad->msgQueue, &slowLoad->msg, 1);
    AudioLoad_Dma(&slowLoad->ioMesg, 0, 0, slowLoad->curDevAddr, slowLoad->curRamAddr, size, &slowLoad->msgQueue,
                  slowLoad->medium, "SLOWCOPY");
}

// original name: __Nas_SlowDiskCopy
// The same "Not Supported" print as AudioLoad_SyncDmaUnkMedium, and nothing else.
// AudioLoad_ProcessSlowLoads calls it with a slot's source and destination pointers.
void AudioLoad_DmaSlowCopyUnkMedium(UNUSED s32 devAddr, UNUSED u8* ramAddr, UNUSED s32 size, UNUSED s32 arg3) {
    PRINTF("Not Supported\n");
}

// original name: SeqLoad
// Starts a slow load of sequence `seqId` into `ramAddr` in the free slow-load slot and flips slowLoadPos to the
// other one. Always returns 0.
// Load-bearing: `aligned` rounded in place in a second statement, every entry read as
// `seqTable->entries[seqId].x`, and the tail stores' order.
s32 AudioLoad_SlowLoadSeq(s32 seqId, u8* ramAddr, s8* status) {
    AudioTable* seqTable;
    AudioSlowLoad* slowLoad;
    s32 aligned;

    PRINTF("Seq Load Entry--- %d %x\n");
    seqId = AudioLoad_GetRealTableIndex(SEQUENCE_TABLE, seqId);
    seqTable = AudioLoad_GetLoadTable(SEQUENCE_TABLE);
    slowLoad = &gAudioCtx.slowLoads[gAudioCtx.slowLoadPos];
    if (slowLoad->state == SLOW_LOAD_STATE_DONE) {
        slowLoad->state = SLOW_LOAD_STATE_WAITING;
    }
    slowLoad->sample.sampleAddr = 0;
    slowLoad->status = status;
    aligned = seqTable->entries[seqId].size;
    aligned = (aligned + 0xF) & ~0xF;
    slowLoad->bytesRemaining = aligned;
    slowLoad->state = SLOW_LOAD_STATE_START;
    slowLoad->curRamAddr = ramAddr;
    slowLoad->ramAddr = ramAddr;
    slowLoad->curDevAddr = seqTable->entries[seqId].romAddr;
    slowLoad->medium = seqTable->entries[seqId].medium;
    slowLoad->seqOrFontId = seqId;
    gAudioCtx.slowLoadPos ^= 1;
    return 0;
}

// original name: Nas_BgCopyReq
// Fills the first free async-load slot, or returns NULL: chunks of size / nChunks rounded up to 0x100 (at
// least 0x100; 0x1000 for `nChunks` 0), and a start delay of 3 frames.
AudioAsyncLoad* AudioLoad_StartAsyncLoad(s32 devAddr, s32 ramAddr, s32 size, s32 medium, s32 nChunks, s32 retQueue,
                                         s32 retMsg) {
    AudioAsyncLoad* asyncLoad;
    s32 i;
    u32 chunk;

    PRINTF("Req: Src %x Dest %x Len %x,media %d,retcode %d\n");
    for (i = 0; i < 16; i++) {
        if (gAudioCtx.asyncLoads[i].status == 0) {
            asyncLoad = &gAudioCtx.asyncLoads[i];
            break;
        }
    }
    if (i == 16) {
        return NULL;
    }
    asyncLoad->status = 1;
    asyncLoad->curDevAddr = devAddr;
    asyncLoad->ramAddr = (u8*)ramAddr;
    asyncLoad->curRamAddr = (u8*)ramAddr;
    asyncLoad->bytesRemaining = size;
    if (nChunks == 0) {
        asyncLoad->chunkSize = 0x1000;
    } else {
        chunk = size / nChunks;
        chunk = (chunk + 0xFF) & ~0xFF;
        asyncLoad->chunkSize = chunk;
        if (chunk < 0x100) {
            asyncLoad->chunkSize = 0x100;
        }
    }
    asyncLoad->retQueue = (OSMesgQueue*)retQueue;
    asyncLoad->delay = 3;
    asyncLoad->medium = medium;
    asyncLoad->retMsg = (OSMesg)retMsg;
    osCreateMesgQueue(&asyncLoad->msgQueue, &asyncLoad->msg, 1);
    return asyncLoad;
}

// original name: Nas_BgCopyMain
// Services every active async-load slot, except while resetTimer is 1. YS has no unk-medium load queue here.
// Load-bearing: resetTimer `volatile` (audio.h).
void AudioLoad_ProcessAsyncLoads(s32 resetStatus) {
    s32 i;

    if (gAudioCtx.resetTimer == 1) {
        return;
    }
    for (i = 0; i < 16; i++) {
        if (gAudioCtx.asyncLoads[i].status == 1) {
            AudioLoad_ProcessAsyncLoad(&gAudioCtx.asyncLoads[i], resetStatus);
        }
    }
}

// original name: __BgCopySub
// Runs one async-load slot per frame: ticks its delay, waits on its queue (drained on reset), moves one chunk,
// or finishes it (OoT's AudioLoad_FinishAsyncLoad inlined: load status, a font's banks and relocation, retMsg).
// Load-bearing: AudioLoad_AsyncDma declared without a prototype (audio.h), `retMsg` typed OSMesg, `chunk`
// re-bound from the field after the call, `u32 word`, the s32 switch selector and its cases in order 0/2/1,
// `id0`/`id1`, and the seven scalar declarations in order.
void AudioLoad_ProcessAsyncLoad(AudioAsyncLoad* asyncLoad, s32 resetStatus) {
    AudioTable* sampleBankTable = gAudioCtx.sampleBankTable;
    u32 word;
    s32 kind;
    s32 idx;
    u32 chunk;
    s32 id0;
    s32 id1;
    SampleBankRelocInfo sampleBankReloc;

    PRINTF("Remain Size %d\n");
    if (asyncLoad->delay >= 2) {
        asyncLoad->delay--;
        return;
    }
    if (asyncLoad->delay == 1) {
        asyncLoad->delay = 0;
    } else {
        if (resetStatus != 0) {
            PRINTF("---Block BG here\n");
            osRecvMesg(&asyncLoad->msgQueue, NULL, OS_MESG_BLOCK);
            PRINTF("===Block BG end\n");
            asyncLoad->status = 0;
            return;
        }
        if (osRecvMesg(&asyncLoad->msgQueue, NULL, OS_MESG_NOBLOCK) == -1) {
            return;
        }
    }
    if (asyncLoad->bytesRemaining == 0) {
        word = (u32)asyncLoad->retMsg;
        PRINTF("Retcode %x\n");
        kind = (word >> 16) & 0xFF;
        switch (kind) {
            case SEQUENCE_TABLE:
                AudioLoad_SetSeqLoadStatus((word >> 8) & 0xFF, word & 0xFF);
                break;
            case SAMPLE_TABLE:
                AudioLoad_SetSampleFontLoadStatus((word >> 8) & 0xFF, word & 0xFF);
                break;
            case FONT_TABLE:
                idx = (word >> 8) & 0xFF;
                AudioLoad_SetFontLoadStatus(idx, word & 0xFF);
                id0 = gAudioCtx.soundFontList[idx].sampleBankId1;
                id1 = gAudioCtx.soundFontList[idx].sampleBankId2;
                sampleBankReloc.sampleBankId1 = id0;
                sampleBankReloc.sampleBankId2 = id1;
                if (id0 != 0xFF) {
                    sampleBankReloc.baseAddr1 = AudioLoad_TrySyncLoadSampleBank(id0, &sampleBankReloc.medium1);
                } else {
                    sampleBankReloc.baseAddr1 = 0;
                }
                if (id1 != 0xFF) {
                    sampleBankReloc.baseAddr2 = AudioLoad_TrySyncLoadSampleBank(id1, &sampleBankReloc.medium2);
                } else {
                    sampleBankReloc.baseAddr2 = 0;
                }
                AudioLoad_RelocateFontAndPreloadSamples(idx, (SoundFontData*)asyncLoad->ramAddr, &sampleBankReloc, 1);
                break;
            default:
                PRINTF("Other Type: Not Write ID.\n");
                break;
        }
        asyncLoad->status = 0;
        osSendMesg(asyncLoad->retQueue, asyncLoad->retMsg, OS_MESG_NOBLOCK);
        return;
    }
    chunk = asyncLoad->chunkSize;
    if (chunk == 0) {
        PRINTF("BGLOAD:Error: dma length 0\n");
    }
    if (asyncLoad->bytesRemaining < chunk) {
        if (asyncLoad->medium == MEDIUM_UNK) {
            AudioLoad_AsyncDmaUnkMedium(asyncLoad->curDevAddr, asyncLoad->curRamAddr, asyncLoad->bytesRemaining,
                                        sampleBankTable->header.unkMediumParam);
        } else {
            AudioLoad_AsyncDma(asyncLoad, asyncLoad->bytesRemaining);
        }
        asyncLoad->bytesRemaining = 0;
        return;
    }
    if (asyncLoad->medium == MEDIUM_UNK) {
        AudioLoad_AsyncDmaUnkMedium(asyncLoad->curDevAddr, asyncLoad->curRamAddr, chunk,
                                    sampleBankTable->header.unkMediumParam);
    } else {
        AudioLoad_AsyncDma(asyncLoad, chunk);
    }
    chunk = asyncLoad->chunkSize;
    asyncLoad->bytesRemaining -= chunk;
    asyncLoad->curDevAddr += chunk;
    asyncLoad->curRamAddr += chunk;
}

// original name: __Nas_BgCopy
// Starts one chunk of an async load: rounds the size up to 16, invalidates the destination, builds the slot's
// reply queue and hands AudioLoad_Dma the request, tagged "BGCOPY".
// Load-bearing: the rounding as two compound assignments to the `size` parameter.
void AudioLoad_AsyncDma(AudioAsyncLoad* asyncLoad, s32 size) {
    size += 0xF;
    size &= ~0xF;

    Audio_InvalDCache(asyncLoad->curRamAddr, size);
    osCreateMesgQueue(&asyncLoad->msgQueue, &asyncLoad->msg, 1);
    AudioLoad_Dma(&asyncLoad->ioMesg, 0, 0, asyncLoad->curDevAddr, asyncLoad->curRamAddr, size, &asyncLoad->msgQueue,
                  asyncLoad->medium, "BGCOPY");
}

// original name: __Nas_BgDiskCopy
// The same "Not Supported" print as AudioLoad_SyncDmaUnkMedium, and nothing else. Called only from
// AudioLoad_ProcessAsyncLoad.
void AudioLoad_AsyncDmaUnkMedium(UNUSED s32 devAddr, UNUSED void* ramAddr, UNUSED s32 size, UNUSED s32 arg3) {
    PRINTF("Not Supported\n");
}

// original name: __WaveTouch
// Relocates a tuned sample's sample pointer, and on first visit the sample's own pointers,
// sample-bank base and medium.
// Load-bearing: the `p`/`d` scratch pointers; the error checks first (the strings' order).
void AudioLoad_RelocateSample(TunedSample* tunedSample, s32 fontData, SampleBankRelocInfo* sampleBankReloc) {
    u8* p;
    Sample* d;

    PRINTF("InstTop is %x\n");
    if ((u32)tunedSample->sample > 0x80000000) {
        PRINTF("Error: Already wavetable is touched %x.\n");
    } else {
        p = (u8*)tunedSample->sample + fontData;
        d = (Sample*)p;
        tunedSample->sample = d;
        if (d->size == 0) {
            PRINTF("Touch Warning: Length zero %x\n");
        } else if (d->isRelocated != 1) {
            p = (u8*)d->loop + fontData;
            d->loop = (AdpcmLoop*)p;
            p = (u8*)d->book + fontData;
            d->book = (AdpcmBook*)p;
            PRINTF("Media Convert %d ->");
            switch (d->medium) {
                case 0:
                    p = (u8*)d->sampleAddr + sampleBankReloc->baseAddr1;
                    d->sampleAddr = p;
                    d->medium = sampleBankReloc->medium1;
                    break;
                case 1:
                    p = (u8*)d->sampleAddr + sampleBankReloc->baseAddr2;
                    d->sampleAddr = p;
                    d->medium = sampleBankReloc->medium2;
                    break;
                case 2:
                    break;
                case 3:
                    break;
            }
            PRINTF("%d\n");
            d->isRelocated = 1;
            if (d->unk_bit26 != 0 && d->medium != MEDIUM_RAM) {
                gAudioCtx.usedSamples[gAudioCtx.numUsedSamples] = d;
                gAudioCtx.numUsedSamples++;
            }
        }
    }
}

// original name: Nas_BankOfsToAddr
// Relocates a freshly loaded font, then preloads the samples it flagged: `async` 0 copies them into the
// persistent sample cache at once, 1 stacks preload records in the temporary one and starts the first.
// Load-bearing: the tail's `kind` local, `total` reused as the tail's block address, the empty `if (rate) {}`,
// and the eight declarations in order.
void AudioLoad_RelocateFontAndPreloadSamples(s32 fontId, SoundFontData* fontData, SampleBankRelocInfo* sampleBankReloc,
                                             s32 async) {
    s32 kind;
    Sample* job;
    void* block;
    s32 i;
    u32 size;
    u32 rate;
    s32 flag;
    s32 total;
    // original name: awheap (AC's Nas_BankOfsToAddr). Only its base is ever written; OoT: D_8016B780, an s32.
    UNUSED static ALHeap sAwHeap;

    flag = 0;
    if (gAudioCtx.preloadSampleStackTop != 0) {
        flag = 1;
        PRINTF("It's busy now!!!!! %d\n");
    } else {
        sAwHeap.base = NULL;
    }
    gAudioCtx.numUsedSamples = 0;
    AudioLoad_RelocateFont(fontId, fontData, sampleBankReloc);

    total = 0;
    for (i = 0; i < gAudioCtx.numUsedSamples; i++) {
        total += (gAudioCtx.usedSamples[i]->size + 0xF) & ~0xF;
    }
    for (i = 0; i < gAudioCtx.numUsedSamples; i++) {
        if (gAudioCtx.preloadSampleStackTop == 0x78) {
            PRINTF("BG LOAD BUFFER is OVER.\n");
            break;
        }
        job = gAudioCtx.usedSamples[i];
        switch (async) {
            case 0:
                if (job->medium == sampleBankReloc->medium1) {
                    PRINTF("Warning: Length zero %x\n");
                    block = AudioHeap_AllocPersistentSampleCache(job->size, sampleBankReloc->sampleBankId1,
                                                                 (s32)job->sampleAddr, job->medium);
                } else if (job->medium == sampleBankReloc->medium2) {
                    block = AudioHeap_AllocPersistentSampleCache(job->size, sampleBankReloc->sampleBankId2,
                                                                 (s32)job->sampleAddr, job->medium);
                }
                break;
            case 1:
                if (job->medium == sampleBankReloc->medium1) {
                    block = AudioHeap_AllocTemporarySampleCache(job->size, sampleBankReloc->sampleBankId1,
                                                                (s32)job->sampleAddr, job->medium);
                } else if (job->medium == sampleBankReloc->medium2) {
                    block = AudioHeap_AllocTemporarySampleCache(job->size, sampleBankReloc->sampleBankId2,
                                                                (s32)job->sampleAddr, job->medium);
                }
                break;
        }
        if (block == NULL) {
            continue;
        }
        PRINTF("Wave Load %d \n");
        {
            switch (async) {
                case 0:
                    if (job->medium == MEDIUM_UNK) {
                        AudioLoad_SyncDmaUnkMedium((s32)job->sampleAddr, (s32)block, job->size,
                                                   gAudioCtx.sampleBankTable->header.unkMediumParam);
                        job->sampleAddr = block;
                        job->medium = MEDIUM_RAM;
                    } else {
                        AudioLoad_SyncDma((s32)job->sampleAddr, (s32)block, job->size, job->medium);
                        job->sampleAddr = block;
                        job->medium = MEDIUM_RAM;
                    }
                    break;
                case 1:
                    gAudioCtx.preloadSampleStack[gAudioCtx.preloadSampleStackTop].sample = job;
                    gAudioCtx.preloadSampleStack[gAudioCtx.preloadSampleStackTop].ramAddr = block;
                    gAudioCtx.preloadSampleStack[gAudioCtx.preloadSampleStackTop].encodedInfo =
                        (gAudioCtx.preloadSampleStackTop << 24) | 0xFFFFFF;
                    gAudioCtx.preloadSampleStack[gAudioCtx.preloadSampleStackTop].isFree = 0;
                    gAudioCtx.preloadSampleStack[gAudioCtx.preloadSampleStackTop].endAndMediumKey =
                        (void*)((u8*)job->sampleAddr + job->size + job->medium);
                    gAudioCtx.preloadSampleStackTop++;
                    break;
            }
        }
    }
    PRINTF("Total Bg Wave Load %d \n");
    gAudioCtx.numUsedSamples = 0;
    if (gAudioCtx.preloadSampleStackTop != 0) {
        if (flag == 0) {
            job = gAudioCtx.preloadSampleStack[gAudioCtx.preloadSampleStackTop - 1].sample;
            total = (s32)gAudioCtx.preloadSampleStack[gAudioCtx.preloadSampleStackTop - 1].ramAddr;
            size = job->size;
            kind = job->medium;
            rate = (size >> 12) + 1;
            if (rate) {}
            AudioLoad_StartAsyncLoad((s32)job->sampleAddr, total, size, kind, rate, (s32)&gAudioCtx.preloadSampleQueue,
                                     gAudioCtx.preloadSampleStack[gAudioCtx.preloadSampleStackTop - 1].encodedInfo);
        }
    }
}

// original name: Nas_CheckBgWave
// Services the sample-preload stack: a completed record re-points its sample if it still matches, then the
// stack pops finished records and starts the next one. Returns 0 on reset or with no message, else 1.
// Load-bearing: `mesg` declared second and the unread `unused`, `end` for the first sum, the pointer store
// before the medium clear, the empty `if`s on the record's address (a typed null) and on `rate` above its
// assignment, `rate` computed above `end =`, and `rate | 0` at the call.
s32 AudioLoad_ProcessSamplePreloads(s32 resetStatus) {
    Sample* entry;
    OSMesg mesg;
    void* base;
    void* end;
    u32 size;
    u32 rate;
    UNUSED u32 unused;

    if (gAudioCtx.preloadSampleStackTop > 0) {
        if (resetStatus != 0) {
            osRecvMesg(&gAudioCtx.preloadSampleQueue, &mesg, OS_MESG_NOBLOCK);
            gAudioCtx.preloadSampleStackTop = 0;
            return 0;
        }
        if (osRecvMesg(&gAudioCtx.preloadSampleQueue, &mesg, OS_MESG_NOBLOCK) == -1) {
            return 0;
        }
        mesg = (OSMesg)((u32)mesg >> 24);
        PRINTF("Receive %d\n");
        if (gAudioCtx.preloadSampleStack[(s32)mesg].isFree == 0) {
            entry = gAudioCtx.preloadSampleStack[(s32)mesg].sample;
            end = (void*)((u8*)entry->sampleAddr + entry->size + entry->medium);
            if (end == gAudioCtx.preloadSampleStack[(s32)mesg].endAndMediumKey) {
                entry->sampleAddr = gAudioCtx.preloadSampleStack[(s32)mesg].ramAddr;
                entry->medium = MEDIUM_RAM;
            } else {
                PRINTF("============Error: Magic is Broken after loading.\n");
            }
            gAudioCtx.preloadSampleStack[(s32)mesg].isFree = 1;
        }
        while (1) {
            if (gAudioCtx.preloadSampleStackTop <= 0) {
                break;
            }
            PRINTF("Remain DMA: %d\n");
            if (gAudioCtx.preloadSampleStack[gAudioCtx.preloadSampleStackTop - 1].isFree == 1) {
                gAudioCtx.preloadSampleStackTop--;
                continue;
            }
            PRINTF("N start %d\n");
            if (&gAudioCtx.preloadSampleStack[gAudioCtx.preloadSampleStackTop - 1] != (AudioPreloadReq*)NULL) {}
            entry = gAudioCtx.preloadSampleStack[gAudioCtx.preloadSampleStackTop - 1].sample;
            base = entry->sampleAddr;
            if (rate) {}
            size = entry->size;
            rate = (size >> 12) + 1;
            end = (void*)((u8*)base + size + entry->medium);
            if (end != gAudioCtx.preloadSampleStack[gAudioCtx.preloadSampleStackTop - 1].endAndMediumKey) {
                PRINTF("============Error: Magic is Broken: %x\n");
                gAudioCtx.preloadSampleStack[gAudioCtx.preloadSampleStackTop - 1].isFree = 1;
                gAudioCtx.preloadSampleStackTop--;
                continue;
            }
            if (AudioLoad_StartAsyncLoad(
                    (s32)base, (s32)gAudioCtx.preloadSampleStack[gAudioCtx.preloadSampleStackTop - 1].ramAddr, size,
                    entry->medium, rate | 0, (s32)&gAudioCtx.preloadSampleQueue,
                    gAudioCtx.preloadSampleStack[gAudioCtx.preloadSampleStackTop - 1].encodedInfo) == NULL) {
                PRINTF("Error: No Handle.\n");
            } else {
                PRINTF("Success: %x\n");
            }
            break;
        }
    }
    return 1;
}

// original name: __AddList
// Appends `sample` to a `numSamples`-entry set unless a sample with the same address (sampleAddr) is
// already in it. Returns the new count.
s32 AudioLoad_AddToSampleSet(Sample* sample, s32 numSamples, Sample** sampleSet) {
    s32 i;

    for (i = 0; i < numSamples; i++) {
        if (sample->sampleAddr == sampleSet[i]->sampleAddr) {
            break;
        }
    }
    if (i == numSamples) {
        sampleSet[numSamples] = sample;
        numSamples++;
    }
    return numSamples;
}

// original name: MakeWaveList
// Collects every distinct sample font `fontId` uses into `sampleSet` (one per drum, up to three per
// instrument) and returns the count. No callers.
// Load-bearing: each loop's own pointer (`drum`, `prog`), the empty `if (0)` after the first loop's call, and
// `numSamples = 0` between the two count reads.
s32 AudioLoad_GetSamplesForFont(s32 fontId, Sample** sampleSet) {
    Drum* drum;
    s32 numSamples;
    s32 i;
    s32 numData;
    Instrument* prog;
    s32 numProg;

    numData = gAudioCtx.soundFontList[fontId].numDrums;
    numSamples = 0;
    numProg = gAudioCtx.soundFontList[fontId].numInstruments;
    for (i = 0; i < numData; i++) {
        drum = Audio_GetDrum(fontId, i);
        if (0) {}
        if (drum != NULL) {
            numSamples = AudioLoad_AddToSampleSet(drum->tunedSample.sample, numSamples, sampleSet);
        }
    }
    for (i = 0; i < numProg; i++) {
        prog = Audio_GetInstrumentInner(fontId, i);
        if (prog != NULL) {
            if (prog->normalRangeLo != 0) {
                numSamples = AudioLoad_AddToSampleSet(prog->lowPitchTunedSample.sample, numSamples, sampleSet);
            }
            if (prog->normalRangeHi != 0x7F) {
                numSamples = AudioLoad_AddToSampleSet(prog->highPitchTunedSample.sample, numSamples, sampleSet);
            }
            numSamples = AudioLoad_AddToSampleSet(prog->normalPitchTunedSample.sample, numSamples, sampleSet);
        }
    }
    return numSamples;
}

// original name: __Reload
// Adds `tunedSample`'s sample to the preload list at usedSamples, but only if it is flagged for
// preloading, non-empty and not in RAM (three bitfield tests on its head word).
void AudioLoad_AddUsedSample(TunedSample* tunedSample) {
    Sample* sample = tunedSample->sample;

    if (sample->size != 0 && sample->unk_bit26 != 0 && sample->medium != MEDIUM_RAM) {
        gAudioCtx.usedSamples[gAudioCtx.numUsedSamples] = sample;
        gAudioCtx.numUsedSamples++;
    }
}

// original name: WaveReload
// Preloads the samples font `fontId` flags (AudioLoad_AddUsedSample per instrument and drum sample), then
// continues as AudioLoad_RelocateFontAndPreloadSamples' tail; an empty list returns early.
// Load-bearing: the empty `if (0)` after the first loop's call, both counts read as full
// `gAudioCtx.soundFontList[fontId].numX` expressions, `drum` declared fourth, and the tail's `kind` local with
// the empty `if (rate) {}`.
void AudioLoad_PreloadSamplesForFont(s32 fontId, s32 async, SampleBankRelocInfo* sampleBankReloc) {
    Instrument* entry;
    s32 n0;
    s32 n1;
    Drum* drum;
    s32 kind;
    Sample* job;
    void* block;
    s32 i;
    u32 size;
    u32 rate;
    s32 flag;
    s32 total;

    flag = 0;
    if (gAudioCtx.preloadSampleStackTop != 0) {
        flag = 1;
        PRINTF("It's busy now!!!!! %d\n");
    }
    gAudioCtx.numUsedSamples = 0;
    n0 = gAudioCtx.soundFontList[fontId].numInstruments;
    n1 = gAudioCtx.soundFontList[fontId].numDrums;

    for (i = 0; i < n0; i++) {
        entry = Audio_GetInstrumentInner(fontId, i);
        if (0) {}
        if (entry != NULL) {
            if (entry->normalRangeLo != 0) {
                AudioLoad_AddUsedSample(&entry->lowPitchTunedSample);
            }
            if (entry->normalRangeHi != 0x7F) {
                AudioLoad_AddUsedSample(&entry->highPitchTunedSample);
            }
            AudioLoad_AddUsedSample(&entry->normalPitchTunedSample);
        }
    }
    for (i = 0; i < n1; i++) {
        drum = Audio_GetDrum(fontId, i);
        if (drum != NULL) {
            AudioLoad_AddUsedSample(&drum->tunedSample);
        }
    }

    if (gAudioCtx.numUsedSamples == 0) {
        return;
    }
    total = 0;
    for (i = 0; i < gAudioCtx.numUsedSamples; i++) {
        total += (gAudioCtx.usedSamples[i]->size + 0xF) & ~0xF;
    }
    for (i = 0; i < gAudioCtx.numUsedSamples; i++) {
        if (gAudioCtx.preloadSampleStackTop == 0x78) {
            PRINTF("BG LOAD BUFFER is OVER.\n");
            break;
        }
        job = gAudioCtx.usedSamples[i];
        PRINTF("Base %x,Len %x Media %d\n");
        switch (async) {
            case 0:
                if (job->medium == sampleBankReloc->medium1) {
                    block = AudioHeap_AllocPersistentSampleCache(job->size, sampleBankReloc->sampleBankId1,
                                                                 (s32)job->sampleAddr, job->medium);
                } else if (job->medium == sampleBankReloc->medium2) {
                    block = AudioHeap_AllocPersistentSampleCache(job->size, sampleBankReloc->sampleBankId2,
                                                                 (s32)job->sampleAddr, job->medium);
                }
                break;
            case 1:
                if (job->medium == sampleBankReloc->medium1) {
                    block = AudioHeap_AllocTemporarySampleCache(job->size, sampleBankReloc->sampleBankId1,
                                                                (s32)job->sampleAddr, job->medium);
                } else if (job->medium == sampleBankReloc->medium2) {
                    block = AudioHeap_AllocTemporarySampleCache(job->size, sampleBankReloc->sampleBankId2,
                                                                (s32)job->sampleAddr, job->medium);
                }
                break;
        }
        if (block == NULL) {
            continue;
        }
        PRINTF("Wave Load %d \n");
        {
            switch (async) {
                case 0:
                    if (job->medium == MEDIUM_UNK) {
                        AudioLoad_SyncDmaUnkMedium((s32)job->sampleAddr, (s32)block, job->size,
                                                   gAudioCtx.sampleBankTable->header.unkMediumParam);
                        job->sampleAddr = block;
                        job->medium = MEDIUM_RAM;
                    } else {
                        AudioLoad_SyncDma((s32)job->sampleAddr, (s32)block, job->size, job->medium);
                        job->sampleAddr = block;
                        job->medium = MEDIUM_RAM;
                    }
                    break;
                case 1:
                    gAudioCtx.preloadSampleStack[gAudioCtx.preloadSampleStackTop].sample = job;
                    gAudioCtx.preloadSampleStack[gAudioCtx.preloadSampleStackTop].ramAddr = block;
                    gAudioCtx.preloadSampleStack[gAudioCtx.preloadSampleStackTop].encodedInfo =
                        (gAudioCtx.preloadSampleStackTop << 24) | 0xFFFFFF;
                    gAudioCtx.preloadSampleStack[gAudioCtx.preloadSampleStackTop].isFree = 0;
                    gAudioCtx.preloadSampleStack[gAudioCtx.preloadSampleStackTop].endAndMediumKey =
                        (void*)((u8*)job->sampleAddr + job->size + job->medium);
                    gAudioCtx.preloadSampleStackTop++;
                    break;
            }
        }
    }
    PRINTF("Total Bg Wave Load %d \n");
    gAudioCtx.numUsedSamples = 0;
    if (gAudioCtx.preloadSampleStackTop != 0) {
        if (flag == 0) {
            job = gAudioCtx.preloadSampleStack[gAudioCtx.preloadSampleStackTop - 1].sample;
            size = job->size;
            kind = job->medium;
            rate = (size >> 12) + 1;
            if (rate) {}
            AudioLoad_StartAsyncLoad((s32)job->sampleAddr,
                                     (s32)gAudioCtx.preloadSampleStack[gAudioCtx.preloadSampleStackTop - 1].ramAddr,
                                     size, kind, rate, (s32)&gAudioCtx.preloadSampleQueue,
                                     gAudioCtx.preloadSampleStack[gAudioCtx.preloadSampleStackTop - 1].encodedInfo);
        }
    }
}

// original name: EmemReload
// Preloads (sync) the samples of every font in the permanent cache, with the medium of each of its two sample
// banks that is not 0xFF.
// Load-bearing: the record address as a byte offset `off` into permanentCache, the dead-bodied `if` on the two
// bank ids, and `sampleBankReloc` declared last.
void AudioLoad_LoadPermanentSamples(void) {
    AudioTable* sampleBankTable = AudioLoad_GetLoadTable(SAMPLE_TABLE);
    AudioCacheEntry* m;
    s32 fontId;
    s32 i;
    s32 off;
    SampleBankRelocInfo sampleBankReloc;

    for (i = 0, off = 0; i < gAudioCtx.permanentPool.numEntries; i++, off += sizeof(AudioCacheEntry)) {
        m = (AudioCacheEntry*)&gAudioCtx.permanentCache[off];
        if (m->tableType != FONT_TABLE) {
            continue;
        }
        fontId = AudioLoad_GetRealTableIndex(FONT_TABLE, m->id);
        sampleBankReloc.sampleBankId1 = gAudioCtx.soundFontList[fontId].sampleBankId1;
        if ((!gAudioCtx.soundFontList[fontId].sampleBankId1) && (!gAudioCtx.soundFontList[fontId].sampleBankId2)) {}
        sampleBankReloc.sampleBankId2 = gAudioCtx.soundFontList[fontId].sampleBankId2;
        if (sampleBankReloc.sampleBankId1 != 0xFF) {
            sampleBankReloc.sampleBankId1 = AudioLoad_GetRealTableIndex(SAMPLE_TABLE, sampleBankReloc.sampleBankId1);
            sampleBankReloc.medium1 = sampleBankTable->entries[sampleBankReloc.sampleBankId1].medium;
        }
        if (sampleBankReloc.sampleBankId2 != 0xFF) {
            sampleBankReloc.sampleBankId2 = AudioLoad_GetRealTableIndex(SAMPLE_TABLE, sampleBankReloc.sampleBankId2);
            sampleBankReloc.medium2 = sampleBankTable->entries[sampleBankReloc.sampleBankId2].medium;
        }
        AudioLoad_PreloadSamplesForFont(fontId, 0, &sampleBankReloc);
    }
}

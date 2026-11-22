// original filename: memory.c
#include "global.h"
#include "audio.h"

// NA audio driver, OoT's heap.c: the pool allocators, the sequence/font/sample-bank caches and their pool
// splits, the filters, the audio-spec change and the driver init (AudioHeap_Init), and the sample caches.
// The PRINTFs are stripped debug prints whose strings stay in .data (-use_readwrite_const), in the ROM's order.

// original name: MakeReleaseTable
// Fills the ADSR decay table AudioHeap_Init allocates: 256 * updatesPerFrameInvScaled over a duration the
// index encodes, in four bands (OoT's AudioHeap_CalculateAdsrDecay inlined); index 0 is 0.
// Load-bearing: one of the two 60s spelled `(f32)60`.
void AudioHeap_InitAdsrDecayTable(void) {
    s32 i;

    gAudioCtx.adsrDecayTable[255] = 256.0f * gAudioCtx.audioBufferParameters.updatesPerFrameInvScaled / 0.25f;
    gAudioCtx.adsrDecayTable[254] = 256.0f * gAudioCtx.audioBufferParameters.updatesPerFrameInvScaled / 0.33f;
    gAudioCtx.adsrDecayTable[253] = 256.0f * gAudioCtx.audioBufferParameters.updatesPerFrameInvScaled / 0.5f;
    gAudioCtx.adsrDecayTable[252] = 256.0f * gAudioCtx.audioBufferParameters.updatesPerFrameInvScaled / 0.66f;
    gAudioCtx.adsrDecayTable[251] = 256.0f * gAudioCtx.audioBufferParameters.updatesPerFrameInvScaled / 0.75f;

    for (i = 128; i < 251; i++) {
        gAudioCtx.adsrDecayTable[i] =
            256.0f * gAudioCtx.audioBufferParameters.updatesPerFrameInvScaled / (f32)(251 - i);
    }

    for (i = 16; i < 128; i++) {
        gAudioCtx.adsrDecayTable[i] =
            256.0f * gAudioCtx.audioBufferParameters.updatesPerFrameInvScaled / (60.0f + (f32)(128 - i) * 4.0f);
    }

    for (i = 1; i < 16; i++) {
        gAudioCtx.adsrDecayTable[i] =
            256.0f * gAudioCtx.audioBufferParameters.updatesPerFrameInvScaled / (480.0f + (f32)(15 - i) * (f32)60);
    }

    gAudioCtx.adsrDecayTable[0] = 0.0f;
}

// original name: Nas_ResetIDtable
// Resets every font, sample-font and sequence load status to NOT_LOADED, except PERMANENTLY_LOADED ones.
void AudioHeap_ResetLoadStatus(void) {
    s32 i;

    for (i = 0; i < 0x46; i++) {
        if (gAudioCtx.fontLoadStatus[i] != LOAD_STATUS_PERMANENTLY_LOADED) {
            gAudioCtx.fontLoadStatus[i] = LOAD_STATUS_NOT_LOADED;
        }
    }
    for (i = 0; i < 0x46; i++) {
        if (gAudioCtx.sampleFontLoadStatus[i] != LOAD_STATUS_PERMANENTLY_LOADED) {
            gAudioCtx.sampleFontLoadStatus[i] = LOAD_STATUS_NOT_LOADED;
        }
    }
    for (i = 0; i < 0x46; i++) {
        if (gAudioCtx.seqLoadStatus[i] != LOAD_STATUS_PERMANENTLY_LOADED) {
            gAudioCtx.seqLoadStatus[i] = LOAD_STATUS_NOT_LOADED;
        }
    }
}

// original name: Nas_ForceStopChannel
// Disables every note playing from font `fontId` and moves it to the disabled free list (noteFreeLists[0]),
// first marking a still-running note's layer finished.
void AudioHeap_DiscardFont(s32 fontId) {
    s32 i;
    Note* note;

    for (i = 0; i < gAudioCtx.numNotes; i++) {
        note = &gAudioCtx.notes[i];
        if (fontId == note->playbackState.fontId) {
            PRINTF("Warning:Kill Note  %x \n");
            if (note->playbackState.status == 0 && note->playbackState.priority != 0) {
                PRINTF("Kill Voice %d (ID %d) %d\n");
                PRINTF("Warning: Running Sequence's data disappear!\n");
                note->playbackState.parentLayer->h.f.enabled = 0;
                note->playbackState.parentLayer->h.f.finished = 1;
            }
            Audio_NoteDisable(note);
            Audio_AudioListRemove(&note->listItem);
            AudioSeq_AudioListPushBack(&gAudioCtx.noteFreeLists[0], &note->listItem);
        }
    }
}

// original name: Nas_ForceStopSeq
// Disables every enabled sequence player that plays sequence `seqId`.
void AudioHeap_DiscardSequence(s32 seqId) {
    s32 i;

    for (i = 0; i < gAudioCtx.audioBufferParameters.numSequencePlayers; i++) {
        if (gAudioCtx.seqPlayers[i].enabled != 0 && gAudioCtx.seqPlayers[i].seqId == seqId) {
            AudioSeq_SequencePlayerDisable(&gAudioCtx.seqPlayers[i]);
        }
    }
}

// original name: Nas_NcHeapAlloc_CL
// The two DMA-memory allocators (zeroed, then plain): allocate, write the data cache back and return the
// uncached KSEG1 alias. OoT's stop after the writeback.
void* AudioHeap_AllocDmaMemoryZeroed(AudioAllocPool* pool, s32 size) {
    void* ramAddr = AudioHeap_AllocZeroed(pool, size);

    if (ramAddr != NULL) {
        Audio_WritebackDCache(ramAddr, size);
        ramAddr = (void*)((u32)ramAddr + 0x20000000);
    }
    return ramAddr;
}

// original name: Nas_NcHeapAlloc
void* AudioHeap_AllocDmaMemory(void* pool, s32 size) {
    void* ramAddr = AudioHeap_Alloc(pool, size);

    if (ramAddr != NULL) {
        Audio_WritebackDCache(ramAddr, size);
        ramAddr = (void*)((u32)ramAddr + 0x20000000);
    }
    return ramAddr;
}

// original name: Nas_HeapAlloc_CL
// Allocates `size` bytes from `pool` and zeroes them.
void* AudioHeap_AllocZeroed(AudioAllocPool* pool, s32 size) {
    u8* ramAddr = AudioHeap_Alloc(pool, size);
    u8* ptr;

    if (ramAddr != NULL) {
        for (ptr = ramAddr; ptr < pool->curRamAddr; ptr++) {
            *ptr = 0;
        }
    }
    return ramAddr;
}

// original name: Nas_HeapAlloc
// Bump-allocates `size` bytes, rounded up to 16, from `pool`: the old cursor, or NULL when the pool is full.
// numEntries counts allocations, not bytes.
void* AudioHeap_Alloc(AudioAllocPool* pool, s32 size) {
    u8* ramAddr;
    s32 aligned = (size + 0xF) & ~0xF;

    ramAddr = pool->curRamAddr;
    if (pool->startRamAddr + pool->size >= pool->curRamAddr + aligned) {
        pool->curRamAddr += aligned;
    } else {
        PRINTF("Heap Now %x %x %x\n");
        PRINTF("Audio:Memory:Heap OverFlow : Not Allocate %d!\n");
        return NULL;
    }
    pool->numEntries++;
    return ramAddr;
}

// original name: Nas_HeapInit
// Lays a pool over [ramAddr, ramAddr + size): the start rounded up to 16, the slack taken off the size.
void AudioHeap_InitPool(AudioAllocPool* pool, u8* ramAddr, s32 size) {
    u8* start = (u8*)(((u32)ramAddr + 0xF) & ~0xF);

    pool->startRamAddr = start;
    pool->curRamAddr = start;
    pool->size = size - ((u32)ramAddr & 0xF);
    pool->numEntries = 0;
    PRINTF("Heap Init : hp %x : base %x,cur %x,len %x\n");
}

// original name: Nas_SzStayClear
// Empties a persistent cache: no entries, and its pool rewound to the start.
void AudioHeap_InitPersistentCache(AudioPersistentCache* persistent) {
    persistent->pool.numEntries = 0;
    persistent->numEntries = 0;
    persistent->pool.curRamAddr = persistent->pool.startRamAddr;
}

// original name: Nas_SzAutoClear
// Empties a temporary cache: pool rewound, next side 0, entry 0 at the pool's start and entry 1 at
// its end (the two sides grow towards each other), both ids invalidated.
void AudioHeap_InitTemporaryCache(AudioTemporaryCache* temporary) {
    temporary->pool.numEntries = 0;
    temporary->nextSide = 0;
    temporary->entries[1].ramAddr = temporary->pool.startRamAddr + temporary->pool.size;
    temporary->entries[0].id = -1;
    temporary->entries[1].id = -1;
    temporary->pool.curRamAddr = temporary->pool.startRamAddr;
    temporary->entries[0].ramAddr = temporary->pool.startRamAddr;
}

// original name: Nas_SzCustomClear
// Rewinds a pool: everything handed out is forgotten in one step. No callers.
void AudioHeap_ResetPool(AudioAllocPool* pool) {
    pool->numEntries = 0;
    pool->curRamAddr = pool->startRamAddr;
}

// original name: Nas_SzHeapReset
// Splits the audio heap at `initPoolSize` into the init and session pools; the external pool stays unset
// until AudioThread_InitExternalPool gives it a block.
void AudioHeap_InitMainPools(s32 initPoolSize) {
    AudioHeap_InitPool(&gAudioCtx.initPool, gAudioHeap, initPoolSize);
    AudioHeap_InitPool(&gAudioCtx.sessionPool, &gAudioHeap[initPoolSize], gAudioHeapInitSizes.heapSize - initPoolSize);
    gAudioCtx.externalPool.startRamAddr = NULL;
}

// original name: Nas_SzHeapDivide
// The four pool splits, each rewinding the parent pool and carving its children out of it: session -> misc +
// cache, cache -> persistent + temporary common, each common pool -> its seq/font/sample-bank caches.
void AudioHeap_InitSessionPools(AudioSessionPoolSplit* split) {
    gAudioCtx.sessionPool.curRamAddr = gAudioCtx.sessionPool.startRamAddr;
    AudioHeap_InitPool(&gAudioCtx.miscPool, AudioHeap_Alloc(&gAudioCtx.sessionPool, split->miscPoolSize),
                       split->miscPoolSize);
    AudioHeap_InitPool(&gAudioCtx.cachePool, AudioHeap_Alloc(&gAudioCtx.sessionPool, split->cachePoolSize),
                       split->cachePoolSize);
}

// original name: Nas_SzDataDivide
void AudioHeap_InitCachePools(AudioCachePoolSplit* split) {
    gAudioCtx.cachePool.curRamAddr = gAudioCtx.cachePool.startRamAddr;
    AudioHeap_InitPool(&gAudioCtx.persistentCommonPool,
                       AudioHeap_Alloc(&gAudioCtx.cachePool, split->persistentCommonPoolSize),
                       split->persistentCommonPoolSize);
    AudioHeap_InitPool(&gAudioCtx.temporaryCommonPool,
                       AudioHeap_Alloc(&gAudioCtx.cachePool, split->temporaryCommonPoolSize),
                       split->temporaryCommonPoolSize);
    PRINTF("Audio:Memory:DataHeap Not Allocate \n");
}

// original name: Nas_SzStayDivide
void AudioHeap_InitPersistentPoolsAndCaches(AudioCommonPoolSplit* split) {
    gAudioCtx.persistentCommonPool.curRamAddr = gAudioCtx.persistentCommonPool.startRamAddr;
    AudioHeap_InitPool(&gAudioCtx.seqCache.persistent.pool,
                       AudioHeap_Alloc(&gAudioCtx.persistentCommonPool, split->seqCacheSize), split->seqCacheSize);
    AudioHeap_InitPool(&gAudioCtx.fontCache.persistent.pool,
                       AudioHeap_Alloc(&gAudioCtx.persistentCommonPool, split->fontCacheSize), split->fontCacheSize);
    AudioHeap_InitPool(&gAudioCtx.sampleBankCache.persistent.pool,
                       AudioHeap_Alloc(&gAudioCtx.persistentCommonPool, split->sampleBankCacheSize),
                       split->sampleBankCacheSize);
    PRINTF("StayHeap Not Allocate %d\n");
    AudioHeap_InitPersistentCache(&gAudioCtx.seqCache.persistent);
    AudioHeap_InitPersistentCache(&gAudioCtx.fontCache.persistent);
    AudioHeap_InitPersistentCache(&gAudioCtx.sampleBankCache.persistent);
}

// original name: Nas_SzAutoDivide
void AudioHeap_InitTemporaryPoolsAndCaches(AudioCommonPoolSplit* split) {
    gAudioCtx.temporaryCommonPool.curRamAddr = gAudioCtx.temporaryCommonPool.startRamAddr;
    AudioHeap_InitPool(&gAudioCtx.seqCache.temporary.pool,
                       AudioHeap_Alloc(&gAudioCtx.temporaryCommonPool, split->seqCacheSize), split->seqCacheSize);
    AudioHeap_InitPool(&gAudioCtx.fontCache.temporary.pool,
                       AudioHeap_Alloc(&gAudioCtx.temporaryCommonPool, split->fontCacheSize), split->fontCacheSize);
    AudioHeap_InitPool(&gAudioCtx.sampleBankCache.temporary.pool,
                       AudioHeap_Alloc(&gAudioCtx.temporaryCommonPool, split->sampleBankCacheSize),
                       split->sampleBankCacheSize);
    PRINTF("AutoHeap Not Allocate %d\n");
    AudioHeap_InitTemporaryCache(&gAudioCtx.seqCache.temporary);
    AudioHeap_InitTemporaryCache(&gAudioCtx.fontCache.temporary);
    AudioHeap_InitTemporaryCache(&gAudioCtx.sampleBankCache.temporary);
}

// original name: Nas_SzHeapAlloc
// Allocates `size` bytes for entry `id` of table `tableType` in its persistent cache (CACHE_EITHER retries in
// the temporary one) or in its temporary cache, a two-sided allocator that evicts the side it reuses.
// Load-bearing: the self-tail-call retry, the separate `temporaryPool` pointer, the ids read at each use, the
// `switch`es on nextSide, cache and the discards' tableType, the unconditional `loadStatus` resets, the shared
// `fail:` exit and the second counter `j`.
s32 AudioHeap_AllocCached(s32 tableType, s32 size, s32 cache, s32 id) {
    AudioPersistentCache* loadedCache;
    u8* ramAddr;
    s32 i;
    s32 j;
    AudioTemporaryCache* temporaryCache;
    u8 loadStatusEntry0;
    u8 loadStatusEntry1;
    AudioAllocPool* temporaryPool;
    u8* loadStatus;

    switch (tableType) {
        case SEQUENCE_TABLE:
            loadedCache = &gAudioCtx.seqCache.persistent;
            loadStatus = gAudioCtx.seqLoadStatus;
            break;
        case FONT_TABLE:
            loadedCache = &gAudioCtx.fontCache.persistent;
            loadStatus = gAudioCtx.fontLoadStatus;
            break;
        case SAMPLE_TABLE:
            loadedCache = &gAudioCtx.sampleBankCache.persistent;
            loadStatus = gAudioCtx.sampleFontLoadStatus;
            break;
    }

    if (cache == CACHE_TEMPORARY) {
        temporaryCache = (AudioTemporaryCache*)&loadedCache[1];
        temporaryPool = &temporaryCache->pool;
        if (temporaryCache->entries[0].id == -1) {
            loadStatusEntry0 = LOAD_STATUS_NOT_LOADED;
        } else {
            loadStatusEntry0 = loadStatus[temporaryCache->entries[0].id];
        }
        if (temporaryCache->entries[1].id == -1) {
            loadStatusEntry1 = LOAD_STATUS_NOT_LOADED;
        } else {
            loadStatusEntry1 = loadStatus[temporaryCache->entries[1].id];
        }
        PRINTF("Status ID0 : %d  ID1 : %d\n");

        if (tableType == FONT_TABLE) {
            if (loadStatusEntry0 == LOAD_STATUS_MAYBE_DISCARDABLE) {
                PRINTF("id 0 is Stopping\n");
                for (i = 0; i < gAudioCtx.numNotes; i++) {
                    if (gAudioCtx.notes[i].playbackState.fontId == temporaryCache->entries[0].id) {
                        if (gAudioCtx.notes[i].sampleState.h.f.enabled != 0) {
                            break;
                        }
                    }
                }
                if (i == gAudioCtx.numNotes) {
                    PRINTF("id 0 is Stop\n");
                    AudioLoad_SetFontLoadStatus(temporaryCache->entries[0].id, LOAD_STATUS_DISCARDABLE);
                    loadStatusEntry0 = LOAD_STATUS_DISCARDABLE;
                }
            }
            if (loadStatusEntry1 == LOAD_STATUS_MAYBE_DISCARDABLE) {
                PRINTF("id 1 is Stopping\n");
                for (i = 0; i < gAudioCtx.numNotes; i++) {
                    if (gAudioCtx.notes[i].playbackState.fontId == temporaryCache->entries[1].id) {
                        if (gAudioCtx.notes[i].sampleState.h.f.enabled != 0) {
                            break;
                        }
                    }
                }
                if (i == gAudioCtx.numNotes) {
                    PRINTF("id 1 is Stop\n");
                    AudioLoad_SetFontLoadStatus(temporaryCache->entries[1].id, LOAD_STATUS_DISCARDABLE);
                    loadStatusEntry1 = LOAD_STATUS_DISCARDABLE;
                }
            }
        }

        if (loadStatusEntry0 == LOAD_STATUS_NOT_LOADED) {
            temporaryCache->nextSide = 0;
        } else if (loadStatusEntry1 == LOAD_STATUS_NOT_LOADED) {
            temporaryCache->nextSide = 1;
        } else {
            PRINTF("WARNING: NO FREE AUTOSEQ AREA.\n");
            if (loadStatusEntry0 == LOAD_STATUS_DISCARDABLE && loadStatusEntry1 == LOAD_STATUS_DISCARDABLE) {
                // keep current state
            } else if (loadStatusEntry0 == LOAD_STATUS_DISCARDABLE) {
                temporaryCache->nextSide = 0;
            } else if (loadStatusEntry1 == LOAD_STATUS_DISCARDABLE) {
                temporaryCache->nextSide = 1;
            } else {
                PRINTF("WARNING: NO STOP AUTO AREA.\n");
                PRINTF("         AND TRY FORCE TO STOP SIDE \n");
                if (tableType == SEQUENCE_TABLE) {
                    if (loadStatusEntry0 == LOAD_STATUS_COMPLETE) {
                        PRINTF("Check ID0  (seq ID %d) Useing ...\n");
                        for (j = 0; j < gAudioCtx.audioBufferParameters.numSequencePlayers; j++) {
                            if (gAudioCtx.seqPlayers[j].enabled != 0 &&
                                gAudioCtx.seqPlayers[j].seqId == temporaryCache->entries[0].id) {
                                break;
                            }
                        }
                        if (j == gAudioCtx.audioBufferParameters.numSequencePlayers) {
                            temporaryCache->nextSide = 0;
                            goto allocate;
                        }
                    }
                    if (loadStatusEntry1 == LOAD_STATUS_COMPLETE) {
                        PRINTF("Check ID1  (seq ID %d) Useing ...\n");
                        for (j = 0; j < gAudioCtx.audioBufferParameters.numSequencePlayers; j++) {
                            if (gAudioCtx.seqPlayers[j].enabled != 0 &&
                                gAudioCtx.seqPlayers[j].seqId == temporaryCache->entries[1].id) {
                                break;
                            }
                        }
                        if (j == gAudioCtx.audioBufferParameters.numSequencePlayers) {
                            temporaryCache->nextSide = 1;
                            goto allocate;
                        }
                    }
                    PRINTF("No Free Seq area.\n");
                } else if (tableType == FONT_TABLE) {
                    if (loadStatusEntry0 == LOAD_STATUS_COMPLETE) {
                        for (i = 0; i < gAudioCtx.numNotes; i++) {
                            if (gAudioCtx.notes[i].playbackState.fontId == temporaryCache->entries[0].id) {
                                if (gAudioCtx.notes[i].sampleState.h.f.enabled != 0) {
                                    PRINTF("CH %d: ID %d\n");
                                    break;
                                }
                            }
                        }
                        if (i == gAudioCtx.numNotes) {
                            temporaryCache->nextSide = 0;
                            goto allocate;
                        }
                    }
                    if (loadStatusEntry1 == LOAD_STATUS_COMPLETE) {
                        for (i = 0; i < gAudioCtx.numNotes; i++) {
                            if (gAudioCtx.notes[i].playbackState.fontId == temporaryCache->entries[1].id) {
                                if (gAudioCtx.notes[i].sampleState.h.f.enabled != 0) {
                                    break;
                                }
                            }
                        }
                        if (i == gAudioCtx.numNotes) {
                            temporaryCache->nextSide = 1;
                            goto allocate;
                        }
                    }
                }

                if (temporaryCache->nextSide == 0) {
                    if (loadStatusEntry0 == LOAD_STATUS_IN_PROGRESS) {
                        if (loadStatusEntry1 != LOAD_STATUS_IN_PROGRESS) {
                            temporaryCache->nextSide = 1;
                        } else {
                            goto fail;
                        }
                    }
                } else {
                    if (loadStatusEntry1 == LOAD_STATUS_IN_PROGRESS) {
                        if (loadStatusEntry0 != LOAD_STATUS_IN_PROGRESS) {
                            temporaryCache->nextSide = 0;
                        } else {
                        fail:
                            PRINTF("TWO SIDES ARE LOADING... ALLOC CANCELED.\n");
                            return 0;
                        }
                    }
                }
            }
        }

    allocate:
        if (temporaryCache->entries[temporaryCache->nextSide].id != -1) {
            loadStatus[temporaryCache->entries[temporaryCache->nextSide].id] = LOAD_STATUS_NOT_LOADED;
            if (tableType == FONT_TABLE) {
                AudioHeap_DiscardFont(temporaryCache->entries[temporaryCache->nextSide].id);
            }
        }

        switch (temporaryCache->nextSide) {
            case 0:
                temporaryCache->entries[0].ramAddr = temporaryPool->startRamAddr;
                temporaryCache->entries[0].id = id;
                temporaryCache->entries[0].size = size;
                temporaryPool->curRamAddr = temporaryPool->startRamAddr + size;
                if (temporaryCache->entries[1].id != -1) {
                    if (temporaryCache->entries[1].ramAddr < temporaryPool->curRamAddr) {
                        PRINTF("WARNING: Before Area Overlaid After.");
                        loadStatus[temporaryCache->entries[1].id] = LOAD_STATUS_NOT_LOADED;
                        switch (tableType) {
                            case SEQUENCE_TABLE:
                                AudioHeap_DiscardSequence(temporaryCache->entries[1].id);
                                break;
                            case FONT_TABLE:
                                AudioHeap_DiscardFont(temporaryCache->entries[1].id);
                                break;
                        }
                        temporaryCache->entries[1].id = -1;
                        temporaryCache->entries[1].ramAddr = temporaryPool->startRamAddr + temporaryPool->size;
                    }
                }
                ramAddr = temporaryCache->entries[0].ramAddr;
                break;
            case 1:
                temporaryCache->entries[1].ramAddr =
                    (u8*)(((u32)(temporaryPool->startRamAddr + temporaryPool->size - size)) & ~0xF);
                temporaryCache->entries[1].id = id;
                temporaryCache->entries[1].size = size;
                if (temporaryCache->entries[0].id != -1) {
                    if (temporaryCache->entries[1].ramAddr < temporaryPool->curRamAddr) {
                        PRINTF("WARNING: After Area Overlaid Before.");
                        loadStatus[temporaryCache->entries[0].id] = LOAD_STATUS_NOT_LOADED;
                        switch (tableType) {
                            case SEQUENCE_TABLE:
                                AudioHeap_DiscardSequence(temporaryCache->entries[0].id);
                                break;
                            case FONT_TABLE:
                                AudioHeap_DiscardFont(temporaryCache->entries[0].id);
                                break;
                        }
                        temporaryCache->entries[0].id = -1;
                        temporaryPool->curRamAddr = temporaryPool->startRamAddr;
                    }
                }
                ramAddr = temporaryCache->entries[1].ramAddr;
                break;
            default:
                PRINTF("MEMORY:SzHeapAlloc ERROR: sza->side %d\n");
                return 0;
        }

        temporaryCache->nextSide = temporaryCache->nextSide ^ 1;
        return (s32)ramAddr;
    } else {
        ramAddr = AudioHeap_Alloc(&loadedCache->pool, size);
        loadedCache->entries[loadedCache->numEntries].ramAddr = ramAddr;
        if (ramAddr == NULL) {
            switch (cache) {
                case CACHE_EITHER:
                    return AudioHeap_AllocCached(tableType, size, CACHE_TEMPORARY, id);
                case CACHE_TEMPORARY:
                case CACHE_PERSISTENT:
                    PRINTF("Audio:MEMORY:SzHeap Overflow error. (%d bytes)\n");
                    return 0;
            }
        }
        loadedCache->entries[loadedCache->numEntries].id = id;
        loadedCache->entries[loadedCache->numEntries].size = size;
        return (s32)loadedCache->entries[loadedCache->numEntries++].ramAddr;
    }
}

// original name: Nas_SzCacheCheck
// Returns the address of entry `id` of table `tableType`, or 0: the permanent cache first, then, unless
// `cache` is CACHE_PERMANENT, the regular caches.
s32 AudioHeap_SearchCaches(s32 tableType, s32 cache, s32 id) {
    s32 ramAddr = AudioHeap_SearchPermanentCache(tableType, id);

    if (ramAddr != 0) {
        return ramAddr;
    }
    if (cache == CACHE_PERMANENT) {
        return 0;
    }
    return AudioHeap_SearchRegularCaches(tableType, cache, id);
}

// original name: __Nas_SzCacheCheck_Inner
// Looks `id` up in the temporary cache, where a hit points nextSide at the other side, or in the persistent
// cache, which falls back to the temporary one for CACHE_EITHER.
// Load-bearing: the table `switch` without a `default`.
s32 AudioHeap_SearchRegularCaches(s32 tableType, s32 cache, s32 id) {
    u32 i;
    AudioPersistentCache* loadedCache;
    AudioTemporaryCache* temporaryCache;
    AudioPersistentCache* persistentCache;

    switch (tableType) {
        case SEQUENCE_TABLE:
            loadedCache = &gAudioCtx.seqCache.persistent;
            break;
        case FONT_TABLE:
            loadedCache = &gAudioCtx.fontCache.persistent;
            break;
        case SAMPLE_TABLE:
            loadedCache = &gAudioCtx.sampleBankCache.persistent;
            break;
    }
    temporaryCache = (AudioTemporaryCache*)(loadedCache + 1);
    if (cache == CACHE_TEMPORARY) {
        if (id == temporaryCache->entries[0].id) {
            temporaryCache->nextSide = 1;
            return (s32)temporaryCache->entries[0].ramAddr;
        }
        if (id == temporaryCache->entries[1].id) {
            temporaryCache->nextSide = 0;
            return (s32)temporaryCache->entries[1].ramAddr;
        }
        PRINTF("Auto Heap Unhit for ID %d\n");
        return 0;
    }
    persistentCache = loadedCache;
    for (i = 0; i < persistentCache->numEntries; i++) {
        if (id == loadedCache->entries[i].id) {
            return (s32)loadedCache->entries[i].ramAddr;
        }
    }
    if (cache == CACHE_EITHER) {
        return AudioHeap_SearchCaches(tableType, CACHE_TEMPORARY, id);
    }
    return 0;
}

// original name: Nas_InitFilterCoef
// Unreferenced and buggy, as in OoT: two interleaved second-order recurrences scaled by 262159 that store
// into `out` instead of `t`, so the final loop converts uninitialised stack.
// Load-bearing: `q * t[i - 2]` written first.
void AudioHeap_InitFilterCoef(f32 p, f32 q, u16* out) {
    s32 i;
    f32 t[16];

    t[0] = q * 262159.0f;
    t[8] = p * 262159.0f;
    t[1] = (q * p) * 262159.0f;
    t[9] = (p * p + q) * 262159.0f;

    for (i = 2; i < 8; i++) {
        //! @bug value should be stored to t[i] and t[i + 8], otherwise we read
        //! garbage in later loop iterations.
        out[i] = q * t[i - 2] + t[i - 1] * p;
        out[i + 8] = q * t[i + 6] + t[i + 7] * p;
    }

    for (i = 0; i < 16; i++) {
        out[i] = t[i];
    }
}

// original name: Nas_ClearFilter
// Clears an 8-halfword filter. No callers.
void AudioHeap_ClearFilter(s16* filter) {
    s32 i;

    for (i = 0; i < 8; i++) {
        filter[i] = 0;
    }
}

// original name: Nas_SetLPFilter
// The two filter loaders copy one 8-halfword row out of the low-pass (gLowPassFilterData) or the
// high-pass (gHighPassFilterData) coefficient table; the high-pass table has no row for cutoff 0.
void AudioHeap_LoadLowPassFilter(s16* filter, s32 cutoff) {
    s32 i;

    for (i = 0; i < 8; i++) {
        filter[i] = gLowPassFilterData[cutoff * 8 + i];
    }
}

// original name: Nas_SetHPFilter
void AudioHeap_LoadHighPassFilter(s16* filter, s32 cutoff) {
    s16* src = &gHighPassFilterData[(cutoff - 1) * 8];
    s32 i;

    for (i = 0; i < 8; i++) {
        filter[i] = src[i];
    }
}

// original name: Nas_SetBPFilter
// Loads low-pass row `lowPassCutoff` or high-pass row `highPassCutoff`, whichever is set (neither: low-pass
// row 0); both set average the two tables' row `highPassCutoff` (OoT: rows lowPassCutoff, highPassCutoff - 1).
void AudioHeap_LoadFilter(s16* filter, s32 lowPassCutoff, s32 highPassCutoff) {
    s32 i;

    if (lowPassCutoff == 0 && highPassCutoff == 0) {
        AudioHeap_LoadLowPassFilter(filter, 0);
        return;
    }
    if (highPassCutoff == 0) {
        AudioHeap_LoadLowPassFilter(filter, lowPassCutoff);
        return;
    }
    if (lowPassCutoff == 0) {
        AudioHeap_LoadHighPassFilter(filter, highPassCutoff);
        return;
    }
    for (i = 0; i < 8; i++) {
        filter[i] = (gLowPassFilterData[highPassCutoff * 8 + i] + gHighPassFilterData[highPassCutoff * 8 + i]) / 2;
    }
}

// original name: __Nas_DelayDown
// Takes a third off every reverb's decayRatio, twice when specUnk4 is 2: OoT's per-reverb
// AudioHeap_UpdateReverb (original __DownDelay, an empty stub there) inlined.
void AudioHeap_UpdateReverbs(void) {
    s32 passes;
    s32 rate;
    s32 i;
    s32 j;

    rate = 3;
    if (gAudioCtx.audioBufferParameters.specUnk4 == 2) {
        passes = 2;
    } else {
        passes = 1;
    }
    for (i = 0; i < gAudioCtx.numSynthesisReverbs; i++) {
        for (j = 0; j < passes; j++) {
            gAudioCtx.synthesisReverbs[i].decayRatio -= gAudioCtx.synthesisReverbs[i].decayRatio / rate;
        }
    }
}

// original name: __Nas_DacClear
// Zeroes the AI buffer about to be filled and sets its length to the minimum.
void AudioHeap_ClearCurrentAiBuffer(void) {
    s32 curAiBufferIndex = gAudioCtx.curAiBufIndex;
    s32 i;

    gAudioCtx.aiBufLengths[curAiBufferIndex] = gAudioCtx.audioBufferParameters.minAiBufferLength;
    for (i = 0; i < 0x580; i++) {
        gAudioCtx.aiBuffers[curAiBufferIndex][i] = 0;
    }
}

// original name: Nas_SpecChange
// The audio-spec change, one phase per call while resetStatus counts 5 down to 1: stop the players, release
// the notes, drain, discard the sample caches, re-run AudioHeap_Init. Returns 1 while resetStatus >= 3.
// Load-bearing: resetStatus `volatile` (audio.h), and the last clear loop's single store and reused `i`/`j`.
s32 AudioHeap_ResetStep(void) {
    s32 i;
    s32 j;
    s32 n;

    if (gAudioCtx.audioBufferParameters.specUnk4 == 2) {
        n = 2;
    } else {
        n = 1;
    }
    switch (gAudioCtx.resetStatus) {
        case 5:
            for (i = 0; i < gAudioCtx.audioBufferParameters.numSequencePlayers; i++) {
                AudioSeq_SequencePlayerDisable(&gAudioCtx.seqPlayers[i]);
            }
            gAudioCtx.audioResetFadeOutFramesLeft = 4 / n;
            gAudioCtx.resetStatus--;
            break;
        case 4:
            if (gAudioCtx.audioResetFadeOutFramesLeft != 0) {
                gAudioCtx.audioResetFadeOutFramesLeft--;
                AudioHeap_UpdateReverbs();
            } else {
                for (i = 0; i < gAudioCtx.numNotes; i++) {
                    if (gAudioCtx.notes[i].sampleState.h.f.enabled != 0) {
                        if (gAudioCtx.notes[i].playbackState.adsr.h.f.state != ADSR_STATE_DISABLED) {
                            gAudioCtx.notes[i].playbackState.adsr.fadeOutVel =
                                gAudioCtx.audioBufferParameters.updatesPerFrameInv;
                            gAudioCtx.notes[i].playbackState.adsr.h.f.release = 1;
                        }
                    }
                }
                gAudioCtx.audioResetFadeOutFramesLeft = 0x10 / n;
                gAudioCtx.resetStatus--;
            }
            break;
        case 3:
            if (gAudioCtx.audioResetFadeOutFramesLeft != 0) {
                gAudioCtx.audioResetFadeOutFramesLeft--;
                AudioHeap_UpdateReverbs();
            } else {
                gAudioCtx.audioResetFadeOutFramesLeft = 4 / n;
                gAudioCtx.resetStatus--;
            }
            break;
        case 2:
            AudioHeap_ClearCurrentAiBuffer();
            if (gAudioCtx.audioResetFadeOutFramesLeft != 0) {
                gAudioCtx.audioResetFadeOutFramesLeft--;
            } else {
                gAudioCtx.resetStatus--;
                AudioHeap_DiscardSampleCaches();
            }
            break;
        case 1:
            AudioHeap_Init();
            gAudioCtx.resetStatus = 0;
            for (i = 0; i < 3; i++) {
                gAudioCtx.aiBufLengths[i] = gAudioCtx.audioBufferParameters.maxAiBufferLength;
                for (j = 0; j < 0x580; j++) {
                    gAudioCtx.aiBuffers[i][j] = 0;
                }
            }
            break;
    }
    if (gAudioCtx.resetStatus < 3) {
        return 0;
    }
    return 1;
}

// original name: __Nas_MemoryReconfig
// Sets the driver up for the current audio spec: AI rate, buffer parameters, notes, command
// buffers, pool splits and one SynthesisReverb per reverb setting.
// Load-bearing: the reverb loop's index-expression cursors (only inst_fp walks), the one-line
// `do {`, `if (ch_offset) {}`, the dummy locals and declaration order, and the statement order.
void AudioHeap_Init(void) {
    UNUSED s32 dummy0;
    UNUSED s32 dummy1;
    UNUSED s32 dummy2;
    UNUSED s32 dummy3;
    ReverbSettings* ch;
    s32 quotient;
    SynthesisReverb* inst;
    s32 j;
    OSIntMask mask;
    SynthesisReverb* inst_fp;
    s32 i;
    s32 ch_offset;
    UNUSED s32 dummy5;
    AudioSpec* spec;

    spec = &gAudioSpecs[gAudioCtx.specId];
    PRINTF("Heap Reconstruct Start %x\n");
    gAudioCtx.sampleDmaCount = 0;

    gAudioCtx.audioBufferParameters.samplingFrequency = spec->samplingFrequency;
    gAudioCtx.audioBufferParameters.aiSamplingFrequency =
        osAiSetFrequency(gAudioCtx.audioBufferParameters.samplingFrequency);

    gAudioCtx.audioBufferParameters.samplesPerFrameTarget =
        (gAudioCtx.audioBufferParameters.samplingFrequency / gAudioCtx.refreshRate + 15) & 0xFFF0;
    gAudioCtx.audioBufferParameters.minAiBufferLength = gAudioCtx.audioBufferParameters.samplesPerFrameTarget - 16;
    gAudioCtx.audioBufferParameters.updatesPerFrame =
        (gAudioCtx.audioBufferParameters.samplesPerFrameTarget + 16) / 208 + 1;
    gAudioCtx.audioBufferParameters.maxAiBufferLength = gAudioCtx.audioBufferParameters.samplesPerFrameTarget + 16;
    gAudioCtx.audioBufferParameters.samplesPerUpdate =
        (gAudioCtx.audioBufferParameters.samplesPerFrameTarget / gAudioCtx.audioBufferParameters.updatesPerFrame) & ~7;

    gAudioCtx.audioBufferParameters.samplesPerUpdateMax = gAudioCtx.audioBufferParameters.samplesPerUpdate + 8;
    gAudioCtx.audioBufferParameters.samplesPerUpdateMin = gAudioCtx.audioBufferParameters.samplesPerUpdate - 8;

    gAudioCtx.audioBufferParameters.resampleRate =
        32000.0f / (f32)(s32)gAudioCtx.audioBufferParameters.samplingFrequency;
    gAudioCtx.audioBufferParameters.updatesPerFrameInvScaled =
        0.00390625f / (f32)gAudioCtx.audioBufferParameters.updatesPerFrame;
    gAudioCtx.audioBufferParameters.updatesPerFrameScaled =
        (f32)gAudioCtx.audioBufferParameters.updatesPerFrame * 0.25f;
    gAudioCtx.audioBufferParameters.updatesPerFrameInv = 1.0f / (f32)gAudioCtx.audioBufferParameters.updatesPerFrame;

    gAudioCtx.numNotes = spec->numNotes;
    gAudioCtx.audioBufferParameters.numSequencePlayers = spec->numSequencePlayers;
    if (gAudioCtx.audioBufferParameters.numSequencePlayers >= 4) {
        gAudioCtx.audioBufferParameters.numSequencePlayers = 3;
    }

    gAudioCtx.unused2 = spec->unkC;
    gAudioCtx.maxTempo = (u32)((f32)gAudioCtx.audioBufferParameters.updatesPerFrame * 2880000.0f /
                               (f32)gTempoData.seqTicksPerBeat / gAudioCtx.maxTempoTvTypeFactors);
    PRINTF("---------------------------------------TEMPO %d %f\n");
    PRINTF("%f \n");
    PRINTF("%f \n");

    gAudioCtx.audioBufferParameters.specUnk4 = spec->unk4;
    gAudioCtx.audioBufferParameters.samplesPerFrameTarget =
        gAudioCtx.audioBufferParameters.samplesPerFrameTarget * gAudioCtx.audioBufferParameters.specUnk4;
    gAudioCtx.audioBufferParameters.maxAiBufferLength =
        gAudioCtx.audioBufferParameters.maxAiBufferLength * gAudioCtx.audioBufferParameters.specUnk4;
    gAudioCtx.audioBufferParameters.minAiBufferLength =
        gAudioCtx.audioBufferParameters.minAiBufferLength * gAudioCtx.audioBufferParameters.specUnk4;
    gAudioCtx.audioBufferParameters.updatesPerFrame =
        gAudioCtx.audioBufferParameters.updatesPerFrame * gAudioCtx.audioBufferParameters.specUnk4;

    if (gAudioCtx.audioBufferParameters.specUnk4 >= 2) {
        gAudioCtx.audioBufferParameters.maxAiBufferLength -= 16;
    }

    gAudioCtx.maxAudioCmds =
        (gAudioCtx.numNotes * 20) * gAudioCtx.audioBufferParameters.updatesPerFrame + spec->numReverbs * 32 + 480;

    PRINTF("AHPBASE %x\n");
    PRINTF("AHPCUR  %x\n");
    PRINTF("HeapTop %x\n");
    {
        s32 s0 = spec->persistentSeqCacheSize + spec->persistentFontCacheSize + spec->persistentSampleBankCacheSize +
                 spec->persistentSampleCacheSize + 16;
        s32 s1 = spec->temporarySeqCacheSize + spec->temporaryFontCacheSize + spec->temporarySampleBankCacheSize +
                 spec->temporarySampleCacheSize + 16;

        quotient = gAudioCtx.sessionPool.size - (s0 + s1) - 256;
        gAudioCtx.sessionPoolSplit.miscPoolSize = quotient;
        gAudioCtx.sessionPoolSplit.cachePoolSize = s0 + s1;
        AudioHeap_InitSessionPools(&gAudioCtx.sessionPoolSplit);

        gAudioCtx.cachePoolSplit.persistentCommonPoolSize = s0;
        gAudioCtx.cachePoolSplit.temporaryCommonPoolSize = s1;
        AudioHeap_InitCachePools(&gAudioCtx.cachePoolSplit);

        gAudioCtx.persistentCommonPoolSplit.seqCacheSize = spec->persistentSeqCacheSize;
        gAudioCtx.persistentCommonPoolSplit.fontCacheSize = spec->persistentFontCacheSize;
        gAudioCtx.persistentCommonPoolSplit.sampleBankCacheSize = spec->persistentSampleBankCacheSize;
        AudioHeap_InitPersistentPoolsAndCaches(&gAudioCtx.persistentCommonPoolSplit);

        gAudioCtx.temporaryCommonPoolSplit.seqCacheSize = spec->temporarySeqCacheSize;
        gAudioCtx.temporaryCommonPoolSplit.fontCacheSize = spec->temporaryFontCacheSize;
        gAudioCtx.temporaryCommonPoolSplit.sampleBankCacheSize = spec->temporarySampleBankCacheSize;
        AudioHeap_InitTemporaryPoolsAndCaches(&gAudioCtx.temporaryCommonPoolSplit);
    }

    AudioHeap_InitSampleCaches(spec->persistentSampleCacheSize, spec->temporarySampleCacheSize);

    quotient = (s32)gAudioCtx.externalPool.startRamAddr;
    if (quotient != 0) {
        gAudioCtx.externalPool.curRamAddr = (u8*)quotient;
    }

    AudioHeap_ResetLoadStatus();

    gAudioCtx.notes = AudioHeap_AllocZeroed(&gAudioCtx.miscPool, gAudioCtx.numNotes * sizeof(Note));
    Audio_NoteInitAll();
    Audio_InitNoteFreeList();

    gAudioCtx.sampleStates =
        AudioHeap_AllocZeroed(&gAudioCtx.miscPool, gAudioCtx.audioBufferParameters.updatesPerFrame *
                                                       gAudioCtx.numNotes * sizeof(NoteSampleState));

    for (i = 0; i != 2; i++) {
        gAudioCtx.abiCmdBufs[i] = (s32)AudioHeap_AllocDmaMemoryZeroed(&gAudioCtx.miscPool, gAudioCtx.maxAudioCmds * 8);
    }
    PRINTF("SynoutRate %d / %d \n");

    gAudioCtx.adsrDecayTable = (f32*)AudioHeap_Alloc(&gAudioCtx.miscPool, 0x400);
    AudioHeap_InitAdsrDecayTable();

    for (i = 0; i < ARRAY_COUNT(gAudioCtx.synthesisReverbs); i++) {
        gAudioCtx.synthesisReverbs[i].useReverb = 0;
    }

    gAudioCtx.numSynthesisReverbs = spec->numReverbs;
    i = 0;
    if (gAudioCtx.numSynthesisReverbs > 0) {
        // clang-format off
        inst_fp = (SynthesisReverb*)((u8*)&gAudioCtx.synthesisReverbs[0] + 0x118); do {
            // clang-format on
            ch_offset = i * 18;
            ch = (ReverbSettings*)((u8*)spec->reverbSettings + ch_offset);
            if (ch_offset) {}

            gAudioCtx.synthesisReverbs[i].downsampleRate = ch->downsampleRate;
            gAudioCtx.synthesisReverbs[i].windowSize = ch->windowSize << 6;
            gAudioCtx.synthesisReverbs[i].windowSize /= gAudioCtx.synthesisReverbs[i].downsampleRate;
            PRINTF("FXSIZE %d\n");
            PRINTF("FXCOMP %d\n");
            PRINTF("FXDOWN %d\n");

            gAudioCtx.synthesisReverbs[i].decayRatio = ch->decayRatio;
            gAudioCtx.synthesisReverbs[i].volume = ch->volume;
            gAudioCtx.synthesisReverbs[i].leakRtl = ch->leakRtl;
            gAudioCtx.synthesisReverbs[i].leakLtr = ch->leakLtr;
            gAudioCtx.synthesisReverbs[i].mixReverbIndex = ch->mixReverbIndex;
            gAudioCtx.synthesisReverbs[i].mixReverbStrength = ch->mixReverbStrength;
            gAudioCtx.synthesisReverbs[i].useReverb = 8;

            inst = &gAudioCtx.synthesisReverbs[i];
            inst->leftRingBuf =
                AudioHeap_AllocDmaMemoryZeroed(&gAudioCtx.miscPool, gAudioCtx.synthesisReverbs[i].windowSize * 2);
            inst->rightRingBuf = AudioHeap_AllocDmaMemoryZeroed(&gAudioCtx.miscPool, inst->windowSize * 2);

            inst->nextRingBufPos = 0;
            inst->unused18 = 0;
            inst->curFrame = 0;
            inst->framesToIgnore = 2;
            inst->resampleFlags = 1;
            inst->tunedSample.sample = (Sample*)inst_fp;
            inst->bufSizePerChan = inst->windowSize;
            inst->sample.loop = (AdpcmLoop*)((u8*)&gAudioCtx.synthesisReverbs[i] + 0x128);
            inst->tunedSample.tuning = 1.0f;

            inst->sample.codec = CODEC_REVERB;
            inst->sample.medium = MEDIUM_RAM;
            inst->sample.size = inst->bufSizePerChan * 2;
            inst->loop.start = 0;
            inst->loop.end = inst->windowSize;
            inst->sample.sampleAddr = (u8*)inst->leftRingBuf;
            inst->loop.count = 1;
            if (inst->downsampleRate != 1) {
                inst->downsamplePitch = 0x8000 / inst->downsampleRate;

                inst->leftLoadResampleBuf = AudioHeap_AllocZeroed(&gAudioCtx.miscPool, 32);
                inst->rightLoadResampleBuf = AudioHeap_AllocZeroed(&gAudioCtx.miscPool, 32);
                inst->leftSaveResampleBuf = AudioHeap_AllocZeroed(&gAudioCtx.miscPool, 32);
                inst->rightSaveResampleBuf = AudioHeap_AllocZeroed(&gAudioCtx.miscPool, 32);

                {
                    for (j = 0; j < gAudioCtx.audioBufferParameters.updatesPerFrame; j++) {
                        void* ptr0;
                        void* ptr1;

                        ptr0 = AudioHeap_AllocZeroed(&gAudioCtx.miscPool, 832);
                        inst->items[0][j].toDownsampleLeft = ptr0;
                        inst->items[0][j].toDownsampleRight = (s16*)((u8*)ptr0 + 416);

                        ptr1 = AudioHeap_AllocZeroed(&gAudioCtx.miscPool, 832);
                        inst->items[1][j].toDownsampleLeft = ptr1;
                        inst->items[1][j].toDownsampleRight = (s16*)((u8*)ptr1 + 416);
                    }
                }
            }

            if (ch->lowPassFilterCutoffLeft != 0) {
                inst->filterLeftState = AudioHeap_AllocDmaMemory(&gAudioCtx.miscPool, 32);
                inst->filterLeft = AudioHeap_AllocDmaMemory(&gAudioCtx.miscPool, 16);
                AudioHeap_LoadLowPassFilter(inst->filterLeft, ch->lowPassFilterCutoffLeft);
            } else {
                inst->filterLeft = NULL;
            }

            if (ch->lowPassFilterCutoffRight != 0) {
                inst->filterRightState = AudioHeap_AllocDmaMemory(&gAudioCtx.miscPool, 32);
                inst->filterRight = AudioHeap_AllocDmaMemory(&gAudioCtx.miscPool, 16);
                AudioHeap_LoadLowPassFilter(inst->filterRight, ch->lowPassFilterCutoffRight);
            } else {
                inst->filterRight = NULL;
            }

            i++, inst_fp = (SynthesisReverb*)((u8*)inst_fp + 0x158);
        } while (i < gAudioCtx.numSynthesisReverbs);
    }

    AudioSeq_InitSequencePlayers();
    for (j = 0; j < gAudioCtx.audioBufferParameters.numSequencePlayers; j++) {
        AudioSeq_InitSequencePlayerChannels(j);
        AudioSeq_ResetSequencePlayer(&gAudioCtx.seqPlayers[j]);
    }

    AudioLoad_InitSampleDmaBuffers(gAudioCtx.numNotes);
    PRINTF("WaveCacheLen: %d\n");
    gAudioCtx.preloadSampleStackTop = 0;
    AudioLoad_InitSlowLoads();

    gAudioCtx.unused4 = 0x1000;
    AudioLoad_LoadPermanentSamples();

    mask = osSetIntMask(1);
    osWritebackDCacheAll();
    osSetIntMask(mask);
    PRINTF("SpecChange Finished\n");

    PRINTF("Heap overview\n");
    PRINTF("HP_MAIN %8x\n");
    PRINTF(" DAC1    %8x\n");
    PRINTF(" DAC2    %8x\n");
    PRINTF(" DAC3    %8x\n");
    PRINTF("HP_SYND %8x\n");
    PRINTF(" CHANNEL  %8x\n");
    PRINTF(" DREG     %8x\n");
    PRINTF(" TASK1    %8x\n");
    PRINTF(" TASK2    %8x\n");
    PRINTF(" RELT     %8x\n");
    PRINTF(" delay0L   %8x\n");
    PRINTF(" delay0R   %8x\n");
    PRINTF(" delay1L   %8x\n");
    PRINTF(" delay1R   %8x\n");
    PRINTF(" Gr0:Sub0  %8x\n");
    PRINTF(" -------------\n");
}

// original name: EmemOnCheck
// Looks (tableType, id) up in the permanent cache's 0xC-byte entries and returns its address.
s32 AudioHeap_SearchPermanentCache(s32 tableType, s32 id) {
    s32 i;

    for (i = 0; i < gAudioCtx.permanentPool.numEntries; i++) {
        if (tableType == PERMANENT_CACHE_ENTRY(i)->tableType && id == PERMANENT_CACHE_ENTRY(i)->id) {
            return (s32)PERMANENT_CACHE_ENTRY(i)->ramAddr;
        }
    }
    return 0;
}

// original name: EmemAlloc
// Allocates a block from the permanent pool and files it in the permanent cache under (tableType, id).
// Load-bearing: no `return` on the success path (OoT's `@bug`), and `ramAddr` assigned apart from its declaration.
void* AudioHeap_AllocPermanent(s32 tableType, s32 id, s32 size) {
    void* ramAddr;
    s32 index = gAudioCtx.permanentPool.numEntries;

    ramAddr = AudioHeap_Alloc(&gAudioCtx.permanentPool, size);
    PERMANENT_CACHE_ENTRY(index)->ramAddr = ramAddr;
    if (ramAddr == NULL) {
        PRINTF("Warning:Emem Over,not alloc %d\n");
        return NULL;
    }
    PERMANENT_CACHE_ENTRY(index)->tableType = tableType;
    PERMANENT_CACHE_ENTRY(index)->id = id;
    PERMANENT_CACHE_ENTRY(index)->size = size;
}

// The temporary and persistent sample-cache allocators (originals unattested; their `_Inner` callees suggest
// Nas_Alloc_Single_Auto/_Stay): take an entry, record bank, address and medium in it, return the block.
void* AudioHeap_AllocTemporarySampleCache(s32 size, s32 fontId, s32 sampleAddr, s8 medium) {
    SampleCacheEntry* entry = AudioHeap_AllocTemporarySampleCacheEntry(size);

    if (entry != NULL) {
        entry->sampleBankId = fontId;
        entry->sampleAddr = sampleAddr;
        entry->origMedium = medium;
        return entry->allocatedAddr;
    }
    return NULL;
}

void* AudioHeap_AllocPersistentSampleCache(s32 size, s32 fontId, s32 sampleAddr, s8 medium) {
    SampleCacheEntry* entry = AudioHeap_AllocPersistentSampleCacheEntry(size);

    if (entry != NULL) {
        entry->sampleBankId = fontId;
        entry->sampleAddr = sampleAddr;
        entry->origMedium = medium;
        return entry->allocatedAddr;
    }
    return NULL;
}

// original name: Nas_Init_Single
// Lays the persistent and temporary sample caches over blocks of the two common pools (OoT: the misc pool)
// and empties both; a failed allocation leaves that cache's pool size 0.
// Load-bearing: the empty `if` on the work area and the gAudioCtxPersistentSampleCachePoolSize alias store.
void AudioHeap_InitSampleCaches(s32 persistentSampleCacheSize, s32 temporarySampleCacheSize) {
    void* block0;
    void* block1;

    block0 = AudioHeap_Alloc(&gAudioCtx.persistentCommonPool, persistentSampleCacheSize);
    if (block0 == NULL) {
        gAudioCtxPersistentSampleCachePoolSize = 0;
    } else {
        AudioHeap_InitPool(&gAudioCtx.persistentSampleCache.pool, block0, persistentSampleCacheSize);
    }
    if (gAudioCtx.persistentSampleCache.numEntries) {}
    block1 = AudioHeap_Alloc(&gAudioCtx.temporaryCommonPool, temporarySampleCacheSize);
    PRINTF("Single AutoSize %d\n");
    PRINTF("Single Ptr %x\n");
    if (block1 == NULL) {
        gAudioCtx.temporarySampleCache.pool.size = 0;
    } else {
        AudioHeap_InitPool(&gAudioCtx.temporarySampleCache.pool, block1, temporarySampleCacheSize);
    }
    gAudioCtx.persistentSampleCache.numEntries = 0;
    gAudioCtx.temporarySampleCache.numEntries = 0;
}

// original name: __Nas_Alloc_Single_Auto_Inner
// Allocates `size` bytes from the temporary sample cache's pool, wrapping to its base once, then abandons the
// preloads and discards the entries the block overlaps, reusing the first entry freed (else appending one).
// Load-bearing: `endRamAddr = *(u8**)&size`, loop 2's two bindings before its guards, the `continue` guards
// (loop 1's first test nested), the `cache` and `cur` pointers, and the re-read `cache->numEntries + 1`.
SampleCacheEntry* AudioHeap_AllocTemporarySampleCacheEntry(s32 size) {
    u8* allocBefore;
    u8* allocAfter;
    void* ramAddr;
    u8* old;
    s32 i;
    s32 index;
    u8* startRamAddr;
    u8* endRamAddr;
    AudioPreloadReq* preload;
    AudioSampleCache* cache = &gAudioCtx.temporarySampleCache;
    AudioAllocPool* pool = &cache->pool;
    u8** cur = &gAudioCtx.temporarySampleCache.pool.curRamAddr;

    PRINTF("Request--------Single-Auto, %d\n");
    allocBefore = gAudioCtx.temporarySampleCache.pool.curRamAddr;
    endRamAddr = *(u8**)&size;
    ramAddr = AudioHeap_Alloc(pool, (s32)endRamAddr);
    if (ramAddr == NULL) {
        PRINTF("Retry %x, %x, len %x\n");
        old = *cur;
        *cur = pool->startRamAddr;
        ramAddr = AudioHeap_Alloc(pool, size);
        if (ramAddr == NULL) {
            *cur = old;
            return NULL;
        }
        allocBefore = pool->startRamAddr;
    }
    allocAfter = *cur;

    index = -1;
    for (i = 0; i < gAudioCtx.preloadSampleStackTop; i++) {
        preload = &gAudioCtx.preloadSampleStack[i];
        if (preload->isFree == 0) {
            startRamAddr = preload->ramAddr;
            if ((u8*)preload->ramAddr + preload->sample->size - 1 < allocBefore && startRamAddr < allocBefore) {
                continue;
            }
            if (!((u8*)preload->ramAddr + preload->sample->size - 1 < allocAfter || startRamAddr < allocAfter)) {
                continue;
            }
            PRINTF("DMAing list %d is killed.\n");
            preload->isFree = 1;
        }
    }
    for (i = 0; i < cache->numEntries; i++) {
        if (cache->entries[i].inUse == 0) {
            continue;
        }
        startRamAddr = cache->entries[i].allocatedAddr;
        endRamAddr = startRamAddr + cache->entries[i].size - 1;
        if (endRamAddr < allocBefore && startRamAddr < allocBefore) {
            continue;
        }
        if (!(endRamAddr < allocAfter || startRamAddr < allocAfter)) {
            continue;
        }
        AudioHeap_DiscardSampleCacheEntry(&cache->entries[i]);
        if (index == -1) {
            index = i;
        }
    }
    if (index == -1) {
        index = cache->numEntries;
        cache->numEntries = cache->numEntries + 1;
    }
    cache->entries[index].inUse = 1;
    cache->entries[index].allocatedAddr = ramAddr;
    cache->entries[index].size = size;
    return &cache->entries[index];
}

void AudioHeap_UnapplySampleCache(SampleCacheEntry* entry, Sample* sample);

// original name: __KillSwMember
// For every cached, fully loaded font that names `entry`'s sample bank (every font for bank 0), gives back the
// samples it borrowed from `entry`: OoT's AudioHeap_UnapplySampleCacheForFont inlined, minus sound effects.
// Load-bearing: the trailing `if (fontId) {}`, the three `s32` sample-bank locals, and `inst` reused for the
// drums through a `(Drum*)` cast.
void AudioHeap_DiscardSampleCacheEntry(SampleCacheEntry* entry) {
    Instrument* inst;
    s32 numFonts;
    s32 fontId;
    s32 i;
    s32 j;
    s32 sampleBankId;
    s32 sampleBankId1;
    s32 sampleBankId2;

    numFonts = gAudioCtx.soundFontTable->header.numEntries;
    for (fontId = 0; fontId < numFonts; fontId++) {
        sampleBankId = entry->sampleBankId;
        sampleBankId1 = gAudioCtx.soundFontList[fontId].sampleBankId1;
        sampleBankId2 = gAudioCtx.soundFontList[fontId].sampleBankId2;
        if ((sampleBankId1 != 0xFF && sampleBankId1 == sampleBankId) ||
            (sampleBankId2 != 0xFF && sampleBankId2 == sampleBankId) || sampleBankId == 0) {
            PRINTF("Try Kill %d \n");
            PRINTF("Try Kill %x %x\n");
            PRINTF("Try Kill %x %x %x\n");
            if (AudioHeap_SearchCaches(FONT_TABLE, CACHE_EITHER, fontId) != 0 &&
                AudioLoad_IsFontLoadComplete(fontId) != 0) {
                for (i = 0; i < gAudioCtx.soundFontList[fontId].numInstruments; i++) {
                    inst = Audio_GetInstrumentInner(fontId, i);
                    if (inst != NULL) {
                        if (inst->normalRangeLo != 0) {
                            AudioHeap_UnapplySampleCache(entry, inst->lowPitchTunedSample.sample);
                        }
                        if (inst->normalRangeHi != 0x7F) {
                            AudioHeap_UnapplySampleCache(entry, inst->highPitchTunedSample.sample);
                        }
                        AudioHeap_UnapplySampleCache(entry, inst->normalPitchTunedSample.sample);
                    }
                }
                for (j = 0; j < gAudioCtx.soundFontList[fontId].numDrums; j++) {
                    inst = Audio_GetDrum(fontId, j);
                    if (inst != NULL) {
                        AudioHeap_UnapplySampleCache(entry, ((Drum*)inst)->tunedSample.sample);
                    }
                }
            }
        }
    }
    if (fontId) {}
}

// original name: __RomAddrSet
// Gives `sample` its original address and medium back if it still points at the block `entry` lent it.
void AudioHeap_UnapplySampleCache(SampleCacheEntry* entry, Sample* sample) {
    if (sample != NULL) {
        if (sample->sampleAddr == entry->allocatedAddr) {
            PRINTF("Rom back %x %x \n");
            sample->sampleAddr = (void*)entry->sampleAddr;
            sample->medium = entry->origMedium;
        }
    } else {
        PRINTF("Error sw NULL \n");
    }
}

// original name: __Nas_Alloc_Single_Stay_Inner
// Allocates `size` bytes from the persistent sample cache's pool and records them in the cache's next entry.
// Load-bearing: the `s32* p` pointer to numEntries and the gAudioCtxPersistentSampleCache alias for the entry.
SampleCacheEntry* AudioHeap_AllocPersistentSampleCacheEntry(s32 size) {
    void* ramAddr = AudioHeap_Alloc(&gAudioCtx.persistentSampleCache.pool, size);
    SampleCacheEntry* entry;
    s32* p = &gAudioCtx.persistentSampleCache.numEntries;

    PRINTF("Request--------Single-Stay, %d\n");
    if (ramAddr == NULL) {
        return NULL;
    }
    entry = &gAudioCtxPersistentSampleCache.entries[*p];
    entry->inUse = 1;
    entry->allocatedAddr = ramAddr;
    entry->size = size;
    (*p)++;
    return entry;
}

// original name: Emem_KillSwMember
// Before a spec change, gives back what the persistent sample cache lent each permanently cached font that
// names a bank, stopping at the first entry of another bank: OoT's AudioHeap_DiscardSampleCacheForFont inlined.
// Load-bearing: `u32 sampleBankId1` beside `s32 sampleBankId2`, the bank read through `entry`, and `drum` and
// `entry` declared seventh and eighth.
void AudioHeap_DiscardSampleCaches(void) {
    Instrument* inst;
    s32 r;
    s32 numFonts;
    s32 fontId;
    s32 i;
    s32 j;
    Drum* drum;
    SampleCacheEntry* entry;
    u32 sampleBankId1;
    s32 sampleBankId2;

    numFonts = gAudioCtx.soundFontTable->header.numEntries;
    for (fontId = 0; fontId < numFonts; fontId++) {
        sampleBankId1 = gAudioCtx.soundFontList[fontId].sampleBankId1;
        sampleBankId2 = gAudioCtx.soundFontList[fontId].sampleBankId2;
        if (sampleBankId1 != 0xFF || sampleBankId2 != 0xFF) {
            PRINTF("Try Kill %d \n");
            PRINTF("Try Kill %x %x\n");
            PRINTF("Try Kill %x %x %x\n");
            if (AudioHeap_SearchCaches(FONT_TABLE, CACHE_PERMANENT, fontId) != 0 &&
                AudioLoad_IsFontLoadComplete(fontId) != 0) {
                for (r = 0; r < gAudioCtx.persistentSampleCache.numEntries; r++) {
                    entry = &gAudioCtx.persistentSampleCache.entries[r];
                    if (sampleBankId1 != entry->sampleBankId && sampleBankId2 != entry->sampleBankId &&
                        entry->sampleBankId != 0) {
                        break;
                    }
                    for (i = 0; i < gAudioCtx.soundFontList[fontId].numInstruments; i++) {
                        inst = Audio_GetInstrumentInner(fontId, i);
                        if (inst != NULL) {
                            if (inst->normalRangeLo != 0) {
                                AudioHeap_UnapplySampleCache(entry, inst->lowPitchTunedSample.sample);
                            }
                            if (inst->normalRangeHi != 0x7F) {
                                AudioHeap_UnapplySampleCache(entry, inst->highPitchTunedSample.sample);
                            }
                            AudioHeap_UnapplySampleCache(entry, inst->normalPitchTunedSample.sample);
                        }
                    }
                    for (j = 0; j < gAudioCtx.soundFontList[fontId].numDrums; j++) {
                        drum = Audio_GetDrum(fontId, j);
                        if (drum != NULL) {
                            AudioHeap_UnapplySampleCache(entry, drum->tunedSample.sample);
                        }
                    }
                }
            }
        }
    }
}

#include "global.h"
#include "audio.h"

// The audio driver's session data, OoT's session_config.c: the work area gAudioCtx, the two const records
// AudioLoad_Init and AudioHeap_Init read, the reverb settings and the five AudioSpecs.
// It builds in na/, without the driver's -use_readwrite_const, so the const records stay in .rodata.

// The work area and its update hook, between two unused arrays that OoT's file has too.
// Load-bearing: both unused arrays.
u8 D_800B4CE0[0x30]; // unused
AudioContext gAudioCtx;
AudioCustomUpdateFunction gAudioCustomUpdateFunction;
s32 D_800BA510[3]; // unused

const TempoData gTempoData = {
    24, // maxNotes
    48, // seqTicksPerBeat
};

const AudioHeapInitSizes gAudioHeapInitSizes = {
    0x40000, // heapSize
    0x14200, // initPoolSize
    0x11200, // permanentPoolSize
};

// The only reverb: every spec below points here with numReverbs 1.
ReverbSettings sReverbSettings[] = {
    { 1, 0x30, 0x2000, 0x7FFF, 0x0000, 0x0000, -1, 0x0000, 0, 0 },
};

// The specs AudioHeap_Init picks from by gAudioCtx.specId: four identical, and spec 3 with 16 notes instead of 24.
AudioSpec gAudioSpecs[] = {
    { 32000, 1, 24, 3, 1, sReverbSettings, 0x7FFF, 0, 0, 0, 0x4000, 0x2000, 0, 0, 0 },
    { 32000, 1, 24, 3, 1, sReverbSettings, 0x7FFF, 0, 0, 0, 0x4000, 0x2000, 0, 0, 0 },
    { 32000, 1, 24, 3, 1, sReverbSettings, 0x7FFF, 0, 0, 0, 0x4000, 0x2000, 0, 0, 0 },
    { 32000, 1, 16, 3, 1, sReverbSettings, 0x7FFF, 0, 0, 0, 0x4000, 0x2000, 0, 0, 0 },
    { 32000, 1, 24, 3, 1, sReverbSettings, 0x7FFF, 0, 0, 0, 0x4000, 0x2000, 0, 0, 0 },
};

// original filename: os.c
#include "global.h"
#include "audio.h"

// NA audio driver, OoT's os.c: the interrupt-masked cache helpers and the driver's own osAiSetNextBuffer.
// A unit of its own: its one .data byte, sHdwrBugFlag, starts 16-aligned past thread.c's strings.

// Cache maintenance with every interrupt masked (OS_IM_NONE); the previous mask is put back
// afterwards.
void Audio_InvalDCache(void* buf, s32 size) {
    OSIntMask prevMask = osSetIntMask(OS_IM_NONE);

    osInvalDCache(buf, size);
    osSetIntMask(prevMask);
}

// The writeback twin of Audio_InvalDCache.
void Audio_WritebackDCache(void* buf, s32 size) {
    OSIntMask prevMask = osSetIntMask(OS_IM_NONE);

    osWritebackDCache(buf, size);
    osSetIntMask(prevMask);
}

// The driver's private copy of libultra's pre-J osAiSetNextBuffer (OoT's name; Audio_ keeps it clear of
// libultra's), with __osAiDeviceBusy open-coded after the 0x2000 work-around; -1 when the AI FIFO is full.
// Load-bearing: sHdwrBugFlag a function-scope static, and the status read into a local.
s32 Audio_AiSetNextBuffer(void* bufPtr, u32 size) {
    static u8 sHdwrBugFlag = FALSE;
    u8* bptr = bufPtr;

    if (sHdwrBugFlag) {
        bptr = (u8*)bufPtr - 0x2000;
    }
    if ((((u32)bufPtr + size) & 0x1FFF) == 0) {
        sHdwrBugFlag = TRUE;
    } else {
        sHdwrBugFlag = FALSE;
    }
    {
        register s32 status = IO_READ(AI_STATUS_REG);

        if (status & AI_STATUS_FIFO_FULL) {
            return -1;
        }
    }
    IO_WRITE(AI_DRAM_ADDR_REG, (u32)bptr + 0x80000000);
    IO_WRITE(AI_LEN_REG, size);
    return 0;
}

// The 28 zero bytes up to nad/playback are SUBALIGN(16) fill and the yaml's 16-byte pad.

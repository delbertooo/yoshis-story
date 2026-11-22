#include "global.h"
#include "audio.h"

// The SE layer's state machine, Nai_SeRoutin, over the 50 slots of gNaiSeBuffer (defined in nai_se_buffer.c).
// Every function name here is INVENTED (no string).
// The PRINTFs are stripped debug prints whose strings stay in .rodata.

// Library reset, the SE twin of Nai_BgmInit: cancels the queued commands of every live slot, then empties the
// buffer and restarts the SE group's sequence.
void Nai_SeInit(void) {
    s16 i;

    for (i = 0; i < 0x32; i++) {
        if (gNaiSeBuffer[i].status != NAI_SE_STATUS_FREE) {
            Nai_PortBufferFreeSe(&gNaiSeBuffer[i]);
        }
    }
    Nai_SeBufferClear();
    Nai_SeGroupStart();
}

// Per frame, for each of the 50 slots: steps its NaiSeStatus, then posts its changes (Nai_SePostChanges skips a
// slot that is not STARTING or PLAYING). The PRINTFs are a stripped dump of the buffer, one tag per slot.
// Load-bearing: the `alloc`/`voice` pair and the `^ 0`.
void Nai_SeRoutin(void) {
    NaiSe* entry;
    s32 alloc;
    s16 voice;
    s16 i;

    PRINTF("NAI: SE Buffer [ ");
    for (i = 0; i < 0x32; i++) {
        entry = &gNaiSeBuffer[i];
        switch (entry->status) {
            case NAI_SE_STATUS_FREE:
                PRINTF("- ");
                break;
            case NAI_SE_STATUS_WAITING:
                PRINTF("W ");
                voice = alloc = Nai_SeSubAlloc(entry);
                if (alloc >= 0) {
                    entry->sub = voice;
                    Nai_SeMuteSub(entry);
                }
                break;
            case NAI_SE_STATUS_QUEUED:
                PRINTF("s ");
                if ((Nai_SeSubIsPlaying(entry->sub) == 0) && (entry->paused != 1)) {
                    Nai_SeSendNote(entry);
                }
                break;
            case NAI_SE_STATUS_STARTING:
                PRINTF("S ");
                if ((Nai_SeSubIsPlaying(entry->sub) ^ 0) == 1) {
                    entry->status = NAI_SE_STATUS_PLAYING;
                }
                break;
            case NAI_SE_STATUS_PLAYING:
                PRINTF("%d ");
                if (Nai_SeSubIsPlaying(entry->sub) == 0) {
                    entry->status = NAI_SE_STATUS_FREE;
                    Nai_PortBufferFreeSe(entry);
                }
                break;
        }
        Nai_SePostChanges(entry);
    }
    PRINTF("]\n");
}

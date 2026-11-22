// original filename: sub_sys.c
#include "global.h"
#include "audio.h"

// NA audio driver, OoT's thread.c: the per-frame update (AudioThread_UpdateImpl), the command ring and its
// dispatch, the spec-change handshake, and the driver's inbound entry points.
// The PRINTFs are stripped debug prints whose strings stay in .data (-use_readwrite_const), in the ROM's order.

// original name: CreateAudioTask
// The driver's per-frame update, called by the game (YS has no AudioThread_Update wrapper): rotates the AI
// buffers, runs the loads, reset and command ring, builds the frame's command list and returns its task.
// Load-bearing: the empty `if`s on aiBuffers[index], aiBufLengths[index], `msg` and `task`, the repeated
// `yield_data_size = 0`, the gAudioCtxSpecUnk4/TotalTaskCount aliases on one line and one `mode == rem + 1` test,
// postfix `resetTimer++`, the top-tested drain `while`, `audioRandom = ... + audioRandom`, `index` assigned
// alone, curAiBufIndex stored twice, `s32 abiCmdCnt`, the statement order and the eleven declarations in order.
AudioTask* AudioThread_UpdateImpl(void) {
    static s32 sMaxAbiCmdCnt = 0x80;
    static AudioTask* sWaitingAudioTask = NULL;
    u32 samplesRemainingInAi;
    s32 abiCmdCnt;
    s16 mode;
    s32 rem;
    s32 dmaCount;
    s16* curAiBuffer;
    s32 index;
    s32 count;
    OSMesg msg;
    s32 validCount;
    AudioTask* task;

    gAudioCtx.totalTaskCount++;
    mode = gAudioCtx.audioBufferParameters.specUnk4;
    rem = gAudioCtx.totalTaskCount % mode;

    if (rem != 0) {
        if (gAudioCustomUpdateFunction != NULL) {
            gAudioCustomUpdateFunction();
            mode = gAudioCtxSpecUnk4, rem = gAudioCtxTotalTaskCount % mode;
        }
        if (mode == rem + 1) {
            return sWaitingAudioTask;
        }
        PRINTF("DAC:Lost 1 Frame.\n");
        return NULL;
    }

    osSendMesg(gAudioCtx.taskStartQueueP, (OSMesg)gAudioCtx.totalTaskCount, OS_MESG_NOBLOCK);

    gAudioCtx.rspTaskIndex ^= 1;
    gAudioCtx.curAiBufIndex++;
    gAudioCtx.curAiBufIndex %= 3;
    index = (gAudioCtx.curAiBufIndex + 1) % 3;

    samplesRemainingInAi = osAiGetLength() / 4;

    if (gAudioCtx.resetTimer < 16) {
        if (gAudioCtx.aiBufLengths[index] != 0) {
            Audio_AiSetNextBuffer(gAudioCtx.aiBuffers[index], gAudioCtx.aiBufLengths[index] * 4);
            if (gAudioCtx.aiBuffers[index]) {
                PRINTF("Address Error %x\n");
            }
            if (gAudioCtx.aiBufLengths[index]) {}
        }
    }

    if (gAudioCustomUpdateFunction != NULL) {
        gAudioCustomUpdateFunction();
    }

    dmaCount = gAudioCtx.curAudioFrameDmaCount;
    if (dmaCount > 0x40) {
        PRINTF("DMA: Request queue over.( %d )\n");
    }
    for (index = 0; index < gAudioCtx.curAudioFrameDmaCount; index++) {
        if (osRecvMesg(&gAudioCtx.curAudioFrameDmaQueue, NULL, OS_MESG_NOBLOCK) == 0) {
            dmaCount--;
        }
    }
    if (dmaCount != 0) {
        PRINTF("DMA [ %d lines] TIMEOUT\n");
        for (index = 0; index < dmaCount; index++) {
            osRecvMesg(&gAudioCtx.curAudioFrameDmaQueue, NULL, OS_MESG_BLOCK);
        }
    }

    validCount = gAudioCtx.curAudioFrameDmaQueue.validCount;
    if (validCount != 0) {
        PRINTF("Warning: WaveDmaQ contains %d msgs.\n");
        for (index = 0; index < validCount; index++) {
            osRecvMesg(&gAudioCtx.curAudioFrameDmaQueue, NULL, OS_MESG_NOBLOCK);
        }
    }

    gAudioCtx.curAudioFrameDmaCount = 0;
    AudioLoad_DecreaseSampleDmaTtls();
    AudioLoad_ProcessLoads(gAudioCtx.resetStatus);

    if (gAudioCtx.resetStatus != 0) {
        if (AudioHeap_ResetStep() == 0) {
            if (gAudioCtx.resetStatus == 0) {
                osSendMesg(gAudioCtx.audioResetQueueP, (OSMesg)(u32)gAudioCtx.specId, OS_MESG_NOBLOCK);
            }
            sWaitingAudioTask = NULL;
            return NULL;
        }
    }

    if (gAudioCtx.resetTimer >= 17) {
        return NULL;
    }

    if (gAudioCtx.resetTimer != 0) {
        gAudioCtx.resetTimer++;
    }

    gAudioCtx.curTask = &gAudioCtx.rspTask[gAudioCtx.rspTaskIndex];
    gAudioCtx.curAbiCmdBuf = gAudioCtx.abiCmdBufs[gAudioCtx.rspTaskIndex];
    index = gAudioCtx.curAiBufIndex;
    curAiBuffer = gAudioCtx.aiBuffers[index];
    gAudioCtx.aiBufLengths[index] =
        (((gAudioCtx.audioBufferParameters.samplesPerFrameTarget - samplesRemainingInAi) + 128) & ~0xF) + 16;

    if (gAudioCtx.aiBufLengths[index] < gAudioCtx.audioBufferParameters.minAiBufferLength) {
        gAudioCtx.aiBufLengths[index] = gAudioCtx.audioBufferParameters.minAiBufferLength;
    }
    if (gAudioCtx.audioBufferParameters.maxAiBufferLength < gAudioCtx.aiBufLengths[index]) {
        gAudioCtx.aiBufLengths[index] = gAudioCtx.audioBufferParameters.maxAiBufferLength;
    }

    count = 0;
    if (gAudioCtx.resetStatus == 0) {
        while (osRecvMesg(gAudioCtx.threadCmdProcQueueP, &msg, OS_MESG_NOBLOCK) != -1) {
            AudioThread_ProcessCmds((u32)msg);
            count++;
            if (msg) {}
        }
        if (count == 0) {
            if (gAudioCtx.threadCmdQueueFinished != 0) {
                AudioThread_ScheduleProcessCmds();
            }
        }
    }

    gAudioCtx.curAbiCmdBuf = (s32)AudioSynth_Update((Acmd*)gAudioCtx.curAbiCmdBuf, &abiCmdCnt, (s32*)curAiBuffer,
                                                    gAudioCtx.aiBufLengths[index]);

    gAudioCtx.audioRandom = osGetCount() * (gAudioCtx.audioRandom + gAudioCtx.totalTaskCount);
    gAudioCtx.audioRandom = gAudioCtx.aiBuffers[index][gAudioCtx.totalTaskCount & 0xFF] + gAudioCtx.audioRandom;
    gWaveSamples[8] = (s16*)((gAudioCtx.audioRandom & 0xFFF0) + (u32)&AudioThread_UpdateImpl);
    index = gAudioCtx.rspTaskIndex;

    gAudioCtx.curTask->msgQueue = NULL;
    gAudioCtx.curTask->unk44 = NULL;
    task = gAudioCtx.curTask;
    task->task.t.type = 2;
    task->task.t.flags = 0;
    task->task.t.ucode_boot = (u64*)aspMainTextStart;
    task->task.t.ucode_boot_size = 0x1000;
    task->task.t.ucode_data_size = ((((s32)&gSoundFontTable - (s32)&aspMainDataStart) >> 3) << 3) - 1;
    task->task.t.ucode = (u64*)aspMainTextStart;
    task->task.t.ucode_data = (u64*)aspMainDataStart;
    task->task.t.ucode_size = 0x1000;
    task->task.t.dram_stack = NULL;
    task->task.t.dram_stack_size = 0;
    task->task.t.output_buff = NULL;
    task->task.t.output_buff_size = NULL;
    task->task.t.data_ptr = (u64*)gAudioCtx.abiCmdBufs[index];
    task->task.t.data_size = abiCmdCnt << 3;
    task->task.t.yield_data_ptr = NULL;
    task->task.t.yield_data_size = 0;

    if (sMaxAbiCmdCnt < abiCmdCnt) {
        task->task.t.yield_data_size = 0;
        sMaxAbiCmdCnt = abiCmdCnt;
        PRINTF("Audio:now-max tasklen is %d / %d\n");
    }

    if (task) {
        PRINTF("Audio:Warning:ABI Tasklist length over (%d)\n");
    }
    if (gAudioCtx.audioBufferParameters.specUnk4 == 1) {
        return gAudioCtx.curTask;
    } else {
        sWaitingAudioTask = gAudioCtx.curTask;
        return NULL;
    }
}

// original name: Nap_AudioSysProcess
// Runs one global command (OoT's AUDIOCMD_OP_GLOBAL_*): seq player starts and stops, mute, loads, the sound
// mode, the channel mask, a spec change and the custom update hook; any other op is dropped.
// Load-bearing: `sampleState` for the unmute's `finished` store.
void AudioThread_ProcessGlobalCmd(AudioCmd* cmd) {
    s32 i;
    Note* note;
    NoteSampleState* sampleState;

    switch (cmd->op) {
        case 0x81:
            AudioLoad_SyncLoadSeqParts(cmd->arg1, 3);
            break;
        case 0x82:
        case 0x88:
            AudioLoad_SyncInitSeqPlayer(cmd->arg0, cmd->arg1, cmd->arg2);
            AudioThread_SetFadeInTimer(cmd->arg0, cmd->payload.asInt);
            break;
        case 0x85:
            AudioLoad_SyncInitSeqPlayerSkipTicks(cmd->arg0, cmd->arg1, cmd->payload.asInt);
            break;
        // An enabled seq player stops at once for a payload of 0, else fades out over that many ticks.
        case 0x83:
            if (gAudioCtx.seqPlayers[cmd->arg0].enabled != 0) {
                if (cmd->payload.asInt == 0) {
                    AudioSeq_SequencePlayerDisable(&gAudioCtx.seqPlayers[cmd->arg0]);
                } else {
                    AudioThread_SetFadeOutTimer(cmd->arg0, cmd->payload.asInt);
                }
            }
            break;
        case 0xF0:
            gAudioCtx.soundOutputMode = cmd->payload.asInt;
            break;
        // Mute every seq player.
        case 0xF1:
            for (i = 0; i < gAudioCtx.audioBufferParameters.numSequencePlayers; i++) {
                gAudioCtx.seqPlayers[i].muted = 1;
                gAudioCtx.seqPlayers[i].recalculateVolume = 1;
            }
            break;
        // Unmute every seq player; payload 1 first finishes each sounding note whose channel has MUTE_BEHAVIOR_3.
        case 0xF2:
            if (cmd->payload.asInt == 1) {
                for (i = 0; i < gAudioCtx.numNotes; i++) {
                    note = &gAudioCtx.notes[i];
                    sampleState = &note->sampleState;
                    if (sampleState->h.f.enabled != 0 && note->playbackState.status == 0 &&
                        (note->playbackState.parentLayer->channel->h.b.muteBehavior & 8)) {
                        sampleState->h.f.finished = 1;
                    }
                }
            }
            for (i = 0; i < gAudioCtx.audioBufferParameters.numSequencePlayers; i++) {
                gAudioCtx.seqPlayers[i].muted = 0;
                gAudioCtx.seqPlayers[i].recalculateVolume = 1;
            }
            break;
        case 0xF3:
            AudioLoad_SyncLoadInstrument(cmd->arg0, cmd->arg1, cmd->arg2);
            break;
        case 0xF4:
            PRINTF("BGLOAD Start %d\n");
            AudioLoad_AsyncLoadSampleBank(cmd->arg0, cmd->arg1, cmd->arg2, &gAudioCtx.externalLoadQueue);
            break;
        case 0xF5:
            AudioLoad_AsyncLoadSeqFonts(cmd->arg0, cmd->arg1, cmd->arg2, &gAudioCtx.externalLoadQueue);
            break;
        case 0xF6:
            AudioLoad_DiscardSeqFonts(cmd->arg1);
            break;
        case 0x90:
            gAudioCtx.threadCmdChannelMask[cmd->arg0] = cmd->payload.asUShort;
            break;
        case 0xF9:
            gAudioCtx.resetStatus = 5;
            gAudioCtx.specId = cmd->payload.asInt;
            break;
        case 0xFB:
            gAudioCustomUpdateFunction = (AudioCustomUpdateFunction)cmd->payload.asInt;
            break;
    }
}

// original name: __Nas_GroupFadeOut
// Starts a fade-out from the player's current fade volume: `fadeTimer` steps of -volume/fadeTimer,
// SEQPLAYER_STATE_FADE_OUT. A zero timer is treated as one, so the volume drops in a single step.
void AudioThread_SetFadeOutTimer(s32 seqPlayerIndex, s32 fadeTimer) {
    SequencePlayer* seqPlayer = &gAudioCtx.seqPlayers[seqPlayerIndex];

    if (fadeTimer == 0) {
        fadeTimer = 1;
    }
    seqPlayer->state = SEQPLAYER_STATE_FADE_OUT;
    seqPlayer->fadeTimer = fadeTimer;
    seqPlayer->fadeVelocity = -(seqPlayer->fadeVolume / (f32)fadeTimer);
}

// original name: __Nas_GroupFadeIn
// Starts a fade-in over `fadeTimer` ticks from silence: SEQPLAYER_STATE_FADE_IN, both timers set,
// fade volume and velocity 0. A zero timer does nothing.
void AudioThread_SetFadeInTimer(s32 seqPlayerIndex, s32 fadeTimer) {
    if (fadeTimer != 0) {
        gAudioCtx.seqPlayers[seqPlayerIndex].state = SEQPLAYER_STATE_FADE_IN;
        gAudioCtx.seqPlayers[seqPlayerIndex].fadeTimerUnkEu = fadeTimer;
        gAudioCtx.seqPlayers[seqPlayerIndex].fadeTimer = fadeTimer;
        gAudioCtx.seqPlayers[seqPlayerIndex].fadeVolume = 0.0f;
        gAudioCtx.seqPlayers[seqPlayerIndex].fadeVelocity = 0.0f;
    }
}

// original name: Nap_AudioPortInit
// Creates the driver's three message queues (task start 1, command processing 4, audio reset 1) and empties
// the command ring; the prints were two OSMesgQueue dumps.
// Load-bearing: the first osCreateMesgQueue reads taskStartQueueP back instead of `&gAudioCtx.taskStartQueue`.
void AudioThread_InitMesgQueuesImpl(void) {
    gAudioCtx.threadCmdWritePos = 0;
    gAudioCtx.threadCmdReadPos = 0;
    gAudioCtx.threadCmdQueueFinished = 0;
    gAudioCtx.taskStartQueueP = &gAudioCtx.taskStartQueue;
    gAudioCtx.threadCmdProcQueueP = &gAudioCtx.threadCmdProcQueue;
    gAudioCtx.audioResetQueueP = &gAudioCtx.audioResetQueue;
    osCreateMesgQueue(gAudioCtx.taskStartQueueP, gAudioCtx.taskStartMsgBuf, 1);
    osCreateMesgQueue(gAudioCtx.threadCmdProcQueueP, gAudioCtx.threadCmdProcMsgBuf, 4);
    osCreateMesgQueue(gAudioCtx.audioResetQueueP, gAudioCtx.audioResetMsgBuf, 1);
    PRINTF("Init FIN-QUEUE! %x \n");
    PRINTF("QueuAdd %x\n");
    PRINTF("mtqueue %x\n");
    PRINTF("fullqueue %x\n");
    PRINTF("validc %x\n");
    PRINTF("first  %x\n");
    PRINTF("count  %x\n");
    PRINTF("Mesgs  %x\n");
    PRINTF("------------------\n");
    PRINTF("Init FIN-QUEUE!\n");
    PRINTF("QueuAdd %x\n");
    PRINTF("mtqueue %x\n");
    PRINTF("fullqueue %x\n");
    PRINTF("validc %x\n");
    PRINTF("first  %x\n");
    PRINTF("count  %x\n");
    PRINTF("Mesgs  %x\n");
    PRINTF("------------------\n");
}

// original name: Nap_PortSet
// Pushes one (command, payload) pair onto the command ring; a push that would reach the read index is undone,
// so a full ring drops the newest command.
void AudioThread_QueueCmd(u32 opArgs, void* data) {
    u32* slot = gAudioCtx.threadCmdBuf[gAudioCtx.threadCmdWritePos & 0xFF];

    slot[0] = opArgs;
    slot[1] = *(u32*)data;
    gAudioCtx.threadCmdWritePos++;
    if (gAudioCtx.threadCmdWritePos == gAudioCtx.threadCmdReadPos) {
        PRINTF("Error: OverFlow Your Request\n");
        gAudioCtx.threadCmdWritePos--;
    }
}

// original name: Nap_SetF32
// The typed pushes (F32, S32, S8, U16): each passes AudioThread_QueueCmd the address of a stack copy of its
// payload, S8 and U16 shifted to the word's top.
void AudioThread_QueueCmdF32(u32 opArgs, f32 data) {
    AudioThread_QueueCmd(opArgs, &data);
}

// original name: Nap_SetS32
void AudioThread_QueueCmdS32(u32 opArgs, s32 data) {
    AudioThread_QueueCmd(opArgs, &data);
}

// original name: Nap_SetS8
void AudioThread_QueueCmdS8(u32 opArgs, s8 data) {
    u32 uData = data << 24;

    AudioThread_QueueCmd(opArgs, &uData);
}

// original name: Nap_SetU16
void AudioThread_QueueCmdU16(u32 opArgs, u16 data) {
    u32 uData = data << 16;

    AudioThread_QueueCmd(opArgs, &uData);
}

// original name: Nap_SendStart
// Posts the pending ring window to the audio thread (read index in the high byte, write index in the low) and,
// once it is queued, advances the read index; sMaxWriteReadDiff keeps the deepest backlog.
void AudioThread_ScheduleProcessCmds(void) {
    // MM's name; OoT: D_801304E8.
    static s32 sMaxWriteReadDiff = 0;
    s32 write = gAudioCtx.threadCmdWritePos;
    s32 read = gAudioCtx.threadCmdReadPos;
    s32 pending = (write - read + 0x100) & 0xFF;

    PRINTF("---AudioSending (%d->%d) \n");
    if (sMaxWriteReadDiff < pending) {
        sMaxWriteReadDiff = pending;
    }
    PRINTF("AudioSend: %d -> %d (%d)\n");
    if (osSendMesg(gAudioCtx.threadCmdProcQueueP, (OSMesg)(((read & 0xFF) << 8) | (write & 0xFF)), OS_MESG_NOBLOCK) !=
        -1) {
        gAudioCtx.threadCmdReadPos = gAudioCtx.threadCmdWritePos;
    } else {
        PRINTF("Warning: MesgQ is Full, Retry Next Send.\n");
    }
    PRINTF("Next Start %d \n");
}

// original name: Nap_FlushPort
// Discards every pending command by pulling the read index up to the write index, and clears
// threadCmdQueueFinished.
void AudioThread_ResetCmdQueue(void) {
    gAudioCtx.threadCmdReadPos = gAudioCtx.threadCmdWritePos;
    gAudioCtx.threadCmdQueueFinished = 0;
}

// AudioThread_ProcessChannelCmd's view of a SequenceChannel: the flag byte as u8 bitfields (audio.h has a
// u32 union) and only the fields it writes; `changes` holds the 0x20 pan, 0x40 volume and 0x80 freqScale bits.
typedef struct SequenceChannelCmdView {
    /* 0x00 */ u8 : 3;       // enabled, finished, stopScript
    /* 0x00 */ u8 muted : 1; // mask 0x10 of byte 0
    /* 0x00 */ u8 : 4;       // hasInstrument, stereoHeadsetEffects, largeNotes, unused
    /* 0x01 */ u8 changes;
    /* 0x02 */ char pad2[0x1];
    /* 0x03 */ s8 muteBehavior;
    /* 0x04 */ u8 targetReverbVol;
    /* 0x05 */ char pad5[0x5];
    /* 0x0A */ u8 newPan;
    /* 0x0B */ char padB[0x9];
    /* 0x14 */ u16 vibratoRateTarget;
    /* 0x16 */ u16 vibratoDepthTarget;
    /* 0x18 */ u16 vibratoRateChangeDelay;
    /* 0x1A */ u16 vibratoDepthChangeDelay;
    /* 0x1C */ char pad1C[0xC];
    /* 0x28 */ f32 volumeScale;
    /* 0x2C */ f32 volume;
    /* 0x30 */ char pad30[0x8];
    /* 0x38 */ f32 freqScale;
    /* 0x3C */ char pad3C[0x90];
    /* 0xCC */ s8 seqScriptIO[8];
} SequenceChannelCmdView; // size >= 0xD4

void AudioThread_ProcessChannelCmd(SequenceChannelCmdView* channel, AudioCmd* cmd);

// original name: Nap_AudioPortProcess
// Runs the commands in the ring window `msg` names (OoT's ProcessCmd and ProcessSeqPlayerCmd inlined): global,
// seq player (0x41..0x4C) and channel commands; 0xF8 pauses the drain, and sCurCmdRdPos resumes it.
// Load-bearing: `if (seqPlayer) {}` after its definition, the `for (;;)` whose leading test stores and returns,
// `pCur` with a non-volatile sCurCmdRdPos, `cmd->arg1` spelled at both uses, `(msg >> 8) & 0xFF` and
// `(u8)(msg & 0xFF)`, the ring as `threadCmdBuf[0x100][2]`, `(s32)` on both float conversions, and the u16
// channel mask shifted as `((s32)mask >> 1) & 0xFFFF`.
void AudioThread_ProcessCmds(u32 msg) {
    static u8 sCurCmdRdPos = 0;
    AudioCmd* cmd;
    SequencePlayer* seqPlayer;
    s32 op;
    s32 seqPlayerIndex;
    s32 channelIndex;
    u16 threadCmdChannelMask;
    f32 fadeVolume;
    f32 cur;
    s32 fadeTimer;
    s32 endPos;
    u8* pCur;

    pCur = &sCurCmdRdPos;
    if (gAudioCtx.threadCmdQueueFinished == 0) {
        *pCur = (msg >> 8) & 0xFF;
    } else {
        PRINTF("Continue Port\n");
    }
    endPos = (u8)(msg & 0xFF);
    PRINTF("%d -> %d\n");

    for (;;) {
        if (endPos == *pCur) {
            gAudioCtx.threadCmdQueueFinished = 0;
            return;
        }
        cmd = (AudioCmd*)&gAudioCtx.threadCmdBuf[*pCur & 0xFF];
        (*pCur)++;
        op = cmd->op;
        if (op == 0xF8) {
            gAudioCtx.threadCmdQueueFinished = 1;
            PRINTF("Sync-Frame  Break. (Remain %d [%d,%d])\n");
            return;
        }
        if ((op & 0xF0) == 0xF0) {
            AudioThread_ProcessGlobalCmd(cmd);
        } else {
            seqPlayerIndex = cmd->arg0;
            if (seqPlayerIndex < gAudioCtx.audioBufferParameters.numSequencePlayers) {
                seqPlayer = &gAudioCtx.seqPlayers[seqPlayerIndex];
                if (seqPlayer) {}
                if (op & 0x80) {
                    AudioThread_ProcessGlobalCmd(cmd);
                } else if (op & 0x40) {
                    switch (op) {
                        case 0x41:
                            if (seqPlayer->fadeVolumeScale != cmd->payload.asFloat) {
                                seqPlayer->fadeVolumeScale = cmd->payload.asFloat;
                                seqPlayer->recalculateVolume = 1;
                            }
                            break;
                        case 0x47:
                            seqPlayer->tempo = cmd->payload.asInt * 0x30;
                            break;
                        case 0x49:
                            seqPlayer->tempoChange = cmd->payload.asInt * 0x30;
                            break;
                        case 0x48:
                            seqPlayer->transposition = cmd->payload.asSbyte;
                            break;
                        case 0x46:
                            seqPlayer->seqScriptIO[cmd->arg2] = cmd->payload.asSbyte;
                            break;
                        case 0x4A:
                            fadeVolume = (s32)cmd->arg1 / 127.0f;
                            goto ramp;
                        case 0x4B:
                            fadeVolume = ((s32)cmd->arg1 / 100.0f) * seqPlayer->fadeVolume;
                        ramp:
                            if (seqPlayer->state != SEQPLAYER_STATE_FADE_OUT) {
                                cur = seqPlayer->fadeVolume;
                                seqPlayer->volume = cur;
                                fadeTimer = cmd->payload.asInt;
                                if (fadeTimer == 0) {
                                    seqPlayer->fadeVolume = fadeVolume;
                                } else {
                                    seqPlayer->state = SEQPLAYER_STATE_0;
                                    seqPlayer->fadeTimer = fadeTimer;
                                    seqPlayer->fadeVelocity = (fadeVolume - cur) / (f32)fadeTimer;
                                }
                            }
                            break;
                        case 0x4C:
                            if (seqPlayer->state != SEQPLAYER_STATE_FADE_OUT) {
                                fadeTimer = cmd->payload.asInt;
                                if (fadeTimer == 0) {
                                    seqPlayer->fadeVolume = seqPlayer->volume;
                                } else {
                                    seqPlayer->state = SEQPLAYER_STATE_0;
                                    seqPlayer->fadeTimer = fadeTimer;
                                    seqPlayer->fadeVelocity =
                                        (seqPlayer->volume - seqPlayer->fadeVolume) / (f32)fadeTimer;
                                }
                            }
                            break;
                    }
                } else if (seqPlayer->enabled != 0) {
                    if (cmd->arg1 < 0x10) {
                        AudioThread_ProcessChannelCmd((SequenceChannelCmdView*)seqPlayer->channels[cmd->arg1], cmd);
                    } else if (cmd->arg1 == 0xFF) {
                        threadCmdChannelMask = gAudioCtx.threadCmdChannelMask[seqPlayerIndex];
                        for (channelIndex = 0; channelIndex < 0x10; channelIndex++) {
                            if (threadCmdChannelMask & 1) {
                                AudioThread_ProcessChannelCmd(
                                    (SequenceChannelCmdView*)seqPlayer->channels[channelIndex], cmd);
                            }
                            threadCmdChannelMask = ((s32)threadCmdChannelMask >> 1) & 0xFFFF;
                        }
                    }
                }
            }
        }
        cmd->op = 0;
    }
}

// Polls the external load queue without blocking: returns the message's top byte and stores its low 24 bits
// in `*out` (0 and 0 when it is empty). No callers; OoT names it by its address.
s32 AudioThread_GetExternalLoadQueueMsg(u32* out) {
    OSMesg msg;

    if (osRecvMesg(&gAudioCtx.externalLoadQueue, &msg, OS_MESG_NOBLOCK) == -1) {
        *out = 0;
        return 0;
    }
    *out = (u32)msg & 0xFFFFFF;
    return (u32)msg >> 24;
}

// A wrapper for AudioLoad_GetFontsForSequence: the fonts a sequence uses, and how many.
u8* AudioThread_GetFontsForSequence(s32 seqId, s32* outNumFonts) {
    return AudioLoad_GetFontsForSequence(seqId, outNumFonts);
}

// original name: Nap_CheckSpecChange
// Polls the spec-change queue without blocking: 1 when the acknowledgement names gAudioCtx.specId, else 0
// (OoT returns -1 for a mismatch). OoT names it by its address.
// Load-bearing: the unused local declared ahead of `msg`.
s32 AudioThread_CheckSpecChange(void) {
    UNUSED s32 unused;
    OSMesg msg;

    if (osRecvMesg(gAudioCtx.audioResetQueueP, &msg, OS_MESG_NOBLOCK) == -1) {
        return 0;
    }
    if ((u32)msg != gAudioCtx.specId) {
        PRINTF("specchg conjunction error (Msg:%d Cur:%d)\n");
        return 0;
    }
    return 1;
}

// original name: __ClearSpecChangeQ
// Drains the spec-change queue without blocking, so a later wait cannot pick up a stale acknowledgement. OoT
// names it by its address.
// Load-bearing: `empty` declared ahead of `msg` and assigned inside the loop body.
void AudioThread_ClearSpecChangeQueue(void) {
    s32 empty;
    OSMesg msg;

    do {
        empty = -1;
        PRINTF("Warning : Queue is not empty,drop this (number %d ) \n");
    } while (osRecvMesg(gAudioCtx.audioResetQueueP, &msg, OS_MESG_NOBLOCK) != empty);
}

// original name: Nap_StartSpecChange
// Requests an audio heap reset into `specId` (global command 0xF9). During a running reset it flushes the ring,
// then gives up for the same spec or past status 2 (OoT stores specId there) or waits for that reset's ack.
s32 AudioThread_ResetAudioHeap(s32 specId) {
    s32 resetStatus;
    OSMesg msg;

    AudioThread_ClearSpecChangeQueue();
    resetStatus = gAudioCtx.resetStatus;
    if (resetStatus != 0) {
        PRINTF("Warning: SpecChange is processing now[number %d]\n");
        AudioThread_ResetCmdQueue();
        if (specId == gAudioCtx.specId) {
            PRINTF("Same SpecChange...  Cancel.\n");
            return 0;
        }
        if (resetStatus >= 3) {
            PRINTF("Overwrite SpecNumber.[->%d]\n");
            return 0;
        }
        PRINTF("Coherency safety wait..\n");
        osRecvMesg(gAudioCtx.audioResetQueueP, &msg, OS_MESG_BLOCK);
    }
    PRINTF("Normal SpecChange [num %d]\n");
    AudioThread_ClearSpecChangeQueue();
    AudioThread_QueueCmdS32(0xF9000000, specId);
    AudioThread_ScheduleProcessCmds();
}

// original name: Nap_StartReset
// Before a reset (PreNMI): stops the audio task after 16 more frames (resetTimer) and requests spec 0. No callers.
// Load-bearing: resetTimer and resetStatus `volatile` (audio.h).
void AudioThread_PreNMIInternal(void) {
    gAudioCtx.resetTimer = 1;
    AudioThread_ResetAudioHeap(0);
    gAudioCtx.resetStatus = 0;
}

// original name: Nap_ReadSubPort
// Reads one of a channel's script IO ports (seqScriptIO), or SEQ_IO_VAL_NONE when the seq player is not
// enabled. Na_GetYoshiStepStatus reads channel 15's port 0 through it.
s32 AudioThread_GetChannelIO(s32 seqPlayerIndex, s32 channelIndex, s32 ioPort) {
    SequencePlayer* seqPlayer = &gAudioCtx.seqPlayers[seqPlayerIndex];
    SequenceChannel* channel;

    if (seqPlayer->enabled != 0) {
        channel = seqPlayer->channels[channelIndex];
        return channel->seqScriptIO[ioPort];
    }
    return SEQ_IO_VAL_NONE;
}

// original name: Nap_ReadGrpPort
// Reads one of a seq player's script IO ports (seqScriptIO).
s8 AudioThread_GetSeqPlayerIO(s32 seqPlayerIndex, s32 ioPort) {
    return gAudioCtx.seqPlayers[seqPlayerIndex].seqScriptIO[ioPort];
}

// Gives the external pool (gAudioCtx.externalPool) a caller-supplied block. No callers.
void AudioThread_InitExternalPool(u8* ramAddr, s32 size) {
    AudioHeap_InitPool(&gAudioCtx.externalPool, ramAddr, size);
}

// Marks the external pool unusable: a zero start is what the allocators test. No callers.
void AudioThread_ResetExternalPool(void) {
    gAudioCtx.externalPool.startRamAddr = NULL;
}

// original name: __SetSubParam
// Applies one channel command (OoT's AUDIOCMD_OP_CHANNEL_*): 1-5 volume scale, volume, pan, freqScale and reverb
// volume (on a change only), 6 script IO, 8 mute, 9 mute behavior, 10/11 vibrato depth/rate targets.
// Load-bearing: one redundant `& 0xFF` per change-bit read, and cases 10 and 11 storing the value first.
void AudioThread_ProcessChannelCmd(SequenceChannelCmdView* channel, AudioCmd* cmd) {
    switch (cmd->op) {
        case 1:
            if (channel->volumeScale != cmd->payload.asFloat) {
                channel->volumeScale = cmd->payload.asFloat;
                channel->changes = (channel->changes & 0xFF) | 0x40;
            }
            break;
        case 2:
            if (channel->volume != cmd->payload.asFloat) {
                channel->volume = cmd->payload.asFloat;
                channel->changes = (channel->changes & 0xFF) | 0x40;
            }
            break;
        case 3:
            if (channel->newPan != cmd->payload.asSbyte) {
                channel->newPan = cmd->payload.asSbyte;
                channel->changes = (channel->changes & 0xFF) | 0x20;
            }
            break;
        case 4:
            if (channel->freqScale != cmd->payload.asFloat) {
                channel->freqScale = cmd->payload.asFloat;
                channel->changes = (channel->changes & 0xFF) | 0x80;
            }
            break;
        case 5:
            if (channel->targetReverbVol != cmd->payload.asSbyte) {
                channel->targetReverbVol = cmd->payload.asSbyte;
            }
            break;
        case 6:
            if (cmd->arg2 < 8) {
                channel->seqScriptIO[cmd->arg2] = cmd->payload.asSbyte;
            }
            break;
        case 7:
            break;
        case 8:
            channel->muted = cmd->payload.asSbyte;
            break;
        case 9:
            channel->muteBehavior = cmd->payload.asSbyte;
            break;
        case 10:
            channel->vibratoDepthTarget = cmd->payload.asUbyte << 3;
            channel->vibratoDepthChangeDelay = 1;
            break;
        case 11:
            channel->vibratoRateTarget = cmd->payload.asUbyte << 5;
            channel->vibratoRateChangeDelay = 1;
            break;
        default:
            PRINTF("Undefined Port Command %d\n");
            break;
    }
}

// Posts global command 0xFA (OoT's AUDIOCMD_OP_GLOBAL_NOOP_1) with three argument bytes and data 1, which
// AudioThread_ProcessGlobalCmd ignores. No callers; OoT's Noop2Cmd (0xFD) does not exist here.
void AudioThread_Noop1Cmd(s32 arg0, s32 arg1, s32 arg2) {
    AudioThread_QueueCmdS32(0xFA000000 | ((arg0 & 0xFF) << 16) | ((arg1 & 0xFF) << 8) | (arg2 & 0xFF), 1);
}

// The same command with zero arguments and data 0. No callers.
void AudioThread_Noop1CmdZeroed(void) {
    AudioThread_QueueCmdS32(0xFA000000, 0);
}

// Set and clear gAudioCustomUpdateFunction, which AudioThread_UpdateImpl calls each frame (global command 0xFB
// sets it through the ring). YS-only, so the names are invented in OoT's style; no callers.
void AudioThread_SetCustomUpdateFunction(AudioCustomUpdateFunction functionPtr) {
    gAudioCustomUpdateFunction = functionPtr;
}

void AudioThread_ClearCustomUpdateFunction(void) {
    gAudioCustomUpdateFunction = NULL;
}

// original name: Nas_InitGAudio
// Library-facing wrapper around the message-queue setup.
void AudioThread_InitMesgQueues(void) {
    AudioThread_InitMesgQueuesImpl();
}

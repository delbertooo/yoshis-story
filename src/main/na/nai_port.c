#include "global.h"
#include "audio.h"

// The command pool's dispatcher, Nai_PortRoutin: once a frame it turns each queued NaiPort into driver commands and
// frees the entries that finished; the rest, ramps among them, stay queued for the next frame.
// Every function name here but Nai_PortRoutin (from its exception print's "PortRoutin") is INVENTED (no string).
// The PRINTFs are stripped debug prints whose strings stay in .rodata.

// The spec-change gate, raised from SPEC CHANGE START until the driver acknowledges the new spec: meanwhile
// Nai_PortRoutin holds every other command and Nai_Routin schedules none.
u8 gNaiSpecChange = 0;

// Drops every queued entry and lowers the spec-change gate.
void Nai_PortInit(void) {
    Nai_PortBufferClear();
    gNaiSpecChange = 0;
}

// Per frame, walks the pool's list in post order and sends each entry's command, held while the spec-change gate is
// up, then frees the entry if it finished. The PRINTFs are a stripped dump, one line per entry.
// Load-bearing: every switch's case order, OUT_MODE's per-mode calls, the gAudioCtxSeqPlayers alias, the print-only
// arms' early `break`, `u8 finished` and `entry = entry->next` inside the `inUse` test.
void Nai_PortRoutin(void) {
    NaiPort* entry;
    u8 finished;
    s16 i;

    entry = Nai_PortBufferGetHead();
    while (entry != NULL) {
        if (entry->inUse != 0) {
            finished = 0;
            PRINTF("NAI: PORT[%x] ");
            switch (entry->type) {
                case NAI_PORT_TYPE_SEQ_START:
                    if (gNaiSpecChange == 0) {
                        PRINTF("SEQ START ");
                        PRINTF("[ group: %d num: %d time: %d ]");
                        AudioThread_QueueCmdS32(0x82000000 | ((entry->group & 0xFF) << 16) | ((entry->num & 0xFF) << 8),
                                                entry->time);
                        finished = 1;
                    }
                    break;
                case NAI_PORT_TYPE_SEQ_STOP:
                    if (gNaiSpecChange == 0) {
                        PRINTF("SEQ STOP ");
                        PRINTF("[ group: %d time: %d ]");
                        AudioThread_QueueCmdS32(0x83000000 | ((entry->group & 0xFF) << 16), entry->time);
                        finished = 1;
                    }
                    break;
                case NAI_PORT_TYPE_GRP_PORT_WRITE:
                    if (gNaiSpecChange == 0) {
                        PRINTF("GRP PORT WRITE ");
                        PRINTF("[ group: %d num: %d value: %f ]");
                        finished = Nai_PortRampStep(entry);
                        AudioThread_QueueCmdS8(0x46000000 | ((entry->group & 0xFF) << 16) | (entry->num & 0xFF),
                                               entry->value);
                    }
                    break;
                case NAI_PORT_TYPE_SUB_PORT_WRITE:
                    if (gNaiSpecChange == 0) {
                        PRINTF("SUB PORT WRITE ");
                        if (gAudioCtxSeqPlayers[entry->group].enabled) {
                            if (gAudioCtx.seqPlayers[entry->group].channels[entry->sub]->h.f.enabled) {
                                PRINTF("[ group: %d sub: %d num: %d value: %f ]");
                                finished = Nai_PortRampStep(entry);
                                AudioThread_QueueCmdS8(0x06000000 | ((entry->group & 0xFF) << 16) |
                                                           ((entry->sub & 0xFF) << 8) | (entry->num & 0xFF),
                                                       entry->value);
                            }
                        }
                    }
                    break;
                case NAI_PORT_TYPE_GRP_EVOLUME:
                    if (gNaiSpecChange == 0) {
                        PRINTF("GRP_EVOLUME ");
                        if (entry->ownerType == 1) {
                            if (Nai_BgmIsActive(entry->bgm) != 0) {
                                PRINTF("[ group: %d volume: %f ] ");
                                finished = 1;
                                AudioThread_QueueCmdF32(0x41000000 | ((entry->bgm->group & 0xFF) << 16),
                                                        entry->rampValue);
                            }
                        }
                    }
                    break;
                case NAI_PORT_TYPE_GRP_TEMPO:
                    if (gNaiSpecChange != 0) {
                        break;
                    }
                    PRINTF("GRP_TEMPO ");
                    break;
                case NAI_PORT_TYPE_GRP_TEMPO_OFFSET:
                    if (gNaiSpecChange == 0) {
                        PRINTF("GRP_TEMPO_OFFSET ");
                        if (entry->ownerType == 1) {
                            if (Nai_BgmIsActive(entry->bgm) != 0) {
                                PRINTF("[ group: %d tempo offset: %f ] ");
                                finished = 1;
                                AudioThread_QueueCmdS32(0x49000000 | ((entry->bgm->group & 0xFF) << 16),
                                                        entry->rampValue);
                            }
                        }
                    }
                    break;
                case NAI_PORT_TYPE_GRP_TRANS:
                    if (gNaiSpecChange != 0) {
                        break;
                    }
                    PRINTF("GRP_TRANS ");
                    break;
                case NAI_PORT_TYPE_SUB_EVOLUME:
                    if (gNaiSpecChange == 0) {
                        PRINTF("SUB_EVOLUME ");
                        switch (entry->ownerType) {
                            case 1:
                                if (Nai_BgmIsActive(entry->bgm) != 0) {
                                    PRINTF("[ group: %d sub: %d volume: %f ] ");
                                    finished = Nai_PortRampStep(entry);
                                    AudioThread_QueueCmdF32(0x01000000 | ((entry->bgm->group & 0xFF) << 16) |
                                                                ((entry->sub & 0xFF) << 8),
                                                            entry->value);
                                    entry->bgm->subVolume[entry->sub] = entry->value;
                                }
                                break;
                            case 2:
                                if (Nai_SeIsActive(entry->se) != 0) {
                                    PRINTF("[ group: %d sub: %d volume: %f ] ");
                                    finished = 1;
                                    AudioThread_QueueCmdF32(0x01000000 | ((entry->group & 0xFF) << 16) |
                                                                ((entry->se->sub & 0xFF) << 8),
                                                            entry->rampValue);
                                }
                                break;
                        }
                    }
                    break;
                case NAI_PORT_TYPE_SUB_VIB:
                    if (gNaiSpecChange != 0) {
                        break;
                    }
                    PRINTF("SUB_VIB ");
                    break;
                case NAI_PORT_TYPE_SUB_TUNE:
                    if (gNaiSpecChange == 0) {
                        PRINTF("SUB TUNE ");
                        switch (entry->ownerType) {
                            case 2:
                                if (Nai_SeIsActive(entry->se) != 0) {
                                    PRINTF("[ group: %d sub: %d tune: %f ] ");
                                    finished = 1;
                                    AudioThread_QueueCmdF32(0x04000000 | ((entry->group & 0xFF) << 16) |
                                                                ((entry->se->sub & 0xFF) << 8),
                                                            entry->rampValue);
                                }
                                break;
                            case 1:
                                if (Nai_BgmIsActive(entry->bgm) != 0) {
                                    PRINTF("[ group: %d sub_mask: %x tune: %f ] ");
                                    finished = 1;
                                    AudioThread_QueueCmdU16(0x90000000 | ((entry->bgm->group & 0xFF) << 16),
                                                            *entry->maskRamp);
                                    AudioThread_QueueCmdF32(0x04000000 | ((entry->bgm->group & 0xFF) << 16) | 0xFF00,
                                                            entry->rampValue);
                                }
                                break;
                        }
                    }
                    break;
                case NAI_PORT_TYPE_SUB_FXMIX:
                    if (gNaiSpecChange == 0) {
                        PRINTF("SUB FXMIX ");
                        switch (entry->ownerType) {
                            case 2:
                                if (Nai_SeIsActive(entry->se) != 0) {
                                    PRINTF("[ group: %d sub: %d fxmix: %f ] ");
                                    finished = 1;
                                    AudioThread_QueueCmdS8(0x05000000 | ((entry->group & 0xFF) << 16) |
                                                               ((entry->se->sub & 0xFF) << 8),
                                                           entry->rampValue);
                                }
                                break;
                            case 1:
                                if (Nai_BgmIsActive(entry->bgm) != 0) {
                                    PRINTF("[ group: %d fxmix: %f ] ");
                                    finished = Nai_PortRampStep(entry);
                                    for (i = 0; i < 16; i++) {
                                        AudioThread_QueueCmdS8(0x05000000 | ((entry->bgm->group & 0xFF) << 16) |
                                                                   ((i & 0xFF) << 8),
                                                               entry->value);
                                    }
                                    entry->bgm->fxmix = entry->value;
                                }
                                break;
                        }
                    }
                    break;
                case NAI_PORT_TYPE_SUB_PAN:
                    if (gNaiSpecChange == 0) {
                        PRINTF("SUB PAN ");
                        if (entry->ownerType == 2) {
                            if (Nai_SeIsActive(entry->se) != 0) {
                                PRINTF("[ group: %d sub: %d pan: %f ] ");
                                finished = 1;
                                AudioThread_QueueCmdS8(0x03000000 | ((entry->group & 0xFF) << 16) |
                                                           ((entry->se->sub & 0xFF) << 8),
                                                       entry->rampValue);
                            }
                        }
                    }
                    break;
                case NAI_PORT_TYPE_SUB_MUTE: // mute the subs in the mask, unmute the rest
                    if (gNaiSpecChange == 0) {
                        PRINTF("SUB MUTE ");
                        if (entry->ownerType == 1) {
                            PRINTF("[ group: %d sub_mask: %x ] ");
                            AudioThread_QueueCmdU16(0x90000000 | ((entry->bgm->group & 0xFF) << 16), *entry->maskRamp);
                            AudioThread_QueueCmdS8(0x08000000 | ((entry->bgm->group & 0xFF) << 16) | 0xFF00, 1);
                            AudioThread_QueueCmdU16(0x90000000 | ((entry->bgm->group & 0xFF) << 16), ~*entry->maskRamp);
                            AudioThread_QueueCmdS8(0x08000000 | ((entry->bgm->group & 0xFF) << 16) | 0xFF00, 0);
                            finished = 1;
                        }
                    }
                    break;
                case NAI_PORT_TYPE_PAUSE_ON:
                    if (gNaiSpecChange == 0) {
                        PRINTF("PAUSE ON ");
                        AudioThread_QueueCmdS32(0xF1000000, 0);
                        finished = 1;
                    }
                    break;
                case NAI_PORT_TYPE_PAUSE_OFF:
                    if (gNaiSpecChange == 0) {
                        PRINTF("PAUSE OFF ");
                        AudioThread_QueueCmdS32(0xF2000000, 0);
                        finished = 1;
                    }
                    break;
                case NAI_PORT_TYPE_PAUSE_OFF_STOP:
                    if (gNaiSpecChange == 0) {
                        PRINTF("PAUSE OFF STOP ");
                        AudioThread_QueueCmdS32(0xF2000000, 1);
                        finished = 1;
                    }
                    break;
                case NAI_PORT_TYPE_OUT_MODE:
                    if (gNaiSpecChange == 0) {
                        PRINTF("OUT MODE ");
                        PRINTF("%d ");
                        switch (entry->num) {
                            case SOUND_OUTPUT_STEREO:
                                AudioThread_QueueCmdS32(0xF0000000, SOUND_OUTPUT_STEREO);
                                break;
                            case SOUND_OUTPUT_SURROUND:
                                AudioThread_QueueCmdS32(0xF0000000, SOUND_OUTPUT_SURROUND);
                                break;
                            case SOUND_OUTPUT_HEADSET:
                                AudioThread_QueueCmdS32(0xF0000000, SOUND_OUTPUT_HEADSET);
                                break;
                            case SOUND_OUTPUT_MONO:
                                AudioThread_QueueCmdS32(0xF0000000, SOUND_OUTPUT_MONO);
                                break;
                        }
                        finished = 1;
                    }
                    break;
                case NAI_PORT_TYPE_SPEC_CHANGE_START: // requeue as SPEC_CHANGE_END, hold the gate till it lands
                    if (gNaiSpecChange == 0) {
                        PRINTF("SPEC CHANGE START ");
                        PRINTF("[ num: %d ] ");
                        AudioThread_ResetAudioHeap(entry->num);
                        entry->type = NAI_PORT_TYPE_SPEC_CHANGE_END;
                        gNaiSpecChange = 1;
                    }
                    break;
                case NAI_PORT_TYPE_SPEC_CHANGE_END:
                    PRINTF("SPEC CHANGE ");
                    finished = AudioThread_CheckSpecChange();
                    if (finished != 0) {
                        PRINTF(" END");
                        gNaiSpecChange = 0;
                    }
                    break;
                default:
                    PRINTF("NAI: 例外 PORT TYPE in PortRoutin\n");
                    break;
            }
            PRINTF("\n");
            if (finished != 0) {
                Nai_PortBufferFree(entry);
            }
            entry = entry->next;
        }
    }
}

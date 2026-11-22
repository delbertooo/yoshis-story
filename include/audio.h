#ifndef AUDIO_H
#define AUDIO_H

#include "ultra64.h"
#include "unk.h"

// The NA sound library's (src/main/na/) and the NA audio driver's (src/main/nad/) declarations, the counterpart of
// OoT's include/audio.h; O2/65520.c, the game's one C caller, includes it too. Not in global.h, which every file
// includes: a declaration there can re-order a game file's .bss.
// The library's type, field, constant and data names follow its stripped debug prints where one spells them, and
// are INVENTED otherwise (NaiSe.sndId is the prints' `num`).

// The options menu's sound setting, in menu order (Stereo, Mono, Headphones), which Na_SetSoundOutputMode turns
// into a SoundOutputMode. YS offers no SURROUND; OoT's id keeps the numbering.
typedef enum SoundSetting {
    /* 0 */ SOUND_SETTING_STEREO,
    /* 1 */ SOUND_SETTING_MONO,
    /* 2 */ SOUND_SETTING_HEADSET,
    /* 3 */ SOUND_SETTING_SURROUND
} SoundSetting;

// The driver's output mode, set through Nai_OutMode: Audio_InitSampleState gives HEADSET the Haas-delay pan,
// STEREO the hard pan and MONO 0.707 on both sides. Nothing sends SURROUND.
typedef enum SoundOutputMode {
    /* 0 */ SOUND_OUTPUT_STEREO,
    /* 1 */ SOUND_OUTPUT_HEADSET,
    /* 2 */ SOUND_OUTPUT_SURROUND,
    /* 3 */ SOUND_OUTPUT_MONO
} SoundOutputMode;

// NA sound library

struct NaiBgm;

// What the game's SoundEventParams_Set* builders fill for Na_SeTrig, Na_SeStart and Na_SeLevelChange: the
// position Na_SeCalcPanVolume turns into pan and volume, and the sound's own numbers.
typedef struct SoundEventParams {
    /* 0x00 */ s16 posX; // screen coord clamped [-320,640], scaled x2/5 (posY too)
    /* 0x02 */ s16 posY;
    /* 0x04 */ s32 yoshiColor; // the current Yoshi's colour as a one-hot bit, which colorPitch maps to a pitch
    /* 0x08 */ s32 ground;     // what Yoshi walks on, see Na_SeTrig's footstep switches
    /* 0x0C */ UNK_TYPE unkC;
    /* 0x10 */ UNK_TYPE unk10;
    // The sound's own numbers (a volume or pitch scale, a count, an id), as a stripped print labels them.
    /* 0x14 */ s32 value;
    /* 0x18 */ s32 value2;
} SoundEventParams; // size = 0x1C

// The event Na_Event dispatches on (the game's Sound_Event), named after its stripped "NA : Na_Event <NAME>"
// prints; NA_EVENT_NONE (0, an empty case) is INVENTED.
typedef enum NaEvent {
    /* 0x00 */ NA_EVENT_NONE,
    /* 0x01 */ NA_EVENT_YOSHI_DOWN,
    /* 0x02 */ NA_EVENT_YOSHI_FALL,
    /* 0x03 */ NA_EVENT_OPEN_SOGEN_PAGE,
    /* 0x04 */ NA_EVENT_OPEN_MAGMA_PAGE,
    /* 0x05 */ NA_EVENT_OPEN_MOUNTAIN_PAGE,
    /* 0x06 */ NA_EVENT_OPEN_JUNGLE_PAGE,
    /* 0x07 */ NA_EVENT_OPEN_WATER_PAGE,
    /* 0x08 */ NA_EVENT_OPEN_CASTLE_PAGE,
    /* 0x09 */ NA_EVENT_SUBGAME_START,
    /* 0x0A */ NA_EVENT_OPEN_SUBGAME_END,
    /* 0x0B */ NA_EVENT_OPENING_PLAYER_SELECT,
    /* 0x0C */ NA_EVENT_START_TURESARI_DEMO,
    /* 0x0D */ NA_EVENT_YOSHI_BURN,
    /* 0x0E */ NA_EVENT_YOSHI_TABERARERU,
    /* 0x0F */ NA_EVENT_ECHO_START,
    /* 0x10 */ NA_EVENT_ECHO_STOP,
    /* 0x11 */ NA_EVENT_FRUITS_30_GET,
    /* 0x12 */ NA_EVENT_SUBGAME_MISS,
    /* 0x13 */ NA_EVENT_OPEN_SUBGAME_BEST,
    /* 0x14 */ NA_EVENT_TIMER_START,
    /* 0x15 */ NA_EVENT_TIMER_END,
    /* 0x16 */ NA_EVENT_PAGE,
    /* 0x17 */ NA_EVENT_PAGE_FIRST,
    /* 0x18 */ NA_EVENT_VOICE_STOP,
    /* 0x19 */ NA_EVENT_YOSHI_HASAMARERU,
    /* 0x1A */ NA_EVENT_FRUITS_30_BOSS,
    /* 0x1B */ NA_EVENT_NULL,
    /* 0x1C */ NA_EVENT_MID_BOSS_DEMO,
    /* 0x1D */ NA_EVENT_MID_BOSS_START,
    /* 0x1E */ NA_EVENT_MID_BOSS_DOWN_2,
    /* 0x1F */ NA_EVENT_KUPPA_DEMO,
    /* 0x20 */ NA_EVENT_KUPPA_START_1,
    /* 0x21 */ NA_EVENT_KUPPA_DOWN_1,
    /* 0x22 */ NA_EVENT_KUPPA_DEMO_2,
    /* 0x23 */ NA_EVENT_NYORO_DANGER_IN,
    /* 0x24 */ NA_EVENT_KUPPA_START_2,
    /* 0x25 */ NA_EVENT_COURSE_CLEAR,
    /* 0x26 */ NA_EVENT_NYORO_DANGER_OUT,
    /* 0x27 */ NA_EVENT_KUPPA_DOWN_2,
    /* 0x28 */ NA_EVENT_KUPPA_TSURESARI,
    /* 0x29 */ NA_EVENT_GAMEOVER,
    /* 0x2A */ NA_EVENT_COURSE_CLEAR_BOSS,
    /* 0x2B */ NA_EVENT_MID_BOSS_DOWN,
    /* 0x2C */ NA_EVENT_MELON_30_GET,
    /* 0x2D */ NA_EVENT_MELON_30_BOSS,
    /* 0x2E */ NA_EVENT_OPEN_SUBGAME_END_OKA,
    /* 0x2F */ NA_EVENT_OPEN_SUBGAME_BEST_OKA,
    /* 0x30 */ NA_EVENT_SAME_FRUITS_30_GET,
    /* 0x31 */ NA_EVENT_SAME_FRUITS_30_BOSS
} NaEvent;

// The command type Nai_PortRoutin dispatches on (its stripped assert's "PORT TYPE"), named after its stripped
// per-entry dump's labels ("NAI: PORT[%x] <LABEL> [ args ]").
typedef enum NaiPortType {
    /* 0x00 */ NAI_PORT_TYPE_SEQ_START,
    /* 0x01 */ NAI_PORT_TYPE_SEQ_STOP,
    /* 0x02 */ NAI_PORT_TYPE_PAUSE_ON,
    /* 0x03 */ NAI_PORT_TYPE_PAUSE_OFF,
    /* 0x04 */ NAI_PORT_TYPE_PAUSE_OFF_STOP,
    /* 0x05 */ NAI_PORT_TYPE_OUT_MODE,
    /* 0x06 */ NAI_PORT_TYPE_SUB_EVOLUME,
    /* 0x07 */ NAI_PORT_TYPE_SUB_PAN,
    /* 0x08 */ NAI_PORT_TYPE_SUB_TUNE,
    /* 0x09 */ NAI_PORT_TYPE_SUB_VIB,
    /* 0x0A */ NAI_PORT_TYPE_SUB_FXMIX,
    /* 0x0B */ NAI_PORT_TYPE_SUB_MUTE,
    /* 0x0C */ NAI_PORT_TYPE_SUB_PORT_WRITE,
    /* 0x0D */ NAI_PORT_TYPE_GRP_EVOLUME,
    /* 0x0E */ NAI_PORT_TYPE_GRP_TEMPO,
    /* 0x0F */ NAI_PORT_TYPE_GRP_TEMPO_OFFSET,
    /* 0x10 */ NAI_PORT_TYPE_GRP_TRANS,
    /* 0x11 */ NAI_PORT_TYPE_GRP_PORT_WRITE,
    /* 0x12 */ NAI_PORT_TYPE_SPEC_CHANGE_END,
    /* 0x13 */ NAI_PORT_TYPE_SPEC_CHANGE_START
} NaiPortType;

// One command of the 0x100-entry "port buffer" gNaiPortBuffer, linked oldest first on gNaiPortList while inUse.
// A ramp command steps `value` from start to end over `time` frames (Nai_PortRampStep).
typedef struct NaiPort {
    /* 0x00 */ s32 type;      // NaiPortType
    /* 0x04 */ s32 ownerType; // 0 none, 1 = `bgm` owns the entry, 2 = `se` does
    /* 0x08 */ struct NaiBgm* bgm;
    /* 0x0C */ struct NaiSe* se;
    /* 0x10 */ s16 group;
    /* 0x12 */ s16 sub;
    /* 0x14 */ s16 num;
    /* 0x16 */ char pad16[0x02];
    /* 0x18 */ f32 rampValue; // payload of the handle-ramped commands (Nai_StepRamp, Nai_StepMaskRamp)
    /* 0x1C */ s32 time;
    /* 0x20 */ f32* ramp;     // points at the handle's ramp triple (na/nai_ramp.c's NaiRamp)
    /* 0x24 */ u16* maskRamp; // points at a mask pair, then a ramp triple (NaiMaskRamp)
    /* 0x28 */ f32 start;
    /* 0x2C */ f32 value;
    /* 0x30 */ f32 end;
    /* 0x34 */ f32 delta;
    /* 0x38 */ s32 elapsed;
    /* 0x3C */ s32 inUse;
    /* 0x40 */ struct NaiPort* prev;
    /* 0x44 */ struct NaiPort* next;
} NaiPort; // size = 0x48

// An SE slot's state (NaiSe.status), stepped every frame by Nai_SeRoutin and tagged - W s S %d in its buffer
// dump. Nai_SeBufferAlloc marks nothing: the slot's starter takes it out of FREE.
typedef enum NaiSeStatus {
    /* 0x00 */ NAI_SE_STATUS_FREE,
    /* 0x01 */ NAI_SE_STATUS_WAITING,  // waits for a voice (none free, or a stronger sound took it)
    /* 0x02 */ NAI_SE_STATUS_QUEUED,   // holds a voice it has muted, waits for it to fall idle
    /* 0x03 */ NAI_SE_STATUS_STARTING, // note sent, waits for the voice to sound
    /* 0x04 */ NAI_SE_STATUS_PLAYING   // sounding; freed when it ends
} NaiSeStatus;

// One of gNaiSeBuffer's 50 slots ("SE Buffer"): a sound on channel `sub` of the SE group (sequence player 2),
// with NaiBgm's {current, target, time} ramp triples, which Nai_SePostChanges posts when their halves disagree.
typedef struct NaiSe {
    /* 0x00 */ s16 sub;
    /* 0x02 */ s16 sndId; // sent as note (% 128) and bank (/ 128)
    /* 0x04 */ s16 group; // -1 = none; otherwise one slot per group plays at a time
    /* 0x06 */ char pad6[0x02];
    /* 0x08 */ f32 volume;
    /* 0x0C */ f32 volumeTarget;
    /* 0x10 */ s16 volumeTime;
    /* 0x12 */ char pad12[0x02];
    /* 0x14 */ f32 pan;
    /* 0x18 */ f32 panTarget;
    /* 0x1C */ s16 panTime;
    /* 0x1E */ char pad1E[0x02];
    /* 0x20 */ f32 tune;
    /* 0x24 */ f32 tuneTarget;
    /* 0x28 */ s16 tuneTime;
    /* 0x2A */ char pad2A[0x02];
    /* 0x2C */ f32 fxmix;
    /* 0x30 */ f32 fxmixTarget;
    /* 0x34 */ s16 fxmixTime;
    /* 0x36 */ char pad36[0x02];
    /* 0x38 */ f32 priority; // its starter's priority, as a float
    /* 0x3C */ u8 paused;    // set while Nai_SePause holds the slot
    /* 0x3D */ char pad3D[0x03];
    /* 0x40 */ s32 status; // NaiSeStatus
    /* 0x44 */ s32 owned;  // 0 for a slot started by Nai_SeTrig, 1 by Nai_SeStart (a caller holds it)
} NaiSe;                   // size = 0x48

// A sound handle's state (NaiBgm.status), stepped every frame by Nai_BgmRoutin. 5 is never assigned (only
// Nai_BgmStop and Nai_BgmIsActive test for it).
typedef enum NaiBgmStatus {
    /* 0x00 */ NAI_BGM_STATUS_PLAYING,  // the group sounds; retired when it falls silent
    /* 0x01 */ NAI_BGM_STATUS_QUEUED,   // counts startDelay down, then posts the start once the group is free
    /* 0x02 */ NAI_BGM_STATUS_STARTING, // start posted; the group sounding makes it gNaiCurrentBgm
    /* 0x03 */ NAI_BGM_STATUS_STOPPING, // stop posted; freed or kept once the group falls silent
    /* 0x04 */ NAI_BGM_STATUS_WAITING,  // on gNaiBgmWaitList until a group frees up
    /* 0x05 */ NAI_BGM_STATUS_5,        // never assigned
    /* 0x06 */ NAI_BGM_STATUS_KEPT,     // parked once its group falls silent with keep raised
    /* 0x07 */ NAI_BGM_STATUS_FREE      // free slot
} NaiBgmStatus;

// A sound handle: one BGM on a group (sequence player 0 or 1), with {current, target, frames left} ramp triples
// and requested/live mask pairs, which the nai_bgm_set.c setters stage and Nai_BgmPostChanges posts.
typedef struct NaiBgm {
    /* 0x00 */ s16 group;
    /* 0x02 */ s16 num;
    /* 0x04 */ s16 time;
    /* 0x06 */ s16 startDelay; // frames NAI_BGM_STATUS_QUEUED waits before it posts the start
    /* 0x08 */ f32 volume;
    /* 0x0C */ f32 volumeTarget;
    /* 0x10 */ s16 volumeTime;
    /* 0x12 */ char pad12[0x02];
    /* 0x14 */ f32 tempoOffset;
    /* 0x18 */ f32 tempoOffsetTarget;
    /* 0x1C */ s16 tempoOffsetTime;
    /* 0x1E */ char pad1E[0x02];
    /* 0x20 */ u16 muteSubMask;     // the subs to mute; the driver unmutes every other sub
    /* 0x22 */ u16 muteSubMaskLive; // read back each frame from the group's 16 channels
    /* 0x24 */ char pad24[0x0C];
    /* 0x30 */ u16 tuneSubMask;     // the subs the tune applies to
    /* 0x32 */ u16 tuneSubMaskSent; // synced to tuneSubMask when the port is posted
    /* 0x34 */ f32 tune;
    /* 0x38 */ f32 tuneTarget;
    /* 0x3C */ s16 tuneTime;
    /* 0x3E */ char pad3E[0x02];
    /* 0x40 */ f32 subVolume[16]; // each sub's last SUB_EVOLUME volume; never read
    /* 0x80 */ f32 fxmix;         // the last SUB FXMIX value, sent to all 16 subs; never read
    /* 0x84 */ s32 status;        // NaiBgmStatus
    /* 0x88 */ s32 alongside;     // 1 = play alongside a BGM on the other group; 0 = exclusive, take both
    /* 0x8C */ s32 keep;          // 1 = park as NAI_BGM_STATUS_KEPT once the group falls silent, 0 = free
} NaiBgm;                         // size = 0x90

// A node of the BGM waiting list gNaiBgmWaitList, out of the 10-node pool gNaiBgmWaitNodes.
typedef struct NaiBgmWaitNode {
    /* 0x00 */ u8 inUse;
    /* 0x01 */ char pad1[0x03];
    /* 0x04 */ NaiBgm* bgm; // the handle waiting for a group to free up
    /* 0x08 */ struct NaiBgmWaitNode* prev;
    /* 0x0C */ struct NaiBgmWaitNode* next;
} NaiBgmWaitNode; // size = 0x10

// NA audio driver: OoT's types under OoT's names; each note says where YS's layout differs. The head words
// (AdsrStateHead, SequenceChannelHead, SequenceLayerHead, NoteSampleStateHead, StereoPanWord) are unions of a
// u32 bitfield view and a byte view of one word, where OoT has bitfields or locals. Load-bearing: both views.

// OoT's EnvelopePoint, one point of an envelope: `delay` in ticks or an ADSR_* code below, `arg` the level or,
// for ADSR_GOTO, the index to jump to.
typedef struct EnvelopePoint {
    /* 0x00 */ s16 delay;
    /* 0x02 */ s16 arg;
} EnvelopePoint; // size = 0x4

// OoT's special `delay` values of an EnvelopePoint; any value above 0 is a delay.
#define ADSR_DISABLE 0
#define ADSR_HANG -1
#define ADSR_GOTO -2
#define ADSR_RESTART -3

// OoT's AdpcmLoop and AdpcmBook (the SGI library's ALADPCMloop and ALADPCMbook), flat as in OoT; a Sample
// points at one of each.
typedef struct AdpcmLoop {
    /* 0x00 */ u32 start;              // in source samples
    /* 0x04 */ u32 end;                // the position the decoder may not read past
    /* 0x08 */ u32 count;              // 0 = one-shot: the note ends here
    /* 0x0C */ u32 sampleEnd;          // MM's name (OoT's unk_0C); never read here
    /* 0x10 */ s16 predictorState[16]; // only exists if count != 0. 8-byte aligned
} AdpcmLoop;                           // size = 0x30 (or 0x10)

typedef struct AdpcmBook {
    /* 0x00 */ s32 order;
    /* 0x04 */ s32 numPredictors;
    /* 0x08 */ s16 book[1]; // size 8 * order * numPredictors. 8-byte aligned
} AdpcmBook;                // size >= 0x8

// OoT's Sample (the SGI library's ALWaveTable): codec, medium and size in one word, then the data, loop and book.
typedef struct Sample {
    /* 0x00 */ u32 codec : 4;
    /* 0x00 */ u32 medium : 2; // mask 0x0C of byte 0
    /* 0x00 */ u32 unk_bit26 : 1;
    /* 0x00 */ u32 isRelocated : 1; // mask 0x01 of byte 0
    /* 0x01 */ u32 size : 24;
    /* 0x04 */ u8* sampleAddr;
    /* 0x08 */ AdpcmLoop* loop;
    /* 0x0C */ AdpcmBook* book;
} Sample; // size = 0x10

// OoT's TunedSample: a Sample and the frequency scale it plays at.
typedef struct TunedSample {
    /* 0x00 */ Sample* sample;
    /* 0x04 */ f32 tuning;
} TunedSample; // size = 0x8

// OoT's Instrument, a font's instrument record (Audio_GetInstrumentInner): normalRangeLo 0 means no low-pitch
// sample, normalRangeHi 0x7F no high-pitch one.
typedef struct Instrument {
    /* 0x00 */ u8 isRelocated; // have the envelope and all samples been relocated (offsets to pointers)
    /* 0x01 */ u8 normalRangeLo;
    /* 0x02 */ u8 normalRangeHi;
    /* 0x03 */ u8 adsrDecayIndex;
    /* 0x04 */ EnvelopePoint* envelope;
    /* 0x08 */ TunedSample lowPitchTunedSample;
    /* 0x10 */ TunedSample normalPitchTunedSample;
    /* 0x18 */ TunedSample highPitchTunedSample;
} Instrument; // size = 0x20

// OoT's Drum, a font's drum record (Audio_GetDrum); program 0 makes the key the instrument.
typedef struct Drum {
    /* 0x00 */ u8 adsrDecayIndex;
    /* 0x01 */ u8 pan;
    /* 0x02 */ u8 isRelocated; // have tunedSample.sample and envelope been relocated (offsets to pointers)
    /* 0x04 */ TunedSample tunedSample;
    /* 0x0C */ EnvelopePoint* envelope;
} Drum; // size = 0x10

// OoT's AdsrStatus, the envelope state Audio_AdsrUpdate switches on (AdsrStateHead.f.state).
typedef enum AdsrStatus {
    /* 0 */ ADSR_STATE_DISABLED,
    /* 1 */ ADSR_STATE_INITIAL,
    /* 2 */ ADSR_STATE_START_LOOP,
    /* 3 */ ADSR_STATE_LOOP,
    /* 4 */ ADSR_STATE_FADE,
    /* 5 */ ADSR_STATE_HANG,
    /* 6 */ ADSR_STATE_DECAY,
    /* 7 */ ADSR_STATE_RELEASE,
    /* 8 */ ADSR_STATE_SUSTAIN
} AdsrStatus;

// AdsrState's head word (OoT's `action` union and the two fields after it): the AdsrStatus in byte 0's low
// nibble, hang/decay/release requests above it.
typedef union AdsrStateHead {
    struct {
        /* 0x00 */ u32 unused : 1;
        /* 0x00 */ u32 hang : 1;    // ADSR_STATE_INITIAL goes straight to ADSR_STATE_HANG
        /* 0x00 */ u32 decay : 1;   // request: jump to ADSR_STATE_DECAY
        /* 0x00 */ u32 release : 1; // request: jump to ADSR_STATE_RELEASE
        /* 0x00 */ u32 state : 4;   // AdsrStatus
        /* 0x01 */ u32 : 8;         // b.envIndex
        /* 0x02 */ u32 : 16;        // b.delay
    } f;
    struct {
        /* 0x00 */ u8 action;   // OoT's action.asByte
        /* 0x01 */ u8 envIndex; // index of the current point in AdsrState.envelope
        /* 0x02 */ s16 delay;   // ticks left in the current segment
    } b;
} AdsrStateHead; // size = 0x4

// OoT's AdsrState, the envelope generator: Audio_AdsrInit resets it, Audio_AdsrUpdate steps it one tick and
// returns the level, clamped to [0, 1].
typedef struct AdsrState {
    /* 0x00 */ AdsrStateHead h;
    /* 0x04 */ f32 sustain;    // 0.0f means "no sustain phase"
    /* 0x08 */ f32 velocity;   // per-tick increment of current while ramping
    /* 0x0C */ f32 fadeOutVel; // per-tick decrement of current while releasing
    /* 0x10 */ f32 current;
    /* 0x14 */ f32 target; // level of the current segment
    /* 0x18 */ char pad18[0x4];
    /* 0x1C */ EnvelopePoint* envelope;
} AdsrState; // size >= 0x20

// OoT's Portamento (Audio_GetPortamentoFreqScale): `cur` is 8.8 fixed point, and the slide stops at 0x7F00 by
// clearing `mode`.
typedef struct Portamento {
    /* 0x00 */ u8 mode; // nonzero while sliding
    /* 0x01 */ char pad1[0x1];
    /* 0x02 */ u16 cur;
    /* 0x04 */ u16 speed; // added to cur per call
    /* 0x06 */ char pad6[0x2];
    /* 0x08 */ f32 extent;
} Portamento; // size = 0xC

// OoT's AudioListItem, a node of a circular doubly linked list whose head is an item too: `u` counts a head's
// items and points at an item's owner. A Note begins with one.
typedef struct AudioListItem {
    /* 0x00 */ struct AudioListItem* prev; // NULL on an item means "not linked anywhere"
    /* 0x04 */ struct AudioListItem* next;
    /* 0x08 */ union {
        s32 count;
        struct Note* note;
        void* value;
    } u;
    /* 0x0C */ struct AudioListItem* pool; // the note pool (four list heads) this item belongs to
} AudioListItem;                           // size = 0x10

// OoT's SeqScriptState, one script's interpreter state: its cursor, call/loop stack and value register.
typedef struct SeqScriptState {
    /* 0x00 */ u8* pc;             // stream cursor
    /* 0x04 */ u8* stack[4];       // return cursors, one per open call or loop
    /* 0x14 */ u8 remLoopIters[4]; // repeats left, one per open loop
    /* 0x18 */ u8 depth;           // stack depth
    /* 0x19 */ s8 value;           // the script's value register; the conditional jumps test it
} SeqScriptState;                  // size = 0x1C

// OoT's AdsrSettings, the envelope a channel or layer plays its notes with.
typedef struct AdsrSettings {
    /* 0x00 */ u8 decayIndex;
    /* 0x01 */ u8 sustain;
    /* 0x02 */ char pad2[0x2];
    /* 0x04 */ EnvelopePoint* envelope;
} AdsrSettings; // size = 0x8

// SequenceChannel's head word: OoT's leading bitfields, `changes`, noteAllocPolicy and muteBehavior.
typedef union SequenceChannelHead {
    struct {
        /* 0x00 */ u32 enabled : 1;
        /* 0x00 */ u32 finished : 1;
        /* 0x00 */ u32 stopScript : 1;
        /* 0x00 */ u32 muted : 1;
        /* 0x00 */ u32 hasInstrument : 1; // set by AudioSeq_SetInstrument
        /* 0x00 */ u32 stereoHeadsetEffects : 1;
        /* 0x00 */ u32 largeNotes : 1; // notes carry their own velocity and duration
        /* 0x00 */ u32 unused : 1;
        // OoT's `changes.s`: the freqScale, volume and pan dirty flags.
        /* 0x01 */ u32 changesFreqScale : 1;
        /* 0x01 */ u32 changesVolume : 1;
        /* 0x01 */ u32 changesPan : 1;
        /* 0x01 */ u32 : 21; // the rest of byte 1, b.noteAllocPolicy, b.muteBehavior
    } f;
    struct {
        /* 0x00 */ u8 flags;   // f's eight flag bits as one byte
        /* 0x01 */ u8 changes; // OoT's changes.asByte
        /* 0x02 */ u8 noteAllocPolicy;
        /* 0x03 */ u8 muteBehavior; // bit 0x08 and bit 0x40 both gate voice stealing
    } b;
} SequenceChannelHead; // size = 0x4

// OoT's SequenceChannel (0xD4 there), one channel of a SequencePlayer. YS has six layers where OoT has four, so
// scriptState and everything after it sit 8 bytes later.
typedef struct SequenceChannel {
    /* 0x00 */ SequenceChannelHead h;
    /* 0x04 */ u8 targetReverbVol;
    /* 0x05 */ u8 notePriority; // what a note playing on this channel is worth when stealing
    /* 0x06 */ u8 someOtherPriority;
    /* 0x07 */ u8 fontId;
    /* 0x08 */ u8 reverbIndex;
    /* 0x09 */ u8 bookOffset;
    /* 0x0A */ u8 newPan;           // 0..0x7F (reset to 0x40)
    /* 0x0B */ u8 panChannelWeight; // 0..0x80 (reset to 0x80)
    /* 0x0C */ u8 gain;
    /* 0x0D */ u8 velocityRandomVariance;
    /* 0x0E */ u8 gateTimeRandomVariance;
    /* 0x0F */ u8 combFilterSize;
    /* 0x10 */ u16 vibratoRateStart;
    /* 0x12 */ u16 vibratoDepthStart;
    /* 0x14 */ u16 vibratoRateTarget;
    /* 0x16 */ u16 vibratoDepthTarget;
    /* 0x18 */ u16 vibratoRateChangeDelay;
    /* 0x1A */ u16 vibratoDepthChangeDelay;
    /* 0x1C */ u16 vibratoDelay;
    /* 0x1E */ u16 delay;
    /* 0x20 */ u16 combFilterGain;
    /* 0x22 */ u16 ptr;           // the ASEQ_OP_CHAN_*PTR* events' register (OoT's unk_22), an offset into seqData
    /* 0x24 */ s16 instOrWave;    // what a layer asking for 0xFF plays
    /* 0x26 */ s16 transposition; // added into event 0xC7's byte
    /* 0x28 */ f32 volumeScale;
    /* 0x2C */ f32 volume;        // controller-set level, byte/127 (AudioSeq_SequenceChannelSetVolume)
    /* 0x30 */ s32 pan;           // newPan * panChannelWeight (Audio_SequenceChannelProcessSound)
    /* 0x34 */ f32 appliedVolume; // the squared level the layers scale their velocity by
    /* 0x38 */ f32 freqScale;
    /* 0x3C */ u8* dynTable; // set by event 0xC2/0xB4
    /* 0x40 */ struct Note* noteUnused;
    /* 0x44 */ struct SequenceLayer* layerUnused;
    /* 0x48 */ Instrument* instrument; // the bank-table record AudioSeq_GetInstrument resolved
    /* 0x4C */ struct SequencePlayer* seqPlayer;
    /* 0x50 */ struct SequenceLayer* layers[6];
    /* 0x68 */ SeqScriptState scriptState;
    /* 0x84 */ AdsrSettings adsr;
    /* 0x8C */ AudioListItem notePool[4];
    /* 0xCC */ s8 seqScriptIO[8];
    /* 0xD4 */ void* filter;
} SequenceChannel; // size >= 0xD8

// OoT's VibratoState (Audio_GetVibratoFreqScale). A channel of (SequenceChannel*)-1 freezes the depth and rate
// ramps; the oscillator keeps running.
typedef struct VibratoState {
    /* 0x00 */ SequenceChannel* channel;
    /* 0x04 */ u32 time; // phase accumulator; bits 10..15 index the 64-entry curve
    /* 0x08 */ s16* curve;
    /* 0x0C */ f32 depth;
    /* 0x10 */ f32 rate; // added to time each call
    /* 0x14 */ u8 active;
    /* 0x15 */ char pad15[0x1];
    /* 0x16 */ u16 rateChangeTimer;  // calls left in the rate ramp
    /* 0x18 */ u16 depthChangeTimer; // calls left in the depth ramp
    /* 0x1A */ u16 delay;            // start delay in calls
} VibratoState;                      // size = 0x1C

// SequenceLayer's head word (OoT inlines the bitfields).
typedef union SequenceLayerHead {
    struct {
        /* 0x00 */ u32 enabled : 1;
        /* 0x00 */ u32 finished : 1;
        /* 0x00 */ u32 muted : 1;
        /* 0x00 */ u32 continuousNotes : 1;
        // OoT's bit3: set when a note is bound to the layer and when Audio_AllocNote finds none, cleared by
        // Audio_SeqLayerDecayRelease, read only by Step5's same-note test. Meaning unproven.
        /* 0x00 */ u32 unk0_4 : 1;
        /* 0x00 */ u32 ignoreDrumPan : 1;
        // OoT's bit1: set where Audio_ProcessNotes disables the layer's note, cleared by a new note or rest; a
        // legato layer skips re-triggering while it is set.
        /* 0x00 */ u32 noteDisabled : 1;
        /* 0x00 */ u32 notePropertiesNeedInit : 1;
        /* 0x01 */ u32 : 24; // b.stereo, b.instOrWave, b.gateTime
    } f;
    struct {
        /* 0x00 */ u8 flags;      // f's eight flag bits as one byte
        /* 0x01 */ u8 stereo;     // a Stereo (playback.c), copied as a whole
        /* 0x02 */ u8 instOrWave; // 0xFF means "use the channel's instOrWave"
        /* 0x03 */ u8 gateTime;
    } b;
} SequenceLayerHead; // size = 0x4

// A note's parent-layer sentinel (OoT's): no layer owns or wants the note.
#define NO_LAYER ((SequenceLayer*)(-1))

// What a script IO port holds when nothing has been written to it (OoT's).
#define SEQ_IO_VAL_NONE -1

// OoT's SequenceLayer (0x80 in both), one of a channel's six note scripts. YS has no `semitone`, so
// portamentoTargetNote, pan and notePan sit one byte lower.
typedef struct SequenceLayer {
    /* 0x00 */ SequenceLayerHead h;
    /* 0x04 */ u8 portamentoTargetNote; // event 0xC7's byte plus the three transposes, forced to 0 at 0x80
    /* 0x05 */ u8 pan;
    /* 0x06 */ u8 notePan; // pan mixed with the channel's (Audio_SequenceChannelProcessSound)
    /* 0x07 */ char pad7[0x1];
    /* 0x08 */ s16 delay;     // ticks left in the note
    /* 0x0A */ s16 gateDelay; // tick count at which the release is triggered
    /* 0x0C */ u16 delay2;
    /* 0x0E */ u16 portamentoTime;
    /* 0x10 */ s16 transposition;         // added into event 0xC7's byte
    /* 0x12 */ s16 shortNoteDefaultDelay; // set by event 0xC3
    /* 0x14 */ s16 lastDelay;             // the note length the 0x80 short form repeats
    /* 0x16 */ char pad16[0x2];
    /* 0x18 */ AdsrSettings adsr; // decayIndex 0 selects the instrument's own envelope instead of adsr.envelope
    /* 0x20 */ Portamento portamento;
    /* 0x2C */ struct Note* note;
    /* 0x30 */ f32 freqScale; // rescaled in place by Audio_BuildSyntheticWave
    /* 0x34 */ f32 bend;
    /* 0x38 */ f32 velocitySquare2;
    /* 0x3C */ f32 velocitySquare; // (v/127)^2, from event 0xC1 or the 0xD0 short form
    /* 0x40 */ f32 noteVelocity;
    /* 0x44 */ f32 noteFreqScale;
    /* 0x48 */ Instrument* instrument;
    /* 0x4C */ TunedSample* tunedSample;
    /* 0x50 */ SequenceChannel* channel;
    /* 0x54 */ SeqScriptState scriptState;
    /* 0x70 */ AudioListItem listItem; // free-list link, threaded on gAudioCtx.layerFreeList while the layer is idle
} SequenceLayer;                       // size >= 0x80

// MM's SeqPlayerState (OoT leaves the values bare), SequencePlayer.state: FADE_IN ramps the next ASEQ_OP_SEQ_VOL
// up from silence, FADE_OUT ignores volume commands and disables the player when the fade ends.
typedef enum SeqPlayerState {
    /* 0 */ SEQPLAYER_STATE_0,
    /* 1 */ SEQPLAYER_STATE_FADE_IN,
    /* 2 */ SEQPLAYER_STATE_FADE_OUT
} SeqPlayerState;

// OoT's SequencePlayer, one of gAudioCtx.seqPlayers. YS has no `bend`, so `channels` and everything after it
// sit 4 lower, and the last 4 bytes (to OoT's 0x160) go untouched.
typedef struct SequencePlayer {
    /* 0x00 */ u32 enabled : 1;
    /* 0x00 */ u32 finished : 1;
    /* 0x00 */ u32 muted : 1;
    /* 0x00 */ u32 seqDmaInProgress : 1;
    /* 0x00 */ u32 fontDmaInProgress : 1;
    /* 0x00 */ u32 recalculateVolume : 1;
    /* 0x00 */ u32 stopScript : 1;
    /* 0x00 */ u32 : 1;  // OoT's applyBend; unused
    /* 0x01 */ u8 state; // SeqPlayerState
    /* 0x02 */ u8 noteAllocPolicy;
    /* 0x03 */ u8 muteBehavior;
    /* 0x04 */ u8 seqId;
    /* 0x05 */ u8 defaultFont;
    /* 0x06 */ char pad6[0x2];
    /* 0x08 */ u16 tempo; // reset to 0x1680
    /* 0x0A */ u16 tempoAcc;
    /* 0x0C */ s16 tempoChange;
    /* 0x0E */ s16 transposition; // added into event 0xC7's byte
    /* 0x10 */ u16 delay;
    /* 0x12 */ u16 fadeTimer; // ticks left in the volume ramp
    /* 0x14 */ u16 fadeTimerUnkEu;
    /* 0x16 */ char pad16[0x2];
    /* 0x18 */ s32 seqData;      // AudioLoad_SyncInitSeqPlayerInternal stores it here and in scriptState.pc
    /* 0x1C */ f32 fadeVolume;   // current ramp level, clamped to [0, 1]
    /* 0x20 */ f32 fadeVelocity; // per-tick increment
    /* 0x24 */ f32 volume;
    /* 0x28 */ f32 muteVolumeScale; // reset to 0.5f
    /* 0x2C */ f32 fadeVolumeScale;
    /* 0x30 */ f32 appliedFadeVolume; // = fadeVolume * fadeVolumeScale
    /* 0x34 */ SequenceChannel* channels[16];
    /* 0x74 */ SeqScriptState scriptState;
    /* 0x90 */ u8* shortNoteVelocityTable; // reset to gDefaultShortNoteVelocityTable; the 0xD0 low nibble indexes it
    /* 0x94 */ u8* shortNoteGateTimeTable; // reset to gDefaultShortNoteGateTimeTable; the 0xE0 low nibble indexes it
    /* 0x98 */ AudioListItem notePool[4];
    /* 0xD8 */ s32 skipTicks; // ticks this player still owes the sequencer, run down by AudioSeq_SkipForwardSequence
    /* 0xDC */ s32 scriptCounter;
    /* 0xE0 */ char padE0[0x74];
    /* 0x154 */ s8 seqScriptIO[8];
    /* 0x15C */ char pad15C[0x4];
} SequencePlayer; // size = 0x160

// NoteSampleState's head word: OoT's bitField0 and bitField1 and the two bytes after them.
typedef union NoteSampleStateHead {
    struct {
        /* 0x00 */ volatile u32 enabled : 1; // load-bearing `volatile`, as in OoT
        /* 0x00 */ u32 needsInit : 1;
        /* 0x00 */ u32 finished : 1;
        /* 0x00 */ u32 unused : 1;
        /* 0x00 */ u32 stereoStrongRight : 1;
        /* 0x00 */ u32 stereoStrongLeft : 1;
        /* 0x00 */ u32 stereoHeadsetEffects : 1;
        /* 0x00 */ u32 usesHeadsetPanEffects : 1;
        /* 0x01 */ u32 reverbIndex : 3;
        /* 0x01 */ u32 bookOffset : 2;
        /* 0x01 */ u32 isSyntheticWave : 2; // two bits here, one in OoT (where hasTwoParts moves up)
        /* 0x01 */ u32 hasTwoParts : 1;
        /* 0x02 */ u32 : 16; // b.gain, b.haasEffectLeftDelaySize
    } f;
    struct {
        /* 0x00 */ u8 bitField0;
        /* 0x01 */ u8 bitField1;
        /* 0x02 */ u8 gain; // UQ4.4, 0x10 = 1.0
        /* 0x03 */ u8 haasEffectLeftDelaySize;
    } b;
} NoteSampleStateHead; // size = 0x4

// Audio_InitSampleState's stereo byte, reverb and pan in one word (OoT: three locals). Byte 0 is OoT's
// StereoData two bits lower: YS keeps the headset flags in the sample state.
typedef union StereoPanWord {
    struct {
        /* 0x00 */ u32 unused : 4;
        /* 0x00 */ u32 bit2 : 2; // how the two strong flags combine with the ones the pan produced
        /* 0x00 */ u32 strongRight : 1;
        /* 0x00 */ u32 strongLeft : 1;
        /* 0x01 */ u32 : 24; // b.pad1, b.reverbVol, b.pan
    } f;
    struct {
        /* 0x00 */ u8 stereo;
        /* 0x01 */ char pad1[0x1];
        /* 0x02 */ u8 reverbVol;
        /* 0x03 */ u8 pan;
    } b;
} StereoPanWord; // size = 0x4

// OoT's NoteSampleStateAttributes (0x18 there: no comb filter), what Audio_ProcessNotes builds for
// Audio_InitSampleState from the note's own attributes or its layer's and channel's.
typedef struct NoteSampleStateAttributes {
    /* 0x00 */ u8 reverbVol;
    /* 0x01 */ u8 gain;       // UQ4.4, 0x10 = 1.0
    /* 0x02 */ u8 pan;        // 0..0x7F
    /* 0x03 */ u8 stereo;     // a Stereo (playback.c), copied as a whole
    /* 0x04 */ f32 frequency; // resampling ratio
    /* 0x08 */ f32 velocity;  // clamped to [0, 1] before it is scaled out
    /* 0x0C */ char padC[0x4];
    /* 0x10 */ void* filter;
} NoteSampleStateAttributes; // size = 0x14

// OoT's NoteSampleState, what the synthesis reads for a note: each Note's own copy, which
// AudioSynth_SyncSampleStates syncs with gAudioCtx.sampleStates.
typedef struct NoteSampleState {
    /* 0x00 */ NoteSampleStateHead h;
    /* 0x04 */ u8 haasEffectRightDelaySize;
    /* 0x05 */ u8 reverbVol;
    /* 0x06 */ u8 harmonicIndexCurAndPrev;
    /* 0x07 */ char pad7[0x1];
    /* 0x08 */ s16 targetVolLeft; // 0..4095
    /* 0x0A */ s16 targetVolRight;
    /* 0x0C */ u16 resamplingRateFixedPoint; // 1.15 fixed point
    /* 0x0E */ char padE[0x2];
    /* 0x10 */ union {
        TunedSample* tunedSample;
        s16* waveSampleAddr; // for a synthetic wave, the wave's samples at its harmonic index
    };
    /* 0x14 */ void* filter;
    /* 0x18 */ char pad18[0x8];
} NoteSampleState; // size = 0x20

// OoT's NoteAttributes (0x24 there: no comb filter or filter buffer), what a note keeps when its layer lets go.
typedef struct NoteAttributes {
    /* 0x00 */ u8 reverb;
    /* 0x01 */ u8 gain;
    /* 0x02 */ u8 pan;
    /* 0x03 */ u8 stereo; // a Stereo (playback.c), copied as a whole
    /* 0x04 */ char pad4[0x4];
    /* 0x08 */ f32 freqScale;
    /* 0x0C */ f32 velocity;
    /* 0x10 */ void* filter;
} NoteAttributes; // size = 0x14

// OoT's NoteSynthesisState, AudioSynth_ProcessNote's per-note state; the synthesis touches only the fields up
// to curVolRight.
typedef struct NoteSynthesisState {
    /* 0x00 */ u8 restart;        // a loop restart is pending; clears once A_SETLOOP is emitted
    /* 0x01 */ u8 sampleDmaIndex; // AudioLoad_DmaSampleData's remembered DMA slot, passed by address
    /* 0x02 */ u8 prevHaasEffectLeftDelaySize;
    /* 0x03 */ u8 prevHaasEffectRightDelaySize;
    /* 0x04 */ u8 reverbVol;           // last update's NoteSampleState.reverbVol
    /* 0x05 */ u8 numParts;            // last update's stream count, 1 or 2
    /* 0x06 */ u16 samplePosFrac;      // the low half of the 16.16 position accumulator
    /* 0x08 */ s32 samplePosInt;       // read position in source samples
    /* 0x0C */ void* synthesisBuffers; // addressed physically, with a per-buffer bias
    /* 0x10 */ u16 curVolLeft;         // the volume the mix has reached, NoteSampleState.targetVolLeft * 16 scale
    /* 0x12 */ u16 curVolRight;
    /* 0x14 */ char pad14[0xC];
} NoteSynthesisState; // size = 0x20

// OoT's NotePlaybackState (0x88 there: a bigger attributes block): priority, layers, the attributes kept after
// the layer lets go, and the envelope, portamento and vibrato.
typedef struct NotePlaybackState {
    /* 0x00 */ u8 priority;
    /* 0x01 */ u8 waveId;
    /* 0x02 */ u8 harmonicIndex;
    /* 0x03 */ u8 fontId;
    /* 0x04 */ u8 status; // MM's name (OoT's unk_04): 0 normal, 1 decaying, 2 releasing (Audio_SeqLayerDecayRelease)
    /* 0x05 */ char pad5[1];
    /* 0x06 */ s16 adsrVolScaleUnused;
    /* 0x08 */ f32 portamentoFreqScale;
    /* 0x0C */ f32 vibratoFreqScale;
    /* 0x10 */ SequenceLayer* prevParentLayer;
    /* 0x14 */ SequenceLayer* parentLayer;       // the layer this note is playing; -1 means none
    /* 0x18 */ SequenceLayer* wantedParentLayer; // a layer queued to take this note over; -1 means none
    /* 0x1C */ NoteAttributes attributes;
    /* 0x30 */ AdsrState adsr;
    /* 0x50 */ Portamento portamento;
    /* 0x5C */ VibratoState vibratoState;
} NotePlaybackState; // size = 0x78

// OoT's Note (0xE0 there: no startSamplePos, a smaller attributes block). listItem threads it onto one of its
// note pool's four lists; sampleState is what the synthesis reads.
typedef struct Note {
    /* 0x00 */ AudioListItem listItem;
    /* 0x10 */ NoteSynthesisState synthesisState;
    /* 0x30 */ NotePlaybackState playbackState;
    /* 0xA8 */ char padA8[0x8];
    /* 0xB0 */ NoteSampleState sampleState;
} Note; // size = 0xD0

// OoT's SoundFont (0x14 there: no sound effects), one entry of gAudioCtx.soundFontList; its two counts bound
// Audio_GetInstrumentInner's and Audio_GetDrum's lookups.
typedef struct SoundFont {
    /* 0x00 */ u8 numInstruments;
    /* 0x01 */ u8 numDrums;
    /* 0x02 */ u8 sampleBankId1; // 0xFF for none
    /* 0x03 */ u8 sampleBankId2; // 0xFF for none
    /* 0x04 */ void** instruments;
    /* 0x08 */ void** drums;
} SoundFont; // size = 0xC

// OoT's ReverbRingBufferItem less its three trailing u16s (0x1C there), the cut of a reverb's ring one update
// covers; AudioSynth_InitNextRingBuf fills it.
typedef struct ReverbRingBufferItem {
    /* 0x00 */ s16 numSamplesAfterDownsampling; // samples this slot covers
    /* 0x02 */ s16 chunkLen;                    // the chunk length the slot was cut for
    /* 0x04 */ s16* toDownsampleLeft;
    /* 0x08 */ s16* toDownsampleRight;
    /* 0x0C */ s32 startPos;
    /* 0x10 */ s16 lengthA;
    /* 0x12 */ s16 lengthB;
} ReverbRingBufferItem; // size = 0x14

// OoT's SynthesisReverb (0x2C8 there: no unk_14..unk_1B or items2; MM's names for most of OoT's unknowns), one
// per reverb, built by AudioHeap_Init from its ReverbSettings.
// Load-bearing: mixReverbStrength, volume, leakRtl and leakLtr as u16 (OoT: s16).
typedef struct SynthesisReverb {
    /* 0x000 */ u8 resampleFlags;
    /* 0x001 */ u8 useReverb;
    /* 0x002 */ u8 framesToIgnore;
    /* 0x003 */ u8 curFrame;       // selects the live half of items
    /* 0x004 */ u8 downsampleRate; // also the source-sample stride
    /* 0x005 */ s8 mixReverbIndex; // the reverb this one mixes into, -1 for none
    /* 0x006 */ u16 windowSize;
    /* 0x008 */ u16 mixReverbStrength;
    /* 0x00A */ u16 volume;
    /* 0x00C */ u16 decayRatio; // determines how much reverb persists
    /* 0x00E */ u16 downsamplePitch;
    /* 0x010 */ u16 leakRtl;
    /* 0x012 */ u16 leakLtr;
    /* 0x014 */ s32 nextRingBufPos; // write cursor into the two rings
    /* 0x018 */ s32 unused18;       // zeroed with nextRingBufPos, never read
    /* 0x01C */ s32 bufSizePerChan; // length of those rings, in samples
    /* 0x020 */ s16* leftRingBuf;
    /* 0x024 */ s16* rightRingBuf;
    /* 0x028 */ void* leftLoadResampleBuf;
    /* 0x02C */ void* rightLoadResampleBuf;
    /* 0x030 */ void* leftSaveResampleBuf; // this pair is allocated, never used
    /* 0x034 */ void* rightSaveResampleBuf;
    /* 0x038 */ ReverbRingBufferItem items[2][5];
    /* 0x100 */ s16* filterLeft; // NULL = no filter
    /* 0x104 */ s16* filterRight;
    /* 0x108 */ s16* filterLeftState;
    /* 0x10C */ s16* filterRightState;
    /* 0x110 */ TunedSample tunedSample;
    /* 0x118 */ Sample sample; // the ring buffer, played back as a codec-4 (reverb) sample
    /* 0x128 */ AdpcmLoop loop;
} SynthesisReverb; // size = 0x158

// OoT's AudioAllocPool, a bump allocator; AudioHeap_InitPool aligns its base up to 16 and shrinks its size to fit.
typedef struct AudioAllocPool {
    /* 0x00 */ u8* startRamAddr; // 16-aligned base of the region
    /* 0x04 */ u8* curRamAddr;   // next free byte
    /* 0x08 */ s32 size;
    /* 0x0C */ s32 numEntries; // allocations handed out so far
} AudioAllocPool;              // size = 0x10

// OoT's AudioCacheEntry: one cached sequence, font or sample bank (tableType, id) at ramAddr.
typedef struct AudioCacheEntry {
    /* 0x00 */ u8* ramAddr;
    /* 0x04 */ s32 size;
    /* 0x08 */ s16 tableType;
    /* 0x0A */ s16 id; // -1 when the entry is free
} AudioCacheEntry;     // size = 0xC

// The two halves of OoT's AudioCache: the persistent cache, then the temporary one right behind it.
typedef struct AudioPersistentCache {
    /* 0x00 */ u32 numEntries;
    /* 0x04 */ AudioAllocPool pool;
    /* 0x14 */ AudioCacheEntry entries[16];
} AudioPersistentCache; // size = 0xD4

typedef struct AudioTemporaryCache {
    /* 0x00 */ u32 nextSide; // which of the two entries the next allocation reuses
    /* 0x04 */ AudioAllocPool pool;
    /* 0x14 */ AudioCacheEntry entries[2]; // 0 grows up from the pool's start, 1 down from its end
} AudioTemporaryCache;                     // size = 0x2C

// OoT's AudioCache, one table type's pair: AudioContext's seqCache, fontCache and sampleBankCache.
typedef struct AudioCache {
    /* 0x000 */ AudioPersistentCache persistent;
    /* 0x0D4 */ AudioTemporaryCache temporary;
    /* 0x100 */ char pad100[0x10]; // OoT unk_100
} AudioCache;                      // size = 0x110

// OoT's SampleCacheEntry, a block (allocatedAddr) a sample cache lends a sample.
typedef struct SampleCacheEntry {
    /* 0x00 */ s8 inUse;
    /* 0x01 */ s8 origMedium;
    /* 0x02 */ s8 sampleBankId;
    /* 0x03 */ char pad3[0x5];
    /* 0x08 */ void* allocatedAddr;
    /* 0x0C */ s32 sampleAddr; // where the sample lived before it was cached
    /* 0x10 */ s32 size;
} SampleCacheEntry; // size = 0x14

// OoT's AudioSampleCache: a pool and the entries carved from it.
typedef struct AudioSampleCache {
    /* 0x000 */ AudioAllocPool pool;
    /* 0x010 */ SampleCacheEntry entries[32];
    /* 0x290 */ s32 numEntries; // the live-entry count, immediately past the last entry
} AudioSampleCache;             // size = 0x294

// OoT's three pool splits: the sizes AudioHeap_Init sets in gAudioCtx and passes to the pool initialisers.
typedef struct AudioSessionPoolSplit {
    /* 0x00 */ u32 miscPoolSize;
    /* 0x04 */ u32 unkSizes[2];
    /* 0x0C */ u32 cachePoolSize;
} AudioSessionPoolSplit; // size = 0x10

typedef struct AudioCachePoolSplit {
    /* 0x00 */ u32 persistentCommonPoolSize;
    /* 0x04 */ u32 temporaryCommonPoolSize;
} AudioCachePoolSplit; // size = 0x8

typedef struct AudioCommonPoolSplit {
    /* 0x00 */ u32 seqCacheSize;
    /* 0x04 */ u32 fontCacheSize;
    /* 0x08 */ u32 sampleBankCacheSize;
} AudioCommonPoolSplit; // size = 0xC

// OoT's ReverbSettings (0x18 there: no unk_6/unk_8, a byte windowSize), one per reverb of an AudioSpec;
// mixReverbIndex and mixReverbStrength are MM's names.
typedef struct ReverbSettings {
    /* 0x00 */ u8 downsampleRate;
    /* 0x01 */ u8 windowSize; // in units of 64 samples
    /* 0x02 */ u16 decayRatio;
    /* 0x04 */ u16 volume;
    /* 0x06 */ u16 leakRtl;
    /* 0x08 */ u16 leakLtr;
    /* 0x0A */ s8 mixReverbIndex;
    /* 0x0C */ u16 mixReverbStrength;
    /* 0x0E */ s16 lowPassFilterCutoffLeft;
    /* 0x10 */ s16 lowPassFilterCutoffRight;
} ReverbSettings; // size = 0x12

// OoT's AudioSpec (0x38 there: no unk_07/unk_08 or sampleDmaBufSize1/2), the heap and voice layout
// AudioHeap_Init applies; unk4 and unkC are OoT's unk_04 and unk_14.
typedef struct AudioSpec {
    /* 0x00 */ s32 samplingFrequency;
    /* 0x04 */ u8 unk4;
    /* 0x05 */ u8 numNotes;
    /* 0x06 */ u8 numSequencePlayers;
    /* 0x07 */ u8 numReverbs;
    /* 0x08 */ ReverbSettings* reverbSettings;
    /* 0x0C */ u16 unkC;
    /* 0x10 */ s32 persistentSeqCacheSize;
    /* 0x14 */ s32 persistentFontCacheSize;
    /* 0x18 */ s32 persistentSampleBankCacheSize;
    /* 0x1C */ s32 temporarySeqCacheSize;
    /* 0x20 */ s32 temporaryFontCacheSize;
    /* 0x24 */ s32 temporarySampleBankCacheSize;
    /* 0x28 */ s32 persistentSampleCacheSize;
    /* 0x2C */ s32 temporarySampleCacheSize;
} AudioSpec; // size = 0x30

// OoT's TempoData and AudioHeapInitSizes, na/session_config.c's two const records (one record, AGC, in Animal
// Crossing's port). maxNotes is AC's byte MAXCHAN; OoT reads it and its pad as one unused s16.
typedef struct TempoData {
    /* 0x0 */ s8 maxNotes;         // unused; the most notes any AudioSpec asks for (AC's maxChan)
    /* 0x2 */ s16 seqTicksPerBeat; // AudioHeap_Init derives maxTempo from it (AC's timeBase)
} TempoData;                       // size = 0x4

typedef struct AudioHeapInitSizes {
    /* 0x0 */ u32 heapSize;          // how much of gAudioHeap the driver uses; AudioLoad_Init clears exactly that
    /* 0x4 */ u32 initPoolSize;      // AC's fixSize
    /* 0x8 */ u32 permanentPoolSize; // AC's ememSize; AudioLoad_Init zeroes it when the pool cannot be allocated
} AudioHeapInitSizes;                // size = 0xC

// OoT's AudioBufferParameters, the output-buffer and update-rate settings AudioHeap_Init derives from the spec,
// under OoT's earlier field names (before its "ticks" rename).
typedef struct AudioBufferParameters {
    /* 0x00 */ s16 specUnk4; // AudioSpec.unk4, a buffer-size multiplier; 2 ages the reverbs twice
    /* 0x02 */ u16 samplingFrequency;
    /* 0x04 */ u16 aiSamplingFrequency;
    /* 0x06 */ s16 samplesPerFrameTarget;
    /* 0x08 */ s16 maxAiBufferLength; // AudioHeap_ResetStep stamps it into every aiBufLengths slot
    /* 0x0A */ s16 minAiBufferLength;
    /* 0x0C */ s16 updatesPerFrame; // number of command-list chunks the driver splits a frame into
    // AudioSynth_Update's chunk length: Max or Min once remaining / chunks left reaches it, else samplesPerUpdate.
    /* 0x0E */ s16 samplesPerUpdate;
    /* 0x10 */ s16 samplesPerUpdateMax;
    /* 0x12 */ s16 samplesPerUpdateMin;
    /* 0x14 */ s16 numSequencePlayers; // sequence players in use; bounds the seqPlayers walks
    /* 0x16 */ char pad16[0x2];
    /* 0x18 */ f32 resampleRate;             // 32000 / samplingFrequency, folded into every note's frequency
    /* 0x1C */ f32 updatesPerFrameInv;       // also every note release's fade-out rate
    /* 0x20 */ f32 updatesPerFrameInvScaled; // updatesPerFrameInv / 256, AudioHeap_InitAdsrDecayTable's scale
    /* 0x24 */ f32 updatesPerFrameScaled;    // updatesPerFrame / 4, the scale of every envelope point's delay
} AudioBufferParameters;                     // size = 0x28

// OoT's DmaHandler, the type of load.c's two hooks: sDmaHandler (osEPiStartDma, which AudioLoad_Dma calls) and
// the never-called sUnusedHandler.
typedef s32 (*DmaHandler)(OSPiHandle* handle, OSIoMesg* mb, s32 direction);

// OoT's Sample.medium and AudioTableEntry.medium values (AudioLoad_RelocateSample's 0 and 1 are bank numbers).
typedef enum SampleMedium {
    /* 0 */ MEDIUM_RAM,
    /* 1 */ MEDIUM_UNK,
    /* 2 */ MEDIUM_CART,
    /* 3 */ MEDIUM_DISK_DRIVE
} SampleMedium;

// OoT's Sample.codec values.
typedef enum SampleCodec {
    /* 0 */ CODEC_ADPCM, // 16 samples in 9 bytes: 4-bit samples + 1 header byte
    /* 1 */ CODEC_S8,    // 16 samples in 16 bytes: 8-bit samples
    /* 2 */ CODEC_S16_INMEMORY,
    /* 3 */ CODEC_SMALL_ADPCM, // 16 samples in 5 bytes: 2-bit samples + 1 header byte
    /* 4 */ CODEC_REVERB,
    /* 5 */ CODEC_S16
} SampleCodec;

// OoT's table types, the `tableType` the cache and load functions take.
typedef enum SampleBankTableType {
    /* 0 */ SEQUENCE_TABLE,
    /* 1 */ FONT_TABLE,
    /* 2 */ SAMPLE_TABLE
} SampleBankTableType;

// OoT's cache selectors for AudioHeap_AllocCached and AudioHeap_SearchCaches.
typedef enum AudioCacheType {
    /* 0 */ CACHE_TEMPORARY,
    /* 1 */ CACHE_PERSISTENT,
    /* 2 */ CACHE_EITHER,
    /* 3 */ CACHE_PERMANENT
} AudioCacheType;

// OoT's AudioTableEntry.cachePolicy values, the cache a table entry is loaded into.
typedef enum AudioCacheLoadType {
    /* 0 */ CACHE_LOAD_PERMANENT,
    /* 1 */ CACHE_LOAD_PERSISTENT,
    /* 2 */ CACHE_LOAD_TEMPORARY,
    /* 3 */ CACHE_LOAD_EITHER,
    /* 4 */ CACHE_LOAD_EITHER_NOSYNC // never loaded: AudioLoad_TrySyncLoadSampleBank returns the ROM address
} AudioCacheLoadType;

// OoT's states of the seq, font and sample-bank load-status bytes.
typedef enum AudioLoadStatus {
    /* 0 */ LOAD_STATUS_NOT_LOADED,
    /* 1 */ LOAD_STATUS_IN_PROGRESS, // being loaded asynchronously
    /* 2 */ LOAD_STATUS_COMPLETE,
    /* 3 */ LOAD_STATUS_DISCARDABLE,
    /* 4 */ LOAD_STATUS_MAYBE_DISCARDABLE, // fonts only: like COMPLETE, but discarded first
    /* 5 */ LOAD_STATUS_PERMANENTLY_LOADED // in the permanent pool; never reset or discarded
} AudioLoadStatus;

// OoT's AudioTableEntry, one entry of a sequence, font or sample-bank table.
typedef struct AudioTableEntry {
    /* 0x00 */ s32 romAddr;
    /* 0x04 */ s32 size;
    /* 0x08 */ s8 medium;
    /* 0x09 */ s8 cachePolicy; // AudioCacheLoadType
    /* 0x0A */ s16 shortData1; // fonts: sample-bank ids 1 (high byte) and 2
    /* 0x0C */ s16 shortData2; // fonts: instrument count (high byte) and drum count
    /* 0x0E */ s16 shortData3;
} AudioTableEntry; // size = 0x10

// OoT's AudioTableHeader, the 0x10 bytes in front of an AudioTable's entries.
typedef struct AudioTableHeader {
    /* 0x00 */ s16 numEntries;
    /* 0x02 */ s16 unkMediumParam;
    /* 0x04 */ s32 romAddr;
    /* 0x08 */ char pad8[0x8];
} AudioTableHeader; // size = 0x10

// OoT's AudioTable.
typedef struct AudioTable {
    /* 0x00 */ AudioTableHeader header;
    /* 0x10 */ AudioTableEntry entries[1];
} AudioTable; // size >= 0x20

// OoT's SampleDma, one record of gAudioCtx.sampleDmas; AudioLoad_DecreaseSampleDmaTtls queues it for reuse
// when its ttl runs out.
typedef struct SampleDma {
    /* 0x00 */ void* ramAddr; // the record's buffer, sampleDmaBufSize bytes of it
    /* 0x04 */ s32 devAddr;
    /* 0x08 */ s16 sizeUnused;
    /* 0x0A */ u16 size; // == sampleDmaBufSize at the time the record was built
    /* 0x0C */ u8 unused;
    /* 0x0D */ u8 reuseIndex;
    /* 0x0E */ u8 ttl;
    /* 0x0F */ char padF[0x1];
} SampleDma; // size = 0x10

// A font file's header in RAM (OoT keeps the pointer `void`; the fields are ours), file-relative offsets until
// AudioLoad_RelocateFont turns them into pointers. The counts come from gAudioCtx.soundFontList.
typedef struct SoundFontData {
    /* 0x00 */ Drum** drums;
    /* 0x04 */ Instrument* instruments[1];
} SoundFontData; // size >= 0x8

// OoT load.c's SampleBankRelocInfo: the id, load address and medium of each of a font's two sample banks.
typedef struct SampleBankRelocInfo {
    /* 0x00 */ s32 sampleBankId1; // 0xFF for none
    /* 0x04 */ s32 sampleBankId2; // 0xFF for none
    /* 0x08 */ s32 baseAddr1;
    /* 0x0C */ s32 baseAddr2;
    /* 0x10 */ s32 medium1;
    /* 0x14 */ s32 medium2;
} SampleBankRelocInfo; // size = 0x18

// OoT's AudioAsyncLoad, one of the 16 transfer jobs at gAudioCtx.asyncLoads. YS has no unkMediumParam, so
// retMsg and everything after it sit 4 lower.
typedef struct AudioAsyncLoad {
    /* 0x00 */ s8 status; // 0 free, 1 in use
    /* 0x01 */ s8 delay;
    /* 0x02 */ s8 medium;
    /* 0x03 */ char pad3[0x1];
    /* 0x04 */ u8* ramAddr;
    /* 0x08 */ u32 curDevAddr;
    /* 0x0C */ u8* curRamAddr;
    /* 0x10 */ u32 bytesRemaining;
    /* 0x14 */ u32 chunkSize; // per call: size / nChunks rounded up to 0x100 (0x1000 for nChunks 0)
    /* 0x18 */ OSMesg retMsg; // a packed word echoed on retQueue at the end; the OSMesg type is load-bearing
    /* 0x1C */ OSMesgQueue* retQueue;
    /* 0x20 */ OSMesgQueue msgQueue; // this job's own completion queue
    /* 0x38 */ OSMesg msg;
    /* 0x3C */ OSIoMesg ioMesg; // the I/O message AudioLoad_Dma fills in
} AudioAsyncLoad;               // size = 0x54

// OoT's AudioSlowLoad, one half of gAudioCtx.slowLoads. YS has no unkMediumParam, so curDevAddr and everything
// after it sit 4 lower.
typedef struct AudioSlowLoad {
    /* 0x00 */ u8 medium;
    /* 0x01 */ u8 seqOrFontId;
    /* 0x02 */ u8 instId;
    /* 0x03 */ char pad3[0x1];
    /* 0x04 */ u32 curDevAddr;
    /* 0x08 */ u8* curRamAddr;
    /* 0x0C */ u8* ramAddr;
    /* 0x10 */ s32 state;            // SlowLoadState (load.c)
    /* 0x14 */ s32 bytesRemaining;   // AudioLoad_ProcessSlowLoads drains it 0x400 at a time
    /* 0x18 */ s8* status;           // caller's completion flag, set to 1 when bytesRemaining reaches zero
    /* 0x1C */ Sample sample;        // the request's sample, copied in whole
    /* 0x2C */ OSMesgQueue msgQueue; // this half's own DMA completion queue
    /* 0x44 */ OSMesg msg;
    /* 0x48 */ OSIoMesg ioMesg; // the I/O message AudioLoad_Dma fills in
} AudioSlowLoad;                // size = 0x60

// OoT's AudioPreloadReq, one record of gAudioCtx.preloadSampleStack: once `sample` has loaded,
// AudioLoad_ProcessSamplePreloads points it at ramAddr if its end + medium still equals endAndMediumKey.
typedef struct AudioPreloadReq {
    /* 0x00 */ void* endAndMediumKey;
    /* 0x04 */ Sample* sample;
    /* 0x08 */ void* ramAddr;
    /* 0x0C */ s32 encodedInfo; // handed straight through to AudioLoad_StartAsyncLoad as its retMsg
    /* 0x10 */ s32 isFree;      // 1 once the record has been dealt with
} AudioPreloadReq;              // size = 0x14

// OoT's AudioTask, the RSP task AudioThread_UpdateImpl builds each frame (gAudioCtx.rspTask, double-buffered).
typedef struct AudioTask {
    /* 0x00 */ OSTask task;
    /* 0x40 */ OSMesgQueue* msgQueue;
    /* 0x44 */ void* unk44;
    /* 0x48 */ char pad48[8];
} AudioTask; // size = 0x50

// OoT's AudioCustomUpdateFunction, the hook gAudioCustomUpdateFunction holds.
typedef void (*AudioCustomUpdateFunction)(void);

// OoT's AudioCmd, one slot of the command ring gAudioCtx.threadCmdBuf: an opcode, three argument bytes and a
// payload word the posters fill left-aligned (an s8 in the top byte, a u16 in the top half), read at every width.
typedef struct AudioCmd {
    /* 0x00 */ u8 op;
    /* 0x01 */ u8 arg0;
    /* 0x02 */ u8 arg1;
    /* 0x03 */ u8 arg2;
    union {
        /* 0x04 */ s32 asInt;
        /* 0x04 */ u16 asUShort;
        /* 0x04 */ s8 asSbyte;
        /* 0x04 */ u8 asUbyte;
        /* 0x04 */ f32 asFloat;
    } payload; // anonymous in OoT
} AudioCmd;    // size = 0x8

// OoT's AudioContext, the driver's work area (the library reads its seqPlayers too): OoT's fields in OoT's order,
// with some members dropped and arrays shortened. na/session_config.c defines gAudioCtx.
// Load-bearing: the seven uses of the six gAudioCtx<Field> aliases below the struct (undefined_syms.ld).
struct Note;
struct NoteSampleState;
struct SampleDma;
struct SoundFont;
struct AudioTable;

typedef struct AudioContext {
    /* 0x0000 */ char pad0[0x1];
    /* 0x0001 */ s8 numSynthesisReverbs; // how many synthesisReverbs are live
    /* 0x0002 */ s16 unused2;            // OoT unk_2: AudioSpec.unkC, never read
    /* 0x0004 */ s16 unused4;            // OoT unk_4: AudioHeap_Init sets 0x1000, never read
    /* 0x0006 */ char pad2[0xA];
    /* 0x0010 */ s32 curLoadedBook; // the ADPCM book last loaded; AudioSynth_Update clears it every frame
    // The NoteSampleStates the synthesis reads, 0x20 each; this instance's first is at index sampleStateOffset.
    /* 0x0014 */ struct NoteSampleState* sampleStates;
    // OoT has four; only one fits before the next known field.
    /* 0x0018 */ SynthesisReverb synthesisReverbs[1];
    /* 0x0170 */ char pad170[0x10];
    // Counted by numUsedSamples, appended by AudioLoad_AddUsedSample. The 0x80 length is a guess.
    /* 0x0180 */ Sample* usedSamples[0x80];
    // A stack, topped at preloadSampleStackTop; its 0x80 length is bounded by numUsedSamples, not measured.
    /* 0x0380 */ AudioPreloadReq preloadSampleStack[0x80];
    /* 0x0D80 */ s32 numUsedSamples;
    /* 0x0D84 */ s32 preloadSampleStackTop;
    /* 0x0D88 */ AudioAsyncLoad asyncLoads[16];
    // slowLoadPos picks the half AudioLoad_SlowLoadSeq fills, toggled ^1 on the way out.
    /* 0x12C8 */ s32 slowLoadPos;
    /* 0x12CC */ AudioSlowLoad slowLoads[2];
    // The two PI handles AudioLoad_Dma picks between by medium (cart or disk drive).
    /* 0x138C */ OSPiHandle* cartHandle;
    /* 0x1390 */ OSPiHandle* driveHandle;
    /* 0x1394 */ OSMesgQueue externalLoadQueue;
    /* 0x13AC */ OSMesg externalLoadMsgBuf[8];
    // Where the preload stack's async loads report back; AudioLoad_ProcessSamplePreloads polls it.
    /* 0x13CC */ OSMesgQueue preloadSampleQueue;
    /* 0x13E4 */ OSMesg preloadSampleMsgBuf[8];
    // The frame's sample DMAs (AudioLoad_DmaSampleData), which AudioThread_UpdateImpl drains every frame.
    /* 0x1404 */ OSMesgQueue curAudioFrameDmaQueue;
    /* 0x141C */ OSMesg curAudioFrameDmaMsgBuf[64];
    // One OSIoMesg per sample DMA, taken by curAudioFrameDmaCount; 64 as in OoT (nothing observed bounds it).
    /* 0x151C */ OSIoMesg curAudioFrameDmaIoMsgBuf[64];
    // AudioLoad_SyncDma's channel: one transfer at a time.
    /* 0x1B1C */ OSMesgQueue syncDmaQueue;
    /* 0x1B34 */ OSMesg syncDmaMesg;
    /* 0x1B38 */ OSIoMesg syncDmaIoMesg;
    // sampleDmaCount SampleDma records, the first sampleDmaListSize1 short-lived; AudioLoad_DecreaseSampleDmaTtls
    // queues an expired one on reuse queue 1 or 2.
    /* 0x1B50 */ struct SampleDma* sampleDmas;
    /* 0x1B54 */ u32 sampleDmaCount;
    /* 0x1B58 */ u32 sampleDmaListSize1;
    /* 0x1B5C */ s32 unused1B5C; // OoT unused2628: AudioLoad_DecreaseSampleDmaTtls zeroes it, never read
    /* 0x1B60 */ u8 sampleDmaReuseQueue1[0x100];
    /* 0x1C60 */ u8 sampleDmaReuseQueue2[0x100];
    /* 0x1D60 */ u8 sampleDmaReuseQueue1RdPos;
    /* 0x1D61 */ u8 sampleDmaReuseQueue2RdPos;
    /* 0x1D62 */ u8 sampleDmaReuseQueue1WrPos;
    /* 0x1D63 */ u8 sampleDmaReuseQueue2WrPos;
    // OoT's three load tables, which AudioLoad_GetLoadTable hands out by table type.
    /* 0x1D64 */ struct AudioTable* sequenceTable;
    /* 0x1D68 */ struct AudioTable* soundFontTable;
    /* 0x1D6C */ struct AudioTable* sampleBankTable;
    // The sequence-to-font map: a u16 offset per sequence, then each one's length-prefixed font list.
    /* 0x1D70 */ u8* sequenceFontTable;
    /* 0x1D74 */ s16 numSequences;
    /* 0x1D76 */ char pad1D76[2];
    /* 0x1D78 */ struct SoundFont* soundFontList; // one SoundFont per font id
    /* 0x1D7C */ AudioBufferParameters audioBufferParameters;
    /* 0x1DA4 */ s32 sampleDmaBufSize; // one SampleDma buffer: 0x400 for the first group, 0x500 the second
    /* 0x1DA8 */ s32 maxAudioCmds;
    /* 0x1DAC */ s32 numNotes;       // number of driver voices in the notes array
    /* 0x1DB0 */ s16 maxTempo;       // derived from gTempoData; divides the portamento speed
    /* 0x1DB2 */ s8 soundOutputMode; // SoundOutputMode (OoT's soundMode)
    /* 0x1DB3 */ char pad1DB3[1];
    /* 0x1DB4 */ s32 totalTaskCount;
    /* 0x1DB8 */ s32 curAudioFrameDmaCount; // DMA requests this frame; reset by AudioThread_UpdateImpl
    /* 0x1DBC */ s32 rspTaskIndex;
    /* 0x1DC0 */ s32 curAiBufIndex; // which of the aiBuffers is being filled
    /* 0x1DC4 */ s32 abiCmdBufs[2];
    /* 0x1DCC */ s32 curAbiCmdBuf;
    /* 0x1DD0 */ AudioTask* curTask;
    // OSTask holds a long long, so AudioTask is 8-aligned: hence the hole at 0x1DD4 and the 0x57F8 size.
    /* 0x1DD8 */ AudioTask rspTask[2];
    /* 0x1E78 */ f32 maxTempoTvTypeFactors;
    /* 0x1E7C */ s32 refreshRate;
    /* 0x1E80 */ s16* aiBuffers[3]; // the driver's three output buffers
    /* 0x1E8C */ s16 aiBufLengths[4];
    // The driver's random value, re-stirred every update from osGetCount() and the output (na_se.c reads it too).
    /* 0x1E94 */ u32 audioRandom;
    /* 0x1E98 */ u32 audioErrorFlags; // last error: a code in the top byte, the offending ids below it
    // Frames into a shutdown (AudioThread_PreNMIInternal sets 1); AudioLoad_Dma runs only below 0x11.
    // Load-bearing: the `volatile`, as in OoT.
    /* 0x1E9C */ volatile u32 resetTimer;
    /* 0x1EA0 */ char pad1EA0[0x8];
    // OoT's pools, caches and pool splits, at OoT's offsets relative to each other.
    /* 0x1EA8 */ AudioAllocPool sessionPool;
    /* 0x1EB8 */ AudioAllocPool externalPool; // a non-NULL startRamAddr lets AudioLoad_InitSampleDmaBuffers use it
    /* 0x1EC8 */ AudioAllocPool initPool;
    /* 0x1ED8 */ AudioAllocPool miscPool;
    /* 0x1EE8 */ char pad1EE8[0x20]; // OoT: probably two unused pools
    /* 0x1F08 */ AudioAllocPool cachePool;
    /* 0x1F18 */ AudioAllocPool persistentCommonPool;
    /* 0x1F28 */ AudioAllocPool temporaryCommonPool;
    /* 0x1F38 */ AudioCache seqCache;
    /* 0x2048 */ AudioCache fontCache;
    /* 0x2158 */ AudioCache sampleBankCache;
    /* 0x2268 */ AudioAllocPool permanentPool; // its numEntries counts the permanentCache records
    // OoT's AudioCacheEntry permanentCache[32], typed through PERMANENT_CACHE_ENTRY. Load-bearing: the u8 array.
    /* 0x2278 */ u8 permanentCache[0x180];
    /* 0x23F8 */ AudioSampleCache persistentSampleCache;
    /* 0x268C */ AudioSampleCache temporarySampleCache;
    /* 0x2920 */ AudioSessionPoolSplit sessionPoolSplit;
    /* 0x2930 */ AudioCachePoolSplit cachePoolSplit;
    /* 0x2938 */ AudioCommonPoolSplit persistentCommonPoolSplit;
    /* 0x2944 */ AudioCommonPoolSplit temporaryCommonPoolSplit;
    // AudioLoadStatus by id (0..0x45, 0xFF none); the AudioLoad_Set*LoadStatus setters leave a PERMANENTLY_LOADED
    // one alone.
    /* 0x2950 */ u8 sampleFontLoadStatus[0x46];
    /* 0x2996 */ u8 fontLoadStatus[0x46];
    /* 0x29DC */ u8 seqLoadStatus[0x46];
    // Load-bearing: the `volatile`, as in OoT.
    /* 0x2A22 */ volatile u8 resetStatus; // shutdown/restart phase, counted down 5 -> 0 by AudioHeap_ResetStep
    /* 0x2A23 */ u8 specId;
    /* 0x2A24 */ s32 audioResetFadeOutFramesLeft;     // frames left in the current shutdown phase
    /* 0x2A28 */ f32* adsrDecayTable;                 // 256 decay rates from AudioHeap_InitAdsrDecayTable, [0] = 0
    /* 0x2A2C */ struct Note* notes;                  // the driver voice array, numNotes entries of 0xD0
    /* 0x2A30 */ SequencePlayer seqPlayers[3];        // three (OoT: four): the layer pool starts where the third ends
    /* 0x2E50 */ SequenceLayer sequenceLayers[64];    // the layers, idle ones on layerFreeList
    /* 0x4E50 */ SequenceChannel sequenceChannelNone; // the placeholder in an unused channel slot
    /* 0x4F28 */ s32 sampleStateOffset;
    /* 0x4F2C */ AudioListItem layerFreeList; // the idle sequenceLayers
    // The driver-wide note pool (OoT's noteFreeLists): [0] disabled, [1] decaying, [2] releasing, [3] active; it
    // holds the notes no instrument or wave has reserved.
    /* 0x4F3C */ AudioListItem noteFreeLists[4];
    // The command ring threadCmdBuf: u8 write and read indices that wrap on their own, so 0x100 slots;
    // AudioThread_ResetCmdQueue drops what is pending by moving the read index up.
    /* 0x4F7C */ u8 threadCmdWritePos;
    /* 0x4F7D */ u8 threadCmdReadPos;
    /* 0x4F7E */ u8 threadCmdQueueFinished; // the drain stopped at a 0xF8 break and resumes next frame
    /* 0x4F7F */ char pad4F7F[0x1];
    // Per sequence player: the channels a channel command for channel 0xFF reaches (set by command 0x90).
    /* 0x4F80 */ u16 threadCmdChannelMask[4];
    // The three queues AudioThread_InitMesgQueuesImpl creates: pointers, then the queues and their buffers.
    /* 0x4F88 */ OSMesgQueue* audioResetQueueP;
    /* 0x4F8C */ OSMesgQueue* taskStartQueueP;
    /* 0x4F90 */ OSMesgQueue* threadCmdProcQueueP;
    /* 0x4F94 */ OSMesgQueue taskStartQueue;
    /* 0x4FAC */ OSMesgQueue threadCmdProcQueue;
    /* 0x4FC4 */ OSMesgQueue audioResetQueue;
    /* 0x4FDC */ OSMesg taskStartMsgBuf[1];
    /* 0x4FE0 */ OSMesg audioResetMsgBuf[1];
    /* 0x4FE4 */ OSMesg threadCmdProcMsgBuf[4];
    /* 0x4FF4 */ u32 threadCmdBuf[0x100][2]; // one 8-byte AudioCmd per slot
} AudioContext;                              // size = 0x57F8

// gAudioCtx.seqPlayers under a symbol of its own, for Nai_PortRoutin's and Nai_SeSubIsPlaying's `enabled` reads.
extern SequencePlayer gAudioCtxSeqPlayers[]; // == gAudioCtx.seqPlayers
// gAudioCtx.fontCache.temporary.entries[0].id under a symbol of its own (AudioLoad_DiscardFont's first read).
extern s16 gAudioCtxFontCacheTemporaryEntry0Id;
// gAudioCtx.persistentSampleCache under a symbol of its own (AudioHeap_AllocPersistentSampleCacheEntry's entry).
extern AudioSampleCache gAudioCtxPersistentSampleCache;
// gAudioCtx.persistentSampleCache.pool.size under a symbol of its own (AudioHeap_InitSampleCaches' first store).
extern s32 gAudioCtxPersistentSampleCachePoolSize;
// Two more under symbols of their own (AudioThread_UpdateImpl's second modulo).
extern s32 gAudioCtxTotalTaskCount; // == gAudioCtx.totalTaskCount
extern s16 gAudioCtxSpecUnk4;       // == gAudioCtx.audioBufferParameters.specUnk4

// Entry i of gAudioCtx.permanentCache, typed (the field is bytes).
#define PERMANENT_CACHE_ENTRY(i) (&((AudioCacheEntry*)gAudioCtx.permanentCache)[i])

// The two NoteSampleState reset templates: all zero, and with the top two flag bits set.
extern NoteSampleState gZeroNoteSampleState;
extern NoteSampleState gDefaultNoteSampleState;
// The synthetic waves (sawtooth, triangle, sine, square, white noise, bell, eighth pulse, quarter pulse twice),
// four 64-sample harmonics each. AudioThread_UpdateImpl points entry 8 into its own code every update, as noise.
extern s16* gWaveSamples[];
// The Haas-effect delay per pan position (read forwards for the left, backwards for the right) and the three
// output modes' pan volume curves.
extern u16 gHaasEffectDelaySizes[0x40];
extern f32 gHeadsetPanVolume[0x80];
extern f32 gStereoPanVolume[0x80];
extern f32 gDefaultPanVolume[0x80];

// 256 pitch ratios from 0.5 to 2.0, an octave either way, with 1.0f at entry 128. Every user indexes it by a
// signed offset + 128: the portamento, the vibrato and the channel bend event 0xD3.
extern f32 gBendPitchOneOctaveFrequencies[];
// The fine pitch-bend curve, shaped like gBendPitchOneOctaveFrequencies but running 0.891 to 1.122, one
// semitone either way. The bend events 0xEE and 0xCE read it, 0xD3 reads the octave one.
extern f32 gBendPitchTwoSemitonesFrequencies[];
// 128 pitch ratios, one per key: entry k is 2^((k - 39) / 12), so entry 39 is 1.0.
extern f32 gPitchFrequencies[];
// The envelope a channel starts out with, and the two 16-entry tables a sequence player's shortNoteVelocityTable
// and shortNoteGateTimeTable are reset to.
extern EnvelopePoint gDefaultEnvelope[];
extern u8 gDefaultShortNoteVelocityTable[];
extern u8 gDefaultShortNoteGateTimeTable[];
// data.c's s16[64] (MM's name, OoT's D_8012FBA8): AudioSynth_ProcessNote loads &gInvalidAdpcmCodeBook[1] as the
// codebook when a voice's book offset is 1.
extern s16 gInvalidAdpcmCodeBook[];
// Two tables of 8-halfword filter coefficient rows.
extern s16 gLowPassFilterData[];
extern s16 gHighPassFilterData[];
// The tables AudioLoad_Init hands gAudioCtx (sequence, sound font, sample bank, sequence-to-font map), then the
// ROM starts of the three segments AudioLoad_InitTable adds to their entries. gSoundFontTable starts where the
// ucode data ends: AudioThread_UpdateImpl measures the data from aspMainDataStart to it (OoT: to aspMainDataEnd).
extern s16 gSequenceTable[];
extern s16 gSoundFontTable[];
extern u8 gSampleBankTable[];
extern u8 gSequenceFontTable[];
extern u8 D_4F3930[];
extern u8 D_B58B0[];
extern u8 D_E1AD0[];
// aspMainTextStart and aspMainDataStart, the audio microcode, are declared by PR/ucode.h.

// Functions

// na/na_main
void Na_Initialize(void);
void Na_GameFrameWork(void);
void Na_PauseOn(void);
void Na_PauseOff(void);
void Na_SetSoundOutputMode(s32 soundSetting);

// na/na_scene
void Na_SceneChange(s32 scene);
s32 Na_BgmFollowsYoshiStatus(void);

// na/nai_port_buffer
void Nai_PortBufferClear(void);
s32 Nai_PortRampStep(NaiPort* port);
void Nai_PortBufferFree(NaiPort* port);
NaiPort* Nai_PortBufferAlloc(void);
void Nai_PortRampCalcDelta(NaiPort* port);
NaiPort* Nai_PortBufferGetHead(void);
NaiPort* Nai_PortBufferGetTail(void);
void Nai_PortBufferFreeBgm(struct NaiBgm* bgm);
void Nai_PortBufferFreeSe(NaiSe* se);
s32 Nai_PortBufferHasType(s32 type, struct NaiBgm* bgm);

// na/nai_bgm_start
NaiBgm* Nai_BgmStart(s16 num, s16 time, s32 alongside);
void Nai_BgmRestart(NaiBgm* bgm, s16 time, s32 alongside);
NaiBgm* Nai_BgmStartWaiting(s16 num, s16 time, s32 alongside);
void Nai_BgmRestartWaiting(NaiBgm* bgm, s16 time, s32 alongside);
void Nai_BgmStop(NaiBgm* bgm, s16 time);
void Nai_BgmStopKeep(NaiBgm* bgm, s16 time);
void Nai_BgmRelease(NaiBgm* bgm);
s32 Nai_BgmIsActive(NaiBgm* bgm);
void Nai_BgmWaitForGroup(NaiBgm* bgm, s16 time, s32 alongside);
void Nai_BgmTakeGroup(NaiBgm* bgm, s16 time, s32 alongside);

// na/nai_bgm
void Nai_BgmInit(void);
void Nai_BgmRoutin(void);
void Nai_BgmBufferClear(void);
NaiBgm* Nai_BgmBufferAlloc(void);
void Nai_BgmResetCurrent(NaiBgm* bgm);

// na/nai_bgm_group
s32 Nai_BgmGroupFindFree(void);
s16 Nai_BgmGroupRegister(NaiBgm* bgm);
void Nai_BgmGroupUnregister(s16 group);
void Nai_BgmGroupUnregisterBgm(NaiBgm* bgm);
void Nai_BgmGroupClear(void);
NaiBgm* Nai_BgmGroupGetOpposite(NaiBgm* bgm);
NaiBgm* Nai_BgmGroupGet(s16 group);
s32 Nai_BgmAnotherGroup(s16 group);

// na/nai_bgm_wait
NaiBgmWaitNode* Nai_BgmWaitNodeAlloc(void);
void Nai_BgmWaitNodeFree(NaiBgmWaitNode* node);
void Nai_BgmWaitAdd(NaiBgm* bgm);
void Nai_BgmWaitRemove(NaiBgm* bgm);
NaiBgm* Nai_BgmWaitGetHead(void);
void Nai_BgmWaitClear(void);

// na/na_se_handle
s32 Na_SeLevelCheck(s32 se, s16 subHandle);
void Na_SeLevelStop(s32 se, s16 subHandle);
void Na_AllSeClear(void);
void Na_PauseOnJingle(void);
void Na_PauseOffJingle(void);
void Na_SeCalcPanVolume(SoundEventParams* params, s16* pan, f32* volume);
void Na_AllSeStop(void);
void Na_LevelSeStop(void);

// na/na_se
f32 Na_Fabs(f32 value);
void Na_SeTrig(s32 sound, SoundEventParams* params);
void Na_SeStart(s32 se, s16 subHandle, SoundEventParams* params);
void Na_SeLevelChange(s32 se, s16 subHandle, SoundEventParams* params);

// na/na_bgm
void Na_AllBgmClear(void);
void Na_BgmStop(s16 fadeTime);
void Na_SubGameBgmStart(s16 bgmId, s16 fadeTime);
void Na_SubGameBgmStop(s16 fadeTime);
void Na_JingleBgmStart(s16 bgmId, s16 fadeTime);
void Na_BgmInterrupt(void);
void Na_BgmResume(void);
void Na_BgmVolumeUpdate(void);
void Na_BgmChange(s16 bgmId, s16 fadeTime);
void Na_AllBgmStop(void);
s32 Na_SceneBgmGet(void);

// na/na_event
void Na_Se30FruitsClear(void);
void Na_Event(s32 event);

// na/na_yoshi_status
void Na_YoshiStatusChange(s32 status);
void Na_BgmTuneReset(void);
void Na_SetWhichMinobon(s16 which);

// na/nai_port
void Nai_PortInit(void);
void Nai_PortRoutin(void);

// na/nai_ramp
void Nai_StepRamp(NaiPort* port);
void Nai_StepMaskRamp(NaiPort* port);

// na/nai_se
void Nai_SeInit(void);
void Nai_SeRoutin(void);

// na/nai_se_buffer
void Nai_SeGroupStart(void);
void Nai_SeSetPanVolume(NaiSe* se, s16 pan, f32 volume);
void Nai_SeMuteSub(NaiSe* se);
void Nai_SeSendNote(NaiSe* se);
NaiSe* Nai_SeBufferAlloc(void);
void Nai_SeResetCurrent(NaiSe* se);
s32 Nai_SeSubAlloc(NaiSe* se);
void Nai_SeBufferClear(void);
s32 Nai_SeSubIsPlaying(s16 sub);
NaiSe* Nai_SeFindBySub(s16 sub);
f32 Nai_SePriority(NaiSe* se);

// na/nai_se_start
void Nai_SeTrigDefault(s16 sndId);
void Nai_SeStartDefault(s16 sndId);
void Nai_SeTrig(s16 sndId, s16 group, s16 priority, s16 pan, f32 volume);
NaiSe* Nai_SeStart(s16 sndId, s16 group, s16 priority, s16 pan, f32 volume);
void Nai_SeStop(NaiSe* se);
void Nai_SePause(u8 pause);
s32 Nai_SeIsActive(NaiSe* se);

// na/nai_se_set
f32 Nai_SeFabs(f32 value);
NaiPort* Nai_SePortAlloc(NaiSe* se);
void Nai_SePostChanges(NaiSe* se);
void Nai_SeSetVolume(NaiSe* se, f32 volume, s16 time);
void Nai_SeSetPan(NaiSe* se, u8 pan, s16 time);
void Nai_SeSetTune(NaiSe* se, f32 tune, s16 time);
void Nai_SeSetFxmix(NaiSe* se, u8 fxmix, s16 time);

// na/nai_port_write
void Nai_SeqStart(s16 group, s16 num, s16 time);
void Nai_SeqStop(s16 group, s16 time);
void Nai_GrpPortWrite(s16 group, s16 num, s16 value);
void Nai_GrpPortWriteRamp(s16 group, s16 num, s16 start, s16 end, s16 time);
void Nai_SubPortWrite(s16 group, s16 sub, s16 num, s16 value);
void Nai_SubPortWriteRamp(s16 group, s16 sub, s16 num, s16 start, s16 end, s16 time);

// na/nai_bgm_set
f32 Nai_Fabs(f32 value);
NaiPort* Nai_BgmPortAlloc(NaiBgm* bgm);
void Nai_BgmPostChanges(NaiBgm* bgm);
void Nai_BgmSetVolume(NaiBgm* bgm, f32 volume, s16 time);
void Nai_BgmSetTempoOffset(NaiBgm* bgm, s16 tempoOffset, s16 time);
void Nai_BgmSetMuteSub(NaiBgm* bgm, s16 sub, u8 mute);
void Nai_BgmSetMuteSubMask(NaiBgm* bgm, u16 subMask);
void Nai_BgmSetTuneSub(NaiBgm* bgm, s16 sub, u8 on);
void Nai_BgmSetTuneSubMask(NaiBgm* bgm, u16 subMask);
void Nai_BgmSetTune(NaiBgm* bgm, f32 tune, s16 time);
void Nai_BgmSetNoop1(s16 arg0, s16 arg1, s16 arg2, s16 arg3);
void Nai_BgmSetNoop2(s16 arg0, s16 arg1, s16 arg2, s16 arg3);
u16 Nai_SetMaskBit(u16 subMask, s16 bit, u8 on);

// nad/synthesis
Acmd* AudioSynth_Update(Acmd* cmdStart, s32* cmdCnt, s32* aiStart, s32 aiBufLen);

// nad/effects
void Audio_SequenceChannelProcessSound(SequenceChannel* channel, s32 recalculateVolume);
void Audio_SequencePlayerProcessSound(SequencePlayer* seqPlayer);
f32 Audio_GetPortamentoFreqScale(Portamento* portamento);
s16 Audio_GetVibratoPitchChange(VibratoState* vib);
f32 Audio_GetVibratoFreqScale(VibratoState* vib);
void Audio_NoteVibratoUpdate(Note* note);
void Audio_NoteVibratoInit(Note* note);
void Audio_NotePortamentoInit(Note* note);
void Audio_AdsrInit(AdsrState* adsr, EnvelopePoint* envelope, s32 volOut);
f32 Audio_AdsrUpdate(AdsrState* adsr);

// nad/playback
void Audio_InitSampleState(Note* note, NoteSampleState* sampleState, NoteSampleStateAttributes* attrs);
void Audio_NoteSetResamplingRate(NoteSampleState* sampleState, f32 resamplingRateInput);
void Audio_NoteInit(Note* note);
void Audio_NoteDisable(Note* note);
void Audio_ProcessNotes(void);
TunedSample* Audio_GetInstrumentTunedSample(Instrument* instrument, s32 semitone);
void* Audio_GetInstrumentInner(s32 fontId, s32 instId);
void* Audio_GetDrum(s32 fontId, s32 drumId);
void Audio_SeqLayerDecayRelease(SequenceLayer* layer, s32 target);
void Audio_SeqLayerNoteDecay(SequenceLayer* layer);
void Audio_SeqLayerNoteRelease(SequenceLayer* layer);
s32 Audio_BuildSyntheticWave(Note* note, SequenceLayer* layer, s32 waveId);
void Audio_InitSyntheticWave(Note* note, SequenceLayer* layer);
void Audio_InitNoteList(AudioListItem* list);
void Audio_InitNoteLists(AudioListItem* pool);
void Audio_InitNoteFreeList(void);
void Audio_NotePoolClear(AudioListItem* pool);
void Audio_NotePoolFill(AudioListItem* pool, s32 count);
void Audio_AudioListPushFront(AudioListItem* list, AudioListItem* item);
void Audio_AudioListRemove(AudioListItem* item);
Note* Audio_FindNodeWithPrioLessThan(AudioListItem* list, s32 limit);
void Audio_NoteInitForLayer(Note* note, SequenceLayer* layer);
void Audio_NoteInterruptForLayer(Note* note, SequenceLayer* layer);
void Audio_NoteReleaseAndTakeOwnership(Note* note, SequenceLayer* layer);
Note* Audio_AllocNoteFromDisabled(AudioListItem* pool, SequenceLayer* layer);
Note* Audio_AllocNoteFromDecaying(AudioListItem* pool, SequenceLayer* layer);
Note* Audio_AllocNoteFromActive(AudioListItem* pool, SequenceLayer* layer);
Note* Audio_AllocNote(SequenceLayer* layer);
void Audio_NoteInitAll(void);

// nad/seqplayer
void AudioSeq_SequenceChannelDisable(SequenceChannel* channel);
void AudioSeq_SequencePlayerDisable(SequencePlayer* seqPlayer);
void AudioSeq_AudioListPushBack(AudioListItem* list, AudioListItem* item);
void* AudioSeq_AudioListPopBack(AudioListItem* list);
void AudioSeq_ProcessSequences(s32 arg0);
void AudioSeq_SkipForwardSequence(SequencePlayer* seqPlayer);
void AudioSeq_ResetSequencePlayer(SequencePlayer* seqPlayer);
void AudioSeq_InitSequencePlayerChannels(s32 playerIdx);
void AudioSeq_InitSequencePlayers(void);

// The driver's top layer (heap, load, thread, os): its allocator, command ring and DMA side, every entry point
// the library calls, and every libultra call the driver makes.

// nad/heap
void AudioHeap_DiscardFont(s32 fontId);
void* AudioHeap_AllocDmaMemory(void* pool, s32 size);
void* AudioHeap_AllocZeroed(AudioAllocPool* pool, s32 size);
void* AudioHeap_Alloc(AudioAllocPool* pool, s32 size);
void AudioHeap_InitPool(AudioAllocPool* pool, u8* ramAddr, s32 size);
void AudioHeap_InitPersistentCache(AudioPersistentCache* persistent);
void AudioHeap_InitTemporaryCache(AudioTemporaryCache* temporary);
void AudioHeap_InitMainPools(s32 initPoolSize);
s32 AudioHeap_AllocCached(s32 tableType, s32 size, s32 cache, s32 id);
s32 AudioHeap_SearchCaches(s32 tableType, s32 cache, s32 id);
s32 AudioHeap_SearchRegularCaches(s32 tableType, s32 cache, s32 id);
void AudioHeap_LoadFilter(s16* filter, s32 lowPassCutoff, s32 highPassCutoff);
s32 AudioHeap_ResetStep(void);
void AudioHeap_Init(void);
s32 AudioHeap_SearchPermanentCache(s32 tableType, s32 id);
void* AudioHeap_AllocPermanent(s32 tableType, s32 id, s32 size);
// Load-bearing: the s8 `medium`.
void* AudioHeap_AllocTemporarySampleCache(s32 size, s32 fontId, s32 sampleAddr, s8 medium);
void* AudioHeap_AllocPersistentSampleCache(s32 size, s32 fontId, s32 sampleAddr, s8 medium);
void AudioHeap_InitSampleCaches(s32 persistentSampleCacheSize, s32 temporarySampleCacheSize);
SampleCacheEntry* AudioHeap_AllocTemporarySampleCacheEntry(s32 size);
void AudioHeap_DiscardSampleCacheEntry(SampleCacheEntry* entry);
SampleCacheEntry* AudioHeap_AllocPersistentSampleCacheEntry(s32 size);
void AudioHeap_DiscardSampleCaches(void);

// nad/load
void AudioLoad_DecreaseSampleDmaTtls(void);
s32 AudioLoad_DmaSampleData(s32 devAddr, u32 size, s32 arg2, u8* dmaIndexRef, s32 medium);
void AudioLoad_InitSampleDmaBuffers(s32 numNotes);
s32 AudioLoad_IsFontLoadComplete(s32 fontId);
s32 AudioLoad_IsSeqLoadComplete(s32 seqId);
void AudioLoad_SetFontLoadStatus(s32 fontId, s32 loadStatus);
void AudioLoad_SetSeqLoadStatus(s32 seqId, s32 loadStatus);
void AudioLoad_SetSampleFontLoadStatus(s32 sampleBankId, s32 loadStatus);
s32 AudioLoad_SyncLoadSeqFonts(s32 seqId, s32* outDefaultFontId);
void AudioLoad_SyncLoadSeqParts(s32 seqId, s32 flags);
s32 AudioLoad_SyncLoadInstrument(s32 fontId, s32 instId, s32 drumId);
void AudioLoad_AsyncLoadSampleBank(s32 sampleBankId, s32 nChunks, s32 retData, OSMesgQueue* retQueue);
void AudioLoad_AsyncLoadSeqFonts(s32 seqId, s32 nChunks, s32 retData, OSMesgQueue* retQueue);
u8* AudioLoad_GetFontsForSequence(s32 seqId, s32* outNumFonts);
void AudioLoad_DiscardSeqFonts(s32 seqId);
void AudioLoad_DiscardFont(s32 fontId);
s32 AudioLoad_SyncInitSeqPlayer(s32 playerIdx, s32 seqId, s32 arg2);
s32 AudioLoad_SyncInitSeqPlayerSkipTicks(s32 playerIdx, s32 seqId, s32 skipTicks);
s32 AudioLoad_SyncInitSeqPlayerInternal(s32 playerIdx, s32 seqId, s32 arg2);
s32 AudioLoad_SyncLoadSeq(s32 seqId);
s32 AudioLoad_TrySyncLoadSampleBank(s32 sampleBankId, s32* outMedium);
s32 AudioLoad_SyncLoadFont(s32 fontId);
s32 AudioLoad_SyncLoad(s32 tableType, s32 id, s32* didAllocate);
s32 AudioLoad_GetRealTableIndex(s32 tableType, s32 id);
s32 AudioLoad_SearchCaches(s32 tableType, s32 id);
AudioTable* AudioLoad_GetLoadTable(s32 tableType);
void AudioLoad_SyncDma(s32 devAddr, s32 ramAddr, u32 size, s32 medium);
void AudioLoad_SyncDmaUnkMedium(s32 devAddr, s32 addr, s32 size, s32 unkMediumParam);
s32 AudioLoad_Dma(OSIoMesg* mesg, s32 priority, s32 direction, u32 devAddr, void* ramAddr, u32 size,
                  OSMesgQueue* reqQueue, s32 medium, const char* dmaFuncType);
s32 AudioLoad_AsyncLoadInner(s32 tableType, s32 id, s32 nChunks, s32 retData, OSMesgQueue* retQueue);
void AudioLoad_ProcessLoads(s32 resetStatus);
void AudioLoad_Init(void);
void AudioLoad_InitSlowLoads(void);
s32 AudioLoad_SlowLoadSample(s32 fontId, u8 instId, s8* status);
void AudioLoad_FinishSlowLoad(AudioSlowLoad* slowLoad);
void AudioLoad_ProcessSlowLoads(s32 resetStatus);
void AudioLoad_DmaSlowCopy(AudioSlowLoad* slowLoad, s32 size);
void AudioLoad_DmaSlowCopyUnkMedium(s32 devAddr, u8* ramAddr, s32 size, s32 arg3);
s32 AudioLoad_SlowLoadSeq(s32 seqId, u8* ramAddr, s8* status);
AudioAsyncLoad* AudioLoad_StartAsyncLoad(s32 devAddr, s32 ramAddr, s32 size, s32 medium, s32 nChunks, s32 retQueue,
                                         s32 retMsg);
void AudioLoad_ProcessAsyncLoads(s32 resetStatus);
void AudioLoad_ProcessAsyncLoad(AudioAsyncLoad* asyncLoad, s32 resetStatus);
// No prototype: AudioLoad_AsyncDma is defined below its one caller, AudioLoad_ProcessAsyncLoad, and its real
// signature is (AudioAsyncLoad* asyncLoad, s32 size). Load-bearing: the empty parameter list.
void AudioLoad_AsyncDma();
void AudioLoad_AsyncDmaUnkMedium(s32 devAddr, void* ramAddr, s32 size, s32 arg3);
void AudioLoad_RelocateSample(TunedSample* tunedSample, s32 fontData, SampleBankRelocInfo* sampleBankReloc);
void AudioLoad_RelocateFontAndPreloadSamples(s32 fontId, SoundFontData* fontData, SampleBankRelocInfo* sampleBankReloc,
                                             s32 async);
s32 AudioLoad_ProcessSamplePreloads(s32 resetStatus);
void AudioLoad_PreloadSamplesForFont(s32 fontId, s32 async, SampleBankRelocInfo* sampleBankReloc);
void AudioLoad_LoadPermanentSamples(void);

// nad/thread
void AudioThread_SetFadeOutTimer(s32 seqPlayerIndex, s32 fadeTimer);
void AudioThread_SetFadeInTimer(s32 seqPlayerIndex, s32 fadeTimer);
void AudioThread_InitMesgQueuesImpl(void);
void AudioThread_QueueCmd(u32 opArgs, void* data);
void AudioThread_QueueCmdF32(u32 opArgs, f32 data);
void AudioThread_QueueCmdS32(u32 opArgs, s32 data);
void AudioThread_QueueCmdS8(u32 opArgs, s8 data);
void AudioThread_QueueCmdU16(u32 opArgs, u16 data);
void AudioThread_ScheduleProcessCmds(void);
void AudioThread_ResetCmdQueue(void);
void AudioThread_ProcessCmds(u32 msg);
s32 AudioThread_CheckSpecChange(void);
void AudioThread_ClearSpecChangeQueue(void);
s32 AudioThread_ResetAudioHeap(s32 specId);
s32 AudioThread_GetChannelIO(s32 seqPlayerIndex, s32 channelIndex, s32 ioPort);
void AudioThread_InitMesgQueues(void);

// nad/os
void Audio_InvalDCache(void* buf, s32 size);
void Audio_WritebackDCache(void* buf, s32 size);
s32 Audio_AiSetNextBuffer(void* bufPtr, u32 size);

// na/na_game_state
void Na_SetTimerCount(s32 timerType, s16 count);
u16 Na_GetYoshiMuteMask(void);

// na/nai_main
void Nai_Init(void);
void Nai_Routin(void);
void Nai_SpecChange(s16 spec);
void Nai_PauseOn(void);
void Nai_PauseOff(void);
void Nai_PauseOffStop(void);
void Nai_OutMode(s32 mode);

// Data

// na/session_config.c's records: AudioLoad_Init carves gAudioHeap by gAudioHeapInitSizes, AudioHeap_Init
// applies gAudioSpecs[gAudioCtx.specId] and derives maxTempo from gTempoData.
extern const TempoData gTempoData;
extern const AudioHeapInitSizes gAudioHeapInitSizes;
extern AudioSpec gAudioSpecs[];
// Nonzero between Na_PauseOn and Na_PauseOff: Nai_SePause holds the live SEs, Na_SeLevelChange returns at once
// and the BGM plays at half volume.
extern u8 gNaPause;
// Frames until every level SE is stopped; runs to -1 and stays there once it has fired.
extern s16 gNaLevelSeStopTimer;
// Pan-center arbitration slot for SE 0x76 - the frame's winning posX, or -0x100 for unclaimed.
extern s16 gNaSe76PosX;
// Nonzero while Na_BgmInterrupt has the scene BGM swapped for track 0x14 (cleared by Na_BgmResume).
extern u8 gNaBgmInterrupted;
// The scene id Na_SceneChange last recorded, and the audio spec it selected for it (0 = no spec change).
extern s16 gNaScene;
extern s16 gNaSceneSpec;
// Nonzero between NA_EVENT_ECHO_START and ECHO_STOP: positional SEs get fxmix 0x50 (scene 0x36 ends it).
extern u8 gNaEcho;
// Nonzero between NA_EVENT_TIMER_START and TIMER_END: Na_BgmVolumeUpdate ducks the BGM to 0.6.
extern u8 gNaTimerBgm;
// Raised by the page-ending events (the YOSHI_* deaths, the 30-fruit and melon clears) and by Na_SeTrig's SE
// 0xB4 over BGM 0x35, until the next scene change: Na_SetTimerCount and ten Na_SeTrig arms go silent, and SE
// 0x26 drops to pitch 0.
extern u8 gNaPageEnd;
// Triggers of Na_SeTrig's SE 0x211; the third stops BGM 0xE (MID_BOSS_DEMO's tune) and wraps to 0.
extern s16 gNaSe211Count;
// The three BGM handles na_bgm.c owns: the scene's music, the sub-game track layered over it, and the one-shot
// fanfares Na_JingleBgmStart starts.
extern NaiBgm* gNaBgm;
extern NaiBgm* gNaSubGameBgm;
extern NaiBgm* gNaJingleBgm;
// Yoshi status, as last handed to Na_YoshiStatusChange.
extern s32 gNaYoshiStatus;
// Which Minobon is active, as last handed to Na_SetWhichMinobon.
extern s32 gNaWhichMinobon;
// What na_game_state.c's setters last stored (nothing in the library reads the cursor pair); the yoshi mask has
// a bit per yoshi, which Na_GetYoshiMuteMask returns inverted and permuted.
extern s16 gNaCursorX;
extern s16 gNaCursorY;
extern u32 gNaActiveYoshiMask;
extern u8 gNaChibiYoshi;
extern s16 gNaResult;
extern u8 gNaiSpecChange;
// How many of the SE group's 16 channels the library hands out (10).
extern s16 gNaiSeSubCount;
// The SE slot Nai_SeBufferAlloc last handed out (NULL when the buffer was full). Na_SeTrig reads
// it back right after its Nai_SeTrig call to stage the new sound's tune and fxmix.
extern NaiSe* gNaiSeLastAlloc;

// Bss

extern AudioContext gAudioCtx;
// The audio heap.
extern u8 gAudioHeap[];
// The hook AudioThread_UpdateImpl calls once per audio frame while it is non-NULL.
extern AudioCustomUpdateFunction gAudioCustomUpdateFunction;
// The BGM Na_BgmInterrupt interrupted, so Na_BgmResume can restart it.
extern s16 gNaBgmInterruptedNum;
// Live-SE registry, indexed [se][subHandle]; NULL means the slot is not playing.
extern NaiSe* gNaSeHandles[0x77][0xA];
// Na_SeTrig's breathing latch: cases 0x056 and 0x0D4 store params->value, case 0x00F picks its sound by it.
extern u16 gNaBreathing;
// Command pool: 0x100 entries, with gNaiPortList heading the doubly-linked list of live ones.
extern NaiPort gNaiPortBuffer[0x100];
extern NaiPort* gNaiPortList;
// The BGM Buffer: 10 handles, with gNaiCurrentBgm (the handle owning the BGM) just past them.
extern NaiBgm gNaiBgmBuffer[10];
extern NaiBgm* gNaiCurrentBgm;
// Group registry: which handle is playing on group 0 and group 1.
extern NaiBgm* gNaiBgmGroups[2];
// Waiting list: a 10-node pool, with gNaiBgmWaitList heading the linked ones.
extern NaiBgmWaitNode gNaiBgmWaitNodes[10];
extern NaiBgmWaitNode* gNaiBgmWaitList;
// The SE buffer: 50 slots, swept every frame by Nai_SeRoutin.
extern NaiSe gNaiSeBuffer[0x32];
// Each SE slot's tune target while Nai_SePause holds it.
extern f32 gNaiSePausedTune[0x32];

#endif

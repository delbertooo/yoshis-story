#ifndef ASEQ_H
#define ASEQ_H

// The NA audio driver's sequence-script command bytes, under the names of OoT's include/audio/aseq.h: those
// nad/seqplayer.c implements, at OoT's values. Not named: the sequence player's 0x10 and 0x20 (no-ops, no OoT
// command) and 0x60 (OoT's LDRES, a no-op here), and channel 0xBD (OoT's RANDPTR), which this driver lacks.
// The tables are aligned by hand; .clang-format does not align macros, hence the guard.

// clang-format off
// control flow commands
#define ASEQ_OP_CONTROL_FLOW_FIRST  0xF2
#define ASEQ_OP_RBLTZ               0xF2
#define ASEQ_OP_RBEQZ               0xF3
#define ASEQ_OP_RJUMP               0xF4
#define ASEQ_OP_BGEZ                0xF5
#define ASEQ_OP_BREAK               0xF6
#define ASEQ_OP_LOOPEND             0xF7
#define ASEQ_OP_LOOP                0xF8
#define ASEQ_OP_BLTZ                0xF9
#define ASEQ_OP_BEQZ                0xFA
#define ASEQ_OP_JUMP                0xFB
#define ASEQ_OP_CALL                0xFC
#define ASEQ_OP_DELAY               0xFD
#define ASEQ_OP_DELAY1              0xFE
#define ASEQ_OP_END                 0xFF

// sequence commands
#define ASEQ_OP_SEQ_TESTCHAN        0x00 // low nibble used as argument
#define ASEQ_OP_SEQ_STOPCHAN        0x40 // low nibble used as argument
#define ASEQ_OP_SEQ_SUBIO           0x50 // low nibble used as argument
#define ASEQ_OP_SEQ_STIO            0x70 // low nibble used as argument
#define ASEQ_OP_SEQ_LDIO            0x80 // low nibble used as argument
#define ASEQ_OP_SEQ_LDCHAN          0x90 // low nibble used as argument
#define ASEQ_OP_SEQ_RLDCHAN         0xA0 // low nibble used as argument
#define ASEQ_OP_SEQ_LDSEQ           0xB0 // low nibble used as argument
#define ASEQ_OP_SEQ_SCRIPTCTR       0xC5
#define ASEQ_OP_SEQ_STOP            0xC6
#define ASEQ_OP_SEQ_STSEQ           0xC7
#define ASEQ_OP_SEQ_SUB             0xC8
#define ASEQ_OP_SEQ_AND             0xC9
#define ASEQ_OP_SEQ_LDI             0xCC
#define ASEQ_OP_SEQ_DYNCALL         0xCD
#define ASEQ_OP_SEQ_RAND            0xCE
#define ASEQ_OP_SEQ_NOTEALLOC       0xD0
#define ASEQ_OP_SEQ_LDSHORTGATEARR  0xD1
#define ASEQ_OP_SEQ_LDSHORTVELARR   0xD2
#define ASEQ_OP_SEQ_MUTEBHV         0xD3
#define ASEQ_OP_SEQ_MUTE            0xD4
#define ASEQ_OP_SEQ_MUTESCALE       0xD5
#define ASEQ_OP_SEQ_FREECHAN        0xD6
#define ASEQ_OP_SEQ_INITCHAN        0xD7
#define ASEQ_OP_SEQ_VOLSCALE        0xD9
#define ASEQ_OP_SEQ_VOLMODE         0xDA
#define ASEQ_OP_SEQ_VOL             0xDB
#define ASEQ_OP_SEQ_TEMPOCHG        0xDC
#define ASEQ_OP_SEQ_TEMPO           0xDD
#define ASEQ_OP_SEQ_RTRANSPOSE      0xDE
#define ASEQ_OP_SEQ_TRANSPOSE       0xDF
#define ASEQ_OP_SEQ_FREENOTELIST    0xF0
#define ASEQ_OP_SEQ_ALLOCNOTELIST   0xF1

// channel commands
#define ASEQ_OP_CHAN_CDELAY         0x00 // low nibble used as argument
#define ASEQ_OP_CHAN_LDSAMPLE       0x10 // low nibble used as argument
#define ASEQ_OP_CHAN_LDCHAN         0x20 // low nibble used as argument
#define ASEQ_OP_CHAN_STCIO          0x30 // low nibble used as argument
#define ASEQ_OP_CHAN_LDCIO          0x40 // low nibble used as argument
#define ASEQ_OP_CHAN_SUBIO          0x50 // low nibble used as argument
#define ASEQ_OP_CHAN_LDIO           0x60 // low nibble used as argument
#define ASEQ_OP_CHAN_STIO           0x70 // lower 3 bits used as argument
#define ASEQ_OP_CHAN_RLDLAYER       0x78 // lower 3 bits used as argument
#define ASEQ_OP_CHAN_TESTLAYER      0x80 // lower 3 bits used as argument
#define ASEQ_OP_CHAN_LDLAYER        0x88 // lower 3 bits used as argument
#define ASEQ_OP_CHAN_DELLAYER       0x90 // lower 3 bits used as argument
#define ASEQ_OP_CHAN_DYNLDLAYER     0x98 // lower 3 bits used as argument
#define ASEQ_OP_CHAN_LDFILTER       0xB0
#define ASEQ_OP_CHAN_FREEFILTER     0xB1
#define ASEQ_OP_CHAN_LDSEQTOPTR     0xB2
#define ASEQ_OP_CHAN_FILTER         0xB3
#define ASEQ_OP_CHAN_PTRTODYNTBL    0xB4
#define ASEQ_OP_CHAN_DYNTBLTOPTR    0xB5
#define ASEQ_OP_CHAN_DYNTBLV        0xB6
#define ASEQ_OP_CHAN_RANDTOPTR      0xB7
#define ASEQ_OP_CHAN_RAND           0xB8
#define ASEQ_OP_CHAN_RANDVEL        0xB9
#define ASEQ_OP_CHAN_RANDGATE       0xBA
#define ASEQ_OP_CHAN_COMBFILTER     0xBB
#define ASEQ_OP_CHAN_PTRADD         0xBC
#define ASEQ_OP_CHAN_INSTR          0xC1
#define ASEQ_OP_CHAN_DYNTBL         0xC2
#define ASEQ_OP_CHAN_SHORT          0xC3
#define ASEQ_OP_CHAN_NOSHORT        0xC4
#define ASEQ_OP_CHAN_DYNTBLLOOKUP   0xC5
#define ASEQ_OP_CHAN_FONT           0xC6
#define ASEQ_OP_CHAN_STSEQ          0xC7
#define ASEQ_OP_CHAN_SUB            0xC8
#define ASEQ_OP_CHAN_AND            0xC9
#define ASEQ_OP_CHAN_MUTEBHV        0xCA
#define ASEQ_OP_CHAN_LDSEQ          0xCB
#define ASEQ_OP_CHAN_LDI            0xCC
#define ASEQ_OP_CHAN_STOPCHAN       0xCD
#define ASEQ_OP_CHAN_LDPTR          0xCE
#define ASEQ_OP_CHAN_STPTRTOSEQ     0xCF
#define ASEQ_OP_CHAN_EFFECTS        0xD0
#define ASEQ_OP_CHAN_NOTEALLOC      0xD1
#define ASEQ_OP_CHAN_SUSTAIN        0xD2
#define ASEQ_OP_CHAN_BEND           0xD3
#define ASEQ_OP_CHAN_REVERB         0xD4
#define ASEQ_OP_CHAN_VIBFREQ        0xD7
#define ASEQ_OP_CHAN_VIBDEPTH       0xD8
#define ASEQ_OP_CHAN_RELEASERATE    0xD9
#define ASEQ_OP_CHAN_ENV            0xDA
#define ASEQ_OP_CHAN_TRANSPOSE      0xDB
#define ASEQ_OP_CHAN_PANWEIGHT      0xDC
#define ASEQ_OP_CHAN_PAN            0xDD
#define ASEQ_OP_CHAN_FREQSCALE      0xDE
#define ASEQ_OP_CHAN_VOL            0xDF
#define ASEQ_OP_CHAN_VOLEXP         0xE0
#define ASEQ_OP_CHAN_VIBFREQGRAD    0xE1
#define ASEQ_OP_CHAN_VIBDEPTHGRAD   0xE2
#define ASEQ_OP_CHAN_VIBDELAY       0xE3
#define ASEQ_OP_CHAN_DYNCALL        0xE4
#define ASEQ_OP_CHAN_REVERBIDX      0xE5
#define ASEQ_OP_CHAN_SAMPLEBOOK     0xE6
#define ASEQ_OP_CHAN_LDPARAMS       0xE7
#define ASEQ_OP_CHAN_PARAMS         0xE8
#define ASEQ_OP_CHAN_NOTEPRI        0xE9
#define ASEQ_OP_CHAN_STOP           0xEA
#define ASEQ_OP_CHAN_FONTINSTR      0xEB
#define ASEQ_OP_CHAN_VIBRESET       0xEC
#define ASEQ_OP_CHAN_GAIN           0xED
#define ASEQ_OP_CHAN_BENDFINE       0xEE
#define ASEQ_OP_CHAN_FREENOTELIST   0xF0
#define ASEQ_OP_CHAN_ALLOCNOTELIST  0xF1

// layer commands
#define ASEQ_OP_LAYER_NOTEDVG       0x00
#define ASEQ_OP_LAYER_NOTEDV        0x40
#define ASEQ_OP_LAYER_NOTEVG        0x80
#define ASEQ_OP_LAYER_LDELAY        0xC0
#define ASEQ_OP_LAYER_SHORTVEL      0xC1
#define ASEQ_OP_LAYER_TRANSPOSE     0xC2
#define ASEQ_OP_LAYER_SHORTDELAY    0xC3
#define ASEQ_OP_LAYER_LEGATO        0xC4
#define ASEQ_OP_LAYER_NOLEGATO      0xC5
#define ASEQ_OP_LAYER_INSTR         0xC6
#define ASEQ_OP_LAYER_PORTAMENTO    0xC7
#define ASEQ_OP_LAYER_NOPORTAMENTO  0xC8
#define ASEQ_OP_LAYER_SHORTGATE     0xC9
#define ASEQ_OP_LAYER_NOTEPAN       0xCA
#define ASEQ_OP_LAYER_ENV           0xCB
#define ASEQ_OP_LAYER_NODRUMPAN     0xCC
#define ASEQ_OP_LAYER_STEREO        0xCD
#define ASEQ_OP_LAYER_BENDFINE      0xCE
#define ASEQ_OP_LAYER_RELEASERATE   0xCF
#define ASEQ_OP_LAYER_LDSHORTVEL    0xD0 // low nibble used as an argument
#define ASEQ_OP_LAYER_LDSHORTGATE   0xE0 // low nibble used as an argument
// clang-format on

#endif

#include "global.h"
#include "audio.h"

// The posters of SEQ START, SEQ STOP and the PORT WRITEs, each filling in an entry from Nai_PortBufferAlloc.
// Every function name here is INVENTED (no string).

// Posts SEQ START: group `group` starts sequence `num`, fading in over `time`.
// Load-bearing: the `type` store last.
void Nai_SeqStart(s16 group, s16 num, s16 time) {
    NaiPort* port;

    port = Nai_PortBufferAlloc();
    if (port != NULL) {
        port->group = group;
        port->num = num;
        port->time = time;
        port->type = NAI_PORT_TYPE_SEQ_START;
    }
}

// Posts SEQ STOP: group `group`'s sequence fades out over `time`, or stops at once for 0.
// Load-bearing: the `type` store last.
void Nai_SeqStop(s16 group, s16 time) {
    NaiPort* port;

    port = Nai_PortBufferAlloc();
    if (port != NULL) {
        port->group = group;
        port->time = time;
        port->type = NAI_PORT_TYPE_SEQ_STOP;
    }
}

// Sets IO port `num` of group `group`'s sequence to `value` at once: a zero-length ramp from 0, which
// Nai_PortRampStep snaps to `value` on its first tick.
void Nai_GrpPortWrite(s16 group, s16 num, s16 value) {
    Nai_GrpPortWriteRamp(group, num, 0, value, 0);
}

// Posts GRP PORT WRITE: ramps IO port `num` of group `group`'s sequence from `start` to `end` over `time` ticks.
// Load-bearing: the `type` store after `end`, the redundant `elapsed = 0`, the `port->end - port->start` re-read.
void Nai_GrpPortWriteRamp(s16 group, s16 num, s16 start, s16 end, s16 time) {
    NaiPort* port;

    port = Nai_PortBufferAlloc();
    if (port != NULL) {
        port->group = group;
        port->num = num;
        port->start = start;
        port->end = end;
        port->type = NAI_PORT_TYPE_GRP_PORT_WRITE;
        port->elapsed = 0;
        port->time = time;
        if (time != 0) {
            port->delta = (port->end - port->start) / time;
        }
    }
}

// Nai_GrpPortWrite's twin for channel `sub`: sets its IO port `num` to `value` at once.
void Nai_SubPortWrite(s16 group, s16 sub, s16 num, s16 value) {
    Nai_SubPortWriteRamp(group, sub, num, 0, value, 0);
}

// Posts SUB PORT WRITE: Nai_GrpPortWriteRamp for channel `sub`, held while the group or the channel is disabled.
// Load-bearing: the `type` store after `end`, the redundant `elapsed = 0`, the `port->end - port->start` re-read.
void Nai_SubPortWriteRamp(s16 group, s16 sub, s16 num, s16 start, s16 end, s16 time) {
    NaiPort* port;

    port = Nai_PortBufferAlloc();
    if (port != NULL) {
        port->group = group;
        port->sub = sub;
        port->num = num;
        port->start = start;
        port->end = end;
        port->type = NAI_PORT_TYPE_SUB_PORT_WRITE;
        port->elapsed = 0;
        port->time = time;
        if (time != 0) {
            port->delta = (port->end - port->start) / time;
        }
    }
}

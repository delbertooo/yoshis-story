#include "global.h"
#include "audio.h"

// The BGM layer's group registry: gNaiBgmGroups holds the handle on each of the two groups, NULL while a group is
// free. Its own object, as its 16-aligned `.bss` block shows.
// Every function name here but Nai_BgmAnotherGroup (verbatim from its exception print) is INVENTED (no string).
// The PRINTF is a stripped debug print whose string stays in .rodata.

NaiBgm* gNaiBgmGroups[2];

// The first unregistered group, or -1 when both are taken.
s32 Nai_BgmGroupFindFree(void) {
    s16 i;

    for (i = 0; i < 2; i++) {
        if (gNaiBgmGroups[i] == NULL) {
            return i;
        }
    }
    return -1;
}

// Registers a handle on a free group and returns the group; with both taken, on the one opposite the current BGM
// (group 0 without one).
s16 Nai_BgmGroupRegister(NaiBgm* bgm) {
    s32 freeGroup;
    s16 group;

    freeGroup = Nai_BgmGroupFindFree();
    group = freeGroup;
    if (freeGroup < 0) {
        if (gNaiCurrentBgm != NULL) {
            group = Nai_BgmAnotherGroup(gNaiCurrentBgm->group);
        } else {
            group = 0;
        }
    }
    gNaiBgmGroups[group] = bgm;
    return group;
}

void Nai_BgmGroupUnregister(s16 group) {
    gNaiBgmGroups[group] = NULL;
}

// Unregisters a handle from whichever group holds it.
void Nai_BgmGroupUnregisterBgm(NaiBgm* bgm) {
    s16 i;

    for (i = 0; i < 2; i++) {
        if (gNaiBgmGroups[i] == bgm) {
            gNaiBgmGroups[i] = NULL;
        }
    }
}

// Empties the registry; the twin of Nai_BgmBufferClear, both called by Nai_BgmInit.
void Nai_BgmGroupClear(void) {
    s16 i;

    for (i = 0; i < 2; i++) {
        gNaiBgmGroups[i] = NULL;
    }
}

// The handle registered on the group opposite bgm's; NULL for a NULL bgm.
NaiBgm* Nai_BgmGroupGetOpposite(NaiBgm* bgm) {
    if (bgm == NULL) {
        return NULL;
    }
    return Nai_BgmGroupGet(Nai_BgmAnotherGroup(bgm->group));
}

NaiBgm* Nai_BgmGroupGet(s16 group) {
    return gNaiBgmGroups[group];
}

// The other group of the pair; -1 for anything that is not a group id.
s32 Nai_BgmAnotherGroup(s16 group) {
    switch (group) {
        case 0:
            return 1;
        case 1:
            return 0;
        default:
            PRINTF("NAI:例外 group in Nai_BgmAnotherGroup\n"); // 例外 = "exception"
    }
    return -1;
}

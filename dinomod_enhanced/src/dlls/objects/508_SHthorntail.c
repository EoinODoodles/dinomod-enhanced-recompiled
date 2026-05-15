#include "modding.h"

#include "dlls/engine/27.h"
#include "dlls/objects/common/sidekick.h"
#include "recomputils.h"
#include "sys/curves.h"
#include "sys/main.h"
#include "sys/objects.h"
#include "sys/objexpr.h"
#include "sys/objtype.h"

#include "recomp/dlls/objects/508_SHthorntail_recomp.h"

typedef struct {
    // dll 53 start
/*000*/ u8 _unk0[0x4A9 - 0x0];
/*4A9*/ u8 unk4A9;
/*4AA*/ u8 _unk4AA[0x4B8 - 0x4AA];
    // dll 53 end (probably)
/*4B8*/ s8 unk4B8;
/*4B9*/ u8 unk4B9;
/*4BA*/ u8 unk4BA;
/*4BB*/ u8 mapAct;
/*4BC*/ s8 unk4BC;
/*4BD*/ u8 nextSeq; // next seq in rotation to play on interact
/*4BE*/ u8 _unk4BE[0x4C0 - 0x4BE];
/*4C0*/ u8* seqRotation;
/*4C4*/ u8 seqRotationCount; // number of seqs in rotation
/*4C5*/ u8 _unk4C5[0x4D0 - 0x4C5];
/*4D0*/ s16 unk4D0;
/*4D2*/ s16 unk4D2;
/*4D4*/ s16 unk4D4;
/*4D6*/ s16 unk4D6;
/*4D8*/ s16 unk4D8;
/*4DA*/ s16 unk4DA;
/*4DC*/ s16 unk4DC;
/*4E0*/ s32 unk4E0;
/*4E4*/ DLL27_Data unk4E4;
/*744*/ u8 _unk744[0x75C - 0x744];
/*75C*/ CurveSetup* unk75C;
/*760*/ CurveSetup* unk760;
/*764*/ CurveSetup* unk764;
/*768*/ u8 _unk768[0x804 - 0x768];
/*804*/ f32 unk804;
/*808*/ f32 unk808;
/*80C*/ u8 _unk80C[0x840 - 0x80C];
/*840*/ f32 playerDist;
/*844*/ f32 unk844;
/*848*/ f32 unk848;
/*84C*/ f32 unk84C;
/*850*/ HeadAnimation unk850;
/*874*/ u8 unk874;
/*875*/ u8 _unk875[0x878 - 0x875];
} SHthorntail_Data;

typedef struct {
/*00*/ ObjSetup base;
/*18*/ u8 unk18;
} SHthorntail_Setup;

static u8 burrowThornsHintSeq = 10;
static u8 howsRandornSeq = 6;

// TODO: this is a temporary change since the log trading quest doesn't currently exist
/** Make the log trader talk about the burrows after being woken up. */
RECOMP_HOOK_DLL(thorntail_func_1C48) void thorntail_func_1C48_hook(Object *self, SHthorntail_Data *objdata, SHthorntail_Setup *setup) {
    if (main_get_bits(objdata->unk4DC)) {
        objdata->seqRotation = &burrowThornsHintSeq;
        objdata->seqRotationCount = 1;
        objdata->nextSeq = 0;
    }
}

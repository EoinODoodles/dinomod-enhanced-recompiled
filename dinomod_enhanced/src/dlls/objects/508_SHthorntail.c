#include "modding.h"
#include "recomputils.h"

#include "dlls/engine/27.h"
#include "dlls/engine/53_movelib.h"
#include "dlls/objects/common/sidekick.h"
#include "sys/curves.h"
#include "sys/main.h"
#include "sys/objects.h"
#include "sys/objexpr.h"
#include "sys/objtype.h"

#include "recomp/dlls/objects/508_SHthorntail_recomp.h"

typedef struct {
/*000*/ MoveLibData movedata;
/*4B8*/ s8 state;
/*4B9*/ u8 _unk4B9;
/*4BA*/ u8 flags;
/*4BB*/ u8 mapAct;
/*4BC*/ s8 grazingAltAnimSelector;
/*4BD*/ u8 nextSeq; // next seq in rotation to play on interact
/*4BE*/ u8 _unk4BE[0x4C0 - 0x4BE];
/*4C0*/ u8* talkSeqs;
/*4C4*/ u8 talkSeqsCount; // number of seqs in rotation
/*4C5*/ u8 _unk4C5[0x4D0 - 0x4C5];
/*4D0*/ s16 eatingTimer;
/*4D2*/ s16 grazingTimer;
/*4D4*/ s16 drinkTimer;
/*4D6*/ s16 targetAngle;
/*4D8*/ s16 angleToTarget;
/*4DA*/ s16 startAngle;
/*4DC*/ s16 progressionBlockerGamebit;
/*4E0*/ s32 unk4E0;
/*4E4*/ DLL27_Data collider;
/*744*/ u8 _unk744[0x75C - 0x744];
/*75C*/ CurveSetup* prevCurve;
/*760*/ CurveSetup* currentCurve;
/*764*/ CurveSetup* targetCurve;
/*768*/ u8 _unk768[0x804 - 0x768];
/*804*/ f32 modAnimDelta;
/*808*/ f32 walkSpeed;
/*80C*/ u8 _unk80C[0x840 - 0x80C];
/*840*/ f32 playerDist;
/*844*/ f32 distToTargetCurve;
/*848*/ f32 unk848;
/*84C*/ f32 unk84C;
/*850*/ HeadAnimation headAnim;
/*874*/ u8 unk874;
/*875*/ u8 _unk875[0x878 - 0x875];
} SHthorntail_Data;

typedef struct {
/*00*/ ObjSetup base;
/*18*/ u8 thorntail; // which Thorntail this is
} SHthorntail_Setup;

static u8 burrowThornsHintSeq = 10;
static u8 howsRandornSeq = 6;

// TODO: this is a temporary change since the log trading quest doesn't currently exist
/** Make the log trader talk about the burrows after being woken up. */
RECOMP_HOOK_DLL(thorntail_trader_act1_control) void thorntail_trader_act1_control_hook(Object *self, SHthorntail_Data *objdata, SHthorntail_Setup *setup) {
    if (main_get_bits(objdata->progressionBlockerGamebit)) {
        objdata->talkSeqs = &burrowThornsHintSeq;
        objdata->talkSeqsCount = 1;
        objdata->nextSeq = 0;
    }
}

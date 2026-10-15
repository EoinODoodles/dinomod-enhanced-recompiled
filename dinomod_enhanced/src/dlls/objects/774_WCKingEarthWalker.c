#include "recomputils.h"

#include "PR/os.h"
#include "dll.h"
#include "dlls/engine/53_movelib.h"
#include "game/gamebits.h"
#include "game/objects/object.h"
#include "game/objects/interaction_arrow.h"
#include "sys/joypad.h"
#include "sys/main.h"
#include "sys/objexpr.h"

#include "recomp/dlls/objects/774_WCKingEarthWalker_recomp.h"

typedef struct {
    ObjSetup base;
    s8 yaw;
} WCKingEarthWalker_Setup;

typedef struct {
    MoveLibData moveData;
    HeadAnimation headAnim;
    u8 state;
    u8 flags;
} WCKingEarthWalker_Data;

typedef enum {
    WCKingEarthWalker_STATE_0_Initial,
    WCKingEarthWalker_STATE_1_Met_Sabre,
    WCKingEarthWalker_STATE_2_SpellStone_Retrieved
} WCKingEarthWalker_States;

typedef enum {
    WCKingEarthWalker_FLAG_1_Waiting_for_Sequence = 1
} WCKingEarthWalker_Flags;

extern void WCKingEarthWalker_updateShadow(Object* self);
extern int WCKingEarthWalker_animCallback(Object* self, Object* animObj, AnimObj_Data* animData, s8 prevCallbackValue);

/* Reposition King EarthWalker when revisiting after rescuing him */
RECOMP_PATCH void WCKingEarthWalker_obj_Setup(Object* self, WCKingEarthWalker_Setup* objSetup, s32 reset) {
    WCKingEarthWalker_Data* objData = self->data;
/*0x0*/ s16 data_0[] = { 0, 8} ;
/*0x4*/ s16 data_4[] = { 10, 35 };

    self->animCallback = WCKingEarthWalker_animCallback;
    
    mainCreateTempDLL(DLL_ID_MOVELIB);
    ((DLL_53_movelib*)gTempDLLInsts[1])->vtbl->func2(self, &objData->moveData, -0x1FFF, 0x31C6, 2);
    ((DLL_53_movelib*)gTempDLLInsts[1])->vtbl->func6(&objData->moveData, 0, data_0, 2);
    
    objData->moveData.unk4A9 |= 2;
    
    self->srt.yaw = objSetup->yaw << 8;

    //@recomp: restore state
    if (mainGetBits(BIT_SpellStone_WC)) {
        //Move to post-boss sequence position
        self->srt.transl.x += -84.993652;
        self->srt.transl.y += -0.590332;
        self->srt.transl.z += 440.437988;
        self->srt.yaw = -7966;

        objData->state = WCKingEarthWalker_STATE_2_SpellStone_Retrieved;
    } else if (mainGetBits(BIT_WC_Met_King_EarthWalker)) {
        objData->state = WCKingEarthWalker_STATE_1_Met_Sabre;
    }
}

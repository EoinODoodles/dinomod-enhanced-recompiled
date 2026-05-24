#include "modding.h"
#include "recomputils.h"

#include "dll.h"
#include "dlls/engine/27.h"
#include "dlls/engine/53_movelib.h"
#include "dlls/objects/common/sidekick.h"
#include "game/gamebits.h"
#include "game/objects/interaction_arrow.h"
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
/*19*/ u8 yaw;          //@recomp: custom param
/*1A*/ s16 gamebitAway; //@recomp: ThornTail doesn't show up when set
} SHthorntail_Setup;

typedef enum {
    THORNTAIL_1_Sleepy = 1, //The sleepy-voiced ThornTail near the tree hollow/river crossing
    THORNTAIL_2_Log_Trader, //The ThornTail near the burrows behind Rocky
    THORNTAIL_3_Elder       //The elderly ThornTail near Willow Grove
} ThornTail_Indices;

#define THORNTAIL_4_Willow_Grove 4 //Extra ThornTail blocking Willow Grove (like the one MusicalProgrammer added in older patches) TODO: config for this?

extern int thorntail_anim_callback(Object *actor, Object *animObj, AnimObj_Data *animObjData, s8 a3);
extern void thorntail_sleepy_setup(Object *self, SHthorntail_Data *objdata, SHthorntail_Setup *setup);
extern void thorntail_trader_setup(Object *self, SHthorntail_Data *objdata, SHthorntail_Setup *setup);
extern void thorntail_elder_setup(Object *self, SHthorntail_Data *objdata, SHthorntail_Setup *setup);
extern void thorntail_update_shadow(Object* self);
extern void thorntail_sleepy_control(Object* self, SHthorntail_Data* objdata, SHthorntail_Setup* setup);
extern void thorntail_trader_control(Object* self, SHthorntail_Data* objdata, SHthorntail_Setup* setup);
extern void thorntail_elder_control(Object* self, SHthorntail_Data* objdata, SHthorntail_Setup* setup);

extern Vec3f data_0[];
extern f32 data_30[];
extern Unk80026DF4 data_40[];
extern u8 sStateFlagMap[];

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

RECOMP_PATCH void thorntail_setup(Object *self, SHthorntail_Setup *setup, s32 reset) {
    SHthorntail_Data *objdata = self->data;
    s32 _pad;
    u8 sp3C[] = {1, 1, 1, 1}; // data_224

    switch (setup->thorntail) {
    case THORNTAIL_1_Sleepy:
        thorntail_sleepy_setup(self, objdata, setup);
        break;
    case THORNTAIL_2_Log_Trader:
        thorntail_trader_setup(self, objdata, setup);
        break;
    case THORNTAIL_3_Elder:
        thorntail_elder_setup(self, objdata, setup);
        break;
    //@recomp: custom extra ThornTail
    case THORNTAIL_4_Willow_Grove:
        self->srt.yaw = setup->yaw << 8;
        return;
    }

    gDLL_27->vtbl->init(&objdata->collider, DLL27FLAG_4000000 | DLL27FLAG_2000000, DLL27FLAG_400, DLL27MODE_1);
    gDLL_27->vtbl->setup_terrain_collider(&objdata->collider, 4, data_0, data_30, sp3C);
    gDLL_27->vtbl->reset(self, &objdata->collider);

    self->shadow->flags |= (OBJ_SHADOW_FLAG_TOP_DOWN | OBJ_SHADOW_FLAG_USE_OBJ_YAW | OBJ_SHADOW_FLAG_CUSTOM_OBJ_POS | OBJ_SHADOW_FLAG_CUSTOM_DIR);
    self->shadow->distFadeMaxOpacity = 128;
    self->shadow->distFadeMinOpacity = 90;
    self->animCallback = thorntail_anim_callback;

    objdata->state = -1;

    create_temp_dll(53);
    ((DLL_53_movelib*)gTempDLLInsts[1])->vtbl->func2(self, &objdata->movedata, -0x1FFF, 0x2AAA, 3);
    ((DLL_53_movelib*)gTempDLLInsts[1])->vtbl->func5(&objdata->movedata, 400, 30);
    objdata->movedata.unk4A9 &= ~0x8;

    obj_add_object_type(self, OBJTYPE_40);
}

// Behaviour for the custom sleeping ThornTail
static void thorntail_control_willow_grove(Object* self, SHthorntail_Data* objdata, SHthorntail_Setup* setup) {
    UnkFunc_80024108Struct animInfo;
    TextureAnimator* texAnimA;
    TextureAnimator* texAnimB;

    //Check if gamebit set (wandered off)
    if ((setup->gamebitAway > (NO_GAMEBIT + 1)) && main_get_bits(setup->gamebitAway)) {
        obj_destroy_object(self);
        return;
    }

    //Go to sleep
    if (self->curModAnimId != 4) {
        func_80023D30(self, 4, 0.0f, 0);

        //Close eyes
        texAnimA = func_800348A0(self, 5, 0);
        texAnimB = func_800348A0(self, 4, 0);
        if (texAnimA != NULL) {
            texAnimA->frame = 0x200;
        }
        if (texAnimB != NULL) {
            texAnimB->frame = 0x200;
        }

        self->srt.scale = self->def->scale * 1.15f;
    }

    self->unkAF |= ARROW_FLAG_8_No_Targetting;

    //Advance animation
    func_80024108(self, 0.005f, gUpdateRateF, &animInfo);
}

RECOMP_PATCH void thorntail_control(Object* self) {
    SHthorntail_Data* objdata;
    SHthorntail_Setup* setup;
    Object* player;

    objdata = self->data;
    setup = (SHthorntail_Setup*)self->setup;
    player = get_player();
    self->unkAF &= ~ARROW_FLAG_8_No_Targetting;

    //@recomp
    if (setup->thorntail == THORNTAIL_4_Willow_Grove) {
        thorntail_control_willow_grove(self, objdata, setup);
        return;
    }

    if (func_80026DF4(self, data_40, 15, objdata->unk874, &objdata->unk84C) == 0) {
        objdata->unk874 = 0;
        objdata->mapAct = gDLL_29_Gplay->vtbl->get_act(self->mapID);
        objdata->playerDist = vec3_distance(&self->globalPosition, &player->globalPosition);

        switch (setup->thorntail) {
        case THORNTAIL_1_Sleepy:
            thorntail_sleepy_control(self, objdata, setup);
            break;
        case THORNTAIL_2_Log_Trader:
            thorntail_trader_control(self, objdata, setup);
            break;
        case THORNTAIL_3_Elder:
            thorntail_elder_control(self, objdata, setup);
            break;
        }
        
        thorntail_update_shadow(self);

        gDLL_27->vtbl->func_1E8(self, &objdata->collider, gUpdateRateF);
        gDLL_27->vtbl->func_5A8(self, &objdata->collider);
        gDLL_27->vtbl->func_624(self, &objdata->collider, gUpdateRateF);

        if (!(sStateFlagMap[objdata->state] & 2)) {
            func_80032A08(self, &objdata->headAnim);
        }
    }
}

RECOMP_PATCH void thorntail_free(Object *self, s32 onlySelf) {
    SHthorntail_Setup* objSetup = (SHthorntail_Setup*)self->setup;

    //@recomp
    if (objSetup && objSetup->thorntail == THORNTAIL_4_Willow_Grove) {
        return;
    }

    remove_temp_dll(53);
    obj_free_object_type(self, OBJTYPE_40);
}

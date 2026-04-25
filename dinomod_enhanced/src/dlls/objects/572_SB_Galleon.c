#include "modding.h"
#include "recomputils.h"

#include "PR/os.h"
#include "common.h"
#include "macros.h"
#include "game/objects/object_id.h"
#include "sys/joypad.h"
#include "sys/objects.h"
#include "sys/objtype.h"
#include "sys/print.h"
#include "sys/rand.h"
#include "sys/segment_1460.h"
#include "dlls/engine/6_amsfx.h"

#include "recomp/dlls/objects/572_SB_Galleon_recomp.h"

#include "button_code.h"
#include "core/main.h"

#define DEBUG_SKIP_GALLEON_FIGHT FALSE

#define SOME_MAP_ID 80

typedef struct {
/*00*/ f32 x1;
/*04*/ f32 y1;
/*08*/ f32 z1;
/*0C*/ f32 cloudrunnerZ;
/*10*/ s8 _unk10[0x1C - 0x10];
/*1C*/ f32 unk1C;
/*20*/ s16 unk20;
/*22*/ s16 unk22;
/*24*/ s16 unk24;
/*26*/ s16 unk26;
/*28*/ s8 unk28;
/*29*/ s8 unk29;
/*2A*/ s8 unk2A;
/*2B*/ s8 unk2B;
/*2C*/ f32 x2;
/*30*/ f32 y2;
/*34*/ f32 z2;
/*38*/ f32 unk38;
/*3C*/ f32 unk3C;
/*40*/ f32 unk40;
/*44*/ f32 unk44;
/*48*/ Object *cloudrunner;
/*4C*/ Object *shiphead;
/*50*/ void *dll;
/*54*/ f32 x3;
/*58*/ f32 y3;
/*5C*/ f32 z3;
/*60*/ f32 x4;
/*64*/ f32 y4;
/*68*/ f32 z4;
/*6C*/ u32 soundHandle;
/*70*/ u32 soundHandle2;
/*74*/ s16 unk74;
/*76*/ s16 unk76;
/*78*/ s16 unk78;
/*7A*/ s16 unk7A;
/*7C*/ s8 state;
/*7D*/ s8 unk7D;
/*7E*/ s16 mapID;
/*80*/ u8  unk80;
/*82*/ s16 unk82;
/*84*/ s8 unk84;
/*85*/ s8 unk85;
/*86*/ s8 unk86;
/*88*/ s16 _unk88;
/*8A*/ u8 unk8A;
/*8B*/ u8 unk8B;
/*8C*/ s16 hintCounter;
/*8E*/ s8 unk8E;
} SB_Galleon_Data;

typedef struct {
/*00*/ Texture *tex1;
/*04*/ Texture *tex2;
/*08*/ s8 fadeoutStarted;
} Data0;

extern Data0 _data_0;

enum SB_Galleon_State {
    STATE_0,
    STATE_1,
    STATE_2,
    STATE_3,
    STATE_4
};

/** Deletes the Galleon's cannons */
static void galleon_delete_cannons() {
    Object** objects;
    s32 i;
    s32 count;

    for (objects = get_world_objects(&i, &count); i < count; i++) {
        if (objects[i]->id == OBJ_SB_ShipGun) {
            obj_destroy_object(objects[i]);
        }
    }
}

/** 
  * Sets the approximate initial state `SB_Galleon` would have when first entering State 1 of its State Machine 
  * TODO: revisit once Galleon DLLs are documented, make sure this doesn't cause instability.
  */
static void skip_galleon_fight(Object* self) {
    SB_Galleon_Data *objData;
    ObjSetup *objSetup;

    if (!self) {
        return;
    }

    objData = self->data;
    objSetup = self->setup;

    if (objData->state >= STATE_1) {
        return;
    }

    self->srt.yaw = M_90_DEGREES;
    objData->unk29 = 6;
    gDLL_12_Minic->vtbl->func7(0);
    gDLL_12_Minic->vtbl->func8(0);
    gDLL_12_Minic->vtbl->func9(0.0f, 25.0f);

    if (objData->unk8A == 0) {
        objData->unk8A = 1;
    }

    objData->state = STATE_1;
    gDLL_28_ScreenFade->vtbl->fade_reversed(25, SCREEN_FADE_BLACK);
    self->srt.transl.x = objSetup->x;
    self->srt.transl.y = objSetup->y;
    self->srt.transl.z = objSetup->z;

    if (objData->soundHandle != 0) {
        gDLL_6_AMSFX->vtbl->func_A1C(objData->soundHandle);
        objData->soundHandle = 0;
    }

    gDLL_29_Gplay->vtbl->set_obj_group_status(SOME_MAP_ID, 3, 0);
    gDLL_29_Gplay->vtbl->set_obj_group_status(SOME_MAP_ID, 2, 1);
    gDLL_29_Gplay->vtbl->set_obj_group_status(SOME_MAP_ID, 5, 1);

    if (!gDLL_29_Gplay->vtbl->get_obj_group_status(self->unk34, 1)) {
        gDLL_29_Gplay->vtbl->set_obj_group_status(self->unk34, 1, 1);
    }
    if (!gDLL_29_Gplay->vtbl->get_obj_group_status(self->unk34, 3)) {
        gDLL_29_Gplay->vtbl->set_obj_group_status(self->unk34, 3, 1);
    }

    gDLL_3_Animation->vtbl->func17(0, self, -1);
    gDLL_30_Task->vtbl->mark_task_completed(1);

    galleon_delete_cannons();
}

/** Automatically skip the Galleon fight (optional convenience when debugging) */
RECOMP_HOOK_DLL(SB_Galleon_setup) void galleon_setup_hook(Object *self) {
    if (DEBUG_SKIP_GALLEON_FIGHT) {
        skip_galleon_fight(self);
    }    
}

/** Allows the Galleon fight to be skipped with a button code */
RECOMP_HOOK_DLL(SB_Galleon_control) void galleon_control_hook(Object *self) {
    static u8 rsPosition = 0;
    static u16 rsCheatCode[10] = {
        B_BUTTON,
        A_BUTTON,
        D_JPAD,
        B_BUTTON,
        A_BUTTON,
        D_JPAD,
        L_TRIG,
        A_BUTTON,
        D_JPAD,
        START_BUTTON
    };
    static s16 soundIDs[4] = {
        0x5e0,
        0x5e3,
        0x71b,
        0x8b0
    };
    static ButtonCode galleonCheat;
    SB_Galleon_Data* objData = self->data;

    //Return if the Galleon's already defeated
    if (objData->state > STATE_0) {
        return;
    }

    //Set up button code
    if (galleonCheat.initialised == FALSE) {
        button_code_setup(
            &galleonCheat, 
            rsCheatCode, 
            ARRAYCOUNT(rsCheatCode)
        );
    }

    //Button sequence entered
    if (!galleonCheat.finished && button_code_entered(&galleonCheat)) {
        gDLL_6_AMSFX->vtbl->play_sound(NULL, 
            soundIDs[rand_next(0, ARRAYCOUNT(soundIDs) - 1)], 
            MAX_VOLUME, 0, 0, 0, 0
        );

        skip_galleon_fight(self);
        main_block_pausing(PauseBlock_Next_Tick_Only);
    }    
}

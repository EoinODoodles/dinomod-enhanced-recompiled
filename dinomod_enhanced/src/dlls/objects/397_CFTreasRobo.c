#include "modding.h"

#include "dlls/engine/6_amsfx.h"
#include "dlls/engine/17_partfx.h"
#include "dlls/engine/26_curves.h"
#include "dlls/objects/217_GuardClaw.h"
#include "dlls/objects/396_CFSupTreasureCh.h"
#include "game/gamebits.h"
#include "game/objects/object.h"
#include "game/objects/object_id.h"
#include "sys/gfx/animseq.h"
#include "sys/gfx/textable.h"
#include "sys/main.h"
#include "sys/map_enums.h"
#include "sys/objects.h"
#include "sys/objexpr.h"
#include "sys/objprint.h"
#include "sys/objtype.h"
#include "sys/objlib.h"
#include "sys/objhits.h"
#include "sys/rand.h"
#include "dll.h"
#include "macros.h"

#include "recomp/dlls/objects/397_CFTreasRobo_recomp.h"

typedef struct {
    ObjSetup base;
    u8 unk18;
} CFTreasRobo_Setup;

typedef struct {
    f32 unk0;
    s16 unk4;
    s16 unk6[12];
    s16 unk1E[12];
    s16 unk36;
    s16 unk38;
    s16 unk3A;
} CFTreasRobo_Data_15C;

typedef struct {
    UnkCurvesStruct unk0;
    u8 _unk108[0x13C - 0x108];
    u32 unk13C;
    Vec3f unk140;
    f32 unk14C;
    Object* chest;
    Object* unk154;
    Object* beam;
    CFTreasRobo_Data_15C unk15C;
    u8 _unk198[0x19E - 0x198];
    s16 unk19E;    
    u8 state;
    u8 _unk1A1;
    u8 unk1A2;
    u8 unk1A3;
    u8 unk1A4;
    u8 unk1A5;
    u8 unk1A6;
    u8 unk1A7;
    u8 _unk1A8[0x1AC - 0x1A8];
    // @recomp:
    u8 createdEnergyBar;
} CFTreasRobo_Data;

extern void CFTreasRobo_func_FAC(Vec3f* arg0, Object* arg1, s16* arg2);
extern s32 CFTreasRobo_func_1168(Object* self);
extern s32 CFTreasRobo_func_1214(Object* self, f32 x, f32 y, f32 z);
extern void CFTreasRobo_func_13DC(Object*, s32, Vec3f*, s32);
extern s32 CFTreasRobo_func_14AC(Object*);
extern void CFTreasRobo_func_1544(Object*, UnkCurvesStruct*, s32, s32, f32);
extern s32 CFTreasRobo_func_15D0(Object*, UnkCurvesStruct*);
extern Object* CFTreasRobo_func_168C(Object*, s32);
extern void CFTreasRobo_func_16A0(CFTreasRobo_Data_15C* arg0);
extern void CFTreasRobo_func_16B0(Object*, CFTreasRobo_Data_15C*, f32, f32, f32, f32);
extern s32 CFTreasRobo_func_1844(Object*, CFTreasRobo_Data_15C*);
extern void CFTreasRobo_func_1B50(f32, f32, f32, s16*, s16*);
extern void CFTreasRobo_func_1BEC(s16, s16, Vec3f*, f32);

static int custom_CFTreasRobo_animCallback(Object* self, Object* animObj, AnimObj_Data* animObjData, s8 a3);

RECOMP_PATCH void CFTreasRobo_obj_Setup(Object* self, CFTreasRobo_Setup* setup, s32 reset) {
    CFTreasRobo_Data* objdata;

    STUBBED_PRINTF(" ROBOT  ROBOT  CREATED \n\n\n");

    objdata = self->data;
    objdata->state = 0;
    objdata->chest = NULL;
    objdata->unk19E = 0;
    objdata->unk1A2 = 0;
    objdata->unk1A4 = 0;
    objdata->unk1A6 = 4;
    objdata->unk1A7 = 0;
    objAddObjectType(self, OBJTYPE_CFTreasRobo);
    if (setup->unk18 != 0) {
        objdata->state = 14;
        objdata->unk1A2 = 2;
        objdata->unk1A4 = 2;
    }
    CFTreasRobo_func_16A0(&objdata->unk15C);
    objdata->beam = NULL;
    self->srt.transl.x = setup->base.x;
    self->srt.transl.y = setup->base.y;
    self->srt.transl.z = setup->base.z;
    gDLL_1_cmdmenu->vtbl->energy_bar_create(0, objdata->unk1A6, TEXTABLE_56D, TEXTABLE_570, objdata->unk1A6);
    // @recomp: Track energy bar creation and add a custom anim callback
    objdata->createdEnergyBar = TRUE;
    self->animCallback = custom_CFTreasRobo_animCallback;
}

RECOMP_PATCH void CFTreasRobo_obj_Control(Object* self) {
    CFTreasRobo_Data* objdata = self->data;
    ObjSetup* beamSetup;
    s32 sp68[] = {0x00000001, 0x00000003, 0x00000007, 0x0000000f};
    s32 sp60[] = {0x00000020, 0x00000007};
    Object* baddie;
    Vec3f sp50;
    f32 sp4C;
    f32 var_fv1;
    SeqJoint* sp44;
    s16 var_v1;
    s16 sp38;

    // @recomp: Recreate energy bar after seq ends
    if (!objdata->createdEnergyBar) {
        objdata->createdEnergyBar = TRUE;
        gDLL_1_cmdmenu->vtbl->energy_bar_create(0, objdata->unk1A6, TEXTABLE_56D, TEXTABLE_570, objdata->unk1A6);
        gDLL_1_cmdmenu->vtbl->energy_bar_set(objdata->unk1A6 - objdata->unk1A2);
    }

    sp4C = 200.0f;
    if (objdata->unk154 == NULL) {
        objdata->unk154 = CFTreasRobo_func_168C(self, 0x147);
    }
    if (objdata->beam == NULL) {
        beamSetup = objAllocSetup(sizeof(ObjSetup), OBJ_RobottrackBeam);
        beamSetup->x = self->srt.transl.x;
        beamSetup->y = self->srt.transl.y;
        beamSetup->z = self->srt.transl.z;
        beamSetup->loadFlags = OBJSETUP_LOAD_MANUAL;
        beamSetup->fadeFlags = OBJSETUP_FADE_MANUAL;
        objdata->beam = objSetupObject(beamSetup, OBJINIT_STANDALONE | OBJINIT_FLAG4, -1, -1, self->parent);
    }
    if (objdata->unk154 != NULL) {
        objdata->unk154->srt.transl.x = self->srt.transl.x;
        objdata->unk154->srt.transl.y = self->srt.transl.y;
        objdata->unk154->srt.transl.z = self->srt.transl.z;
    }
    if (objdata->state == 0xE) {
        if (mainGetBits(BIT_334) != 0) {
            STUBBED_PRINTF(" First Robot Activated ");
            objdata->state = 0;
        }
        return;
    }

    if (CFTreasRobo_func_14AC(self) == 3) {
        mainSetBits(BIT_8C9, 1);
    }
    if (CFTreasRobo_func_14AC(self) == 2) {
        mainSetBits(BIT_8CA, 1);
    }
    if (objdata->chest != NULL) {
        baddie = objGetNearestTypeTo(OBJTYPE_Baddie, self, &sp4C);
        if ((baddie != NULL) && (baddie->id == OBJ_GuardClaw)) {
            STUBBED_PRINTF(" Asking to Stand Aside %i ", 0); // guessed location, unknown arg
            ((DLL_217_GuardClaw*)baddie->dll)->vtbl->Func10(baddie, sp4C < 150.0f);
        }
    }
    if (objdata->unk1A3 != 0) {
        if ((objdata->unk15C.unk4 < 0x28) && (CFTreasRobo_func_15D0(self, &objdata->unk0) != 0)) {
            objdata->unk1A3 = 0;
            objdata->unk140.f[0] = objdata->unk0.unk0.unk68.x;
            objdata->unk14C = objdata->unk0.unk0.unk68.y; // ?
            objdata->unk140.f[2] = objdata->unk0.unk0.unk68.z;
        }
    } else {
        switch (objdata->state) {
        case 0:
            STUBBED_PRINTF(" Get Treasure");
            CFTreasRobo_func_1544(self, &objdata->unk0, 0, sp68[objdata->unk1A4], 2000.0f);
            objdata->unk1A3 = 1;
            objdata->state = 1;
            break;
        case 1:
            if (CFTreasRobo_func_1168(self) != 0) {
                CFTreasRobo_func_13DC(self, sp68[objdata->unk1A4 + 1], &objdata->unk140, 0);
                objdata->state = 2;
                objdata->unk1A5 = sp68[objdata->unk1A4 + 1];
            }
            break;
        case 2:
            if (CFTreasRobo_func_1214(self, objdata->unk140.x, objdata->unk140.y + 70.0f, objdata->unk140.z) != 0) {
                objdata->state = 3;
            }
            break;
        case 3:
            CFTreasRobo_func_1544(self, &objdata->unk0, objdata->unk1A7, objdata->unk1A5, 2000.0f);
            objdata->unk1A3 = 1;
            objdata->state = 8;
            break;
        case 8:
            if (objdata->chest != NULL) {
                objdata->state = 9;
                objdata->unk1A2++;
                gDLL_1_cmdmenu->vtbl->energy_bar_set(objdata->unk1A6 - objdata->unk1A2);
                ((DLL_396_CFSupTreasureCh*)objdata->chest->dll)->vtbl->Func9(objdata->chest, self, objdata->unk14C);
                objdata->chest = NULL;
            }
            STUBBED_PRINTF(" Hello ");
            if (objdata->unk1A2 == 2) {
                objdata->unk1A4 = 2;
                STUBBED_PRINTF(" On next Nodes ");
            }
            if (objdata->unk1A2 >= objdata->unk1A6) {
                // @recomp: fadeout instead, looks nicer
                //gDLL_1_cmdmenu->vtbl->energy_bar_free();
                gDLL_1_cmdmenu->vtbl->energy_bar_fadeout();
                objdata->state = 10;
            }
            CFTreasRobo_func_13DC(self, sp68[objdata->unk1A4], &objdata->unk140, 0);
            objdata->unk0.unk9C->pos.z -= 40.0f; //?????
            break;
        case 11:
            if (CFTreasRobo_func_1214(self, objdata->unk140.x, objdata->unk140.y + 70.0f, objdata->unk140.z) != 0) {
                CFTreasRobo_func_1544(self, &objdata->unk0, 1, objdata->unk1A5, 2000.0f);
                objdata->unk1A3 = 1;
                objdata->state = 1;
                STUBBED_PRINTF(" Tunnel is collapsing ");
                objdata->unk1A4 = 2;
                objdata->unk1A5 = 0xF;
                objdata->unk1A2 = 2;
            }
            break;
        case 9:
            if (CFTreasRobo_func_1214(self, objdata->unk140.x, objdata->unk140.y + 70.0f, objdata->unk140.z) != 0) {
                objdata->state = 0;
            }
            break;
        case 10:
            mainSetBits(BIT_8F7, 0);
            mainSetBits(BIT_Play_Seq_02C7_Scales_Takes_Baby_Cloudrunner_Away, 1);
            mainSetBits(BIT_CRF_Galleon_Fade_to_High_Detail, 1);
            mainSetBits(BIT_CRF_Galleon_Fade_to_Low_Detail, 0);
            dll_gplay->set_obj_group_status(MAP_CLOUDRUNNER_FORTRESS, 30, 1);
            dll_gplay->set_obj_group_status(MAP_CLOUDRUNNER_FORTRESS, 9, 1);
            STUBBED_PRINTF(" Picked Up Treasure ");
            STUBBED_PRINTF(" Sorry You Have Failed ");
            if (mapGetLayer() != 0) {
                mapIncrementLayer();
            }
            objdata->state = 15;
            break;
        case 12:
            STUBBED_PRINTF(" TP");
            mainSetBits(BIT_334, 1);
            if (objdata->unk14C < self->srt.transl.y) {
                self->velocity.y -= 0.05f;
                self->srt.transl.y += self->velocity.y;
            } else {
                self->srt.transl.y = objdata->unk14C;
            }
            self->srt.transl.y += 0.2f * mathSinfInterp(objdata->unk19E);
            break;
        case 13:
            if (objdata->unk14C < self->srt.transl.y) {
                self->velocity.y -= 0.05f;
                self->srt.transl.y += self->velocity.y;
            } else {
                self->srt.transl.y = objdata->unk14C;
            }
            self->srt.transl.y += 0.2f * mathSinfInterp(objdata->unk19E);
            break;
        case 15:
            break;
        default:
            break;
        }
    }
    if (objdata->chest != NULL) {
        ((DLL_396_CFSupTreasureCh*)objdata->chest->dll)->vtbl->Func8(objdata->chest, self);
    }
    var_fv1 = sqrtf(SQ(self->srt.transl.z - self->prevLocalPosition.z) + SQ(self->srt.transl.x - self->prevLocalPosition.x)) / 15.0f;
    var_fv1 = CLAMP_EXPR(var_fv1, -1.0f, 1.0f);;
    sp38 = var_fv1 * 5461.0f;
    self->srt.pitch = self->srt.pitch + ((sp38 - self->srt.pitch) >> 3);
    sp50.f[0] = self->srt.transl.x - self->prevLocalPosition.x;
    sp50.f[1] = self->srt.transl.y - self->prevLocalPosition.y;
    sp50.f[2] = self->srt.transl.z - self->prevLocalPosition.z;
    if ((sp50.f[0] != 0.0f) || (sp50.f[1] != 0.0f)) {
        var_v1 = mathAtan2(sp50.f[0], sp50.f[2]) & 0xFFFF;
        var_v1 = var_v1 - self->srt.yaw;
        CIRCLE_WRAP(var_v1);
        self->srt.yaw = self->srt.yaw + (var_v1 >> 2);
    }
    sp44 = objExpr_func_80034804(self, 0);
    if (sp44 != NULL) {
        sp44->pitch = (mathSinfInterp(objdata->unk19E) * (sp38 * 0.0625f)) + -(sp38 * 2);
    }
    if (CFTreasRobo_func_1844(self, &objdata->unk15C) != 0) {
        if (objdata->unk13C != 0) {
            gDLL_6_AMSFX->vtbl->Stop(objdata->unk13C);
            objdata->unk13C = 0;
        }
    }
}

RECOMP_PATCH void CFTreasRobo_obj_Free(Object* self, s32 onlySelf) {
    CFTreasRobo_Data* objdata = self->data;
    
    if (onlySelf == 0) {
        if (objdata->beam != NULL) {
            objFreeObject(objdata->beam);
        }
    }
    if ((objdata->unk154 != NULL) && (onlySelf == 0)) {
        objFreeObject(objdata->unk154);
    }
    objFreeObjectType(self, OBJTYPE_CFTreasRobo);
    // @recomp: fadeout instead, looks nicer
    //gDLL_1_cmdmenu->vtbl->energy_bar_free();
    gDLL_1_cmdmenu->vtbl->energy_bar_fadeout();
    gDLL_13_Expgfx->vtbl->func5(self);
    STUBBED_PRINTF("ROBOT KILLED ");
}

RECOMP_PATCH u32 CFTreasRobo_obj_GetDataSize(Object* self, u32 offsetAddr) {
    return sizeof(CFTreasRobo_Data); // @recomp: custom size
}

static int custom_CFTreasRobo_animCallback(Object* self, Object* animObj, AnimObj_Data* animObjData, s8 a3) {
    CFTreasRobo_Data* objdata = self->data;

    // @recomp: Hide energy bar during seq
    if (objdata->createdEnergyBar) {
        objdata->createdEnergyBar = FALSE;
        gDLL_1_cmdmenu->vtbl->energy_bar_free();
    }

    return 0;
}

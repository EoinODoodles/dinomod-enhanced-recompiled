
#include "modding.h"
#include "recomputils.h"

#include "game/objects/interaction_arrow.h"
#include "game/objects/object_id.h"
#include "dlls/engine/6_amsfx.h"
#include "dlls/engine/53_movelib.h"
#include "dlls/engine/54_pickup.h"
#include "sys/intersect.h"
#include "sys/main.h"
#include "sys/math.h"
#include "sys/objects.h"
#include "sys/objtype.h"
#include "sys/print.h"
#include "sys/rand.h"
#include "dll.h"

#include "recomp/dlls/objects/388_CFbarrel_recomp.h"

typedef struct {
/*00*/ Pickup pickup;
/*0C*/ Object* unkC;
/*10*/ u8 _unk10;
/*11*/ u8 unk11;
/*12*/ u8 unk12;
/*13*/ u8 unk13;
/*14*/ s32 unk14;
/*18*/ Vec3f unk18;
/*24*/ f32 unk24;
/*28*/ f32 unk28;
/*2C*/ f32 unk2C;
/*30*/ s16 unk30;
/*32*/ s8 unk32;
/*34*/ s32 unk34;
/*38*/ s16 unk38;
/*3A*/ s16 unk3A;
/*3C:0*/ u8 unk3C_0 : 1;
/*3C:1*/ u8 unk3C_1 : 1;
/*3C:2*/ u8 unk3C_2 : 1;
/*3D*/ u8 unk3D;
/*3E:0*/ u8 unk3E_0 : 1;
/*3E:1*/ u8 unk3E_1 : 1;
} CFbarrel_Data;

void CFbarrel_func_AD4(Object* self);
void CFbarrel_func_1214(Object* self);
void CFbarrel_func_14A0(Object* self, s16 arg1, s16 arg2);
void CFbarrel_func_1848(Object* self);
void CFbarrel_func_1948(Object* self, u8 a1);

RECOMP_HOOK_DLL(CFbarrel_setup) void CFbarrel_setup_hook(Object* self, ObjSetup* setup, s32 reset) {
    // @recomp: CFbarrel obj update already handles this and the automatic set can conflict in a way
    //          that makes the barrel not detect the hitlines that force an exit from windlifts
    self->srt.flags |= OBJFLAG_MANUAL_PREV_POSITIONS;
}

RECOMP_PATCH void CFbarrel_control(Object* self) {
    CFbarrel_Data* objdata = self->data;
    
    if (objdata->unk14 != 0) {
        objdata->unk14 = objdata->unk14 - gUpdateRate;
        if (objdata->unk14 <= 0) { // @recomp: fix condition
            objdata->unk14 = 0;
            objdata->unk13 = 0;
            objdata->unk12 = 0;
            objdata->unk3D |= 1;
            obj_func_80023C6C(self); // @recomp: Reset lockdata on respawn since we modify it in another patch
        }
    } else {
        if ((self->parent != NULL) && (self->parent->id == OBJ_DR_PushCart)) {
            self->unkAF |= ARROW_FLAG_8_No_Targetting;
        } else {
            self->unkAF &= ~ARROW_FLAG_8_No_Targetting;
        }
        if (mapWorldCoordsToBlockIndex(self->globalPosition.x, self->globalPosition.y, self->globalPosition.z) != -1) {
            if (objdata->unk13 != 0) {
                objdata->unk13 = objdata->unk13 + gUpdateRate;
                objdata->unk24 = (objdata->unk2C * (f32) objdata->unk13) + 1.0f;
                func_8002683C(self, (s16) objdata->unk24, (s16) (-objdata->unk24 * 0.5f), (s16) (objdata->unk24 * 0.5f));
                gDLL_17_partfx->vtbl->spawn(self, PARTICLE_353, NULL, 0, -1, NULL);
                gDLL_17_partfx->vtbl->spawn(self, PARTICLE_353, NULL, 0, -1, NULL);
                gDLL_17_partfx->vtbl->spawn(self, PARTICLE_353, NULL, 0, -1, NULL);
                if (objdata->unk13 >= 0x15) {
                    objdata->unk3E_0 = 0;
                    // Move to "create point" node
                    ((DLL_53_movelib*)gTempDLLInsts[1])->vtbl->func7(0x1A, &self->srt);
                    STUBBED_PRINTF(" Should has Set position  %i %f %f \n", 0x1A, &self->srt.transl.x, &self->srt.transl.z);
                    self->srt.transl.x += (f32) mathRnd(-30, 30) * 0.1f;
                    self->srt.transl.z += (f32) mathRnd(-30, 30) * 0.1f;
                    bzero(&objdata->unk18, sizeof(Vec3f));
                    bzero(&self->velocity, sizeof(Vec3f));
                    if (objdata->unk3C_0) {
                        mapSaveObject(self->setup, self->mapID, self->srt.transl.x, self->srt.transl.y, self->srt.transl.z);
                        objdata->unk14 = 0x258;
                        func_80026160(self);
                        func_8002683C(self, 8, -2, 0x19);
                    } else {
                        objDisable(self);
                        func_800267A4(self);
                        self->srt.flags |= OBJSTATE_PRINT_DISABLED;
                    }
                }
            } else {
                if (gDLL_54_pickup->vtbl->control(self, &objdata->pickup) == 0) {
                    if (objdata->unk11 != 0) {
                        objAddObjectType(self, OBJTYPE_WindLiftable);
                    }
                    objdata->unk11 = 0;
                    func_8002674C(self);
                    if (objdata->unk3D & 1) {
                        CFbarrel_func_AD4(self);
                    }
                    CFbarrel_func_1214(self);
                } else {
                    objdata->unk3D |= 1;
                    objdata->unk11 = 1;
                    objdata->unk3E_1 = 1;
                    // @recomp: Disable voxmap lock icon check after getting picked up for the first time. It's too easy
                    //          to place the barrel partway in a wall...
                    obj_func_80023BF8(self, 0, 0, 0, 0, 0x6);
                }
                CFbarrel_func_1848(self);
                if (objdata->unk3E_0) {
                    self->unkAF |= ARROW_FLAG_8_No_Targetting;
                    if ((objdata->unk3E_1) && (objdata->unk3E_0)) {
                        objdata->unk18.x = self->velocity.x;
                        objdata->unk18.y = self->velocity.y;
                        objdata->unk18.z = self->velocity.z;
                        objdata->unk18.y = 0.0f;
                        objdata->unk3E_1 = 0;
                    }
                }
                if (objdata->unk3C_0) {
                    mapSaveObject(self->setup, self->mapID, self->srt.transl.x, self->srt.transl.y, self->srt.transl.z);
                }
            }
        }
    }
}

RECOMP_PATCH void CFbarrel_update(Object* self) {
    CFbarrel_Data* objdata = self->data;
    f32 sp90[3];
    TrackLineIntersectResult sp3C;

    if ((objdata->unk13 == 0) && (objdata->unk14 == 0)) {
        if (objdata->unkC != NULL) {
            trackIntersect_func_8005B5B8(self, objdata->unkC, 1);
            objdata->unkC = 0;
        }
        if (objdata->unk3E_0) {
            diPrintf(" floating ");
            sp90[0] = self->srt.transl.x - self->prevLocalPosition.x;
            sp90[1] = self->srt.transl.y - self->prevLocalPosition.y;
            sp90[2] = self->srt.transl.z - self->prevLocalPosition.z;
            sp90[0] *= 0.99f * (1.0f / gUpdateRateF);
            sp90[1] *= 0.99f * (1.0f / gUpdateRateF);
            sp90[2] *= 0.99f * (1.0f / gUpdateRateF);
            objdata->unk18.x += sp90[0];
            objdata->unk18.y += sp90[1];
            objdata->unk18.z += sp90[2];
            sp90[1] = 0.0f;
            objdata->unk3D |= 1;
            objdata->unk18.x *= 0.5f;
            objdata->unk18.y = 0.0f;
            objdata->unk18.z *= 0.5f;
        }
        if (objdata->unk11 == 0) {
            if (trackGetLineIntersect(&self->prevLocalPosition, &self->srt.transl, 4.0f, 1, &sp3C, self, 8, -1, 0xFF, 0) != 0) {
                STUBBED_PRINTF(" Line IDNO %i \n", sp3C.unk51);
                if ((objdata->unk3E_0) && (sp3C.unk51 == 3)) {
                    recomp_printf("hit windlift end hitline\n");
                    CFbarrel_func_1948(self, 0);
                } else {
                    // @recomp: Cap speed here or else the following code can result in super high velocities
                    //          if the barrel is ping-ponging off of two close walls.
                    self->velocity.x = CLAMP_EXPR(self->velocity.x * -1.1f, -5.0f, 5.0f);
                    self->velocity.z = CLAMP_EXPR(self->velocity.z * -1.1f, -5.0f, 5.0f);
                    objdata->unk18.x = CLAMP_EXPR(objdata->unk18.x * -1.1f, -5.0f, 5.0f);
                    objdata->unk18.z = CLAMP_EXPR(objdata->unk18.z * -1.1f, -5.0f, 5.0f);
                    // self->velocity.x *= -1.1f;
                    // self->velocity.z *= -1.1f;
                    // objdata->unk18.x *= -1.1f;
                    // objdata->unk18.z *= -1.1f;
                    STUBBED_PRINTF(" Hit Line ");
                }
            }
        }
        self->prevLocalPosition.x = self->srt.transl.x;
        self->prevLocalPosition.y = self->srt.transl.y;
        self->prevLocalPosition.z = self->srt.transl.z;
        // @recomp: A patch above disables the automatic set of prevGlobalPosition, so do it ourselves
        self->prevGlobalPosition.x = self->globalPosition.x;
        self->prevGlobalPosition.y = self->globalPosition.y;
        self->prevGlobalPosition.z = self->globalPosition.z;
    }
}

RECOMP_PATCH void CFbarrel_func_AD4(Object* self) {
    CFbarrel_Data* objdata = self->data;
    Object* obj;
    f32 temp_fa1;
    f32 temp_fv1;
    f32 var_fa0;
    f32 pad;
    s16 var_s2;
    s32 temp_v0;
    s32 spCC[4]; // size?
    s32 spC8; // s0
    s32 var_s3;
    s32 var_s4;
    s32 var_v1;
    f32 spB8;
    f32 temp;
    f32 spB0;
    f32 spAC;
    TrackHeightResult** spA8;
    s16 sp98[] = {0xffff, 0x0000, 0x0000, 0x0001, 0x0001, 0x0000, 0x0000, 0xffff};
    s16 sp90[] = {0x0002, 0x0003, 0x0000, 0x0001};
    s32* temp_v0_2;
    // @recomp: new vars
    s32 numCloseFloors = 0;
    _Bool anyFloorsBelow = FALSE;

    self->srt.yaw = 0;
    var_s2 = 0;
    var_s4 = 0;
    if (objdata->unk18.y > 0.01f) {
        objdata->unk18.y -= 0.1f;
        self->velocity.x = objdata->unk18.x * gUpdateRateF;
        self->velocity.y = objdata->unk18.y * gUpdateRateF;
        self->velocity.z = objdata->unk18.z * gUpdateRateF;
        objMove(self, self->velocity.x, self->velocity.y, self->velocity.z);
        self->srt.roll += (objdata->unk18.y * 500.0f);
        return;
    }
    spAC = 0.0f;
    for (var_s3 = 0; var_s3 < 4; var_s3++) {
        spB8 =  (10.0f * mathSinfInterp(var_s2)) * mathCosfInterp(self->srt.roll);
        spB0 = (10.0f * mathCosfInterp(var_s2)) * mathCosfInterp(self->srt.pitch);
        temp = ((10.0f * mathSinfInterp(self->srt.roll)) * mathSinfInterp(var_s2)) + (10.0f * mathSinfInterp(self->srt.pitch) * mathCosfInterp(var_s2));

        var_s2 += 0x3FD2;

        temp_v0 = trackGetHeight(self, 
                                self->srt.transl.x + spB8, 
                                self->srt.transl.y + temp,
                                self->srt.transl.z + spB0, 
                                &spA8, 0, 1);
        if (temp_v0 != 0) {
            var_fa0 = 10000.0f;

            for (var_v1 = 0; var_v1 < temp_v0; var_v1++) {
                temp_fv1 = self->srt.transl.y - spA8[var_v1]->y;
                if (
                    (temp_fv1 > 0.0f && temp_fv1 < (var_fa0 >= 0 ? var_fa0 : -var_fa0)) || 
                    (temp_fv1 <= 0.0f && temp_fv1 > -20.0f)
                ) {
                    var_fa0 = temp_fv1;
                    spC8 = var_v1;
                }

                // @recomp: Record whether there is anything below us at all
                if (temp_fv1 >= 0.0f) {
                    anyFloorsBelow = TRUE;
                }

                // @recomp: Also track floors that are below us but pretty close
                if (temp_fv1 > 0.0f && temp_fv1 < 2.0f) {
                    numCloseFloors++;
                }
            }
          
            if (var_fa0 <= 0.0f) {
                temp_v0_2 = &spCC[var_s4];
                var_s4 += 1;
                if (var_fa0 < spAC) {
                    spAC = var_fa0;
                }
                objdata->unk18.y = 0.0f;
                if (var_fa0 > 0.0f) {
                    *temp_v0_2 = var_s3;
                } else {
                    *temp_v0_2 = sp90[var_s3];
                }
            }
        }
    }
    if (var_s4 != 0) {
        self->velocity.y = -spAC;
    }
    if (var_s4 == 0) {
        self->velocity.y = objdata->unk18.y * gUpdateRateF;
        self->velocity.x = objdata->unk18.x * gUpdateRateF;
        self->velocity.z = objdata->unk18.z * gUpdateRateF;
        objdata->unk18.y -= 0.1f;// @recomp: moved to prevent falling when windlift unloads
        if (objdata->unk18.y < -0.2f) {
            CFbarrel_func_14A0(self, objdata->unk38, objdata->unk3A);
        }
        // @recomp: Check if we're in the void and if so just respawn
        if (!anyFloorsBelow) {
            objdata->unk13 = 1;
        }
    }
    if (var_s4 > 0) {
        //recomp_printf(" Landed On World Obj \n");
        objdata->unk3D &= ~0x1;
        obj = spA8[spC8]->obj;
        if (obj != NULL) {
            if ((obj->def->flags & OBJDEF_IS_MOBILE_MAP) && !(obj->def->flags & OBJDEF_MOBILE_MAP_NEVER_PLAYER_PARENT)) {
                objdata->unkC = obj;
                //recomp_printf(" ob Obj %x  Defno %i \n\n", obj, obj->id);
            }
        }
        objdata->unk18.x = 0.0f;
        objdata->unk18.y = 0.0f;
        objdata->unk18.z = 0.0f;
        self->srt.roll = 0;
        self->srt.pitch = 0;
    }
    if ((objdata->unk3E_0) && (var_s4 >= 3)) { // @recomp: only require 3 test points instead of 4
        //recomp_printf(" landed for real \n");
        CFbarrel_func_1948(self, 0);
    } 
    // @recomp: If in a windlift and there's enough floors that are "close enough", then we're probably
    //          outside of the windlift. This is almost certainly the floor of the blown up entrance
    //          to the "treasure" windlift in front of the throne room. The angled floor there doesn't
    //          play nice with the vanilla floor check and the windlift radius extends out there slightly.
    else if (objdata->unk3E_0 && numCloseFloors >= 3) {
        CFbarrel_func_1948(self, 0);
    }
    objMove(self, self->velocity.x, self->velocity.y, self->velocity.z);
}

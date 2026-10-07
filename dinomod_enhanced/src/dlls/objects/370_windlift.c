#include "modding.h"

#include "dlls/objects/210_player.h"
#include "sys/objects.h"
#include "sys/objmsg.h"
#include "sys/print.h"

#include "recomp/dlls/objects/370_WindLift_recomp.h"

// size: 0x18
typedef struct {
    Object* unk0;
    f32 unk4;
    f32 unk8;
    f32 unkC;
    u8 unk10;
    u8 unk11;
    s32 unk14;
} WindLift_Data_1C;

typedef struct {
    s32 windLiftID;
    s32 playerInsideBit;
    s32 reverseBit;
    u8 _unkC[0x10 - 0xC];
    u32 unk10;
    f32 unk14;
    s32 unk18;
    WindLift_Data_1C unk1C[14];
    u8 _unk16C[0x170 - 0x16C];
    u32 unk170_0 : 1;
} WindLift_Data;

RECOMP_HOOK_DLL(WindLift_obj_Free) void WindLift_obj_Free_hook(Object* self, s32 onlySelf)  {
    WindLift_Data* objdata = self->data;

    // @recomp: Announce to windliftable objects when we're unloading and let them decide whether
    //          to treat this as a real exit message or not.
    for (s32 i = 2; i < 14; i++) {
        WindLift_Data_1C* slot = &objdata->unk1C[i];
        if (slot->unk14 == -1) {
            break;
        }
        if (slot->unk0 != NULL && (slot->unk10 & 0x40)) { // 0x40 == IsBeingLifted
            Object* obj = slot->unk0;
            // For saftey, zero out the object's velocity
            obj->velocity.x = 0.0f;
            obj->velocity.y = 0.0f;
            obj->velocity.z = 0.0f;

            // Set bit 12 to state that we're unloading
            s32 mesgArg = (1 << 12) | (((slot->unk10 & 0xE0) >> 4) << 8) | objdata->windLiftID;
            objSendMesg(obj, 0x10, self, (void* ) (mesgArg));
        }
    }
}

RECOMP_PATCH void WindLift_func_880(Object* self, Object* obj, WindLift_Data_1C* arg2, f32 arg3, s32 switchedOff, s32 isPlayer, s32 windLiftID) {
    Object* player;
    f32 objHeight;
    f32 temp_fv0_2;
    f32 var_fa0;
    f32 xzDist;
    f32 var_ft5;
    f32 var_fv0_2;
    s32 temp;

    player = objGetPlayer();
    objHeight = obj->srt.transl.y - self->srt.transl.y;
    if (objHeight < 0.0f) {
        return;
    }
    xzDist = vec3DistanceXZ(&obj->globalPosition, &self->globalPosition);
    //diPrintf("\nwl dist %f (%s)", &xzDist, obj->def->name);
    if (!(xzDist > (110.0f + 3.0f)) || (arg2->unk10 & 0xE0)) {
        if (!(arg2->unk10 & 0x80) || !switchedOff) {
            if (xzDist < 110.0f) {
                if (!(arg2->unk10 & 0xE0) || (arg2->unk10 & 0x80)) {
                    //if (((!arg2->unk10) & 0x80) && (objHeight < 20.0f)) { // @bug? should probably be !(arg2->unk10 & 0x80)
                    if ((!(arg2->unk10 & 0x80)) && (objHeight < 20.0f)) { // @recomp: fix condition
                        arg2->unk10 |= 0x80;
                        return;
                    }
                    if (arg2->unk10 & 2) {
                        if ((objHeight / arg3) > 0.8f) {
                            arg2->unk10 |= 4;
                            arg2->unk10 &= ~0x8;
                        } else {
                            arg2->unk10 |= 8;
                            arg2->unk10 &= ~0x4;
                        }
                        arg2->unk10 &= ~0x2;
                    }
                    if (!switchedOff) {
                        arg2->unk10 |= 0x40;
                        arg2->unk10 &= ~0x20;
                        temp = (((s32) (arg2->unk10 & 0xE0) >> 4) << 8) | windLiftID;
                        objSendMesg(obj, 0xF, self, (void* ) (temp));
                        arg2->unk10 &= ~0x80;
                    } else {
                        arg2->unk10 |= 0x20;
                        arg2->unk10 &= ~0x40;
                    }
                    //do { } while (0); // @fake
                }
                var_ft5 = 0.324f;
                if ((arg2->unk10 & 0xE) && (arg2->unk10 & 8) && !switchedOff) {
                    arg3 *= 0.6f;
                }
                arg3 *= 0.6f;
                if (!(arg3 <= 10.0f)) {
                    if (objHeight < 3.0f) {
                        objHeight = 3.0f;
                    }
                    if (!switchedOff) {
                        temp_fv0_2 = arg3 - (arg2->unkC * arg2->unkC * arg2->unkC * (arg3 / 50.0f));
                        var_fa0 = (temp_fv0_2 - objHeight) < 0.0f
                            ? 0.0f
                            : (temp_fv0_2 - objHeight) > 20
                                ? 1.0f
                                : (temp_fv0_2 - objHeight) / 20;
                        //if (arg2->unk10){} // @fake
                        arg2->unk10 |= 1;
                        if ((arg2->unkC < -0.2f && (arg2->unk11 % 2)) || (arg2->unkC > 0.2f && !(arg2->unk11 % 2))) {
                            if (arg2->unk10 & 8) {
                                if (arg2->unk11++ > 2) {
                                    arg2->unk10 &= ~0x8;
                                    arg2->unk10 |= 4;
                                }
                            }
                        }
                    } else {
                        if (arg2->unk10 & 0xE) {
                            var_fv0_2 = 0.1f;
                        } else {
                            var_fv0_2 = 0.5f;
                        }
                        if (var_fv0_2 < arg2->unkC) {
                            arg2->unk11 = 1;
                        }
                        var_ft5 *= 1.5f;
                        if (arg2->unk11 == 0) {
                            if (arg2->unk10 & 0xE) {
                                var_fa0 = 1.0f - (objHeight / (arg3 * 1.55f));
                            } else {
                                var_fa0 = 1.0f - (objHeight / (arg3 * 1.1f));
                            }
                            if (var_fa0 < 0) {
                                var_fa0 = 0.0f;
                            }
                            var_fa0 *= var_fa0;
                        } else {
                            var_fa0 = 0.01f;
                        }
                    }
                    arg2->unk8 = (var_ft5 * var_fa0) - 0.18f;
                    arg2->unkC += arg2->unk8;
                    if (arg2->unkC > 8.0f) {
                        arg2->unkC = 8.0f;
                    }
                    if (arg2->unkC == 0.0f) {
                        arg2->unkC = -0.001f;
                    }
                    if ((objHeight < 20.0f) && switchedOff) {
                        arg2->unk11 = 0;
                        arg2->unkC = 0.0f;
                        temp = (((s32) (arg2->unk10 & 0xE0) >> 4) << 8) | windLiftID;
                        objSendMesg(obj, 0x10, self, (void* ) (temp));
                        arg2->unk10 |= 0x80;
                        if (isPlayer) {
                            player->velocity.y = 0.0f;
                        }
                    }
                    if (isPlayer) {
                        ((DLL_210_Player*)obj->dll)->vtbl->func58(obj, arg2->unkC);
                        return;
                    }
                    obj->srt.transl.y += arg2->unkC * gUpdateRateF;
                    obj->velocity.y = arg2->unkC * gUpdateRateF;
                }
            } else {
                if (isPlayer) {
                    ((DLL_210_Player*)obj->dll)->vtbl->func58(obj, 0);
                }
                if (!isPlayer && (xzDist < (110.0f + 3.0f))) {
                    temp = (((s32) (arg2->unk10 & 0xE0) >> 4) << 8) | windLiftID;
                    objSendMesg(obj, 0x10, self, (void* ) (temp));
                    arg2->unk10 &= ~0xF1;
                    if (arg2->unk10 & 0xE) {
                        arg2->unk10 |= 2;
                    }
                    arg2->unkC = 0.0f;
                    arg2->unk11 = 0;
                }
            }
        }
    }
}

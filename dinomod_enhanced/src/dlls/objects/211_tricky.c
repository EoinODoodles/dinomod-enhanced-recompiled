#include "modding.h"
#include "recomputils.h"
#include "dll_util.h"
#include "sidekick_util.h"

#include "dlls/engine/26_curves.h"
#include "dlls/engine/27.h"
#include "dlls/engine/29_gplay.h"
#include "game/gamebits.h"
#include "sys/curves.h"
#include "sys/dll.h"
#include "sys/main.h"
#include "sys/objects.h"
#include "sys/objexpr.h"
#include "sys/objtype.h"
#include "sys/print.h"
#include "sys/route.h"
#include "dll.h"

#include "recomp/dlls/objects/211_Tricky_recomp.h"

typedef s32 (*DLL210_Unk5FC_Callback)(Object* , s32, void *, Object* );

// size: 0x610
typedef struct {
    /*000*/ void *unk0; // holds a pointer to a (loaded) DLL
    /*004*/ SidekickStats *stats;
    /*008*/ Object* player;
    /*00C*/ s32 unkC;
    /*010*/ s32 unk10;
    /*014*/ s32 unk14;
    /*018*/ u8 state; // TrickyState
    /*019*/ u8 unk19;
    /*01A*/ u8 unk1A; // some sort of state?
    /*01B*/ u8 unk1B; //bitfield of available sidekick commands
    /*01C*/ u8 unk1C;
    /*01D*/ u8 unk1D;
    /*01E*/ u16 pad1E;
    /*020*/ f32 unk20;
    /*024*/ f32 unk24;
    /*028*/ Object* unk28;
    /*02C*/ Vec3f *pathGoalPos;
    /*030*/ f32 unk30[2];
    /*038*/ f32 unk38;
    /*03C*/ f32 unk3C;
    /*040*/ f32 unk40;
    /*044*/ f32 unk44;
    /*048*/ f32 unk48;
    /*04C*/ s32 flags; // TrickyFlags
    /*050*/ f32 unk50;
    /*054*/ f32 unk54;
    /*058*/ f32 unk58;
    /*05C*/ f32 unk5C;
    /*060*/ f32 unk60;
    /*064*/ s16 unk64;
    /*066*/ s16 unk66;
    /*068*/ Vec3f *unk68;
    /*06C*/ s16 unk6C[4];
    /*074*/ Vec3f unk74[4];
    /*0A4*/ u16 currWalkGroup;
    /*0A6*/ s16 unkA6;
    /*0A8*/ Vec3f unkA8;
    /*0B4*/ Vec3f unkB4;
    /*0C0*/ Vec3f unkC0;
    /*0CC*/ DLL27_Data unkCC;
    /*32C*/ Object* unk32C;
    /*330*/ f32 unk330;
    /*334*/ s32 hitByDamageType;
    /*338*/ Object* hitBy;
    /*33C*/ HeadAnimation unk33C;
    /*360*/ HeadAnimation unk360;
    /*384*/ Vec3f unk384[4];
    /*3B4*/ Vec3f unk3B4;
    /*3C0*/ CurveSetup *unk3C0;
    /*3C4*/ u8 unk3C4;
    /*3C8*/ UnkCurvesStruct unk3C8;
    /*4D0*/ CurveSetup *unk4D0;
    /*4D4*/ CurveSetup *unk4D4;
    /*4D8*/ u16 unk4D8;
    /*4DA*/ u8 unk4DA;
    /*4DB*/ u8 pad4DB;
    /*4DC*/ Route routes[4];
    /*59C*/ Route unk59C;
    /*5CC*/ CurveSetup *unk5CC;
    /*5D0*/ void *unk5D0;
    /*5D4*/ Vec3f *unk5D4;
    /*5D8*/ Vec3f unk5D8;

    // unk5E4 .. unk600 are dynamic based on something?
    /*5E4*/ Object* unk5E4[3]; // objects or floats or curve setups or object setups?
    /*5F0*/ union {
        // sometimes float or u32 as well
        Object* unk5F0;
        f32 unk5F0_f32;
        u32 unk5F0_u32;
    };
    /*5F4*/ union {
        Vec3f unk5F4_vec;
        struct {
            union {
                CurveSetup *unk5F4;
                u32 unk5F4_amsFxID;
            };
            union {
                CurveSetup *unk5F8;
                s32 unk5F8_s32;
            };
            union {
                DLL210_Unk5FC_Callback unk5FC;
                s32 unk5FC_s32;
            };
        };
    };
    /*600*/ f32 unk600;
    /*604*/ Object* unk604;
    /*608*/ s32 unk608;
    /*60C*/ u8 unk60C;
} Tricky_Data;

enum TrickyState {
    TRICKY_STATE_0 = 0,
    TRICKY_STATE_1 = 1,
    TRICKY_STATE_2 = 2,
    TRICKY_STATE_3 = 3,
    TRICKY_STATE_4 = 4,
    TRICKY_STATE_5 = 5,
    TRICKY_STATE_6 = 6,
    TRICKY_STATE_7 = 7,
    TRICKY_STATE_8 = 8,
    TRICKY_STATE_9 = 9,
    TRICKY_STATE_10 = 10,
    TRICKY_STATE_11 = 11,
    TRICKY_STATE_12 = 12,
    TRICKY_STATE_13 = 13,
    TRICKY_STATE_14 = 14,
    TRICKY_STATE_15 = 15,
    TRICKY_STATE_16 = 16,
    TRICKY_STATE_17 = 17
};

enum TrickyFlags {
    TRICKY_FLAG_UNK1 = 0x1,
    TRICKY_FLAG_UNK2 = 0x2,
    TRICKY_FLAG_UNK4 = 0x4,
    TRICKY_FLAG_UNK8 = 0x8,
    TRICKY_FLAG_UNK10 = 0x10,
    TRICKY_FLAG_UNK20 = 0x20,
    TRICKY_FLAG_UNK40 = 0x40,
    TRICKY_FLAG_UNK80 = 0x80,
    TRICKY_FLAG_UNK100 = 0x100,
    TRICKY_FLAG_UNK200 = 0x200,
    TRICKY_FLAG_UNK400 = 0x400,
    TRICKY_FLAG_EmittingFlames = 0x800,
    TRICKY_FLAG_UNK1000 = 0x1000,
    TRICKY_FLAG_UNK2000 = 0x2000,
    TRICKY_FLAG_UNK4000 = 0x4000,
};

extern Vec3f _data_74[2];
extern f32 _data_8C[2];
extern Vec3f _data_94[1];
extern f32 _data_A0;
extern f32 _data_A4[18];
extern u8 _data_EC[120];

extern int tricky_animCallback(Object* self, Object* animObj, AnimObj_Data* animObjData, s8 arg3);
extern void tricky_func_8ED0(f32 arg0, f32 arg1, f32* arg2);
extern f32 tricky_func_81D8(Object* self);
extern void tricky_func_8470(Object* self, Vec3f* arg1);
extern s32 tricky_func_9668(DLL27_Data* objData);
extern CurveSetup* tricky_func_7A7C(Tricky_Data* objData, CurveSetup* arg1, s32 arg2, u8 arg3);
extern void tricky_func_8974(Object* self, UnkCurvesStruct* arg1, f32 arg2);
extern void tricky_func_507C(Object* self, Tricky_Data* objData);
extern void tricky_func_514C(Object* self, Tricky_Data* objData);
extern void tricky_func_52B8(Object* self, Tricky_Data* objData);
extern void tricky_findRouteStarts(Object* self, u8* routeFlags, s16 currWalkGroup, CurveSetup** startCurves);
extern s8 tricky_func_7BAC(Tricky_Data* objData, CurveSetup** startCurves, u8* routeFlags, s32 goalWalkGroup);
extern void tricky_func_87E4(Object* self);
extern void tricky_func_8A94(Object* self, UnkCurvesStruct* arg1);
extern void tricky_func_8F84(Object* self, Vec3f* arg1, f32* arg2);

typedef void (*ObjControlFunc)(Object *obj);
static ObjControlFunc tricky_control_func; 
static void tricky_control_hijack(Object *self);

RECOMP_HOOK_DLL(tricky_ctor) void tricky_ctor_hook(DLLFile *dll) {
    tricky_control_func = dinomod_hijack_dll_export(dll, OBJEXPORT_CONTROL, tricky_control_hijack);
}

RECOMP_HOOK_RETURN_DLL(tricky_dtor) void tricky_dtor_hook(void) {
    tricky_control_func = NULL;
}

static void tricky_control_hijack(Object *self) {
    // @recomp: If the map Tricky is on unloads, unload him and skip the control function.
    if (!dinomod_unload_sidekick_if_map_unloaded(self)) {
        recomp_printf("[dinomod] Unloading Tricky due to map unload!\n");
        return;
    }

    tricky_control_func(self);
}

/** Stop a possible buffer overflow when Tricky spawns. Original patch by MusicalProgrammer */
RECOMP_PATCH void tricky_obj_Setup(Object* self, ObjSetup* setup, s32 arg2) {
    f32 var_ft5;
    f32 var_fv0;
    f32 var_fv1;
    Tricky_Data* objData = self->data;
    s32 pad;
    s32 pad2;
    s32 i;
    u8 pad3;
    u8 sp50[] = {1, 2};
    s32 pad4;
    s32 temp_v1;

    mainSetBits(BIT_4E3, 0xFF);
    if (mainGetBits(BIT_SW_Tricky_Ball_Collected) != 0) {
        mainSetBits(BIT_Tricky_Ball_Unlocked, 1);
    }

    self->animCallback = tricky_animCallback;
    objAddObjectType(self, OBJTYPE_Sidekick);
    routeInit(&objData->routes[0]);
    routeInit(&objData->routes[1]);
    routeInit(&objData->routes[2]);
    routeInit(&objData->routes[3]);
    routeInit(&objData->unk59C);
    objData->stats = gDLL_29_Gplay->vtbl->get_sidekick_stats();
    objData->player = objGetPlayer();
    temp_v1 = _data_EC[map_func_80045D58()];
    objData->state = TRICKY_STATE_0;
    objData->unk1B = 0;
    objData->unk5D4 = 0;
    objData->currWalkGroup = 0;
    objData->unk14 = temp_v1;
    objData->unk10 = temp_v1;
    objData->unkC = temp_v1;
    gDLL_27->vtbl->init(&objData->unkCC, 
        DLL27FLAG_2000000, 
        DLL27FLAG_80 | DLL27FLAG_20 | DLL27FLAG_4 | DLL27FLAG_2 | DLL27FLAG_1, 
        DLL27MODE_1);
    gDLL_27->vtbl->setup_hits_collider(&objData->unkCC, 1, _data_94, &_data_A0, 2);
    gDLL_27->vtbl->setup_terrain_collider(&objData->unkCC, 2, _data_74, _data_8C, sp50);
    gDLL_25->vtbl->LoadWalkGroups();
    // @recomp: Don't rely on float variable for the loop condition (potentially avoid overrun)
    for (i = 0, var_ft5 = 0/*.0f*/; i < 18; i++, var_ft5 += 0.1f) {
        for (var_fv0 = var_ft5, var_fv1 = 0.0f; var_fv0 > 0.0f; var_fv0 -= 0.09f) {
            var_fv1 += var_fv0 * 3.0f;
        }
        _data_A4[i] = var_fv1 * 1.75f;
    }
}

RECOMP_PATCH void tricky_func_940C(Object* self, Tricky_Data* objData) {
    s32 i;

    // @recomp: Don't run this function if flags & 0x800 is not set. The pointers loaded
    //          below will not be valid in that case. It seems that area of state is a union
    //          that holds unrelated memory when Tricky is in other states.
    if (!(objData->flags & TRICKY_FLAG_EmittingFlames)) {
        return;
    }
    objData->flags &= ~TRICKY_FLAG_EmittingFlames;
    objData->flags |= TRICKY_FLAG_UNK1000;
    // @recomp: Do null checks before unloading stuff, also reset pointers to null after.
    //          objFreeObject will crash if given a null/invalid pointer.
    if (objData->unk0 != NULL) {
        dllFree(objData->unk0);
        objData->unk0 = NULL;
    }
    if (objData->unk5F0 != NULL) {
        objFreeObject(objData->unk5F0);
        objData->unk5F0 = NULL;
    }
    for (i = 0; i < 3; i++) {
        if (objData->unk5E4[i] != NULL) {
            objFreeObject(objData->unk5E4[i]);
            objData->unk5E4[i] = NULL;
        }
    }
}

/** 
 * - Fixed odd Tricky crash when jumping off ledges. Original patch by MusicalProgrammer.
 * - Let tricky reroute when stuck outside of a walk group (by Shinx).
 */
RECOMP_PATCH s32 tricky_func_53E4(Object* self, f32 arg1, Tricky_Data* objData) {
    s32 pad;
    s32 inWalkGroup;
    s32 goalWalkGroup;
    s32 bitWalkGroup;
    Vec3f* goalPos;
    f32 xzDist;
    f32 var_fv1;
    u8 routeFlags[4];
    s16 patchGroupID;
    s16 var_s1;
    CurveSetup* spA0;
    s32 var_v1;
    s32 i;
    CurveSetup* routeStarts[4];
    f32 sp84; // fv0?
    CurveSetup* setup;
    Vec3f sp74;
    f32 sp68[2];
    s32 pad2;
    s16 sp60[3];
    CurveSetup* setup2;
    u8 sp5B;
    u8 temp_s1_4;
    u8 mask;
#ifdef AVOID_UB
    s32 temp_v1_4;
#else
    u8 temp_v1_4;
#endif
    f32 sp54;
    f32 sp50;
    WalkGroupInfo goalWalkGroupInfo;

    goalPos = objData->pathGoalPos;
    inWalkGroup = gDLL_25->vtbl->FindWalkGroup(&self->srt.transl, NULL);
    if (inWalkGroup != 0 && inWalkGroup != objData->currWalkGroup) {
        objData->currWalkGroup = inWalkGroup;
        objData->flags &= ~TRICKY_FLAG_UNK400;
        for (i = 0; i < 4; i++) { objData->unk6C[i] = 0; }
    }
    goalWalkGroup = gDLL_25->vtbl->FindWalkGroup(goalPos, &goalWalkGroupInfo);
    bitWalkGroup = mainGetBits(BIT_Tricky_Walk_Group);
    if (goalWalkGroup != 0 && goalWalkGroup != bitWalkGroup) {
        bitWalkGroup = goalWalkGroup;
        mainSetBits(BIT_Tricky_Walk_Group, goalWalkGroup);
    }
    sp84 = objData->unk20;
    objData->unk20 += -0.03f * gUpdateRateF;
    if (objData->unk20 < 0.0f) {
        objData->unk20 = 0.0f;
    }
    xzDist = vec3DistanceXZSquared(goalPos, &self->srt.transl);
    i = objData->unk20 * 10.0f;
    if (i >= 18) {
        var_fv1 = 100.0f;
    } else {
        var_fv1 = _data_A4[i] + arg1;
        var_fv1 *= var_fv1;
    }
    if (var_fv1 < xzDist) {
        tricky_func_8ED0(sp84, 1.5f, &objData->unk20);
    } else if (xzDist > 400.0f) {
        xzDist = tricky_func_81D8(self);
        if (objData->unk20 < xzDist && xzDist < sp84) {
            objData->unk20 = xzDist;
        }
    }
    if (goalWalkGroup == objData->currWalkGroup) {
        objData->flags |= TRICKY_FLAG_UNK400;
        for (i = 0, mask = 1; i < 4; i++, mask <<= 1) {
            if (goalWalkGroupInfo.patchFlags & mask) {
                objData->unk6C[i] = goalWalkGroupInfo.patchIDs[i];
                objData->unk74[i].x = goalPos->x;
                objData->unk74[i].y = goalPos->y;
                objData->unk74[i].z = goalPos->z;
            }
        }
    }
    if (goalWalkGroup != 0 && goalWalkGroup == objData->currWalkGroup) {
        objData->unkA6 = 0;
    } else {
        patchGroupID = objData->currWalkGroup * goalWalkGroup;
        if (patchGroupID != 0) {
            for (i = 0; i < 4; i++) {
                if ((patchGroupID == goalWalkGroupInfo.patchIDs[i]) && (goalWalkGroupInfo.patchFlags & (1 << i))) {
                    objData->unkA6 = patchGroupID;
                    objData->unkA8.x = goalPos->x;
                    objData->unkA8.y = goalPos->y;
                    objData->unkA8.z = goalPos->z;
                }
            }
        }
    }
    gDLL_25->vtbl->IsPointWithinAnyGroup(goalPos);
    gDLL_25->vtbl->GetPatchGroupContainingPoint(goalPos, objData->currWalkGroup);
    if (objData->flags & TRICKY_FLAG_UNK400) {
        // @the fakest of fakes (this is an empty loop in default.dol)
        for (i = 0; i < 4; i++) {
            // FAKE
            do { } while (0);
            if (goalWalkGroupInfo.patchFlags & (1 << i)) {
            }
        }
    }
    patchGroupID = gDLL_25->vtbl->GetPatchGroupContainingPoint(goalPos, objData->currWalkGroup);
    // diPrintf("goalWalkGroup: %d\n", goalWalkGroup);
    // diPrintf("inWalkGroup: %d\n", inWalkGroup);
    // diPrintf("currWalkGroup: %d\n", objData->currWalkGroup);
    // diPrintf("unk19: %d\n", objData->unk19);
    // diPrintf("patchGroupID: %d\n", patchGroupID);
    // diPrintf("sp84: %f\n", &sp84);
    // diPrintf("unk20: %f\n", &objData->unk20);
    if (objData->unk19 != 0xF && 
            ((goalWalkGroup != 0 && inWalkGroup == goalWalkGroup) || 
                (patchGroupID != 0 && gDLL_25->vtbl->IsPointWithinPatchGroup(&self->srt.transl, objData->currWalkGroup, patchGroupID) != 0))) {
        objData->unk19 = 1;
    } else if (objData->unk19 < 5) {
        patchGroupID = gDLL_25->vtbl->GetPatchGroupContainingPoint(goalPos, objData->currWalkGroup);\
        if (patchGroupID != 0) {
            if (goalWalkGroup == 0) {
                for (i = 0; i < 4; i++) {
                    if (patchGroupID == objData->unk6C[i]) {
                        sp5B = i;
                        objData->unk19 = 2;
                        break;
                    }
                }
                if (i == 4) {
                    objData->unk19 = 0;
                }
            } else if (inWalkGroup != 0) {
                for (i = 0; i < 4; i++) {
                    if (patchGroupID == objData->unk6C[i]) {
                        sp5B = i;
                        objData->unk19 = 2;
                        break;
                    }
                }
                if (i == 4) {
                    objData->unk19 = 5;
                }
            } else {
                if (inWalkGroup == 0 && 
                        (patchGroupID = gDLL_25->vtbl->GetPatchGroupContainingPoint(&self->srt.transl, objData->currWalkGroup), (patchGroupID != 0))) {
                    if (patchGroupID == objData->unkA6) {
                        objData->unk19 = 3;
                    } else {
                        gDLL_25->vtbl->GetClosestExitOfPatch(&objData->player->srt.transl, &objData->unkC0, patchGroupID);
                        objData->unk19 = 4;
                    }
                } else {
                    objData->unk19 = 0;
                    gDLL_25->vtbl->IsPointWithinPatchGroup(&self->srt.transl, objData->currWalkGroup, patchGroupID);
                    // @recomp: Let tricky reroute if he is stuck outside of a walk group but the goal has moved to a different group.
                    //          Without this, tricky will just be stuck pretty much forever. Don't try to continue direct-to-goal movement
                    //          if tricky was in the same group as the goal, instead the walk group bounds should be fixed.
                    if (objData->currWalkGroup != 0 && goalWalkGroup != objData->currWalkGroup) {
                        recomp_eprintf("[tricky] stuck outside of a walk group, rerouting...\n");
                        objData->unk19 = 5;
                        inWalkGroup = objData->currWalkGroup;
                    } else {
                        recomp_eprintf("[tricky] stuck outside of walk group %d! pls fix\n", objData->currWalkGroup);
                    }
                }
            }
        } else if (goalWalkGroup == 0) {
            objData->unk19 = 0;
        } else if (inWalkGroup != 0) {
            patchGroupID = goalWalkGroup * inWalkGroup;
            if (gDLL_25->vtbl->IsPointWithinPatchGroup(&self->srt.transl, objData->currWalkGroup, patchGroupID) != 0) {
                if (patchGroupID == objData->unkA6) {
                    objData->unk19 = 3;
                } else {
                    objData->unk19 = 5;
                }
            } else {
                for (i = 0; i < 4; i++) {
                    if (patchGroupID == objData->unk6C[i]) {
                        sp5B = i;
                        objData->unk19 = 2;
                        break;
                    }
                }
                if (i == 4 || patchGroupID != objData->unkA6) {
                    objData->unk19 = 5;
                }
            }
        } else {
            patchGroupID = gDLL_25->vtbl->GetPatchGroupContainingPoint(&self->srt.transl, objData->currWalkGroup);\
            if (patchGroupID != 0) {
                if (goalWalkGroup == objData->currWalkGroup) {
                    for (i = 0; i < 4; i++) {
                        if (patchGroupID == objData->unk6C[i]) {
                            sp5B = i;
                            objData->unk19 = 2;
                            break;
                        }
                    }
                    if (i == 4) {
                        gDLL_25->vtbl->GetClosestExitOfPatch(&objData->player->srt.transl, &objData->unkC0, patchGroupID);
                        objData->unk19 = 4;
                    }
                } else if (patchGroupID == objData->unkA6) {
                    objData->unk19 = 3;
                } else {
                    gDLL_25->vtbl->GetClosestExitOfPatch(&objData->player->srt.transl, &objData->unkC0, patchGroupID);
                    objData->unk19 = 4;
                }
            } else {
                // @recomp: Add in default.dol code to bring tricky to a stop when stuck
                objData->unk19 = 0; // default.dol
                // @recomp: Let tricky reroute if he is stuck outside of a walk group but the goal has moved to a different group.
                //          Without this, tricky will just be stuck pretty much forever. Don't try to continue direct-to-goal movement
                //          if tricky was in the same group as the goal, instead the walk group bounds should be fixed.
                if (objData->currWalkGroup != 0 && goalWalkGroup != objData->currWalkGroup) {
                    recomp_eprintf("[tricky] stuck outside of a walk group, rerouting...\n");
                    objData->unk19 = 5;
                    inWalkGroup = objData->currWalkGroup;
                } else {
                    recomp_eprintf("[tricky] stuck outside of walk group %d! pls fix\n", objData->currWalkGroup);
                }
            }
        }
    }
    switch (objData->unk19) {
    case 0:
        objData->unk20 = (-0.03f * gUpdateRateF) + sp84;
        if (objData->unk20 < 0.0f) {
            objData->unk20 = 0.0f;
        }
        tricky_func_8470(self, goalPos);
        break;
    default:
        break;
    case 1:
        if (sp84 != 0.0f) {
            tricky_func_8470(self, goalPos);
            break;
        }
        /* fallthrough */
    case 2:
        if (sp84 != 0.0f) {
            tricky_func_8470(self, &objData->unk74[sp5B]);
            break;
        }
        /* fallthrough */
    case 4:
        if (sp84 != 0.0f) {
            tricky_func_8470(self, &objData->unkC0);
        }
        break;
    case 3:
        if (sp84 != 0.0f) {
            tricky_func_8470(self, &objData->unkA8);
            break;
        }

        if (tricky_func_9668(&objData->unkCC) == 0) {
            sp68[0] = objData->pathGoalPos->x - self->srt.transl.x;
            sp68[1] = objData->pathGoalPos->z - self->srt.transl.z;
            var_s1 = (u16)mathAtan2f(-sp68[0], -sp68[1]);
            var_s1 = self->srt.yaw - (var_s1 & 0xFFFF);
            CIRCLE_WRAP(var_s1);
            var_v1 = (var_s1 >= 0 ? var_s1 : -var_s1);
            if (var_v1 >= 0x1A17) {
                var_s1 = (var_s1 * 5) / 4;
                if (var_s1 > 0) {
                    if (var_s1 >= 0x342F) {
                        objAnimSet(self, 12, 0.0f, 0);
                        var_v1 = (var_s1 >= 0 ? var_s1 : -var_s1);
                        objData->unk48 = var_v1 / 24490.0f;
                        objData->unk38 = 0.01f;
                    } else {
                        objAnimSet(self, 10, 0.0f, 0);
                        var_v1 = (var_s1 >= 0 ? var_s1 : -var_s1);
                        objData->unk48 = var_v1 / 6679.0f;
                        objData->unk38 = 0.06f;
                    }
                } else if (var_s1 >= 0x342F) {
                    objAnimSet(self, 11, 0.0f, 0);
                    var_v1 = (var_s1 >= 0 ? var_s1 : -var_s1);
                    objData->unk48 = var_v1 / 24490.0f;
                    objData->unk38 = 0.01f;
                } else {
                    objAnimSet(self, 9, 0.0f, 0);
                    var_v1 = (var_s1 >= 0 ? var_s1 : -var_s1);
                    objData->unk48 = var_v1 / 6679.0f;
                    objData->unk38 = 0.06f;
                }
                objData->unk19 = 0xF;
                objData->unk38 /= objData->unk48;
                objData->unk3C = 1.0f;
                objData->unk40 = 1.0f;
                objData->flags |= TRICKY_FLAG_UNK20;
                objData->flags |= TRICKY_FLAG_UNK40;
                objData->flags |= TRICKY_FLAG_UNK100;
            }
        }
        break;
    case 6:
        vec3DistanceXZSquared(&objData->unk3C0->pos, &self->srt.transl);
        if (vec3DistanceXZSquared(&objData->unk3C0->pos, &self->srt.transl) < 60.0f) {
            setup = objData->unk3C0;
            objData->unk3C8.unk0.unk80 = objData->unk3C4;
            spA0 = tricky_func_7A7C(objData, setup, bitWalkGroup, objData->unk3C4);
            gDLL_25->vtbl->func_1D30(&objData->unk3C8, setup, spA0, tricky_func_7A7C(objData, spA0, bitWalkGroup, objData->unk3C4));
            curves_func_800053B0(&objData->unk3C8.unk0, 0.1f);
            tricky_func_8974(self, &objData->unk3C8, objData->unk20);
            tricky_func_8470(self, &objData->unk3C8.unk0.unk68);
            if (setup != NULL && spA0 != NULL /*&& ? != NULL*/) {
                // TODO: the third param should be using the result of the 2nd tricky_func_7A7C call, but i can't get it to match 
                STUBBED_PRINTF("walk types are %d %d %d\n", setup->unk1A, spA0->unk1A, 0/*?->unk1A*/);
            }
            switch (setup->unk1A) {
            case 1:
                tricky_func_507C(self, objData);
                break;
            case 5:
                tricky_func_514C(self, objData);
                break;
            case 6:
                tricky_func_52B8(self, objData);
                break;
            case 2:
            case 7:
                objData->flags |= TRICKY_FLAG_UNK2000;
                /* fallthrough */
            default:
                objData->unk19 = 7;
                break;
            }
        } else {
            tricky_func_8470(self, &objData->unk3C0->pos);
        }
        break;
    case 5:
        tricky_findRouteStarts(self, routeFlags, inWalkGroup, routeStarts);
        
        i = tricky_func_7BAC(objData, routeStarts, routeFlags, bitWalkGroup);
        if (i == -1) {
            objData->unk20 = sp84;
            return 2;
        }
        objData->unk3C4 = routeFlags[i];
        objData->unk3C0 = routeStarts[i];

        // FAKE
        if (1) {
            tricky_func_8470(self, &objData->unk3C0->pos);
        }
        objData->unk19 = 6;
        break;
    case 7:
        sp74.x = objData->pathGoalPos->x - self->srt.transl.x;
        sp74.y = objData->pathGoalPos->y - self->srt.transl.y;
        sp74.z = objData->pathGoalPos->z - self->srt.transl.z;
        sp60[0] = -self->srt.yaw;
        sp60[1] = 0;
        sp60[2] = 0;
        mathRotateRPY((SRT*)&sp60, sp74.f);
        if (sp74.z > 0.0f && objData->unk20 != 0.0f) {
            if (!(objData->flags & TRICKY_FLAG_UNK2) || (objData->unk64 != bitWalkGroup) || (objData->unk68 != objData->pathGoalPos) || (objData->unk66 != objData->unk3C8.unk0.unk80)) {
                objData->flags |= TRICKY_FLAG_UNK2;
                objData->flags &= ~TRICKY_FLAG_UNK4;
                objData->unk64 = bitWalkGroup;
                objData->unk68 = objData->pathGoalPos;
                objData->unk66 = objData->unk3C8.unk0.unk80;
                routeSetup(&objData->routes[0], objData->unk3C8.unkA0, objData->pathGoalPos, (void* ) bitWalkGroup, objData->unk3C8.unk0.unk80 ^ 1);
                routeSetup(&objData->routes[1], objData->unk3C8.unkA0, objData->pathGoalPos, (void* ) bitWalkGroup, objData->unk3C8.unk0.unk80);
            }
            if (!(objData->flags & TRICKY_FLAG_UNK4)) {
                temp_v1_4 = routeFind(&objData->routes[1], 10);
                switch (temp_v1_4) {
                case 1:
                    objData->flags &= ~TRICKY_FLAG_UNK2;
                    break;
                case -1:
                    objData->flags |= TRICKY_FLAG_UNK4;
                    break;
                default:
                    temp_v1_4 = routeFind(&objData->routes[0], 10);
                    if (temp_v1_4 != -1) {
                        if (temp_v1_4 == 1) {
                            objData->flags |= TRICKY_FLAG_UNK4;
                        }
                    } else {
                        objData->flags &= ~TRICKY_FLAG_UNK2;
                    }
                    break;
                }
            }
            objData->unk50 -= gUpdateRateF;
            if (objData->unk50 <= 0.0f && objData->flags & TRICKY_FLAG_UNK4) {
                temp_s1_4 = objData->unk3C8.unk0.unk80 ^ 1;
                objData->unk50 = 120.0f;
                if (!temp_s1_4) {
                    curves_func_800053B0(&objData->unk3C8.unk0, 2.0f);
                } else {
                    curves_func_800053B0(&objData->unk3C8.unk0, -2.0f);
                }
                objData->unk3C8.unk0.unk80 = temp_s1_4;
                gDLL_25->vtbl->func_29FC(&objData->unk3C8);
            }
        } else {
            objData->unk50 = 120.0f;
            objData->flags &= ~TRICKY_FLAG_UNK2;
        }
        if ((objData->unk3C8.unk0.unk80 == 0 && objData->unk3C8.unk0.unk10 != 0) || (objData->unk3C8.unk0.unk80 != 0 && objData->unk3C8.unk0.unk10 == 0)) {
            setup2 = tricky_func_7A7C(objData, objData->unk3C8.unkA4, bitWalkGroup, objData->unk3C8.unk0.unk80);
            if (setup2 != NULL) {
                //dummy:
                gDLL_25->vtbl->func_21F4(&objData->unk3C8, setup2);
                if ((objData->unk3C8.unk9C->unk1A == 2) || (objData->unk3C8.unk9C->unk1A == 7)) {
                    if (objData->flags & TRICKY_FLAG_UNK2000) {
                        objData->flags &= ~TRICKY_FLAG_UNK2000;
                    } else {
                        objData->flags |= TRICKY_FLAG_UNK2000;
                    }
                }
            } else {
                if (objData->unk3C8.unk0.unk80 != 0) {
                    curves_func_800053B0(&objData->unk3C8.unk0, 2.0f);
                } else {
                    curves_func_800053B0(&objData->unk3C8.unk0, -2.0f);
                }
                objData->unk3C8.unk9C = (CurveSetup *)((s32)objData->unk3C8.unk9C ^ (s32)objData->unk3C8.unkA4);\
                objData->unk3C8.unkA4 = (CurveSetup *)((s32)objData->unk3C8.unkA4 ^ (s32)objData->unk3C8.unk9C);\
                objData->unk3C8.unk9C = (CurveSetup *)((s32)objData->unk3C8.unk9C ^ (s32)objData->unk3C8.unkA4);
            }
        } else {
            setup2 = tricky_func_7A7C(objData, objData->unk3C8.unkA0, bitWalkGroup, objData->unk3C8.unk0.unk80);
            if (setup2 != objData->unk3C8.unkA4) {
                //dummy2:
                gDLL_25->vtbl->func_27A8(&objData->unk3C8, setup2);
            }
        }
        tricky_func_8974(self, &objData->unk3C8, objData->unk20);
        tricky_func_8470(self, &objData->unk3C8.unk0.unk68);
        switch (var_v1 = objData->unk3C8.unkA0->unk1A) {
        case 1:
            objData->unk19 = 8;
            break;
        case 5:
            objData->unk19 = 11;
            break;
        case 6:
            objData->unk19 = 13;
            break;
        }
        break;
    case 8:
        tricky_func_8ED0(sp84, 1.5f, &objData->unk20);
        tricky_func_8974(self, &objData->unk3C8, objData->unk20);
        tricky_func_8470(self, &objData->unk3C8.unk0.unk68);
        if ((objData->unk3C8.unk0.unk80 == 0 && objData->unk3C8.unk0.unk10 != 0) || (objData->unk3C8.unk0.unk80 != 0 && objData->unk3C8.unk0.unk10 == 0)) {
            gDLL_25->vtbl->func_21F4(&objData->unk3C8, tricky_func_7A7C(objData, objData->unk3C8.unkA4, bitWalkGroup, objData->unk3C8.unk0.unk80));
            tricky_func_507C(self, objData);
        }
        break;
    case 9:
        tricky_func_8ED0(sp84, 2.3f, &objData->unk20);
        tricky_func_87E4(self);
        if (self->animProgress < 0.5f) {
            objGetAnimChange(self, objData->unk20, &sp54);
            self->srt.transl.x += objData->unk30[0] * objData->unk20 * gUpdateRateF;
            self->srt.transl.z += objData->unk30[1] * objData->unk20 * gUpdateRateF;
        } else {
            objGetAnimChange(self, objData->unk20 * 0.25f, &sp54);
            self->srt.transl.x += objData->unk30[0] * (objData->unk20 * 0.25f) * gUpdateRateF;
            // FAKE
            //if (var_v1) {}
            self->srt.transl.z += objData->unk30[1] * (objData->unk20 * 0.25f) * gUpdateRateF;
        }
        if (objAnimAdvance(self, sp54, gUpdateRateF, NULL) != 0) {
            setup = objData->unk3C8.unkA0;
            xzDist = setup->pos.x - self->srt.transl.x;
            var_fv1 = setup->pos.z - self->srt.transl.z;
            xzDist = sqrtf(SQ(xzDist) + SQ(var_fv1));
            objData->unk54 = xzDist / 2.3f;
            objData->unk58 = 0.0f;
            objData->unk60 = self->srt.transl.y;
            objData->unk5C = (((setup->pos.y - self->srt.transl.y) - (-0.017f * objData->unk54 * objData->unk54)) / objData->unk54);
            objAnimSet(self, 0x16, 0.0f, 0);
            objAnimSetProgress(self, objData->unk58 / objData->unk54);
            objData->unk20 = 2.3f;
            objData->unk19 = 10;
            tricky_func_8A94(self, &objData->unk3C8);
        }
        objData->unk38 = 0.0f;
        break;
    case 10:
        objData->unk58 += gUpdateRateF;
        if (objData->unk54 <= objData->unk58) {
            self->srt.transl.y = objData->unk3C8.unkA0->pos.y;
            objAnimSetProgress(self, 1.0f);
            objData->unk19 = 7;
            objData->unk38 = 0.0f;
        } else {
            self->srt.transl.y = objData->unk60 + (objData->unk5C * objData->unk58) + (-0.017f * objData->unk58 * objData->unk58);
            var_fv1 = objData->unk54;
            if (var_fv1 <= 24.0f) {
                objAnimSetProgress(self, objData->unk58 / var_fv1);
                // @recomp: Initialize variable (fix potential crash when jumping off ledges)
                sp50 = 1.0f;
            } else {
                sp50 = 1.0f;
                if (objData->unk58 <= 6.0f) {
                    objAnimSetProgress(self, objData->unk58 / 24.0f);
                } else if ((var_fv1 - 6.0f) <= objData->unk58) {
                    objAnimSetProgress(self, ((24.0f - var_fv1) + objData->unk58) / 24.0f);
                    sp50 = (((objData->unk54 - objData->unk58) / 6.0f) * 0.75f) + 0.25f;
                } else {
                    objAnimSetProgress(self, (((objData->unk58 - 6.0f) / (var_fv1 - 12.0f)) * 0.5f) + 0.25f);
                }
            }
            objData->unkCC.mode = 0;
            objData->unk38 = 0.0f;
            self->srt.transl.x += objData->unk30[0] * 2.3f * gUpdateRateF * sp50;
            self->srt.transl.z += objData->unk30[1] * 2.3f * gUpdateRateF * sp50;
        }
        break;
    case 11:
        //STUBBED_PRINTF("i before %d\n", objData->unk3C8.unk0.unk10);
        tricky_func_8ED0(sp84, 1.5f, &objData->unk20);
        tricky_func_8974(self, &objData->unk3C8, objData->unk20);
        tricky_func_8470(self, &objData->unk3C8.unk0.unk68);
        //STUBBED_PRINTF("i now %d\n", objData->unk3C8.unk0.unk10);
        if ((objData->unk3C8.unk0.unk80 == 0 && objData->unk3C8.unk0.unk10 != 0) || (objData->unk3C8.unk0.unk80 != 0 && objData->unk3C8.unk0.unk10 == 0)) {
            gDLL_25->vtbl->func_21F4(&objData->unk3C8, tricky_func_7A7C(objData, objData->unk3C8.unkA4, bitWalkGroup, objData->unk3C8.unk0.unk80));
            tricky_func_514C(self, objData);
        }
        break;
    case 12:
    case 14:
        objData->unkCC.mode = 0;
        tricky_func_8974(self, &objData->unk3C8, objData->unk20);
        tricky_func_87E4(self);
        if (self->animProgress >= 0.95f) {
            objData->unk20 = 0.375f;
            tricky_func_8470(self, &objData->unk3C8.unk0.unk68);
            objData->unk19 = 7;
        }
        break;
    case 13:
        tricky_func_8ED0(sp84, 1.5f, &objData->unk20);
        tricky_func_8974(self, &objData->unk3C8, objData->unk20);
        tricky_func_8470(self, &objData->unk3C8.unk0.unk68);
        if ((objData->unk3C8.unk0.unk80 == 0 && objData->unk3C8.unk0.unk10 != 0) || (objData->unk3C8.unk0.unk80 != 0 && objData->unk3C8.unk0.unk10 == 0)) {
            gDLL_25->vtbl->func_21F4(&objData->unk3C8, tricky_func_7A7C(objData, objData->unk3C8.unkA4, bitWalkGroup, objData->unk3C8.unk0.unk80));
            tricky_func_52B8(self, objData);
        }
        break;
    case 15:
        if (self->animProgress >= 0.95f) {
            objData->flags &= ~TRICKY_FLAG_UNK20;
            objData->flags &= ~TRICKY_FLAG_UNK40;
            objData->flags &= ~TRICKY_FLAG_UNK100;
            objData->unk20 = 0.0f;
            tricky_func_8F84(self, goalPos, objData->unk30);
            objData->unk19 = 1;
            return 1;
        }
        break;
    }

    if (objData->unk19 < 5 && (gDLL_25->vtbl->IsPointWithinAnyGroup(&self->srt.transl) == FALSE)) {
        self->srt.transl.x = objData->unkB4.x;
        self->srt.transl.y = objData->unkB4.y;
        self->srt.transl.z = objData->unkB4.z;
    }
    objData->unkB4.x = self->srt.transl.x;
    objData->unkB4.y = self->srt.transl.y;
    objData->unkB4.z = self->srt.transl.z;
    if (objData->unk19 == 0) {
        return 2;
    }
    if (objData->unk20 == 0.0f && objData->unk19 < 8) {
        return 0;
    }
    return 1;
}

/** "Make Tricky get stuck less... HOPEFULLY". Originally by MusicalProgrammer */
RECOMP_PATCH void tricky_func_8974(Object* self, UnkCurvesStruct* arg1, f32 arg2) {
    f32 square;
    f32 var_fs0;
    f32 distanceSquared;
    s32 i;
    
    distanceSquared = (arg2 * gUpdateRateF) * 1.5f;
    square = distanceSquared * distanceSquared;
    distanceSquared = vec3DistanceXZSquared((Vec3f *) (&arg1->unk0.unk68), &self->srt.transl);
    
    if (arg1->unk0.unk80 != 0){
        var_fs0 = -2.0f;
    } else {
        var_fs0 = 2.0f;
    }
    
    for (i = 0; i < 5; i++){
        if (square < distanceSquared){
            break;
        }
        curves_func_800053B0(&arg1->unk0, var_fs0);
        // @recomp: Do... whatever this does
        // TODO: no seriously what does this do
        if (arg1->unk0.unk0 == 1.0f) {
            recomp_printf("[tricky_func_8974] unk0 1.0f -> 0.99609375f\n");
            arg1->unk0.unk0 = 0.99609375f;
        }
        distanceSquared = vec3DistanceXZSquared((Vec3f *) (&arg1->unk0.unk68), &self->srt.transl);
    }
}

#include "modding.h"
#include "recomputils.h"
#include "dll_util.h"
#include "sidekick_util.h"

#include "dlls/engine/26_curves.h"
#include "dlls/engine/27.h"
#include "dlls/engine/29_gplay.h"
#include "sys/curves.h"
#include "sys/dll.h"
#include "sys/main.h"
#include "sys/objects.h"
#include "sys/objexpr.h"
#include "sys/route.h"

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

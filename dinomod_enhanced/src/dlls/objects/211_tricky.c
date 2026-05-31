#include "modding.h"
#include "recomputils.h"
#include "dll_util.h"
#include "sidekick_util.h"

#include "sys/curves.h"
#include "sys/dll.h"
#include "sys/main.h"
#include "sys/objects.h"

#include "recomp/dlls/objects/211_Tricky_recomp.h"

typedef void (*ObjControlFunc)(Object *obj);
static ObjControlFunc tricky_control_func; 
static void tricky_control_hijack(Object *self);

RECOMP_HOOK_DLL(dll_211_ctor) void tricky_ctor_hook(DLLFile *dll) {
    tricky_control_func = dinomod_hijack_dll_export(dll, OBJEXPORT_CONTROL, tricky_control_hijack);
}

RECOMP_HOOK_RETURN_DLL(dll_211_dtor) void tricky_dtor_hook() {
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

// TODO: replace with a real decomp of this function
RECOMP_PATCH void dll_211_func_940C(Object *self, void *state) {
    u32 *unk4c = (u32*)((u32)state + 0x4c);
    // @recomp: Don't run this function if unk4c & 0x800 is not set. The pointers loaded
    //          below will not be valid in that case. It seems that area of state is a union
    //          that holds unrelated memory when Tricky is in other states.
    if (!(*unk4c & 0x800)) {
        return;
    }
    *unk4c &= ~0x800;
    *unk4c |= 0x1000;

    // @recomp: Do null checks before unloading stuff, also reset pointers to null after.
    //          obj_destroy_object will crash if given a null/invalid pointer.
    void **unk0 = (void**)((u32)state + 0x0);
    if (*unk0 != NULL) {
        dll_unload(*unk0);
        *unk0 = NULL;
    }

    Object **unk5f0 = (Object**)((u32)state + 0x5f0);
    if (*unk5f0 != NULL) {
        obj_destroy_object(*unk5f0);
        *unk5f0 = NULL;
    }

    for (s32 i = 0; i < 3; i++) {
        Object **ptr = (Object**)((u32)state + (0x5e4 + (i * 4)));
        if (*ptr != NULL) {
            obj_destroy_object(*ptr);
            *ptr = NULL;
        }
    }
}

/** "Make Tricky get stuck less... HOPEFULLY". Originally by MusicalProgrammer */
RECOMP_PATCH void dll_211_func_8974(Object* arg0, UnkCurvesStruct* arg1, f32 arg2) {
    f32 square;
    f32 var_fs0;
    f32 distanceSquared;
    s32 i;
    
    distanceSquared = (arg2 * gUpdateRateF) * 1.5f;
    square = distanceSquared * distanceSquared;
    distanceSquared = vec3_distance_xz_squared((Vec3f *) (&arg1->unk68), &arg0->srt.transl);
    
    if (arg1->unk80 != 0){
        var_fs0 = -2.0f;
    } else {
        var_fs0 = 2.0f;
    }
    
    for (i = 0; i < 5; i++){
        if (square < distanceSquared){
            break;
        }
        func_800053B0(arg1, var_fs0);
        // @recomp: Do... whatever this does
        // TODO: no seriously what does this do
        if (arg1->unk0 == 1.0f) {
            recomp_printf("[dll_211_func_8974] unk0 1.0f -> 0.99609375f\n");
            arg1->unk0 = 0.99609375f;
        }
        distanceSquared = vec3_distance_xz_squared((Vec3f *) (&arg1->unk68), &arg0->srt.transl);
    }
}

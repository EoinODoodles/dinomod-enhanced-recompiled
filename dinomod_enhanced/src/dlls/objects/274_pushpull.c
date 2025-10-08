#include "modding.h"

#include "PR/ultratypes.h"
#include "PR/gbi.h"
#include "dlls/engine/6_amsfx.h"
#include "game/objects/object.h"
#include "sys/main.h"
#include "sys/objects.h"
#include "types.h"
#include "dll.h"

#include "recomp/dlls/_asm/274_recomp.h"

typedef struct {
    u8 _unk0[0xC8 - 0x0];
    f32 unkC8;
    f32 unkCC;
    u8 _unkD0[0xDC - 0xD0];
} DLL274_State;

RECOMP_PATCH s32 dll_274_func_2A74(Object* arg0, DLL274_State* arg1) {
    if ((arg1->unkCC == 0.0f) && (arg1->unkC8 > 0.0f)) {
        gDLL_6_AMSFX->vtbl->play_sound(arg0, 0x3D8, MAX_VOLUME, NULL, NULL, 0, NULL);
        // @recomp: Destroy Ice Block when it hits the water in DIM, stops a crash if it clips through the floor 
        //          (originally by MusicalProgrammer)
        obj_destroy_object(arg0);
        set_gplay_bitstring(0x272, 1);
    }
    return 0;
}

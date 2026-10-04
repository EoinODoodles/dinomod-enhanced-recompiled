#include "common_objsetups.h"
#include "modding.h"

#include "common.h"
#include "sys/gfx/animseq.h"
#include "sys/objanim.h"

#include "recomp/dlls/_asm/432_recomp.h"

//TEMPORARY DEFINES
#define DF_Lantern_obj_Free dll_432_obj_Free
//END OF TEMPORARY DEFINES

typedef struct {
    u16 lfxActionIdxOn;    //LightAction to use when the player is near the lantern
    u16 lfxActionIdxOff;   //LightAction to use when the player isn't near the lantern
    u16 camDistance;       //Camera's current distance from the lantern
    u16 playerRange;       //Range (2D) for emitting a lightAction point light, and playing sound loop
    u16 camRange;          //Range (3D) for drawing the partFX glow (with expensive occlusion checks), and playing sound loop
    u8 useOtherLFXConfig;  //Boolean, affects which lightAction indices are used (TODO: what's the difference between them? Maybe one pair are night-only?)
    u8 flags;              //See `DF_Lantern_Flags`
    u8 prevFlags;          //Used to handle when flags change
    u32 soundHandle;       //For the crackling sound loop
} DF_Lantern_Data;

RECOMP_PATCH void DF_Lantern_obj_Free(Object* self, s32 onlySelf) {
    DF_Lantern_Data* objData = self->data;
    
    if (objData->flags & DF_Lantern_FLAG_2_Emit_Light) {
        lfxAction(self, self, objData->lfxActionIdxOff, 0, 0, 0);
    }

    //@recomp: free soundHandle too
    if (objData->soundHandle) {
        dll_amSfx->Stop(objData->soundHandle);
        objData->soundHandle = 0;
    }
    
    gDLL_13_Expgfx->vtbl->func5(self);
}

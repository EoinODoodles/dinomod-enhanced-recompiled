#include "modding.h"
#include "recomputils.h"

#include "dll.h"
#include "game/objects/object.h"
#include "game/objects/object_id.h"
#include "sys/gfx/modgfx.h"
#include "sys/main.h"
#include "sys/objects.h"
#include "sys/objtype.h"
#include "sys/dll.h"

#include "dlls/engine/6_amsfx.h"
#include "dlls/objects/210_player.h"
#include "recomp/dlls/objects/295_smallbasket_recomp.h"

extern DLL_IModgfx* dModGfxDLL[];

typedef struct{
/*00*/ s32 unk0;
/*04*/ s32 unk4;
/*08*/ u32 soundHandle;
/*0C*/ s32 unkC;
/*10*/ s32 unk10;
/*14*/ s16 shakeSoundTimer;
/*16*/ s8 unk16[0x28 - 0x16];
} SmallBasket_Data;

// Frees the Small Basket objects' memory properly (originally by MusicalProgrammer)
// Prevent crash if the player's holding the basket when it unloads
RECOMP_PATCH void smallbasket_free(Object* self, s32 arg1) {
    SmallBasket_Data* objdata = self->data;
    Object* player; //@recomp
    Player_Data* playerObjdata; //@recomp
    
    //@recomp: Remove basket if the player's holding it (Banjeoin)
    player = get_player();
    if (player){
        playerObjdata = player->data;
        if (playerObjdata->unk868 && 
            (playerObjdata->unk868->id == OBJ_SmallBasket || playerObjdata->unk868->id == OBJ_SmallCrate)){
            playerObjdata->unk868 = NULL;
        }
    }

    gDLL_14_Modgfx->vtbl->func5(self);
    obj_free_object_type(self, 0x12); //@recomp: remove self from type category 0x12
    dll_unload((void*)dModGfxDLL[0]);
    if (objdata->soundHandle) {
        gDLL_6_AMSFX->vtbl->stop(objdata->soundHandle);
        objdata->soundHandle = 0;
    }
}

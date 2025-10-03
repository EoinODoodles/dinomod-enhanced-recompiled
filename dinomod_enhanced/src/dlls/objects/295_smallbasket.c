#include "modding.h"
#include "recomputils.h"

#include "dll.h"
#include "sys/dll.h"
#include "game/objects/object.h"
#include "game/objects/object_id.h"
#include "sys/main.h"
#include "sys/objects.h"
#include "sys/objtype.h"

#include "dlls/engine/6_amsfx.h"
#include "dlls/objects/210_player.h"
#include "recomp/dlls/_asm/295_recomp.h"

extern s32 _data_0[];

typedef struct{
s32 unk0;
s32 unk4;
s32 sound;
s32 unkC;
s32 unk10;
s16 shakeSoundTimer;
} SmallBasketState;

// Frees the Small Basket objects' memory properly (originally by MusicalProgrammer)
// Prevent crash if the player's holding the basket when it unloads
RECOMP_PATCH void dll_295_func_D04(Object* self, s32 arg1) {
    SmallBasketState* state = self->state;
    Object* player; //@recomp
    PlayerState* playerState; //@recomp
    
    //@recomp: Remove basket if the player's holding it (Banjeoin)
    player = get_player();
    if (player){
        playerState = player->state;
        if (playerState->unk868 && 
            (playerState->unk868->id == OBJ_SmallBasket || playerState->unk868->id == OBJ_SmallCrate)){
            playerState->unk868 = NULL;
        }
    }

    gDLL_14_Modgfx->vtbl->func5(self);
    obj_free_object_type(self, 0x12); //@recomp: remove SmallBasket type objects from category 0x12
    dll_unload((void*)_data_0[0]);
    if (state->sound) {
        gDLL_6_AMSFX->vtbl->func_A1C(state->sound);
        state->sound = 0;
    }
}

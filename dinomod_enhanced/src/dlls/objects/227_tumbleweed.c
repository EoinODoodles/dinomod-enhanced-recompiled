#include "player_util.h"
#include "recomputils.h"
#include "modding.h"

#include "objects/227_tumbleweed.h"

#include "PR/ultratypes.h"
#include "common.h"
#include "game/objects/interaction_arrow.h"
#include "sys/objanim.h"
#include "sys/objects.h"
#include "sys/objmsg.h"
#include "sys/objtype.h"
#include "dlls/objects/210_player.h"
#include "dlls/objects/226_tumbleweedbush.h"
#include "dlls/objects/227_tumbleweed.h"

#include "recomp/dlls/objects/227_tumbleweed_recomp.h"

extern int Tumbleweed_did_player_lift_twig(Object* self);

/** Toggles creating particles/sounds when the Tumbleweed is deleted from out of the player's hands */
void tumbleweed_set_silent_delete(Object* self, int enable){
    Tumbleweed_Data_Extended* objData;

    if (!self){
        return;
    }

    objData = self->data;
    if (!objData){
        return;
    }
    objData->destroySilently = (enable != 0);
}

void tumbleweed_stop_being_carried(Object* self){
    Tumbleweed_Data_Extended* objData;
    if (!self){
        return;
    }

    objData = self->data;
    if (!objData){
        return;
    }

    objData->beingCarried = FALSE;
}

/** 
  * Creates leaves/dust and sounds that persist after the Tumbleweed is destroyed 
  * (useful when the Tumbleweed is suddenly unloaded while still being carried by the player) 
  */
void tumbleweed_create_disintegrate_effects(Object* self, int createLeaves, int createDust, int playSound) {
    Object* player = get_player();
    SRT transform;
    u32 ID;
    u32 i;

    if (!player ){
        return;
    }

    ID = self->id;
    transform.transl.x = self->srt.transl.x;
    transform.transl.y = self->srt.transl.y;
    transform.transl.z = self->srt.transl.z;

    if (createLeaves) {
        switch (ID) {
        case OBJ_Tumbleweed1:
        case OBJ_Tumbleweed1twig:
        case OBJ_Tumbleweed3:
        case OBJ_Tumbleweed3twig:
            //Create leaf particles
            for (i = 20; i > 0; i--) {
                gDLL_17_partfx->vtbl->spawn(player, PARTICLE_34D, &transform, PARTFXFLAG_2, -1, NULL);
            }
            break;
        default:
            //Create frosty leaf particles
            for (i = 20; i > 0; i--) {
                gDLL_17_partfx->vtbl->spawn(player, PARTICLE_32E, &transform, PARTFXFLAG_2, -1, NULL);
            }
            break;
        }
    }

    if (createDust) {
        switch (ID) {
        case OBJ_Tumbleweed1:
        case OBJ_Tumbleweed1twig:
        case OBJ_Tumbleweed3:
        case OBJ_Tumbleweed3twig:
            //Create cloud of dust
            gDLL_17_partfx->vtbl->spawn(player, PARTICLE_34C, &transform, PARTFXFLAG_2, -1, NULL);
            break;
        default:
            //Create cloud of frost
            gDLL_17_partfx->vtbl->spawn(player, PARTICLE_32D, &transform, PARTFXFLAG_2, -1, NULL);
        }
    }

    if (playSound){
        gDLL_6_AMSFX->vtbl->play(player, SOUND_5F7_Tumbleweed_Disintegrate, MAX_VOLUME, 0, 0, 0, 0);
    }
}

/** 
  * Fix potential crashes when the Tumbleweed is unloaded. 
  *
  * - Avoid crash when being carried by player.
  * - Avoid crash when parent TumbleweedBush is already deleted.
  */
RECOMP_PATCH void Tumbleweed_free(Object* self, s32 arg1) {
    Tumbleweed_Data_Extended* objData;
    Object* object;
    s32 i;
    s32 count;
    s32 id;
    Object** objects;

    objData = self->data;
      
    switch (self->id) {
        case OBJ_Tumbleweed1:
        case OBJ_Tumbleweed1twig:
            id = OBJ_TumbleWeedBush1;
            break;
        case OBJ_Tumbleweed2:
        case OBJ_Tumbleweed2twig:
            id = OBJ_TumbleWeedBush2;
            break;
        case OBJ_Tumbleweed3:
        case OBJ_Tumbleweed3twig:
            id = OBJ_TumbleWeedBush3;
            break;
    }

    {
        //@recomp: force player to let go of carried Tumbleweed (avoids a crash)

        Object* player = get_player();
        Player_Data* playerData;

        if (player && player->data){
            playerData = player->data;
            if (playerData->unk868 && playerData->unk868 == self){
                //Create particles so it looks like the Tumbleweed fell apart (instead of randomly vanishing)
                if (objData->destroySilently == FALSE){
                    tumbleweed_create_disintegrate_effects(self, TRUE, TRUE, TRUE);
                }
                playerData->unk868 = NULL;
            }
        }
    }
    
    //Find parent tree object
    for (objects = get_world_objects(&i, &count); i < count; i++) {
        object = objects[i];
        if (id == object->id && !(object->stateFlags & OBJSTATE_DESTROYED)) { //@recomp: check Object isn't deleted
            ((DLL_226_TumbleweedBush*)object->dll)->vtbl->remove_tumbleweed(object, self);
        }
    }
    
    if (objData->goldenNugget) {
        main_set_bits(objData->goldDroppedGamebit, 1);
        objData->goldenNugget = NULL;
    }

    obj_free_object_type(self, OBJTYPE_Baddie);
    obj_free_object_type(self, OBJTYPE_TrickyTarget);
}

/**
  * Fix issue where player could immediately drop a Tumbleweed after picking it up.
  */
RECOMP_PATCH int Tumbleweed_handle_carry_behaviour(Object* self) {
    u32 pad;
    Object* player;
    u32 messageArg;
    u32 soundVol;
    u32 soundID;
    Tumbleweed_Data_Extended* objData;

    objData = self->data;
    player = get_player();

    //Not being carried
    if (objData->carryFlags == Twig_FLAG_None) {
        //Check for player to lift twig via interaction arrow
        if ((objData->carryFlags = Tumbleweed_did_player_lift_twig(self))) {
            objData->beingCarried = TRUE;
        }
        self->unkAF &= ~ARROW_FLAG_8_No_Targetting;
        return TRUE;
    }
    
    //Being carried
    else {
        //Handle squeaking and growing in size temporarily
        objData->twigSqueakTimer -= gUpdateRateF;
        if (objData->twigSqueakTimer < 0.0f) {
            objData->twigSqueakTimer = rand_next(120, 240);
            soundID = rand_next(SOUND_614_Tumbleweed_Squeak_1, SOUND_615_Tumbleweed_Squeak_2);
            soundVol = rand_next(90, 100);
            gDLL_6_AMSFX->vtbl->play(self, soundID, soundVol, 0, 0, 0, 0);
            self->srt.scale = 0.2f;
        } else {
            self->srt.scale = 0.15f;
        }
        
        self->unkAF |= ARROW_FLAG_8_No_Targetting;
        objData->timer = 0.0f;

        //Stop being carried when A button pressed
        {
            //@recomp: only allow letting go of Tumbleweed when player standing/walking
            //Fixes issue where player can immediately drop Tumbleweed after lifting it (from tapping A)
            if (playerUtil_is_player_standing_or_walking(player)){
                if (joy_get_pressed(0) & A_BUTTON) {
                    joy_disable_buttons(0, A_BUTTON);
                    objData->beingCarried = FALSE;
                }
            }
        }

        //Check if Tumbleweed dropped by player
        if (self->unkE0 == 1) {
            objData->carryFlags = Twig_FLAG_Dropped;
        }
        if ((objData->carryFlags == Twig_FLAG_Dropped) && (self->unkE0 == 0)) {
            objData->carryFlags = Twig_FLAG_None;
            objData->beingCarried = FALSE;
        }

        //Send message to player object while being carried
        if (objData->beingCarried) {
            messageArg = (objData->carryMessageArgHi << 0x10) | (objData->carryMessageArgLo & 0xFFFF);
            obj_send_mesg(player, 0x100008, self, (void*)messageArg);
        }

        return FALSE;
    }
}

/** Extend objData */
RECOMP_PATCH u32 Tumbleweed_get_data_size(Object* self, s32 arg1) {
    return sizeof(Tumbleweed_Data_Extended);
}

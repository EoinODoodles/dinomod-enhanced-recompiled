#include "PR/ultratypes.h"
#include "modding.h"

#include "game/objects/object.h"
#include "game/objects/object_id.h"
#include "game/gamebits.h"
#include "recomputils.h"
#include "sys/dll.h"
#include "sys/main.h"
#include "sys/menu.h"
#include "sys/print.h"
#include "sys/objects.h"
#include "sys/objtype.h"
#include "game/gamebits.h"
#include "dll.h"

#include "engine/78_credits.h"

#include "recomp/dlls/objects/590_WL_Galleon_recomp.h"

void func_8005B5B8(Object*, Object*, s32);

#define CREDITS_SKIP_TO_FRAME 2982

typedef struct {
    u8 unk0;
    Vec3f translate;
    u8 unk10;
    s16 yaw;
} WLGalleonObjdata;

/** 
  * - Removes a gamebit check which could prevent the "Scales Escapes with Kyte" sequence from playing 
  *   just before you teleport away to SwapStone Circle (originally by MusicalProgrammer)
  *
  * - Fixes character landing sound during Galleon arrival sequence.
  *
  * - Synchronises title credits when skipping sequences.
  */
RECOMP_PATCH void WLgalleon_control(Object* self) {
    u32 arrivedAtWM;
    Object* player;
    WLGalleonObjdata* objData;
    u8 colourRGBA[4] = {0xe4, 0x9c, 0x44, 0xff}; //unused orange colour?
    
    if (main_get_bits(BIT_Play_Seq_00EF_Scales_Escapes_With_Kyte)) {
        return;
    }

    if (self->id == OBJ_SB_ShipShadow) {
        self->opacity = 0x80;
        return;
    }
    
    player = get_player();
    objData = self->data;
    
    if (main_get_bits(BIT_429)) {
        if (gDLL_29_Gplay->vtbl->get_obj_group_status(self->mobileMapID, 2)) {
            gDLL_29_Gplay->vtbl->set_obj_group_status(self->mobileMapID, 1, 0);
            gDLL_29_Gplay->vtbl->set_obj_group_status(self->mobileMapID, 2, 0);
        }
    } else if (
            // !main_get_bits(BIT_WM_Played_Randorn_First_Meeting) && //@recomp: remove check
            !gDLL_29_Gplay->vtbl->get_obj_group_status(self->mobileMapID, 2)) {
        gDLL_29_Gplay->vtbl->set_obj_group_status(self->mobileMapID, 1, 1);
        gDLL_29_Gplay->vtbl->set_obj_group_status(self->mobileMapID, 2, 1);
    }
    
    if (1 
        // && !main_get_bits(BIT_WM_Played_Randorn_First_Meeting) //@recomp: remove check 
        ) {
        if (!objData->unk10 && !main_get_bits(BIT_429)) {
            gDLL_29_Gplay->vtbl->set_obj_group_status(self->mobileMapID, 1, 1);
            gDLL_29_Gplay->vtbl->set_obj_group_status(self->mobileMapID, 2, 1);
            objData->unk10 = TRUE;
        }
    } else {
        if (!gDLL_29_Gplay->vtbl->get_obj_group_status(self->mobileMapID, 4)) {
            gDLL_29_Gplay->vtbl->set_obj_group_status(self->mobileMapID, 4, 1);
        }
        if (objData->unk10) {
            objData->unk10 = FALSE;
        }
    }
    
    arrivedAtWM = main_get_bits(BIT_Galleon_Arrived_at_Warlock_Mountain);

    if (arrivedAtWM) {
        self->unkDC = 0xA;
    }
    
    if (!arrivedAtWM) {
        player->srt.transl.x = -121.0f;
        // player->srt.transl.y = 116.0f;
        player->srt.transl.z = 5.0f;

        //@recomp: fix issue where Krystal starts slightly too high above Galleon, causing fall sound (Banjeoin)
        player->srt.transl.y = 100.0f; 
        *((u16*)((u32)player + 0x0360)) = 0; //modanim (TODO: clean up)
        player->animProgress = 0.0f;

        //@recomp: synchronise the credits when skipping sequences to get here
        if (credits_get_frame() < 100) {
            //Play Galleon arrival music (TODO: skip to the playback time it should be at, if possible?)
            gDLL_5_AMSEQ2->vtbl->set(self, 0x47, 0, 0, 0);
            credits_sync_frame(CREDITS_SKIP_TO_FRAME);
        }

        func_8005B5B8(player, self, 0);
        ((DLL_Unknown*)player->dll)->vtbl->func[68].withOneArg((s32)player);
        self->unkE0 = 1;
        return;
    }
    
    if (self->unkE0 == 1) {
        self->srt.transl.x = objData->translate.x;
        self->srt.transl.y = objData->translate.y;
        self->srt.transl.z = objData->translate.z;
        self->srt.yaw = objData->yaw;
        gDLL_3_Animation->vtbl->func17(0, self, -1);
        self->unkE0 = 2;
    }
}

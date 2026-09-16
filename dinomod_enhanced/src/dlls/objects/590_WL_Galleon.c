#include "modding.h"
#include "recomputils.h"

#include "PR/ultratypes.h"
#include "dll.h"
#include "dlls/objects/210_player.h"
#include "game/objects/object.h"
#include "game/objects/object_id.h"
#include "game/gamebits.h"
#include "sys/dll.h"
#include "sys/gfx/animseq.h"
#include "sys/main.h"
#include "sys/menu.h"
#include "sys/print.h"
#include "sys/objects.h"
#include "sys/objtype.h"

#include "anim_util.h"
#include "engine/78_credits.h"

#include "recomp/dlls/objects/590_WL_Galleon_recomp.h"

#define CREDITS_SKIP_TO_FRAME 2982

typedef struct {
    u8 unk0;
    Vec3f translate;
    u8 unk10;
    s16 yaw;
} WLGalleonObjdata;

/*0x0*/ extern s8 dataShowKrystalsAdventureScreen[];
/*0x8*/ extern void* dataDLLUnused;
/*0xC*/ extern u32 sUpdateRateCopy;

/** 
  * - Removes a gamebit check which could prevent the "Scales Escapes with Kyte" sequence from playing 
  *   just before you teleport away to SwapStone Circle (originally by MusicalProgrammer)
  *
  * - Fixes character landing sound during Galleon arrival sequence.
  */
RECOMP_PATCH void WLgalleon_control(Object* self) {
    u32 arrivedAtWM;
    Object* player;
    WLGalleonObjdata* objData;
    u8 colourRGBA[4] = {0xe4, 0x9c, 0x44, 0xff}; //unused orange colour?
    
    if (mainGetBits(BIT_Play_Seq_00EF_Scales_Escapes_With_Kyte)) {
        return;
    }

    if (self->id == OBJ_SB_ShipShadow) {
        self->opacity = 0x80;
        return;
    }
    
    player = objGetPlayer();
    objData = self->data;
    
    if (mainGetBits(BIT_429)) {
        if (gDLL_29_Gplay->vtbl->get_obj_group_status(self->mobileMapID, 2)) {
            gDLL_29_Gplay->vtbl->set_obj_group_status(self->mobileMapID, 1, 0);
            gDLL_29_Gplay->vtbl->set_obj_group_status(self->mobileMapID, 2, 0);
        }
    } else if (
            // !mainGetBits(BIT_WM_Played_Randorn_First_Meeting) && //@recomp: remove check
            !gDLL_29_Gplay->vtbl->get_obj_group_status(self->mobileMapID, 2)) {
        gDLL_29_Gplay->vtbl->set_obj_group_status(self->mobileMapID, 1, 1);
        gDLL_29_Gplay->vtbl->set_obj_group_status(self->mobileMapID, 2, 1);
    }
    
    if (1 
        // && !mainGetBits(BIT_WM_Played_Randorn_First_Meeting) //@recomp: remove check 
        ) {
        if (!objData->unk10 && !mainGetBits(BIT_429)) {
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
    
    arrivedAtWM = mainGetBits(BIT_Galleon_Arrived_at_Warlock_Mountain);

    if (arrivedAtWM) {
        self->unkDC = 0xA;
    }
    
    if (!arrivedAtWM) {
        //@recomp: add null check just in case
        if (player == NULL) {
            return;
        }

        player->srt.transl.x = -121.0f;
        // player->srt.transl.y = 116.0f;
        player->srt.transl.z = 5.0f;

        //@recomp: fix issue where Krystal starts slightly too high above Galleon, causing fall sound (Banjeoin)
        player->srt.transl.y = 100.0f; 
        player->animProgress = 0.0f;
        gDLL_18_objfsa->vtbl->set_anim_state(player, player->data, PLAYER_ASTATE_Standing);

        trackIntersect_func_8005B5B8(player, self, 0);
        ((DLL_210_Player*)player->dll)->vtbl->func68(player);
        self->unkE0 = 1;
        return;
    }
    
    if (self->unkE0 == 1) {
        self->srt.transl.x = objData->translate.x;
        self->srt.transl.y = objData->translate.y;
        self->srt.transl.z = objData->translate.z;
        self->srt.yaw = objData->yaw;
        gDLL_3_Animation->vtbl->start_obj_sequence(0, self, -1);
        self->unkE0 = 2;
    }
}

/* Play Warlock Mountain Act 1's music when the Galleon Arrival sequences are skipped. */
static void WLgalleon_animEndCallback(Object* self, Object* animObj, AnimObj_Data* animData) {
    AnimObj_Setup* animSetup = (AnimObj_Setup*)animObj->setup;
    u16 sequenceID = 0;
    if (animSetup) {
        sequenceID = GET_SEQID(animSetup->sequenceIdBitfield);
    }

    //@recomp: play regular Warlock Mountain Act 1 music if either of the arrival sequences are skipped
    if (animData->unk9D & 0x80) {
        switch (sequenceID) {
        case 0xB9:
        case 0xBA:
            mainSetBits(BIT_Galleon_Arrived_at_Warlock_Mountain, TRUE);
            gDLL_5_AMSEQ2->vtbl->set(self, 0x11D, 0, 0, 0);
            break;
        }
    }
}

/**
  * - Synchronises title credits when skipping past prior sequences to get to Warlock Mountain.
  * - Adds a custom end-of-sequence callback function.
  */
RECOMP_PATCH int WLgalleon_anim_callback(Object* self, Object* animObj, AnimObj_Data* animData, s8 prevCallbackValue) {
    s32 index;
    /* RECOMP */
    AnimObj_Setup* animSetup = (AnimObj_Setup*)animObj->setup;
    u16 sequenceID = 0;

    //@recomp: get current sequenceID
    if (animSetup) {
        sequenceID = GET_SEQID(animSetup->sequenceIdBitfield);
    }

    sUpdateRateCopy = gUpdateRate; //unused?
    animData->unk7A = -1;
    animData->unk62 = 0;

    //@recomp: use custom end-of-sequence callback (for handling when the sequence was skipped)
    animData->unkF4 = WLgalleon_animEndCallback;

    //@recomp: synchronise the credits when skipping previous sequences to get here
    if (sequenceID == 0xB9 && animData->time < 30 && credits_get_frame() < 100) {
        //Play Galleon arrival music (TODO: skip music to the playback time it should be at, maybe?)
        gDLL_5_AMSEQ2->vtbl->set(self, 0x47, 0, 0, 0);
        credits_sync_frame(CREDITS_SKIP_TO_FRAME);
    }

    for (index = 0; index < animData->messageCount; index++){
        switch (animData->messages[index]) {
            case 1:
                self->unkDC = 0xA;
                break;
            case 9:
                self->unkDC = 0xB;
                break;
            case 4:
                self->unkDC = 0xC;
                break;
            case 5:
                self->unkDC = 0xD;
                break;
            case 6:
                gDLL_29_Gplay->vtbl->set_obj_group_status(self->mobileMapID, 1, 0);
                gDLL_29_Gplay->vtbl->set_obj_group_status(self->mobileMapID, 2, 0);
                gDLL_29_Gplay->vtbl->set_obj_group_status(self->mobileMapID, 4, 0);
                mainSetBits(BIT_WL_Load_Unload_Galleon, 0);
                break;
            case 2:
                //Setting envFxActions
                lfxAction(self, self, 0x77, 0, 0, 0);
                lfxAction(self, self, 0x78, 0, 0, 0);
                lfxAction(self, self, 0x80, 0, 0, 0);
                break;
            case 3:
                gDLL_23->vtbl->func_4C(0, 0x1e, 0x50);
                break;
            case 7:
                dataShowKrystalsAdventureScreen[0] = TRUE;
                break;
            case 8:
                dataShowKrystalsAdventureScreen[0] = FALSE;
                break;
        }
    }

    if (mainGetBits(BIT_429) && gDLL_29_Gplay->vtbl->get_obj_group_status(self->mobileMapID, 2)) {
        gDLL_29_Gplay->vtbl->set_obj_group_status(self->mobileMapID, 1, 0);
        gDLL_29_Gplay->vtbl->set_obj_group_status(self->mobileMapID, 2, 0);
    }

    return 0;
}

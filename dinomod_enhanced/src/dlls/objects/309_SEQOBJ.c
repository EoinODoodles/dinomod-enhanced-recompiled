#include "modding.h"
#include "recomputils.h"

#include "PR/ultratypes.h"
#include "dll.h"
#include "dlls/engine/29_gplay.h"
#include "dlls/engine/6_amsfx.h"
#include "game/gamebits.h"
#include "dll_util.h"

#include "game/objects/object_id.h"
#include "sys/gfx/animseq.h"
#include "sys/dll.h"
#include "sys/main.h"
#include "sys/map.h"
#include "sys/map_enums.h"
#include "sys/objects.h"
#include "sys/objtype.h"
#include "sys/print.h"

#include "recomp/dlls/objects/309_SEQOBJ_recomp.h"

typedef struct {
    ObjSetup base;
    s16 gamebitPlay;        //The sequence will play when this gamebit is set
    s16 gamebitFinished;    //This gamebit will be set when the sequence has played
    u8 rotate;
    u8 playbackOptions;
    s8 seqIndex;            //The index of the sequence in the Object.bin entry's sequence list
    s8 modelInstIdx;        //Choose between 3D models, visible when debugging (usually a clapperboard)
    s16 unk20;
    u16 unk22;
    u8 warpID;              //Optionally warp the player
} SeqObj_Setup;

typedef struct {
    u8 flags;
    s8 finished;
} SeqObj_Data;

typedef enum {
    SEQOBJ_FLAG_None = 0,
    SEQOBJ_FLAG_Playing = 1,
    SEQOBJ_FLAG_Unk_2 = 2, //finished?
    SEQOBJ_FLAG_Anim_Callback_Ran = 4
} SeqObj_Flags;

typedef enum {
    SEQOBJ_OPTIONS_None = 0,
    SEQOBJ_OPTIONS_Stoppable = 1,
    SEQOBJ_OPTIONS_2 = 2,
    SEQOBJ_OPTIONS_4 = 4,
    SEQOBJ_OPTIONS_8 = 8,
    SEQOBJ_OPTIONS_A = 16
} SeqObj_PlaybackOptions; //TO-DO: figure out what these do! Looping and auto-play may be options?

extern int SeqObj_anim_callback(Object* self, Object* animObj, AnimObj_Data* animObjData, s8 arg3);

//TEMPORARY: plays unused cutscene when Sabre leaves the shop! (found by jeebs2kx)
//TO-DO: add this via MAPS asset edits instead
RECOMP_PATCH void SeqObj_setup(Object* self, SeqObj_Setup* objSetup, s32 arg2) {
    SeqObj_Data* objData;
    s16 originalSeqID;

    //Force cutscene when Sabre leaving shop
    if (objSetup->base.uID == 0x00042272 && main_get_bits(BIT_SP_Exiting_Shop)){
        main_set_bits(BIT_SP_Exiting_Shop, 0);

        originalSeqID = self->def->pSeq[0];
        self->def->pSeq[0] = 0x44A;
        gDLL_3_Animation->vtbl->start_obj_sequence(0, self, -1);

        self->def->pSeq[0] = originalSeqID;
        return;
    }

    self->srt.yaw = objSetup->rotate << 8;
    self->animCallback = SeqObj_anim_callback;
    
    objData = self->data;
    self->modelInstIdx = objSetup->modelInstIdx;
    if (self->modelInstIdx >= self->def->numModels) {
        self->modelInstIdx = 0;
    }
    
    obj_add_object_type(self, OBJTYPE_17);
    
    objData->flags = SEQOBJ_FLAG_None;
    if (objSetup->gamebitPlay != -1) {
        if (main_get_bits(objSetup->gamebitPlay)) {
            objData->flags |= SEQOBJ_FLAG_Playing;
            if (objSetup->unk20 != 0) {
                objData->flags |= SEQOBJ_FLAG_Unk_2;
            }
        }
    }

    objData->finished = FALSE;
    self->stateFlags |= OBJSTATE_UPDATE_DISABLED;
}

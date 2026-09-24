#include "modding.h"
#include "object_util.h"
#include "recomputils.h"

#include "common.h"
#include "dll.h"
#include "game/objects/object.h"
#include "game/objects/object_id.h"
#include "sys/main.h"
#include "sys/objects.h"

#include "objects/429_DFSH_Door1Special.h"

#include "recomp/dlls/objects/430_DFSH_door2Special_recomp.h"

//TEMPORARY DEFINES
#define DFSH_Door2Special_obj_Setup DFSH_Door2Special_setup
#define DFSH_Door2Special_obj_Control DFSH_Door2Special_control
#define DFSH_Door2Special_animCallback DFSH_Door2Special_anim_callback

typedef enum {
    SeqDoor_SEQCMD_1_Finished_Closing = 1,
    SeqDoor_SEQCMD_2_Finished_Opening = 2
} SeqDoor_ObjSeqMessages;
//END OF TEMPORARY DEFINES

extern int DFSH_Door2Special_animCallback(Object* self, Object* overrideObj, AnimObj_Data* animData, s8 prevCallbackResult);

RECOMP_PATCH void DFSH_Door2Special_obj_Setup(Object* self, DFSH_DoorSpecial_Setup* objSetup, s32 reset) {
    DFSH_DoorSpecial_Data* objData;
    TextureAnimator* texAnim;

    objData = self->data;
    self->animCallback = DFSH_Door2Special_animCallback;

    //Restore texture glow state
    {
        if (mainGetBits(objSetup->gamebitLit)) {
            objData->glowState = DFSH_DoorSpecial_GLOW_2_Pulse;
        } else {
            objData->glowState = DFSH_DoorSpecial_GLOW_0_Unlit;
        }
        
        texAnim = objExprGetTexAnimator(self, 0, 0);
        if (texAnim != NULL) {
            if (objData->glowState == DFSH_DoorSpecial_GLOW_2_Pulse) {
                texAnim->frame = 1;
            } else {
                texAnim->frame = 0;
            }
        }
    }

    //@recomp: set yaw
    self->srt.yaw = objSetup->yaw << 8;

    //@recomp: unload after setup if the door's already open
    if (GAMEBIT_SPECIFIED_AND_SET(objSetup->gamebitDoorState)) {
        objData->unload = TRUE;
        self->opacity = 0;
    }
    
    objData->phase = 0;
}

/* Free self if the door was already open on setup, or at the end of the door opening sequence */
RECOMP_PATCH void DFSH_Door2Special_obj_Control(Object* self) { 
    DFSH_DoorSpecial_Data* objData = self->data;
    DFSH_DoorSpecial_Setup* objSetup = (DFSH_DoorSpecial_Setup*)self->setup;
    
    if (objData->unload || GAMEBIT_SPECIFIED_AND_SET(objSetup->gamebitDoorState)) {
        objFreeObject(self);
    }
}

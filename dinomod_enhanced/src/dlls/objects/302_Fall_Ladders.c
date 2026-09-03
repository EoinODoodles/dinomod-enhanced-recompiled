#include "modding.h"
#include "recomputils.h"

#include "common.h"
#include "dlls/objects/210_player.h"

#include "recomp/dlls/objects/302_Fall_Ladders_recomp.h"
#include "sys/main.h"

//TEMPORARY DEFINES
#define Fall_Ladders_obj_Setup Fall_Ladders_setup
#define Fall_Ladders_fallOrRiseBySequence Fall_Ladders_fall_or_rise_by_sequence
#define Fall_Ladders_animCallback Fall_Ladders_anim_callback
//END OF TEMPORARY DEFINES

typedef struct {
    ObjSetup base;
    s8 yaw;
    s8 modelIdx;        //Which ladder model to use
    s16 raisedOffsetY;  //When raised, the ladder starts off this far above its base position
    s16 unused1C;
    s16 gamebitRaise;   //Ladder rises up by sequence when this gamebit is set (exclusive to `VFP_Ladders2`)
    s16 gamebitFall;    //Ladder starts falling when this gamebit is set
} Fall_Ladders_Setup;

typedef struct {
    f32 raisedOffsetY;  //When raised, the ladder starts off this far above its base position
    s16 gamebitRaise;   //Ladder rises up by sequence when this gamebit is set (exclusive to `VFP_Ladders2`)
    s16 gamebitFall;    //Ladder starts falling when this gamebit is set
    s16 state;          //Raised/Falling/Lowered
    s16 dropTimer;      //Counts down to ladder falling, after gamebitFall set
} Fall_Ladders_Data;

typedef enum {
    Fall_Ladders_STATE_0_Raised,
    Fall_Ladders_STATE_1_Falling,
    Fall_Ladders_STATE_2_Lowered
} Fall_Ladders_States;

extern void Fall_Ladders_fallOrRiseBySequence(Object* self);
extern int Fall_Ladders_animCallback(Object* actor, Object* animObj, AnimObj_Data* animObjData, s8 prevCallbackValue);

/* Restore ladder's lowered state, to avoid playing a sound every time a lowered ladder loads */
RECOMP_PATCH void Fall_Ladders_obj_Setup(Object* self, Fall_Ladders_Setup* objSetup, s32 reset) {
    Fall_Ladders_Data* objData = self->data;
    
    self->srt.yaw = objSetup->yaw << 8;
    
    objData->gamebitFall = objSetup->gamebitFall;
    objData->gamebitRaise = objSetup->gamebitRaise;
    objData->raisedOffsetY = objSetup->raisedOffsetY;
    
    self->stateFlags |= OBJSTATE_PRINT_DISABLED | OBJSTATE_UPDATE_DISABLED;
    self->animCallback = Fall_Ladders_animCallback;
    
    objSetModel(self, objSetup->modelIdx);
    
    //@recomp: restore state
    if (self->id != OBJ_VFP_Ladders2) { //TODO: revisit and handle restoring VFP_Ladders2's state too (this one's driven by objSeqs)
        if (mainGetBits(objSetup->gamebitFall)) {
            objData->state = Fall_Ladders_STATE_2_Lowered;
            self->srt.transl.y = objSetup->base.y;
            return;
        }
    }

    objData->state = Fall_Ladders_STATE_0_Raised;
    self->srt.transl.y = objSetup->base.y + objData->raisedOffsetY;
}

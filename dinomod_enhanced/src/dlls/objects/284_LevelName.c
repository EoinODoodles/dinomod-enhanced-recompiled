#include "configs.h"
#include "modding.h"
#include "recompconfig.h"
#include "recomputils.h"

#include "common.h"

#include "recomp/dlls/objects/284_levelname_recomp.h"

//TEMPORARY DEFINES
#define LevelName_obj_Control levelname_control
#define LevelName_animCallback levelname_anim_callback
//END OF TEMPORARY DEFINES

typedef struct {
    ObjSetup base;
    s16 gamebitShown;
    s16 unk1A;
    s16 unk1C;
    s16 textID;
    u8 activationRadius;
} LevelName_Setup;

typedef struct {
/*00*/ GameTextChunk* gametext;
/*04*/ char* strings;
/*08*/ u32 displayDuration;
/*0C*/ u8 activationRadius;
/*0D*/ s8 unusedD;
/*0E*/ s16 gamebitShown;
/*10*/ s16 timer;
/*12*/ s16 opacity;
/*14*/ u8 state;
/*15*/ u8 pad[3];
} LevelName_Data;

enum LevelNameStates{
    LevelName_STATE_0_WAITING = 0,
    LevelName_STATE_1_FADING_IN = 1,
    LevelName_STATE_2_HOLDING = 2,
    LevelName_STATE_3_FADING_OUT = 3,
    LevelName_STATE_4_FINISHED = 4
};

extern void LevelName_obj_Control(Object* self);

/* Add the ability to fade out or immediately hide the level name using custom anim messages */
RECOMP_PATCH int LevelName_animCallback(Object* self, Object* overrideObj, AnimObj_Data* animData, s8 prevCallbackValue) {
    LevelName_Data* objdata;
    s32 i;

    objdata = self->data;

    for (i = 0; i < animData->messageCount; i++) {        
        switch (animData->messages[i]) {
        case 1: //Fade in
            if (objdata->gamebitShown != NO_GAMEBIT) {
                mainSetBits(objdata->gamebitShown, 1);
            }
            objdata->state = LevelName_STATE_1_FADING_IN;
            return 4;
        case 2: //Same as message 1, but stays in sequence instead of breaking out to control
            if (objdata->gamebitShown != NO_GAMEBIT) {
                mainSetBits(objdata->gamebitShown, 1);
            }
            objdata->state = LevelName_STATE_1_FADING_IN;
            break;
        case 3: //Fade out
            objdata->state = LevelName_STATE_3_FADING_OUT;
            break;
        case 4: //Hide
            objdata->opacity = 0;
            objdata->state = LevelName_STATE_4_FINISHED;
            break;
        }
    }

    //@recomp: run control too, to handle fading while still in a sequence
    if (objdata->state >= LevelName_STATE_1_FADING_IN) {
        LevelName_obj_Control(self);
    }

    return 0;
}

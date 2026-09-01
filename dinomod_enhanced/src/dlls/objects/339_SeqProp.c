#include "modding.h"
#include "recomputils.h"

#include "PR/ultratypes.h"
#include "game/objects/object_id.h"
#include "game/objects/object.h"
#include "game/gamebits.h"
#include "sys/gfx/animseq.h"
#include "sys/main.h"
#include "sys/map.h"
#include "sys/math.h"
#include "sys/objects.h"

#include "objects/779_WCLevelControl.h"

#include "recomp/dlls/objects/339_recomp.h"

//TEMPORARY DEFINES
#define SeqProp_obj_Setup dll_339_setup
#define SeqProp_obj_Control dll_339_control
#define SeqProp_obj_GetDataSize dll_339_get_data_size
//END OF TEMPORARY DEFINES

typedef struct {
/*00*/ ObjSetup base;
/*18*/ u8 roll;
/*19*/ u8 pitch;
/*1A*/ u8 yaw;
/*1B*/ u8 scale;
} SeqProp_Setup;

/* RECOMP */
typedef struct {
    u8 flags;
} SeqProp_Data;

typedef enum {
    SeqProp_FLAG_1_Seq_Played = 1
} SeqProp_Flags;

/* @recomp: custom animCallback, for restoring WCSunDoor's scale after its sequence preempt */
static int SeqProp_animCallback(Object* self, Object* animObj, AnimObj_Data* animData, s8 prevCallbackValue) {
    SeqProp_Data* objData = self->data;

    if ((objData->flags & SeqProp_FLAG_1_Seq_Played) == FALSE) {
        objData->flags |= SeqProp_FLAG_1_Seq_Played;
    }

    return 0;
}

RECOMP_PATCH void SeqProp_obj_Setup(Object* self, SeqProp_Setup* setup, s32 reset) {
    self->srt.roll = setup->roll << 8;
    self->srt.pitch = setup->pitch << 8;
    self->srt.yaw = setup->yaw << 8;

    if (setup->scale != 0) {
        self->srt.scale = setup->scale / 255.0f;
        if (self->srt.scale == 0.0f) {
            self->srt.scale = 1.0f;
        }
        self->srt.scale *= self->def->scale;
    }

    //@recomp: fix Sun Temple Door flashing up in the wrong place for a frame when leaving the Sun Temple
    if (self->id == OBJ_WCSunDoor) {
        if (mainGetBits(BIT_WC_Sun_Temple_Opened)) {
            self->srt.scale = 0; //Become invisibly small
            self->animCallback = SeqProp_animCallback; //Add an animCallback to tell when the preempt has played
            return;
        }
    }

    self->stateFlags |= (OBJSTATE_CONTROL_DISABLED | OBJSTATE_UPDATE_DISABLED);
}

RECOMP_PATCH void SeqProp_obj_Control(Object* self) {
    SeqProp_Data* objData;
    SeqProp_Setup* objSetup;

    //@recomp: restore WCSunDoor's scale after its seq preempt has restored its correct position
    if (self->id == OBJ_WCSunDoor) {
        objData = self->data;
        objSetup = (SeqProp_Setup*)self->setup;
        if (objData && (objData->flags & SeqProp_FLAG_1_Seq_Played) && mainGetBits(BIT_WC_Sun_Temple_Opened)) {
            //Restore scale
            if (objSetup->scale != 0) {
                self->srt.scale = objSetup->scale / 255.0f;
                if (self->srt.scale == 0.0f) {
                    self->srt.scale = 1.0f;
                }
                self->srt.scale *= self->def->scale;
            }

            //Stop running control
            self->stateFlags |= OBJSTATE_CONTROL_DISABLED;
        }
    }
}

/* Add objData */
RECOMP_PATCH u32 SeqProp_obj_GetDataSize(Object* self, u32 offsetAddr) {
    return sizeof(SeqProp_Data);
}

#include "modding.h"
#include "recompconfig.h"
#include "recomputils.h"

#include "common.h"
#include "sys/gfx/animseq.h"
#include "sys/main.h"
#include "sys/objects.h"
#include "sys/objtype.h"
#include "dlls/objects/common/vehicle.h"

#include "recomp/dlls/objects/214_animobj_recomp.h"

typedef struct {
    AnimObj_Data base;
    s32 vehicleState;
    Object *vehicle;
} AnimObj_Data_Extended;

typedef enum {
    ANIMCMD_Parent_to_Vehicle = 3,
    ANIMCMD_Unparent_from_Vehicle = 4
} AnimCommands_Custom;

RECOMP_PATCH u32 animobj_get_data_size(Object *self, s32 arg1) {
    return sizeof(AnimObj_Data_Extended);
}

/**
  * Fixes a bug where Krystal was missing during the ending of the Rolling Demo.
  * Handles parenting/unparenting to the CloudRunner object at various points throughout the sequence.
  * (Originally by MusicalProgrammer)
  *
  * NOTE: this patch was originally applied via the print function, but it seems to need to run elsewhere in recomp
  * since the print function only runs intermittently (possibly due to different frustum culling), and misses the
  * relevant parent/unparent anim commands. The coord code here ensures Krystal is in frame, so her print function runs! 
  * The update functions seem to run after the print function however, so she lags behind the CloudRunner by 1 frame: 
  * to fix this, the coord applying part of this function also runs in print, to sync her location before the draw.
  */
static void animobj_krystal_handle_rolling_demo_ending(Object* self) {
    AnimObj_Data_Extended* objData;
    f32 minDistance;
    f32 distance;
    s32 i;
    s32 count;
    Object** objects;
    Object* vehicle;
    ObjSetup* objSetup;
    Vec3f parentPosition;

    objData = self->data;

    if (self->id == OBJ_AnimKrystal) {
        //Handle custom sequence commands
        switch (objData->base.lastMessage) {
            case ANIMCMD_Parent_to_Vehicle:
                objData->vehicleState = 3;

                //Find closest DR_CloudRunner object
                objects = obj_get_all_of_type(OBJTYPE_Vehicle, &count);
                minDistance = 131072.0;
                for (i = 0; i < count; i++) {
                    distance = vec3_distance(&self->globalPosition, &objects[i]->globalPosition);
                    if ((objects[i]->id == OBJ_DR_CloudRunner) && (distance < minDistance)) {
                        //Store object
                        objData->vehicle = objects[i];
                        minDistance = distance;
                    }
                }

                break;
            case ANIMCMD_Unparent_from_Vehicle:
                objData->vehicleState = 0;
                break;
        }

        //When parented to CloudRunner, inherit its coords
        if (objData->vehicleState == 3) {
            vehicle = objData->vehicle;

            if (!vehicle) {
                return;
            }

            self->srt.yaw = vehicle->srt.yaw;
            self->srt.pitch = vehicle->srt.pitch;
            self->srt.roll = vehicle->srt.roll;

            //Get position from vehicle DLL
            ((DLL_IVehicle*)vehicle->dll)->vtbl->get_rider_position(
                vehicle, 
                &parentPosition.x, 
                &parentPosition.y, 
                &parentPosition.z 
            );

            objSetup = self->setup;
            if (objSetup) {
                objSetup->x = parentPosition.x;
                objSetup->y = parentPosition.y;
                objSetup->z = parentPosition.z;
            }

            self->srt.transl.x = parentPosition.x;
            self->srt.transl.y = parentPosition.y;
            self->srt.transl.z = parentPosition.z;

            self->globalPosition.x = parentPosition.x;
            self->globalPosition.y = parentPosition.y;
            self->globalPosition.z = parentPosition.z;
        }
    }
}

//@recomp: call custom function fixing AnimKrystal's Rolling Demo behaviour
RECOMP_PATCH void animobj_update(Object *self) { 
    animobj_krystal_handle_rolling_demo_ending(self);
}

//@recomp: apply AnimKrystal's parent coords here too, so they're not a frame behind when printing
RECOMP_PATCH void animobj_print(Object *self, Gfx **gdl, Mtx **mtxs, Vertex **vtxs, Triangle **pols, s8 visibility) {
    if (!visibility) {
        return;
    }

    //@recomp: sync AnimKrystal's parented position before the draw
    if (self->id == OBJ_AnimKrystal) {
        AnimObj_Data_Extended* objData = self->data;
        Object* vehicle;
        Vec3f parentPosition;

        if (objData->vehicleState == 3) {
            vehicle = objData->vehicle;

            if (!vehicle) {
                return;
            }

            self->srt.yaw = vehicle->srt.yaw;
            self->srt.pitch = vehicle->srt.pitch;
            self->srt.roll = vehicle->srt.roll;

            //Get position from vehicle DLL
            ((DLL_IVehicle*)vehicle->dll)->vtbl->get_rider_position(
                vehicle, 
                &parentPosition.x, 
                &parentPosition.y, 
                &parentPosition.z 
            );

            self->srt.transl.x = parentPosition.x;
            self->srt.transl.y = parentPosition.y;
            self->srt.transl.z = parentPosition.z;

            self->globalPosition.x = parentPosition.x;
            self->globalPosition.y = parentPosition.y;
            self->globalPosition.z = parentPosition.z;
        }
    }

    draw_object(self, gdl, mtxs, vtxs, pols, 1.0f);
}

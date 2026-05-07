#include "modding.h"
#include "recompconfig.h"
#include "recomputils.h"

#include "common.h"
#include "sys/main.h"
#include "sys/objects.h"
#include "sys/objtype.h"
#include "dlls/objects/common/vehicle.h"

#include "recomp/dlls/objects/214_animobj_recomp.h"

typedef struct {
/*000*/ Object* actor;
/*004*/ s32 unk4;
/*008*/ s8 unk8[0x1A - 0x8];
/*01A*/ s16 unk1A; //angle
/*004*/ s8 unk1C[0x20 - 0x1C];
/*020*/ f32 unk20;
/*024*/ f32 unk24; //some speed
/*028*/ s32 unk28;
/*02C*/ void *unk2C;
/*030*/ u32 unk30; //soundHandle?
/*034*/ u32 unk34[4];   //soundHandles?
/*044*/ s16 unk44[4];   //soundHandle related - playback frequency?
/*04C*/ Vec3f unk4C; //position diff between parent "override" animObj and child "actor" object?
/*058*/ f32 unk58;
/*05C*/ s16 yawDiff;
/*05E*/ s16 pitchDiff;
/*060*/ s16 rollDiff;
/*062*/ s8 unk62;
/*063*/ s8 unk63;
/*064*/ s16 animCurvesCurrentFrameA;
/*066*/ s16 animCurvesCurrentFrameB;
/*068*/ s16 animCurvesDuration;
/*06A*/ s16 unk6A;
/*06C*/ s16 unk6C;
/*06E*/ s16 animCurvesEventCount;
/*070*/ s16 animCurvesKeyframeCount;
/*072*/ s16 unk72;
/*074*/ s16 unk74;
/*076*/ s16 unk76;
/*078*/ s8 unk78[0x7A - 0x78];
/*07A*/ s16 unk7A;
/*07C*/ s16 unk7C;
/*07E*/ s8 unk7E[0x80 - 0x7E];
/*08B*/ s32 unk80;
/*084*/ s8 unk84[0x86 - 0x84];
/*086*/ s8 unk86;
/*087*/ s8 unk87;
/*088*/ s8 unk88; // unk type
/*088*/ s8 unk89;
/*08A*/ s8 unk8A;                   //soundHandle-related
/*08B*/ u8 unk8B;
/*08C*/ u8 unk8C;
/*08D*/ u8 unk8D;
/*08E*/ u8 unk8E[0x98 - 0x8E];      //sequence subcommand IDs?
/*098*/ u8 unk98;                   //sequence subcommand count?
/*099*/ u8 unk99;
/*09A*/ u8 unk9A;
/*09B*/ u8 unk9B;
/*09C*/ s8 unk9C[0x9D - 0x9C];
/*09D*/ u8 unk9D;
/*09E*/ s8 unk9E[0xA0 - 0x9E];
/*0A0*/ AnimCurvesEvent* animCurvesEvents;
/*0A4*/ AnimCurvesKeyframe* animCurvesKeyframes;
/*0A8*/ s16 channelFirstKeyIndex[ANIMCURVES_KEYFRAME_CHANNELS];
/*0CE*/ s16 channelTotalKeys[ANIMCURVES_KEYFRAME_CHANNELS];
/*0F4*/ AnimObj_DataF4Callback unkF4; //end-of-sequence callback function
/*0F8*/ AnimObj_DataF8Callback unkF8;
/*0FC*/ s8 unkFC[0x11C - 0xFC];
/*11C*/ Object* unk11C;
/*120*/ s8 unk120[0x134 - 0x120];
/*134*/ s8 unk134;                      //@recomp: new member (currently unmapped in decomp)
/*138*/ Object* unk138;                 //@recomp: new member (currently unmapped in decomp)
/*120*/ s8 unk13C[0x142 - 0x13C];
/*142*/ u8 unk142_4: 4;
/*142*/ u8 unk142_0: 4;
/*143*/ s8 unk143[0x144 - 0x143];
} AnimObj_Data_CustomMapping; //TO-DO: remove once the actual AnimObj_Data struct is more complete?

typedef enum {
    ANIMCMD_Parent_to_Vehicle = 3,
    ANIMCMD_Unparent_from_Vehicle = 4
} AnimCommands_Custom;

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
    AnimObj_Data_CustomMapping* objData;
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
        switch (objData->unk8D) {
            case ANIMCMD_Parent_to_Vehicle:
                objData->unk134 = 3;

                //Find closest DR_CloudRunner object
                objects = obj_get_all_of_type(OBJTYPE_11, &count);
                minDistance = 131072.0;
                for (i = 0; i < count; i++) {
                    distance = vec3_distance(&self->globalPosition, &objects[i]->globalPosition);
                    if ((objects[i]->id == OBJ_DR_CloudRunner) && (distance < minDistance)) {
                        //Store object
                        objData->unk138 = objects[i];
                        minDistance = distance;
                    }
                }

                break;
            case ANIMCMD_Unparent_from_Vehicle:
                objData->unk134 = 0;
                break;
        }

        //When parented to CloudRunner, inherit its coords
        if (objData->unk134 == 3) {
            vehicle = objData->unk138;

            if (!vehicle) {
                return;
            }

            self->srt.yaw = vehicle->srt.yaw;
            self->srt.pitch = vehicle->srt.pitch;
            self->srt.roll = vehicle->srt.roll;

            //Get position from vehicle DLL
            ((DLL_IVehicle*)vehicle->dll)->vtbl->func9(
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
        AnimObj_Data_CustomMapping* objData = self->data;
        Object* vehicle;
        Vec3f parentPosition;

        if (objData->unk134 == 3) {
            vehicle = objData->unk138;

            if (!vehicle) {
                return;
            }

            self->srt.yaw = vehicle->srt.yaw;
            self->srt.pitch = vehicle->srt.pitch;
            self->srt.roll = vehicle->srt.roll;

            //Get position from vehicle DLL
            ((DLL_IVehicle*)vehicle->dll)->vtbl->func9(
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

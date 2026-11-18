#include "modding.h"

#include "dll.h"
#include "dlls/engine/84_camnormal.h"
#include "sys/joypad.h"

#include "recomp/dlls/engine/86_cam1stperson_recomp.h"

//TEMPORARY DEFINES
#define cam1stperson_Control cam1stperson_func_4F8
#define cam1stperson_easeIn cam1stperson_func_A74
#define cam1stperson_lookAround cam1stperson_func_D90
#define cam1stperson_easeOutCalculate cam1stperson_func_1008
#define cam1stperson_easeOutBegin cam1stperson_func_1524

#define easeActive1stPerson unk118

/* Flags for `camnormal_func_1A58` */
typedef enum {
    CamNormal_TRACKFLAG_Do_Intersect_Check = 1,
    CamNormal_TRACKFLAG_Get_Floor_Ceiling_Heights = 2
} CamNormal_TrackFlags;
#define CamNormal_TRACKFLAG_Check_All (CamNormal_TRACKFLAG_Do_Intersect_Check | CamNormal_TRACKFLAG_Get_Floor_Ceiling_Heights)
//END OF TEMPORARY DEFINES

typedef struct {
    f32 distance3D; //The goal distance for the camera (relative to the player) when easing out of 1st person
    f32 offsetY;    //The goal height for the camera (relative to the player) when easing out of 1st person
    f32 offsetX;    //The camera's position offset (relative to the player) when the ease into 1st person has finished
    f32 offsetZ;    //The camera's position offset (relative to the player) when the ease into 1st person has finished
    f32 splineX[4];         //Ease interpolation points
    f32 splineY[4];         //Ease interpolation points
    f32 splineZ[4];         //Ease interpolation points
    f32 splineYaw[4];       //Ease interpolation points
    f32 splinePitch[4];     //Ease interpolation points
    u8 _unk60[0x70 - 0x60];
    f32 nearestFloorY;      //Track height value, stored during the ease out
    f32 nearestCeilingY;    //Track height value, stored during the ease out
    CurvesStruct curves;    //For ease interpolation
    f32 verticalAdjust;     //A camera pitch adjustment (relative to player pitch) when in locked view mode, or a secondary goal Y adjustment for the exit ease
    f32 easeDistance2D;     //The lateral distance to travel during the exit ease
    f32 yawSpeed;           //Left/right turn speed while looking around, controlled with the joystick
    u8 state;               //`Cam1stPerson_States`
} Cam1stPerson;

typedef enum {
    Cam1stPerson_STATE_0_Ease_In,
    Cam1stPerson_STATE_1_Ease_In_Finished,
    Cam1stPerson_STATE_2_Player_Has_Control,
    Cam1stPerson_STATE_3_Ease_Out_Start,
    Cam1stPerson_STATE_4_Ease_Out,
    Cam1stPerson_STATE_5_Locked_View
} Cam1stPerson_States;

#define PLAYER_HEAD_HEIGHT 27.0f

/*0x0*/ extern Cam1stPerson* sState;

extern s32 cam1stperson_easeIn(Cam* cam, Object* player);
extern void cam1stperson_lookAround(Cam* cam);
extern void cam1stperson_easeOutCalculate(Cam* cam);
extern void cam1stperson_easeOutBegin(Cam* cam, Object* player);

/* Use a track intersect check during the exit ease, to help prevent the camera popping out of bounds */
RECOMP_PATCH void cam1stperson_Control(Cam* cam) {
    u16 buttons;
    s32 prevEaseFinished;
    f32 distanceSq;
    f32 dx;
    f32 dz;
    f32 tValueOpacity;
    CamControl_Module* camNormal;
    Object* player;

    player = cam->player;
    cam->highlightFlags |= 2;
    buttons = joyGetPressed(0);

    switch (sState->state) {
    case Cam1stPerson_STATE_0_Ease_In:
        sState->state = cam1stperson_easeIn(cam, cam->player);
        cam->easeActive1stPerson = TRUE;
        break;
    case Cam1stPerson_STATE_1_Ease_In_Finished:
        if (curves_func_800053B0(&sState->curves, 1000.0f)) {
            sState->offsetX = cam->srt.transl.x - cam->player->srt.transl.x;
            sState->offsetZ = cam->srt.transl.z - cam->player->srt.transl.z;
            sState->state = Cam1stPerson_STATE_2_Player_Has_Control;
        }
        cam->srt.yaw = sState->curves.unk68.x;
        cam->easeActive1stPerson = TRUE;
        break;
    case Cam1stPerson_STATE_2_Player_Has_Control:
        //Look around with the joystick
        cam1stperson_lookAround(cam);

        //Exit 1st person if any button is pressed
        if (buttons != 0) {
            cam1stperson_easeOutCalculate(cam);
            sState->state = Cam1stPerson_STATE_3_Ease_Out_Start;
        }

        cam->easeActive1stPerson = FALSE;
        break;
    case Cam1stPerson_STATE_5_Locked_View:
        cam->srt.yaw = M_180_DEGREES - player->srt.yaw;
        cam->srt.pitch = player->srt.pitch + (sState->verticalAdjust * 182.0f);
        cam->srt.roll = player->srt.roll;
        cam->srt.transl.x = player->globalPosition.x;
        cam->srt.transl.y = player->globalPosition.y + sState->offsetY;
        cam->srt.transl.z = player->globalPosition.z;
        cam->fov = 80.0f;
        break;
    case Cam1stPerson_STATE_3_Ease_Out_Start:
        prevEaseFinished = curves_func_800053B0(&sState->curves, 1000.0f);
        cam->srt.yaw = sState->curves.unk68.x;
        cam->srt.pitch = sState->curves.unk68.y;
        if (prevEaseFinished) {
            cam1stperson_easeOutBegin(cam, cam->player);
            sState->state = Cam1stPerson_STATE_4_Ease_Out;
        }
        cam->easeActive1stPerson = TRUE;
        break;
    case Cam1stPerson_STATE_4_Ease_Out:
        //Fade player back in
        {
            tValueOpacity = sState->curves.unk0 - 0.2f;
            if (tValueOpacity < 0.0f) {
                tValueOpacity = 0.0f;
            }
            tValueOpacity *= 1.25f;
            cam->player->opacity = (s32) (tValueOpacity * OBJECT_OPACITY_MAX);
        }

        //Switch back to camnormal when the exit ease is finished
        if (curves_func_800053B0(&sState->curves, 5.0f)) {
            gDLL_2_Camera->vtbl->change_camera_module(DLL_ID_CAMNORMAL, FALSE, 1, 0, NULL, 0, Cam_Ease_All);
            cam->player->opacity = OBJECT_OPACITY_MAX;
        }

        //Move the camera along the curves
        cam->srt.transl.x = sState->curves.unk68.x;
        cam->srt.transl.z = sState->curves.unk68.z;

        //Check track floor/ceiling heights, and limit the camera's Y accordingly
        camNormal = gDLL_2_Camera->vtbl->get_camnormal_module();
        //@recomp: change flags to do an intersection check too
        ((DLL_84_camnormal*)camNormal->dll)->vtbl->func8(cam, CamNormal_TRACKFLAG_Check_All, 3, &sState->nearestFloorY, &sState->nearestCeilingY);
        ((DLL_84_camnormal*)camNormal->dll)->vtbl->func6(cam, cam->player, sState->nearestFloorY, sState->nearestCeilingY);

        dx = cam->srt.transl.x - cam->player->srt.transl.x;
        dz = cam->srt.transl.z - cam->player->srt.transl.z;
        distanceSq = SQ(dx) + SQ(dz);
        if (distanceSq < SQ(10)) {
            cam->srt.pitch = 0;
        } else {
            ((DLL_84_camnormal*)camNormal->dll)->vtbl->func5(cam, cam->player->srt.transl.y, sqrtf(distanceSq));
        }

        cam->easeActive1stPerson = TRUE;

        break;
    }

    //Exit first-person when the player is hit (@bug: doesn't seem to work)
    if (func_80025F40(cam->player, NULL, NULL, NULL) != 0) {
        cam1stperson_easeOutCalculate(cam);
        cam->srt.transl.x = sState->splineX[1];
        cam->srt.transl.y = sState->splineY[1];
        cam->srt.transl.z = sState->splineZ[1];
        gDLL_2_Camera->vtbl->change_camera_module(DLL_ID_CAMNORMAL, FALSE, 1, 0, NULL, 0, Cam_Ease_None);
    }
}

#include "modding.h"
#include "recomputils.h"
#include "dll_util.h"
#include "sidekick_util.h"

#include "game/objects/interaction_arrow.h"
#include "game/objects/object_id.h"
#include "sys/dll.h"
#include "sys/joypad.h"
#include "sys/main.h"
#include "sys/menu.h"
#include "sys/print.h"
#include "sys/objects.h"
#include "sys/objtype.h"
#include "dll.h"
#include "dlls/engine/2_camcontrol.h"

#include "recomp/dlls/engine/2_camcontrol_recomp.h"

typedef enum {
    LockIcon_STATE_Hidden = 0,
    LockIcon_STATE_Vanish = 1,
    LockIcon_STATE_Highlight_Start = 2,
    LockIcon_STATE_Highlighted = 3,
    LockIcon_STATE_Lock_On = 4
} LockIcon_States;

extern CamControl_Data sCamDataStruct;
extern CamControl_Data* sCamData;

extern s32 sActiveID;          //Active module: DLL ID
extern s32 sActiveLoadedIndex; //Active module: The loaded index of the camera module DLL currently in use
extern s32 sNextID;            //Ease (End Module): DLL ID of next module (when switching camera DLLs)

extern s8 sIconRapidTimer;    //LockIcon spins rapidly for this long at start of lock-on, before decelerating to lower speed
extern s8 sIconState;         //State Machine value for OBJ_LockIcon
extern s16 sIconRotateSpeed;  //Rotation of lock-on icon
extern Object* sLockIcon;     //OBJ_LockIcon

extern void CamControl_set_letterbox_goal(s32 height, s32 startAtGoal);

#ifdef DINOMOD_ROM_PATCH
/**
  * (For ROM patches only, not used in recomp)
  *
  * Clears the letterboxed region of the framebuffer when starting a save by initialising 
  * the camera letterboxing's Y decrement speed.
  *
  * This region usually retains some junk pixels from drawing the Save Select screen, which 
  * aren't cleared until the first time the camera's letterboxing changes (usually when 
  * Z-targetting or entering a sequence).
  */
static void set_letterbox_on_game_start(void) {
    static u8 rsLetterboxEditDone = FALSE;
    static u8 rsFramesIntoGameplay = 0;

    if (!rsLetterboxEditDone && 
        (menu_get_previous() == MENU_GAME_SELECT) && 
        (menu_get_current() == MENU_GAMEPLAY)
    ) {
        if (rsFramesIntoGameplay < 1) {
            rsFramesIntoGameplay += gUpdateRate;
            return;
        }

        CamControl_set_letterbox_goal(10, TRUE);
        rsLetterboxEditDone = TRUE;
    }
}
#endif

/**
  * Clear LockIcon's parent pointer when no longer focused on an object (originally by MusicalProgrammer).
  */
RECOMP_PATCH void CamControl_lock_icon_tick(void) {
    Object* hlObject;
    Object* arrow;
    Object* player;
    f32 dx;
    f32 dy;
    f32 dz;
    f32 distSquared;
    s32 opacity;
    u8 targetFlags;
    ObjectStruct74* targetCoords;

    //@rom-patch: clear letterbox region of framebuffer as gameplay begins
    #ifdef DINOMOD_ROM_PATCH
    set_letterbox_on_game_start();
    #endif

    arrow = sLockIcon;
    hlObject = sCamData->highlight;
    player = get_player();
    
    if (menu_get_current() == MENU_TITLE_SCREEN) {
        return;
    }
    
    sCamData->targetFlags &= ~ARROW_FLAG_4_Highlighted;
    
    if ((player == NULL) || (player->stateFlags & OBJSTATE_IN_SEQ)) {
        sIconState = LockIcon_STATE_Hidden;
        return;
    }
    
    if (hlObject != NULL) {
        //Get lateral distance between player and lock-on point
        distSquared = hlObject->unk78[hlObject->unkD4].interactRadius * 4;
        distSquared *= distSquared;

        targetCoords = &hlObject->unk74[hlObject->unkD4];
        dx = player->globalPosition.x - targetCoords->refPoint.x;
        dz = player->globalPosition.z - targetCoords->refPoint.z;
        
        //Check whether to ignore differences in player/target's y coordinates
        if (hlObject->unkAF & ARROW_FLAG_80_Ignore_TranslateY) {
            dy = 0.0f;
        } else {
            dy = player->globalPosition.y - targetCoords->refPoint.y;
        }
        
        //Highlight the target object when in range
        if ((SQ(dx) + SQ(dz) < distSquared) && ((dy > -100.0f) && (dy < 100.0f))) {
            hlObject->unkAF |= ARROW_FLAG_4_Highlighted;
            sCamData->targetFlags |= 4;
        }
        
        //Update target Object's interaction flags when A button pressed
        if (hlObject->unkAF & ARROW_FLAG_4_Highlighted) {
            if (!(hlObject->unkAF & ARROW_FLAG_10_Greyed_Out)){
                if (joy_get_pressed(0) & A_BUTTON) {
                hlObject->unkAF |= ARROW_FLAG_1_Interacted;
                }
            } else if (joy_get_pressed(0) & A_BUTTON) {
                gDLL_6_AMSFX->vtbl->play_sound(hlObject, SOUND_6E6_Interaction_Refused, MAX_VOLUME, 0, 0, 0, 0);
            }
        }
    }
    
    //OBJ_LockIcon State Machine
    switch (sIconState) {
    case LockIcon_STATE_Hidden:
    default:
        sIconRotateSpeed = 0;
        arrow->opacity = OBJECT_OPACITY_MAX;
        if (hlObject != NULL) {
            sIconState = LockIcon_STATE_Highlight_Start;
        }
        break;

    case LockIcon_STATE_Highlight_Start:
        //When a nearby Object starts being highlighted by the LockIcon (playing a little "blip!" sound)
        sIconRotateSpeed = 0;
        arrow->opacity = OBJECT_OPACITY_MAX;
        sIconState = LockIcon_STATE_Highlighted;
        gDLL_6_AMSFX->vtbl->play_sound(hlObject, SOUND_43C_Target_Highlighted, MAX_VOLUME, 0, 0, 0, 0);
        break;

    case LockIcon_STATE_Highlighted:
        //While the LockIcon is highlighting a nearby Object (but it's not targeted yet)
        if ((hlObject == NULL) || (sActiveID == DLL_ID_CAM1STPERSON) || (hlObject->unkAF & (ARROW_FLAG_20_Removed | ARROW_FLAG_8_No_Targetting))) {
            gDLL_6_AMSFX->vtbl->play_sound(hlObject, SOUND_72E_Lock_Disengage, MAX_VOLUME, 0, 0, 0, 0);
            sIconState = LockIcon_STATE_Vanish;
        } else {
            //Automatically lock-on (during Ship Battle?)
            if (sActiveID == DLL_ID_CAMSHIPBATTLE1) {
                gDLL_6_AMSFX->vtbl->play_sound(hlObject, SOUND_72D_Lock_On, MAX_VOLUME, 0, 0, 0, 0);
                sIconState = LockIcon_STATE_Lock_On;
                sIconRapidTimer = 60; //icon spins extra quickly for 1st second
            }
            
            //Decelerate rotation speed
            if (sIconRotateSpeed > 0) {
                sIconRotateSpeed -= gUpdateRate * 8;
            }
            break;
        }
        break;
    case LockIcon_STATE_Lock_On:
        //Disengage lock when needed
        if ((hlObject == NULL) || (hlObject->unkAF & (ARROW_FLAG_20_Removed | ARROW_FLAG_8_No_Targetting))) {
            gDLL_6_AMSFX->vtbl->play_sound(hlObject, SOUND_72E_Lock_Disengage, MAX_VOLUME, 0, 0, 0, 0);
            sIconState = LockIcon_STATE_Vanish;
            break;
        }
        
        if (sActiveID != DLL_ID_CAMSHIPBATTLE1) {
            sIconState = LockIcon_STATE_Highlighted;
        }
        
        hlObject->unkAF |= ARROW_FLAG_2_Targeted;
        
        //Handle icon rotation speed (fast at first, then slows to lower speed)
        if (sIconRapidTimer > 0) {
            sIconRapidTimer -= gUpdateRate;
            sIconRotateSpeed = 0x400;
        } else {
            if (sIconRotateSpeed > 0x200) {
                sIconRotateSpeed -= gUpdateRate * 8;
            }
            if (sIconRotateSpeed < 0x200) {
                sIconRotateSpeed = 0x200;
            }
        }
        break;
    case LockIcon_STATE_Vanish:
        //Fade out the LockIcon
        opacity = arrow->opacity - (gUpdateRate * 12);
        if (opacity < 0) {
            opacity = 0;
        }
        arrow->opacity = opacity;
        
        //LockIcon moves upwards while vanishing
        if (arrow->opacity > 0) {
            sIconRapidTimer -= gUpdateRate;
            arrow->srt.transl.y += 2.0f * gUpdateRateF;
            arrow->globalPosition.y += 2.0f * gUpdateRateF;
        } else {
            sIconState = LockIcon_STATE_Hidden;

            //@recomp: clear LockIcon's parent pointer
            arrow->parent = NULL;
        }

        //Reappear if an object becomes highlighted midway through the fade-out
        if (hlObject) {
            sIconState = LockIcon_STATE_Highlight_Start;
        }
        break;
    }
    
    //Update rotation
    if (sIconRotateSpeed < 0) {
        sIconRotateSpeed = 0;
    }    
    arrow->srt.yaw += sIconRotateSpeed * gUpdateRate;
}

RECOMP_PATCH void CamControl_set_target_flag_2(s32 enable) {
    // @recomp: Return early if in Galadon fight (removes forced z-targeting)
    {
        Object **objectList;
        s32 count;

        objectList = obj_get_all_of_type(OBJTYPE_4, &count);

        for (s32 i = 0; i < count; i++) {
            if (objectList[i]->id == OBJ_DIM_Boss) {
                return;
            }
        }
    }

    if (enable) {
        sCamData->targetFlags |= 2;
    } else {
        sCamData->targetFlags &= ~2;
    }
}

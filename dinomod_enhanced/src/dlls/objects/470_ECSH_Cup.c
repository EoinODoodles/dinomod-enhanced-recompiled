#include "modding.h"

#include "PR/ultratypes.h"
#include "PR/gbi.h"
#include "dlls/objects/210_player.h"
#include "dlls/objects/469_ECSHshrine.h"
#include "game/objects/object.h"
#include "game/gamebits.h"
#include "sys/gfx/animseq.h"
#include "sys/gfx/model.h"
#include "sys/gfx/modgfx.h"
#include "sys/gfx/texture.h"
#include "sys/dll.h"
#include "sys/objects.h"
#include "sys/main.h"
#include "sys/math.h"
#include "sys/map_enums.h"
#include "sys/map.h"
#include "sys/objmsg.h"
#include "sys/objprint.h"
#include "sys/objtype.h"
#include "sys/print.h"
#include "dll.h"
#include "types.h"

#include "recomp/dlls/objects/470_ECSH_Cup_recomp.h"

#define SINK_DISTANCE 50.0f

/*0x0*/ static Object* dShrine = NULL; //ECSH_Shrine object

typedef struct {
    ObjSetup base;
    s16 unk18;
    s16 cupIndex;
} ECSHCup_Setup;

typedef struct {
    Vec3f home;
    f32 speedX;
    f32 speedY;
    f32 speedZ;
    f32 groundY;
    s32 prevState;
    s32 cupIndex;
    s16 fxTimer;
    s16 rotateSpeed;
    s16 bobTimer;
    s8 bobSpeed;
    /* RECOMP */
    u8 state;       //The cup's current state
    u8 shrineState; //The shrine's current state
    f32 goalX;      //The position the cup's moving to
    f32 goalZ;      //The position the cup's moving to
} ECSHCup_Data;

/* Return early when underground, to avoid poking through the floor before the test begins. */
RECOMP_PATCH void ECSHCup_control(Object* self) {
    ECSHCup_Data* objData = self->data;
    s32 states[] = { ECShrine_STATE_Waiting, Cup_STATE_Unknown }; //@recomp: use an array, to store the shrine's state as well as the cups' state
    Vec3f goal = VEC3F(0, 0, 0);
    u8 cupWithSpirit = 0;
    s16 opacity;
    f32 objectDistance = 500.0f;
    Object* player;
    /* RECOMP */
    u8 state;

    player = objGetPlayer();
    
    if (dShrine == NULL){
        dShrine = objGetNearestTypeTo(OBJTYPE_13, self, &objectDistance);
    }
    if (dShrine == NULL){
        return;
    }
    
    //Get cup minigame state, and the index of the cup holding the Spirit
    ((DLL_469_ECSHshrine*)dShrine->dll)->vtbl->get_minigame_state(states, &cupWithSpirit);
    
    //@recomp: store shrine state
    objData->shrineState = states[0];
    objData->state = states[1];
    state = objData->state;

    //@recomp: return early when still underground
    if (objData->shrineState < ECShrine_STATE_Cup_Game_Start) {
        self->srt.transl.y = objData->home.y - SINK_DISTANCE;
        return;
    }

    //Spin
    self->srt.yaw += objData->rotateSpeed * gUpdateRate / 2; //@recomp: fix framerate dependency

    //Create particles under cup
    if (state != Cup_STATE_Rise_Up){
        objData->fxTimer -= gUpdateRate;
        if (objData->fxTimer <= 0){
            objData->fxTimer = 10;
            if (((state != Cup_STATE_Underground) && (state != Cup_STATE_Rise_Up)) && (state != Cup_STATE_Sink_Down)){
                gDLL_17_partfx->vtbl->spawn(self, 0x270, 0, 0, -1, 0);
            }
        }
    }    

    //Animate up/down oscillation (no easing, even spacing)
    objData->bobTimer -= gUpdateRate;
    if (objData->bobTimer <= 0){
        objData->bobSpeed = -objData->bobSpeed;
        objData->bobTimer = 100;
    }
    self->srt.transl.y += 0.09f * objData->bobSpeed * gUpdateRateF * 0.5f; //@recomp: fix framerate dependency

    if (objData->cupIndex == cupWithSpirit){
    }
    
    //Move
    if ((state == Cup_STATE_Moving) && (objData->prevState == Cup_STATE_Moving)) {
        //@recomp: fix framerate dependency
        self->srt.transl.x += objData->speedX * gUpdateRateF * 0.5f; 
        self->srt.transl.z += objData->speedZ * gUpdateRateF * 0.5f;

        //@recomp: don't go beyond goal
        if (objData->speedX < 0) {
            if (self->srt.transl.x < objData->goalX) {
                self->srt.transl.x = objData->goalX;
            }
        } else {
            if (self->srt.transl.x > objData->goalX) {
                self->srt.transl.x = objData->goalX;
            }
        }
        if (objData->speedZ < 0) {
            if (self->srt.transl.z < objData->goalZ) {
                self->srt.transl.z = objData->goalZ;
            }
        } else {
            if (self->srt.transl.z > objData->goalZ) {
                self->srt.transl.z = objData->goalZ;
            }
        }
    }
    
    //STATE MACHINE
    if (state == Cup_STATE_Rise_Up) {
        //Rise up from ground
        if (self->srt.transl.y < objData->groundY) {
            self->srt.transl.y += 0.5f * gUpdateRateF;
        }
        
        //Fade in
        if (self->opacityWithFade != OBJECT_OPACITY_MAX) {
            opacity = self->opacityWithFade;
            opacity += gUpdateRate * 2;
            if (opacity >= OBJECT_OPACITY_MAX) {
                opacity = OBJECT_OPACITY_MAX;
            }
            self->opacityWithFade = opacity;
        }
        
        //Create particles
        objData->fxTimer -= gUpdateRate;
        if (objData->fxTimer <= 0) {
            objData->fxTimer = 10;
            gDLL_17_partfx->vtbl->spawn(self, 0x271, 0, 0, -1, 0);
        }
    } else if (state == Cup_STATE_Sink_Down) {
        //Sink into ground
        if ((objData->groundY - SINK_DISTANCE) < self->srt.transl.y) {
            self->srt.transl.y -= 0.5f * gUpdateRateF;
            
            //Create particles
            objData->fxTimer -= gUpdateRate;
            if (objData->fxTimer <= 0) {
                objData->fxTimer = 10;
                if (state != Cup_STATE_Underground) {
                    gDLL_17_partfx->vtbl->spawn(self, 0x271, 0, 0, -1, 0);
                }
            }
        }
        
        //Fade out
        if (self->opacityWithFade != 0) {
            opacity = self->opacityWithFade;
            opacity -= gUpdateRate * 2;
            if (opacity <= 0) {
                opacity = 0;
            }
            self->opacityWithFade = opacity;
        }
        
        dll_amSfx->FreeObject(self);
    } else if ((state == Cup_STATE_Round_Start) && (state != objData->prevState)) {
        if (objData->cupIndex == cupWithSpirit) {
            gDLL_3_Animation->vtbl->start_obj_sequence(0, self, -1);
            dll_amSfx->Play(0, SOUND_343_Eerie_Ringing, 0x57, 0, 0, 0, 0);
            dll_amSfx->Play(self, SOUND_33F_Cup_Slide_Loop, MAX_VOLUME, 0, 0, 0, 0);
        } else {
            dll_amSfx->Play(self, SOUND_33F_Cup_Slide_Loop, 0x1E, 0, 0, 0, 0);
        }
        objData->prevState = state;
    } else if ((state == Cup_STATE_Moving) && (state != objData->prevState)) {
        ((DLL_469_ECSHshrine*)dShrine->dll)->vtbl->get_cup_coords(objData->cupIndex, &goal.x, &goal.z);
        //@recomp: store current goal
        objData->goalX = goal.x;
        objData->goalZ = goal.z;

        objData->speedX = (goal.x - self->srt.transl.x) / 12.0f;
        objData->speedZ = (goal.z - self->srt.transl.z) / 12.0f;
        objData->home.x = self->srt.transl.x;
        objData->home.z = self->srt.transl.z;
        objData->prevState = state;
    } else if ((state == Cup_STATE_Stopped) && (state != objData->prevState)) {
        objData->speedX = 0.0f;
        objData->speedZ = 0.0f;
        objData->prevState = state;
    } else if ((state == Cup_STATE_Shuffle) && (state != objData->prevState)) {
        objData->speedX = 0.0f;
        objData->speedZ = 0.0f;
        ((DLL_469_ECSHshrine*)dShrine->dll)->vtbl->set_cup_coords(objData->cupIndex, self->srt.transl.x, self->srt.transl.z);
        objData->prevState = state;
    } else if ((state == Cup_STATE_Underground) && (state != objData->prevState)) {
        objData->prevState = state;
    } else if ((state == Cup_STATE_Move_Finished) && (state != objData->prevState)) {
        ((DLL_469_ECSHshrine*)dShrine->dll)->vtbl->get_cup_coords(objData->cupIndex, &goal.x, &goal.z);
        self->srt.transl.x = goal.x;
        self->srt.transl.z = goal.z;
        objData->prevState = state;
    } else if ((state == Cup_STATE_Await_Choice) && player && vec3Distance(&self->globalPosition, &player->globalPosition) < 30.0f) {
        ((DLL_469_ECSHshrine*)dShrine->dll)->vtbl->choose_cup(objData->cupIndex);
        if (objData->cupIndex == cupWithSpirit) {
            gDLL_3_Animation->vtbl->start_obj_sequence(1, self, -1);
        }
    }
}

/* Don't draw the cup before the test starts */
RECOMP_PATCH void ECSHCup_print(Object *self, Gfx **gdl, Mtx **mtxs, Vertex **vtxs, Triangle **pols, s8 visibility) {
    ECSHCup_Data* objData = self->data; //@recomp

    if (visibility && objData && (objData->shrineState >= ECShrine_STATE_Cup_Game_Start)) {
        objprintDrawModel(self, gdl, mtxs, vtxs, pols, 1.0f);
    }
}

/* Extend objData */
RECOMP_PATCH u32 ECSHCup_get_data_size(Object* self, u32 offsetAddr){
    return sizeof(ECSHCup_Data);
}

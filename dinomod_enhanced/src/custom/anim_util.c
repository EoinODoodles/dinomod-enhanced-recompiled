/* Helper functions for animations */

#include "anim_util.h"
#include "recomputils.h"
#include "sys/joypad.h"
#include "sys/main.h"

/** Inspect an Object's animations using the D-pad.
  * - D-left: previous animation
  * - D-right: next animation
  * - D-down: reposition object
  *
  * One way to use this is to call it near the start of an object's 
  * control function, returning early when overridden. 
  *
  * It can be useful for documenting modanim enums!
  */
int object_modanim_debugger(Object* obj) {
    static Object* rsOverriddenObject = NULL;
    static s32 rsModAnimOverrideIdx = 0;

    static int buttonsReleased = TRUE;

    //Set an Object to override (useful to override just the first instance of an Object)
    if (rsOverriddenObject == NULL) {
        rsOverriddenObject = obj;
        rsModAnimOverrideIdx = 0;
    }
    if (rsOverriddenObject && (rsOverriddenObject->id != obj->id)) {
        rsOverriddenObject = obj;
        rsModAnimOverrideIdx = 0;
    }
    if (rsOverriddenObject == NULL) {
        return FALSE;
    }
    if (rsOverriddenObject && (rsOverriddenObject->stateFlags & OBJSTATE_DESTROYED)) {
        rsOverriddenObject = NULL;
        return FALSE;
    }
    if (obj != rsOverriddenObject) {
        return FALSE;
    }

    obj = rsOverriddenObject;
    
    //Get button presses/holds/releases 
    //(TODO: move to a controller util function, or use core funcs if there's a quick way to get unmasked releases/holds?)
    u16 pressedButtons = 0;
    u16 releasedButtons = 0;
    u16 heldButtons = 0;
    {
        static u16 prevButtons;
        static u16 currentButtons;
        
        currentButtons = gContPads[gVirtualContPortMap[0]].button;
        
        for (int i = 0; i < 16; i++) {
            //Pressed
            if (!(prevButtons & (1 << i)) && (currentButtons & (1 << i))) {
                pressedButtons |= (1 << i);
            }
            //Released
            if ((prevButtons & (1 << i)) && !(currentButtons & (1 << i))) {
                releasedButtons |= (1 << i);
            }
            //Held
            if ((prevButtons & (1 << i)) && (currentButtons & (1 << i))) {
                heldButtons |= (1 << i);
            }
            
        }

        prevButtons = currentButtons;
    }
    
    //Control modanim index
    
    //Decrease
    {
        static s32 decreaseTimer = 10;
        int doDecrease = FALSE;
        u16 decreaseButtons = (L_CBUTTONS | L_JPAD);

        if (pressedButtons & decreaseButtons) {
            doDecrease = TRUE;
        } 
        
        if (heldButtons & decreaseButtons) {
            decreaseTimer -= gUpdateRate;
            if (decreaseTimer < 0) {
                decreaseTimer = 5;
                doDecrease = TRUE;
            }
        } else {
            decreaseTimer = 30;
        }
        
        if (doDecrease) {
            rsModAnimOverrideIdx--;
            if (rsModAnimOverrideIdx < 0) {
                rsModAnimOverrideIdx = 0;
            }
        }
    } 

    //Increase 
    {
        static s32 increaseTimer = 30;
        int doIncrease = FALSE;
        u16 increaseButtons = (R_CBUTTONS | R_JPAD);
        
        if (pressedButtons & increaseButtons) {
            doIncrease = TRUE;
        }

        if (heldButtons & increaseButtons) {
            increaseTimer -= gUpdateRate;
            if (increaseTimer < 0) {
                increaseTimer = 5;
                doIncrease = TRUE;
            }
        } else {
            increaseTimer = 30;
        }

        if (doIncrease) {
            s32 animCount;
            if (obj->modelInsts && 
                obj->modelInsts[obj->modelInstIdx] && 
                obj->modelInsts[obj->modelInstIdx]->model &&
                obj->modelInsts[obj->modelInstIdx]->model->animCount
            ) {
                animCount = obj->modelInsts[obj->modelInstIdx]->model->animCount;
            } else {
                animCount = 0;
            }

            if (animCount > 0) {
                rsModAnimOverrideIdx++;
                if (rsModAnimOverrideIdx >= animCount) {
                    rsModAnimOverrideIdx = animCount - 1;
                }
            }
        }
    }

    //Reposition object in front of player with D-down
    if ((pressedButtons | heldButtons) & D_JPAD) {
        Object* player = get_player();
        f32 radius = 30.0f;
        if (player) {
            s32 yaw = player->srt.yaw + M_180_DEGREES;
            CIRCLE_WRAP(yaw)
            obj->srt.transl.x = player->srt.transl.x + fsin16_precise(yaw)*radius;
            obj->srt.transl.y = player->srt.transl.y;
            obj->srt.transl.z = player->srt.transl.z + fcos16_precise(yaw)*radius;
            obj->srt.yaw = player->srt.yaw;
        }
    }

    //Change and advance animation
    if (obj->curModAnimId != rsModAnimOverrideIdx) {
        func_80023D30(obj, rsModAnimOverrideIdx, 0.0f, 0);
    }
    func_80024108(obj, 0.005f, gUpdateRate, NULL);

    //Print info
    if (obj->def) {
        diPrintf("DEBUGGING %s\n", obj->def->name);
    }
    diPrintf("MODANIM: %d\n", obj->curModAnimId);

    return TRUE;
}

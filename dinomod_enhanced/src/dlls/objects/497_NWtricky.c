#include "modding.h"
#include "recomputils.h"
#include "recompconfig.h"

#include "PR/os.h"
#include "common.h"
#include "game/gametexts.h"
#include "sys/main.h"
#include "sys/print.h"
#include "dlls/objects/common/sidekick.h"

#include "recomp/dlls/objects/497_NWtricky_recomp.h"

#include "engine/1_cmdmenu.h"

typedef struct {
    u8 state;
    u8 doneDemo;
    u8 demoState;
    f32 timer;
    SidekickStats *sidekickStats;
} NWtricky_Data;

typedef enum {
    STATE_0_Initial,
    STATE_1_Chased_by_SharpClaw,
    STATE_2_Learning_Sidekick_Commands
} NWtricky_State;

typedef enum {
    NWtricky_DEMO_STATE_Initial,
    NWtricky_DEMO_STATE_Show_Inventory,
    NWtricky_DEMO_STATE_Close_Inventory
} NWtricky_Demo_State;

typedef struct {
    ObjSetup base;
    u8 unk18[0x23 - 0x18];
    u8 unk23;
} GroundAnimator_Setup;

// Time for Tricky to call out while being chased by a SharpClaw
#define NWTRICKY_INVERVAL_CALL_FOR_HELP 600.0f

// Time for Tricky to offer hint
#define NWTRICKY_INVERVAL_OFFER_HINT 2000.0f

/** 
  * - Ensure Tricky's sidekick commands are unlocked if his tutorial cutscene is skipped 
  * - Add a null check for Tricky's Toy's GroundAnimator object.
  */
RECOMP_PATCH void NWtricky_control(Object *self) {
    NWtricky_Data *objdata;
    Object *tricky;
    Object *player;
    Object *trickyballGroundAnimator;
    GroundAnimator_Setup *gaSetup;

    objdata = self->data;
    tricky = get_sidekick();
    if (tricky == NULL) {
        return;
    }

    switch (objdata->state) {
    case STATE_0_Initial:
        if (main_get_bits(BIT_SnowHorn_Tutorial_Defeated_SharpClaw)) {
            main_set_bits(BIT_8, 1);
            main_set_bits(BIT_Tricky_Unlocked_Sidekick_Commands, 1);
            objdata->state = STATE_2_Learning_Sidekick_Commands;
        } else if (((DLL_ISidekick*)tricky->dll)->vtbl->func24(tricky) != 0) {
            objdata->state = STATE_1_Chased_by_SharpClaw;
            objdata->timer = 0.0f;
        }
        break;

    case STATE_1_Chased_by_SharpClaw:
        if (main_get_bits(BIT_SnowHorn_Tutorial_Defeated_SharpClaw)) {
            ((DLL_ISidekick*)tricky->dll)->vtbl->func21(tricky, 0, 0);
            gDLL_6_AMSFX->vtbl->stop_object(tricky);
            main_set_bits(BIT_4E3, 0);
            objdata->state = STATE_2_Learning_Sidekick_Commands;
        } else {
            //Call out while being chased
            objdata->timer += gUpdateRateF;
            if (objdata->timer >= NWTRICKY_INVERVAL_CALL_FOR_HELP) {
                objdata->timer -= NWTRICKY_INVERVAL_CALL_FOR_HELP;
                gDLL_6_AMSFX->vtbl->play(tricky, SOUND_222_NW_Tricky_Sharpclaw_Help, MAX_VOLUME, NULL, NULL, 0, NULL);
            }
        }
        break;

    case STATE_2_Learning_Sidekick_Commands:
        //@recomp: make sure Heel and Find are unlocked (allows skipping Tricky's sequence)
        if (!objdata->doneDemo && cmdmenu_is_button_override_active()) {
            main_set_bits(BIT_Tricky_Unlocked_Sidekick_Commands, TRUE);

            //Clear tutorial's cmdmenu button overrides
            gDLL_1_cmdmenu->vtbl->set_buttons_override(CMDMENU_CLEAR_BUTTONS_OVERRIDE);

            objdata->doneDemo = TRUE;
        }

        objdata->timer += gUpdateRateF;
        if (objdata->timer >= NWTRICKY_INVERVAL_OFFER_HINT) {
            if (main_get_bits(BIT_4E3) == 0xFF) {
                objdata->timer = 0.0f;
                if (objdata->sidekickStats->blueFood < 4) {
                    main_set_bits(BIT_4E3, 1);
                } else if (!main_get_bits(BIT_SW_Tricky_Toy_Unearthed) && main_get_bits(BIT_Tricky_Unlocked_Sidekick_Commands)) {
                    player = get_player();

                    //Get GroundAnimator object for the hole containing Tricky's ball
                    trickyballGroundAnimator = func_800211B4(0x1785); //search by uID

                    //@recomp: fix missing null check
                    if (trickyballGroundAnimator) {
                        gaSetup = (GroundAnimator_Setup*)trickyballGroundAnimator->setup;
                    } else {
                        gaSetup = NULL;
                    }

                    //Offer a hint if Tricky and the player stay at the toy's dig spot for a while
                    if (gaSetup && (vec3_distance_squared(
                            &trickyballGroundAnimator->globalPosition,
                            &player->globalPosition) <= SQ(gaSetup->unk23)) && 
                        (vec3_distance_squared(
                            &player->globalPosition, 
                            &tricky->globalPosition) <= 10000.0f)
                    ) {
                        gDLL_6_AMSFX->vtbl->play(tricky, SOUND_4BC_Tricky_Dig_EMPTY, MAX_VOLUME, NULL, NULL, 0, NULL);
                        gDLL_22_Subtitles->vtbl->func_368(GAMETEXT_0BE_SW_Tricky_Tutorial_Hint);
                    }
                }
            }
        }

        //Unlock Tricky's Ball
        if (main_get_bits(BIT_SW_Tricky_Ball_Collected)) {
            main_set_bits(BIT_Tricky_Ball_Unlocked, 1);
        }
        break;
    }
}

/** Allow more controller inputs during tutorial (for inventory's optional D-pad controls/new controls) */
RECOMP_PATCH int NWtricky_anim_callback(Object *self, Object *animObj, AnimObj_Data *animObjData, s8 arg3) {
    NWtricky_Data *objdata;
    Object *tricky;
    s32 i;
    s32 buttonMask;
    /* RECOMP */
    u16 pressedButtons;

    objdata = self->data;
    buttonMask = 0;

    if (!objdata->doneDemo) {
        tricky = get_sidekick();
        ((DLL_ISidekick*)tricky->dll)->vtbl->enable_command(tricky, Sidekick_Command_INDEX_1_Find);
        ((DLL_ISidekick*)tricky->dll)->vtbl->enable_command(tricky, Sidekick_Command_INDEX_2_Distract);
        ((DLL_ISidekick*)tricky->dll)->vtbl->enable_command(tricky, Sidekick_Command_INDEX_3_Guard);
        ((DLL_ISidekick*)tricky->dll)->vtbl->enable_command(tricky, Sidekick_Command_INDEX_4_Flame);
        ((DLL_ISidekick*)tricky->dll)->vtbl->enable_command(tricky, Sidekick_Command_INDEX_5_Play);
        ((DLL_ISidekick*)tricky->dll)->vtbl->enable_command(tricky, Sidekick_Command_INDEX_0_Heel);

        switch (objdata->demoState) {
        case NWtricky_DEMO_STATE_Initial:
            STUBBED_PRINTF("menu start\n");
            for (i = 0; i < animObjData->messageCount; i++) {
                if (animObjData->messages[i] == 3)
                    objdata->demoState = NWtricky_DEMO_STATE_Show_Inventory;
            }
            break;
        case NWtricky_DEMO_STATE_Show_Inventory:
            pressedButtons = joy_get_pressed_raw(0); //@recomp
            STUBBED_PRINTF("menu cbuttons %d\n", pressedButtons);
            for (i = 0; i < animObjData->messageCount; i++) {
                if (animObjData->messages[i] == 4) {
                    objdata->demoState = NWtricky_DEMO_STATE_Close_Inventory;
                    break;
                } else if (animObjData->messages[i] == 1)
                    buttonMask = D_CBUTTONS; // simulate C-Down press
            }

            //@recomp: allow up/down on C-button/D-pad
            buttonMask |= pressedButtons & (U_CBUTTONS | U_JPAD);
            buttonMask |= pressedButtons & (D_CBUTTONS | D_JPAD);
            break;
        case NWtricky_DEMO_STATE_Close_Inventory:
            STUBBED_PRINTF("menu a button\n");
            for (i = 0; i < animObjData->messageCount; i++) {
                if (animObjData->messages[i] == 2)
                    buttonMask = A_BUTTON; // simulate A press
            }

            buttonMask |= joy_get_pressed_raw(0) & A_BUTTON;

            if (buttonMask & A_BUTTON) {
                objdata->doneDemo = TRUE;
            }
            break;
        }
        gDLL_1_cmdmenu->vtbl->set_buttons_override(buttonMask);
    } else {
        gDLL_1_cmdmenu->vtbl->set_buttons_override(CMDMENU_CLEAR_BUTTONS_OVERRIDE);
    }

    return 0;
}

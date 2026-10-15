#include "cheats.h"
#include "custom_gamebits.h"
#include "math_util.h"
#include "modding.h"
#include "recompconfig.h"
#include "recomputils.h"

#include "PR/os.h"
#include "common.h"
#include "core/main.h"
#include "game/gametexts.h"
#include "sys/fonts.h"

// #define DEBUG_CHEATS_MESSAGE

typedef struct {
    GameTextChunk* gametext;
    char* string;
    u16 displayDuration;
    u16 timer;
    s16 opacity;
    u8 state;
} CheatMessage;

typedef enum {
    CheatMessage_STATE_1_FADING_IN = 1,
    CheatMessage_STATE_2_HOLDING = 2,
    CheatMessage_STATE_3_FADING_OUT = 3,
    CheatMessage_STATE_4_FINISHED = 4
} CheatMessage_States;

static CheatMessage rsCheatMessage;

#define MESSAGE_OPACITY_PULSE_DURATION 60 
#define MESSAGE_OPACITY_PULSE_SPEED (M_360_DEGREES / MESSAGE_OPACITY_PULSE_DURATION) 
#define MESSAGE_OPACITY_MAX 0xFF
#define MESSAGE_OPACITY_VARIANCE 70 
#define MESSAGE_OPACITY_PULSE_CENTRE (MESSAGE_OPACITY_MAX - (MESSAGE_OPACITY_VARIANCE>>1))
#define MESSAGE_OPACITY_PULSE_MIN (MESSAGE_OPACITY_PULSE_CENTRE - (MESSAGE_OPACITY_VARIANCE>>1))

/* Sets up a message to display */
static void cheatMessageShow(u8 cheatIdx) {
    CheatMessage* cheat = &rsCheatMessage;

    //Get the cheats' Gametext
    if (cheat->gametext == NULL) {
        cheat->gametext = gDLL_21_Gametext->vtbl->get_chunk(GAMETEXT_1F6_Menu_Cheats);
    }
    if (cheat->gametext == NULL) {
        return;
    }

    //Make sure the cheatIdx is in range
    if (cheatIdx >= cheat->gametext->count) {
        return;
    }
    cheat->string = cheat->gametext->strings[cheatIdx];

    //Set up fonts/timers/etc.
    fontLoad(FONT_FUN_FONT);
    fontLoad(FONT_DINO_MEDIUM_FONT_IN);
    cheat->opacity = 0;
    cheat->timer = 0;
    cheat->state = CheatMessage_STATE_1_FADING_IN;

    //Extra half-phase to ensure opacity's dropping at max speed on entering fade-out state
    cheat->displayDuration = (3 * MESSAGE_OPACITY_PULSE_DURATION) + MESSAGE_OPACITY_PULSE_DURATION/2; 

    //Play sound
    dll_amSfx->Play(NULL, SOUND_19A_Magic_Reverse_Cymbal, MAX_VOLUME, 0, 0, 0, 0);
}

/* Can be used to unlock specified cheats via Gamebits */
static void cheatCheckUnlockConditions(void) {
    typedef struct {
        u8 cheatIdx;
        s16 gamebitUnlock;
    } CheatGamebits;

    //Which gamebits to check, and the cheats they're paired with
    static CheatGamebits rsUnlockables[] = {
        {0, DINOMOD_BIT_930_Unlock_Cheat0},
        {1, DINOMOD_BIT_931_Unlock_Cheat1},
    };

    u32 i;

    //Do nothing if a message is currently being displayed, so multiple unlocks can appear in a row
    if (rsCheatMessage.string != NULL) {
        return;
    }

    //Loop over each unlockable cheat
    for (i = 0; i < ARRAYCOUNT(rsUnlockables); i++) {
        //Ignore if already unlocked
        if (gDLL_29_Gplay->vtbl->is_cheat_unlocked(rsUnlockables[i].cheatIdx)) {
            continue;
        }

        //Otherwise, check if the unlock gamebit is set
        if (mainGetBits(rsUnlockables[i].gamebitUnlock)) {

            //Unlock the cheat, save it, and display a message
            gDLL_29_Gplay->vtbl->unlock_cheat(rsUnlockables[i].cheatIdx);
            gDLL_29_Gplay->vtbl->set_cheat_enabled(rsUnlockables[i].cheatIdx, FALSE);
            gDLL_29_Gplay->vtbl->save_game_options();
            cheatMessageShow(rsUnlockables[i].cheatIdx);

#ifdef DEBUG_CHEATS_MESSAGE
            recomp_printf("UNLOCKED CHEAT %d (%s) via BIT_%x!\n",
                rsUnlockables[i].cheatIdx,
                gDLL_21_Gametext->vtbl->get_text(GAMETEXT_1F6_Menu_Cheats, rsUnlockables[i].cheatIdx),
                rsUnlockables[i].gamebitUnlock
            );
#endif

            // End when a cheat is enabled, so if multiple cheats are unlocked 
            // they'll be queued for after this message finished
            break;
        }
    }
}

/* Checks whether any cheats have been unlocked, and updates the cheat message's State Machine */
void cheatMessageTick(void) {
    CheatMessage* cheat = &rsCheatMessage;

    cheatCheckUnlockConditions();

    //Return early if there's no message to update
    if (cheat->string == NULL) {
        return;
    }

    //Block pausing while the message's being shown
    main_block_pausing(PauseBlock_Next_Tick_Only);

    //State Machine
    switch (cheat->state) {
    case CheatMessage_STATE_1_FADING_IN:
        cheat->opacity += gUpdateRate * 4;
        if (cheat->opacity > MESSAGE_OPACITY_PULSE_CENTRE) {
            cheat->opacity = MESSAGE_OPACITY_PULSE_CENTRE;
            cheat->state = CheatMessage_STATE_2_HOLDING;
        }
        break;
    case CheatMessage_STATE_2_HOLDING:
        cheat->timer += gUpdateRate;
        if (cheat->timer > cheat->displayDuration) {
            cheat->state = CheatMessage_STATE_3_FADING_OUT;
        }    
        cheat->opacity = MESSAGE_OPACITY_PULSE_CENTRE + (s32)(((f32)MESSAGE_OPACITY_VARIANCE/2) * mathSinfInterp(cheat->timer * MESSAGE_OPACITY_PULSE_SPEED));
        break;
    case CheatMessage_STATE_3_FADING_OUT:
        cheat->opacity -= gUpdateRate * 4;
        if (cheat->opacity < 0) {
            cheat->opacity = 0;
            cheat->state = CheatMessage_STATE_4_FINISHED;
        }
        break;
    case CheatMessage_STATE_4_FINISHED:
        cheat->string = NULL;
        cheat->timer = 0;
        cheat->opacity = 0;
        break;
    }
}

/* Draws a message about the unlocked cheat */
void cheatMessagePrint(Gfx** gdl, Mtx** mtxs, Vertex** vtxs) {
    #define HEADING_HEIGHT 17
    #define NAME_HEIGHT 30
    static u8 rsTextColourDim[] = {0xc0, 0x7c, 0x2c};
    static u8 rsTextColourLit[] = {0xFF, 0xD7, 0x3D};
    static char rsCheatHeading[] = "CHEAT UNLOCKED"; //TODO: store in Gametext so it can be translated
    CheatMessage* cheat = &rsCheatMessage;
    s32 dimensions;
    s32 xCoord;
    s32 yCoord;
    f32 tValueOpacity;
    f32 tValueColour;
    u8 textColour[3];
    Gfx* dl;
    
    //Do nothing if there isn't a message, or the message isn't visible
    if ((cheat->opacity == 0) || (cheat->string == NULL)) {
        return;
    }

    dl = *gdl;
    dimensions = viGetCurrentSize();
    
    //Print heading
    {
        yCoord = GET_VIDEO_HEIGHT(dimensions)/2 - (HEADING_HEIGHT + NAME_HEIGHT)/2;
        fontWindowSetCoords(6, 0, 0, GET_VIDEO_WIDTH(dimensions), GET_VIDEO_HEIGHT(dimensions));
        fontWindowUseFont(6, FONT_FUN_FONT);
        fontWindowFlushStrings(6);
        fontWindowSetExtraCharSpacing(6, -0.5f);

        xCoord = GET_VIDEO_WIDTH(dimensions)/2;

        //Get opacity for header
        tValueOpacity = (f32)cheat->opacity/MESSAGE_OPACITY_PULSE_CENTRE;
        if (tValueOpacity > 1.0f) {
            tValueOpacity = 1.0f;
        } else if (tValueOpacity < 0.0f) {
            tValueOpacity = 0.0f;
        }

        //Get tValue for text colour change
        tValueColour = (f32)(cheat->opacity - MESSAGE_OPACITY_PULSE_MIN) / MESSAGE_OPACITY_VARIANCE; //Check opacity's progress in 195-255 range (min/max of sinusoidal opacity)
        tValueColour = (1.0f + tValueColour)/2.0f; //Blend from 50% colourA/B to 100% colourB
        if (tValueColour > 1.0f) {
            tValueColour = 1.0f;
        } else if (tValueColour < 0.0f) {
            tValueColour = 0.0f;
        }
        lerpColoursRGB(tValueColour, rsTextColourDim, rsTextColourLit, textColour);

        //Main text
        fontWindowSetTextColour(6, textColour[0], textColour[1], textColour[2], 0xFF, (f32)tValueOpacity*MESSAGE_OPACITY_MAX);
        fontWindowAddStringXY(6, xCoord, yCoord, rsCheatHeading, 0, ALIGN_TOP_CENTER);
        
        //Drop shadow
        fontWindowSetTextColour(6, 0, 0, 0, 0xFF, cheat->opacity/2);
        fontWindowAddStringXY(6, xCoord - 2, yCoord - 2, rsCheatHeading, 1, ALIGN_TOP_CENTER);

        fontWindowDraw(&dl, mtxs, vtxs, 6);
        yCoord += HEADING_HEIGHT;
    }
    
    //Print cheat name
    {
        fontWindowUseFont(6, FONT_DINO_MEDIUM_FONT_IN);
        fontWindowSetTextColour(6, 0xFF, 0xFF, 0xFF, 0, cheat->opacity);
        fontWindowAddStringXY(6, -0x8000, yCoord, cheat->string, 0, ALIGN_TOP_CENTER);
        fontWindowSetExtraCharSpacing(6, 0);
        fontWindowDraw(&dl, mtxs, vtxs, 6);
        yCoord += NAME_HEIGHT;
    }

    *gdl = dl;
}

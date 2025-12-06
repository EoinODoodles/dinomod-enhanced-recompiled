#include "dlls/objects/210_player.h"
#include "modding.h"
#include "recomputils.h"

#include "common.h"

#include "recomp/dlls/objects/549_GP_ShrinePillar_recomp.h"

typedef struct {
    ObjSetup base;
    s16 gamebitRise;      //plays sequence of pillar rising
    s16 gamebitRaised;    //set when pillar rising sequence has played
    s16 unk1C;            //sequence-related, value of 0x96 on each pillar instance (something to do with door event?)
    s8 sequenceIndex;     //sequence index to play (always index 0 on instances)
    u8 yaw;
    u8 unk20;             //used as arg2 when playing sequence (values for the 3 pillar instances: 0xFF, 0, 1)
    s16 gamebitDoorOpen;  //GP_PillarDoor already open, but seems to be intended for playing Seq 0x39A
} GP_ShrinePillar_Setup;

typedef struct {
    u8 state;
    u8 startSequence;   //used by the control func to invoke sequence, letting animCallback func take over
    f32 cooledTimer;    //how much time until pillar heats up after using Ice Blast Spell
    /** RECOMP EXTENDED */
    Object* door;               //pillar's nearest GP_PillarDoor object
    u8 kyteLandedPlayed;        //handling sound effect when Kyte lands on pillar
    u8 doorOpenPlayed;          //handling sound effect when Kyte opens pillar door
    u8 playShrineRiseSound;     //handling sound effect when shrine rises
    u32 soundHandleSwitch;      //for adjusting pitch
    u32 soundHandleDoor;        //for adjusting pitch
    s16 soundTimerCool;         //debounce timer
    s16 soundTimerHot;          //debounce timer
    s16 soundTimerDeflect;      //debounce timer
    s16 soundDelayStone;        //delay timer
} GP_ShrinePillar_Data;

typedef enum {
    STATE_Underground = 0,               //under sand
    STATE_Rising = 1,                    //emerging from sand
    STATE_Raised = 6,                    //enters this state at end of pillar rise sequence
    STATE_Waiting_for_Door_Open = 7,     //waiting for Kyte to land on pillar
    STATE_Hot = 2,                       //waiting for player to cool pillar with Ice Blast Spell
    STATE_Fade_Texture_to_Cooled = 4,    //blending texture to iced-over stone
    STATE_Cooled = 3,                    //timer counts down until pillar returns to hot state
    STATE_Fade_Texture_to_Hot = 5,       //time ran out, blending back to hot stone
    STATE_Finished = 8                   //unused, but presumably for when Shrine appears (3 pillars cooled)
} GP_ShrinePillar_States;

typedef enum {
    Pillar_Nearest_to_DFPT_Passageway = 0x34e19,
    PillarDoor_Nearest_to_DFPT_Passageway = 0x40f92,
    Pillar_Nearest_to_Chimney_Swipe = 0x34e16,
    PillarDoor_Nearest_to_Chimney_Swipe = 0x40f81,
    Pillar_Nearest_to_Desert_Sand_Arches = 0x34e1a,
    PillarDoor_Nearest_to_Desert_Sand_Arches = 0x40f93,
    HitAnimator_Shrine = 0x406db,
    HitAnimator_Shrine_Sand = 0x406da,
    HitAnimator_Shrine_HITS_Climb = 0xBE0000a, //custom object
    GPSeqObject_Shrine = 0x406cc,
    GPSeqObject_Pillar1 = 0x4059e,
    GPSeqObject_Pillar2 = 0x406b8,
    GPSeqObject_Pillar3 = 0x406b9
} GP_ShrinePillar_uIDs;

extern int GP_ShrinePillar_anim_callback(Object* self, Object* animObj, AnimObj_Data* animObjData, s8 a3);

/** @recomp: variable for keeping track of total pillars cooled */
static u8 GP_ShrinePillar_Counter = 0;

/** How long until an Ice Blasted pillar thaws out */
#define COOL_TIMER 1200.0f

static Object* GP_ShrinePillar_getDoor(Object* pillar){
    Object **objects;
    s32 objIndex;
    s32 objCount;
    s32 i;
    s32 door_uID;

    if (!pillar || !pillar->setup){
        return NULL;
    }

    switch (pillar->setup->uID){
        case Pillar_Nearest_to_DFPT_Passageway:
            door_uID = PillarDoor_Nearest_to_DFPT_Passageway;
            break;
        case Pillar_Nearest_to_Chimney_Swipe:
            door_uID = PillarDoor_Nearest_to_Chimney_Swipe;
            break;
        case Pillar_Nearest_to_Desert_Sand_Arches:
            door_uID = PillarDoor_Nearest_to_Desert_Sand_Arches;
            break;
        default:
            return NULL;
    }

    objects = get_world_objects(&objIndex, &objCount);
    for (i = objIndex; i < objCount; i++) {
        if (!objects[i] || !objects[i]->setup){
            continue;
        }
        if (objects[i]->id == OBJ_GP_PillarDoor && objects[i]->setup->uID == door_uID) {
            return objects[i];
        }
    }

    return NULL;
}

static void play_sound_freezing(Object* self){
    GP_ShrinePillar_Data *objData = self->data;
    if (objData->soundTimerCool == 0){
        objData->soundTimerCool = 120;
        gDLL_6_AMSFX->vtbl->play_sound(self, SOUND_80B_Crackling_Freezing, 0x50, NULL, NULL, 0, NULL);
    }
}

static void play_sound_thawing(Object* self){
    GP_ShrinePillar_Data *objData = self->data;
    Object* player = get_player();
    if (player && objData->soundTimerHot == 0){
        objData->soundTimerHot = 120;
        gDLL_6_AMSFX->vtbl->play_sound(player, SOUND_80C_Steam_Hissing, 0x40, NULL, NULL, 0, NULL);
    }
}

static void play_sound_deflect(Object* self){
    GP_ShrinePillar_Data *objData = self->data;
    if (objData->soundTimerDeflect == 0){
        objData->soundTimerDeflect = 20;
        gDLL_6_AMSFX->vtbl->play_sound(self, rand_next(0, 5) ? SOUND_25B_Magic_Attack_Deflected : SOUND_25C_Melee_Attack_Deflected, 0x40, NULL, NULL, 0, NULL);
    }
}

static void decrement_sound_debounce_timer(s16* timer){
    if (*timer > 0){
        *timer -= gUpdateRate;
    } else if (*timer < 0){
        *timer = 0;
    }
}

RECOMP_PATCH void GP_ShrinePillar_setup(Object *self, GP_ShrinePillar_Setup *setup, s32 arg2) {
    TextureAnimator *animTexture;
    GP_ShrinePillar_Data *objdata;

    objdata = (GP_ShrinePillar_Data*)self->data;

    animTexture = func_800348A0(self, 0, 0);

    //Set initial state based on gamebits
    if (main_get_bits(BIT_7D0)){
        objdata->state = STATE_Finished;
    } else if ((setup->gamebitRaised != NO_GAMEBIT) && main_get_bits(setup->gamebitRaised)) {
        objdata->state = STATE_Raised;
    } else {
        objdata->state = STATE_Underground;
    }

    objdata->startSequence = TRUE;
    self->srt.yaw = setup->yaw << 8;
    self->animCallback = GP_ShrinePillar_anim_callback;

    //Set pillar's animated stone texture to frame 0
    if (animTexture){
        animTexture->frame = 0;
    }

    //@recomp: Check sound-related gamebits
    if (main_get_bits(setup->gamebitDoorOpen)){
        objdata->kyteLandedPlayed = TRUE;
        objdata->doorOpenPlayed = TRUE;
    }

    //@recomp: Get paired door
    objdata->door = GP_ShrinePillar_getDoor(self);
}

RECOMP_PATCH void GP_ShrinePillar_control(Object* self) {
    GP_ShrinePillar_Data* objdata;
    GP_ShrinePillar_Setup* setup;
    s32 seqArg2;
    Block* block;

    objdata = (GP_ShrinePillar_Data*)self->data;
    setup = (GP_ShrinePillar_Setup*)self->setup;

    //@recomp: don't do anything until local Block loaded (stops GP_ShrineDoors from blasting off into outer space due to sequence)
    //TODO: figure out a better fix for this bug?
    block = func_80044BB0(func_8004454C(self->srt.transl.x, self->srt.transl.y + 200, self->srt.transl.z));
    if (!block){
        return;
    }

    diPrintf("control\n");
    if (objdata->startSequence) {
        if ((setup->unk1C != 0) && (objdata->state != STATE_Underground)) {
            seqArg2 = setup->unk20;
            gDLL_3_Animation->vtbl->func20(self, setup->unk1C);
        } else {
            seqArg2 = -1; 
        }

        if (setup->sequenceIndex != -1) {
            gDLL_3_Animation->vtbl->func17(setup->sequenceIndex, self, seqArg2);
        }
        objdata->startSequence = FALSE;
    }
}

/** - Allow Shrine to rise after 3 pillars cooled (originally by MusicalProgrammer)
  * - Stops pillar doors from entering open/close loop after puzzle complete
  * - Adds sounds effects and other polish, like the ability to re-cool a pillar without having to wait for timer to expire 
  */
RECOMP_PATCH int GP_ShrinePillar_anim_callback(Object* self, Object* animObj, AnimObj_Data* animObjData, s8 a3) {
    GP_ShrinePillar_Setup* setup;
    GP_ShrinePillar_Data* objdata;
    TextureAnimator* animatedTexture;
    s32 opacity;
    s32 index;
    s32 pad;
    Object* player;

    objdata = (GP_ShrinePillar_Data*)self->data;
    setup = (GP_ShrinePillar_Setup*)self->setup;

    diPrintf("override %d\n", objdata->state);
    diPrintf("pillars cooled: %d\n", GP_ShrinePillar_Counter);

    //@recomp: Manage sound timers
    decrement_sound_debounce_timer(&objdata->soundTimerCool);
    decrement_sound_debounce_timer(&objdata->soundTimerHot);
    decrement_sound_debounce_timer(&objdata->soundTimerDeflect);

    switch (objdata->state) {
    case STATE_Underground:
        //Waiting for gamebit (set when leaving each act of Desert Force Point Temple)
        if (main_get_bits(setup->gamebitRise)) {
            objdata->state = STATE_Rising;
        }
        break;
    case STATE_Rising:
        //Waiting for pillar rising sequence to call subcommand
        for (index = 0; index < animObjData->unk98; index++) {
            if (animObjData->unk8E[index] == 1) {
                objdata->state = STATE_Raised;
                if (setup->gamebitRaised != -1) {
                    main_set_bits(setup->gamebitRaised, 1);
                }
            }
        }
        break;
    case STATE_Raised:
        //Waiting for door opening gamebit to be set (when Kyte lands on pillar)
        if (main_get_bits(setup->gamebitDoorOpen)) {
            objdata->state = STATE_Waiting_for_Door_Open;

            //@recomp: play sound when Kyte lands, to help indicate something's happened
            if (!objdata->kyteLandedPlayed){
                gDLL_6_AMSFX->vtbl->play_sound(self, SOUND_36E_Lever_Clunk, 0x50, &objdata->soundHandleSwitch, NULL, 0, NULL);
                gDLL_6_AMSFX->vtbl->func_954(objdata->soundHandleSwitch, 0.75f);

                objdata->kyteLandedPlayed = TRUE;
                objdata->soundDelayStone = 50;
            }

            gDLL_3_Animation->vtbl->func20(self, setup->unk1C);
    
        } else if (func_80025F40(self, NULL, NULL, NULL) == 0x19) {
            //@recomp: check for Ice Blast collision while door closed,
            //and play deflect sound to help indicate an interaction exists
            play_sound_deflect(self);
        }
        break;
    case STATE_Waiting_for_Door_Open:
        //@recomp: play sound
        if (!objdata->doorOpenPlayed){
            objdata->soundDelayStone -= gUpdateRate;
            if (objdata->soundDelayStone <= 0){
                objdata->soundDelayStone = 0;
                objdata->doorOpenPlayed = TRUE;
                gDLL_6_AMSFX->vtbl->play_sound(self, SOUND_43E_Stone_Block_Moving, MAX_VOLUME, &objdata->soundHandleDoor, NULL, 0, NULL);
                gDLL_6_AMSFX->vtbl->func_954(objdata->soundHandleDoor, 1.05f);
            }
        }

        if (func_80025F40(self, NULL, NULL, NULL) == 0x19) {
            play_sound_deflect(self);
        }

        //Waiting for door opening sequence to call subcommand
        for (index = 0; index < animObjData->unk98; index++) {
            if (animObjData->unk8E[index] == 2) {
                objdata->state = STATE_Hot;
            }
        }
        break;
    case STATE_Hot:
        //Waiting for Ice Blast Spell to be used on pillar
        if (func_80025F40(self, NULL, NULL, NULL) == 0x19) {
            objdata->state = STATE_Fade_Texture_to_Cooled;

            //@recomp: play freezing sound
            play_sound_freezing(self);
        }
        break;
    case STATE_Cooled:
        //@recomp: re-cool the pillar if Ice Spell used again while in this state
        if (func_80025F40(self, NULL, NULL, NULL) == 0x19) {
            objdata->cooledTimer = COOL_TIMER;

            play_sound_freezing(self);
        }

        //Counting down until returning to hot state
        objdata->cooledTimer -= gUpdateRateF;
        if (objdata->cooledTimer <= 0.0f) {
            GP_ShrinePillar_Counter--; //@recomp: decrement pillar counter
            objdata->state = STATE_Fade_Texture_to_Hot;

            //@recomp: play heating sound
            play_sound_thawing(self);
        }
        break;
    case STATE_Fade_Texture_to_Cooled:
    //Fading animated texture from hot stone frame to iced-over stone frame
        animatedTexture = func_800348A0(self, 0, 0);
        if (animatedTexture != NULL) {
            opacity = animatedTexture->frame + (gUpdateRate * 8);
            if (opacity > 0x100) {
                GP_ShrinePillar_Counter++; //@recomp: increment pillar counter
                opacity = 0x100;
                objdata->state = STATE_Cooled;
                //objdata->cooledTimer = 800.0f;   //original value
                objdata->cooledTimer = COOL_TIMER; //@recomp: longer time window
            }
            animatedTexture->frame = opacity;
        }
        break;
    case STATE_Fade_Texture_to_Hot:
        //Fading texture from hot stone frame to iced-over stone frame
        animatedTexture = func_800348A0(self, 0, 0);
        if (animatedTexture != NULL) {
            opacity = animatedTexture->frame - (gUpdateRate * 2); //@recomp: slower
            if (opacity < 0) {
                opacity = 0;
                objdata->state = STATE_Hot;
            }
            animatedTexture->frame = opacity;
        }

        //@recomp: re-cool the pillar if Ice Spell used again while in this state
        if (func_80025F40(self, NULL, NULL, NULL) == 0x19) {
            objdata->cooledTimer = COOL_TIMER;
            objdata->state = STATE_Fade_Texture_to_Cooled;
            play_sound_freezing(self);
        }
        break;
    case STATE_Finished:
        animatedTexture = func_800348A0(self, 0, 0);
        if (animatedTexture != NULL) {
            animatedTexture->frame = 0;
        }

        /** Destroy door (avoids bug where it goes into an opening/closing loop after puzzle complete) */
        if (objdata->door){
            obj_destroy_object(objdata->door);
            objdata->door = NULL;
        }
        break;
    }

    //@recomp: Play heavy rumbling sound as shrine rises
    if (objdata->playShrineRiseSound){
        objdata->playShrineRiseSound = FALSE;
        player = get_player();
        if (player){
            gDLL_6_AMSFX->vtbl->play_sound(player, SOUND_15_Heavy_Stone_Moving, MAX_VOLUME, NULL, 0, 0, 0);
        }
    }

    //@recomp: uncovering the Shrine when all 3 pillars activated
    if (GP_ShrinePillar_Counter >= 3){
        if (!main_get_bits(BIT_7D0)){
            main_set_bits(BIT_7D0, 1);
            //TO-DO: lock player control (via the sequence file itself), since moving around causes camera issues

            player = get_player();
            if (player){
                gDLL_6_AMSFX->vtbl->play_sound(player, SOUND_B89_Puzzle_Solved, MAX_VOLUME, NULL, 0, 0, 0);
                //Change player FSA state to remove aiming reticle (TODO: remove once player added to sequence)
                gDLL_18_objfsa->vtbl->set_anim_state(player, player->data, 1);
            }
            objdata->playShrineRiseSound = TRUE;
        }

        objdata->state = STATE_Finished;
    }

    if (objdata->state == STATE_Rising ||
        objdata->state == STATE_Waiting_for_Door_Open || 
        objdata->state == STATE_Finished
    ){
        return 0;
    } else {
        return 1;
    }
}

RECOMP_PATCH u32 GP_ShrinePillar_get_data_size(Object *self, u32 a1) {
    return sizeof(GP_ShrinePillar_Data);
}

RECOMP_PATCH void GP_ShrinePillar_free(Object *self, s32 a1) {
    GP_ShrinePillar_Data *objData = self->data;

    //@recomp: free soundHandles
    if (objData->soundHandleSwitch) {
        gDLL_6_AMSFX->vtbl->func_A1C(objData->soundHandleSwitch);
        objData->soundHandleSwitch = 0;
    }

    if (objData->soundHandleDoor) {
        gDLL_6_AMSFX->vtbl->func_A1C(objData->soundHandleDoor);
        objData->soundHandleDoor = 0;
    }
}

#include "math_util.h"
#include "modding.h"
#include "player_util.h"
#include "recompconfig.h"
#include "recomputils.h"

#include "PR/gbi.h"
#include "PR/ultratypes.h"
#include "macros.h"
#include "common.h"
#include "game/objects/object.h"
#include "game/objects/object_id.h"
#include "game/objects/interaction_arrow.h"
#include "game/gamebits.h"
#include "sys/main.h"
#include "sys/math.h"
#include "sys/objanim.h"
#include "sys/objects.h"
#include "sys/objlib.h"
#include "sys/objmsg.h"
#include "sys/print.h"
#include "sys/rand.h"
#include "dll.h"
#include "dlls/engine/6_amsfx.h"
#include "dlls/engine/17_partfx.h"
#include "dlls/objects/503_SHkillermushroom.h"

#include "recomp/dlls/_asm/504_recomp.h"

typedef struct {
    ObjSetup base;
    s16 regrowthDelay;
    s16 sporeLaunchInterval;    //Frames between spore launches (actual interval has some randomised variance added to this value)
    s16 gamebitGrow;            //When specified, plant only grows when gamebit set (and gamebit unset upon exploding)
    s8 sporeAngularRange;       //Variance in launched SHspores' flight yaw
    s8 yaw;
} SHrocketmushroom_Setup;

typedef struct {
    f32 timer;          //Countdown timer for various states (waiting, growing, launching spore)
    f32 scaleMax;       //Scale threshold when growing
    f32 timerDefault;   //Stores a copy of the max timer value, but unused otherwise
    f32 scaleInitial;   //Stores a copy of the initial scale
    f32 scaleSpeed;     //Rate of change of scale
    u8 state;           //State Machine value
    u8 flags;           //Tracks animations finishing, entering a state, and launching spores
    /* RECOMP */
    u8 isPurpleMushroom; //Tracks whether this is a plant or mushroom
    Vec3f attach;        //Coordinates for the mushroom's cap, used when creating particles
} SHrocketmushroom_Data_Extended;

typedef enum {
    STATE_0_Idle,           //Waiting, counting down to launching a spore
    STATE_1_Hidden,         //Hidden after being destroyed, waiting to regrow
    STATE_2_Growing,        //Growing from ground, into idle state afterwards
    STATE_3_Launch_Spore,   //Launching a spore
    STATE_4_Damaged         //Explodes into pieces
} SHrocketmushroom_States;

typedef enum {
    FLAG_None = 0,
    FLAG_Animation_Finished = 1,
    FLAG_State_Entered = 2,
    FLAG_Spore_Launched = 4
} SHrocketmushroom_Flags;

typedef enum {
    ANIMFLAG_Vulnerable = 1,
    ANIMFLAG_Can_Target = 2,
    ANIMFLAG_Hidden = 4,
    ANIMFLAG_Obstructs = 8,
    ANIMFLAG_Exploding = 0x10
} SHrocketmushroom_AnimFlags;

typedef struct {
    s16 modAnimID;
    f32 animSpeed;
    u8 flags;
} SHrocketmushroom_AnimData;

//TODO: replace after decomp update
#define dAnimData data_0 
#define SHrocketmushroom_setup dll_504_setup
#define SHrocketmushroom_control dll_504_control
#define SHrocketmushroom_print dll_504_print
#define SHrocketmushroom_get_data_size dll_504_get_data_size
#define SHrocketmushroom_handle_state_1_hidden dll_504_func_5C8
#define SHrocketmushroom_handle_state_2_growing dll_504_func_70C
#define SHrocketmushroom_handle_state_3_launch_spore dll_504_func_86C
#define SHrocketmushroom_handle_state_4_damaged dll_504_func_970
#define SHrocketmushroom_handle_state_0_idle dll_504_func_A40
#define SHrocketmushroom_set_state dll_504_func_B08
#define SHrocketmushroom_create_spore dll_504_func_B78
#define SHrocketmushroom_explode dll_504_func_CF8
#define SHrocketmushroom_reset dll_504_func_104C
#define SHrocketmushroom_is_player_far_away dll_504_func_114C
#define SOUND_8A1_Spore_Launched 0x8A1
#define SOUND_8A3_Rocket_Mushroom_Grow 0x8A3

/*0x0*/ extern SHrocketmushroom_AnimData dAnimData[5];

extern void SHrocketmushroom_handle_state_1_hidden(Object* self, SHrocketmushroom_AnimData* animData, SHrocketmushroom_Data_Extended* objData);
extern void SHrocketmushroom_handle_state_2_growing(Object* self, SHrocketmushroom_AnimData* animData, SHrocketmushroom_Data_Extended* objData);
extern void SHrocketmushroom_handle_state_3_launch_spore(Object* self, SHrocketmushroom_AnimData* animData, SHrocketmushroom_Data_Extended* objData);
extern void SHrocketmushroom_handle_state_4_damaged(Object* self, SHrocketmushroom_AnimData* animData, SHrocketmushroom_Data_Extended* objData);
extern void SHrocketmushroom_handle_state_0_idle(Object* self, SHrocketmushroom_AnimData* animData, SHrocketmushroom_Data_Extended* objData);
extern void SHrocketmushroom_set_state(SHrocketmushroom_Data_Extended* self, s32 state);
extern void SHrocketmushroom_create_spore(Object* self, SHrocketmushroom_Data_Extended* objData);
extern void SHrocketmushroom_explode(Object* self, SHrocketmushroom_Data_Extended* objData);
extern void SHrocketmushroom_reset(Object* self, SHrocketmushroom_Data_Extended* objData, int startAtZeroScale);
extern int SHrocketmushroom_is_player_far_away(Object* self);

/** Handles switching the plant/mushroom config mid-gameplay */
static void handle_config_change(Object* self) {
    u8 enabled = recomp_get_config_u32("shrocketmushroom_purple");
    SHrocketmushroom_Data_Extended* objData;

    if (enabled && (self->modelInstIdx != 1)) {
        self->modelInstIdx = 1;
        objData = self->data;
        objData->isPurpleMushroom = TRUE;
    } else if (!enabled && (self->modelInstIdx != 0)){
        self->modelInstIdx = 0;
        objData = self->data;
        objData->isPurpleMushroom = FALSE;
    }
}

RECOMP_PATCH void SHrocketmushroom_setup(Object* self, SHrocketmushroom_Setup* objSetup, s32 reset) {
    SHrocketmushroom_Data_Extended* objData = self->data;
    
    self->srt.yaw = objSetup->yaw << 8;
    self->stateFlags |= OBJSTATE_UPDATE_DISABLED;
    // self->stateFlags |= OBJSTATE_PRINT_DISABLED; //@recomp: use print, in order to get attach point for particles
    objData->scaleInitial = self->srt.scale;
    
    if (reset == FALSE) {
        if ((objSetup->gamebitGrow != NO_GAMEBIT) && (main_get_bits(objSetup->gamebitGrow) == FALSE)) {
            //If the plant only grows when a gamebit is set, start out hidden
            SHrocketmushroom_reset(self, objData, TRUE);
            objData->state = STATE_1_Hidden;
        } else {
            //Otherwise start out in idle state
            SHrocketmushroom_reset(self, objData, FALSE);
        }
    }
}

RECOMP_PATCH void SHrocketmushroom_control(Object* self) {
    SHrocketmushroom_Data_Extended* objData;
    SHrocketmushroom_AnimData* animData;
    s32 hitSphereID;
    s32 hitDamage;
    Object* hitBy;
    SRT srt;
    Vec3f offset;
    s32 i;
    MtxF mtx;
    /* RECOMP */
    s16 modAnimID;
    f32 animSpeed;

    handle_config_change(self);

    objData = self->data;
    animData = &dAnimData[objData->state];
    
    switch (objData->state) {
    case STATE_1_Hidden:
        SHrocketmushroom_handle_state_1_hidden(self, animData, objData);
        break;
    case STATE_2_Growing:
        SHrocketmushroom_handle_state_2_growing(self, animData, objData);
        break;
    case STATE_3_Launch_Spore:
        SHrocketmushroom_handle_state_3_launch_spore(self, animData, objData);
        break;
    case STATE_4_Damaged:
        SHrocketmushroom_handle_state_4_damaged(self, animData, objData);
        break;
    case STATE_0_Idle:
    default:
        SHrocketmushroom_handle_state_0_idle(self, animData, objData);
        break;
    }

    //Handle being attacked
    if ((animData->flags & ANIMFLAG_Vulnerable) && 
        func_80025F40(self, &hitBy, &hitSphereID, &hitDamage) && 
        (hitDamage != 0)
    ) {
        gDLL_6_AMSFX->vtbl->play(self, SOUND_744_Mushroom_Hit, MAX_VOLUME, NULL, NULL, 0, NULL);
        SHrocketmushroom_set_state(objData, STATE_4_Damaged);

        //Create big explosion effect
        srt.yaw = self->srt.yaw;
        srt.pitch = self->srt.pitch;
        srt.roll = self->srt.roll;
        srt.transl.x = 0.0f;
        srt.transl.y = 0.0f;
        srt.transl.z = 0.0f;
        srt.scale = 1.0f;
        matrix_from_srt(&mtx, &srt);
        vec3_transform(&mtx, 0.0f, 1.0f, 0.0f, &offset.x, &offset.y, &offset.z);
        srt.transl.x = self->srt.transl.x + (offset.x * 23.0f);
        srt.transl.y = self->srt.transl.y + (offset.y * 23.0f);
        srt.transl.z = self->srt.transl.z + (offset.z * 23.0f);
        srt.scale = 518.0f;
        gDLL_17_partfx->vtbl->spawn(self, PARTICLE_323, &srt, 0x200001, -1, NULL);
        
        //Create lots of small particles
        srt.transl.x -= self->srt.transl.x;
        srt.transl.y -= self->srt.transl.y;
        srt.transl.z -= self->srt.transl.z;
        for (i = 0; i < 20; i++) {
            gDLL_17_partfx->vtbl->spawn(self, PARTICLE_324, &srt, 2, -1, NULL);
        }
        
        //ObjHits
        func_8002683C(self, self->def->hitbox_flagsB6 + 0x50, self->def->unk94 - 0x50, self->def->unk96 + 0x50);
    }
    
    //Handle the animation's flags
    {
        //Apply/remove obstructive collision
        if (animData->flags & ANIMFLAG_Obstructs) {
            func_8002674C(self);
        } else {
            func_800267A4(self);
        }
        
        //Apply/remove damaging collision
        if (animData->flags & ANIMFLAG_Exploding) {
            func_80026128(self, Damage_Type_Sword_Staff_Strike1, 1, 0);
        } else {
            func_80026160(self);
        }
        
        //Allow/disallow targetting
        if (animData->flags & ANIMFLAG_Can_Target) {
            self->unkAF &= ~ARROW_FLAG_8_No_Targetting;
        } else {
            self->unkAF |= ARROW_FLAG_8_No_Targetting;
        }
        
        //Hide/show
        if (animData->flags & ANIMFLAG_Hidden) {
            self->srt.flags |= OBJFLAG_INVISIBLE;
        } else {
            self->srt.flags &= ~OBJFLAG_INVISIBLE;
        }
    }
    
    //@recomp: override modAnimID and animSpeed when this is a Purple Mushroom
    if (objData->isPurpleMushroom){
        switch (objData->state){
            case STATE_3_Launch_Spore:
                modAnimID = SHkillermushroom_MODANIM_7_Spring_Jump;
                animSpeed = 0.006f;
                break;
            default:
                modAnimID = animData->modAnimID;
                animSpeed = animData->animSpeed;
                break;
        }
    } else {
        modAnimID = animData->modAnimID;
        animSpeed = animData->animSpeed;
    }

    //Change modAnim when needed
    if (self->curModAnimId != modAnimID) {
        func_80023D30(self, modAnimID, 0.0f, 0);
    }
    
    //Track when the current animation is finished
    if (func_80024108(self, animSpeed, gUpdateRateF, NULL) != 0) {
        objData->flags |= FLAG_Animation_Finished;
    } else {
        objData->flags &= ~FLAG_Animation_Finished;
    }
}

RECOMP_PATCH void SHrocketmushroom_handle_state_3_launch_spore(Object* self, SHrocketmushroom_AnimData* animData, SHrocketmushroom_Data_Extended* objData) {
    SRT fxTransform;
    s32 i;
    
    //Clear launch flag on entering state
    if (objData->flags & FLAG_State_Entered) {
        //@recomp: play sound when squashing down
        if (objData->isPurpleMushroom) {
            gDLL_6_AMSFX->vtbl->play(self, SOUND_8A3_Rocket_Mushroom_Grow, MAX_VOLUME, NULL, NULL, 0, NULL);
        }
        objData->flags &= ~(FLAG_Spore_Launched | FLAG_State_Entered);
    }
    
    //Launch a spore
    if (
        ((!objData->isPurpleMushroom && (self->animProgress > 0.5f)) || 
         (objData->isPurpleMushroom && (self->animProgress > 0.96f)) //@recomp: different timing for Purple Mushroom
        )
        && !(objData->flags & FLAG_Spore_Launched)
    ) {
        gDLL_6_AMSFX->vtbl->play(self, SOUND_8A1_Spore_Launched, MAX_VOLUME, NULL, NULL, 0, NULL);
        SHrocketmushroom_create_spore(self, objData);
        objData->flags |= FLAG_Spore_Launched;

        //@recomp: create spore particles (harmless ones)
        if (objData->isPurpleMushroom) {
            fxTransform.transl.x = objData->attach.x;
            fxTransform.transl.y = objData->attach.y - 30;
            fxTransform.transl.z = objData->attach.z;

            for (i = 0; i < 5; i++) {
                gDLL_17_partfx->vtbl->spawn(self, PARTICLE_3F3, &fxTransform, PARTFXFLAG_200000 | PARTFXFLAG_1, -1, NULL);
            }
        }
    }
    
    if (objData->flags & FLAG_Animation_Finished) {
        SHrocketmushroom_set_state(objData, STATE_0_Idle);
    }
}

RECOMP_PATCH void SHrocketmushroom_print(Object *self, Gfx **gdl, Mtx **mtxs, Vertex **vtxs, Triangle **pols, s8 visibility) { 
    SHrocketmushroom_Data_Extended* objData = self->data;

    if (visibility) {
        draw_object(self, gdl, mtxs, vtxs, pols, 1.0f);

        //Store attach point coords (for particle effects)
        if (objData->isPurpleMushroom && 
            (objData->state == STATE_3_Launch_Spore) &&
            !(objData->flags & FLAG_Spore_Launched)
        ) {
            func_80031F6C(self, 0,
                &objData->attach.x,
                &objData->attach.y,
                &objData->attach.z,
                0
            );
        }
    }
}

RECOMP_PATCH u32 SHrocketmushroom_get_data_size(Object *self, u32 offsetAddr) {
    return sizeof(SHrocketmushroom_Data_Extended);
}

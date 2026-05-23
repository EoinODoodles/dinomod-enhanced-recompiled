#include "dbgui.h"

#include "math_util.h"
#include "modding.h"
#include "player_util.h"
#include "recompconfig.h"
#include "recomputils.h"

#include "common.h"
#include "macros.h"
#include "game/gamebits.h"
#include "game/objects/object.h"
#include "game/objects/interaction_arrow.h"
#include "game/objects/object_id.h"
#include "string_util.h"
#include "sys/main.h"
#include "sys/math.h"
#include "sys/objects.h"
#include "sys/objmsg.h"
#include "sys/objtype.h"
#include "sys/print.h"
#include "dll.h"
#include "dlls/engine/6_amsfx.h"
#include "dlls/objects/210_player.h"
#include "dlls/objects/418_DFriverflow.h"

#include "recomp/dlls/objects/280_SidekickBall_recomp.h"

#define SOUND_161_Toy_Squeak 0x161 //TODO: remove after decomp update

#define MAX_ROLL_TIME 120

// #define DEBUG_TOY

//Coords for the player's hand - retrieved from the player's print function while their model matrices are readable
static Vec3f rsHandCoords;

typedef struct {
    ObjSetup base;
} SidekickToy_Setup;

typedef struct {
    DLL27_Data collision;
    f32 collisionRadius;
    f32 timer;
    f32 interactionTimer;
    u8 state;
    //@recomp
    u8 configFixZoomies;        //Bool: when enabled, allows the ball to roll to a stop on sloped surfaces
    u8 groundTimer;             //Tracks how long the ball's been on the ground
    u8 surfacedFromWater;       //Bool: ball emerged from being underwater
    s8 splashDebounceTimer;     //For splash sounds
    f32 radius;                 //The radius of the ball in worldSpace
    f32 radiusPush;             //The player can push the ball in the water when inside this radius
} SidekickToy_Data_Extended;

RECOMP_PATCH u32 SidekickToy_get_data_size(Object *self, u32 a1) {
    return sizeof(SidekickToy_Data_Extended);
}

/*0x0*/ extern Vec3f data_collisionPoint;

extern void SidekickToy_tick_collision(Object* self, SidekickToy_Data_Extended* objdata);
extern s32 SidekickToy_tick_flight(Object* self);

typedef enum {
    TOY_STATE_0_Carried = 0, //by player or sidekick
    TOY_STATE_1_At_Rest = 1,
    TOY_STATE_2_In_Flight = 2,
    TOY_STATE_3_Collected = 3,
    TOY_STATE_4_Vanish = 4
} SidekickToyStates;

static void handle_config_change(Object* self) {
    u8 zoomies_fix_enabled = recomp_get_config_u32("sidekick_toy_zoomies");
    SidekickToy_Data_Extended* objData = self->data;

    if (!objData) {
        return;
    }

    if (zoomies_fix_enabled && (objData->configFixZoomies == FALSE)) {
        objData->configFixZoomies = TRUE;
    } else if (!zoomies_fix_enabled && (objData->configFixZoomies != FALSE)){
        objData->configFixZoomies = FALSE;
    }
}

RECOMP_PATCH void SidekickToy_setup(Object* self, SidekickToy_Setup* objsetup, s32 arg2) {
    SidekickToy_Data_Extended* objdata = self->data;
    Object* player;
    u8 colliderArg = 5;

    bzero(objdata, sizeof(SidekickToy_Data_Extended));
    player = get_player();

    objdata->state = TOY_STATE_0_Carried;
    objdata->timer = 0.0f;
    objdata->interactionTimer = 0.0f;

    self->srt.flags |= OBJFLAG_INVISIBLE;

    self->stateFlags |= OBJSTATE_PRINT_DISABLED | OBJSTATE_UPDATE_DISABLED;
    if (player) {
        ((DLL_210_Player*)player->dll)->vtbl->func9(player, self);
    }

    self->unkAF |= ARROW_FLAG_8_No_Targetting;
    objdata->collisionRadius = self->objhitInfo->unk52;

    //@recomp: set up flags to check for water as well
    gDLL_27->vtbl->init(&objdata->collision, 
        0, 
        DLL27FLAG_8000000 | DLL27FLAG_40000 | DLL27FLAG_20 | DLL27FLAG_2 | DLL27FLAG_1, 
        DLL27MODE_1);

    //TODO: set up HITS collider too, so the ball bounces off any lines that block the player?
    gDLL_27->vtbl->setup_terrain_collider(&objdata->collision, 1, &data_collisionPoint, &objdata->collisionRadius, &colliderArg);
    func_800267A4(self);
    objdata->collision.mode = 0;
    obj_init_mesg_queue(self, 1);
    main_set_bits(BIT_Tricky_Ball_Unlocked, FALSE);

    //@recomp: Store radii
    objdata->radius = (self->def) ? (38.0f * self->def->scale) : 4.75f;
    objdata->radiusPush = 8.0f*objdata->collisionRadius + 2.0f;
}

/** 
  * Handles DFriverflow objects' influence, causing the ball to get swept away in currents.
  * Handled the same way as Rare's player/log interactions!
  */
static void ball_handle_river_flow(Object* self) {
    Object* flow;
    s32 objCount;
    Object** riverflows;
    f32 strength;
    f32 dx;
    f32 dz;
    f32 distance;
    f32 range;
    f32 pushZ;
    f32 pushX;
    s32 i;
    s32 pushDivisor;

    pushZ = 0.0f;
    pushX = 0.0f;
    riverflows = obj_get_all_of_type(OBJTYPE_Riverflow, &objCount);
    pushDivisor = 0;
    for (i = 0; i < objCount; i++) {
        flow = riverflows[i];
        if (((DFriverflow_Setup*)flow->setup)->flags & 2) {
            pushDivisor = 1;
            distance = flow->srt.transl.f[1] - self->srt.transl.f[1];
            if (200.0f >= distance && distance >= -200.0f) {
                dx = flow->srt.transl.x - self->srt.transl.x;
                dz = flow->srt.transl.z - self->srt.transl.z;
                distance = sqrtf(SQ(dx) + SQ(dz));
                range = (u8)((DFriverflow_Setup*)flow->setup)->range * 1.5f;
                if (distance < range) {
                    strength = (range - distance) / range;
                    strength *= flow->srt.scale * 10.0f;
                    pushX += fsin16_precise(flow->srt.yaw) * strength;
                    pushZ += fcos16_precise(flow->srt.yaw) * strength;
                }
            }
        }
    }

    if (pushDivisor) {
        self->velocity.x -= pushX * 0.05f * gUpdateRateF;
        self->velocity.z -= pushZ * 0.05f * gUpdateRateF;
        self->velocity.x *= 0.99f;
        self->velocity.z *= 0.99f;
        strength = sqrtf(SQ(self->velocity.x) + SQ(self->velocity.z));
        if (strength > 0.85f) {
            strength = 0.85f / strength;
            self->velocity.x *= strength;
            self->velocity.z *= strength;
        }
    }
}

/**
  * Allows the ball to float on water and be pushed back to shore.
  */
static void ball_handle_water_behaviour(Object* self){
    SidekickToy_Data_Extended* objdata;
    Object* player;
    Player_Data* playerData;
    u8 playerIsSwimming;
    f32 playerDistance;     //Distance between player and ball
    Vec3f vPlayerToBall;    //Direction from player to ball
    Vec3f vPlayerPush;      //Direction player is moving in
    s32 playerAngle;
    f32 dotProduct_vPush_vPlayerToBall;
    f32 depth;
    f32 waterX;
    f32 waterZ;
    f32 volumeSpeed;
    s32 volume;
    u32 soundID;
    u8 i;

    objdata = self->data;

    //Only apply water behaviour in relevant states (not when being held/collected/etc.)
    if (!(objdata->state == TOY_STATE_1_At_Rest || objdata->state == TOY_STATE_2_In_Flight)){
        return;
    }

    //Handle splash debounce
    if (objdata->splashDebounceTimer > 0){
        objdata->splashDebounceTimer -= gUpdateRate;
        if (objdata->splashDebounceTimer < 0){
            objdata->splashDebounceTimer = 0;
        }
    }

    depth = objdata->collision.underwaterDist - 1.0f;

    #ifdef DEBUG_TOY
    diPrintf("BALL STATE: %d\n", objdata->state);
    diPrintf("Depth: %d\n", (s32)depth);
    diPrintf("waterY: %d\n", (s32)objdata->collision.waterY);
    diPrintf("waterYList: %d\n", (s32)(objdata->collision.waterYList[0]));
    #endif

    if (depth > 0.0f){
        if (objdata->surfacedFromWater){
            objdata->surfacedFromWater = FALSE;
            self->velocity.x *= 0.9f;
            self->velocity.y *= 0.5f;
            self->velocity.z *= 0.9f;

            //Limit downward speed (important, or ball can stop detecting water)
            if (self->velocity.y < -2.0f){
                self->velocity.y = -2.0f;
            }

            self->srt.roll += self->velocity.y*gUpdateRate;

            if (!objdata->splashDebounceTimer && self->velocity.y < -0.15f){
                //Play splash sound (correlate volume with speed relative to surface)
                volumeSpeed = -self->velocity.y / 5.0f * 255.0f;
                volume = volumeSpeed;

                //Pick soundID
                if (volume > 20){
                    volume *= 2;
                    soundID = SOUND_3D8_Water_Splash;
                } else {
                    volume = ((f32)4.0f*volumeSpeed);
                    soundID = (rand_next(0, 1) ? SOUND_3EC_Water_Wade_Slow_A : SOUND_3ED_Water_Wade_Slow_B);
                }

                if (volume > MAX_VOLUME){
                    volume = MAX_VOLUME;
                }
                if (volume > 0){
                    gDLL_6_AMSFX->vtbl->play(self, soundID, volume, 0, 0, 0, 0);
                }

                //Create splash effects
                for (i = 0; i < 2; i++) {
                    waterX = ((f32) rand_next(-10, 10) / 10.0f) + self->srt.transl.x;
                    waterZ = ((f32) rand_next(-10, 10) / 10.0f) + self->srt.transl.z;
                    gDLL_24_Waterfx->vtbl->spawn_splash(waterX, objdata->collision.waterY, waterZ, 4.0f);
                    gDLL_24_Waterfx->vtbl->spawn_circular_ripple(waterX, objdata->collision.waterY, waterZ, 0, 0.0f, 3);
                }

                objdata->splashDebounceTimer = 30;
            }
        }

        //Get swept away by OBJ_DFriverflow objects (similar to how it works for player)
        ball_handle_river_flow(self);

        self->velocity.y += 0.25f * gUpdateRateF;
        if (self->velocity.y > 1.4f){
            self->velocity.y = 1.4f;
        }
    } else if (!objdata->surfacedFromWater){
        objdata->surfacedFromWater = TRUE;

        //Lose some momentum
        self->velocity.y *= 0.75f;
        self->velocity.x *= 0.95f;
        self->velocity.z *= 0.95f;
    }

    //Handle player interactions
    player = get_player();
    if (!player){
        return;
    }

    //Check if player swimming
    playerData = player->data;
    if (playerData->unk0.animState == 33 ||
        playerData->unk0.animState == 32 ||
        playerData->unk0.animState == 31){
        playerIsSwimming = TRUE;
    } else {
        playerIsSwimming = FALSE;
    }

    #ifdef DEBUG_TOY
    diPrintf("Ball velocityY: %s\n", f2s(self->velocity.y));
    #endif

    //Don't allow arrow interaction when ball and player are both in the water
    if (playerIsSwimming){
        if (!(self->unkAF & ARROW_FLAG_8_No_Targetting)){
            self->unkAF |= ARROW_FLAG_8_No_Targetting;
        }
    } else if (self->unkAF & ARROW_FLAG_8_No_Targetting) {
        //Make sure ball can be collected when at rest, 
        //or when surfaced from water (and the initial interaction lock timer has expired)
        if (objdata->state == TOY_STATE_1_At_Rest || 
           (objdata->surfacedFromWater && objdata->interactionTimer <= 0.0f)){
            self->unkAF &= ~ARROW_FLAG_8_No_Targetting;
        }
    }

    //Get player-to-ball vector
    VECTOR_SUBTRACT(self->globalPosition, player->globalPosition, vPlayerToBall)
    playerDistance = VECTOR_MAGNITUDE(vPlayerToBall);

    //Store y difference
    f32 dy = vPlayerToBall.y;

    //Destroy the ball if it goes too far away from player
    if (playerDistance > 7000.0f){
        if (playerData->unk708 == self){
            playerData->unk708 = NULL;
        }
        obj_destroy_object(self);
    } else if (playerIsSwimming){
        //Let player push the ball when it's nearby in the water
        if (playerDistance > objdata->radiusPush){
            return;
        }

        //Get player push unit vector, based on their real yaw (vec is multiplied by player's speed later)
        playerAngle = player->parent ? player->parent->srt.yaw + player->srt.yaw : player->srt.yaw;
        rotate_point_by_angle_2D(
            0.0f, -1.0f, 
            &vPlayerPush.x, &vPlayerPush.z,
            playerAngle);

        //Normalise player-to-ball vector
        if (playerDistance > 0.0f){
            playerDistance = 1.0f/playerDistance; //convert to divisor
        }
        vPlayerToBall.x *= playerDistance;
        vPlayerToBall.y *= playerDistance;
        vPlayerToBall.z *= playerDistance;

        //Get dot product between player push direction and player-to-ball direction
        dotProduct_vPush_vPlayerToBall = DOT_PRODUCT(vPlayerPush, vPlayerToBall);
        if (dotProduct_vPush_vPlayerToBall < 0.0f){
            dotProduct_vPush_vPlayerToBall = 0.0f;
        }

        //Push the ball (factor in dot product to increase push strength as player pushes towards ball)
        dotProduct_vPush_vPlayerToBall *= 0.05f*playerData->unk0.speed*gUpdateRateF;
        self->velocity.x += vPlayerPush.x*dotProduct_vPush_vPlayerToBall;
        self->velocity.z += vPlayerPush.z*dotProduct_vPush_vPlayerToBall;
    } else if (!(self->unkAF & ARROW_FLAG_8_No_Targetting) && (-30.0f > dy || dy > 50.0f)){
        // Don't allow ball to be caught if too high above/below
        self->unkAF |= ARROW_FLAG_8_No_Targetting;
    }
}

/** Increase the ball's interaction radii (so it's easier to catch in motion), or reset back to usual grounded radii */
static void ball_update_interaction_distances(Object* self, SidekickToy_Data_Extended* objdata, _Bool increase) {
    if (increase) {
        //Increase the ball's LockIcon radii/angles while it's in motion, so it's easier to catch out of the air
        if (self->unk78 && self->def && self->def->lockdata) {
            //Only HL when ball can be caught, so you can react to the LockIcon showing up
            self->unk78->interactRadius =   2.5*self->def->lockdata->interactRadius;
            self->unk78->hlRadius       =   2.5*self->def->lockdata->interactRadius;
            self->unk78->lockExitRadius =   2.5*self->def->lockdata->lockExitRadius;

            //Can be facing away from ball and still catch it
            self->unk78->hlAngularRange = 0xB4;
        }
    } else {
        //Back to grounded values (still slightly increased from defaults, for convenience)
        if (self->unk78 && self->def && self->def->lockdata) {
            self->unk78->interactRadius =   1.5*self->def->lockdata->interactRadius;
            self->unk78->hlRadius       =   1.5*self->def->lockdata->hlRadius;
            self->unk78->lockExitRadius =   1.5*self->def->lockdata->lockExitRadius;

            self->unk78->hlAngularRange = self->def->lockdata->hlAngularRange;
        }
    }
}

RECOMP_PATCH void SidekickToy_control(Object* self) {
    u32 outMessage;
    SidekickToy_Data_Extended* objdata;
    /* RECOMP */
    Object* player;
    Player_Data* playerData;
    
    objdata = self->data;

    //@recomp: handle config changes
    handle_config_change(self);

    if (objdata->interactionTimer != 0.0f) {
        objdata->interactionTimer -= gUpdateRateF;
        if (objdata->interactionTimer <= 0.0f) {
            objdata->interactionTimer = 0.0f;
            self->unkAF &= ~ARROW_FLAG_8_No_Targetting;
            func_8002674C(self);
        }

        // @recomp: enable collision immediately instead of after a delay, to help avoid being thrown through walls
        // (Can still happen if the player throws the ball while pressed up against a wall, though! TODO: add a fix for this?)
        if (!objdata->collision.mode){
            objdata->collision.mode = 1;
        }
    }

    //@recomp: Track how long the ball's been on the ground for
    if (objdata->state == TOY_STATE_2_In_Flight) {
        if (objdata->collision.unk25D 
            || (objdata->configFixZoomies && (self->srt.transl.y - objdata->collision.floorY < 5.0f))
        ) {
            if (objdata->groundTimer < MAX_ROLL_TIME + 20) {
                objdata->groundTimer += gUpdateRate*2;
            } else {
                objdata->groundTimer = MAX_ROLL_TIME + 20;
            }
        } else {
            if (objdata->groundTimer > gUpdateRate) {
                objdata->groundTimer -= gUpdateRate;
            } else {
                objdata->groundTimer = 0;
            }
        }
    } else {
        objdata->groundTimer = 0;
    }

    while (obj_recv_mesg(self, &outMessage, 0, 0)){
        if (outMessage == 0x7000B) {
            objdata->timer = 0.0f;
            objdata->state = TOY_STATE_4_Vanish;
        }
    }
    
    //@recomp: float on water
    ball_handle_water_behaviour(self);

    switch (objdata->state) {
    case TOY_STATE_2_In_Flight:
        objdata->timer += gUpdateRateF;
        if (objdata->timer >= 6000.0f) {
            objdata->timer = 0.0f;
            objdata->state = TOY_STATE_4_Vanish;
            return;
        }
        objdata->state = SidekickToy_tick_flight(self);

        //@recomp: allow ball to be caught while in flight
        if (self->unkAF & ARROW_FLAG_1_Interacted) {
            gDLL_3_Animation->vtbl->set_variable_obj(OBJ_SidekickBallAni, 0, 0);
            obj_send_mesg(get_player(), 0x7000A, self, (void*)BIT_ALWAYS_1);
            objdata->state = TOY_STATE_3_Collected;
        }
        return;
    case TOY_STATE_1_At_Rest:
        objdata->timer += gUpdateRateF;
        if (objdata->timer >= 3000.0f) {
            objdata->timer = 0.0f;
            objdata->state = TOY_STATE_4_Vanish;
            return;
        }

        if (self->unkAF & ARROW_FLAG_1_Interacted) {
            gDLL_3_Animation->vtbl->set_variable_obj(OBJ_SidekickBallAni, 0, 0);
            obj_send_mesg(get_player(), 0x7000A, self, (void*)BIT_ALWAYS_1);
            objdata->state = TOY_STATE_3_Collected;
        }
        break;
    case TOY_STATE_0_Carried:
        //@recomp: position the ball in player's hands
        if ((player = get_player()) && (playerData = player->data)) {
            if (self == playerData->unk708) {
                player_get_hand_coords(&rsHandCoords);
                self->srt.transl = rsHandCoords;
            }
        }
        break;
    case TOY_STATE_4_Vanish:
        objdata->timer += gUpdateRateF;
        if (objdata->timer >= 60.0f) {
            obj_destroy_object(self);
        } else {
            self->opacity = OBJECT_OPACITY_MAX - (s32) ((objdata->timer * 255.0f) / 60.0f);
            break;
        }
        return;
    case TOY_STATE_3_Collected:
        break;
    }

    SidekickToy_tick_collision(self, objdata);
}

/** Ensures only one instance of the ball exists at a time, fading out duplicates. */
static void ball_remove_duplicates(Object* player){
    Object** objects;
    Object* ball;
    s32 i;
    s32 total;
    SidekickToy_Data_Extended* objData;
    Player_Data* playerData;

    if (!player){
        return;
    }

    objects = get_world_objects(&i, &total);

    playerData = player->data;
    for (i = 0; i < total; i++) {
        ball = objects[i];
        if (ball->id != OBJ_SidekickBall || 
            ball == playerData->unk708 || //skip if it's held by the player
            !ball->data){
            continue;
        }

        objData = ball->data;
        objData->state = TOY_STATE_4_Vanish;
        objData->timer = 0.0f;
    }
}

/** Fix bug where ball gets thrown from sidekick's location instead of from player's hands */
RECOMP_PATCH void SidekickToy_throw(Object* self, Object* thrownBy, f32 speedX, f32 speedY, f32 speedZ) {
    SidekickToy_Data_Extended* objdata = self->data;

    objdata->state = TOY_STATE_2_In_Flight;
    objdata->timer = 0.0f;
    self->velocity.x = speedX;
    self->velocity.y = speedY;
    self->velocity.z = speedZ;
    self->srt.flags = 0;
    objdata->interactionTimer = 20.0f; //@recomp: allow catching a little earlier

    /* @recomp: if the ball was thrown by the player (not by Tricky)
       start throw trajectory from the worldSpace coords of the player's hands */
    Object* player = get_player();
    if (player && (thrownBy == player)){
        ball_remove_duplicates(player);
    
        player_get_hand_coords(&rsHandCoords);
        self->srt.transl = rsHandCoords;
    }

    //@recomp: increase the ball's interaction radii
    ball_update_interaction_distances(self, objdata, TRUE);
}

RECOMP_PATCH s32 SidekickToy_tick_flight(Object* self) {
    Vec3f velocity;
    Vec3f vNormal;
    f32 speed;
    f32 dotProductDouble;
    f32 volume;
    SidekickToy_Data_Extended* objdata;
    /* RECOMP */
    f32 tRoll;

    objdata = self->data;

    if (self->srt.transl.y > objdata->collision.floorY) {
        self->velocity.y -= 0.05f * gUpdateRateF;
    }
    obj_move(self,
        self->velocity.x * gUpdateRateF,
        self->velocity.y * gUpdateRateF,
        self->velocity.z * gUpdateRateF
    );

    if (self->srt.transl.y < objdata->collision.floorY) {
        self->srt.transl.y = objdata->collision.floorY;
    }

    SidekickToy_tick_collision(self, objdata);

    //Handle bouncing off terrain
    if (objdata->collision.unk25D) {
        velocity.f[0] = -self->velocity.x;
        velocity.f[1] = -self->velocity.y;
        velocity.f[2] = -self->velocity.z;
        speed = VECTOR_MAGNITUDE(velocity);

        //Play a squeak sound, correlating speed with volume
        volume = speed;
        if (volume > 0.75f) { //@recomp: don't squeak on softer impacts
            if (volume > 2.0f) {
                volume = 2.0f;
            }
            gDLL_6_AMSFX->vtbl->play(self, SOUND_161_Toy_Squeak, volume * 32.0f, 0, 0, 0, 0);
        }

        //Get velocity unit vector
        if (speed != 0.0f) {
            velocity.f[0] *= 1.0f / speed;
            velocity.f[1] *= 1.0f / speed;
            velocity.f[2] *= 1.0f / speed;
        }

        //Get surface normal
        vNormal.f[0] = objdata->collision.unk68.unk0[0].x;
        vNormal.f[1] = objdata->collision.unk68.unk0[0].y;
        vNormal.f[2] = objdata->collision.unk68.unk0[0].z;

        //Reflect velocity vector off surface normal
        dotProductDouble = 2.0f * DOT_PRODUCT(velocity, vNormal);
        self->velocity.x = vNormal.x*dotProductDouble;
        self->velocity.y = vNormal.y*dotProductDouble;
        self->velocity.z = vNormal.z*dotProductDouble;
        VECTOR_SUBTRACT(self->velocity, velocity, self->velocity)

        //Lose some momentum on each bounce
        self->velocity.x *= speed * 0.7f;
        self->velocity.y *= speed * 0.70f; //NOTE: compiles as separate rodata
        self->velocity.z *= speed * 0.7f;

        //@recomp: Limit how long ball can roll for
        if (objdata->configFixZoomies) {
            tRoll = (MAX_ROLL_TIME - (f32)objdata->groundTimer)/(f32)MAX_ROLL_TIME;
            if (tRoll > 1) {
                tRoll = 1;
            }
            tRoll = SQ(tRoll);
            self->velocity.x *= tRoll;
            self->velocity.y *= tRoll;
            self->velocity.z *= tRoll;
        }

        speed = VECTOR_MAGNITUDE(self->velocity);

        //Handle when nearly at rest
        if (objdata->configFixZoomies) {
            if (speed < 0.02f) { //@recomp: stop less suddenly
                //@recomp: allow ball to rest on sloped ground
                if (vNormal.f[1] > 0.3f) {
                    //@recomp: restore the ball's interaction radii
                    ball_update_interaction_distances(self, objdata, FALSE);

                    return TOY_STATE_1_At_Rest;
                }
            }
        } else {
            if (speed < 0.3f) {
                //Come to a stop if the ground is mostly flat
                if (gDLL_25->vtbl->func_12FC(self->srt.transl.f)) {
                    //@recomp: restore the ball's interaction radii
                    ball_update_interaction_distances(self, objdata, FALSE);

                    return TOY_STATE_1_At_Rest;
                }

                //Otherwise, gain a burst of momentum
                self->velocity.x *= 10.0f;
                self->velocity.y *= 10.0f;
                self->velocity.z *= 10.0f;
            }
        }
    }

    //@recomp: calculate tValue from 0 up to max roll duration
    tRoll = ((f32)objdata->groundTimer)/((f32)MAX_ROLL_TIME/4);
    if (tRoll > 1) {
        tRoll = 1;
    }

    //@recomp: different roll speeds on the ground and in the air
    if (objdata->groundTimer >= 4) {
        if (objdata->collision.floorDist > 10) {
            objdata->groundTimer = 0;
        }

        self->srt.pitch += self->velocity.x * 4000.0f * gUpdateRate;
        self->srt.roll  -= self->velocity.z * 4000.0f * gUpdateRate;

    } else {
        self->srt.pitch += self->velocity.x * 1000.0f * gUpdateRate;
        self->srt.roll  -= self->velocity.z * 1000.0f * gUpdateRate;
    }

    return TOY_STATE_2_In_Flight;
}

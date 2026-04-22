#include "modding.h"
#include "recomputils.h"
#include "object_util.h"

#include "common.h"
#include "sys/map_enums.h"
#include "sys/objmsg.h"
#include "sys/segment_1050.h"
#include "dlls/objects/210_player.h"

extern MapHeader* gMapActiveStreamMap;

#include "recomp/dlls/objects/331_genprops_recomp.h"

typedef struct {
    ObjSetup base;
    s8 yaw;
    s8 modelInstanceIdx; //modelInstanceIdx (OBJ_DFP_blockwall)
    s16 pitch;           //usually pitch, but some objects (vines etc.) use as debug print distance, or to toggle sounds (DIM2IceFloe2)
    s16 roll;            //usually roll, but used for minCooldown on DIM2IceFloe2
    s16 gamebitA;        //gamebitID (NOTE: can be stored on opposite gamebit slot in GenProps_Data)
    s16 gamebitB;        //gamebitID (NOTE: can be stored on opposite gamebit slot in GenProps_Data)
} GenProps_Setup;

typedef struct {
    s32 unk0;               //unused
    f32 pStartX;            //initial position for WMplatform
    f32 pStartY;            //initial position for WMplatform
    f32 pStartZ;            //initial position for WMplatform
    f32 tValue;             //used by WMPlatform for lerping position
    f32 pitch;              //used by WMrock (pitch), SB_Galleon (?)
    f32 roll;               //used by WMrock (roll), SB_Galleon (?)
    f32 unk1C;              //used by SB_Galleon, Krazcol, SB_Lamp
    f32 unk20;              //used by SB_Galleon, Krazcol
    f32 unk24;              //used by SB_Galleon, Krazcol, SB_Lamp
    f32 unk28;              //used by SB_Galleon, Krazcol
    f32 debugPrintDistance; //usually radius for debug prints, but WMrock uses it differently 
    s16 unk30;              //unused
    s16 lampZero;           //used by SB_Lamp
    s16 lampRandom;         //used by SB_Lamp (randomised value)
    s16 speed;              //usually speed, but WMrock seems to use it as a timer
    s16 gamebitA;           //used by many objects
    s16 gamebitB;           //used by many objects
    s16 timer;              //used by WM_Platform, DIM2IceFloe2
    u8 unk3E;               //used by WMrock, SB_Galleon, Krazcol (sound/debug related?)
    u8 lampBool;            //used by SB_Lamp
    u8 lastSoundIndex;      //used by DIM2IceFloe2, last index into soundID data (_data_4)
    s8 vineHealth;          //used by NoPassVines
    s16 unk42;              //unused
    u16* soundIDs;          //used by DIM2IceFloe2
    u16 minCooldown;        //used by DIM2IceFloe2, affecting time between sounds
    s16 unk4A;              //unused
    /** RECOMP EXTENDED */
    s8 state;
    s8 steppedOffSinceLastMove;         //tracks whether the player has stepped off after lift stops
    s8 muteSounds;                      //shooshes the platform (while player sequence is playing, e.g. crystal transformation)
    s8 isKrystalsPlatform;              //tracks which side of WM the lift is on (Krystal vs. Sabre's sides)
    s16 pStartYaw;
    f32 pEndX;
    f32 pEndY;
    f32 pEndZ;
    s16 pEndYaw;
    f32 dX;
    f32 dY;
    f32 dZ;
    s16 dYaw;
    f32 midpointY;         //halfway between top and bottom positions
    u32 soundHandleHum;    //manages hum sound
    f32 oscillateTimer;    //manages impact vibration effect
    Object* crystalSwitch; //for toggling player collision on crystal switch just below platform's upper goal
} GenProps_Data_Extended;

typedef enum {
    Platform_State_Stopped_Bottom = 0,
    Platform_State_Moving_Up = 1,
    Platform_State_Moving_Down = 2,
    Platform_State_Stopped_Top = 3,
    Platform_State_Locked_Bottom = 4,
    Platform_State_Locked_Top = 5
} WMPlatform_States;

extern Object* _data_0;
extern u16 _data_4[];

extern s32 dll_331_func_1D34(Object* self, Object* animObj, AnimObj_Data* animObjData, s32 arg3);

#define PLATFORM_DEBUG FALSE

//WMPlatform params         //patched values    //unpatched values        
#define platformStartX      13219.537           //34022.0f
#define platformStartY      -34.524             //457.0f
#define platformStartZ      1337.1449           //22523.0f

#define platformKrystalEndX 13527               //14807.0f
#define platformKrystalEndY 457                 //457.0f
#define platformKrystalEndZ 2025                //3305.0f

#define platformSabreEndX   13341                //14621.0f
#define platformSabreEndY   457                  //457.0f
#define platformSabreEndZ   1175                 //2453.0f

#define platformStartYaw    8192.0f //(unpatched value)

#define platformWidth 32        //really 30, but allowing some padding
#define platformCooldown 120    //waiting time until lift starts moving

#define oscillateDuration 30.0f
#define oscillateAmplitude 0.4f
#define oscillateFrequency 7000

static void rotatePointByAngle2D(f32 x, f32 y, f32* ox, f32* oy, s16 theta){
    f32 sinTheta = fsin16_precise(-theta);
    f32 cosTheta = fcos16_precise(-theta);

    *ox = x*cosTheta - y*sinTheta;
    *oy = x*sinTheta + y*cosTheta;
}

static f32 easeInOutSine(f32 x) {
    return -(fcos16_precise(0x8000 * x) - 1) / 2;
}

/** Checks if the player is within a short cuboid encompassing the surface of the platform */
static int isPlayerOnPlatform(Vec3f* localCoords){
    if (
        (-platformWidth < localCoords->x) && (localCoords->x < platformWidth) &&
        (-5 < localCoords->y) && (localCoords->y < 5) &&
        (-platformWidth < localCoords->z) && (localCoords->z < platformWidth)
    ){
        return TRUE;
    }
    return FALSE;
}

/** Checks if the player is dangling within a cuboid sliver encompassing the ledge at the platform's upper goal */
static int isPlayerDanglingAtUpperGoal(Object* player, s8 isKrystalsPlatform){
    s32 targetXmin;
    s32 targetXmax;
    s32 targetY;
    s32 targetZ;
    Vec3f fCoords;
    s32 sCoords[3];

    if (isKrystalsPlatform){
        targetXmin = 1336;
        targetXmax = 1422;
        targetY = 457;
        targetZ = 2696;
    } else {
        targetXmin = 1138;
        targetXmax = 1224;
        targetY = 457;
        targetZ = 1783;
    }

    //Get player's coords in local mapSpace
    fCoords = player->srt.transl;
    fCoords.x -= gMapActiveStreamMap->originWorldX;
    fCoords.z -= gMapActiveStreamMap->originWorldZ;
    sCoords[0] = fCoords.x;
    sCoords[1] = fCoords.y;
    sCoords[2] = fCoords.z;

    //Check if at dangling position
    if (
        (targetXmin <= sCoords[0] && sCoords[0] <= targetXmax) &&
        (sCoords[1] == targetY) &&
        (sCoords[2] == targetZ)
    ){
        //Check if dangling
        if ((((Player_Data*)player->data)->unk0.flags & 0xA00000)){
            return TRUE;
        }
    }
    return FALSE;
}

//Repurposing these lift gamebits to keep track of the ledge grab HitAnimators 
#define LIFT_NEAR_TOP_GAMEBIT_KRYSTAL BIT_322
#define LIFT_NEAR_TOP_GAMEBIT_SABRE   BIT_369

/** Repurposing one of Rare's lift gamebits so it tracks whether the lift is near/at its upper destination
  * (Gamebit now used to toggle the ledge grab HITS line in the upper tier) 
  */
static void setHitAnimatorGamebitTop(GenProps_Data_Extended* objData){
    u16 gamebit = objData->isKrystalsPlatform ? LIFT_NEAR_TOP_GAMEBIT_KRYSTAL : LIFT_NEAR_TOP_GAMEBIT_SABRE;

    if (objData->tValue > 0.97f){ //small activation margin in case player runs off just before lift arrives
        if (!main_get_bits(gamebit)){
            main_set_bits(gamebit, TRUE);
        }
    } else if (main_get_bits(gamebit)){
        main_set_bits(gamebit, FALSE);
    }
}

/** Calculate linear interpolation between platform's start/end points (and yaw) */
static void calculateLerp(Object* self, f32 t_value, Vec3f* pLerp, s16* yawLerp){
    GenProps_Data_Extended* objData = self->data;

    if (t_value == 0){
        pLerp->x = objData->pStartX;
        pLerp->y = objData->pStartY;
        pLerp->z = objData->pStartZ;
        *yawLerp = objData->pStartYaw;
        return;
    } else if (t_value == 1){
        pLerp->x = objData->pEndX;
        pLerp->y = objData->pEndY;
        pLerp->z = objData->pEndZ;
        *yawLerp = objData->pEndYaw;
        return;
    }

    pLerp->x = objData->pStartX + (t_value * objData->dX);
    pLerp->y = objData->pStartY + (t_value * objData->dY);
    pLerp->z = objData->pStartZ + (t_value * objData->dZ);
    *yawLerp = objData->pStartYaw + (s16)(t_value * objData->dYaw);
}

/** Apply linear interpolation between platform's start/end points (and yaw) */
static void applyLerp(Object* self, f32 t_value){
    GenProps_Data_Extended* objData = self->data;
    Vec3f pLerp;
    s16 yawLerp;

    calculateLerp(self, t_value, &pLerp, &yawLerp);

    objData->tValue = t_value;
    self->srt.transl.x = pLerp.x;
    self->srt.transl.y = pLerp.y;
    self->srt.transl.z = pLerp.z;
    self->srt.yaw = yawLerp;

    setHitAnimatorGamebitTop(objData);
}

static void playSoundHum(Object* self){
    GenProps_Data_Extended* objData = self->data;
    if (objData->muteSounds){
        return;
    }
    if (objData->soundHandleHum == 0) {
        gDLL_6_AMSFX->vtbl->play_sound(self, SOUND_6EC_Mechanical_Hum_Loop, 0x50, &objData->soundHandleHum, NULL, 0, NULL);
    }
}

static void stopSoundHum(Object* self){
    GenProps_Data_Extended* objData = self->data;
    if (objData->soundHandleHum != 0) {
        gDLL_6_AMSFX->vtbl->func_A1C(objData->soundHandleHum);
        objData->soundHandleHum = 0;
    }
}

static void playSoundClunk(Object* self){
    GenProps_Data_Extended* objData = self->data;
    if (objData->muteSounds){
        return;
    }
    gDLL_6_AMSFX->vtbl->play_sound(self, SOUND_B5C_Machinery_Clunk, 0x30, NULL, NULL, 0, NULL);
}

/** Applies a y-axis vibration to the lift (and the player if they're on it) */
static void impactOscillate(Object* self, Object* player, int playerOnPlatform){
    GenProps_Data_Extended* objData = self->data;
    f32 displacement;
    f32 centreY;
    f32 direction;

    objData->oscillateTimer -= gUpdateRate;
    if (objData->oscillateTimer < 0){
        objData->oscillateTimer = 0;
        return;
    }

    centreY = (objData->state == Platform_State_Stopped_Bottom) ? objData->pStartY : objData->pEndY;
    direction = objData->speed > 0 ? 1 : -1;

    displacement = fsin16_precise((oscillateDuration - objData->oscillateTimer)*oscillateFrequency)*(objData->oscillateTimer/oscillateDuration)*oscillateAmplitude*direction;
    self->srt.transl.y = centreY + displacement;
    if (player && playerOnPlatform){
        player->srt.transl.y = centreY + displacement;
    }
}

static void WMPlatform_setup_custom(Object* self, GenProps_Setup* objSetup, s32 arg2){
    GenProps_Data_Extended* objData = self->data;
    Object* player = get_player();
    s8 wmAct;

    self->srt.yaw = platformStartYaw;
    self->srt.pitch = objSetup->pitch;
    objData->gamebitA = objSetup->gamebitA;
    objData->gamebitB = objSetup->gamebitB;
    objData->tValue = 0.0f;
    objData->pStartX = objSetup->base.x;
    objData->pStartY = objSetup->base.y;
    objData->pStartZ = objSetup->base.z;
    objData->pStartYaw = self->srt.yaw;
    objData->timer = platformCooldown;
    objData->oscillateTimer = 0;
    objData->speed = 2;
    objData->steppedOffSinceLastMove = TRUE;
    objData->soundHandleHum = 0;
    self->animCallback = (void*)&dll_331_func_1D34;

    //Store end position
    //(TO-DO: maybe do this differently from Rare's implementation, storing end coords in GenProps_Setup instead to make this more reusable?
    //Could use s16 XYZ offsets from base position, so it's easy to reposition when moving whole map)
    switch (objData->gamebitA){
        default:
        case BIT_319: //Krystal's platform
            objData->pEndX = platformKrystalEndX;
            objData->pEndY = platformKrystalEndY;
            objData->pEndZ = platformKrystalEndZ;
            objData->isKrystalsPlatform = TRUE;
            objData->crystalSwitch = func_800211B4(0x2abb);
            break;
        case BIT_363: //Sabre's platform
            objData->pEndX = platformSabreEndX;
            objData->pEndY = platformSabreEndY;
            objData->pEndZ = platformSabreEndZ;
            objData->isKrystalsPlatform = FALSE;
            objData->crystalSwitch = func_800211B4(0x37);
            break;
    }
    objData->pEndYaw = 0;

    //Calculate lerp deltas
    objData->dX = objData->pEndX - objData->pStartX;
    objData->dY = objData->pEndY - objData->pStartY;
    objData->dZ = objData->pEndZ - objData->pStartZ;
    objData->dYaw = objData->pEndYaw - objData->pStartYaw;

    //Calculate Y midpoint
    objData->midpointY = objData->pStartY + (objData->pEndY - objData->pStartY)/2.0f;

    //Lock the platform if Randorn has yet to activate it later in the game (unused MPEG dialogue hints at this)
    wmAct = gDLL_29_Gplay->vtbl->get_map_setup(MAP_WARLOCK_MOUNTAIN);
    if ((objData->isKrystalsPlatform && wmAct < 6) || (!objData->isKrystalsPlatform && wmAct < 7)){
        objData->state = Platform_State_Locked_Top;
        applyLerp(self, 1);
        return;
    }

    //Lock the platform if it's not intended to be used by the current character (act-specific positioning)
    //(Prevents player from getting to other character's side of Warlock Mountain by crossing summit and using other lift)
    if (player){
        //Krystal (Sabre's platform unlocks on act 7, so keep his one locked to top until then)
        if (player->id == OBJ_Krystal && !objData->isKrystalsPlatform){
            objData->state = (wmAct <= 6) ? Platform_State_Locked_Top : Platform_State_Locked_Bottom;
            applyLerp(self, (objData->state == Platform_State_Locked_Bottom) ? 0 : 1);
            return;
        //Sabre (Krystal's platform unlocks on act 6, so keep her one locked to top until then)
        } else if (player->id == OBJ_Sabre && objData->isKrystalsPlatform){
            objData->state = (wmAct <= 5) ? Platform_State_Locked_Top : Platform_State_Locked_Bottom;
            applyLerp(self, (objData->state == Platform_State_Locked_Bottom) ? 0 : 1);
            return;
        }
    }

    //Set initial "stopped" state based on player height range on object load
    if (player && (player->srt.transl.y > objData->midpointY)){
        objData->state = Platform_State_Stopped_Top;
        applyLerp(self, 1);
    } else {
        objData->state = Platform_State_Stopped_Bottom;
        applyLerp(self, 0);
    }
}

/** A customised revamp of WM_Platform's control code */
static void WMPlatform_control_custom(Object* self){
    GenProps_Data_Extended* objData = self->data;
    s8 state = objData->state;
    Object* player;
    int playerOnPlatform;
    f32 tValue_clamped;
    f32 tValue_eased;
    Vec3f lerpP;
    s16 lerpYaw;
    Vec3f deltaP;
    s16 deltaYaw;
    Vec3f localCoords;
    Vec3f deltaYawCoords;

    player = get_player();
    if (!player){
        return;
    }

    //Calculate player's local coordinates in platform's objectSpace
    localCoords.x = player->srt.transl.x - self->srt.transl.x;
    localCoords.y = player->srt.transl.y - self->srt.transl.y;
    localCoords.z = player->srt.transl.z - self->srt.transl.z;
    rotate_vec_inv(&self->srt, &localCoords);
    playerOnPlatform = isPlayerOnPlatform(&localCoords);

    //If the player is dangling from a drop point, consider them not to be on the platform
    if ((((Player_Data*)player->data)->unk0.flags & 0xA00000)){
        playerOnPlatform = FALSE;
    }

    //Mute platform sounds if the player is in a sequence
    //(Makes sure it shooshes during important sequences like the crystal transformation)
    //TODO: make this check more specific, ignoring minor sequences like using the lantern
    if (player && (player->unkB0 & 0x1000)){
        objData->muteSounds = TRUE;
    } else {
        objData->muteSounds = FALSE;
    }
    if (objData->muteSounds && objData->soundHandleHum){
        stopSoundHum(self);
    }

    //Switch off crystal switch's player collision while player on platform
    if (objData->crystalSwitch){
        if (playerOnPlatform && (objData->crystalSwitch->objhitInfo->unk58 & 1)){
            objData->crystalSwitch->objhitInfo->unk58 &= ~1;
        } else if ((objData->crystalSwitch->objhitInfo->unk58 & 1) == 0){
            objData->crystalSwitch->objhitInfo->unk58 |= 1;
        }
    }

    //Debug print player's coords in objectSpace, and other details
    if (PLATFORM_DEBUG) {
        diPrintf("local coords:\nx: %d\ny: %d\nz: %d\n", (s32)localCoords.x, (s32)localCoords.y, (s32)localCoords.z);
        diPrintf("playerOnPlatform: %d\n", playerOnPlatform);
        diPrintf("timer: %d\n", (s32)objData->timer);
        diPrintf("state: %d\n", state);
        diPrintf("soundHandleHum: %d\n", objData->soundHandleHum);
        diPrintf("t_value: %3d%s\n", (s32)(objData->tValue*100.0f), "%");
        diPrintf("speed: %d\n\n", (s32)objData->speed);
    }

    //State Machine
    switch (state){
    case Platform_State_Stopped_Bottom:
    case Platform_State_Stopped_Top:
        //TODO: return early if a specific activation gamebit hasn't been set yet? (e.g. shooting the crystal in the upper tier?)

        //Auto-follow the player up/down if they ended up in the opposite tier of WM without using the lift
        if ((state == Platform_State_Stopped_Bottom) && (player->srt.transl.y > objData->midpointY)){
            objData->state = Platform_State_Moving_Up;
            playSoundHum(self);
            return;
        } else if ((state == Platform_State_Stopped_Top) && (player->srt.transl.y < objData->midpointY)){
            objData->state = Platform_State_Moving_Down;
            playSoundHum(self);
            return;
        }

        //Track whether the player's stepped off the platform since the last move
        if (!playerOnPlatform){
            objData->steppedOffSinceLastMove = TRUE;
        }

        //Handle timer: player stays on platform until it moves, otherwise the cooldown slowly resets
        if (playerOnPlatform && objData->steppedOffSinceLastMove){
            objData->timer -= gUpdateRate;
            if (objData->timer <= 0) {
                objData->timer = 0;
                objData->state = (state == Platform_State_Stopped_Bottom) ? Platform_State_Moving_Up : Platform_State_Moving_Down;
                playSoundHum(self);
            }
        } else {
            objData->timer += gUpdateRate;
            if (objData->timer > platformCooldown){
                objData->timer = platformCooldown;
            }
        }

        //Animate impact vibration
        if (objData->oscillateTimer > 0){
            impactOscillate(self, player, playerOnPlatform);
        }

        return;
    case Platform_State_Moving_Up:
    case Platform_State_Moving_Down:
        //Make sure lift is moving in the correct direction
        if ((state == Platform_State_Moving_Up && objData->speed < 0) ||
            (state == Platform_State_Moving_Down && objData->speed > 0)
        ){
            objData->speed *= -1;
        }

        objData->steppedOffSinceLastMove = FALSE;

        //If lift's moving up and nearly at upper destination, check if player's dangling from the destination: 
        //If so, reverse direction to prevent player/platform clipping
        if (state == Platform_State_Moving_Up && 
            objData->tValue > 0.93f && 
            isPlayerDanglingAtUpperGoal(player, objData->isKrystalsPlatform)
        ){
            playSoundClunk(self);
            objData->state = Platform_State_Moving_Down;
            return;
        }

        //Linearly interpolate to calculate the platform's updated position
        objData->tValue += 0.001f * objData->speed * gUpdateRateF;
        tValue_clamped = (objData->tValue < 0.0f) ? 0.0f : ((objData->tValue > 1.0f) ? 1.0f : objData->tValue);
        tValue_eased = easeInOutSine(tValue_clamped);
        // recomp_eprintf("t_value: %f\nt_value_eased: %f\n", tValue_clamped, tValue_eased);
        calculateLerp(self, tValue_eased, &lerpP, &lerpYaw);

        //Calculate the position/rotation deltas (for updating player transform)
        deltaP.x = lerpP.x - self->srt.transl.x;
        deltaP.y = lerpP.y - self->srt.transl.y;
        deltaP.z = lerpP.z - self->srt.transl.z;
        deltaYaw = lerpYaw - self->srt.yaw;

        //Check if player is on platform (in local objectSpace) and update their transform
        if (playerOnPlatform){
            //Rotate player's local coords with platform's own rotation delta on this tick
            if (deltaYaw != 0){
                rotatePointByAngle2D(localCoords.x, localCoords.z, &deltaYawCoords.x, &deltaYawCoords.z, deltaYaw); //objectSpace rotation

                //Calculate objectSpace position delta (as a result of applying deltaYaw)
                deltaYawCoords.x -= localCoords.x;
                deltaYawCoords.z -= localCoords.z;
                
                //Transform position delta from objectSpace -> worldSpace
                rotatePointByAngle2D(deltaYawCoords.x, deltaYawCoords.z, &deltaYawCoords.x, &deltaYawCoords.z, -lerpYaw);

                //Add worldSpace delta
                player->srt.transl.x += deltaYawCoords.x;
                player->srt.transl.z += deltaYawCoords.z;
            }

            //Apply trans deltas and rotate delta
            player->srt.transl.x += deltaP.x;
            player->srt.transl.y += deltaP.y;
            player->srt.transl.z += deltaP.z;
            player->srt.yaw += deltaYaw;
            //TO-DO: possibly need -0x8000 to 0x8000 wrap for player yaw update, just to be safe?
        }

        //Apply platform's transform deltas
        self->srt.transl.x = lerpP.x;
        self->srt.transl.y = lerpP.y;
        self->srt.transl.z = lerpP.z;
        self->srt.yaw = lerpYaw;
        setHitAnimatorGamebitTop(objData);

        //Handle end condition
        if (objData->tValue > 1.0f) {
            objData->tValue = 1.0f;
            objData->state = Platform_State_Stopped_Top;
            objData->timer = platformCooldown;
            objData->oscillateTimer = oscillateDuration;
            stopSoundHum(self);
            playSoundClunk(self);
            return;
        } else if (objData->tValue < 0.0f) {
            objData->tValue = 0.0f;
            objData->state = Platform_State_Stopped_Bottom;
            objData->timer = objData->timer = platformCooldown;
            objData->oscillateTimer = oscillateDuration;
            stopSoundHum(self);
            playSoundClunk(self);
            return;
        }
        return;
    case Platform_State_Locked_Bottom:
    case Platform_State_Locked_Top:
        return;
    }
}

RECOMP_PATCH void dll_331_setup(Object* self, GenProps_Setup* objSetup, s32 arg2) {
    s16 id;
    s16 temp;
    GenProps_Data_Extended* objData;

    id = objSetup->base.objId;
    objData = self->data;
    objData->debugPrintDistance = 1.0f;
    
    switch (id) {
    case OBJ_NWbigrock:
        self->srt.pitch = objSetup->pitch;
        self->srt.roll = objSetup->roll;
        self->srt.scale = 1.0f;
        self->animCallback = (void*)&dll_331_func_1D34;
        return;
    case OBJ_WMfallencol:
        self->srt.yaw = objSetup->yaw << 8;
        self->srt.pitch = objSetup->pitch;
        self->srt.roll = objSetup->roll;
        self->srt.scale = 1.0f;
        return;
    case OBJ_DFSHcol:
    case OBJ_MMSHcol:
    case OBJ_ECSHcol:
    case OBJ_GPSHcol:
    case OBJ_DBSHcol:
    case OBJ_WGSHcol:
    case OBJ_DFP_dish:
        self->srt.yaw = objSetup->yaw << 8;
        return;
    case OBJ_WM_Walkway1:
    case OBJ_WM_Walkway2:
        self->srt.yaw = objSetup->yaw << 8;
        self->srt.pitch = objSetup->pitch;
        objData->gamebitA = objSetup->gamebitA;
        objData->gamebitB = objSetup->gamebitB;
        objData->tValue = 0.0f;
        objData->pStartX = objSetup->base.x;
        objData->pStartY = objSetup->base.y;
        objData->pStartZ = objSetup->base.z;
        objData->timer = 500;
        objData->speed = 1;
        self->animCallback = (void*)&dll_331_func_1D34;
        return;
    case OBJ_WM_Platform:
        WMPlatform_setup_custom(self, objSetup, arg2);
        return;
    case OBJ_WM_MoonSeedMoun:
        self->srt.yaw = objSetup->yaw << 8;
        objData->debugPrintDistance = objSetup->pitch;
        return;
    case OBJ_WM_NoPassVine:
        self->srt.yaw = objSetup->yaw << 8;
        objData->debugPrintDistance = objSetup->pitch;
        objData->vineHealth = 2;
        return;
    case OBJ_WM_NoPassHorzVi:
        self->srt.yaw = objSetup->yaw << 8;
        objData->debugPrintDistance = objSetup->pitch;
        objData->vineHealth = 2;
        objData->gamebitA = objSetup->gamebitA;
        return;
    case OBJ_NWSH_col:
        self->srt.yaw = objSetup->yaw << 8;
        self->srt.pitch = objSetup->pitch;
        return;
    case OBJ_NWSH_rock:
    case OBJ_WMrock:
        objData->roll = rand_next(100, 400);
        objData->pitch = rand_next(100, 400);
        self->velocity.x = rand_next(0, 100) * 0.04f;
        self->velocity.z = rand_next(0, 100) * 0.04f;
        self->srt.scale *= 0.3f + (0.01f * rand_next(0, 10));
        objData->speed = 200;
        objData->debugPrintDistance = 0;
        objData->unk3E = 2;
        self->modelInstIdx = rand_next(0, 1);
        return;
    case OBJ_DFP_blockwall:
        self->srt.yaw = objSetup->yaw << 8;
        self->modelInstIdx = objSetup->modelInstanceIdx;
        objData->gamebitA = objSetup->gamebitB;
        if (main_get_bits(objData->gamebitA)) {
            self->srt.transl.y = objSetup->base.y + 30.0f;
            return;
        }
        return;
    case OBJ_WMlargerock:
        self->srt.yaw = objSetup->yaw << 8;
        self->srt.pitch = objSetup->pitch;
        self->srt.roll = objSetup->roll;
        self->srt.scale = 1.0f;
        self->animCallback = (void*)&dll_331_func_1D34;
        objData->gamebitB = objSetup->gamebitB;
        objData->gamebitA = objSetup->gamebitA;
        return;
    case 133: //unknown deleted object
        self->srt.yaw = objSetup->yaw << 8;
        self->srt.pitch = objSetup->pitch;
        self->srt.roll = objSetup->roll;
        self->srt.scale = 0.05f;
        self->unkDC = 100;
        self->unkE0 = 0;
        self->animCallback = (void*)&dll_331_func_1D34;
        return;
    case 134: //unknown deleted object
        self->srt.yaw = objSetup->yaw << 8;
        self->srt.pitch = objSetup->pitch;
        self->srt.roll = objSetup->roll;
        self->srt.scale = 0.5f;
        self->unkDC = 0;
        self->unkE0 = 0;
        self->animCallback = (void*)&dll_331_func_1D34;
        return;
    case OBJ_SB_Galleon:
        self->srt.yaw = 0;
        self->srt.pitch = 0;
        if (objSetup->roll >= 1000) {
            self->srt.scale = 1.0f / (objSetup->roll / 1000.0f);
        } else {
            self->srt.scale = 0.2f;
        }
        objData->unk3E = 0;
        objData->pStartX = objSetup->base.x;
        objData->pStartY = objSetup->base.y;
        objData->pStartZ = objSetup->base.z;
        objData->roll = 0.0f;
        objData->pitch = 0.0f;
        objData->unk24 = 0.5f;
        objData->unk1C = 0.5f;
        objData->unk28 = 1000.0f;
        objData->unk20 = 400.0f;
        self->srt.roll = 0;
        self->animCallback = (void*)&dll_331_func_1D34;
        return;
    case OBJ_Krazcol:
        self->srt.yaw = objSetup->yaw << 8;
        self->srt.scale = 1.0f;
        objData->unk3E = 0;
        objData->pStartX = objSetup->base.x;
        objData->pStartY = objSetup->base.y;
        objData->pStartZ = objSetup->base.z;
        objData->roll = 0.0f;
        objData->pitch = 0.0f;
        objData->unk28 = 0.0f;
        objData->unk20 = 0.0f;
        objData->unk24 = 0.0f;
        objData->unk1C = 0.0f;
        self->animCallback = (void*)&dll_331_func_1D34;
        return;
    case OBJ_SB_Lamp:
        self->srt.yaw = 0;
        self->srt.pitch = 0;
        self->srt.roll = 0;
        self->srt.scale = 1.0f;
        self->unkDC = 0;
        self->unkE0 = 0;
        objData->lampZero = 0;
        objData->unk1C = 0.5f;
        objData->unk24 = 600.0f;
        objData->lampRandom = rand_next(1000, 5000);
        objData->lampBool = TRUE;
        self->animCallback = (void*)&dll_331_func_1D34;
        return;
    case OBJ_DIM2IceFloe2:
        self->objhitInfo = 0;
        if (objSetup->pitch == 0) {
            objData->soundIDs = _data_4;
            objData->lastSoundIndex = 4;
        }
        temp = objSetup->roll ^ 0;
        objData->minCooldown = temp;
        objData->timer = temp;
        return;
    case OBJ_DFdebris1:
        objData->roll = 30.0f;
        return;
    case OBJ_FireFly:
        self->srt.scale = self->def->scale * 40.0f;
        break;
    }
}

RECOMP_PATCH void dll_331_control(Object* self) {
    s32 index;
    Object* player;
    ObjectPolyhits* temp_v0;
    s16 id;
    Object** new_var;
    GenProps_Setup *objSetup;
    s32 temp_t3;
    GenProps_Data_Extended* objData;
    DLL_Unknown* tempDLL2;
    DLL_Unknown* tempDLL;
    u8 temp_t8;
    u8 var_v1;
    f32 dx;
    f32 dy;
    f32 dz;
    f32 distance;
    GenProps_Data_Extended* objData2;
    Camera* camera;

    player = get_player();
    objData = self->data;
    camera = get_camera();
    objSetup = (GenProps_Setup*)self->setup;
    
    id = self->id;
    
    switch (id) {
    case OBJ_NWbigrock: //0x81
        break;
    case OBJ_DFP_PowerBolt: //0x4fa
        self->unkE0 -= (s16)gUpdateRateF;
        self->srt.transl.f[0] += self->velocity.f[0] * gUpdateRateF; 
        self->srt.transl.f[1] += self->velocity.f[1] * gUpdateRateF; 
        self->srt.transl.f[2] += self->velocity.f[2] * gUpdateRateF;
        gDLL_17_partfx->vtbl->spawn(self, 0x5F3, NULL, 0x10001, -1, NULL);
        if (vec3_distance(&self->globalPosition, &player->globalPosition) < 30.0f) {
            diPrintf("\tHit Krystal\n");
            obj_send_mesg(player, 0x60004, self, (void*)1);
            obj_destroy_object(self);
        }
        if (self->unkE0 <= 0) {
            obj_destroy_object(self);
        }
        return;
    case OBJ_VFP_PowerBolt: //0x549
        self->unkE0 -= (s16)gUpdateRateF;
        self->srt.transl.f[0] += self->velocity.f[0] * gUpdateRateF;
        self->srt.transl.f[1] += self->velocity.f[1] * gUpdateRateF;
        self->srt.transl.f[2] += self->velocity.f[2] * gUpdateRateF;
        gDLL_17_partfx->vtbl->spawn(self, 0x39D, NULL, 0x10001, -1, NULL);
        if (vec3_distance(&self->globalPosition, &player->globalPosition) < 30.0f) {
            diPrintf("\tHit Krystal\n");
            obj_send_mesg(player, 0x60004, self, (void*)1);
            obj_destroy_object(self);
        }
        if ((self->unkE0 <= 0) != 0) {
            obj_destroy_object(self);
        }
        break;
    case OBJ_DFP_blockwall: //0x4bf
        if ((self->srt.transl.f[1] < (objSetup->base.y + 30.0f)) && (main_get_bits(objData->gamebitA))) {
            self->srt.transl.f[1] += gUpdateRateF;
        }
        break;
    case OBJ_GPSHswapstone: //0x409
        gDLL_3_Animation->vtbl->func17(0, self, -1);
        break;
    case OBJ_DFturbinelever: //0xae
        
        new_var = &self->polyhits->unk100[0];
        if ((self->unkDC == 0) && (new_var != NULL) && (*(s16*)(((s32)self->polyhits) + 0x146) == 0x2B)) {
            gDLL_3_Animation->vtbl->func17(0, self, -1);
            self->unkDC = 1;
        }
        break;
        
    case OBJ_DFdebris1: //0xab
        objData->roll -= gUpdateRate;
        if (objData->roll < 0) {
            tempDLL2 = dll_load_deferred(0x1003, 1);
            tempDLL2->vtbl->func[0].withSixArgs((s32)self, 0, 0, 1, -1, 0);
            dll_unload(tempDLL2);
            objData->roll = 30.0f;
            return;
        }
        break;
    case OBJ_WMlargerock: //0x2b7
        if (main_get_bits(objData->gamebitA) == 0) {
            gDLL_3_Animation->vtbl->func17(0, self, -1);
        }
        break;
    case OBJ_WM_MoonSeedMoun: //0x271
        if (vec3_distance(&self->globalPosition, &player->globalPosition) < objData->debugPrintDistance) {
            diPrintf("\tMoonSeed Mound\n");
            diPrintf("\tThe Player Guesses that a Seed goes here!\n");
        }
        break;
    case OBJ_WM_Walkway1: //0x293
    case OBJ_WM_Walkway2: //0x294
        if ((objData->gamebitA != -1) && (main_get_bits(objData->gamebitA) != 0)) {
            if (self->srt.pitch <= 0) {
                self->srt.pitch += 50;
            } else {
                self->srt.pitch = 0;
            }
        }
        break;
    case OBJ_WM_Platform: //0x295
        WMPlatform_control_custom(self);
        break;
    case OBJ_WM_NoPassVine: //0x273
        //Handle collisions damaging the vines
        if (func_80025F40(self, NULL, NULL, NULL) != 0) {
            objData->vineHealth--;
            if (objData->vineHealth < 0) {
                obj_destroy_object(self);
            }
        }
        //Print debug info about object's purpose
        if (vec3_distance(&self->globalPosition, &player->globalPosition) < objData->debugPrintDistance) {
            diPrintf("\tNoPass Vine\n");
            diPrintf("\tThe Player Burns it away!\n");
        }
        break;
    case OBJ_WM_NoPassHorzVi: //0x28c
        //Handle collisions damaging the vines
        if (func_80025F40(self, NULL, NULL, NULL) != 0) {
            objData->vineHealth--;
            if (objData->vineHealth < 0) {
                main_set_bits(objData->gamebitA, 1);
                obj_destroy_object(self);
            }
        }
        //Print debug info about object's purpose
        if (vec3_distance(&self->globalPosition, &player->globalPosition) < objData->debugPrintDistance) {
            diPrintf("\tNoPass Vine\n");
            diPrintf("\tThe Player Burns it away!\n");
        }
        break;
    case OBJ_NWSH_rock: //0x221
    case OBJ_WMrock: //0x2bc
        self->objhitInfo->unk5D = 13;   //width?
        self->objhitInfo->unk5E = 2;    //height?
        self->objhitInfo->unk52 = 10;   //depth?
        self->objhitInfo->unkC = 10.0f;
        self->objhitInfo->unk50 = 30;
        self->objhitInfo->unk58 |= 1;
        if (main_get_bits(BIT_Player_Immune_to_Rainbow_Scarabs) != 0) {
            self->objhitInfo->unk58 &= 0xFFFE;
        }
        if (objData->unk3E == 2) {
            gDLL_6_AMSFX->vtbl->play_sound(self, 0x35A, 0x43, NULL, NULL, 0, NULL);
            objData->unk3E--;
        }
        if ((objData->unk3E != 0) && ((self->srt.transl.f[1] + self->velocity.f[1]) <= player->srt.transl.f[1])) {
            objData->unk3E = 0;
            self->velocity.f[1] *= -0.4f;
            self->velocity.f[0] *= 2.0f;
            self->velocity.f[2] *= 2.0f;
            self->srt.scale *= 0.5f;
            objData->roll *= 2;
            objData->pitch *= 2;
            gDLL_6_AMSFX->vtbl->play_sound(NULL, 0x35B, 0x43, NULL, NULL, 0, NULL);
            camera_set_shake_offset(0.5f);
        }
        if (objData->unk3E == 0) {
            if (objData->debugPrintDistance <= 40000.0f) {
                objData->debugPrintDistance += 2.0f * gUpdateRateF;
                func_80026940(self, (s16)(objData->debugPrintDistance / 10.0f) + 20);
            }
        }
        self->velocity.f[1] += -0.15f * gUpdateRateF;
        self->srt.roll += objData->roll;
        self->srt.pitch += objData->pitch;
        self->srt.transl.y += self->velocity.y;
        self->srt.transl.x += self->velocity.x;
        self->srt.transl.z += self->velocity.z;
        self->globalPosition.y = self->srt.transl.y;
        self->globalPosition.x = self->srt.transl.x;
        self->globalPosition.z = self->srt.transl.z;
        objData->speed -= gUpdateRate;
        if (player->srt.transl.f[1] < self->srt.transl.f[1]) {
            if (rand_next(0, 2) == 0) {
                gDLL_17_partfx->vtbl->spawn(self, 0x27F, NULL, 0x10001, -1, NULL);
            }
        }
        if (objData->speed <= 0) {
            obj_destroy_object(self);
        }
        break;
    case OBJ_SB_Galleon: //0x322
        objData->pitch += objData->unk1C * 3.0f;
        if ((objData->pitch > 180.0f) || (objData->pitch < -180.0f)) {
            objData->unk1C = 0.0f - objData->unk1C;
        }
        if ((objData->roll > 90.0f) || (objData->roll < -90.0f)) {
            objData->unk24 = 0.0f - objData->unk24;
        }
        objData->roll += objData->unk24 * 3.0f;
        break;
    case 133: //unknown deleted object (0x85)
        player = get_player();
        if (player != NULL) {
            dx = player->globalPosition.f[0] - self->globalPosition.f[0];
            dz = player->globalPosition.f[2] - self->globalPosition.f[2];
            distance = sqrtf(SQ(dx) + SQ(dz));
            if ((distance < 400.0f) && (self->unkDC <= 0)) {

                index = 1;
                tempDLL2 = dll_load_deferred(0x1008, 1);

                while (index != 0){ 
                    tempDLL2->vtbl->func[0].withSixArgs((s32)self, 0, 0, 1, -1, 0);
                    index--;
                }
                
                dll_unload(tempDLL2);
                self->unkDC = rand_next(100, 200);
            } else if (distance < 200.0f) {
                self->unkDC -= 1;
            }
            if ((_data_0 != NULL) && (self->unkE0 == 0)) {
                self->unkE0 = 1;
                tempDLL = dll_load_deferred(0x200A, 1);
                tempDLL->vtbl->func[0].withSevenArgs((s32)self, 0, 0, 1, -1, 0xA, 0);
                if (tempDLL != NULL) {
                    dll_unload(tempDLL);
                }
            }
            if (_data_0 == NULL) {
                self->unkE0 = 0;
            }
        }
        break;
    case 134: //unknown deleted object (0x86)
        player = get_player();
        if (player != NULL) {
            dx = player->globalPosition.f[0] - self->globalPosition.f[0];
            dz = player->globalPosition.f[2] - self->globalPosition.f[2];
            distance = sqrtf(SQ(dx) + SQ(dz));
            if (self->unkDC != 0) {
                self->unkDC -= gUpdateRate;
                if (self->unkDC <= 0) {
                    tempDLL = dll_load_deferred(0x2009, 1);
                    tempDLL->vtbl->func[0].withSevenArgs((s32)_data_0, 0, 0, 1, -1, 9, 0);
                    if (tempDLL != NULL) {
                        dll_unload(tempDLL);
                    }
                    _data_0 = NULL;
                    self->unkDC = 0;
                    self->unkE0 = 100;
                }
            } else if ((distance <= 10.0f) && (_data_0 == NULL) && (self->unkE0 == 0)) {
                gDLL_6_AMSFX->vtbl->play_sound(self, 0x1D2, 0x7F, NULL, NULL, 0, NULL);
                _data_0 = self;
                self->unkDC = 0x46;
            } else if ((distance < 40.0f) && (self->unkE0 == 0) && (self->unkDC == 0)) {
                gDLL_3_Animation->vtbl->func17(0, self, -1);
            }
            if (--self->unkE0 <= 0) {
                self->unkE0 = 0;
            }
        }
        break;
    case OBJ_DIM2IceFloe2: //10d
        objData->timer -= gUpdateRate;
        if (objData->timer < 0) {
            gDLL_6_AMSFX->vtbl->play_sound(self, objData->soundIDs[rand_next(0, objData->lastSoundIndex)], 0x7F, NULL, NULL, 0, NULL);
            objData->timer = objData->minCooldown;
            objData->timer += rand_next(0, objData->minCooldown);
        }
        break;
    case OBJ_SB_Lamp: //0x125
        self->srt.roll = -camera->srt.roll * 1.5;
        player = get_player();
        dx = player->globalPosition.x - self->globalPosition.x;
        dz = player->globalPosition.z - self->globalPosition.z;
        dy = player->globalPosition.y - self->globalPosition.y;
        distance = sqrtf(SQ(dx) + SQ(dz) + SQ(dy));
        if ((distance < 75.0f) && (objData->lampBool == TRUE)) {
            objData->lampBool = FALSE;
            func_80000450(self, self, 0x5C, 0, 0, 0);
        } else if ((distance > 75.0f) && (objData->lampBool == FALSE)) {
            objData->lampBool = TRUE;
            func_80000450(self, self, 0x5D, 0, 0, 0);
        }
        break;
    }
}

RECOMP_PATCH u32 dll_331_get_data_size(Object *self, u32 a1){
    return sizeof(GenProps_Data_Extended);
}

RECOMP_PATCH void dll_331_free(Object* self, s32 arg1) {
    GenProps_Data_Extended *objData = self->data;

    gDLL_13_Expgfx->vtbl->func5(self);

    //@recomp: free soundHandle
    if (objData->soundHandleHum) {
        gDLL_6_AMSFX->vtbl->func_A1C(objData->soundHandleHum);
        objData->soundHandleHum = 0;
    }
}

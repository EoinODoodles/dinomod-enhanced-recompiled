#include "modding.h"

#include "PR/ultratypes.h"
#include "PR/gbi.h"
#include "PR/gu.h"
#include "dlls/objects/common/sidekick.h"
#include "dlls/objects/210_player.h"
#include "game/objects/object.h"
#include "game/objects/object_id.h"
#include "recomputils.h"
#include "sys/asset_thread.h"
#include "sys/camera.h"
#include "sys/dll.h"
#include "sys/gfx/model.h"
#include "sys/main.h"
#include "sys/math.h"
#include "sys/objects.h"
#include "sys/objtype.h"
#include "sys/objmsg.h"
#include "sys/gfx/gx.h"
#include "sys/map.h"
#include "dll.h"
#include "types.h"
#include "functions.h"

#include "recomp/dlls/objects/325_trigger_recomp.h"

typedef struct {
    u8 condition; // TriggerCommandConditionFlags enum
    u8 id;
    // For some commands, param1 and 2 are read as a single u16, but 
    // in code they are read individually and then combined with bit math.
    u8 param1;
    u8 param2;
} TriggerCommand;

typedef struct {
/*00*/ ObjCreateInfo base;
/*18*/ TriggerCommand commands[8];
/*38*/ s16 localID; // TODO: needs verification
/*3A*/ u8 sizeX; // unit depends on trigger type
/*3B*/ u8 sizeY; // unit depends on trigger type
/*3C*/ u8 sizeZ; // unit depends on trigger type
/*3D*/ u8 rotationY; // unit depends on trigger type
/*3E*/ u8 rotationX; // unit depends on trigger type
/*3F*/ u8 _unk3F[4];
/// The object type of the object that can activate the trigger.
/// If multiple instances of the given type exist in the scene, the one
/// closest to the trigger will be used.
/// Exceptions:
/// 0 - Always the primary player (even if multiple player instances exist)
/// 1 - Always the primary sidekick (even if multiple sidekick instances exist)
/// 2 - The camera(?)
/*43*/ u8 activatorObjType;
// Game bit flag to save the entered state of the trigger in.
// This flag will be set the first time the trigger is entered. If the flag is already
// set upon object creation, the entered state of the trigger will be restored, possibly
// running commands on the next update that support being restored.
/*44*/ s16 bitFlagID;
// Number of game ticks from object creation to wait before activating a 
// TriggerTime instance.
/*46*/ u16 timerDuration;
// Game bit flags that must be *all* set before the trigger can activate.
// Only supported by TriggerPlane and TriggerBits (plane only supports one flag to check).
// A negative ID indicates that there is no flag to check for that condition slot. 
/*48*/ s16 conditionBitFlagIDs[4];
} TriggerCreateInfo;

DLL_INTERFACE(DLL_TriggerScript) {
    /*:*/ DLL_INTERFACE_BASE(DLL);
    // Array length will vary
    void (*subscripts[1])(Object *trigger, Object *activator, s8 dir, s32 activatorDistSquared);
};

typedef struct {
/*00*/ u8 flags; // TriggerFlags enum
/*01*/ u8 _unk1[3];
/*04*/ f32 radiusSquared;
/*08*/ u8 _unk8[4];
/*0C*/ s32 elapsedTicks; // for TriggerTime
/*10*/ Vec3f lookVector; // for TriggerPlane
/*1C*/ f32 lookVectorNegDot;
/*20*/ Vec3f activatorPrevPos;
/*2C*/ Vec3f activatorCurrPos;
/*38*/ Vec3f planeMin;
/*44*/ Vec3f planeMax;
/*50*/ u8 _unk50[8];
/*58*/ s16 bitFlagID;
/*5A*/ s16 conditionBitFlagIDs[4];
/*62*/ u8 _unk62[2];
/*64*/ u32 soundHandles[8];
// Special "script" DLLs where each export is a "subscript".
/*84*/ DLL_TriggerScript *scripts[8];
} TriggerState;

typedef enum {
    // Activator entered at least once
    TRG_ACTIVATOR_ENTERED = 0x1,
    // Activator exited at least once
    TRG_ACTIVATOR_EXITED = 0x2,
    TRG_RESTORE_ENTERED_STATE = 0x4,
    TRG_FIRST_TICK = 0x40
} TriggerFlags;

typedef enum {
    // When activator is "inside" the trigger
    CMD_COND_IN = 0x1,
    // When activator is "outside" of the trigger
    CMD_COND_OUT = 0x2,
    // Command can be activated if trigger is entered more than once
    CMD_COND_RE_ENTER = 0x4,
    // Command can be activated if trigger is exited more than once
    CMD_COND_RE_EXIT = 0x8,
    // Command is activated every game tick the in/out conditions are met,
    // and not just on the initial entry/exit tick
    CMD_COND_CONTINUOUS = 0x10,
    // Activate the command when restoring the trigger's entered state
    CMD_COND_RESTORE = 0x20
} TriggerCommandConditionFlags;

typedef enum {
    TRG_CMD_HAZARD = 0x1, // "gameplay vulnerable"?
    TRG_CMD_2 = 0x2, // not implemented
    TRG_CMD_MUSIC_ACTION = 0x3,
    TRG_CMD_SOUND = 0x4,
    TRG_CMD_5 = 0x5,
    TRG_CMD_CAMERA_ACTION = 0x6,
    TRG_CMD_7 = 0x7, // not implemented
    TRG_CMD_TRACK = 0x8,
    TRG_CMD_9 = 0x9, // not implemented
    TRG_CMD_ENV_FX = 0xA,
    TRG_CMD_ANIM_SEQ = 0xB,
    TRG_CMD_TRIGGER = 0xC,
    TRG_CMD_LIGHTING = 0xD,
    TRG_CMD_E = 0xE, // not implemented
    TRG_CMD_F = 0xF,
    TRG_CMD_LOD_MODEL = 0x10,
    TRG_CMD_11 = 0x11, // TRG_CMD_TRICKY_?
    TRG_CMD_FLAG = 0x12,
    TRG_CMD_ENABLE_OBJ_GROUP = 0x13,
    TRG_CMD_DISABLE_OBJ_GROUP = 0x14,
    TRG_CMD_TEXTURE_LOAD = 0x15,
    TRG_CMD_TEXTURE_FREE = 0x16,
    TRG_CMD_17 = 0x17, // not implemented
    TRG_CMD_SET_MAP_SETUP = 0x18,
    TRG_CMD_SCRIPT = 0x19,
    TRG_CMD_WORLD_ENABLE_OBJ_GROUP = 0x1A,
    TRG_CMD_WORLD_DISABLE_OBJ_GROUP = 0x1B,
    TRG_CMD_KYTE_FLIGHT_GROUP = 0x1C,
    TRG_CMD_KYTE_TALK_SEQ = 0x1D,
    TRG_CMD_WORLD_SET_MAP_SETUP = 0x1E,
    TRG_CMD_SAVE_GAME = 0x1F,
    TRG_CMD_MAP_LAYER = 0x20,
    TRG_CMD_FLAG_TOGGLE = 0x21,
    TRG_CMD_TOGGLE_OBJ_GROUP = 0x22,
    TRG_CMD_RESTART = 0x23,
    TRG_CMD_WATER_FALLS_FLAGS = 0x24,
    TRG_CMD_WATER_FALLS_FLAGS2 = 0x25,
    TRG_CMD_SIDEKICK = 0x26
} TriggerCommandID;

extern void trigger_process_commands(Object *self, Object *activator, s8 dir, s32 activatorDistSquared);
extern void trigger_func_1754(u8 param1, u8 param2);
extern void trigger_func_1764(u16 param1);
extern void trigger_func_17FC(u16 param1);
extern void trigger_func_1868(u16 param1);
extern void trigger_func_1920(u16 param1);

extern void trigger_point_setup(Object *obj, TriggerCreateInfo *createInfo);
extern void trigger_point_update(Object *self, Object *activator);

extern void trigger_cylinder_setup(Object *obj, TriggerCreateInfo *createInfo);
extern void trigger_cylinder_update(Object *self, Object* activator);

extern void trigger_plane_setup(Object *obj, TriggerCreateInfo *createInfo);
extern void trigger_plane_update(Object *self, Object *activator);

extern void trigger_area_setup(Object *self, TriggerCreateInfo *createInfo);
extern void trigger_area_update(Object *self, Object *activator);
extern s32 trigger_func_273C(Object *self, Vec3f *vec);
extern void trigger_func_2884(Object *self, f32 *ox, f32 *oy, f32 *oz);
extern void trigger_func_29C0(u16 localID, Object *activator, s8 dir, s32 activatorDistSquared);

extern void trigger_curve_setup(Object *self, TriggerCreateInfo *createInfo);
extern void trigger_curve_update(Object *self, Object *activator);

RECOMP_PATCH void trigger_update(Object* self) {
    TriggerState* state;
    TriggerCreateInfo* createInfo;
    Object* player;
    Object* temp_v0_2;
    Object* sidekick;
    Object* activatorObj;
    s32 i;
    s32 b_allBitsSet;
    s32 b_foundActivatorObj;
    f32 maxObjSearchDist;

    state = (TriggerState*)self->state;
    createInfo = (TriggerCreateInfo*)self->createInfo;
    
    maxObjSearchDist = 200.0f;
   
    player = get_player();
    if (player != NULL) {
        temp_v0_2 = ((DLL_210_Player*)player->dll)->vtbl->func7(player);
        if (temp_v0_2 != NULL) {
            player = temp_v0_2;
        }
    }
    
    sidekick = get_sidekick();
    
    if ((player != NULL) || (sidekick != NULL)) {
        if (state->flags & TRG_RESTORE_ENTERED_STATE) {
            trigger_process_commands(self, player, 1, 0);
            state->flags &= ~TRG_RESTORE_ENTERED_STATE;
            state->flags |= TRG_ACTIVATOR_ENTERED;
            return;
        }
        
        b_foundActivatorObj = TRUE;
        if (createInfo->activatorObjType >= 3) {
            activatorObj = obj_get_nearest_type_to(createInfo->activatorObjType, self, &maxObjSearchDist);
            if (activatorObj == NULL) {
                b_foundActivatorObj = FALSE;
            }
        } else {
            switch (createInfo->activatorObjType) {
            case 0:
                activatorObj = player;
                if (player == NULL) {
                    b_foundActivatorObj = FALSE;
                }
                break;
            case 1:
                activatorObj = sidekick;
                if (sidekick == NULL) {
                    b_foundActivatorObj = FALSE;
                }
                break;
            case 2:
                activatorObj = gDLL_2_Camera->vtbl->func2();
                break;
            }
        }
        
        if (b_foundActivatorObj) {
            if (state->flags & TRG_FIRST_TICK) {
                switch (createInfo->activatorObjType) {
                default:
                    state->activatorPrevPos.x = activatorObj->positionMirror2.x;
                    state->activatorPrevPos.y = activatorObj->positionMirror2.y;
                    state->activatorPrevPos.z = activatorObj->positionMirror2.z;
                    break;
                case 2:
                    // Camera
                    state->activatorPrevPos.x = activatorObj->srt.transl.x;
                    state->activatorPrevPos.y = activatorObj->srt.transl.y;
                    state->activatorPrevPos.z = activatorObj->srt.transl.z;
                    break;
                case 0:
                case 1:
                    // Player/sidekick
                    state->activatorPrevPos.x = activatorObj->positionMirror3.x;
                    state->activatorPrevPos.y = activatorObj->positionMirror3.y;
                    state->activatorPrevPos.z = activatorObj->positionMirror3.z;
                    break;
                }
                
                state->flags &= ~TRG_FIRST_TICK;
            } else {
                state->activatorPrevPos.x = state->activatorCurrPos.x;
                state->activatorPrevPos.y = state->activatorCurrPos.y;
                state->activatorPrevPos.z = state->activatorCurrPos.z;
            }
            
            switch (createInfo->activatorObjType) {
            case 0:
            case 1:
                // Player/sidekick
                state->activatorCurrPos.x = activatorObj->positionMirror.x;
                state->activatorCurrPos.y = activatorObj->positionMirror.y;
                state->activatorCurrPos.z = activatorObj->positionMirror.z;
                break;
            default:
            case 2:
                // Camera/other
                state->activatorCurrPos.x = activatorObj->srt.transl.x;
                state->activatorCurrPos.y = activatorObj->srt.transl.y;
                state->activatorCurrPos.z = activatorObj->srt.transl.z;
                break;
            }
        }
        
        switch (createInfo->base.objId) {
        case OBJ_TriggerPoint:
            if (b_foundActivatorObj) {
                trigger_point_update(self, activatorObj);
            }
            break;
        case OBJ_TriggerCylinder:
            if (b_foundActivatorObj) {
                trigger_cylinder_update(self, activatorObj);
            }
            break;
        case OBJ_TriggerPlane:
            b_allBitsSet = TRUE;
            if (state->conditionBitFlagIDs[0] >= 0) {
                // @recomp: Dino Mod condition bit extension for checking unset flags (originally by MusicalProgrammer)
                if (state->conditionBitFlagIDs[0] & 0x4000) {
                    if (get_gplay_bitstring(state->conditionBitFlagIDs[0] & ~0x4000) != 0) {
                        b_allBitsSet = FALSE;
                    }
                } else {
                    if (get_gplay_bitstring(state->conditionBitFlagIDs[0]) == 0) {
                        b_allBitsSet = FALSE;
                    }
                }
            }
            if (b_allBitsSet && b_foundActivatorObj) {
                trigger_plane_update(self, activatorObj);
            }
            break;
        case OBJ_TriggerTime:
            state->elapsedTicks += delayByte;
            if (state->elapsedTicks >= createInfo->timerDuration) {
                trigger_process_commands(self, NULL, 1, 0);
            }
            break;
        case OBJ_TriggerArea:
            if (b_foundActivatorObj) {
                trigger_area_update(self, activatorObj);
            }
            break;
        case OBJ_TriggerSetup:
            trigger_process_commands(self, player, 1, 0);
            if (ret1_8001454c() != 0) {
                obj_destroy_object(self);
            }
            break;
        case OBJ_TriggerBits:
            b_allBitsSet = TRUE;
            for (i = 0; i < 4 && b_allBitsSet; i++) {
                if (state->conditionBitFlagIDs[i] >= 0) {
                    if (get_gplay_bitstring(state->conditionBitFlagIDs[i]) == 0) {
                        b_allBitsSet = FALSE;
                    }
                }
            }
            if (b_allBitsSet) {
                trigger_process_commands(self, player, 1, 0);
            }
            break;
        case OBJ_TriggerCurve:
            if (b_foundActivatorObj) {
                trigger_curve_update(self, activatorObj);
            }
            break;
        }
    }
}

static void set_map_layer(s8 layer) {
    D_80092A8C = layer;
    if (D_80092A8C >= 3) {
        D_80092A8C = 2;
    }
    if (D_80092A8C < -2) {
        D_80092A8C = -2;
    }
    UINT_80092a98 |= 0x4000;
}

RECOMP_PATCH void trigger_process_commands(Object *self, Object *activator, s8 dir, s32 activatorDistSquared) {
    TriggerCreateInfo* createInfo; // sp+74
    TriggerState* state; // sp+70
    //Object* player;
    s32 pad;
    TriggerCommand *cmd;
    s32 temp_a1;
    u8 i;
    Object* var_v0_2;
    Object* sidekick;
    s32 pad2;

    state = (TriggerState*)self->state;
    createInfo = (TriggerCreateInfo*)self->createInfo;
    
    for (i = 0, cmd = createInfo->commands; i < 8; i++, cmd++) {
        if (cmd->id == 0) {
            // No command assigned to this slot
            continue;
        }

        if ((state->flags & TRG_RESTORE_ENTERED_STATE) && !(cmd->condition & CMD_COND_RESTORE)) {
            continue;
        }
        
        if (!(cmd->condition & CMD_COND_CONTINUOUS)) {
            // Not continuous, check for entry/exit tick
            if (dir == 1) {
                // Entered
                if (!(cmd->condition & CMD_COND_IN)) {
                    continue;
                }
                if ((state->flags & TRG_ACTIVATOR_ENTERED) && !(cmd->condition & CMD_COND_RE_ENTER)) {
                    continue;
                }
            } else if (dir == -1) {
                // Exited
                if (!(cmd->condition & CMD_COND_OUT)) {
                    continue;
                }
                if ((state->flags & TRG_ACTIVATOR_EXITED) && !(cmd->condition & CMD_COND_RE_EXIT)) {
                    continue;
                }
            } else {
                continue;
            }
        } else {
            // Continuous, check if in/out
            if (cmd->condition & CMD_COND_IN) {
                if (dir < 0) {
                    continue;
                }
            } else if (cmd->condition & CMD_COND_OUT) {
                if (dir > 0) {
                    continue;
                }
            }
        }
        
        switch (cmd->id) {                  /* switch 1 */
        case TRG_CMD_HAZARD: 
            // "Trigger [%d], Gamplay Vulnerable"
            switch (cmd->param1) {              /* switch 2 */
            case 0:                         /* switch 2 */
            case 1:                         /* switch 2 */
            case 2:                         /* switch 2 */
            case 3:                         /* switch 2 */
            case 4:                         /* switch 2 */
            case 5:                         /* switch 2 */
            case 6:                         /* switch 2 */
                break;
            case 7: {                         /* switch 2 */
                    s32 mesgID; // sp+50
                    if (dir == 1) {
                        mesgID = 0x80;
                    } else {
                        mesgID = 0x81;
                    }
                    obj_send_mesg_many_nearby(OBJ_Swoop,           6000.0f, 0, self, mesgID, 0);
                    obj_send_mesg_many_nearby(OBJ_GP_ChimneySwipe, 6000.0f, 0, self, mesgID, 0);
                }
                break;
            case 8: {                        /* switch 2 */
                    Object *player = get_player();
                    if (player != NULL) {
                        ((DLL_210_Player*)player->dll)->vtbl->func67(player, 9, 0.0f);
                    }
                }
                break;
            case 9: {                        /* switch 2 */
                    Object *player = get_player();
                    if (player != NULL) {
                        ((DLL_210_Player*)player->dll)->vtbl->func67(player, 10, 0.0f);
                    }
                }
                break;
            case 10: {                       /* switch 2 */
                    Object *player = get_player();
                    if (player != NULL) {
                        ((DLL_210_Player*)player->dll)->vtbl->func67(player, 11, 0.0f);
                    }
                }
                break;
            }
            break;
        case TRG_CMD_MUSIC_ACTION: 
            if ((dir < 0) && (gDLL_5_AMSEQ->vtbl->func2(self, (cmd->param2 | (cmd->param1 << 8))) != 0)) {
                // "Trigger [%d], Music Action,       Action Num [%d] Free"
                gDLL_5_AMSEQ2->vtbl->func1(self, (cmd->param2 | (cmd->param1 << 8)), 0, 0, 0);
            } else {
                // "Trigger [%d], Music Action,       Action Num [%d] Set"
                gDLL_5_AMSEQ2->vtbl->func0(self, (cmd->param2 | (cmd->param1 << 8)), 0, 0, 0);
            }
            break;
        case TRG_CMD_SOUND: 
            // "Trigger [%d], Sound FX,           Action Num [%d],Handle Num [%d]"
            if (dir >= 0) {
                gDLL_6_AMSFX->vtbl->func_10D0(self, (cmd->param2 | (cmd->param1 << 8)), &state->soundHandles[i]);
            } else {
                if (state->soundHandles[i] != 0) {
                    gDLL_6_AMSFX->vtbl->func_A1C(state->soundHandles[i]);
                    state->soundHandles[i] = 0;
                }
            }
            break;
        case TRG_CMD_CAMERA_ACTION: 
            // "Trigger [%d], Camera,             Action [%d], Camera Num [%d], PassDir [%d]"
            gDLL_2_Camera->vtbl->func8(cmd->param1, cmd->param2);
            break;
        case TRG_CMD_TRACK: 
            // "Trigger [%d], Track Sky On"
            // "Trigger [%d], Track Sky Off"
            // "Trigger [%d], Track AntiAlias On"
            // "Trigger [%d], Track AntiAlias Off"
            // "Trigger [%d], Track SkyObjects On"
            // "Trigger [%d], Track SkyObjects Off"
            // "Trigger [%d], Track Dome On"
            // "Trigger [%d], Track Dome Off"
            // "Trigger [%d], Track MrSheen On %d"
            // "Trigger [%d], Track MrSheen Off"
            switch (cmd->param1) {              /* switch 3 */
            case 0:                         /* switch 3 */
                if ((s32) cmd->param2 >= 2) {
                    cmd->param2 = 1;
                }
                func_80041C6C(cmd->param2);
                if (cmd->param2 != 0) {

                }
                break;
            case 1:                         /* switch 3 */
                if ((s32) cmd->param2 >= 2) {
                    cmd->param2 = 1;
                }
                func_80041CA8((s32) cmd->param2);
                if (cmd->param2 != 0) {

                }
                break;
            case 2:                         /* switch 3 */
                if ((s32) cmd->param2 >= 2) {
                    cmd->param2 = 1;
                }
                func_80041CE4((s32) cmd->param2);
                if (cmd->param2 != 0) {

                }
                break;
            case 3:                         /* switch 3 */
                if ((s32) cmd->param2 >= 2) {
                    cmd->param2 = 1;
                }
                gDLL_12_Minic->vtbl->func6(cmd->param2);
                if (cmd->param2 != 0) {

                }
                break;
            case 4:                         /* switch 3 */
                gDLL_16->vtbl->func2(cmd->param2);
                if (cmd->param2 != 0) {

                }
                break;
            case 5:                         /* switch 3 */
                func_8005CA5C((u32) cmd->param2);
                break;
            case 6:                         /* switch 3 */
                if ((s32) cmd->param2 > 0) {
                    func_8001EBD0(1);
                } else {
                    func_8001EBD0(0);
                }
                break;
            case 7:                         /* switch 3 */
                if ((s32) cmd->param2 > 0) {
                    func_80041E24(1);
                } else {
                    func_80041E24(0);
                }
                break;
            }
            break;
        case TRG_CMD_5: 
            if ((state->radiusSquared != 0.0f) && (activatorDistSquared != 0) && (createInfo->base.objId == OBJ_TriggerPlane)) {

            }
            break;
        case TRG_CMD_ENV_FX:
            // "Trigger [%d], Environment Effect, Action Num [%d], Range [%d]"
            func_80000860(self, activator, (cmd->param2 | (cmd->param1 << 8)), activatorDistSquared);
            break;
        case TRG_CMD_LIGHTING:
            // "Trigger [%d], Lighting,           Action      [%d], Range [%d], PassDir [%d]"
            func_80000450(self, activator, (cmd->param2 | (cmd->param1 << 8)), dir, activatorDistSquared, 0);
            break;
        case TRG_CMD_ANIM_SEQ:
            switch (cmd->param1) {
                case 0:
                case 3:
                    // "Trigger [%d], Anim Sequence,      SequenceID [%d], Activate"
                    gDLL_3_Animation->vtbl->func1(cmd->param2, 0); // should be 3 params?
                    break;
                case 1:
                    // "Trigger [%d], Anim Sequence,      SequenceID [%d], Flag = 1"
                    gDLL_3_Animation->vtbl->func2(cmd->param2, 1);
                    break;
                case 2:
                    // "Trigger [%d], Anim Sequence,      SequenceID [%d], Flag = 0"
                    gDLL_3_Animation->vtbl->func2(cmd->param2, 0);
                    break;
            }
            break;
        case TRG_CMD_TRIGGER:
            // "Trigger [%d], Trigger,            Local ID   [%d]"
            trigger_func_29C0((cmd->param2 | (cmd->param1 << 8)), activator, dir, activatorDistSquared);
            break;
        // case TRG_CMD_?
            // "Storyboard disabled, please remove trigger\n"
        case TRG_CMD_LOD_MODEL:
            // "Trigger [%d], LOD Model [%d]"
            func_80023A18(get_player(), (s32) cmd->param1);
            break;
        case TRG_CMD_F:
            // "Trigger [%d], Setup Point,        Level      [%d], SetupPoint [%d]"
            // ?
            trigger_func_1754(cmd->param1, cmd->param2);
            break;
        case TRG_CMD_FLAG:
            // "Trigger [%d], Bits\n"
            trigger_func_1764((cmd->param2 | (cmd->param1 << 8)));
            break;
        case TRG_CMD_FLAG_TOGGLE:
            trigger_func_17FC((cmd->param2 | (cmd->param1 << 8)));
            break;
        case TRG_CMD_ENABLE_OBJ_GROUP:
            // "Trigger [%d], Object Load\n"
            gDLL_29_Gplay->vtbl->set_obj_group_status((s32) self->mapID, cmd->param2 | (cmd->param1 << 8), 1);
            break;
        case TRG_CMD_DISABLE_OBJ_GROUP:
            // "Trigger [%d], Object Free\n"
            gDLL_29_Gplay->vtbl->set_obj_group_status((s32) self->mapID, cmd->param2 | (cmd->param1 << 8), 0);
            break;
        case TRG_CMD_TOGGLE_OBJ_GROUP:
            // "Trigger [%d], Object Toggle\n"
            temp_a1 = (cmd->param2 | (cmd->param1 << 8));
            gDLL_29_Gplay->vtbl->set_obj_group_status((s32) self->mapID, temp_a1, gDLL_29_Gplay->vtbl->get_obj_group_status((s32) self->mapID, temp_a1) ^ 1);
            break;
        case TRG_CMD_TEXTURE_LOAD:
            // "Trigger [%d], Tex Load\n"
            trigger_func_1868((cmd->param2 | (cmd->param1 << 8)));
            break;
        case TRG_CMD_TEXTURE_FREE:
            // "Trigger [%d], Tex Free\n"
            trigger_func_1920((cmd->param2 | (cmd->param1 << 8)));
            break;
        case TRG_CMD_SET_MAP_SETUP:
            gDLL_29_Gplay->vtbl->set_map_setup((s32) self->mapID, cmd->param2 | (cmd->param1 << 8));
            break;
        case TRG_CMD_SCRIPT:
            // "TRIGGER: warning DLL not loaded\n"
            // "Script [%d], Subscript [%d]\n"
            if (state->scripts[i] != NULL) {
                state->scripts[i]->vtbl->subscripts[cmd->param2](self, activator, dir, activatorDistSquared);
            }
            break;
        case TRG_CMD_WORLD_ENABLE_OBJ_GROUP:
            // "Trigger [%d], Object Load\n"
            gDLL_29_Gplay->vtbl->set_obj_group_status((s32) cmd->param2, (s32) cmd->param1, 1);
            break;
        case TRG_CMD_WORLD_DISABLE_OBJ_GROUP:
            // "Trigger [%d], Object Free\n"
            gDLL_29_Gplay->vtbl->set_obj_group_status((s32) cmd->param2, (s32) cmd->param1, 0);
            break;
        case TRG_CMD_KYTE_FLIGHT_GROUP:
            set_gplay_bitstring(0x46E, cmd->param2 | (cmd->param1 << 8));
            break;
        case TRG_CMD_KYTE_TALK_SEQ:
            set_gplay_bitstring(0x488, cmd->param2 | (cmd->param1 << 8));
            break;
        case TRG_CMD_WORLD_SET_MAP_SETUP:
            gDLL_29_Gplay->vtbl->set_map_setup((s32) cmd->param2, (s32) cmd->param1);
            break;
        case TRG_CMD_11:
            // Tricky related?
            set_gplay_bitstring(0x4E2, cmd->param2 | (cmd->param1 << 8));
            break;
        case TRG_CMD_SAVE_GAME:
            gDLL_29_Gplay->vtbl->checkpoint(&self->srt.transl, (s16) ((s16) self->srt.yaw >> 8), (s32) cmd->param2, func_80048498());
            break;
        case TRG_CMD_MAP_LAYER:
            // @recomp: Dino Mod trigger mapLayer command extension (originally by MusicalProgrammer)
            if ((s8)cmd->param2 == -1) {
                set_map_layer(cmd->param1);
            } else {
                if (cmd->param1 == 0) {
                    func_80047374();
                } else {
                    func_800473BC();
                }
            }
            break;
        case TRG_CMD_RESTART:
            switch (cmd->param1) {            /* switch 4; irregular */
            case 0:                         /* switch 4 */
                // "Restart Set [%d]\n"
                gDLL_29_Gplay->vtbl->restart_set(&self->srt.transl, self->srt.yaw, func_80048498());
                break;
            case 1:                         /* switch 4 */
                // "Restart Clear [%d]\n"
                gDLL_29_Gplay->vtbl->restart_clear();
                break;
            case 2:                         /* switch 4 */
                // "Restart Goto [%d]\n"
                gDLL_29_Gplay->vtbl->restart_goto();
                break;
            }
            break;
        case TRG_CMD_SIDEKICK:
            sidekick = get_sidekick();
            if (sidekick != NULL) {
                switch (cmd->param1) {       /* switch 5; irregular */
                case 0:                     /* switch 5 */
                    ((DLL_ISidekick *)sidekick->dll)->vtbl->func23(sidekick);
                    break;
                case 1:                     /* switch 5 */
                    // "killing sidekick\n"
                    obj_destroy_object(get_sidekick());
                    break;
                case 2:                     /* switch 5 */
                    // "findobj %i \n"
                    var_v0_2 = obj_get_nearest_type_to(0x34, sidekick, NULL);
                    if (var_v0_2 == NULL) {
                        var_v0_2 = obj_get_nearest_type_to(0x33, sidekick, NULL);
                    }
                    if (var_v0_2 != NULL) {
                        ((DLL_ISidekick *)sidekick->dll)->vtbl->func22(sidekick, var_v0_2);
                    }
                    break;
                }
            }
            break;
        case TRG_CMD_WATER_FALLS_FLAGS:
        case TRG_CMD_WATER_FALLS_FLAGS2:
            // "Trigger [%d], amSfxWaterFallsSetFlags,   Action [%d], PassDir [%d]"
            gDLL_6_AMSFX->vtbl->water_falls_set_flags(cmd->param1);
            break;
        }
    }

    if (dir > 0) {
        // In
        state->flags |= TRG_ACTIVATOR_ENTERED;
        set_gplay_bitstring(state->bitFlagID, 1);
    } else if (dir < 0) {
        // Out
        state->flags |= TRG_ACTIVATOR_EXITED;
    }
}

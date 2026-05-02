#include "modding.h"
#include "recomputils.h"

#include "PR/ultratypes.h"
#include "PR/gbi.h"
#include "PR/gu.h"
#include "types.h"
#include "game/objects/object.h"
#include "game/objects/object_id.h"
#include "game/gamebits.h"
#include "sys/asset_thread.h"
#include "sys/camera.h"
#include "sys/dll.h"
#include "sys/footsteps.h"
#include "sys/gfx/model.h"
#include "sys/gfx/texture.h"
#include "sys/main.h"
#include "sys/math.h"
#include "sys/objects.h"
#include "sys/objmsg.h"
#include "sys/objtype.h"
#include "sys/segment_1D900.h"
#include "sys/segment_1050.h"
#include "sys/segment_1460.h"
#include "sys/vi.h"
#include "dll.h"
#include "dlls/objects/common/sidekick.h"
#include "dlls/objects/210_player.h"

#include "dlls/objects/325_trigger.h"

#include "recomp/dlls/objects/325_trigger_recomp.h"

extern s8 gMapLayer;
extern u32 UINT_80092a98;

extern void trigger_process_commands(Object *self, Object *activator, s8 dir, s32 activatorDistSquared);
extern void trigger_func_1754(u8 param1, u8 param2);
extern void trigger_func_1764(u16 param1);
extern void trigger_func_17FC(u16 param1);
extern void trigger_func_1868(u16 param1);
extern void trigger_func_1920(u16 param1);

extern void trigger_point_setup(Object *obj, Trigger_Setup *setup);
extern void trigger_point_update(Object *self, Object *activator);

extern void trigger_cylinder_setup(Object *obj, Trigger_Setup *setup);
extern void trigger_cylinder_update(Object *self, Object* activator);

extern void trigger_plane_setup(Object *obj, Trigger_Setup *setup);
extern void trigger_plane_update(Object *self, Object *activator);

extern void trigger_area_setup(Object *self, Trigger_Setup *setup);
extern void trigger_area_update(Object *self, Object *activator);
extern s32 trigger_func_273C(Object *self, Vec3f *vec);
extern void trigger_func_2884(Object *self, f32 *ox, f32 *oy, f32 *oz);
extern void trigger_func_29C0(u16 localID, Object *activator, s8 dir, s32 activatorDistSquared);

extern void trigger_curve_setup(Object *self, Trigger_Setup *setup);
extern void trigger_curve_update(Object *self, Object *activator);

RECOMP_PATCH void trigger_control(Object* self) {
    Trigger_Data* objdata;
    Trigger_Setup* setup;
    Object* player;
    Object* vehicle;
    Object* sidekick;
    Object* activatorObj;
    s32 i;
    s32 b_allBitsSet;
    s32 b_foundActivatorObj;
    f32 maxObjSearchDist;

    objdata = (Trigger_Data*)self->data;
    setup = (Trigger_Setup*)self->setup;
    
    maxObjSearchDist = 200.0f;
   
    player = get_player();
    if (player != NULL) {
        vehicle = ((DLL_210_Player*)player->dll)->vtbl->get_vehicle(player);
        if (vehicle != NULL) {
            player = vehicle;
        }
    }
    
    sidekick = get_sidekick();
    
    if ((player != NULL) || (sidekick != NULL)) {
        if (objdata->flags & TRG_RESTORE_ENTERED_STATE) {
            trigger_process_commands(self, player, 1, 0);
            objdata->flags &= ~TRG_RESTORE_ENTERED_STATE;
            objdata->flags |= TRG_ACTIVATOR_ENTERED;
            return;
        }
        
        b_foundActivatorObj = TRUE;
        if (setup->activatorObjType >= 3) {
            activatorObj = obj_get_nearest_type_to(setup->activatorObjType, self, &maxObjSearchDist);
            if (activatorObj == NULL) {
                b_foundActivatorObj = FALSE;
            }
        } else {
            switch (setup->activatorObjType) {
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
                activatorObj = (Object*)gDLL_2_Camera->vtbl->get_data();
                break;
            }
        }
        
        if (b_foundActivatorObj) {
            if (objdata->flags & TRG_FIRST_TICK) {
                switch (setup->activatorObjType) {
                default:
                    objdata->activatorPrevPos.x = activatorObj->prevLocalPosition.x;
                    objdata->activatorPrevPos.y = activatorObj->prevLocalPosition.y;
                    objdata->activatorPrevPos.z = activatorObj->prevLocalPosition.z;
                    break;
                case 2:
                    // Camera
                    objdata->activatorPrevPos.x = activatorObj->srt.transl.x;
                    objdata->activatorPrevPos.y = activatorObj->srt.transl.y;
                    objdata->activatorPrevPos.z = activatorObj->srt.transl.z;
                    break;
                case 0:
                case 1:
                    // Player/sidekick
                    objdata->activatorPrevPos.x = activatorObj->prevGlobalPosition.x;
                    objdata->activatorPrevPos.y = activatorObj->prevGlobalPosition.y;
                    objdata->activatorPrevPos.z = activatorObj->prevGlobalPosition.z;
                    break;
                }
                
                objdata->flags &= ~TRG_FIRST_TICK;
            } else {
                objdata->activatorPrevPos.x = objdata->activatorCurrPos.x;
                objdata->activatorPrevPos.y = objdata->activatorCurrPos.y;
                objdata->activatorPrevPos.z = objdata->activatorCurrPos.z;
            }
            
            switch (setup->activatorObjType) {
            case 0:
            case 1:
                // Player/sidekick
                objdata->activatorCurrPos.x = activatorObj->globalPosition.x;
                objdata->activatorCurrPos.y = activatorObj->globalPosition.y;
                objdata->activatorCurrPos.z = activatorObj->globalPosition.z;
                break;
            default:
            case 2:
                // Camera/other
                objdata->activatorCurrPos.x = activatorObj->srt.transl.x;
                objdata->activatorCurrPos.y = activatorObj->srt.transl.y;
                objdata->activatorCurrPos.z = activatorObj->srt.transl.z;
                break;
            }
        }
        
        switch (setup->base.objId) {
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
            if (objdata->conditionBitFlagIDs[0] >= 0) {
                // @recomp: Dino Mod condition bit extension for checking unset flags (originally by MusicalProgrammer)
                if (objdata->conditionBitFlagIDs[0] & 0x4000) {
                    if (main_get_bits(objdata->conditionBitFlagIDs[0] & ~0x4000) != 0) {
                        b_allBitsSet = FALSE;
                    }
                } else {
                    if (main_get_bits(objdata->conditionBitFlagIDs[0]) == 0) {
                        b_allBitsSet = FALSE;
                    }
                }
            }
            if (b_allBitsSet && b_foundActivatorObj) {
                trigger_plane_update(self, activatorObj);
            }
            break;
        case OBJ_TriggerTime:
            objdata->elapsedTicks += gUpdateRate;
            if (objdata->elapsedTicks >= setup->timerDuration) {
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
                if (objdata->conditionBitFlagIDs[i] >= 0) {
                    if (main_get_bits(objdata->conditionBitFlagIDs[i]) == 0) {
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
    gMapLayer = layer;
    if (gMapLayer >= 3) {
        gMapLayer = 2;
    }
    if (gMapLayer < -2) {
        gMapLayer = -2;
    }
    UINT_80092a98 |= 0x4000;
}

RECOMP_PATCH void trigger_process_commands(Object *self, Object *activator, s8 dir, s32 activatorDistSquared) {
    Trigger_Setup* setup; // sp+74
    Trigger_Data* objdata; // sp+70
    s32 pad;
    TriggerCommand *cmd;
    s32 temp_a1;
    u8 i;
    Object* var_v0_2;
    Object* sidekick;

    objdata = (Trigger_Data*)self->data;
    setup = (Trigger_Setup*)self->setup;
    
    for (i = 0, cmd = setup->commands; i < 8; i++, cmd++) {
        if (cmd->id == 0) {
            // No command assigned to this slot
            continue;
        }

        if ((objdata->flags & TRG_RESTORE_ENTERED_STATE) && !(cmd->condition & CMD_COND_RESTORE)) {
            continue;
        }
        
        if (!(cmd->condition & CMD_COND_CONTINUOUS)) {
            // Not continuous, check for entry/exit tick
            if (dir == 1) {
                // Entered
                if (!(cmd->condition & CMD_COND_IN)) {
                    continue;
                }
                if ((objdata->flags & TRG_ACTIVATOR_ENTERED) && !(cmd->condition & CMD_COND_RE_ENTER)) {
                    continue;
                }
            } else if (dir == -1) {
                // Exited
                if (!(cmd->condition & CMD_COND_OUT)) {
                    continue;
                }
                if ((objdata->flags & TRG_ACTIVATOR_EXITED) && !(cmd->condition & CMD_COND_RE_EXIT)) {
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
            switch (cmd->param1) {
            Object *obj;
            case 0:
            case 1:
            case 2:
            case 3:
            case 4:
            case 5:
            case 6:
                break;
            case 7: {
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
            case 8: {
                    // "Trigger [%d], Death drop" (default.dol)
                    s16 pad;
                    obj = get_player();
                    if (obj != NULL) {
                        ((DLL_210_Player*)obj->dll)->vtbl->func67(obj, 9, 0.0f);
                    }
                }
                break;
            case 9: {
                    // "Trigger [%d], Dangerous Water" (default.dol)
                    s16 pad;
                    obj = get_player();
                    if (obj != NULL) {
                        ((DLL_210_Player*)obj->dll)->vtbl->func67(obj, 10, 0.0f);
                    }
                }
                break;
            case 10: {
                    // "Trigger [%d], Safe Water" (default.dol)
                    s32 pad;
                    obj = get_player();
                    if (obj != NULL) {
                        ((DLL_210_Player*)obj->dll)->vtbl->func67(obj, 11, 0.0f);
                    }
                }
                break;
            }
            break;
        case TRG_CMD_MUSIC_ACTION: 
            if ((dir < 0) && (gDLL_5_AMSEQ->vtbl->is_set(self, (cmd->param2 | (cmd->param1 << 8))) != 0)) {
                // "Trigger [%d], Music Action,       Action Num [%d] Free"
                gDLL_5_AMSEQ2->vtbl->free(self, (cmd->param2 | (cmd->param1 << 8)), 0, 0, 0);
            } else {
                // "Trigger [%d], Music Action,       Action Num [%d] Set"
                gDLL_5_AMSEQ2->vtbl->set(self, (cmd->param2 | (cmd->param1 << 8)), 0, 0, 0);
            }
            break;
        case TRG_CMD_SOUND: 
            // "Trigger [%d], Sound FX,           Action Num [%d],Handle Num [%d]"
            if (dir >= 0) {
                gDLL_6_AMSFX->vtbl->func_10D0(self, (cmd->param2 | (cmd->param1 << 8)), &objdata->soundHandles[i]);
            } else {
                if (objdata->soundHandles[i] != 0) {
                    gDLL_6_AMSFX->vtbl->func_A1C(objdata->soundHandles[i]);
                    objdata->soundHandles[i] = 0;
                }
            }
            break;
        case TRG_CMD_CAMERA_ACTION: 
            // "Trigger [%d], Camera,             Action [%d], Camera Num [%d], PassDir [%d]"
            gDLL_2_Camera->vtbl->change_mode(cmd->param1, cmd->param2);
            break;
        case TRG_CMD_TRACK: 
            switch (cmd->param1) {
            case 0:
                if ((s32) cmd->param2 >= 2) {
                    cmd->param2 = 1;
                }
                func_80041C6C(cmd->param2);
                if (cmd->param2 != 0) {
                    // "Trigger [%d], Track Sky On"
                } else {
                    // "Trigger [%d], Track Sky Off"
                }
                break;
            case 1:
                if ((s32) cmd->param2 >= 2) {
                    cmd->param2 = 1;
                }
                func_80041CA8((s32) cmd->param2);
                if (cmd->param2 != 0) {
                    // "Trigger [%d], Track AntiAlias On"
                } else {
                    // "Trigger [%d], Track AntiAlias Off"
                }
                break;
            case 2:
                if ((s32) cmd->param2 >= 2) {
                    cmd->param2 = 1;
                }
                func_80041CE4((s32) cmd->param2);
                if (cmd->param2 != 0) {
                    // "Trigger [%d], Track SkyObjects On"
                } else {
                    // "Trigger [%d], Track SkyObjects Off"
                }
                break;
            case 3:
                if ((s32) cmd->param2 >= 2) {
                    cmd->param2 = 1;
                }
                gDLL_12_Minic->vtbl->func6(cmd->param2);
                if (cmd->param2 != 0) {
                    // "Trigger [%d], Track Dome On"
                } else {
                    // "Trigger [%d], Track Dome Off"
                }
                break;
            case 4:
                gDLL_16->vtbl->func2(cmd->param2);
                if (cmd->param2 != 0) {
                    // "Trigger [%d], Track MrSheen On %d"
                } else {
                    // "Trigger [%d], Track MrSheen Off"
                }
                break;
            case 5:
                footsteps_toggle((u32) cmd->param2);
                // "Trigger [%d], footstepsTurnOn %d" (default.dol)
                break;
            case 6:
                if ((s32) cmd->param2 > 0) {
                    func_8001EBD0(1);
                    // "Trigger [%d], newlightInside(1)" (default.dol)
                } else {
                    func_8001EBD0(0);
                    // "Trigger [%d], newlightInside(0)" (default.dol)
                }
                break;
            case 7:
                if ((s32) cmd->param2 > 0) {
                    func_80041E24(1);
                    // "Trigger [%d], trackSetSunGlareOn(1)" (default.dol)
                } else {
                    func_80041E24(0);
                    // "Trigger [%d], trackSetSunGlareOn(0)" (default.dol)
                }
                break;
            }
            break;
        case TRG_CMD_5: 
            if ((objdata->radiusSquared != 0.0f) && (activatorDistSquared != 0) && (setup->base.objId == OBJ_TriggerPlane)) {

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
        case TRG_CMD_STORYBOARD:
            // "Storyboard disabled, please remove trigger\n"
            break;
        case TRG_CMD_LOD_MODEL:
            // "Trigger [%d], LOD Model [%d]"
            obj_set_model(get_player(), (s32) cmd->param1);
            break;
        case TRG_CMD_SETUP_POINT:
            // "Trigger [%d], Setup Point,        Level      [%d], SetupPoint [%d]"
            trigger_func_1754(cmd->param1, cmd->param2);
            break;
        case TRG_CMD_FLAG:
            // "Trigger [%d], Bits\n"
            // "Trigger [%d], Bits No %d \n" (default.dol)
            trigger_func_1764((cmd->param2 | (cmd->param1 << 8)));
            break;
        case TRG_CMD_FLAG_TOGGLE:
            // "Trigger [%d], toggleBits (%d)"(default.dol)
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
            // "Trigger [%d], act changed to %d" (default.dol)
            gDLL_29_Gplay->vtbl->set_map_setup((s32) self->mapID, cmd->param2 | (cmd->param1 << 8));
            break;
        case TRG_CMD_SCRIPT:
            if (objdata->scripts[i] != NULL) {
                objdata->scripts[i]->vtbl->subscripts[cmd->param2](self, activator, dir, activatorDistSquared);
            } else {
                // "TRIGGER: warning DLL not loaded\n"
            }
            // "Script [%d], Subscript [%d]\n"
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
            // "Trigger [%d], kyte flight group change\n" (default.dol)
            main_set_bits(BIT_Kyte_Flight_Curve, cmd->param2 | (cmd->param1 << 8));
            break;
        case TRG_CMD_KYTE_TALK_SEQ:
            // "Trigger [%d], kyte flight talk sequence set\n" (default.dol)
            main_set_bits(BIT_488, cmd->param2 | (cmd->param1 << 8));
            break;
        case TRG_CMD_WORLD_SET_MAP_SETUP:
            // "Trigger [%d], Act change on map %d to act %d\n" (default.dol)
            gDLL_29_Gplay->vtbl->set_map_setup((s32) cmd->param2, (s32) cmd->param1);
            break;
        case TRG_CMD_TRICKY_TALK_SEQ:
            // "Trigger [%d], Tricky talk sequence set to %d\n" (default.dol)
            main_set_bits(BIT_4E2, cmd->param2 | (cmd->param1 << 8));
            break;
        case TRG_CMD_SAVE_GAME:
            // "Trigger [%d], Save Point\n" (default.dol)
            gDLL_29_Gplay->vtbl->checkpoint(&self->srt.transl, (s16) ((s16) self->srt.yaw >> 8), (s32) cmd->param2, map_get_layer());
            break;
        case TRG_CMD_MAP_LAYER:
            // @recomp: Dino Mod trigger mapLayer command extension (originally by MusicalProgrammer)
            if ((s8)cmd->param2 == -1) {
                set_map_layer(cmd->param1);
            } else {
                if (cmd->param1 == 0) {
                    // "Trigger [%d],trackIncMapLayer\n" (default.dol)
                    map_increment_layer();
                } else {
                    // "Trigger [%d],trackIncMapLayer\n" (default.dol)
                    map_decrement_layer();
                }
            }
            break;
        case TRG_CMD_RESTART:
            switch (cmd->param1) {
            case 0:
                // "Restart Set [%d]\n"
                gDLL_29_Gplay->vtbl->restart_set(&self->srt.transl, self->srt.yaw, map_get_layer());
                break;
            case 1:
                // "Restart Clear [%d]\n"
                gDLL_29_Gplay->vtbl->restart_clear();
                break;
            case 2:
                // "Restart Goto [%d]\n"
                gDLL_29_Gplay->vtbl->restart_goto();
                break;
            /*
            // default.dol
            case 3:
                // "Trigger [%d],Restart Set Dazed [%d]\n"
                gDLL_29_Gplay->vtbl->restart_set(&self->srt.transl, self->srt.yaw, map_get_layer(), 1);
                break;
            */
            }
            break;
        case TRG_CMD_SIDEKICK:
            sidekick = get_sidekick();
            if (sidekick != NULL) {
                switch (cmd->param1) {
                case 0:
                    // "Trigger [%d], Sidekick Auto Heel\n" (default.dol)
                    ((DLL_ISidekick *)sidekick->dll)->vtbl->func23(sidekick);
                    break;
                case 1:
                    // "killing sidekick\n"
                    // "Trigger [%d], Unloading Sidekick\n" (default.dol)
                    obj_destroy_object(get_sidekick());
                    break;
                case 2:
                    // "findobj %i \n"
                    var_v0_2 = obj_get_nearest_type_to(OBJTYPE_52, sidekick, NULL);
                    if (var_v0_2 == NULL) {
                        var_v0_2 = obj_get_nearest_type_to(OBJTYPE_51, sidekick, NULL);
                    }
                    if (var_v0_2 != NULL) {
                        // "Trigger [%d], Sidekick Find On Object %d\n"
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
        objdata->flags |= TRG_ACTIVATOR_ENTERED;
        main_set_bits(objdata->bitFlagID, 1);
    } else if (dir < 0) {
        // Out
        objdata->flags |= TRG_ACTIVATOR_EXITED;
    }
}

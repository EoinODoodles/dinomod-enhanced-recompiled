#pragma once

#include "PR/gbi.h"
#include "types.h"
#include "dlls/objects/325_trigger.h"

/* TRANSFORMS */

#define TRIGGER_YAW(degrees) ((u8)((float)degrees*0x10/90.0f + 0.5f)) //Yaw for TriggerPlanes etc. (other axes use DEGREES_TO_ANGLE8)
#define TRIGGER_SCALE(scaleFloat) ((u8)(scaleFloat*0x10 + 0.5f))


/* COMMANDS */

//A shortcut for emptying out one of a Trigger Object's command slots
#define EMPTY_TRIGGER_COMMAND(triggerObject, cmdSlot)\
    (triggerObject)->commands[cmdSlot].condition = 0;\
    (triggerObject)->commands[cmdSlot].id = 0;\
    (triggerObject)->commands[cmdSlot].paramCombined = 0;


/* COMMANDS: OBJECT GROUPS */

//A shortcut for switching on an objectGroup when entering a Trigger Object
#define ENTER_OBJGROUP_ON(groupID, triggerObject, cmdSlot)\
    (triggerObject)->commands[cmdSlot].condition = CMD_COND_IN | CMD_COND_RE_ENTER;\
    (triggerObject)->commands[cmdSlot].id = TRG_CMD_ENABLE_OBJ_GROUP;\
    (triggerObject)->commands[cmdSlot].paramCombined = groupID;

//A shortcut for switching off an objectGroup when entering a Trigger Object
#define ENTER_OBJGROUP_OFF(groupID, triggerObject, cmdSlot)\
    (triggerObject)->commands[cmdSlot].condition = CMD_COND_IN | CMD_COND_RE_ENTER;\
    (triggerObject)->commands[cmdSlot].id = TRG_CMD_DISABLE_OBJ_GROUP;\
    (triggerObject)->commands[cmdSlot].paramCombined = groupID;

//A shortcut for switching on an objectGroup when exiting a Trigger Object
#define EXIT_OBJGROUP_ON(groupID, triggerObject, cmdSlot)\
    (triggerObject)->commands[cmdSlot].condition = CMD_COND_OUT | CMD_COND_RE_EXIT;\
    (triggerObject)->commands[cmdSlot].id = TRG_CMD_ENABLE_OBJ_GROUP;\
    (triggerObject)->commands[cmdSlot].paramCombined = groupID;

//A shortcut for switching off an objectGroup when exiting a Trigger Object
#define EXIT_OBJGROUP_OFF(groupID, triggerObject, cmdSlot)\
    (triggerObject)->commands[cmdSlot].condition = CMD_COND_OUT | CMD_COND_RE_EXIT;\
    (triggerObject)->commands[cmdSlot].id = TRG_CMD_DISABLE_OBJ_GROUP;\
    (triggerObject)->commands[cmdSlot].paramCombined = groupID;

//A quick way to add directionally specific ObjectGroup on/off commands to Trigger Objects (enter: on, exit: off)
#define DIRECTIONAL_OBJGROUP_TOGGLE(groupID, triggerObject, cmdSlotIn, cmdSlotOut)\
    ENTER_OBJGROUP_ON(groupID, triggerObject, cmdSlotIn)\
    EXIT_OBJGROUP_OFF(groupID, triggerObject, cmdSlotOut);

//A quick way to add directionally specific ObjectGroup off/on commands to Trigger Objects (enter: off, exit: on)
#define DIRECTIONAL_OBJGROUP_TOGGLE_REVERSE(groupID, triggerObject, cmdSlotIn, cmdSlotOut)\
    ENTER_OBJGROUP_OFF(groupID, triggerObject, cmdSlotIn)\
    EXIT_OBJGROUP_ON(groupID, triggerObject, cmdSlotOut);


/* COMMANDS: WORLD OBJECT GROUPS */

//A shortcut for switching on a worldObjectGroup when entering a Trigger Object
#define ENTER_WORLD_OBJGROUP_ON(groupID, mapID, triggerObject, cmdSlot)\
    (triggerObject)->commands[cmdSlot].condition = CMD_COND_IN | CMD_COND_RE_ENTER;\
    (triggerObject)->commands[cmdSlot].id = TRG_CMD_WORLD_ENABLE_OBJ_GROUP;\
    (triggerObject)->commands[cmdSlot].param1 = groupID;\
    (triggerObject)->commands[cmdSlot].param2 = mapID;

//A shortcut for switching off a worldObjectGroup when entering a Trigger Object
#define ENTER_WORLD_OBJGROUP_OFF(groupID, mapID, triggerObject, cmdSlot)\
    (triggerObject)->commands[cmdSlot].condition = CMD_COND_IN | CMD_COND_RE_ENTER;\
    (triggerObject)->commands[cmdSlot].id = TRG_CMD_WORLD_DISABLE_OBJ_GROUP;\
    (triggerObject)->commands[cmdSlot].param1 = groupID;\
    (triggerObject)->commands[cmdSlot].param2 = mapID;

//A shortcut for switching on a worldObjectGroup when exiting a Trigger Object
#define EXIT_WORLD_OBJGROUP_ON(groupID, mapID, triggerObject, cmdSlot)\
    (triggerObject)->commands[cmdSlot].condition = CMD_COND_OUT | CMD_COND_RE_EXIT;\
    (triggerObject)->commands[cmdSlot].id = TRG_CMD_WORLD_ENABLE_OBJ_GROUP;\
    (triggerObject)->commands[cmdSlot].param1 = groupID;\
    (triggerObject)->commands[cmdSlot].param2 = mapID;

//A shortcut for switching off a worldObjectGroup when exiting a Trigger Object
#define EXIT_WORLD_OBJGROUP_OFF(groupID, mapID, triggerObject, cmdSlot)\
    (triggerObject)->commands[cmdSlot].condition = CMD_COND_OUT | CMD_COND_RE_EXIT;\
    (triggerObject)->commands[cmdSlot].id = TRG_CMD_WORLD_DISABLE_OBJ_GROUP;\
    (triggerObject)->commands[cmdSlot].param1 = groupID;\
    (triggerObject)->commands[cmdSlot].param2 = mapID;

//A quick way to add directionally specific World ObjectGroup on/off commands to Trigger Objects (enter: on, exit: off)
#define DIRECTIONAL_WORLD_OBJGROUP_TOGGLE(groupID, mapID, triggerObject, cmdSlotIn, cmdSlotOut)\
    ENTER_WORLD_OBJGROUP_ON(groupID, mapID, triggerObject, cmdSlotIn)\
    EXIT_WORLD_OBJGROUP_OFF(groupID, mapID, triggerObject, cmdSlotOut);

//A quick way to add directionally specific World ObjectGroup off/on commands to Trigger Objects (enter: off, exit: on)
#define DIRECTIONAL_WORLD_OBJGROUP_TOGGLE_REVERSE(groupID, mapID, triggerObject, cmdSlotIn, cmdSlotOut)\
    ENTER_WORLD_OBJGROUP_OFF(groupID, mapID, triggerObject, cmdSlotIn)\
    EXIT_WORLD_OBJGROUP_ON(groupID, mapID, triggerObject, cmdSlotOut);

    
/* COMMANDS: GAMEBITS */
#define TRG_GAMEBIT_ON(gamebit) ((gamebit & 0x3FFF) | (1 << 14))
#define TRG_GAMEBIT_OFF(gamebit) (gamebit & 0x3FFF)
#define TRG_GAMEBIT(gamebit, enable) ((gamebit & 0x3FFF) | ((enable == TRUE) << 14))

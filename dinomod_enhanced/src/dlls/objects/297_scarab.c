#include "modding.h"
#include "recomputils.h"

#include "dll.h"
#include "sys/dll.h"
#include "game/objects/object.h"
#include "game/objects/object_id.h"
#include "sys/main.h"
#include "sys/objects.h"
#include "sys/objtype.h"

#include "dlls/engine/6_amsfx.h"
#include "dlls/objects/210_player.h"
#include "recomp/dlls/objects/297_scarab_recomp.h"

typedef struct {
/*00*/ u32 soundHandle;         //Manages crawling sound loop
/*04*/ f32 speedX;              //Scurry speed
/*08*/ f32 speedZ;              //Scurry speed
/*0C*/ f32 goldClimbTimer;      //Amount of time Gold Scarab has been climbing wall
/*10*/ f32 initialY;            //Y value from objSetup - used as fallback height
/*14*/ s16 destructDelayTimer;  //Used to delay deletion when collected
/*16*/ s16 unused16;
/*18*/ s16 lifetime;            //Decrements while scurrying, Scarab disappears when 0
/*1A*/ s16 rollSpeed;           //Rate of change of roll
/*1C*/ s16 scurryInitialYaw;    //Reference yaw
/*1E*/ s16 stunTimer;           //Scarab can't continue scurrying while active. Rainbow Scarabs can only be collected while stunned!
/*20*/ s16 goldClimbDuration;   //Amount of time Gold Scarab should climb wall before stopping (randomised)
/*22*/ s16 collectSoundID;      //Sound to play when collected
/*24*/ s16 collectFXScale;      //Size of effect created when collected
/*26*/ u8 state;                //State machine value
/*27*/ s8 unused27;
/*27*/ s8 unused28;
/*29*/ u8 scarabTypeIndex;      //0) Green, 1) Red, 2) Gold, 3) Rainbow
/*2A*/ u8 flags;                //Receive messages from other objects
} Scarab_Data;

// Enables the Scarab UI counter upon collection
RECOMP_PATCH void scarab_collect(Object* self, Object* player, Scarab_Data* objData) {
	/** The amount of currency each Scarab type adds to the player's counter */
	u8 values[4] = {1, 5, 10, 50};

	//@recomp: enable Scarab counter UI
	main_set_bits(0x919, 1);

	((DLL_210_Player*)player->dll)->vtbl->add_scarab(player, values[objData->scarabTypeIndex]);

	objData->destructDelayTimer = 80;
	objData->lifetime = 0;
}

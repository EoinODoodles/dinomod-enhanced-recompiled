// #include "PR/ultratypes.h"
// #include "functions.h"
// #include "modding.h"

// #include "game/objects/object.h"
// #include "game/gamebits.h"
// #include "recomputils.h"
// #include "sys/gfx/map.h"
// #include "sys/main.h"
// #include "sys/map_enums.h"
// #include "sys/objects.h"
// #include "sys/objtype.h"
// #include "dll.h"
// #include "dlls/objects/210_player.h"

// #include "recomp/dlls/objects/718_PerchObject_recomp.h"

// typedef struct {
// /*00*/	s16 objId;
// /*02*/	u8 quarterSize;
// /*03*/	u8 act;
// /*04*/	u8 nextCurveGroup;
// /*05*/	u8 prevCurveGroup;
// /*06*/	u8 branch1CurveGroup;
// /*07*/	u8 branch2CurveGroup;
// /*08*/	f32 x;
// /*0c*/	f32 y;
// /*10*/	f32 z;
// /*14*/	s32 uID;
// } CurveCreateInfoBase; //curves use the base objCreateInfo fields differently, so creating a unique base struct

// typedef struct {
// /*00*/ CurveCreateInfoBase base;
// /*18*/ s8 curveType; //2) KTrex, 3) RedEye, 1A) camera?, 1B) camera?, 1D) ThornTail, 1F) crawlSpace, 22) Kyte, 24) Tricky
// /*1C*/ s32 unk1C;
// /*20*/ s32 nextCurveUID;
// /*24*/ s32 prevCurveUID;
// /*28*/ s32 branch1UID;
// /*2C*/ s32 branch2UID;
// /*30*/ s8 yaw;
// /*31*/ s8 pitch;
// /*32*/ s16 usedBit; //gameBit
// /*34*/ s16 unk34; //gameBit?
// /*36*/ s16 unk36;
// /*38*/ s16 unk38; //gameBit?
// } KyteCurveCreateInfo;

// typedef struct {
// /*00*/ ObjCreateInfo base;
// /*18*/ u8 interactionDistance;
// /*19*/ u8 unk19;
// /*1A*/ u16 kyteFlightGroup; //curve group Kyte should traverse when using "Find" command
// /*1C*/ s16 unk1C;
// /*1E*/ u8 useDistance3D; //affects how player-to-perch distance is calculated (2D X/Z vs. full 3D)
// } PerchObjectCreateInfo;

// typedef struct {
//     s8 stateIndex;
//     KyteCurveCreateInfo* curveCreate;
// } PerchObjectState;

// /** Fixes a crash in Cape Claw's perches for Kyte (originally by MusicalProgrammer) */
// RECOMP_PATCH s32 perchObject_callbackBC(Object* self, s32 arg1, s32 arg2, s32 arg3) {
//     s32 pad;
//     Object* kyte;
//     Object* player;
//     PerchObjectState* state;
//     PerchObjectCreateInfo* createInfo;

//     createInfo = (PerchObjectCreateInfo*)self->createInfo;

//     //@recomp: bail if the PerchObject's curveCreateInfo is missing
//     state = self->state;
//     if (!state->curveCreate){
//         return 0;
//     }

//     //@recomp: bail if Kyte is missing
//     kyte = get_sidekick();
//     if (!kyte){
//         return 0;
//     }

//     //@recomp: bail if Krystal is missing
//     player = get_player();
//     if (!player){
//         return 0;
//     }

//     if (vec3_distance_squared(&player->positionMirror, (Vec3f*)&(state->curveCreate)->base.x) <= (createInfo->interactionDistance * createInfo->interactionDistance)) {
//         ((DLL_Unknown*)kyte->dll)->vtbl->func[14].withTwoArgs((s32)kyte, 1);
//         if (gDLL_1_UI->vtbl->func7(1)) {
//             main_set_bits(BIT_Kyte_Flight_Curve, createInfo->kyteFlightGroup);
//         }
//     }
//     return 0;
// }

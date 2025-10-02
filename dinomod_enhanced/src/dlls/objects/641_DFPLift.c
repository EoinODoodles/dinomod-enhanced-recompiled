#include "modding.h"

#include "custom_object_ids.h"

#include "game/objects/object.h"
#include "sys/main.h"
#include "sys/objects.h"
#include "dll.h"

typedef struct {
/*0x00*/ ObjCreateInfo base;
/*0x18*/ s8 rotation;
/*0x1A*/ s16 unk1A;
/*0x1C*/ s16 unk1C;
/*0x1E*/ s16 unk1E;
/*0x20*/ s16 unk20;
} DFPLiftCreateInfo;

typedef struct {
/*0x00*/ f32 unk0;
/*0x04*/ u32 soundHandle;
/*0x08*/ s16 stateType;
/*0x0A*/ s16 unkA;
/*0x0C*/ s16 unkC;
/*0x0E*/ s16 cooldown;
/*0x10*/ u8 unk10;
/*0x11*/ u8 unk11;
} DFPLiftState;

typedef enum {
    LIFT_STATE_INIT = 0,
    LIFT_STATE_INIT_DONE = 1,
    LIFT_STATE_STOPPED = 2,
    LIFT_STATE_GO_DOWN = 3,
    LIFT_STATE_GO_UP = 4
} LiftStateType;

#define LIFT_UP 60.0f // how far up from spawn the lift goes
#define LIFT_DOWN 1228.0f // how far down from spawn the lift goes
#define LIFT_COOLDOWN 100
#define PLAYER_ACTIVATE_RANGE 50.0f
#define PLAYER_INIT_ACTIVATE_RANGE 230.0f

#include "recomp/dlls/objects/641_DFPLift_recomp.h"

extern s32 DFPLift_func_91C(Object *, Object *, void *, void *);

static f32 dinomod_get_lift_down(Object *self) {
    // @recomp: Use different 'down' value for the custom KPLift1 object
    if (self->id == OBJ_KPLift1) {
        return 900.0f;
    } else {
        return LIFT_DOWN;
    }
}

RECOMP_PATCH void DFPLift_create(Object *self, DFPLiftCreateInfo *createInfo, s32 a2) {
    DFPLiftState *state;

    state = (DFPLiftState*)self->state;

    self->unk0xbc = DFPLift_func_91C;
    self->srt.yaw = createInfo->rotation * 256;

    state->stateType = LIFT_STATE_INIT;
    state->unkA = createInfo->unk20;
    state->unkC = createInfo->unk1E;
    state->unk0 = createInfo->unk1A;
    state->unk10 = createInfo->unk1C;

    // @recomp: Get LIFT_DOWN depending on object ID
    self->srt.transl.y -= dinomod_get_lift_down(self);
    self->unk0xb0 |= 0x2000;
}

RECOMP_PATCH void DFPLift_update(Object* self) {
    DFPLiftCreateInfo* createInfo;
    DFPLiftState* state;
    Object* player;

    createInfo = (DFPLiftCreateInfo*)self->createInfo;
    state = (DFPLiftState*)self->state;
    
    player = get_player();
    if (player == NULL) {
        return;
    }
    
    switch (state->stateType) {
    case LIFT_STATE_INIT:
        if ((get_gplay_bitstring(state->unkA) != 0) && (state->unk10 != 1) && 
                (vec3_distance_xz(&self->positionMirror, &player->positionMirror) < PLAYER_INIT_ACTIVATE_RANGE)) {
            // go up with sound
            if (self->srt.transl.y < (createInfo->base.y + LIFT_UP)) {
                if (state->soundHandle == 0) {
                    gDLL_6_AMSFX->vtbl->play_sound(self, 0x6EC, 0x75, &state->soundHandle, NULL, 0, NULL);
                    state->unk11 = 1;
                }
                self->srt.transl.y += delayFloat;
                if ((createInfo->base.y + LIFT_UP) <= self->srt.transl.y) {
                    self->srt.transl.y = (createInfo->base.y + LIFT_UP);
                    state->stateType = LIFT_STATE_INIT_DONE;
                    if (state->soundHandle != 0) {
                        gDLL_6_AMSFX->vtbl->func_A1C(state->soundHandle);
                        state->soundHandle = 0;
                    }
                }
                return;
            }
        } else if (state->unk10 == 1) {
            if (vec3_distance_xz(&self->positionMirror, &player->positionMirror) < PLAYER_INIT_ACTIVATE_RANGE) {
                // go up without sound
                if (self->srt.transl.y < (createInfo->base.y + LIFT_UP)) {
                    self->srt.transl.y += delayFloat;
                    if ((createInfo->base.y + LIFT_UP) <= self->srt.transl.y) {
                        self->srt.transl.y = (createInfo->base.y + LIFT_UP);
                        state->stateType = LIFT_STATE_INIT_DONE;
                    }
                }
            }
        }
        break;
    case LIFT_STATE_INIT_DONE:
        state->stateType = LIFT_STATE_STOPPED;
        state->cooldown = LIFT_COOLDOWN;
        break;
    case LIFT_STATE_STOPPED:
        // stopped
        if (state->cooldown != 0) {
            state->cooldown -= (s16)delayFloat;
            if (state->cooldown <= 0) {
                state->cooldown = 0;
            }
        } else {
            if (vec3_distance_xz(&self->positionMirror, &player->positionMirror) < PLAYER_ACTIVATE_RANGE) {
                // player is on lift, start moving
                if (self->srt.transl.y == (createInfo->base.y + LIFT_UP)) {
                    // at top, start going down
                    state->stateType = LIFT_STATE_GO_DOWN;
                    if (state->soundHandle == 0) {
                        gDLL_6_AMSFX->vtbl->play_sound(self, 0x6EC, 0x6B, &state->soundHandle, NULL, 0, NULL);
                        state->unk11 = 1;
                    }
                // @recomp: Get LIFT_DOWN depending on object ID
                } else if (self->srt.transl.y == (createInfo->base.y - dinomod_get_lift_down(self))) {
                    // at bottom, start going up
                    state->stateType = LIFT_STATE_GO_UP;
                    if (state->soundHandle == 0) {
                        gDLL_6_AMSFX->vtbl->play_sound(self, 0x6EC, 0x43, &state->soundHandle, NULL, 0, NULL);
                        state->unk11 = 1;
                    }
                }
            } else {
                // player is not on the lift, move to where they are
                if (player->srt.transl.y < createInfo->base.y) {
                    state->stateType = LIFT_STATE_GO_DOWN;
                    if (state->unk11 == 1) {
                        state->unk11 = 0;
                    }
                } else if (createInfo->base.y < player->srt.transl.y) {
                    state->stateType = LIFT_STATE_GO_UP;
                    if (state->unk11 == 1) {
                        state->unk11 = 0;
                    }
                }
            }
        }
        break;
    case LIFT_STATE_GO_DOWN:
        // going down
        // @recomp: Get LIFT_DOWN depending on object ID
        if ((createInfo->base.y - dinomod_get_lift_down(self)) < self->srt.transl.y) {
            self->srt.transl.y -= delayFloat;
            if (self->srt.transl.y <= (createInfo->base.y - dinomod_get_lift_down(self))) {
                // reached bottom
                self->srt.transl.y = (createInfo->base.y - dinomod_get_lift_down(self));
                state->stateType = LIFT_STATE_STOPPED;
                if (state->soundHandle != 0) {
                    gDLL_6_AMSFX->vtbl->func_A1C(state->soundHandle);
                    state->soundHandle = 0;
                }
                state->cooldown = LIFT_COOLDOWN;
            }
            if ((vec3_distance_xz(&self->positionMirror, &player->positionMirror) < PLAYER_ACTIVATE_RANGE) && (state->unk11 == 1)) {
                
            }
        } else {
            // already at bottom
            if (state->soundHandle != 0) {
                gDLL_6_AMSFX->vtbl->func_A1C(state->soundHandle);
                state->soundHandle = 0;
            }
            vec3_distance_xz(&self->positionMirror, &player->positionMirror);
            state->stateType = LIFT_STATE_STOPPED;
            state->cooldown = LIFT_COOLDOWN;
        }
        break;
    case LIFT_STATE_GO_UP:
        // going up
        if (self->srt.transl.y < (createInfo->base.y + LIFT_UP)) {
            self->srt.transl.y += delayFloat;
            if ((createInfo->base.y + LIFT_UP) <= self->srt.transl.y) {
                // reached top
                self->srt.transl.y = (createInfo->base.y + LIFT_UP);
                state->stateType = LIFT_STATE_STOPPED;
                state->cooldown = LIFT_COOLDOWN;
                if (state->soundHandle != 0) {
                    gDLL_6_AMSFX->vtbl->func_A1C(state->soundHandle);
                    state->soundHandle = 0;
                }
            }
            if ((vec3_distance_xz(&self->positionMirror, &player->positionMirror) < PLAYER_ACTIVATE_RANGE) && (state->unk11 == 1)) {
                
            }
        } else {
            // already at top
            state->stateType = LIFT_STATE_STOPPED;
            state->cooldown = LIFT_COOLDOWN;
            if (state->soundHandle != 0) {
                gDLL_6_AMSFX->vtbl->func_A1C(state->soundHandle);
                state->soundHandle = 0;
            }
            if ((vec3_distance_xz(&self->positionMirror, &player->positionMirror) < PLAYER_ACTIVATE_RANGE) && (state->unk11 == 1)) {

            }
        }
        break;
    default:
        break;
    }
}

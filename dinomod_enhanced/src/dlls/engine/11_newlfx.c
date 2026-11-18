#include "modding.h"
#include "recomputils.h"

#include "dlls/engine/11_newlfx.h"
#include "dlls/engine/29_gplay.h"
#include "sys/lfx.h"
#include "sys/lighting.h"
#include "sys/lighting_asm.h"
#include "sys/main.h"
#include "sys/memory.h"
#include "sys/newshadows.h"
#include "sys/objects.h"
#include "sys/print.h"
#include "sys/rand.h"
#include "dll.h"
#include "macros.h"

#include "recomp/dlls/engine/11_newlfx_recomp.h"

typedef struct {
/*00*/ u8 _unk0[0x6 - 0x0];
/*06*/ s8 unk6;
/*07*/ s8 unk7;
/*08*/ u8 unk8;
/*09*/ u8 unk9;
/*0A*/ u8 maxRange;
/*0B*/ u8 minRange;
/*0C*/ u8 unkC;
/*0D*/ u8 lightSlot;
/*10*/ Vec3f pos;
/*1C*/ Object* unk1C;
/*20*/ Object* unk20;
/*24*/ s32 unk24;
/*28*/ s32 timer;
/*2C*/ s32 time;
/*30*/ u8 _unk30[0x34 - 0x30];
/*34*/ s32 slot;
/*38*/ s32 unk38;
/*3C*/ s32 unk3C;
/*40*/ f32 unk40;
/*44*/ f32 unk44;
/*48*/ u8 _unk48[0x54 - 0x48];
/*54*/ f32 velX;
/*58*/ f32 velY;
/*5C*/ f32 velZ;
/*60*/ u8 _unk60[0x68 - 0x60];
/*68*/ s16 type;
/*6A*/ u8 flags1;
/*6B*/ u8 flags2;
/*6C*/ u8 flags3;
/*6D*/ u8 _unk6D[0x73 - 0x6D];
/*73*/ u8 r1;
/*74*/ u8 g1;
/*75*/ u8 b1;
/*76*/ u8 r2;
/*77*/ u8 g2;
/*78*/ u8 b2;
/*79*/ u8 paletteIdx;
/*7A*/ u8 unk7A;
/*7B*/ u8 rndMax;
/*7C*/ u8 rndMin;
/*7D*/ u8 unk7D;
/*7E*/ u8 unk7E;
/*7F*/ u8 unk7F;
} LfxLight;

extern u8 data_18;
extern LfxLight* sLights[30];

extern void newlfx_allocLight(LightAction*, f32, f32, f32, Object*, Object*);
extern s32 newlfx_findLight(s32);
extern void newlfx_freeLight(s32);

static u16 recomp_autoSlotIDs[16];

static s32 recomp_newlfx_allocAutoSlot(s32 id) {
    for (s32 i = 0; i < 16; i++) {
        s32 slot = (data_18++) + 11;
        if (data_18 >= 16) {
            data_18 = 0;
        }

        if (newlfx_findLight(slot) == -1) {
            //recomp_printf("[newlfx] alloc auto slot (action: %d, slot: %d)\n", id - 1, slot);
            recomp_autoSlotIDs[slot - 11] = id;
            return slot;
        }
    }

    recomp_eprintf("[newlfx] WARN: auto-slots are full! Actions: ");
    for (s32 i = 0; i < 16; i++) {
        if (i > 0) {
            recomp_eprintf(", ");
        }
        recomp_eprintf("%d", recomp_autoSlotIDs[i] - 1);
    }
    recomp_eprintf("\n");

    return -1;
}

static void recomp_newlfx_freeAutoSlot(s32 slot) {
    if (slot >= 11 && slot < (11 + 16)) {
        //recomp_printf("[newlfx] free auto slot (action: %d, slot: %d)\n", recomp_autoSlotIDs[slot - 11] - 1, slot);
        recomp_autoSlotIDs[slot - 11] = 0;
    }
}

/**
 * - Modification of MusicalProgrammer's newlfx patch. Auto-assigned slots can no longer overwrite each other, avoiding
 *   a scenario where lights might get freed earlier or live longer than intended. Rather than forcefully freeing lights
 *   when we run out of auto slots, now we just refuse to allocate a light and print a warning.
 * - Newlfx action IDs are now only persisted to the save game if it's the kind of action that can be restored correctly.
 */
RECOMP_PATCH void newlfx_DoAction(Object* arg0, Object* arg1, LightAction* action, s32 arg3, s32 arg4, SRT* arg5) {
    s32 existingLight = -1;
    PlayerLightActions* plLightActions = dll_gplay->get_current_player_lactions();
    
    if (action == NULL) {
        return;
    }
    if (action->slot != 0xFFFE) {
        existingLight = newlfx_findLight(action->slot);
    }
    if (existingLight == -1) {
        if (!(action->flags & 2) && (action->flags & 1)) {
            if (!(action->flags & 0x40) && (arg0->def->unk87 & 0x10)) {
                // @recomp: Use first free auto-slot instead of always overwriting the next one. Additionally, if all auto slots
                //          are full then don't allocate a light at all to avoid messing up the existing lights.
                s32 slot = recomp_newlfx_allocAutoSlot(action->id);
                if (slot == -1) {
                    return;
                }
                action->slot = slot;
                arg0->unkD6 = (u8) action->slot;
                newlfx_allocLight(action, arg0->srt.transl.x, arg0->srt.transl.y, arg0->srt.transl.z, arg1, arg0);
            } else if (action->slot == 0xFFFE) {
                // @recomp: Same new auto-slot selection as above ^
                s32 slot = recomp_newlfx_allocAutoSlot(action->id);
                if (slot == -1) {
                    return;
                }
                action->slot = slot;
                if (!(action->flags & 0x40)) {
                    arg0->unkD6 = (u8) action->slot;
                    newlfx_allocLight(action, arg0->srt.transl.x, arg0->srt.transl.y, arg0->srt.transl.z, arg1, arg0);
                } else {
                    newlfx_allocLight(action, arg5->transl.x, arg5->transl.y, arg5->transl.z, arg1, arg0);
                }
            } else if ((arg5 != NULL) && (action->flags & 0x40)) {
                newlfx_allocLight(action, arg5->transl.x, arg5->transl.y, arg5->transl.z, arg1, arg0);
            } else {
                if (arg0->parent != NULL) {
                    newlfx_allocLight(action, arg0->srt.transl.x, arg0->srt.transl.y, arg0->srt.transl.z, arg1, arg0);
                } else {
                    newlfx_allocLight(action, arg0->globalPosition.x, arg0->globalPosition.y, arg0->globalPosition.z, arg1, arg0);
                }
                if (action->type == 0) {
                    // @recomp: Only save action if it's is attached to the player. The game can only restore those kinds
                    //          of newlfx actions correctly in its current state and otherwise can end up with unrelated
                    //          actions being applied to the player on a save reload.
                    Object* player = objGetPlayer();
                    if (arg0 == player && arg1 == player) {
                        plLightActions->newlfx = action->id - 1;
                    }
                }
            }
        }
    } else {
        if (action->flags & 1) {
            if (action->flags2 & 8) {
                sLights[existingLight]->velX = (f32) action->x;
                sLights[existingLight]->velY = (f32) action->y;
                sLights[existingLight]->velZ = (f32) action->z;
            }
        } else if (action->flags & 2) {
            // @recomp: Also free slot for our custom auto-slot tracking
            recomp_newlfx_freeAutoSlot(action->slot);
            newlfx_freeLight(action->slot);
            if (action->type == 0) {
                // @recomp: Only save action if it's attached to the player. See the other half of this patch above ^
                Object* player = objGetPlayer();
                if (arg0 == player && arg1 == player) {
                    plLightActions->newlfx = action->id - 1;
                }
            }
        }
    }
}

RECOMP_HOOK_DLL(newlfx_updateLights) void newlfx_updateLights_hook(void) {
    // @recomp: Clear object pointers if the object was destroyed
    for (s32 i = 0; i < ARRAYCOUNT_S(sLights); i++) {
        if (sLights[i] != NULL) {
            if (sLights[i]->unk1C != NULL && (sLights[i]->unk1C->stateFlags & OBJSTATE_DESTROYED)) {
                sLights[i]->unk1C = NULL;
            }
            if (sLights[i]->unk20 != NULL && (sLights[i]->unk20->stateFlags & OBJSTATE_DESTROYED)) {
                sLights[i]->unk20 = NULL;
            }
        }
    }
}

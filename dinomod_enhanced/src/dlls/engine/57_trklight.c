#include "modding.h"
#include "recomputils.h"

#include "PR/ultratypes.h"
#include "dlls/engine/29_gplay.h"
#include "sys/lfx.h"
#include "sys/main.h"
#include "sys/rand.h"
#include "dll.h"

#include "recomp/dlls/engine/57_trklight_recomp.h"

// size: 0x28
typedef struct {
    u8 _unk0[0x4 - 0x0];
    s32 timer;
    u8 _unk8[0xC - 0x8];
    f32 time;
    f32 rate;
    s16 type; // LightActionType
    s16 min;
    s16 max;
    u8 flags;
    u8 r1;
    u8 g1;
    u8 b1;
    u8 r2;
    u8 g2;
    u8 b2;
    u8 unk21;
    u8 unk22;
    u8 speed;
    u8 inUse;
    u8 slot;
    u8 _unk26[0x28 - 0x26];
} TrackLight;

extern TrackLight sLights[2];

extern void trklight_initSlot(LightAction* action);
extern void trklight_updateSlot(LightAction* action);

RECOMP_PATCH void trklight_DoAction(LightAction* laction) {
    PlayerLightActions* lactions;

    lactions = gDLL_29_Gplay->vtbl->get_current_player_lactions();
    if (laction->slot >= 2) {
        recomp_eprintf("[trklight] Out of bounds light action (idx %d). Slot: %d. Ignoring.\n", 
            laction->id - 1, laction->slot);
        // @recomp: Skip out of bounds light action slot and don't write action out of bounds in savefile.
        //          There's a bunch of light actions that use slots 2-10, suggesting that trklight had more than two slots
        //          in the past. It's not clear what this if-statement is trying to do.
        //          In most cases, these actions can be updated to use slot 0 instead and they will function as intended. 
        // @bug: stores the action index out of bounds
        //lactions->trklights[laction->slot] = laction->id - 1;
        //laction->slot = 1;
        return;
    }
    if (laction->flags & 1) {
        // Set light
        if (sLights[laction->slot].inUse == FALSE) {
            trklight_initSlot(laction);
        } else {
            trklight_updateSlot(laction);
        }
        // @recomp: Persist active lights
        lactions->trklights[laction->slot] = laction->id - 1;
    } else if (laction->flags & 2) {
        // Free light (doesn't reset the color?)
        sLights[laction->slot].inUse = FALSE;
        lactions->trklights[laction->slot] = laction->id - 1;
    }
}

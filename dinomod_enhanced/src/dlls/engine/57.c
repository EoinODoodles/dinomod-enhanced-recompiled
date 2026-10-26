#include "modding.h"
#include "recomputils.h"

#include "PR/ultratypes.h"
#include "dlls/engine/29_gplay.h"
#include "sys/lfx.h"
#include "sys/main.h"
#include "sys/rand.h"
#include "dll.h"

#include "recomp/dlls/engine/57_recomp.h"

// size: 0x28
typedef struct {
    u8 _unk0[0x4 - 0x0];
    s32 unk4;
    u8 _unk8[0xC - 0x8];
    f32 unkC;
    f32 unk10;
    s16 unk14;
    s16 unk16;
    s16 unk18;
    u8 unk1A;
    u8 unk1B;
    u8 unk1C;
    u8 unk1D;
    u8 unk1E;
    u8 unk1F;
    u8 unk20;
    u8 unk21;
    u8 unk22;
    u8 unk23;
    u8 unk24;
    u8 unk25;
    u8 _unk26[0x28 - 0x26];
} BSS30;

extern BSS30 bss_30[2];

extern void dll_57_func_9A0(LightAction* action);
extern void dll_57_func_BB0(LightAction* action);

RECOMP_PATCH void dll_57_Func_74(LightAction* laction) {
    PlayerLightActions* lactions;

    //recomp_printf("DLL 57 LAction: %d\n", laction->unke - 1);

    lactions = gDLL_29_Gplay->vtbl->get_current_player_lactions();
    if (laction->unk10 >= 2) {
        recomp_printf("[DLL 57] Out of bounds light action (idx %d). Slot: %d. Using slot 1 instead.\n", 
            laction->unke - 1, laction->unk10);
        // @recomp: Don't write action out of bounds in savefile.
        //          There's a bunch of light actions that use slots 2-10, suggesting that DLL 57 had more than two slots
        //          in the past. It's not clear what this if-statement is trying to do.
        //lactions->unkC[laction->unk10 - 2] = (s16) (laction->unke - 1);
        laction->unk10 = 1;
        lactions->unkC[laction->unk10] = (s16) (laction->unke - 1);
    }
    if (laction->unk12 & 1) {
        if (bss_30[laction->unk10].unk24 == 0) {
            dll_57_func_9A0(laction);
        } else {
            dll_57_func_BB0(laction);
        }
    } else if (laction->unk12 & 2) {
        bss_30[laction->unk10].unk24 = 0;
        lactions->unk8[laction->unk10] = (s16) (laction->unke - 1);
    }
}

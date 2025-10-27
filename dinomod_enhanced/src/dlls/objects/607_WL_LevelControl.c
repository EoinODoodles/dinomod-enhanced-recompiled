#include "modding.h"

#include "PR/ultratypes.h"
#include "dlls/objects/210_player.h"
#include "game/gamebits.h"
#include "game/objects/object.h"
#include "game/objects/object_id.h"
#include "sys/main.h"
#include "sys/map.h"
#include "sys/objects.h"
#include "sys/objtype.h"
#include "functions.h"
#include "dll.h"

#include "recomp/dlls/objects/607_WL_LevelControl_recomp.h"

typedef struct {
    s16 unk0;
    s16 unk2;
    s16 unk4;
    u8 unk6;
    u8 unk7;
    u8 unk8;
    u8 unk9;
} WL_LevelControl_Data;

/*0x14*/ extern u8 _data_14;
RECOMP_PATCH void WL_LevelControl_setup5_tick(Object* self) {
    WL_LevelControl_Data* objdata;
    f32 sp40;
    Object* temp_v0;
    Object** var_a2;
    s32 sp34;
    s16 i;
    s16 temp_v0_2;
    ObjSetup *someObjsetup;
    Object* player;
    
    sp34 = 0;
    sp40 = 10000.0f;
    player = get_player();
    objdata = self->data;
    if ((_data_14 != 0) && (main_get_bits(BIT_318) == 0)) {
        main_set_bits(BIT_Spell_Projectile, 1);
        main_set_bits(BIT_Spell_Forcefield, 1);
        main_set_bits(BIT_Spell_Illusion, 1);
        ((DLL_210_Player*)player->dll)->vtbl->func39(player, 8, 1);
        ((DLL_210_Player*)player->dll)->vtbl->func14(player, 0x14);
        main_set_bits(BIT_2DD, 0);
        _data_14 = 0;
    }
    if (main_get_bits(BIT_2DB) != 0) {
        func_80059038(0x18, 0, 0);
    }
    if (main_get_bits(BIT_2DD) != 0) {
        main_set_bits(BIT_CFExplodeTunnel_Trigger_31B6F, 1);
        main_set_bits(BIT_2DD, 0);
        temp_v0 = obj_get_nearest_type_to(OBJTYPE_4, self, &sp40);
        if (temp_v0 != NULL) {
            obj_destroy_object(temp_v0);
        }
        objdata->unk2 = 0x1E;
    }
    if (main_get_bits(BIT_2F7) != 0) {
        var_a2 = obj_get_all_of_type(OBJTYPE_4, &sp34);
        for (i = 0; i < sp34; i++) {
            someObjsetup = var_a2[i]->setup;
            if ((someObjsetup->uID == 0x296E) || (someObjsetup->uID == 0x296F)) {
                obj_destroy_object(var_a2[i]);
            }
        }
        main_set_bits(BIT_2F7, 0);
    }
    if (main_get_bits(BIT_2EE) != 0) {
        temp_v0_2 = ((DLL_210_Player*)player->dll)->vtbl->func50(player);
        if ((temp_v0_2 != 0x40) && (temp_v0_2 != 0x1D7) && (main_get_bits(BIT_2F3) == 0)) {
            // @recomp: Instead of warping, play a cutscene for the GuardClaw in Warlock Mountain, 
            //          to avoid crashing the game after you deposit the spirit. 
            //          The set bits plays the cutscene. (originally by MusicalProgrammer)
            //warpPlayer(WARP_WM_SABRE_KRAZOA_CORRIDOR, /*fadeToBlack=*/FALSE);
            main_set_bits(0x1DE, 1);
        }
        main_set_bits(BIT_2EE, 0);
    }
    if (main_get_bits(BIT_2FA) != 0) {
        if (main_get_bits(BIT_2F7) == 0) {
            main_set_bits(BIT_2F7, 1);
        }
        objdata->unk2 -= (s16)gUpdateRate;
        if (objdata->unk2 <= 0) {
            objdata->unk2 = 0;
            main_set_bits(BIT_2FA, 0);
            main_set_bits(BIT_2F3, 1);
            objdata->unk2 = 0x1E;
        }
    }
}

/*0x18*/ extern u8 _data_18;
RECOMP_PATCH void WL_LevelControl_setup6_tick(Object* self) {
    Object* player;
    Object* temp_v0;

    player = get_player();
    if ((_data_18 != 0) && (main_get_bits(BIT_Play_Seq_020D) == 0)) {
        temp_v0 = ((DLL_210_Player*)player->dll)->vtbl->func66(player, 0xF);
        ((DLL_Unknown*)temp_v0->dll)->vtbl->func[10].withOneArg((s32)temp_v0);
        ((DLL_Unknown*)temp_v0->dll)->vtbl->func[11].withTwoArgs((s32)temp_v0, 1);
        ((DLL_Unknown*)temp_v0->dll)->vtbl->func[11].withTwoArgs((s32)temp_v0, 1);
        ((DLL_Unknown*)temp_v0->dll)->vtbl->func[11].withTwoArgs((s32)temp_v0, 1);
        ((DLL_Unknown*)temp_v0->dll)->vtbl->func[11].withTwoArgs((s32)temp_v0, 1);
        ((DLL_Unknown*)temp_v0->dll)->vtbl->func[11].withTwoArgs((s32)temp_v0, 1);
        ((DLL_Unknown*)temp_v0->dll)->vtbl->func[11].withTwoArgs((s32)temp_v0, 1);
        ((DLL_Unknown*)temp_v0->dll)->vtbl->func[11].withTwoArgs((s32)temp_v0, 1);
        ((DLL_Unknown*)temp_v0->dll)->vtbl->func[11].withTwoArgs((s32)temp_v0, 1);
        ((DLL_Unknown*)temp_v0->dll)->vtbl->func[11].withTwoArgs((s32)temp_v0, 1);
        ((DLL_Unknown*)temp_v0->dll)->vtbl->func[11].withTwoArgs((s32)temp_v0, 1);
        ((DLL_Unknown*)temp_v0->dll)->vtbl->func[11].withTwoArgs((s32)temp_v0, 2);
        ((DLL_Unknown*)temp_v0->dll)->vtbl->func[11].withTwoArgs((s32)temp_v0, 4);
        main_set_bits(BIT_Spell_Projectile, 1);
        main_set_bits(BIT_Spell_Forcefield, 1);
        // @recomp: Stop Warlock Mountain from giving you back one of the Spirits. 
        //          Possibly unwanted debug code? (originally by MusicalProgrammer)
        //((DLL_210_Player*)player->dll)->vtbl->func39(player, 0x20, 1);
        _data_18 = 0;
    }
}

/*0x1C*/ extern u8 _data_1C;
RECOMP_PATCH void WL_LevelControl_setup7_tick(Object* self) {
    WL_LevelControl_Data* objdata;
    Object* player;

    get_player();
    objdata = (WL_LevelControl_Data*)self->data;
    if ((_data_1C != 0) && (main_get_bits(BIT_Play_Seq_020D) == 0)) {
        main_set_bits(BIT_Spell_Projectile, 1);
        main_set_bits(BIT_Spell_Forcefield, 1);
        player = get_player();
        // @recomp: Stop Warlock Mountain from giving you back one of the Spirits.
        //          Possibly unwanted debug code? (originally by MusicalProgrammer)
        //((DLL_210_Player*)player->dll)->vtbl->func39(player, 0x40, 1);
        ((DLL_210_Player*)player->dll)->vtbl->func14(player, 0x14);
        _data_1C = 0;
        objdata->unk2 = 1;
        func_80000860(self, self, 0x32, 0);
        func_80000860(self, self, 0x33, 0);
        main_set_bits(BIT_221, 1);
    }
    if (main_get_bits(BIT_36C) != 0) {
        if (objdata->unk4 > 0) {
            objdata->unk4 -= (s16)gUpdateRate;
            if (objdata->unk2 != 0) {
                objdata->unk2 -= (s16)gUpdateRate;
                if (objdata->unk2 <= 0) {
                    main_set_bits(BIT_36D, 1);
                    if (objdata->unk6 >= 0xB) {
                        objdata->unk6 = objdata->unk6 - 1;
                    }
                    objdata->unk2 = objdata->unk6;
                }
            }
        }
    }
    if (rand_next(0, 30) == 0) {
        main_set_bits(BIT_WM_Randorn_Door_OpenClose, 1);
    }
    if (rand_next(0, 10) == 0) {
        main_set_bits(BIT_WM_Randorn_Door_OpenClose, 0);
    }
}

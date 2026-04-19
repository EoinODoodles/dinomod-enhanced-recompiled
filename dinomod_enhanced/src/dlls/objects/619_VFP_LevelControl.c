#include "modding.h"

#include "PR/ultratypes.h"
#include "dlls/objects/210_player.h"
#include "game/gamebits.h"
#include "game/objects/object.h"
#include "sys/main.h"
#include "sys/objects.h"
#include "sys/objtype.h"
#include "sys/print.h"
#include "sys/segment_1050.h"
#include "sys/segment_1460.h"
#include "dll.h"

#include "recomp/dlls/objects/619_VFP_LevelControl_recomp.h"

extern void VFP_LevelControl_func_8EC(Object *self);
extern void VFP_LevelControl_func_A08(Object *self);
extern void VFP_LevelControl_func_AAC(Object *self);

RECOMP_PATCH void VFP_LevelControl_setup(Object* self, ObjSetup* setup, s32 a2) {
    u8 mapSetupID;

    obj_add_object_type(self, 0xA);
    // @recomp: Don't force act 1
    //gDLL_29_Gplay->vtbl->set_map_setup((s32) self->mapID, 1);
    mapSetupID = gDLL_29_Gplay->vtbl->get_map_setup((s32) self->mapID);
    switch (mapSetupID) {
    case 1:
    case 2:
        break;
    case 0:
        func_80000860(self, self, 0x105, 0);
        func_80000860(self, self, 0x106, 0);
        func_80000860(self, self, 0x107, 0);
        break;
    case 3:
        func_80000860(self, self, 0x105, 0);
        func_80000860(self, self, 0x106, 0);
        func_80000860(self, self, 0x107, 0);
        func_80000450(self, self, 0x166, 0, 0, 0);
        func_80000450(self, self, 0x167, 0, 0, 0);
        func_80000450(self, self, 0x168, 0, 0, 0);
        func_80000450(self, self, 0x169, 0, 0, 0);
        func_80000450(self, self, 0x174, 0, 0, 0);
        break;
    }
    self->unkB0 |= 0x6000;
}

/*0x0*/ extern s16 _data_0;

RECOMP_PATCH void VFP_LevelControl_control(Object* self) {
    Object* player;
    u8 mapSetupID;

    player = get_player();
    map_get_map_id_from_xz_ws(player->srt.transl.x, player->srt.transl.z);
    diPrintf("ACT %d \n", gDLL_29_Gplay->vtbl->get_map_setup(self->mapID));
    mapSetupID = gDLL_29_Gplay->vtbl->get_map_setup(self->mapID);
    switch (mapSetupID) {
    case 1:
        if (_data_0 != 0) {
            _data_0 -= (s16)gUpdateRateF;
            if (_data_0 <= 0) {
                _data_0 = 0;
                func_80000860(self, self, 0x105, 0);
                func_80000860(self, self, 0x106, 0);
                func_80000860(self, self, 0x107, 0);
                func_80000450(self, self, 0x166, 0, 0, 0);
                func_80000450(self, self, 0x167, 0, 0, 0);
                func_80000450(self, self, 0x168, 0, 0, 0);
                func_80000450(self, self, 0x169, 0, 0, 0);
                func_80000450(self, self, 0x174, 0, 0, 0);
                func_80000450(self, self, 0x178, 0, 0, 0);
                // @recomp: Don't give SpellStone back (breaks progression)
                //main_set_bits(BIT_SpellStone_DIM, 1); // spellstone 1
            }
        }
        VFP_LevelControl_func_8EC(self);
    case 0:
        return;
    case 2:
        if (_data_0 != 0) {
            _data_0 -= (s16)gUpdateRateF;
            if (_data_0 <= 0) {
                _data_0 = 0;
                main_set_bits(BIT_DB_Unlock_Act_Two, 1);
                func_80000860(self, self, 0x105, 0);
                func_80000860(self, self, 0x106, 0);
                func_80000860(self, self, 0x107, 0);
                func_80000450(self, self, 0x166, 0, 0, 0);
                func_80000450(self, self, 0x167, 0, 0, 0);
                func_80000450(self, self, 0x168, 0, 0, 0);
                func_80000450(self, self, 0x169, 0, 0, 0);
                func_80000450(self, self, 0x174, 0, 0, 0);
                func_80000450(self, self, 0x178, 0, 0, 0);
                // @recomp: Don't give SpellStone back (breaks progression)
                //main_set_bits(BIT_SpellStone_WC, 1); // spellstone 2
                main_set_bits(BIT_SpellStone_DIM_Activated, 1);
            }
        }
        VFP_LevelControl_func_A08(self);
        break;
    case 3:
        if (_data_0 != 0) {
            _data_0 -= (s16)gUpdateRateF;
            if (_data_0 <= 0) {
                _data_0 = 0;
                main_set_bits(BIT_DB_Unlock_Act_Two, 1);
                main_set_bits(BIT_DB_Unlock_Act_Three, 1);
                func_80000860(self, self, 0x105, 0);
                func_80000860(self, self, 0x106, 0);
                func_80000860(self, self, 0x107, 0);
                func_80000450(self, self, 0x166, 0, 0, 0);
                func_80000450(self, self, 0x167, 0, 0, 0);
                func_80000450(self, self, 0x168, 0, 0, 0);
                func_80000450(self, self, 0x169, 0, 0, 0);
                func_80000450(self, self, 0x174, 0, 0, 0);
                main_set_bits(BIT_SpellStone_DR, 1);
            }
        }
        VFP_LevelControl_func_AAC(self);
        break;
    }
}

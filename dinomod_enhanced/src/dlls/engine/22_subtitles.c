#include "modding.h"
#include "recompconfig.h"

#include "PR/ultratypes.h"
#include "PR/gbi.h"
#include "sys/main.h"
#include "sys/gfx/gx.h"

typedef enum {
    UPCOMING_SUBTITLE_TRANS_VANILLA,
    UPCOMING_SUBTITLE_TRANS_INVISIBLE,
} UpcomingSubtitleTransparencyOption;

static _Bool hide_upcoming() {
    return recomp_get_config_u32("upcoming_subtitle_trans") == UPCOMING_SUBTITLE_TRANS_INVISIBLE;
}

#include "recomp/dlls/engine/22_subtitles_recomp.h"

// Size: 0x18
typedef struct InnerBss38 {
    Texture *unk0;
    s32 unk4;
    s16 unk8;
    s16 unkA;
    s32 unkC;
    s32 pad10;
    s32 pad14;
} InnerBss38;

// Size: 0x26C
typedef struct StructBss38 {
/*0x00*/ char *unk0[2][8];
/*0x40*/ s32 unk40[2][8];
/*0x80*/ u16 unk80[2][8];
/*0xA0*/ u16 unkA0[2][8];
/*0xC0*/ s16 unkC0[4];
/*0xC8*/ InnerBss38 unkC8[2][8];
/*0x248*/ u16 unk248;
/*0x24C*/ u8 pad24C[0x268 - 0x24A];
/*0x268*/ u16 unk268;
/*0x26A*/ u16 unk26A;
} StructBss38;

extern u32 _data_34;
extern u32 _data_38;
extern u32 _data_3C;
extern s32 _data_40;
extern s32 _data_44;
extern f32 _data_48;
extern f32 _data_4C;

extern StructBss38 *_bss_780[3];
extern u8 _bss_78C[0x4];
extern s32 _bss_798;
extern s32 _bss_79C;
extern u16 _bss_7A0;
extern u8 _bss_7A2;
extern u8 _bss_7A4;

extern void dll_22_func_448(void);
extern void dll_22_func_8F4();
extern void dll_22_func_94C(void);
extern void dll_22_func_AA8(void);
extern void dll_22_func_C64(void);
extern void dll_22_func_D9C(Gfx **gdl);
extern f32 dll_22_func_16A0(void);
extern f32 dll_22_func_2118(void);

RECOMP_PATCH void dll_22_func_578(Gfx **gdl) {
    f32 temp;
    s32 temp_fv1;
    s32 temp_ft5;
    s32 temp_v1;
    s32 temp_v0;

    temp_ft5 = (delayFloat * 100.0f) / 30.0f;
    if (_data_38 != 0) {
        _data_40 += temp_ft5;
        if (_data_40 >= 0x65) {
            _data_40 = 0x64;
        }
    } else {
        _data_40 -= temp_ft5;
        if (_data_40 < 0) {
            _data_40 = 0;
        }
    }
    dll_22_func_8F4();
    if (_data_34 == 0) {
        return;
    }

    temp_v0 = get_some_resolution_encoded();
    _bss_7A0 = temp_v0 & 0xFFFF;
    _bss_79C = _bss_7A0 - (_bss_7A2 * 2);
    _bss_798 = (_bss_7A0 - _bss_79C) / 2;
    if (_data_4C <= 0.0f) {
        temp_fv1 = dll_22_func_16A0();
        if (temp_fv1 == 0.0f) {
            dll_22_func_448();
            return;
        }

        temp_fv1 -= 600.0f;
        _data_4C += 600.0f;
        if (_bss_780[0]->unk26A & 1) {
            _data_44 = 0x258;
            temp_fv1 -= 600.0f;
        }
        _data_48 += temp_fv1;
    }

    if (_data_44 > 0.0f) {
        _data_44 -= dll_22_func_2118();
        dll_22_func_94C();
    } else if (_data_48 > 0.0f) {
        _data_48 -= dll_22_func_2118();
        _data_3C = 0;
        _bss_78C[0] = 0xFF;
        _bss_78C[1] = hide_upcoming() ? 0 : 0x17; // @recomp: hide upcoming subtitle
        _bss_78C[2] = 0;
        _bss_7A4 = 0xC7;
    } else if (_data_4C > 0.0f) {
        _data_4C -= dll_22_func_2118();
        dll_22_func_AA8();
    }
    dll_22_func_C64();
    dll_22_func_D9C(gdl);
}

RECOMP_PATCH void dll_22_func_94C(void) {
    f32 temp_fv1;

    _data_3C = 0;
    if (_data_44 > 0.0f) {
        temp_fv1 = 600.0f - _data_44;
        _bss_78C[0] = (s32)((255.0f * temp_fv1) / 600.0f);
        _bss_78C[1] = hide_upcoming() ? 0 : (s32)((23.0f * temp_fv1) / 600.0f);
        _bss_78C[2] = 0;
        _bss_7A4 = ((199.0f * temp_fv1) / 600.0f);
        return;
    }
    _bss_78C[0] = 0xFF;
    _bss_78C[1] = hide_upcoming() ? 0 : 0x17; // @recomp: hide upcoming subtitle
    _bss_78C[2] = 0;
    _bss_7A4 = 0xC7;
}

RECOMP_PATCH void dll_22_func_AA8(void) {
    f32 temp_fv1;

    _data_3C = 0;
    if (_data_4C > 0.0f) {
        if (_bss_780[1]->unk0[0][0] == 0) {
            _bss_78C[0] = (s32)((255.0f * _data_4C) / 600.0f);
            _bss_7A4 = (s32)((199.0f * _data_4C) / 600.0f);
            return;
        }
        temp_fv1 = _data_4C / 600.0f;
        _bss_78C[2] = hide_upcoming() ? 0 : 0x17 - (s32)(23.0f * temp_fv1); // @recomp: hide upcoming subtitle
        if (temp_fv1 > 0.6f) {
            _bss_78C[1] = hide_upcoming() ? 0 : 0x17; // @recomp: hide upcoming subtitle
        } else {
            _bss_78C[1] = hide_upcoming()
                ? (s32)((((0.6f - temp_fv1) * 255.0f) / 0.6f))
                : (s32)((((0.6f - temp_fv1) * 232.0f) / 0.6f) + 23.0f);
        }
        if (temp_fv1 > 0.4f) {
            _bss_78C[0] = (s32)(((temp_fv1 - 0.4f) * 255.0f) / 0.6f);
        } else {
            _bss_78C[0] = 0;
        }
        _bss_7A4 = 0xC7;
        return;
    }
    _bss_78C[0] = 0;
    if (_bss_780[1]->unk0[0][0] == 0) {
        _bss_7A4 = 0;
        return;
    }
    _bss_7A4 = 0xC7;
    _bss_78C[1] = 0xFF;
    _bss_78C[2] = hide_upcoming() ? 0 : 0x17;  // @recomp: hide upcoming subtitle
}

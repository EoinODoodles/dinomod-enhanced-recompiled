#include "modding.h"

#include "PR/ultratypes.h"
#include "game/objects/object.h"

#include "recomp/dlls/engine/13_expgfx_recomp.h"

extern s16 _data_0[30];

extern Object *_bss_118[30];
extern s32 _bss_78[30];
extern s8 _bss_F0[30];

RECOMP_PATCH s32 dll_13_func_4A18(s16* arg0, s16* arg1, s16 arg2, s32 arg3, Object *arg4) {
    s32 var_t0;
    s32 var_t1_2;
    s16 var_v1;
    s32 var_t1;

    var_v1 = -1;
    var_t1 = 0;
    var_t0 = 0;
    for (var_t0 = 0; var_t0 < 30; var_t0++) {
        if ((arg4 == _bss_118[var_t0]) && (arg2 == _data_0[var_t0]) && (_bss_F0[var_t0] < 30)) {
            var_v1 = var_t0;
            var_t1 = 1;
            break;
        }
    }
    
    if (var_t1 != 0) {
        for (var_t1_2 = 0; var_t1_2 < 30; var_t1_2++) {
            if (!(_bss_78[var_v1] & (1 << var_t1_2))) {
                *arg1 = var_t1_2;
                *arg0 = var_v1;
                _bss_78[var_v1] |= (1 << var_t1_2);
                _bss_F0[var_v1]++;
                return 1;
            }
        }
    }
    
    var_t1 = 0;
    // @recomp: Always take the first branch (disables a debug feature?)
    if (arg3 == -1 || TRUE) {
        for (var_t0 = 0; var_t0 < 29; var_t0++) {
            if (_bss_F0[var_t0] <= 0) {
                var_v1 = var_t0;
                var_t1 = 1;
                _bss_F0[var_t0] = 0;
                break;
            }
        }
    } else if (arg3 != -1) {
        var_t0 = arg3;
        if (_bss_F0[arg3] < 30) {
            var_v1 = arg3;
            var_t1 = 1;
        }
    }
    
    if (var_t1 != 0) {
        for (var_t1_2 = 0; var_t1_2 < 30; var_t1_2++) {
            if (arg2){} // fake

            if (!(_bss_78[var_v1] & (1 << var_t1_2))) {
                *arg1 = var_t1_2;
                *arg0 = var_v1;
                _bss_78[var_v1] |= (1 << var_t1_2);
                _data_0[var_t0] = arg2;
                _bss_F0[var_v1]++;
                return 1;
            }
        }

        return -1;
    }
    
    return -1;
}

#include "modding.h"
#include "dbgui.h"

#include "PR/ultratypes.h"
#include "PR/gbi.h"
#include "dll.h"

#include "core/debug_fog.h"

static s32 windowOpen = FALSE;

static s32 overrideFog = FALSE;
static s32 twoCycle = FALSE;

static s32 sFogParam0 = 0;
static s32 sFogParam1 = 0;
static s32 sFogParam2 = 0;
static s32 sFogParam3 = 0;

static s32 sFog2Param0 = 0;
static s32 sFog2Param1 = 0;
static s32 sFog2Param2 = 0;
static s32 sFog2Param3 = 0;

static s32 sCombine1a = 0;
static s32 sCombine1b = 0;
static s32 sCombine1c = 0;
static s32 sCombine1d = 0;
static s32 sCombine1A = 0;
static s32 sCombine1B = 0;
static s32 sCombine1C = 0;
static s32 sCombine1D = 0;

static s32 sCombine2a = 0;
static s32 sCombine2b = 0;
static s32 sCombine2c = 0;
static s32 sCombine2d = 0;
static s32 sCombine2A = 0;
static s32 sCombine2B = 0;
static s32 sCombine2C = 0;
static s32 sCombine2D = 0;

static s32 sFogStrengthR = 100;
static s32 sFogStrengthG = 100;
static s32 sFogStrengthB = 100;
static s32 sFogStrengthA = 100;

RECOMP_CALLBACK(".", my_debug_menu_event) void fog_debug_menu_callback() {
    dbgui_menu_item("Water Fog", &windowOpen);
}

#define LIMIT(var, min, max) {\
    if (var < min) var = min;\
    if (var > max) var = max;\
}

#define LIMIT_BL 3
#define LIMIT_CC 31

RECOMP_CALLBACK(".", my_dbgui_event) void fog_debug_dbgui_callback() {
    if (windowOpen) {
        if (dbgui_begin("Water Fog Debug", &windowOpen)) {
            dbgui_checkbox("Override Fog", &overrideFog);
            dbgui_checkbox("Two Cycle", &twoCycle);

            dbgui_separator();

            if (dbgui_input_int("Fog Param 0", &sFogParam0)) {
                LIMIT(sFogParam0, 0, LIMIT_BL);
            }
            if (dbgui_input_int("Fog Param 1", &sFogParam1)) {
                LIMIT(sFogParam1, 0, LIMIT_BL);
            }
            if (dbgui_input_int("Fog Param 2", &sFogParam2)) {
                LIMIT(sFogParam2, 0, LIMIT_BL);
            }
            if (dbgui_input_int("Fog Param 3", &sFogParam3)) {
                LIMIT(sFogParam3, 0, LIMIT_BL);
            }

            if (dbgui_input_int("Fog 2 Param 0", &sFog2Param0)) {
                LIMIT(sFog2Param0, 0, LIMIT_BL);
            }
            if (dbgui_input_int("Fog 2 Param 1", &sFog2Param1)) {
                LIMIT(sFog2Param1, 0, LIMIT_BL);
            }
            if (dbgui_input_int("Fog 2 Param 2", &sFog2Param2)) {
                LIMIT(sFog2Param2, 0, LIMIT_BL);
            }
            if (dbgui_input_int("Fog 2 Param 3", &sFog2Param3)) {
                LIMIT(sFog2Param3, 0, LIMIT_BL);
            }

            dbgui_separator();

            if (dbgui_input_int("Fog Strength R", &sFogStrengthR)) {
                LIMIT(sFogStrengthR, 0, 100);
            }
            if (dbgui_input_int("Fog Strength G", &sFogStrengthG)) {
                LIMIT(sFogStrengthG, 0, 100);
            }
            if (dbgui_input_int("Fog Strength B", &sFogStrengthB)) {
                LIMIT(sFogStrengthB, 0, 100);
            }
            if (dbgui_input_int("Fog Strength A", &sFogStrengthA)) {
                LIMIT(sFogStrengthA, 0, 100);
            }

            dbgui_separator();

            if (dbgui_input_int("Combine 1a", &sCombine1a)) {
                LIMIT(sCombine1a, 0, LIMIT_CC);
            }
            if (dbgui_input_int("Combine 1b", &sCombine1b)) {
                LIMIT(sCombine1b, 0, LIMIT_CC);
            }
            if (dbgui_input_int("Combine 1c", &sCombine1c)) {
                LIMIT(sCombine1c, 0, LIMIT_CC);
            }
            if (dbgui_input_int("Combine 1d", &sCombine1d)) {
                LIMIT(sCombine1d, 0, LIMIT_CC);
            }
            if (dbgui_input_int("Combine 1A", &sCombine1A)) {
                LIMIT(sCombine1A, 0, LIMIT_CC);
            }
            if (dbgui_input_int("Combine 1B", &sCombine1B)) {
                LIMIT(sCombine1B, 0, LIMIT_CC);
            }
            if (dbgui_input_int("Combine 1C", &sCombine1C)) {
                LIMIT(sCombine1C, 0, LIMIT_CC);
            }
            if (dbgui_input_int("Combine 1D", &sCombine1D)) {
                LIMIT(sCombine1D, 0, LIMIT_CC);
            }

            dbgui_separator();

            if (dbgui_input_int("Combine 2a", &sCombine2a)) {
                LIMIT(sCombine2a, 0, LIMIT_CC);
            }
            if (dbgui_input_int("Combine 2b", &sCombine2b)) {
                LIMIT(sCombine2b, 0, LIMIT_CC);
            }
            if (dbgui_input_int("Combine 2c", &sCombine2c)) {
                LIMIT(sCombine2c, 0, LIMIT_CC);
            }
            if (dbgui_input_int("Combine 2d", &sCombine2d)) {
                LIMIT(sCombine2d, 0, LIMIT_CC);
            }
            if (dbgui_input_int("Combine 2A", &sCombine2A)) {
                LIMIT(sCombine2A, 0, LIMIT_CC);
            }
            if (dbgui_input_int("Combine 2B", &sCombine2B)) {
                LIMIT(sCombine2B, 0, LIMIT_CC);
            }
            if (dbgui_input_int("Combine 2C", &sCombine2C)) {
                LIMIT(sCombine2C, 0, LIMIT_CC);
            }
            if (dbgui_input_int("Combine 2D", &sCombine2D)) {
                LIMIT(sCombine2D, 0, LIMIT_CC);
            }

        }

        dbgui_end();
    }
}

s8 get_override_fog() {
    return overrideFog;
}

s8 get_two_cycle() {
    return twoCycle;
}

s32 get_fog_param(s8 index) {
    switch (index) {
    case 0:
        return sFogParam0;
    case 1:
        return sFogParam1;
    case 2:
        return sFogParam2;
    case 3:
        return sFogParam3;
    }
    return 0;
}

s32 get_fog2_param(s8 index) {
    switch (index) {
    case 0:
        return sFog2Param0;
    case 1:
        return sFog2Param1;
    case 2:
        return sFog2Param2;
    case 3:
        return sFog2Param3;
    }
    return 0;
}

s32 get_fog_colour_component(s8 index) {
    switch (index) {
    case 0:
        return sFogStrengthR;
    case 1:
        return sFogStrengthG;
    case 2:
        return sFogStrengthB;
    case 3:
        return sFogStrengthA;
    }
    return 0;
}


s32 get_cc1(s8 index) {
    switch (index) {
    case 0:
        return sCombine1a;
    case 1:
        return sCombine1b;
    case 2:
        return sCombine1c;
    case 3:
        return sCombine1d;
    case 4:
        return sCombine1A;
    case 5:
        return sCombine1B;
    case 6:
        return sCombine1C;
    case 7:
        return sCombine1D;
    }
    return 0;
}

s32 get_cc2(s8 index) {
    switch (index) {
    case 0:
        return sCombine2a;
    case 1:
        return sCombine2b;
    case 2:
        return sCombine2c;
    case 3:
        return sCombine2d;
    case 4:
        return sCombine2A;
    case 5:
        return sCombine2B;
    case 6:
        return sCombine2C;
    case 7:
        return sCombine2D;
    }
    return 0;
}
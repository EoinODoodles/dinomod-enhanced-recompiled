#include "modding.h"
#include "recomputils.h"

#include "dll.h"
#include "dlls/objects/210_player.h"
#include "game/objects/object_id.h"
#include "sys/dll.h"
#include "sys/main.h"
#include "sys/objects.h"

#include "recomp/dlls/objects/787_WCApertureSymbol_recomp.h"

//TEMPORARY DEFINES
#define WCApertureSymbol_control dll_787_control
#define WCApertureSymbol_getTargetOpacity dll_787_func_490
//END OF TEMPORARY DEFINES

typedef struct {
/*00*/ ObjSetup base;
/*18*/ s8 yaw;
/*19*/ s8 modelIdx;
/*1A*/ s16 opacityThreshold;
/*1C*/ u16 unk1C;
/*1E*/ s16 gamebitViewed;
/*20*/ s16 gamebitEnabled;
} WCApertureSymbol_Setup;

typedef struct {
/*0*/ s16 targetOpacity;
/*2*/ u8 state;
/*3*/ u8 flags;
} WCApertureSymbol_Data;

typedef enum {
    STATE_Inactive,
    STATE_Waiting_for_View,
    STATE_Viewed
} WCApertureSymbol_States;

typedef enum {
    WCApertureSymbol_MODELIDX_Sun,
    WCApertureSymbol_MODELIDX_Moon
} WCApertureSymbol_ModelIndices;

typedef enum {
    WCApertureSymbol_FLAG_Visible = 1
} WCApertureSymbol_Flags;

#define VIEWING_TERRAIN_TYPE 0x21
#define VIEWING_DURATION_SUN 8000
#define VIEWING_DURATION_MOON 4500
#define PEAK_SUN 70000
#define PEAK_MOON 79250

extern s16 WCApertureSymbol_getTargetOpacity(Object* self, WCApertureSymbol_Data* objdata, f32 minTimeOfDay, f32 maxTimeOfDay, f32 timeOfDay);

RECOMP_PATCH void WCApertureSymbol_control(Object* self) {
    WCApertureSymbol_Setup* setup;
    f32 time;
    WCApertureSymbol_Data* objdata;
    s32 opacity;
    Object* player;

    setup = (WCApertureSymbol_Setup*)self->setup;
    objdata = self->data;
    
    player = objGetPlayer();
    objdata->targetOpacity = 0;
    
    switch (objdata->state) {
    case STATE_Viewed:
        objdata->targetOpacity = OBJECT_OPACITY_MAX;
        break;
    case STATE_Inactive:
        //@recomp: become active after aperture opens
        if (mainGetBits(setup->gamebitEnabled)) {
            if (mainGetBits(setup->gamebitViewed)) {
                objdata->state = STATE_Viewed;
            } else {
                objdata->state = STATE_Waiting_for_View;
            }
        }
        break;
    case STATE_Waiting_for_View:
        if (gDLL_2_Camera->vtbl->get_dll_ID() == DLL_ID_CAM1STPERSON) {
            if ((((DLL_210_Player*)player->dll)->vtbl->func70(player) == VIEWING_TERRAIN_TYPE) && (vec3Distance(&player->globalPosition, &self->globalPosition) < 200.0f)) {
                gDLL_7_Newday->vtbl->func4(&time);

                if (setup->modelIdx == WCApertureSymbol_MODELIDX_Sun) {
                    objdata->targetOpacity = WCApertureSymbol_getTargetOpacity(self, objdata, 
                        PEAK_SUN - (VIEWING_DURATION_SUN >> 1), 
                        PEAK_SUN + (VIEWING_DURATION_SUN >> 1), 
                        time
                    );
                } else {
                    objdata->targetOpacity = WCApertureSymbol_getTargetOpacity(self, objdata, 
                        PEAK_MOON - (VIEWING_DURATION_MOON >> 1), 
                        PEAK_MOON + (VIEWING_DURATION_MOON >> 1), 
                        time
                    );
                }

                if (self->opacity > setup->opacityThreshold) {
                    mainSetBits(setup->gamebitViewed, TRUE);
                    objdata->state = STATE_Viewed;
                    objdata->targetOpacity = OBJECT_OPACITY_MAX;
                }
            }
        }
        break;
    }

    if (self->opacity < objdata->targetOpacity) {
        opacity = self->opacity + (gUpdateRate * 4);
        if (objdata->targetOpacity < opacity) {
            opacity = objdata->targetOpacity;
        }
        self->opacity = opacity;
    } else if (self->opacity > objdata->targetOpacity) {
        opacity = self->opacity - (gUpdateRate * 4);
        if (opacity < objdata->targetOpacity) {
            opacity = objdata->targetOpacity;
        }
        self->opacity = opacity;
    }
}

#include "dll.h"
#include "game/objects/object_id.h"
#include "modding.h"
#include "recomputils.h"
#include "math_util.h"

#include "sys/dll.h"
#include "sys/gfx/animseq.h"
#include "sys/intersect.h"
#include "sys/main.h"
#include "sys/math.h"
#include "sys/objects.h"
#include "sys/objexpr.h"
#include "sys/objhits.h"
#include "sys/print.h"

#include "recomp/dlls/_asm/791_recomp.h"
#include "sys/rand.h"

//TEMPORARY DEFINES
#define WCFloorTile_obj_Control dll_791_obj_Control
#define WCFloorTile_manageHitLines dll_791_func_544

#define BIT_WC_Hazards_Deactivated 0x338
#define BIT_WC_Moon_Temple_Illusory_Wall_Switch_Pressed 0x265
#define SOUND_A11_Stone_Crumbling 0xA11
//END OF TEMPORARY DEFINES

typedef struct {
    ObjSetup base;
    s16 unk18;
    s16 hitsAnimatorID;
} WCFloorTile_Setup;

typedef struct {
    f32 timer;
    s16 maxAngle;
    u8 state;
    u8 flags;
} WCFloorTile_Data;

typedef enum {
    WCFloorTile_STATE_0_Waiting_to_Fall,
    WCFloorTile_STATE_1_Falling,
    WCFloorTile_STATE_2_Gone,
    WCFloorTile_STATE_3_Finished
} WCFloorTile_States;

typedef enum {
    WCFloorTile_FLAG_Hits_Line_Enabled = 1,
    WCFloorTile_FLAG_Hits_Update_Needed = 2,
    WCFloorTile_FLAG_Can_Fall = 4
} WCFloorTile_Flags;

extern void WCFloorTile_manageHitLines(Object* self, WCFloorTile_Data* objData);

/* Fix some bugs when the tiles reappear using the `BIT_WC_Hazards_Deactivated` gamebit */
RECOMP_PATCH void WCFloorTile_obj_Control(Object* self) {
    f32 dy;
    s32 i;
    f32 opacity;
    Object* obj;
    WCFloorTile_Data* objData;
    WCFloorTile_Setup* objSetup;

    objData = self->data;
    objSetup = (WCFloorTile_Setup*)self->setup;

    //Reappear and don't fall if a particular gamebit is set
    if (objData->state != WCFloorTile_STATE_3_Finished && //@recomp: tweak condition so this block only runs once
        mainGetBits(BIT_WC_Hazards_Deactivated)
    ) {
        self->srt.transl.y = objSetup->base.y;

        //@recomp: reset rotations too (since they could be nonzero from the random rattling)
        self->srt.pitch = 0;
        self->srt.roll = 0;

        //@recomp: ensure hits lines are in the correct state
        objData->flags &= ~WCFloorTile_FLAG_Hits_Line_Enabled;
        objData->flags |= WCFloorTile_FLAG_Hits_Update_Needed;

        objData->state = WCFloorTile_STATE_3_Finished;
    }

    switch (objData->state) {
    default:
    case WCFloorTile_STATE_0_Waiting_to_Fall:
        //Wait until the player has pressed the wall switch, and then fall when Sabre stands on the tile
        if (objData->flags & WCFloorTile_FLAG_Can_Fall) {
            //Check if Sabre's standing on the tile
            if (self->polyhits->unk10F > 0) {
                for (i = 0; i < self->polyhits->unk10F; i++) {
                    obj = self->polyhits->unk100[i];
                    if (obj->id == OBJ_Sabre) {
                        dll_amSfx->Play(self, SOUND_A11_Stone_Crumbling, 0x28, NULL, NULL, 0, NULL);
                        objData->state = WCFloorTile_STATE_1_Falling;
                        objData->timer = 0.0f;
                        self->velocity.y = 0.0f;
                    }
                }
            }
        } else if (mainGetBits(BIT_WC_Moon_Temple_Illusory_Wall_Switch_Pressed)) {
            objData->flags |= WCFloorTile_FLAG_Can_Fall;
        }
        break;
    case WCFloorTile_STATE_1_Falling:
        objData->timer += gUpdateRateF;

        //Start falling after 2 seconds
        if (objData->timer > 120.0f) {
            objData->timer = 120.0f;
            objData->flags |= WCFloorTile_FLAG_Hits_Line_Enabled | WCFloorTile_FLAG_Hits_Update_Needed;
            self->velocity.y += gUpdateRateF * -0.1f;
        }

        //Rattle around to indicate the tile's going to fall
        objData->maxAngle = (s16) ((objData->timer / 120.0f) * 256.0f);
        self->srt.pitch = mathRnd(-objData->maxAngle, objData->maxAngle);
        self->srt.roll = mathRnd(-objData->maxAngle, objData->maxAngle);

        //Drop
        self->srt.transl.y += self->velocity.y * gUpdateRateF;

        //Correlate opacity with fall distance
        dy = objSetup->base.y - self->srt.transl.y;
        if (dy < 50.0f) {
            opacity = OBJECT_OPACITY_MAX;
        } else if (dy > 150.0f) {
            opacity = 0.0f;
        } else {
            opacity = 1.0f - ((dy - 50.0f) / 100.0f);
            if (opacity > 1.0f) {
                opacity = 1.0f;
            } else if (opacity < 0.0f) {
                opacity = 0.0f;
            }
            opacity *= OBJECT_OPACITY_MAX;
        }
        self->opacity = (s32)opacity;

        //Advance state when fully faded out
        if (self->opacity == 0) {
            objData->state = WCFloorTile_STATE_2_Gone;
        }
        break;
    case WCFloorTile_STATE_2_Gone:
        self->opacity = 0;
        func_800267A4(self);
        objData->flags |= WCFloorTile_FLAG_Hits_Line_Enabled | WCFloorTile_FLAG_Hits_Update_Needed;
        break;
    case WCFloorTile_STATE_3_Finished:
        //Fade back in, and don't fall from now on
        opacity = self->opacity;
        opacity += (gUpdateRateF * 8.0f);
        if (opacity > OBJECT_OPACITY_MAX) {
            opacity = OBJECT_OPACITY_MAX;
        }
        self->opacity = opacity;

        func_8002674C(self);
        break;
    }

    WCFloorTile_manageHitLines(self, objData);
}

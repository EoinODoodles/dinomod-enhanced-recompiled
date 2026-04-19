#include "modding.h"
#include "recomputils.h"

#include "common.h"
#include "sys/main.h"
#include "sys/segment_1460.h"

#include "recomp/dlls/objects/612_WL_Crystal_recomp.h"

typedef struct {
/*00*/    ObjSetup base;
/*18*/    s8 yaw;
/*19*/    s8 modelIdx;
/*1A*/    s16 unused1A;
/*1C*/    s16 scale;
} WL_Crystal_Setup;

typedef struct {
/*00*/    s16 unused0;
/*02*/    s16 yawSpeed;       //rotation speed for crystal and sun
/*04*/    s16 rollSpeed;      //roll speed for sun
/*06*/    s16 explosionTimer; //@recomp-specific
/*08*/    s16* inroomBuffer;  //unused aside from setup, seems to store a value for each vertex
/*0C*/    DLL_Unknown *sunFX; //projgfx DLL (2, 18) used by Quan Ata Lachu sun
/*10*/    s8 unused10;  
/*11*/    u8 showCrystal;     //unloads crystal once transformation into sun complete
} WL_Crystal_Data;

typedef enum {
    WMSun_Core = 0,
    WMSun_Middle_Shell = 1,
    WMSun_Outer_Shell = 2
} WMSun_Model_Indices;

#define WMinroom_Max_Opacity     250

#define WMSun_Max_Opacity_Core   255
#define WMSun_Max_Opacity_Middle 85
#define WMSun_Max_Opacity_Outer  25

extern DLL_Unknown* data_sun_modGFX;

extern s16 fxTimer1;
extern s16 fxTimer2;
extern s16 fxTimer3;
extern s16 fxTimer4;
extern s16 fxTimer5;

extern void WL_Crystal_handle_sun_flare_effects(Object* self);

/** Returns rotational speed of crystal based on Spirits deposited */
static s16 WL_crystal_get_angular_velocity_goal(){
    if (main_get_bits(BIT_222)) {
        return 6400;
    }
    if (main_get_bits(BIT_221)) {
        return 1600;
    }
    if (main_get_bits(BIT_21F_Spirit_Collected)) {
        return 800;
    }
    if (main_get_bits(BIT_21D)) {
        return 400;
    }
    if (main_get_bits(BIT_21C)) {
        return 200;
    }
    if (main_get_bits(BIT_Set_During_Spirit_Release_1)) {
        return 100;
    }

    /* @recomp: (almost) no rotation when 0 spirits deposited
       Based on description in Condensed Story document:
       "Krystal must take its spirit and release it within Warlock Mountain.
       This causes the huge floating diamond within the mountain to rotate."

       Also based on "goal" variable being initialised to 0 when no Spirit gamebits set
       ("goal" only influences rotation speed when speed less than goal, so crystal still
       spins in unpatched code due to fixed speed in setup function, possibly by mistake)
        
       Setting a very small value rather than 0 just to keep it looking "alive"! */
    return 10;
}

/** Adjust timing of sequence where Quan Ata Lachu crystal transforms into a star (originally by MusicalProgrammer) */
RECOMP_PATCH void WL_Crystal_setup(Object* self, WL_Crystal_Setup* objSetup, s32 arg2) {
    WL_Crystal_Data* objData;
    ModelInstance* modelInstance;
    Vtx *vertices;
    s16 i;

    objData = self->data;
    
    if ((gDLL_29_Gplay->vtbl->get_map_setup(self->mapID) == 3) && !main_get_bits(BIT_Set_During_Spirit_Release_1)){
        main_set_bits(BIT_Set_During_Spirit_Release_1, TRUE);
    }
    
    objData->inroomBuffer = NULL;
    objData->sunFX = NULL;
    objData->showCrystal = TRUE;
    
    //Set up Warlock Mountain's Crystal
    if (self->id == OBJ_WL_Crystal){
        self->srt.yaw = objSetup->yaw << 8;
        objData->yawSpeed = WL_crystal_get_angular_velocity_goal(); //@recomp: set initial speed based on Spirits deposited
        if (objSetup->scale >= 1000) {
            self->srt.scale = objSetup->scale / 1000.0f;
        } else {
            self->srt.scale = 1.0f;
        }
        return;
    }

    //Set up Quan Ata Lachu sun
    if (self->id == OBJ_WMsun){
        //@recomp: tweaked timing
        fxTimer1 = 368;
        fxTimer2 = 368;
        fxTimer3 = 368;
        fxTimer4 = 368;
        fxTimer5 = 368;

        self->srt.yaw = objSetup->yaw << 8;
        if (objSetup->scale >= 0) {
            self->srt.scale = objSetup->scale / 1000.0f;
        } else {
            self->srt.scale = 1.0f;
        }

        self->modelInstIdx = objSetup->modelIdx;        
        if (self->modelInstIdx == WMSun_Core) {
            objData->sunFX = dll_load_deferred(0x2012, 1);
            objData->yawSpeed = rand_next(300, 600);
            objData->rollSpeed = rand_next(300, 600);
            data_sun_modGFX = dll_load_deferred(0x1036, 1);
        } else {
            if (self->modelInstIdx == WMSun_Middle_Shell) {
                objData->yawSpeed = rand_next(500, 800);
                objData->rollSpeed = rand_next(500, 800);
            } else if (self->modelInstIdx == WMSun_Outer_Shell) {
                objData->sunFX = dll_load_deferred(0x2012, 1);
                objData->yawSpeed = rand_next(700, 1000);
                objData->rollSpeed = rand_next(700, 1000);
            }
        }

        self->opacity = 0;
        return;
    }

    //Set up room-filling electrified wall effect for main chamber (unused)
    //(Model uses 200x200px animated texture incompatible with N64 TMEM cache)
    if (self->id == OBJ_WMinroom){
        modelInstance = self->modelInsts[self->modelInstIdx];
        objData->inroomBuffer = mmAlloc(160, ALLOC_TAG_OBJECTS_COL, NULL);

        i = 20;
        while (i != 0) {
            i--;
            objData->inroomBuffer[i] = rand_next(0, modelInstance->model->vertexCount - 1);
            objData->inroomBuffer[i + 20] = 0;
            objData->inroomBuffer[i + 40] = rand_next(10, 20);
            objData->inroomBuffer[i + 60] = rand_next(80, 255);
        }
        
        //Set vertex alpha (animated vertex buffer 0)
        modelInstance = self->modelInsts[self->modelInstIdx];
        vertices = modelInstance->vertices[0];
        i = modelInstance->model->vertexCount;
        while (i != 0) {
            i--;
            vertices[i].n.a = 85;
        }        
        
        //Set vertex alpha (animated vertex buffer 1)
        vertices = modelInstance->vertices[1];
        i = modelInstance->model->vertexCount;
        while (i != 0) {
            i--;
            vertices[i].n.a = 85;
        }
        
        //Start at 0 opacity, and set overall scale
        self->opacity = 0;
        if (objSetup->scale) {
            self->srt.scale = 1.0f / (objSetup->scale / 1000.0f);
        }
    }
}

/** Adjust timing of sequence where Quan Ata Lachu crystal transforms into a star (originally by MusicalProgrammer) */
RECOMP_PATCH void WL_Crystal_control(Object* self) {
    WL_Crystal_Data* objData;
    TextureAnimator* animTexture;
    s16 pad;
    s16 yawAcceleration;
    s16 goal;
    f32 transformSpeed;
    SRT transform;
    s16 opacity;

    objData = self->data;

    goal = 0;
    yawAcceleration = 1;
    transformSpeed = 0.0f;

    //Handle Warlock Mountain's Crystal
    if (self->id == OBJ_WL_Crystal) {
        //Remove if gamebit 0x38F is set
        if (main_get_bits(BIT_WM_Quan_Ata_Lachu_Sun)) {
            obj_destroy_object(self);
        }
        
        //Scroll texture UVs
        animTexture = func_800348A0(self, 1, 0);
        if (animTexture) {
            animTexture->positionV -= 0x10;
            if (animTexture->positionV < -0x3E0) {
                animTexture->positionV = 0;
            }
        }

        //Rotate faster as more spirits are placed
        //(for first 6 spirits - transforms into sun afterwards)
        goal = WL_crystal_get_angular_velocity_goal(); //@recomp: goal speed gamebit checks split out for reuse in setup
        if (main_get_bits(BIT_222)) {
            yawAcceleration = 3;

            // goal = 6400; //unpatched value
            // goal = 5376; //MP's value (affects when crystal explodes: ~25.5s from 800->5376)
            // @recomp NOTE: using Rare's rotation speeds, but keeping MusicalProgrammer's cutscene timing for the explosion

            // transformSpeed = 0.00375f;   //unpatched value
            transformSpeed = 0.00475f;      //@recomp: faster transformation

            objData->explosionTimer += gUpdateRate;
        }
        
        //Increase rotation speed to goal speed
        if (objData->yawSpeed < goal) {
            objData->yawSpeed += gUpdateRate * yawAcceleration;

            //After 6th spirit placed, shrink and rise up
            if (transformSpeed){
                self->srt.scale -= transformSpeed * gUpdateRateF;
                //@recomp: prevent negative scale
                if (self->srt.scale < 0.0f){
                    self->srt.scale = 0.0f;
                }
                self->srt.transl.y += transformSpeed * gUpdateRateF * 54.0f; //@recomp: faster rate of ascent
            }
        }

        //@recomp: explode crystal at precise time (~25.5s)
        if ((main_get_bits(BIT_38D) == 0) && objData->explosionTimer >= 1524){
            //Set gamebits and destroy crystal 
            main_set_bits(BIT_38D, 1);
            main_set_bits(BIT_370, 0);
            objData->showCrystal = FALSE;
        }

        //While gamebit 0x38D not set and crystal spinning rapidly, 1% chance of camera shake(?)
        if (!main_get_bits(BIT_38D) && (objData->yawSpeed > 2400) && !rand_next(0, 100)) {
            camera_set_shake_offset(((objData->yawSpeed - 2400) / 2400.0f) * 0.8f);
            main_set_bits(BIT_370, 1);
        }
        self->srt.yaw += objData->yawSpeed;

        //Destroy once 6th spirit is deposited and transformation into sun complete
        if (!objData->showCrystal) {
            obj_destroy_object(self);
        }
        return;
    }

    //Handle room-filling electrified wall effect for main chamber of WM (unused)
    if (self->id == OBJ_WMinroom) {
        if (main_get_bits(BIT_WM_Quan_Ata_Lachu_Sun)) {
            //Increase opacity
            opacity = self->opacity; //@recomp: avoid uninitialised value
            if (self->opacity < WMinroom_Max_Opacity) {
                opacity = self->opacity + gUpdateRate;
            }
            if (opacity > WMinroom_Max_Opacity) {
                opacity = WMinroom_Max_Opacity;
            }
            self->opacity = opacity;

            //Scroll texture UVs
            animTexture = func_800348A0(self, 0, 0);
            if (animTexture != NULL) {
                animTexture->positionU -= gUpdateRate * 8;
                if (animTexture->positionU < -0x3E0) {
                    animTexture->positionU = 0;
                }
            }
        }
        return;
    }

    //Handle WMsun
    if (main_get_bits(BIT_WM_Quan_Ata_Lachu_Sun)) {
        //Fade in the Quan Ata Lachu sun
        opacity = self->opacity; //@recomp: avoid uninitialised value
        if (self->modelInstIdx == WMSun_Core && self->opacity != WMSun_Max_Opacity_Core) {
            if (self->opacity < WMSun_Max_Opacity_Core) {
                opacity = self->opacity + gUpdateRate;
            }
            if (opacity > WMSun_Max_Opacity_Core) {
                opacity = WMSun_Max_Opacity_Core;
            }
            self->opacity = opacity;
        } else if (self->modelInstIdx == WMSun_Middle_Shell && self->opacity != WMSun_Max_Opacity_Middle) {
            if (self->opacity < WMSun_Max_Opacity_Middle) {
                opacity = self->opacity + gUpdateRate;
            }
            if (opacity > WMSun_Max_Opacity_Middle) {
                opacity = WMSun_Max_Opacity_Middle;
            }
            self->opacity = opacity;
        } else if (self->modelInstIdx == WMSun_Outer_Shell && self->opacity != WMSun_Max_Opacity_Outer) {
            if (self->opacity < WMSun_Max_Opacity_Outer) {
                opacity = self->opacity + gUpdateRate;
            }
            if (opacity > WMSun_Max_Opacity_Outer) {
                opacity = WMSun_Max_Opacity_Outer;
            }
            self->opacity = opacity;
        }

        //Create particles and effects
        if (self->modelInstIdx == WMSun_Core) {
            gDLL_17_partfx->vtbl->spawn(self, 0x1A9, NULL, 0x10000, -1, NULL);
            gDLL_17_partfx->vtbl->spawn(self, 0x1A9, NULL, 0x10000, -1, NULL);

            //25% chance of 3D mesh light ray effect
            if (!rand_next(0, 4)) {
                //TODO: use exact DLL interface
                data_sun_modGFX->vtbl->func[0].withSixArgs((s32)self, 0, 0, 1, -1, 0);
            }

            //~0.6% chance of creating particles and playing sound effect
            if (!rand_next(0, 150)) {
                goal = 50;
                transform.transl.x = 0.0f;
                transform.transl.y = 0.0f;
                transform.transl.z = 0.0f;
                transform.scale = 1.0f;
                transform.roll = rand_next(0, 0xFFFF);
                transform.pitch = rand_next(0, 0xFFFF);
                transform.yaw = rand_next(0, 0xFFFF);
                gDLL_6_AMSFX->vtbl->play_sound(NULL, SOUND_WM_Sun_Whoosh, 0x43, NULL, NULL, 0, NULL);
                while (goal) {
                    goal--;
                    gDLL_17_partfx->vtbl->spawn(self, 0x1AA, &transform, 0x10000, -1, NULL);
                }
            }
            WL_Crystal_handle_sun_flare_effects(self);
        }
        return;
    }

    self->srt.roll += objData->rollSpeed;
    self->srt.yaw += objData->yawSpeed;
    if (main_get_bits(BIT_38D) == 0) {
        return;
    }

    //Handle crystal-to-sun transformation sequence effects
    if (self->modelInstIdx == WMSun_Core) {
        if (fxTimer4 == 0) {
            if ((fxTimer5 > 600) && !rand_next(0, 10)) {
                camera_set_shake_offset(2.8f);
            }
            if ((fxTimer5 < 700) && !rand_next(0, 5)) {
                objData->sunFX->vtbl->func[0].withSevenArgs((s32)self, 0, 0, 0x10000, -1, 0x12, 0);
            }
            if (fxTimer5 > 0) {
                fxTimer5 -= gUpdateRate;
                if ((fxTimer5 < 200) && !rand_next(0, 1)) {
                    transform.transl.x = (fxTimer5 / 200.0f) + 0.1f;
                    gDLL_17_partfx->vtbl->spawn(self, 0x1B1, &transform, 0x10000, -1, NULL);
                }

                goal = 200;
                if (fxTimer5 <= 0) {
                    fxTimer5 = 0;
                    while (goal) {
                        goal--;
                        gDLL_17_partfx->vtbl->spawn(self, 0x1B2, NULL, 0x10000, -1, NULL);
                    }
                    main_set_bits(BIT_38D, 0);
                    main_set_bits(BIT_WM_Quan_Ata_Lachu_Sun, 1);
                    func_80000860(self, self, 0x31, 0);
                    camera_set_shake_offset(4.8f);
                }
            }
        }

        if (fxTimer1 == 0) {
            if (!rand_next(0, (fxTimer2 + 2) / 200)) {
                gDLL_17_partfx->vtbl->spawn(self, 0x1AE, &transform, 0x10000, -1, NULL);
            }
            if (!rand_next(0, fxTimer2 / 70)) {
                gDLL_17_partfx->vtbl->spawn(self, 0x1AB, &transform, 0x10000, -1, NULL);
            }
            if (!rand_next(0, fxTimer2 / 70)) {
                gDLL_17_partfx->vtbl->spawn(self, 0x1AB, &transform, 0x10000, -1, NULL);
            }
            if (!rand_next(0, fxTimer2 / 70)) {
                gDLL_17_partfx->vtbl->spawn(self, 0x1AB, &transform, 0x10000, -1, NULL);
            }
            if (!rand_next(0, fxTimer2 / 70)) {
                gDLL_17_partfx->vtbl->spawn(self, 0x1AB, &transform, 0x10000, -1, NULL);
            }
            if (!rand_next(0, fxTimer2 / 70)) {
                gDLL_17_partfx->vtbl->spawn(self, 0x1AB, &transform, 0x10000, -1, NULL);
            }
            if (fxTimer2 > 0) {
                fxTimer2 -= gUpdateRate;
                if (fxTimer2 < 0) {
                    fxTimer2 = 0;
                }
            }
        } else {
            transform.transl.x = 0.1f;
            gDLL_17_partfx->vtbl->spawn(self, 0x1B0, &transform, 0x10000, -1, NULL);
            if ((fxTimer1 > 50) && !rand_next(0, 1)) {
                transform.transl.x = ((fxTimer1 - 50) / 750.0f) + 0.1f;
                gDLL_17_partfx->vtbl->spawn(self, 0x1B0, &transform, 0x10000, -1, NULL);
            }
            if (fxTimer1 < 700) {
                goal = fxTimer1 / 60;
                while (goal) {
                    goal--;
                    transform.transl.x = fxTimer1 / 300.0f;
                    gDLL_17_partfx->vtbl->spawn(self, 0x1AF, &transform, 0x10000, -1, NULL);
                }
            }
            if (fxTimer1 > 0) {
                fxTimer1 -= gUpdateRate;
                if (fxTimer1 <= 0) {
                    fxTimer1 = 0;
                    func_80000860(self, self, 0x30, 0);
                    func_80000860(self, self, 0x34, 0);
                }
            }
            if (rand_next(0, 8) == 0) {
                camera_set_shake_offset(2.8f);
            }
        }
    }

    if ((self->modelInstIdx == WMSun_Middle_Shell) && (fxTimer2 == 0)) {
        if (rand_next(0, fxTimer3 / 60) == 0) {
            gDLL_17_partfx->vtbl->spawn(self, 0x1AC, NULL, 0x10000, -1, NULL);
        }
        if (rand_next(0, fxTimer3 / 60) == 0) {
            gDLL_17_partfx->vtbl->spawn(self, 0x1AC, NULL, 0x10000, -1, NULL);
        }
        if (rand_next(0, fxTimer3 / 60) == 0) {
            gDLL_17_partfx->vtbl->spawn(self, 0x1AC, NULL, 0x10000, -1, NULL);
        }
        if (fxTimer3 > 0) {
            fxTimer3 -= gUpdateRate;
            if (fxTimer3 < 0) {
                fxTimer3 = 0;
            }
        }
    }
    
    if ((self->modelInstIdx == WMSun_Outer_Shell) && (fxTimer2 <= 0) && (fxTimer3 <= 0)) {
        if (rand_next(0, fxTimer4 / 60) == 0) {
            gDLL_17_partfx->vtbl->spawn(self, 0x1AD, NULL, 0x10000, -1, NULL);
        }
        if (rand_next(0, fxTimer4 / 60) == 0) {
            gDLL_17_partfx->vtbl->spawn(self, 0x1AD, NULL, 0x10000, -1, NULL);
        }
        if (fxTimer4 > 0) {
            fxTimer4 -= gUpdateRate;
            if (fxTimer4 <= 0) {
                fxTimer4 = 0;
            }
        }
    }
}

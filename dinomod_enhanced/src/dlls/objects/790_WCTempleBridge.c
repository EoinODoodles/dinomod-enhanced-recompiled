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

#include "recomp/dlls/_asm/790_recomp.h"

// #define RAINBOW_BRIDGE

//TEMPORARY DEFINES
#define WCTempleBridge_obj_Setup dll_790_obj_Setup
#define WCTempleBridge_obj_Control dll_790_obj_Control
#define WCTempleBridge_obj_GetDataSize dll_790_obj_GetDataSize
#define WCTempleBridge_animCallback dll_790_func_500
#define WCTempleBridge_advanceAnimation dll_790_func_644
#define WCTempleBridge_updateVertices dll_790_func_7A4
//END OF TEMPORARY DEFINES

typedef struct {
    ObjSetup base;
    s8 yaw;
    s8 modelIdx;                    //Which bridge model to use
    s16 hitsAnimatorID;             //@recomp: repurpose unused field
    s16 unk1C;
    s16 gamebitVisible;             //Stores the bridge's visibility state
} WCTempleBridge_Setup;

typedef struct {
    f32 minZ;                       //The position of the vertex furthest from the model's origin along Z (will be negative, and effectively bridge's length)
    f32 vertexZs[15];               //A array of unique Z positions extracted from the model's vertices (with +-10 tolerance), sorted along negative Z
    u8 vertexFadeIn[15];            //Causes a unique vertex opacity to fade in when true (forced on)
    u8 vertexZCount;                //A count of the unique vertex Z values stored in the vertexZ array (calculation gets overridden to 10 later on in setup)
    u8 vertexAlphas[15];            //Vertex colour alpha values for each Z position in the vertexZs array
    u8 visible;                     //The bridge is drawn when this is set
    u16 phaseAngleA;                //Angle value for the vertices' sinusoidal waving animation
    u16 phaseAngleB;                //Advances, but not used for anything
    u16 unk64;                      //Unused
    u8 flags;                       //Tracks whether the gamebit has been set
    /* RECOMP */
    s16 scrollU;                    //How far the outer cloud's faces have scrolled away from their initial U axis position
    s16 scrollV;                    //How far the outer cloud's faces have scrolled away from their initial V axis position
    s16 secondaryScrollV;           //How far the inner cloud's faces have scrolled away from their initial V axis position
} WCTempleBridge_Data;

typedef enum {
    WCTempleBridge_FLAG_Visibility_Gamebit_Set = 1
} WCTempleBridge_Flags;

typedef enum {
    WCTempleBridge_CUSTOMFLAG_Outer_Scroll_U_Wrap = 2,
    WCTempleBridge_CUSTOMFLAG_Outer_Scroll_V_Wrap = 4,
    WCTempleBridge_CUSTOMFLAG_Inner_Scroll_V_Wrap = 8
} WCTempleBridge_CustomFlags;

#define MAX_OPACITY 0xFF
#define OUTER_SCROLL_SPEED_U 2
#define OUTER_SCROLL_SPEED_V 4
#define INNER_SCROLL_SPEED_V 5
#define UV_RANGE_WRAP 0x400

extern void WCTempleBridge_advanceAnimation(Object* self, WCTempleBridge_Data* objData);
extern void WCTempleBridge_updateVertices(Object* self, WCTempleBridge_Data* objData);
extern int WCTempleBridge_animCallback(Object* self, Object* animObj, AnimObj_Data* animData, s8 prevCallbackValue);

RECOMP_PATCH void WCTempleBridge_obj_Control(Object* self) {
    WCTempleBridge_Data* objData;
    s32 opacity;
    s32 i;
    WCTempleBridge_Setup* objSetup;

    objData = self->data;
    objSetup = (WCTempleBridge_Setup*)self->setup;
    
    WCTempleBridge_advanceAnimation(self, objData);

    if (objData->visible) {
        if ((objData->flags & WCTempleBridge_FLAG_Visibility_Gamebit_Set) == FALSE) {
            objData->flags |= WCTempleBridge_FLAG_Visibility_Gamebit_Set;
            mainSetBits(objSetup->gamebitVisible, TRUE);
        }

        for (i = 0; i < objData->vertexZCount; i++) {
            objData->vertexFadeIn[i] = TRUE;
            if (objData->vertexFadeIn[i]) {
                    opacity = objData->vertexAlphas[i] + gUpdateRate;
                    if (opacity > MAX_OPACITY) {
                        opacity = MAX_OPACITY;
                    }
                objData->vertexAlphas[i] = opacity;
            }
        }

        //@recomp: remove HITS line
        if (objSetup->hitsAnimatorID > 0) {
            trackToggleHitLine(objSetup->hitsAnimatorID, self->parent, FALSE);
        }
        
        func_8002674C(self);
    } else {
        func_800267A4(self);
    }
    
    WCTempleBridge_updateVertices(self, objData);
}

/* Storing scroll positions to objData instead of using TextureAnimators, to avoid seams/desyncs between bridge faces */
RECOMP_PATCH void WCTempleBridge_advanceAnimation(Object* self, WCTempleBridge_Data* objData) {
    // TextureAnimator* texAnim;
    s32 angle;
    s32 i;

    //Scroll texture UVs (@recomp: handled differently, to avoid seam that appears when using TextureAnimators)
    {
        objData->scrollV += OUTER_SCROLL_SPEED_V * gUpdateRate; //@recomp: fix framerate dependency
        if (objData->scrollV > UV_RANGE_WRAP) {
            objData->scrollV -= UV_RANGE_WRAP;
            objData->flags |= WCTempleBridge_CUSTOMFLAG_Outer_Scroll_V_Wrap;
        }
        
        objData->scrollU += OUTER_SCROLL_SPEED_U * gUpdateRate; //@recomp: fix framerate dependency
        if (objData->scrollU > UV_RANGE_WRAP) {
            objData->scrollU -= UV_RANGE_WRAP;
            objData->flags |= WCTempleBridge_CUSTOMFLAG_Outer_Scroll_U_Wrap;
        }
        
        objData->secondaryScrollV += INNER_SCROLL_SPEED_V * gUpdateRate; //@recomp: fix framerate dependency
        if (objData->secondaryScrollV > UV_RANGE_WRAP) {
            objData->secondaryScrollV -= UV_RANGE_WRAP;
            objData->flags |= WCTempleBridge_CUSTOMFLAG_Inner_Scroll_V_Wrap;
        }
    }
    
    //Advance phaseAngleA
    angle = objData->phaseAngleA + (gUpdateRate << 8);
    if (angle >= M_360_DEGREES) {
        angle += 0xFFFF0001;
    }
    objData->phaseAngleA = angle;
    
    //Advance phaseAngleB (not used for anything?)
    angle = objData->phaseAngleB + (gUpdateRate << 7);
    if (angle >= M_360_DEGREES) {
        angle += 0xFFFF0001;
    }
    objData->phaseAngleB = angle;
}

/* Instead of using TextureAnimators, manually scroll the bridge's UVs to avoid seams/desyncs between bridge faces */
RECOMP_PATCH void WCTempleBridge_updateVertices(Object* self, WCTempleBridge_Data* objData) {
    ModelInstance* modelInst;
    Model* model;
    Vtx* vertices;
    s32 vertexIdx;
    s32 shapeIdx;
    f32 tValue;
    s32 phase;
    s32 idx;

    //@recomp: Wrap all the vertices when they go too far from their origin on a UV axis
    u8 wrapUOuter = objData->flags & WCTempleBridge_CUSTOMFLAG_Outer_Scroll_U_Wrap;
    u8 wrapVOuter = objData->flags & WCTempleBridge_CUSTOMFLAG_Outer_Scroll_V_Wrap;
    u8 wrapVInner = objData->flags & WCTempleBridge_CUSTOMFLAG_Inner_Scroll_V_Wrap;

    modelInst = self->modelInsts[self->modelInstIdx];
    model = modelInst->model;
    vertices = modelInst->vertices[(modelInst->unk34 >> 1) & 1];
    
    for (shapeIdx = 0; shapeIdx < model->unk70; shapeIdx++) {
        for (vertexIdx = model->faces[shapeIdx].baseVertexID; vertexIdx < model->faces[shapeIdx + 1].baseVertexID; vertexIdx++) {
            //Get the vertex's animation phase angle, based on its tValue position in Z along the bridge
            tValue = vertices[vertexIdx].v.ob[2] / objData->minZ;
            phase = (((u32) (tValue * (M_360_DEGREES - 1))) & 0xFFFF) + objData->phaseAngleA;
            
            //Oscillate the vertex along X axis (mirrored across X)
            if (model->vertices[vertexIdx].v.ob[0] > 0) {
                vertices[vertexIdx].v.ob[0] = model->vertices[vertexIdx].v.ob[0] + (mathSinfInterp(phase) * 20.0f);
            } else {
                vertices[vertexIdx].v.ob[0] = model->vertices[vertexIdx].v.ob[0] - (mathSinfInterp(phase) * 20.0f);
            }

#ifdef RAINBOW_BRIDGE
            //Fun with colours!
            {
                u8 r;
                u8 g;
                u8 b;
                s32 phaseTemp;
                f32 phaseC;

                phaseTemp = (((u32) (tValue * (M_360_DEGREES - 1))) & 0xFFFF) - objData->phaseAngleA + M_360_DEGREES;
                phaseC = phaseTemp / M_360_DEGREES_F;

                colourHSVtoRGB(phaseC, 1.0f, 1.0f, &r, &g, &b);

                if (model->faces[shapeIdx].tagB <= 10) {
                    vertices[vertexIdx].v.cn[0] = r;
                    vertices[vertexIdx].v.cn[1] = g;
                    vertices[vertexIdx].v.cn[2] = b;
                } else {
                    vertices[vertexIdx].v.cn[0] = lerp_float(0.5f, r, model->vertices[vertexIdx].v.cn[0]);
                    vertices[vertexIdx].v.cn[1] = lerp_float(0.5f, g, model->vertices[vertexIdx].v.cn[1]);
                    vertices[vertexIdx].v.cn[2] = lerp_float(0.5f, b, model->vertices[vertexIdx].v.cn[2]);
                }
            }
#endif
            
            //Animate vertex opacity (only on vertices with nonzero initial alpha)
            if (model->vertices[vertexIdx].v.cn[3] != 0) {

                idx = model->faces[shapeIdx].tagB;
                if (idx > 10) {
                    idx -= 10;
                }
                idx--;
                
                //Use the vertex's model tags and Z position to determine which vertexAlpha index to use
                if (vertices[vertexIdx].v.ob[2] >= (s16)objData->vertexZs[idx] - 8) {
                    vertices[vertexIdx].v.cn[3] = objData->vertexAlphas[idx];
                } else if (objData->vertexFadeIn[idx + 1]) {
                    vertices[vertexIdx].v.cn[3] = objData->vertexAlphas[idx + 1];
                } else {
                    vertices[vertexIdx].v.cn[3] = 0;
                }
            }

            //@recomp: handle texture scrolling manually, to avoid seams appearing
            if (model->faces[shapeIdx].tagB <= 10) {
                if (wrapUOuter) {
                    model->vertices[vertexIdx].v.tc[0] += (OUTER_SCROLL_SPEED_U * gUpdateRate) - UV_RANGE_WRAP;
                } else {
                    model->vertices[vertexIdx].v.tc[0] += (OUTER_SCROLL_SPEED_U * gUpdateRate);
                }

                if (wrapVOuter) {
                    model->vertices[vertexIdx].v.tc[1] += (OUTER_SCROLL_SPEED_V * gUpdateRate) - UV_RANGE_WRAP;
                } else {
                    model->vertices[vertexIdx].v.tc[1] += (OUTER_SCROLL_SPEED_V * gUpdateRate);
                }
            } else {
                if (wrapVOuter) {
                    model->vertices[vertexIdx].v.tc[1] += (INNER_SCROLL_SPEED_V * gUpdateRate) - UV_RANGE_WRAP;
                } else {
                    model->vertices[vertexIdx].v.tc[1] += (INNER_SCROLL_SPEED_V * gUpdateRate);
                }
            }
        }   
    }

    //Clear wrap flags
    objData->flags &= ~(WCTempleBridge_CUSTOMFLAG_Outer_Scroll_U_Wrap | 
                        WCTempleBridge_CUSTOMFLAG_Outer_Scroll_V_Wrap | 
                        WCTempleBridge_CUSTOMFLAG_Inner_Scroll_V_Wrap);
}

RECOMP_PATCH u32 WCTempleBridge_obj_GetDataSize(Object* self, u32 offsetAddr) {
    return sizeof(WCTempleBridge_Data);
}

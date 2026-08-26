#include "modding.h"
#include "recomputils.h"
#include "recompconfig.h"

#include "common.h"
#include "sys/gfx/texture.h"

#include "block_util.h"
#include "common_objsetups.h"

#include "recomp/dlls/_asm/350_recomp.h"

//TEMPORARY DEFINES
#define AlphaAnimator_obj_Control dll_350_obj_Control
#define AlphaAnimator_animateVertices dll_350_func_808
#define AlphaAnimator_calculateAnimatedVertexDistances dll_350_func_C8C
//END OF TEMPORARY DEFINES

// #define DEBUG_OPAQUE_FLAG

typedef struct {
    s32 animatedVertexCount;                //How many vertices are affected by the AlphaAnimator
    f32 fadeRadiusOuter;                    //(For radial mode) Current radius of the radial fade (+50)
    f32 fadeRadiusInner;                    //(For radial mode) Current radius of the radial fade
    f32 fadeRadiusGoal;                     //(For radial mode) End radius of the fade animation
    f32* vtxDistances;                      //(For radial mode) Distance between the AlphaAnimator and each vertex it animates 
    s16 vtxOpacity;                         //Current vertex opacity (for non-radial modes)
    s8 animatorID;                          //Vertices will only be affected if they have this shape animatorID
    s8 fadeActivated;                       //Fade animation started
    s8 fadeCompletedTicks;                  //Ticks since a fade animation has completed
    s8 prevFadeActivated;                   //Whether the fade was activated on the previous tick (used to tell when fade has just been activated)
} AlphaAnimator_Data;

#define GET_MODE(flags) (flags & 3)

//@recomp: check a specific bit, freeing up the higher bits (this is safe, since all existing AlphaAnimator objSetups only use bit2 to denote playing a sound)
#define SHOULD_SOUND_PLAY(flags) (flags & AlphaAnimator_FLAG_Play_Sound) 

#define MAX_OPACITY 255

extern void AlphaAnimator_animateVertices(AlphaAnimator_Data* objData, AlphaAnimator_Setup* objSetup, Block* block);
extern void AlphaAnimator_calculateAnimatedVertexDistances(Object* self, AlphaAnimator_Data* objData);

/** 
  * Change the "SHOULD_SOUND_PLAY" condition so it specifically checks bit2 of the flags, 
  * freeing up the higher bits for flagging custom things.
  *
  * All the game's existing AlphaAnimators use bit2 for flagging the sound, so this should be safe!
  */
RECOMP_PATCH void AlphaAnimator_obj_Control(Object* self) {
    AlphaAnimator_Data* objData;
    AlphaAnimator_Setup* objSetup;
    Block* block;
    s32 fadeSpeed;
    s32 pad;
    s32 opacityDiff;
    s32 mode;

    objSetup = (AlphaAnimator_Setup*)self->setup;
    objData = self->data;
    
    mode = GET_MODE(objSetup->flags);
    
    //Get the object's local BLOCKS model
    block = mapGetBlockByIndex(mapWorldCoordsToBlockIndex(self->srt.transl.x, self->srt.transl.f[1], self->srt.transl.f[2]));
    if (block == NULL) {
        objData->fadeCompletedTicks = 0;
        return;
    }
    
    //Bail if the BLOCKS model isn't animatable
    if ((block->vtxFlags & 8) == FALSE) {
        return;
    }
    
    //Set up animated vertices
    if (objData->animatedVertexCount == 0) {
        objData->animatorID = objSetup->animatorID;
        objData->animatedVertexCount = blockGetAnimatorVertexCount(self, objData->animatorID);
        
        //If no animatable vertices were found, zero out the animatorID
        if (objData->animatedVertexCount == 0) {
            objData->animatorID = 0;
        }
        
        if (objData->animatorID != 0) {
            objData->fadeRadiusOuter = 0.0f;
            objData->fadeRadiusInner = 0.0f;
            objData->fadeRadiusGoal = objSetup->fadeRadiusGoal;

            if (objSetup->gamebitActivate == NO_GAMEBIT) {
                objData->fadeActivated = TRUE;
            } else {
                objData->fadeActivated = mainGetBits(objSetup->gamebitActivate);
            }
            
            objData->vtxOpacity = objSetup->initialOpacity;

            if ((objSetup->gamebitSetWhenFaded != NO_GAMEBIT) && mainGetBits(objSetup->gamebitSetWhenFaded)) {
                objData->vtxOpacity = objSetup->goalOpacity;
                objData->fadeActivated = TRUE;
                objData->fadeRadiusOuter = objData->fadeRadiusGoal + 1.0f;
            }

            if (mode == AlphaAnimator_MODE_3_Radial_Falloff_Fade) {
                objData->vtxDistances = mmAlloc(objData->animatedVertexCount * sizeof(f32), ALLOC_TAG_TRACK_COL, NULL);
                AlphaAnimator_calculateAnimatedVertexDistances(self, objData);
            }

            AlphaAnimator_animateVertices(objData, objSetup, block);
            block->vtxFlags ^= 1;
            AlphaAnimator_animateVertices(objData, objSetup, block);
            block->vtxFlags ^= 1;
        } else {
            return;
        }
    } 
    
    //Bail if no shape animation tag is specified
    if (objData->animatorID == 0) {
        return;
    }
    
    if (mode == AlphaAnimator_MODE_2_Toggleable_Fade) {
        objData->fadeActivated = mainGetBits(objSetup->gamebitActivate);
        if ((objData->fadeCompletedTicks >= 3) && (objData->fadeActivated != objData->prevFadeActivated)) {
            //Optionally play a sound soon after the fade is completed
            if (SHOULD_SOUND_PLAY(objSetup->flags)) {
                gDLL_6_AMSFX->vtbl->Play(self, objSetup->soundID, MAX_VOLUME, NULL, NULL, 0, NULL);
            }

            objData->fadeCompletedTicks = 0;
            objData->prevFadeActivated = objData->fadeActivated;
        }

        if (objData->fadeCompletedTicks >= 3) {
            return;
        }
    } else {
        if (objData->fadeCompletedTicks >= 3) {
            return;
        }
        
        //Don't continue to the fade State Machine until gamebitActivate is set
        if (objData->fadeActivated == FALSE) {
            objData->fadeActivated = mainGetBits(objSetup->gamebitActivate);

            //Optionally play a sound when fading in
            if (objData->fadeActivated) {
                if (SHOULD_SOUND_PLAY(objSetup->flags)) {
                    gDLL_6_AMSFX->vtbl->Play(self, objSetup->soundID, MAX_VOLUME, NULL, NULL, 0, NULL);
                }
            } else {
                return;
            }
            
            if (objSetup->gamebitActivate) { }
        }
    }
    
    switch (mode) {
    case AlphaAnimator_MODE_0_Basic_Fade_and_Set_Gamebit:
        if (objSetup->goalOpacity < objSetup->initialOpacity) {
            //Fade out
            objData->vtxOpacity -= objSetup->fadeSpeed * gUpdateRate;
            if (objData->vtxOpacity <= objSetup->goalOpacity) {
                objData->vtxOpacity = objSetup->goalOpacity;

                //Optionally set a gamebit when the goal opacity has been reached
                if (objSetup->gamebitSetWhenFaded != NO_GAMEBIT) {
                    mainSetBits(objSetup->gamebitSetWhenFaded, TRUE);
                }

                objData->fadeCompletedTicks++;
            }
        } else {
            //Fade in
            objData->vtxOpacity += objSetup->fadeSpeed * gUpdateRate;
            if (objData->vtxOpacity >= objSetup->goalOpacity) {
                objData->vtxOpacity = objSetup->goalOpacity;

                //Optionally set a gamebit when the goal opacity has been reached
                if (objSetup->gamebitSetWhenFaded != NO_GAMEBIT) {
                    mainSetBits(objSetup->gamebitSetWhenFaded, TRUE);
                }

                objData->fadeCompletedTicks++;
            }
        }
        break;
    case AlphaAnimator_MODE_1_Basic_Fade:
        if (objSetup->goalOpacity < objSetup->initialOpacity) {
            //Fade out
            objData->vtxOpacity -= objSetup->fadeSpeed * gUpdateRate;
            if (objData->vtxOpacity < objSetup->goalOpacity) {
                opacityDiff = objSetup->goalOpacity - objData->vtxOpacity;
                objData->vtxOpacity = objSetup->initialOpacity - opacityDiff;
            }
        } else {
            //Fade in
            objData->vtxOpacity += objSetup->fadeSpeed * gUpdateRate;
            if (objData->vtxOpacity > objSetup->initialOpacity) {
                opacityDiff = objData->vtxOpacity - objSetup->goalOpacity;
                objData->vtxOpacity = objSetup->goalOpacity + opacityDiff;
            }
        }
        break;
    case AlphaAnimator_MODE_2_Toggleable_Fade:
        if (objData->fadeActivated) {
            if (objSetup->goalOpacity < objSetup->initialOpacity) {
                //Fade out
                objData->vtxOpacity -= objSetup->fadeSpeed * gUpdateRate;
                if (objData->vtxOpacity <= objSetup->goalOpacity) {
                    objData->vtxOpacity = objSetup->goalOpacity;

                    //Optionally set a gamebit when the goal opacity has been reached
                    if (objSetup->gamebitSetWhenFaded != NO_GAMEBIT) {
                        mainSetBits(objSetup->gamebitSetWhenFaded, TRUE);
                    }

                    objData->fadeCompletedTicks++;
                }
            } else {
                //Fade in
                objData->vtxOpacity += objSetup->fadeSpeed * gUpdateRate;
                if (objData->vtxOpacity >= objSetup->goalOpacity) {
                    objData->vtxOpacity = objSetup->goalOpacity;

                    //Optionally set a gamebit when the goal opacity has been reached
                    if (objSetup->gamebitSetWhenFaded != NO_GAMEBIT) {
                        mainSetBits(objSetup->gamebitSetWhenFaded, TRUE);
                    }

                    objData->fadeCompletedTicks++;
                }
            }
        } else if (objSetup->goalOpacity < objSetup->initialOpacity) {
            //Fade back in
            objData->vtxOpacity += objSetup->fadeSpeed * gUpdateRate;
            if (objData->vtxOpacity >= objSetup->initialOpacity) {
                objData->vtxOpacity = objSetup->initialOpacity;

                //Optionally unset a gamebit when the fade has returned to its initial opacity
                if (objSetup->gamebitSetWhenFaded != NO_GAMEBIT) {
                    mainSetBits(objSetup->gamebitSetWhenFaded, FALSE);
                }

                objData->fadeCompletedTicks++;
            }
        } else {
            //Fade back out
            objData->vtxOpacity -= objSetup->fadeSpeed * gUpdateRate;
            if (objData->vtxOpacity <= objSetup->initialOpacity) {
                objData->vtxOpacity = objSetup->initialOpacity;

                //Optionally unset a gamebit when the fade has returned to its initial opacity
                if (objSetup->gamebitSetWhenFaded != NO_GAMEBIT) {
                    mainSetBits(objSetup->gamebitSetWhenFaded, FALSE);
                }

                objData->fadeCompletedTicks++;
            }
        }
        break;
    case AlphaAnimator_MODE_3_Radial_Falloff_Fade:
        fadeSpeed = (objSetup->fadeSpeed < 0) ? -objSetup->fadeSpeed : objSetup->fadeSpeed;
        
        objData->fadeRadiusOuter += (fadeSpeed / 10.0f) * gUpdateRateF;
        if (objData->fadeRadiusOuter > objData->fadeRadiusGoal) {
            objData->fadeRadiusOuter = objData->fadeRadiusGoal;
            mainSetBits(objSetup->gamebitSetWhenFaded, TRUE);
            objData->fadeCompletedTicks++;
        }

        objData->fadeRadiusInner = objData->fadeRadiusOuter - 50.0f;
        break;
    }
    
    AlphaAnimator_animateVertices(objData, objSetup, block);
}

/* Optionally change render flags at full opacity, to prevent other transparent surfaces being drawn behind the shape */
RECOMP_PATCH void AlphaAnimator_animateVertices(AlphaAnimator_Data* objData, AlphaAnimator_Setup* objSetup, Block* block) {
    f32 tOpacity;
    s32 vtxIdx;
    BlockShape* shapes;
    Vtx_t* vertices;
    Vtx_t* vtx;
    s32 animVtxIdx;
    s32 shapeIdx;

    vertices = block->vertices2[block->vtxFlags & 1];
    
    //Loop over the block's shapes
    for (shapeIdx = 0, animVtxIdx = 0, shapes = block->shapes; shapeIdx < block->shapeCount; shapeIdx++) {
        //Check if the shape has a matching animatorID tag
        if (objData->animatorID == shapes[shapeIdx].animatorID) {
            //Loop over the shape's vertices
            for (vtxIdx = shapes[shapeIdx].vtxBase; vtxIdx < shapes[shapeIdx + 1].vtxBase; vtxIdx++) {
                if (GET_MODE(objSetup->flags) != AlphaAnimator_MODE_3_Radial_Falloff_Fade) {
                    vertices[vtxIdx].cn[3] = objData->vtxOpacity;
                } else {
                    //Mode 3: Radial Falloff Fade
                    if (objData->fadeActivated) {
                        if (block->vertices[vtxIdx].cn[3] != 0) { //Ignore vertices if they have 0 opacity painted
                            //Calculate the vertex's opacity tValue, based on its distance and the current fade radius
                            tOpacity = (objData->vtxDistances[animVtxIdx] - objData->fadeRadiusInner) * 0.02f;
                            if (tOpacity > 1.0f) {
                                tOpacity = 1.0f;
                            } else if (tOpacity < 0.0f) {
                                tOpacity = 0.0f;
                            }
                            
                            //Optionally invert behaviour
                            if (objSetup->fadeSpeed < 0) {
                                //Fade out vertices inside the radius
                                vertices[vtxIdx].cn[3] = tOpacity * MAX_OPACITY;
                            } else {
                                //Or fade out vertices outside the radius
                                vertices[vtxIdx].cn[3] = (1.0f - tOpacity) * MAX_OPACITY;
                            }
                        }
                        animVtxIdx++;
                    } else {
                        vertices[vtxIdx].cn[3] = objSetup->initialOpacity;
                    }
                }
            }
            
            //Update shape's render flags
            if (GET_MODE(objSetup->flags) != AlphaAnimator_MODE_3_Radial_Falloff_Fade) {
                if (objData->vtxOpacity == 0) {
                    shapes[shapeIdx].flags |= RENDER_SHAPE_HIDE;
                    if (objSetup->removeCollisionWhenHidden) {
                        shapes[shapeIdx].flags |= RENDER_UNK800;
                    }
                } else {
                    shapes[shapeIdx].flags &= ~RENDER_SHAPE_HIDE;
                    if (objSetup->removeCollisionWhenHidden) {
                        shapes[shapeIdx].flags &= ~RENDER_UNK800;
                    }

                }

                //@recomp: Optionally change to fully opaque rendering when at max opacity 
                //(prevents other semi-transparent shapes from showing through this shape)
                if (objSetup->flags & AlphaAnimator_CUSTOMFLAG_Opaque_at_Max_Opacity) {
                    #define RENDER_VTX_ALPHA (RENDER_SEMI_TRANSPARENT | RENDER_DECAL_SIMPLE)

                    if (objData->vtxOpacity == MAX_OPACITY) {
                        if (shapes[shapeIdx].flags & RENDER_VTX_ALPHA) {
                            shapes[shapeIdx].flags &= ~RENDER_VTX_ALPHA;
                            blockRedoShapeDLGroups(block, shapeIdx);
                            
#ifdef DEBUG_OPAQUE_FLAG
                            recomp_printf("Opaque opacity %d! Changed render mode to: %08x\n", objData->vtxOpacity, shapes[shapeIdx].flags);
#endif
                        } 
                    } else {
                        if ((shapes[shapeIdx].flags & RENDER_VTX_ALPHA) != RENDER_VTX_ALPHA) {
                            shapes[shapeIdx].flags |= RENDER_VTX_ALPHA;
                            blockRedoShapeDLGroups(block, shapeIdx);

#ifdef DEBUG_OPAQUE_FLAG
                            recomp_printf("Opacity is %d! Changed render mode to: %08x\n", objData->vtxOpacity, shapes[shapeIdx].flags);
#endif
                        } 
                    }
                }
            } else {
                //Mode 3: Radial Falloff Fade
                if (objData->fadeActivated) {
                    shapes[shapeIdx].flags &= ~RENDER_SHAPE_HIDE;
                    if ((objData->fadeCompletedTicks != 0) && (objSetup->goalOpacity == 0)) {
                        shapes[shapeIdx].flags |= RENDER_SHAPE_HIDE;
                    }
                } else if (objSetup->initialOpacity == 0) {
                    shapes[shapeIdx].flags |= RENDER_SHAPE_HIDE;
                }
            }
        }
    }
}

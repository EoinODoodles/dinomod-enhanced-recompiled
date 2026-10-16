#include "configs.h"
#include "dbgui.h"
#include "math_util.h"
#include "modding.h"
#include "recomputils.h"

#include "PR/ultratypes.h"
#include "common.h"
#include "dlls/engine/17_partfx.h"
#include "dlls/objects/210_player.h"
#include "game/objects/object_id.h"
#include "game/objects/object.h"
#include "macros.h"
#include "sys/camera.h"
#include "sys/math.h"
#include "sys/objanim.h"
#include "sys/objects.h"
#include "sys/objexpr.h"
#include "sys/print.h"
#include "sys/rand.h"

#include "recomp/dlls/engine/32_modelfx_recomp.h"
#include "recomp/dlls/objects/210_player_recomp.h"

// #define DEBUG_EYE_GLOW
// #define DEBUG_OCCLUSION
// #define EDIT_GLOWS

#ifdef EDIT_GLOWS
#include "3d.h"
#endif

extern s32 _data_1864;

extern s32 dll_32_func_1D3C(Vec3f* A, Vec3f* B, f32* arg2);

typedef struct {
    f32 u;
    f32 v;
} VecUV;

typedef struct {
    s32 u;
    s32 v;
} SVecUV;

typedef enum {
    MODEL_Krystal_Gameplay = 0,
    MODEL_Krystal_Anim = 1,
    MODEL_Krystal_HQ_Textures = 2,  //Unused, same polycount as gameplay/illusion but with textures on back-of-head/arms
    MODEL_Krystal_Shadow = 3,
    MODEL_Krystal_Illusion = 4,

    MODEL_Sabre_Gameplay = 7,       //Recovered from Sabre's placeholder Illusion Spell model
    MODEL_Sabre_Anim = 8,           //Formerly unused HQ textures model, now with MusicalProgrammer's blendshapes
    MODEL_Fox_Gameplay = 9,         //Gameplay model in vanilla
    MODEL_Sabre_Shadow = 10,        //Rare just used a copy of Krystal's shadow model for this
    MODEL_Fox_Anim = 11             //Anim model in vanilla
} DinomodEnhanced_PlayerModels;

typedef struct {
    u16 eyeL;               //facebatchID for left eye, used to find head joint's matrix
    u16 eyeR;               //facebatchID for right eye, used to find head joint's matrix
    u16 pendantID;          //facebatchID for pendant, used to find upper body joint's matrix
    u8 blink;               //whether to hide glows during character's blinks (switch off for Fox/illusion spell: models that aren't set up to blink)
    Vec3f eyeCoords;        //local coords relative to head joint (mirrored across X for the right eye)
    Vec3f pendantCoords;    //local coords relative to upper body joint
    Vec3f eyeNormal;        //precalculated surface normal for determining eye visibility (whether eye polygons are pointing towards/away from camera)
    Vec3f eyeLDirU;         //direction for glow to move when sliding along L eye's U axis
    Vec3f eyeLDirV;         //direction for glow to move when sliding along L eye's V axis
    //NOTE: separate L/R UV vectors because Sabre's eyes' UV planes aren't symmetrical (small rotation compared to the other)
    Vec3f eyeRDirU;         //direction for glow to move when sliding along R eye's U axis
    Vec3f eyeRDirV;         //direction for glow to move when sliding along R eye's V axis
    VecUV multiplier;       //scale factor for eyes' U/V axes
    VecUV eyeLOffset;       //origin offset for L eye's U/V axes
    VecUV eyeROffset;       //origin offset for R eye's U/V axes
    //TODO: separate offset values for seqs may not be necessary? Need to figure out why coords are working out slightly differently when objExprEyeDart is in use vs. when objSeqs are controlling eye UVs!
    VecUV eyeLOffsetSeq;    //origin offset for L eye's U/V axes during sequences
    VecUV eyeROffsetSeq;    //origin offset for R eye's U/V axes during sequences
    //TO-DO: normalise all these vectors in advance? Could factor in the multiplierU and multiplierV ahead of time, too.
} SpiritGlowData;

/** NOTE: for use with Dinomod Enhanced's modified character models */
static SpiritGlowData glowData[2][4] = {
    //Krystal
    {
        {19,   20,   8, TRUE,  {-33.5, 53.0, -90.5}, {1,  -23, -46}, {-0.729, 0.0789, -0.679}, {-0.681,     0, 0.731}, {    0,     1,      0}, {0.681,     0, 0.731}, {     0,     1,      0}, {0.065, -0.065},  { 4.0,   0}, {3,       -1},  {-3,    0}, {  0.5, -1.5} }, //(models_0) gameplay
        {36,   37,  55, TRUE,  {-38.5, 55.5, -86.5}, {1,   35, -51}, {-0.729, 0.0789, -0.679}, {-0.622, 0.277, 0.733}, {0.179, 0.961, -0.211}, {0.622, 0.277, 0.733}, {-0.179, 0.961, -0.211}, {0.072, -0.072},  {-2.13,  0}, {-6.13, 2.13},  {-2.13, 0}, {-6.13, 2.13} }, //(models_1) anim    
        {30,   31,   4, FALSE, {-33.5, 53.0, -90.5}, {1,  -23, -46}, {-0.729, 0.0789, -0.679}, {-0.681,     0, 0.731}, {    0,     1,      0}, {0.681,     0, 0.731}, {     0,     1,      0}, {    0,      0},  { 3.0,  -1}, {3.0,     -1},  { 2,   -1}, {    5,   -1} }, //(models_4) illusion
        {64,   65,  15, TRUE,  {-33.5, 53.0, -90.5}, {1,  -23, -46}, {-0.729, 0.0789, -0.679}, {-0.681,     0, 0.731}, {    0,     1,      0}, {0.681,     0, 0.731}, {     0,     1,      0}, {0.065, -0.065},  { 4.0,   0}, {3,       -1},  {-3,    0}, {  0.5, -1.5} }  //(models_2) unused HQ textures model (not mapped in Krystal's objDef)
    },

    //Sabre
    {
        {49,   50,  51, TRUE,  {-38.33, 65.33, -88},   {0, 32, -63}, {-5.361, 0.72, -8.411},   {-8.433,     0, 5.375}, {0.387, 9.974, 0.607},  {8.282, 2.739, 4.889}, {-2.359, 9.618, -1.392}, {-0.006, -0.006}, {-1,     0}, {-0.82,    0},  {-0.5,  0}, {-0.2,     0} }, //(models_7)  gameplay (Sabre: from unfinished illusion leftover)
        {49,   50,  51, TRUE,  {-38.33, 65.33, -88},   {0, 32, -63}, {-5.361, 0.72, -8.411},   {-8.433,     0, 5.375}, {0.387, 9.974, 0.607},  {8.282, 2.739, 4.889}, {-2.359, 9.618, -1.392}, {-0.006, -0.006}, {-0.83,  0}, {-0.82,    0},  {-0.83, 0}, {-0.82,    0} }, //(models_8)  anim     (Sabre: unused HQ textures model, with MusicalProgrammer's blendshapes)
        {20,   20,  -1, FALSE, {-27.00, 53.00, -76},   {0,  0,   0}, {-5.604, 0.76, -8.247},   {     0,     0,     0}, {    0,     0,     0},  {    0,     0,     0}, {     0,     0,      0}, {     0,      0}, {-0.83,  0}, {-0.82,    0},  {-0.83, 0}, {-0.82,    0} }, //(models_9)  illusion (Fox: no pendant glow)
        {-1,   -1,  -1, TRUE,  {     0,     0,   0},   {0,  0,   0}, {-5.604, 0.76, -8.247},   {     0,     0,     0}, {    0,     0,     0},  {    0,     0,     0}, {     0,     0,      0}, {     0,      0}, {-0.83,  0}, {-0.82,    0},  {-0.83, 0}, {-0.82,    0} }  //(models_11) unused   (Fox: other jacket)
    } 
};

#ifdef EDIT_GLOWS
static s32 eyeEditOpen = FALSE;

static s32 rsEnableEyeParamEdits = FALSE;
static s32 rsEnableModelParamEdits = FALSE;
static s32 rsEnableModelIdxOverride = FALSE;
static s32 rsModelIdxOverride = 0;
static s32 rsPauseEyeDarts = FALSE;
static s32 rsHideLGlow = FALSE;
static s32 rsHideRGlow = FALSE;
static s32 rsOverridePupils = FALSE;
static SVecUV rsPupil;
static VecUV rsPupilLOffset = { 0, 0 };
static VecUV rsPupilROffset = { 0, 0 };
static VecUV rsFactor = { 0, 0 };
static s32 rsEyeShapeID = 0;
static s32 rsPendantShapeID = 0;
static Vec3f rsEyePosition = VEC3F(0, 0, 0);
static Vec3f rsPendantPosition = VEC3F(0, 0, 0);

static Vec3f eyeCoordL = { 0, 0, 0 };
static Vec3f eyeNormalL = { 0, 0, 0 };
static Vec3f eyeCoordR = { 0, 0, 0 };
static Vec3f eyeNormalR = { 0, 0, 0 };
static MtxF headMtx;

RECOMP_PATCH void objExprEyeDart(Object* obj, HeadAnimation* headAnim, f32 arg2) {
    TextureAnimator* pupilL;
    TextureAnimator* pupilR;
    s32 animationFinished;

    if (rsPauseEyeDarts && obj == objGetPlayer()) {
        return;
    }

    pupilL = objExprGetTexAnimator(obj, HEAD_ANIMATION_TAG_Pupil_R, 0);
    pupilR = objExprGetTexAnimator(obj, HEAD_ANIMATION_TAG_Pupil_L, 0);

    if ((pupilL == NULL) || (pupilR == NULL)) {
        return;
    }

    //Check if the current eye dart is already finished
    animationFinished = FALSE;
    if (headAnim->eyeSpeed == 0) {
        animationFinished = TRUE;
    }
    if ((headAnim->eyeSpeed > 0) && (pupilL->positionU >= headAnim->eyeGoal)) {
        animationFinished = TRUE;
    }
    if ((headAnim->eyeSpeed < 0) && (headAnim->eyeGoal >= pupilL->positionU)) {
        animationFinished = TRUE;
    }

    //Set up the next eye dart animation
    if (animationFinished) {
        headAnim->eyeGoal = mathRnd(-100, 10);
        if (headAnim->eyeGoal < pupilL->positionU) {
            headAnim->eyeSpeed = -5;
        } else {
            headAnim->eyeSpeed = 5;
        }
        headAnim->eyeDelayTimer = mathRnd(30, 100);
    }

    //Wait a random amount of time between darts
    if (headAnim->eyeDelayTimer > 0) {
        headAnim->eyeDelayTimer -= gUpdateRate;
        return;
    }

    //Animate eye dart
    pupilL->positionU += headAnim->eyeSpeed * gUpdateRate;
    pupilL->positionV = 0;
    pupilR->positionU = -100 - pupilL->positionU;
    pupilR->positionV = 0;
}

static void eyeEdit_updateModel(void) {
    Object* player = objGetPlayer();
    if (player == NULL) {
        return;
    }

    if (rsEnableModelIdxOverride) {
        if (player) {
            player->modelInstIdx = rsModelIdxOverride;
            objSetModel(player, rsModelIdxOverride);
        }
    }
}

static void eyeEdit_updatePupils(void) {
    TextureAnimator* texAnimL = NULL;
    TextureAnimator* texAnimR = NULL;
    
    if (rsOverridePupils == FALSE) {
        return;
    }

    Object* player = objGetPlayer();
    if (player == NULL) {
        return;
    }

    eyeEdit_updateModel();

    texAnimL = objExprGetTexAnimator(player, HEAD_ANIMATION_TAG_Pupil_L, 0);
    texAnimR = objExprGetTexAnimator(player, HEAD_ANIMATION_TAG_Pupil_R, 0);

    #define STEP_SIZE 16
    if (texAnimL != NULL) {
        texAnimL->positionU = -(s16)rsPupil.u * STEP_SIZE;
        texAnimL->positionV = (s16)rsPupil.v * STEP_SIZE;
    }

    if (texAnimR != NULL && texAnimL != NULL) {
        // NOTE: the left pupil actually uses the right pupil's value plus
        // an offset when the eye idle is active (-100 - texAnimL->positionU)
        if (player->stateFlags & OBJSTATE_IN_SEQ || player->animObj) {
            texAnimR->positionU = (s16)rsPupil.u * STEP_SIZE; 
        } else {
            texAnimR->positionU = -100 - texAnimL->positionU;
        }
        texAnimR->positionV = (s16)rsPupil.v * STEP_SIZE;
    }
}

static void dbgui_eyeParamsReset(void) {
    SpiritGlowData* glow;
    Object* player = objGetPlayer();
    u8 glowDataIndex;

    if (player == NULL) {
        return;
    }

    if (rsEnableModelIdxOverride) {
        glowDataIndex = rsModelIdxOverride;
    } else {
        glowDataIndex = player->modelInstIdx;
    }
    if (player->modelInstIdx == 0 && player->def->pModelList[0] == MODEL_Krystal_HQ_Textures){
        glowDataIndex = 3;
    }
    glow = &glowData[player->id == OBJ_Krystal ? 0 : 1][glowDataIndex];

    if (glow == NULL) {
        return;
    }

    rsFactor.u = glow->multiplier.u;
    rsFactor.v = glow->multiplier.v;
    if (player->animObj || player->stateFlags & OBJSTATE_IN_SEQ) {
        rsPupilLOffset.u = glow->eyeLOffsetSeq.u;
        rsPupilLOffset.v = glow->eyeLOffsetSeq.v;
        rsPupilROffset.u = glow->eyeROffsetSeq.u;
        rsPupilROffset.v = glow->eyeROffsetSeq.v;
    } else {
        rsPupilLOffset.u = glow->eyeLOffset.u;
        rsPupilLOffset.v = glow->eyeLOffset.v;
        rsPupilROffset.u = glow->eyeROffset.u;
        rsPupilROffset.v = glow->eyeROffset.v;
    }
}

static void dbgui_modelParamsReset(void) {
    SpiritGlowData* glow;
    Object* player = objGetPlayer();
    u8 glowDataIndex;

    if (player == NULL) {
        return;
    }

    if (rsEnableModelIdxOverride) {
        glowDataIndex = rsModelIdxOverride;
    } else {
        glowDataIndex = player->modelInstIdx;
    }
    if (player->modelInstIdx == 0 && player->def->pModelList[0] == MODEL_Krystal_HQ_Textures){
        glowDataIndex = 3;
    }
    glow = &glowData[player->id == OBJ_Krystal ? 0 : 1][glowDataIndex];

    if (glow == NULL) {
        return;
    }

    rsEyeShapeID = glow->eyeL;
    rsPendantShapeID = glow->pendantID;
    rsEyePosition.x = glow->eyeCoords.x;
    rsEyePosition.y = glow->eyeCoords.y;
    rsEyePosition.z = glow->eyeCoords.z;
    rsPendantPosition.x = glow->pendantCoords.x;
    rsPendantPosition.y = glow->pendantCoords.y;
    rsPendantPosition.z = glow->pendantCoords.z;
}

static void eyeEdit_debugDraw(void) {
    #define C_RED    0xFF0000FF
    #define C_GREEN  0xFF00FF00
    #define C_BLUE   0xFFFF0000
    #define C_YELLOW 0xFF00FFFF
    Object* player;
    SpiritGlowData* glow; 
    Vec3f start;
    Vec3f end;
    TextureAnimator* texAnim;
    TextureAnimator* texAnimL;
    
    player = objGetPlayer();
    if (player == NULL) {
        return;
    }
    
    glow = &glowData[player->id == OBJ_Krystal ? 0 : 1][rsModelIdxOverride];

    //Draw eyes' normal vectors
    for (u32 i = 0; i < 2; i++) {
        //Get normal vector start and endpoint in headSpace
        start = glow->eyeCoords;
        end = glow->eyeNormal;
        VECTOR_MULTIPLY_BY_SCALAR(end, 100.0f);
        VECTOR_ADD(glow->eyeCoords, end, end);
        if (i > 0){
            start.x *= -1.0f;
            end.x *= -1.0f;
        }

        //Transform startpoint to worldSpace
        mathMtxXFMF(&headMtx,
                        start.x, start.y, start.z, 
                        &start.x, &start.y, &start.z);
        start.x += gWorldX;
        start.z += gWorldZ;

        //Transform endpoint to worldSpace
        mathMtxXFMF(&headMtx,
                        end.x, end.y, end.z, 
                        &end.x, &end.y, &end.z);
        end.x += gWorldX;
        end.z += gWorldZ;

        //Get startpoint-to-endpoint unit vector, and scale it up
        VECTOR_SUBTRACT(end, start, end);
        vec3Normalize(&end);
        VECTOR_MULTIPLY_BY_SCALAR(end, 10.0f);

        //Anchor normal vector at pupil
        if (i == 0) {
            start = eyeCoordL;
        } else {
            start = eyeCoordR;
        }
        VECTOR_ADD(start, end, end);

        draw_3d_line(start.x, start.y, start.z, end.x, end.y, end.z, C_YELLOW);
    }

    //Draw eyes' UV axes
    for (u32 i = 0; i < 2; i++) {
        for (u32 j = 0; j < 2; j++) {
            Vec3f dirU = (i > 0) ? glow->eyeRDirU : glow->eyeLDirU;
            Vec3f dirV = (i > 0) ? glow->eyeRDirV : glow->eyeLDirV;
            Vec3f dir = (j > 0) ? dirV : dirU;
            f32 axisScale = (j > 0) ? glow->multiplier.v : glow->multiplier.u;
            f32 offsetU;
            f32 offsetV;

            offsetU = (i > 0) ? glow->eyeROffset.u : glow->eyeLOffset.u;
            offsetV = (i > 0) ? glow->eyeROffset.v : glow->eyeLOffset.v;
            if (rsEnableEyeParamEdits) {
                offsetU = (i > 0) ? rsPupilROffset.u : rsPupilLOffset.u;
                offsetV = (i > 0) ? rsPupilROffset.v : rsPupilLOffset.v;
            }

            start = glow->eyeCoords;
            end = glow->eyeCoords;
            if (i > 0){
                start.x *= -1.0f;
                end.x *= -1.0f;
            }

            //Shift start/endpoint based on pupil origin offset
            {
                start.x += dirU.x * offsetU + dirV.x * offsetV;
                start.y += dirU.y * offsetU + dirV.y * offsetV;
                start.z += dirU.z * offsetU + dirV.z * offsetV;

                end.x += dirU.x * offsetU + dirV.x * offsetV;
                end.y += dirU.y * offsetU + dirV.y * offsetV;
                end.z += dirU.z * offsetU + dirV.z * offsetV;
            }

            end.x += dir.x * axisScale * 500.0f;
            end.y += dir.y * axisScale * 500.0f;
            end.z += dir.z * axisScale * 500.0f;

            mathMtxXFMF(&headMtx,
                start.x, start.y, start.z, 
                &start.x, &start.y, &start.z);
            start.x += gWorldX;
            start.z += gWorldZ;

            mathMtxXFMF(&headMtx,
                end.x, end.y, end.z, 
                &end.x, &end.y, &end.z);
            end.x += gWorldX;
            end.z += gWorldZ;

            draw_3d_line(start.x, start.y, start.z, end.x, end.y, end.z, (j == 0) ? C_RED : C_GREEN);
        }
    }
}

static void dbgui_pupilUVOverride(void) {
    dbgui_checkbox("Override Pupils", &rsOverridePupils);

    if (dbgui_begin_tab_bar("tabs")) {
        dbgui_push_item_width(140);

        dbgui_input_int("U", &rsPupil.u);
        dbgui_input_int("V", &rsPupil.v);

        dbgui_pop_item_width();

        dbgui_end_tab_bar();
    }

    eyeEdit_updatePupils();
}

static void dbgui_eyeParams(void) {
    dbgui_checkbox("Enable eye param edits", &rsEnableEyeParamEdits);
    dbgui_separator();
    
    dbgui_checkbox("Hide L", &rsHideLGlow);
    dbgui_checkbox("Hide R", &rsHideRGlow);

    dbgui_separator();

    if (dbgui_button("RESET")) {
        dbgui_eyeParamsReset();
    }

    dbgui_input_float("factorU", &rsFactor.u);
    dbgui_input_float("factorV", &rsFactor.v);

    dbgui_text_wrapped("Eye L");
    dbgui_input_float("lOffsetU", &rsPupilLOffset.u);
    dbgui_input_float("lOffsetV", &rsPupilLOffset.v);

    dbgui_text_wrapped("Eye R");
    dbgui_input_float("rOffsetU", &rsPupilROffset.u);
    dbgui_input_float("rOffsetV", &rsPupilROffset.v);
}

static void dbgui_modelParams(void) {
    dbgui_checkbox("Enable model param edits", &rsEnableModelParamEdits);

    if (dbgui_begin_tab_bar("tabs")) {
        dbgui_push_item_width(140);

        if (dbgui_button("RESET")) {
            dbgui_modelParamsReset();
        }

        dbgui_text_wrapped("ShapeIDs");
        dbgui_input_int("Eye", &rsEyeShapeID);
        dbgui_input_int("Pendant", &rsPendantShapeID);

        dbgui_separator();

        dbgui_text_wrapped("Position Offsets");
        dbgui_input_float("Eye x", &rsEyePosition.x);
        dbgui_input_float("Eye y", &rsEyePosition.y);
        dbgui_input_float("Eye z", &rsEyePosition.z);
        dbgui_text_wrapped("Pendant");
        dbgui_input_float("Pendant x", &rsPendantPosition.x);
        dbgui_input_float("Pendant y", &rsPendantPosition.y);
        dbgui_input_float("Pendant z", &rsPendantPosition.z);

        dbgui_pop_item_width();

        dbgui_end_tab_bar();
    }

    //Ensure the shapeID values stay in-bounds
    {
        Object* player = objGetPlayer();
        if (player) {
            s32 modelIdx = player->modelInstIdx;
            if (rsEnableModelIdxOverride) {
                modelIdx = rsModelIdxOverride;
                if (player->def == NULL) {
                    return;
                }
                if (modelIdx >= player->def->numModels) {
                    modelIdx = player->def->numModels - 1;
                }
            }
            ModelInstance* modelInst = player->modelInsts[modelIdx];
            if (modelInst == NULL) {
                return;
            }
            Model* model = modelInst->model;
            if (model == NULL) {
                return;
            }

            rsEyeShapeID = MAX(0, MIN(rsEyeShapeID, model->unk70));
            rsPendantShapeID = MAX(0, MIN(rsPendantShapeID, model->unk70));
        } 
    }
}

void dbgui_spiritGlowEdit(s32* open) {
    static _Bool editsInitialised = FALSE;
    static s32 holdHeadStill = FALSE;
    static s32 pauseAnimation = FALSE;
    static f32 animProgressPaused = 0.0f;
    static Vec3s16 headPose = { 0, 0, 0 };

    Object* player = objGetPlayer();

    if (dbgui_begin("Krazoa Spirit Glows", open)) {
        dbgui_text_wrapped("NOTE: the `Spirit Glows: Enhanced` config must be on to apply edits.");

        dbgui_checkbox("Override modelIdx", &rsEnableModelIdxOverride);
        if (dbgui_input_int("modelIdx", &rsModelIdxOverride)) {
            if (rsModelIdxOverride < 0) {
                rsModelIdxOverride = 0;
            } else if (player && player->def && rsModelIdxOverride >= player->def->numModels) {
                rsModelIdxOverride = player->def->numModels - 1;
            }
        }
        dbgui_separator();

        if (dbgui_checkbox("Pause animation", &pauseAnimation)) {
            if (player) {
                animProgressPaused = player->animProgress;
            }
        }
        dbgui_checkbox("Pause eye darts", &rsPauseEyeDarts);
        if (dbgui_checkbox("Hold head still", &holdHeadStill)) {
            SeqJoint* head;
            if (player) {
                head = objExpr_func_80034804(player, 0);
                headPose.s[0] = head->yaw;
                headPose.s[1] = head->pitch;
                headPose.s[2] = head->roll;
            }
        }

        if (dbgui_begin_tab_bar("tabs")) {
            if (dbgui_begin_tab_item("Pupil UVs", NULL)) {
                dbgui_pupilUVOverride();
                dbgui_end_tab_item();
            }
            if (dbgui_begin_tab_item("TexAnim Params", NULL)) {
                dbgui_eyeParams();
                dbgui_end_tab_item();
            }
            if (dbgui_begin_tab_item("Model Params", NULL)) {
                dbgui_modelParams();
                dbgui_end_tab_item();
            }
            dbgui_end_tab_bar();
        }
    }
    dbgui_end();

    if (holdHeadStill) {
        SeqJoint* head;
        if (player) {
            head = objExpr_func_80034804(player, 0);
            head->yaw   = headPose.s[0];
            head->pitch = headPose.s[1];
            head->roll  = headPose.s[2];
        }
    }

    if (pauseAnimation) {
        if (player) {
            objAnimSetProgress(player, animProgressPaused);
        }
    }

    eyeEdit_updateModel();

    if (editsInitialised == FALSE) {
        dbgui_eyeParamsReset();
        dbgui_modelParamsReset();
        editsInitialised = TRUE;
    }
}

RECOMP_CALLBACK("*", recomp_on_dbgui) void dinomodEditMenu() {
    if (!dbgui_is_open()) { 
        return;
    }

    if (dbgui_begin_main_menu_bar()) {
        if (dbgui_begin_menu("Edit")) {
            dbgui_menu_item("Krazoa Spirit Glows", &eyeEditOpen);
            dbgui_end_menu();
        }
        
        dbgui_end_main_menu_bar();
    }

    if (eyeEditOpen) {
        dbgui_spiritGlowEdit(&eyeEditOpen);
    }

    if (configs_GetSpiritGlows()) {
        eyeEdit_debugDraw();
    }
}

RECOMP_HOOK_DLL(dll_210_print) void eyeEdit_animOverridePupils(Object* player, Gfx** gdl, Mtx** mtxs, Vertex** vtxs, Triangle** pols, s8 visibility) {
    eyeEdit_updatePupils();
}
#endif

typedef struct {
    u8 flags;
    s8 depthCheckInterval;
    s32 prevZDepthObj;
} OcclusionData;

typedef enum {
    Occlusion_FLAG_Visible = 1,
    Occlusion_FLAG_New_Check_Needed = 2
} OcclusionFlags;

static OcclusionData rsOcclusionData;

/* Checks if the centre of the player's head is occluded, in a similar way to Rare's code for the Lanterns etc.
 * TODO: maybe revisit and try to refine this so there's a separate occlusion check for each eye? */
static void custom_modelfx_checkHeadOcclusion(Object* self, Vec3f* coords) {
#ifdef DINOMOD_ROM_PATCH
    #define DEPTH_CHECK_INVERVAL 60
#else
    #define DEPTH_CHECK_INVERVAL 0
#endif

    OcclusionData* occlusion;
    Vec3f point;
    Vec3f projected;
    Vec3f uView;
    s32 screenX;
    s32 screenY;
    s32 zDepth;
    s32 zDepthOcclude;
    SRT fxTransform;

    occlusion = &rsOcclusionData;

#ifdef DEBUG_OCCLUSION
    if (self->modelInstIdx == 0) {
        diPrintf("prevZDepthObj: %d\n", occlusion->prevZDepthObj);
        diPrintf("depthCheckInterval: %d\n", occlusion->depthCheckInterval);
    }
#endif

    //Return early if a check isn't needed yet
    if ((occlusion->flags & Occlusion_FLAG_New_Check_Needed) == FALSE) {
        return;
    }

    //Stagger depth checks (unless prev obj z-depth is still unknown)
    if (occlusion->depthCheckInterval > 0) {
        occlusion->depthCheckInterval -= gUpdateRate;
        if (occlusion->prevZDepthObj) {
            return;
        }
    } else {
        occlusion->depthCheckInterval = DEPTH_CHECK_INVERVAL;
        occlusion->prevZDepthObj = 0;
    }

    point.x = coords->x - gWorldX;
    point.y = coords->y;
    point.z = coords->z - gWorldZ;
    camProjectPoint(point.x, point.y, point.z, &projected.x, &projected.y, &projected.z);
    camClipToScreen(projected.x, projected.y, projected.z, &screenX, &screenY, NULL);
    zDepthOcclude = viObjDepth(screenX, screenY, self);

    //Use the last nonzero obj depth value
    if (zDepthOcclude == 0) {
        zDepthOcclude = occlusion->prevZDepthObj;
    } else {
        occlusion->prevZDepthObj = zDepthOcclude;
    }

    if (zDepthOcclude == 0) {
        return;
    }

    camGetVec3ToCameraNormalized(self->srt.transl.x, self->srt.transl.y, self->srt.transl.z, &uView.x, &uView.y, &uView.z);
    camProjectPoint(point.x += (uView.x * 20.0f), point.y += (uView.y * 20.0f), point.z += (uView.z * 20.0f), &projected.x, &projected.y, &projected.z);
    camClipToScreen(projected.x, projected.y, projected.z, NULL, NULL, &zDepth);

    if (viContainsPoint(screenX, screenY) && (zDepth > 0) && (zDepth < zDepthOcclude)) {
        occlusion->flags |= Occlusion_FLAG_Visible;
    } else {
        occlusion->flags &= ~Occlusion_FLAG_Visible;
    }

#ifdef DEBUG_OCCLUSION
    vec3_diPrintf(&point);
    vec3_diPrintf(&projected);
    diPrintf("viContainsPoint: %d\n", viContainsPoint(screenX, screenY));
    diPrintf("screenX: %d\t screenY: %d\n", screenX, screenY);
    diPrintf("zDepth: %d, zDepthOcclude: %d, visible: %s\n", zDepth, zDepthOcclude, (occlusion->flags & Occlusion_FLAG_Visible) ? "YES" : "NO");
#endif

    occlusion->flags &= ~Occlusion_FLAG_New_Check_Needed;
}

static void modelfx_spiritEyesCustom(Gfx** gdl, Mtx** mtxs, Object* character) {
    Camera* camera;
    Object* player;
    SpiritGlowData* glow;
    ModelInstance* modelInstance;
    ModelFacebatch* facebatch;
    TextureAnimator* texAnim;
    TextureAnimator* texAnimL;
    MtxF* jointMatrix;
    SRT fxTransform;
    f32 dotProduct;
    f32 speedSquared;
    Vec3f eyeTranslateLocal;
    Vec3f eyeDirection;
    Vec3f cameraToEye;
    u8 glowDataIndex;
    s8 facebatchID;
    u8 i;
    u8 playerIsInSequence = FALSE;

#ifdef EDIT_GLOWS
    eyeEdit_updatePupils();
#endif

    player = objGetPlayer();
    camera = camGet();
    if (player == NULL || camera == NULL){
        return;
    }

    //Check if the player's in a sequence
    if (player->stateFlags & OBJSTATE_IN_SEQ || player->animObj) {
        playerIsInSequence = TRUE;
    }

    //Do nothing if the model index is unrecognised
    if ((0 <= character->modelInstIdx && character->modelInstIdx < 3) == FALSE) {
        return;
    }

    //Get glow data (also checking if Krystal's HQ textures model is in use)
#ifdef EDIT_GLOWS
    glowDataIndex = rsEnableModelIdxOverride ? rsModelIdxOverride : character->modelInstIdx;
#else
    glowDataIndex = character->modelInstIdx;
#endif
    if (character->modelInstIdx == 0 && character->def->pModelList[0] == MODEL_Krystal_HQ_Textures){
        glowDataIndex = 3;
    }
    glow = &glowData[character->id == OBJ_Krystal ? 0 : 1][glowDataIndex];

#ifdef EDIT_GLOWS
    modelInstance = character->modelInsts[character->modelInstIdx];
    if (rsEnableModelParamEdits) {
        facebatch = &modelInstance->model->faces[rsEyeShapeID];
    } else {
        facebatch = &modelInstance->model->faces[glow->eyeL];
    }
    jointMatrix = (MtxF *) ((f32**)(modelInstance->matrices)[(modelInstance->unk34 & 1)] + (facebatch->jointID_A << 4));
    bcopy(jointMatrix, &headMtx, sizeof(MtxF));
#endif

    //Don't draw glows if this is the cutscene model (since eye blendshapes affect how the pupils move) (TODO: revisit!)
    if (character->modelInstIdx == 1) {
        return;
    }

    modelInstance = character->modelInsts[character->modelInstIdx];

    fxTransform.yaw = 0;
    fxTransform.roll = 0;
    fxTransform.pitch = 0;
    fxTransform.scale = 1.0f;

    texAnimL = objExprGetTexAnimator(character, HEAD_ANIMATION_TAG_Pupil_L, 0);

    //Eyes
    for (i = 0; i < 2; i++) {
        //Check the eyelid texture's frame to tell if character's blinking
        texAnim = objExprGetTexAnimator(character, (i > 0) ? HEAD_ANIMATION_TAG_Eyelid_R : HEAD_ANIMATION_TAG_Eyelid_L, 0);
        if (glow->blink && (texAnim->frame >= 0x200)){
            continue;
        }

        //Get the facebatchID
        //NOTE: using the same shapeID for both eyes, since we just need to get the head jointID with this custom approach, instead of 
        //averaging vertex positions, because the base eye coords (relative to the head joint) are saved as data instead of calculated each time
        facebatchID = glow->eyeL; 
#ifdef EDIT_GLOWS
        if (rsEnableModelParamEdits) {
            facebatchID = rsEyeShapeID;
        }
#endif

        if (facebatchID < 0){
            continue;
        }

        //Don't draw either eye glow if the player's head is occluded
        if (i == 0) {
            fxTransform.transl = glow->eyeCoords;
            fxTransform.transl.x = 0.0f;

            facebatch = &modelInstance->model->faces[glow->eyeL];
            jointMatrix = (MtxF *) ((f32**)(modelInstance->matrices)[(modelInstance->unk34 & 1)] + (facebatch->jointID_A << 4));
            mathMtxXFMF(jointMatrix,
                            fxTransform.transl.x, fxTransform.transl.y, fxTransform.transl.z, 
                            &fxTransform.transl.x, &fxTransform.transl.y, &fxTransform.transl.z);
            fxTransform.transl.x += gWorldX;
            fxTransform.transl.z += gWorldZ;

            //Occlusion checks break down when very close to the camera, so do a distance check
            if (vec3DistanceSquared(&camera->srt.transl, &fxTransform.transl) > SQ(19)) {
                rsOcclusionData.flags |= Occlusion_FLAG_New_Check_Needed;
                custom_modelfx_checkHeadOcclusion(character, &fxTransform.transl);
                if ((rsOcclusionData.flags & Occlusion_FLAG_Visible) == FALSE) {
                    break;
                }
            }
        }

        //Get base position
        fxTransform.transl = glow->eyeCoords;
#ifdef EDIT_GLOWS
        if (rsEnableModelParamEdits) {
            fxTransform.transl = rsEyePosition;
        }
#endif
        if (i > 0){
            fxTransform.transl.x *= -1.0f;
        }

        fxTransform.scale = 1.0f;

        //Slide the eye position over surface as the iris animates
        Vec3f dirU = (i > 0) ? glow->eyeRDirU : glow->eyeLDirU;
        Vec3f dirV = (i > 0) ? glow->eyeRDirV : glow->eyeLDirV;

        f32 offsetU;
        f32 offsetV;
        f32 factorU;
        f32 factorV;
        f32 positionU;
        f32 positionV;

        texAnim = objExprGetTexAnimator(character, (i > 0) ? HEAD_ANIMATION_TAG_Pupil_R : HEAD_ANIMATION_TAG_Pupil_L, 0);
#ifdef DEBUG_EYE_GLOW
        if (playerIsInSequence) {
            diPrintf("Player is in sequence\n");
        }
        diPrintf("Eyelid frame: %d\n", texAnim->frame);
        diPrintf("Eye U: %d\n", texAnim->positionU);
#endif
        
        //Separate texAnim offsets when in a sequence (TODO: possibly not needed - figure out why the coords work out differently!)
        if (playerIsInSequence) {
            offsetU = (i > 0) ? glow->eyeROffsetSeq.u : glow->eyeLOffsetSeq.u;
            offsetV = (i > 0) ? glow->eyeROffsetSeq.v : glow->eyeLOffsetSeq.v;
        } else {
            offsetU = (i > 0) ? glow->eyeROffset.u : glow->eyeLOffset.u;
            offsetV = (i > 0) ? glow->eyeROffset.v : glow->eyeLOffset.v;
        }

        //TO-DO: on the anim model, blend in a secondary U/V multiplier based on the blendShape index currently active (and its strength)?
        //Probably need to lerp-adjust the eye centrepoint for each blendshape too... complicated! No wonder Rare switched off the effect on the anim models.
        factorU = glow->multiplier.u;
        factorV = glow->multiplier.v;

#ifdef EDIT_GLOWS
        if (rsEnableEyeParamEdits) {
            offsetU = (i > 0) ? rsPupilROffset.u : rsPupilLOffset.u;
            offsetV = (i > 0) ? rsPupilROffset.v : rsPupilLOffset.v;

            factorU = rsFactor.u;
            factorV = rsFactor.v;
        }
#endif

        //NOTE: right eye uses the left eye's positionU (plus an offset) outside of sequences, because of the eye idle behaviour
        if ((i > 0) && (playerIsInSequence == FALSE)) {
            positionU = factorU*((-100.0f - texAnimL->positionU)) + offsetU;
        } else {
            positionU = factorU*((f32)texAnim->positionU) + offsetU;
        }
        positionV = factorV*((f32)texAnim->positionV) + offsetV;

        fxTransform.transl.x += dirU.x*positionU + dirV.x*positionV;
        fxTransform.transl.y += dirU.y*positionU + dirV.y*positionV;
        fxTransform.transl.z += dirU.z*positionU + dirV.z*positionV;

        eyeTranslateLocal = fxTransform.transl;

        //Get head joint's matrix, and use it to find the worldSpace coords of the eye
        facebatch = &modelInstance->model->faces[facebatchID];
        jointMatrix = (MtxF *) ((f32**)(modelInstance->matrices)[(modelInstance->unk34 & 1)] + (facebatch->jointID_A << 4));
        mathMtxXFMF(jointMatrix,
                        fxTransform.transl.x, fxTransform.transl.y, fxTransform.transl.z, 
                        &fxTransform.transl.x, &fxTransform.transl.y, &fxTransform.transl.z);
        fxTransform.transl.x += gWorldX;
        fxTransform.transl.z += gWorldZ;

#ifdef EDIT_GLOWS
        if (i == 0) {
            eyeCoordL = fxTransform.transl;
        } else {
            eyeCoordR = fxTransform.transl;
        }

        if ((rsHideLGlow && i == 0) ||
            (rsHideRGlow && i == 1)
        ) {
            continue;
        }
#endif

        //Get eye's surface normal (normalised, and transformed wrt. head joint)
        eyeDirection = glow->eyeNormal;
        if (i > 0){
            eyeDirection.x *= -1.0f;
        }
        VECTOR_ADD(eyeDirection, eyeTranslateLocal, eyeDirection);
        mathMtxXFMF(jointMatrix,
                        eyeDirection.x, eyeDirection.y, eyeDirection.z, 
                        &eyeDirection.x, &eyeDirection.y, &eyeDirection.z);
        eyeDirection.x += gWorldX;
        eyeDirection.z += gWorldZ;

        VECTOR_SUBTRACT(eyeDirection, fxTransform.transl, eyeDirection)
        vec3Normalize(&eyeDirection);

        //Get camera-to-eye vector (normalised)
        VECTOR_SUBTRACT(fxTransform.transl, camera->srt.transl, cameraToEye)
        vec3Normalize(&cameraToEye);

        //Get dot product of cameraToEye and eyeDirection
        f32 dotProduct = DOT_PRODUCT(cameraToEye, eyeDirection);

        //Don't draw the eye particle when the eye is facing away from the camera
        if (dotProduct > 0.0f){ //TO-DO: model-specific thresholds? Could use a cone range to hide behind Fox's jutting nose, for example!
            continue;
        }

        //Different eye particles when (mostly) stopped vs. in motion
        speedSquared = VECTOR_MAGNITUDE_SQUARED(character->velocity);
        if (speedSquared < 0.1f) {
            gDLL_17_partfx->vtbl->spawn(character, 0x19A, &fxTransform, PARTFXFLAG_200000, -1, NULL);
        } else {
            gDLL_17_partfx->vtbl->spawn(character, 0x19B, &fxTransform, PARTFXFLAG_200000, -1, NULL);
        }
        gDLL_17_partfx->vtbl->spawn(character, 0x199, &fxTransform, PARTFXFLAG_200000, -1, NULL);
    }

    //Pendant
    facebatchID = glow->pendantID; 
#ifdef EDIT_GLOWS
    if (rsEnableModelParamEdits) {
        facebatchID = rsPendantShapeID;
    } 
#endif
    if (facebatchID >= 0){
        _data_1864 = 4;

        //Get camera-to-character vector
        Vec3f cameraToCharacter;
        VECTOR_SUBTRACT(player->srt.transl, camera->srt.transl, cameraToCharacter)

        //Get character direction vector (pointing along objectSpace -Z)
        Vec3f characterDirection = {0.0f, 0.0f, -1.0f};
        fxTransform.transl.x = 0.0f;
        fxTransform.transl.y = 0.0f;
        fxTransform.transl.z = 0.0f;
        fxTransform.yaw = player->srt.yaw;
        mathRotateRPY(&fxTransform, characterDirection.f);

        //Get dot product of cameraToCharacter and characterDirection
        dotProduct = DOT_PRODUCT(cameraToCharacter, characterDirection);

        //Don't draw pendant glow when the player is mostly facing away
        if (dotProduct > 0.5f){
            return;
        }

        //Get base position
        fxTransform.transl = glow->pendantCoords;
#ifdef EDIT_GLOWS
        if (rsEnableModelParamEdits) {
            fxTransform.transl = rsPendantPosition;
        }
#endif
        fxTransform.scale = 1.0f;

        //Add jitter
        fxTransform.transl.x += mathRnd(-5, 5);
        fxTransform.transl.y += mathRnd(-5, 5);
        fxTransform.transl.z += mathRnd(-5, 5);

        //Get the facebatch
        facebatch = &modelInstance->model->faces[facebatchID];

        //Transform by upper body joint
        jointMatrix = (MtxF *) &(((f32 **)modelInstance->matrices)[(modelInstance->unk34 & 1)][facebatch->jointID_A << 4]);
        mathMtxXFMF(jointMatrix, 
                        fxTransform.transl.x, fxTransform.transl.y, fxTransform.transl.z, 
                        &fxTransform.transl.x, &fxTransform.transl.y, &fxTransform.transl.z);
        fxTransform.transl.x += gWorldX;
        fxTransform.transl.z += gWorldZ;
        
        //Add character speed (stops pendant glow particles' later frames from clipping through character's back when running forwards into screen)
        //@recomp: blend this in as camera moves behind character
        if (dotProduct > 0.0f){
            dotProduct *= 2.0f;
            fxTransform.transl.x += dotProduct * character->velocity.x;
            fxTransform.transl.y += dotProduct * character->velocity.y;
            fxTransform.transl.z += dotProduct * character->velocity.z;
        }

        //More particles when mostly stopped
        speedSquared = SQ(character->velocity.x) + SQ(character->velocity.y) + SQ(character->velocity.z);
        if (speedSquared < 0.01f) {
            gDLL_17_partfx->vtbl->spawn(character, 0x19D, &fxTransform, PARTFXFLAG_200000, -1, NULL);
        }
        gDLL_17_partfx->vtbl->spawn(character, 0x19C, &fxTransform, PARTFXFLAG_200000, -1, NULL);
    }
}

// @recomp: Fix Sabre's glowing spirit eyes/pendant effect (original patch by MusicalProgrammer)
// TODO: replace with cleaner decomp, eventually
RECOMP_PATCH void dll_32_func_1314(Gfx** gdl, Mtx** mtxs, Object* character) {
    s32 vtxCount;
    s32 direction;
    SRT fxTransform;
    s32 shapeIdx;
    s32 vtxIdx;
    f32 speedSquared;
    f32 normalise;
    ModelFacebatch* facebatch;
    ModelInstance* modelInstance;
    f32 spB4;
    Vec3f vCharCam = VEC3F(-1.0f, 0.0f, 0.0f); // _data_184C
    Vec3f vCharDirection = VEC3F(0.0f, 0.0f, 1.0f); // _data_1858
    Camera* camera;
    Vtx* shapeVertices;
    s32 eyesEndShapeIdx;
    s32 pendantShapeIdx;
    Object* player;
    s16 baseVertexID;
    MtxF* jointMatrix;

    // @recomp: optionally use a revamped version of the effect
    if (configs_GetSpiritGlows()) {
        modelfx_spiritEyesCustom(gdl, mtxs, character);
        return;
    }

    // @recomp: Pull face ID from Sabre/Krystal ObjDef
    //sp90 = 0x15;\
    //sp8C = 8;
    eyesEndShapeIdx = *(s8*)((u32)character->def + 0x68);
    pendantShapeIdx = *(s8*)((u32)character->def + 0x69);
    player = objGetPlayer();

    if (character->modelInstIdx != 0) {
        return;
    }

    modelInstance = character->modelInsts[character->modelInstIdx];

    fxTransform.yaw = 0;
    fxTransform.roll = 0;
    fxTransform.pitch = 0;
    fxTransform.scale = 1.0f;

    camera = camGet();
    if (camera != NULL) {
        VECTOR_SUBTRACT(camera->srt.transl, character->srt.transl, vCharCam);
        normalise = VECTOR_MAGNITUDE_SQUARED(vCharCam);
        if (normalise != 0) {
            normalise = sqrtf(normalise);
        }
        vCharCam.x /= normalise;
        vCharCam.y /= normalise;
        vCharCam.z /= normalise;

        fxTransform.yaw = character->srt.yaw;
        fxTransform.transl.x = 0.0f;
        fxTransform.transl.y = 0.0f;
        fxTransform.transl.z = 0.0f;
        mathRotateRPY(&fxTransform, vCharDirection.f);
        fxTransform.yaw = 0;
    }

    direction = dll_32_func_1D3C(&vCharCam, &vCharDirection, &spB4);
    if (direction != -1) {
        return;
    }

    if (((DLL_210_Player*)player->dll)->vtbl->func44(player) == 0) {
        if (direction == -1) {
            // @recomp: Pull face ID from Sabre/Krystal ObjDef
            shapeIdx = *(s8*)((u32)character->def + 0x67);
            for (; shapeIdx < eyesEndShapeIdx; shapeIdx++) {
                if ((character->id != OBJ_Sabre) || (shapeIdx != 42)) {
                    facebatch = &modelInstance->model->faces[shapeIdx];
                    baseVertexID = facebatch->baseVertexID;
                    shapeVertices = &modelInstance->vertices[0][baseVertexID];
                    vtxCount = modelInstance->model->faces[shapeIdx + 1].baseVertexID - baseVertexID;
                    fxTransform.transl.x = 0.0f;
                    fxTransform.transl.y = -26.0f;
                    fxTransform.transl.z = 0.0f;

                    for (vtxIdx = 0; vtxIdx < vtxCount; vtxIdx++) {
                        fxTransform.transl.x += shapeVertices[vtxIdx].n.ob[0];
                        fxTransform.transl.y += shapeVertices[vtxIdx].n.ob[1];
                        fxTransform.transl.z += shapeVertices[vtxIdx].n.ob[2];
                    }

                    fxTransform.scale = 1.0f;
                    fxTransform.transl.x /= vtxCount;
                    fxTransform.transl.y /= vtxCount;
                    fxTransform.transl.z /= vtxCount;

                    jointMatrix = (MtxF *) ((f32**)(modelInstance->matrices)[(modelInstance->unk34 & 1)] + (facebatch->jointID_A << 4));
                    mathMtxXFMF(jointMatrix,
                                   fxTransform.transl.x, fxTransform.transl.y, fxTransform.transl.z, 
                                   &fxTransform.transl.x, &fxTransform.transl.y, &fxTransform.transl.z);
                    fxTransform.transl.x += gWorldX;
                    fxTransform.transl.z += gWorldZ;

                    speedSquared = SQ(character->velocity.x) + SQ(character->velocity.y) + SQ(character->velocity.z);
                    if (speedSquared < 0.1f) {
                        gDLL_17_partfx->vtbl->spawn(character, 0x19A, &fxTransform, 0x200000, -1, NULL);
                    } else {
                        gDLL_17_partfx->vtbl->spawn(character, 0x19B, &fxTransform, 0x200000, -1, NULL);
                    }
                    gDLL_17_partfx->vtbl->spawn(character, 0x199, &fxTransform, 0x200000, -1, NULL);
                }
            }
        }
    }
    
    for (shapeIdx = pendantShapeIdx; shapeIdx < (pendantShapeIdx + 1); shapeIdx++) {
        facebatch = &modelInstance->model->faces[shapeIdx];
        baseVertexID = facebatch->baseVertexID;
        shapeVertices = &modelInstance->vertices[0][baseVertexID];
        _data_1864 = 4;
        fxTransform.transl.x = 0.0f;
        fxTransform.transl.y = 10.0f;
        fxTransform.transl.z = -40.0f;
        // @recomp: Adjust pendant fx position for Sabre
        if (character->id == OBJ_Sabre) {
            fxTransform.transl.x = shapeVertices[4].n.ob[0] + 25.0f;
            fxTransform.transl.y = shapeVertices[4].n.ob[1] + 0.0f;
            fxTransform.transl.z = shapeVertices[4].n.ob[2] + 0.0f;
        } else {
            fxTransform.transl.x = shapeVertices[4].n.ob[0];
            fxTransform.transl.y = shapeVertices[4].n.ob[1] + 2.0f;
            fxTransform.transl.z = shapeVertices[4].n.ob[2] + 10.0f;
        }
        fxTransform.transl.x += mathRnd(-5, 5);
        fxTransform.transl.y += mathRnd(-5, 5);
        fxTransform.transl.z += mathRnd(-5, 5);
        fxTransform.scale = 1.0f;

        //Transform by upper body joint
        jointMatrix = (MtxF *) &(((f32 **)modelInstance->matrices)[(modelInstance->unk34 & 1)][facebatch->jointID_A << 4]);
        mathMtxXFMF(jointMatrix, 
                       fxTransform.transl.x, fxTransform.transl.y, fxTransform.transl.z, 
                       &fxTransform.transl.x, &fxTransform.transl.y, &fxTransform.transl.z);
        fxTransform.transl.x += gWorldX;
        fxTransform.transl.z += gWorldZ;

        // @recomp: Don't factor in object velocity, the partfx is already relative to the player
        // fxTransform.transl.x += (2.0f * character->velocity.x);
        // fxTransform.transl.y += (2.0f * character->velocity.y);
        // fxTransform.transl.z += (2.0f * character->velocity.z);

        //More particles when mostly stopped
        speedSquared = SQ(character->velocity.x) + SQ(character->velocity.y) + SQ(character->velocity.z);
        if (speedSquared < 0.01f) {
            gDLL_17_partfx->vtbl->spawn(character, 0x19D, &fxTransform, 0x200000, -1, NULL);
        }
        gDLL_17_partfx->vtbl->spawn(character, 0x19C, &fxTransform, 0x200000, -1, NULL);
    }
}

#include "modding.h"
#include "recompconfig.h"
#include "recomputils.h"

#include "common.h"

extern s16 *gFile_OBJINDEX;
extern int gObjIndexCount;

RECOMP_HOOK_RETURN("init_objects") void init_objects_return_hook(void) {
    // @recomp: Change all -1 OBJINDEX.bin mappings to DummyObject. Otherwise, attempting to load
    //          one of those object IDs will result in a crash since the object setup code will
    //          return null and pretty much no part of the game is coded to expect a null there.
    for (s32 i = 0; i < gObjIndexCount; i++) {
        if (gFile_OBJINDEX[i] == -1) {
            gFile_OBJINDEX[i] = 0;
        }
    }
}

extern int gNumObjectsTabEntries;
extern ObjDef **gLoadedObjDefs;
extern u8 *gObjDefRefCount;
extern s32  *gFile_OBJECTS_TAB;
ModLine *obj_load_objdef_modlines(s32 modLineNo, s16 *modLineCount);
extern void func_800596BC(ObjDef*);

static int useExtraDescriptionsObjects = -1;
static s16* customObjDefTextIDs = NULL;

/** For toggling LaminGaming's extra object description text */
static void remove_extra_descriptions_object(ObjDef* def){
    u16 bankID = (def->unkA2 & 0xF00) >> 8;
    u16 lineID = def->unkA2 & 0xFF;

    //Check if description text in bank 0 (gametext3) and beyond original last line
    if (bankID == 0 && lineID > 107){
        def->unkA2 = -1;
        
    //Check if description text in bank 1 (gametext568) and beyond original last line
    } else if (bankID == 1 && lineID > 3){
        def->unkA2 = -1;
    }
}

static void remove_extra_descriptions_objects(){
    s32 index;
    ObjDef* def;

    for (index = 0; index < gNumObjectsTabEntries; index++){
        def = gLoadedObjDefs[index];
        if (def == NULL){
            continue;
        }

        remove_extra_descriptions_object(def);
    }
}

static void add_extra_descriptions_objects(){
    s32 index;
    s16 customIndex;
    ObjDef* def;

    for (index = 0; index < gNumObjectsTabEntries; index++){
        def = gLoadedObjDefs[index];
        if (def == NULL){
            continue;
        }

        def->unkA2 = customObjDefTextIDs[index];
    }
}

/** Allocate memory for custom text values (so they can be toggled easily) */
RECOMP_HOOK_RETURN("init_objects") void init_custom_text_ids(void) {
    customObjDefTextIDs = recomp_alloc(gNumObjectsTabEntries*2);
}

RECOMP_PATCH ObjDef *obj_load_objdef(s32 tabIdx) {
    ObjDef *def;
    s32 fileOffset;
    s32 fileSize;
    //@recomp
    int useExtraText = recomp_get_config_u32("lamingaming_extra_description_text");

    if (tabIdx >= gNumObjectsTabEntries) {
        return NULL;
    }
    
    if (gObjDefRefCount[tabIdx] != 0) {
        gObjDefRefCount[tabIdx]++;
        def = gLoadedObjDefs[tabIdx];
        return def;
    }
    
    fileOffset = gFile_OBJECTS_TAB[tabIdx];
    fileSize = gFile_OBJECTS_TAB[tabIdx + 1] - fileOffset;

    def = (ObjDef*)mmAlloc(fileSize, ALLOC_TAG_OBJECTS_COL, NULL);
    if (def != NULL) {
        read_file_region(OBJECTS_BIN, (void*)def, fileOffset, fileSize);

        if (def->pEvent != 0) {
            def->pEvent = (ObjDefEvent*)((u32)def + (u32)def->pEvent);
        }

        if (def->pHits != 0) {
            def->pHits = (ObjDefHit*)((u32)def + (u32)def->pHits);
        }

        if (def->pWeaponData != 0) {
            def->pWeaponData = (ObjDefWeaponData*)((u32)def + (u32)def->pWeaponData);
        }

        def->pModelList = (u32*)((u32)def + (u32)def->pModelList);
        def->pTextures = (UNK_PTR*)((u32)def + (u32)def->pTextures);
        def->pSequenceBones = (UNK_PTR*)((u32)def + (u32)def->pSequenceBones);

        if (def->unk18 != 0) {
            def->unk18 = (u32*)((u32)def + (u32)def->unk18);
        }

        if (def->unk40 != 0) {
            def->unk40 = (ObjDefStruct40*)((u32)def + (u32)def->unk40);
        }

        if (def->pSeq != 0) {
            def->pSeq = (s16*)((u32)def + (u32)def->pSeq);
        }

        def->pModLines = NULL;

        def->pAttachPoints = (AttachPoint*)((u32)def + (u32)def->pAttachPoints);

        def->pIntersectPoints = NULL;

        if (def->modLineNo > -1) {
            def->pModLines = obj_load_objdef_modlines(def->modLineNo, &def->modLineCount);
            func_800596BC(def);
        }

        gLoadedObjDefs[tabIdx] = def;
        gObjDefRefCount[tabIdx] = 1;

        //@recomp: store copy of gametext index so custom object description strings can be toggled
        customObjDefTextIDs[tabIdx] = def->unkA2;

        //@recomp: remove extra text if toggled off
        if (!useExtraText){
            remove_extra_descriptions_object(def);
        }

    } else {
        return NULL;
    }

    return def;
}

/** Check if extra text config has changed */
RECOMP_CALLBACK("*", recomp_on_game_tick_start) void updateExtraTextObjects() {
    int setting = recomp_get_config_u32("lamingaming_extra_description_text");
    if (useExtraDescriptionsObjects == setting){
        return;
    }

    useExtraDescriptionsObjects = setting;

    if (useExtraDescriptionsObjects == TRUE){
        add_extra_descriptions_objects();
    } else {
        remove_extra_descriptions_objects();
    }
}

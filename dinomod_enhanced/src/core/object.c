#include "modding.h"
#include "recompconfig.h"
#include "recomputils.h"

#include "common.h"
#include "game/objects/object_def.h"
#include "sys/newshadows.h"

extern s16 *gFile_OBJINDEX;
extern int gObjIndexCount;
extern int gNumObjectsTabEntries;
extern ObjDef **gLoadedObjDefs;
extern u8 *gObjDefRefCount;
extern s32  *gFile_OBJECTS_TAB;

extern ModLine *obj_load_objdef_modlines(s32 modLineNo, s16 *modLineCount);
extern void func_800596BC(ObjDef*);
extern u32 obj_get_model_flags(Object *obj);
extern u32 obj_calc_mem_size(Object *obj, ObjDef *def, u32 flags);
extern void obj_free_objdef(s32 tabIdx);
extern void func_80021E74(f32 scale, ModelInstance *modelInst);
extern void func_80022200(Object *obj, s32 param2, s32 param3);
extern u32 obj_alloc_objdata(Object *obj, u32 addr);
extern u32 obj_init_event_data(s32 param1, Object *obj, u32 addr);
extern u32 func_8002298C(s32 param1, ModelInstance *param2, Object *obj, u32 addr);
extern f32 obj_calc_vis_radius(Object *obj);

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

static int useExtraDescriptionsObjects = -1;
static s16* customObjDefTextIDs = NULL;

/** For toggling LaminGaming's extra object description text */
static void remove_extra_descriptions_object(ObjDef* def){
    u16 bankID = (def->gametextIndex[0] & 0xF00) >> 8;
    u16 lineID = def->gametextIndex[0] & 0xFF;

    //Check if description text in bank 0 (gametext3) and beyond original last line
    if (bankID == 0 && lineID > 107){
        def->gametextIndex[0] = -1;
        
    //Check if description text in bank 1 (gametext568) and beyond original last line
    } else if (bankID == 1 && lineID > 3){
        def->gametextIndex[0] = -1;
    }
}

static void remove_extra_descriptions_objects(void) {
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

static void add_extra_descriptions_objects(void) {
    s32 index;
    s16 customIndex;
    ObjDef* def;

    for (index = 0; index < gNumObjectsTabEntries; index++){
        def = gLoadedObjDefs[index];
        if (def == NULL){
            continue;
        }

        def->gametextIndex[0] = customObjDefTextIDs[index];
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
        // @recomp: Warn about failures
        recomp_eprintf("obj_load_objdef: tab idx %d out of range\n", tabIdx);
        return NULL;
    }
    
    if (gObjDefRefCount[tabIdx] != 0) {
        gObjDefRefCount[tabIdx]++;
        def = gLoadedObjDefs[tabIdx];
        return def;
    }
    
    fileOffset = gFile_OBJECTS_TAB[tabIdx];
    fileSize = gFile_OBJECTS_TAB[tabIdx + 1] - fileOffset;

    def = (ObjDef*)mmAlloc(fileSize, ALLOC_TAG_OBJECTS_COL, ALLOC_NAME("obj:def"));
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

        if (def->collectableDef != 0) {
            def->collectableDef = (CollectableDef*)((u32)def + (u32)def->collectableDef);
        }

        if (def->lockdata != 0) {
            def->lockdata = (ObjDefLockData*)((u32)def + (u32)def->lockdata);
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
        customObjDefTextIDs[tabIdx] = def->gametextIndex[0];

        //@recomp: remove extra text if toggled off
        if (!useExtraText){
            remove_extra_descriptions_object(def);
        }

    } else {
        // @recomp: Warn about failures
        recomp_eprintf("obj_load_objdef: alloc failed (tab idx %d)\n", tabIdx);
        return NULL;
    }

    return def;
}

/** Check if extra text config has changed */
RECOMP_CALLBACK("*", recomp_on_game_tick_start) void updateExtraTextObjects(void) {
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

/**
 * Set quarterSize for dynamically created setups. This isn't required by the game but is 
 * handy to have for debugging (the game does sometimes set this at runtime, it's not consistent).
 */
RECOMP_PATCH void *obj_alloc_setup(s32 size, s32 objId) {
    ObjSetup *setup;

    setup = (ObjSetup*)mmAlloc(size, ALLOC_TAG_OBJECTS_COL, ALLOC_NAME("romdef"));
    bzero(setup, size);

    // @recomp: Set quarterSize
    setup->quarterSize = mmAlign4(size) / 4;
    setup->uID = -1;
    setup->loadDistance = 100;
    setup->fadeDistance = 50;
    setup->loadFlags = OBJSETUP_LOAD_CAMERA;
    setup->fadeFlags = OBJSETUP_FADE_CAMERA;
    setup->objId = objId;

    return (void*)setup;
}

RECOMP_PATCH Object *obj_create(ObjSetup *setup, u32 initFlags, s32 mapID, s32 param4, Object *parent) {
    Object *obj;

    obj = NULL;
    queue_load_map_object(&obj, setup, initFlags, mapID, param4, parent, 0);
    // @recomp: Restore default.dol behavior and warn about errors
    if (obj != NULL) {
        obj_add_object(obj, initFlags);
    } else {
        recomp_eprintf("Warning: obj_create failed to instantiate object (obj id: %d, UID: 0x%X)\n", setup->objId, setup->uID);
    }
    return obj;
}

RECOMP_PATCH Object *obj_setup_object(ObjSetup *setup, u32 initFlags, s32 mapID, s32 param4, Object *parent, s32 param6) {
    ObjDef *def;
    s32 modelCount;
    s32 var;
    u32 modflags;
    ModelInstance *tempModel;
    Object *obj;
    s32 tabIdx;
    s32 objId;
    s32 j;
    Object objHeader;
    s32 addr;
    s8 modelLoadFailed;

    objId = setup->objId;

    update_pi_manager_array(0, objId);

    if (initFlags & OBJINIT_BY_TABIDX) {
        tabIdx = objId;
    } else {
        if (objId > gObjIndexCount) {
            // @recomp: Restore printf
            recomp_eprintf("objSetupObjectActual objtype out of range %d/%d\n", objId, gObjIndexCount);
            update_pi_manager_array(0, -1);
            return NULL;
        }

        tabIdx = gFile_OBJINDEX[objId];
    }

    bzero(&objHeader, sizeof(Object));

    objHeader.def = obj_load_objdef(tabIdx);
    def = objHeader.def;

    if (def == NULL || (u32)def == 0xFFFFFFFF) {
        // @recomp: Semi restore printfs
        recomp_eprintf("Warning: Unknown object type '%d/%d'\n", tabIdx, setup->objId);

        // "Warning: Unknown object type '%d/%d romdefno %d', using DummyObject (128)\n"
        // "Warning: Object romdefno is -1, check the object is in objects.spec" (default.dol)
        return NULL;
    } 
    
    objHeader.srt.flags = OBJFLAG_UNK_2;

    if (def->flags & OBJDEF_FORCE_TRANSPARENT_DRAW_ORDER) {
        objHeader.srt.flags |= OBJFLAG_FORCE_TRANSPARENT_DRAW_ORDER;
    }

    if (def->flags & OBJDEF_FLAG40000) {
        objHeader.stateFlags |= OBJSTATE_UNK80;
    }

    if (initFlags & OBJINIT_FLAG4) {
        objHeader.srt.flags |= OBJFLAG_OWNS_SETUP;
    }

    objHeader.srt.transl.x = setup->x;
    objHeader.srt.transl.y = setup->y;
    objHeader.srt.transl.z = setup->z;
    objHeader.setup = setup;
    objHeader.tabIdx = tabIdx;
    objHeader.id = objId;
    objHeader.unkB2 = param4;
    objHeader.mapID = mapID;
    objHeader.curModAnimIdLayered = -1;
    objHeader.seqSlot = SEQSLOT_NONE;
    objHeader.srt.scale = def->scale;
    objHeader.opacity = 255;
    objHeader.mesgQueue = NULL;
    objHeader.loadDistance = setup->loadDistance * 8;
    objHeader.fadeDistance = setup->fadeDistance * 8;
    objHeader.dll = NULL;

    if (def->dllID != 0) {
        objHeader.dll = (DLL_IObject*)dll_load(def->dllID, 6, 1);
        // @recomp: Restore printf
        if (objHeader.dll == NULL) {
            recomp_eprintf("OBJECTS: warning DLL load failed\n");
        }
    }

    modflags = obj_get_model_flags(&objHeader);

    if (def->flags & OBJDEF_FLAG20) {
        modflags &= ~MODFLAGS_1;
    } else {
        modflags |= MODFLAGS_1;
    }

    if (def->shadowType != OBJ_SHADOW_NONE) {
        modflags |= MODFLAGS_SHADOW;
    } else {
        modflags &= ~MODFLAGS_SHADOW;
    }

    if (def->flags & OBJDEF_INVISIBLE) {
        modflags |= MODFLAGS_DONT_LOAD_MODEL;
    }

    var = obj_calc_mem_size(&objHeader, def, modflags);

    obj = (Object*)mmAlloc(var, ALLOC_TAG_OBJECTS_COL, ALLOC_NAME("obj"));

    if (obj == NULL) {
        // @recomp: Restore printf
        recomp_eprintf("ObjSetupObject(3) Memory fail!!\n");
        obj_free_objdef(tabIdx);
        return NULL;
    }

    bcopy(&objHeader, obj, sizeof(Object));
    bzero((void*)((u32)obj + sizeof(Object)), var - sizeof(Object));

    modelCount = def->numModels;

    obj->modelInsts = (ModelInstance**)((u32)obj + sizeof(Object));

    modelLoadFailed = FALSE;
    var = 0;
    
    if (!(modflags & MODFLAGS_DONT_LOAD_MODEL)) {
        if (modflags & MODFLAGS_LOAD_SINGLE_MODEL) {
            var = MODFLAGS_GET_MODEL_INDEX(modflags);

            if (var < modelCount) {
                obj->modelInsts[var] = model_load_create_instance(-def->pModelList[var], modflags);

                if (obj->modelInsts[var] == NULL) {
                    modelLoadFailed = TRUE;
                    goto modelLoadFailedLabel;
                } else {
                    tempModel = obj->modelInsts[var];
                    func_80021E74(obj->srt.scale, tempModel);
                }
            }
        } else {
            for (; var < modelCount; var++) {
                obj->modelInsts[var] = model_load_create_instance(-def->pModelList[var], modflags);
                if (obj->modelInsts[var] == NULL) {
                    modelLoadFailed = TRUE;
                } else {
                    tempModel = obj->modelInsts[var];
                    func_80021E74(obj->srt.scale, tempModel);
                }
            }
        }
    }

    modelLoadFailedLabel:
    if (modelLoadFailed) {
        // @recomp: Warn about failed model loads
        recomp_eprintf("Warning: Model load failed for object type '%d/%d'\n", tabIdx, setup->objId);
        func_80022200(obj, modelCount, objId);
        obj_free_objdef(tabIdx);
        return NULL;
    }
     
    addr = obj_alloc_objdata(obj, (u32)&obj->modelInsts[def->numModels]);

    if (modflags & MODFLAGS_EVENTS) {
        addr = obj_init_event_data(obj->id, obj, addr);
    }

    if (modflags & MODFLAGS_100) {
        addr = func_8002298C(obj->id, obj->modelInsts[0], obj, addr);
    }

    if ((modflags & MODFLAGS_SHADOW) && (def->shadowType != OBJ_SHADOW_NONE)) {
        addr = shadows_init_obj_shadow(obj, addr, 0);
    }

    obj->visRadius = obj_calc_vis_radius(obj) * obj->srt.scale;

    if (def->unk8F != 0) {
        addr = func_8002667C(obj, addr);

        if (def->unk93 & 0x8) {
            addr = func_80026BD8(obj, addr);
        }
    }

    if (def->numSequenceBones != 0) {
        obj->unk6C = (void*)mmAlign4(addr);
        addr = (u32)obj->unk6C + (def->numSequenceBones * sizeof(s16) * 9);
    }

    if (def->numAnimatedFrames != 0) {
        obj->unk70 = (Vtx*)mmAlign4(addr);
        addr = (u32)obj->unk70 + (def->numAnimatedFrames * sizeof(Vtx));
    }

    if (def->numLockdata != 0) {
        obj->unk74 = (ObjectStruct74*)mmAlign4(addr);
        addr = (u32)obj->unk74 + (def->numLockdata * sizeof(ObjectStruct74));
    }

    if (def->unk8F != 0 && def->unk74 != 0) {
        addr = mmAlign4(addr);
        addr = func_80026A20(obj->id, obj->modelInsts[0], obj->objhitInfo, addr, obj);
    }

    if (def->numLockdata != 0) {
        obj->unk78 = (ObjectStruct78*)mmAlign4(addr);

        for (j = 0; j < def->numLockdata; j++) {
            obj->unk78[j].flags = def->lockdata[j].flags;
            obj->unk78[j].interactRadius = def->lockdata[j].interactRadius;
            obj->unk78[j].hlAngularRange = def->lockdata[j].hlAngularRange;
            obj->unk78[j].lockExitRadius = def->lockdata[j].lockExitRadius;
            obj->unk78[j].hlRadius = def->lockdata[j].hlRadius;
        }

        // addr = (u32)obj->unk78 + (def->unk9b * sizeof(ObjectStruct78)); // default.dol
    }

    // default.dol (size is mmAlloc size)
    // if (size != (addr - (s32)obj)) {
    //     // "objects.c: objSetupObject: sizes do not match\n"
    // }

    obj->parent = parent;
    
    return obj;
}

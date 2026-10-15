#include "modding.h"
#include "recomputils.h"

#include "dlls/engine/33_BaddieControl.h"
#include "dlls/objects/210_player.h"
#include "dlls/objects/260_Pollen.h"
#include "game/gamebits.h"
#include "game/objects/interaction_arrow.h"
#include "game/objects/object.h"
#include "game/objects/object_id.h"
#include "sys/main.h"
#include "sys/memory.h"
#include "sys/objmsg.h"
#include "sys/objtype.h"
#include "sys/objects.h"
#include "dll.h"

#include "recomp/dlls/engine/33_BaddieControl_recomp.h"

extern Vec3f _data_8;
extern f32 _data_14;
extern Vec3f _data_18;
extern f32 _data_24;

RECOMP_PATCH void BaddieControl_setup(Object* obj, Baddie_Setup* setup, Baddie* baddie, s32 totalAnimStates, s32 totalLogicStates, s32 arg5, u8 arg6, f32 arg7) {
    s32 sp4C[] = { 0x2 };
    u8 sp4B;
    u8 hitpoints;
    s32 sp3C;
    s32 sp38;

    sp4B = 1;
    baddie->objdata = (void*)(baddie + 1);
    baddie->unk3B6 = 0;
    sp38 = arg6 & 1;
    sp3C = (s32) arg6;
    if ((sp38 == 0) && !(arg6 & 0x20)) {
        objAddObjectType(obj, OBJTYPE_Baddie);
        objInitMesgQueue(obj, 4);
    }
    gDLL_18_objfsa->vtbl->func0(obj, &baddie->fsa, totalAnimStates, totalLogicStates);
    baddie->fsa.flags = 0;
    baddie->fsa.unk33D = 0;
    baddie->fsa.unk278 = 0.0f;
    baddie->fsa.unk27C = 0.0f;
    hitpoints = setup->quarterHitpoints;
    if (hitpoints != 0) {
        baddie->fsa.hitpoints = hitpoints * 4;
    } else {
        baddie->fsa.hitpoints = 6 * 4; // default to 6 HP
    }
    baddie->unk39E = setup->unk30;
    baddie->unk3A0 = setup->unk1A;
    baddie->unk3A2 = setup->unk1C;
    if (baddie->unk39E != -1) {
        mainSetBits(baddie->unk39E, 0);
    }
    if (sp3C & 2) {
        gDLL_27->vtbl->init(&baddie->fsa.unk4, DLL27FLAG_NONE, arg5 | DLL27FLAG_200000, DLL27MODE_1);
    } else {
        gDLL_27->vtbl->init(&baddie->fsa.unk4, DLL27FLAG_NONE, DLL27FLAG_NONE, DLL27MODE_DISABLED);
    }
    gDLL_27->vtbl->setup_hits_collider(&baddie->fsa.unk4, 1, &_data_18, &_data_24, 4);
    if (sp3C & 4) {
        gDLL_27->vtbl->setup_terrain_collider(&baddie->fsa.unk4, 1, &_data_8, &_data_14, &sp4B);
    }
    gDLL_27->vtbl->reset(obj, &baddie->fsa.unk4);
    baddie->unk3A8 = 0;
    baddie->unk3B0 = setup->unk2B;
    baddie->unk3E0 = setup->unk22;
    baddie->unk3B8 = setup->unk2F;
    baddie->nextWeaponID = setup->initialWeaponID;
    baddie->unk3BA = setup->unk28;
    obj->stateFlags |= baddie->unk3BA & OBJSTATE_UNK_ATTACH_INDEX_MASK;
    if (sp3C & 8) {
        baddie->unk3A4 = setup->unk20;
        baddie->unk3A6 = setup->unk1E;
    } else {
        baddie->unk3A4 = 0;
        baddie->unk3A6 = 0;
    }
    baddie->unk3B2 = 0;
    baddie->unk3E2 = setup->unk29 * 8;
    baddie->unk3B4 = 0;
    obj->srt.transl.x = setup->base.x;
    obj->srt.transl.y = setup->base.y;
    obj->srt.transl.z = setup->base.z;
    baddie->unk3E4 = arg7;
    obj->srt.yaw = setup->unk2A << 8;
    obj->opacity = OBJECT_OPACITY_MAX;
    obj->unkAF &= ~ARROW_FLAG_8_No_Targetting;

    baddie->unk39C = setup->unk18;
    if (baddie->unk39C != NO_GAMEBIT) {
        obj->unkDC = mainGetBits(baddie->unk39C);
    } else {
        obj->unkDC = FALSE;
    }

    if (gDLL_29_Gplay->vtbl->did_time_expire(setup->base.uID) == 0) {
        obj->unkDC = 1;
    }
    if (obj->unkDC != 0) {
        func_800267A4(obj);
        // @recomp: Don't return here, otherwise Baddies that spawn with a gplay timer (i.e. are still defeated)
        //          will not have the curve/voxmap data that's allocated below actually allocated. Notably, this
        //          causes some SharpClaw to crash if their timer expires while the player is near.
        //return;
    } else {
        func_8002674C(obj);
    }
    if (setup->unk2E == -1) {
        obj->unkE0 = 1;
    } else {
        obj->unkE0 = 0;
    }
    if ((sp38 == 0) && !(sp3C & 0x20)) {
        vox_func_80008DC0(&baddie->unk374);
        baddie->unk34C.unk26 = 4;
        baddie->unk34C.unk27 = 0x14;
    }
    if (sp3C & 0x10) {
        if ((baddie->unk3F8 == NULL) && !(sp3C & 0x20)) {
            baddie->unk3F8 = mmAlloc(sizeof(UnkCurvesStruct), ALLOC_TAG_TEST_COL, ALLOC_NAME("BaddieControlDLL"));
        }
        if (baddie->unk3F8 != NULL) {
            bzero(baddie->unk3F8, sizeof(UnkCurvesStruct));
        }
        if (gDLL_26_Curves->vtbl->func_4288(baddie->unk3F8, obj, (f32) baddie->unk3E2, sp4C, -1) == 0) {
            baddie->unk3B2 |= 8;
        }
    } else {
        baddie->unk3F8 = NULL;
    }
}

// #define DEBUG_ICE_BLAST_DAMAGE_MULTIPLIER

/* Apply a damage multiplier when Baddies are attacked while frozen by Ice Blast */
RECOMP_PATCH s32 BaddieControl_check_hit(Object* obj, ObjFSA_Data* fsa, Unk80009024* arg2, s32 arg3, 
        s32* hitAnimStateMap, s8* hitDamageMap, s16 hitLogicState, u32* soundHandle, SRT* hitSRT) {
    Baddie* baddie;
    Object* player;
    s32 hitType;
    s32 hitSphereID;
    s32 damage;
    Object* hitBy;
    f32 hitX;
    f32 hitY;
    f32 hitZ;

    baddie = (Baddie*)obj->data;
    player = objGetPlayer();

    if (baddie->unk3E8 > 0.0f) {
        baddie->unk3E8 += (gUpdateRateF * baddie->unk3EC);
        if (baddie->unk3B2 & 0x20) {
            baddie->unk3B2 &= ~0x20;
            baddie->unk3B2 |= 0x40;
            if (baddie->unk3E8 > 2.0f) {
                baddie->unk3E8 = 0.0f;
                baddie->unk3B2 &= ~0x40;
            }
        } else if (baddie->unk3B2 & 0x40) {
            if (baddie->unk3E8 > 2.0f) {
                Baddie_Setup* objsetup = (Baddie_Setup*)obj->setup;
                baddie->unk3E8 = 0.0f;
                baddie->unk3B2 &= ~0x40;
                fsa->hitpoints = 0;
                obj->opacity = 0;
                obj->unkDC = 1;
                gDLL_29_Gplay->vtbl->add_time(objsetup->base.uID, (f32) (objsetup->unk2C * 60));
            }
        } else {
            if (baddie->unk3E8 < 0.0f) {
                baddie->unk3E8 = 0.0f;
            } else if (baddie->unk3E8 > 120.0f) {
                baddie->unk3E8 = (120.0f - (baddie->unk3E8 - 120.0f));
                baddie->unk3EC = -baddie->unk3EC;
            }
        }
    }

    if (fsa->hitpoints == 0) {
        return 0;
    }

    hitType = func_8002601C(obj, &hitBy, &hitSphereID, &damage, &hitX, &hitY, &hitZ);
    baddie->unk3F0 = hitSphereID;

    if ((obj != NULL) && (hitType != 0) && (hitBy != NULL)) {
        switch (obj->id) {
        case OBJ_ScorpionRobot:
            if ((hitBy->id != OBJ_sword) && (hitBy->id != OBJ_staff) && (hitBy->id != OBJ_projball)) {
                return 0;
            }
            break;
        case OBJ_WG_PollenCannon:
            if (hitBy->id == OBJ_Pollen) {
                if (((Pollen_Data*)hitBy->data)->unk12 == 0) {
                    hitBy->opacity = 0;
                }
                return 0;
            }
            if (hitBy->id == OBJ_PollenFragment) {
                hitBy->opacity = 0;
                return 0;
            }
            break;
        }
    }
    
    if (hitType != 0) {
        damage *= 4;
        if (hitSRT != NULL) {
            hitSRT->transl.x = hitX + gWorldX;
            hitSRT->transl.y = hitY;
            hitSRT->transl.z = hitZ + gWorldZ;
        }
        if (hitDamageMap != NULL) {
            if (hitDamageMap[hitType - 2] != -1) {
                damage = hitDamageMap[hitType - 2];
            }
        } else {
            damage = 0;
        }

        //@recomp: multiply damage by 4 when frozen
        if (baddie->unk3B2 & 0x80) {
            damage *= 4;
#ifdef DEBUG_ICE_BLAST_DAMAGE_MULTIPLIER
            if (damage) {
                recomp_printf("ATTACKED WHILE FROZEN BY ICE BLAST, x4 DAMAGE!\n");
            }
#endif
        }

        // STUBBED_PRINTF("%s hit by type %d for %d points\n", obj->def->name, hitType, damage); (default.dol)

        fsa->hitpoints -= damage;
        if (fsa->hitpoints <= 0) {
            baddie->unk3B2 |= 0x20;
            baddie->unk3E8 = 1.0f;
            baddie->unk3EC = 0.01f;
            fsa->logicState = hitLogicState;
            fsa->hitpoints = 0;
        } else if (damage != 0) {
            if ((fsa->target == NULL) && dll_player(player)->func66(player, 1)) {
                fsa->target = player;
                fsa->unk33D = 0;
            }
            baddie->unk3E8 = 1.0f;
            baddie->unk3EC = 12.0f;
            if (hitAnimStateMap != 0) {
                if (hitAnimStateMap[hitType - 2] != -1) {
                    gDLL_18_objfsa->vtbl->set_anim_state(obj, fsa, hitAnimStateMap[hitType - 2]);
                    fsa->logicState = hitLogicState;
                }
            }
            fsa->lastHitType = hitType;
        }

        if (*soundHandle != 0) {
            dll_amSfx->Stop(*soundHandle);
            *soundHandle = 0;
        }

        objSendMesg(hitBy, 0xE0001, obj, NULL);
    }

    return hitType;
}

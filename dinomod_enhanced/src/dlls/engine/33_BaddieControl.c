#include "modding.h"
#include "recomputils.h"

#include "PR/ultratypes.h"
#include "dll.h"
#include "dlls/objects/210_player.h"
#include "dlls/objects/260_Pollen.h"
#include "game/objects/object_id.h"
#include "game/objects/object.h"
#include "sys/main.h"
#include "sys/objects.h"
#include "sys/objmsg.h"
#include "types.h"

#include "recomp/dlls/engine/33_BaddieControl_recomp.h"

//TEMPORARY DEFINES
#define dll_player(obj) (((DLL_210_Player*)obj->dll)->vtbl)
//END OF TEMPORARY DEFINES

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

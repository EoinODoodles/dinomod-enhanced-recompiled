/** Helper functions for getting player's SpellStone/Spirit/Duster counts */

#include "player_stats.h"
#include "game/gamebits.h"
#include "sys/main.h"
#include "sys/print.h"

typedef enum {
    FLAG_WARLOCK_MOUNTAIN_SETUP = BIT_WM_Map_Setup,

    FLAG_SPELLSTONE_DIM_COLLECTED = BIT_SpellStone_DIM,
    FLAG_SPELLSTONE_DIM_ACTIVATED = BIT_SpellStone_DIM_Activated,
    FLAG_SPELLSTONE_DIM_DEPOSITED = 0x877,

    FLAG_SPELLSTONE_CRF_COLLECTED = NULL,
    FLAG_SPELLSTONE_CRF_ACTIVATED = BIT_SpellStone_CRF,
    FLAG_SPELLSTONE_CRF_DEPOSITED = BIT_DFP_Place_Spellstone_One,

    FLAG_SPELLSTONE_WC_COLLECTED = NULL,
    FLAG_SPELLSTONE_WC_ACTIVATED = BIT_SpellStone_WC,
    FLAG_SPELLSTONE_WC_DEPOSITED = 0x878,

    FLAG_SPELLSTONE_BWC_COLLECTED = NULL,
    FLAG_SPELLSTONE_BWC_ACTIVATED = BIT_SpellStone_BWC,
    FLAG_SPELLSTONE_BWC_DEPOSITED = 0x5F4,

    FLAG_SPELLSTONE_KP_COLLECTED = NULL,
    FLAG_SPELLSTONE_KP_ACTIVATED = BIT_SpellStone_KP,
    FLAG_SPELLSTONE_KP_DEPOSITED = 0x5f5,

    FLAG_SPELLSTONE_DR_COLLECTED = NULL,
    FLAG_SPELLSTONE_DR_ACTIVATED = 0x7cc,
    FLAG_SPELLSTONE_DR_DEPOSITED = 0x8fa,
} SpellStoneSpiritGamebits;

const int spellStoneFlags[6][3] = {
    {
        FLAG_SPELLSTONE_DIM_COLLECTED, 
        FLAG_SPELLSTONE_DIM_ACTIVATED, 
        FLAG_SPELLSTONE_DIM_DEPOSITED, 
    },
    {
        FLAG_SPELLSTONE_CRF_COLLECTED, 
        FLAG_SPELLSTONE_CRF_ACTIVATED, 
        FLAG_SPELLSTONE_CRF_DEPOSITED, 
    },
    {
        FLAG_SPELLSTONE_WC_COLLECTED, 
        FLAG_SPELLSTONE_WC_ACTIVATED, 
        FLAG_SPELLSTONE_WC_DEPOSITED, 
    },
    {
        FLAG_SPELLSTONE_BWC_COLLECTED, 
        FLAG_SPELLSTONE_BWC_ACTIVATED, 
        FLAG_SPELLSTONE_BWC_DEPOSITED, 
    },
    {
        FLAG_SPELLSTONE_KP_COLLECTED, 
        FLAG_SPELLSTONE_KP_ACTIVATED, 
        FLAG_SPELLSTONE_KP_DEPOSITED, 
    },
    {
        FLAG_SPELLSTONE_DR_COLLECTED, 
        FLAG_SPELLSTONE_DR_ACTIVATED, 
        FLAG_SPELLSTONE_DR_DEPOSITED, 
    }
};

s8 getCountSpellStones(){
    s8 spellStones = 0;
    s8 spellStoneIndex;
    s8 stateIndex;

    for (spellStoneIndex = 0; spellStoneIndex < 6; spellStoneIndex++){
        for (stateIndex = 0; stateIndex < 3; stateIndex++){
            if (spellStoneFlags[spellStoneIndex][stateIndex] == NULL){
                continue;
            }
            if (main_get_bits(spellStoneFlags[spellStoneIndex][stateIndex])){
                spellStones++;
                break;
            }
        }
    }

    return spellStones;
}

s8 getCountSpirits(){
    s8 spirits = 0;

    spirits = main_get_bits(FLAG_WARLOCK_MOUNTAIN_SETUP) - 1;
    if (spirits < 0){
        spirits = 0;
    } else if (spirits > 8){
        spirits = 8;
    }

    return spirits;
}

s8 getCountDusters(){
    s8 dusters = 0;
    Object *player = get_player();
    PlayerStats *playerStats;

    if (player){
        playerStats = ((Player_Data*)player->data)->stats;
        if (playerStats){
            dusters = playerStats->dusters;
        }
    }
    return dusters;
}

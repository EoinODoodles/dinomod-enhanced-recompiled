/** Helper functions for getting player's SpellStone/Spirit/Duster counts */

#include "player_stats.h"
#include "sys/print.h"

enum GameBits{
    FLAG_WARLOCK_MOUNTAIN_SETUP = 0xe4,

    FLAG_SPELLSTONE_DIM_COLLECTED = 0x123,
    FLAG_SPELLSTONE_DIM_ACTIVATED = 0x22b,
    FLAG_SPELLSTONE_DIM_DEPOSITED = 0x877,

    FLAG_SPELLSTONE_CRF_COLLECTED = NULL,
    FLAG_SPELLSTONE_CRF_ACTIVATED = 0x2e8,
    FLAG_SPELLSTONE_CRF_DEPOSITED = 0x5f3,

    FLAG_SPELLSTONE_WC_COLLECTED = NULL,
    FLAG_SPELLSTONE_WC_ACTIVATED = 0x83b,
    FLAG_SPELLSTONE_WC_DEPOSITED = 0x878,

    FLAG_SPELLSTONE_BWC_COLLECTED = NULL,
    FLAG_SPELLSTONE_BWC_ACTIVATED = 0x83a,
    FLAG_SPELLSTONE_BWC_DEPOSITED = 0x5f4,

    FLAG_SPELLSTONE_KP_COLLECTED = NULL,
    FLAG_SPELLSTONE_KP_ACTIVATED = 0x7bd,
    FLAG_SPELLSTONE_KP_DEPOSITED = 0x5f5,

    FLAG_SPELLSTONE_DR_COLLECTED = NULL,
    FLAG_SPELLSTONE_DR_ACTIVATED = 0x7cc,
    FLAG_SPELLSTONE_DR_DEPOSITED = 0x8fa,
};

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
            if (get_gplay_bitstring(spellStoneFlags[spellStoneIndex][stateIndex])){
                spellStones++;
                break;
            }
        }
    }

    // diPrintf("Spellstones: %1d\n", spellStones);

    return spellStones;
}

s8 getCountSpirits(){
    s8 spirits = 0;

    spirits = get_gplay_bitstring(FLAG_WARLOCK_MOUNTAIN_SETUP) - 1;
    if (spirits < 0)
        spirits = 0;
    if (spirits > 8)
        spirits = 8;

    // diPrintf("Spirits: %1d\n", spirits);

    return spirits;
}

s8 getCountDusters(){
    s8 dusters = 0;
    Object *player = get_player();
    PlayerStats *playerStats;

    if (player){
        playerStats = ((PlayerState*)player->state)->stats;
        if (playerStats){
            dusters = playerStats->dusters;
        }
    }
    return dusters;
}
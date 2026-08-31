#include "common_objsetups.h"
#include "configs.h"
#include "modding.h"
#include "recomputils.h"
#include "recompconfig.h"

#include "old_pickup_sfx_bank.h"

#include "PR/os.h"
#include "PR/ultratypes.h"
#include "libnaudio/n_libaudio.h"
#include "libnaudio/n_sndplayer.h"
#include "mp3/mp3.h"
#include "dll.h"
#include "game/gamebits.h"
#include "game/objects/object.h"
#include "game/objects/object_id.h"
#include "sys/acache.h"
#include "sys/asset.h"
#include "sys/dll.h"
#include "sys/pi.h"
#include "sys/main.h"
#include "sys/map.h"
#include "sys/memory.h"
#include "sys/mpeg.h"
#include "sys/objects.h"
#include "types.h"

#include "recomp/dlls/engine/6_AMSFX_recomp.h"

// #define DEBUG_SOUNDS

#define IS_MP3 0x8000
#define PITCH_DEFAULT 100

#define MAX_SOUND_SLOTS 64

enum SoundSlotFlags {
    SOUNDSLOT_IN_USE = 0x40,
    SOUNDSLOT_PLAYING = 0x80
};

typedef struct SoundSlot {
    SoundDef def;
    s16 soundID;
    u16 pad10;
    u8 flags; // SoundSlotFlags
    u8 unk13;
    s8 unk14;
    s8 unk15;
    s8 volume; // effective volume
    s8 distVolume; // volume with only distance-based falloff considered
    Object *source; // object playing the sound
    sndstate *sndpHandle;
} SoundSlot;

typedef struct WaterFallSpray {
    Vec3f pos;
    u16 unkC;
    u16 unkE;
} WaterFallSpray;

#define MAX_WATER_FALL_SPRAY 16

enum AMSFXWaterFallsFlags {
    // The follow flags cut the volume of the high or low waterfall sfx in half for each flag set.
    AMSFX_WATERFALLS_LOWER_HIGH = 0x1,
    AMSFX_WATERFALLS_LOWER_LOW = 0x2,
    AMSFX_WATERFALLS_LOWER_HIGH2 = 0x4,
    AMSFX_WATERFALLS_LOWER_LOW2 = 0x8,
    // Clear the list of waterfall sprays and re-search the map for an updated list.
    AMSFX_WATERFALLS_REFRESH = 0x10
};

extern ALBankFile *_bss_0; //SFX.bin buffer
extern SoundSlot *sSndSlots; //active sounds
extern s32 sSndSlotsLen; //active sound count?
extern u32 _bss_C;
extern u32 _bss_10;
extern u32 sWaterfallHighHandle;
extern u32 sWaterfallLowHandle;
extern u8 sWaterfallsLastMap; //mapID?
extern u8 sWaterFallSprayCount;
extern u8 sWaterfallFlags;
extern u8 sWaterfallLowVolume; // sound volume
extern u8 sWaterfallHighVolume; // sound volume
extern WaterFallSpray sWaterFallSprays[MAX_WATER_FALL_SPRAY];
extern ACache *sSoundDefCache;

extern void amSfx_SetVol(u32 soundHandle, u8 volume);
extern void amSfx_Stop(u32 arg0);
extern s32 amSfx_GetDefault(u16 arg0, SoundDef* arg1);
extern s32 amSfx_waterFallsFindSprays(void);
extern s32 amSfx_makeHandle(s32 handle, char *filename, s32 lineNo);
extern s32 amSfx_freeHandle(u32);
extern void amSfx_func_1F78(void);
extern void amSfx_func_2240(Object* obj, f32* xo, f32* yo, f32* zo, u16* yawOut);
extern void amSfx_func_22FC(f32 arg0, f32 arg1, f32 arg2, SoundDef* arg3, s8* outVolume);
extern void amSfx_func_2438(f32 arg0, f32 arg1, s32 arg2, s8* outPan, s8* outFx);

enum RecompSoundIDs {
    SOUND_B8A_FirstTimeItemPickup = 0xB8A,
    SOUND_749_Garunda_Te_If_you_bring_me_12_FrostWeeds = 0x749
};

enum SoundTypes {
    WAV = 0,
    MP3 = 1
};
enum SoundFlags {
    SOUND_IS_MP3 = 0x8000
};

#define SOUND(fileID, bankID) (fileID + 1) | (bankID << 15)

/** Changes it so Garunda Te doesn't say "12" FrostWeeds if the total needed has changed */
static void recomp_sound_remap_garunda_te_frostweeds(u16 soundID, SoundDef* soundEntry){
    //Don't replace the MP3 if the FrostWeed goal hasn't changed
    if (configs_GetFrostWeedMax() == 12){
        return;
    }

    soundEntry->bankAndClipID = SOUND(1033, MP3);
}

static void recomp_intercept_soundIDs(Object* obj, u16 soundID, SoundDef* soundEntry, ALBank **bank) {
    u32 pickupJingleConfig;

#ifdef DEBUG_SOUNDS
    if (obj && obj->def) {
        if (obj->controlNo == OBJCONTROL_AnimObj) {
            AnimObj_Data* animData = obj->data;
            recomp_printf("PLAY SOUND: %0x [%s (Override)]\n", soundID, (animData->actor && animData->actor->def) ? animData->actor->def->name : "");
        } else {
            recomp_printf("PLAY SOUND: %0x [%s]\n", soundID, obj->def->name);
        }
    } else {
        recomp_printf("PLAY SOUND: %0x\n", soundID);
    }
#endif

    switch (soundID){
        case SOUND_B8A_FirstTimeItemPickup:
            // @recomp: Replace item pickup jingle with the old version (original patch by nuggs)
            pickupJingleConfig = configs_GetPickupJingleMode();
            if (pickupJingleConfig != RECOMP_PICKUP_JINGLE_NEW) {
                *bank = recomp_oldPickupSfxBank;
                soundEntry->bankAndClipID = pickupJingleConfig == RECOMP_PICKUP_JINGLE_OLD_A ? 1 : 2;
                soundEntry->volume = MAX_VOLUME;
                soundEntry->pitch = PITCH_DEFAULT;
                soundEntry->pan = PAN_CENTRE;
            }
            break;
        case SOUND_749_Garunda_Te_If_you_bring_me_12_FrostWeeds:
            recomp_sound_remap_garunda_te_frostweeds(soundID, soundEntry);
            break;
    }
}

RECOMP_PATCH u32 amSfx_Play(Object* obj, u16 soundID, u8 volume, u32* soundHandle, char *filename, s32 lineNo, char *code) {
    SoundDef soundDef;
    f32 x;
    f32 y;
    f32 z;
    u32 handle;
    u16 yaw;
    s8 volumeCalc;

    sSndSlots->flags = 0;
    sSndSlots->sndpHandle = NULL;
    
    //Bail if soundID is 0
    if (!soundID) {
        return 0;
    }

    //Get sound definition from AUDIO.bin subfile 0
    amSfx_GetDefault(soundID, &soundDef);

    // @recomp: Support multiple banks
    ALBank *bank = _bss_0->bankArray[0];

    //@recomp: intercept sound calls and edit as needed
    //recomp_printf("AMSFX: play sound #%d (%s)\n", soundID, soundEntry.unk0 & 0x8000 ? "MP3" : "WAV");
    recomp_intercept_soundIDs(obj, soundID, &soundDef, &bank);

    //Bail if sound's clipID is 0
    if (!(soundDef.bankAndClipID & 0x7FFF)) {
        return 0;
    }

    if (soundHandle != NULL) {
        handle = *soundHandle;
    } else {
        handle = 0;
    }

    handle = amSfx_makeHandle(handle, filename, lineNo);
    sSndSlots[handle].soundID = soundID;
    sSndSlots[handle].source = obj;
    sSndSlots[handle].distVolume = MAX_VOLUME;

    if ((obj != NULL) && (soundDef.volumeFalloff & 3)) {
        amSfx_func_2240(obj, &x, &y, &z, &yaw);
        amSfx_func_22FC(x, y, z, &soundDef, &volumeCalc);
        sSndSlots[handle].distVolume = volumeCalc;
    }

    sSndSlots[handle].volume = (volume * soundDef.volume) >> 7;
    volumeCalc = (sSndSlots[handle].volume * sSndSlots[handle].distVolume) >> 7;
    sSndSlots[handle].unk13 = volumeCalc;

    if (soundDef.bankAndClipID & IS_MP3) {
        sSndSlots[handle].sndpHandle = (sndstate* )-2;
        // @fake
        if (sSndSlots[handle].def.bankAndClipID) {}
        mpegPlay((soundDef.bankAndClipID & 0x7FFF) - 1);
        mp3SetVolume(volumeCalc << 8, 0);
        // @fake
        if (sSndSlots) {}
    } else {
        // @recomp: Support multiple banks
        some_sound_func(
            bank, 
            soundDef.bankAndClipID, 
            (volumeCalc << 8), 
            PAN_CENTRE, 
            soundDef.pitch / 100.0f, 
            (s32)(f32)soundDef.unk6, 
            1, 
            &sSndSlots[handle].sndpHandle);
    }

    bcopy(&soundDef, &sSndSlots[handle], sizeof(SoundDef));
    bcopy(&sSndSlots[handle], sSndSlots, sizeof(SoundSlot));

    if (soundHandle != NULL) {
        *soundHandle = handle;
    } else {
        sSndSlots[handle].flags |= SOUNDSLOT_PLAYING;
    }
    return handle;
}

RECOMP_PATCH void amSfx_WaterFallsControl(void) {
    Object* player;
    s32 i;
    f32 distance;
    u32 lowVolume;
    u32 highVolume;
    u8 mapID;
    Vec3f* camera;

    player = objGetPlayer();
    mapID = mapWorldXZToMapID(player->srt.transl.x, player->srt.transl.z);
    if ((mapID != sWaterfallsLastMap) || (sWaterfallFlags & AMSFX_WATERFALLS_REFRESH)) {
        amSfx_waterFallsFindSprays();
        sWaterfallsLastMap = mapID;
        sWaterfallFlags &= ~AMSFX_WATERFALLS_REFRESH;
    }
    // @recomp: Still run if there are no sprays
    // if (sWaterFallSprayCount == 0) {
    //     return;
    // }

    camera = (Vec3f*)camGet();
    highVolume = 0;
    lowVolume = 0;
    for (i = 0; i < sWaterFallSprayCount; i++) {
        distance = vec3Distance(camera + 1, &sWaterFallSprays[i].pos);
        if (distance < sWaterFallSprays[i].unkC) {
            lowVolume += MAX_VOLUME - (u8)((u32)((distance / sWaterFallSprays[i].unkC) * MAX_VOLUME_F));
        }
        if (distance < sWaterFallSprays[i].unkE) {
            highVolume += MAX_VOLUME - (u8)((u32)((distance / sWaterFallSprays[i].unkE) * MAX_VOLUME_F));
        }
    }
    if (sWaterfallFlags & AMSFX_WATERFALLS_LOWER_HIGH) {
        highVolume >>= 1;
    }
    if (sWaterfallFlags & AMSFX_WATERFALLS_LOWER_LOW) {
        lowVolume >>= 1;
    }
    if (sWaterfallFlags & AMSFX_WATERFALLS_LOWER_HIGH2) {
        highVolume >>= 1;
    }
    if (sWaterfallFlags & AMSFX_WATERFALLS_LOWER_LOW2) {
        lowVolume >>= 1;
    }
    if (highVolume > MAX_VOLUME) {
        highVolume = MAX_VOLUME;
    }
    if (lowVolume > MAX_VOLUME) {
        lowVolume = MAX_VOLUME;
    }
    // @recomp: Partially rewrite to handle fading out when no more sprays exist rather than abruptly stopping
    if (lowVolume == 0 && sWaterfallLowVolume == 0) {
        if (sWaterfallLowHandle != 0) {
            amSfx_Stop(sWaterfallLowHandle);
            sWaterfallLowHandle = 0;
        }
    } else {
        if (sWaterfallLowHandle == 0) {
            sWaterfallLowVolume = 1;
            amSfx_Play(NULL, SOUND_986_Waterfall_Low_Loop, sWaterfallLowVolume, &sWaterfallLowHandle, "game/amsfx.c", 1016, "");
        }
        // @bug: framerate dependent
        if (lowVolume < sWaterfallLowVolume) {
            sWaterfallLowVolume -= 1;
        } else {
            sWaterfallLowVolume += 1;
        }
        amSfx_SetVol(sWaterfallLowHandle, sWaterfallLowVolume);
    }
    if (highVolume == 0 && sWaterfallHighVolume == 0) {
        if (sWaterfallHighHandle != 0) {
            amSfx_Stop(sWaterfallHighHandle);
            sWaterfallHighHandle = 0;
        }
    } else {
        if (sWaterfallHighHandle == 0) {
            sWaterfallHighVolume = 1;
            amSfx_Play(NULL, SOUND_987_Waterfall_High_Loop, sWaterfallHighVolume, &sWaterfallHighHandle, "game/amsfx.c", 1036, "");
        }
        // @bug: framerate dependent
        if (highVolume < sWaterfallHighVolume) {
            sWaterfallHighVolume -= 1;
        } else {
            sWaterfallHighVolume += 1;
        }
        amSfx_SetVol(sWaterfallHighHandle, sWaterfallHighVolume);
    }
}

/** When searching for WaterFallSpray objects, ignore any that are switched off via a gamebit */
RECOMP_PATCH s32 amSfx_waterFallsFindSprays(void) {
    Object* player;
    s32 offset;
    ObjSetup* setup;
    s32 setupListLength;

    player = objGetPlayer();
    if (player == NULL) {
        return TRUE;
    }
    
    setup = mapWorldXZToMapObjSetupList(player->srt.transl.x, player->srt.transl.z, &setupListLength);
    if (setup == NULL) {
        return TRUE;
    }

    offset = 0;
    sWaterFallSprayCount = 0;
    while (setupListLength > offset && sWaterFallSprayCount < MAX_WATER_FALL_SPRAY) {
        if (setup->objId == OBJ_WaterFallSpray) {
            //@recomp: ignore the WaterFallSpray object if it's switched off via its gamebit
            WaterFallSpray_Setup* spraySetup = (WaterFallSpray_Setup*)setup;
            if ((spraySetup->gamebit <= NO_GAMEBIT + 1) || //the WaterFallSpray doesn't use a gamebit, so it's always on
                (   (spraySetup->invertGamebit && mainGetBits(spraySetup->gamebit))   ||  //switched on when gamebit set
                    (!spraySetup->invertGamebit && !mainGetBits(spraySetup->gamebit))     //switched on when gamebit unset
                )
            ) {
                sWaterFallSprays[sWaterFallSprayCount].pos.x = setup->x;
                sWaterFallSprays[sWaterFallSprayCount].pos.y = setup->y;
                sWaterFallSprays[sWaterFallSprayCount].pos.z = setup->z;
                sWaterFallSprays[sWaterFallSprayCount].unkC = spraySetup->unk21 * 16;
                sWaterFallSprays[sWaterFallSprayCount].unkE = spraySetup->unk22 * 16;
                sWaterFallSprayCount++;
            }
        }
        offset += setup->quarterSize << 2;
        setup = (ObjSetup*)((u8*)setup + (setup->quarterSize << 2));
    }
    return FALSE;
}

#include "PR/os.h"
#include "PR/ultratypes.h"
#include "modding.h"
#include "recomputils.h"
#include "recompconfig.h"

#include "old_pickup_sfx_bank.h"

#include "libnaudio/n_libaudio.h"
#include "libnaudio/n_sndplayer.h"
#include "mp3/mp3.h"
#include "sys/acache.h"
#include "sys/fs.h"
#include "sys/map.h"
#include "sys/mpeg.h"
#include "sys/asset_thread.h"
#include "sys/dll.h"
#include "sys/fs.h"
#include "sys/memory.h"
#include "dll.h"
#include "types.h"

enum RecompPickupJingle {
    RECOMP_PICKUP_JINGLE_OLD_A,
    RECOMP_PICKUP_JINGLE_OLD_B,
    RECOMP_PICKUP_JINGLE_NEW
};

static u32 dinomod_pickup_jingle(void) {
    return recomp_get_config_u32("pickup_jingle");
}

#include "recomp/dlls/engine/6_AMSFX_recomp.h"

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

extern void amsfx_set_vol(u32 soundHandle, u8 volume);
extern void amsfx_stop(u32 arg0);
extern s32 amsfx_get_default(u16 arg0, SoundDef* arg1);
extern s32 amsfx_water_falls_find_sprays(void);
extern s32 amsfx_make_handle(s32 handle, char *filename, s32 lineNo);
extern s32 amsfx_free_handle(u32);
extern void amsfx_func_1F78(void);
extern void amsfx_func_2240(Object* obj, f32* xo, f32* yo, f32* zo, u16* yawOut);
extern void amsfx_func_22FC(f32 arg0, f32 arg1, f32 arg2, SoundDef* arg3, s8* outVolume);
extern void amsfx_func_2438(f32 arg0, f32 arg1, s32 arg2, s8* outPan, s8* outFx);

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
    s32 frostWeedsMax = recomp_get_config_u32("garunda_te_frostweeds_override");

    //Don't replace the MP3 if the FrostWeed goal hasn't changed
    if (frostWeedsMax == 12){
        return;
    }

    soundEntry->bankAndClipID = SOUND(1033, MP3);
}

static void recomp_intercept_soundIDs(u16 soundID, SoundDef* soundEntry, ALBank **bank) {
    u32 pickupJingleConfig;
    switch (soundID){
        case SOUND_B8A_FirstTimeItemPickup:
            // @recomp: Replace item pickup jingle with the old version (original patch by nuggs)
            pickupJingleConfig = dinomod_pickup_jingle();
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

RECOMP_PATCH u32 amsfx_play(Object* obj, u16 soundID, u8 volume, u32* soundHandle, char *filename, s32 lineNo, char *code) {
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
    amsfx_get_default(soundID, &soundDef);

    // @recomp: Support multiple banks
    ALBank *bank = _bss_0->bankArray[0];

    //@recomp: intercept sound calls and edit as needed
    //recomp_printf("AMSFX: play sound #%d (%s)\n", soundID, soundEntry.unk0 & 0x8000 ? "MP3" : "WAV");
    recomp_intercept_soundIDs(soundID, &soundDef, &bank);

    //Bail if sound's clipID is 0
    if (!(soundDef.bankAndClipID & 0x7FFF)) {
        return 0;
    }

    if (soundHandle != NULL) {
        handle = *soundHandle;
    } else {
        handle = 0;
    }

    handle = amsfx_make_handle(handle, filename, lineNo);
    sSndSlots[handle].soundID = soundID;
    sSndSlots[handle].source = obj;
    sSndSlots[handle].distVolume = MAX_VOLUME;

    if ((obj != NULL) && (soundDef.volumeFalloff & 3)) {
        amsfx_func_2240(obj, &x, &y, &z, &yaw);
        amsfx_func_22FC(x, y, z, &soundDef, &volumeCalc);
        sSndSlots[handle].distVolume = volumeCalc;
    }

    sSndSlots[handle].volume = (volume * soundDef.volume) >> 7;
    volumeCalc = (sSndSlots[handle].volume * sSndSlots[handle].distVolume) >> 7;
    sSndSlots[handle].unk13 = volumeCalc;

    if (soundDef.bankAndClipID & IS_MP3) {
        sSndSlots[handle].sndpHandle = (sndstate* )-2;
        // @fake
        if (sSndSlots[handle].def.bankAndClipID) {}
        mpeg_play((soundDef.bankAndClipID & 0x7FFF) - 1);
        mp3_set_volume(volumeCalc << 8, 0);
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

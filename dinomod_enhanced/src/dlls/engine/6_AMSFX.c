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

typedef struct UnkBss4 {
    SoundDef def;
    s16 soundID;
    u16 pad10;
    u8 unk12;
    u8 unk13;
    s8 unk14;
    s8 unk15;
    s8 unk16;
    s8 unk17; //volume?
    Object *source; //object playing the sound
    sndstate *unk1C;
} UnkBss4;

typedef struct UnkBss28 {
    Vec3f pos;
    u16 unkC;
    u16 unkE;
} UnkBss28;

extern ALBankFile *_bss_0;
extern UnkBss4 *_bss_4;
extern s32 _bss_8;
extern u32 _bss_C;
extern u32 _bss_10;
extern u32 _bss_14;
extern u32 _bss_18;
extern u8 _bss_1C;
extern u8 _bss_1D;
extern u8 _bss_1E;
extern u8 _bss_1F; // sound volume
extern u8 _bss_20[0x8]; // sound volume(s?)
extern UnkBss28 _bss_28[0x10];
extern ACache *_bss_128;

extern void dll_6_func_860(u32, u8);
extern void dll_6_func_A1C(u32);
extern s32 dll_6_func_DE8(u16, SoundDef *);
extern s32 dll_6_func_1C38(void);
extern s32 dll_6_func_1D58(s32, char *, s32);
extern s32 dll_6_func_1E64(u32);
extern void dll_6_func_1F78(void);
extern void dll_6_func_2240(Object*, f32*, f32*, f32*, u16*);
extern void dll_6_func_22FC(f32, f32, f32, SoundDef*, s8*);
extern void dll_6_func_2438(f32 arg0, f32 arg1, s32 arg2, u8* arg3, u8* arg4);

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

RECOMP_PATCH u32 dll_6_play_sound(Object* obj, u16 soundID, u8 volume, u32* soundHandle, char *arg4, s32 arg5, char *arg6) {
    SoundDef soundDef;
    f32 x;
    f32 y;
    f32 z;
    u32 activeSoundIndex;
    u16 yaw;
    s8 volumeCalc;

    _bss_4->unk12 = 0;
    _bss_4->unk1C = NULL;
    
    //Bail if soundID is 0
    if (!soundID) {
        return 0;
    }

    //Get sound definition from AUDIO.bin subfile 0
    dll_6_func_DE8(soundID, &soundDef);

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
        activeSoundIndex = soundHandle[0];
    } else {
        activeSoundIndex = 0;
    }

    activeSoundIndex = dll_6_func_1D58(activeSoundIndex, arg4, arg5);
    _bss_4[activeSoundIndex].soundID = soundID;
    _bss_4[activeSoundIndex].source = obj;
    _bss_4[activeSoundIndex].unk17 = MAX_VOLUME;

    if ((obj != NULL) && (soundDef.volumeFalloff & 3)) {
        dll_6_func_2240(obj, &x, &y, &z, &yaw);
        dll_6_func_22FC(x, y, z, &soundDef, &volumeCalc);
        _bss_4[activeSoundIndex].unk17 = volumeCalc;
    }

    _bss_4[activeSoundIndex].unk16 = (volume * soundDef.volume) >> 7;
    volumeCalc = (_bss_4[activeSoundIndex].unk16 * _bss_4[activeSoundIndex].unk17) >> 7;
    _bss_4[activeSoundIndex].unk13 = volumeCalc;

    if (soundDef.bankAndClipID & IS_MP3) {
        _bss_4[activeSoundIndex].unk1C = (sndstate* )-2;
        // @fake
        if (_bss_4[activeSoundIndex].def.bankAndClipID) {}
        mpeg_play((soundDef.bankAndClipID & 0x7FFF) - 1);
        mp3_set_volume(volumeCalc << 8, 0);
        // @fake
        if (_bss_4) {}
    } else {
        // @recomp: Support multiple banks
        some_sound_func(bank, soundDef.bankAndClipID, (volumeCalc << 8), PAN_CENTRE, soundDef.pitch / 100.0f, (s32)(f32)soundDef.unk6, 1U, &_bss_4[activeSoundIndex].unk1C);
    }

    bcopy(&soundDef, &_bss_4[activeSoundIndex], sizeof(SoundDef));
    bcopy(&_bss_4[activeSoundIndex], _bss_4, sizeof(UnkBss4));

    if (soundHandle != NULL) {
        soundHandle[0] = activeSoundIndex;
    } else {
        _bss_4[activeSoundIndex].unk12 |= 0x80;
    }
    return activeSoundIndex;
}

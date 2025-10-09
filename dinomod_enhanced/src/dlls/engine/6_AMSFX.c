#include "PR/os.h"
#include "PR/ultratypes.h"
#include "modding.h"
#include "recomputils.h"
#include "recompconfig.h"

#include "libnaudio/n_libaudio.h"
#include "libnaudio/n_sndplayer.h"
#include "libnaudio/mp3/segment_67F50.h"
#include "sys/acache.h"
#include "sys/fs.h"
#include "sys/map.h"
#include "mpeg_fs.h"
#include "sys/asset_thread.h"
#include "sys/dll.h"
#include "sys/fs.h"
#include "sys/memory.h"
#include "dll.h"
#include "functions.h"
#include "types.h"

#include "recomp/dlls/engine/6_AMSFX_recomp.h"

typedef struct UnkBss4 {
    u16 pad0;
    u8 unk2;
    u8 unk3;
    u8 unk4;
    u8 unk5;
    u8 pad6;
    u8 unk7;
    u8 pad8[0xE - 0x8];
    s16 unkE;
    u16 pad10;
    u8 unk12;
    u8 unk13;
    s8 unk14;
    s8 unk15;
    s8 unk16;
    s8 unk17;
    Object *unk18;
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
extern s32 dll_6_func_DE8(u16, UnkDE8 *);
extern s32 dll_6_func_1C38(void);
extern s32 dll_6_func_1D58(s32, char *, s32);
extern s32 dll_6_func_1E64(u32);
extern void dll_6_func_1F78(void);
extern void dll_6_func_2240(Object*, f32*, f32*, f32*, u16*);
extern void dll_6_func_22FC(f32, f32, f32, UnkDE8*, s8*);
extern void dll_6_func_2438(f32 arg0, f32 arg1, s32 arg2, u8* arg3, u8* arg4);

enum RecompSoundIDs {
    SOUND_1865_Garunda_Te_If_you_bring_me_12_FrostWeeds = 1865
};

enum SoundTypes {
    WAV = 0,
    MP3 = 1
};
enum SoundFlags {
    SOUND_IS_MP3 = 0x8000
};

#define SOUND(fileID, bankID) (fileID + 1) | (bankID << 15)

typedef struct {
/*0*/    u16 clipID;
/*2*/    u8 volume;
/*3*/    u8 unk3;
/*4*/    u8 rate;
/*5*/    u8 flags;
/*6*/    u8 unk6;
/*7*/    u8 falloff;
/*8*/    u16 unk8; //falloff-related?
/*A*/    u16 falloff_maxDistance;
/*C*/    u16 falloff_otherDistance;
} SoundEntry; //file0 in AUDIO.bin

/** Changes it so Garunda Te doesn't say "12" FrostWeeds if the total needed has changed */
static void recomp_sound_remap_garunda_te_frostweeds(u16 soundID, SoundEntry* soundEntry){
    s32 frostWeedsMax = recomp_get_config_u32("garunda_te_frostweeds_override");

    //Don't replace the MP3 if the FrostWeed goal hasn't changed
    if (frostWeedsMax == 12){
        return;
    }

    soundEntry->clipID = SOUND(1033, MP3);
}

static void recomp_intercept_soundIDs(u16 soundID, SoundEntry* soundEntry) {
    switch (soundID){
        case SOUND_1865_Garunda_Te_If_you_bring_me_12_FrostWeeds:
            recomp_sound_remap_garunda_te_frostweeds(soundID, soundEntry);
            break;
    }
}

RECOMP_PATCH u32 dll_6_play_sound(Object* obj, u16 soundID, u8 volume, u32* arg3, char *arg4, s32 arg5, char *arg6) {
    UnkDE8 soundEntry;
    f32 sp54;
    f32 sp50;
    f32 sp4C;
    u32 temp_v0;
    u16 sp46;
    s8 sp45;
    
    _bss_4->unk12 = 0;
    _bss_4->unk1C = NULL;
    if (!soundID) {
        return 0;
    }
    dll_6_func_DE8(soundID, &soundEntry);

    //@recomp: intercept sound calls and edit as needed
    recomp_eprintf("AMSFX: play sound #%d (%s)\n", soundID, soundEntry.unk0 & 0x8000 ? "MP3" : "WAV");
    recomp_intercept_soundIDs(soundID, (SoundEntry*)&soundEntry);

    if (!(soundEntry.unk0 & 0x7FFF)) {
        return 0;
    }
    if (arg3 != NULL) {
        temp_v0 = arg3[0];
    } else {
        temp_v0 = 0;
    }
    temp_v0 = dll_6_func_1D58(temp_v0, arg4, arg5);
    _bss_4[temp_v0].unkE = soundID;
    _bss_4[temp_v0].unk18 = obj;
    _bss_4[temp_v0].unk17 = 0x7F;
    if ((obj != NULL) && (soundEntry.unk7 & 3)) {
        dll_6_func_2240(obj, &sp54, &sp50, &sp4C, &sp46);
        dll_6_func_22FC(sp54, sp50, sp4C, &soundEntry, &sp45);
        _bss_4[temp_v0].unk17 = sp45;
    }
    _bss_4[temp_v0].unk16 = (volume * soundEntry.unk2) >> 7;
    sp45 = (_bss_4[temp_v0].unk16 * _bss_4[temp_v0].unk17) >> 7;
    _bss_4[temp_v0].unk13 = sp45;
    if (soundEntry.unk0 & 0x8000) {
        _bss_4[temp_v0].unk1C = (sndstate* )-2;
        // @fake
        if (_bss_4[temp_v0].pad0) {}
        mpeg_fs_play((soundEntry.unk0 & 0x7FFF) - 1);
        func_80067650(sp45 << 8, 0);
        // @fake
        if (_bss_4) {}
    } else {
        some_sound_func(_bss_0->bankArray[0], soundEntry.unk0, (sp45 << 8), 0x40, soundEntry.unk4 / 100.0f, (s32)(f32)soundEntry.unk6, 1U, &_bss_4[temp_v0].unk1C);
    }
    bcopy(&soundEntry, &_bss_4[temp_v0], 0xE);
    bcopy(&_bss_4[temp_v0], _bss_4, sizeof(UnkBss4));
    if (arg3 != NULL) {
        arg3[0] = temp_v0;
    } else {
        _bss_4[temp_v0].unk12 |= 0x80;
    }
    return temp_v0;
}

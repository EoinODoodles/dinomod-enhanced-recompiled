#pragma once

#include "game/objects/object.h"
#include "game/objects/setups/18_StaticCamera.h"
#include "game/objects/setups/32_SideLoad.h"
#include "game/objects/setups/35_TexScroll.h"
#include "game/objects/setups/39_Warp.h"
#include "game/objects/setups/51_Generic.h"
#include "game/objects/setups/77_AlphaAnimator.h"
#include "game/objects/setups/96_SfxPlayer.h"
#include "game/objects/setups/103.h"
#include "game/objects/setups/105_Exploder.h"
#include "game/objects/setups/115_KyteTarget.h"
#include "game/objects/setups/127_WaterFallSpray.h"

typedef enum {
    HitAnimator_Mode_Invert = 1,  //Remove the target when gamebit set (instead of enabling it when gamebit set)
    HitAnimator_Mode_Update_Visibility = 2, //When targetting BLOCKS Shapes: show/hide the shapes as well as removing their collision
    HitAnimator_Mode_BLOCKS = 4,  //Targetting BLOCKS Shapes
    HitAnimator_Mode_HITS = 8     //Targetting HITS Lines
} HitAnimator_Modes;

#define HITANIMATOR_MODE_SHAPE_OFF hitAnimator_configureModeFlags(TRUE, TRUE, TRUE)  //Shape switched off when gamebit set
#define HITANIMATOR_MODE_SHAPE_ON hitAnimator_configureModeFlags(FALSE, TRUE, TRUE)  //Shape switched on when gamebit set
#define HITANIMATOR_MODE_HITS_OFF hitAnimator_configureModeFlags(TRUE, FALSE, FALSE) //Line switched off when gamebit set
#define HITANIMATOR_MODE_HITS_ON hitAnimator_configureModeFlags(FALSE, FALSE, FALSE) //Line switched on when gamebit set

typedef struct {
    ObjSetup base;
    s16 gamebitActivate;                    //Gamebit controlling when the fade is activated/deactivated
    s16 gamebitActivated;                   //(For modes 0, 2, 3) Optionally set a gamebit when the fade animation is complete - also used to restore fully-faded state
    u8 initialOpacity;                      //Opacity at the beginning of the fade animation (can be higher than the goal opacity)
    u8 goalOpacity;                         //Opacity at the end of the fade animation (can be lower than the initial opacity)
    s8 animatorID;                          //Vertices will only be affected if they have this shape animatorID
    s8 fadeSpeed;                           //Fade in rate. In radial mode, the sign inverts the behaviour (fade inside/outside radius)!
    union {
        u8 flags;                           //Mode stored on bits 0, 1 (see `GET_MODE` macro) - bit2 determines whether to play a sound, and (@recomp) bit3 determines whether to change render mode when fully opaque
        struct {
            u8 flagsUpper : 3;              //Unused
            u8 noSoundOnReverse : 1;        //@recomp: don't play sound when fade is being switched off (e.g. WC Sun Temple's illusory walls fading back in)
            u8 opaqueAtMaxOpacity : 1;      //@recomp: switch to a fully-opaque render mode when vertices are at max opacity (prevents other transparent shapes drawing through the animated shape)
            u8 playSound : 1;               //Play objSetup->soundID (@recomp: any upper bit used to enable this, though the sound-using objSetups only ever set bit2. The code's now been changed so it specifically checks bit2, freeing up the higher bits for custom flags!)
            u8 mode : 2;                    //See `AlphaAnimator_Modes`
        };
    };
    u8 removeCollisionWhenHidden;           //Boolean, fade affects whether the affected shapes are tangible
    u16 fadeRadiusGoal;                     //(For radial mode) End radius of the fade animation
    u16 soundID;                            //Which sound to play (requires sound playing enabled on `objData->flags`)
    u16 soundIDReverse;                     //@recomp: Which sound to play when reversing fade (requires sound playing enabled on `objData->flags`)
} AlphaAnimator_Setup_Dinomod;
_Static_assert(sizeof(AlphaAnimator_Setup_Dinomod) == sizeof(AlphaAnimator_Setup), "Custom AlphaAnimator setup size is incorrect.");

typedef enum {
    AlphaAnimator_MODE_0_Basic_Fade_and_Set_Gamebit, //Fade using a gamebit (and optionally set a secondary gamebit when the fade is complete)
    AlphaAnimator_MODE_1_Basic_Fade,                 //Fade using a gamebit 
    AlphaAnimator_MODE_2_Toggleable_Fade,            //Activate/deactivate a fade using a gamebit (and optionally set/unset a secondary gamebit when the fade is complete)
    AlphaAnimator_MODE_3_Radial_Falloff_Fade         //Using a gamebit, fade vertices inside an expanding radius (or outside, based on fadeSpeed's sign!)
} AlphaAnimator_Modes;

typedef enum {
    AlphaAnimator_FLAG_Play_Sound = 4
} AlphaAnimator_Flags;

typedef enum {
    AlphaAnimator_CUSTOMFLAG_Opaque_at_Max_Opacity = 8, //Change to fully-opaque render flags when at max opacity, to prevent other transparent surfaces being drawn behind the shape
    AlphaAnimator_CUSTOMFLAG_Mute_Sound_on_Reverse_Fade = 0x10 //For toggleable fade with sound enabled: don't play sound when fade is being switched off (e.g. WC Sun Temple's illusory walls fading back in)
} AlphaAnimator_CustomFlags;

typedef struct {
/*00*/ ObjSetup base;
/*18*/ s16 gamebitActivate;    //Gamebit to begin the animation
/*1A*/ s16 gamebitFinished;    //Gamebit to set once the animation's finished
/*1C*/ s16 initDisplacementX;  //Initial position
/*1E*/ s16 initDisplacementY;  //Initial position
/*20*/ s16 initDisplacementZ;  //Initial position
/*22*/ s16 endDisplacementX;   //Goal position
/*24*/ s16 endDisplacementY;   //Goal position
/*26*/ s16 endDisplacementZ;   //Goal position
/*28*/ s8 animatorID;          //Shape animator tag
/*29*/ s8 speedX;              //Interpolation speed (in tenths of a worldSpace unit)
/*2A*/ s8 speedY;              //Interpolation speed (in tenths of a worldSpace unit)
/*2B*/ s8 speedZ;              //Interpolation speed (in tenths of a worldSpace unit)
/*2C*/ u8 mode;                //`XYZAnimator_Modes`
/*2D*/ u8 options;             //@recomp: repurpose padding
/*2E*/ u16 goalRadius;         //For `XYZAnimator_MODE_3_Radial_Falloff_Set_Gamebit`
} XYZAnimator_Setup;

typedef enum {
    XYZAnimator_MODE_0_Oneshot_and_Set_Gamebit,     //Interpolate to the goal, and set gamebitFinished
    XYZAnimator_MODE_1_Oneshot,                     //Interpolate to the goal
    XYZAnimator_MODE_2_Reversible_Set_Gamebit,      //Interpolate to/from the goal, and set gamebitFinished
    XYZAnimator_MODE_3_Radial_Falloff_Set_Gamebit,  //Animate vertices within an expanding radius, and set gamebitFinished
    XYZAnimator_MODE_4_Oneshot_and_Set_Gamebit_No_Initial_Update //Interpolate to the goal, and set gamebitFinished (without updating the vertices during initialisation)
} XYZAnimator_Modes;

/* Custom options */
typedef enum {
    // Reapply animation when the Block reloads after previously being animated 
    // (used to fix a bug where Walled City's secret transporter chamber stays at its raised position on
    // subsequent Block loads, after the XYZAnimator animated it when the Block was previously loaded.)
    XYZAnimator_OPTION_Reanimate_on_Block_Reload = 1 
} XYZAnimator_Options;

typedef struct {
/*00*/ ObjSetup base;
/*18*/ s16 textureIndex;       //Primary material: index in TABLES.bin subfile #14 ("scroll table")
/*1A*/ s16 gamebitActivate;    //gamebitID that activates animation (or -1 to skip bit check)
/*1C*/ s8 uSpeedB;             //U scroll speed for secondary blend material 
/*1D*/ s8 vSpeedB;             //V scroll speed for secondary blend material 
/*1E*/ s8 uSpeedA;             //U scroll speed for primary material              
/*1F*/ s8 vSpeedA;             //V scroll speed for primary material              
/*20*/ s16 blendTextureIndex;  //Secondary blend material: index in TABLES.bin subfile #14 (optional, -1 if unused)
                               //Blend material is used for multitextured scrolling water, etc.
/* RECOMP */
/*22*/ u16 options;             //Repurposed padding (see `BlockTexScrollAdd_RecycleOptions`)
} TexScroll2_Setup_Dinomod;
_Static_assert(sizeof(TexScroll2_Setup_Dinomod) == sizeof(TexScroll_Setup), "Custom TexScroll setup size is incorrect.");

typedef struct {
/*00*/ ObjSetup base;
/*18*/ s16 gamebit;   // LOOPING SOUNDS: start/stop when value changes, or use distance check if gamebit unspecified.
                      // ONE-SHOT SOUNDS: play sound when value changes (if "Play At Random Inside Radius" mode isn't being used).
/*1A*/ s16 soundID;
/*1C*/ u8 flags;      //See `SfxPlayer_Flags`.
/*1D*/ u8 halfRadius; //Stored halved, the sound plays when the player is inside this radius. Looping sounds stop when the player is outside of (radius + 10).
/* RECOMP */
/*1E*/ u8 volume;       //Optional volume adjustment (MAX_VOLUME used if 0)
/*1F*/ s8 pitch;        //Optional pitch adjustment for looped sounds (ignored if 0) (useful for desyncing nearby sources using same sound)
/*20*/ u8 fadeDuration; //Duration for fading looped sounds
} SfxPlayer_Setup_Dinomod;
// TODO: custom SfxPlayer setup struct is too big!
//_Static_assert(sizeof(SfxPlayer_Setup_Dinomod) == sizeof(SfxPlayer_Setup), "Custom SfxPlayer setup size is incorrect.");

typedef enum {
    SfxPlayer_FLAG_1_Looping_Sound = 1,                //Use this if the sound loops (plays with a soundHandle, and stops it when sound finished) [Has priority over "play at random inside radius" mode]
    SfxPlayer_FLAG_2_Play_if_Gamebit_Set = 2,
    SfxPlayer_FLAG_4_Play_if_Gamebit_Unset = 4,
    SfxPlayer_FLAG_8_Play_At_Random_Inside_Radius = 8, //Sound plays at random intervals when player is inside radius [Has priority over one-shot sound "play by gamebit" modes]
    SfxPlayer_CUSTOMFLAG_10_Fade = 0x10,               //Fade looped sounds in/out, using a variable fadeDuration
    SfxPlayer_CUSTOMFLAG_20_Double_Radius = 0x20,      //Double activation distance
    SfxPlayer_CUSTOMFLAG_40_Play_if_Gamebit_Set_on_Setup = 0x40 //Start out playing a looping sound if its gamebit is already set during setup
} SfxPlayer_Flags;

typedef enum  {
    USEOBJ_HideIfAlreadyUsed = 0x1,
    USEOBJ_SeqControlsUsedBit = 0x4,
    USEOBJ_DisableAfterUse = 0x8,
    USEOBJ_NoTargetingWhenDisabled = 0x10,
    USEOBJ_ReplayIncludeActor2 = 0x20,
    USEOBJ_ReplayIncludeActor3 = 0x40,
    USEOBJ_ReplayIncludeActor4 = 0x80
} UseObjFlags;

typedef enum {
    WCUseObj_FLAG_1_Hide_On_Revisit_When_Used = 1,
    WCUseObj_CUSTOMFLAG_2_Invert_GamebitUnlocked = 2, //@recomp: repurpose unused flag: invert gamebitUnlocked behaviour (lock when gamebit set, instead of unlock when gamebit set)
    WCUseObj_FLAG_4_ObjSeq_Sets_Gamebit = 4,
    WCUseObj_FLAG_8_Lock_After_Use = 8,
    WCUseObj_FLAG_10_No_Targetting_When_Locked = 0x10,
    WCUseObj_FLAG_20_PreemptActors1and2 = 0x20,
    WCUseObj_FLAG_40_PreemptActors1and2 = 0x40,
    WCUseObj_FLAG_80_PreemptActors1and3 = 0x80
} WCUseObj_Flags;

typedef enum {
    EffectBox_EFFECT_Fall_Reset = 0,        //Puts camera underneath you and causes fall reset
    EffectBox_EFFECT_WMInsert_Unk = 4,      //? (used in the unused WMInsert map)
    EffectBox_EFFECT_Gravity_Unk = 8,       //? (something to do with player gravity?)
    EffectBox_EFFECT_Stealth_Region = 12    //Robots can't see you when inside! Used a lot in CRF
} EffectBox_Effects;

typedef enum {
    EffectBox_TARGET_Player = 0,
    EffectBox_TARGET_Sidekick = 1,
    EffectBox_TARGET_Pushblock = 2
} EffectBox_Targets;

typedef struct {
    ObjSetup base;
    s16 gamebitUnlocked;    //GamebitID determining whether the sidekick is unlocked (i.e. Kyte rescued)
    u8 sidekickIndex;       //Whether this should load Tricky/Kyte (see `SideLoad_Indices`)
    /* RECOMP */
    u8 options;             //`SideLoad_Options`
} SideLoad_Setup_Dinomod;
_Static_assert(sizeof(SideLoad_Setup_Dinomod) == sizeof(SideLoad_Setup), "Custom SideLoad setup size is incorrect.");

typedef enum {
    SideLoad_OPTION_Repeatable = 1 //If the sidekick unloads after being loaded by this SideLoad object, the SideLoad can reload them
} SideLoad_Options;

typedef struct {
/*00*/ ObjSetup base;
/*18*/ s16 gamebit;     //Spray stops when this gamebit is set (when specified)
/*1A*/ s8 roll;
/*1B*/ s8 pitch;
/*1C*/ s8 yaw;
/*1D*/ u8 amplitudeX;
/*1E*/ u8 amplitudeZ;
/*1F*/ u8 amplitudeY;
/*20*/ u8 distance;
/*21*/ u8 unk21;
/*22*/ u8 unk22;
/*23*/ u8 flags;
/*24*/ u8 iterations;
/*RECOMP*/
// Note: This addition is OK because this setup has a size of 0x28 in the map files.
/*25*/ u8 invertGamebit; //When set: spray plays when gamebit set (if a gamebit was specified)
} WaterFallSpray_Setup_Dinomod;
_Static_assert(sizeof(WaterFallSpray_Setup_Dinomod) == sizeof(WaterFallSpray_Setup), "Custom WaterFallSpray setup size is incorrect.");

typedef struct {
/*00*/ ObjSetup base;
/*18*/ u8 yaw;
/*19*/ s8 objectSeqIndexDepart; //departure sequence, using custom mode 5 (@recomp: repurpose unused field)
/*1A*/ s8 warpID;
/*1B*/ s8 objectSeqIndex;    //arrival sequence for inbound warp
/*1C*/ s8 zShiftArrival : 3;        //@recomp: custom mode 5, optional z-shift for arrival sequence (stored at 16th size)
/*1C*/ s8 zShiftDeparture : 3;      //@recomp: custom mode 5, optional z-shift for departure sequence (stored at 16th size)
/*1C*/ u8 noRotateOnDeparture : 1;  //@recomp: custom mode 5, optionally skip rotating by 180 for departure sequence
/*1C*/ u8 isInboundWarp : 1;
/*1D*/ s8 mode;
/*1E*/ s8 quarterRange;
/*20*/ s16 gamebit;
/*22*/ s16 gamebitDepart;    //departure sequence, using custom mode 5 (@recomp: repurpose padding as new field)
} Warp_Setup_Dinomod;
_Static_assert(sizeof(Warp_Setup_Dinomod) == sizeof(Warp_Setup), "Custom Warp setup size is incorrect.");

typedef enum {
    WarpPoint_MODE_0, //Play an arrival objSeq (fade in); warp when nearby (fade out) (ignores delay timer)
    WarpPoint_MODE_1, //Play arrival objSeq1 (uncurling from ball) (fade in); warp via objSeq0 (no fade out) (use delay timer)
    WarpPoint_MODE_2, //Play an arrival objSeq if a gamebit it set (no fade in); warp and unset gamebit if gamebit set (no fade out) (checks delay timer, but ignores it in practice by setting it to 0)
    WarpPoint_MODE_3, //(Inbound only) Play an arrival objSeq if a gamebit is set (no fade in) and unset the gamebit; no warp away
    WarpPoint_MODE_4  //Play an arrival objSeq if a warpID is specified (fade in); warp if a gamebit is set, unsetting gamebit (fade out) (uses delay timer)
} WarpPoint_Modes;

typedef enum {
    WarpPoint_CUSTOMMODE_5 = 5, //Play an arrival objSeq if gamebitA is set (no fade); play a departure objSeq and unset gamebitB if gamebitB is set (no fade) (ignores delay timer)
} WarpPoint_CustomModes;

#define NO_WARP_ID -1

typedef struct {
    ObjSetup base;
    u8 cameraID;
    u8 speedFactor; //@recomp
    u8 fov;
    u8 flags;
    s16 yaw;
    s16 pitch;
    s16 roll;
} StaticCamera_Setup_Dinomod;
_Static_assert(sizeof(StaticCamera_Setup_Dinomod) == sizeof(StaticCamera_Setup), "Custom StaticCamera setup size is incorrect.");

#define CamStatic_FLAG_Aim_at_Player_Centre 0x10
#define CamStatic_LOOK_AT (CamStatic_FLAG_Aim_Yaw_at_Player | CamStatic_FLAG_Aim_Pitch_at_Player | CamStatic_FLAG_Aim_at_Player_Centre)

//For use in Trigger Object CameraAction commands
#define CamStatic_PreviousCameraEasesIn 0x80

typedef enum {
    Switch_FLAG_Can_Be_Toggled_Via_Attacks = 1, //Repeated Projectile Spell hits will toggle the switch's state
    Switch_FLAG_Resets_After_Delay = 2          //Switch resets resets to its unpressed state after a number of seconds (specified by `objData->resetDelay`)
} ProjectileSwitch_Flags;

#define CHECK_IF_GAMEBIT_UNSET 0x4000 //For Trigger_Setups' conditionBitFlagIDs

typedef struct {
    ObjSetup base;
    s16 _unused18;
    u8 yaw;
    s16 _unused1C;
    s16 gamebitDisable; //@recomp: repurpose padding, barrel disappears after this gamebit is set
} DFBarrel_Setup_Dinomod;
_Static_assert(sizeof(DFBarrel_Setup_Dinomod) == sizeof(Ctrl103_Setup), "Custom DFBarrel setup size is incorrect.");

typedef struct {
    ObjSetup base;
    u8 pieceCount;
    s16 yaw;
    s16 pitch;
    s16 roll;
    Vec3s16 displacementOrigin;
    Vec3s16 _unk26;
    s16 explosionPower;
    s16 floorOffset;
    s16 acceleration;
    Vec3s16 _unk32;
    u16 lifetimeMax;
    s16 _unk3A;
    s16 _unk3C;
    s16 gamebitFinished;
    s16 gamebitExplode;
    u8 options; //@recomp: repurpose padding
    u8 _unk43[0x48 - 0x43];
} CFExplodeWall_CustomSetup;
_Static_assert(sizeof(CFExplodeWall_CustomSetup) == sizeof(Exploder_Setup), "Custom CFExplodeWall setup size is incorrect.");

typedef enum {
    CFExplodeWall_CUSTOMOPTION_Use_Default_Explosion_Sound = 1,
    CFExplodeWall_CUSTOMOPTION_Camera_Shake = 2,
    CFExplodeWall_CUSTOMOPTION_Piece_Plays_Impact_Sound = 4,
    CFExplodeWall_CUSTOMOPTION_No_Falloff_On_Piece_Sounds = 8
} CFExplodeWall_CustomOptions;

typedef struct {
    ObjSetup base;
    u8 roll;
    u8 pitch;
    u8 yaw;
    u8 useOtherLFXConfig;  //Boolean, affects which lightAction indices are used
    f32 scale;
    u8 unk20;
    u8 unk21;
    u8 unk22;
    u8 flags;              //See `DF_Lantern_Flags`, used so indoor lanterns are always lit (instead of just at night), or to use fadeDistance for camera range
} DF_Lantern_Setup;

typedef enum {
    DF_Lantern_FLAG_1_Player_Collision = 1,     //Toggle objHits during setup (doesn't seem to work, but probably meant to decide whether the lantern burns you?)
    DF_Lantern_FLAG_2_Emit_Light = 2,           //Apply a lightAction
    DF_Lantern_FLAG_4_Show_PartFX_Glow = 4,     //Draw the partFX
    DF_Lantern_FLAG_8_Play_Sound = 8,           //Play a crackling sound loop when near the lantern
    DF_Lantern_FLAG_10_Always_Lit = 0x10,       //For indoor lanterns - always lit, instead of only at night-time
    DF_Lantern_FLAG_20_Use_Fade_Distance = 0x20, //Use the objSetup fadeDistance as the camera range for the partFX glow
    /* RECOMP */
    DF_Lantern_FLAG_40_Quadruple_Fade_Distance = 0x40 //Allows longer camera ranges (> 0xFF) to be specified for the partFX glow
} DF_Lantern_Flags;

typedef struct {
/*00*/ ObjSetup base;
/*18*/ u8 findRange;
/*19*/ u8 timerSeconds;
/*1A*/ u16 kyteFlightCurve;
/*1C*/ u8 kyteTalkSeq;
/*1D*/ u8 unk1D;
union {
    struct {
    /*1E*/ u8 flagsUpper : 7;
    /*1E*/ u8 checkDistance2D : 1;
    }; 
    /*1E*/ u8 flags; //@recomp: use as a general flags field instead of just a Boolean
};
} FindKyteObject_Setup_Dinomod;
_Static_assert(sizeof(FindKyteObject_Setup_Dinomod) == sizeof(KyteTarget_Setup), "Custom FindKyteObject setup size is incorrect.");

typedef enum {
    FindKyteObject_FLAG_1_2D_Distance_Check = 1,
    /* RECOMP*/
    FindKyteObject_FLAG_2_Set_Gamebit_After_Timeout = 2,      //Gamebit set automatically after `timerSeconds`, to avoid Kyte taking too long/failing to reach the Find target
    FindKyteObject_FLAG_4_Start_Timeout_When_Kyte_is_Nearby = 4, //Timeout countdown doesn't start until Kyte's within 4x the Find command's range
} FindKyteObject_Flags;

/* RECOMP - edited version of struct */
typedef struct {
/*00*/ ObjSetup base;
/*19*/ s8 yaw;
/*19*/ s8 modelIndex;
/*1A*/ u8 _unk1A[0x1C - 0x1A];
/*1C*/ s16 gamebitSlab;     //@recomp: repurpose unused field
/*1E*/ s16 gamebitLit;
/*20*/ s16 gamebitRise;
/*22*/ s16 gamebitSwitch;   //@recomp: repurpose unused field
} WCBeacon_Setup_Dinomod;
_Static_assert(sizeof(WCBeacon_Setup_Dinomod) == sizeof(WCBeacon_Setup), "Custom FindKyteObject setup size is incorrect.");

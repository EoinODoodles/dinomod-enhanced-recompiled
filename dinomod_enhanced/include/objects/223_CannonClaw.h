#pragma once

typedef enum {
    CannonClaw_FLAG_1_Cannon_Fired = 1,
    CannonClaw_FLAG_2_Hit_Player_or_Sidekick = 2,
    CannonClaw_FLAG_4_Entered_Silo = 4,
    CannonClaw_FLAG_8_Exited_Silo = 8,
    CannonClaw_FLAG_10_Distracted = 0x10,
} CannonClaw_Flags; //@recomp: custom flags for CannonClaw's reactions to various events

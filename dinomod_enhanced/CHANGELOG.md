## v1.0.0
> Note: 1.0 marks full parity with the original ROM patch but is **not** the end of Dinomod development!

### Warlock Mountain
- Fixed issue where the act 1 music would not play if the Galleon Arrival sequences were skipped.

### Ice Mountain
- The player's SnowBike no longer gets flung at the start of the race.

### SnowHorn Wastes
- Fixed an issue where Tricky's sidekick command tutorial UI could play out incorrectly.
- Fixed issues with skipping Tricky's sidekick command tutorial sequence.
- Added mod option to restore the SnowHorn Wastes music's harp trill.

### SwapStone Hollow
- Sabre no longer attacks when trying to talk to ThornTails.
- The Queen EarthWalker will now keep her sitting animation after being healed.
- Tricky can now follow you through the connection between SwapStone Hollow and Walled City.
- Fixed issue where Tricky could fall into the well area when approaching from SnowHorn Wastes.

### Discovery Falls
- TODO: a lot

### DarkIce Mines
- Player control is now disabled during the Waterfall Cave iceberg lever cutscene.

### Moon Mountain Pass
- Added fall reset trigger to the pit next to the climb up from Discovery Falls.
- Added a savepoint right after the climb up from Discovery Falls.

### CloudRunner Fortress

#### CRF: Entrance
- The entrance music is now stopped when going back to Moon Mountain Pass.
- Fixed the size of a trigger in the entrance area that could be accidentally walked around.
- Fixed Kyte's flight group toggle in the entrance area so she can fly up the stairwell.
- Fixed issue where one of the stealth regions in the entrance stairwell would not be loaded when Krystal was standing in it.

#### CRF: Dungeon
- Fixed many softlocks, bugs, and jank involving the guardian.
- Added a magic plant to the dungeon to prevent a softlock.

#### CRF: Courtyard
- Restored courtyard robot patrols.
- The guardian is now correctly present in the courtyard water draining cutscene.
- The baby CloudRunner in the courtyard now loads in from farther away.
- Fixed issue where the map layer could go out of sync when entering/exiting the tunnel in the courtyard pool with some camera angles.
- Re-entering CloudRunner Fortress after the cutscene with Scales and the Queen no longer results in broken cutscene objects appearing in the courtyard.

#### CRF: Power room
- The power room door now unlocks when Krystal physically places the key rather than instantly opening when the lock is interacted with.
- Fixed cutscene jank with the power room crystal holders.
- The savepoint location set after completing the power room is now correctly located in the power room and not the middle of the dungeon windlift.

#### CRF: Dock
- Fixed many softlocks related to barrels.
- Barrels are now dropped when entering windlifts, freefall, or ledge grabs.
- Barrels can now be safely left inside of a windlift while the player is away.
- Fixed crash that would occur if a carried barrel's originating map was unloaded.
- Barrels can no longer be picked up while the player is floating in a windlift.
- The high-detail version of the Galleon is now always shown (recomp-specific).
- Fixed issue where Krystal would be pushed forward a little every time the throne room switches loaded in after being pulled.
- Hitline adjustments in the treasure windlift to prevent floating barrels from going out of bounds.
- Centered the treasure windlift.
- Treasure room doors: Removed debug code that could result in a softlock if accidentally triggered.

#### CRF: Throne room
- Rock falling SFX under the throne now stops after blowing up the floor.
- Adjusted rock fall particle FX under the throne to better represent the floor's damage state.
- The "slide door" to the throne room no longer awkwardly despawns after walking by it.
- Added a new savepoint that triggers after draining the courtyard water.
- Increased the load distance of the throne room SharpClaw.
- Fixed issue where Kyte would get stuck in the throne room forcefield's curve network if the game was reloaded after rescuing her.
- Fixed issue where some baby CloudRunners wouldn't show up in the throne room after they were rescued.

#### CRF: Treasure rooms
- Treasure tractor-beam robo: The treasure chest UI element now fades in/out and is delayed until the second half of the intro cutscene.
- Fixed uncommon crash that could occur when failing the treasure robot minigame.
- The pit in the treasure room tunnels no longer has an invisible floor across the top of it (you can now fall in).
- The treasure room GuardClaw no longer starts the GuardClaw music.
- Fixed issue where a treasure room trigger could be hit through a wall before the treasure room doors were unlocked.

#### CRF: Galleon
- The GuardClaw music now stops when leaving the Galleon.
- Fixed environment FX jank when leaving the Galleon interior.
- Fixed Kyte flight group transitions inside the Galleon so she isn't stuck in the entrance.

#### CRF: Racetrack
- The duster at the top of the ladder to the racetrack is now accessible.
- Fixed issue where the SpellStone race would be broken if it was reloaded after being started.
- Attempting to leave the racetrack area after starting the race will now count as a loss.
- Fixed issue where the guardian would try to talk to you from very far away in the racetrack area.

#### CRF: Misc
- Baby CloudRunners: Softlock and animation fixes.
- The Z-lock icon above the baby CloudRunners is now grey when you are too far away.
- "Inside lighting" is now toggled correctly in several locations.
- Fixed some minor guardian talk sequence jank.

### Walled City
- TODO: a lot

### BlackWater Canyon
- Added rustyspottedcat's custom textures for the ShadowHunter!

### Dragon Rock
- Fixed the particle FX and SFX emitters inside of the pressure pad dig spots around The Rock. 

### Baddies
- CannonClaw: Now plays voice lines on various events, has hit particles and animation when defeated, fixed sidekick related crash.
- SnowWorm: Sound timing fixes.
- DBStealerWorm: Sound timing fixes.
- VampireBat: Can now be defeated!

### UI
- Game Select: Fixed bug where task text could overlap when the text wrapped onto three lines.
- Name Entry: Pressing Start will now focus the end button.
- Options: Fixed some recomp specific crashes.
- Fixed the "old menus" and made them accessible for fun.
- Added mod option for displaying timer decimal places.
- Fixed game state corruption that could occur when pressing "Continue" on the Game Over screen.
- Placing apples, fish, and beans from the foodbag will now spawn the correct pickup object.
- Black screen fades now cover cmdmenu UI elements that are placed in letterboxed regions.

### Misc
- A secret unlock after the credits!
- Using the Ice Blast spell on a SharpClaw no longer deals damage directly but attacking a frozen SharpClaw is now lethal.
- Sabre seq joint fixes (fixes some cutscene jank in Galadon cutscenes).
- Restored Krystal's uphill/downhill walk animation blend.
- Fixed issue where the player's animation could get stuck when starting a talk sequence. Also added an animation transition to idle during talk sequence setup.
- MP3 playback is now paused while the game is paused.
- DIMCannon: Added sound effects, barrel shake while charging, misc fixes.
- MagicDust: Fixed issue where stationary gems would rapidly play sounds while touched.
- Pollen: Fixed crash that could occur if the pollen cannon despawned before the pollen projectile.
- Fixed uncommon divide by zero crash that could occur in the day/night cycle code during a warp.
- Fixed issue where the day/night cycle could get stuck in a really fast infinite loop.
- Restored night sky star animations.
- Fixed issue where magic would not be increased after the tutorial sequence for collecting magic dust for the first time.
- Fixed crash that would occur if a static camera unloaded while still in use.
- Climbing camera: Added smoothing when moving between differently-angled surfaces.
- When aiming a spell, the camera no longer clips through walls and the player will fade out if the camera is close enough.
- Fixed rare crash that could happen when a SharpClaw respawned.
- Timesaves and "setup moves" are now bound to savepoints rather than always being saved. This prevents many softlocks that could occur by simply saving the game at the wrong time.
- Fixed issue where saving and quitting on the Game Over screen would result in the player starting with zero health the next time that save was loaded.
- Fixed issue where loading a savepoint or restartpoint could result in game state corruption.
- Fixed minor save file corruption that could occur when the game tried to save certain "track light" actions.
- Fixed rare newlfx model light related crash.
- Fixed stale shadow display list pointer issue, fixing two notable cutscenes (and possibly more):
    - Chief LightFoot's "tooth" getting stretched down to his foot during the SC beacon intro cutscene.
    - An orange MagicPlant fx getting attached to Tricky's face briefly during the SnowHorn Wastes hotspring cutscene.
- Fixed issue where Tricky could get stuck forever if he accidentally navigated outside of his current walk group region.
- Patched many "walk group" regions throughout the game to prevent Tricky from getting stuck in many places.
- Fixed issue where savepoint triggers would not save the savepoint's yaw correctly.
- Gametext tweaks/fixes.
- Added a work-in-progress chapter select menu to the debug UI (recomp-specific).
- Removed the 60 Hz mode option (this option is too problematic and needs many more patches to be usable).

### ROM patch ports
- Fixed issue where SharpClaw racers in the CloudRunner race could get stuck in a wall.
- Fixed some issues with auto-assigned newlfx slot selection.
- Fixed memory leak in RobotAnimPatrol.
- Fixed potential buffer overrun when Tricky spawns.
- Fixed crash that could occur when Tricky jumps off ledges.
- Ported "crawler" (Skeetla) related fixes.
- Fixed some cases where the camera could be clipped through a wall.

## v0.9.3
- New mod option to make spell aiming not be fixed in place while firing the Projectile or Grenade spell.
- Fixed SwapStone Hollow river log not appearing after activating the Dragon Rock SpellStone.
- The invisible wall behind the blue SnowHorn now correctly disappears when it's time to enter the NW shrine.
- Fixed issue where the barrel by the rock in Dragon Rock would unload too early and not keep one of the pressure pads down.
- Fixed softlock where the Test of Knowledge pickups would no longer spawn after failing the test.
- Fixed crash that occurred if the player was holding an item when the Test of Knowledge timer ran out.
- The player is now correctly warped out of the Test of Knowledge after completing the test.
- The player now has their magic refilled when entering the Test of Magic and when starting the test.
- Fixed issue where the Test of Sacrifice would instantly fail if the player had previously failed the test and returned to retry it.
- MMP, WC, CC, WG, and NW shrine transporters now work correctly (the player can leave the shrine before starting the test if needed).
- CC, WG, and GP shrine doors are now correctly locked while the test is active.
- GP and WG shrines now set the correct spirit bits.
- NW shrine now actually gives a spirit.
- Fixed `WL_seqpoint` anim callback message loop.

## v0.9.2
- Fixed an issue where the Garunda Te SpellStone guardian cutscene could repeat the first segment a couple times.
- Fixed an issue where NPC head lookat would be reversed after visiting the shop.
- Fixed a rare case where the Projectile Spell would not be unequipped if a Z-target was broken in a specific way.
- Fixed issue where already completed tasks could re-enter the recently completed task list.
- `ScorpionRobot`:
    - Now actually fires a laser when attacking.
    - Correctly enters damage recoil state when attacked.
    - No longer deals damage on touch while standing still or dead.
    - Stops targeting the player if they are dead.
    - VFP entrance robos no longer get flung into a wall by the nearby crates.
    - No longer ignores terrain (e.g. the DFP ramp).
    - Animation jank fixes (spinning (partial fix), turning).
    - Fixed case where you could Z-target a defeated robo.
    - Added some explosion particles on death (similar to big robo).
- `BigScorpionRobot`:
    - Fixed rotation desync.
    - Fixed jank with repeated turning animations.
    - Stops targeting the player if they are dead.

## v0.9.1
- Fixed various issues with holding Z while exiting Z-targeting.
- Aimed spells can no longer be selected while in first person (fixes a camera issue).

## v0.9.0

### New
- The SwapStone Hollow river is now correctly drained at the start of the game (talk to the Thorntail for advice!).
- The Projectile Spell can now be used while Z-targeting.
- Collectable pickup now shows the item in a little pop-up.
- Restored SwapStone Circle beacon/tumbleweed gameplay.
- Restored water "movement" ripple fx (a water effect used by some objects that is disabled in the December 2000 build).
- Walled City T. rexes now make use of their unused attack animations.
- New mod options:
    - Play as Fox option.
    - New accessibility options for button-mashing gameplay.
    - Optional alternative command menu controls (scrolling up, D-pad support).
    - Optional new UI icons for certain items, sourced or adapted from Kiosk leftovers.
    - Magic Gems: option to use a leftover fancier bounce calculation.
    - Option to restore some mushroom animations, states, and models.
    - Other miscellaneous options: check out the 'Configure' page to see the new list!

### ROM patch ports
- The game now boots to the Rolling Demo/main menu (many thanks to nuggs!).
- Restored older item collection jingle.
- Fixed sidekick hunger meters.
- Weapons now ignore intangible terrain like light beams.
- Cape Claw Sand Worm Boss crash fix.
- Dragon Rock pushcart fixes.
- CClightfoot model swap (show normal LightFoot instead of chief).
- DIM2/Galadon music action fixes.
- Skeleton objhit crash fix.
- Fixed rare Walled City fog glitch.
- `DR_NiceSharpy` cutscene start time fix.

### Changes
- Desert Force Point lift now uses its unused sequences to transport the player.
- Subtitles now pause while gameplay is paused.
- Sabre's intro and Tricky's tutorial cutscenes can now be skipped.
- Log roll action is now disabled by default (can be re-enabled in mod options).
- The log now behaves more like how it was intended for Cape Claw and Discovery Falls.
- Mount/dismount animations for the log now use reasonable animations used by other vehicles rather than completely unrelated animations (actual anims for the log don't exist). 
- Dragon Rock EarthWarrior call pad is now disabled before saving the EarthWarrior.
- Immediate animation switching when stowing weapons while walking.

### Bug fixes
- The Projectile Spell no longer has a chance to auto-activate after dismounting a vehicle.
- Ice Blast Spell fixes (debug cubes hidden, stops when out of magic).
- Tricky ball fixes (thrown from hand, floats on water).
- Barrels are now visually held correctly.
- Player head/body expression no longer gets stuck after some cutscenes.
- Developer credits fixes (restored when pausing/unpausing/skipping).
- Button-mashing gameplay is no longer framerate dependent.
- Waterfall spray SFX no longer gets stuck if all sprays unload before the sound fades out.
- Fix snow persisting when reloading save outside of SnowHorn Wastes.
- Getting kicked out of the shop no longer crashes.
- Many miscellaneous UI fixes.
- Fixed Magic Plant crash.
- SwapStone Hollow: initial Tricky sequence crash fix (”that’s my mom!”).
- White Mushroom collection softlock fixed.
- SwapStone Hollow Well fixes (holes in terrain, sequence breaks).
- Fixed SwapStone Hollow plant spore sequence break, allowing early access to the SwapStone.
- Forcefield Spell should no longer be lost in early game after reset.
- DarkIce Mines: tent fixes.
- Fixed Desert Force Point act being reset in some situations. 
- The scarab counter in the shop minigame is now visible.
- Test of Magic `flybaddie`s now work as intended.
- The player is no longer invisible while standing on the ice in the DIM river.
- Fixed DFP wheel flames (regression from previous patch).
- Fixed issue with Sabre's jaw animation during the Galadon intro cutscene.
- Fixed Dragon Rock EarthWarrior becoming unridable if summoned after they despawned.
- ...And more small fixes!

## v0.8.0
- Ported most patches from the January 26th, 2025 build of Dinomod Enhanced, including *all* progression-related patches.
- In addition, adds the following new patches:
    - Added a fade-to-black when leaving the shop.
    - Added an unused cutscene when Sabre leaves the shop (found by jeebs2kx).
    - Several `KT_Rex` improvements (including new optional Enhanced mode that adds dynamic music and slightly new behavior).
    - Several `BWlog` improvements (restored pitch tilt, should no longer crash in any scenario).
    - The speed of moving between menu options when holding down a direction input is now independent of framerate.
    - `IMSnowBike` now behaves accurately at 30 Hz and 60 Hz.
    - Fix `Lunaimar` crash when Kyte is unloaded.
    - Restored Walled City's moon temple aperture gameplay.
    - Fixed spirit particle effect for Sabre's pendant.
    - Sabre no longer voids out sometimes at the end of the Galadon defeat/DIM exit cutscene.
    - Character blinking is now framerate independent.
    - `FXEmit` no longer flashes the default particle on the first frame after loading.
    - The `BWlog` given by the LightFoot village totem puzzle should now work reliably.
    - `GPSH_flybaddie` now fires its intended projectile.
    - Fixed issue where anim object seq slots would not be freed if an object tried to run a sequence that didn't exist, eventually preventing any sequence from playing.
    - Several `WM_Platform` improvements.
    - Ice Blast Spell's debug cubes are hidden, it optionally costs less magic, and fixed an issue where it continued firing invisibly after running out of magic.
    - Optional quality-of-life patches for Garunda Te's FrostWeeds gameplay.
    - The UI's Scarab counter now only appears after Scarabs have first been collected.
    - Prevent a crash when small baskets unload while carried by the player.
    - Krazoa Spirits' unused trail texture animation is now applied.
    - Static `DR_EarthWarrior` spawns in Dragon Rock Top are removed (these were leftover for testing).
    - `KamerianBoss` has a slightly new health bar.

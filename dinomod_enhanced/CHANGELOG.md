## v0.9.3
- Fixed SwapStone Hollow river log not appearing after activating the Dragon Rock SpellStone.

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

# Dinomod Enhanced (Recompiled)
The mod ports over the fan-made fixes included in **Dinomod Enhanced** - a community ROM patch for the *December 2000* prototype of Rare's unreleased game **Dinosaur Planet**. The patch is developed by multiple contibutors and fixes a large number of the prototype's bugs and progression-blockers, allowing much more of the game to be explored.

The bulk of **Dinomod Enhanced**'s changes work through direct asset file modifications (i.e. editing text, textures, models, object instance layouts, etc.), but it also contains many code patches which are created through hand-written modification of the game's MIPS assembly. Patches in this latter category are nontrivial to port, since it ideally involves recreating the edit in C (informed by the game's [WIP decompilation](https://github.com/zestydevy/dinosaur-planet)). The recomp mod currently includes all of the ROM patch's asset patches, and C ports of approximately 80% of its code patches.

For more information about Dinomod Enhanced, visit:
- [www.dinosaurpla.net (the Dinosaur Planet Fansite)](http://www.dinosaurpla.net/)
- [The Dinosaur Planet Community Discord](https://discord.gg/yPDYXzYFb4)

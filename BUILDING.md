# Build Guide

## Prerequisites
You'll need to install `clang` and `make` to build these mods.
* On Windows, using [chocolatey](https://chocolatey.org/) to install both is recommended. The packages are `llvm` and `make` respectively.
  * The LLVM 19.1.0 [llvm-project](https://github.com/llvm/llvm-project) release binary, which is also what chocolatey provides, does not support MIPS. The solution is to install 18.1.8 instead, which can be done in chocolatey by specifying `--version 18.1.8` or by downloading the 18.1.8 release directly.
  * As an alternative to downgrading LLVM/clang, newer versions with MIPS support can be downloaded from [n64recomp-clang](https://github.com/LT-Schmiddy/n64recomp-clang/releases).
* On Linux, these can both be installed using your distro's package manager.
* On MacOS, these can both be installed using Homebrew. Apple clang won't work, as you need a mips target for building the mod code.

You'll also need to build [N64Recomp](https://github.com/N64Recomp/N64Recomp) for the `RecompModTool` utility.

## Building
* First, enter the directory of the mod you would like to build.
* Next, run `make` (with an optional job count) to build the mod code itself.
* Finally, run the `RecompModTool` utility with `mod.toml` as the first argument and `build` as the second argument.
  * This will produce the mod's `.nrm` file in the `build` folder.

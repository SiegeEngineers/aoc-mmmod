# aoc-mmmod
A mini DLL mod loader for Age of Empires 2: The Conquerors.

<span style="color: blue">m</span><span style="color: red">m</span><span style="color: green">m</span><span style="color: red">od</span> stands for <span style="color: blue">Mini</span> <span style="color: red">Mod</span> <span style="color: green">Manager</span>. You can pronounce it 'triple em'.

mmmod requires UserPatch 1.5.

This tool is only intended for use in other mods at this time. You can use it to easily load other modules, like [aoc-builtin-rms](https://github.com/siegeengineers/aoc-builtin-rms) or [aoc-language-ini](https://github.com/siegeengineers/aoc-language-ini). In the future, mmmod may support a configuration mechanism for mods, and APIs to add in-game options and other things mods might want.

## Usage
Download the `language_x1_p1.dll` file from the [releases page](https://github.com/siegeengineers/aoc-mmmod/releases) and put it in `%AOC-DIR%\Games\%YOUR-MOD-NAME%\Data\language_x1_p1.dll`. Then, place DLL files into `%AOC-DIR%\Games\%YOUR-MOD-NAME%\mmmods\` to install mod modules.

**If you already have a `language_x1_p1.dll` for custom strings**, rename it to `language_x1_m.dll`. mmmod will tell the game to use `language_x1_m.dll` for loading custom strings. It should look like:
```
Games\%YOUR-MOD-NAME%\Data\language_x1_p1.dll ← the mmmod loader
Games\%YOUR-MOD-NAME%\Data\language_x1_m.dll ← the real language file
```

**If you are already using the aoc-language-ini mod**, delete its `language_x1_p1.dll` file and install the `aoc-language-ini.dll` file from the [aoc-language-ini releases page](https://github.com/SiegeEngineers/aoc-language-ini/releases) into the `mmmods\` folder. It should look like:
```
Games\%YOUR-MOD-NAME%\Data\language_x1_p1.dll ← the mmmod loader
Games\%YOUR-MOD-NAME%\mmmods\aoc-language-ini.dll ← the aoc-language-ini loader
Games\%YOUR-MOD-NAME%\language.ini ← your language INI file
```

## Writing a mod
Mod DLLs can expose a few functions that will be called by mmmod at specific points. In the future, there may be more granular functions. At the moment, these are available:

### `void mmm_load(mmm_mod_info* info)`
Called as soon as the DLL is loaded. This is intended for mod configuration code in the future, and should not be used yet.

`info` is a pointer to a `mmm_mod_info` structure. You can set some metadata to tell mmmod about your mod:

```c
info->name = "Your mod name";
info->version = "1.0.0";
```

In the future, this may be used for an "about" menu.

### `void mmm_before_setup(mmm_mod_info* info)`
Called right before the main AoC game setup code runs. AoC game data is not yet loaded at this point, but UserPatch mod data and the main game object are available. This is a good place to install hooks.

`info` is the `mmm_mod_info` structure for this mod.

### `void mmm_after_setup(mmm_mod_info* info)`
Called right after the main AoC game setup code runs. Game data has been loaded and the main menu is fading in. This happens quite late; you may want to be able to run code earlier in the startup process, but after `before_setup`. In that case, you can use `before_setup` to install a hook in the appropriate place, and run your setup code in that hook.

`info` is the `mmm_mod_info` structure for this mod.

### `void mmm_unload(mmm_mod_info* info)`
Called right before the DLL unloads. Revert your hooks and free your data structures here.

`info` is the `mmm_mod_info` structure for this mod.

### `mmm_mod_info* info`

The `mmm_mod_info*` structure contains a `meta` property that will tell you certain things about the game.

```c
// const char* containing the version of the mmmod loader
info->meta->version;
// const char* containing the base directory of the game, eg C:\Program Files\Age of Empires II
info->meta->game_base_dir;
// const char* containing the base directory of the UserPatch mod that is loading this DLL,
// or NULL if it's being loaded into the base game.
info->meta->mod_base_dir;
// const char* containing the short name of the UserPatch mod.
// (the one given by GAME= command line parameter)
info->meta->mod_short_name;
```

## Building a mod
Mods must be provided as DLL files. The `mmm_` functions must be exported from the DLL. The exact process depends on your compiler.

To get the `mmm_mod_info` structures, copy-paste mmmod.h from this repository into your mod source code. The ABI will be maintained as much as possible.

## License
[LGPL-3.0](./LICENSE.md)

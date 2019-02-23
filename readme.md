# aoc-mmmod
A mini DLL mod loader for Age of Empires 2: The Conquerors.

<span style="color: blue">m</span><span style="color: red">m</span><span style="color: green">m</span><span style="color: red">od</span> stands for <span style="color: blue">Mini</span> <span style="color: red">Mod</span> <span style="color: green">Manager</span>. You can pronounce it 'triple em'.

mmmod requires UserPatch 1.5.

This tool is only intended for use in other mods at this time. You can use it to easily load other modules, like [aoc-builtin-rms](https://github.com/siegeengineers/aoc-builtin-rms) or [aoc-language-ini](https://github.com/siegeengineers/aoc-language-ini). In the future, mmmod may support a configuration mechanism for mods, and APIs to add in-game options and other things mods might want.

## Usage
Download the `language_x1_p1.dll` file from the [releases page](https://github.com/siegeengineers/aoc-mmmod/releases) and put it in `%AOC-DIR%\Games\%YOUR-MOD-NAME%\Data\language_x1_p1.dll`. Then, place DLL files into `%AOC-DIR%\Games\%YOUR-MOD-NAME%\mmmods\` to install mod modules.

If you are already using `language_x1_p1.dll` for custom strings, I recommend installing [aoc-language-ini](https://github.com/siegeengineers/aoc-language-ini) as your first mod in `mmmods\aoc-language-ini.dll`!
If you are already using the standalone `language_x1_p1.dll` from an older version of aoc-language-ini, delete it and install the `aoc-language-ini.dll` file from the [aoc-language-ini releases page](https://github.com/SiegeEngineers/aoc-language-ini/releases) into the `mmmods\` folder.

## Writing a mod
Mod DLLs can expose a few functions that will be called by mmmod at specific points. In the future, there may be more granular functions. At the moment, these are available:

### `void mmm_load(void* reserved)`
Called as soon as the DLL is loaded. This is intended for mod configuration code in the future, and should not be used yet.

`reserved` is always NULL.

### `void mmm_before_setup(void* reserved)`
Called right before the main AoC game setup code runs. AoC game data is not yet loaded at this point, but UserPatch mod data and the main game object are available. This is a good place to install hooks.

`reserved` is always NULL.

### `void mmm_after_setup(void* reserved)`
Called right after the main AoC game setup code runs. Game data has been loaded and the main menu is fading in. This happens quite late; you may want to be able to run code earlier in the startup process, but after `before_setup`. In that case, you can use `before_setup` to install a hook in the appropriate place, and run your setup code in that hook.

`reserved` is always NULL.

### `void mmm_unload(void* reserved)`
Called right before the DLL unloads. Revert your hooks and free your data structures here.

`reserved` is always NULL.

## Building a mod
Mods must be provided as DLL files. The `mmm_` functions must be exported from the DLL. The exact process depends on your compiler.

## License
[GPL-3.0](./LICENSE.md)

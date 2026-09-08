# ENB Input Disabler for Fallout 4

F4SE port of [enb-input-disabler](https://github.com/doodlum/enb-input-disabler).
While ENB's editor is open, the plugin sets Fallout 4's native
`ControlMap::ignoreKeyboardMouse` flag, following the original mod's approach.
Closing the editor restores the flag's value from before it opened.
Fallout 4's native console-input exception is intentionally preserved.
Version 1.0.1 also widens the game's temporary cursor clip rectangle to its full
client area while ENB is open. This addresses the reported smaller central cursor
area when using the jarari/fo4test upscaler with this plugin. It hooks only
Fallout4.exe's `ClipCursor` import; rendering and upscaler code are untouched.

ENB owns the hotkey and cursor. Shift+Enter works with ENB's usual configuration;
custom ENB editor hotkeys work automatically because this plugin reads ENB's
actual editor state. No ESP is needed. Gamepad input is unchanged.

## Requirements and installation

- Fallout 4 **1.10.163**, **1.11.221**, or **1.11.240** (exact versions only).
- F4SE matching the installed executable.
- **Address Library for F4SE Plugins**, with the matching
  `Data/F4SE/Plugins/version-<game-version>-0.bin` database.
- Fallout 4 ENBSeries exposing SDK 1.01 or a compatible later 1.x SDK.

Install the release ZIP with a mod manager, or copy its `F4SE` folder into the
game's `Data` directory. The DLL belongs at
`Data/F4SE/Plugins/ENBInputDisablerFO4.dll`. Launch through F4SE.
The PDB is optional and helps diagnose crashes.

The log is `Documents/My Games/Fallout4/F4SE/ENBInputDisablerFO4.log`
(or F4SE's configured save folder). Look for `Connected to ENB SDK`.
Missing ENB or incompatible SDK exports leave game input unchanged.
Missing Address Library files are reported by CommonLibF4 when resolving addresses.

`ENBInputDisablerFO4.ini`, beside the DLL, has `[Compatibility] FixCursorBounds=1`.
Set it to `0` and restart to compare against the original cursor behavior.
The default is enabled if the INI is absent. The log reports the original and
expanded clip rectangles once per editor session when the fix is exercised.

## Build

Windows x64, Visual Studio 2022 C++ tools, Git, and xmake 3.0+ are required.
Run `./BuildRelease.ps1` in PowerShell. It bootstraps the pinned CommonLibF4
dependency if absent, builds the DLL, runs input ownership tests, and produces
`build/ENBInputDisablerFO4-1.0.2.zip`. It does not deploy to a game installation.
It also populates `release/` with just the DLL, PDB, and INI.

CommonLibF4 is pinned at `ca31eeb6c7353555973bc351c6733d6492f2c66e`, matching
PowerArmorPipBoyUI. Its commonlib-shared submodule is
`f0b1670ee9caac2e349497f6f3c08a69633a8ea7`.
The local dependency copy is ignored by Git; `tools/bootstrap.ps1` reconstructs it.

## Validation status

The DLL builds and input ownership/restoration and cursor-rectangle tests pass. Static binary checks
verify the singleton, flag initialization, and keyboard/mouse predicate on all
three runtimes. See `docs/runtime-validation.md` for evidence.
The user confirmed that 1.0.1 works correctly with the jarari/fo4test upscaler,
including the cursor fix (runtime unspecified). Version 1.0.2 preserves that
behavior and refines error handling, diagnostics, and release packaging.
**The 1.0.2 build still needs an in-game smoke test; separate in-game validation
of all three runtimes has not been reported.**

Test opening/closing ENB while idle and while holding movement or fire buttons;
mouse movement, both mouse buttons, wheel, movement, jumping, and menu hotkeys;
first/third person and power armor; opening/closing over game menus; Alt+Tab;
and normal input after closing. Also test a remapped ENB hotkey and no ENB installed.
Check for held actions remaining active or input leaking during transitions.
For the cursor fix, verify all four window edges, Alt+Tab, windowed/borderless
modes, changed upscaling quality, and restoration after closing ENB.

The flag is shared engine state, not a per-plugin ownership API. Another mod
writing it during an ENB session can interfere. This follows the original
callback approach, with state updates once per ENB BeginFrame; transition timing
and interactions with other input mods need in-game verification.

## License

GPL-3.0-or-later with the original project's exceptions in `EXCEPTIONS.md`.
The ENB SDK declarations and callback approach are adapted from the original
`enb-input-disabler` source. CommonLibF4 retains its own license.

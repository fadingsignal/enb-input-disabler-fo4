# Runtime validation — 2026-09-07

The port uses the same ENB SDK callback and native suppression flag as
`G:/Git/enb-input-disabler`. Runtime gating and F4SE metadata follow the
exact-version approach in PowerArmorPipBoyUI. All three versions are explicitly
listed in `src/Runtime.h`; no later runtime is admitted implicitly.

## Address Library and layout

The DLL resolves `RE::ID::ControlMap::Singleton` through CommonLibF4's
OG/NG/AE VariantID selection and the installed per-executable Address Library.
The NG entry is never selected by this plugin's version gate.
There are no hard-coded executable addresses in the DLL. Version 1.0.1 also
wraps the game's named `USER32.dll!ClipCursor` import; it patches no instructions.

| Runtime | Singleton ID | Singleton RVA | Constructor ID | Constructor RVA | Suppression predicate RVA |
| --- | ---: | --- | ---: | --- | --- |
| 1.10.163 | 325206 | 0x59DA210 | 425955 | 0x1B27830 | 0x1B29B20 |
| 1.11.221 | 4799307 | 0x31E5B08 | 2268327 | 0x166E570 | 0x1670807 |
| 1.11.240 | 4799307 | 0x31F0B88 | 2268327 | 0x166E890 | 0x1670B27 |

Constructor mappings are corroborated by
`F:/Projects/Fallout 4 Mods/F4SE/IDA_Functions_OG_163_and_AE_221.csv`.
The audit checks that each constructor stores its `this` pointer into the
Address-Library-resolved singleton and initializes the word at object +0x140,
which includes `ignoreKeyboardMouse` at +0x141. It also checks the singleton
resides in a writable PE section and verifies the native suppression predicate:
read byte +0x141, branch if clear, then restrict suppression to device IDs 0/1
(keyboard/mouse). The CommonLibF4 member offset is asserted when compiling.

The predicate preserves the engine's console exception, as requested.
Input from gamepads remains unaffected. Static analysis does not prove held-key
release behavior, rendering/input scheduling, or compatibility with other mods.

## Reproducing the static checks

Install `pefile==2024.8.26` and `capstone==5.0.9` into your Python environment
or into `build/python-deps`, then run:

```powershell
python tools/audit-runtime.py <unpacked-Fallout4.exe> <matching-version-file.bin>
```

The tool validates the executable version and database filename/record structure.
It rejects unsupported runtimes, mismatched database names, invalid layouts,
or changed suppression instructions. Audit-only RVAs are never used by the DLL.

All three audits passed. The OG executable's encrypted code section was unpacked
in a repository-local build copy for inspection; installed executables were not
changed. The AE code sections could be inspected directly. No game executable or
Address Library database is included in the distributable.

| Input | SHA-256 |
| --- | --- |
| OG .163 audit executable | `9a147a35686691ac6c52c2b3bc24027fc26119c2eb77716e10900af29a61609e` |
| AE .221 executable | `428f9996cc4248e26c0f62f9fdd3eaf0e5eb305834b67ee5996538e593218b61` |
| AE .240 executable | `fdcef37ac1230af6d0b0050eb2142b139ef3a867b37b9211fb6edfcc646072f8` |
| .163 Address Library | `d849ae8989c54a2ceebbc587c2a77f62a44f74d0c42a42e0bed30d65118d3395` |
| .221 Address Library | `2fdcc2a0926659c37255d2eaf335775240ec7fefdf6cb3b35b063faca25a448f` |
| .240 Address Library | `65985cc2259384a13cffb538d74776e422e62b0cd3766485a000620242b72b06` |

## Build and behavior validation

Visual Studio 2022 x64 / xmake releasedbg build succeeds. The standalone state
tests cover open/close cycles, repeated open frames, preexisting suppression,
inactive frames, absent singletons, and singleton replacement.

ENB registration checks all three required exports and SDK compatibility before
installing a callback. Module enumeration grows its buffer if needed. Startup
retries at F4SE GameLoaded, and successful registration is idempotent. SDK state
queries occur exclusively inside ENB callbacks. Closing/OnExit restores prior
state; inactive frames do not clear suppression owned by another plugin.

In-game testing is pending on **all three runtimes**. See the README checklist.

## 1.0.1 cursor bounds compatibility

The user reported that 1.0.0 suppresses input but confines the ENB cursor to a
smaller central area; disabling this plugin removes that restriction. The
upscaler was identified as https://github.com/jarari/fo4test. Source inspected at
commit `0347ce28a17a580ff3ffcd1865aacb86ecc8b072` has no direct cursor-position,
ClipCursor, or ignoreKeyboardMouse hooks. Its ENB render-resolution adjustments
alone do not establish the cause of this plugin-triggered restriction.

Fallout 4's `Main::Run_WindowsMessageLoop` was inspected directly. During gameplay
it computes a central clip rectangle with +0.25/-0.25 size factors, calls
ClipCursor before pumping messages, and restores the prior clip rectangle
afterward. This explains why a once-per-frame ClipCursor override would be
insufficient. The audit now checks this path and the named import on all versions.
Message-loop IDs are 847266 (OG) and 2228915 (AE); these are used by the offline
audit only, not the DLL's import wrapper.

The new wrapper expands only valid requested rectangles strictly smaller than
and wholly contained in the current foreground game window's client rectangle.
It forwards null/unclip requests, desktop-sized restoration requests, inactive
editor calls, foreign foreground windows, and minimized/invalid windows unchanged.
Window coordinates come from USER32, not render-target dimensions. ENB and the
upscaler's own imports are untouched. Any existing game-import wrapper is chained.

No persistent ClipCursor override is installed on editor-open/close: the game
still restores its original rectangle after the message pump. ENB callback state
is transferred to the message-loop thread with an atomic flag. INI opt-out permits
comparison with 1.0.0. Rectangle tests cover centered bounds, desktop restoration,
equal/invalid bounds, negative monitor coordinates, and moved windows.

The user subsequently reported "Working perfectly now" with 1.0.1. This confirms
the fix on that setup; the tested runtime was not specified. It does not establish
separate in-game results for all three runtimes. The log records the first
expansion in each ENB session.

## 1.0.2 cleanup review — 2026-09-08

Reviewed ENB SDK registration, input ownership/restoration, cursor import wrapping,
INI lookup, runtime gating, and build/package tooling. The working input flag,
cursor expansion conditions, and Address Library IDs remain unchanged.

- Preserve the Windows last-error value across cursor helper calls and logging.
- Report failed plugin-path lookup and unavailable/failed F4SE retry registration.
- Use a named SDK editor-state enum instead of a numeric literal.
- Reduce the precompiled header to required interfaces and guard Windows macros.
- Derive DLL metadata, log/query versions, and ZIP names from xmake's version.
- Stage an explicit archive payload so stale files under dist cannot be shipped.
- Verify the exact archive payload and equality with the three release files.
- Correct the license label to GPL-3.0-or-later and record user-tested 1.0.1 status.

The original 1.0.1 DLL/PDB/INI and source snapshot are retained in
`build/review-baseline-1.0.1`. An in-game smoke test is still needed for 1.0.2.

Validation passed: releasedbg build; input-state and cursor-rectangle regressions;
exact archive/DLL metadata verification; and direct calls to the built DLL's
`F4SEPlugin_Query` via `tests/query-plugin.py`. The latter accepts .163/.221/.240,
rejects .168/.984/.241 and editor/null requests, and confirms Query reports the
same version as the exported metadata. It does not invoke plugin Load, ENB, or
engine addresses. Prior three-runtime binary audits remain applicable because
the runtime IDs, member offset, and suppression/clip conditions did not change.

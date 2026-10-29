# EasyRPG Player 3DS investigation

## Scope and source checked

Imported the EasyRPG Player `master` source (upstream commit `096c004de`) into this repository and fetched all five advertised branches into local refs `upstream/*`, plus all 26 official version tags from 0.1 through 0.8.1.1. I checked the battle status refresh path at each tagged release, and the 3DS exit hook where that code exists. This is a source-history comparison, not a benchmark of every release binary or a full review of every commit. The two focused fixes described below are applied in `src/scene_battle.cpp` and `src/platform/3ds/ui.cpp`.

## Battle performance regression

The reported symptom closely matches upstream [issue #3497](https://github.com/EasyRPG/Player/issues/3497): RPG Maker 2003 battles can fall to 2–10 FPS on a 2DS while the overworld stays near 60 FPS; the reporter says 0.6.2.x did not have the battle slowdown.

A likely hot path was in upstream `Scene_Battle::UpdateEvents()` (`src/scene_battle.cpp`): before this fix, it called `status_window->Refresh()` on every battle update. `Refresh()` clears and redraws the complete battler-status bitmap (names, states, HP/SP and gauges). The status window also updates gauges in its own `Update()`. That repeated full software redraw is especially costly on the original 3DS/2DS CPU.

This is a meaningful version-to-version regression, not just a hardware-capability issue: the 0.6.2.3 tag (2020-10-03) refreshes after a battle event finishes (`events_running && !interp.IsRunning()`), whereas the 0.7.0 tag (2021-10-29) and every later release tag through 0.8.1.1 do an unconditional full refresh from `UpdateEvents()`. The source change dates to the battle-state-machine refactor in October 2020, before it shipped in 0.7.0. So the source evidence suggests a step regression between 0.6.2.3 and 0.7.0, rather than a gradual slowdown newly introduced by each subsequent release. Upstream issue #3497's discussion independently points at redraw work and suggests skipping refreshes when displayed stats have not changed.

### Change applied

The fix restores the previous edge-triggered refresh behavior: record whether the battle interpreter was active, update it, and perform the expensive refresh only when it transitions from running to stopped. The window's existing per-frame update continues to refresh animated gauges. This should remove the dominant unnecessary redraw in idle/event-running battle frames while preserving the established 0.6.2 behavior. It is a focused first fix; a more complete dirty-state mechanism could later refresh only when visible actor/enemy fields actually change.

This is strongly evidenced by source comparison, but not yet measured on hardware. Validate on Old 2DS/3DS with a blank RPG Maker 2003 battle and a representative game, comparing frame rate and checking event commands that change party status, status effects, HP/SP, battle transitions, and gauge battle layouts.

## HOME/Power exit crash

This matches upstream [issue #2952](https://github.com/EasyRPG/Player/issues/2952). Before this fix, `src/platform/3ds/ui.cpp`'s `APTHOOK_ONEXIT` hook directly called `Player::Exit()` from inside the libctru APT hook callback. `Player::Exit()` is not just a request: it tears down the Player and destroys `DisplayUi` (and thus the `CtrUi` whose `ProcessEvents()` is on the stack). After the callback returns, `aptMainLoop()` reports termination and `ProcessEvents()` returns false; the normal main-loop shutdown path then calls `Player::Exit()` again. This creates teardown from inside an active UI callback, possible re-entrant/double teardown, and object lifetime hazards.

### Change applied

Player teardown is no longer called from the APT callback. Let the callback log the notification; `CtrUi::ProcessEvents()` already checks `aptMainLoop()`, and `Player::MainLoop()` already handles a false result by unwinding scenes and calling `Player::Exit()` from the normal main-loop context. This gives a single, orderly teardown path. Validate HOME and Power-button exits from the title screen, while loading/playing, and during audio playback on both Old and New 3DS-family hardware, and confirm the HOME menu is restored without a crash dump.

## Patch and validation status

The two fixes are applied to the imported Player source. I could not build or run the project here because CMake is not installed, and no 3DS devkit/toolchain or hardware is available in this environment. The source patch was checked for clean application against the fetched upstream tree before integration, but it has not been compiled or measured on hardware. Treat the performance improvement as expected, not yet measured; complete the hardware validation above before considering the fixes confirmed.

# HITS (Phase 3 lane, change 2): hit confirmation + pooled, merged damage numbers

Not built, not run (rules: no build/editor/PIE/commit). Code written against the files read; Main builds once and runs the tests below.

## Files
Edited (all in the lane's list):
- `UI/HUD/HudHitMarker.h/.cpp`: `FTuning` new fields/cvars below; new `FadeSecondsOf`, `ScaleOf`, `Rim`, `LookOf` (+ `FLook` fill/rim/width); `Tint(Crit)` pale gold -> white (its red is the rim); order Kill > WardBreak > Crit > Ward > Body and `Cue`/`SoundsAgain` untouched.
- `UI/HUD/PlayerHUDWidgetHitMarker.cpp`: on takeover sets `HitMarkerTime` = `SecondsOf`, `HitMarkerShowFor` = `FadeSecondsOf` (NativeTick's alpha is clamped at 1, so time > fade holds the marker whole and popped first, then it fades and settles: no edit to `PlayerHUDWidget.cpp`), gives each tick `LooterUI::RectBrush(Fill, Rim, RimWidth)` tinted white, size = hit size x `ScaleOf(ShownMark)` (a blast's crit keeps growing as pellets land).
- `Combat/HealthComponent.cpp` (`SpawnDamageNumber` + 2 includes only): builds an `FDamageNumberHit` (target, damage, crit, place, `ElementCallout::LookFor` look, `UElementStatusComponent::IsDealingNow()` read here now, `DamageNumberClass`) and hands it to `UDamageNumberSubsystem::ShowHit`. No SpawnActor. `.h` untouched.
- `UI/World/DamageNumberActor.h/.cpp`: pooled. `Show` replaced by `ShowHit` / `MergeHit` / `ShowStream` (public, was `StartStream`) / `AddToStream`; `Advance` (tick body, tests call it), `SetPooled`, `IsShowing`, getters. Hidden + not ticking while it waits (XpPopupActor's pattern); done = back to the pool (`Finish`), a non-pooled one (a callout) is destroyed. Screen space + draw-at-desired-size kept (fixed on-screen size). Motion math (`DamageNumberMotion`), `Lifetime` 0.9 / `CriticalExtraLife` 0.25 / rise / drift unchanged.
- `UI/World/DamageNumberActorStream.cpp`: stream logic unchanged (roll, hold 0.8 + fade 0.45, pulse, slots, follows the creature); `FindStream`/`CountStreams` (two `TActorIterator` walks per status piece) moved to the subsystem as pool walks; actor keeps `IsStreamOf`/`IsStreamOn`; spent stream -> `Finish()` (pool) instead of `Destroy()`. Callout: `Begin` + ends in `TickCallout` at 1.3 s (was `SetLifeSpan` 1.4 s; opacity was 0 from 1.3 s anyway).
- `UI/World/DamageNumberWidget.h/.cpp`: new `SetTotal` (a merged number: restyles only when crit/tag/tint/size changes, else just the text, so a settled crit doesn't flash white); `NumberText()`; `ApplyDamageText` collapses the glyph (a reused widget never shows a callout's glyph).

New (CODEMAP one-liners):
- `UI/World/DamageNumberSubsystem.h/.cpp`: `UDamageNumberSubsystem`, every floating damage number: a pool of 20 `ADamageNumberActor` made at play start and used again (no spawn per hit), hits on one target within 80 ms summed into one number (a shotgun blast one big number; a crit pellet makes it a crit), status pieces into the creature's stream; `FDamageNumberHit`; `DamageNumberMerge` (`Joins`, `SameLook`, `Merged`, `GrowFor`, `Looter.UI.DamageNumbers.*`).
- `Tests/DamageNumberPoolTests.cpp`: `Looter.UI.DamageNumbers.Merge` (window, looks, sum's look and size as plain rules), `.Pool` (pre-made, merge, crit pellet, rising on, reuse, burst cap, no spawns), `.Stream` (status pieces through the pool, untouched).

CODEMAP edits for Main: line `UI/HUD/HudHitMarker` -> add `FadeSecondsOf`, `ScaleOf`, `Rim`/`LookOf`; line `UI/World/DamageNumberActor` -> "pooled by `UDamageNumberSubsystem`; a merged number (`MergeHit`) sums a target's hits in 80 ms". Stale comments in files not mine: `PlayerHUDWidget.h:76` ("orange on a crit"), Pipeline `Gunplay.md` ~l.241 ("colours white / CyanText / Accent / Hurt").

## Numbers (before -> after)
Hit marker (34 px box, four 11x4 px ticks):
| | before | after |
|---|---|---|
| time body / ward | 0.18 s, fading from the first frame | 0.36 s: holds whole 40% (0.14 s), fades 60% (`FadeShare` 0.6) |
| time crit | 0.18 | 0.42 (`CritSeconds`, new) |
| time kill / break | 0.34 / 0.3 | 0.6 / 0.45 |
| pop body / crit / kill / break | 0.3 / 0.3 / 0.75 / 0.6, decaying over the whole life | 0.12 / 0.2 / 0.25 / 0.3, held through the hold, settling during the fade |
| scale vs a plain hit, same hit | all 1 | crit 1.6, kill 1.8, break 1.6 (`CritScale`/`KillScale`/`BreakScale`, new) |
| crit look | pale gold (Accent 255,214,130) fill, dark rim | white fill, red (Hurt 255,59,46) rim 1.25 px (`CritRim`): red and white |
| kill / body / ward | Hurt red / white / cyan or ward element | same, all with the kit's 1 px dark outline |
On screen during the hold: body 1.12x (38 px, to 53 px at a quarter of health), crit 1.92x (65 to 91 px), kill 2.25x (77 to 107 px); settling to 1 / 1.6 / 1.8x as they fade. Size by health share (`SizeGain` 0.4, `SizeShare` 0.25) and `BlastWindow` 0.08 unchanged.
Damage numbers:
| | before | after |
|---|---|---|
| actors | SpawnActor + widget per bullet/pellet/status piece (a piece's actor destroyed at once if a stream existed) | 20 made at play start (`PoolSize`, 4-64), reused; past 20 the oldest hit's number is taken (a stream never); 0 spawns per hit |
| shotgun blast | 8-12 numbers on one chest | one number = the pellets' sum, at the first pellet's spot, rising on; size x(1 + 0.04 per extra hit) up to 1.3 (12-pellet blast 28.6 pt vs a crit's 36) |
| window | none | 80 ms from the number's first hit (`MergeSeconds`; not sliding: a rifle at 650 rpm = 92 ms/round stays one number a shot) |
| crit in a group | n/a | the number becomes a crit (slams in white-hot again where it is, "!", crit life 1.15 s, lob) |
| what merges | n/a | same target, same element, and both or neither a status's own colour (a smite's burst stays its own number); WEAK/RESIST tag of any hit kept; biggest `SizeScale`; an element crit tints the sum |
| status stream | per-element rolling stream | same, through the pool; never merged with hits |
| stream lookup | 2 TActorIterator walks per piece | walk of the 20-actor pool |

## Cvars (all read live)
- `Looter.UI.HitMarker.`: `Seconds` [0.36], `CritSeconds` [0.42], `KillSeconds` [0.6], `BreakSeconds` [0.45], `FadeShare` [0.6; 1 = the old fade-from-start], `Pop` [0.12], `CritPop` [0.2], `KillPop` [0.25], `BreakPop` [0.3], `CritScale` [1.6], `KillScale` [1.8], `BreakScale` [1.6], `CritRim` [1.25 px, 0-1.75], unchanged `SizeGain` [0.4], `SizeShare` [0.25], `BlastWindow` [0.08]. Old look: Seconds 0.18, CritSeconds 0.18, KillSeconds 0.34, BreakSeconds 0.3, FadeShare 1, Pop/CritPop 0.3, KillPop 0.75, BreakPop 0.6, all Scales 1 (the crit's colour is code).
- `Looter.UI.DamageNumbers.`: `MergeSeconds` [0.08; 0 = every hit its own number], `MergeGrow` [0.04], `MergeGrowMax` [1.3], `PoolSize` [20, read at play start and when the pool grows].

## Tests
`Tools\runtests.ps1 -Filter Looter.UI.HUD.HitMarker` (`.Rules`, updated: every mark >= 0.35 s and holds before its fade, crit > body, kill longest, crit >= 1.6x body, kill > crit, own pops, looks: body white/dark, crit white/red rim, kill solid red, none alike) and `-Filter Looter.UI.DamageNumbers` (`.Element` untouched and its expectations unchanged; new `.Merge`, `.Pool`, `.Stream`). Once each also: `-Filter Looter.UI.StatusIcons` (the stream math and `Tint(Crit, Thunder) == Tint(Crit)`, unchanged) and `-Filter Looter.Feedback.DamageNumbers` (the motion math, unchanged).

## PIE check for the user
Ranchhand point-blank at a spider: one big number per blast, a crit pellet turns it the white-hot crit "!"; rifle bursts still one number a round. Every hit's X stays ~0.35 s; a headshot's X is bigger, white rimmed red; a kill's is the biggest, solid red.

## Notes for Main
- Idle pooled numbers: actor tick off, but each hidden `UWidgetComponent` still runs its own cheap tick (as the 8 XP popups do): 20 of them; PERF may measure. The first show of each pooled number builds its widget tree once.
- `UStatusCalloutSubsystem` still spawns its callout actor (once per status per creature, not per hit; not this lane's file): it could take a pooled one later.
- A creature naming its own `DamageNumberClass` gets its own pool of that class (up to `PoolSize`, made as needed); none do today.

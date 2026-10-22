# The twelve new ember powers (Odessa, Crane, Gauge, Pike): holding tree

Written 2026-10-20 by the POWERS agent while the Gravemother agent owned Source/ and the builds. Nothing here is built.
Nothing was committed. Main moves this tree into Source/, builds, runs the steps below, fixes, commits.

**Phase 2 (2026-10-20, done): the tree is in Source/ and built; Source/ is now the truth, this tree is stale.** Changes
made in Source/ after the move (not mirrored here): HeroTests (.Playable/.Save/.Screen use Odessa: Crane, Gauge and Pike
are playable now); EmberGaugeTests SlagBomb (the pool burns on after End(false), stepped out); look tunes from the PIE
check: EmberSlagBomb(Look) pool, EmberIronHide forge-glow tint on the player, EmberRailSpike fading trail + smoke thread,
EmberTripwire wire 1.6 cm, EmberTollChain links 3 cm, EmberFiftyTwoPickup flying cards x1.35. Pipeline/CODEMAP have the
text. Pike and Crane imported (ImportModels + build_hero_retarget.py), DA_Hero_* filled by create_ember_assets.py.

## Files

Paths are relative to this folder (`Saved/Powers/`), mirroring the repo.

### New: copy into Source/AI_Looter_Shooter/ as they are
- `Embers/Powers/EmberPowerBase.h/.cpp`: `UEmberPowerBase` (Abstract), what the twelve share: `BeginUse` (body, controller,
  colour, level scale), `Hurt` (UEmberDamageType, the controller as instigator, no causer, a point hit; logs into `GetDealt`
  for the tests), `GetAim`, `HandPoint`, `FeetOf`, `FindInSights`, `CanSee`, `FindGround`, `IsBoss`, `DistanceToBody`,
  `BodyOf`, `ScaledDamage`, `EffectsIn`, `Kick`, and the next-frame "linger" chain on the player's clock (`StartLinger`, `Linger`).
- `Embers/Powers/EmberFX.h/.cpp`: `EmberFX` namespace: `DustColor`, `GroundRing`, `DrawLine`, `DrawCard`, `FEmberSparks`
  (glowing ballistic streaks), `FEmberTint` (a body worn in one colour through M_FX_HitFlash's overlay; never over another overlay).
- `Embers/Powers/EmberPropActor.h/.cpp`: `AEmberPropActor` (NotPlaceable, Transient): a baked prop set down by a power (no
  collision) with an optional movable shadowless point light. Grave Ward's lantern, Tripwire's stakes, Slag Bomb's sound anchor.
- `Embers/Powers/Ember<Power>.h/.cpp` x12: `UEmberLuckyStreak`, `UEmberFiftyTwoPickup`, `UEmberDoubleOrNothing`,
  `UEmberTollChain`, `UEmberGraveWard`, `UEmberLastRites`, `UEmberSlagBomb`, `UEmberIronHide`, `UEmberRailSpike`,
  `UEmberSpyglass`, `UEmberTripwire`, `UEmberLongShot`; plus `Embers/Powers/EmberSlagBombLook.cpp` (the pool's look, split
  off so EmberSlagBomb.cpp stays under 500 lines).
- `UI/World/EmberMarkActor.h/.cpp`, `UI/World/EmberMarkWidget.h/.cpp`: a power's mark over a creature (screen-space widget
  component: through walls, hidden with the world labels under menus): the minimap's hostile diamond in the power's colour
  and the distance (Spyglass, Long Shot), or the power's Inked icon and a countdown (Last Rites); pops in, breathes, flares,
  fades out and destroys itself when retired or its creature is gone.
- `Tests/EmberHeroPowerBench.h`, `Tests/EmberOdessaTests.cpp`, `Tests/EmberCraneTests.cpp`, `Tests/EmberGaugeTests.cpp`,
  `Tests/EmberPikeTests.cpp`: `Looter.Embers.Power.<Id>` for all twelve (deterministic, test level, no PIE; each also checks
  its Inked icon exists).

### Replaced: whole new versions here; a unified diff of each in `Patches/` (`git apply Saved/Powers/Patches/<name>.patch`
from the repo root; all ten apply cleanly to the working copy as of 2026-10-20 01:30. Main applies the patch where the
Gravemother agent may have changed the file since, else copies the file)
- `Combat/BulletSubsystem.h` (**the Gravemother agent edits this file too: apply `Patches/Combat_BulletSubsystem.h.patch`**):
  `FBulletShot` gains `bSureCritical` and `bBlank`; new `FBulletLanding` {Traveled, bCritical, DamageScale}; new delegates
  `FOnBulletFiring(FBulletShot&)`, `FOnBulletLanding(const FBulletShot&, const FHitResult&, FBulletLanding&)` and members
  `OnBulletFiring`, `OnBulletLanding`.
- `Combat/BulletSubsystem.cpp` (**also the Gravemother agent's: apply the patch**): `Fire` copies the shot and broadcasts
  `OnBulletFiring` before anything reads it; `ResolveHit`: critical = `bSureCritical` or the target's spot, then
  `OnBulletLanding` (may set critical, scale damage), damage x `DamageScale`; a blank deals 0, applies no damage, tells the
  gun no hit, and shows/sounds nothing on anything with health (folded into their `bOwnLook`); its world hit looks as ever.
- `Audio/LooterSoundCues.h` (**also the Gravemother agent's: apply the patch, or paste `CueBlock.h.txt` after
  `Ember.Denied` in `namespace Ember`**): the 35 new cue names.
- `Combat/HealthComponent.h/.cpp`: `SetDamageTakenScale(FName Source, float Scale)` / `GetDamageTakenScale()` (a TMap by
  source, multiplied; 1 removes); `HandleAnyDamage` scales the incoming damage first, so the number, `OnDamaged` and death
  all see the scaled hit.
- `Embers/EmberComponent.h/.cpp`: `Finish()`: ends the running power now as if its time ran out (End not interrupted; the
  cooldown runs on). Last Rites calls it when its mark is settled by a death.
- `Embers/EmberCommands.cpp`: the two help strings no longer list only Ellis's ids.
- `Tests/HeroTests.cpp`: `.Playable` checks every hero's powers have code and that a non-Ellis hero is playable exactly when
  body and retargeter load (was: "only Ellis"); the unlock check reads "picked only once playable"; `.Character` expects
  `Refused` (Toll Chain's own say on a bare actor) instead of `NoPower`.
- `UI/Style/InkedIconData.inl` (generated): the twelve icons added (nothing else changed; verified the generator reproduces
  today's file before adding them).
- `Art/Icons/InkedIcons.py` (copy to `Art/Icons/`; patch in `Patches/`): `rot`, `rrect`, `closed`, `card` helpers and the
  twelve icon functions; `EMBER_ICONS` and `icons()` list them. Run from this tree it writes the .inl into this tree.

### Edited in place (not compiled; not committed)
- `Tools/Unreal/create_ember_assets.py`: the twelve entries now name their class and icon, with tuned numbers (below).
- `Art/Sounds/recipes/embers_odessa.py`, `embers_crane.py`, `embers_gauge.py`, `embers_pike.py` (new) and
  `recipes/__init__.py` (the four added to `GROUPS`); `Art/Sounds/cues.json` (35 entries added, nothing else moved; the
  standing -1.0 dB shift is the existing one); `Art/Sounds/Out/Ember_<Power>_*.wav` (95 files). Check: 0 flagged.

## Run order after moving the tree
1. Copy the new files; patch or copy the replaced ones (BulletSubsystem.h/.cpp and LooterSoundCues.h via the patches if
   the Gravemother agent touched them); copy `Art/Icons/InkedIcons.py`.
2. `Tools\launch.ps1 -Build` (full rebuild: new UCLASSes, changed headers).
3. In the editor: `Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/create_ember_assets.py"`: expect 15
   `EMBERS ... updated` lines naming `/Script/AI_Looter_Shooter.Ember<Power>` and `EMBERS done` (a "not a class yet" warning
   means the build didn't land).
4. The sound bank: `build_sound_bank.py` as usual (after step 1, so the header lists the 35 names; expect no
   "played by the code but has no sound" / "not in LooterSoundCues.h" warnings for `Ember.*`).
5. `Tools\runtests.ps1`: new `Looter.Embers.Power.LuckyStreak/.FiftyTwoPickup/.DoubleOrNothing/.TollChain/.GraveWard/
   .LastRites/.SlagBomb/.IronHide/.RailSpike/.Spyglass/.Tripwire/.LongShot`; changed `Looter.Heroes.Playable`,
   `.Character`; and everything touching bullets and health (the hooks are no-ops with no listener).
6. Play: `Looter.Hero.Unlock 1` + `Looter.Hero Odessa` (etc., no session) then `Looter.Embers.Use <Id>`; `perf.ps1`
   with a pool, a ward and a spyglass sweep up.
7. Docs: CODEMAP and Pipeline text below; Handoff.

## Each power, tick by tick
- **Lucky Streak** (8 s): Activate binds OnBulletFiring/OnBulletLanding, fans 14 cards from the hand, gold sparks, Start
  cue. Each of the player's pulls (frame + gun) counts; every 3rd pull's pellets get `bSureCritical`, a gold, wider tracer,
  a muzzle glint, the Lucky cue. Each landing on a living creature (one per frame) sends a ricochet at the nearest other
  living creature within 12 m in sight of the hit: a gold streak homing at 6500 cm/s on the linger, 14 x level on arrival
  (point damage), card flick, sparks, Ricochet cue (3D). End unbinds; flying ricochets still land (cut short: dropped).
- **Fifty-Two Pickup** (instant): 52 cards laid in rows/columns over a 35 x 12 degree fan from the hand, 3600 cm/s +-10%,
  spinning. Each linger frame: a card's segment vs every candidate creature's capsule (5 cm reach) and a world-static trace;
  a body: card spent, its 8 added to that creature's sum for the frame; the world: it sticks 1.4 s glinting; past 14 m: gone.
  Each creature hit that frame takes its sum as one hit, one Hit cue (louder with more cards), ink-red sparks.
- **Double or Nothing** (6 s): binds OnBulletFiring. Each pull tosses: heads (50%, or forced after a tail: never two misses
  running) = every pellet x2 damage, green wider tracer, green glint, Double cue; tails = every pellet turned 5-9 degrees off
  its line and `bBlank` (0 damage, no hit shown on bodies), dull tracer, a grey wisp, Nothing cue. A coin flicked up and
  caught on use (linger).
- **Toll Chain** (0.5 s): CanActivate refuses without a living creature within 18 m, 4 degrees (or its width) of the aim,
  in sight. Tick: 0-0.13 s the chain head flies hand to chest; then the bite: 20 x level, Hook cue, sparks, the view pulled;
  not a boss: Daze(rest of the haul + 1.5 s) (brain held, sparks over its head), then hauled eased-in-out along the ground
  (ground traced each step, a capsule sweep that ignores pawns stops it at anything solid) to a stride in front of him
  (shorter for Rare/Epic/Legendary/size), dust trailing, a dust ring and a jolt on arrival; the chain drawn as links, then
  snapped back over 0.18 s (linger). A boss: bitten only.
- **Grave Ward** (8 s): CanActivate needs ground a step ahead or underfoot. Plants SM_RR_Mine_Lantern x1.3 there (lit,
  14 cd, pale green), its 48-point ring laid on the ground once, Plant cue, Hum loop on it. Each tick: every living,
  non-boss creature flat within 5 m (and 3 m of height) gets CustomTimeDilation 0.5 if its clock is 1 (re-taken after a
  hit-stop), one that leaves or dies gets 1 back if still 0.5; the player standing in it and hurt heals 5 x dt; motes in
  the light, off the slowed, off the mending player; the light flickers. End: every clock back, Out cue, smoke, lantern gone.
- **Last Rites** (8 s): CanActivate needs a living creature in sights within 40 m. Marks it: sigil actor (the LastRites
  icon, seconds left), pale wash, a turning ring at its feet, Mark cue. Each tick: dead (by anything) -> Burst: every other
  living creature within 4.5 m (to its body) in sight of the corpse takes 60 x level and is thrown 2.2 m, dazed 0.5 s (not a
  boss), soul-light burst, a ring racing out, Burst cue; then `UEmberComponent::Finish()`. Alive at the end -> Strike: 90 x
  level, a column of light, flash, dust ring, Strike cue. Interrupted: the mark just fades.
- **Slag Bomb** (7 s): CanActivate needs a landing: the creature in sights (5 degrees, 20 m) -> its feet; else where the aim
  meets the world (a wall: the ground under it); else 20 m out, down onto the ground. Lob: a parabola over the straight
  line, 0.45-1 s by distance, top 1.2 m + 12% of it, traced each frame (bursts early on the world), a molten ball, sparks,
  smoke. Burst: 48 within 2.5 m, 26 within 4.5 m (to the body's side, in sight), flash, dirt, 44 molten globs, Burst cue.
  Pool 6 s: who's in it (flat within the radius, feet within 1.6 m x size) is noted every frame; every 0.5 s, all in it or
  out under 0.6 s burn 10 x level, worn orange (flaring per burn), Sear cue, sparks; a glowing edge, crusting blobs,
  rising sparks and smoke, a light, the Pool loop. End: tint off, loop stopped, smoke. If the power's time runs out
  with the pool still hot (only a lob landing late could), End bursts a bomb still in the air and lets the pool burn out
  its own time on the linger; cut short (death, scene), everything goes at once.
- **Iron Hide** (6 s): player's health `SetDamageTakenScale("IronHide", 0.4)`; `UMovementSlowComponent::Apply(0.75,
  0.15 s)` every tick; binds the player's OnDamaged: every blow -> Clang cue, sparks where it landed; a blow whose causer is
  a living creature (a bite) -> the biter takes 12 x level (once per 0.3 s per biter), Burn cue, sizzle sparks, a wisp. Lock
  cue, steam vents, a heavy view dip on use; steam every 0.45 s; End: scale 1, unbind, Unlock cue, steam, view lift.
- **Rail Spike** (instant): the line from the aim's start to 25 m or the first world-static hit; every living creature whose
  capsule comes within 60 cm, nearest first. Fire cue, steam, flash, a hard view kick. The spike flies 28000 cm/s on the
  linger (seen converging from the forearm onto the aim line, steam laid every 70 cm); as it passes each: 60 x level, a
  1.5 m shove along the line (dizzy 0.3 s, not a boss), sparks, steam, Hit cue. At a wall: Embed cue, ricochet sparks,
  dust; it glows there 2.5 s cooling. In open air it tumbles down.
- **Spyglass** (10 s): Start cue. Each tick: the ring's radius eases 0 -> 60 m over 1.2 s round the player's feet (56
  points traced to the ground per frame, cyan, fading at its reach); every creature inside it is marked (then every 0.5 s,
  anything within 60 m): health `SetDamageTakenScale("Spyglass", 1.3)`, mark actor (diamond, distance), orange wash, a ping
  (rate-limited, rising pitch). The dead let go at once. The lens: view narrowed 5 degrees and the cameras' vignette to
  0.85 through the sweep, cleared over 0.6 s. End: every mark and scale undone, End cue.
- **Tripwire** (20 s): CanActivate needs a plan of at least 1.5 m: centre where the aim meets walkable ground within 15 m,
  else the ground 4 m ahead; each half out 3 m across the aim in 25 cm steps over the ground (rise/drop limit, a 5 cm sweep at
  wire height). Two SM_FencePost stakes (0.45 x 0.42 scale), dust, Set cue, a shiver. Each tick: creatures near the wire:
  their feet's path since last frame crossing it (2D) or standing on it, within 1.2 m of its height -> trip (once per wire):
  30 x level, `Knock` 1.4 m along their going, stun 2 s once landed (bosses: hit only), the wire shivers, Trip cue, dust. A
  glint runs along the wire. End: stakes pulled, the wire sags and fades, Slack cue.
- **Long Shot** (8 s): binds OnBulletLanding. On use marks up to 8 living creatures within 30 degrees of the aim, 60 m, in
  sight. Each of the player's landings: damage x (1 + 0.15 per 10 m flown, at most 2); a creature hit from 25 m or farther is
  marked; a marked creature hit is always critical (its mark flares); a tink (pitch and volume up with the bonus, at most
  every 0.06 s) and teal sparks sized by the bonus. The dead's marks retire each tick; End unbinds and retires all.

## Decisions (mine, unattended; each is data or a constant, easy to change)
- **Balance against Ellis's three and today's numbers**: the demo's damage was set against a 14 rifle round; today's level 1
  Common rifle deals 20 (FGraveSaltRules' comment), spiders have 300 health, the grenade 900. So damage is the demo's x1.43
  (rounded) unless noted. Per power (old -> new): Lucky Streak ricochet 10 -> 14; Fifty-Two Pickup 6 -> 8 a card (+ new
  `FanTall` 12); Double or Nothing cooldown 18 -> 15 and new `MissStreak` 1 (even odds at x2 pay nothing on average; never two
  misses running makes it ~4/3); Toll Chain 12 -> 20 (one round; Dust Devil's 15 hits everything by its path); Grave Ward heal
  4 -> 5 a second (40 over 8 s vs Borrowed Time's 8 s at the same 24 s cooldown, only while standing in it); Last Rites 30 ->
  60 burst, 45 -> 90 strike (x1.43 then up: both pay only after a wait or a kill); Slag Bomb 34/18 -> 48/26, burn 7 -> 10,
  duration 6 -> 7 (flight + the 6 s pool, new `PoolSeconds` 6); Iron Hide bite 8 -> 12; Rail Spike 40 -> 60 (+ new `Shove`
  150, `Dizzy` 0.3); Tripwire 20 -> 30; Long Shot 10% -> 15% per 10 m (fights are 20-30 m; 10% was hardly felt), new
  `Range` 6000, `ConeDegrees` 30, `MarkFrom` 2500. Spyglass unchanged. All in create_ember_assets.py with a why-comment.
- **Long Shot's "marked"**: one power runs at a time and a swap keeps the cooldown, so Spyglass's marks can never meet Long
  Shot. Long Shot marks its own: what's in his sights as it starts, and anything he hits from 25 m.
- **Lucky Streak's "one shot in three"**: every third trigger pull (a rhythm the player can feel), not a 1/3 roll; a
  shotgun pull's pellets count as one; one ricochet per frame (a blast's pellets send one). Ricochets are the power's own
  homing streaks with point damage (UEmberDamageType, no gun notch), not bullets.
- **Double or Nothing**: rolled per pull (all pellets alike); "nothing" is a real miss (thrown 5-9 degrees wide and blank),
  not a vanished bullet.
- **Toll Chain** targets by aim cone (4 degrees, or the body's width + 45 cm); the haul is the power teleporting the
  creature along the traced ground while Daze holds its brain; bosses only take the bite.
- **Grave Ward** slows by the creature's own clock (CustomTimeDilation: steps, attacks, pose alike, as Sundown does the
  world's) rather than its walk speed (creatures set their speed each frame, so a speed cap would be overwritten).
  The lantern is the mine's (Crane's model has no lantern); `SM_RR_Mine_Lantern` x1.3.
- **Last Rites** ends early on a death (`UEmberComponent::Finish`, new) so the HUD arc stops with it.
- **Iron Hide**'s "biter" = a hurt whose damage causer is a living creature (ACreatureBase::HitWithAttack passes itself);
  globs and other causers only clang. The cut uses a new UHealthComponent damage scale (also used by Spyglass).
- **Spyglass**: the mark through walls is the screen-space mark widget; the "orange silhouette through walls" needs a
  custom-depth post-process material (an asset): not made. What's there: an orange overlay wash where the creature is seen.
  The lens vignette is the cameras' own post-process override, restored exactly.
- **Tripwire**: strung across the aim; each creature trips once per wire.
- **Icons**: drawn in Art/Icons/InkedIcons.py in Ellis's box and ink: Lucky Streak two fanned cards + spark, Fifty-Two
  Pickup a five-card fan, Double or Nothing a struck coin with a star before an empty ring, Toll Chain links and a hook, Grave
  Ward a lantern with rays, Last Rites a ringed cross on a plinth, Slag Bomb a cracked molten ball with a fire trail, Iron Hide
  a riveted shield, Rail Spike a spike with speed streaks and steam, Spyglass a drawn spyglass with a glint, Tripwire two
  stakes and a plucked wire, Long Shot a reticle with a bullet streak. Checked at 36 px (the socket's size).
- **Sounds**: 35 cues (names and descriptions in LooterSoundCues.h). 2D for the hero's own, 3D for the world. Frequent
  ones (Lucky, Double, Nothing, Spyglass.Mark, LongShot.Hit, Ricochet, FiftyTwoPickup.Hit, Clang, Sear) kept quiet and
  mostly off 2-5 kHz (Ricochet's whine is still 63-71% presence; it's a whistle). All check-clean.
- Colours are the definitions' (create_ember_assets.py), not added to LooterUI::Color (the task's "colour is data");
  the mark widget's own pieces use the kit (MarkerBrush, GlowBrush, floating text, Color::Text).
- Meshes loaded by path at run time: `/Game/Art/Mine/SM_RR_Mine_Lantern`, `/Game/Art/Props/SM_FencePost` (both placed in
  Ransom's Rest, so they cook with it; if a later level lacks them, add their folders to DirectoriesToAlwaysCook).
  A missing mesh only hides the prop; the power still works.

## Compile risks (written blind)
A compile reviewer (an Opus agent; `REVIEW.md`) read the whole tree against the project's headers and UE 5.8's source. Fixed
since: `#include "Core/LooterCharacter.h"` in EmberPowerBase.cpp and every power .cpp (FEmberContext::Character is
`ALooterCharacter*`, forward-declared in EmberPower.h, and converts to `ACharacter*` only when complete); Lucky Streak's and
Double or Nothing's `LastPullWeapon` is a `TWeakObjectPtr<AWeaponBase>` with `Weapons/WeaponBase.h` included; `class AActor;`
in EmberPowerBase.h and EmberIronHide.h; `FEmberTint::SetMaterial` out of line (the weak pointer's assignment needs the
complete type); Toll Chain's own flag renamed `bChainOut` (it hid UEmberPower's private `bRunning`); the mark actor's and
widget's declaration parameters renamed (`InTarget`, `InWords`); the slag pool on End(false) (above); BulletSubsystem.h's
comment on blanks now says OnBulletHit still reports a blank's landing with 0 damage (the Double or Nothing test reads it).
The reviewer found no UHT, override, delegate, signature, container, shadowing or test-overload problems. What's left:
- New `.generated.h` for 16 UCLASSes; `UEmberPowerBase` is Abstract and inherits `Within = EmberComponent`.
- Nested plain structs inside UCLASS bodies (`UEmberPowerBase::FDealt` public, others private) as Dust Devil does.
- `static const FName ScaleSource` members (IronHide, Spyglass) defined in their .cpp.
- Delegates with reference parameters (`FOnBulletFiring(FBulletShot&)`) bound with `AddUObject` to non-UFUNCTION members.
- Implicit double -> float conversions in a few places (as the project already has); no brace narrowing found.
- `TMap`/`TArray` of `TWeakObjectPtr<ACreatureBase>` looked up with raw pointers.
- `Looter.Heroes.Playable` (every power has code) and `.Character` (Refused) pass only after create_ember_assets.py has
  set the twelve PowerClasses (run order step 3 before step 5).

## For CODEMAP.md (Main pastes under "## Embers" and the sound list)
- `Embers/Powers/EmberPowerBase`: `UEmberPowerBase`, the twelve new powers' footing: the use's body and numbers, `Hurt` (the
  contract's damage, logged for tests), aim, `HandPoint`, `FindInSights`, `CanSee`, `FindGround`, `IsBoss`, `DistanceToBody`,
  the linger frame chain. `Embers/Powers/EmberFX`: sparks, ground rings, cards, `FEmberTint` (overlay wash).
  `Embers/Powers/EmberPropActor`: `AEmberPropActor`, a baked prop a power sets down (lantern, stakes), optional light.
- `Embers/Powers/EmberLuckyStreak`, `EmberFiftyTwoPickup`, `EmberDoubleOrNothing` (Odessa), `EmberTollChain`,
  `EmberGraveWard`, `EmberLastRites` (Crane), `EmberSlagBomb`, `EmberIronHide`, `EmberRailSpike` (Gauge), `EmberSpyglass`,
  `EmberTripwire`, `EmberLongShot` (Pike): one power each (README's lines, or each header's comment);
  `EmberSlagBombLook.cpp` the slag pool's look.
- `UI/World/EmberMarkActor` + `EmberMarkWidget`: a power's screen-space mark over a creature (diamond and distance, or the
  power's icon and seconds).
- Tests: `EmberHeroPowerBench.h`, `EmberOdessaTests.cpp`, `EmberCraneTests.cpp`, `EmberGaugeTests.cpp`, `EmberPikeTests.cpp`
  (`Looter.Embers.Power.<Id>`).
- `Combat/BulletSubsystem`: add "`OnBulletFiring`/`OnBulletLanding` (the powers' say over shots; `bSureCritical`, `bBlank`)".
  `Combat/HealthComponent`: add "`SetDamageTakenScale` by source". `Embers/EmberComponent`: add "`Finish`".
- `Art/Sounds/recipes/embers_odessa.py`, `embers_crane.py`, `embers_gauge.py`, `embers_pike.py`: the twelve powers' 35 cues.

## For Docs/Pipeline.md, "Ember powers" (Main condenses)
- The twelve: as "Each power, tick by tick" above (one bullet each), the hooks (BulletSubsystem's two delegates, the health
  damage scale, `Finish`), the mark actor, the prop actor, the icons and the cues' list.
- The hero pick section: ".Playable expects only Ellis while the twelve powers have no code" is now: every power has code;
  Odessa, Crane, Gauge and Pike wait only on SK_<Id> and RTG_MannyTo<Id>.
- `Looter.Heroes.Screen` still asserts "Odessa is in the works": it will fail the day SK_Odessa and RTG_MannyToOdessa land.

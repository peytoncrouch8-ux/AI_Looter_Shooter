# Pipeline: Ember powers

Part of the pipeline notes (index: `Docs/Pipeline.md`). Rules: `CLAUDE.md`.

## Ember powers

The heroes' active powers (Docs/Story.md, "The heroes"; the user's calls 2026-10-15: three owned from the start, one
equipped, Q, the HUD socket by the weapon column, the powers page as the inventory's second tab). The contract the first
round was built to: the shared headers in `Source/AI_Looter_Shooter/Embers/` (EmberTypes, EmberDefinition, HeroDefinition,
EmberPower, EmberComponent); ids, colours, cues and file ownership in the round's `Saved/Embers/CONTRACT.md` (not
committed; its content is in this section once the round lands). The agents' notes follow.

- **Ids (never change: the session saves the equipped one by it):** `DustDevil` (Wind, #e8f6ff), `Sundown` (Dusk,
  #ffb24d), `BorrowedTime` (Debt, #c9b4ff); hero `Ellis`. Cues: `Ember.DustDevil.Use`, `Ember.Sundown.Start`/`.End`,
  `Ember.BorrowedTime.Start`/`.Settle`, `Ember.Ready` (off cooldown), `Ember.Denied` (a press that did nothing).
- **Making a power:** (1) a `UEmberPower` subclass in `Embers/Powers/` (CanActivate, Activate, Tick while it lasts, End
  with bInterrupted; it leaves the world as it found it, and documents which definition fields it reads); (2) its
  definition `/Game/Data/Embers/DA_Ember_<Id>` written by `Tools/Unreal/create_ember_assets.py` (POWERS list: id,
  words, colour as sRGB hex, Inked icon name, cooldown, duration, Damage/Range/Radius/Strength, Extras by name,
  `power_class` = `/Script/AI_Looter_Shooter.<Class without U>`); (3) its place in the hero's list (HEROES: the first
  is equipped in a new game). Run the script after the C++ is built (`Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/
  Tools/Unreal/create_ember_assets.py"`): it prints `EMBERS ... made/updated` and `EMBERS done`, rewrites every field
  it names, leaves a missing class empty with a warning (rerun once built).
- **The component** (`UEmberComponent` "Embers" on ALooterCharacter, like the grenade's "Throw"): the hero is the
  Blueprint's `HeroDefinition` or `DA_Hero_Ellis`; one power object per owned power, made at BeginPlay (lazily on first
  use too, so a restore before BeginPlay works). Clocks in its own tick (game time; off while Ready): `CooldownLeft`
  from the use (through the duration), `ActiveLeft` for the duration; an instant power (duration 0) ends inside its use.
  The gate's order: NoPower (none equipped, or no class), Dead, Busy (scene, pause, menu, mantle/vault, gun swap, melee
  strike, grenade throw), Active, Cooldown, then the power's CanActivate (a power answering Cooldown/Active reads as
  Refused). A denied press plays `Ember.Denied` (not for Dead or Busy) and fires OnDenied. Death or a scene interrupts a
  running power at the next tick; EndPlay interrupts too. A swap mid-cooldown keeps the wait (held to the new power's
  cooldown); no swap while one runs.
- **Damage:** `UEmberDamageType`, applied with the player's controller as instigator and NO causer: AwardKill gives XP
  through the instigator, `AWeaponBase::FindKillWeapon` finds no gun without a causer (no notch).
- **The key:** binding `Ember`, "Ember power" under Combat, Q, the left bumper (fixed on pads), in the character context
  (`UKeyBindingSubsystem::AddEmberAction`, `GetEmberAction`); bound on Started.
- **The save:** `ULooterSessionSave::bHasEmbers` + `Embers` (`FEmberSave`: equipped id, cooldown left), captured and
  restored beside the grenades (`SessionSubsystemWorld.cpp`). An older save reads as none captured: the first power,
  ready; an id the hero no longer owns gives the first; a cooldown is held to the power's own.
- **Console:** `Looter.Embers.Use [id]` (equips the named one, ends a running one, fires past the cooldown; the body's
  gate still applies), `Looter.Embers.Equip <id>`, `Looter.Embers.State` (hero, owned with numbers and class, state,
  clocks, what blocks a press). Tests: `Looter.Embers.Component` (a bare actor in a test world, a probe power
  `Tests/EmberProbePower.h`, clocks ticked by hand), `Looter.Embers.Keys`.
- **The HUD socket** (`UHudEmberSocketWidget`, bottom-right LEFT of the weapon slots, its bottom level with the last
  slot's, 34 px off them: the slots' 12 px gap plus the 22 px the slot in hand reaches left as it lifts). It is NOT in
  the weapon cluster (which hides with no gun in hand; Q works unarmed): a row of its own on the HUD canvas at the
  cluster's anchor, the socket then a spacer as wide as the Arms row (slot row 122 + 14 + cartridge 51), lifted over the
  cluster's two lines (52 px); it takes the cluster's fade while the cluster shows. A change to the slots' or the
  cartridge's widths must change `ArmsWidth` in PlayerHUDWidgetLayout.cpp too. The slots' 64 px ring
  (disc a background, rim, gunmetal, hairline; `HudEmberSocketWidgetPictures.cpp`, the slots' radii), the power's Inked
  icon in a 36 px box, the Ember key's tab under it (grenade-style plate; wider per extra letter, `TabWidthFor`), the
  power's name (10 px) under that. Ready: the orange ring and a glow in the power's colour beating 0.18-0.6 over 2.4 s.
  Running: an arc in its colour from 12 o'clock round the gunmetal, shrinking with `GetActiveFraction`, the name in its
  colour. Cooling: a dark pie (`Color::Track` 0.8) covering the share still to wait, drained clockwise, the seconds
  (rounded up, never 0) in white 20 px, the icon greyed. The pie is one `MakeLines` stroke as wide as the radius (less
  1.5 px at the middle, so its inner edge never folds); pie, arc and seconds are drawn in `NativePaint` with the style's
  opacity, so they fade with the cluster (a `ULoadoutPaintLayer` would not: its painter gets no widget style). Denied
  Cooldown/Active: a 0.4 s shake (4 px); the rest a 0.35 s dim; no sound (the component plays `Ember.Denied`). Back to
  Ready (OnStateChanged): a 0.35 s pop (scale 1.18, the ring flashing white). `PlayerHUDWidgetEmbers.cpp` binds the
  pawn's component each frame like the grenade's; a denied press or a state change brings the column forward.
- **The powers page** (`UEmbersWidget`, `EInventoryPage::Embers`, the inventory's second tab "Embers"; the Ledger is
  third): three 440 x 560 cards side by side (nature chip in the power's colour, a 132 px medallion ringed in its colour
  with the 92 px icon, name, description, cooldown and duration via `SecondsWords`, the state line), the equipped one
  with an orange EQUIPPED tag and its colour's glow; the chosen card's edge orange. A / D, arrows, D-pad (W / S too)
  choose; E, Enter, Space, A, the Interact key or a click equips (`EquipIndex`); a refusal while a power runs says
  "<power> is running: it finishes before you can swap" with `UI.Denied`; an equip plays the loadout's `UI.Equip`; the
  choice moving plays `UI.Hover` (the cards' own sounds are off). Pages: 1-5 (`LoadoutParts::PageForNumberKey`), the
  shoulders to the neighbours (loadout's LB stays Move); the inventory key closes, as on every page.
  `LoadoutParts::NumPages`/`PageTabName` hold the tabs' order and words; the control hints now say "Then 3: the Ledger",
  "Then 4: your missions". `Looter.MenuShots` has an `embers` step, `Looter.HudShots` an `ember` step
  (`Looter.Embers.Use`: Dust Devil dashes the player in the picture).
- **Icons** (`Art/Icons/InkedIcons.py`: `dust_devil`, `sundown`, `borrowed_time`, in the ammo icons' shared box and ink
  width; `EMBER_ICONS` makes them square): Dust Devil three tapered spiral streaks (`offset_band`) round a ringed eye;
  Sundown a half sun on a horizon bar, four tapered rays, two bands cut across the sun; Borrowed Time an hourglass
  between end plates, the lower bulb shaded with a heap and a stream, a slanted ledger rule through the waist with its
  edges cut so it reads over the glass (posts and a top sand line were dropped: the posts thickened the ink at 30 px,
  the line made a face). The generator also writes `InkedIconData::Find` (every icon by name), behind
  `LooterUI::FindInkedIcon`, so a definition's `IconName` finds its icon. Tests: `Looter.Embers.Pages`, `.Socket`.
- **Dust Devil** (`UEmberDustDevil`; reads Damage, Range, Radius, Extras FlingMin/FlingMax/Dizzy/FovKick, the duration
  as the dash's time): the dash is laid out first (`PlanDash`, 30 cm steps): each step's ground found by a world-static
  trace (`LooterWorld::StaticGeometryParams`) within a step's rise plus a 45-degree slope of the step, else it stops at
  the edge; a capsule sweep (the player's own responses, pawns, physics bodies and destructibles ignored; lifted half a
  step, radius 0.9) stops it short of walls, the playable area's included; `APlayableArea::Contains` too. Under 1 m:
  refused. A start high in the air keeps its height and ends falling. Run on the component's Tick (the player's clock):
  MOVE_None while the capsule is teleported along the path (an even pace blended 0.4 with an ease-out: fastest as it
  leaves, 0.6 of the mean speed as it arrives), Velocity set to the dash's for the animation, bInvulnerable on (only if
  it was off), then walking on at 700 cm/s. End(false) runs the rest at once; End(true) stops where it is. Creatures
  within Radius (flat) and 220 cm of height of the stretch just run, in sight of it (world-static trace), are struck
  once: damage, then `UCreatureKnockComponent::Knock` along their side of the path (+0.2 of the dash's way), 400-500
  cm, arc top 110 cm. Felt: `UPlayerViewComponent::SetPowerFieldOfView` (up over 0.07 s, back over 0.45 s after), 80
  speed lines a second (world streaks 200-650 cm ahead, 70-190 cm round the eye, never low, 0.16-0.24 s), a 16-puff dust
  ring with grit and five spinning wisps at the start, 9 at the end, a dust puff every 110 cm along the path, per
  creature a dust burst, grit, `Player.Melee.Hit` and a small view jolt; a lean kick at the start, a dip at the end. What
  lingers after (the settle, the lines) runs on a next-frame timer chain of its own.
- **Knocked and dizzy** (`Creatures/CreatureKnockComponent`, a default subobject "Knock" on every `ACreatureBase`;
  `TickBrain` waits while `HoldsBrain`, as for a stagger): `Knock(Away, Distance, Height, Dizzy)` launches like a hop
  (`LaunchVelocity`: up for the arc's top, across for the distance in that air time; its speed replaced), the distance
  times the rank's share (Rare 0.85, Epic 0.7, Hallowed 0.6, Ethereal 0.5, Legendary 0.45, Boss none) over the square root of its size; the landing
  must have ground within 110 cm over / 150 cm under its feet (sizes scale it) and inside the playable area, else 0.75,
  0.5, 0.3 of it, else only dazed. The body cartwheels once over the planned flight (the mesh turned round the
  capsule's middle from its captured rest transform), lands (LandedDelegate, the floor found, or 2.5 s) with a dust ring,
  grit and a dull `Player.Melee.Hit`, then reels for the daze (at least 0.2 s): the tumble settles upright over 0.15 s,
  a 9-degree sway at 1.6 Hz, three gold sparks (FWeaponFX streaks) circling 24 cm out over the head, drawn only within
  50 m of a view. Death, EndPlay or the end of the daze put the mesh back. Bosses and the dead are never knocked or dazed.
- **Sundown** (`UEmberSundown`; Strength = the world's speed, Extras Pitch 0.75): `SetGlobalTimeDilation(Strength)`;
  the player's actors get CustomTimeDilation = 1 / the world's effective dilation: the pawn, everything attached (each
  Tick, for a gun swapped in), the controller (the look), the camera manager (the kicks) and the HUD actor.
  `UBulletSubsystem::Tick` advances each bullet by the shooter's CustomTimeDilation, so the player's shots fly at full
  speed. The component ticks on the pawn, so its 6 s are the player's. `SetGlobalPitchModulation(Pitch, 0.35 s)`, back to
  1 at the end. Refused while time is already bent (a boss's slow-motion death). End undoes only what is still its own
  (the world's dilation if still the one it set; each actor's if still the compensation); another slow taken meanwhile is
  followed (the compensation recomputed each Tick). `AWeaponBase::GetOwnerTimeScale` (the holder's CustomTimeDilation)
  divides the fire-rate and reload timers' seconds, so a gun in the player's hands fires and reloads at full speed; the
  melee and throw cooldowns still read world time (short, left as they are).
- **Borrowed Time** (`UEmberBorrowedTime`; Strength = the share a kill strikes off, Extras PayBackSeconds 3): binds the
  player's `UHealthComponent::OnDamaged` and `OnHealthChanged` (the last known health tells what a hurt really took) and
  the progression's `OnKillXP` while it runs. A hurt is healed back inside OnDamaged, before the health component counts
  a death, so a killing blow is borrowed too; the debt grows by the damage. `NoteKill` strikes the share off (a violet
  soul-light flare at the one killed). End: death forgives it; otherwise (a debt of 0.5 or more) it's drained evenly
  over PayBackSeconds with `Drain` (never the last point), on a next-frame timer chain at the player's clock, with
  `Ember.BorrowedTime.Settle`, a heavy view dip and a 0.25 shake for the pay-back. Violet motes rise round the feet
  while it runs; a violet puff where each hurt is taken back. The HUD reads `GetDebt`, `GetDebtFraction` (of the most
  health), `IsSettling`; after End the object is `UEmberComponent::GetPowerFor("BorrowedTime")`.
- **The powers' sounds** (`Art/Sounds/recipes/embers.py`, group `embers`; all 2D): Dust Devil a two-ear gust, whump,
  grit and a receding whirl; Sundown a tape-stop chord sinking an octave, a slowed bronze chime, a drone and a heartbeat
  thud (End: the chord wound back up, a shimmer); Borrowed Time a ledger's cover, a nib, a clock tick and a cold minor
  chord (Settle: the ledger slammed, three slowing ticks, the chord sinking flat); Ready a breath of flame and a chime;
  Denied a fizzle and a muted click. Tests: `Looter.Embers.Power.DustDevil`, `.Sundown`, `.BorrowedTime`
  (`Tests/EmberPowerTests.cpp`: definitions made in the test; the dash stepped by hand; a test level has no game mode,
  so the damage itself isn't checked there; hurts are broadcast as `AActor::TakeDamage` hands them out).
- **The twelve** (Odessa's, Crane's, Gauge's, Pike's; built 2026-10-20 by the POWERS agent, its notes in
  `Saved/Powers/README.md`, not committed). All on `UEmberPowerBase` (`Embers/Powers/EmberPowerBase`): damage through
  `Hurt` (UEmberDamageType point damage, no causer, logged in `GetDealt` for tests), the target `FindInSights` (nearest the
  crosshair: within the cone or the body's width + 45 cm, in sight), lingering looks on a next-frame chain on the player's
  clock (`StartLinger`/`Linger`; a test level never ticks: each power's `Step...` is public). Hooks they added:
  `UBulletSubsystem::OnBulletFiring(FBulletShot&)` (a listener changes the shot: damage, way, tracer, `bSureCritical`,
  `bBlank`: 0 damage, no hit shown on bodies, no gun marker, OnBulletHit still says where) and `OnBulletLanding` (before the
  roll: `FBulletLanding::bCritical`, `DamageScale`); `UHealthComponent::SetDamageTakenScale(Source, Scale)` (multiplied,
  before everything sees the hit); `UEmberComponent::Finish()`. Marks over creatures: `AEmberMarkActor` (screen-space, so
  through walls; diamond + distance, or the power's icon + seconds). Props: `AEmberPropActor` (`SM_RR_Mine_Lantern` x1.3,
  `SM_FencePost` scaled 0.45/0.42 as stakes; loaded by path, no collision). Numbers: the demo's damage x1.43 (its rifle was
  14, today's 20), the rest per entry in create_ember_assets.py. Each:
  - Lucky Streak: every 3rd pull (a pull = one frame's pellets of one gun) sure-critical with a gold tracer; each hit on a
    creature (one per frame) sends a homing gold ricochet to the nearest other creature within 12 m in sight, 14.
  - Fifty-Two Pickup: 52 cards over a 35 x 12 degree fan, 3600 cm/s, 14 m; a card's segment vs capsules each frame; a
    creature's cuts in a frame summed into one hit (8 each); world hits stick 1.4 s (round 3: real cards, "Odessa's cards" below).
  - Double or Nothing: per pull, heads x2 (green tracer) or tails: every pellet turned 5-9 degrees and blank; never two
    tails running (`MissStreak` 1), so about 4/3 of the damage.
  - Toll Chain: 4-degree sights to 18 m (else refused); chain out 0.13 s, bite 20, `Daze` for the haul + 1.5 s, hauled
    eased along traced ground (a capsule sweep ignoring pawns stops it) to a stride off; shorter by rank/size; bosses only bitten.
  - Grave Ward: the lantern a step ahead; creatures flat within 5 m (and 3 m height) get CustomTimeDilation 0.5 while their
    clock is 1 (re-taken after a hit-stop, given back on leaving); the player inside mends 5/s.
  - Last Rites: mark in sights to 40 m (sigil, pale wash); dies within 8 s (by anything) -> 60 to every other creature
    within 4.5 m in sight, thrown back, then `Finish`; lives -> struck for 90 at the end.
  - Slag Bomb: landing = the creature in sights' feet, else the aim's world hit (ground under a wall), else 20 m out; a
    parabolic lob 0.45-1 s; 48 within 2.5 m, 26 within 4.5 m; a 6 s pool burning 10 every 0.5 s (and 0.6 s after leaving),
    worn orange; duration 7 s, a pool still hot at the end burns out on the linger. The pool is a model since 2026-10-20
    (`Art/Models/Props/SlagPool.py`, POLISH-ART; the contract `Saved/Polish/SLAGPOOL.md`, not committed): SM_SlagPool on
    the anchor prop (`AEmberPropActor::GetModel`), XY Radius / 50 (9), Z 5 (at 9 its plates stood 13 cm), tilted to the
    plane through the edge's ground ring (Newell's normal; level where that plane tilts past 60 degrees), its pivot on that plane over the
    burst (held -5..+15 cm of it), random yaw, no shadow; SM_SlagPoolCrust on it (`AddPiece`) rising from -0.9 to 0
    (model cm) by smoothstep(0.2, 0.9, cool); 2-3 SM_SlagSplatA/B splashes 0.8-1.5 at the rim, each on its own ground and
    normal. One dynamic instance per glowing slot (SlagMolten, SlagWhiteHot, SlagGlowEdge on M_StylizedSurface), shared
    by the pool and splashes, starting from the slot's own Color and Glow: Glow x lerp(0.15, 1, Heat) x the last 0.6 s's
    fade (x 0.93-1 flicker but the edge), Color lerp(#5A1406, own, Heat), Heat = 1 - cool^1.5; it sinks 2 cm (model) over
    its last 0.6 s. It replaced the glowing edge and the blob streaks (still drawn when the models are missing); sparks,
    smoke, haze and the High/Epic light stay. Reads as molten slag on Medium with no light (`slag_med_*.png`), lit on
    High (`slag_high_*.png`).
  - Iron Hide: the player's damage x0.4 ("IronHide"), `UMovementSlowComponent` 0.75 renewed each tick; a hurt whose causer
    is a living creature burns it for 12 (once per 0.3 s each); every blow clangs.
  - Rail Spike: `PlanLine` along the aim to 25 m or the world; every creature within 60 cm, hit as the 28000 cm/s spike
    passes: 60, shoved 1.5 m; embeds glowing 2.5 s. Its looks are `EmberRailSpikeLook.cpp` (2026-10-20 polish: its line
    runs straight away from the eye in first person and from the camera behind, so a streak along it showed end-on as
    nothing): a round glow on its head (`FWeaponFX::AddGlow`, 16 cm + 1.2% of its distance, so it keeps a size on
    screen), shock rings square to its way (`EmberFX::DrawRing`; at 1.6 and 2.4 m out as it leaves, 22 and 36 cm: nearer
    and wider they filled the first-person view; one at each body, 95 cm; one on the surface it embeds in), a steam
    doughnut at 1.2 m, a helix of hot air round the line (from 1.1 m, a turn per 90 cm, 8 -> 24 cm, 0.5 s, glow 16,
    3.5 cm wide: thinner and dimmer it vanished in daylight), rings of steam every 2.6 m, sparks burst out sideways at
    each body (square to the line they read from behind) and its spill thrown out the far side, the kick Pitch 1.6, Roll
    0.4-0.7, FOV +4.5%. The ground under the line (traced every 1 m from his feet on): dust thrown out both sides as it
    passes where the line is within 3.2 m of it, grit within 1.2 m, and a scar (`AGroundCrack`, 16 cm, iron-orange seam)
    torn open behind it along the first stretch within 1.5 m, 2 m past his feet on, that holds 1.2 s and closes; the
    scar, dark lips and a glowing seam running away from the eye, is what reads best from first person. Screenshots
    `Saved/Screenshots/Tools/rs2_fp_*.png`, `rs2_3p_*.png` (before: `tuned_RailSpike_*`).
  - Spyglass: a ground-traced cyan ring to 60 m in 1.2 s marks as it passes (then every 0.5 s within 60 m): damage x1.3
    ("Spyglass"), mark, orange wash; lens: view -5 degrees and the cameras' vignette 0.85, restored exactly. No through-wall
    silhouette (needs a custom-depth post-process material).
  - Tripwire: across the aim where it meets ground within 15 m (else 4 m ahead), 3 m each side in 25 cm ground steps;
    a creature's feet crossing it (2D) or standing on it trips once: 30, `Knock` 1.4 m, stunned 2 s.
  - Long Shot: damage x(1 + 0.15 per 10 m, at most 2); marks what's in his sights (30 degrees, 60 m, 8) and anything hit
    from 25 m; marked = sure critical. (Spyglass's marks never meet it: one power at a time.)
  Icons in `Art/Icons/InkedIcons.py` (Ellis's box and ink). Sounds `recipes/embers_odessa.py`, `_crane`, `_gauge`, `_pike`
  (35 cues; the per-shot ones quiet and off 2-5 kHz). Tests `Looter.Embers.Power.<Id>` (`Tests/Ember<Hero>Tests.cpp`).
  Play check 2026-10-20 (Ransom's Rest village, screenshots `Saved/Screenshots/Tools/<Id>_*.png`): all twelve work in PIE.
  Lessons: code-drawn streaks under ~2 cm vanish in daylight past ~5 m; a power over in a few frames needs something that
  lingers. Tuned then: Slag Bomb's pool (it read as a thin ring: 36 bigger blobs, a 7 cm edge, a low unlit molten haze
  of puffs, more smoke); Iron Hide's body glows forge orange while locked (FEmberTint on the player, 0.12 breathing,
  flaring 0.6 on the lock, 0.45 per blow); Rail Spike leaves a 0.4 s fading glow line and a smoke thread; Tripwire's wire
  1.6 cm at glow 7; Toll Chain's links 3 cm at 9; flying cards drawn x1.35 (replaced on 2026-10-27: "Odessa's cards"). To see a quick power in a screenshot: `slomo
  0.03-0.15` before `Looter.Embers.Use`, `slomo 1` after (console.ps1 calls take ~1.5 s each).

## Round 3 (playtest, 2026-10-27, notes 27 and 6)

- **Cast pose** (note 27, "action skill activation animation"; `Embers/EmberComponentCast.cpp`, `Player/FirstPersonHandsCast.cpp`,
  `FirstPersonHandsActions.cpp` `CastSupport`): in first person a press no longer fires at once. `UEmberComponent::BeginCast` makes
  the power wait `Looter.Embers.CastLead` (0.1 s, never over `MaxCastLead` 0.15, so a power is never felt late; 0 fires at the
  press) while the left hand raises (the gun stays in the right); the power fires at the release; the hand flares open (`.Flare`
  0.07 s, fingers splayed `.Splay` 12 degrees), holds (`.Hold` 0.12 s, drifting on `.Drift` 3 cm), and returns (`.Back` 0.18 s).
  Third person, or a body no local player controls, fires at the press, so every existing test's timing stands. The gate is run at
  the press and again at the release: a power refused then clicks `Refused`. Melee waits while a cast is under way
  (`IsCasting`); a cast can't be started in a melee. `GetCastTime` for tests.
  - The hand by power (`FirstPersonHandsMotion::CastKindOf`, `EHandsCast`): Fling (Fifty-Two Pickup, Slag Bomb, Toll Chain: the
    hand throws), Clench (Lucky Streak, Double or Nothing, Iron Hide, Borrowed Time, Sundown: a fist before the face), Plant (Grave
    Ward, Tripwire: set down), Push (Dust Devil, Last Rites, Rail Spike, Spyglass, Long Shot: the palm presses out). A power not in
    the table pushes. Tests `Looter.Embers.Cast.Pose`, `Looter.Embers.Component`, `.Keys`, `Looter.Player.Hands`.
- **Odessa's cards** (note 6: "shoots tiny particles, needs better animations and bigger card like projectiles"): the
  `EmberFX::FCardDeck` (`Embers/Powers/EmberFXCards.cpp`) draws real cards, not streaks: an ivory face with a red pip, the back
  in the power's colour (Odessa's red), planes on `M_StylizedSurface` instanced meshes, drawn
  `Looter.Embers.Cards.Size` 2.2 x a real card (8.9 x 6.4 cm), tipped back toward the thrower (`.Tilt` 50 degrees) and
  fluttering (`.Flutter` 22 degrees, `FlyingCardTurn`), spinning, with a faint trail in her colour (`Looter.Embers.FiftyTwo.Trail`
  3; `.Glow` 0.6 lets them read in shade). They leave the left hand (`Looter.Embers.FiftyTwo.HandRight` -12 cm right of the eye;
  Lucky Streak's burst -10), stick edge-first in the world (1.4 s, `.WallStick`; sunk `.Sink` 0.35 of the half length) and in the
  bodies they cut (1.2 s, `.BodyStick`, riding the body), then shrink away over the last 0.35 s. `CardRadius` for the hit test
  5 -> 6 cm (the bigger card). A card riding a body that dies or goes (round 4) is shaken loose: it falls like paper, lies where it
  lands and shrinks away by `Looter.Embers.FiftyTwo.Fall` [2 s] (they used to pop out of sight a second after the kill). Lucky Streak's ricochet cards use the same deck at `Looter.Embers.LuckyStreak.CardSize` 0.7 (they
  burst close to the eye). Tests `Looter.Embers.Cards`, `Looter.Embers.Power.FiftyTwoPickup`, `.LuckyStreak`.
- Not checked in play yet: every cast kind's pose, the cards at a spider and at a wall (hands' spots are camera-space guesses; tune
  with the cvars).

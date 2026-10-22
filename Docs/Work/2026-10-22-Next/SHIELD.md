# SHIELD: the player's ward (design and merge notes; SHIELD agent, 2026-10-22)

The user's pick (2026-10-22): a recharging shield over health, found as loot (Borderlands' core loop), so the harder fights
(12.2) feel fair and loot gains a slot. In the world it's a **ward**: a soul-coal in an iron belt box (the **Ward Box**)
throwing a ward of light round its wearer. Code: `Source/AI_Looter_Shooter/Shields/` (holding tree `Saved/ShieldCode`,
`HASHES.txt`). Phase 1 done: code, art script, icon, sound recipes, tests; nothing built, imported or committed.

## Rules (all cvars `Looter.Shield.*`, defaults = `FShieldTuning` fields)
- Order of a hit: scales and defense (as before) -> **ward** soaks all of it while it holds more -> the hit that empties it
  **breaks** it and only `BreakSpill` [0.5] of its overflow reaches health -> an empty ward takes nothing. Any hit (soaked or
  not) restarts the ward's wait. Drains (Hungry's toll) never touch it. `UHealthComponent::Shield` hook (the creature wards'
  way); `GetHitBeingTaken()` tells the ward who hit; `GetLastHealthLoss()` = what reached health (0 when the ward took it all).
- Refill: after `RechargeDelay` unhurt it fills at `RechargeRate` points/s (`ShieldRules::FShieldCharge`). Sounds: Hit
  (throttled `HitSoundGap` 0.08 s, pitch rising as it runs low), Break, RechargeStart (not for a top-up over 85%), Full.
- Health's regen (12.2: 7 s wait, 7%/s) is unchanged, but its **ramp holds until the ward is full** (`HealthWaits` [1]); its
  wait still counts while the ward fills, so with a ward that fills inside 7 s nothing changes (`UPlayerVitalsSubsystem`).
- Death: ward out; respawn: ward full. Load: worn ward full. A ward put on (wear, swap) comes on **empty** and waits its
  delay (no free refill by swapping mid-fight); a dev/starter/loaded ward comes full.
- Starter: the first gun in the player's hands brings a Common Ward Box at the player's level (as the starting grenades;
  `Starter` [1]; saved `bStarterGiven`; an old save gets one on load).
- `Enable` [1]: 0 = no ward takes anything (A/B feel).

## Numbers (`UShieldDefinition` C++ defaults; `create_shields.py` never writes them)
- Common, level 1: **capacity 40** (player health 100), **wait 3.2 s**, **refill 28% of capacity/s** (empty to full ~3.6 s;
  wait + fill ~6.8 s, inside health's 7 s). Each number +-8% seeded. Capacity +8% a level (= player health's, so the ratio
  holds at every level).
- Rarity (weights = the guns' 60/25/10/4/1, colours = the guns'): capacity x1.0/1.15/1.3/1.5/1.75, wait x1.0/0.95/0.9/0.85/
  0.8, refill share x1.0/1.05/1.1/1.15/1.2; trait chance 0/0/0.5/0.85/1.0; trait strength Rare 1, Epic 1.25, Legendary 1.6.
  Level-1 capacities: 40 / 46 / 52 / 60 / 70.
- Builds (from the seed, a third each; name the ward): **Ward Box** (standard), **Bulwark** (capacity x1.2, wait x1.1, refill
  x0.8), **Quickset** (capacity x0.85, wait x0.7, refill x1.15). Name = trait word + build: "Barbed Bulwark", "Reaping
  Quickset", "Ward Box".
- Compare (cards, labels, arrows): `Worth` = capacity + 2 s of refill scaled by 3.2/wait; within 3% = about the same.

## Traits (Rare+; original names)
| Trait (word) | What it does (strength x) | Cvars |
|---|---|---|
| **Last Breath** (Gasping) | Breaking looses a burst of the ward's element (Ember / Soul-light / Grave-salt, seeded) round the player: 260 at its heart x level's enemy scale, whole within 1.5 m, 35% at 4.5 m, seen bodies only; sets the element's status (no roll, at the creature's weakness), knocks the near back; boss capped at 5% of its health. Once per full charge. | `Nova.Damage/Radius/Core/Edge/BossCap` |
| **Barbwire** (Barbed) | A bite or claw (UCreatureAttackDamageType) landing on a charged ward cuts the biter for 80% of the ward's capacity. | `Barb.Share` |
| **Lodestone** (Lodestone) | 40% of hits on a charged ward give rounds for the gun in hand (10% of its magazine, at least 1). | `Lodestone.Chance/Share` |
| **Harvest** (Reaping) | A kill (progression's OnKillXP) puts back 20% of the ward and skips the rest of its wait. | `Harvest.Share` |
| **Saint's Grace** (Graced) | Broken, with health under 30%: the ward flares back to 60% (warm HUD flash, `Shield.Grace`). Once per full charge. | `Grace.Below/Refill` |
Trait damage is `UShieldDamageType` (never critical, no causer: XP counts, no gun notched), dealt on the ward's next tick
(never inside the creature's own attack call).

## Retune note vs creature damage x1.5 (12.2) -- Main's call, `Looter.Fight.DamageScale` (Creatures/FightTuning, not mine)
x1.5 assumed a health-only player (a big fight drained most of 100). The Common ward makes the pool 140 at every level, and
it refills in lulls. **Proposal: DamageScale 1.5 -> 1.8.** A fight that took ~85 of 100 now deals ~102: 73% of 140 with no
lull, ~57% with one refill; a spider bite (12 x 1.8 = 21.6) breaks a fresh Common ward in two bites (the break is the
"get out" cue). If the playtest finds it soft, 2.0 (~81%). Better wards (52-70 at level 1) are meant to make the player
tankier: that's the loot's reward. `Looter.Fight.Tuning`'s test asserts 1.5: update it with the default.

## Drops
- `ULootTable::bDropsShields` (C++ default off: tests' tables roll exactly as before). Each gun pick of a table that drops
  wards becomes a ward of the **same rarity** with `Drop.Share` [0.2] (drawn after the picks), at the kill's level: the
  weapon-drop share split, a kill's loot count unchanged. `create_shields.py` turns it on for Default (chests copy it:
  `AChest::MakeLootTable`), Rare, Epic, Hallowed, Ethereal, Legendary. **Boss stays off** until `ABossLootShower` throws
  `Roll.Shields` (Bosses/, Main's).
- On the ground: `AShieldPickup` (SM_WardBox_<Rarity> x1.7, spinning, coal window lit in the rarity colour, the guns' beam
  numbers `LootDropMoments`, Rare+ landing sting and flare, `UShieldLabelWidget`). Interact: tap = wear if none worn, else a
  spare (tap hidden when spares full); hold 0.4 s = wear now (the worn one to the spares, or dropped). Lies 300 s (held while
  the player is within 9 m); **not saved** with the map and **not** sent to Lost Property (guns only) -- later, if wanted.

## Carrying, saving
- `UPlayerShieldComponent`: one worn + `MaxSpares` 3 spares. Saved inside `FWeaponInventorySave::Shields`
  (`FShieldInventorySave`: Equipped, Spares, bStarterGiven) by the manager's Save/RestoreInventory (wards restored before the
  guns). Stats rebuilt from definition+rarity+level+seed+build on load (balance changes reach saved wards). Enums saved by
  name: append only.
- The component: `ALooterCharacter` should make it (default subobject, Main's file); until then the weapon manager's BeginPlay
  adds one to any pawn with health (`FindOrAdd`).

## UI
- **HUD** (`UHudShieldBarWidget`, in the player frame's canvas behind the medallion): a slim 13 px slanted bar over the
  health bar (x 128, y 915, 540 long; the track starts at the bezel's edge as the health bar's does), the kit's cyan
  (XPLight/XPDark halves, GemLight lit line), the charge's number after it (CyanText, dim when broken). Hit: drop + pale chip
  (hold 0.3 s, drain 0.4 s); break: white flash + 7 slivers flying off + the empty track pulsing faint cyan until it fills;
  refill: shimmer sweeping (0.9 s); full: bright flash; Grace: gold flash. Hidden with no ward. Wakes the frame's idle fade;
  a ward not full keeps the frame at full strength. Driven from the frame's tick (`Update`).
- **Loadout**: a **WARD** section between EQUIPPED and BACKPACK (`LoadoutWidgetWard.cpp`): the worn row (58 px, WORN chip,
  Inked icon, name in rarity colour, kind/level/trait, capacity) then a row per spare (verdict arrow vs worn). W/S walk
  slots -> wards -> backpack. E on the worn row goes to the spares (or wears the first when none worn); E/F on a spare wears
  it (the worn one takes its place); C stows the worn; Q drops; X/M denied. Card: banner (worn / better / weaker), name,
  big capacity with delta, bars for Holds / Wait / Refill / Full in, the trait line, the worn ward's live state. Showcase:
  the Inked icon at 300 px. Hidden entirely when the holder has no shield component (old tests' stand-in pawns).
- **Ground label**: name + icon (+ trait name) from afar; looked at: plate, stats line, compare line (+capacity, sooner
  seconds; green/red/dim), keys.

## Art and sounds
- Model `Art/Models/Shields/WardBox.py` -> `/Game/Art/Shields/SM_WardBox_Common..Legendary` (1060 / 1412 / 1688 / 2056 / 2448
  tris; Nanite off, no collision). Uncommon + brass corners, Rare + gauge, Epic + lid sigil, Legendary + saint's medal. Glow
  slot `WardGlow` (flat glow: the pickup sets Color/Glow per rarity). Previews `Saved/ArtPreviews/Shields/WardBox*.png`.
- Icon `WardBox` in `Art/Icons/InkedIcons.py` (square box, appended last); `InkedIconData.inl` regenerated into the holding
  tree. **If MANUFACTURERS regenerates the .inl too, rerun InkedIcons.py after both merges.**
- Sounds `Art/Sounds/recipes/shields.py` (group `shields` added to `recipes/__init__.py` GROUPS after BOUNTY's `bounties`):
  `Shield.Hit` (5), `.Break` (3), `.RechargeStart` (3), `.Full` (3), `.Nova` (3, 3D), `.Grace` (2), `.Pickup` (3): rendered
  (`Art/Sounds/Out/Shield_*`, cues.json entries added, check 0 flags; cues.json also gained BOUNTY's Bounty.* entries). Cue names
  live in `Shields/ShieldSounds.h` (`ShieldCue::`) until Main moves them to `LooterSoundCues.h`.

## Tests (`Tests/ShieldTests.cpp`, `-Filter Looter.Shields`)
`.Tuning` (defaults, every cvar exists), `.Stats` (numbers, levels, rarity, builds, trait odds, starter, names), `.Rules`
(soak, break spill, refill clock, trait numbers, compare), `.OnHealth` (a real health component: soak, break, wait, fill,
death/respawn, Enable 0), `.Wards` (take/wear/swap/spares full, save round trip, old saves), `.Loot` (off = no wards; on ~20%
of picks, one item a kill, kill level), `.HudPlace` (bar over the health bar, track at the bezel, inside the frame).

## Merge for Main (phase 2 order)
1. Check `Saved/ShieldCode/HASHES.txt`, copy the tree. New files: `Shields/*` (ShieldTypes.h, ShieldDefinition, ShieldTuning,
   ShieldRules, ShieldText, ShieldSounds.h, ShieldDamageType.h, PlayerShieldComponent + PlayerShieldTraits.cpp +
   PlayerShieldWards.cpp, ShieldPickup, ShieldCommands.cpp), `UI/HUD/HudShieldBarWidget` (+ HudShieldBarWidgetPictures.cpp),
   `UI/World/ShieldLabelWidget`, `UI/Inventory/LoadoutWidgetWard.cpp`, `Tests/ShieldTests.cpp`.
2. Optional: `ALooterCharacter` ctor `Shield = CreateDefaultSubobject<UPlayerShieldComponent>(TEXT("Shield"));` (+ UPROPERTY).
3. Build; `Tools\models.ps1 -Only WardBox`; `create_shields.py`; `build_sound_bank.py` (the Shield.* WAVs); move
   `ShieldCue` into `LooterSoundCues.h` if the bank's check wants every cue there; add `Shield.Hit` to check.py's FREQUENT.
4. `Config/DefaultGame.ini` DirectoriesToAlwaysCook: `/Game/Art/Shields`, `/Game/Data/Shields` (loaded by path).
5. `runtests -Filter Looter.Shields`; also `Looter.Loot`, `Looter.Recovery`, `Looter.UI` loadout tests touched by the edits.
6. `Tools\hudshots.ps1` (the frame with a ward), PIE: `Looter.Shield.Give Epic 3 LastBreath wear`, `Looter.Shield.Break`,
   `Looter.Shield.State`, a spider fight.
7. Perf: the bar is a handful of images; pickups tick at 5 Hz; nothing on Medium's budget beyond a loot beam per ward.

## For Main's files (hooks I didn't make)
- `Creatures/FightTuning`: DamageScale 1.8 (above) + its test.
- `Bosses/BossLootShower`: plan/throw `Roll.Shields` (then turn on the boss table in create_shields.py).
- `Audio/PlayerSoundComponent::HandleDamaged`: no flesh hurt grunt when `Health->GetLastHealthLoss() == 0` (the ward took it).
- `Embers/Powers/EmberBorrowedTime::HandleDamaged`: take the debt from `GetLastHealthLoss()`, not the whole hit (else a ward
  hit heals health that wasn't lost).
- `UI/HUD/HudScreenEdgeWidget` / portrait flinch: today they fire on any hit; Borderlands tints a shield hit (cyan edges, no
  red) -- a later polish.
- `Dev/HudShotScene`: a state with a ward (full, chipped, broken) for hudshots.
- Lost Property / map save for wards on the ground; vendors buying wards; Docs/Pipeline (Gunplay) "Shields" section from this.
- CODEMAP lines:
  - `## Shields` (new section after Combat): `Shields/ShieldTypes.h`: `EShieldTrait`, `EShieldBuild`, `FShieldStats`,
    `FShieldInstanceData`, `FShieldInventorySave`. `Shields/ShieldDefinition`: `UShieldDefinition` (DA_Shield_WardBox: base
    numbers, rarity table, models per rarity, glow slot, icon; `GetWardBox`). `Shields/ShieldTuning`: `FShieldTuning`, the
    `Looter.Shield.*` cvars. `Shields/ShieldRules`: rolling, stats, soak/break, refill clock, traits' numbers, compare,
    identity. `Shields/ShieldText`: names, trait lines, colours, stat strings. `Shields/ShieldSounds.h`: `ShieldCue`.
    `Shields/ShieldDamageType.h`: `UShieldDamageType`. `Shields/PlayerShieldComponent`: `UPlayerShieldComponent`, the worn
    ward on the health's Shield hook (soak, break, wait, fill, starter, death/respawn); `PlayerShieldTraits.cpp` Last Breath,
    Barbwire, Lodestone, Saint's Grace (Harvest in the main file); `PlayerShieldWards.cpp` wearing, spares, dropping, save.
    `Shields/ShieldPickup`: `AShieldPickup`, a ward lying as loot (beam, label, tap take / hold wear, landing sting).
    `Shields/ShieldCommands.cpp`: `Looter.Shield.Give/State/Break/Fill`.
  - UI: `UI/HUD/HudShieldBarWidget`: the ward bar over the health bar (`HudShieldBarWidgetPictures.cpp`); `UI/World/
    ShieldLabelWidget`: a ward's ground label; `UI/Inventory/LoadoutWidgetWard.cpp`: the loadout's WARD section, card,
    showcase icon and keys.
  - Edits to existing lines: HealthComponent (+`GetHitBeingTaken`, `GetLastHealthLoss`), PlayerVitalsSubsystem (wounds'
    ramp waits for the ward), LootTable (`bDropsShields`), LootLibrary (`FLootRoll::Shields`), WeaponInventorySave (`Shields`).
  - Tests: `ShieldTests.cpp` (`Looter.Shields.Tuning`, `.Stats`, `.Rules`, `.OnHealth`, `.Wards`, `.Loot`, `.HudPlace`).

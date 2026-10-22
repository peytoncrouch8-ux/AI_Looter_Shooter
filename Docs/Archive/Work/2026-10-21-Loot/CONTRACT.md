# Loot contract (Phase 12.3; Main, 2026-10-21 night)

The user's calls (Docs/Handoff.md "Overnight run"): four new gun kinds (Rustler SMG, Longhorn sniper, Thresher LMG,
Powderkeg rocket launcher; models by GUNS-ART-A/B per `Saved/Guns/CONTRACT.md`, look `Saved/ArtPreviews/Weapons/Concepts/`
NewGuns_ConceptB.png + SMG_Options2.png option 7 Helix); story elements **Ember** (burn), **Soul-light** (vs undead),
**Grave-salt** (slow, vs spirits) where each element's effect deals damage (over time) and is **more effective against the
monsters weakest to it**; gun effects on parts (behaviour, not only stats); bigger drop moments. Read CLAUDE.md,
this file, Saved/Gunfeel/CONTRACT.md "Rules for every agent" (same rules: cvars for feel numbers, plain testable rules,
own files only, compile-clean, CODEMAP, Pipeline section, report under 300 words), Docs/Pipeline.md item 7 "Guns from
parts" and the Drover's steps (a new kind's C++ first, then `create_revolver.py`-style asset scripts, `setup_gun_parts.py`,
`create_rank_assets.py`), the gunfeel sections, and memory notes in Docs on loot (ammo rule: kill ammo 18-36 rounds
leaning 2x to the kill gun's class; chests fixed 36; weapons ~30%).

**Phase 1 (now): write code only.** No build, editor, tests, Unreal scripts or commits: FIGHT work (12.2) owns the build
lane until Main commits it. **Phase 2 (Main resumes you):** you get the editor lane in turn (GUNS-CODE first), build,
fix, run your editor scripts and tests, check in PIE with single `Tools\grab.ps1` grabs (no Python capture batches),
report; Main reviews and commits.

## Shared names (both code agents use exactly these)
- `EWeaponKind` appended (saved by name, keep the old numbers): `SMG`, `Sniper`, `LMG`, `Launcher`.
- `EAmmoType`: SMG and Sniper exist; LMG uses `AssaultRifle`; append `Rocket` (before `Count`).
- `ELooterElement` (new `Elements/LooterElement.h`): `None`, `Ember`, `SoulLight`, `GraveSalt`.
- `EPartEffect` (new `Weapons/PartEffect.h`): `None`, `Ricochet` (a bullet bounces once to the nearest creature within
  8 m), `Burst` (3-round burst), `Explosive` (small splash r 150, 60% to the target, 40% splash), `Piercing` (through
  one creature), `Overcharge` (hold to charge 0.6 s: x2 damage, x0.5 fire rate), `Twin` (two bullets a shot, one round).
- Part rows gain CSV columns `Effect` and `Element` (blank = none); `FWeaponPartSlot`/option fields `Effect`, `Element`.
- New reload steps (`EReloadStep` appended): `BoltOpen`, `BoltClose` (sniper), `BoxOut`, `BoxIn`, `BeltLay` (LMG),
  `RocketOut`, `RocketIn` (launcher), `HelixOut`, `HelixIn` (SMG).
- Sound cue names the code may call before they exist: `Weapon.<Kind>.<Layer>` per GunshotSound for the new kinds
  (KindWord Smg/Sniper/Lmg/Launcher -> "SMG","Sniper","LMG","Launcher"), `Weapon.<Kind>.<ReloadStep>`,
  `Weapon.Launcher.Explosion`, `Weapon.LMG.SpinUp`, `Weapon.Sniper.BoltCycle`, `Element.<Element>.Apply`,
  `Element.<Element>.Tick`, `Element.<Element>.Kill`, `Loot.Drop.<Rarity>` (Rare, Epic, Legendary stingers),
  `Loot.Drop.Legendary.Land`.

## GUNS-CODE (Opus): the four kinds end to end
Owns: `Weapons/WeaponTypes.h`, `Weapons/AmmoTypes.*`, `Weapons/WeaponDefinition.*`, `Weapons/WeaponBase*.cpp/.h`,
`Weapons/WeaponModelComponent*`, `Weapons/WeaponHipHold.*`, `Weapons/ReloadMotion.*`, `Weapons/WeaponSounds.h`
(reload-step mapping only), new `Weapons/WeaponRocket*` (the rocket projectile: flight, smoke trail through FWeaponFX,
explosion splash with falloff, never through walls, self-damage x0.5 to the player), `Weapons/GunfeelKickTuning.*`
(add the four kinds' kick/recoil/rumble/aim/swap numbers), `Player/ViewKick.cpp`, `Player/WeaponSwapMotion.*`,
`Player/FirstPersonHandsGrips*`, `Player/Animation/LooterStancePose*` (holds, grips, reloads for the new kinds),
`Player/Animation/BodyFigureComponent.cpp` (kind switches only), `UI/Inventory/Loadout*` (kind switches only),
`UI/HUD/HudWeaponSlotsWidget*` (kind/ammo icons), `Art/Icons/InkedIcons.py` + `UI/Style/InkedIconData.inl` (four gun
icons + a rocket ammo icon, inked style like today's), `Art/Models/Ammo/Ammo.py` (a rocket pickup model; models run via
artrun), `Loot/LootTable*` and `Loot/*` ammo rules (rocket ammo amounts: small, 1-3 a kill drop leaning to the kill gun),
`Weapons/BulletMarks.cpp`, `WeaponBrass.cpp`, `WeaponBaseSmoke.cpp`, `WeaponNotches.cpp` (kind switches),
`Combat/HitReaction.cpp` + `Creatures/CreatureHitReactionComponent.cpp` (kind switches only: the new kinds' kills: SMG
like the rifle, sniper like the revolver's crit burst x1.3, LMG like the rifle with pressure, launcher = the explosion
knock), new `Tools/Unreal/create_new_guns.py` (DA_SMG, DA_Sniper, DA_LMG, DA_Launcher like create_revolver.py, into the
default loot table), new `Tests/NewGunsTests.cpp`.
Mechanics per kind (start numbers as cvars `Looter.Gunfeel.*` beside the existing ones): Rustler full auto ~900 rpm,
helix glow driven by rounds left (custom primitive data on the GunAmmoGlow slot), fastest swap; Longhorn bolt action
~45 rpm (the bolt cycles between shots: `BoltCycle`), scope zoom from the sight's Zoom column (4-8x), a sway while
aimed that holding breath (sprint key while aimed) steadies for 3 s, crits x2.0 (the user allowed per-gun crits
above 1.5x for the revolver's role; sniper's job is crits; record as Main's unattended call), no hip spread mercy;
Thresher ~650 rpm with a 0.35 s spin-up (fire rate ramps from 60%), heat glow on GunHeatGlow by sustained fire, slow aim
and move while aimed x0.8; Powderkeg one rocket (the Magazine column), rocket ~4000 cm/s, splash r 350, long reload,
the rocket model shows in the tube while loaded (SOCKET_Rocket).

## LOOT-CODE (Opus): elements, part effects, drop moments
Owns: new `Elements/` folder (`LooterElement.h`, `ElementStatusComponent` (added at runtime to any creature hit by an
element: stacks, damage over time, slow, looks, sounds), `ElementRules` (chances, durations, damage per tick, the
weakness table), `ElementTuning` (cvars `Looter.Element.*`), `ElementDamageType`), new `Weapons/PartEffect*` (the
effects through `UBulletSubsystem::OnBulletFiring`/`OnBulletLanding` and a per-gun effect component; Burst through
`Weapons/WeaponTrigger.*`, which you own), `Weapons/WeaponParts.*`, `Tools/Unreal/setup_gun_parts.py` (Effect and
Element columns), the existing guns' `Art/Models/Weapons/*.parts.csv` (add element and effect parts: rows only, never
rename keys; new keys get models from the art agents later or reuse an existing part's model with a glow tint),
`Weapons/WeaponFX*` (element hit looks: Ember embers and flame licks, Soul-light white-gold motes and rays, Grave-salt
pale crystals and frost; a burning/smiting/salted creature's status look), `UI/World/WeaponLabelWidget*` and
`UI/Style/WeaponText*` + `UI/HUD/PlayerHUDWidgetPickupCard.cpp` (element and effect lines on cards and labels, colours
added to `LooterUI::Color`: Ember orange-red, Soul-light pale gold, Grave-salt pale cyan-white), `Loot/LootDrop*`,
`Loot/*Beam*` and pickup code (drop moments), `Combat/BulletSubsystem*.cpp` (hooks only), new `Tests/ElementTests.cpp`,
`Tests/PartEffectTests.cpp`, `Tests/DropMomentTests.cpp`.
- Elements: a gun gets an element from a part (Uncommon+ chance, MinRarity in CSV); each hit has a chance (by gun and
  part, ~20-35%) to apply a status: Ember burn (damage over time ~40% of the hit over 3 s, stacking to 3), Soul-light
  smite (a delayed burst of light ~60% of the hit after 1 s, chains to one undead within 4 m), Grave-salt (damage over
  time ~25% over 4 s and a 30% slow; salted wards break faster). **Weakness table** (`Element.Weak` x1.75 to the
  effect's damage and chance, `Element.Resist` x0.5): Ember strong vs spiders, Spitter, slimes, Gravemother (flesh,
  chitin, gel) and weak vs the Unpaid; Soul-light strong vs the Unpaid, dead miners, Bell-Ringer, Abel's risen (undead)
  and weak vs slimes; Grave-salt strong vs Warded/Phasing traits, Ethereal tier, the Unpaid's coal (spirits) and weak vs
  spiders. Bosses take effects at x0.5 duration. The bestiary page lists each kind's weakness (its data is the table).
  The Grave-salt grenade's damage type becomes the Grave-salt element's (same name, same feel).
- Part effects: Epic+ parts (MinRarity Epic, a few Rare) carrying one `EPartEffect` each, shown on the card.
- Drop moments: by rarity, a taller beam in its colour, a sound stinger (`Loot.Drop.<Rarity>`), Epic+ a spin and a
  ground ring as it lands, Legendary a 0.25 s world slow-down at the drop (not in boss fights' own slow-mo), a column of
  light visible 60 m away, the name popping above it, `Loot.Drop.Legendary.Land`. No new lights on Medium.

## GUNS-SOUND (Opus): sounds for all of the above
Owns: `Art/Sounds/recipes/guns_*.py` (new files per kind fine), `Art/Sounds/recipes/elements.py`, `loot.py` (drop
stingers), `Art/Sounds/cues.json`, `Art/Sounds/packs/*` (add the excerpts you use), `Audio/LooterSoundCues.h`,
`Weapons/GunshotSound.*` (the KindWord for the new kinds), `Weapons/WeaponSounds.h` (fire/equip/holster by kind;
GUNS-CODE adds reload steps there only through the names above: coordinate by leaving the reload switch to GUNS-CODE).
Sources: `Saved/SoundPacks/Sonniss` (list `Saved/Gunfeel/SONNISS_FILES.txt`): SMGs (MP5, MP7, Ingram M10, Uzi, Mini Uzi,
Steyr TMP, Vz61, FN P90, M3, Glock 18, Beretta 93R), snipers (Barrett M98B, Remington 700, Sako TRG), LMGs (L110A2,
L7A2, MG4), explosions (Gamemaster Explosion pack, Bluezone War Zone), plus the existing excerpts. Shape them into our
guns (not stock): Rustler bright and fast with a whirr of the helix, Longhorn a huge crack with a long valley tail and a
satisfying bolt, Thresher a deep chug with a spin-up, Powderkeg a whoosh launch and a big layered explosion (near, far).
Elements: apply/tick/kill cues per element (Ember crackle and whoomph, Soul-light choir-ish shimmer and ring, Grave-salt
crystalline crunch and hiss). Drop stingers: Rare short chime, Epic rising chord, Legendary a big rising stinger and a
landing impact. Render with Tools\sounds.ps1 (check: 0 flags), never build_sound_bank.py (Main imports).

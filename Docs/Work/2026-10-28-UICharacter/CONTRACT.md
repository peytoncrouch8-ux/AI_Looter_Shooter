# UI character, round 1: the gun card (2026-10-28)

The user's asks (at work, no playtest until Friday 2026-10-30):
- Character through **motion and juice** and **maker branding**; **simplicity**: don't overload simple things like guns.
- A gun on the ground: its card shows **over the gun**, not on the screen's right. The **gun in hand** shows on the right.
- Inventory **back to the backpack on the right**, not under the equipped guns.
- Look (four rounds of mockups, `mockup/`): "hate the boxy style", then "a little curvy", then picked **Wide Sweep**,
  "less curvy, slightly more futuristic, hollow Hold E buttons", then "bolder lines, just a little". Final: `mockup/wide.png`
  (`wide.js`). The first full layout: `mockup/sheet1-3.png` (the layout stands; the look is wide.png's).
- Not in this round: menus, pause, HUD popups (round 2, same look and motion), other screens' restyle (vendor, bench...).

## The look ("Wide Sweep", wide.png)

- Card: dark blue glass, vertical gradient `CardFillTop` (8,30,44 @ .92) to `CardFillBottom` (6,20,30 @ .86); corners cut
  16 px top-right and bottom-left, the other two square-ish. Fill fades with UI transparency (MarkBackground); lines don't.
- Lines 2 px: cyan `CardLine` (90,200,255 @ .85) from the sweep's end along the top, down the top-right cut; a bracket
  along the bottom-left cut and 44 px of the bottom edge (`CardBracket`, @ .7).
- The maker's **sweep**: top-left band 28 px tall, 276 of 420 px long (scaled with the card's width: 0.657), ending in an
  S-curve (path `M0 0 H232 C250 0 256 28 276 28 H0 Z`); maker colours left to right; a 3 px trim line along its foot in the
  maker's trim; its deco (Roundhouse rivets, Saint Maud's gilt diamond, Windfall's tape and bolts, Meridian's ruler ticks);
  the maker's Inked mark (15 px) and word (Chakra bold 11, spacing 250) in the maker's ink. Right side of the band: the
  verdict ("▲ UPGRADE" green) on a compare card, else the quirk name (dim, 11).
- Body (padding 8/18/14): name (Chakra bold 20, rarity colour), "Level 6 · Pump shotgun" (13, dim); two columns 150 | rest:
  left the damage (Chakra bold 40, "×8" 18) over "DAMAGE ▲ +22" (11, dim; the delta green/red only on a compare card);
  right three stats (FIRE RATE, MAGAZINE, RELOAD): label (11, dim), a **segmented bar** (10 skewed segments, 7 px tall,
  2 px gaps, lit `Text`, unlit cyan @ .14), the compared gun's tick (3 px orange, past the bar by 3 px each side), arrow
  ▲ green / ▼ red. Then one **special line** (element > part effect > curse > notches): "◆ EMBER sets them alight".
  World card only: keys row, **hollow key caps** (2 px orange outline, chamfered 5 px, orange letter, no fill):
  "[E] TAKE   [HOLD E] EQUIP"; the HOLD cap fills orange left to right as the hold runs.
- Rows (inventory, later menus): 52 px, top-right cut 12 px, dark glass @ .78; the lit row an orange wash (.26 to .03) with
  a 2 px orange outline @ .75 and a 4 px orange skewed tick; an empty row @ .35 with "EMPTY".
- Fonts: titles/numbers Chakra Petch (have it). Small text was Exo 2 in the mockup: **keep Chakra Petch** in game (no new
  font without asking; Chakra regular reads close enough). Colours only from `LooterUI::Color`.

## Motion (the card's intro, `GunCardRules`)

- 0-0.12 s unfold: scale Y from 0.15 to 1.06 to 1 (pivot = bottom for the world card, top otherwise), opacity 0 to 1.
- Bars fill one after another, 0.06 s apart, 0.16 s each (ease out). Damage counts up 0 to its value over 0.25 s.
- Epic and Legendary: a shine sweeps the band once, 0.30-0.65 s.
- Sound on open: `UI.Card.Open` (soft unfold tick) + the maker's `UI.Card.Maker.<Key>` (clank, bell ting, rattle, click);
  no maker: Open only. World card: on a new gun looked at. Loadout: Open only, rate-limited (0.12 s); maker sound on Inspect.

## Files and lanes (each lane only its files; no agent builds or commits; Main builds, tests, commits)

**Lane A, Main: the card kit.** New:
- `UI/Cards/GunCardStyle.h/.cpp`: `GunCardStyle::FStyle`, `For(Gun)` (maker's, or Unmarked).
- `UI/Cards/GunCardRules.h/.cpp`: pure rules (stats shown, special line, verdict word, key hints, motion curves).
- `UI/Cards/LooterCardBackdrop.h/.cpp`: `SLooterCardBackdrop` + UMG `ULooterCardBackdrop` (card or row shape; fill
  part, lines part; sweep; shine).
- `UI/Cards/LooterSegmentBar.h/.cpp`: `SLooterSegmentBar` + `ULooterSegmentBar`.
- `UI/Cards/LooterHollowKey.h/.cpp`: `ULooterHollowKey` (hollow cap, hold fill).
- `UI/Cards/GunCardWidget.h/.cpp` (+ `GunCardWidgetMotion.cpp`): `UGunCardWidget`.
- Colours in `LooterUI::Color` (`UI/Style/LooterUIStyle.h/.cpp`): Card*, Maker*.
- Tests `Tests/GunCardTests.cpp` (`Looter.UI.GunCard`).

**Lane B (Opus): the card over the gun and the in-hand card.** Files: `UI/World/WeaponLabelWidget.h/.cpp`,
`UI/World/LootLabelSubsystem.*`, `UI/World/LootLabelLayout.*` (only if the bigger card needs it),
`UI/HUD/PlayerHUDWidget.h`, `UI/HUD/PlayerHUDWidgetLayout.cpp`, `UI/HUD/PlayerHUDWidgetPickupCard.cpp`,
`Weapons/WeaponBase*.cpp` only for the label component's settings, tests `Tests/NamedWeaponTests.cpp`,
`Tests/LootLabelLayoutTests.cpp`, `Tests/DropMomentTests.cpp` (only what the change breaks), new `Tests/WorldGunCardTests.cpp`.
- Looked at, the label's plate (stats, prompt, maker row) is replaced by a `UGunCardWidget` (World) over the gun, its
  bottom on a 2 px cyan tether 40 px long down to a hollow diamond (rarity colour) where the name hung; compared with the
  gun in hand; keys from the weapon manager (slots full, backpack full) and the interact hold. Not looked at: as now (name,
  trait, curse, folding, pop).
- The HUD's right card (`PickupCard`) becomes a `UGunCardWidget` (InHand) of the gun in hand under "IN HAND", shown only while
  loot is looked at and a gun is held; keeps `LootCardDrop` placement and `GetLootCardBounds` (the tracker makes way).

**Lane C (Opus): the inventory.** Files: `UI/Inventory/LoadoutWidget*.{h,cpp}`, `UI/Inventory/LoadoutParts.*`,
`UI/Inventory/LoadoutRules.*` (only if needed), `Tests/LoadoutTests.cpp`.
- Left (x 56, w 488): EQUIPPED (slot rows), WARD, AMMO. Right (x 1056, w 488): BACKPACK, every slot, scrolling. Middle:
  the gun under the cursor turning (showcase) over its `UGunCardWidget` (Loadout, compared with the slot it would go to).
  Left/right (A/D, arrows, stick) switch columns; up/down within. Rows in the new row look (`ULooterCardBackdrop` Row).
  Inspect (X): as now (lists hide, gun large, every stat), its card on the new backdrop. Drag and drop, E/F/Q, sort R kept.

**Lane D (Sonnet): the sounds.** Files: `Art/Sounds/recipes/ui_cards.py` (new), `Art/Sounds/cues.json`,
`Audio/LooterSoundCues.h` (namespace `LooterSoundCue::Card`). Cues: `UI.Card.Open`, `UI.Card.Maker.Roundhouse`,
`.SaintMaud`, `.Windfall`, `.Meridian`. Short (0.15-0.4 s), quiet, UI class; 3 takes each. Main renders and imports.

## The API (Lane A writes the headers first; B and C code against them)

See `Source/AI_Looter_Shooter/UI/Cards/*.h`. In short:
- `UGunCardWidget::SetGun(const FWeaponInstanceData& Gun, const FWeaponInstanceData* Compare, EGunCardMode Mode)`,
  `SetKeys(const FGunCardKeys&)` (World), `PlayIntro(bool bWithSound)`, `GetCardWidth(Mode)`.
- `ULooterCardBackdrop`: `SetShape(ELooterBackdropShape)`, `SetPart(ELooterBackdropPart)`, `SetLit(bool)`, `SetSweep(...)`.

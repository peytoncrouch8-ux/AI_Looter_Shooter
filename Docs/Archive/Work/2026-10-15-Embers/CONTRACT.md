# Ember powers, round 1: the contract (orchestrator, 2026-10-15 night; the user is asleep, nobody asks them anything)

The user's rules: each hero owns three powers from the start, ONE equipped at a time; the key is Q (rebindable; the
left bumper on a gamepad); the HUD socket sits beside the weapon column, bottom-right; the powers page is the
inventory's SECOND tab, after the Loadout (the Ledger becomes the third). First hero: Ellis (Dust Devil, Sundown,
Borrowed Time). Designs and numbers: `Docs/Handoffs/CloudIslandConcepts_2026-10-05.md`, "Ember powers" and "Five heroes".

## The shared headers (written; code against them, never change a public signature without telling the orchestrator)
- `Source/AI_Looter_Shooter/Embers/EmberTypes.h`: EEmberState, EEmberDenied, FEmberSave, the delegates.
- `Embers/EmberDefinition.h`: UEmberDefinition (data asset per power; Id, words, Color, IconName, clock, numbers, PowerClass).
- `Embers/HeroDefinition.h`: UHeroDefinition (a hero's three powers).
- `Embers/EmberPower.h`: UEmberPower (CanActivate/Activate/Tick/End) and FEmberContext.
- `Embers/EmberComponent.h`: UEmberComponent (owned, equipped, the gate, clocks, save, reports, console commands).

## Ids, names, colours, cues (fixed)
| Power | Id | Nature | Colour (LooterUI::Color name, hex) | Icon (InkedIconData function) | Cues (LooterSoundCue::Ember::) |
|---|---|---|---|---|---|
| Dust Devil | `DustDevil` | Wind | `EmberDustDevil` #e8f6ff | `DustDevil` | `DustDevilUse` |
| Sundown | `Sundown` | Dusk | `EmberSundown` #ffb24d | `Sundown` | `SundownStart`, `SundownEnd` |
| Borrowed Time | `BorrowedTime` | Debt | `EmberBorrowedTime` #c9b4ff | `BorrowedTime` | `BorrowedTimeStart`, `BorrowedTimeSettle` |
| (shared) | | | | | `Ready` (a power comes off cooldown), `Denied` (a press that did nothing) |
Cue strings: `Ember.DustDevil.Use`, `Ember.Sundown.Start`, `Ember.Sundown.End`, `Ember.BorrowedTime.Start`,
`Ember.BorrowedTime.Settle`, `Ember.Ready`, `Ember.Denied`.
Data assets: `/Game/Data/Embers/DA_Ember_DustDevil`, `DA_Ember_Sundown`, `DA_Ember_BorrowedTime`, `DA_Hero_Ellis`.
Key binding id `Ember`, display "Ember power", category "Combat", default Q, gamepad left shoulder.
Save: `ULooterSessionSave::Embers` (FEmberSave) + `bool bHasEmbers` (a save from before reads as none captured: the
hero's first power equipped, ready).
Inventory page: `EInventoryPage::Embers` inserted after Loadout (order Loadout, Embers, Bestiary, Missions, Map); its
tab reads "Embers"; the page widget class `UEmbersWidget` in `UI/Embers/`.
HUD: `UHudEmberSocketWidget` in `UI/HUD/`, placed in the Arms row to the LEFT of the weapon slots (PlayerHUDWidgetLayout).
Damage type: `UEmberDamageType` (Embers/EmberDamageType.h) so hit markers and kill credit know a power did it; a
power's kill gives XP (AwardKill) but notches no gun.
Tests: `Looter.Embers.*` (core, UI), `Looter.Embers.Power.*` (the three powers).

## Who owns which files (disjoint; nobody builds or opens Unreal; the orchestrator builds and fixes compile errors)
- CORE: `Embers/EmberComponent.cpp`, `Embers/EmberDamageType.h`, `Embers/EmberCommands.cpp` (Looter.Embers.*),
  `Settings/KeyBindingSubsystem.h/.cpp` (the Ember action), `Core/LooterCharacter.h/.cpp` (the component),
  `Session/SessionSave.h` + `SessionSubsystemWorld.cpp` (capture/restore), `Tools/Unreal/create_ember_assets.py`,
  `Tests/EmberTests.cpp`, CODEMAP lines for these.
- UI: `UI/HUD/HudEmberSocketWidget.h/.cpp`, `UI/HUD/PlayerHUDWidget*.cpp/.h` (placing and updating the socket),
  `UI/HUD/LooterHUD.h/.cpp` + every EInventoryPage site (`UI/Inventory/LoadoutParts.cpp` tabs,
  `UI/Bestiary/BestiaryWidgetInput.cpp`, `Missions/MissionPlayerObjectives.cpp`, `Dev/MenuShotScene.cpp`),
  `UI/Embers/EmbersWidget.h/.cpp` (+ `EmbersWidgetInput.cpp` if needed), `UI/Style/LooterUIStyle.h/.cpp` (colours),
  `Art/Icons/InkedIcons.py` + regenerated `UI/Style/InkedIconData.inl`, `Tools/hudshots.ps1` (a socket state),
  `Tests/EmberUITests.cpp`, CODEMAP lines.
- POWERS: `Embers/Powers/EmberDustDevil.h/.cpp`, `EmberSundown.h/.cpp`, `EmberBorrowedTime.h/.cpp`,
  `Creatures/CreatureKnockComponent.h/.cpp` (knocked + dizzy states; wire into ACreatureBase with the smallest edit),
  `Combat/BulletSubsystem*` only for Sundown's per-shooter time (the player's bullets at full speed),
  `Player/PlayerViewComponent` only for an ember FOV offset hook, `Audio/LooterSoundCues.h` (namespace Ember) and
  `Art/Sounds/recipes/embers.py` + `Art/Sounds/cues.json` entries, `Tests/EmberPowerTests.cpp`, CODEMAP lines.
Docs: each agent appends its notes to `Docs/Pipeline.md` under a new "Ember powers" section (CORE creates it; UI and
POWERS add bullets) and lists unattended decisions in `Docs/Handoff.md` under "Decided unattended (2026-10-15 night)".
No git: the orchestrator commits.

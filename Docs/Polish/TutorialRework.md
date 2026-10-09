# Skyreach's tutorial, reworked (design, 2026-10-08)

The user: "the tutorial feels rushed and unnecessary". Today it is one forced checklist of six steps (move, sprint into
town, take the rifle, shoot the dummies, kill two spiders, open the inventory), each prompt chasing the last. Players who
know shooters are told what they already know, and nobody is given a reason to be on Skyreach.

Story rule (`Docs/Story.md`, "The tutorial island, outside the story"): no story character, name, line or plot appears on
Skyreach. It teaches controls and stays as a practice island. So no story givers: the town's **notice board** is the giver.

## The shape

1. **Arrive and move freely.** The game starts at the farm as now, with no checklist. Control hints are contextual and
   appear only when the player hasn't already done the thing (below). The tracker shows one goal: *"Find a gun in town"*,
   its arrow on the gun rack.
2. **The gun rack** (the existing rifle). Taking it completes the first goal and points to the notice board: *"Read the
   notice board"*.
3. **The notice board** in the square is Skyreach's mission board: practice postings in the town's voice (no story names),
   turned in at the board (the turn-in system from the missions agent):
   - **Range Practice:** hit the target dummies under the windmill (teaches aiming down sights, reload, fire modes).
     Optional.
   - **Clear Web Hollow:** the six spiders in the webbed clearing on the forest rise. Main. The reward is a shotgun at
     the board (two gun kinds to try; it teaches swapping and the bench).
   - **The Wallow:** the five slimes in the bog west of the range. Optional; the reward is a gun of Uncommon or better.
   - **Up to the Lookout:** climb the plateau ramp to the ruined lookout (teaches the mantle, the minimap and the view).
     Optional.
   - **Board the Skiff:** at the jetty past the lookout, once Web Hollow is turned in (the existing mission). Leaving
     is always the player's choice.
4. Skyreach gives no experience (a practice area, as now); rewards are guns and the feeling of a loop: a posting, a fight,
   loot, a turn-in, a new gun, the bench.

## Contextual hints (one at a time, never blocking, each shown at most a few times)

- Move/look: only if the player hasn't moved for 6 s after control returns.
- Sprint: after 8 s of walking on a road without sprinting.
- Jump and mantle: near the first chest-high ledge or fence (the mantle comes from the traversal agent), once.
- Interact: the existing prompt at things you can use (rack, board, chests, bench).
- Aim down sights: the first time a target is over 25 m away with a gun in hand.
- Reload: magazine under a quarter with a reserve, or empty (the HUD's own "[R] RELOAD" already shows; the hint teaches
  it once with its key).
- Melee: a live creature close in front (about 3.4 m, in the strike's cone and in sight), before the player's first
  strike; gone as they strike (`UPlayerMeleeComponent`, key from `UKeyBindingSubsystem::MeleeBindingId()`).
- Grenade: two or more live creatures ahead (within 15 m, 35 degrees of the look, in sight) while the player holds a
  grave-salt grenade and hasn't thrown one yet ("[G] Grenade: salt the crowd"); gone as they throw
  (`UPlayerThrowComponent::CountTargetsAhead`, `GetThrowCount`; key from `UKeyBindingSubsystem::GrenadeBindingId()`).
  Momentary like Melee, weighed right after it.
- Swap guns: the first time a second gun is carried.
- Inventory: on the first gun pickup from loot ("[I] Inventory: compare and equip").
- Bench: when near the bench with two guns of the same kind.
- Slide: after sprinting for 4 s, once.
Hints use the mission tracker's key-hint style (or a small prompt above it), the bound key from the key-binding subsystem,
and disappear the moment the action is done. A setting turns them off (Settings > Interface: "Control hints").

## The main menu

New session: a choice on the session picker, *"Start on Skyreach (learn the basics)"* or *"Skip to Ransom's Rest"*
(the skip already exists as `Looter.Tutorial skip` and the skiff's first-cast-off logic).

## What changes in code (for the tutorial agent)

- `Tutorial/TutorialDirector*`: from a forced six-step mission to the first goal plus the hint system (or a new
  `Tutorial/ControlHintSubsystem`), and the board's postings as missions (`create_mission_assets.py`) with the notice
  board as their turn-in giver.
- The notice board (an actor with the interaction component, placed by the island build at the town square's board) that
  lists, accepts and turns in Skyreach's postings, in the UI kit.
- The menu choice (`UI/Menus/MainMenuSessions.cpp`).
- Tests: `Looter.Missions.TutorialMission`, `Tests/TutorialTests.cpp` reworked; new tests for each hint's trigger.

# Lane MAIN67's shared contract (Main 6 "The Pilot Light", Main 7 "Given to the Wind"; 2026-11-01)

The design: `MAINS_2_7.md` sections 1-3, 10, 11, 14, 15 and `DECISIONS.md` (the user's calls). This file fixes the names,
signatures, steps and file ownership the MAIN67 sub-lanes share. Nothing is built or run; every C++ file goes under
`Docs/Work/2026-10-31-Wardens/main67_src/` mirroring its `Source/` path (a changed existing file: copy it there first and
edit the copy; never write under `Source/`). Python in place (only your own files; others' files: exact edits in your
report). Edit with Edit/Write only. No new sound cues (`Audio/LooterSoundCues.h` only), no new assets (stand-ins from
existing meshes/MIs or engine BasicShapes; a Tripo model is a soft path that falls back), no Tripo calls. Perf: everything
on the barge and the furnace hall exists only during Main 6 (hidden, no collision, no tick otherwise); instanced where
there are many; no translucency, no new dynamic shadowed lights (shadowless point lights, at most 3 in view).

## Sub-lanes and their files (disjoint)

| Lane | Owns |
|---|---|
| **W1 (MAIN67 itself)** | `create_mission_assets.py` (Main6/Main7), `create_story_lines.py` (every line set below), `story_texts.py` (a "Mains 6-7 (lane MAIN67)" block: perches, cues, encounters, topics, items), `TRIPO_MODELS.md` "Mains 6-7", `lanes/MAIN67.md`, `Tests/PilotLightTests.cpp`, `Tests/GivenToTheWindTests.cpp`, the old `LanternLeans*` tests' retirement, `PacingPaths.h` exact edit, `layout.json` exact edits (DuskSmog state, encounters) |
| **W2 FLIGHT6** | the Lamplighter's Main 6 sortie: `World/LamplighterShip*.h/.cpp` (staged copies), `World/LandfallWinch.h/.cpp` (cast off), `World/CompanyTender.h/.cpp` (gas-bag weak spot, escort mode), new `World/PilotSortie*.h/.cpp`, `World/LamplighterLauncher.h/.cpp`, `Dev/PilotSortieDevCommands.cpp`, `Tests/PilotSortieTests.cpp`; python `Tools/Unreal/build_area_main6_sortie.py` |
| **W3 BARGE** | the barge's boarding stage: new `World/BargeBoarding*.h/.cpp`, `World/BargeFurnace*.h/.cpp`, `World/FurnaceDraftValve.h/.cpp`, `World/BargeHoseDrum.h/.cpp`, `World/BargeRules.h/.cpp`, `Scenes/CageBurst.h/.cpp`, `Scenes/BargeFall.h/.cpp`, `Dev/BargeDevCommands.cpp`, `Tests/BargeBoardingTests.cpp`; python `Tools/Unreal/build_area_main6.py` (the barge, its encounters, horns, manifest, wreck; it calls W2's module) |
| **W4 ABEL** | Abel corrupted: `Bosses/AbelKeeper*.h/.cpp`, `Bosses/AbelRules.h/.cpp` (staged copies), new `Bosses/AbelCorruption*.h/.cpp`, `World/KeeperLanternPost.h/.cpp` (call-back hold), `UI/HUD/HudBossBarWidget.h/.cpp` + new `HudBossBarWidgetPips.cpp`, `Scenes/FreeAbel.h/.cpp` (SitWithPa retired), `Dev/AbelDevCommands.cpp`, the Abel/Gravewind/Boss tests that read Main 6's fight, new `Tests/AbelCorruptionTests.cpp`; python `Tools/Unreal/build_area_deck.py` (the deck for Main 7, the dusk rules), `Tools/Unreal/build_area_shrines.py` is NOT yours (exact edit) |
| **W5 HOME** | Main 7 after the fight: new `World/SoulCoal*.h/.cpp`, `World/CarriedLantern*.h/.cpp` (+ its HUD meter `UI/HUD/HudCarriedLanternWidget.h/.cpp`), `World/ReliquaryLantern.h/.cpp`, `Scenes/SaintAdaWakes.h/.cpp`, `Story/AbelOnBoard.h/.cpp` (staged: a standing soul look for the Reliquary), `Creatures/DredgerHoseMan*.h/.cpp` (staged: latch the lantern), `World/ChapelReliquary.h/.cpp` (staged if needed), `World/Train.cpp` (staged: warm after Main 7), `Dev/Main7DevCommands.cpp`, `Tests/GivenToTheWindWorldTests.cpp`, the Heirloom/Station/Train tests that read Main 7's old shape; python `Tools/Unreal/build_area_main7.py` (new), `Tools/Unreal/build_area_depot.py` |

Anything you need changed in another lane's file: write the exact edit (old/new) in your report; never edit it.

## Main 6, "The Pilot Light" (id Main6; steps from 0)

Offered by Captain Holloway at the Lamplighter's wheel (`Speaker_Holloway`, meet first) after BackPay; accepting fades the
Rest to dusk (rule below). Turned in to Holloway at the wheel (card, then her lines) as the ship flies home and sets down.

| Step | Objective (tag / event / scene) | Who gives it |
|---|---|---|
| 0 CastOff | Interact, held 2.5 s, `Landfall_AnchorWinch` | W2 (the winch's cast-off mode while `Main6` step 0) |
| 1 Tenders | Event `Pilot.TendersDown` (both tenders down and every boarder dead) | W2 |
| 2 Grapple | Interact, held 1 s, `Lamplighter_GrappleLauncher`, count 2 | W2 |
| 3 Deck | Clear `BargeDeck` 9 + Reach `Barge_FurnaceHatch` (3 m, height counted) + optional Interact held 2 s `Barge_HoseDrum` (flag `Pilot.DrumJammed`) + optional KillNamed `Barge_Horn` 4 | W3 |
| 4 Manifest | Interact `Item_BargeManifest` (AMissionItem, item `BargeManifest`, Paid on Delivery's item; shown during Main6 from step 4) | W3 places, W1 data |
| 5 Furnace | Interact held 1.5 s `Furnace_DraftValve` count 3 + Clear `FurnaceHall` 8 | W3 |
| 6 Cage | Interact held 3 s `Furnace_SoulCage` (only with Abel's lantern: `AKeepersLantern::IsTakenIn`) + Scene `CageBurst` | W3 |
| 7 Escape | Reach `Lamplighter_Deck` (an `ALandfallMarker` on her deck, 4 m, height counted) + Scene `BargeFall` | W2 marker + plays the scene, W3 writes the scene |

Constants: `namespace PilotLight` in `World/PilotSortieRules.h` (W2 writes; W3 includes it): `Mission` "Main6",
`CastOffStep 0`, `TendersStep 1`, `GrappleStep 2`, `DeckStep 3`, `ManifestStep 4`, `FurnaceStep 5`, `CageStep 6`,
`EscapeStep 7`, `ReadyStep 8`; `TendersDownEvent()` "Pilot.TendersDown"; `DrumJammedFlag()` "Pilot.DrumJammed";
`BargeFellFlag()` "Pilot.BargeFell"; `LauncherTag()` "Lamplighter_GrappleLauncher"; `DeckMarkerTag()` "Lamplighter_Deck".

Encounters (W3's spawners, ids exact): `BargeDeck` (6 DredgerRifleman + 3 DredgerHoseMan, level 6, active during Main6
from step 2 before step 4), `FurnaceHall` (two pushes: push 1 the Chief Stoker = DredgerHoseMan, rank Fixed Hallowed, level
7 (set so the final level is 7), DisplayName "The Chief Stoker", leader, HealthScale 1.76 (x6 with Hallowed's 3.4) + 2
riflemen + 1 hose-man; push 2 through the other stokehold: 2 riflemen + 2 hose-men; level 6; active during Main6 from step 5
before step 6). The tenders' boarders (W2): 2 riflemen + 1 hose-man at level 6, uncounted by any spawner (the event counts
them). The tenders' own deck riflemen fall with them (uncounted, as in Landfall).

Shared C++ between W2 and W3 (W3 writes `World/BargeBoarding.h`; W2 includes it):
```cpp
class ABargeBoarding : public AActor   // the walkable barge over the chapel knoll during Main 6
{
public:
	static ABargeBoarding* Find(const UWorld* World);
	bool IsPresent() const;                       // shown this visit (during Main 6, before it falls)
	/** Where the Lamplighter's keel lies grappled alongside (world, her +X along the barge, her starboard to it). */
	FTransform GetBerth() const;
	/** Where she hangs for the jump in step 7 (world): under the broken gangway's end, her deck ~3.5 m below it. Moves
	 *  with the dying barge. */
	FTransform GetJumpBerth() const;
	/** The two points on the barge's rail the Lamplighter's grapples bite (world), Index 0 fore, 1 aft. */
	FVector GetGrappleAnchor(int32 Index) const;
	/** Where the gangplank from her starboard rail lands on the barge's deck (world), and the deck's spot a player is put
	 *  on (a respawn during steps 3-7, the dev jump). */
	FVector GetBoardingPoint() const;
	FTransform GetPlayerStart() const;
	/** The barge dies (step 7): its lights out, its list and drift, the stack's fall. Called by the cage's burst. */
	void BeginDying();
	bool IsDying() const;
	/** BargeFall's control: its pose Seconds into the fall (drift west over the Rest, over the Rim, down into Gravewind
	 *  Canyon), and its end state (hidden for good; Pilot.BargeFell set). */
	void ApplyFallAt(float Seconds);
	void FinishFall();
};
```
`Scenes/BargeFall.h` (W3): `namespace BargeFall { FName SceneName(); /* "BargeFall" */ float Duration(); /* ~12 s */
bool Play(ABargeBoarding& Barge, ALamplighterShip* Ship); }` played by W2's sortie once the player stands on her deck in
step 7 (scenes off: `Barge.FinishFall()` and `USceneSubsystem::MarkPlayed`). The ship holds still at her jump berth's last
pose through it; W2 then flies her home.

Python: W3's `build_area_main6.py` `place(build)` places the barge (`ABargeBoarding`, tag `Barge_Pilot`) and everything on
it, then calls `importlib.reload(importlib.import_module('build_area_main6_sortie')).place(build, barge)` (W2's module:
the sortie actor, its course, the launchers' tags are the ship's own); returns Hob's spots
`{'wheel': (x, y, z, yaw) or None}` (Hob on the Lamplighter's wheel housing: W2 computes and returns it from its module,
W3 passes it through). Folder `Gameplay/Main6`.

## Main 7, "Given to the Wind" (id Main7; steps from 0)

Offered by Holloway (one conversation, straight after Main 6's turn-in), started on the move. Turned in to Delia at the
chapel door (`Speaker_DeliaChapel`; the card, then her topic's lines: W1's data). Dusk all through.

| Step | Objective | Who gives it |
|---|---|---|
| 0 Point | Reach `Place_GravewindPoint` (10 m) + optional LastingInteract held 1 s `SoulCoal` count 12 + optional Reach `Place_KeepersBridge` | W5 (coal), existing places |
| 1 Hang (AAbelKeeper::HangStep) | Interact `LanternPost_Keeper` | existing post, W4 conditions |
| 2 Fight (FightStep) | KillNamed `Boss_Abel` (any kill) + optional Event `Abel.SixSaved` | W4 |
| 3 Free (SceneStep) | Scene `FreeAbel` (its text: "Call Abel home: hold {Interact} on the keeper's post.") | W4 |
| 4 Carry | Reach `Place_Chapel` (15 m) | W5 (the road's fight `WestRoadCrew`, the latch on the lantern) |
| 5 Reliquary | Interact held 3 s `Reliquary_Lantern` + Scene `SaintAdaWakes` | W5 |

`AAbelKeeper::Mission` becomes "Main7" (HangStep 1, FightStep 2, SceneStep 3 keep their numbers). W5 adds
`namespace GivenToTheWind` in `World/CarriedLanternRules.h`: `CarryStep 4`, `ReliquaryStep 5`, `ReadyStep 6`,
`SixSavedEvent()` "Abel.SixSaved" (W4 sends it, `FMissionEvent::Named`, and sets the flag of the same name),
`ReliquaryLanternTag()` "Reliquary_Lantern", `SoulCoalTag()` "SoulCoal", `DeliaChapelTag()` "Speaker_DeliaChapel".
(W4 includes it for the event name.)

Encounter (W5): `WestRoadCrew` (3 DredgerRifleman + 2 DredgerHoseMan, level 6, the west road between the deck and the
chapel, active during Main7 from step 4 before step 6; its hose-men latch the carried lantern, not Sully).

The carried lantern (W5's `UCarriedLanternSubsystem`, `World/CarriedLantern.h`): `static Get(ctx)`, `bool IsCarried()`
(Main 7 from step 4 before step 6, or forced), `float GetLight()` 0-1, `void Drain(float Share, AActor* By)`,
`void SetLatched(AActor* By, bool)`. W4's FreeAbel ends with the lantern lit and taken off the post (the post's lantern
hidden from Main7 step 4: W4's post conditions).

The six dead on the boards: `build_area_main2.py`'s `Main2_Shroud_<k>` story props on biers `SHROUD_BIERS`. W4 reads their
spots in `build_area_deck.py` (from the deck's Bier sockets and `build_area_main2.SHROUD_BIERS`, `SHROUD_LENGTH`), sets them
on Abel (`BoardsDead` world points), and Abel's corruption hides/shows the nearest AStoryProp to each when drunk/reset.
They're hidden after Main 7 already (main2's hidden_when). Their rising Restless Unpaid are Abel's boss adds.

## Lighting (W4 writes the rules in build_area_deck.place_dusk; W1 the state's exact edit)

New state `DuskSmog` (layout.json `level.environment.states`, W1's exact edit): Dusk's sun under thin smog. Rules, first
match: during Main7 from step 6 -> `Dusk`; during Main7 -> `DuskSmog`; during Main6 -> `DuskSmog`. W5's SaintAdaWakes
blends `DuskSmog` -> `Dusk` over its wave (`ULightingStateSubsystem::SetState(TEXT("Dusk"), ELightingSwitch::Blend, 6.f)`).
After Main 7: Day (W1's exact edit stops `build_area_main5.py`'s SmogThin rule after Main 7).

## Line sets (W1 writes them in create_story_lines.py; C++ loads by name: `LandfallRules::LoadLines`/`Say`, Vane through
`UVaneBroadcast::Say` / an `ACompanyLoudspeaker::Broadcast`). Speakers: HOLLOWAY 'Captain Holloway', FINCH 'Brother Finch',
JUNIE 'Junie Calloway', HOB 'Hob', VANE 'Cornelius Vane', NED 'Lucky Ned', ABEL 'Abel', DELIA 'Delia Ransom', SULLY 'Sully',
STOKER 'The Chief Stoker'. Captions under 90 characters.

| Set | Said by / when | Lines |
|---|---|---|
| `DA_Lines_Main6CastOff` | W2: the winch's hold done, she lifts | HOLLOWAY "Anchor's up. Hold on to something, Warden." / JUNIE "Engine's hot. Bag's holding. Mostly." |
| `DA_Lines_Main6Tenders` | W2: the tenders sighted | HOLLOWAY "Two tenders off the barge! Coming up on us!" / JUNIE "Shoot their bags! They burn lovely!" |
| `DA_Lines_Main6TenderDown` | W2: a tender falls (each) | JUNIE "That's one cooked!" (first) / HOLLOWAY "Second one's down. Watch the deck!" (second) — two one-line sets: `DA_Lines_Main6TenderDown1`, `DA_Lines_Main6TenderDown2` |
| `DA_Lines_Main6Boarders` | W2: the escort tender's plank down | HOLLOWAY "They're boarding! Clear my deck, Warden!" |
| `DA_Lines_Main6Launchers` | W2: step 2 starts (alongside the barge) | HOLLOWAY "Alongside! Starboard launchers, Warden. Junie rigged them." / JUNIE "Their own grapples. Waste not." |
| `DA_Lines_Main6Flicker` | W2: the grapples bite (step 2 done) | FINCH "Her light's flickering! Hurry!" |
| `DA_Lines_Main6Plank` | W2: the plank down (step 3 starts) | HOLLOWAY "Plank's down. Go. We'll hold her here." |
| `DA_Lines_Main6VaneDeck` | W3: the barge's horns as step 3 starts | VANE "Welcome aboard, Warden. Mind the vents. They bite." / VANE "Gentlemen, the Warden is trespassing. See to it." |
| `DA_Lines_Main6Vents` | W3: the first vent burst near the player | HOB "Vents glow before they blow, sunshine. Count to one." |
| `DA_Lines_Main6DrumJammed` | W3: the hose drum jammed | FINCH "It's stopped! Bless you, Warden." |
| `DA_Lines_Main6FinchDimming` | W3: while the drum isn't jammed, once at step 4 and once at step 5 | FINCH "She's dimming, Warden. That hose is nearly on her roof." |
| `DA_Lines_Main6Manifest` | W3: the manifest taken | HOB "Vane's fare bill. The Ferryman'll frame it." |
| `DA_Lines_Main6NedRadio` | W3: the office's radio, after the manifest | NED "Barge, this is Purcell. Is my keeper still burning?" / NED "I've money on him. Don't let him go out." |
| `DA_Lines_Main6Hall` | W3: the hall entered (step 5 starts) | STOKER "Keep her lit! Mister Vane's watching!" / VANE "Warden, those valves are worth more than you are." |
| `DA_Lines_Main6Valve1`, `..Valve2`, `..Valve3` | W3: each valve shut | STOKER "She's cooling! Stoke her!" / HOB "Two down. She's choking, sunshine." / FINCH "The cage, Warden! Now, while she's cold!" |
| `DA_Lines_Main6Cage` | W3: CageBurst, after the burst | VANE "Oh, that's not good." / VANE "Gentlemen, abandon my barge. Leave the Warden." |
| `DA_Lines_Main6Jump` | W2: the ship under the gangway (step 7) | HOLLOWAY "She's going down! Get to the gangway!" / HOLLOWAY "Jump, Warden!" |
| `DA_Lines_Main6Fall` | W3: BargeFall's captions | HOB "There she goes. Into the canyon." / FINCH "Look. Something came down on the boards." |
| `DA_Lines_Main7Coal` | W5: the first soul coal cracked | HOB "A soul in every lump, sunshine. They'll follow the lantern." |
| `DA_Lines_Main7Gate` (Vane, `Loudspeaker_KeepersGate`) | W1's cue: Main 7 from step 4 | VANE "That was a very expensive barge, Warden." / VANE "I have forty more. Do come and see them." |
| `DA_Lines_Main7Road` | W5: WestRoadCrew seen | HOB "Company men off the barge. They want his light, sunshine." |
| `DA_Lines_Main7Latch` | W5: the first latch on the lantern | FINCH "They're draining him! Break that line!" |
| `DA_Lines_Main7Arrive` | W5: step 5 starts | FINCH "Bring him here, Warden. On the lid. Gently." |
| `DA_Lines_SaintAdaWakes` | W5: the scene | ABEL "Ma. You came up the hill. Your knees." / DELIA "Hush. You held the door." / ABEL "I'll keep her now. From in here. She won't go out again." / ABEL "Warden. Whatever they took me for, there's more of it. Go and get them back." |
| `DA_Lines_FreeAbel` | W4: the scene | ABEL "That's my lantern. You've been looking after it." / ABEL "I can't get home on my own. Will you carry me?" |
| `DA_Lines_AbelCallBack1`, `DA_Lines_AbelCallBack2` | W4: each call-back's hold done | ABEL "...Ma? Is that my lantern?" / ABEL "Who are you? You're not from here." |
| `DA_Lines_AbelMutter` | W4: his walk toward the six (phase 1), one line a time, in turn | ABEL "Dusk. It's dusk. Who's sitting with them?" / ABEL "Six on the boards. Somebody walk them." / ABEL "Saint Ada's dark. Why is she dark?" |
| `DA_Lines_AbelCallHint` | W4: the first lock (Hob) | HOB "He's down! Get to the keeper's post. Call him back!" |
| `DA_Lines_AbelDrink` | W4: the first drink line (Hob) | HOB "He's drinking the dead! Shoot the smog where it bites!" |
| `DA_Lines_AbelSurge` | W4: phase 3's fog surge (Hob) | HOB "Can't see a thing. Aim for the coal, sunshine." |
| `DA_Lines_AbelFinal` | W4: zero Smog, the last call | FINCH "Now, Warden! Hold the post! Call him home!" |
| `DA_Lines_DeliaMain7` / `DA_Lines_DeliaMain7Jammed` | W1: Main 7's turn-in topic at the chapel door | the design's four lines (Holloway's on the tube last); Jammed has DELIA "Finch says you stopped that hose short of her roof." first |
| `DA_Lines_HollowayMain6` | W1: Holloway's word mid-Main 6 (talked to on deck) | HOLLOWAY "Eleven years I've wanted a shot at one of his barges." |
| `DA_Lines_TillyMain7` (reworded) | W1 | "Your car's coupled, Warden. Mind the brass." |
| `DA_Lines_AbelReliquary` | W1: Abel's soul at the Reliquary after Main 7 (topic) | ABEL "She's warm again. I can feel the whole valley sleeping." / ABEL "Go on, Warden. I'm not going anywhere." |
| `DA_Lines_HobMain6Wheel`, `DA_Lines_HobMain7Point`, `DA_Lines_HobMain7Chapel` | W1: Hob's perch arrivals | (W1 words) |

## Existing sound cues to use (examples; no new ones)
Ship: `ShipEngine`, `ShipCreak`, `ShipLurch`, `ShipBagTear`, `GrappleFire`, `GrappleHit`, `GrappleStrain`, `GrappleCut`,
`TenderFall`, `AnchorCrank`, `AnchorSet`. Vane: `VaneRadioIn/Out/Hiss`. Dredgers: `DredgerHoseLatch/Drain/Snap`,
`DredgerTankBurst`. Big moments: `Explosion`, `ExplosionDistant`, `ImpactMetal`, `ImpactHeavy`, `BossStagger`, `BossPhase`,
`StingPhase`, `StingBossVictory`, `MissionStep`, `OptionalDone`, `ItemTaken`. Fire/heat: `FuseHiss`, `FuseLight`, `Sear`,
`Burn`, `Sizzle`, `GaugeVent`, `Boiler`, `Crank`. Abel: `AbelIntro`, `AbelWail`, `AbelLanternsOut`, `AbelFogFlare`,
`AbelWindRise`, `AbelStagger`, `AbelDeath`, `UnpaidRise`, `UnpaidDissolve`, `SoulLightApply`, `SoulLightKill`. Chapel:
`ChapelBellToll`, `ChapelBellHum`. Music: `MusicBossKeeper`, `MusicBossKeeperRage`. Check the exact `LooterSoundCue::`
names in `Audio/LooterSoundCues.h` (some sit in nested groups).

## Your report (hand back to MAIN67)
Files (new / staged-changed, with one line each), the CODEMAP lines, exact edits for others' files (old/new text), what
the build script logs to check, the tests (names), a PIE walkthrough for your part, risks (what you couldn't check), numbers
to tune. Under 1,500 words, plain terse notes.

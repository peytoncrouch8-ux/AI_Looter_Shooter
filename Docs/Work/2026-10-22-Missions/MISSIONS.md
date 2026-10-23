# Missions rework (2026-10-22)

The user, verbatim: "So regarding the missions. The main missions shouldn't have to be accepted and should automatically be
added to the missions list. The side missions are all automatically placed in the world and should only be available after
progression is made. Lastly; I shouldn't have to listen to dialogue before being able to accept a quest. Once I accept a
quest it should move or skip to the dialogue that plays once you accept, borderlands style."

C++ in the holding tree `Saved/MissionsCode/Source/AI_Looter_Shooter/...` (originals' hashes in `Saved/MissionsCode/HASHES.txt`;
`bash Tools/hashcheck.sh Saved/MissionsCode/HASHES.txt` before copying). Python edited in place.

## Rules

1. **Main missions are never offered.** Data still says `EMissionStart::Offered` with giver, lines and briefing; the code
   decides (`UMissionDefinition::IsMainByItself` / `IsMetFirst`).
   - **By itself** (Main 2, 4, 5, 7): the moment it's due (`StartDue`) it's recorded accepted, started at step 0, tracked,
     and `Offer.Lines` then `Offer.AcceptLines` are queued as captions in the giver's name (`MissionTalk::SayOnTheMove`,
     after the turn-in's words). `IsAvailable` is false for it: no ?, no card, no pin.
   - **Met first** (`FMissionOffer::bMeetFirst`; Main 3 and Main 6, both "go ask Delia"): due, it's a *meeting*: Missions
     page Active, tracker "Talk to Grandma Delia" with the arrow on her, gold ? over her. Not running: "during MainN"
     conditions don't hold yet (Main 6's dusk waits for her talk). Talking to her says `Offer.Lines`, then
     `USpeakerPointComponent::StartMet` accepts it (after the Talk event) and queues `AcceptLines`. No card. Meetings are
     derived, never saved (`UMissionRunner::IsMeeting`); display lines in `UMissionRunner::Meetings`.
   - Turn-ins unchanged. A main mission the runner starts is always tracked (also `AcceptMission` of a main).
2. **Side missions: story gate = `Prerequisites`** (already in the data; unchanged). Before it: `IsAvailable` false, so no ?
   (GiverMarkSubsystem), no minimap/map pin, no card, the artifact's Interact unusable. Accepted ones in old saves keep
   running (`StartsByItself`: accepted).
3. **Offer card at once.** `Talk` queues the offer with no wait: the card opens on the 0.1 s timer beside the words
   (`ArmSeconds` 0.35 still guards the opening key). The captions keep playing under an offer-mode card
   (`ALooterHUD::CoversCaptions`) and are lifted over its dimming (`LiftCaptions`, z 31). **Accept**:
   `MissionTalk::SayAccepted` → `USpeakerPointComponent::SayInstead(AcceptLines)` (Interrupt: offer words cut, accept lines
   at once; none: only stops). Board/artifact accept lines are queued. **Not now**: `MissionTalk::StopGiver` (Clear while
   the giver's words play). A choice card still waits for the turn-in's words; a job offered right after a turn-in opens
   when its own words come up (or silence), so Accept never cuts the turn-in or a main mission's words.
4. Bounty board (own flow), mission banner, pacing (rewards unchanged), Skyreach/tutorial missions (Tutorial kind): untouched.

## Gates (side missions)

| Mission | Gate | Level band then |
|---|---|---|
| Side1 Wanted: Already Dead, CaldersCaches, OldFare | Main3 | ~2.7 |
| Side2 Unfinished Business, KnockingBelow | Main4 | ~3.3-3.8 |
| Side3 The Gravemother, DeadLift | Main5 | ~4.1-4.8 |
| LastWatch, BackPay | DeadLift | 5-6 |

All match `Docs/Areas/RansomsRest_Expansion.md` section 2 and the assets (checked by name in the .uasset files). Pinned by
`Looter.Missions.MainStart.SideGates`.

## Data (rerun both scripts)

- `create_mission_assets.py`: `offer(accept_lines=(), meet_first=False)`; Main 3 and Main 6 `meet_first=True`.
- `create_side_mission_assets.py`: `offer(accept_lines=())`. Accept lines: Old Fare (Sloane's "If you're dead too..."
  moved), Dead Lift (Ellis "Riding." + Hiram's ask moved; the "Riding." hero row still matches), Unfinished Business (new:
  "Obliged. I'll be at the fence."), Knocking Below (new: "The key's on the hook in the workshop. Mind the steps."). No
  family words; no hero_lines rows needed. Boards/artifacts: none.

## Save handling

No version bump. A main mission available-but-never-accepted in an old save: by-itself ones start (accepted) at the first
update, words said; met-first ones show as meetings. Accepted side missions run on. `ActiveMission`/`ReadyMissions`
resume as before.

## Files

Changed: `Missions/MissionDefinition.h` (FMissionOffer `AcceptLines`, `bMeetFirst`; `IsMainByItself`, `IsMetFirst`),
`Missions/MissionRunner.h/.cpp`, `MissionRunnerOffers.cpp`, `MissionRunnerFlow.cpp`, `MissionRunnerEvents.cpp`,
`Story/SpeakerPointComponent.h/.cpp`, `SpeakerPointComponentOffers.cpp`, `UI/World/MissionOfferWidget.cpp`,
`UI/HUD/LooterHUD.h`, `LooterHUDMissionOffer.cpp`, `HudCaptionWidget.cpp`, `UI/Inventory/MissionsWidgetDetails.cpp`,
`Tests/MissionGiverTests.cpp` (OffersAt: mains met first; Talk: the main starts by itself, the card at once),
`Tests/MissionGiverViewTests.cpp` (MapPins, Marks: the main met first).
New: `Missions/MissionRunnerMeetings.cpp`, `Missions/MissionTalk.h/.cpp`, `Tests/MissionMainStartTests.cpp`.

CODEMAP lines to add (Missions):
- `Missions/MissionRunnerMeetings.cpp`: main missions never offered: meetings (`IsMeeting`, `MeetText`, "Talk to <giver>"
  lines in the display), tracking and words on the move for ones started by themselves (`SayStartTalks`).
- `Missions/MissionTalk`: `MissionTalk::SayOnTheMove`, `SayAccepted`, `StopGiver`, `InGiverName`: a giver's offer/accept lines.
- Tests: `MissionMainStartTests.cpp` (`Looter.Missions.MainStart.ByItself`, `.MeetFirst`, `.OldSave`, `.AcceptTalk`,
  `.SideGates`, `.Assets`).

Pipeline note for `Docs/Pipeline/Story.md` "How words reach the player": the offer card no longer waits for the words
(rules 1-3 above, in short).

## What Main runs

1. Hash check, copy the tree into `Source/`, CODEMAP lines, `Tools\launch.ps1 -Build`.
2. `create_mission_assets.py`, then `create_side_mission_assets.py` (Tools\console.ps1 "py <abs path>").
3. Tests: `Looter.Missions` (Givers, MainStart incl. `.Assets` which needs step 2), `Looter.Story`, `Looter.Economy.VendorVoice`,
   `Looter.Poster`, `Looter.Lines.NoFamily`, `Looter.UI.NoticeBoard`, `Looter.Bounty`, `Looter.Progression.Pacing`,
   `Looter.Tutorial`.
4. PIE (Ransom's Rest, new session): Main 1's turn-in → Main 2 tracked, Hob's lines on the move; Sexton's turn-in → "Talk
   to Grandma Delia" + gold ? → her talk starts Main 3 (no card); Tilly's Main 3 turn-in → Main 4 tracked, her line after the
   turn-in; Mr. Sloane: card at once, his words above the card, Accept jumps to his accept line, Not now silences him; Main
   5's turn-in → Main 6 meeting, dusk only after Delia's talk; Side 3/Dead Lift givers show nothing before Main 5. Load an
   old session that had Main N offered: it starts on load.

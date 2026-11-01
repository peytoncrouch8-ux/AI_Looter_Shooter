# Lane TURNIN: the turn-in card (card first, then the talk), every mission

The user: "main missions should have a UI that pops up to turn it in if the final step is to talk to someone or turn in to a
board"; picked "Every mission" and "Card, then talk". Choice turn-ins (two rewards) keep their choice card unchanged.
Not built, not run (lane rules). S = `Source/AI_Looter_Shooter/`.

## The flow

- Speaker point (someone, a vendor's voice, Delia): Interact -> `USpeakerPointComponent::Talk` -> CanTalk -> `OpenTurnInCard`
  (private, Offers.cpp): a mission ready here (`FindTurnInAt`) with no choice + a HUD for the listener ->
  `ALooterHUD::OpenTurnInCard(Id, Owner)`. Nothing is said, no Talk event, nothing turned in; `OnTalked` still broadcast (the
  character turns to the player). No HUD (tests/headless) or the card refused -> `TalkNow` (today's whole Talk body, unchanged).
- Card's Turn In (E / Interact, Enter/Space/gamepad A on the focused button, mouse click) -> `UMissionOfferWidget::TurnIn` ->
  `TurnInAtGiver`: speaker point -> `USpeakerPointComponent::TurnInFromCard(Listener, Id)` -> `TalkNow` (turn-in lines in the
  giver's voice, Talk event turns it in, follow-up offer queued with `QueueOffer(..., bWaitForWords=true)`); board ->
  `ANoticeBoard::TurnIn` (existing, lines as written); artifact -> new `AMissionArtifact::TurnInFromCard` (Talk event, then
  TurnIn.Lines as written); no giver -> `Runner->TurnIn` + lines in `GetGiverName()`. All lines are played while the card is
  up; the caption widget holds them (`CoversCaptions` is true for every mode but Offer), so they're heard once the card is
  gone; the follow-up offer waits for the card to close (IsMenuOpen) and for those words (existing pending-offer rules).
- Stamp: TURNED IN (the ACCEPTED plate, mark colour), `LooterSoundCue::TurnInStamp` (+ `BoardTurnIn` at a board), a burst
  (soft glow + 16-ray starburst in the mark colour, spreading and fading 0.5 s from the stamp's landing), the card thumps
  (2.2% squash over 0.16 s), holds 0.8 s (`TurnInStampHoldSeconds`), lifts away. The fanfare stays the mission-complete
  banner's (no double).
- Not now (Esc / B / the button): closes, mission stays ready; the next talk opens the card again.
- Board: `UNoticeBoardWidget::Act` on a Ready posting with no choice -> `HUD->OpenTurnInCard(Id, Board)` over the board's
  screen (bOverScreen), back to the board on close. No HUD (tests' screens) -> today's direct `ANoticeBoard::TurnIn`.
- Artifact: `AMissionArtifact::Use` with a waiting non-choice turn-in -> HUD present: `OpenTurnInCard` (false if a menu is
  open); no HUD -> `TurnInFromCard` at once.

## The card (EMissionCardMode::TurnIn, `FMissionOfferCard::MakeTurnIn`)

- Header: TurnIn `!` mark (gold main / white side), kind "Mission complete", title, "Turn in to <giver> · <place>" (someone)
  or "Turn in at <board/artifact title> · <place>". Giver name: `TurnIn.GiverName` if set, else the speaker point's
  `SpeakerName`, else the board's/artifact's `Title`, else from the tag.
- "What you did" (`Card.Done`, `FMissionDoneLine`): every step's objectives in order by their tracker line
  (`GetShortText`): required ones ticked (XPLight tick), optional done = bonus (SoulGold tick, "BONUS" tag), the LAST step's
  optional ones not done = open (dim box, "Optional: ...", "STILL OPEN" tag, plus the hint "Not now keeps it ready: what's
  still open can be done first"). Missed optionals of earlier steps and `UMissionSceneObjective` (Landfall's hidden
  HardLanding) are left out. Max 10 lines, then "And N more". Lines slide/fade in 0.06 s apart from 0.16 s, ticks pop.
- Rewards: `DescribeRewards` (XP, silver purse, gun in rarity colour, named gun, areas) + "Bonus: +N silver · K optional
  done" (`MissionRewards::OptionalBonus`, SoulGold) when earned.
- Buttons: Not now | Turn in (primary, focused). Prompts as the offer card's (E Turn in / Esc Not now; pad A/B).

## Files

| File | What |
|---|---|
| `S/UI/World/MissionOfferWidget.h` | `EMissionCardMode::TurnIn` (appended), `FMissionDoneLine`, `FMissionOfferCard::Done/bOptionalOpen/MakeTurnIn/HasTurnInCard`, widget `OpenTurnIn`, `TurnIn`, static `TurnInAtGiver`, `WasTurnedIn`, `TurnInStampHoldSeconds`, private `GetStampHoldSeconds`, `PressPrimary`, `BuildTurnInContent`, `MakeDoneList`, `MakeBurst`, `ApplyTurnInMotion`, members `DoneRows`, `DoneTicks`, `BurstGlow`, `BurstRays`. |
| `S/UI/World/MissionOfferWidget.cpp` | BuildCard: TurnIn branch, burst under the stamp, resets the new members; header glyph TurnIn for the mode; `GetStampHoldSeconds`. |
| `S/UI/World/MissionOfferWidgetCard.cpp` | `HasTurnInCard`, `MakeTurnIn`. |
| `S/UI/World/MissionOfferWidgetInput.cpp` | `PressPrimary` (Accept or Turn In) for E and the primary button; TurnIn mode uses the offer's prompts. |
| `S/UI/World/MissionOfferWidgetMotion.cpp` | stamp hold via `GetStampHoldSeconds`; calls `ApplyTurnInMotion` in TurnIn mode. |
| `S/UI/World/MissionOfferWidgetTurnIn.cpp` (new) | the mode: open, Turn In, `TurnInAtGiver`, the list, the burst, its motion. |
| `S/UI/HUD/LooterHUD.h`, `S/UI/HUD/LooterHUDMissionOffer.cpp` | `bool OpenTurnInCard(FName MissionId, AActor* Giver)`. |
| `S/Story/SpeakerPointComponent.h/.cpp`, `SpeakerPointComponentOffers.cpp` | `Talk` = CanTalk + `OpenTurnInCard` (private) else `TalkNow` (private, the old body); public `TurnInFromCard(Listener, MissionId)`. |
| `S/World/MissionArtifact.h/.cpp` | `Use` opens the card; public `TurnInFromCard(MissionId)`. |
| `S/UI/World/NoticeBoardWidgetCards.cpp` | Act on a Ready posting opens the turn-in card (direct turn-in only without a HUD). |
| `S/Tests/MissionTurnInCardTests.cpp` (new) | the tests below. |

`World/NoticeBoard.{h,cpp}` needed no change (its `TurnIn` already turns in then says the lines).

## CODEMAP lines (Main writes)

- Replace the `UI/World/MissionOfferWidget` line with: ``- `UI/World/MissionOfferWidget`: `UMissionOfferWidget`, `FMissionOfferCard`, `EMissionCardMode` (Offer, Choice, Read, TurnIn), `FMissionChoiceCard`, `FMissionDoneLine`; card has Accept / Not now and the ACCEPTED stamp; turn-in mode (`OpenTurnIn`, `MakeTurnIn`, `TurnIn`, `TurnInAtGiver`): what you did, rewards, Turn In / Not now, TURNED IN stamp with a burst. `MissionOfferWidgetCard.cpp`, `MissionOfferWidgetInput.cpp`, `MissionOfferWidgetMotion.cpp`, `MissionOfferWidgetChoice.cpp`, `MissionOfferWidgetRead.cpp`, `MissionOfferWidgetTurnIn.cpp`.``
- `Story/SpeakerPointComponent` line: after "(Talk tap: captions + Talk event; turns in ready missions)" add "; a turn-in with no choice opens the turn-in card first (`ALooterHUD::OpenTurnInCard`), its Turn In runs the talk (`TurnInFromCard`)".
- `World/MissionArtifact` line: add "; a mission turned in at it opens the turn-in card (`TurnInFromCard`)".
- `UI/HUD/LooterHUD` line: in the `LooterHUDMissionOffer.cpp` list add `OpenTurnInCard`.
- `UI/World/NoticeBoardWidget` line: add "a Ready posting opens the turn-in card over the board".
- Tests line: add ``MissionTurnInCardTests.cpp` (`Looter.Missions.TurnInCard.*`: Card, CardThenWords, NotNow, Board, Artifact, ChoiceUnchanged, NoHUD)`.

## Hooks for Main (other lanes' files)

1. **SOUND dependency (build blocker):** `MissionOfferWidgetTurnIn.cpp` uses `LooterSoundCue::TurnInStamp` (top level of
   `namespace LooterSoundCue`, value "UI.TurnIn.Stamp", per the contract). If SOUND put it in a sub-namespace, change that one
   line in `TurnIn()`.
2. **Optional, `S/UI/World/NoticeBoardWidget.cpp` `HandleOfferClosed`** (not my file): the board says what a card turn-in gave,
   as its direct turn-in did. Exact edit:
   old:
   ```
   	const bool bMain = Was && Was->Mission && Was->Mission->Kind == EMissionKind::Main;
   	Refresh();
   	if (bAccepted)
   	{
   		SetStatus(FText::Format(LOCTEXT("Taken", "{0}: taken. The tracker follows it."), Title), MissionMarkIcons::Color(bMain));
   	}
   ```
   new:
   ```
   	const bool bMain = Was && Was->Mission && Was->Mission->Kind == EMissionKind::Main;
   	const bool bWasReady = Was && Was->State == ENoticePosting::Ready;
   	const UMissionDefinition* WasMission = Was ? Was->Mission : nullptr;
   	Refresh();
   	if (bAccepted)
   	{
   		SetStatus(FText::Format(LOCTEXT("Taken", "{0}: taken. The tracker follows it."), Title), MissionMarkIcons::Color(bMain));
   	}
   	else if (const FMissionRewardsGiven* Given = bWasReady ? GivenHere.Find(MissionId) : nullptr)
   	{
   		// Turned in on its card over the board: what it gave, as a turn-in here always said.
   		const FText Gave = WasMission ? DescribeGiven(WasMission->Rewards, *Given) : FText::GetEmpty();
   		SetStatus(Gave.IsEmpty() ? FText::Format(LOCTEXT("TurnedIn", "{0}: turned in."), Title)
   			: FText::Format(LOCTEXT("TurnedInGave", "{0}: turned in. {1}."), Title, Gave), Given->GunColor);
   	}
   ```
   (check `FNoticeBoardPosting::Mission`'s type is a const pointer; `Was` must not be read after `Refresh()`.)
3. Docs: Pipeline Game.md (the mission cards section) and Handoff: "every non-choice turn-in is card first
   (`OpenTurnInCard`), then the giver's words".

## Run order

Nothing to run but the build (after SOUND's cue exists) and the tests. No assets, no scripts.

## Tests (new; `Tools\runtests.ps1 -Filter Looter.Missions.TurnInCard`)

- `Looter.Missions.TurnInCard.Card`: MakeTurnIn's words: kind, title, "Turn in to Delia Ransom", 5 deeds in order (scene left
  out), bonus done, last step's optional open, XP/silver/bonus rewards, buttons; `HasTurnInCard` false for automatic and choice.
- `.CardThenWords`: the widget opened, not armed -> nothing; Turn In -> completed, Delia's turn-in line on screen in her voice,
  follow-up offer pending behind it, stamp holds, no second turn-in.
- `.NotNow`: Decline keeps it ready, nothing said, no press on its way out; reopened -> turned in.
- `.Board`: "Turn in at Notice Board", turned in through the card, the board's line as written.
- `.Artifact`: "Turn in at Ruth's tally", card turn-in with its words; no-HUD `Use` turns in at once.
- `.ChoiceUnchanged`: the turn-in card never settles a choice; the talk still pends the choice card.
- `.NoHUD`: a talk with no HUD turns in at once with her words (today's path).

Existing tests expected to need changes: **none** found. All existing turn-in tests (`Looter.Missions.TurnIn.*`,
`.Choice.*`, `.Givers.*`, `.BoardNote.*`, `Looter.UI.NoticeBoard.*`, story tests that Talk to a giver: SevenDays, Lantern
Leans, Gravewind, Heirloom, ColdWelcome, Amos, Unfinished Business, TalkBusiness, Seed, Story) use marker actors or pawns with
no PlayerController, or `UNoticeBoardWidget::Open(nullptr, ...)`, so `ALooterHUD::FindFor` is null and the direct path runs
unchanged. Worth running once after the build: `-Filter Looter.Missions` and `-Filter Looter.UI.NoticeBoard`.

## PIE check for Main

1. New game -> Landfall to the turn-in; E at Delia: the card (gold `!`, "Mission complete", "Landfall", "Turn in to Delia
   Ransom · Ransom Farm"), the deeds tick in (HardLanding absent; the hat as BONUS if picked up, else not listed — it's step 7,
   not the last step), rewards (450 XP, 20 silver, bonus if any). No caption while it's up.
2. Esc: it closes, Delia still says "Turn in" on the key; E again: the card again.
3. E (and once with the mouse, once with a pad's A): stamp + burst + TurnInStamp sound, card goes ~1.1 s later, Delia's
   `DA_Lines_DeliaLandfall` lines play, the mission banner; then whatever comes next (Main 2 starting by itself, or an offer
   card after her words).
4. A board turn-in (Skyreach board, or a bounty board): the card over the board's screen, Turn In, back to the board.
5. Back Pay (choice) still: words first, then the choice card. 6. `Tools\uishots.ps1`/grab of the card at 1080p for the look.

## Risks / not checked

- Not compiled. Watch: `UE_DOUBLE_TWO_PI`, `GetOwningPlayerPawn()`, the `for (UImage* Piece : { Glow, Rays })` list.
- Several missions ready at the same speaker: the card shows the first (`FindTurnInAt`); Turn In's talk turns in all of them,
  as the talk always did (the banner lists each).
- The Talk event is sent at Turn In, not at the card's opening: another mission's "Talk to Delia" objective counts only once
  the player presses Turn In (Not now leaves it uncounted).
- `TurnInFromCard` skips `CanTalk` (the card already validated it); a giver hidden by a scene between open and Turn In
  would still talk.
- Artifact turn-in without a HUD now also says its TurnIn.Lines (before: nothing was said there at all).
- Card height for Landfall: ~10 deed lines + 3-4 rewards, about 640 px; fine at 1080p; capped at 10 lines.
- Burst rays texture is 600 px drawn up to ~570 px (PPU 6): slightly soft by design. Tune: `BurstSeconds` 0.5,
  `GlowTo` 2.1, `RaysTo` 1.9, `RaysTurn` 14, `ThumpDepth` 0.022, `TurnInStampHoldSeconds` 0.8, `ListStagger` 0.06.
- The open sound reuses `MissionFeatures::ChoiceOpen`; a dedicated "UI.TurnIn.Open" cue could be asked of SOUND later.

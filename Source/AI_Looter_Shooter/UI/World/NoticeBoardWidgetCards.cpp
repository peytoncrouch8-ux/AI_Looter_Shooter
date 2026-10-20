// UNoticeBoardWidget: the posting cards (title, notice, count, reward, what can be done here) and acting on one: turning it
// in, tracking it or taking it (the offer card).

#include "UI/World/NoticeBoardWidget.h"
#include "Audio/LooterSound.h"
#include "Economy/EconomyRules.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionRewards.h"
#include "Missions/MissionRunner.h"
#include "Story/HeroLines.h"
#include "UI/Inventory/LoadoutParts.h"
#include "UI/HUD/LooterHUD.h"
#include "UI/Style/LooterButton.h"
#include "UI/Style/LooterUIFocus.h"
#include "UI/Style/LooterUIStyle.h"
#include "UI/Style/MissionMarkIcons.h"
#include "UI/World/MissionOfferWidget.h"
#include "Weapons/WeaponDefinition.h"
#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

#define LOCTEXT_NAMESPACE "NoticeBoard"

using namespace LooterUI;

namespace
{
	const FName ActionCard(TEXT("Card"));

	/** A sound asked of Main (Audio/LooterSoundCues.h and Art/Sounds/cues.json): a pin pulled and a stamp. Until made, it plays nothing. */
	const FName TurnInCue(TEXT("UI.BoardTurnIn"));

	constexpr float CardMinHeight = 96.f;
	/** The notice wraps here, leaving the card's right side to its state and key. */
	constexpr float NoticeWrap = 470.f;
	constexpr float SideWidth = 190.f;

	FText RarityName(EWeaponRarity Rarity)
	{
		return UEnum::GetDisplayValueAsText(Rarity);
	}

	/** The reward gun's kind by name ("Pump Shotgun"), or empty when the mission rolls one from the loot table. */
	FText GunKindName(const FMissionRewards& Rewards)
	{
		return Rewards.GunKind && !Rewards.GunKind->DisplayName.IsEmpty() ? Rewards.GunKind->DisplayName : FText::GetEmpty();
	}

	/**
	 * The rewards in a few words each, from the same numbers the offer card lists (FMissionOfferCard::Make): the experience,
	 * the purse (the rewards' own, else the giver's kept by PurseId), the gun with its floor, the named gun, the areas opened.
	 */
	TArray<FText> RewardParts(const FMissionRewards& Rewards, FName PurseId)
	{
		TArray<FText> Parts;
		if (Rewards.Experience > 0)
		{
			Parts.Add(FText::Format(LOCTEXT("PartXP", "+{0} XP"), FText::AsNumber(Rewards.Experience)));
		}
		else if (Rewards.ExperienceShare > 0.f)
		{
			Parts.Add(FText::Format(LOCTEXT("PartXPShare", "+{0}% of a level"), FText::AsNumber(FMath::RoundToInt32(Rewards.ExperienceShare * 100.f))));
		}
		if (const int32 Purse = MissionRewards::PurseOf(Rewards, PurseId); Purse > 0)
		{
			Parts.Add(FText::Format(LOCTEXT("PartSilver", "+{0} silver"), FText::FromString(LooterEconomy::Format(Purse))));
		}
		if (Rewards.bGun)
		{
			const FText Kind = GunKindName(Rewards);
			const FText Gun = Kind.IsEmpty() ? LOCTEXT("AGun", "a gun") : Kind;
			Parts.Add(Rewards.GunRarityFloor != EWeaponRarity::Common
				? FText::Format(LOCTEXT("PartGunFloor", "{0}, {1} or better"), Gun, RarityName(Rewards.GunRarityFloor)) : Gun);
		}
		if (!Rewards.NamedGun.IsNone())
		{
			Parts.Add(MissionRewards::NamedGunName(Rewards));
		}
		for (const FName AreaId : Rewards.UnlockAreas)
		{
			Parts.Add(FText::Format(LOCTEXT("PartArea", "the way to {0}"), MissionRewards::AreaName(AreaId)));
		}
		return Parts;
	}

	/** "Reward: +1,300 XP · +60 silver"; with nothing, the practice line for a practice posting, else nothing at all. */
	FText JoinReward(const TArray<FText>& Parts, bool bPractice)
	{
		if (Parts.IsEmpty())
		{
			// Skyreach's practice postings give nothing: they're done for the practice. A story job giving nothing says nothing.
			return bPractice ? LOCTEXT("RewardNone", "No reward: it's practice") : FText::GetEmpty();
		}
		return FText::Format(LOCTEXT("RewardList", "Reward: {0}"), FText::Join(LOCTEXT("RewardSeparator", " · "), Parts));
	}
}

// ---------------------------------------------------------------------------
// Words
// ---------------------------------------------------------------------------

FText UNoticeBoardWidget::DescribeReward(const FMissionRewards& Rewards, FName PurseId, bool bPractice)
{
	return JoinReward(RewardParts(Rewards, PurseId), bPractice);
}

FText UNoticeBoardWidget::DescribeReward(const UMissionDefinition& Mission)
{
	// The purse by the mission's own id, as its offer card finds it; a choice at the turn-in said as the card says it.
	TArray<FText> Parts = RewardParts(Mission.Rewards, Mission.GetMissionId());
	if (Mission.HasTurnInChoice())
	{
		Parts.Add(LOCTEXT("PartChoice", "your call at the end"));
	}
	return JoinReward(Parts, Mission.Kind == EMissionKind::Tutorial);
}

FText UNoticeBoardWidget::DescribeGiven(const FMissionRewards& Rewards, const FMissionRewardsGiven& Given)
{
	if (Given.bGun)
	{
		const FText Kind = GunKindName(Rewards);
		return FText::Format(LOCTEXT("GivenGun", "{0} {1}, at your feet"), RarityName(Given.GunRarity),
			Kind.IsEmpty() ? LOCTEXT("Gun", "gun") : Kind);
	}
	if (!Given.NamedGun.IsEmpty())
	{
		return FText::Format(LOCTEXT("GivenNamed", "{0}, at your feet"), Given.NamedGun);
	}
	return FText::GetEmpty();
}

FText UNoticeBoardWidget::StateWord(const FNoticeBoardPosting& Posting) const
{
	switch (Posting.State)
	{
	case ENoticePosting::Locked:
		return LOCTEXT("StateLocked", "Not posted yet");
	case ENoticePosting::Offered:
		// A job only turned in here (Calder's Caches: Ruth's note offers it) is up for the taking, but not on this board.
		return Posting.bOfferedHere ? LOCTEXT("StateOffered", "Up for the taking") : LOCTEXT("StateOfferedElsewhere", "Offered elsewhere");
	case ENoticePosting::Open:
	{
		const FText Word = Posting.bTracked ? LOCTEXT("StateTracked", "Tracked") : LOCTEXT("StateOpen", "Open");
		return Posting.Progress.IsEmpty() ? Word : FText::Format(LOCTEXT("StateCount", "{0}  {1}"), Word, FText::FromString(Posting.Progress));
	}
	case ENoticePosting::Ready:
		return Posting.bTurnedInHere ? LOCTEXT("StateReady", "Ready to turn in") : LOCTEXT("StateNearly", "Done");
	case ENoticePosting::Done:
		return LOCTEXT("StateDone", "Done");
	}
	return FText::GetEmpty();
}

FLinearColor UNoticeBoardWidget::StateColor(const FNoticeBoardPosting& Posting) const
{
	switch (Posting.State)
	{
	case ENoticePosting::Locked: return Color::TextDim();
	case ENoticePosting::Offered:
		return Posting.bOfferedHere ? MissionMarkIcons::Color(Posting.Mission && Posting.Mission->Kind == EMissionKind::Main) : Color::TextDim();
	case ENoticePosting::Open:   return Posting.bTracked ? Color::Accent() : Color::CyanText();
	case ENoticePosting::Ready:  return Color::Accent();
	case ENoticePosting::Done:   return Color::Better();
	}
	return Color::TextDim();
}

// ---------------------------------------------------------------------------
// A card
// ---------------------------------------------------------------------------

UWidget* UNoticeBoardWidget::MakeCard(int32 Index)
{
	const FNoticeBoardPosting& Posting = Postings[Index];
	const UMissionDefinition* Mission = Posting.Mission;
	const bool bFaint = Posting.State == ENoticePosting::Done || Posting.State == ENoticePosting::Locked;
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

	// A strip down the left side: orange to turn in, gold or white for a job to take here, cyan while open, faint once done
	// or while locked. A job offered by someone else (only turned in here) has no mark: it can't be taken here.
	const bool bOffered = Posting.State == ENoticePosting::Offered && Posting.bOfferedHere;
	const bool bMain = Mission && Mission->Kind == EMissionKind::Main;
	const FLinearColor Strip = Posting.State == ENoticePosting::Ready ? Color::Accent() : bOffered ? MissionMarkIcons::Color(bMain)
		: bFaint ? Color::RowLine() : Color::Title();
	Row->AddChildToHorizontalBox(MakeSized(WidgetTree, MakeImage(WidgetTree, RectBrush(Strip)), 3.f));

	// The notice: its title (a job to take with its !), its words in the town's voice, and its reward (what it gave, once
	// turned in here).
	UVerticalBox* Info = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	const FString Title = Mission ? Mission->Title.ToString() : Posting.MissionId.ToString();
	UTextBlock* TitleText = MakeText(WidgetTree, Title, 17, bFaint ? Color::TextDim() : Color::Title(), false, 40);
	TitleText->SetTextOverflowPolicy(ETextOverflowPolicy::Ellipsis);
	if (bOffered)
	{
		UHorizontalBox* TitleRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		UHorizontalBoxSlot* MarkSlot = TitleRow->AddChildToHorizontalBox(MakeImage(WidgetTree,
			MissionMarkIcons::MarkBrush(MissionMarkIcons::EMark::Offer, bMain, FVector2D(22.f))));
		MarkSlot->SetVerticalAlignment(VAlign_Center);
		MarkSlot->SetPadding(FMargin(0.f, 0.f, 6.f, 0.f));
		UHorizontalBoxSlot* TitleSlot = TitleRow->AddChildToHorizontalBox(TitleText);
		TitleSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		TitleSlot->SetVerticalAlignment(VAlign_Center);
		Info->AddChildToVerticalBox(TitleRow);
	}
	else
	{
		Info->AddChildToVerticalBox(TitleText);
	}
	if (Mission && !Mission->Summary.IsEmpty())
	{
		UTextBlock* Notice = MakeText(WidgetTree, HeroLines::Resolve(Mission->Summary).ToString(), 12, bFaint ? Color::TextDim() : Color::Text());
		Notice->SetWrapTextAt(NoticeWrap);
		Info->AddChildToVerticalBox(Notice)->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));
	}
	if (Mission && Posting.State == ENoticePosting::Offered && (!Posting.bOfferedHere || Posting.PinnedGiver.IsValid()))
	{
		// Where it is picked up: "Ruth's note, at Calder's Post" when this board only takes it in; "Ruth Calder's note,
		// pinned to this board" when that note hangs here (taken here all the same).
		UTextBlock* Where = MakeText(WidgetTree, WhereToTake(Posting).ToString(), 11, Color::CyanText());
		Where->SetWrapTextAt(NoticeWrap);
		Info->AddChildToVerticalBox(Where)->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));
	}
	if (Mission)
	{
		// What it gave once turned in here, else what it pays (the offer card's numbers); a story job paying nothing says
		// nothing rather than calling itself practice.
		const FMissionRewardsGiven* Given = GivenHere.Find(Posting.MissionId);
		const FText Gave = Given ? DescribeGiven(Mission->Rewards, *Given) : FText::GetEmpty();
		const FText Pays = Gave.IsEmpty() ? DescribeReward(*Mission) : FText::GetEmpty();
		UTextBlock* Reward = !Gave.IsEmpty() ? MakeText(WidgetTree, Gave.ToString(), 11, Given->GunColor, true, 100)
			: !Pays.IsEmpty() ? MakeText(WidgetTree, Pays.ToString(), 10, Color::TextDim(), true, 150) : nullptr;
		if (Reward)
		{
			Info->AddChildToVerticalBox(Reward)->SetPadding(FMargin(0.f, 6.f, 0.f, 0.f));
		}
	}
	UHorizontalBoxSlot* InfoSlot = Row->AddChildToHorizontalBox(Info);
	InfoSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	InfoSlot->SetVerticalAlignment(VAlign_Center);
	InfoSlot->SetPadding(FMargin(12.f, 0.f, 12.f, 0.f));

	// On the right: where it stands, and the key for what can be done with it here.
	UVerticalBox* Side = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	UVerticalBoxSlot* StateSlot = Side->AddChildToVerticalBox(MakeText(WidgetTree, StateWord(Posting).ToString(), 11, StateColor(Posting), true, 120));
	StateSlot->SetHorizontalAlignment(HAlign_Right);
	FCard& Card = Cards[Index];
	if (CanAct(Index))
	{
		const bool bTurnIn = Posting.State == ENoticePosting::Ready;
		const FText Does = bTurnIn ? LOCTEXT("ActTurnIn", "Turn in") : bOffered ? LOCTEXT("ActTake", "Take a look") : LOCTEXT("ActTrack", "Track");
		UWidget* Hint = LoadoutParts::MakeKeyHint(WidgetTree, ActKeyName(), Does.ToString(), bTurnIn || bOffered);
		// Its keycap's words follow the device (E, a pad's A): the first text in the hint is the cap's.
		UWidgetTree::ForWidgetAndChildren(Hint, [&Card](UWidget* Each)
		{
			Card.KeyText = Card.KeyText ? Card.KeyText : Cast<UTextBlock>(Each);
		});
		UVerticalBoxSlot* KeySlot = Side->AddChildToVerticalBox(Hint);
		KeySlot->SetHorizontalAlignment(HAlign_Right);
		KeySlot->SetPadding(FMargin(0.f, 8.f, 0.f, 0.f));
	}
	Row->AddChildToHorizontalBox(MakeSized(WidgetTree, Side, SideWidth))->SetVerticalAlignment(VAlign_Center);

	UOverlay* CardBox = LoadoutParts::MakeCard(WidgetTree, Row, FMargin(10.f, 10.f, 14.f, 10.f), 1.5f, Card.Fill, Card.Line);
	USizeBox* Sized = MakeSized(WidgetTree, CardBox, 0.f);
	Sized->SetMinDesiredHeight(CardMinHeight);
	// Every card is a button and can be chosen (a click, W and S): one that can't be acted on here (offered elsewhere,
	// tracked already, done, locked) says why when it's pressed, rather than ignoring the click (SayWhyNot). The kit's
	// focus look (an orange frame, a lit edge, a faint wash) sits under the card's words: lit while keys drive the board.
	Card.Focus = AddFocusFrame(CardBox);
	ULooterButton* Button = WidgetTree->ConstructWidget<ULooterButton>(ULooterButton::StaticClass());
	Button->SetupContent(Sized, ActionCard, Index, EButtonKind::Bare);
	// A row is clicked as the button goes down, as a list's rows are: the press alone chooses it (or acts on the chosen one),
	// with no release over the same card to wait for.
	Button->SetClickMethod(EButtonClickMethod::MouseDown);
	Button->OnButtonClicked.BindUObject(this, &UNoticeBoardWidget::HandleButton);
	Card.Button = Button;
	return Button;
}

// ---------------------------------------------------------------------------
// Acting on a card
// ---------------------------------------------------------------------------

bool UNoticeBoardWidget::CanAct(int32 Index) const
{
	if (!Postings.IsValidIndex(Index))
	{
		return false;
	}
	const FNoticeBoardPosting& Posting = Postings[Index];
	return (Posting.State == ENoticePosting::Ready && Posting.bTurnedInHere) || (Posting.State == ENoticePosting::Open && !Posting.bTracked)
		|| (Posting.State == ENoticePosting::Offered && Posting.bOfferedHere);
}

void UNoticeBoardWidget::Act(int32 Index)
{
	UMissionRunner* Runner = GetRunner();
	const ANoticeBoard* From = Board.Get();
	if (!Runner || !From || !CanAct(Index))
	{
		LooterSound::Play2D(this, LooterSoundCue::Denied);
		return;
	}
	// A copy: the list is read again below.
	const FNoticeBoardPosting Posting = Postings[Index];
	const FText Title = Posting.Mission ? Posting.Mission->Title : FText::FromName(Posting.MissionId);

	if (Posting.State == ENoticePosting::Offered)
	{
		// A job to take: the offer card over the board, which comes back as it closes (HandleOfferClosed). One picked up from
		// a note pinned here opens from the note (its card reads "Found: Ruth Calder's note"), else from the board.
		ALooterHUD* HUD = OwningHUD.Get();
		AActor* Giver = Posting.PinnedGiver.IsValid() ? Posting.PinnedGiver.Get() : Board.Get();
		if (!HUD || !Posting.Mission || !HUD->OpenMissionOffer(Posting.Mission, Giver))
		{
			LooterSound::Play2D(this, LooterSoundCue::Denied);
		}
		return;
	}

	if (Posting.State == ENoticePosting::Ready)
	{
		// A turn-in with a choice of two asks on its card over the board; the card turns it in.
		if (Posting.Mission && Posting.Mission->HasTurnInChoice())
		{
			ALooterHUD* HUD = OwningHUD.Get();
			if (!HUD || !HUD->OpenTurnInChoice(Posting.Mission, Board.Get()))
			{
				LooterSound::Play2D(this, LooterSoundCue::Denied);
			}
			return;
		}
		if (!From->TurnIn(*Runner, Posting.MissionId))
		{
			LooterSound::Play2D(this, LooterSoundCue::Denied);
			SetStatus(FText::Format(LOCTEXT("CantTurnIn", "{0} can't be turned in just now."), Title), Color::Worse());
			return;
		}
		// The pin and the stamp; the tracker sounds the fanfare when the tracked mission ends, the board for any other.
		LooterSound::Play2D(this, TurnInCue);
		if (!Posting.bTracked)
		{
			LooterSound::Play2D(this, LooterSoundCue::MissionComplete);
		}
		Refresh();
		const FMissionRewardsGiven* Given = GivenHere.Find(Posting.MissionId);
		const FText Gave = Given && Posting.Mission ? DescribeGiven(Posting.Mission->Rewards, *Given) : FText::GetEmpty();
		SetStatus(Gave.IsEmpty() ? FText::Format(LOCTEXT("TurnedIn", "{0}: turned in."), Title)
			: FText::Format(LOCTEXT("TurnedInGave", "{0}: turned in. {1}."), Title, Gave), Given ? Given->GunColor : Color::Better());
		return;
	}

	// Open and not tracked: the minimap and the tracker follow it from now on.
	From->Track(*Runner, Posting.MissionId);
	Refresh();
	SetStatus(FText::Format(LOCTEXT("Tracking", "Tracking {0}."), Title), Color::Title());
}

FText UNoticeBoardWidget::WhereToTake(const FNoticeBoardPosting& Posting) const
{
	if (!Posting.Mission)
	{
		return FText::GetEmpty();
	}
	// Pinned on this very board (Ruth's note on the Rangers' board): here, not at the street the map names round it.
	if (const AActor* Pinned = Posting.PinnedGiver.Get())
	{
		return FText::Format(LOCTEXT("PinnedHere", "{0}, pinned to this board"), FMissionOfferCard::GiverNameOf(*Posting.Mission, Pinned));
	}
	// As the Missions page says it: its giver's kind and name, and the place on the map nearest them.
	const UMissionRunner* Runner = GetRunner();
	const AActor* Giver = Runner ? Runner->FindOfferGiverActor(*Posting.Mission) : nullptr;
	return FMissionOfferCard::WhereToGet(*Posting.Mission, Giver, GetWorld(), FText::GetEmpty());
}

void UNoticeBoardWidget::SayWhyNot(int32 Index)
{
	LooterSound::Play2D(this, LooterSoundCue::Denied);
	if (!Postings.IsValidIndex(Index))
	{
		return;
	}
	const FNoticeBoardPosting& Posting = Postings[Index];
	const FText Title = Posting.Mission ? Posting.Mission->Title : FText::FromName(Posting.MissionId);
	FText Why;
	switch (Posting.State)
	{
	case ENoticePosting::Offered:
		Why = FText::Format(LOCTEXT("WhyElsewhere", "{0} is picked up elsewhere: {1}."), Title, WhereToTake(Posting));
		break;
	case ENoticePosting::Open:
		Why = FText::Format(LOCTEXT("WhyTracked", "{0} is tracked already."), Title);
		break;
	case ENoticePosting::Ready:
		Why = Posting.Mission ? FText::Format(LOCTEXT("WhyTurnInElsewhere", "{0} is turned in to {1}."), Title, Posting.Mission->GetGiverName())
			: FText::GetEmpty();
		break;
	case ENoticePosting::Done:
		Why = FText::Format(LOCTEXT("WhyDone", "{0} is done."), Title);
		break;
	case ENoticePosting::Locked:
	{
		const ANoticeBoard* From = Board.Get();
		Why = From && !From->LockedNote.IsEmpty() ? From->LockedNote : FText::Format(LOCTEXT("WhyLocked", "{0} isn't posted yet."), Title);
		break;
	}
	}
	SetStatus(Why, Color::TextDim());
}

void UNoticeBoardWidget::HandleMissionCompleted(const UMissionDefinition& Mission, const FMissionRewardsGiven& Given)
{
	// Only what this board turned in matters to its cards; the rest are kept harmlessly until it closes.
	if (!Given.IsEmpty())
	{
		GivenHere.Add(Mission.GetMissionId(), Given);
	}
}

#undef LOCTEXT_NAMESPACE

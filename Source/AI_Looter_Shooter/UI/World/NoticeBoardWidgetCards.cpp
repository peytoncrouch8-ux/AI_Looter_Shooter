// UNoticeBoardWidget: the posting cards (title, notice, count, reward, what can be done here) and acting on one: turning it
// in or tracking it.

#include "UI/World/NoticeBoardWidget.h"
#include "Audio/LooterSound.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionRewards.h"
#include "Missions/MissionRunner.h"
#include "UI/Inventory/LoadoutParts.h"
#include "UI/Style/LooterButton.h"
#include "UI/Style/LooterUIStyle.h"
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
}

// ---------------------------------------------------------------------------
// Words
// ---------------------------------------------------------------------------

FText UNoticeBoardWidget::DescribeReward(const FMissionRewards& Rewards)
{
	if (Rewards.bGun)
	{
		const FText Kind = GunKindName(Rewards);
		const FText Gun = Kind.IsEmpty() ? LOCTEXT("AGun", "a gun") : Kind;
		if (Rewards.GunRarityFloor != EWeaponRarity::Common)
		{
			return FText::Format(LOCTEXT("RewardFloor", "Reward: {0}, {1} or better"), Gun, RarityName(Rewards.GunRarityFloor));
		}
		return FText::Format(LOCTEXT("RewardGun", "Reward: {0}"), Gun);
	}
	if (!Rewards.NamedGun.IsNone())
	{
		return FText::Format(LOCTEXT("RewardNamed", "Reward: {0}"), MissionRewards::NamedGunName(Rewards));
	}
	// Skyreach gives no experience: a posting without a gun is done for the practice.
	return LOCTEXT("RewardNone", "No reward: it's practice");
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

	// A strip down the left side: orange to turn in, cyan while open, faint once done or while locked.
	const FLinearColor Strip = Posting.State == ENoticePosting::Ready ? Color::Accent() : bFaint ? Color::RowLine() : Color::Title();
	Row->AddChildToHorizontalBox(MakeSized(WidgetTree, MakeImage(WidgetTree, RectBrush(Strip)), 3.f));

	// The notice: its title, its words in the town's voice, and its reward (what it gave, once turned in here).
	UVerticalBox* Info = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	const FString Title = Mission ? Mission->Title.ToString() : Posting.MissionId.ToString();
	UTextBlock* TitleText = MakeText(WidgetTree, Title, 17, bFaint ? Color::TextDim() : Color::Title(), false, 40);
	TitleText->SetTextOverflowPolicy(ETextOverflowPolicy::Ellipsis);
	Info->AddChildToVerticalBox(TitleText);
	if (Mission && !Mission->Summary.IsEmpty())
	{
		UTextBlock* Notice = MakeText(WidgetTree, Mission->Summary.ToString(), 12, bFaint ? Color::TextDim() : Color::Text());
		Notice->SetWrapTextAt(NoticeWrap);
		Info->AddChildToVerticalBox(Notice)->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));
	}
	if (Mission)
	{
		const FMissionRewardsGiven* Given = GivenHere.Find(Posting.MissionId);
		const FText Gave = Given ? DescribeGiven(Mission->Rewards, *Given) : FText::GetEmpty();
		UTextBlock* Reward = Gave.IsEmpty()
			? MakeText(WidgetTree, DescribeReward(Mission->Rewards).ToString(), 10, Color::TextDim(), true, 150)
			: MakeText(WidgetTree, Gave.ToString(), 11, Given->GunColor, true, 100);
		Info->AddChildToVerticalBox(Reward)->SetPadding(FMargin(0.f, 6.f, 0.f, 0.f));
	}
	UHorizontalBoxSlot* InfoSlot = Row->AddChildToHorizontalBox(Info);
	InfoSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	InfoSlot->SetVerticalAlignment(VAlign_Center);
	InfoSlot->SetPadding(FMargin(12.f, 0.f, 12.f, 0.f));

	// On the right: where it stands, and the key for what can be done with it here.
	UVerticalBox* Side = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	UVerticalBoxSlot* StateSlot = Side->AddChildToVerticalBox(MakeText(WidgetTree, StateWord(Posting).ToString(), 11, StateColor(Posting), true, 120));
	StateSlot->SetHorizontalAlignment(HAlign_Right);
	if (CanAct(Index))
	{
		const bool bTurnIn = Posting.State == ENoticePosting::Ready;
		const FText Does = bTurnIn ? LOCTEXT("ActTurnIn", "Turn in") : LOCTEXT("ActTrack", "Track");
		UVerticalBoxSlot* KeySlot = Side->AddChildToVerticalBox(LoadoutParts::MakeKeyHint(WidgetTree, TEXT("E"), Does.ToString(), bTurnIn));
		KeySlot->SetHorizontalAlignment(HAlign_Right);
		KeySlot->SetPadding(FMargin(0.f, 8.f, 0.f, 0.f));
	}
	Row->AddChildToHorizontalBox(MakeSized(WidgetTree, Side, SideWidth))->SetVerticalAlignment(VAlign_Center);

	FCard& Card = Cards[Index];
	UOverlay* CardBox = LoadoutParts::MakeCard(WidgetTree, Row, FMargin(10.f, 10.f, 14.f, 10.f), 1.5f, Card.Fill, Card.Line);
	USizeBox* Sized = MakeSized(WidgetTree, CardBox, 0.f);
	Sized->SetMinDesiredHeight(CardMinHeight);
	if (!CanAct(Index))
	{
		// Done, locked or already tracked: nothing to press.
		return Sized;
	}
	ULooterButton* Button = WidgetTree->ConstructWidget<ULooterButton>(ULooterButton::StaticClass());
	Button->SetupContent(Sized, ActionCard, Index, EButtonKind::Bare);
	Button->OnButtonClicked.BindUObject(this, &UNoticeBoardWidget::HandleButton);
	Button->OnButtonHovered.BindUObject(this, &UNoticeBoardWidget::HandleHovered);
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
	return (Posting.State == ENoticePosting::Ready && Posting.bTurnedInHere) || (Posting.State == ENoticePosting::Open && !Posting.bTracked);
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

	if (Posting.State == ENoticePosting::Ready)
	{
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

void UNoticeBoardWidget::HandleMissionCompleted(const UMissionDefinition& Mission, const FMissionRewardsGiven& Given)
{
	// Only what this board turned in matters to its cards; the rest are kept harmlessly until it closes.
	if (!Given.IsEmpty())
	{
		GivenHere.Add(Mission.GetMissionId(), Given);
	}
}

#undef LOCTEXT_NAMESPACE

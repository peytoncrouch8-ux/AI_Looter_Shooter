// UMissionOfferWidget's turn-in mode: a mission with no choice to make, turned in card first (the user's call, 2026-10-31:
// "Every mission", "Card, then talk"). What the player did ticks in line by line, then what it pays; Turn In has the giver
// turn it in (their words queued under the card, heard once it's gone), a TURNED IN stamp slams down with a burst of light;
// Not now leaves it ready for the next visit.

#include "UI/World/MissionOfferWidget.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionRunner.h"
#include "Story/CaptionSubsystem.h"
#include "Story/SpeakerPointComponent.h"
#include "UI/Inventory/LoadoutParts.h"
#include "UI/Style/LooterUIStyle.h"
#include "UI/Style/MissionMarkIcons.h"
#include "World/MissionArtifact.h"
#include "World/NoticeBoard.h"
#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"

#define LOCTEXT_NAMESPACE "MissionOffer"

using namespace LooterUI;

namespace
{
	/** More lines than this and the rest are counted ("And 3 more"): the card stays on the screen. */
	constexpr int32 MaxDoneLines = 10;
	constexpr float TickSize = 14.f;
	/** A tag at a line's end ("BONUS", "STILL OPEN"). */
	constexpr float TagWidth = 96.f;

	/** The list comes in after the card has risen, a line at a time, each sliding in from the left as its tick pops. */
	constexpr float ListDelay = 0.16f;
	constexpr float ListStagger = 0.06f;
	constexpr float LineInSeconds = 0.2f;
	constexpr float LineSlide = -12.f;
	constexpr float TickDelay = 0.08f;
	constexpr float TickPopSeconds = 0.22f;

	/**
	 * The burst under the stamp: drawn small (so it never grows the card's layout, the shortest card being taller) and
	 * scaled out as it fades, from the moment the stamp lands.
	 */
	constexpr float BurstBox = 300.f;
	constexpr float BurstSeconds = 0.5f;
	constexpr float GlowFrom = 0.5f;
	constexpr float GlowTo = 2.1f;
	constexpr float RaysFrom = 0.6f;
	constexpr float RaysTo = 1.9f;
	/** How far the rays turn as they spread (degrees). */
	constexpr float RaysTurn = 14.f;
	/** The card gives under the stamp's blow, then springs back. */
	constexpr float ThumpSeconds = 0.16f;
	constexpr float ThumpDepth = 0.022f;

	float Saturate(float Value)
	{
		return FMath::Clamp(Value, 0.f, 1.f);
	}

	float EaseOutCubic(float Alpha)
	{
		const float Left = 1.f - Saturate(Alpha);
		return 1.f - Left * Left * Left;
	}

	/** The tick by a deed done. */
	const FVectorIcon& TickIcon()
	{
		static const FVectorIcon Icon = []
		{
			FVectorIcon Tick;
			Tick.ViewBox = FVector2D(12.f, 12.f);
			Tick.Strokes.Add({ FVector2D(2.2, 6.4), FVector2D(4.9, 9.1), FVector2D(9.8, 3.2) });
			Tick.StrokeWidth = 2.f;
			return Tick;
		}();
		return Icon;
	}

	/** The empty box by an optional objective still open. */
	const FVectorIcon& OpenIcon()
	{
		static const FVectorIcon Icon = []
		{
			FVectorIcon Box;
			Box.ViewBox = FVector2D(12.f, 12.f);
			Box.Strokes.Add({ FVector2D(2.5, 2.5), FVector2D(9.5, 2.5), FVector2D(9.5, 9.5), FVector2D(2.5, 9.5), FVector2D(2.5, 2.5) });
			Box.StrokeWidth = 1.4f;
			return Box;
		}();
		return Icon;
	}

	/** Sixteen rays out from the middle, long and short by turns: the light that bursts from under the stamp. */
	const FVectorIcon& RaysIcon()
	{
		static const FVectorIcon Icon = []
		{
			FVectorIcon Rays;
			Rays.ViewBox = FVector2D(100.f, 100.f);
			const FVector2D Middle(50.0, 50.0);
			constexpr int32 Count = 16;
			for (int32 Index = 0; Index < Count; ++Index)
			{
				// Clockwise on screen (y down): one side of the base, the tip, the other side.
				const double Angle = Index * UE_DOUBLE_TWO_PI / Count;
				const double Reach = Index % 2 == 0 ? 48.0 : 33.0;
				const double Half = Index % 2 == 0 ? 0.075 : 0.06;
				const double Base = 7.0;
				Rays.Fills.Add({
					Middle + Base * FVector2D(FMath::Cos(Angle - Half * 2.0), FMath::Sin(Angle - Half * 2.0)),
					Middle + Reach * FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)),
					Middle + Base * FVector2D(FMath::Cos(Angle + Half * 2.0), FMath::Sin(Angle + Half * 2.0)) });
			}
			return Rays;
		}();
		return Icon;
	}

	void ShowIf(UWidget* Widget, bool bShown)
	{
		const ESlateVisibility Wanted = bShown ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;
		if (Widget && Widget->GetVisibility() != Wanted)
		{
			Widget->SetVisibility(Wanted);
		}
	}
}

// ---------------------------------------------------------------------------
// Opening, turning in
// ---------------------------------------------------------------------------

void UMissionOfferWidget::OpenTurnIn(ALooterHUD* InHUD, const UMissionDefinition* InMission, AActor* InGiver, bool bInOverScreen)
{
	Card = InMission ? FMissionOfferCard::MakeTurnIn(*InMission, InGiver, GetWorld()) : FMissionOfferCard();
	OpenAs(EMissionCardMode::TurnIn, InHUD, InMission, InGiver, bInOverScreen);
	// The mission's end come round: the same rising paper a choice to make opens with.
	LooterSound::Play2D(this, LooterSoundCue::MissionFeatures::ChoiceOpen);
}

bool UMissionOfferWidget::TurnIn()
{
	if (!IsArmed() || Mode != EMissionCardMode::TurnIn)
	{
		return false;
	}
	const UMissionDefinition* Ready = Mission.Get();
	AActor* At = Giver.Get();
	if (!Ready || !TurnInAtGiver(this, *Ready, At, GetOwningPlayerPawn()))
	{
		LooterSound::Play2D(this, LooterSoundCue::Denied);
		SetStatus(LOCTEXT("CantTurnIn", "This can't be turned in here now."));
		return false;
	}
	bAccepted = true;
	bClosing = true;
	CloseAge = 0.f;
	Nav.SetShown(false);
	SetStatus(FText::GetEmpty());
	if (StampWord)
	{
		StampWord->SetText(LOCTEXT("TurnedIn", "TURNED IN"));
	}
	// The stamp's blow (the mission-complete banner sounds the fanfare with its rewards); a board's pin pulled with it.
	LooterSound::Play2D(this, LooterSoundCue::TurnInStamp);
	if (Cast<ANoticeBoard>(At))
	{
		LooterSound::Play2D(this, LooterSoundCue::BoardTurnIn);
	}
	UE_LOG(LogLooter, Log, TEXT("Turn-in card: %s turned in%s."), *Ready->GetMissionId().ToString(),
		At ? *FString::Printf(TEXT(" at %s"), *At->GetActorNameOrLabel()) : TEXT(""));
	ApplyMotion();
	return true;
}

bool UMissionOfferWidget::TurnInAtGiver(const UObject* WorldContext, const UMissionDefinition& ReadyMission, AActor* AtGiver, AActor* Listener)
{
	// (The parameters aren't named Mission and Giver: those are the widget's members, and a static function's parameters
	// hiding them is a shadowing error under UE's warnings.)
	UMissionRunner* Runner = UMissionRunner::Get(WorldContext);
	const FName MissionId = ReadyMission.GetMissionId();
	if (!Runner || !FMissionOfferCard::HasTurnInCard(ReadyMission) || !Runner->IsRunning(MissionId) || !Runner->IsReadyToTurnIn(MissionId))
	{
		return false;
	}
	// Each giver turns it in its own way, its words said after (they wait under the card): a board's tally, a found thing's.
	if (const ANoticeBoard* Board = Cast<ANoticeBoard>(AtGiver))
	{
		return Board->TurnIn(*Runner, MissionId);
	}
	if (AMissionArtifact* Artifact = Cast<AMissionArtifact>(AtGiver))
	{
		return Artifact->TurnInFromCard(MissionId);
	}
	// Someone at a speaker point: the talk the card stood in front of (their turn-in words in their voice, the Talk event
	// that turns it in, what they have next once those are said).
	if (USpeakerPointComponent* Point = AtGiver ? AtGiver->FindComponentByClass<USpeakerPointComponent>() : nullptr)
	{
		if (Point->TurnInFromCard(Listener, MissionId))
		{
			// The talk turns in everything ready for them; this one, should it somehow still wait, by the runner.
			return !Runner->IsRunning(MissionId) || Runner->TurnIn(MissionId);
		}
	}
	// Nobody here to say it (the console's card, a giver out of reach): the runner, the words in the giver's name.
	TArray<FStoryLine> Said = ReadyMission.TurnIn.Lines;
	if (!Runner->TurnIn(MissionId))
	{
		return false;
	}
	const FText Voice = ReadyMission.GetGiverName();
	for (FStoryLine& Line : Said)
	{
		if (Line.Speaker.IsEmpty())
		{
			Line.Speaker = Voice;
		}
	}
	UCaptionSubsystem* Captions = UCaptionSubsystem::Get(WorldContext);
	if (Captions && !Said.IsEmpty())
	{
		Captions->Play(Said, ECaptionPlay::Interrupt);
	}
	return true;
}

// ---------------------------------------------------------------------------
// Layout
// ---------------------------------------------------------------------------

void UMissionOfferWidget::BuildTurnInContent(UVerticalBox* Content)
{
	Content->AddChildToVerticalBox(MakeDoneList())->SetPadding(FMargin(0.f, 0.f, 0.f, 16.f));
	Content->AddChildToVerticalBox(MakeRewards());
	Content->AddChildToVerticalBox(StatusText)->SetPadding(FMargin(0.f, 14.f, 0.f, 0.f));
	Content->AddChildToVerticalBox(MakeButtons())->SetPadding(FMargin(0.f, 20.f, 0.f, 0.f));
}

UWidget* UMissionOfferWidget::MakeDoneList()
{
	DoneRows.Reset();
	DoneTicks.Reset();
	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Box->AddChildToVerticalBox(MakeSection(WidgetTree, LOCTEXT("WhatYouDid", "What you did").ToString()));
	if (Card.Done.IsEmpty())
	{
		Box->AddChildToVerticalBox(LoadoutParts::Label(WidgetTree, LOCTEXT("AllDone", "Everything it asked").ToString(), 9,
			Color::TextDim(), 150))->SetPadding(FMargin(0.f, 6.f, 0.f, 0.f));
		return Box;
	}
	// Done in the experience colour the tracker's finished steps have; a bonus in soul-light gold; what's open, dimmed.
	const FSlateBrush Tick = IconBrush(TEXT("TurnInTick"), TickIcon(), 3.f, FVector2D(TickSize), Color::XPLight());
	const FSlateBrush BonusTick = IconBrush(TEXT("TurnInTick"), TickIcon(), 3.f, FVector2D(TickSize), Color::SoulGold());
	const FSlateBrush OpenBox = IconBrush(TEXT("TurnInOpen"), OpenIcon(), 3.f, FVector2D(TickSize), Color::TextDim());
	for (int32 Index = 0; Index < Card.Done.Num(); ++Index)
	{
		if (Index == MaxDoneLines)
		{
			const int32 Left = Card.Done.Num() - MaxDoneLines;
			UWidget* More = LoadoutParts::Label(WidgetTree, FText::Format(LOCTEXT("MoreDone", "And {0} more"), Left).ToString(), 8,
				Color::TextDim(), 150);
			Box->AddChildToVerticalBox(More)->SetPadding(FMargin(TickSize + 12.f, 6.f, 0.f, 0.f));
			DoneRows.Add(More);
			DoneTicks.Add(nullptr);
			break;
		}
		const FMissionDoneLine& Line = Card.Done[Index];
		const bool bBonus = Line.bOptional && Line.bDone;
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		UImage* Mark = MakeImage(WidgetTree, !Line.bDone ? OpenBox : bBonus ? BonusTick : Tick);
		Mark->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		UHorizontalBoxSlot* MarkSlot = Row->AddChildToHorizontalBox(MakeSized(WidgetTree, Mark, TickSize, TickSize));
		MarkSlot->SetVerticalAlignment(VAlign_Top);
		MarkSlot->SetPadding(FMargin(2.f, 2.f, 10.f, 0.f));
		const FString Words = Line.bOptional && !Line.bDone
			? FText::Format(LOCTEXT("OpenOptional", "Optional: {0}"), FText::FromString(Line.Text)).ToString() : Line.Text;
		UTextBlock* Text = MakeText(WidgetTree, Words, 13, Line.bDone ? Color::Text() : Color::TextDim());
		Text->SetAutoWrapText(true);
		UHorizontalBoxSlot* TextSlot = Row->AddChildToHorizontalBox(Text);
		TextSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		TextSlot->SetVerticalAlignment(VAlign_Center);
		if (Line.bOptional)
		{
			// Said in a word at the line's end: a bonus earned, or one still to be had (Not now keeps it ready).
			const FText Tag = Line.bDone ? LOCTEXT("BonusTag", "Bonus") : LOCTEXT("OpenTag", "Still open");
			UHorizontalBoxSlot* TagSlot = Row->AddChildToHorizontalBox(MakeSized(WidgetTree, LoadoutParts::Label(WidgetTree,
				Tag.ToString(), 8, Line.bDone ? Color::SoulGold() : Color::TextDim(), 200), TagWidth));
			TagSlot->SetVerticalAlignment(VAlign_Center);
			TagSlot->SetPadding(FMargin(12.f, 0.f, 0.f, 0.f));
		}
		Box->AddChildToVerticalBox(Row)->SetPadding(FMargin(0.f, Index == 0 ? 8.f : 5.f, 0.f, 0.f));
		DoneRows.Add(Row);
		DoneTicks.Add(Mark);
	}
	if (Card.bOptionalOpen)
	{
		Box->AddChildToVerticalBox(LoadoutParts::Label(WidgetTree, LOCTEXT("OpenHint", "Not now keeps it ready: what's still open can be done first").ToString(),
			8, Color::TextDim(), 150))->SetPadding(FMargin(TickSize + 12.f, 10.f, 0.f, 0.f));
	}
	return Box;
}

UWidget* UMissionOfferWidget::MakeBurst()
{
	// In the mark's colour (gold for the story, white for a side job), hidden until the stamp lands.
	const FLinearColor Ink = MissionMarkIcons::Color(Card.bMain);
	UOverlay* Burst = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	Burst->SetVisibility(ESlateVisibility::HitTestInvisible);
	UImage* Glow = MakeImage(WidgetTree, GlowBrush(FVector2D(BurstBox), Ink));
	UImage* Rays = MakeImage(WidgetTree, IconBrush(TEXT("TurnInRays"), RaysIcon(), 6.f, FVector2D(BurstBox), Ink));
	for (UImage* Piece : { Glow, Rays })
	{
		Piece->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		Piece->SetVisibility(ESlateVisibility::Collapsed);
		UOverlaySlot* PieceSlot = Burst->AddChildToOverlay(MakeSized(WidgetTree, Piece, BurstBox, BurstBox));
		PieceSlot->SetHorizontalAlignment(HAlign_Center);
		PieceSlot->SetVerticalAlignment(VAlign_Center);
	}
	BurstGlow = Glow;
	BurstRays = Rays;
	return Burst;
}

// ---------------------------------------------------------------------------
// Motion
// ---------------------------------------------------------------------------

void UMissionOfferWidget::ApplyTurnInMotion()
{
	// What was done, a line at a time once the card has risen: each slides in as it fades up, its tick popping a moment
	// after (overshooting, then settling).
	for (int32 Index = 0; Index < DoneRows.Num(); ++Index)
	{
		const float Start = ListDelay + Index * ListStagger;
		const float In = EaseOutCubic((Age - Start) / LineInSeconds);
		if (UWidget* Row = DoneRows[Index])
		{
			Row->SetRenderOpacity(In);
			Row->SetRenderTranslation(FVector2D((1.f - In) * LineSlide, 0.f));
		}
		if (UWidget* Tick = DoneTicks.IsValidIndex(Index) ? DoneTicks[Index].Get() : nullptr)
		{
			const float Pop = Saturate((Age - Start - TickDelay) / TickPopSeconds);
			const float Scale = Pop <= 0.f ? 0.f : Pop < 0.6f ? FMath::Lerp(0.2f, 1.3f, EaseOutCubic(Pop / 0.6f))
				: FMath::Lerp(1.3f, 1.f, EaseOutCubic((Pop - 0.6f) / 0.4f));
			Tick->SetRenderScale(FVector2D(Scale));
		}
	}

	// The stamp lands: light bursts from under it and fades as it spreads, and the card gives under the blow.
	const float SinceLanded = bAccepted ? CloseAge - StampSlamSeconds : -1.f;
	const float Burst = Saturate(SinceLanded / BurstSeconds);
	const bool bBursting = SinceLanded >= 0.f && Burst < 1.f;
	ShowIf(BurstGlow, bBursting);
	ShowIf(BurstRays, bBursting);
	if (bBursting)
	{
		const float Spread = EaseOutCubic(Burst);
		if (BurstGlow)
		{
			BurstGlow->SetRenderScale(FVector2D(FMath::Lerp(GlowFrom, GlowTo, Spread)));
			BurstGlow->SetRenderOpacity((1.f - Burst) * 0.85f);
		}
		if (BurstRays)
		{
			BurstRays->SetRenderTransform(FWidgetTransform(FVector2D::ZeroVector, FVector2D(FMath::Lerp(RaysFrom, RaysTo, Spread)),
				FVector2D::ZeroVector, Spread * RaysTurn));
			BurstRays->SetRenderOpacity(FMath::Square(1.f - Burst));
		}
	}
	if (CardBody)
	{
		const float Thump = SinceLanded >= 0.f && SinceLanded < ThumpSeconds ? FMath::Sin(SinceLanded / ThumpSeconds * UE_PI) : 0.f;
		CardBody->SetRenderScale(FVector2D(1.f - ThumpDepth * Thump));
	}
}

#undef LOCTEXT_NAMESPACE

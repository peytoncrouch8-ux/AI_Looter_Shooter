// UHudMissionCompleteWidget: hearing missions end, taking turns with the level-up banner, the words and the show. Its
// pictures and widgets are built in HudMissionCompleteWidgetLayout.cpp.

#include "UI/HUD/HudMissionCompleteWidget.h"
#include "Audio/LooterSound.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionRunner.h"
#include "Scenes/SceneSubsystem.h"
#include "UI/HUD/HudLevelUpBannerWidget.h"
#include "UI/HUD/LooterHUD.h"
#include "UI/Style/LooterUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

using namespace LooterUI;

namespace
{
	/** The show: in by InSeconds (overshooting), settled by SettleSeconds, out from OutSeconds to the end. */
	constexpr float InSeconds = 0.3f;
	constexpr float SettleSeconds = 0.5f;
	constexpr float OutSeconds = 3.8f;
	constexpr float StartScale = 0.82f;
	constexpr float Overshoot = 1.04f;
	/** The name and the rewards come in a beat after the title, the rewards rising a little as they do. */
	constexpr float NameInFrom = 0.18f;
	constexpr float NameInTo = 0.45f;
	constexpr float RewardsInFrom = 0.55f;
	constexpr float RewardsInTo = 0.9f;
	constexpr float RewardsRise = 10.f;
	/** The rays turn this many degrees a second while it shows. */
	constexpr float RaysTurn = 14.f;

	/** The rewards row: its words, and the orange diamond between two of them. */
	constexpr int32 RewardSize = 17;
	constexpr int32 RewardSpacing = 160;
	constexpr float DiamondSize = 6.f;
	constexpr float DiamondGap = 14.f;

	float Step(float From, float To, float Time)
	{
		return FMath::Clamp((Time - From) / (To - From), 0.f, 1.f);
	}

	float EaseOut(float Alpha)
	{
		return FMath::InterpEaseOut(0.f, 1.f, Alpha, 2.f);
	}
}

TSharedRef<SWidget> UHudMissionCompleteWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildTree();
		PaintShow(/*bCovered*/ false);
	}
	return Super::RebuildWidget();
}

void UHudMissionCompleteWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	// Bound once, for the widget's whole life, not its Slate widget's: Slate's is dropped and rebuilt (NativeDestruct, then
	// NativeConstruct) whenever nothing holds it, and a mission ending in between must still be announced. The binding is
	// weak, so a widget gone is never called.
	BindRunner();
}

void UHudMissionCompleteWidget::NativeConstruct()
{
	Super::NativeConstruct();
	// It never takes the mouse.
	SetVisibility(ESlateVisibility::HitTestInvisible);
	// The level's runner, should it be another than the one first bound (a no-op when it's the same).
	BindRunner();
}

void UHudMissionCompleteWidget::BeginDestroy()
{
	if (UMissionRunner* Runner = BoundRunner.Get())
	{
		Runner->OnMissionCompleted.Remove(CompletedHandle);
	}
	BoundRunner.Reset();
	CompletedHandle.Reset();
	Super::BeginDestroy();
}

void UHudMissionCompleteWidget::BindRunner()
{
	UMissionRunner* Runner = UMissionRunner::Get(this);
	if (Runner == BoundRunner.Get())
	{
		return;
	}
	if (UMissionRunner* Old = BoundRunner.Get())
	{
		Old->OnMissionCompleted.Remove(CompletedHandle);
	}
	BoundRunner = Runner;
	CompletedHandle = Runner ? Runner->OnMissionCompleted.AddUObject(this, &UHudMissionCompleteWidget::HandleMissionCompleted) : FDelegateHandle();
}

void UHudMissionCompleteWidget::SetLevelUpBanner(UHudLevelUpBannerWidget* InLevelUpBanner)
{
	LevelUpBanner = InLevelUpBanner;
}

// ---------------------------------------------------------------------------
// What it announces, and taking turns with the level-up banner
// ---------------------------------------------------------------------------

void UHudMissionCompleteWidget::HandleMissionCompleted(const UMissionDefinition& Mission, const FMissionRewardsGiven& Given)
{
	// The story's missions; the tutorial's end has its own closing line on the tracker.
	if (Mission.Kind != EMissionKind::Tutorial)
	{
		Announce(Mission, Given);
	}
}

void UHudMissionCompleteWidget::Announce(const UMissionDefinition& Mission, const FMissionRewardsGiven& Given)
{
	FAnnouncement& Added = Queue.AddDefaulted_GetRef();
	Added.Name = Mission.Title;
	Added.Given = Given;
}

bool UHudMissionCompleteWidget::DeferLevelUp(int32 NewLevel)
{
	if (!IsBusy())
	{
		return false;
	}
	// Several level-ups from one turn-in show as the highest.
	PendingLevel = FMath::Max(PendingLevel, NewLevel);
	return true;
}

bool UHudMissionCompleteWidget::IsBusy() const
{
	return Time >= 0.f || !Queue.IsEmpty();
}

bool UHudMissionCompleteWidget::IsLevelUpShowing() const
{
	// The level-up banner collapses its root while it waits for a level-up (UHudLevelUpBannerWidget::PaintShow), so a
	// root on screen is a banner showing.
	const UHudLevelUpBannerWidget* LevelUp = LevelUpBanner.Get();
	const UWidget* Root = LevelUp ? LevelUp->GetRootWidget() : nullptr;
	return Root && Root->GetVisibility() != ESlateVisibility::Collapsed;
}

bool UHudMissionCompleteWidget::IsCovered() const
{
	const APlayerController* Player = GetOwningPlayer();
	const ALooterHUD* LooterHUD = Player ? Cast<ALooterHUD>(Player->GetHUD()) : nullptr;
	if (LooterHUD && LooterHUD->IsMenuOpen())
	{
		return true;
	}
	// A scene that holds the player puts the gameplay HUD away (the end of Main 6 finishes it as Pa sits down).
	if (USceneSubsystem::HidesGameplayHUD(this))
	{
		return true;
	}
	const UWorld* World = GetWorld();
	return World && World->IsPaused();
}

// ---------------------------------------------------------------------------
// The show
// ---------------------------------------------------------------------------

void UHudMissionCompleteWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	Advance(InDeltaTime);
}

void UHudMissionCompleteWidget::Advance(float DeltaSeconds)
{
	const bool bCovered = IsCovered();
	// The next one waits for the screen to be the player's and the level-up banner to be done.
	if (Time < 0.f && !Queue.IsEmpty() && !bCovered && !IsLevelUpShowing())
	{
		StartNext();
	}
	// Time under a menu or a scene doesn't count, so the whole show is seen once the game is back.
	if (Time >= 0.f && !bCovered)
	{
		Time += DeltaSeconds;
		if (Time >= ShowSeconds)
		{
			EndShow();
		}
	}
	PaintShow(bCovered);
}

void UHudMissionCompleteWidget::StartNext()
{
	Shown = Queue[0];
	Queue.RemoveAt(0);
	Time = 0.f;
	ApplyTexts();
	LooterSound::Play2D(this, LooterSoundCue::MissionComplete);
}

void UHudMissionCompleteWidget::EndShow()
{
	Time = -1.f;
	// A level-up the turn-in's experience brought shows now, unless another mission's end is still to come.
	if (Queue.IsEmpty() && PendingLevel > 0)
	{
		if (UHudLevelUpBannerWidget* LevelUp = LevelUpBanner.Get())
		{
			LevelUp->Show(PendingLevel);
		}
		PendingLevel = 0;
	}
}

void UHudMissionCompleteWidget::ApplyTexts()
{
	if (!NameText || !RewardsRow)
	{
		return;
	}
	NameText->SetText(Shown.Name.ToUpper());
	RewardsRow->ClearChildren();
	RewardWords.Reset();
	const FMissionRewardsGiven& Given = Shown.Given;
	if (Given.Experience > 0)
	{
		AddReward(FString::Printf(TEXT("+%s XP"), *FText::AsNumber(Given.Experience).ToString()), Color::CyanText());
	}
	if (Given.bGun)
	{
		// The rarity in its own colour, as the gun's name is shown wherever it goes.
		AddReward(UEnum::GetDisplayValueAsText(Given.GunRarity).ToString().ToUpper() + TEXT(" GUN"), Given.GunColor);
	}
	if (!Given.NamedGun.IsEmpty())
	{
		AddReward(Given.NamedGun.ToString().ToUpper(), Color::Accent());
	}
	for (const FText& Area : Given.AreasOpened)
	{
		AddReward(TEXT("OPENS ") + Area.ToString().ToUpper(), Color::Text());
	}
	RewardsRow->SetVisibility(RewardWords.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
}

void UHudMissionCompleteWidget::AddReward(const FString& Words, const FLinearColor& WordsColor)
{
	if (!RewardWords.IsEmpty())
	{
		// A small orange diamond between two rewards, as the rules' ends have.
		USizeBox* Diamond = MakeSized(WidgetTree, MakeImage(WidgetTree, RectBrush(Color::Accent())), DiamondSize, DiamondSize);
		Diamond->SetRenderTransformAngle(45.f);
		UHorizontalBoxSlot* DiamondSlot = RewardsRow->AddChildToHorizontalBox(Diamond);
		DiamondSlot->SetVerticalAlignment(VAlign_Center);
		DiamondSlot->SetPadding(FMargin(DiamondGap, 0.f));
	}
	UTextBlock* Text = MakeFloatingText(WidgetTree, RewardSize, WordsColor, RewardSpacing, ETextJustify::Center);
	Text->SetText(FText::FromString(Words));
	RewardsRow->AddChildToHorizontalBox(Text)->SetVerticalAlignment(VAlign_Center);
	RewardWords.Add(Words);
}

void UHudMissionCompleteWidget::PaintShow(bool bCovered)
{
	if (!Banner)
	{
		return;
	}
	const bool bShowing = Time >= 0.f && !bCovered;
	const ESlateVisibility Wanted = bShowing ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;
	if (Banner->GetVisibility() != Wanted)
	{
		Banner->SetVisibility(Wanted);
	}
	if (!bShowing)
	{
		return;
	}
	float Scale = 1.f;
	float Opacity = 1.f;
	if (Time < InSeconds)
	{
		const float In = EaseOut(Step(0.f, InSeconds, Time));
		Scale = FMath::Lerp(StartScale, Overshoot, In);
		Opacity = In;
	}
	else if (Time < SettleSeconds)
	{
		Scale = FMath::Lerp(Overshoot, 1.f, EaseOut(Step(InSeconds, SettleSeconds, Time)));
	}
	else if (Time > OutSeconds)
	{
		Opacity = 1.f - EaseOut(Step(OutSeconds, ShowSeconds, Time));
	}
	Banner->SetRenderScale(FVector2D(Scale));
	Banner->SetRenderOpacity(Opacity);
	if (NameText)
	{
		NameText->SetRenderOpacity(EaseOut(Step(NameInFrom, NameInTo, Time)));
	}
	if (RewardsRow)
	{
		const float RewardsIn = EaseOut(Step(RewardsInFrom, RewardsInTo, Time));
		RewardsRow->SetRenderOpacity(RewardsIn);
		RewardsRow->SetRenderTranslation(FVector2D(0.f, RewardsRise * (1.f - RewardsIn)));
	}
	if (Rays)
	{
		Rays->SetRenderTransformAngle(Time * RaysTurn);
	}
}

// ---------------------------------------------------------------------------
// For the tests
// ---------------------------------------------------------------------------

FString UHudMissionCompleteWidget::GetShownName() const
{
	return NameText ? NameText->GetText().ToString() : Shown.Name.ToString().ToUpper();
}

TArray<FString> UHudMissionCompleteWidget::GetShownRewards() const
{
	return RewardWords;
}

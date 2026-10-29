// UHudFindBannerWidget: the "FOUND" banner. What it shows and when is here; its pictures and widgets are built below.

#include "UI/HUD/HudFindBannerWidget.h"
#include "Scenes/SceneSubsystem.h"
#include "UI/HUD/HudGraveMarkedWidget.h"
#include "UI/HUD/HudLevelUpBannerWidget.h"
#include "UI/HUD/HudMissionCompleteWidget.h"
#include "UI/HUD/LooterHUD.h"
#include "UI/Style/LooterUIStyle.h"
#include "World/FindRules.h"
#include "World/FindSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
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
	constexpr float OutSeconds = 2.25f;
	constexpr float StartScale = 0.86f;
	constexpr float Overshoot = 1.04f;

	/** The parts come in one after another: the tally stamps, the name, the count line, then the experience counts up. */
	constexpr float TallyStampSeconds = 0.28f;
	constexpr float TallyStartScale = 1.5f;
	constexpr float NameInFrom = 0.15f;
	constexpr float NameInTo = 0.45f;
	constexpr float CountInFrom = 0.35f;
	constexpr float CountInTo = 0.65f;
	constexpr float XpCountFrom = 0.55f;
	constexpr float XpCountTo = 1.55f;

	/** The tally: 18 x 16 units of strokes drawn at this scale, its middle this far over the box's middle. */
	constexpr float TallyUnitsWide = 18.f;
	constexpr float TallyUnitsHigh = 16.f;
	constexpr float TallyScale = 2.2f;
	constexpr float TallyPixelsPerUnit = 4.f;
	constexpr float TallyRise = -53.f;

	/** A warm glow behind the words, so they lift off a bright sky without a panel. */
	constexpr float GlowWidth = 460.f;
	constexpr float GlowHeight = 120.f;
	constexpr float GlowOpacity = 0.14f;

	/** "FOUND": soul-gold, letter-spaced, its middle this far under the box's middle; the salt rules either side. */
	constexpr int32 TitleSize = 30;
	constexpr int32 TitleSpacing = 220;
	/** The room between the rules for the title (px), and the smallest type a long title shrinks to. */
	constexpr float TitleRoom = 420.f;
	constexpr int32 MinTitleSize = 17;
	constexpr float TitleDrop = -13.f;
	constexpr float RuleLength = 70.f;
	constexpr float RuleThickness = 2.f;
	constexpr float RuleDiamond = 3.f;
	constexpr float RuleArtHeight = 8.f;
	constexpr float RuleGap = 16.f;
	constexpr float PixelsPerUnit = 2.f;

	/** The find's name, then the count line and the experience. */
	constexpr int32 NameSize = 23;
	constexpr int32 NameSpacing = 100;
	constexpr float NameDrop = 22.f;
	constexpr int32 CountSize = 14;
	constexpr int32 CountSpacing = 140;
	constexpr float CountDrop = 51.f;
	constexpr float CountGap = 18.f;

	float Step(float From, float To, float Time)
	{
		return FMath::Clamp((Time - From) / (To - From), 0.f, 1.f);
	}

	float EaseOut(float Alpha)
	{
		return FMath::InterpEaseOut(0.f, 1.f, Alpha, 2.f);
	}

	/**
	 * The notches' tally (four cuts and a slash, the kit's TallyBrush's strokes) drawn inked: each stroke white over a wider ink
	 * one, so tinting it colours the white and leaves the ink dark, and it reads over sky and snow alike.
	 */
	const FPaintedIcon& InkedTallyPicture()
	{
		static const FPaintedIcon Picture = []
		{
			TArray<TArray<FVector2D>> Strokes;
			for (const float X : { 3.5f, 7.f, 10.5f, 14.f })
			{
				Strokes.Add({ FVector2D(X, 3.f), FVector2D(X, 13.f) });
			}
			Strokes.Add({ FVector2D(1.5f, 11.5f), FVector2D(16.5f, 4.5f) });
			FPaintedIcon Result;
			Result.ViewBox = FVector2D(TallyUnitsWide, TallyUnitsHigh);
			FPaintLayer Ink;
			Ink.Strokes = Strokes;
			Ink.StrokeWidth = 3.4f;
			Ink.Color = Color::Ink();
			Result.Layers.Add(Ink);
			FPaintLayer Cuts;
			Cuts.Strokes = Strokes;
			Cuts.StrokeWidth = 1.6f;
			Cuts.Color = FLinearColor::White;
			Result.Layers.Add(Cuts);
			return Result;
		}();
		return Picture;
	}

	/**
	 * A salt rule fading out away from the title, with a small soul-gold diamond at its end by the title: bAfter is the one
	 * after it (its diamond on the left, fading toward the right).
	 */
	const FPaintedIcon& RulePicture(bool bAfter)
	{
		auto Make = [](bool bRight)
		{
			FPaintedIcon Result;
			const float Total = RuleLength + RuleDiamond * 2.f + 2.f;
			Result.ViewBox = FVector2D(Total, RuleArtHeight);
			const float MidY = RuleArtHeight * 0.5f;
			const float LineFrom = bRight ? RuleDiamond * 2.f + 2.f : 0.f;
			const float LineTo = LineFrom + RuleLength;
			FPaintLayer Line;
			Line.Fills.Add(TArray<FVector2D>{ FVector2D(LineFrom, MidY - RuleThickness * 0.5f), FVector2D(LineTo, MidY - RuleThickness * 0.5f),
				FVector2D(LineTo, MidY + RuleThickness * 0.5f), FVector2D(LineFrom, MidY + RuleThickness * 0.5f) });
			Line.Color = Color::SaltLine().CopyWithNewOpacity(bRight ? 1.f : 0.f);
			Line.GradientTo = Color::SaltLine().CopyWithNewOpacity(bRight ? 0.f : 1.f);
			Line.GradientStart = FVector2D(LineFrom, MidY);
			Line.GradientEnd = FVector2D(LineTo, MidY);
			Result.Layers.Add(MoveTemp(Line));
			const float DiamondX = bRight ? RuleDiamond : Total - RuleDiamond;
			FPaintLayer Diamond;
			Diamond.Fills.Add(TArray<FVector2D>{ FVector2D(DiamondX, MidY - RuleDiamond), FVector2D(DiamondX + RuleDiamond, MidY),
				FVector2D(DiamondX, MidY + RuleDiamond), FVector2D(DiamondX - RuleDiamond, MidY) });
			Diamond.Color = Color::SoulGold();
			Result.Layers.Add(MoveTemp(Diamond));
			return Result;
		};
		static const FPaintedIcon Before = Make(false);
		static const FPaintedIcon After = Make(true);
		return bAfter ? After : Before;
	}
}

// ---------------------------------------------------------------------------
// The words
// ---------------------------------------------------------------------------

FString UHudFindBannerWidget::TitleWords(const FFindBannerShow& Find)
{
	if (Find.Merged > 1)
	{
		return FindRules::CollapsedTitle(Find.Merged).ToString().ToUpper();
	}
	return Find.Title.IsEmptyOrWhitespace() ? FString(TEXT("FOUND")) : Find.Title.ToString().ToUpper();
}

FString UHudFindBannerWidget::CountWords(const FFindBannerShow& Find)
{
	return Find.Count.ToString().ToUpper();
}

int32 UHudFindBannerWidget::TitleSizeFor(const FString& Title)
{
	// Room between the rules is about TitleRoom px; a letter with its spacing is about 0.84 of the size wide.
	return FMath::Clamp(FMath::FloorToInt(TitleRoom / (0.84f * static_cast<float>(FMath::Max(Title.Len(), 1)))), MinTitleSize, TitleSize);
}

FString UHudFindBannerWidget::XpWords(int32 Xp)
{
	return Xp > 0 ? FString::Printf(TEXT("+%s XP"), *FText::AsNumber(Xp).ToString()) : FString();
}

int32 UHudFindBannerWidget::XpAt(int32 Xp, float Time)
{
	if (Xp <= 0 || Time <= XpCountFrom)
	{
		return 0;
	}
	if (Time >= XpCountTo)
	{
		return Xp;
	}
	return FMath::Clamp(FMath::FloorToInt(static_cast<float>(Xp) * EaseOut(Step(XpCountFrom, XpCountTo, Time))), 0, Xp);
}

FFindBannerShow UHudFindBannerWidget::Merge(const TArray<FFindBannerShow>& Finds)
{
	FFindBannerShow Result;
	if (Finds.IsEmpty())
	{
		return Result;
	}
	// The latest find's count line (the tally as it stands), everyone's experience; the title says how many (TitleWords).
	Result = Finds.Last();
	Result.Merged = 0;
	Result.Xp = 0;
	for (const FFindBannerShow& Find : Finds)
	{
		Result.Merged += FMath::Max(Find.Merged, 1);
		Result.Xp += Find.Xp;
	}
	Result.Title = FText::GetEmpty();
	Result.Name = NSLOCTEXT("LooterHUD", "FindsMerged", "Off the path");
	return Result;
}

// ---------------------------------------------------------------------------
// Building it
// ---------------------------------------------------------------------------

TSharedRef<SWidget> UHudFindBannerWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildTree();
		PaintShow(/*bCovered*/ false);
	}
	return Super::RebuildWidget();
}

void UHudFindBannerWidget::BuildTree()
{
	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
	const FVector2D Centre(Width * 0.5f, Height * 0.5f);
	// Everything by its middle, on the box's centre line.
	auto Place = [Canvas](UWidget* Child, const FVector2D& Position)
	{
		UCanvasPanelSlot* ChildSlot = Canvas->AddChildToCanvas(Child);
		ChildSlot->SetAutoSize(true);
		ChildSlot->SetPosition(Position);
		ChildSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	};

	// Back to front: the warm glow, the tally, the title between its rules, the name, the count line and the experience.
	Place(MakeImage(WidgetTree, GlowBrush(FVector2D(GlowWidth, GlowHeight), Color::SoulGold().CopyWithNewOpacity(GlowOpacity))),
		Centre + FVector2D(0.f, 8.f));

	const FVector2D TallySize(TallyUnitsWide * TallyScale, TallyUnitsHigh * TallyScale);
	UImage* TallyImage = MakeImage(WidgetTree, PaintedIconBrush(TEXT("HudFindTally"), InkedTallyPicture(), TallyPixelsPerUnit, TallySize, Color::SoulGold()));
	TallyImage->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	Place(TallyImage, Centre + FVector2D(0.f, TallyRise));
	Tally = TallyImage;

	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	const FVector2D RuleSize(RuleLength + RuleDiamond * 2.f + 2.f, RuleArtHeight);
	Row->AddChildToHorizontalBox(MakeImage(WidgetTree, PaintedIconBrush(TEXT("HudFindRuleIn"), RulePicture(false), PixelsPerUnit, RuleSize)))
		->SetVerticalAlignment(VAlign_Center);
	TitleText = MakeFloatingText(WidgetTree, TitleSize, Color::SoulGold(), TitleSpacing, ETextJustify::Center);
	UHorizontalBoxSlot* WordsSlot = Row->AddChildToHorizontalBox(TitleText);
	WordsSlot->SetVerticalAlignment(VAlign_Center);
	WordsSlot->SetPadding(FMargin(RuleGap, 0.f));
	Row->AddChildToHorizontalBox(MakeImage(WidgetTree, PaintedIconBrush(TEXT("HudFindRuleOut"), RulePicture(true), PixelsPerUnit, RuleSize)))
		->SetVerticalAlignment(VAlign_Center);
	Place(Row, Centre + FVector2D(0.f, TitleDrop));

	NameText = MakeFloatingText(WidgetTree, NameSize, Color::SlateText(), NameSpacing, ETextJustify::Center);
	Place(NameText, Centre + FVector2D(0.f, NameDrop));

	UHorizontalBox* Counts = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	CountText = MakeFloatingText(WidgetTree, CountSize, Color::SaltLine(), CountSpacing, ETextJustify::Center);
	Counts->AddChildToHorizontalBox(CountText)->SetVerticalAlignment(VAlign_Center);
	XpText = MakeFloatingText(WidgetTree, CountSize, Color::SoulGold(), CountSpacing, ETextJustify::Center);
	UHorizontalBoxSlot* XpSlot = Counts->AddChildToHorizontalBox(XpText);
	XpSlot->SetVerticalAlignment(VAlign_Center);
	XpSlot->SetPadding(FMargin(CountGap, 0.f, 0.f, 0.f));
	Place(Counts, Centre + FVector2D(0.f, CountDrop));

	USizeBox* Root = MakeSized(WidgetTree, Canvas, Width, Height);
	// It pops from its middle.
	Root->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	WidgetTree->RootWidget = Root;
	Banner = Root;
}

void UHudFindBannerWidget::NativeConstruct()
{
	Super::NativeConstruct();
	// It never takes the mouse.
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

// ---------------------------------------------------------------------------
// What it announces, and taking turns with the other banners
// ---------------------------------------------------------------------------

void UHudFindBannerWidget::SetOtherBanners(UHudLevelUpBannerWidget* InLevelUpBanner, UHudMissionCompleteWidget* InMissionBanner,
	UHudGraveMarkedWidget* InRoostBanner)
{
	LevelUpBanner = InLevelUpBanner;
	MissionBanner = InMissionBanner;
	RoostBanner = InRoostBanner;
}

void UHudFindBannerWidget::Announce(const FFindBannerShow& Find)
{
	Queue.Add(Find);
	// A crowd of finds (an old save's, a console mark) is one banner, not a storm.
	if (Queue.Num() > MaxQueued)
	{
		const FFindBannerShow All = Merge(Queue);
		Queue.Reset();
		Queue.Add(All);
	}
}

bool UHudFindBannerWidget::IsBusy() const
{
	return Time >= 0.f || !Queue.IsEmpty();
}

bool UHudFindBannerWidget::IsOtherBannerShowing() const
{
	// The level-up banner collapses its root while it waits for a level-up, so a root on screen is a banner showing.
	const UHudLevelUpBannerWidget* LevelUp = LevelUpBanner.Get();
	const UWidget* Root = LevelUp ? LevelUp->GetRootWidget() : nullptr;
	if (Root && Root->GetVisibility() != ESlateVisibility::Collapsed)
	{
		return true;
	}
	const UHudMissionCompleteWidget* Mission = MissionBanner.Get();
	if (Mission && Mission->IsBusy())
	{
		return true;
	}
	// The roost banner on screen, not one only waiting (it waits for this banner too: two waiting would never start).
	const UHudGraveMarkedWidget* Roost = RoostBanner.Get();
	return Roost && Roost->IsShowing();
}

bool UHudFindBannerWidget::IsCovered() const
{
	const APlayerController* Player = GetOwningPlayer();
	const ALooterHUD* LooterHUD = Player ? Cast<ALooterHUD>(Player->GetHUD()) : nullptr;
	if (LooterHUD && LooterHUD->IsMenuOpen())
	{
		return true;
	}
	// A scene that holds the player puts the gameplay HUD away.
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

void UHudFindBannerWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	Advance(InDeltaTime);
}

void UHudFindBannerWidget::Advance(float DeltaSeconds)
{
	const bool bCovered = IsCovered();
	// The next one waits for the screen to be the player's and the other banners to be done.
	if (Time < 0.f && !Queue.IsEmpty() && !bCovered && !IsOtherBannerShowing())
	{
		Shown = Queue[0];
		Queue.RemoveAt(0);
		Time = 0.f;
		ShownXp = -1;
		ApplyWords();
	}
	// Time under a menu or a scene doesn't count, so the whole show is seen once the game is back.
	if (Time >= 0.f && !bCovered)
	{
		Time += DeltaSeconds;
		if (Time >= ShowSeconds)
		{
			Time = -1.f;
			// Its show is done: the subsystem pays what the find pays (so a level-up follows the find, never cuts it).
			UFindSubsystem::BannerDone(this, Shown.Merged);
		}
	}
	PaintShow(bCovered);
}

float UHudFindBannerWidget::OpacityAt(float Time)
{
	if (Time < 0.f || Time >= ShowSeconds)
	{
		return 0.f;
	}
	if (Time < InSeconds)
	{
		return EaseOut(Step(0.f, InSeconds, Time));
	}
	if (Time > OutSeconds)
	{
		return 1.f - EaseOut(Step(OutSeconds, ShowSeconds, Time));
	}
	return 1.f;
}

float UHudFindBannerWidget::ScaleAt(float Time)
{
	if (Time < InSeconds)
	{
		return FMath::Lerp(StartScale, Overshoot, EaseOut(Step(0.f, InSeconds, Time)));
	}
	if (Time < SettleSeconds)
	{
		return FMath::Lerp(Overshoot, 1.f, EaseOut(Step(InSeconds, SettleSeconds, Time)));
	}
	return 1.f;
}

void UHudFindBannerWidget::ApplyWords()
{
	if (TitleText)
	{
		const FString Words = TitleWords(Shown);
		// A long title (the completion's) is set smaller, so it stays between its rules.
		StyleFloatingText(TitleText, TitleSizeFor(Words), Color::SoulGold(), TitleSpacing, ETextJustify::Center);
		TitleText->SetText(FText::FromString(Words));
	}
	if (NameText)
	{
		NameText->SetText(Shown.Name.ToUpper());
	}
	if (CountText)
	{
		CountText->SetText(FText::FromString(CountWords(Shown)));
	}
	if (XpText)
	{
		// The experience starts at nothing and counts up (PaintShow).
		XpText->SetText(FText::GetEmpty());
	}
}

void UHudFindBannerWidget::PaintShow(bool bCovered)
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
	Banner->SetRenderScale(FVector2D(ScaleAt(Time)));
	Banner->SetRenderOpacity(OpacityAt(Time));
	if (Tally)
	{
		// The tally is stamped in: from big and clear, down onto the line.
		const float Stamp = EaseOut(Step(0.f, TallyStampSeconds, Time));
		Tally->SetRenderScale(FVector2D(FMath::Lerp(TallyStartScale, 1.f, Stamp)));
		Tally->SetRenderOpacity(Step(0.f, TallyStampSeconds * 0.5f, Time));
	}
	if (NameText)
	{
		NameText->SetRenderOpacity(EaseOut(Step(NameInFrom, NameInTo, Time)));
	}
	if (CountText)
	{
		CountText->SetRenderOpacity(EaseOut(Step(CountInFrom, CountInTo, Time)));
	}
	if (XpText)
	{
		const int32 Xp = XpAt(Shown.Xp, Time);
		if (Xp != ShownXp)
		{
			ShownXp = Xp;
			XpText->SetText(FText::FromString(Xp > 0 ? XpWords(Xp) : FString()));
		}
		XpText->SetRenderOpacity(EaseOut(Step(XpCountFrom - 0.1f, XpCountFrom + 0.1f, Time)));
	}
}

FString UHudFindBannerWidget::GetShownTitle() const
{
	return TitleText ? TitleText->GetText().ToString() : TitleWords(Shown);
}

FString UHudFindBannerWidget::GetShownName() const
{
	return NameText ? NameText->GetText().ToString() : Shown.Name.ToUpper().ToString();
}

FString UHudFindBannerWidget::GetShownCount() const
{
	return CountText ? CountText->GetText().ToString() : CountWords(Shown);
}

FString UHudFindBannerWidget::GetShownXp() const
{
	return XpText ? XpText->GetText().ToString() : XpWords(XpAt(Shown.Xp, Time));
}

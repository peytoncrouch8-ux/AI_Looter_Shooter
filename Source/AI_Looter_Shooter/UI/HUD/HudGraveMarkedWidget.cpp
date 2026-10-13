// UHudGraveMarkedWidget: the "ROOST FOUND" banner. What it shows and when is here; its pictures and widgets are built below.

#include "UI/HUD/HudGraveMarkedWidget.h"
#include "Scenes/SceneSubsystem.h"
#include "UI/HUD/HudLevelUpBannerWidget.h"
#include "UI/HUD/HudMissionCompleteWidget.h"
#include "UI/HUD/LooterHUD.h"
#include "UI/Style/LooterUIStyle.h"
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
	constexpr float OutSeconds = 1.95f;
	constexpr float StartScale = 0.86f;
	constexpr float Overshoot = 1.04f;
	/** The roost's name comes in a beat after the title. */
	constexpr float NameInFrom = 0.15f;
	constexpr float NameInTo = 0.45f;

	/** The roost's picture: a 44 x 52 view box drawn at 2 texels a unit, its middle this far over the box's. */
	constexpr float RoostWidth = 44.f;
	constexpr float RoostHeight = 52.f;
	constexpr float PixelsPerUnit = 2.f;
	constexpr float RoostDrop = -50.f;

	/** The beacon's glow behind the picture, in the roost's colour (the same light the world shows). */
	constexpr float RoostGlowWidth = 96.f;
	constexpr float RoostGlowHeight = 120.f;
	constexpr float RoostGlowOpacity = 0.42f;

	/** A warm glow behind the words, so they lift off a bright sky without a panel. */
	constexpr float GlowWidth = 460.f;
	constexpr float GlowHeight = 120.f;
	constexpr float GlowOpacity = 0.16f;

	/** "ROOST FOUND": orange, letter-spaced, its middle this far under the box's middle; the cyan rules either side. */
	constexpr int32 TitleSize = 32;
	constexpr int32 TitleSpacing = 220;
	constexpr float TitleDrop = 6.f;
	constexpr float RuleLength = 70.f;
	constexpr float RuleThickness = 2.f;
	constexpr float RuleDiamond = 3.f;
	constexpr float RuleArtHeight = 8.f;
	constexpr float RuleGap = 16.f;

	/** The roost's name under it. */
	constexpr int32 NameSize = 21;
	constexpr int32 NameSpacing = 120;
	constexpr float NameDrop = 48.f;

	float Step(float From, float To, float Time)
	{
		return FMath::Clamp((Time - From) / (To - From), 0.f, 1.f);
	}

	float EaseOut(float Alpha)
	{
		return FMath::InterpEaseOut(0.f, 1.f, Alpha, 2.f);
	}

	TArray<FVector2D> Circle(const FVector2D& Center, float Radius)
	{
		TArray<FVector2D> Points;
		constexpr int32 Segments = 24;
		for (int32 Index = 0; Index <= Segments; ++Index)
		{
			const float Angle = 2.f * PI * static_cast<float>(Index) / static_cast<float>(Segments);
			Points.Add(Center + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius);
		}
		return Points;
	}

	TArray<FVector2D> Closed(TArray<FVector2D> Points)
	{
		if (!Points.IsEmpty())
		{
			// A copy of the first point: adding a reference to the array's own element could read it after a reallocation.
			const FVector2D First = Points[0];
			Points.Add(First);
		}
		return Points;
	}

	FPaintLayer Fill(TArray<FVector2D> Shape, const FLinearColor& LayerColor, float Opacity = 1.f)
	{
		FPaintLayer Layer;
		Layer.Fills.Add(MoveTemp(Shape));
		Layer.Color = LayerColor;
		Layer.Opacity = Opacity;
		return Layer;
	}

	FPaintLayer Stroke(TArray<TArray<FVector2D>> Lines, float Width, const FLinearColor& LayerColor, float Opacity = 1.f)
	{
		FPaintLayer Layer;
		Layer.Strokes = MoveTemp(Lines);
		Layer.StrokeWidth = Width;
		Layer.Color = LayerColor;
		Layer.Opacity = Opacity;
		return Layer;
	}

	/** A shape in its colour with an ink edge, so it reads over anything (it has no panel behind it). */
	void AddInked(FPaintedIcon& Icon, const TArray<FVector2D>& Shape, const FLinearColor& Colour)
	{
		Icon.Layers.Add(Fill(Shape, Colour));
		Icon.Layers.Add(Stroke({ Closed(Shape) }, 2.4f, Color::Ink()));
	}

	/** An ellipse as a polygon. */
	TArray<FVector2D> Oval(const FVector2D& Center, float RadiusX, float RadiusY)
	{
		TArray<FVector2D> Points;
		constexpr int32 Segments = 20;
		for (int32 Index = 0; Index < Segments; ++Index)
		{
			const float Angle = 2.f * PI * static_cast<float>(Index) / static_cast<float>(Segments);
			Points.Add(Center + FVector2D(FMath::Cos(Angle) * RadiusX, FMath::Sin(Angle) * RadiusY));
		}
		return Points;
	}

	/** A part of Hob's crow: ink, with a faint metal rim so the black bird reads over a dark sky too. */
	void AddCrowPart(FPaintedIcon& Icon, const TArray<FVector2D>& Shape)
	{
		Icon.Layers.Add(Fill(Shape, Color::Ink()));
		Icon.Layers.Add(Stroke({ Closed(Shape) }, 1.f, Color::MetalLow(), 0.9f));
	}

	/**
	 * Hob's roost, painted: the weathered mast on its cairn with the yard and the old brass bell, the crow's-foot mark burnt
	 * glowing in its board (the map's roost icon), the beacon's flame on top with its column of light rising in Hob's Ember,
	 * and Hob's crow perched on the yard's end, its one eye catching the fire.
	 */
	const FPaintedIcon& RoostPicture()
	{
		static const FPaintedIcon Picture = []
		{
			FPaintedIcon Result;
			Result.ViewBox = FVector2D(RoostWidth, RoostHeight);
			const FLinearColor Light = Color::Roost();
			const FLinearColor HotLight = Color::RoostCore();
			// The column, fading out as it climbs.
			FPaintLayer Column = Fill({ FVector2D(20.4f, 2.f), FVector2D(23.6f, 2.f), FVector2D(25.f, 22.f), FVector2D(19.f, 22.f) },
				Light.CopyWithNewOpacity(0.f));
			Column.GradientTo = Light.CopyWithNewOpacity(0.9f);
			Column.GradientStart = FVector2D(22.f, 2.f);
			Column.GradientEnd = FVector2D(22.f, 22.f);
			Result.Layers.Add(MoveTemp(Column));
			// The post in its footing, the crossbar, the bell under the bar's far end and its clapper.
			AddInked(Result, { FVector2D(20.f, 25.f), FVector2D(24.f, 25.f), FVector2D(24.f, 49.f), FVector2D(20.f, 49.f) }, Color::MetalMid());
			AddInked(Result, { FVector2D(15.5f, 47.5f), FVector2D(28.5f, 47.5f), FVector2D(29.5f, 51.f), FVector2D(14.5f, 51.f) }, Color::MetalLow());
			AddInked(Result, { FVector2D(7.f, 31.f), FVector2D(37.f, 31.f), FVector2D(37.f, 34.f), FVector2D(7.f, 34.f) }, Color::MetalMid());
			AddInked(Result, { FVector2D(29.f, 34.f), FVector2D(33.f, 34.f), FVector2D(34.5f, 40.f), FVector2D(27.5f, 40.f) }, Color::Curse());
			Result.Layers.Add(Fill(Circle(FVector2D(31.f, 41.f), 0.9f), Color::Ink()));
			// The board on the mast with the crow's-foot burnt in it, still glowing: three toes up, the hind toe down.
			AddInked(Result, { FVector2D(18.5f, 37.5f), FVector2D(25.5f, 37.5f), FVector2D(25.5f, 46.5f), FVector2D(18.5f, 46.5f) }, Color::MetalLow());
			Result.Layers.Add(Stroke({ { FVector2D(22.f, 42.6f), FVector2D(22.f, 38.8f) }, { FVector2D(22.f, 42.6f), FVector2D(19.8f, 39.6f) },
				{ FVector2D(22.f, 42.6f), FVector2D(24.2f, 39.6f) }, { FVector2D(22.f, 42.6f), FVector2D(22.f, 45.4f) } }, 0.9f, HotLight));
			// The beacon's flame on the post's top, hot at its heart.
			Result.Layers.Add(Fill(Oval(FVector2D(22.f, 22.5f), 3.3f, 4.2f), Light));
			Result.Layers.Add(Fill(Oval(FVector2D(22.f, 23.2f), 1.5f, 2.1f), HotLight));
			// Hob's crow on the bar's near end, facing out: tail, body, head, beak, its feet on the bar.
			AddCrowPart(Result, { FVector2D(16.5f, 27.f), FVector2D(21.5f, 31.f), FVector2D(20.f, 32.4f), FVector2D(15.f, 29.5f) });
			AddCrowPart(Result, { FVector2D(7.5f, 25.f), FVector2D(10.f, 21.5f), FVector2D(14.5f, 20.5f), FVector2D(17.5f, 22.f),
				FVector2D(18.5f, 25.5f), FVector2D(16.f, 29.5f), FVector2D(11.f, 30.5f), FVector2D(8.5f, 29.f) });
			AddCrowPart(Result, Circle(FVector2D(8.2f, 20.2f), 2.9f));
			AddCrowPart(Result, { FVector2D(5.6f, 19.2f), FVector2D(2.6f, 20.6f), FVector2D(5.6f, 21.4f) });
			Result.Layers.Add(Stroke({ { FVector2D(11.f, 30.2f), FVector2D(11.f, 31.2f) }, { FVector2D(13.5f, 30.4f), FVector2D(13.5f, 31.2f) } },
				0.9f, Color::Ink()));
			// Its one eye, catching the roost's light.
			Result.Layers.Add(Fill(Circle(FVector2D(7.3f, 19.6f), 0.8f), HotLight));
			return Result;
		}();
		return Picture;
	}

	/**
	 * A cyan rule fading out away from the title, with a small orange diamond at its end by the title: bAfter is the one after
	 * it (its diamond on the left, fading toward the right).
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
			FPaintLayer Line = Fill({ FVector2D(LineFrom, MidY - RuleThickness * 0.5f), FVector2D(LineTo, MidY - RuleThickness * 0.5f),
				FVector2D(LineTo, MidY + RuleThickness * 0.5f), FVector2D(LineFrom, MidY + RuleThickness * 0.5f) },
				Color::Hairline().CopyWithNewOpacity(bRight ? 1.f : 0.f));
			Line.GradientTo = Color::Hairline().CopyWithNewOpacity(bRight ? 0.f : 1.f);
			Line.GradientStart = FVector2D(LineFrom, MidY);
			Line.GradientEnd = FVector2D(LineTo, MidY);
			Result.Layers.Add(MoveTemp(Line));
			const float DiamondX = bRight ? RuleDiamond : Total - RuleDiamond;
			Result.Layers.Add(Fill({ FVector2D(DiamondX, MidY - RuleDiamond), FVector2D(DiamondX + RuleDiamond, MidY),
				FVector2D(DiamondX, MidY + RuleDiamond), FVector2D(DiamondX - RuleDiamond, MidY) }, Color::Accent()));
			return Result;
		};
		static const FPaintedIcon Before = Make(false);
		static const FPaintedIcon After = Make(true);
		return bAfter ? After : Before;
	}
}

// ---------------------------------------------------------------------------
// Building it
// ---------------------------------------------------------------------------

TSharedRef<SWidget> UHudGraveMarkedWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildTree();
		PaintShow(/*bCovered*/ false);
	}
	return Super::RebuildWidget();
}

void UHudGraveMarkedWidget::BuildTree()
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

	// Back to front: the glows (warm behind the words, the roost's own light behind the roost), the roost, the title between
	// its rules, the name.
	Place(MakeImage(WidgetTree, GlowBrush(FVector2D(GlowWidth, GlowHeight), Color::Accent().CopyWithNewOpacity(GlowOpacity))),
		Centre + FVector2D(0.f, TitleDrop + 8.f));
	Place(MakeImage(WidgetTree, GlowBrush(FVector2D(RoostGlowWidth, RoostGlowHeight), Color::Roost().CopyWithNewOpacity(RoostGlowOpacity))),
		Centre + FVector2D(0.f, RoostDrop - 6.f));
	Place(MakeImage(WidgetTree, PaintedIconBrush(TEXT("HudRoostFound"), RoostPicture(), PixelsPerUnit, FVector2D(RoostWidth, RoostHeight))),
		Centre + FVector2D(0.f, RoostDrop));

	UHorizontalBox* TitleRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	const FVector2D RuleSize(RuleLength + RuleDiamond * 2.f + 2.f, RuleArtHeight);
	TitleRow->AddChildToHorizontalBox(MakeImage(WidgetTree, PaintedIconBrush(TEXT("HudGraveRuleIn"), RulePicture(false), PixelsPerUnit, RuleSize)))
		->SetVerticalAlignment(VAlign_Center);
	UTextBlock* Words = MakeFloatingText(WidgetTree, TitleSize, Color::Accent(), TitleSpacing, ETextJustify::Center);
	Words->SetText(NSLOCTEXT("LooterHUD", "RoostFound", "ROOST FOUND"));
	UHorizontalBoxSlot* WordsSlot = TitleRow->AddChildToHorizontalBox(Words);
	WordsSlot->SetVerticalAlignment(VAlign_Center);
	WordsSlot->SetPadding(FMargin(RuleGap, 0.f));
	TitleRow->AddChildToHorizontalBox(MakeImage(WidgetTree, PaintedIconBrush(TEXT("HudGraveRuleOut"), RulePicture(true), PixelsPerUnit, RuleSize)))
		->SetVerticalAlignment(VAlign_Center);
	Place(TitleRow, Centre + FVector2D(0.f, TitleDrop));
	Title = TitleRow;

	NameText = MakeFloatingText(WidgetTree, NameSize, FLinearColor::White, NameSpacing, ETextJustify::Center);
	Place(NameText, Centre + FVector2D(0.f, NameDrop));

	USizeBox* Root = MakeSized(WidgetTree, Canvas, Width, Height);
	// It pops from its middle.
	Root->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	WidgetTree->RootWidget = Root;
	Banner = Root;
}

void UHudGraveMarkedWidget::NativeConstruct()
{
	Super::NativeConstruct();
	// It never takes the mouse.
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

// ---------------------------------------------------------------------------
// What it announces, and taking turns with the other banners
// ---------------------------------------------------------------------------

void UHudGraveMarkedWidget::SetOtherBanners(UHudLevelUpBannerWidget* InLevelUpBanner, UHudMissionCompleteWidget* InMissionBanner)
{
	LevelUpBanner = InLevelUpBanner;
	MissionBanner = InMissionBanner;
}

void UHudGraveMarkedWidget::Announce(const FText& GraveName)
{
	Queue.Add(GraveName);
}

bool UHudGraveMarkedWidget::IsBusy() const
{
	return Time >= 0.f || !Queue.IsEmpty();
}

bool UHudGraveMarkedWidget::IsOtherBannerShowing() const
{
	// The level-up banner collapses its root while it waits for a level-up, so a root on screen is a banner showing.
	const UHudLevelUpBannerWidget* LevelUp = LevelUpBanner.Get();
	const UWidget* Root = LevelUp ? LevelUp->GetRootWidget() : nullptr;
	if (Root && Root->GetVisibility() != ESlateVisibility::Collapsed)
	{
		return true;
	}
	const UHudMissionCompleteWidget* Mission = MissionBanner.Get();
	return Mission && Mission->IsBusy();
}

bool UHudGraveMarkedWidget::IsCovered() const
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

void UHudGraveMarkedWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	Advance(InDeltaTime);
}

void UHudGraveMarkedWidget::Advance(float DeltaSeconds)
{
	const bool bCovered = IsCovered();
	// The next one waits for the screen to be the player's and the other banners to be done.
	if (Time < 0.f && !Queue.IsEmpty() && !bCovered && !IsOtherBannerShowing())
	{
		Shown = Queue[0];
		Queue.RemoveAt(0);
		Time = 0.f;
		if (NameText)
		{
			NameText->SetText(Shown.ToUpper());
		}
	}
	// Time under a menu or a scene doesn't count, so the whole show is seen once the game is back.
	if (Time >= 0.f && !bCovered)
	{
		Time += DeltaSeconds;
		if (Time >= ShowSeconds)
		{
			Time = -1.f;
		}
	}
	PaintShow(bCovered);
}

float UHudGraveMarkedWidget::OpacityAt(float Time)
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

float UHudGraveMarkedWidget::ScaleAt(float Time)
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

void UHudGraveMarkedWidget::PaintShow(bool bCovered)
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
	if (NameText)
	{
		NameText->SetRenderOpacity(EaseOut(Step(NameInFrom, NameInTo, Time)));
	}
}

FString UHudGraveMarkedWidget::GetShownName() const
{
	return NameText ? NameText->GetText().ToString() : Shown.ToString().ToUpper();
}

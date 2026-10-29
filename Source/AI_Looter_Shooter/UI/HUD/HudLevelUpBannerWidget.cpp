#include "UI/HUD/HudLevelUpBannerWidget.h"
#include "Audio/LooterSound.h"
#include "Progression/LevelRules.h"
#include "Progression/LevelUpBurstSubsystem.h"
#include "Progression/PlayerProgressionSubsystem.h"
#include "UI/Style/LooterUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/SlateRenderer.h"

using namespace LooterUI;

// Concept 5 "Signal Glass" (ui3/make_b.js, c5's level-up). The mockup is drawn at 1:1 on a 1920 sheet; mockup CSS px are
// Slate font sizes as they are (in game text comes out a third bigger), so the gaps round the type are the mockup's
// grown by about a third too, to keep its proportions round the bigger letters.

namespace
{
	/** Every piece by its offset from the box's centre (the point the HUD places, 260 px down). The title stays under the
	 *  boss bar's cluster (126 px down); the number sits under it, the reward under that. */
	constexpr float TitleDown = -100.f;
	constexpr float BarDown = -64.f;
	constexpr float NumberDown = 56.f;
	/** The hairlines run a touch under the number's middle, as in the mockup (192 against the digit's 188). */
	constexpr float LinesDown = NumberDown + 5.f;
	constexpr float RewardDown = 196.f;

	/** "LEVEL UP": 30 px, letter-spaced 16 px (0.53 em, in thousandths of an em), in text white with the floating outline. */
	constexpr int32 TitleSize = 30;
	constexpr int32 TitleSpacing = 533;
	/** The level: one big bold number. Its outline is set by hand: the floating text's size/14 would swell it. */
	constexpr int32 NumberSize = 160;
	constexpr int32 NumberOutline = 4;
	/** The reward line: 24 px, letter-spaced 0.3 em, a 2 px outline like the mockup's ring. */
	constexpr int32 RewardSize = 24;
	constexpr int32 RewardSpacing = 300;
	constexpr int32 RewardOutline = 2;

	/** The salt hairline (one picture serves both sides: a tick at each end) and its gap past the arrowhead. */
	constexpr float LineLength = 196.f;
	constexpr float LineMargin = 2.f;
	constexpr float LineTick = 7.f;
	constexpr float LineArtHeight = 18.f;
	/** The gold arrowheads beside the number, pointing out along the lines (c5), and the gap from the number's box. */
	constexpr float ArrowWidth = 13.f;
	constexpr float ArrowHalfHeight = 7.5f;
	constexpr float ArrowArtWidth = 18.f;
	constexpr float ArrowArtHeight = 20.f;
	constexpr float NumberGap = 30.f;
	constexpr float ArrowToLine = 4.f;
	/** The gold bar under the title (the mockup's 80x4, grown with the type). */
	constexpr float BarWidth = 104.f;
	constexpr float BarHeight = 5.f;
	constexpr float BarEdge = 2.f;
	constexpr float PixelsPerUnit = 2.f;
	/** The soft dark shade behind it all (the mockup's: half black in the middle) and the gold flash behind the number. */
	const FVector2D ShadeSize(880.f, 460.f);
	constexpr float ShadeOpacity = 0.42f;
	const FVector2D FlashSize(440.f, 360.f);
	constexpr float FlashOpacity = 0.6f;

	/** The show, in seconds. The UI.LevelUp cue rises for 0.42 s into its struck chord, so the count lands on the chord. */
	constexpr float FadeInEnd = 0.15f;
	constexpr float WipeStart = 0.02f;
	constexpr float WipeEnd = 0.38f;
	constexpr float ArrowStart = 0.06f;
	constexpr float ArrowEnd = 0.22f;
	constexpr float CountAt = 0.42f;
	constexpr float PopSeconds = 0.26f;
	constexpr float PopScale = 1.18f;
	constexpr float FlashUp = 0.08f;
	constexpr float FlashDown = 0.55f;
	constexpr float BarStart = 0.62f;
	constexpr float BarEnd = 0.86f;
	constexpr float RewardStart = 0.7f;
	constexpr float RewardEnd = 0.92f;
	/** Holds about 2 s after the reward lands, then fades. */
	constexpr float OutStart = 2.9f;
	constexpr float ShowSeconds = 3.3f;
	/** Words rise this far as they fade in (the concept's "fade in 8 px up"). */
	constexpr float WordRise = 8.f;
	/** The old level stands dimmer until the count. */
	constexpr float OldNumberOpacity = 0.7f;

	FSlateFontInfo OutlinedFont(int32 Size, int32 Spacing, int32 Outline)
	{
		FSlateFontInfo Result = FloatingFont(Size, Spacing);
		Result.OutlineSettings.OutlineSize = Outline;
		return Result;
	}

	FPaintLayer Strokes(TArray<TArray<FVector2D>> Lines, float Width, const FLinearColor& Color, float Opacity)
	{
		FPaintLayer Layer;
		Layer.Strokes = MoveTemp(Lines);
		Layer.StrokeWidth = Width;
		Layer.Color = Color;
		Layer.Opacity = Opacity;
		return Layer;
	}

	/** The hairline with a tick at each end, a salt line on a dark edge like every kit line (it floats over the world). */
	const FPaintedIcon& LinePicture()
	{
		static const FPaintedIcon Picture = []
		{
			FPaintedIcon Result;
			Result.ViewBox = FVector2D(LineLength + LineMargin * 2.f, LineArtHeight);
			const float Mid = LineArtHeight * 0.5f;
			const float Left = LineMargin;
			const float Right = LineMargin + LineLength;
			const TArray<TArray<FVector2D>> Lines = {
				{ FVector2D(Left, Mid), FVector2D(Right, Mid) },
				{ FVector2D(Left, Mid - LineTick), FVector2D(Left, Mid + LineTick) },
				{ FVector2D(Right, Mid - LineTick), FVector2D(Right, Mid + LineTick) } };
			Result.Layers.Add(Strokes(Lines, 3.6f, Color::Ink(), 0.55f));
			Result.Layers.Add(Strokes(Lines, 1.4f, Color::SaltLine(), 0.9f));
			return Result;
		}();
		return Picture;
	}

	/** A small gold arrowhead on a dark edge; bRight points right (the one right of the number). */
	const FPaintedIcon& ArrowPicture(bool bRight)
	{
		auto Make = [](bool bPointRight)
		{
			FPaintedIcon Result;
			Result.ViewBox = FVector2D(ArrowArtWidth, ArrowArtHeight);
			const float Mid = ArrowArtHeight * 0.5f;
			const float Tip = bPointRight ? ArrowArtWidth - 2.5f : 2.5f;
			const float Base = bPointRight ? Tip - ArrowWidth : Tip + ArrowWidth;
			const TArray<FVector2D> Shape = { FVector2D(Tip, Mid), FVector2D(Base, Mid - ArrowHalfHeight), FVector2D(Base, Mid + ArrowHalfHeight) };
			TArray<FVector2D> Edge = Shape;
			Edge.Add(Shape[0]);
			Result.Layers.Add(Strokes({ Edge }, 3.f, Color::Ink(), 0.6f));
			FPaintLayer Fill;
			Fill.Fills.Add(Shape);
			Fill.Color = Color::SoulGold();
			Result.Layers.Add(MoveTemp(Fill));
			return Result;
		};
		static const FPaintedIcon Left = Make(false);
		static const FPaintedIcon Right = Make(true);
		return bRight ? Right : Left;
	}

	/** The gold bar under the title, on a thin dark edge. */
	const FPaintedIcon& BarPicture()
	{
		static const FPaintedIcon Picture = []
		{
			FPaintedIcon Result;
			const FVector2D Size(BarWidth + BarEdge * 2.f, BarHeight + BarEdge * 2.f);
			Result.ViewBox = Size;
			FPaintLayer Edge;
			Edge.Fills.Add({ FVector2D(0.f, 0.f), FVector2D(Size.X, 0.f), FVector2D(Size.X, Size.Y), FVector2D(0.f, Size.Y) });
			Edge.Color = Color::Ink();
			Edge.Opacity = 0.55f;
			Result.Layers.Add(MoveTemp(Edge));
			FPaintLayer Gold;
			Gold.Fills.Add({ FVector2D(BarEdge, BarEdge), FVector2D(Size.X - BarEdge, BarEdge), FVector2D(Size.X - BarEdge, Size.Y - BarEdge),
				FVector2D(BarEdge, Size.Y - BarEdge) });
			Gold.Color = Color::SoulGold();
			Result.Layers.Add(MoveTemp(Gold));
			return Result;
		}();
		return Picture;
	}

	/** How wide a number draws in the level's font (a guess of 0.6 em a digit where Slate can't measure, as in a test). */
	float NumberWidth(const FString& Number, const FSlateFontInfo& NumberFont)
	{
		if (FSlateApplication::IsInitialized() && FSlateApplication::Get().GetRenderer())
		{
			return static_cast<float>(FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(Number, NumberFont).X);
		}
		return Number.Len() * NumberSize * (96.f / 72.f) * 0.6f;
	}

	void MoveTo(UWidget* Widget, const FVector2D& Position)
	{
		if (UCanvasPanelSlot* CanvasSlot = Widget ? Cast<UCanvasPanelSlot>(Widget->Slot) : nullptr)
		{
			CanvasSlot->SetPosition(Position);
		}
	}
}

TSharedRef<SWidget> UHudLevelUpBannerWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
		const FVector2D Centre(Width * 0.5f, Height * 0.5f);
		// Each piece placed by the point Alignment names (the middle unless said), its own size.
		auto Place = [Canvas](UWidget* Child, const FVector2D& Position, const FVector2D& Alignment = FVector2D(0.5f, 0.5f))
		{
			UCanvasPanelSlot* ChildSlot = Canvas->AddChildToCanvas(Child);
			ChildSlot->SetAutoSize(true);
			ChildSlot->SetPosition(Position);
			ChildSlot->SetAlignment(Alignment);
			return Child;
		};
		auto Picture = [this](const TCHAR* Name, const FPaintedIcon& Icon)
		{
			return MakeImage(WidgetTree, PaintedIconBrush(Name, Icon, PixelsPerUnit, Icon.ViewBox));
		};

		// The dark shade keeps the white type readable over bright sky; it is the popup's only background.
		UImage* Shade = MakeImage(WidgetTree, GlowBrush(ShadeSize, FLinearColor::Black.CopyWithNewOpacity(ShadeOpacity)));
		Place(Shade, Centre + FVector2D(0.f, NumberDown * 0.5f));
		MarkBackground(Shade);
		Flash = Place(MakeImage(WidgetTree, GlowBrush(FlashSize, Color::SoulGold().CopyWithNewOpacity(FlashOpacity))),
			Centre + FVector2D(0.f, NumberDown));

		// The hairlines grow out from their inner ends (by the number); ApplyTexts sets how far out those are.
		LeftLine = Place(Picture(TEXT("HudLevelUpLine"), LinePicture()), Centre + FVector2D(0.f, LinesDown), FVector2D(1.f, 0.5f));
		LeftLine->SetRenderTransformPivot(FVector2D(1.f, 0.5f));
		RightLine = Place(Picture(TEXT("HudLevelUpLine"), LinePicture()), Centre + FVector2D(0.f, LinesDown), FVector2D(0.f, 0.5f));
		RightLine->SetRenderTransformPivot(FVector2D(0.f, 0.5f));
		LeftArrow = Place(Picture(TEXT("HudLevelUpArrowLeft"), ArrowPicture(false)), Centre + FVector2D(0.f, LinesDown), FVector2D(1.f, 0.5f));
		RightArrow = Place(Picture(TEXT("HudLevelUpArrowRight"), ArrowPicture(true)), Centre + FVector2D(0.f, LinesDown), FVector2D(0.f, 0.5f));

		LevelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		LevelText->SetFont(OutlinedFont(NumberSize, 0, NumberOutline));
		LevelText->SetJustification(ETextJustify::Center);
		// Chakra Petch's digits sit in the middle of their line, so centring the line centres the digits.
		Place(LevelText, Centre + FVector2D(0.f, NumberDown));

		UTextBlock* Title = MakeFloatingText(WidgetTree, TitleSize, Color::SlateText(), TitleSpacing, ETextJustify::Center);
		Title->SetText(FText::FromString(TEXT("LEVEL UP")));
		TitleText = Place(Title, Centre + FVector2D(0.f, TitleDown));
		// The bar draws out from its middle, last of all.
		GoldBar = Place(Picture(TEXT("HudLevelUpBar"), BarPicture()), Centre + FVector2D(0.f, BarDown));

		RewardText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		RewardText->SetFont(OutlinedFont(RewardSize, RewardSpacing, RewardOutline));
		RewardText->SetColorAndOpacity(FSlateColor(Color::SlateText()));
		RewardText->SetJustification(ETextJustify::Center);
		Place(RewardText, Centre + FVector2D(0.f, RewardDown));

		USizeBox* Root = MakeSized(WidgetTree, Canvas, Width, Height);
		WidgetTree->RootWidget = Root;
		Banner = Root;
		ShownNumber = INDEX_NONE;
		ApplyTexts();
		PaintShow();
	}
	return Super::RebuildWidget();
}

void UHudLevelUpBannerWidget::Show(int32 NewLevel)
{
	Level = NewLevel;
	FromLevel = FMath::Max(1, NewLevel - 1);
	Time = 0.f;
	ShownNumber = INDEX_NONE;
	ApplyTexts();
	PaintShow();
	// With the show, which waits for the experience bar to get there: the cue rises for 0.42 s into its chord, which
	// lands on the count (CountAt).
	LooterSound::Play2D(this, LooterSoundCue::LevelUp);
	// And in the world with them: a ring of light bursting out along the ground round the player, embers rising.
	if (ULevelUpBurstSubsystem* Burst = ULevelUpBurstSubsystem::Get(this))
	{
		Burst->Play(GetOwningPlayerPawn(), NewLevel);
	}
}

void UHudLevelUpBannerWidget::ApplyTexts()
{
	if (!LevelText || !RewardText)
	{
		return;
	}
	// Each level adds this share of the level 1 health (the progression settings' HealthPerLevel), the first reward.
	const float Share = UPlayerProgressionSubsystem::GetLevelRules().HealthPerLevel;
	if (Share > 0.f)
	{
		FNumberFormattingOptions Percent;
		Percent.MinimumFractionalDigits = 0;
		Percent.MaximumFractionalDigits = 1;
		RewardText->SetText(FText::FromString(FString::Printf(TEXT("MAX HEALTH +%s"), *FText::AsPercent(Share, &Percent).ToString())));
		RewardText->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	else
	{
		RewardText->SetVisibility(ESlateVisibility::Collapsed);
	}

	// The arrowheads and hairlines stand off the widest number the count shows, so nothing moves as 9 turns to 10.
	const FSlateFontInfo NumberFont = LevelText->GetFont();
	const float Half = 0.5f * FMath::Max(NumberWidth(FText::AsNumber(FromLevel).ToString(), NumberFont),
		NumberWidth(FText::AsNumber(Level).ToString(), NumberFont));
	const FVector2D Centre(Width * 0.5f, Height * 0.5f + LinesDown);
	const float ArrowIn = Half + NumberGap;
	// The arrowheads' art has a 2.5 px margin at the tip and the base ends ArrowWidth in from it.
	const float LineIn = ArrowIn + ArrowWidth + ArrowToLine;
	MoveTo(LeftArrow, Centre - FVector2D(ArrowIn - (ArrowArtWidth - ArrowWidth - 2.5f), 0.f));
	MoveTo(RightArrow, Centre + FVector2D(ArrowIn - (ArrowArtWidth - ArrowWidth - 2.5f), 0.f));
	MoveTo(LeftLine, Centre - FVector2D(LineIn - LineMargin, 0.f));
	MoveTo(RightLine, Centre + FVector2D(LineIn - LineMargin, 0.f));
}

void UHudLevelUpBannerWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (Time < 0.f)
	{
		return;
	}
	Time += InDeltaTime;
	if (Time >= ShowSeconds)
	{
		Time = -1.f;
	}
	PaintShow();
}

void UHudLevelUpBannerWidget::PaintShow()
{
	if (!Banner)
	{
		return;
	}
	const ESlateVisibility Wanted = Time < 0.f ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible;
	if (Banner->GetVisibility() != Wanted)
	{
		Banner->SetVisibility(Wanted);
	}
	if (Time < 0.f)
	{
		return;
	}
	// Each step eases out (cubic), as the concept's motion does; the fade out eases both ways.
	const float Now = Time;
	auto Span = [Now](float From, float To) { return FMath::Clamp((Now - From) / (To - From), 0.f, 1.f); };
	auto EaseOut = [](float Alpha) { return FMath::InterpEaseOut(0.f, 1.f, Alpha, 3.f); };
	// A render scale of exactly zero can't be inverted; the piece is see-through until it starts anyway.
	auto Grow = [](UWidget* Widget, float Alpha)
	{
		Widget->SetRenderScale(FVector2D(FMath::Max(Alpha, 0.001f), 1.f));
		Widget->SetRenderOpacity(Alpha > 0.f ? 1.f : 0.f);
	};
	auto Rise = [](UWidget* Widget, float Alpha)
	{
		Widget->SetRenderOpacity(Alpha);
		Widget->SetRenderTranslation(FVector2D(0.f, WordRise * (1.f - Alpha)));
	};

	const float Out = 1.f - FMath::SmoothStep(0.f, 1.f, Span(OutStart, ShowSeconds));
	Banner->SetRenderOpacity(EaseOut(Span(0.f, FadeInEnd)) * Out);

	Rise(TitleText, EaseOut(Span(0.f, FadeInEnd + 0.07f)));
	const float Wipe = EaseOut(Span(WipeStart, WipeEnd));
	Grow(LeftLine, Wipe);
	Grow(RightLine, Wipe);
	const float Arrows = EaseOut(Span(ArrowStart, ArrowEnd));
	LeftArrow->SetRenderOpacity(Arrows);
	RightArrow->SetRenderOpacity(Arrows);

	// The count: the old level stands dim, then the new one pops in on the cue's chord with one soft gold flash.
	const bool bCounted = Now >= CountAt;
	const int32 Number = bCounted ? Level : FromLevel;
	if (Number != ShownNumber)
	{
		ShownNumber = Number;
		LevelText->SetText(FText::AsNumber(Number));
		LevelText->SetColorAndOpacity(FSlateColor(bCounted ? Color::SlateText() : Color::SlateLabel()));
	}
	LevelText->SetRenderOpacity(bCounted ? 1.f : OldNumberOpacity);
	LevelText->SetRenderScale(FVector2D(bCounted ? FMath::Lerp(PopScale, 1.f, EaseOut(Span(CountAt, CountAt + PopSeconds))) : 1.f));
	const float FlashIn = Span(CountAt, CountAt + FlashUp);
	const float FlashOut = FMath::SmoothStep(0.f, 1.f, Span(CountAt + FlashUp, CountAt + FlashUp + FlashDown));
	Flash->SetRenderOpacity(FlashIn * (1.f - FlashOut));

	Grow(GoldBar, EaseOut(Span(BarStart, BarEnd)));
	Rise(RewardText, EaseOut(Span(RewardStart, RewardEnd)));
}

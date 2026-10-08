#include "UI/HUD/HudLevelUpBannerWidget.h"
#include "Audio/LooterSound.h"
#include "Progression/LevelRules.h"
#include "Progression/PlayerProgressionSubsystem.h"
#include "UI/Style/LooterUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"

using namespace LooterUI;

namespace
{
	/** The gem's picture: 180 px round the gem's centre, which is the box's centre. */
	constexpr float BannerArt = 180.f;
	constexpr float BannerPixelsPerUnit = 2.f;
	constexpr float BannerGemRadius = 46.f;
	constexpr float BannerRingRadius = 58.f;
	/** The soft cyan glow behind it all. */
	constexpr float BannerGlowSize = 160.f;
	constexpr float BannerGlowOpacity = 0.55f;
	/** The level in ink on the gem, no outline: it sits on the gem, not over the world. */
	constexpr int32 BannerLevelSize = 38;

	/** "LEVEL UP": orange 46 px, letter-spaced 12 px (in thousandths of an em), its middle this far under the gem's centre. */
	constexpr int32 BannerTitleSize = 46;
	constexpr int32 BannerTitleSpacing = 261;
	constexpr float BannerTitleDrop = 114.f;
	/** The cyan rules either side of the title, fading out away from it, and their gap to it. */
	constexpr float BannerRuleLength = 90.f;
	constexpr float BannerRuleThickness = 2.f;
	constexpr float BannerRuleGap = 22.f;
	/** The reward line under it. */
	constexpr int32 BannerRewardSize = 17;
	constexpr int32 BannerRewardSpacing = 235;
	constexpr float BannerRewardDrop = 158.f;

	/** The show: in by 9% of it (overshooting), settled by 14%, held until 80%, then out. */
	constexpr float BannerSeconds = 2.8f;
	constexpr float BannerInShare = 0.09f;
	constexpr float BannerSettleShare = 0.14f;
	constexpr float BannerOutShare = 0.8f;
	constexpr float BannerStartScale = 0.7f;
	constexpr float BannerOvershoot = 1.04f;

	FVector2D BannerAt(float X, float Y)
	{
		return FVector2D(BannerArt * 0.5f + X, BannerArt * 0.5f + Y);
	}

	TArray<FVector2D> BannerDiamond(float Radius)
	{
		return { BannerAt(0.f, -Radius), BannerAt(Radius, 0.f), BannerAt(0.f, Radius), BannerAt(-Radius, 0.f) };
	}

	FPaintLayer BannerLines(TArray<TArray<FVector2D>> Lines, float Width, const FLinearColor& Color, float Opacity = 1.f)
	{
		FPaintLayer Layer;
		Layer.Strokes = MoveTemp(Lines);
		Layer.StrokeWidth = Width;
		Layer.Color = Color;
		Layer.Opacity = Opacity;
		return Layer;
	}

	/** Eight orange rays, a thin cyan ring, and the gem: two flat cyan halves in a heavy ink edge, lit along its top. */
	const FPaintedIcon& BannerGemPicture()
	{
		static const FPaintedIcon Picture = []
		{
			FPaintedIcon Result;
			Result.ViewBox = FVector2D(BannerArt);
			TArray<TArray<FVector2D>> Rays;
			for (const FVector2D& Way : { FVector2D(0.f, -1.f), FVector2D(1.f, 0.f), FVector2D(0.f, 1.f), FVector2D(-1.f, 0.f) })
			{
				Rays.Add({ BannerAt(Way.X * 64.f, Way.Y * 64.f), BannerAt(Way.X * 86.f, Way.Y * 86.f) });
			}
			for (const FVector2D& Way : { FVector2D(1.f, -1.f), FVector2D(1.f, 1.f), FVector2D(-1.f, 1.f), FVector2D(-1.f, -1.f) })
			{
				Rays.Add({ BannerAt(Way.X * 42.f, Way.Y * 42.f), BannerAt(Way.X * 54.f, Way.Y * 54.f) });
			}
			Result.Layers.Add(BannerLines(MoveTemp(Rays), 2.f, Color::Accent(), 0.75f));

			TArray<FVector2D> Ring = BannerDiamond(BannerRingRadius);
			Ring.Add(BannerAt(0.f, -BannerRingRadius));
			Result.Layers.Add(BannerLines({ Ring }, 1.6f, Color::CyanText(), 0.8f));

			FPaintLayer Dark;
			Dark.Fills.Add(BannerDiamond(BannerGemRadius));
			Dark.Color = Color::GemDark();
			Result.Layers.Add(MoveTemp(Dark));
			FPaintLayer Light;
			Light.Fills.Add({ BannerAt(-BannerGemRadius, 0.f), BannerAt(0.f, -BannerGemRadius), BannerAt(BannerGemRadius, 0.f) });
			Light.Color = Color::GemLight();
			Result.Layers.Add(MoveTemp(Light));
			TArray<FVector2D> Edge = BannerDiamond(BannerGemRadius);
			Edge.Add(BannerAt(0.f, -BannerGemRadius));
			Result.Layers.Add(BannerLines({ Edge }, 4.f, Color::Ink()));
			Result.Layers.Add(BannerLines({ { BannerAt(-31.f, 0.f), BannerAt(0.f, -31.f), BannerAt(31.f, 0.f) } }, 2.f, FLinearColor::White, 0.8f));
			return Result;
		}();
		return Picture;
	}

	/** A cyan rule fading out away from the title: bToRight fades toward the right (the rule after it). */
	const FPaintedIcon& BannerRulePicture(bool bToRight)
	{
		auto Make = [](bool bFadeRight)
		{
			FPaintedIcon Result;
			Result.ViewBox = FVector2D(BannerRuleLength, BannerRuleThickness);
			FPaintLayer Rule;
			Rule.Fills.Add({ FVector2D(0.f, 0.f), FVector2D(BannerRuleLength, 0.f), FVector2D(BannerRuleLength, BannerRuleThickness),
				FVector2D(0.f, BannerRuleThickness) });
			Rule.Color = Color::Hairline().CopyWithNewOpacity(bFadeRight ? 1.f : 0.f);
			Rule.GradientTo = Color::Hairline().CopyWithNewOpacity(bFadeRight ? 0.f : 1.f);
			Rule.GradientStart = FVector2D(0.f, 0.f);
			Rule.GradientEnd = FVector2D(BannerRuleLength, 0.f);
			Result.Layers.Add(MoveTemp(Rule));
			return Result;
		};
		static const FPaintedIcon Left = Make(false);
		static const FPaintedIcon Right = Make(true);
		return bToRight ? Right : Left;
	}
}

TSharedRef<SWidget> UHudLevelUpBannerWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
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

		Place(MakeImage(WidgetTree, GlowBrush(FVector2D(BannerGlowSize), Color::CyanText().CopyWithNewOpacity(BannerGlowOpacity))), Centre);
		Place(MakeImage(WidgetTree, PaintedIconBrush(TEXT("HudBannerGem"), BannerGemPicture(), BannerPixelsPerUnit, FVector2D(BannerArt))),
			Centre);
		LevelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		LevelText->SetFont(Font(BannerLevelSize));
		LevelText->SetColorAndOpacity(FSlateColor(Color::Ink()));
		LevelText->SetJustification(ETextJustify::Center);
		// Chakra Petch's digits sit in the middle of their line, so centring the line centres them in the gem.
		Place(LevelText, Centre);

		UHorizontalBox* Title = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		const FVector2D RuleSize(BannerRuleLength, BannerRuleThickness);
		Title->AddChildToHorizontalBox(MakeImage(WidgetTree, PaintedIconBrush(TEXT("HudBannerRuleIn"), BannerRulePicture(false),
			BannerPixelsPerUnit, RuleSize)))->SetVerticalAlignment(VAlign_Center);
		UTextBlock* Words = MakeFloatingText(WidgetTree, BannerTitleSize, Color::Accent(), BannerTitleSpacing, ETextJustify::Center);
		Words->SetText(FText::FromString(TEXT("LEVEL UP")));
		UHorizontalBoxSlot* WordsSlot = Title->AddChildToHorizontalBox(Words);
		WordsSlot->SetVerticalAlignment(VAlign_Center);
		WordsSlot->SetPadding(FMargin(BannerRuleGap, 0.f));
		Title->AddChildToHorizontalBox(MakeImage(WidgetTree, PaintedIconBrush(TEXT("HudBannerRuleOut"), BannerRulePicture(true),
			BannerPixelsPerUnit, RuleSize)))->SetVerticalAlignment(VAlign_Center);
		Place(Title, Centre + FVector2D(0.f, BannerTitleDrop));

		RewardText = MakeFloatingText(WidgetTree, BannerRewardSize, Color::CyanText(), BannerRewardSpacing, ETextJustify::Center);
		Place(RewardText, Centre + FVector2D(0.f, BannerRewardDrop));

		USizeBox* Root = MakeSized(WidgetTree, Canvas, Width, Height);
		// It pops from the gem's centre, the box's middle.
		Root->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		WidgetTree->RootWidget = Root;
		Banner = Root;
		ApplyTexts();
		PaintShow();
	}
	return Super::RebuildWidget();
}

void UHudLevelUpBannerWidget::Show(int32 NewLevel)
{
	Level = NewLevel;
	Time = 0.f;
	ApplyTexts();
	PaintShow();
	// With the banner, which waits for the experience bar to get there, so the fanfare and the gem's flare land together.
	LooterSound::Play2D(this, LooterSoundCue::LevelUp);
}

void UHudLevelUpBannerWidget::ApplyTexts()
{
	if (!LevelText || !RewardText)
	{
		return;
	}
	LevelText->SetText(FText::AsNumber(Level));
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
}

void UHudLevelUpBannerWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (Time < 0.f)
	{
		return;
	}
	Time += InDeltaTime;
	if (Time >= BannerSeconds)
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
	// Each step eases out, as the mockup's keyframes do.
	auto Step = [](float From, float To, float Share) { return FMath::Clamp((Share - From) / (To - From), 0.f, 1.f); };
	auto EaseOut = [](float Alpha) { return FMath::InterpEaseOut(0.f, 1.f, Alpha, 2.f); };
	const float Share = Time / BannerSeconds;
	float Scale = 1.f;
	float Opacity = 1.f;
	if (Share < BannerInShare)
	{
		const float In = EaseOut(Step(0.f, BannerInShare, Share));
		Scale = FMath::Lerp(BannerStartScale, BannerOvershoot, In);
		Opacity = In;
	}
	else if (Share < BannerSettleShare)
	{
		Scale = FMath::Lerp(BannerOvershoot, 1.f, EaseOut(Step(BannerInShare, BannerSettleShare, Share)));
	}
	else if (Share > BannerOutShare)
	{
		Opacity = 1.f - EaseOut(Step(BannerOutShare, 1.f, Share));
	}
	Banner->SetRenderScale(FVector2D(Scale));
	Banner->SetRenderOpacity(Opacity);
}

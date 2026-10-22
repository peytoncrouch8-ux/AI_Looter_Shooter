// UBountyBoardWidget's header: the board's gunmetal plate under the panel's tab, the ranger's star in its medal, BOUNTIES
// in the kit's display type, the three tiers' key (diamonds in their rank colours), and the orange rule under it all, with
// its cyan tail. Pictures are the kit's painted icons (made once, shared); only the medal's glass is a background.

#include "UI/World/BountyBoardWidget.h"
#include "Bounties/BountyBoard.h"
#include "UI/Style/LooterUIStyle.h"
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

#define LOCTEXT_NAMESPACE "LooterBounty"

using namespace LooterUI;

namespace
{
	/** The plate's picture, in its own box (its view box is its size in units; drawn at 1.5 texels a unit): the plate and the rule under it. */
	constexpr float PlateWidth = 1192.f;
	constexpr float PlateBody = 66.f;
	constexpr float PlateHeight = 72.f;
	constexpr float PlatePixelsPerUnit = 1.5f;

	/** The ranger's star in its medal (the mission tracker's, at 52 units: ink rim, gunmetal ring, glass, star). */
	constexpr float MedalArt = 52.f;
	constexpr float MedalRim = 25.5f;
	constexpr float MedalRing = 23.f;
	constexpr float MedalGlass = 18.5f;
	constexpr float StarOuter = 15.6f;
	constexpr float StarInner = 6.8f;
	constexpr float MedalPixelsPerUnit = 3.f;

	// 27, not 34: at 34 the title and its line stood taller than the plate and the line fell onto the rule (PIE 2026-10-22).
	constexpr int32 WordSize = 27;
	constexpr int32 WordSpacing = 420;
	constexpr int32 LineSize = 11;
	constexpr int32 KeySize = 11;
	constexpr float KeyPip = 10.f;

	TArray<FVector2D> Circle(const FVector2D& Center, float Radius, int32 Segments = 48)
	{
		TArray<FVector2D> Points;
		for (int32 Index = 0; Index < Segments; ++Index)
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

	FPaintLayer Fill(TArray<TArray<FVector2D>> Shapes, const FLinearColor& LayerColor, float Opacity = 1.f)
	{
		FPaintLayer Layer;
		Layer.Fills = MoveTemp(Shapes);
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

	/** A fill that runs from one colour to another down the picture between two heights. */
	FPaintLayer Down(TArray<TArray<FVector2D>> Shapes, const FLinearColor& Top, const FLinearColor& Bottom, float FromY, float ToY)
	{
		FPaintLayer Layer = Fill(MoveTemp(Shapes), Top);
		Layer.GradientTo = Bottom;
		Layer.GradientStart = FVector2D(0.f, FromY);
		Layer.GradientEnd = FVector2D(0.f, ToY);
		return Layer;
	}

	/**
	 * The plate: a gunmetal slab with its top-left and bottom-right corners cut, lit from above (the upper half MetalHi to
	 * MetalMid, the lower MetalLow to MetalDeep, as the HUD's bezels), an ink rim, a cyan hairline inside it, a rivet at each
	 * end; under it the orange rule, slanted at its end, and a cyan hairline carrying on from it past a small diamond.
	 */
	const FPaintedIcon& PlatePicture()
	{
		static const FPaintedIcon Picture = []
		{
			FPaintedIcon Result;
			Result.ViewBox = FVector2D(PlateWidth, PlateHeight);
			const float Mid = PlateBody * 0.5f;
			const TArray<FVector2D> Upper = { FVector2D(12.f, 0.f), FVector2D(PlateWidth, 0.f), FVector2D(PlateWidth, Mid), FVector2D(0.f, Mid), FVector2D(0.f, 12.f) };
			const TArray<FVector2D> Lower = { FVector2D(0.f, Mid), FVector2D(PlateWidth, Mid), FVector2D(PlateWidth, PlateBody - 12.f),
				FVector2D(PlateWidth - 12.f, PlateBody), FVector2D(0.f, PlateBody) };
			Result.Layers.Add(Down({ Upper }, Color::MetalHi(), Color::MetalMid(), 0.f, Mid));
			Result.Layers.Add(Down({ Lower }, Color::MetalLow(), Color::MetalDeep(), Mid, PlateBody));
			Result.Layers.Add(Stroke({ Closed({ FVector2D(12.f, 0.f), FVector2D(PlateWidth, 0.f), FVector2D(PlateWidth, PlateBody - 12.f),
				FVector2D(PlateWidth - 12.f, PlateBody), FVector2D(0.f, PlateBody), FVector2D(0.f, 12.f) }) }, 2.4f, Color::Ink()));
			Result.Layers.Add(Stroke({ Closed({ FVector2D(14.f, 6.f), FVector2D(PlateWidth - 6.f, 6.f), FVector2D(PlateWidth - 6.f, PlateBody - 16.f),
				FVector2D(PlateWidth - 16.f, PlateBody - 6.f), FVector2D(6.f, PlateBody - 6.f), FVector2D(6.f, 14.f) }) }, 1.f, Color::Hairline(), 0.5f));
			for (const float RivetX : { 17.f, PlateWidth - 17.f })
			{
				const FVector2D Rivet(RivetX, Mid);
				Result.Layers.Add(Fill({ Circle(Rivet, 5.f, 24) }, Color::Ink()));
				Result.Layers.Add(Down({ Circle(Rivet, 3.7f, 24) }, Color::MetalHi(), Color::MetalLow(), Mid - 4.f, Mid + 4.f));
				Result.Layers.Add(Fill({ Circle(Rivet + FVector2D(-1.1f, -1.2f), 1.1f, 12) }, FLinearColor::White, 0.55f));
			}
			// The rule: orange, slanted at its end, with a diamond and a cyan hairline carrying on.
			const float RuleTop = PlateBody + 1.f;
			const float RuleEnd = PlateWidth - 112.f;
			Result.Layers.Add(Fill({ { FVector2D(0.f, RuleTop), FVector2D(RuleEnd, RuleTop), FVector2D(RuleEnd - 5.f, PlateHeight), FVector2D(0.f, PlateHeight) } },
				Color::Accent()));
			const FVector2D Diamond(RuleEnd + 12.f, RuleTop + 2.5f);
			Result.Layers.Add(Fill({ { Diamond + FVector2D(0.f, -4.f), Diamond + FVector2D(4.f, 0.f), Diamond + FVector2D(0.f, 4.f), Diamond + FVector2D(-4.f, 0.f) } },
				Color::Accent()));
			Result.Layers.Add(Stroke({ { FVector2D(RuleEnd + 22.f, RuleTop + 2.5f), FVector2D(PlateWidth, RuleTop + 2.5f) } }, 1.3f, Color::Hairline(), 0.85f));
			return Result;
		}();
		return Picture;
	}

	const FPaintedIcon& MedalMetalPicture()
	{
		static const FPaintedIcon Picture = []
		{
			FPaintedIcon Result;
			Result.ViewBox = FVector2D(MedalArt);
			const FVector2D Center(MedalArt * 0.5f);
			Result.Layers.Add(Fill({ Circle(Center, MedalRim, 64), Circle(Center, MedalGlass, 64) }, Color::Ink()));
			Result.Layers.Add(Down({ Circle(Center, MedalRing, 64), Circle(Center, MedalGlass, 64) }, Color::BarMetalHi(), Color::BarMetalLow(),
				Center.Y - MedalRing, Center.Y + MedalRing));
			return Result;
		}();
		return Picture;
	}

	const FPaintedIcon& MedalGlassPicture()
	{
		static const FPaintedIcon Picture = []
		{
			FPaintedIcon Result;
			Result.ViewBox = FVector2D(MedalArt);
			Result.Layers.Add(Fill({ Circle(FVector2D(MedalArt * 0.5f), MedalGlass, 64) }, Color::ScreenBg().CopyWithNewOpacity(0.9f)));
			return Result;
		}();
		return Picture;
	}

	/** The cyan hairline inside the ring and the ranger's star, orange in a heavy ink edge, lit along its upper left. */
	const FPaintedIcon& MedalStarPicture()
	{
		static const FPaintedIcon Picture = []
		{
			FPaintedIcon Result;
			Result.ViewBox = FVector2D(MedalArt);
			const FVector2D Center(MedalArt * 0.5f);
			Result.Layers.Add(Stroke({ Closed(Circle(Center, MedalGlass + 0.6f, 64)) }, 1.f, Color::Hairline(), 0.6f));
			TArray<FVector2D> Star;
			for (int32 Index = 0; Index < 10; ++Index)
			{
				const float Angle = FMath::DegreesToRadians(-90.f + 36.f * static_cast<float>(Index));
				Star.Add(Center + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * (Index % 2 == 0 ? StarOuter : StarInner));
			}
			Result.Layers.Add(Fill({ Star }, Color::Accent()));
			Result.Layers.Add(Stroke({ Closed(Star) }, 2.f, Color::Ink()));
			Result.Layers.Add(Stroke({ { Center + FVector2D(-11.4f, -3.9f), Center + FVector2D(-3.1f, -4.3f), Center + FVector2D(0.f, -12.1f) } }, 1.3f,
				FLinearColor::White, 0.55f));
			return Result;
		}();
		return Picture;
	}

	/** A tier's colour on the board: its mark's rank's tag colour (the cards' too). */
	FLinearColor TierTint(EBountyTier Tier)
	{
		switch (Tier)
		{
		case EBountyTier::Dangerous:
			return Color::TierHallowed();
		case EBountyTier::Deadly:
			return Color::TierEthereal();
		default:
			return Color::TierGravebound();
		}
	}
}

UWidget* UBountyBoardWidget::MakeHeader()
{
	const ABountyBoard* Here = Board.Get();
	UOverlay* Plate = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	// The metal and its rule.
	FillOverlaySlot(Plate->AddChildToOverlay(MakeImage(WidgetTree, PaintedIconBrush(TEXT("BountyBoardPlate"), PlatePicture(), PlatePixelsPerUnit,
		FVector2D(PlateWidth, PlateHeight)))));

	// Left: the medal and the board's name.
	UHorizontalBox* Left = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UOverlay* Medal = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	const FVector2D MedalSize(MedalArt);
	FillOverlaySlot(Medal->AddChildToOverlay(MakeImage(WidgetTree, PaintedIconBrush(TEXT("BountyMedalMetal"), MedalMetalPicture(), MedalPixelsPerUnit, MedalSize))));
	UImage* Glass = MakeImage(WidgetTree, PaintedIconBrush(TEXT("BountyMedalGlass"), MedalGlassPicture(), MedalPixelsPerUnit, MedalSize));
	MarkBackground(Glass);
	FillOverlaySlot(Medal->AddChildToOverlay(Glass));
	FillOverlaySlot(Medal->AddChildToOverlay(MakeImage(WidgetTree, PaintedIconBrush(TEXT("BountyMedalStar"), MedalStarPicture(), MedalPixelsPerUnit, MedalSize))));
	Left->AddChildToHorizontalBox(MakeSized(WidgetTree, Medal, MedalArt, MedalArt))->SetVerticalAlignment(VAlign_Center);

	UVerticalBox* Words = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	UTextBlock* Word = MakeText(WidgetTree, Here ? Here->Title.ToString() : LOCTEXT("BoardScreenTitle", "Bounties").ToString(), WordSize, Color::Title(), true,
		WordSpacing);
	Word->SetFont(DisplayFont(WordSize, WordSpacing));
	Words->AddChildToVerticalBox(Word);
	Words->AddChildToVerticalBox(MakeText(WidgetTree, LOCTEXT("PlateLine", "Rim Rangers · Ransom's Rest").ToString(), LineSize, Color::CyanText(), true, 220))
		->SetPadding(FMargin(2.f, -2.f, 0.f, 0.f));
	UHorizontalBoxSlot* WordsSlot = Left->AddChildToHorizontalBox(Words);
	WordsSlot->SetVerticalAlignment(VAlign_Center);
	WordsSlot->SetPadding(FMargin(16.f, 0.f, 0.f, 0.f));
	UOverlaySlot* LeftSlot = Plate->AddChildToOverlay(Left);
	LeftSlot->SetHorizontalAlignment(HAlign_Left);
	LeftSlot->SetVerticalAlignment(VAlign_Center);
	LeftSlot->SetPadding(FMargin(32.f, 0.f, 0.f, 6.f));

	// Right: the tiers' key, one to three diamonds and the tier's word, each in its rank's colour.
	UHorizontalBox* Key = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	for (int32 TierIndex = 0; TierIndex < 3; ++TierIndex)
	{
		const EBountyTier Tier = static_cast<EBountyTier>(TierIndex);
		const FLinearColor Tint = TierTint(Tier);
		UHorizontalBox* Entry = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		for (int32 Pip = 0; Pip <= TierIndex; ++Pip)
		{
			UHorizontalBoxSlot* PipSlot = Entry->AddChildToHorizontalBox(MakeSized(WidgetTree, MakeImage(WidgetTree, MarkerBrush(EMarker::Diamond, Tint)), KeyPip, KeyPip));
			PipSlot->SetVerticalAlignment(VAlign_Center);
			PipSlot->SetPadding(FMargin(Pip == 0 ? 0.f : 2.f, 0.f, 0.f, 0.f));
		}
		UHorizontalBoxSlot* TierSlot = Entry->AddChildToHorizontalBox(MakeText(WidgetTree, BountyRules::TierWord(Tier).ToString(), KeySize, Tint, true, 160));
		TierSlot->SetVerticalAlignment(VAlign_Center);
		TierSlot->SetPadding(FMargin(7.f, 0.f, 0.f, 0.f));
		Key->AddChildToHorizontalBox(Entry)->SetPadding(FMargin(TierIndex == 0 ? 0.f : 24.f, 0.f, 0.f, 0.f));
	}
	UOverlaySlot* KeySlot = Plate->AddChildToOverlay(Key);
	KeySlot->SetHorizontalAlignment(HAlign_Right);
	KeySlot->SetVerticalAlignment(VAlign_Center);
	KeySlot->SetPadding(FMargin(0.f, 0.f, 34.f, 6.f));

	return MakeSized(WidgetTree, Plate, 0.f, PlateHeight);
}

#undef LOCTEXT_NAMESPACE

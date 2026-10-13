#include "UI/Inventory/MapCompass.h"
#include "UI/HUD/MinimapCompass.h"
#include "UI/Style/LooterUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/TextBlock.h"

using namespace LooterUI;

namespace
{
	/** The N's disc and letter, and the dimmer, smaller ones of E, S and W (page units: the page is 1600 x 900). */
	constexpr float NorthDisc = 26.f;
	constexpr int32 NorthTextSize = 12;
	constexpr float SideDisc = 22.f;
	constexpr int32 SideTextSize = 10;
	/** The rings round the discs. */
	constexpr float NorthRing = 1.6f;
	constexpr float SideRing = 1.2f;
	constexpr float SideRingOpacity = 0.65f;
}

void MapCompass::Build(UWidgetTree* Tree, UCanvasPanel& Page, const FVector2D& FramePosition, const FVector2D& FrameSize)
{
	using MinimapCompass::EPoint;
	for (const EPoint Point : MinimapCompass::All)
	{
		const bool bNorth = Point == EPoint::North;
		const float Size = bNorth ? NorthDisc : SideDisc;

		// The disc is ink with a ring, like the minimap bezel's; the letter goes over it.
		UOverlay* Badge = Tree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		Badge->SetVisibility(ESlateVisibility::HitTestInvisible);
		FLinearColor Ring = bNorth ? Color::Accent() : Color::TextDim();
		Ring.A *= bNorth ? 1.f : SideRingOpacity;
		FillOverlaySlot(Badge->AddChildToOverlay(MakeImage(Tree, CircleBrush(Color::Ink(), Ring, bNorth ? NorthRing : SideRing))));
		UTextBlock* Letter = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),
			FName(*FString::Printf(TEXT("MapCompass_%s"), MinimapCompass::Letter(Point))));
		StyleFloatingText(Letter, bNorth ? NorthTextSize : SideTextSize, bNorth ? Color::Accent() : Color::TextDim(), 0, ETextJustify::Center);
		Letter->SetText(FText::FromString(MinimapCompass::Letter(Point)));
		UOverlaySlot* LetterSlot = Badge->AddChildToOverlay(Letter);
		LetterSlot->SetHorizontalAlignment(HAlign_Center);
		LetterSlot->SetVerticalAlignment(VAlign_Center);

		UCanvasPanelSlot* BadgeSlot = Page.AddChildToCanvas(Badge);
		BadgeSlot->SetAlignment(FVector2D(0.5, 0.5));
		BadgeSlot->SetPosition(MinimapCompass::EdgeMidpoint(Point, FramePosition, FrameSize));
		BadgeSlot->SetSize(FVector2D(Size));
	}
}

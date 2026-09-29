#include "UI/Inventory/LoadoutWidget.h"
#include "UI/Inventory/LoadoutPaintLayer.h"
#include "UI/Inventory/LoadoutParts.h"
#include "UI/Inventory/LoadoutRules.h"
#include "AI_Looter_Shooter.h"
#include "UI/Inventory/LoadoutStage.h"
#include "UI/Style/LooterButton.h"
#include "UI/HUD/LooterHUD.h"
#include "UI/Style/LooterUIStyle.h"
#include "UI/Style/WeaponText.h"
#include "Settings/KeyBindingSubsystem.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponDefinition.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScaleBox.h"
#include "Components/ScrollBox.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/LocalPlayer.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "InputCoreTypes.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Rendering/DrawElements.h"
#include "Templates/UnrealTemplate.h"

using namespace LooterUI;
using namespace LoadoutParts;

// ---------------------------------------------------------------------------
// The stand's ring and the callouts
// ---------------------------------------------------------------------------

bool ULoadoutWidget::ProjectToPage(const FVector& WorldLocation, FVector2f& OutPoint) const
{
	const ALoadoutStage* StagePtr = Stage.Get();
	FVector2D UV;
	if (!StagePtr || !StagePtr->ProjectToImage(WorldLocation, UV))
	{
		return false;
	}
	OutPoint = FVector2f(StageTopLeft + UV * StageSize);
	return true;
}

namespace
{
	/** Points around a ring on the floor under the stand-in, from one angle to another (degrees, 0 = toward the camera). */
	template <typename FProject>
	TArray<FVector2f> RingArc(const ALoadoutStage& Stage, float Radius, float From, float To, FProject&& Project)
	{
		const FVector Center = Stage.GetFloorCenter();
		const FVector Toward = Stage.GetTowardCamera();
		const FVector Side = FVector::CrossProduct(FVector::UpVector, Toward);
		TArray<FVector2f> Points;
		constexpr int32 Steps = 40;
		for (int32 Step = 0; Step <= Steps; ++Step)
		{
			const float Angle = FMath::DegreesToRadians(FMath::Lerp(From, To, static_cast<float>(Step) / Steps));
			FVector2f Point;
			if (Project(Center + (Toward * FMath::Cos(Angle) + Side * FMath::Sin(Angle)) * Radius, Point))
			{
				Points.Add(Point);
			}
		}
		return Points;
	}
}

void ULoadoutWidget::PaintBack(const FGeometry& Geometry, FSlateWindowElementList& Elements, int32 LayerId) const
{
	const ALoadoutStage* StagePtr = Stage.Get();
	if (!StagePtr || !StageMaterial)
	{
		return;
	}
	auto Project = [this](const FVector& World, FVector2f& Out) { return ProjectToPage(World, Out); };

	// A soft glow over the whole ring.
	const TArray<FVector2f> Circle = RingArc(*StagePtr, OuterRingRadius, 0.f, 360.f, Project);
	if (Circle.Num() > 2)
	{
		const FBox2f Bounds(Circle);
		DrawBox(Elements, LayerId, Geometry, Bounds.Min, Bounds.GetSize(), &DiscBrush, Colors::RingGlow());
	}
	// Faint pillars rising from the ring's sides.
	const FVector Center = StagePtr->GetFloorCenter();
	const FVector Side = FVector::CrossProduct(FVector::UpVector, StagePtr->GetTowardCamera());
	for (const float Sign : { -1.f, 1.f })
	{
		const FVector Foot = Center + Side * (Sign * OuterRingRadius);
		FVector2f Bottom, Top;
		if (ProjectToPage(Foot, Bottom) && ProjectToPage(Foot + FVector(0.f, 0.f, PillarHeight), Top))
		{
			DrawLines(Elements, LayerId, Geometry, { Bottom, Top }, Colors::Pillar(), 2.f);
		}
	}
	// The far halves of the rings, behind the stand-in's feet.
	DrawLines(Elements, LayerId, Geometry, RingArc(*StagePtr, OuterRingRadius, 90.f, 270.f, Project), Colors::Ring(), 2.f);
	DrawLines(Elements, LayerId, Geometry, RingArc(*StagePtr, InnerRingRadius, 90.f, 270.f, Project), Colors::InnerRing(), 1.5f);
}

void ULoadoutWidget::PaintFront(const FGeometry& Geometry, FSlateWindowElementList& Elements, int32 LayerId) const
{
	const ALoadoutStage* StagePtr = Stage.Get();
	if (!StagePtr || !StageMaterial)
	{
		return;
	}
	auto Project = [this](const FVector& World, FVector2f& Out) { return ProjectToPage(World, Out); };

	// The near halves of the rings, in front of the feet.
	DrawLines(Elements, LayerId, Geometry, RingArc(*StagePtr, OuterRingRadius, -90.f, 90.f, Project), Colors::Ring(), 2.f);
	DrawLines(Elements, LayerId, Geometry, RingArc(*StagePtr, InnerRingRadius, -90.f, 90.f, Project), Colors::InnerRing(), 1.5f);

	// Callouts: from each slot card, level to the elbow, then to where its gun is carried. The gun in hand's is orange.
	const int32 ActiveSlot = GetActiveSlot();
	for (int32 SlotIndex = 0; SlotIndex < SlotCards.Num(); ++SlotIndex)
	{
		FVector Anchor;
		FVector2f End;
		if (!StagePtr->GetSlotAnchor(SlotIndex, Anchor) || !ProjectToPage(Anchor, End))
		{
			continue;
		}
		const FGeometry& CardGeometry = SlotCards[SlotIndex].Button->GetCachedGeometry();
		const FVector2f CardSize(CardGeometry.GetLocalSize());
		if (CardSize.X <= 0.f)
		{
			continue;
		}
		const FVector2f Start(Geometry.AbsoluteToLocal(CardGeometry.LocalToAbsolute(FVector2f(CardSize.X, CardSize.Y * 0.5f))));
		const bool bInHand = SlotIndex == ActiveSlot;
		const FLinearColor LineColor = bInHand ? Color::Accent() : (SlotIndex == ChosenSlot ? Color::TileLine() : Colors::Callout());
		DrawLines(Elements, LayerId, Geometry, { Start, FVector2f(CalloutElbowX, Start.Y), End }, LineColor, bInHand ? 2.f : 1.5f);
		const float Radius = bInHand ? 6.f : 5.f;
		DrawBox(Elements, LayerId + 1, Geometry, End - FVector2f(Radius), FVector2f(Radius * 2.f), &DotBrush,
			bInHand ? Color::Accent() : Color::SegmentOn());
	}
}

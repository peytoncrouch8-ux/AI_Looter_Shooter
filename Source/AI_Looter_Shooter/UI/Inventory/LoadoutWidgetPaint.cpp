// ULoadoutWidget: the showcase's ring under the gun (in its rarity's colour), and the screen's motion: the card sliding
// in, a row's flash as a gun lands in it, and the top rarities' glow breathing.

#include "UI/Inventory/LoadoutWidget.h"
#include "UI/Inventory/LoadoutGunStage.h"
#include "UI/Inventory/LoadoutParts.h"
#include "UI/Style/LooterUIStyle.h"
#include "Components/Image.h"
#include "Components/VerticalBox.h"
#include "Rendering/DrawElements.h"

using namespace LooterUI;
using namespace LoadoutParts;

namespace
{
	/** The inner ring sits this far in from the outer one, as on the other stands. */
	constexpr float InnerRingShare = 32.f / 46.f;
	/** A breathing glow swings this far either side of its brightness, once every BreathSeconds. */
	constexpr float BreathDepth = 0.25f;
	constexpr float BreathSeconds = 2.8f;
	/** How strongly a landing gun's row flashes at first. */
	constexpr float FlashStrength = 0.6f;
	/** The card's contents slide in from this far right, from this faint. */
	constexpr float IntroOffset = 14.f;
	constexpr float IntroOpacity = 0.2f;

	/** Points around a ring on the floor under the gun, from one angle to another (degrees, 0 = toward the camera). */
	template <typename FProject>
	TArray<FVector2f> ShowcaseRingArc(const ALoadoutGunStage& Stage, float Radius, float From, float To, FProject&& Project)
	{
		const FVector Center = Stage.GetFloorCenter();
		const FVector Toward = Stage.GetTowardCamera();
		const FVector Side = FVector::CrossProduct(FVector::UpVector, Toward);
		TArray<FVector2f> Points;
		constexpr int32 Steps = 48;
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

	float EaseOut(float Alpha)
	{
		return 1.f - FMath::Cube(1.f - FMath::Clamp(Alpha, 0.f, 1.f));
	}
}

// ---------------------------------------------------------------------------
// The showcase's ring
// ---------------------------------------------------------------------------

bool ULoadoutWidget::ProjectToPage(const FVector& WorldLocation, FVector2f& OutPoint) const
{
	const ALoadoutGunStage* StagePtr = Stage.Get();
	FVector2D UV;
	if (!StagePtr || !StagePtr->ProjectToImage(WorldLocation, UV))
	{
		return false;
	}
	const FVector2D TopLeft = bInspecting ? LoadoutLayout::InspectShowcaseTopLeft : LoadoutLayout::ShowcaseTopLeft;
	const FVector2D Size = bInspecting ? LoadoutLayout::InspectShowcaseSize : LoadoutLayout::ShowcaseSize;
	OutPoint = FVector2f(TopLeft + UV * Size);
	return true;
}

void ULoadoutWidget::PaintBack(const FGeometry& Geometry, FSlateWindowElementList& Elements, int32 LayerId) const
{
	const ALoadoutGunStage* StagePtr = Stage.Get();
	if (!StagePtr || !StageMaterial || !StagePtr->HasGun())
	{
		return;
	}
	auto Project = [this](const FVector& World, FVector2f& Out) { return ProjectToPage(World, Out); };
	const float Radius = StagePtr->GetRingRadius();
	const float Breath = bCardBreathes ? 1.f + BreathDepth * FMath::Sin(UE_TWO_PI * Clock / BreathSeconds) : 1.f;

	// A soft pool of the gun's rarity on the floor, then the far halves of the rings behind it.
	const TArray<FVector2f> Circle = ShowcaseRingArc(*StagePtr, Radius * 1.25f, 0.f, 360.f, Project);
	if (Circle.Num() > 2)
	{
		const FBox2f Bounds(Circle);
		DrawBox(Elements, LayerId, Geometry, Bounds.Min, Bounds.GetSize(), &DiscBrush, CardRarity * FLinearColor(1.f, 1.f, 1.f, 0.16f * Breath));
	}
	DrawLines(Elements, LayerId, Geometry, ShowcaseRingArc(*StagePtr, Radius, 90.f, 270.f, Project), CardRarity * FLinearColor(1.f, 1.f, 1.f, 0.7f), 2.f);
	DrawLines(Elements, LayerId, Geometry, ShowcaseRingArc(*StagePtr, Radius * InnerRingShare, 90.f, 270.f, Project), Colors::InnerRing(), 1.5f);
}

void ULoadoutWidget::PaintFront(const FGeometry& Geometry, FSlateWindowElementList& Elements, int32 LayerId) const
{
	const ALoadoutGunStage* StagePtr = Stage.Get();
	if (!StagePtr || !StageMaterial || !StagePtr->HasGun())
	{
		return;
	}
	auto Project = [this](const FVector& World, FVector2f& Out) { return ProjectToPage(World, Out); };
	const float Radius = StagePtr->GetRingRadius();
	DrawLines(Elements, LayerId, Geometry, ShowcaseRingArc(*StagePtr, Radius, -90.f, 90.f, Project), CardRarity * FLinearColor(1.f, 1.f, 1.f, 0.7f), 2.f);
	DrawLines(Elements, LayerId, Geometry, ShowcaseRingArc(*StagePtr, Radius * InnerRingShare, -90.f, 90.f, Project), Colors::InnerRing(), 1.5f);
}

// ---------------------------------------------------------------------------
// Motion
// ---------------------------------------------------------------------------

void ULoadoutWidget::TickMotion(float DeltaTime)
{
	Clock += DeltaTime;

	// The card's contents slide in when it changes to another gun: quick, so browsing never waits on it.
	if (CardBox && CardIntroAge < LoadoutLayout::CardIntroSeconds + DeltaTime)
	{
		CardIntroAge += DeltaTime;
		const float Alpha = EaseOut(CardIntroAge / LoadoutLayout::CardIntroSeconds);
		CardBox->SetRenderOpacity(FMath::Lerp(IntroOpacity, 1.f, Alpha));
		CardBox->SetRenderTranslation(FVector2D(IntroOffset * (1.f - Alpha), 0.f));
	}

	// A gun that just landed in a row flashes its rarity there, fading fast.
	if (FlashIndex != INDEX_NONE)
	{
		FlashAge += DeltaTime;
		const TArray<FRow>& Rows = FlashZone == EZone::Slots ? SlotRows : ListRows;
		if (Rows.IsValidIndex(FlashIndex) && Rows[FlashIndex].Flash)
		{
			const float Fade = FMath::Square(1.f - FMath::Clamp(FlashAge / LoadoutLayout::FlashSeconds, 0.f, 1.f));
			Rows[FlashIndex].Flash->SetColorAndOpacity(Rows[FlashIndex].Rarity * FLinearColor(1.f, 1.f, 1.f, FlashStrength * Fade));
		}
		if (FlashAge >= LoadoutLayout::FlashSeconds)
		{
			FlashIndex = INDEX_NONE;
		}
	}

	// The top rarities' glow behind the card breathes, slowly.
	if (CardGlow && bCardBreathes)
	{
		const float Breath = 1.f + BreathDepth * FMath::Sin(UE_TWO_PI * Clock / BreathSeconds);
		CardGlow->SetColorAndOpacity(CardRarity * FLinearColor(1.f, 1.f, 1.f, 0.14f * Breath));
	}
}

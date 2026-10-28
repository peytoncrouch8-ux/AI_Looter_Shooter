#include "UI/World/ObjectiveMarkerActor.h"
#include "UI/World/ObjectiveMarkerWidget.h"
#include "Components/DecalComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/MeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/World.h"

namespace
{
	/** The pop as it lights. */
	constexpr float PopSeconds = 0.35f;
	constexpr float PopPeak = 1.25f;
	/** The glow's breath: slow, as the highlight's own. */
	constexpr float GlowRate = 2.2f;
	/** Less than this showing: the widget is hidden, so a faded mark costs nothing to draw. */
	constexpr float ShownAbove = 0.01f;

	float EaseOut(float Alpha)
	{
		const float Left = 1.f - FMath::Clamp(Alpha, 0.f, 1.f);
		return 1.f - Left * Left * Left;
	}
}

AObjectiveMarkerActor::AObjectiveMarkerActor()
{
	// Driven by the highlight subsystem, only while its paper is lit.
	PrimaryActorTick.bCanEverTick = false;
	SetCanBeDamaged(false);

	Widget = CreateDefaultSubobject<UWidgetComponent>(TEXT("Widget"));
	Widget->SetWidgetSpace(EWidgetSpace::Screen);
	Widget->SetDrawAtDesiredSize(true);
	// Bottom-middle on the spot over the paper: the mark stands over it, never on the words.
	Widget->SetPivot(FVector2D(0.5f, 1.f));
	Widget->SetWidgetClass(UObjectiveMarkerWidget::StaticClass());
	Widget->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Widget->SetGenerateOverlapEvents(false);
	Widget->SetVisibility(false);
	SetRootComponent(Widget);

	Phase = FMath::FRandRange(0.f, UE_TWO_PI);
}

void AObjectiveMarkerActor::Show(AActor* InTarget, const FVector& Point)
{
	Target = InTarget;
	Age = 0.f;
	Presence = 0.f;
	Near = -1.f;
	SetActorLocation(Point);
	// Its widget made as it's first shown where someone plays (a test level only checks the rules).
	const UWorld* World = GetWorld();
	if (!Widget->GetUserWidgetObject() && World && World->IsGameWorld())
	{
		Widget->InitWidget();
	}
}

void AObjectiveMarkerActor::Release()
{
	Target.Reset();
	Presence = 0.f;
	Widget->SetVisibility(false);
}

FVector AObjectiveMarkerActor::MarkPoint(const AActor& InTarget, const FVector& Focus)
{
	// The paper's own parts: its decals (their box is the sheet's size and spot), and any small mesh (a letter).
	double Top = Focus.Z;
	TInlineComponentArray<USceneComponent*> Parts(&InTarget);
	for (const USceneComponent* Part : Parts)
	{
		const bool bPaper = Part && Part->IsRegistered() && Part->IsVisible()
			&& (Part->IsA<UDecalComponent>() || (Part->IsA<UMeshComponent>() && !Part->IsA<UInstancedStaticMeshComponent>()));
		if (bPaper && Part->Bounds.SphereRadius <= MostPaperRadius)
		{
			Top = FMath::Max(Top, Part->Bounds.GetBox().Max.Z);
		}
	}
	return FVector(Focus.X, Focus.Y, Top + OverTop);
}

float AObjectiveMarkerActor::NearFade(float Distance)
{
	return FMath::SmoothStep(GoneWithin, FullFrom, Distance);
}

float AObjectiveMarkerActor::PopAt(float InAge)
{
	const float Alpha = InAge / PopSeconds;
	if (Alpha >= 1.f)
	{
		return 1.f;
	}
	// Up from nothing past its size, then settling: a mark arriving, not just appearing.
	constexpr float Rise = 0.55f;
	return Alpha < Rise ? FMath::Lerp(0.f, PopPeak, EaseOut(Alpha / Rise)) : FMath::Lerp(PopPeak, 1.f, EaseOut((Alpha - Rise) / (1.f - Rise)));
}

void AObjectiveMarkerActor::Advance(float DeltaSeconds, float Strength, float Distance, const FVector& Point)
{
	if (IsFree())
	{
		return;
	}
	Age += FMath::Max(DeltaSeconds, 0.f);
	const float NearWanted = NearFade(Distance);
	Near = Near < 0.f ? NearWanted : FMath::FInterpConstantTo(Near, NearWanted, FMath::Max(DeltaSeconds, 0.f), 1.f / NearFadeSeconds);
	Presence = FMath::Clamp(Strength, 0.f, 1.f) * Near;
	SetActorLocation(Point);

	const bool bShown = Presence > ShownAbove;
	if (Widget->IsVisible() != bShown)
	{
		Widget->SetVisibility(bShown);
	}
	if (!bShown)
	{
		return;
	}
	const UWorld* World = GetWorld();
	const float Time = World ? static_cast<float>(World->GetTimeSeconds()) : Age;
	if (UObjectiveMarkerWidget* Look = Cast<UObjectiveMarkerWidget>(Widget->GetUserWidgetObject()))
	{
		Look->SetLook(Presence, PopAt(Age), 0.5f + 0.5f * FMath::Sin(Time * GlowRate + Phase));
	}
}

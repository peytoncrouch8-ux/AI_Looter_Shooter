#include "Creatures/BellRingerRing.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	/** The engine cube the dashes are made of is this big (cm). */
	constexpr float CubeSize = 100.f;

	/** Each kind's look: dashes round it, a dash's share of its place (the rest is gap), its width and height (cm), its glow. */
	struct FRingLook
	{
		int32 Count;
		float Share;
		float Width;
		float Height;
		float Glow;
	};

	FRingLook LookOf(EBellRingKind Kind)
	{
		switch (Kind)
		{
		case EBellRingKind::Wave: return { 48, 0.5f, 8.f, 5.f, 4.f };
		case EBellRingKind::Burst: return { 28, 0.55f, 3.5f, 3.5f, 6.f };
		case EBellRingKind::Gather: return { 28, 0.5f, 6.f, 5.f, 5.f };
		case EBellRingKind::Halo: return { 10, 0.45f, 2.5f, 2.5f, 3.f };
		case EBellRingKind::Aura: return { 16, 0.5f, 4.f, 2.f, 1.4f };
		}
		return { 24, 0.5f, 5.f, 5.f, 3.f };
	}

	/** The wave runs out this fast (cm/s), and thins away over FadeSeconds at its full width. */
	constexpr float WaveSpeed = 1500.f;
	constexpr float FadeSeconds = 0.25f;
	/** The burst opens over BurstSeconds to BurstRadius; the gather closes over GatherSeconds. */
	constexpr float BurstSeconds = 0.3f;
	constexpr float BurstRadius = 160.f;
	constexpr float GatherSeconds = 0.8f;
	constexpr float GatherEnd = 25.f;
	/** A halo spins this fast at full strength (degrees a second); a halo or an aura fades over FinishSeconds. */
	constexpr float HaloSpin = 420.f;
	constexpr float FinishSeconds = 0.3f;
	/** Rings on the ground run this far above it, so a slope doesn't swallow them. */
	constexpr float GroundLift = 10.f;
}

ABellRingerRing::ABellRingerRing()
{
	PrimaryActorTick.bCanEverTick = true;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Surface(TEXT("/Game/Environment/Materials/M_StylizedSurface.M_StylizedSurface"));
	Dashes = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Dashes"));
	Dashes->SetupAttachment(Root);
	Dashes->SetMobility(EComponentMobility::Movable);
	Dashes->SetStaticMesh(Cube.Object);
	Dashes->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Dashes->SetCastShadow(false);
	Dashes->SetCanEverAffectNavigation(false);
	Dashes->bReceivesDecals = false;
	GlowBase = Surface.Object;
}

ABellRingerRing* ABellRingerRing::SpawnRing(UWorld* World, const FTransform& Where, EBellRingKind InKind, const FLinearColor& InColor,
	float InRadius)
{
	if (!World)
	{
		return nullptr;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.ObjectFlags |= RF_Transient;
	ABellRingerRing* Ring = World->SpawnActor<ABellRingerRing>(ABellRingerRing::StaticClass(), Where, Params);
	if (!Ring)
	{
		return nullptr;
	}
	Ring->Kind = InKind;
	Ring->Color = InColor;
	Ring->MaxRadius = FMath::Max(InRadius, 10.f);
	Ring->Radius = InKind == EBellRingKind::Gather || InKind == EBellRingKind::Halo || InKind == EBellRingKind::Aura ? Ring->MaxRadius : 0.f;
	Ring->Strength = InKind == EBellRingKind::Halo ? 0.f : 1.f;
	// Its glow is made here rather than as play begins, so a ring in a level that isn't playing (a test's) shows too.
	if (Ring->GlowBase)
	{
		Ring->Glow = UMaterialInstanceDynamic::Create(Ring->GlowBase, Ring);
		Ring->Glow->SetVectorParameterValue(TEXT("Color"), InColor);
		Ring->Glow->SetScalarParameterValue(TEXT("Variation"), 0.f);
		Ring->Dashes->SetMaterial(0, Ring->Glow);
	}
	Ring->Draw();
	return Ring;
}

ABellRingerRing* ABellRingerRing::SpawnWave(UWorld* World, const FVector& Center, float InRadius, const FLinearColor& InColor)
{
	return SpawnRing(World, FTransform(Center + FVector(0.f, 0.f, GroundLift)), EBellRingKind::Wave, InColor, InRadius);
}

ABellRingerRing* ABellRingerRing::SpawnBurst(UWorld* World, const FVector& Center, const FLinearColor& InColor)
{
	return SpawnRing(World, FTransform(Center), EBellRingKind::Burst, InColor, BurstRadius);
}

ABellRingerRing* ABellRingerRing::SpawnGather(UWorld* World, const FVector& Center, float InRadius, const FLinearColor& InColor)
{
	return SpawnRing(World, FTransform(Center + FVector(0.f, 0.f, GroundLift)), EBellRingKind::Gather, InColor, InRadius);
}

ABellRingerRing* ABellRingerRing::SpawnHalo(USceneComponent* Bell, const FVector& Offset, float InRadius, const FLinearColor& InColor)
{
	ABellRingerRing* Ring = Bell ? SpawnRing(Bell->GetWorld(), Bell->GetComponentTransform(), EBellRingKind::Halo, InColor, InRadius) : nullptr;
	if (Ring)
	{
		// It rides the bell (its swing too), at its own scale whatever the bell's.
		Ring->AttachToComponent(Bell, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		Ring->GetRootComponent()->SetRelativeLocation(Offset);
	}
	return Ring;
}

ABellRingerRing* ABellRingerRing::SpawnAura(AActor* On, float FeetBelow, float InRadius, const FLinearColor& InColor)
{
	USceneComponent* Body = On ? On->GetRootComponent() : nullptr;
	ABellRingerRing* Ring = Body ? SpawnRing(On->GetWorld(), Body->GetComponentTransform(), EBellRingKind::Aura, InColor, InRadius) : nullptr;
	if (Ring)
	{
		Ring->AttachToComponent(Body, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		Ring->SetActorRelativeLocation(FVector(0.f, 0.f, GroundLift - FeetBelow));
	}
	return Ring;
}

void ABellRingerRing::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Advance(DeltaSeconds);
}

void ABellRingerRing::SetStrength(float InStrength)
{
	Strength = FMath::Clamp(InStrength, 0.f, 1.f);
}

void ABellRingerRing::Finish()
{
	if (!bFinishing)
	{
		bFinishing = true;
		FinishAge = Age;
	}
}

float ABellRingerRing::Showing() const
{
	switch (Kind)
	{
	case EBellRingKind::Wave:
	{
		const float FullAt = MaxRadius / WaveSpeed;
		return Radius >= MaxRadius ? 1.f - FMath::Clamp((Age - FullAt) / FadeSeconds, 0.f, 1.f) : 1.f;
	}
	case EBellRingKind::Burst:
		return 1.f - FMath::Clamp((Age - BurstSeconds) / FadeSeconds, 0.f, 1.f);
	case EBellRingKind::Gather:
		return 1.f - FMath::Clamp((Age - GatherSeconds) / FadeSeconds, 0.f, 1.f);
	default:
		return bFinishing ? 1.f - FMath::Clamp((Age - FinishAge) / FinishSeconds, 0.f, 1.f) : 1.f;
	}
}

void ABellRingerRing::Advance(float DeltaSeconds)
{
	if (IsActorBeingDestroyed())
	{
		return;
	}
	Age += DeltaSeconds;
	switch (Kind)
	{
	case EBellRingKind::Wave:
		Radius = FMath::Min(Radius + WaveSpeed * DeltaSeconds, MaxRadius);
		break;
	case EBellRingKind::Burst:
		// Out fast, slowing as it opens: a sound's ring rather than a stamp.
		Radius = MaxRadius * (1.f - FMath::Square(1.f - FMath::Clamp(Age / BurstSeconds, 0.f, 1.f)));
		break;
	case EBellRingKind::Gather:
		// Slow at first, then drawn in to the grave.
		Radius = FMath::Lerp(MaxRadius, GatherEnd, FMath::Square(FMath::Clamp(Age / GatherSeconds, 0.f, 1.f)));
		break;
	case EBellRingKind::Halo:
		AddActorLocalRotation(FRotator(0.f, HaloSpin * (0.35f + 0.65f * Strength) * DeltaSeconds, 0.f));
		break;
	case EBellRingKind::Aura:
		break;
	}
	Draw();
	if (Showing() <= 0.f)
	{
		Destroy();
	}
}

void ABellRingerRing::Draw()
{
	const FRingLook Look = LookOf(Kind);
	const float Show = Showing();
	// A halo grows with its strength; everything thins away as it fades.
	const float Size = Show * (Kind == EBellRingKind::Halo ? 0.3f + 0.7f * Strength : 1.f);
	float GlowNow = Look.Glow * Show;
	if (Kind == EBellRingKind::Halo)
	{
		GlowNow = (1.5f + 5.f * Strength) * Show;
	}
	else if (Kind == EBellRingKind::Aura)
	{
		GlowNow = Look.Glow * (0.7f + 0.3f * FMath::Sin(Age * 5.f)) * Show;
	}
	if (Glow && !FMath::IsNearlyEqual(GlowNow, ShownGlow, 0.02f))
	{
		Glow->SetScalarParameterValue(TEXT("Glow"), GlowNow);
		ShownGlow = GlowNow;
	}
	// A halo's spin turns the actor and an aura's pulse is its glow: their dashes are laid again only when they change.
	if (FMath::IsNearlyEqual(Size, ShownSize, 0.005f) && FMath::IsNearlyEqual(Radius, ShownRadius, 0.1f))
	{
		return;
	}
	ShownSize = Size;
	ShownRadius = Radius;
	const float Around = FMath::Max(Radius, 5.f);
	const float Length = FMath::Max(2.f * UE_PI * Around / Look.Count * Look.Share, 2.f);
	TArray<FTransform> Transforms;
	Transforms.Reserve(Look.Count);
	for (int32 Index = 0; Index < Look.Count; ++Index)
	{
		const float Angle = 2.f * UE_PI * static_cast<float>(Index) / static_cast<float>(Look.Count);
		const FVector Out(FMath::Cos(Angle), FMath::Sin(Angle), 0.f);
		// Each dash lies along the ring.
		const FRotator Along(0.f, FMath::RadiansToDegrees(Angle) + 90.f, 0.f);
		const FVector Scale(Length / CubeSize, Look.Width * Size / CubeSize, Look.Height * Size / CubeSize);
		Transforms.Emplace(Along, Out * Radius, Scale.ComponentMax(FVector(0.001f)));
	}
	if (Dashes->GetInstanceCount() != Transforms.Num())
	{
		Dashes->ClearInstances();
		Dashes->AddInstances(Transforms, /*bShouldReturnIndices*/ false, /*bWorldSpace*/ false, /*bUpdateNavigation*/ false);
	}
	else
	{
		Dashes->BatchUpdateInstancesTransforms(0, Transforms, /*bWorldSpace*/ false, /*bMarkRenderStateDirty*/ false, /*bTeleport*/ true);
	}
	Dashes->MarkRenderStateDirty();
}

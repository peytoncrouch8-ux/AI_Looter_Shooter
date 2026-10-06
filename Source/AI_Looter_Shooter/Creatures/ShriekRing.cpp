#include "Creatures/ShriekRing.h"
#include "Combat/MovementSlowComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	/** Dashes round the ring: enough to read as a ring at its full width, with gaps between. */
	constexpr int32 DashCount = 40;
	/** A dash's share of its place round the ring (the rest is gap), and its size across and up (cm). */
	constexpr float DashShare = 0.55f;
	constexpr float DashWidth = 7.f;
	constexpr float DashHeight = 5.f;
	/** It runs this far above the ground (cm), so a slope doesn't swallow it. */
	constexpr float Lift = 12.f;
	/** Seconds it fades over once it's at its full width. */
	constexpr float FadeSeconds = 0.25f;
	/** How brightly it glows (M_StylizedSurface's emissive multiplier of its color). */
	constexpr float RingGlow = 4.f;
	/** The engine cube the dashes are made of is this big (cm). */
	constexpr float CubeSize = 100.f;
}

AShriekRing::AShriekRing()
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

AShriekRing* AShriekRing::Spawn(UWorld* World, const FVector& Center, float InMaxRadius, const FLinearColor& InColor, ACharacter* InVictim,
	float InSlowMultiplier, float InSlowSeconds)
{
	if (!World)
	{
		return nullptr;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.ObjectFlags |= RF_Transient;
	AShriekRing* Ring = World->SpawnActor<AShriekRing>(AShriekRing::StaticClass(), FTransform(Center), Params);
	if (!Ring)
	{
		return nullptr;
	}
	Ring->MaxRadius = FMath::Max(InMaxRadius, 100.f);
	Ring->Color = InColor;
	Ring->Victim = InVictim;
	Ring->SlowMultiplier = InSlowMultiplier;
	Ring->SlowSeconds = InSlowSeconds;
	// Its glow is made here rather than as play begins, so a ring sent out in a level that isn't playing (a test's) shows too.
	if (Ring->GlowBase)
	{
		Ring->Glow = UMaterialInstanceDynamic::Create(Ring->GlowBase, Ring);
		Ring->Glow->SetVectorParameterValue(TEXT("Color"), InColor);
		Ring->Glow->SetScalarParameterValue(TEXT("Glow"), RingGlow);
		Ring->Glow->SetScalarParameterValue(TEXT("Variation"), 0.f);
		Ring->Dashes->SetMaterial(0, Ring->Glow);
	}
	Ring->Draw();
	return Ring;
}

void AShriekRing::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Advance(DeltaSeconds);
}

void AShriekRing::Advance(float DeltaSeconds)
{
	if (IsActorBeingDestroyed())
	{
		return;
	}
	Age += DeltaSeconds;
	Radius = FMath::Min(Radius + Speed * DeltaSeconds, MaxRadius);

	// It slows whoever it was sent at as it passes them, once: their body, not their middle, is what it reaches.
	ACharacter* Target = Victim.Get();
	if (!bSlowed && Target)
	{
		const UCapsuleComponent* Capsule = Target->GetCapsuleComponent();
		const float Body = Capsule ? Capsule->GetScaledCapsuleRadius() : 0.f;
		const float HalfHeight = Capsule ? Capsule->GetScaledCapsuleHalfHeight() : 0.f;
		const FVector To = Target->GetActorLocation() - GetActorLocation();
		const float Flat = static_cast<float>(To.Size2D());
		if (Flat - Body <= Radius && Flat - Body <= MaxRadius && FMath::Abs(static_cast<float>(To.Z)) <= ReachUpDown + HalfHeight)
		{
			UMovementSlowComponent::Apply(Target, SlowMultiplier, SlowSeconds);
			bSlowed = true;
		}
	}

	Draw();
	if (Radius >= MaxRadius && Age >= MaxRadius / Speed + FadeSeconds)
	{
		Destroy();
	}
}

void AShriekRing::Draw()
{
	// At its full width it thins away into the ground.
	const float FullAt = MaxRadius / FMath::Max(Speed, 1.f);
	const float Fade = Radius >= MaxRadius ? 1.f - FMath::Clamp((Age - FullAt) / FadeSeconds, 0.f, 1.f) : 1.f;
	const float Length = FMath::Max(2.f * UE_PI * FMath::Max(Radius, 10.f) / DashCount * DashShare, 4.f);
	const FVector Center = GetActorLocation() + FVector(0.f, 0.f, Lift);
	TArray<FTransform> Transforms;
	Transforms.Reserve(DashCount);
	for (int32 Index = 0; Index < DashCount; ++Index)
	{
		const float Angle = 2.f * UE_PI * static_cast<float>(Index) / static_cast<float>(DashCount);
		const FVector Out(FMath::Cos(Angle), FMath::Sin(Angle), 0.f);
		// Each dash lies along the ring.
		const FRotator Along(0.f, FMath::RadiansToDegrees(Angle) + 90.f, 0.f);
		const FVector Size(Length / CubeSize, DashWidth * Fade / CubeSize, DashHeight * Fade / CubeSize);
		Transforms.Emplace(Along, Center + Out * Radius, Size.ComponentMax(FVector(0.001f)));
	}
	if (Dashes->GetInstanceCount() != Transforms.Num())
	{
		Dashes->ClearInstances();
		Dashes->AddInstances(Transforms, /*bShouldReturnIndices*/ false, /*bWorldSpace*/ true, /*bUpdateNavigation*/ false);
	}
	else
	{
		Dashes->BatchUpdateInstancesTransforms(0, Transforms, /*bWorldSpace*/ true, /*bMarkRenderStateDirty*/ false, /*bTeleport*/ true);
	}
	Dashes->MarkRenderStateDirty();
}

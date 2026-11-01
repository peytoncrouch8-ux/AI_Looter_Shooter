// UDredgerHoseComponent's look: the feed hose from the tank to the nozzle and the thrown line, as engine cylinders on one
// instanced mesh (one draw), hanging and rippling (DredgerRules::HosePoint); latched, pulses of soul-light drawn back along
// it into the nozzle and a glow where it holds the victim, and the hose itself glowing teal as it drains. Nothing is posed
// far off out of view (the hose-man's frozen spell), and nothing is made outside a game world.

#include "Creatures/DredgerHoseComponent.h"
#include "Combat/BulletSubsystem.h"
#include "Creatures/DredgerHoseMan.h"
#include "Creatures/DredgerLook.h"
#include "Creatures/DredgerRules.h"
#include "Weapons/WeaponFX.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

namespace
{
	/** The feed hose: its pieces, its thickness (cm across / 2) and its hang (cm). */
	constexpr int32 FeedSegments = 7;
	constexpr float FeedRadius = 2.6f;
	constexpr float FeedSlack = 18.f;
	/** The thrown line: its pieces, thickness, hang (a share of its length, within bounds) and ripple (cm). */
	constexpr int32 LineSegments = 14;
	constexpr float LineRadius = 2.2f;
	constexpr float LineSlackShare = 0.05f;
	constexpr float LineSlackLeast = 6.f;
	constexpr float LineSlackMost = 60.f;
	constexpr float LineWaveLatched = 6.f;
	constexpr float LineWaveFlying = 3.f;
	/** Each piece a little longer than its stretch, so the bends never show gaps. */
	constexpr float Overlap = 1.08f;
	/** The pulses drawn in along it: how many, how fast (shares of its length a second), how long each (share), how bright. */
	constexpr int32 Pulses = 3;
	constexpr float PulseSpeed = 1.4f;
	constexpr float PulseLength = 0.06f;
	constexpr float PulseWidth = 6.f;
	constexpr float PulseGlow = 6.f;
	/** The glow where it holds them (cm across, strength, its throb a second). */
	constexpr float HoldGlowSize = 18.f;
	constexpr float HoldGlowStrength = 4.f;
	constexpr float HoldThrob = 6.f;
	/** The hose's own glow as it drains (M_StylizedSurface's Glow at full) and its teal (linear). */
	constexpr float DrainGlow = 2.2f;
	const FLinearColor DrainTeal(0.03f, 0.25f, 0.3f);

	float SlackFor(float Length)
	{
		return FMath::Clamp(Length * LineSlackShare, LineSlackLeast, LineSlackMost);
	}
}

void UDredgerHoseComponent::EnsureLine()
{
	UWorld* World = GetWorld();
	AActor* Owner = GetOwner();
	if (Line || !World || !World->IsGameWorld() || !Owner || !Owner->GetRootComponent())
	{
		return;
	}
	Line = NewObject<UInstancedStaticMeshComponent>(Owner, TEXT("SoulHose"), RF_Transient);
	Line->SetupAttachment(Owner->GetRootComponent());
	Line->SetUsingAbsoluteLocation(true);
	Line->SetUsingAbsoluteRotation(true);
	Line->SetUsingAbsoluteScale(true);
	Line->SetWorldTransform(FTransform::Identity);
	Line->SetMobility(EComponentMobility::Movable);
	Line->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, DredgerLook::CylinderPath));
	if (UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, DredgerLook::SurfacePath))
	{
		LineLook = UMaterialInstanceDynamic::Create(Base, this);
		LineLook->SetVectorParameterValue(TEXT("Color"), DredgerLook::HoseRubber);
		LineLook->SetScalarParameterValue(TEXT("Glow"), 0.f);
		LineLook->SetScalarParameterValue(TEXT("Variation"), 0.1f);
		Line->SetMaterial(0, LineLook);
	}
	Line->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Line->SetCastShadow(false);
	Line->SetCanEverAffectNavigation(false);
	Line->bReceivesDecals = false;
	Line->RegisterComponent();
}

void UDredgerHoseComponent::LayHose(const FVector& A, const FVector& B, int32 Segments, float Radius, float Slack, float Wave,
	TArray<FTransform>& Transforms) const
{
	const float Across = Radius * 2.f / 100.f;
	FVector Previous = DredgerRules::HosePoint(A, B, 0.f, Slack, Wave, Clock);
	for (int32 Index = 1; Index <= Segments; ++Index)
	{
		const FVector Next = DredgerRules::HosePoint(A, B, static_cast<float>(Index) / static_cast<float>(Segments), Slack, Wave, Clock);
		const FVector Along = Next - Previous;
		const float Length = static_cast<float>(Along.Size());
		if (Length > UE_KINDA_SMALL_NUMBER)
		{
			// The engine cylinder stands 100 cm along its up: turned along the piece, stretched to it.
			const FQuat Turn = FRotationMatrix::MakeFromZ(Along / Length).ToQuat();
			Transforms.Emplace(Turn, (Previous + Next) * 0.5f, FVector(Across, Across, Length * Overlap / 100.f));
		}
		Previous = Next;
	}
}

void UDredgerHoseComponent::Draw(float DeltaSeconds)
{
	if (!Line)
	{
		return;
	}
	const ADredgerHoseMan* HoseMan = GetHoseMan();
	const bool bShown = HoseMan && !bTorn && !HoseMan->IsPoseFrozen() && !HoseMan->IsHidden();
	if (!bShown)
	{
		if (bDrawn)
		{
			Line->ClearInstances();
			bDrawn = false;
		}
		return;
	}

	TArray<FTransform> Transforms;
	Transforms.Reserve(FeedSegments + LineSegments);
	LayHose(HoseMan->GetTankFeed(), HoseMan->GetNozzleBack(), FeedSegments, FeedRadius, FeedSlack, 0.f, Transforms);
	const FVector Mouth = HoseMan->GetNozzleMouth();
	const bool bLineOut = State != EDredgerHoseState::Stowed;
	const FVector End = IsLatched() ? VictimChest() : Tip;
	const float Slack = SlackFor(static_cast<float>(FVector::Dist(Mouth, End)));
	const float Wave = IsLatched() ? LineWaveLatched : LineWaveFlying;
	if (bLineOut)
	{
		LayHose(Mouth, End, LineSegments, LineRadius, Slack, Wave, Transforms);
	}

	if (Line->GetInstanceCount() != Transforms.Num())
	{
		Line->ClearInstances();
		Line->AddInstances(Transforms, /*bShouldReturnIndices*/ false, /*bWorldSpace*/ true, /*bUpdateNavigation*/ false);
	}
	else
	{
		Line->BatchUpdateInstancesTransforms(0, Transforms, /*bWorldSpace*/ true, /*bMarkRenderStateDirty*/ false, /*bTeleport*/ true);
	}
	Line->MarkRenderStateDirty();
	bDrawn = true;

	// Draining, the hose glows teal and throbs; at rest it is plain black rubber.
	if (LineLook)
	{
		const float Throb = 0.6f + 0.4f * FMath::Sin(Clock * 8.f);
		LineLook->SetVectorParameterValue(TEXT("Color"), FMath::Lerp(DredgerLook::HoseRubber, DrainTeal, DrainShown));
		LineLook->SetScalarParameterValue(TEXT("Glow"), DrainGlow * DrainShown * Throb);
	}
	if (!IsLatched())
	{
		return;
	}

	// Latched: soul-light drawn in along it from them to him, and a glow where it holds them.
	UBulletSubsystem* Bullets = GetWorld() ? GetWorld()->GetSubsystem<UBulletSubsystem>() : nullptr;
	if (!Bullets)
	{
		return;
	}
	FWeaponFX& Effects = Bullets->GetEffects();
	Effects.Initialize(GetWorld());
	for (int32 Pulse = 0; Pulse < Pulses; ++Pulse)
	{
		const float Along = 1.f - FMath::Frac(Clock * PulseSpeed + static_cast<float>(Pulse) / static_cast<float>(Pulses));
		const float Ahead = FMath::Max(Along - PulseLength, 0.f);
		Effects.AddStreak(DredgerRules::HosePoint(Mouth, End, Along, Slack, Wave, Clock), DredgerRules::HosePoint(Mouth, End, Ahead, Slack, Wave, Clock),
			PulseWidth, DredgerLook::SoulCyan, PulseGlow * DrainShown);
	}
	Effects.AddGlow(End, HoldGlowSize, DredgerLook::SoulCyan, HoldGlowStrength * (0.7f + 0.3f * FMath::Sin(Clock * HoldThrob)));
}

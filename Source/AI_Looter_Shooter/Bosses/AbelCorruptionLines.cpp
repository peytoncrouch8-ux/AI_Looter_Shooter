// UAbelCorruptionComponent: the look of every smog line he has out (the tendrils and the drink line), on one instanced mesh
// of engine cylinders with soul-light pulses drawn toward him. Split from AbelCorruptionTendril.cpp (the tendrils' rules).

#include "Bosses/AbelCorruption.h"
#include "Bosses/AbelKeeper.h"
#include "Combat/BulletSubsystem.h"
#include "Creatures/DredgerLook.h"
#include "Creatures/DredgerRules.h"
#include "Weapons/WeaponFX.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
	/** The lines' look: pieces, thickness (cm across / 2), hang (share, bounds) and ripple. */
	constexpr int32 TendrilSegments = 14;
	constexpr float TendrilRadius = 3.2f;
	constexpr int32 DrinkSegments = 20;
	constexpr float DrinkRadius = 5.f;
	constexpr float SlackShare = 0.07f;
	constexpr float SlackLeast = 10.f;
	constexpr float SlackMost = 140.f;
	constexpr float WaveLatched = 9.f;
	constexpr float WaveFlying = 4.f;
	constexpr float Overlap = 1.08f;
	/** Soul-light drawn along a line toward him: pulses, speed (shares a second), length (share), width, brightness. */
	constexpr int32 Pulses = 4;
	constexpr float PulseSpeed = 1.2f;
	constexpr float PulseLength = 0.05f;
	constexpr float PulseWidth = 7.f;
	constexpr float PulseGlow = 6.f;
	/** The souls drunk off a body are pale gold under the cyan: the dead, not the living. */
	const FLinearColor DrunkSoul(0.9f, 0.85f, 0.6f);
	/** The smog's lines glow a little cyan through the black as they drain. */
	constexpr float LineGlow = 1.4f;
	const FLinearColor LineTeal(0.02f, 0.12f, 0.15f);
	const FLinearColor LineSoot(0.035f, 0.025f, 0.05f);

	float SlackFor(float Length)
	{
		return FMath::Clamp(Length * SlackShare, SlackLeast, SlackMost);
	}


	FWeaponFX* EffectsIn(UWorld* World)
	{
		UBulletSubsystem* Bullets = World ? World->GetSubsystem<UBulletSubsystem>() : nullptr;
		if (!Bullets)
		{
			return nullptr;
		}
		FWeaponFX& Effects = Bullets->GetEffects();
		Effects.Initialize(World);
		return &Effects;
	}

	/** Engine cylinders along a hanging, rippling line from A to B (DredgerRules::HosePoint's shape), into Transforms. */
	void LayLine(const FVector& A, const FVector& B, int32 Segments, float Radius, float Slack, float Wave, float Clock,
		TArray<FTransform>& Transforms)
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
				Transforms.Emplace(FRotationMatrix::MakeFromZ(Along / Length).ToQuat(), (Previous + Next) * 0.5f,
					FVector(Across, Across, Length * Overlap / 100.f));
			}
			Previous = Next;
		}
	}
}

void UAbelCorruptionComponent::DrawLines(float DeltaSeconds)
{
	const AAbelKeeper* Abel = GetAbel();
	UWorld* World = GetWorld();
	if (!Lines || !Abel)
	{
		return;
	}
	TArray<FTransform> Transforms;
	Transforms.Reserve(TendrilSegments * Tendrils.Num() + DrinkSegments);
	const FVector Globe = Abel->GetLanternGlobe();
	bool bDraining = false;
	for (const FTendril& Tendril : Tendrils)
	{
		if (Tendril.State != FTendril::EState::Flying && Tendril.State != FTendril::EState::Latched && Tendril.State != FTendril::EState::Snapping)
		{
			continue;
		}
		const float Slack = SlackFor(static_cast<float>(FVector::Dist(Globe, Tendril.Tip)));
		LayLine(Globe, Tendril.Tip, TendrilSegments, TendrilRadius, Slack, Tendril.State == FTendril::EState::Latched ? WaveLatched : WaveFlying,
			LineClock, Transforms);
		bDraining |= Tendril.State == FTendril::EState::Latched;
	}
	const FVector Body = Drinking != INDEX_NONE ? GetDeadSpot(Drinking) : FVector::ZeroVector;
	if (Drinking != INDEX_NONE)
	{
		LayLine(Globe, Body, DrinkSegments, DrinkRadius, SlackFor(static_cast<float>(FVector::Dist(Globe, Body))), WaveLatched, LineClock, Transforms);
		bDraining = true;
	}
	if (Lines->GetInstanceCount() != Transforms.Num())
	{
		Lines->ClearInstances();
		if (!Transforms.IsEmpty())
		{
			Lines->AddInstances(Transforms, /*bShouldReturnIndices*/ false, /*bWorldSpace*/ true, /*bUpdateNavigation*/ false);
		}
	}
	else if (!Transforms.IsEmpty())
	{
		Lines->BatchUpdateInstancesTransforms(0, Transforms, /*bWorldSpace*/ true, /*bMarkRenderStateDirty*/ true, /*bTeleport*/ true);
	}
	if (LineLook)
	{
		const float Throb = 0.6f + 0.4f * FMath::Sin(LineClock * 8.f);
		LineLook->SetVectorParameterValue(TEXT("Color"), bDraining ? LineTeal : LineSoot);
		LineLook->SetScalarParameterValue(TEXT("Glow"), bDraining ? LineGlow * Throb : 0.f);
	}
	FWeaponFX* Effects = World && World->IsGameWorld() ? EffectsIn(World) : nullptr;
	if (!Effects)
	{
		return;
	}
	// Latched: the player's soul-light drawn in along it to him; a glow where it holds them.
	for (const FTendril& Tendril : Tendrils)
	{
		if (Tendril.State != FTendril::EState::Latched)
		{
			continue;
		}
		const float Slack = SlackFor(static_cast<float>(FVector::Dist(Globe, Tendril.Tip)));
		for (int32 Pulse = 0; Pulse < Pulses; ++Pulse)
		{
			const float Along = 1.f - FMath::Frac(LineClock * PulseSpeed + static_cast<float>(Pulse) / static_cast<float>(Pulses));
			Effects->AddStreak(DredgerRules::HosePoint(Globe, Tendril.Tip, Along, Slack, WaveLatched, LineClock),
				DredgerRules::HosePoint(Globe, Tendril.Tip, FMath::Max(Along - PulseLength, 0.f), Slack, WaveLatched, LineClock),
				PulseWidth, DredgerLook::SoulCyan, PulseGlow);
		}
		Effects->AddGlow(Tendril.Tip, 18.f, DredgerLook::SoulCyan, 4.f * (0.7f + 0.3f * FMath::Sin(LineClock * 6.f)));
	}
	// The drink: the dead's pale souls drawn up the line out of the body into him; its collar throbbing where it bites, hotter
	// with each hit on it (three break it).
	if (Drinking != INDEX_NONE)
	{
		const float Slack = SlackFor(static_cast<float>(FVector::Dist(Globe, Body)));
		for (int32 Pulse = 0; Pulse < Pulses + 2; ++Pulse)
		{
			const float Along = 1.f - FMath::Frac(LineClock * PulseSpeed * 0.8f + static_cast<float>(Pulse) / static_cast<float>(Pulses + 2));
			Effects->AddStreak(DredgerRules::HosePoint(Globe, Body, Along, Slack, WaveLatched, LineClock),
				DredgerRules::HosePoint(Globe, Body, FMath::Max(Along - PulseLength, 0.f), Slack, WaveLatched, LineClock),
				PulseWidth * 1.3f, DrunkSoul, PulseGlow);
		}
		const float Bitten = 1.f + 0.6f * static_cast<float>(CollarHits);
		Effects->AddGlow(Body, 34.f * Bitten, DredgerLook::SoulCyan, 5.f * Bitten * (0.65f + 0.35f * FMath::Sin(LineClock * 7.f)));
	}
}

#include "Elements/ElementStatusComponent.h"
#include "Elements/ElementDeathLook.h"
#include "Elements/ElementRules.h"
#include "Elements/ElementTuning.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Weapons/WeaponFX.h"

// UElementStatusComponent's looks: what a burning, smiting or salted creature shows while it lasts, a smite's burst and
// leap, a kill by a status. All through the bullets' pooled FWeaponFX (no draws of its own, no lights).

namespace
{
	/** Pieces a second of each look on a body of the spider's size (40 cm round): a burn's for each burn, scaled by sqrt. */
	constexpr float EmberRate = 24.f;
	constexpr float SaltRate = 12.f;
	constexpr float SmiteRate = 16.f;
	/** Past this from the eye nobody sees a status's look: none is spawned (cm). */
	constexpr float LookRange = 6000.f;
	/** The pool's particles past which a status's look waits (the shots' own hits come first). */
	constexpr int32 LookParticleBudget = 700;

	FVector CameraOf(const UWorld* World)
	{
		const APlayerController* Player = World ? World->GetFirstPlayerController() : nullptr;
		return Player && Player->PlayerCameraManager ? Player->PlayerCameraManager->GetCameraLocation() : FVector::ZeroVector;
	}

	/** Whole pieces from what's owed (the rest kept for the next frame). */
	int32 TakeWhole(float& Owed)
	{
		const int32 Whole = FMath::FloorToInt32(Owed);
		Owed -= Whole;
		return Whole;
	}
}

void UElementStatusComponent::PlayLooks(float DeltaSeconds)
{
	const FElementTuning Tuning = FElementTuning::Live();
	FWeaponFX* FX = Tuning.LookScale > 0.f ? Effects() : nullptr;
	if (!FX)
	{
		return;
	}
	FVector Center;
	float Radius = 0.f;
	float HalfHeight = 0.f;
	GetBody(Center, Radius, HalfHeight);
	const FVector Camera = CameraOf(GetWorld());
	const bool bSeen = Camera.IsZero() || FVector::DistSquared(Camera, Center) <= FMath::Square(LookRange);
	if (!bSeen || FX->NumParticles() > LookParticleBudget)
	{
		EmberLookOwed = SaltLookOwed = SmiteLookOwed = 0.f;
		return;
	}
	LookTime += DeltaSeconds;
	// The status glows sit on the body's side toward the eye: a glow at its heart would be hidden inside it.
	const FVector Front = Camera.IsZero() ? FVector::ZeroVector : (Camera - Center).GetSafeNormal() * Radius * 1.1f;
	const float BodyScale = FMath::Clamp(Radius / 40.f, 0.5f, 3.f) * Tuning.LookScale;

	if (const int32 Burns = Status.NumBurns())
	{
		EmberLookOwed += EmberRate * FMath::Sqrt(static_cast<float>(Burns)) * BodyScale * DeltaSeconds;
		FX->SpawnElementAura(ELooterElement::Ember, Center, Radius, HalfHeight, TakeWhole(EmberLookOwed), 0.8f + 0.2f * Burns);
		// A low fire-glow on it, flickering, stronger for each burn: reads as burning from across a fight.
		const float Flicker = 0.75f + 0.25f * FMath::PerlinNoise1D(LookTime * 9.f + static_cast<float>(GetUniqueID() % 97));
		FX->AddGlow(Center + Front + FVector(0.f, 0.f, HalfHeight * 0.15f), Radius * 2.2f * Flicker, ElementRules::FxColor(ELooterElement::Ember),
			(1.6f + 0.8f * Burns) * Flicker * Tuning.LookScale);
	}
	if (Status.IsSalted())
	{
		SaltLookOwed += SaltRate * BodyScale * DeltaSeconds;
		FX->SpawnElementAura(ELooterElement::GraveSalt, Center, Radius, HalfHeight, TakeWhole(SaltLookOwed), 1.f);
		// A cold pale sheen round it while it's salted.
		FX->AddGlow(Center + Front + FVector(0.f, 0.f, HalfHeight * 0.1f), Radius * 2.f, ElementRules::FxColor(ELooterElement::GraveSalt), 2.f * Tuning.LookScale);
	}
	if (Status.NumSmites() > 0)
	{
		// The light gathers in faster as the burst nears, and its heart glows brighter, pulsing.
		const float Charge = Status.GetSmiteCharge(Tuning);
		SmiteLookOwed += SmiteRate * (0.5f + Charge) * BodyScale * DeltaSeconds;
		FX->SpawnElementAura(ELooterElement::SoulLight, Center, Radius, HalfHeight, TakeWhole(SmiteLookOwed), 0.8f + 0.6f * Charge);
		const float Pulse = 0.85f + 0.15f * FMath::Sin(LookTime * (10.f + 14.f * Charge));
		const FLinearColor Color = FMath::Lerp(ElementRules::FxColor(ELooterElement::SoulLight), FLinearColor::White, 0.25f * Charge);
		FX->AddGlow(Center + Front + FVector(0.f, 0.f, HalfHeight * 0.35f), Radius * (0.5f + 1.5f * Charge) * Pulse, Color, (5.f + 14.f * Charge) * Tuning.LookScale);
	}
}

void UElementStatusComponent::PlaySmiteLook(const FVector& Where, const FVector* LeapTo) const
{
	FWeaponFX* FX = Effects();
	if (!FX)
	{
		return;
	}
	FVector Center;
	float Radius = 0.f;
	float HalfHeight = 0.f;
	GetBody(Center, Radius, HalfHeight);
	// Its burst on the near side of the body (inside it the body would hide its heart).
	const FVector Camera = CameraOf(GetWorld());
	const FVector Near = Camera.IsZero() ? Where : Where + (Camera - Where).GetSafeNormal() * Radius * 0.9f;
	FX->SpawnElementBurst(ELooterElement::SoulLight, Near, Radius * 2.f, false);
	if (LeapTo)
	{
		FX->SpawnElementArc(ELooterElement::SoulLight, Near, *LeapTo);
		FX->SpawnElementBurst(ELooterElement::SoulLight, *LeapTo, Radius * 1.4f, false);
	}
}

void UElementStatusComponent::PlayKillLook(ELooterElement Element) const
{
	FWeaponFX* FX = Effects();
	if (!FX)
	{
		return;
	}
	FVector Center;
	float Radius = 0.f;
	float HalfHeight = 0.f;
	GetBody(Center, Radius, HalfHeight);
	// The element's own death (ElementDeathLook): the same one a creature's kill look lays over its kind's.
	ElementDeathLook::Play(*FX, Element, Center, Radius, HalfHeight, CameraOf(GetWorld()));
}

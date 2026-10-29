#include "Elements/ElementStatusComponent.h"
#include "Elements/ElementDeathLook.h"
#include "Elements/ElementRules.h"
#include "Elements/ElementTuning.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Weapons/WeaponFX.h"

// UElementStatusComponent's looks: what a burning, smiting, salted, jolted or rotting creature shows while it lasts, a smite's
// and a jolt's burst and leap, a rot's spread, a kill by a status. All through the bullets' pooled FWeaponFX (no draws of its
// own, no lights).

namespace
{
	/** Pieces a second of each look on a body of the spider's size (40 cm round): a burn's for each burn, scaled by sqrt. */
	constexpr float EmberRate = 24.f;
	constexpr float SaltRate = 12.f;
	constexpr float SmiteRate = 16.f;
	/** A jolt waits only a blink (Thunder.Delay, 0.2 s), so its crawling sparks come thick to be seen at all. */
	constexpr float ThunderRate = 40.f;
	/** A rot lasts (Rot.Seconds, 6 s): its drips and bubbles come slowly, more as it grows. */
	constexpr float RotRate = 10.f;
	/** The rot's spread: puffs of sick air rolling from the dead to each one it reaches (unlit smoke, a dim green). */
	const FLinearColor RotSpreadCloud(0.22f, 0.34f, 0.1f);
	constexpr int32 RotSpreadPuffs = 5;
	/** At most this many of the reached are shown the cloud (a crowd shares one death's look). */
	constexpr int32 RotSpreadShown = 6;
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
		EmberLookOwed = SaltLookOwed = SmiteLookOwed = ThunderLookOwed = RotLookOwed = 0.f;
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
	if (Status.NumJolts() > 0)
	{
		// Sparks crawl over it, thicker as the burst nears, and a cold glow stutters on it: lightning never holds steady,
		// so the glow drops near out and snaps back on fast noise rather than pulsing smoothly.
		const float Charge = Status.GetJoltCharge(Tuning);
		ThunderLookOwed += ThunderRate * (0.6f + Charge) * BodyScale * DeltaSeconds;
		FX->SpawnElementAura(ELooterElement::Thunder, Center, Radius, HalfHeight, TakeWhole(ThunderLookOwed), 0.8f + 0.5f * Charge);
		const float Noise = FMath::PerlinNoise1D(LookTime * 38.f + static_cast<float>(GetUniqueID() % 89));
		const float Flicker = 0.3f + 0.7f * FMath::Clamp(Noise * 2.f + 0.5f, 0.f, 1.f);
		const FLinearColor Color = FMath::Lerp(ElementRules::FxColor(ELooterElement::Thunder), FLinearColor::White, 0.3f);
		FX->AddGlow(Center + Front + FVector(0.f, 0.f, HalfHeight * 0.25f), Radius * (1.4f + 0.6f * Charge) * Flicker, Color,
			(3.f + 6.f * Charge) * Flicker * Tuning.LookScale);
	}
	if (Status.IsRotting())
	{
		// Drips falling off it and slow bubbles, more as the rot grows (its damage grows with it), under a dim green glow
		// that swells slowly: a sickness, not a fire.
		const float Growth = Status.GetRotGrowth();
		RotLookOwed += RotRate * (0.5f + Growth) * BodyScale * DeltaSeconds;
		FX->SpawnElementAura(ELooterElement::Rot, Center, Radius, HalfHeight, TakeWhole(RotLookOwed), 0.8f + 0.5f * Growth);
		const float Swell = 0.85f + 0.15f * FMath::Sin(LookTime * 2.5f + static_cast<float>(GetUniqueID() % 53));
		FX->AddGlow(Center + Front + FVector(0.f, 0.f, HalfHeight * 0.05f), Radius * 2.f * Swell, ElementRules::FxColor(ELooterElement::Rot),
			(1.f + 1.2f * Growth) * Swell * Tuning.LookScale);
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

void UElementStatusComponent::PlayJoltLook(const FVector& Where, const FVector* LeapTo) const
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
	// The burst on the near side of the body, as a smite's: a hard flash with sparks flung out and forks jumping off it, and
	// a last crackle over the whole body as the charge leaves it.
	const FVector Camera = CameraOf(GetWorld());
	const FVector Near = Camera.IsZero() ? Where : Where + (Camera - Where).GetSafeNormal() * Radius * 0.9f;
	FX->SpawnElementBurst(ELooterElement::Thunder, Near, Radius * 2.f, false);
	FX->SpawnElementAura(ELooterElement::Thunder, Center, Radius, HalfHeight, 4, 1.4f);
	if (LeapTo)
	{
		// The arc to the next living thing: a jagged line of beads, and a smaller strike where it lands.
		FX->SpawnElementArc(ELooterElement::Thunder, Near, *LeapTo);
		FX->SpawnElementBurst(ELooterElement::Thunder, *LeapTo, Radius * 1.2f, false);
	}
}

void UElementStatusComponent::PlayRotSpreadLook(const FVector& From, TConstArrayView<FVector> To) const
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
	// The dead one goes in a wet green burst...
	FX->SpawnElementBurst(ELooterElement::Rot, From, FMath::Max(Radius * 2.f, 60.f), false);
	// ...and a cloud rolls from it to each one it reaches: puffs launched one behind another so they string out along the
	// way, the lead puff fast enough to reach the target before it thins (smoke drag 2.8/s: it covers speed / 2.8 cm in all,
	// so speed = distance x 3 gets there in about a second). Where it lands, the rot's drips and bubbles start at once.
	const int32 Shown = FMath::Min(To.Num(), RotSpreadShown);
	for (int32 Target = 0; Target < Shown; ++Target)
	{
		const FVector Way = To[Target] - From;
		const float Distance = Way.Size();
		if (Distance < 1.f)
		{
			continue;
		}
		for (int32 Puff = 0; Puff < RotSpreadPuffs; ++Puff)
		{
			const float Share = 1.f - static_cast<float>(Puff) / RotSpreadPuffs * 0.6f;
			const FVector Speed = Way * (3.f * Share) + FVector(0.f, 0.f, 15.f);
			const float Size = FMath::Clamp(Distance * 0.12f, 18.f, 40.f);
			FX->SpawnDustPuff(From + Way.GetSafeNormal() * Radius * 0.5f, Speed, RotSpreadCloud, 0.4f, Size, Size * 2.6f, 1.1f + 0.1f * Puff);
		}
		FX->SpawnElementAura(ELooterElement::Rot, To[Target], 35.f, 45.f, 5, 1.2f);
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

#include "Weapons/VfxLightRules.h"
#include "Weapons/VfxRules.h"
#include "Weapons/VfxTuning.h"

// The shot light's and the impact flash's plain rules and the pool's bookkeeping (VfxLightRules.h). The audit (2026-11-03,
// change 8): the muzzle light was 24 cd / 3.2 m, invisible in daylight, and a hit threw no light at all. These give the numbers
// their reasons; AWeaponBase (WeaponBaseMuzzle.cpp) hangs the shot light and UImpactFlashSubsystem lights the hits.

namespace
{
	/** The impact light's colours: sparks off stone and metal are gold-white, a timber splinter's flash a deeper orange. */
	const FLinearColor StoneFlash(1.f, 0.78f, 0.5f);
	const FLinearColor MetalFlash(1.f, 0.85f, 0.6f);
	const FLinearColor WoodFlash(1.f, 0.62f, 0.3f);
}

namespace VfxRules
{
	float MuzzleLightKindShare(EWeaponKind Kind, const FVfxTuning& Tuning)
	{
		switch (Kind)
		{
		case EWeaponKind::Shotgun: return Tuning.MuzzleLightShotgun;
		case EWeaponKind::Revolver: return Tuning.MuzzleLightRevolver;
		case EWeaponKind::SMG: return Tuning.MuzzleLightSMG;
		case EWeaponKind::Sniper: return Tuning.MuzzleLightSniper;
		case EWeaponKind::LMG: return Tuning.MuzzleLightLMG;
		case EWeaponKind::Launcher: return Tuning.MuzzleLightLauncher;
		case EWeaponKind::Rifle:
		case EWeaponKind::None:
		default:
			return Tuning.MuzzleLightRifle;
		}
	}

	FVfxLightShape MuzzleLightShape(EWeaponKind Kind, bool bFirstPerson, const FVfxTuning& Tuning)
	{
		FVfxLightShape Shape;
		const float Share = MuzzleLightKindShare(Kind, Tuning);
		if (Tuning.MuzzleLight <= 0.f || Share <= 0.f || Tuning.MuzzleLightRadius <= 0.f)
		{
			return Shape;
		}
		Shape.Candelas = Tuning.MuzzleLight * Share * (bFirstPerson ? Tuning.MuzzleLight1P : 1.f);
		// Strength falls with the square of the distance, so the root of the share keeps every kind's glow edge alike (a light
		// twice as strong reaches 1.41x as far, not twice).
		Shape.RadiusCm = Tuning.MuzzleLightRadius * FMath::Sqrt(Share);
		return Shape;
	}

	float MuzzleLightLife(float FlashSeconds, const FVfxTuning& Tuning)
	{
		return FMath::Min(FMath::Max(Tuning.MuzzleLightSeconds, 0.f), FMath::Max(FlashSeconds, 0.f));
	}

	FVector MuzzleLightPlace(const FVector& Muzzle, const FVector& Barrel, float PushCm)
	{
		return Muzzle + Barrel.GetSafeNormal() * FMath::Max(PushCm, 0.f);
	}

	float ImpactLightSurface(EImpactMaterial Material)
	{
		switch (Material)
		{
		case EImpactMaterial::Stone:
		case EImpactMaterial::Metal:
			return 1.f;
		case EImpactMaterial::SheetMetal:
			return 0.8f;
		case EImpactMaterial::Wood:
			// Timber doesn't spark: a dimmer flash of splinters and dust.
			return 0.55f;
		default:
			return 0.f;
		}
	}

	FLinearColor ImpactLightColor(EImpactMaterial Material)
	{
		switch (Material)
		{
		case EImpactMaterial::Stone: return StoneFlash;
		case EImpactMaterial::Metal:
		case EImpactMaterial::SheetMetal: return MetalFlash;
		case EImpactMaterial::Wood: return WoodFlash;
		default: return FLinearColor::White;
		}
	}

	float ImpactLightEyeShare(float EyeDistance, const FVfxTuning& Tuning)
	{
		if (EyeDistance < 0.f || Tuning.ImpactLightEye <= 0.f)
		{
			return 1.f;
		}
		return FMath::Clamp((EyeDistance - Tuning.ImpactLightEye) / Tuning.ImpactLightEye, 0.f, 1.f);
	}

	FVfxLightShape ImpactLightShape(EImpactMaterial Material, float Weight, float EyeDistance, const FVfxTuning& Tuning)
	{
		FVfxLightShape Shape;
		const float Surface = ImpactLightSurface(Material);
		if (Tuning.ImpactLight <= 0.f || Tuning.ImpactLightMax <= 0 || Surface <= 0.f)
		{
			return Shape;
		}
		// A hit far off is a few pixels of wall: not worth a light (an unknown eye, a negative distance, is never far).
		if (Tuning.ImpactLightFar > 0.f && EyeDistance > Tuning.ImpactLightFar)
		{
			return Shape;
		}
		const float Eye = ImpactLightEyeShare(EyeDistance, Tuning);
		if (Eye <= 0.f)
		{
			return Shape;
		}
		const float Heft = FMath::Clamp(Weight / ImpactLightWeightRef, ImpactLightHeftLeast, ImpactLightHeftMost);
		Shape.Candelas = Tuning.ImpactLight * Surface * Heft * Eye;
		Shape.RadiusCm = Tuning.ImpactLightRadius;
		Shape.Color = ImpactLightColor(Material);
		return Shape;
	}
}

// ---------------------------------------------------------------------------
// The impact flash lights' pool
// ---------------------------------------------------------------------------

int32 FImpactFlashPool::NumLive() const
{
	int32 Live = 0;
	for (const FImpactFlashSlot& Slot : Slots)
	{
		Live += Slot.bLive ? 1 : 0;
	}
	return Live;
}

float FImpactFlashPool::StrengthOf(int32 Index) const
{
	if (Index < 0 || Index >= MaxSlots || !Slots[Index].bLive)
	{
		return 0.f;
	}
	const FImpactFlashSlot& Slot = Slots[Index];
	return Slot.Candelas * VfxRules::MuzzleFade(Slot.Age, Slot.Life, VfxRules::ImpactLightFadePower);
}

int32 FImpactFlashPool::Start(const FVector& Place, const FVfxLightShape& Shape, float LifeSeconds, int32 MaxLit, float MergeReach, uint64 Frame)
{
	const int32 Allowed = FMath::Clamp(MaxLit, 0, MaxSlots);
	if (Allowed == 0 || Shape.Candelas <= 0.f || LifeSeconds <= 0.f)
	{
		return INDEX_NONE;
	}
	auto Fill = [&](int32 Index)
	{
		FImpactFlashSlot& Slot = Slots[Index];
		Slot.bLive = true;
		Slot.Place = Place;
		Slot.Color = Shape.Color;
		Slot.Candelas = Shape.Candelas;
		Slot.RadiusCm = Shape.RadiusCm;
		Slot.Age = 0.f;
		Slot.Life = LifeSeconds;
		Slot.StartFrame = Frame;
		return Index;
	};

	// Beside a live flash: that light takes the hit over (restarting full) unless it is still brighter than the hit.
	if (MergeReach > 0.f)
	{
		const float ReachSquared = FMath::Square(MergeReach);
		for (int32 Index = 0; Index < MaxSlots; ++Index)
		{
			if (Slots[Index].bLive && FVector::DistSquared(Slots[Index].Place, Place) <= ReachSquared)
			{
				return Shape.Candelas < StrengthOf(Index) ? INDEX_NONE : Fill(Index);
			}
		}
	}
	// A free light.
	if (NumLive() < Allowed)
	{
		for (int32 Index = 0; Index < MaxSlots; ++Index)
		{
			if (!Slots[Index].bLive)
			{
				return Fill(Index);
			}
		}
	}
	// All lit: a brighter hit takes over the weakest, a dimmer one is dropped.
	int32 Weakest = INDEX_NONE;
	float Least = TNumericLimits<float>::Max();
	for (int32 Index = 0; Index < MaxSlots; ++Index)
	{
		const float Strength = StrengthOf(Index);
		if (Slots[Index].bLive && Strength < Least)
		{
			Weakest = Index;
			Least = Strength;
		}
	}
	return Weakest != INDEX_NONE && Shape.Candelas > Least ? Fill(Weakest) : INDEX_NONE;
}

uint32 FImpactFlashPool::Advance(float DeltaSeconds, uint64 Frame)
{
	uint32 Ended = 0;
	for (int32 Index = 0; Index < MaxSlots; ++Index)
	{
		FImpactFlashSlot& Slot = Slots[Index];
		if (!Slot.bLive)
		{
			continue;
		}
		if (Slot.StartFrame != Frame)
		{
			Slot.Age += FMath::Max(DeltaSeconds, 0.f);
		}
		if (Slot.Age >= Slot.Life)
		{
			Slot.bLive = false;
			Ended |= 1u << Index;
		}
	}
	return Ended;
}

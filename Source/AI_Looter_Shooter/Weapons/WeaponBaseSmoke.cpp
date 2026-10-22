#include "Weapons/WeaponBase.h"
#include "Combat/BulletSubsystem.h"
#include "Weapons/WeaponDefinition.h"
#include "Weapons/WeaponFX.h"
#include "Weapons/WeaponModelComponent.h"
#include "Engine/World.h"

// AWeaponBase's hot muzzle: every shot heats the barrel and it cools by itself; when firing stops with the barrel hot
// (a long burst, a shotgun blast, a six-gun's heavy round) a thin wisp of smoke curls up off the muzzle for a moment or
// two (FWeaponFX smoke quads, drawn with the shots' effects). The gun ticks only while it waits to look and while it smokes.
//
// No heat shimmer: a refraction pass over the barrel would cost a distortion pass on a frame already render-thread bound.

namespace
{
	/**
	 * Heat a shot puts in the barrel, by kind: a rifle's round a little (about a second's burst smokes, a long one well), a
	 * pump's blast or a six-gun's heavy round enough alone. The SMG's small rounds less than a rifle's (a long burst still
	 * smokes), the LMG's about a rifle's (it fires long), the sniper's big round alone, the launcher's mouth most of all.
	 */
	float SmokeShotHeat(EWeaponKind Kind)
	{
		switch (Kind)
		{
		case EWeaponKind::Shotgun:
			return 0.55f;
		case EWeaponKind::Revolver:
			return 0.45f;
		case EWeaponKind::SMG:
			return 0.06f;
		case EWeaponKind::LMG:
			return 0.08f;
		case EWeaponKind::Sniper:
			return 0.6f;
		case EWeaponKind::Launcher:
			return 1.4f;
		case EWeaponKind::Rifle:
		case EWeaponKind::None:
		default:
			return 0.09f;
		}
	}

	/** The barrel cools this much a second, and holds no more than SmokeMostHeat. */
	constexpr float SmokeCooling = 0.5f;
	constexpr float SmokeMostHeat = 1.5f;
	/** Smokes only this hot or hotter, this long after the last shot (the flash's own puff has gone by then). */
	constexpr float SmokeLeastHeat = 0.3f;
	constexpr float SmokeSettleSeconds = 0.15f;
	/** The wisp lasts between these (hotter: longer), thickest at first. */
	constexpr float SmokeShortest = 0.9f;
	constexpr float SmokeLongest = 2.6f;
	constexpr float SmokeOpacity = 0.2f;
	/** A puff this often while it smokes. */
	constexpr float SmokePuffEvery = 0.05f;
	/** How it rises and curls (cm/s), how big a puff starts and spreads to (cm), how long one hangs (seconds). */
	constexpr float SmokeMinRise = 16.f;
	constexpr float SmokeMaxRise = 26.f;
	constexpr float SmokeCurl = 10.f;
	constexpr float SmokeStartSize = 1.8f;
	constexpr float SmokeMinEndSize = 9.f;
	constexpr float SmokeMaxEndSize = 15.f;
	constexpr float SmokeMinLife = 1.1f;
	constexpr float SmokeMaxLife = 1.7f;
}

float AWeaponBase::GetMuzzleHeat() const
{
	const UWorld* World = GetWorld();
	const double Now = World ? World->GetTimeSeconds() : HeatTime;
	return FMath::Max(MuzzleHeat - static_cast<float>(Now - HeatTime) * SmokeCooling, 0.f);
}

void AWeaponBase::NoteShotHeat()
{
	const UWorld* World = GetWorld();
	if (!World || !Instance.Definition)
	{
		return;
	}
	MuzzleHeat = FMath::Min(GetMuzzleHeat() + SmokeShotHeat(Instance.Definition->Kind), SmokeMostHeat);
	HeatTime = World->GetTimeSeconds();
	// A shot through the smoke clears it (the flash's own puff takes over); once firing stops, the gun looks again.
	SmokeTimeLeft = 0.f;
	WispCheckSeconds = SmokeSettleSeconds;
	RefreshTick();
}

void AWeaponBase::StopMuzzleSmoke()
{
	WispCheckSeconds = -1.f;
	SmokeTimeLeft = 0.f;
}

void AWeaponBase::UpdateMuzzleSmoke(float DeltaSeconds)
{
	if (WispCheckSeconds >= 0.f)
	{
		WispCheckSeconds -= DeltaSeconds;
		if (WispCheckSeconds < 0.f)
		{
			WispCheckSeconds = -1.f;
			const float Heat = GetMuzzleHeat();
			if (Heat >= SmokeLeastHeat)
			{
				const float Hot = FMath::Clamp((Heat - SmokeLeastHeat) / (SmokeMostHeat - SmokeLeastHeat), 0.f, 1.f);
				SmokeLength = FMath::Lerp(SmokeShortest, SmokeLongest, Hot);
				SmokeTimeLeft = SmokeLength;
				SmokeStrength = FMath::Clamp(Heat, 0.4f, 1.2f);
				SmokePuffIn = 0.f;
			}
		}
	}
	if (SmokeTimeLeft <= 0.f)
	{
		return;
	}
	SmokeTimeLeft -= DeltaSeconds;
	SmokePuffIn -= DeltaSeconds;
	UWorld* World = GetWorld();
	UBulletSubsystem* Bullets = World ? World->GetSubsystem<UBulletSubsystem>() : nullptr;
	if (SmokePuffIn > 0.f || !Bullets || IsHidden())
	{
		return;
	}
	SmokePuffIn = SmokePuffEvery;
	// From the muzzle the player sees: rising slowly, swaying side to side and a little fore and aft as it goes, so the puffs
	// one after another make a curling thread; thinning as the barrel cools.
	const FTransform& Gun = bUsingModel ? Model->GetComponentTransform() : GetActorTransform();
	const FVector Along = Gun.GetUnitAxis(EAxis::X);
	const FVector Side = Gun.GetUnitAxis(EAxis::Y);
	const float Age = SmokeLength - SmokeTimeLeft;
	const float Fade = FMath::Clamp(SmokeTimeLeft / FMath::Max(SmokeLength, 0.01f), 0.f, 1.f);
	const FVector Velocity = FVector::UpVector * FMath::FRandRange(SmokeMinRise, SmokeMaxRise)
		+ Side * (FMath::Sin(Age * 9.f) * SmokeCurl) + Along * (4.f + FMath::Cos(Age * 6.f) * 4.f);
	FWeaponFX& Effects = Bullets->GetEffects();
	Effects.Initialize(World);
	Effects.SpawnWisp(GetVisibleMuzzleLocation() + Along * 1.5f, Velocity, SmokeOpacity * SmokeStrength * (0.3f + 0.7f * Fade), SmokeStartSize,
		FMath::FRandRange(SmokeMinEndSize, SmokeMaxEndSize), FMath::FRandRange(SmokeMinLife, SmokeMaxLife));
}

#include "Embers/Powers/EmberVfx.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/CreatureReactSubsystem.h"
#include "Weapons/CreatureBlood.h"
#include "Weapons/VfxTuning.h"
#include "Weapons/WeaponFX.h"

// EmberVfx: the ember powers' shared punch (the VFX pass): activation rings, blows on bodies, strikes, blasts, columns.

namespace
{
	/** The white-hot heart the powers' colours are mixed toward. */
	const FLinearColor EmberWhite(1.f, 0.96f, 0.9f);
}

namespace EmberVfx
{
	float BlowScale(float Damage)
	{
		return FMath::Clamp(Damage / 20.f, 0.6f, 1.6f);
	}

	void Activation(FWeaponFX& FX, const FVector& Feet, const FLinearColor& Color, float Strength)
	{
		const FVfxTuning Vfx = FVfxTuning::Live();
		const float Power = Vfx.PowerScale * FMath::Max(Strength, 0.f);
		if (Power <= 0.f)
		{
			return;
		}
		const float Glow = Vfx.Glow * Power;
		// The ring racing out over the ground from the feet, a second slower and fainter behind it.
		FX.SpawnShockRing(Feet + FVector(0.0, 0.0, 4.0), FVector::UpVector, 30.f, 340.f * Power, 14.f, FMath::Lerp(Color, EmberWhite, 0.25f), 16.f * Glow,
			0.45f);
		FX.SpawnShockRing(Feet + FVector(0.0, 0.0, 6.0), FVector::UpVector, 20.f, 220.f * Power, 22.f, Color, 8.f * Glow, 0.7f);
		// Motes of it rising round the user (kept off the eye: they start low and wide).
		FRandomStream Random(static_cast<int32>(GetTypeHash(Feet)));
		const int32 Motes = FMath::RoundToInt32(10.f * FMath::Min(Power, 1.5f));
		for (int32 Index = 0; Index < Motes; ++Index)
		{
			const float Angle = UE_TWO_PI * (static_cast<float>(Index) + Random.FRandRange(-0.3f, 0.3f)) / FMath::Max(Motes, 1);
			const FVector Out(FMath::Cos(Angle), FMath::Sin(Angle), 0.f);
			FX.SpawnFireball(Feet + Out * Random.FRandRange(60.f, 110.f) + FVector(0.0, 0.0, 10.0), Out * 40.f + FVector(0.0, 0.0, Random.FRandRange(120.f, 220.f)),
				Random.FRandRange(6.f, 10.f), 2.f, Color, 4.f * Glow, Random.FRandRange(0.7f, 1.1f));
		}
	}

	void BodyHit(FWeaponFX& FX, const AActor& Target, const FVector& Where, const FVector& Way, float Damage, const FLinearColor& Color)
	{
		const FVfxTuning Vfx = FVfxTuning::Live();
		if (Vfx.PowerScale <= 0.f || !Target.IsA<ACreatureBase>())
		{
			return;
		}
		const ECreatureBlood Blood = CreatureBlood::Of(&Target);
		if (Blood == ECreatureBlood::None)
		{
			return;
		}
		const FVector Along = Way.IsNearlyZero() ? FVector::DownVector : Way.GetSafeNormal();
		const float Scale = BlowScale(Damage);
		// What it spills, as a gun's round of the blow's weight would (a crit's flare is the guns' own: never here).
		FX.SpawnCreatureHit(Where, -Along, Along, Blood, CreatureBlood::TintOf(&Target), Scale, false);
		// The power's own pop on top: a starburst of its colour.
		FX.SpawnStarburst(Where - Along * 4.f, 34.f * Scale * Vfx.PowerScale, FMath::Lerp(Color, EmberWhite, 0.3f), 18.f * Vfx.Glow, 0.07f, 8);
	}

	void Strike(FWeaponFX& FX, const FVector& Where, const FLinearColor& Color, float Size)
	{
		const FVfxTuning Vfx = FVfxTuning::Live();
		const float Power = Vfx.PowerScale * FMath::Max(Size, 0.f);
		if (Power <= 0.f)
		{
			return;
		}
		FX.SpawnStarburst(Where, 60.f * Power, FMath::Lerp(Color, EmberWhite, 0.35f), 22.f * Vfx.Glow, 0.09f, 10);
		FX.SpawnShockRing(Where, FVector::ZeroVector, 8.f * Power, 80.f * Power, 5.f, Color, 14.f * Vfx.Glow, 0.2f, true);
	}

	void Blast(FWeaponFX& FX, const UObject* WorldContext, const FVector& Center, const FVector& Up, float Radius, const FLinearColor& Color,
		bool bSoul, EImpactMaterial Ground)
	{
		const FVfxTuning Vfx = FVfxTuning::Live();
		if (Vfx.PowerScale <= 0.f)
		{
			return;
		}
		FVfxExplosion Burst;
		Burst.Center = Center;
		Burst.Up = Up;
		Burst.Radius = Radius * Vfx.PowerScale;
		Burst.Fire = Color;
		Burst.bSoul = bSoul;
		Burst.Ground = Ground;
		Burst.bOnGround = Ground != EImpactMaterial::Unknown;
		UCreatureReactSubsystem* Reacts = WorldContext ? UCreatureReactSubsystem::Get(WorldContext) : nullptr;
		Burst.DustLight = Reacts ? Reacts->GetDustLight() : FLinearColor::White;
		FX.SpawnExplosion(Burst);
	}

	void Column(FWeaponFX& FX, const FVector& Where, float Height, const FLinearColor& Color)
	{
		const FVfxTuning Vfx = FVfxTuning::Live();
		if (Vfx.PowerScale <= 0.f)
		{
			return;
		}
		// The shaft: a fireball of light stretched down out of the sky is beyond the shapes, so a stack of them falls fast
		// onto the spot (each blooming and gone in a blink), and a ring where it lands.
		const int32 Steps = FMath::Clamp(FMath::RoundToInt32(Height / 60.f), 4, 14);
		for (int32 Step = 0; Step < Steps; ++Step)
		{
			const float Up = Height * static_cast<float>(Step) / Steps;
			FX.SpawnFireball(Where + FVector(0.0, 0.0, Up), FVector(0.0, 0.0, -2400.0), 26.f, 44.f, FMath::Lerp(Color, EmberWhite, 0.4f), 3.f * Vfx.Glow,
				0.12f + 0.01f * Step);
		}
		FX.SpawnStarburst(Where, 120.f, EmberWhite, 26.f * Vfx.Glow, 0.12f, 12);
		FX.SpawnShockRing(Where + FVector(0.0, 0.0, 4.0), FVector::UpVector, 20.f, 260.f, 12.f, Color, 16.f * Vfx.Glow, 0.4f);
	}
}

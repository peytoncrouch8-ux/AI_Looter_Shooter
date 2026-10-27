#include "Combat/GrenadeKinds.h"
#include "Combat/CombatRules.h"
#include "UI/Style/LooterUIStyle.h"

#define LOCTEXT_NAMESPACE "GrenadeKinds"

namespace
{
	/**
	 * A jar that bursts more than once splits a boss's cap between its bursts: the scatter's own burst and its brood take
	 * half each (the brood's half shared among them), the cinder's burst and its bed half each. So no kind takes more of
	 * a boss's bar than the salt jar's 8%.
	 */
	constexpr float SplitCapShare = 0.5f;

	/** The fires: the salt's hot pale fire (as before), pitch's sootier orange, the scatter's paler crack, the coals' deep ember. */
	const FLinearColor SaltFire(1.f, 0.55f, 0.22f);
	const FLinearColor PitchFire(1.f, 0.45f, 0.14f);
	const FLinearColor ScatterFire(1.f, 0.6f, 0.28f);
	const FLinearColor CinderFire(1.f, 0.36f, 0.08f);
	/** The pitch's tar: near black with a warm cast (unlit globs read as tar, not soot). */
	const FLinearColor TarTint(0.035f, 0.028f, 0.02f);

	FGraveSaltTuning Scaled(const FGraveSaltTuning& Base, float Damage, float Radius, float Core)
	{
		FGraveSaltTuning Made = Base;
		Made.BaseDamage = Base.BaseDamage * FMath::Max(Damage, 0.f);
		Made.Radius = FMath::Max(Base.Radius * Radius, 50.f);
		// The core never past the reach: the falloff divides by their gap.
		Made.CoreRadius = FMath::Clamp(Base.CoreRadius * Core, 0.f, Made.Radius - 1.f);
		return Made;
	}
}

bool GrenadeKinds::IsSpecial(EGrenadeKind Kind)
{
	return Kind != EGrenadeKind::Salt;
}

EGrenadeKind GrenadeKinds::FromSaved(uint8 Saved)
{
	return Saved < Num ? static_cast<EGrenadeKind>(Saved) : EGrenadeKind::Salt;
}

FText GrenadeKinds::Name(EGrenadeKind Kind)
{
	switch (Kind)
	{
	case EGrenadeKind::Pitch: return LOCTEXT("PitchName", "Pitch jar");
	case EGrenadeKind::Scatter: return LOCTEXT("ScatterName", "Scatter jar");
	case EGrenadeKind::Cinder: return LOCTEXT("CinderName", "Cinder jar");
	default: return LOCTEXT("SaltName", "Grave salt");
	}
}

FText GrenadeKinds::Tag(EGrenadeKind Kind)
{
	switch (Kind)
	{
	case EGrenadeKind::Pitch: return LOCTEXT("PitchTag", "PITCH");
	case EGrenadeKind::Scatter: return LOCTEXT("ScatterTag", "SCATTER");
	case EGrenadeKind::Cinder: return LOCTEXT("CinderTag", "CINDER");
	default: return LOCTEXT("SaltTag", "SALT");
	}
}

FText GrenadeKinds::Blurb(EGrenadeKind Kind)
{
	switch (Kind)
	{
	case EGrenadeKind::Pitch: return LOCTEXT("PitchBlurb", "Sticks where it lands; tars what it doesn't kill");
	case EGrenadeKind::Scatter: return LOCTEXT("ScatterBlurb", "Flings four small jars that pop round it");
	case EGrenadeKind::Cinder: return LOCTEXT("CinderBlurb", "Leaves a bed of burning cinders");
	default: return LOCTEXT("SaltBlurb", "A wide burst; salt burns the dead");
	}
}

FLinearColor GrenadeKinds::UiColor(EGrenadeKind Kind)
{
	switch (Kind)
	{
	case EGrenadeKind::Pitch: return LooterUI::Color::AccentLight();
	case EGrenadeKind::Scatter: return LooterUI::Color::Better();
	case EGrenadeKind::Cinder: return LooterUI::Color::ElementEmber();
	default: return LooterUI::Color::ElementGraveSalt();
	}
}

const TCHAR* GrenadeKinds::MeshPath(EGrenadeKind Kind)
{
	switch (Kind)
	{
	case EGrenadeKind::Pitch: return TEXT("/Game/Art/Throwables/SM_PitchJar.SM_PitchJar");
	case EGrenadeKind::Scatter: return TEXT("/Game/Art/Throwables/SM_ScatterJar.SM_ScatterJar");
	case EGrenadeKind::Cinder: return TEXT("/Game/Art/Throwables/SM_CinderJar.SM_CinderJar");
	default: return TEXT("/Game/Art/Throwables/SM_SaltGrenade.SM_SaltGrenade");
	}
}

FGraveSaltTuning GrenadeKinds::BurstTuning(EGrenadeKind Kind, const FGraveSaltTuning& Base, const FGrenadeKindTuning& Kinds)
{
	switch (Kind)
	{
	case EGrenadeKind::Pitch:
	{
		FGraveSaltTuning Made = Scaled(Base, Kinds.PitchDamage, Kinds.PitchRadius, Kinds.PitchCore);
		// The tar holds them where they stand; a knock would throw them out of it.
		Made.KnockShare = 0.f;
		return Made;
	}
	case EGrenadeKind::Scatter:
	{
		FGraveSaltTuning Made = Scaled(Base, Kinds.ScatterDamage, Kinds.ScatterRadius, Kinds.ScatterRadius);
		Made.BossShareCap = Base.BossShareCap * SplitCapShare;
		return Made;
	}
	case EGrenadeKind::Cinder:
	{
		FGraveSaltTuning Made = Scaled(Base, Kinds.CinderDamage, Kinds.CinderRadius, Kinds.CinderRadius);
		Made.BossShareCap = Base.BossShareCap * SplitCapShare;
		// They stay in the fire.
		Made.KnockShare = 0.f;
		return Made;
	}
	default:
		return Base;
	}
}

FGraveSaltTuning GrenadeKinds::BroodTuning(const FGraveSaltTuning& Base, const FGrenadeKindTuning& Kinds)
{
	FGraveSaltTuning Made = Scaled(Base, Kinds.BroodDamage, Kinds.BroodRadius, Kinds.BroodRadius);
	Made.BossShareCap = Base.BossShareCap * SplitCapShare / static_cast<float>(FMath::Max(Kinds.ScatterCount, 1));
	// Four little knocks would bat a body about between them.
	Made.KnockShare = 0.f;
	return Made;
}

float GrenadeKinds::BedBossCap(const FGraveSaltTuning& Base)
{
	return Base.BossShareCap * SplitCapShare;
}

FGrenadeBurstLook GrenadeKinds::LookOf(EGrenadeKind Kind, bool bBrood)
{
	FGrenadeBurstLook Look;
	if (bBrood)
	{
		// A small jar: a sharp pop, a puff of salt, a little scorch; its own crack, not the big blast's sound.
		Look.Fire = ScatterFire;
		Look.Salt = 0.4f;
		Look.bSmall = true;
		Look.Scorch = 0.5f;
		Look.Flash = 0.3f;
		Look.Kick = 0.35f;
		Look.SaltVolume = 0.f;
		Look.LayerCue = GrenadeKindCue::ScatterPop;
		return Look;
	}
	switch (Kind)
	{
	case EGrenadeKind::Pitch:
		// Sootier fire, less salt, black globs of tar flung out and a tar pool left glossy on the ground.
		Look.Fire = PitchFire;
		Look.Salt = 0.5f;
		Look.GlobTint = TarTint;
		Look.GlobSize = 220.f;
		Look.Scorch = 1.3f;
		Look.SaltVolume = 0.85f;
		Look.LayerCue = GrenadeKindCue::PitchBurst;
		break;
	case EGrenadeKind::Scatter:
		// A smaller, paler crack: the brood flying out of it is the show.
		Look.Fire = ScatterFire;
		Look.Salt = 0.7f;
		Look.Scorch = 0.7f;
		Look.Flash = 0.8f;
		Look.Kick = 0.8f;
		Look.SaltVolume = 0.75f;
		Look.LayerCue = GrenadeKindCue::ScatterBurst;
		break;
	case EGrenadeKind::Cinder:
		// Deep ember fire and a burst of embers; the bed burns on after it.
		Look.Fire = CinderFire;
		Look.Salt = 0.5f;
		Look.Element = ELooterElement::Ember;
		Look.Scorch = 1.1f;
		Look.Flash = 1.1f;
		Look.Kick = 0.9f;
		Look.SaltVolume = 0.8f;
		Look.LayerCue = GrenadeKindCue::CinderBurst;
		break;
	default:
		Look.Fire = SaltFire;
		break;
	}
	return Look;
}

EGrenadeKind GrenadeKinds::RollDrop(int32 PlayerLevel, float RollSpecial, float RollWhich, const FGrenadeKindTuning& Kinds)
{
	if (PlayerLevel < Kinds.FromLevel || RollSpecial >= Kinds.SpecialShare)
	{
		return EGrenadeKind::Salt;
	}
	const float Weights[3] = { FMath::Max(Kinds.PitchWeight, 0.f), FMath::Max(Kinds.ScatterWeight, 0.f), FMath::Max(Kinds.CinderWeight, 0.f) };
	const float Total = Weights[0] + Weights[1] + Weights[2];
	if (Total <= 0.f)
	{
		return EGrenadeKind::Salt;
	}
	float Left = FMath::Clamp(RollWhich, 0.f, 0.9999f) * Total;
	const EGrenadeKind Kinds3[3] = { EGrenadeKind::Pitch, EGrenadeKind::Scatter, EGrenadeKind::Cinder };
	for (int32 Index = 0; Index < 3; ++Index)
	{
		if (Weights[Index] > 0.f && Left < Weights[Index])
		{
			return Kinds3[Index];
		}
		Left -= Weights[Index];
	}
	// Rounding at the very top: the last kind with any weight.
	for (int32 Index = 2; Index >= 0; --Index)
	{
		if (Weights[Index] > 0.f)
		{
			return Kinds3[Index];
		}
	}
	return EGrenadeKind::Salt;
}

FGrenadeCollect GrenadeKinds::Collect(EGrenadeKind Carried, int32 Count, EGrenadeKind Found, int32 Amount, int32 Max)
{
	FGrenadeCollect Made;
	// A special tin is a new kind of jar to carry; salt is salt for any jar, so it never changes what's carried.
	Made.Kind = IsSpecial(Found) ? Found : Carried;
	Made.bSwapped = Made.Kind != Carried;
	const int32 Most = FMath::Max(Max, 0);
	const int32 Before = FMath::Clamp(Count, 0, Most);
	Made.Count = FMath::Clamp(Before + FMath::Max(Amount, 0), 0, Most);
	Made.Taken = Made.Count - Before;
	return Made;
}

void GrenadeKinds::BroodVelocities(int32 Count, const FVector& Up, float Speed, float Lift, FRandomStream& Random, TArray<FVector>& OutVelocities)
{
	OutVelocities.Reset();
	if (Count <= 0)
	{
		return;
	}
	FVector Axis = Up.GetSafeNormal();
	if (Axis.IsNearlyZero())
	{
		Axis = FVector::UpVector;
	}
	// Two ways across the surface, square to its up.
	FVector Across = FVector::CrossProduct(Axis, FMath::Abs(Axis.Z) < 0.9f ? FVector::UpVector : FVector::ForwardVector).GetSafeNormal();
	const FVector Other = FVector::CrossProduct(Axis, Across).GetSafeNormal();
	const float Start = Random.FRandRange(0.f, 2.f * PI);
	const float Step = 2.f * PI / static_cast<float>(Count);
	// A little off even, so no two bursts look stamped from one die.
	const float Jitter = Step * 0.15f;
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const float Angle = Start + Step * Index + Random.FRandRange(-Jitter, Jitter);
		const FVector Out = Across * FMath::Cos(Angle) + Other * FMath::Sin(Angle);
		OutVelocities.Add(Out * Speed * Random.FRandRange(0.85f, 1.15f) + Axis * Lift * Random.FRandRange(0.85f, 1.15f));
	}
}

float GrenadeKinds::BroodFuse(int32 Index, int32 Count, float Roll, const FGrenadeKindTuning& Kinds)
{
	const float Low = FMath::Max(Kinds.BroodFuseMin, 0.05f);
	const float High = FMath::Max(Kinds.BroodFuseMax, Low);
	// Each a step later than the last, so they go pop, pop, pop rather than at once; a hair of jitter inside its step.
	const float Steps = static_cast<float>(FMath::Max(Count, 1));
	const float Share = (static_cast<float>(FMath::Clamp(Index, 0, FMath::Max(Count - 1, 0))) + FMath::Clamp(Roll, 0.f, 1.f) * 0.5f) / Steps;
	return FMath::Lerp(Low, High, FMath::Clamp(Share, 0.f, 1.f));
}

float GrenadeKinds::BedDamage(float Seconds, float LevelScale, float Affinity, float Roll, const FGraveSaltTuning& Base, const FGrenadeKindTuning& Kinds)
{
	const float Listed = Base.BaseDamage * Kinds.BedShare * FMath::Max(Seconds, 0.f) * FMath::Max(LevelScale, 0.f) * FMath::Max(Affinity, 0.f);
	if (Listed <= 0.f)
	{
		return 0.f;
	}
	// Like any hit, a spread about the listed damage; a fire finds no critical spot.
	return LooterCombat::HitDamage(Listed, false, Roll);
}

float GrenadeKinds::CapLeft(float Damage, float AlreadyDealt, float MaxHealth, float CapShare)
{
	const float Cap = FMath::Max(MaxHealth, 0.f) * FMath::Clamp(CapShare, 0.f, 1.f);
	return FMath::Clamp(Damage, 0.f, FMath::Max(Cap - FMath::Max(AlreadyDealt, 0.f), 0.f));
}

#undef LOCTEXT_NAMESPACE

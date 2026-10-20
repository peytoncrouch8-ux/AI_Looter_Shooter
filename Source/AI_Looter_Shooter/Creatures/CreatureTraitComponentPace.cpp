// UCreatureTraitComponent: the traits that change its pace. Fleet (always faster, its wind-ups quicker: wind streaks and a
// glint) and Vengeful (under 30% health it enrages for good: harder and faster, a flare, embers). Both multiply what the
// creature has and divide it back, so whatever else changed it meanwhile (a Bell-Ringer's steadying, a rally) keeps its say.

#include "Creatures/CreatureTraitComponent.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Audio/LooterSoundRules.h"
#include "Combat/HealthComponent.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/CreatureTraitRules.h"
#include "Creatures/CreatureTraitSettings.h"
#include "Weapons/WeaponFX.h"
#include "Components/SkeletalMeshComponent.h"

// ---------------------------------------------------------------------------
// Fleet
// ---------------------------------------------------------------------------

void UCreatureTraitComponent::BeginFleet()
{
	ACreatureBase* Creature = GetCreature();
	if (!Creature || !FMath::IsNearlyEqual(FleetSpeedApplied, 1.f))
	{
		return;
	}
	FleetSpeedApplied = FMath::Max(UCreatureTraitSettings::Get().Fleet.SpeedScale, 0.01f);
	ScaleSpeed(*Creature, FleetSpeedApplied);
	// An attack already under way keeps the wind-up it began with; the next one is quickened.
	bWasAttacking = Creature->GetCreatureState() == ECreatureState::Attack;
}

void UCreatureTraitComponent::EndFleet()
{
	EndWindup();
	ACreatureBase* Creature = GetCreature();
	if (Creature && !FMath::IsNearlyEqual(FleetSpeedApplied, 1.f))
	{
		ScaleSpeed(*Creature, 1.f / FleetSpeedApplied);
	}
	FleetSpeedApplied = 1.f;
	GlintLeft = 0.f;
}

void UCreatureTraitComponent::EndWindup()
{
	if (!bWindupScaled)
	{
		return;
	}
	bWindupScaled = false;
	// Only ours to put back: a kind that set a wind-up of its own meanwhile (cut short, or its next move's) keeps it.
	ACreatureBase* Creature = GetCreature();
	if (Creature && FMath::IsNearlyEqual(Creature->AttackWindup, WrittenWindup, 0.0001f))
	{
		Creature->AttackWindup = SavedWindup;
	}
}

void UCreatureTraitComponent::TickFleet(float DeltaSeconds, FWeaponFX* Effects)
{
	ACreatureBase* Creature = GetCreature();
	if (!Creature || !HasTrait(ECreatureTrait::Fleet))
	{
		return;
	}
	const FFleetTraitTuning& Tuning = UCreatureTraitSettings::Get().Fleet;

	// A new attack: its wind-up quickened from its first frame, whatever its kind chose for this one (a spit, a swing);
	// put back as the attack ends.
	const bool bAttacking = Creature->GetCreatureState() == ECreatureState::Attack;
	if (bAttacking && !bWasAttacking && !Creature->IsDead())
	{
		SavedWindup = Creature->AttackWindup;
		Creature->AttackWindup = FMath::Max(SavedWindup * Tuning.WindupScale, 0.05f);
		WrittenWindup = Creature->AttackWindup;
		bWindupScaled = true;
		GlintLeft = Tuning.GlintSeconds;
	}
	else if (!bAttacking && bWindupScaled)
	{
		EndWindup();
	}
	bWasAttacking = bAttacking;

	if (!Effects || !ShowsLooks() || Creature->IsDead())
	{
		GlintLeft = FMath::Max(GlintLeft - DeltaSeconds, 0.f);
		return;
	}
	const FLinearColor Color = FLinearColor::FromSRGBColor(Tuning.Color);
	const float Size = FMath::Max(Creature->GetSizeScale(), 0.3f);
	const FVector Center = BodyCenter();
	const float Radius = BodyRadius();
	const float Half = BodyHalfHeight();

	// The glint: a hard little star at its front as it winds up, the quicker strike's tell.
	if (GlintLeft > 0.f)
	{
		const float Share = FMath::Clamp(GlintLeft / FMath::Max(Tuning.GlintSeconds, 0.01f), 0.f, 1.f);
		const FVector Forward = Creature->GetActorForwardVector().GetSafeNormal2D();
		const FVector Right = FVector::CrossProduct(FVector::UpVector, Forward);
		const FVector Front = Center + Forward * (Radius * 0.75f) + FVector(0.0, 0.0, Half * 0.25f);
		const float Arm = (10.f + 22.f * Share) * Size;
		Effects->AddGlow(Front, (14.f + 20.f * Share) * Size, Color, 10.f * Share);
		Effects->AddStreak(Front - Right * Arm, Front + Right * Arm, 3.f, Color, 9.f * Share);
		Effects->AddStreak(Front - FVector(0.0, 0.0, Arm), Front + FVector(0.0, 0.0, Arm), 3.f, Color, 9.f * Share);
		GlintLeft -= DeltaSeconds;
	}

	// Running: three pale streaks of wind trailing it (its flanks and its back), longer the faster it goes.
	const FVector Velocity = Creature->GetVelocity();
	const float Speed = static_cast<float>(Velocity.Size2D());
	const float Pace = Speed / FMath::Max(Creature->ChaseSpeed, 1.f);
	if (Pace <= Tuning.StreakSpeedShare)
	{
		return;
	}
	const float Strength = FMath::Clamp((Pace - Tuning.StreakSpeedShare) / FMath::Max(1.f - Tuning.StreakSpeedShare, 0.05f), 0.f, 1.f);
	const FVector Back = -Velocity.GetSafeNormal2D();
	const FVector Side = FVector::CrossProduct(FVector::UpVector, Back).GetSafeNormal();
	const FVector Offsets[] = { Side * (Radius * 0.8f) + FVector(0.0, 0.0, Half * 0.1f), -Side * (Radius * 0.8f) + FVector(0.0, 0.0, Half * 0.1f),
		FVector(0.0, 0.0, Half * 0.65f) };
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Offsets); ++Index)
	{
		// Each flows back a little out of step with the others, so they read as moving air rather than lines stuck on. They
		// start off the body's back (from a third of the way back, a running spider's abdomen hid most of each, and the
		// rest was a hairline under it: seen in play 2026-10-20) and are drawn a little bolder.
		const float Flow = 0.65f + 0.35f * FMath::Sin(Clock * 17.f + Index * 2.1f);
		const FVector Start = Center + Offsets[Index] + Back * (Radius * 0.6f);
		const FVector Tail = Start + Back * (Speed * 0.2f * Flow);
		Effects->AddStreak(Tail, Start, 4.5f * FMath::Max(Size, 1.f), Color, 5.f * Strength);
	}
}

// ---------------------------------------------------------------------------
// Vengeful
// ---------------------------------------------------------------------------

void UCreatureTraitComponent::CheckEnrage()
{
	const UHealthComponent* Life = Health.Get();
	const ACreatureBase* Creature = GetCreature();
	if (bEnraged || !HasTrait(ECreatureTrait::Vengeful) || !Life || !Creature || Creature->IsDead() || Life->IsDead() || Life->GetHealth() <= 0.f)
	{
		return;
	}
	if (Life->GetHealthPercent() < UCreatureTraitSettings::Get().Vengeful.HealthShare)
	{
		Enrage();
	}
}

void UCreatureTraitComponent::Enrage()
{
	ACreatureBase* Creature = GetCreature();
	if (bEnraged || !Creature)
	{
		return;
	}
	const FVengefulTraitTuning& Tuning = UCreatureTraitSettings::Get().Vengeful;
	bEnraged = true;
	EnrageSpeedApplied = FMath::Max(Tuning.SpeedScale, 0.01f);
	EnrageDamageApplied = FMath::Max(Tuning.DamageScale, 0.01f);
	ScaleSpeed(*Creature, EnrageSpeedApplied);
	Creature->AttackDamage *= EnrageDamageApplied;
	WrittenDamage = Creature->AttackDamage;
	FlareLeft = Tuning.FlareSeconds;
	EmberIn = 0.f;

	// Its cry, and a spray of red embers flung out and up with the flare (TickVengeance draws the light).
	LooterSound::PlayAttached(LooterSoundCue::Tier::TraitEnrage, Creature->GetRootComponent(), NAME_None, 1.f, LooterSoundRules::PitchForSize(Creature->GetSizeScale()));
	const FLinearColor Color = FLinearColor::FromSRGBColor(Tuning.Color);
	const FVector Center = BodyCenter();
	const float Size = FMath::Max(Creature->GetSizeScale(), 0.3f);
	for (int32 Index = 0; Index < 22; ++Index)
	{
		FVector Out = Random.GetUnitVector();
		Out.Z = FMath::Abs(Out.Z) * 0.8f + 0.2f;
		Sparks.Spawn(Center + Out * (20.f * Size), Out.GetSafeNormal() * Random.FRandRange(260.f, 520.f) * Size, Random.FRandRange(0.5f, 0.9f), 3.f,
			Color, 16.f, 520.f, 2.f);
	}
	UpdateTickEnabled();
	UE_LOG(LogLooter, Verbose, TEXT("%s enrages (Vengeful): damage %.1f, chase %.0f"), *Creature->GetName(), Creature->AttackDamage, Creature->ChaseSpeed);
}

void UCreatureTraitComponent::Calm()
{
	if (!bEnraged)
	{
		return;
	}
	bEnraged = false;
	if (ACreatureBase* Creature = GetCreature())
	{
		ScaleSpeed(*Creature, 1.f / EnrageSpeedApplied);
		// Only ours to take back: stats applied again since (they gave it its damage afresh) are left as they are.
		if (FMath::IsNearlyEqual(Creature->AttackDamage, WrittenDamage, 0.001f))
		{
			Creature->AttackDamage /= EnrageDamageApplied;
		}
	}
	EnrageSpeedApplied = 1.f;
	EnrageDamageApplied = 1.f;
	FlareLeft = 0.f;
}

void UCreatureTraitComponent::TickVengeance(float DeltaSeconds, FWeaponFX* Effects)
{
	ACreatureBase* Creature = GetCreature();
	if (!Creature || !HasTrait(ECreatureTrait::Vengeful))
	{
		return;
	}
	if (!bEnraged)
	{
		// Its health can fall under the line without a hit of its own (its most health raised by a level).
		CheckEnrage();
		if (!bEnraged)
		{
			return;
		}
	}
	// Its stats applied again (a level set anew gives it its damage afresh): the rage goes on top of the new ones.
	if (!FMath::IsNearlyEqual(Creature->AttackDamage, WrittenDamage, 0.001f))
	{
		Creature->AttackDamage *= EnrageDamageApplied;
		WrittenDamage = Creature->AttackDamage;
	}
	const FVengefulTraitTuning& Tuning = UCreatureTraitSettings::Get().Vengeful;
	FlareLeft = FMath::Max(FlareLeft - DeltaSeconds, 0.f);
	if (!Effects || !ShowsLooks() || Creature->IsDead())
	{
		return;
	}
	const FLinearColor Color = FLinearColor::FromSRGBColor(Tuning.Color);
	const FVector Center = BodyCenter();
	const float Radius = BodyRadius();
	const float Half = BodyHalfHeight();
	const float Size = FMath::Max(Creature->GetSizeScale(), 0.3f);

	// The flare: red light bursting round it as it enrages, opening out as it fades.
	if (FlareLeft > 0.f)
	{
		const float Share = FlareLeft / FMath::Max(Tuning.FlareSeconds, 0.01f);
		Effects->AddGlow(Center, Radius * (1.8f + 1.f * (1.f - Share)), Color, 18.f * Share * Share);
	}
	// From then on its heat: a faint red glow at its heart beating fast, and embers rising off its back.
	Effects->AddGlow(Center, Radius * 0.9f, Color, 1.6f + 0.9f * FMath::Abs(FMath::Sin(Clock * 6.f)));
	EmberIn -= DeltaSeconds;
	while (EmberIn <= 0.f)
	{
		EmberIn += 0.07f;
		const float Angle = Random.FRandRange(0.f, UE_TWO_PI);
		const FVector From = Center + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * (Radius * 0.6f * FMath::Sqrt(Random.FRand()))
			+ FVector(0.0, 0.0, Half * 0.4f);
		const FVector Rise(Random.FRandRange(-30.f, 30.f), Random.FRandRange(-30.f, 30.f), Random.FRandRange(90.f, 170.f) * Size);
		Sparks.Spawn(From, Rise, Random.FRandRange(0.5f, 0.9f), 3.f, Color, 11.f, -80.f, 1.2f);
	}
}

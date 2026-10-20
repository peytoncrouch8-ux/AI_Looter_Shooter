// UCreatureTraitComponent: Rallying. Every few frames it looks round for creatures within its reach (never a boss, never
// itself) and holds them (UCreatureRallied: less damage, faster) while it lives; in a fight its ring lies on the ground,
// tethers of light run from it to each one it holds with a bead of light flowing along them, and every 6 s its rallying
// cry pulses out through the ring. The tells say "kill this one first"; dead, its tethers snap.

#include "Creatures/CreatureTraitComponent.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Audio/LooterSoundRules.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/CreatureRallied.h"
#include "Creatures/CreatureTraitRules.h"
#include "Creatures/CreatureTraitSettings.h"
#include "Embers/Powers/EmberFX.h"
#include "Weapons/WeaponFX.h"
#include "Engine/World.h"
#include "EngineUtils.h"

namespace
{
	/** Its ring is laid on the ground again once it has walked this far from where it was laid (cm), at most this often (s). */
	constexpr float RingRelayDistance = 60.f;
	constexpr float RingRelaySeconds = 0.3f;
	constexpr int32 RingPoints = 32;
	/** The cry's pulse runs out to the ring in this long (s). */
	constexpr float PulseSeconds = 0.7f;

	/**
	 * Every creature in the level (the class's own list of objects, a few times a second per rallier: cheap). Not the
	 * encounters' tracked list: it can miss one a test or a script put down without spawning it in play.
	 */
	void ForEachCreature(UWorld& World, TFunctionRef<void(ACreatureBase&)> Visit)
	{
		for (TActorIterator<ACreatureBase> It(&World); It; ++It)
		{
			Visit(**It);
		}
	}
}

void UCreatureTraitComponent::TickRally(float DeltaSeconds, FWeaponFX* Effects)
{
	ACreatureBase* Creature = GetCreature();
	if (!Creature || !HasTrait(ECreatureTrait::Rallying))
	{
		return;
	}
	if (Creature->IsDead() || Creature->IsBossCreature())
	{
		ReleaseRally(!Creature->IsBossCreature());
		return;
	}
	const FRallyingTraitTuning& Tuning = UCreatureTraitSettings::Get().Rallying;
	RallyScanIn -= DeltaSeconds;
	if (RallyScanIn <= 0.f)
	{
		RallyScanIn = Tuning.ScanSeconds;
		ScanRally();
	}
	// In a fight its cry comes at once, then every PulseSeconds.
	const bool bFighting = Creature->GetTarget() != nullptr;
	if (bFighting && !bRallyFighting)
	{
		RallyPulseIn = 0.f;
	}
	bRallyFighting = bFighting;
	RallyPulseAge = FMath::Min(RallyPulseAge + DeltaSeconds, 1000.f);
	if (bFighting)
	{
		RallyPulseIn -= DeltaSeconds;
		if (RallyPulseIn <= 0.f)
		{
			RallyPulseIn = Tuning.PulseSeconds;
			PulseRally();
		}
	}
	RallyRingIn = FMath::Max(RallyRingIn - DeltaSeconds, 0.f);
	if (Effects && ShowsLooks() && (bFighting || GetRalliedCount() > 0))
	{
		DrawRally(*Effects);
	}
}

void UCreatureTraitComponent::ScanRally()
{
	ACreatureBase* Creature = GetCreature();
	UWorld* World = GetWorld();
	if (!Creature || !World)
	{
		return;
	}
	const FRallyingTraitTuning& Tuning = UCreatureTraitSettings::Get().Rallying;
	const FVector Here = Creature->GetActorLocation();
	const double ReachSquared = FMath::Square(static_cast<double>(Tuning.Radius));
	TArray<ACreatureBase*, TInlineAllocator<16>> Near;
	ForEachCreature(*World, [&](ACreatureBase& Other)
	{
		if (&Other != Creature && !Other.IsDead() && !Other.IsActorBeingDestroyed() && !Other.IsBossCreature()
			&& FVector::DistSquared2D(Other.GetActorLocation(), Here) <= ReachSquared
			&& FMath::Abs(Other.GetActorLocation().Z - Here.Z) <= Tuning.MaxRise)
		{
			Near.Add(&Other);
		}
	});
	// Those it held that have left its reach (or died, or gone) are let go; those newly in it are taken up.
	for (int32 Index = Rallied.Num() - 1; Index >= 0; --Index)
	{
		ACreatureBase* Held = Rallied[Index].Get();
		if (!Held || !Near.Contains(Held))
		{
			if (Held)
			{
				UCreatureRallied::Leave(*Held, *Creature);
			}
			Rallied.RemoveAtSwap(Index);
		}
	}
	for (ACreatureBase* Other : Near)
	{
		if (!Rallied.Contains(Other) && UCreatureRallied::Join(*Other, *Creature, Tuning.DamageTakenScale, Tuning.SpeedScale))
		{
			Rallied.Add(Other);
		}
	}
}

void UCreatureTraitComponent::ReleaseRally(bool bSnap)
{
	ACreatureBase* Creature = GetCreature();
	FWeaponFX* Effects = bSnap ? GetEffects() : nullptr;
	const FLinearColor Color = FLinearColor::FromSRGBColor(UCreatureTraitSettings::Get().Rallying.Color);
	for (const TWeakObjectPtr<ACreatureBase>& Each : Rallied)
	{
		ACreatureBase* Held = Each.Get();
		if (!Held || !Creature)
		{
			continue;
		}
		UCreatureRallied::Leave(*Held, *Creature);
		// Its tether snaps: a spit of sparks off each one it held (what killing it first bought).
		if (Effects)
		{
			const FVector At = Held->GetActorLocation();
			for (int32 Index = 0; Index < 7; ++Index)
			{
				Sparks.Spawn(At, Random.GetUnitVector() * Random.FRandRange(150.f, 300.f) + FVector(0.0, 0.0, 120.0), Random.FRandRange(0.3f, 0.5f), 3.f,
					Color, 12.f, 700.f, 2.f);
			}
		}
	}
	Rallied.Reset();
	RallyPulseAge = 1000.f;
	if (bSnap)
	{
		UpdateTickEnabled();
	}
}

void UCreatureTraitComponent::PulseRally()
{
	ACreatureBase* Creature = GetCreature();
	if (!Creature)
	{
		return;
	}
	RallyPulseAge = 0.f;
	LooterSound::PlayAttached(LooterSoundCue::Tier::TraitRally, Creature->GetRootComponent(), NAME_None, 0.85f, LooterSoundRules::PitchForSize(Creature->GetSizeScale()));
}

void UCreatureTraitComponent::DrawRally(FWeaponFX& Effects)
{
	ACreatureBase* Creature = GetCreature();
	UWorld* World = GetWorld();
	if (!Creature || !World)
	{
		return;
	}
	const FRallyingTraitTuning& Tuning = UCreatureTraitSettings::Get().Rallying;
	const FLinearColor Color = FLinearColor::FromSRGBColor(Tuning.Color);
	const FVector Ground = Feet();

	// Its reach, laid on the ground round it (again as it walks on), breathing slowly.
	if (RallyRing.Num() < 3 || (RallyRingIn <= 0.f && FVector::DistSquared2D(Ground, RallyRingAt) > FMath::Square(RingRelayDistance)))
	{
		RallyRing = EmberFX::GroundRing(*World, Ground, Tuning.Radius, RingPoints, 250.f, 18.f);
		RallyRingAt = Ground;
		RallyRingIn = RingRelaySeconds;
	}
	else if (!RallyRingAt.Equals(Ground, 1.0))
	{
		// Between layings the ring keeps up with it (shifted flat; laid properly again soon).
		const FVector Shift(Ground.X - RallyRingAt.X, Ground.Y - RallyRingAt.Y, 0.0);
		for (FVector& Point : RallyRing)
		{
			Point += Shift;
		}
		RallyRingAt += Shift;
	}
	const float Breath = 0.75f + 0.25f * FMath::Sin(Clock * 2.5f);
	EmberFX::DrawLine(Effects, RallyRing, true, 5.f, Color, 5.f * Breath);

	// The cry's pulse running out to the ring.
	if (RallyPulseAge < PulseSeconds)
	{
		const float Share = RallyPulseAge / PulseSeconds;
		const float Eased = 1.f - FMath::Square(1.f - Share);
		EmberFX::DrawRing(Effects, Ground + FVector(0.0, 0.0, 25.0), FVector::UpVector, FMath::Max(Tuning.Radius * Eased, 20.f), 40, 5.f, Color, 12.f * (1.f - Share));
	}

	// A tether to each one it holds, from its chest to theirs, a bead of light flowing out along it (brighter in a pulse).
	const FVector From = BodyCenter() + FVector(0.0, 0.0, BodyHalfHeight() * 0.3f);
	const float Pulse = RallyPulseAge < PulseSeconds ? 1.f - RallyPulseAge / PulseSeconds : 0.f;
	int32 Index = 0;
	for (const TWeakObjectPtr<ACreatureBase>& Each : Rallied)
	{
		const ACreatureBase* Held = Each.Get();
		if (!Held || Held->IsHidden())
		{
			continue;
		}
		const FVector To = Held->GetActorLocation() + FVector(0.0, 0.0, 20.0);
		Effects.AddStreak(From, To, 3.5f, Color, 3.5f + 6.f * Pulse);
		const float Along = FMath::Frac(Clock * 0.8f + Index * 0.31f);
		Effects.AddGlow(FMath::Lerp(From, To, Along), 20.f, Color, 12.f);
		// Where it lands: a small ring at the held one's feet.
		const FVector HeldFeet = Held->GetActorLocation() - FVector(0.0, 0.0, Held->GetSimpleCollisionHalfHeight() - 5.f);
		EmberFX::DrawRing(Effects, HeldFeet, FVector::UpVector, 45.f * FMath::Max(Held->GetSizeScale(), 0.5f), 12, 3.f, Color, 3.f + 5.f * Pulse);
		++Index;
	}
}

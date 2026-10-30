// UCreatureTraitComponent: the traits on its health. Warded (a ward of an element in front of it, through its health's
// Shield: a turning cage of light in its colour, a ripple and a ping per hit, a pop of shards, a stagger and a daze as it
// breaks) and Mending (3 s unhit it heals, motes rising and its shimmer looping; any hit breaks it off, the motes flung away).

#include "Creatures/CreatureTraitComponent.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Audio/LooterSoundRules.h"
#include "Combat/HealthComponent.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/CreatureHitReactionComponent.h"
#include "Creatures/CreatureKnockComponent.h"
#include "Creatures/CreatureTraitRules.h"
#include "Creatures/CreatureTraitSettings.h"
#include "Elements/ElementCallout.h"
#include "Elements/ElementRules.h"
#include "Elements/ElementStatusComponent.h"
#include "Elements/ElementTuning.h"
#include "Embers/Powers/EmberFX.h"
#include "Weapons/WeaponFX.h"
#include "Components/AudioComponent.h"

namespace
{
	/** A hit's ripple on the ward lasts this long (s), and at most this many show at once (a shotgun's pellets). */
	constexpr float RippleSeconds = 0.22f;
	constexpr int32 MaxRipples = 6;
	/** Ward pings are this far apart at least (s): a blast's pellets make one. */
	constexpr float WardSoundGap = 0.07f;
	/** The break's flash and rings, seconds. */
	constexpr float BreakFlashSeconds = 0.18f;
	constexpr float BreakRingSeconds = 0.35f;

	/** A ring of light Rx across one way (U) and Rz the other (V) round Center: a squashed ring for a squat body. */
	void DrawOval(FWeaponFX& Effects, const FVector& Center, const FVector& U, const FVector& V, float Ru, float Rv, int32 Segments, float Width,
		const FLinearColor& Color, float Glow, float Drop, int32 DropSeed)
	{
		if (Glow <= 0.f)
		{
			return;
		}
		const int32 Count = FMath::Max(Segments, 3);
		const float Step = UE_TWO_PI / static_cast<float>(Count);
		auto At = [&](float Angle) { return Center + U * (FMath::Cos(Angle) * Ru) + V * (FMath::Sin(Angle) * Rv); };
		for (int32 Index = 0; Index < Count; ++Index)
		{
			// A weak ward drops pieces of its rings (Drop: the share missing; the same pieces while DropSeed holds, then others).
			if (Drop > 0.f && FMath::Frac(FMath::Sin(static_cast<float>(Index * 131 + DropSeed * 17)) * 43758.5453f) < Drop)
			{
				continue;
			}
			const float From = Step * (static_cast<float>(Index) - 0.3f);
			Effects.AddStreak(At(From), At(From + Step * 1.6f), Width, Color, Glow);
		}
	}

	/**
	 * The element of the hit a ward is being handed: a status's piece or arc (what the status component deals), an element
	 * damage type's own (the salt grenade's is Wardbreak), or an element gun's hit (FHitElementScope). None: a plain hit.
	 */
	ELooterElement ElementOfHit(const UHealthComponent* Life)
	{
		const ELooterElement Dealing = UElementStatusComponent::DealingElement();
		if (Dealing != ELooterElement::None)
		{
			return Dealing;
		}
		const UHealthComponent::FHealthHit* Hit = Life ? Life->GetHitBeingTaken() : nullptr;
		const ELooterElement OfType = Hit ? ElementRules::ElementOf(Hit->DamageType) : ELooterElement::None;
		return OfType != ELooterElement::None ? OfType : FHitElementScope::Current();
	}
}

ELooterElement UCreatureTraitComponent::PickWardElement() const
{
	// A Touched creature's ward is its own element; the rest roll theirs from the seed their traits came from.
	const ACreatureBase* Creature = GetCreature();
	if (!Creature)
	{
		return ELooterElement::Thunder;
	}
	const ELooterElement Touched = Creature->GetTouchedElement();
	return Touched != ELooterElement::None ? Touched : CreatureTraitRules::RollWardElement(Creature->GetTierSeed());
}

void UCreatureTraitComponent::RefreshWardElement()
{
	WardElement = PickWardElement();
}

FLinearColor UCreatureTraitComponent::WardColor() const
{
	return WardElement != ELooterElement::None ? ElementRules::FxColor(WardElement)
		: FLinearColor::FromSRGBColor(UCreatureTraitSettings::Get().Warded.Color);
}

// ---------------------------------------------------------------------------
// Warded
// ---------------------------------------------------------------------------

void UCreatureTraitComponent::RaiseWard()
{
	UHealthComponent* Life = Health.Get();
	if (!Life)
	{
		return;
	}
	WardFraction = 1.f;
	WardElement = PickWardElement();
	WardFlare = 0.f;
	WardBreakAge = 1000.f;
	bDazePending = false;
	Ripples.Reset();
	if (!Life->Shield.IsBound())
	{
		Life->Shield.BindUObject(this, &UCreatureTraitComponent::AbsorbIntoWard);
	}
}

void UCreatureTraitComponent::DropWard()
{
	WardFraction = 0.f;
	UHealthComponent* Life = Health.Get();
	if (Life && Life->Shield.IsBoundToObject(this))
	{
		Life->Shield.Unbind();
	}
}

float UCreatureTraitComponent::AbsorbIntoWard(float Damage, bool bCritical, const FVector& HitLocation)
{
	const ACreatureBase* Creature = GetCreature();
	if (WardFraction <= 0.f || !Creature || Creature->IsDead() || !HasTrait(ECreatureTrait::Warded))
	{
		return Damage;
	}
	const FWardedTraitTuning& Tuning = UCreatureTraitSettings::Get().Warded;
	// The ward is a share of its most health as that is now (a level's growth grows it too), kept as its share.
	const float Capacity = FMath::Max(Tuning.HealthShare, 0.f) * MaxHealthNow();
	if (Capacity <= 0.f)
	{
		return Damage;
	}
	float Left = WardFraction * Capacity;
	// Its type: its own element breaks it x2, Wardbreak any ward x1.5 (the larger alone), then a crit's x2 on top. The old
	// salted-ward bonus is gone.
	const FElementTuning Elements = FElementTuning::Live();
	const float ElementScale = CreatureTraitRules::WardElementScale(WardElement, ElementOfHit(Health.Get()), Elements.WardMatch, Elements.WardBreakAny);
	const float ToHealth = CreatureTraitRules::AbsorbWard(Left, Damage, bCritical, Tuning.CritBreakScale, ElementScale);
	WardFraction = FMath::Clamp(Left / Capacity, 0.f, 1.f);
	if (WardFraction <= 0.f)
	{
		BreakWard(HitLocation);
	}
	else
	{
		NoteWardHit(HitLocation, bCritical);
	}
	return ToHealth;
}

void UCreatureTraitComponent::NoteWardHit(const FVector& HitLocation, bool bCritical)
{
	ACreatureBase* Creature = GetCreature();
	if (!Creature)
	{
		return;
	}
	// The cage flares and a ripple spreads from where it was struck, turned to face out from the body.
	WardFlare = 1.f;
	if (Ripples.Num() >= MaxRipples)
	{
		Ripples.RemoveAt(0);
	}
	FWardRipple& Ripple = Ripples.AddDefaulted_GetRef();
	const FVector Center = BodyCenter();
	FVector Out = HitLocation - Center;
	// A hit with no place of its own (an ember power's, a grenade's) lands on the ward's top.
	if (!Out.Normalize())
	{
		Out = FVector::UpVector;
	}
	Ripple.Location = Center + Out * BodyRadius();
	Ripple.Normal = Out;
	Ripple.Age = 0.f;

	// Its ping, higher as the ward wears thin (a crit's louder).
	if (SinceWardSound >= WardSoundGap)
	{
		SinceWardSound = 0.f;
		LooterSound::PlayAt(Creature, LooterSoundCue::Tier::TraitWardHit, Ripple.Location, bCritical ? 1.f : 0.75f, 1.f + 0.25f * (1.f - WardFraction));
	}
}

void UCreatureTraitComponent::BreakWard(const FVector& HitLocation)
{
	ACreatureBase* Creature = GetCreature();
	if (!Creature)
	{
		return;
	}
	const FWardedTraitTuning& Tuning = UCreatureTraitSettings::Get().Warded;
	WardFraction = 0.f;
	WardFlare = 0.f;
	WardBreakAge = 0.f;
	Ripples.Reset();
	// Dazed once the hit is done with (it may yet die of what spilled over).
	bDazePending = Tuning.BreakDazeSeconds > 0.f;

	// A break always staggers it (the user: "once the ward is broken they stagger"), whatever its stagger cooldown, shoved
	// away from the hit; the daze follows as before.
	const FVector Center = BodyCenter();
	const float Radius = BodyRadius();
	if (UCreatureHitReactionComponent* React = Creature->FindComponentByClass<UCreatureHitReactionComponent>())
	{
		React->WardBreak((Center - HitLocation).GetSafeNormal2D());
	}

	// The pop: its sounds (the shatter, and its element's own), and shards of the cage flung out from all round it that
	// fall (the flash and rings: TickWard).
	const float Pitch = LooterSoundRules::PitchForSize(Creature->GetSizeScale());
	LooterSound::PlayAt(Creature, LooterSoundCue::Tier::TraitWardBreak, Center, 1.f, Pitch);
	const FName ElementCue = ElementRules::Cue(WardElement, EElementCue::WardBreak);
	if (!ElementCue.IsNone())
	{
		LooterSound::PlayAt(Creature, ElementCue, Center, 1.f, Pitch);
	}
	const FLinearColor Color = WardColor();
	for (int32 Index = 0; Index < 28; ++Index)
	{
		const FVector Out = Random.GetUnitVector();
		const FVector From = Center + Out * Radius;
		const FVector Fling = Out * Random.FRandRange(320.f, 640.f) + FVector(0.0, 0.0, Random.FRandRange(80.f, 220.f));
		Sparks.Spawn(From, Fling, Random.FRandRange(0.45f, 0.85f), Random.FRandRange(3.f, 5.f), Color, 14.f, 900.f, 1.6f);
	}
	UpdateTickEnabled();
	UE_LOG(LogLooter, Verbose, TEXT("%s: its ward breaks (Warded)"), *Creature->GetName());
}

void UCreatureTraitComponent::TickWard(float DeltaSeconds, FWeaponFX* Effects)
{
	ACreatureBase* Creature = GetCreature();
	if (!Creature)
	{
		return;
	}
	SinceWardSound = FMath::Min(SinceWardSound + DeltaSeconds, 1000.f);
	WardFlare = FMath::Max(WardFlare - DeltaSeconds * 6.f, 0.f);
	WardBreakAge = FMath::Min(WardBreakAge + DeltaSeconds, 1000.f);
	for (int32 Index = Ripples.Num() - 1; Index >= 0; --Index)
	{
		Ripples[Index].Age += DeltaSeconds;
		if (Ripples[Index].Age >= RippleSeconds)
		{
			Ripples.RemoveAt(Index);
		}
	}
	// The break's daze, on the frame after: never on a body the spill killed.
	if (bDazePending)
	{
		bDazePending = false;
		UCreatureKnockComponent* Knock = Creature->GetKnock();
		if (Knock && !Creature->IsDead())
		{
			Knock->Daze(UCreatureTraitSettings::Get().Warded.BreakDazeSeconds);
		}
	}
	if (!Effects || !HasTrait(ECreatureTrait::Warded))
	{
		return;
	}
	const FLinearColor Color = WardColor();
	if (WardFraction > 0.f && ShowsLooks() && !Creature->IsDead())
	{
		DrawWardCage(*Effects);
	}
	for (const FWardRipple& Ripple : Ripples)
	{
		const float Share = Ripple.Age / RippleSeconds;
		EmberFX::DrawRing(*Effects, Ripple.Location, Ripple.Normal, 10.f + 32.f * Share, 10, 3.f, Color, 12.f * (1.f - Share));
	}
	// Broken: a flash where it stood and two rings of light going out from it, flat and upright.
	if (WardBreakAge < BreakRingSeconds)
	{
		const FVector Center = BodyCenter();
		const float Radius = BodyRadius();
		if (WardBreakAge < BreakFlashSeconds)
		{
			const float Flash = 1.f - WardBreakAge / BreakFlashSeconds;
			Effects->AddGlow(Center, Radius * (1.6f + 1.f * (1.f - Flash)), Color, 18.f * Flash);
		}
		const float Share = WardBreakAge / BreakRingSeconds;
		const float Ring = Radius * (1.f + 2.4f * Share);
		EmberFX::DrawRing(*Effects, Center, FVector::UpVector, Ring, 28, 4.f, Color, 14.f * (1.f - Share));
		EmberFX::DrawRing(*Effects, Center, Creature->GetActorRightVector(), Ring * 0.8f, 24, 3.5f, Color, 10.f * (1.f - Share));
	}
}

void UCreatureTraitComponent::DrawWardCage(FWeaponFX& Effects) const
{
	const ACreatureBase* Creature = GetCreature();
	if (!Creature)
	{
		return;
	}
	const FWardedTraitTuning& Tuning = UCreatureTraitSettings::Get().Warded;
	const FLinearColor Color = WardColor();
	const FVector Center = BodyCenter();
	const float Across = BodyRadius();
	// Squat bodies (a spider, a slime) get a squat cage: as tall as the body, at least a little.
	const float Tall = FMath::Max(BodyHalfHeight() * 1.1f, Across * 0.55f);
	// Fainter as it wears down; under FlickerShare it flickers and drops pieces, about to go.
	const float Strength = Tuning.ShellGlow * (0.4f + 0.6f * WardFraction) + WardFlare * Tuning.HitGlow;
	const bool bWeak = WardFraction < Tuning.FlickerShare;
	const float Flicker = bWeak && FMath::Frac(Clock * 11.f) < 0.3f ? 0.25f : 1.f;
	const float Drop = bWeak ? 0.35f : 0.f;
	const int32 DropSeed = FMath::FloorToInt32(Clock * 8.f);
	// Three rings turning on their own axes: a level one round its middle, two upright ones crossing over it.
	DrawOval(Effects, Center, FVector::ForwardVector, FVector::RightVector, Across, Across, 16, 3.f, Color, Strength * Flicker, Drop, DropSeed);
	for (int32 Index = 0; Index < 2; ++Index)
	{
		const float Turn = Clock * (Index == 0 ? 0.9f : -0.7f) + Index * UE_HALF_PI;
		const FVector Flat(FMath::Cos(Turn), FMath::Sin(Turn), 0.f);
		DrawOval(Effects, Center, Flat, FVector::UpVector, Across, Tall, 16, 3.f, Color, Strength * 0.8f * Flicker, Drop, DropSeed + Index + 1);
	}
}

// ---------------------------------------------------------------------------
// Mending
// ---------------------------------------------------------------------------

void UCreatureTraitComponent::TickMending(float DeltaSeconds, FWeaponFX* Effects)
{
	ACreatureBase* Creature = GetCreature();
	UHealthComponent* Life = Health.Get();
	if (!Creature || !Life || !HasTrait(ECreatureTrait::Mending))
	{
		return;
	}
	const FMendingTraitTuning& Tuning = UCreatureTraitSettings::Get().Mending;
	const bool bHurt = Life->GetHealth() < Life->GetMaxHealth() - 0.01f;
	const bool bCanMend = bHurt && !Creature->IsDead() && !Life->IsDead() && SinceHit >= Tuning.UnhitSeconds && !bBlinking;
	if (!bCanMend)
	{
		if (bMending)
		{
			StopMending(false);
		}
		return;
	}
	if (!bMending)
	{
		StartMending();
	}
	Mended += Life->Heal(CreatureTraitRules::MendAmount(SinceHit, Tuning.UnhitSeconds, Tuning.HealSharePerSecond, Life->GetMaxHealth(), DeltaSeconds));
	if (!Effects || !ShowsLooks())
	{
		return;
	}
	// Green motes rising off it from all round its body, and a soft green breath at its heart.
	const FLinearColor Color = FLinearColor::FromSRGBColor(Tuning.Color);
	const FVector Center = BodyCenter();
	const float Radius = BodyRadius();
	const float Half = BodyHalfHeight();
	Effects->AddGlow(Center, Radius * 1.1f, Color, 1.2f + 0.6f * FMath::Sin(Clock * 4.f));
	MendMoteIn -= DeltaSeconds;
	while (MendMoteIn <= 0.f)
	{
		MendMoteIn += Tuning.MoteEvery;
		const float Angle = Random.FRandRange(0.f, UE_TWO_PI);
		const FVector From = Center + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * (Radius * Random.FRandRange(0.4f, 0.9f))
			- FVector(0.0, 0.0, Half * 0.5f);
		Sparks.Spawn(From, FVector(0.0, 0.0, Random.FRandRange(60.f, 110.f)), Random.FRandRange(0.8f, 1.2f), 3.f, Color, 10.f, -25.f, 0.6f);
	}
}

void UCreatureTraitComponent::StartMending()
{
	ACreatureBase* Creature = GetCreature();
	if (bMending || !Creature)
	{
		return;
	}
	bMending = true;
	const FMendingTraitTuning& Tuning = UCreatureTraitSettings::Get().Mending;
	MendMoteIn = 0.f;
	// Its shimmer loops on it while it mends (quiet, heard near): a heal heard, so a player turns back to it.
	LooterSound::Stop(MendSound.Get(), 0.f);
	MendSound = LooterSound::Start(Creature, LooterSoundCue::Tier::TraitMend, Creature->GetRootComponent(), NAME_None, 1.f,
		LooterSoundRules::PitchForSize(Creature->GetSizeScale()));
	// It begins with a ring of motes lifting off it all at once.
	const FLinearColor Color = FLinearColor::FromSRGBColor(Tuning.Color);
	const FVector Center = BodyCenter();
	const float Radius = BodyRadius();
	for (int32 Index = 0; Index < 12; ++Index)
	{
		const float Angle = UE_TWO_PI * Index / 12.f;
		const FVector Out(FMath::Cos(Angle), FMath::Sin(Angle), 0.f);
		Sparks.Spawn(Center + Out * Radius, Out * 40.f + FVector(0.0, 0.0, 120.0), 0.9f, 3.f, Color, 12.f, -20.f, 0.8f);
	}
	UpdateTickEnabled();
}

void UCreatureTraitComponent::StopMending(bool bBroken)
{
	if (!bMending)
	{
		return;
	}
	bMending = false;
	// Its shimmer stops: cut short by a hit, eased off when it's whole again.
	LooterSound::Stop(MendSound.Get(), bBroken ? 0.08f : 0.4f);
	MendSound.Reset();
	// Broken off by a hit: its motes flung away from it (the hit's reward, seen).
	const ACreatureBase* Creature = GetCreature();
	if (!bBroken || !Creature)
	{
		return;
	}
	const FLinearColor Color = FLinearColor::FromSRGBColor(UCreatureTraitSettings::Get().Mending.Color);
	const FVector Center = BodyCenter();
	const float Radius = BodyRadius();
	for (int32 Index = 0; Index < 10; ++Index)
	{
		const FVector Out = Random.GetUnitVector();
		Sparks.Spawn(Center + Out * (Radius * 0.7f), Out * Random.FRandRange(220.f, 380.f), Random.FRandRange(0.25f, 0.4f), 3.f, Color, 9.f, 300.f, 2.5f);
	}
}

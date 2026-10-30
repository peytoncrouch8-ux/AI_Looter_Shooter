// AStoneSentinelCreature's rank (its traits kept for waking, Fleet taken out), hits (a shot wakes it; chips and a small
// jolt), death (the crumble begins) and coming back (a statue again). Split from StoneSentinelCreature.cpp by topic.

#include "Creatures/StoneSentinelCreature.h"
#include "Audio/LooterSound.h"
#include "Audio/LooterSoundCues.h"
#include "Audio/LooterSoundRules.h"
#include "Combat/HealthComponent.h"
#include "Creatures/CreatureKillLook.h"
#include "Creatures/CreatureTraitComponent.h"
#include "Creatures/GravemotherFX.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"

namespace
{
	/** A hit's jolt on stone (degrees for a full-strength hit): a statue barely rocks. */
	constexpr float JoltPitch = 4.f;
	constexpr float JoltRoll = 3.f;
	/** Its chip sound at most this often (s: one for a shotgun's blast). */
	constexpr double ChipSoundGap = 0.05;
}

void AStoneSentinelCreature::OnRankChanged()
{
	Super::OnRankChanged();
	// Never Fleet: a running statue is wrong, and its slow tread is the point. The trait after the cut stands in (the same
	// seed, one more drawn), so its tier keeps its count.
	OwnTraits.Reset();
	if (UCreatureTraitComponent* Traits = GetTraitComponent())
	{
		const TConstArrayView<ECreatureTrait> Rolled = Traits->GetTraits();
		const int32 Count = Rolled.Num();
		OwnTraits = TArray<ECreatureTrait>(Rolled.GetData(), Rolled.Num());
		if (OwnTraits.Contains(ECreatureTrait::Fleet))
		{
			const TArray<ECreatureTrait> More = UCreatureTraitComponent::RollTraits(Count + 1, GetRank() == ECreatureRank::Ethereal, *this,
				GetTierSeed());
			OwnTraits = StoneSentinelRules::WithoutFleet(More, Count);
		}
	}
	// A statue shows none (a ward's cage, a rally's ring would give it away); they come on as it wakes.
	ShowTraits(Phase == EStoneSentinelPhase::Waking || Phase == EStoneSentinelPhase::Awake);
	ApplyStoneDefense();
}

void AStoneSentinelCreature::ShowTraits(bool bShown)
{
	if (UCreatureTraitComponent* Traits = GetTraitComponent())
	{
		Traits->SetTraits(bShown ? TConstArrayView<ECreatureTrait>(OwnTraits) : TConstArrayView<ECreatureTrait>());
	}
}

void AStoneSentinelCreature::OnHurt(bool bCritical, const FVector& HitLocation)
{
	// A shot wakes a statue (the hunt for whoever fired follows from the hit), and stops one climbing back to sleep. A shot
	// that kills it outright only crumbles it.
	const bool bKilled = Health->GetHealth() <= 0.f;
	if (Phase == EStoneSentinelPhase::Dormant && !bKilled)
	{
		Wake(nullptr);
	}
	else if (Phase == EStoneSentinelPhase::Settling && !bKilled)
	{
		GetCharacterMovement()->SetMovementMode(MOVE_Walking);
		SetPassive(false);
		bShowsHealthTag = true;
		ShowTraits(true);
		SetPhase(EStoneSentinelPhase::Awake);
	}

	// Stone barely rocks: a small jolt away from the shot.
	const FCreatureHit& Hit = GetLastHit();
	const FVector Way = Hit.Direction.IsNearlyZero() ? -GetActorForwardVector() : Hit.Direction;
	const float Strength = FMath::Clamp(0.4f + Hit.BurstShare * 3.f, 0.4f, 1.5f) * (bCritical ? 1.4f : 1.f);
	const FVector Local = GetActorQuat().UnrotateVector(Way);
	Jolt.X -= static_cast<float>(Local.X) * JoltPitch * Strength;
	Jolt.Y += static_cast<float>(Local.Y) * JoltRoll * Strength;

	// Chips off where it landed, flung back toward the shooter; grit and a spark or two; its chip sound once a burst.
	const FVector Back = -Way.GetSafeNormal();
	ThrowChips(HitLocation, Back, StoneSentinelRules::ChipsFor(bCritical, Hit.BurstShare, Tuning), 320.f, 3.f, 8.f);
	if (UGravemotherFX* Effects = UGravemotherFX::Get(this))
	{
		Effects->Chunks(EGraveChunk::StoneChip, HitLocation, Back, bCritical ? 4 : 2, 380.f, 0.6f);
		Effects->Sparks(HitLocation, Back, Way, bCritical ? 5 : 2);
	}
	StoneDust(HitLocation, bCritical ? 3 : 2, 30.f, 10.f);
	const UWorld* World = GetWorld();
	const double Now = World ? World->GetTimeSeconds() : 0.0;
	if (Now - LastChipSound >= ChipSoundGap)
	{
		LastChipSound = Now;
		LooterSound::PlayAt(this, FName(LooterSoundCue::Sentinel::Chip), HitLocation, bCritical ? 1.f : 0.8f, LooterSoundRules::PitchForSize(GetSizeScale()));
	}
}

void AStoneSentinelCreature::OnDied()
{
	// It sags and cracks through (TickCrumbling), then bursts into stone (Crumble): the cracking now, the burst after.
	SetPhase(EStoneSentinelPhase::Crumbling);
	bCrumbled = false;
	const FVector Chest = GetActorLocation() + FVector(0.0, 0.0, 40.0 * GetSizeScale());
	LooterSound::PlayAt(this, FName(LooterSoundCue::Sentinel::Crumble), Chest, 1.f, LooterSoundRules::PitchForSize(GetSizeScale()));
	ThrowChips(Chest, FVector::UpVector, 5, 250.f, 8.f, 16.f);
	StoneDust(Chest, 4, 70.f, 40.f);
	// The kill's own layers (its tier's rings and star, a crit's flare); no splat: stone leaves its pieces, not a pool.
	CreatureKillLook::Play(*this, GetLastHit(), /*bSplat*/ false);
}

void AStoneSentinelCreature::OnRespawned()
{
	// Whole again on its plinth: a statue, as it began.
	GetMesh()->SetVisibility(true);
	if (bStandIn)
	{
		StandInBody->SetVisibility(true);
		StandInHead->SetVisibility(true);
	}
	bCrumbled = false;
	Jolt = FVector2D::ZeroVector;
	Chips.Reset();
	DrawChips();
	PoseNow = StoneSentinelRules::IdlePose();
	BecomeStatue();
}

void AStoneSentinelCreature::PlayEntrance()
{
	// Back on its plinth in view (a respawn): a settle of stone and dust, as if it had always stood there and someone looked away.
	StoneDust(GetFeet(), 5, 80.f, 50.f);
	LooterSound::PlayAt(this, FName(LooterSoundCue::Sentinel::Step), GetFeet(), 0.7f, LooterSoundRules::PitchForSize(GetSizeScale()));
}

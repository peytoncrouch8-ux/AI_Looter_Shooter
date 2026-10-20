// UCreatureTraitComponent: Phasing, the Ethereal's own. Hit hard (a share of its health within a second), never mid-attack
// and every few seconds at most, it blinks aside of whoever hit it to ground it can stand on in their sight: a shimmer where
// it left (its sound, soul-light, a ring of sparks, an after-image), a streak of light to where it's going, a moment gone
// while light gathers where it'll be (the tell to turn the sights there), then a shimmer as it's back. Where it may land is
// the Unpaid phase-step's way: ground it can stand on near its own height, room for its body, no wall between, its
// hitter in sight, no safe zone, inside the playable area and its hunting ground.

#include "Creatures/CreatureTraitComponent.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Audio/LooterSoundRules.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/CreatureKnockComponent.h"
#include "Creatures/CreatureTraitRules.h"
#include "Creatures/CreatureTraitSettings.h"
#include "Creatures/EncounterSubsystem.h"
#include "Creatures/UnpaidCreature.h"
#include "Weapons/WeaponFX.h"
#include "World/PlayableArea.h"
#include "World/WorldQueries.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Components/CapsuleComponent.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"

namespace
{
	/** Ground steeper than about 50 degrees (the creatures' walkable floor angle) is no place to come back on. */
	constexpr double WalkableGroundNormalZ = 0.64;
	/** What stands on the ground (rocks, wagons, buildings) carries this tag for the minimap: it never lands on top of one. */
	const FName StandingObstacleTag(TEXT("Obstacle"));
	/** The room a spot needs is checked this far up from the ground (cm, at size 1), so a walkable slope doesn't touch it. */
	constexpr float RoomLift = 25.f;
	/** Where it sees its hitter from: about its eyes (a share of its half height over the ground). */
	constexpr float EyeShare = 1.6f;
	/** The streak and the after-image last this long (s). */
	constexpr float TrailSeconds = 0.3f;
	/** At most this many hits remembered at once (a shotgun's pellets each count). */
	constexpr int32 MaxRecentHits = 48;
	/** A hard hit taken where it can't blink (mid-attack, dazed) still blinks it if that's over within this long (s). */
	constexpr float PendingBlinkSeconds = 1.5f;
	/**
	 * Its way there and its hitter's sight of it are traced at least this high (cm): over a fence, a gravestone or a crate it
	 * blinks as a ghost would, but never through a wall or round a building's corner (PIE, 2026-10-20: the graveyard's fences
	 * refused every spot).
	 */
	constexpr float OverLowCover = 150.f;
}

void UCreatureTraitComponent::NoteHitForPhase(float Damage)
{
	if (RecentHits.Num() >= MaxRecentHits)
	{
		RecentHits.RemoveAt(0);
	}
	FRecentHit& Hit = RecentHits.AddDefaulted_GetRef();
	Hit.Time = Clock;
	Hit.Damage = Damage;
}

APawn* UCreatureTraitComponent::FindAttacker() const
{
	if (APawn* Last = LastAttacker.Get())
	{
		return Last;
	}
	const ACreatureBase* Creature = GetCreature();
	if (APawn* Hunted = Creature ? Creature->GetTarget() : nullptr)
	{
		return Hunted;
	}
	const UEncounterSubsystem* Encounters = UEncounterSubsystem::Get(this);
	return Encounters ? Encounters->GetPlayer() : nullptr;
}

void UCreatureTraitComponent::TickPhasing(float DeltaSeconds, FWeaponFX* Effects)
{
	ACreatureBase* Creature = GetCreature();
	if (!Creature)
	{
		return;
	}
	const FPhasingTraitTuning& Tuning = UCreatureTraitSettings::Get().Phasing;
	const FLinearColor Color = FLinearColor::FromSRGBColor(Tuning.Color);

	// The streak of light from where it left to where it went, and its after-image fading where it stood.
	TrailAge = FMath::Min(TrailAge + DeltaSeconds, 1000.f);
	if (Effects && TrailAge < TrailSeconds)
	{
		const float Fade = 1.f - TrailAge / TrailSeconds;
		Effects->AddStreak(TrailFrom, TrailTo, 3.f + 5.f * Fade, Color, 10.f * Fade);
		const float Half = BodyHalfHeight();
		const float Radius = BodyRadius();
		for (int32 Index = 0; Index < 4; ++Index)
		{
			const float Angle = UE_TWO_PI * Index / 4.f + Clock * 3.f;
			const FVector Off = FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * (Radius * 0.5f);
			Effects->AddStreak(TrailFrom + Off - FVector(0.0, 0.0, Half * 0.8f), TrailFrom + Off + FVector(0.0, 0.0, Half * 0.8f), 3.f, Color, 6.f * Fade);
		}
	}
	if (bBlinking)
	{
		StepBlink(DeltaSeconds, Effects);
		return;
	}
	if (!HasTrait(ECreatureTrait::Phasing))
	{
		return;
	}
	SinceBlink = FMath::Min(SinceBlink + DeltaSeconds, 1000.f);
	BlinkPendingLeft = FMath::Max(BlinkPendingLeft - DeltaSeconds, 0.f);
	const float Window = Tuning.HardHitWindow;
	const float Now = Clock;
	RecentHits.RemoveAll([Now, Window](const FRecentHit& Hit) { return Now - Hit.Time > Window; });
	if (Creature->IsDead() || SinceBlink < Tuning.Cooldown)
	{
		return;
	}
	float Recent = 0.f;
	for (const FRecentHit& Hit : RecentHits)
	{
		Recent += Hit.Damage;
	}
	// A hard hit taken mid-attack is kept a moment: it blinks as the attack ends (a melee creature in a fight is nearly always
	// winding up or striking, and would otherwise never blink).
	const bool bHardNow = !RecentHits.IsEmpty() && CreatureTraitRules::IsHardHit(Recent, MaxHealthNow(), Tuning.HardHitShare);
	if (!bHardNow && BlinkPendingLeft <= 0.f)
	{
		return;
	}
	// Never mid-attack (its tell would vanish with it), never knocked or dazed, never in its own phase-step, never held back.
	const UCreatureKnockComponent* Knock = Creature->GetKnock();
	const AUnpaidCreature* Unpaid = Cast<AUnpaidCreature>(Creature);
	if (Creature->GetCreatureState() == ECreatureState::Attack || (Knock && Knock->HoldsBrain()) || (Unpaid && Unpaid->IsPhasing())
		|| Creature->IsPassive() || Creature->IsHidden())
	{
		if (bHardNow)
		{
			BlinkPendingLeft = PendingBlinkSeconds;
		}
		return;
	}
	FVector To;
	if (!FindBlinkSpot(To))
	{
		// Nowhere to go from here: it looks again in a moment (the hits still count while they're recent).
		SinceBlink = FMath::Max(Tuning.Cooldown - Tuning.RetrySeconds, 0.f);
		return;
	}
	StartBlink(To);
}

bool UCreatureTraitComponent::FindBlinkSpot(FVector& OutFeet) const
{
	const ACreatureBase* Creature = GetCreature();
	UWorld* World = GetWorld();
	const UCapsuleComponent* Capsule = Creature ? Creature->GetCapsuleComponent() : nullptr;
	if (!World || !Capsule)
	{
		return false;
	}
	const FPhasingTraitTuning& Tuning = UCreatureTraitSettings::Get().Phasing;
	const float HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
	const float Scale = Creature->GetSizeScale();
	const FVector Here = Creature->GetActorLocation();
	const FVector FeetHere = Here - FVector(0.0, 0.0, HalfHeight);
	const APawn* Attacker = FindAttacker();
	FVector Away = Attacker ? Here - Attacker->GetActorLocation() : Creature->GetActorForwardVector();
	Away.Z = 0.0;
	if (!Away.Normalize())
	{
		Away = FVector::ForwardVector;
	}
	const FVector AttackerEye = Attacker ? Attacker->GetPawnViewLocation() : FVector::ZeroVector;

	// The ground and the walls are the world's solid shapes only (volumes skipped, as every trace for real geometry); the
	// room it needs sees the trees and rocks too, and never itself or its hitter.
	const FCollisionQueryParams GroundParams = LooterWorld::StaticGeometryParams(World, TEXT("TraitBlinkGround"), Creature);
	FCollisionQueryParams SightParams = LooterWorld::StaticGeometryParams(World, TEXT("TraitBlinkSight"), Creature, false);
	FCollisionQueryParams RoomParams(SCENE_QUERY_STAT(TraitBlinkRoom), false, Creature);
	if (Attacker)
	{
		SightParams.AddIgnoredActor(Attacker);
		RoomParams.AddIgnoredActor(Attacker);
	}
	const FCollisionObjectQueryParams Solid(ECC_WorldStatic);
	const FCollisionShape Room = FCollisionShape::MakeCapsule(Capsule->GetScaledCapsuleRadius(), HalfHeight);
	const UEncounterSubsystem* Encounters = UEncounterSubsystem::Get(this);
	const APlayableArea* Playable = APlayableArea::Find(World);
	const float Reach = Tuning.MaxRise + 50.f;
	const FVector Home = Creature->GetHome().GetLocation();

	for (const FVector& Candidate : CreatureTraitRules::BlinkCandidates(FeetHere, Away, bRightFirst, Tuning))
	{
		// Ground under it near its own level, walkable, and not the top of a rock, a wagon or a roof.
		FHitResult Hit;
		if (!World->LineTraceSingleByObjectType(Hit, Candidate + FVector(0.0, 0.0, Reach), Candidate - FVector(0.0, 0.0, Reach), Solid, GroundParams)
			|| Hit.bStartPenetrating)
		{
			continue;
		}
		const AActor* Under = Hit.GetActor();
		const FVector Ground = Hit.ImpactPoint;
		if (Hit.ImpactNormal.Z < WalkableGroundNormalZ || (Under && Under->ActorHasTag(StandingObstacleTag))
			|| FMath::Abs(Ground.Z - FeetHere.Z) > Tuning.MaxRise)
		{
			continue;
		}
		// Not onto its hitter, and (for the tests' sake as much as play's) never off its ground or into a safe zone.
		if ((Attacker && FVector::Dist2D(Ground, Attacker->GetActorLocation()) < Tuning.NearestToAttacker)
			|| (Encounters && Encounters->IsInSafeZone(Ground)) || (Playable && !Playable->Contains(Ground))
			|| !Creature->HuntingGround.Contains(Ground, Home))
		{
			continue;
		}
		// Room for its body: never inside a wall, a rock, a tree or another creature.
		const FVector Middle = Ground + FVector(0.0, 0.0, HalfHeight + RoomLift * Scale);
		if (World->OverlapBlockingTestByChannel(Middle, FQuat::Identity, ECC_Pawn, Room, RoomParams))
		{
			continue;
		}
		// Never through a wall: the way from where it stands to there is open over low cover...
		const float Over = FMath::Max(HalfHeight * 2.f, OverLowCover);
		if (World->LineTraceTestByObjectType(FeetHere + FVector(0.0, 0.0, Over), Ground + FVector(0.0, 0.0, Over), Solid, SightParams))
		{
			continue;
		}
		// ...and whoever hit it can see where it comes back (the gathering light is the tell).
		const float Eye = FMath::Max(HalfHeight * EyeShare, Over);
		if (Attacker && World->LineTraceTestByObjectType(Ground + FVector(0.0, 0.0, Eye), AttackerEye, Solid, SightParams))
		{
			continue;
		}
		OutFeet = Ground;
		return true;
	}
	UE_LOG(LogLooter, Verbose, TEXT("%s: nowhere to blink to from %s (Phasing)"), *Creature->GetName(), *Here.ToCompactString());
	return false;
}

void UCreatureTraitComponent::StartBlink(const FVector& ToFeet)
{
	ACreatureBase* Creature = GetCreature();
	if (!Creature || bBlinking)
	{
		return;
	}
	bBlinking = true;
	BlinkTime = 0.f;
	SinceBlink = 0.f;
	BlinkPendingLeft = 0.f;
	RecentHits.Reset();
	++BlinkCount;
	// The next one tries the other side first: it weaves.
	bRightFirst = !bRightFirst;

	const FVector OldLocation = Creature->GetActorLocation();
	TrailFrom = BodyCenter();
	Shimmer(TrailFrom, false);

	// Gone from sight: it moves while unseen, and comes back facing whoever hit it.
	const float HalfHeight = BodyHalfHeight();
	double Yaw = Creature->GetActorRotation().Yaw;
	if (const APawn* Attacker = FindAttacker())
	{
		Yaw = (Attacker->GetActorLocation() - ToFeet).Rotation().Yaw;
	}
	Creature->SetActorHiddenInGame(true);
	Creature->SetActorLocationAndRotation(ToFeet + FVector(0.0, 0.0, HalfHeight + 2.f), FRotator(0.0, Yaw, 0.0), false, nullptr,
		ETeleportType::TeleportPhysics);
	if (UCharacterMovementComponent* Movement = Creature->GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
	}
	TrailTo = Creature->GetActorLocation() + (TrailFrom - OldLocation);
	TrailAge = 0.f;
	UpdateTickEnabled();
	UE_LOG(LogLooter, Verbose, TEXT("%s blinks %.1f m aside (Phasing)"), *Creature->GetName(), FVector::Dist2D(OldLocation, Creature->GetActorLocation()) / 100.0);
}

void UCreatureTraitComponent::StepBlink(float DeltaSeconds, FWeaponFX* Effects)
{
	const ACreatureBase* Creature = GetCreature();
	if (!Creature)
	{
		return;
	}
	const FPhasingTraitTuning& Tuning = UCreatureTraitSettings::Get().Phasing;
	BlinkTime += DeltaSeconds;
	if (Effects)
	{
		// Light gathering where it'll be back, growing and drawing motes in: where to turn the sights.
		const FLinearColor Color = FLinearColor::FromSRGBColor(Tuning.Color);
		const float Share = FMath::Clamp(BlinkTime / FMath::Max(Tuning.GoneSeconds, 0.01f), 0.f, 1.f);
		const FVector At = BodyCenter();
		const float Radius = BodyRadius();
		Effects->AddGlow(At, Radius * (0.4f + 1.2f * Share), Color, 4.f + 14.f * Share);
		for (int32 Index = 0; Index < 2; ++Index)
		{
			const FVector From = At + Random.GetUnitVector() * (Radius * 1.6f);
			Sparks.Spawn(From, (At - From) * 4.f, 0.22f, 3.f, Color, 10.f, 0.f, 0.f);
		}
	}
	if (BlinkTime >= Tuning.GoneSeconds)
	{
		Arrive();
	}
}

void UCreatureTraitComponent::Arrive()
{
	if (!bBlinking)
	{
		return;
	}
	bBlinking = false;
	ACreatureBase* Creature = GetCreature();
	if (!Creature)
	{
		return;
	}
	Creature->SetActorHiddenInGame(false);
	// Its body set up afresh where it stands (feet planted under it, a shroud hanging still), not stretched from where it was.
	Creature->SettleBody();
	Shimmer(BodyCenter(), true);
	UpdateTickEnabled();
}

void UCreatureTraitComponent::EndBlink()
{
	if (!bBlinking)
	{
		return;
	}
	// Cut short (a death, its traits taken, its removal): shown where it is, with no shimmer.
	bBlinking = false;
	if (ACreatureBase* Creature = GetCreature())
	{
		Creature->SetActorHiddenInGame(false);
		Creature->SettleBody();
	}
}

void UCreatureTraitComponent::Shimmer(const FVector& Center, bool bArriving)
{
	const ACreatureBase* Creature = GetCreature();
	if (!Creature)
	{
		return;
	}
	const FLinearColor Color = FLinearColor::FromSRGBColor(UCreatureTraitSettings::Get().Phasing.Color);
	const float Size = FMath::Max(Creature->GetSizeScale(), 0.3f);
	LooterSound::PlayAt(Creature, LooterSoundCue::Tier::TraitPhase, Center, bArriving ? 0.6f : 1.f, LooterSoundRules::PitchForSize(Creature->GetSizeScale()) * (bArriving ? 1.12f : 1.f));
	if (FWeaponFX* Effects = GetEffects())
	{
		Effects->SpawnDeathBurst(EDeathBurst::SoulLight, Center, FVector::UpVector, 70.f * Size, Color);
	}
	// A ring of sparks thrown out level round it.
	const float Radius = BodyRadius();
	for (int32 Index = 0; Index < 14; ++Index)
	{
		const float Angle = UE_TWO_PI * Index / 14.f;
		const FVector Out(FMath::Cos(Angle), FMath::Sin(Angle), 0.f);
		Sparks.Spawn(Center + Out * (Radius * 0.6f), Out * Random.FRandRange(180.f, 320.f) + FVector(0.0, 0.0, Random.FRandRange(40.f, 120.f)),
			Random.FRandRange(0.35f, 0.55f), 3.f, Color, 13.f, 200.f, 2.f);
	}
}

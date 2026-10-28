// UFightDirectorSubsystem: where the waiting ones stand. Melee creatures hold slots on a ring round the player (spread by
// bearing, kept off the player's front where it can, turning slowly, feinting in now and then); ranged ones hold a spot out in
// their band, off the melee pack's side, with a clear line at the player. FightDirectorSubsystem.cpp has the turns.

#include "Creatures/FightDirectorSubsystem.h"
#include "AI_Looter_Shooter.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/PackRules.h"
#include "World/WorldQueries.h"
#include "CollisionQueryParams.h"
#include "Components/CapsuleComponent.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"

namespace
{
	/** A ring slot's ground is at most this far above or below the target's feet (cm): a slot on a ledge or down a drop is shut. */
	constexpr float RingGroundRise = 160.f;
	/** A ranged spot's ground may be this far above or below them (cm): a bank or a rise is a good place to spit from. */
	constexpr float RangedGroundRise = 320.f;
	/** A ranged spot sees its target from this high over its ground (cm): about where the lob leaves. */
	constexpr float RangedEyeHeight = 120.f;
	/** At most this many ranged spots are looked at (two traces each) when one is chosen. */
	constexpr int32 RangedTries = 4;
	/** A ranged spot is chosen again (staying put if it can) once the melee pack has come within this share of the off angle of it. */
	constexpr float RangedCrowdedShare = 0.5f;
	/** A pawn without a capsule stands this high (cm) over its feet. */
	constexpr float DefaultHalfHeight = 88.f;

	FVector BearingDirection(float Degrees)
	{
		const float Radians = FMath::DegreesToRadians(Degrees);
		return FVector(FMath::Cos(Radians), FMath::Sin(Radians), 0.f);
	}

	float HalfHeightOf(const APawn& Pawn)
	{
		const ACharacter* Character = Cast<ACharacter>(&Pawn);
		return Character && Character->GetCapsuleComponent() ? Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : DefaultHalfHeight;
	}

	/** The world's solid shapes (terrain, buildings, rocks placed as meshes) between two points, volumes skipped. */
	bool IsLineClear(const UWorld& World, const FVector& From, const FVector& To, const AActor* Ignored)
	{
		const FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(&World, TEXT("FightPlace"), Ignored, false);
		return !World.LineTraceTestByObjectType(From, To, FCollisionObjectQueryParams(ECC_WorldStatic), Params);
	}

	/** Ground under Point (looking from Above over it to Below under it), world-static only. */
	bool FindGroundUnder(const UWorld& World, const FVector& Point, float Above, float Below, const AActor* Ignored, FVector& OutGround)
	{
		const FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(&World, TEXT("FightPlaceGround"), Ignored, false);
		FHitResult Hit;
		if (World.LineTraceSingleByObjectType(Hit, Point + FVector(0.f, 0.f, Above), Point - FVector(0.f, 0.f, Below),
			FCollisionObjectQueryParams(ECC_WorldStatic), Params))
		{
			OutGround = Hit.ImpactPoint;
			return true;
		}
		return false;
	}
}

float UFightDirectorSubsystem::GetRingReach(const ACreatureBase& Creature) const
{
	const FFightTuning& Numbers = const_cast<UFightDirectorSubsystem*>(this)->Tuning();
	return FightRules::RingReach(Numbers.RingRadius, Creature.GetAttackRange(), Numbers.RingMargin);
}

float UFightDirectorSubsystem::GetApproachDistance(const ACreatureBase& Creature) const
{
	const FFightTuning& Numbers = const_cast<UFightDirectorSubsystem*>(this)->Tuning();
	return FightRules::ApproachDistance(GetRingReach(Creature), Numbers.RingApproach);
}

bool UFightDirectorSubsystem::GetWaitSpot(const ACreatureBase& Creature, const APawn& Target, FVector& OutSpot)
{
	RefreshIfDue();
	const FTargetFight* Fight = Fights.Find(&Target);
	const FPlace* Place = Fight ? Fight->Places.Find(Creature.GetUniqueID()) : nullptr;
	if (!Place)
	{
		return false;
	}
	const FFightTuning& Numbers = Tuning();
	const FVector Center = Target.GetActorLocation();
	const EFightRole Role = Creature.GetFightRole();
	float Bearing = 0.f;
	float Distance = 0.f;
	if (Role == EFightRole::Ranged && Place->bHasSpot)
	{
		Bearing = Place->SpotBearing;
		Distance = Place->SpotDistance;
	}
	else if (Role == EFightRole::Melee && Fight->bRing && Place->Slot != INDEX_NONE)
	{
		// The ring has turned on since the last look; a feint brings it in and back out.
		const double Time = Now();
		const float Turn = Fight->RingTurn + Numbers.RingDrift * static_cast<float>(FMath::Max(Time - LastRefresh, 0.0));
		Bearing = FightRules::SlotBearing(Place->Slot, Fight->Slots, Turn);
		Distance = FightRules::FeintRadius(Place->Radius, static_cast<float>(Time - Place->FeintStart), Numbers.FeintSeconds, Numbers.FeintDepth);
	}
	else
	{
		return false;
	}
	OutSpot = Center + BearingDirection(Bearing) * Distance;
	OutSpot.Z = Creature.GetActorLocation().Z;
	return true;
}

void UFightDirectorSubsystem::RequestNewSpot(const ACreatureBase& Creature)
{
	const uint32 Id = Creature.GetUniqueID();
	for (TPair<TWeakObjectPtr<const APawn>, FTargetFight>& Each : Fights)
	{
		if (FPlace* Place = Each.Value.Places.Find(Id))
		{
			Place->bRelocate = true;
		}
	}
}

// ---------------------------------------------------------------------------
// The ring
// ---------------------------------------------------------------------------

void UFightDirectorSubsystem::PlaceRing(const APawn& Target, FTargetFight& Fight, const TArray<ACreatureBase*>& Melee, double Time)
{
	const FFightTuning& Numbers = Tuning();
	Fight.bRing = FightRules::UsesRing(Melee.Num(), Numbers.RingMinPack);
	if (!Fight.bRing)
	{
		// A pair (or one) just comes in, flanking as before.
		for (TPair<uint32, FPlace>& Each : Fight.Places)
		{
			Each.Value.Slot = INDEX_NONE;
		}
		return;
	}
	const FVector Center = Target.GetActorLocation();
	const uint32 Open = OpenSlots(Target, Fight.RingTurn, Fight.Slots, Numbers.RingRadius);

	// Every melee one asks, those with a turn too: they come back to their slot once their attack is over.
	TArray<FightRules::FRingAsk> Asks;
	Asks.Reserve(Melee.Num());
	for (const ACreatureBase* Creature : Melee)
	{
		FightRules::FRingAsk Ask;
		Ask.Bearing = PackRules::BearingAround(Center, Creature->GetActorLocation());
		const FPlace* Place = Fight.Places.Find(Creature->GetUniqueID());
		Ask.CurrentSlot = Place ? Place->Slot : INDEX_NONE;
		// Never a slot off its own hunting ground (its spawner's), where it would only turn back at the edge.
		if (Creature->HuntingGround.IsSet())
		{
			Ask.Allowed = 0;
			const float Reach = GetRingReach(*Creature);
			for (int32 Slot = 0; Slot < Fight.Slots; ++Slot)
			{
				const FVector Spot = Center + BearingDirection(FightRules::SlotBearing(Slot, Fight.Slots, Fight.RingTurn)) * Reach;
				if (Creature->IsOnItsGround(Spot))
				{
					Ask.Allowed |= 1u << Slot;
				}
			}
		}
		Asks.Add(Ask);
	}
	FightRules::FRingLayout Layout;
	Layout.Slots = Fight.Slots;
	Layout.Turn = Fight.RingTurn;
	Layout.ViewYaw = static_cast<float>(Target.GetViewRotation().Yaw);
	Layout.BehindBias = Numbers.BehindBias;
	Layout.KeepBonus = Numbers.RingKeep;
	Layout.Open = Open;
	const TArray<int32> Slots = FightRules::AssignRingSlots(Asks, Layout);

	for (int32 Index = 0; Index < Melee.Num(); ++Index)
	{
		const ACreatureBase& Creature = *Melee[Index];
		const uint32 Id = Creature.GetUniqueID();
		FPlace& Place = Fight.Places.FindOrAdd(Id);
		Place.Slot = Slots[Index];
		Place.Radius = GetRingReach(Creature);
		// A waiting one feints in now and then (never mid-feint, never one with its turn: it's coming in for real).
		const bool bFeinting = Time - Place.FeintStart < Numbers.FeintSeconds;
		if (Place.Slot != INDEX_NONE && !bFeinting && !Fight.Melee.Holds(Id) && Rolls.FRand() < Numbers.FeintChance)
		{
			Place.FeintStart = Time;
		}
	}
}

uint32 UFightDirectorSubsystem::OpenSlots(const APawn& Target, float Turn, int32 Slots, float Radius) const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return MAX_uint32;
	}
	const FVector Center = Target.GetActorLocation();
	const float HalfHeight = HalfHeightOf(Target);
	const float Feet = static_cast<float>(Center.Z) - HalfHeight;
	uint32 Open = 0;
	for (int32 Slot = 0; Slot < FMath::Min(Slots, 32); ++Slot)
	{
		const FVector Spot = Center + BearingDirection(FightRules::SlotBearing(Slot, Slots, Turn)) * Radius;
		// Walled off from the target (a building, a mine wall, a cliff face): no way in from there.
		if (!IsLineClear(*World, Center, Spot, &Target))
		{
			continue;
		}
		// Ground near the target's own level: never a slot over a drop or up on a ledge.
		FVector Ground;
		if (!FindGroundUnder(*World, Spot, 60.f, HalfHeight + RingGroundRise + 100.f, &Target, Ground)
			|| FMath::Abs(static_cast<float>(Ground.Z) - Feet) > RingGroundRise)
		{
			continue;
		}
		Open |= 1u << Slot;
	}
	return Open;
}

// ---------------------------------------------------------------------------
// Ranged spots
// ---------------------------------------------------------------------------

void UFightDirectorSubsystem::PlaceRanged(const APawn& Target, FTargetFight& Fight, const TArray<ACreatureBase*>& Ranged,
	const TArray<ACreatureBase*>& Melee)
{
	if (Ranged.IsEmpty())
	{
		return;
	}
	const FFightTuning& Numbers = Tuning();
	const FVector Center = Target.GetActorLocation();
	TArray<float> MeleeBearings;
	MeleeBearings.Reserve(Melee.Num());
	for (const ACreatureBase* Creature : Melee)
	{
		MeleeBearings.Add(PackRules::BearingAround(Center, Creature->GetActorLocation()));
	}
	bool bHasMelee = false;
	const float MeleeBearing = FightRules::MeanBearing(MeleeBearings, bHasMelee);

	for (const ACreatureBase* Creature : Ranged)
	{
		float Near = 0.f;
		float Far = 0.f;
		if (!Creature->GetFightBand(Near, Far))
		{
			continue;
		}
		const uint32 Id = Creature->GetUniqueID();
		FPlace& Place = Fight.Places.FindOrAdd(Id);
		const bool bCrowded = Place.bHasSpot && bHasMelee
			&& FightRules::AngleApart(Place.SpotBearing, MeleeBearing) < Numbers.RangedOffAngle * RangedCrowdedShare;
		if (Place.bHasSpot && !Place.bRelocate && !bCrowded)
		{
			continue;
		}
		const bool bMustMove = Place.bHasSpot && Place.bRelocate;
		Place.bRelocate = false;

		// The other ranged ones' spots: two spitters spread round rather than stand together.
		TArray<float> Others;
		for (const TPair<uint32, FPlace>& Each : Fight.Places)
		{
			if (Each.Key != Id && Each.Value.bHasSpot)
			{
				Others.Add(Each.Value.SpotBearing);
			}
		}
		const FVector Here = Creature->GetActorLocation();
		const float Current = PackRules::BearingAround(Center, Here);
		const float Distance = FightRules::RangedDistance(static_cast<float>(FVector::Dist2D(Here, Center)), Near, Far);
		const TArray<float> Bearings = FightRules::RangedBearings(Current, bHasMelee, MeleeBearing, Numbers.RangedOffAngle, bMustMove, Others);
		int32 Tries = 0;
		bool bFound = false;
		for (const float Bearing : Bearings)
		{
			if (Tries >= RangedTries)
			{
				break;
			}
			const FVector Spot = Center + BearingDirection(Bearing) * Distance;
			if (Creature->HuntingGround.IsSet() && !Creature->HuntingGround.Contains(Spot, Creature->GetHome().GetLocation()))
			{
				continue;
			}
			++Tries;
			if (IsRangedSpotUsable(Target, Spot))
			{
				Place.bHasSpot = true;
				Place.SpotBearing = Bearing;
				Place.SpotDistance = Distance;
				bFound = true;
				break;
			}
		}
		// Nowhere better: one that was to move on stays where it was; one with no spot keeps its distance its own way.
		if (!bFound && !bMustMove)
		{
			Place.bHasSpot = false;
		}
	}
}

bool UFightDirectorSubsystem::IsRangedSpotUsable(const APawn& Target, const FVector& Spot) const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}
	const FVector Center = Target.GetActorLocation();
	const float Feet = static_cast<float>(Center.Z) - HalfHeightOf(Target);
	FVector Ground;
	// Looked for from a little over the target's middle, not over its reach: under a roof (the mine) the roof's top isn't ground.
	if (!FindGroundUnder(*World, Spot, 200.f, RangedGroundRise + HalfHeightOf(Target), &Target, Ground)
		|| FMath::Abs(static_cast<float>(Ground.Z) - Feet) > RangedGroundRise)
	{
		return false;
	}
	// A clear line at them from about where its lob leaves: a lob clears low cover too, but never a wall or a roof.
	return IsLineClear(*World, Ground + FVector(0.f, 0.f, RangedEyeHeight), Center, &Target);
}

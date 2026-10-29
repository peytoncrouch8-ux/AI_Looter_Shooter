// UFightDirectorSubsystem: the pressure on a player (FIGHTS.md section 2, 2026-10-28: the user's "clearly harder" =
// Borderlands-normal). A kind family's melee turns spaced by its melee gap; what the director reads of each player at every
// look (standing still, hidden from every fighter, how hurt) and the close-call ease that turns a death into a near-death;
// one leap at a target at a time; the packs' leaders and the lull after one falls; and the fight's moments for the fight log.

#include "Creatures/FightDirectorSubsystem.h"
#include "AI_Looter_Shooter.h"
#include "Combat/HealthComponent.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/FightEvent.h"
#include "Creatures/FightPressureRules.h"
#include "Creatures/FightTuning.h"
#include "Creatures/PackLeaderRules.h"
#include "Shields/PlayerShieldComponent.h"
#include "World/WorldQueries.h"
#include "CollisionQueryParams.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"

namespace
{
	/** At most this many fighters' eye lines are traced at a player a look (the first clear one ends it). */
	constexpr int32 HiddenTraceMax = 12;
	/** A fighter's eye this share of its half height over its middle. */
	constexpr float EyeShare = 0.5f;
	/** A leader's fall is kept this long (s), and at most this many at once: longer than any lull. */
	constexpr double LeaderDownKeep = 60.0;
	constexpr int32 LeaderDownsKept = 8;
	/** GetSinceLeaderDown with no fall to tell of. */
	constexpr float NeverSeconds = 1.0e6f;

	/** The player's health and ward together, against their most. */
	float HealthShareOf(const APawn& Player)
	{
		const UHealthComponent* Life = Player.FindComponentByClass<UHealthComponent>();
		const UPlayerShieldComponent* Ward = Player.FindComponentByClass<UPlayerShieldComponent>();
		return FightPressureRules::HealthShare(Life ? Life->GetHealth() : 0.f, Life ? Life->GetMaxHealth() : 0.f,
			Ward ? Ward->GetCharge() : 0.f, Ward ? Ward->GetCapacity() : 0.f);
	}
}

// ---------------------------------------------------------------------------
// The melee gap
// ---------------------------------------------------------------------------

float UFightDirectorSubsystem::GetMeleeGapFor(const ACreatureBase& Creature)
{
	return FMath::Max(Creature.GetMeleeGap(), 0.f) * Tuning().MeleeGap;
}

// ---------------------------------------------------------------------------
// The player read and the close-call ease
// ---------------------------------------------------------------------------

FFightPlayerRead UFightDirectorSubsystem::GetPlayerRead(const APawn& Target)
{
	RefreshIfDue();
	const FTargetFight* Fight = Fights.Find(&Target);
	if (!Fight)
	{
		return FFightPlayerRead();
	}
	FFightPlayerRead Read = Fight->Read;
	Read.bEasing = Fight->Ease.IsEasing(Now());
	return Read;
}

void UFightDirectorSubsystem::ReadPlayer(const APawn& Target, FTargetFight& Fight, const TArray<ACreatureBase*>& Fighters, double Time)
{
	const FFightTuning& Numbers = Tuning();
	FFightPlayerRead& Read = Fight.Read;
	Read.Fighters = Fighters.Num();
	Read.HealthShare = HealthShareOf(Target);
	if (Fighters.IsEmpty())
	{
		// Out of the fight: the next one starts its watches afresh.
		Fight.Still.Reset();
		Fight.Sight.Reset();
		Read.StillFor = 0.f;
		Read.HiddenFor = 0.f;
		Read.bCamping = false;
		Read.bEasing = Fight.Ease.IsEasing(Time);
		return;
	}
	Fight.Still.Note(Target.GetActorLocation(), Time, Numbers.StillRadius);
	Fight.Sight.Note(AnyFighterSees(Target, Fighters), Time);
	Read.StillFor = Fight.Still.StillFor(Time);
	Read.HiddenFor = Fight.Sight.HiddenFor(Time);
	Read.bCamping = FightPressureRules::IsCamping(Read.StillFor, Read.HiddenFor, Read.Fighters, Numbers.StillSeconds, Numbers.HiddenSeconds,
		Numbers.CampMinFighters);
	// A nearly dead player (not a dead one) gets a few seconds of one turn fewer: the next bite is likelier a near miss.
	if (Read.HealthShare > 0.f
		&& Fight.Ease.TryOpen(Read.HealthShare, Time, Numbers.Ease != 0, Numbers.EaseBelow, Numbers.EaseSeconds, Numbers.EaseGap))
	{
		UE_LOG(LogLooter, Log, TEXT("%s is nearly down (%.0f%%): the fight eases off for %.1f s."), *Target.GetName(), Read.HealthShare * 100.f,
			Numbers.EaseSeconds);
		NoteEvent(*Fighters[0], EFightEvent::Ease, Read.HealthShare);
	}
	Read.bEasing = Fight.Ease.IsEasing(Time);
}

bool UFightDirectorSubsystem::AnyFighterSees(const APawn& Target, const TArray<ACreatureBase*>& Fighters) const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return true;
	}
	// The world's solid shapes only, as a strike's check (volumes skipped: the scatter's PCG volume once answered every
	// sight trace in Ransom's Rest); bodies aren't world-static, so another creature never hides the player.
	const FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(World, TEXT("FightPlayerRead"), &Target, false);
	const FCollisionObjectQueryParams Solid(ECC_WorldStatic);
	const FVector Eye = Target.GetPawnViewLocation();
	const int32 Count = FMath::Min(Fighters.Num(), HiddenTraceMax);
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const ACreatureBase* Fighter = Fighters[Index];
		const FVector From = Fighter->GetActorLocation() + FVector(0.f, 0.f, Fighter->GetSimpleCollisionHalfHeight() * EyeShare);
		if (!World->LineTraceTestByObjectType(From, Eye, Solid, Params))
		{
			return true;
		}
	}
	return false;
}

// ---------------------------------------------------------------------------
// Leaps
// ---------------------------------------------------------------------------

bool UFightDirectorSubsystem::MayLeap(const ACreatureBase& Creature, const APawn& Target, float Gap) const
{
	const FTargetFight* Fight = Fights.Find(&Target);
	return !Fight || Fight->Leaps.MayLeap(Now(), Gap);
}

void UFightDirectorSubsystem::NoteLeap(const ACreatureBase& Creature, const APawn& Target)
{
	Fights.FindOrAdd(&Target).Leaps.Note(Now());
}

// ---------------------------------------------------------------------------
// Pack leaders
// ---------------------------------------------------------------------------

void UFightDirectorSubsystem::NoteLeader(const ACreatureBase& Leader)
{
	Leaders.RemoveAll([](const TWeakObjectPtr<const ACreatureBase>& Each) { return !Each.IsValid(); });
	Leaders.AddUnique(&Leader);
}

bool UFightDirectorSubsystem::IsPackLed(const ACreatureBase& Member) const
{
	const float Reach = const_cast<UFightDirectorSubsystem*>(this)->Tuning().LeaderReach;
	const double ReachSquared = FMath::Square(static_cast<double>(Reach));
	for (const TWeakObjectPtr<const ACreatureBase>& Each : Leaders)
	{
		// A leader that comes back (a placed one's new life) leads again.
		const ACreatureBase* Leader = Each.Get();
		if (Leader && !Leader->IsDead() && (Leader == &Member || Member.SharesPackWith(*Leader))
			&& FVector::DistSquared(Leader->GetActorLocation(), Member.GetActorLocation()) <= ReachSquared)
		{
			return true;
		}
	}
	return false;
}

void UFightDirectorSubsystem::NoteLeaderDown(const ACreatureBase& Leader)
{
	const double Time = Now();
	LeaderDowns.RemoveAll([Time](const FLeaderDown& Each) { return Time - Each.Time > LeaderDownKeep; });
	if (LeaderDowns.Num() >= LeaderDownsKept)
	{
		LeaderDowns.RemoveAt(0);
	}
	FLeaderDown Down;
	Down.PackTag = Leader.PackTag;
	Down.Class = Leader.GetClass();
	Down.Where = Leader.GetActorLocation();
	Down.Time = Time;
	LeaderDowns.Add(Down);
	// Its pack's frenzy ends at once, at every player (a frenzy is a tagged pack's).
	if (!Leader.PackTag.IsNone())
	{
		for (TPair<TWeakObjectPtr<const APawn>, FTargetFight>& Each : Fights)
		{
			if (Each.Value.FrenzyPack == Leader.PackTag)
			{
				Each.Value.FrenzyUntil = FMath::Min(Each.Value.FrenzyUntil, Time);
			}
		}
	}
}

float UFightDirectorSubsystem::GetSinceLeaderDown(const ACreatureBase& Member) const
{
	const float Reach = const_cast<UFightDirectorSubsystem*>(this)->Tuning().LeaderReach;
	const double ReachSquared = FMath::Square(static_cast<double>(Reach));
	const double Time = Now();
	float Since = NeverSeconds;
	for (const FLeaderDown& Down : LeaderDowns)
	{
		if (PackLeaderRules::SamePack(Member.PackTag, Member.GetClass(), Down.PackTag, Down.Class.Get())
			&& FVector::DistSquared(Down.Where, Member.GetActorLocation()) <= ReachSquared)
		{
			Since = FMath::Min(Since, static_cast<float>(FMath::Max(Time - Down.Time, 0.0)));
		}
	}
	return Since;
}

bool UFightDirectorSubsystem::IsInLeaderLull(const ACreatureBase& Member) const
{
	return GetSinceLeaderDown(Member) < const_cast<UFightDirectorSubsystem*>(this)->Tuning().LeaderLull;
}

// ---------------------------------------------------------------------------
// The fight's moments (the fight log)
// ---------------------------------------------------------------------------

void UFightDirectorSubsystem::NoteEvent(const ACreatureBase& Creature, EFightEvent Event, float Value)
{
	const int32 Index = static_cast<int32>(Event);
	if (Index >= 0 && Index < NumFightEvents)
	{
		++EventCounts[Index];
	}
	OnFightEvent.Broadcast(Creature, Event, Value);
}

void UFightDirectorSubsystem::Note(const ACreatureBase& Creature, EFightEvent Event, float Value)
{
	if (UFightDirectorSubsystem* Director = Get(&Creature))
	{
		Director->NoteEvent(Creature, Event, Value);
	}
}

int32 UFightDirectorSubsystem::GetEventCount(EFightEvent Event) const
{
	const int32 Index = static_cast<int32>(Event);
	return Index >= 0 && Index < NumFightEvents ? EventCounts[Index] : 0;
}

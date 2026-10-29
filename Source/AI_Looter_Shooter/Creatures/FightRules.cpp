#include "Creatures/FightRules.h"
#include "UObject/Class.h"

namespace
{
	/** A staggered holder goes back in line this far ahead of anyone's real wait (s), keeping its order among such. */
	constexpr double HeadOfLine = 1.0e6;

	/** A ranged spot this near its own bearing (degrees) counts as staying put; a move after a shot goes at least this far. */
	constexpr float RangedMoveAtLeast = 30.f;
	/** Ranged spots within this (degrees) of another ranged one's crowd it. */
	constexpr float RangedCrowd = 40.f;
	/** Bearings this near (degrees) are walked to straight, not round the circle. */
	constexpr float OrbitStraight = 10.f;
	/** Off the spot's distance by this much (cm) it closes on it at full share while it goes round. */
	constexpr float OrbitCloseOver = 200.f;

	float BearingOf(const FVector& Target, const FVector& Point)
	{
		return FMath::RadiansToDegrees(static_cast<float>(FMath::Atan2(Point.Y - Target.Y, Point.X - Target.X)));
	}
}

// ---------------------------------------------------------------------------
// FFightTokens
// ---------------------------------------------------------------------------

bool FFightTokens::TryTake(uint32 Id, double Now, int32 Max, float ForgetAfter, float MinGap, FName Family, float FamilyGap)
{
	if (Holds(Id))
	{
		return true;
	}
	if (IsResting(Id, Now))
	{
		return false;
	}
	Prune(Now, ForgetAfter);
	FWaiter* Mine = Waiters.FindByPredicate([Id](const FWaiter& Each) { return Each.Id == Id; });
	if (!Mine)
	{
		FWaiter Made;
		Made.Id = Id;
		Made.Since = Now;
		Mine = &Waiters.Add_GetRef(Made);
	}
	Mine->LastAsk = Now;
	Mine->Family = Family;
	Mine->FamilyGap = FamilyGap;
	if (Holders.Num() >= FMath::Max(Max, 1) || (MinGap > 0.f && Now - LastGiven < MinGap))
	{
		return false;
	}
	// The next free turn is the longest waiter's (ties: the lower id, so every asker sees the same order) of those whose kind
	// may begin one now: one held back by its family's gap doesn't hold up another kind behind it.
	const FWaiter* First = nullptr;
	for (const FWaiter& Each : Waiters)
	{
		if (IsFamilyOpen(Each, Now) && (!First || Each.Since < First->Since || (Each.Since == First->Since && Each.Id < First->Id)))
		{
			First = &Each;
		}
	}
	if (!First || First->Id != Id)
	{
		return false;
	}
	const bool bKept = First->bKept;
	Waiters.RemoveAll([Id](const FWaiter& Each) { return Each.Id == Id; });
	Holders.Add({ Id, Now });
	LastGiven = Now;
	if (!bKept && !Family.IsNone() && FamilyGap > 0.f)
	{
		FamilyLastGiven.Add(Family, Now);
	}
	return true;
}

bool FFightTokens::IsFamilyOpen(const FWaiter& Waiter, double Now) const
{
	if (Waiter.bKept || Waiter.FamilyGap <= 0.f || Waiter.Family.IsNone())
	{
		return true;
	}
	const double* Last = FamilyLastGiven.Find(Waiter.Family);
	return !Last || Now - *Last >= Waiter.FamilyGap;
}

bool FFightTokens::Holds(uint32 Id) const
{
	return Holders.ContainsByPredicate([Id](const FHolder& Each) { return Each.Id == Id; });
}

double FFightTokens::HeldSince(uint32 Id) const
{
	const FHolder* Holder = Holders.FindByPredicate([Id](const FHolder& Each) { return Each.Id == Id; });
	return Holder ? Holder->Since : -1.0;
}

bool FFightTokens::IsWaiting(uint32 Id) const
{
	return Waiters.ContainsByPredicate([Id](const FWaiter& Each) { return Each.Id == Id; });
}

bool FFightTokens::IsResting(uint32 Id, double Now) const
{
	const FRest* Rest = Rests.FindByPredicate([Id](const FRest& Each) { return Each.Id == Id; });
	return Rest && Rest->Until > Now;
}

bool FFightTokens::Release(uint32 Id, double Now, float RestSeconds)
{
	const bool bHeld = Holders.RemoveAll([Id](const FHolder& Each) { return Each.Id == Id; }) > 0;
	Waiters.RemoveAll([Id](const FWaiter& Each) { return Each.Id == Id; });
	if (RestSeconds > 0.f)
	{
		if (FRest* Rest = Rests.FindByPredicate([Id](const FRest& Each) { return Each.Id == Id; }))
		{
			Rest->Until = Now + RestSeconds;
		}
		else
		{
			Rests.Add({ Id, Now + RestSeconds });
		}
	}
	return bHeld;
}

bool FFightTokens::ReleaseKeepTurn(uint32 Id, double Now)
{
	const double Since = HeldSince(Id);
	if (Since < 0.0)
	{
		return false;
	}
	Holders.RemoveAll([Id](const FHolder& Each) { return Each.Id == Id; });
	Waiters.RemoveAll([Id](const FWaiter& Each) { return Each.Id == Id; });
	// Ahead of every real wait, in the order these turns were first taken; it leaves the line like anyone else once it has
	// stopped asking for ForgetAfter (a knock that lasts gives the turn to the next one).
	FWaiter Kept;
	Kept.Id = Id;
	Kept.Since = Since - HeadOfLine;
	Kept.LastAsk = Now;
	Kept.bKept = true;
	Waiters.Add(Kept);
	return true;
}

void FFightTokens::Forget(uint32 Id)
{
	Holders.RemoveAll([Id](const FHolder& Each) { return Each.Id == Id; });
	Waiters.RemoveAll([Id](const FWaiter& Each) { return Each.Id == Id; });
	Rests.RemoveAll([Id](const FRest& Each) { return Each.Id == Id; });
}

void FFightTokens::Prune(double Now, float ForgetAfter)
{
	const double Oldest = Now - FMath::Max(ForgetAfter, 0.f);
	Waiters.RemoveAll([Oldest](const FWaiter& Each) { return Each.LastAsk < Oldest; });
	Rests.RemoveAll([Now](const FRest& Each) { return Each.Until <= Now; });
}

// ---------------------------------------------------------------------------
// Who and how much
// ---------------------------------------------------------------------------

bool FightRules::KeepsOwnFight(ECreatureRank Rank, bool bBossRules)
{
	return bBossRules || Rank == ECreatureRank::Boss || Rank == ECreatureRank::Legendary;
}

float FightRules::ChaseSpeedFor(float ChaseSpeed, bool bOwnFight, float Scale)
{
	return bOwnFight ? ChaseSpeed : ChaseSpeed * FMath::Max(Scale, 0.f);
}

float FightRules::DamageScaleFor(bool bOwnFight, float Scale)
{
	return bOwnFight ? 1.f : FMath::Max(Scale, 0.f);
}

FName FightRules::FamilyOf(const UClass* Class)
{
	// The native class: a Blueprint child of a kind fights as its kind does, and so shares its kind's gap.
	while (Class && !Class->HasAnyClassFlags(CLASS_Native))
	{
		Class = Class->GetSuperClass();
	}
	return Class ? Class->GetFName() : NAME_None;
}

// ---------------------------------------------------------------------------
// The ring
// ---------------------------------------------------------------------------

bool FightRules::UsesRing(int32 MeleeHunters, int32 MinPack)
{
	return MeleeHunters >= FMath::Max(MinPack, 1);
}

float FightRules::RingReach(float RingRadius, float AttackRange, float Margin)
{
	return FMath::Max(RingRadius, AttackRange + FMath::Max(Margin, 0.f));
}

float FightRules::ApproachDistance(float RingReach, float ApproachShare)
{
	return RingReach * FMath::Max(ApproachShare, 1.f);
}

float FightRules::AngleApart(float A, float B)
{
	return FMath::Abs(FMath::UnwindDegrees(A - B));
}

float FightRules::SlotBearing(int32 Slot, int32 Slots, float Turn)
{
	return FMath::UnwindDegrees(Turn + 360.f * static_cast<float>(Slot) / static_cast<float>(FMath::Max(Slots, 1)));
}

float FightRules::Frontness(float Bearing, float ViewYaw)
{
	return 0.5f * (1.f + FMath::Cos(FMath::DegreesToRadians(Bearing - ViewYaw)));
}

TArray<int32> FightRules::AssignRingSlots(const TArray<FRingAsk>& Asks, const FRingLayout& Layout)
{
	const int32 Slots = FMath::Clamp(Layout.Slots, 1, 32);
	TArray<int32> Result;
	Result.Init(INDEX_NONE, Asks.Num());
	auto Cost = [&](int32 Ask, int32 Slot)
	{
		const float Bearing = SlotBearing(Slot, Slots, Layout.Turn);
		return AngleApart(Asks[Ask].Bearing, Bearing) / 180.f + Layout.BehindBias * Frontness(Bearing, Layout.ViewYaw)
			- (Asks[Ask].CurrentSlot == Slot ? Layout.KeepBonus : 0.f);
	};
	auto MayTake = [&](int32 Ask, int32 Slot)
	{
		const uint32 Bit = 1u << Slot;
		return (Layout.Open & Bit) != 0 && (Asks[Ask].Allowed & Bit) != 0;
	};

	struct FPair
	{
		float Cost;
		int32 Ask;
		int32 Slot;
	};
	TArray<FPair> Pairs;
	Pairs.Reserve(Asks.Num() * Slots);
	for (int32 Ask = 0; Ask < Asks.Num(); ++Ask)
	{
		for (int32 Slot = 0; Slot < Slots; ++Slot)
		{
			if (MayTake(Ask, Slot))
			{
				Pairs.Add({ Cost(Ask, Slot), Ask, Slot });
			}
		}
	}
	Pairs.Sort([](const FPair& A, const FPair& B)
	{
		if (A.Cost != B.Cost)
		{
			return A.Cost < B.Cost;
		}
		return A.Ask != B.Ask ? A.Ask < B.Ask : A.Slot < B.Slot;
	});
	TArray<int32> Occupants;
	Occupants.Init(0, Slots);
	for (const FPair& Pair : Pairs)
	{
		if (Result[Pair.Ask] == INDEX_NONE && Occupants[Pair.Slot] == 0)
		{
			Result[Pair.Ask] = Pair.Slot;
			Occupants[Pair.Slot] = 1;
		}
	}
	// More creatures than open slots: the rest double up, spread over the least crowded of the cheap ones.
	for (int32 Ask = 0; Ask < Asks.Num(); ++Ask)
	{
		if (Result[Ask] != INDEX_NONE)
		{
			continue;
		}
		float Best = TNumericLimits<float>::Max();
		for (int32 Slot = 0; Slot < Slots; ++Slot)
		{
			if (!MayTake(Ask, Slot))
			{
				continue;
			}
			const float Crowded = Cost(Ask, Slot) + 0.5f * static_cast<float>(Occupants[Slot]);
			if (Crowded < Best)
			{
				Best = Crowded;
				Result[Ask] = Slot;
			}
		}
		if (Result[Ask] != INDEX_NONE)
		{
			++Occupants[Result[Ask]];
		}
	}
	return Result;
}

float FightRules::FeintRadius(float Radius, float Since, float Seconds, float Depth)
{
	if (Since < 0.f || Seconds <= 0.f || Since >= Seconds)
	{
		return Radius;
	}
	// In and back out, quickest in the middle: a lunge that thinks better of it.
	const float In = FMath::Sin(UE_PI * Since / Seconds);
	return Radius * (1.f - (1.f - FMath::Clamp(Depth, 0.f, 1.f)) * In);
}

FVector FightRules::OrbitWay(const FVector& Here, const FVector& Target, const FVector& Spot)
{
	const FVector Out(Here.X - Target.X, Here.Y - Target.Y, 0.0);
	const FVector Direct = FVector(Spot.X - Here.X, Spot.Y - Here.Y, 0.0).GetSafeNormal();
	const double Distance = Out.Size();
	if (Distance < 1.0)
	{
		return Direct;
	}
	const float Delta = FMath::UnwindDegrees(BearingOf(Target, Spot) - BearingOf(Target, Here));
	if (FMath::Abs(Delta) < OrbitStraight)
	{
		return Direct;
	}
	const FVector Radial = Out / Distance;
	const FVector Round = FVector(-Radial.Y, Radial.X, 0.0) * (Delta > 0.f ? 1.0 : -1.0);
	const double SpotDistance = FVector::Dist2D(Spot, Target);
	const double Close = FMath::Clamp((SpotDistance - Distance) / OrbitCloseOver, -1.0, 1.0);
	return (Round + Radial * Close).GetSafeNormal();
}

// ---------------------------------------------------------------------------
// Ranged spots
// ---------------------------------------------------------------------------

float FightRules::MeanBearing(TConstArrayView<float> Bearings, bool& bOutAny)
{
	double SumSin = 0.0;
	double SumCos = 0.0;
	for (const float Bearing : Bearings)
	{
		SumSin += FMath::Sin(FMath::DegreesToRadians(static_cast<double>(Bearing)));
		SumCos += FMath::Cos(FMath::DegreesToRadians(static_cast<double>(Bearing)));
	}
	bOutAny = Bearings.Num() > 0 && FMath::Abs(SumSin) + FMath::Abs(SumCos) > UE_KINDA_SMALL_NUMBER;
	return bOutAny ? FMath::RadiansToDegrees(static_cast<float>(FMath::Atan2(SumSin, SumCos))) : 0.f;
}

TArray<float> FightRules::RangedBearings(float Current, bool bHasMelee, float MeleeBearing, float OffAngle, bool bMustMove,
	TConstArrayView<float> Others)
{
	TArray<float> Candidates;
	if (bHasMelee)
	{
		if (AngleApart(Current, MeleeBearing) >= OffAngle)
		{
			Candidates.Add(Current);
		}
		for (const float Extra : { 0.f, 30.f, 60.f })
		{
			Candidates.Add(FMath::UnwindDegrees(MeleeBearing + OffAngle + Extra));
			Candidates.Add(FMath::UnwindDegrees(MeleeBearing - OffAngle - Extra));
		}
	}
	else
	{
		for (const float Offset : { 0.f, 35.f, -35.f, 70.f, -70.f })
		{
			Candidates.Add(FMath::UnwindDegrees(Current + Offset));
		}
	}
	auto Score = [&](float Bearing)
	{
		const float Moved = AngleApart(Bearing, Current);
		float Value = Moved / 180.f;
		if (bMustMove && Moved < RangedMoveAtLeast)
		{
			Value += 2.f;
		}
		for (const float Other : Others)
		{
			// Two spitters side by side are one threat: worth going a good way round to be another.
			if (AngleApart(Bearing, Other) < RangedCrowd)
			{
				Value += 1.f;
			}
		}
		return Value;
	};
	Candidates.StableSort([&Score](float A, float B) { return Score(A) < Score(B); });
	if (bMustMove)
	{
		// Moving on means moving: one that would stay put is no choice at all.
		Candidates.RemoveAll([Current](float Bearing) { return AngleApart(Bearing, Current) < RangedMoveAtLeast; });
	}
	return Candidates;
}

float FightRules::RangedDistance(float Current, float Near, float Far)
{
	const float Low = FMath::Min(Near, Far);
	const float High = FMath::Max(Near, Far);
	const float Band = High - Low;
	return FMath::Clamp(Current, Low + 0.15f * Band, High - 0.1f * Band);
}

// ---------------------------------------------------------------------------
// A held turn's upkeep
// ---------------------------------------------------------------------------

FightRules::ETokenCheck FightRules::CheckHolder(const FHolderLook& Look, float MaxSeconds)
{
	if (!Look.bFighting)
	{
		return ETokenCheck::Return;
	}
	if (Look.bHeldByHit)
	{
		return ETokenCheck::KeepTurn;
	}
	if (Look.HeldSeconds > MaxSeconds)
	{
		return ETokenCheck::TakeBack;
	}
	if (!Look.bAttacking && Look.LeaveDistance > 0.f && Look.Distance > Look.LeaveDistance)
	{
		return ETokenCheck::Return;
	}
	return ETokenCheck::Keep;
}

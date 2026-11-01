// ADredgerRifleman: his distance and his cover. Inside his band (10-25 m) his own tactics move him (his brain only faces him
// at his target, and brings him in from past the band): he looks for cover by traces round his target, a few candidate spots
// a frame (DredgerRules::SpotCandidates on his squad's flank bearing: pairs flank from both sides), each measured for ground
// he can stand on, the world in front of him crouched (cover) and a clear line for his rifle standing (DredgerRules::
// ScoreSpot weighs them), walks there facing his target, and ducks. He looks again every Looter.Dredger.Rifle.CoverRecheck,
// after a few bursts from one spot, or when shot in it (flanked); too close, he backs away (to cover farther off when
// there is any). No cover: he stands in the open in his band and sidesteps now and then. Holding his ground (the tender's
// deck), he only turns. At a post (ACreatureBase::PostRadius: the town gate's checkpoint) every spot and step stays in it,
// his post's own spots are candidates too, and a target past his band is fought from there, never walked out to.

#include "Creatures/DredgerRifleman.h"
#include "Creatures/CreaturePackComponent.h"
#include "Creatures/FightTuning.h"
#include "World/WorldQueries.h"
#include "Components/CapsuleComponent.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"

namespace
{
	/** Spots measured a frame while searching (three to four traces each). */
	constexpr int32 SearchBudget = 4;
	/**
	 * A spot this much higher or lower than his feet (cm: about a step), plus a share of the way to it (a slope), is another
	 * level: not his (the top of a crate, a roof, the canyon floor under a deck).
	 */
	constexpr float LevelSlack = 45.f;
	constexpr float LevelPerDistance = 0.06f;
	constexpr float LevelMost = 200.f;
	/** Ground steeper than this (its normal's up) is no place to stand. */
	constexpr float FlatEnough = 0.75f;
	/** A way to his spot that runs within this of his target (cm) crosses it. */
	constexpr float CrossNear = 400.f;
	/** Walking to his spot slower than this (cm/s nearer) for StuckGiveUp seconds, he gives the spot up. */
	constexpr float StuckSpeed = 30.f;
	constexpr float StuckGiveUp = 1.5f;
	/** His ways are looked for again this often (s); backing away he walks at this share of his chase. */
	constexpr float KeepLookEvery = 0.2f;
	constexpr float BackOffShare = 0.8f;
	/** In the open: a sidestep this often (s), this long (s), at this share of his chase. */
	constexpr float StrafeEvery = 1.8f;
	constexpr float StrafeFor = 0.6f;
	constexpr float StrafeShare = 0.45f;
	/** His shot's line is looked at this often (s). */
	constexpr double ShotCheckEvery = 0.25;
	/** At a post: this many spots round its middle, this share of the way to its edge, are candidates too. */
	constexpr int32 PostSpots = 4;
	constexpr float PostSpotShare = 0.6f;

	float BearingFrom(const FVector& From, const FVector& To)
	{
		const FVector Way = (To - From).GetSafeNormal2D();
		return FMath::RadiansToDegrees(FMath::Atan2(static_cast<float>(Way.Y), static_cast<float>(Way.X)));
	}
}

// ---------------------------------------------------------------------------
// Each frame
// ---------------------------------------------------------------------------

void ADredgerRifleman::TickTactics(float DeltaSeconds)
{
	const APawn* Victim = GetTarget();
	const bool bFighting = Victim && GetCreatureState() == ECreatureState::Chase && !IsNoticing() && !IsHeldByHit()
		&& !GetPack()->IsRetreating();
	if (!bFighting)
	{
		// In an attack he keeps his place and his spot; anything else (calm, searching, held) is his brain's.
		if (GetCreatureState() != ECreatureState::Attack)
		{
			Tactic = EDredgerTactic::None;
			bSearching = false;
			SetKeepingDistance(false);
		}
		return;
	}
	const FDredgerTuning Tuning = FDredgerTuning::Live();
	FlankedFor = FMath::Max(0.f, FlankedFor - DeltaSeconds);
	if (HoldsGround())
	{
		Tactic = EDredgerTactic::Hold;
		SetKeepingDistance(true);
		return;
	}

	const FVector Here = GetActorLocation();
	const FVector Prey = Victim->GetActorLocation();
	const float Distance = static_cast<float>(FVector::Dist2D(Here, Prey));
	const float Gap = bHasSpot ? static_cast<float>(FVector::Dist2D(Here, Spot)) : 0.f;
	const EDredgerKeep Keep = DredgerRules::ChooseKeep(Distance, Tuning.BandNear, Tuning.BandFar, bHasSpot, Gap);
	// At a post (a checkpoint's crew) he never comes in after a target past his band: he fights from his post's cover.
	const bool bPosted = PostRadius > 0.f;
	if (Keep == EDredgerKeep::None && !bPosted)
	{
		// Out past his band: his brain brings him in; the spot he had is behind the fight now.
		Tactic = EDredgerTactic::None;
		bHasSpot = false;
		bSearching = false;
		SetKeepingDistance(false);
		return;
	}
	SetKeepingDistance(true);

	// His cover, looked for again when he has none, when it has fallen out of his band as the fight moved, when he was shot
	// in it, after a few bursts from it, and at the recheck.
	RecheckIn -= DeltaSeconds;
	const float SpotToPrey = bHasSpot ? static_cast<float>(FVector::Dist2D(Spot, Prey)) : 0.f;
	const bool bMoveOn = FlankedFor > 0.f || (Tuning.RelocateAfter > 0 && BurstsHere >= Tuning.RelocateAfter);
	// (At a post his spot may stay past his band: the fight came no nearer, and nothing in his post is nearer.)
	const bool bStale = !bHasSpot || SpotToPrey < Tuning.BandNear * 0.8f || (!bPosted && SpotToPrey > Tuning.BandFar + 200.f) || bMoveOn
		|| RecheckIn <= 0.f;
	if (bSearching)
	{
		StepSearch(SearchBudget);
	}
	else if (bStale)
	{
		bAvoidCurrent = bMoveOn;
		BeginSearch();
		StepSearch(SearchBudget);
	}

	// Where to go: his spot (backing off toward it too, when it is farther from his target), straight back, or stand.
	const float Size = GetSizeScale();
	const bool bSpotBack = bHasSpot && SpotToPrey >= Tuning.BandNear;
	FVector Want = FVector::ZeroVector;
	float Pace = GetChaseSpeedNow() * (FlankedFor > 0.f ? 1.f : FFightTuning::LiveRangedMoveShare());
	if (Keep == EDredgerKeep::BackOff && !bSpotBack)
	{
		Tactic = EDredgerTactic::BackOff;
		Want = (Here - Prey).GetSafeNormal2D();
		Pace = GetChaseSpeedNow() * BackOffShare;
	}
	else if (bHasSpot && Gap > DredgerRules::SpotArrive * Size)
	{
		Tactic = EDredgerTactic::Advance;
		Want = (Spot - Here).GetSafeNormal2D();
		// Walking to it without getting nearer: something is in the way he can't get round; he gives the spot up.
		StuckFor = Gap > LastGap - StuckSpeed * DeltaSeconds ? StuckFor + DeltaSeconds : 0.f;
		LastGap = Gap;
		if (StuckFor >= StuckGiveUp)
		{
			bHasSpot = false;
			RecheckIn = 0.f;
			StuckFor = 0.f;
		}
	}
	else
	{
		Tactic = bHasSpot && bSpotCovered ? EDredgerTactic::Covered : EDredgerTactic::Exposed;
		StuckFor = 0.f;
		LastGap = Gap;
		// In the open he doesn't stand still to be shot: a sidestep now and then, never off his ground.
		if (Tactic == EDredgerTactic::Exposed)
		{
			StrafeClock += DeltaSeconds;
			if (StrafeClock >= StrafeEvery)
			{
				StrafeClock = 0.f;
				StrafeSide = Dice.FRand() < 0.5f ? -1.f : 1.f;
			}
			if (StrafeClock < StrafeFor)
			{
				const FVector Toward = (Prey - Here).GetSafeNormal2D();
				Want = FVector(-Toward.Y, Toward.X, 0.0) * StrafeSide;
				Pace = GetChaseSpeedNow() * StrafeShare;
			}
		}
	}
	if (Want.IsNearlyZero())
	{
		return;
	}
	KeepLookIn -= DeltaSeconds;
	if (KeepLookIn <= 0.f)
	{
		KeepLookIn = KeepLookEvery;
		const FVector Ways[] = { Want, Want.RotateAngleAxis(30.f, FVector::UpVector), Want.RotateAngleAxis(-30.f, FVector::UpVector),
			Want.RotateAngleAxis(65.f, FVector::UpVector), Want.RotateAngleAxis(-65.f, FVector::UpVector) };
		KeepWay = FirstClearWay(Ways);
	}
	if (!KeepWay.IsNearlyZero())
	{
		GetCharacterMovement()->MaxWalkSpeed = Pace;
		AddMovementInput(KeepWay);
	}
}

void ADredgerRifleman::SetKeepingDistance(bool bKeeping)
{
	if (bKeepingDistance == bKeeping)
	{
		return;
	}
	bKeepingDistance = bKeeping;
	// Keeping his distance he faces his target (his brain turns him) whichever way he walks.
	GetCharacterMovement()->bOrientRotationToMovement = !bKeeping;
}

FVector ADredgerRifleman::FirstClearWay(TConstArrayView<FVector> Ways) const
{
	const float Look = GetSteerProbes().SweepLength;
	const FVector Here = GetActorLocation();
	const FVector Middle = GetHome().GetLocation();
	for (const FVector& Way : Ways)
	{
		if (Way.IsNearlyZero())
		{
			continue;
		}
		// Never off his hunting ground (his spawner's), whose edge he would only turn back from.
		if (HuntingGround.IsSet() && !HuntingGround.Contains(Here + Way * Look, Middle))
		{
			continue;
		}
		// Never out of his post (one pushed out of it may still walk back in).
		const FVector Ahead = Here + Way * Look;
		if (!IsWithinPost(Ahead) && FVector::DistSquared2D(Ahead, Middle) >= FVector::DistSquared2D(Here, Middle))
		{
			continue;
		}
		if (LookAlong(Way, Look).bClear)
		{
			return Way;
		}
	}
	return FVector::ZeroVector;
}

bool ADredgerRifleman::WantsButt() const
{
	const APawn* Victim = GetTarget();
	return Victim && FVector::Dist2D(GetActorLocation(), Victim->GetActorLocation()) <= FDredgerTuning::Live().ButtReach * GetSizeScale();
}

bool ADredgerRifleman::IsShotClear() const
{
	const UWorld* World = GetWorld();
	const APawn* Victim = GetTarget();
	if (!World || !Victim)
	{
		return false;
	}
	const double Now = World->GetTimeSeconds();
	if (Now - ShotCheckedAt < ShotCheckEvery)
	{
		return bShotClear;
	}
	ShotCheckedAt = Now;
	// From where his muzzle will be once he stands (he may be ducked now), to the target's chest: the world only (another
	// Dredger never blocks his round, which passes through creatures).
	const float Size = GetSizeScale();
	const FVector Feet = GetActorLocation() - FVector(0.f, 0.f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
	const FVector Muzzle = Feet + FVector(0.f, 0.f, DredgerRules::StandMuzzleHeight * Size) + GetActorForwardVector() * (30.f * Size);
	const FVector Chest = Victim->GetActorLocation() + FVector(0.f, 0.f, DredgerRules::ChestRise);
	FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(World, TEXT("DredgerShot"), this, false);
	Params.AddIgnoredActor(Victim);
	bShotClear = !World->LineTraceTestByObjectType(Muzzle, Chest, FCollisionObjectQueryParams(ECC_WorldStatic), Params);
	return bShotClear;
}

bool ADredgerRifleman::GetSpot(FVector& OutSpot, bool& bOutCovered) const
{
	OutSpot = Spot;
	bOutCovered = bSpotCovered;
	return bHasSpot;
}

// ---------------------------------------------------------------------------
// The search
// ---------------------------------------------------------------------------

float ADredgerRifleman::SquadBearing(const APawn& Prey) const
{
	// The riflemen after the same target, in a fixed order (their ids): his place among them is his flank off their middle.
	TArray<const ADredgerRifleman*, TInlineAllocator<8>> Squad;
	FVector Mean = FVector::ZeroVector;
	for (TActorIterator<ADredgerRifleman> It(GetWorld()); It; ++It)
	{
		const ADredgerRifleman* Other = *It;
		if (Other && !Other->IsDead() && Other->GetTarget() == &Prey)
		{
			Squad.Add(Other);
			Mean += (Other->GetActorLocation() - Prey.GetActorLocation()).GetSafeNormal2D();
		}
	}
	Squad.Sort([](const ADredgerRifleman& A, const ADredgerRifleman& B) { return A.GetUniqueID() < B.GetUniqueID(); });
	const int32 Index = FMath::Max(Squad.IndexOfByKey(this), 0);
	const float Base = Mean.IsNearlyZero() ? BearingFrom(Prey.GetActorLocation(), GetActorLocation()) : FMath::RadiansToDegrees(
		FMath::Atan2(static_cast<float>(Mean.Y), static_cast<float>(Mean.X)));
	return static_cast<float>(FRotator::NormalizeAxis(Base + DredgerRules::FlankOffset(Index, FDredgerTuning::Live().FlankAngle)));
}

bool ADredgerRifleman::IsClaimed(const FVector& Where) const
{
	for (TActorIterator<ADredgerRifleman> It(GetWorld()); It; ++It)
	{
		const ADredgerRifleman* Other = *It;
		if (!Other || Other == this || Other->IsDead())
		{
			continue;
		}
		FVector TheirSpot;
		bool bCovered = false;
		const bool bTheirs = Other->GetSpot(TheirSpot, bCovered);
		if ((bTheirs && FVector::Dist2D(TheirSpot, Where) < DredgerRules::ClaimRadius)
			|| FVector::Dist2D(Other->GetActorLocation(), Where) < DredgerRules::ClaimRadius * 0.5f)
		{
			return true;
		}
	}
	return false;
}

void ADredgerRifleman::BeginSearch()
{
	const APawn* Victim = GetTarget();
	if (!Victim)
	{
		return;
	}
	const FDredgerTuning Tuning = FDredgerTuning::Live();
	const FVector Prey = Victim->GetActorLocation();
	WantBearing = SquadBearing(*Victim);
	Candidates.Reset();
	// His own spot first (it keeps him there when nothing is better), then round his flank, then one where he stands now.
	if (bHasSpot)
	{
		Candidates.Add(Spot);
	}
	for (const FVector2D& Candidate : DredgerRules::SpotCandidates(WantBearing, Tuning))
	{
		Candidates.Add(Prey + FRotator(0.f, Candidate.Y, 0.f).Vector() * Candidate.X);
	}
	Candidates.Add(Prey + (GetActorLocation() - Prey).GetSafeNormal2D() * Tuning.Preferred);
	// At a post the spots round his target mostly lie outside it: his post's own (where he appeared, behind the checkpoint's
	// sandbags) and a few round it.
	if (PostRadius > 0.f)
	{
		const FVector Post = GetHome().GetLocation();
		Candidates.Add(Post);
		for (int32 Step = 0; Step < PostSpots; ++Step)
		{
			Candidates.Add(Post + FRotator(0.f, 360.f * Step / PostSpots, 0.f).Vector() * (PostRadius * PostSpotShare));
		}
	}
	NextCandidate = 0;
	bSearching = true;
	BestCost = TNumericLimits<float>::Max();
	bBestCovered = false;
}

void ADredgerRifleman::StepSearch(int32 Budget)
{
	const APawn* Victim = GetTarget();
	if (!Victim)
	{
		bSearching = false;
		return;
	}
	const FDredgerTuning Tuning = FDredgerTuning::Live();
	const FVector Prey = Victim->GetActorLocation();
	for (int32 Done = 0; Done < Budget && NextCandidate < Candidates.Num(); ++Done, ++NextCandidate)
	{
		FVector Ground;
		FDredgerSpotFacts Facts;
		if (!FindGroundAt(Candidates[NextCandidate], Ground) || !MeasureSpot(Prey, Ground, Facts))
		{
			continue;
		}
		// At a post, how near his target he gets isn't his to choose: past his band costs no more than its edge, so his post's
		// cover beats an open spot at its edge a step nearer.
		if (PostRadius > 0.f)
		{
			Facts.ToTarget = FMath::Min(Facts.ToTarget, Tuning.BandFar);
		}
		const float Cost = DredgerRules::ScoreSpot(Facts, Tuning);
		if (Cost < BestCost)
		{
			BestCost = Cost;
			BestSpot = Ground;
			bBestCovered = Facts.bCovered;
		}
	}
	if (NextCandidate < Candidates.Num())
	{
		return;
	}
	bSearching = false;
	RecheckIn = Tuning.CoverRecheck * Dice.FRandRange(0.8f, 1.2f);
	FlankedFor = 0.f;
	if (BestCost < TNumericLimits<float>::Max())
	{
		if (!bHasSpot || FVector::Dist2D(BestSpot, Spot) > DredgerRules::ClaimRadius)
		{
			BurstsHere = 0;
		}
		Spot = BestSpot;
		bHasSpot = true;
		bSpotCovered = bBestCovered;
		StuckFor = 0.f;
		LastGap = static_cast<float>(FVector::Dist2D(GetActorLocation(), Spot));
	}
	else
	{
		bHasSpot = false;
	}
	bAvoidCurrent = false;
}

bool ADredgerRifleman::FindGroundAt(const FVector& Where, FVector& OutGround) const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}
	const float Half = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const FVector Feet = GetActorLocation() - FVector(0.f, 0.f, Half);
	const float Top = static_cast<float>(FMath::Max(Where.Z, Feet.Z)) + 250.f;
	const float Bottom = static_cast<float>(FMath::Min(Where.Z, Feet.Z)) - 500.f;
	const FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(World, TEXT("DredgerCover"), this);
	FHitResult Hit;
	if (!World->LineTraceSingleByObjectType(Hit, FVector(Where.X, Where.Y, Top), FVector(Where.X, Where.Y, Bottom),
		FCollisionObjectQueryParams(ECC_WorldStatic), Params))
	{
		return false;
	}
	// His own level of ground (never the top of a crate, a roof or the canyon floor under a deck), flat enough, on his ground.
	const float Travel = static_cast<float>(FVector::Dist2D(Feet, Hit.ImpactPoint));
	const float Slack = FMath::Min(LevelSlack + LevelPerDistance * Travel, LevelMost);
	if (FMath::Abs(static_cast<float>(Hit.ImpactPoint.Z - Feet.Z)) > Slack || Hit.ImpactNormal.Z < FlatEnough)
	{
		return false;
	}
	if ((HuntingGround.IsSet() && !HuntingGround.Contains(Hit.ImpactPoint, GetHome().GetLocation())) || !IsWithinPost(Hit.ImpactPoint))
	{
		return false;
	}
	// Room to stand there: his capsule clear of the world.
	const float Radius = GetCapsuleComponent()->GetScaledCapsuleRadius();
	const FVector Middle = Hit.ImpactPoint + FVector(0.f, 0.f, Half + 5.f);
	if (World->OverlapAnyTestByObjectType(Middle, FQuat::Identity, FCollisionObjectQueryParams(ECC_WorldStatic),
		FCollisionShape::MakeCapsule(Radius, Half), Params))
	{
		return false;
	}
	OutGround = Hit.ImpactPoint;
	return true;
}

bool ADredgerRifleman::MeasureSpot(const FVector& Prey, const FVector& Ground, FDredgerSpotFacts& OutFacts) const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}
	const float Size = GetSizeScale();
	const FVector Chest = Prey + FVector(0.f, 0.f, DredgerRules::ChestRise);
	const FVector Eye = Ground + FVector(0.f, 0.f, DredgerRules::CrouchEyeHeight * Size);
	const FVector Muzzle = Ground + FVector(0.f, 0.f, DredgerRules::StandMuzzleHeight * Size);
	FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(World, TEXT("DredgerCover"), this, false);
	if (const APawn* Victim = GetTarget())
	{
		Params.AddIgnoredActor(Victim);
	}
	const FCollisionObjectQueryParams Solid(ECC_WorldStatic);
	FHitResult Hit;
	// Crouched there, the world in front of him (close: a rail, a crate, a wall's corner, never a hill far off).
	const bool bBlocked = World->LineTraceSingleByObjectType(Hit, Eye, Chest, Solid, Params);
	OutFacts.bCovered = bBlocked && Hit.Distance <= DredgerRules::CoverDepth * Size;
	// Standing there, his rifle clears it.
	OutFacts.bCanShoot = !World->LineTraceTestByObjectType(Muzzle, Chest, Solid, Params);
	OutFacts.ToTarget = static_cast<float>(FVector::Dist2D(Ground, Prey));
	const FVector Here = GetActorLocation();
	OutFacts.Travel = static_cast<float>(FVector::Dist2D(Here, Ground));
	OutFacts.BearingOff = FMath::Abs(static_cast<float>(FRotator::NormalizeAxis(BearingFrom(Prey, Ground) - WantBearing)));
	OutFacts.bClaimed = IsClaimed(Ground) || (bAvoidCurrent && bHasSpot && FVector::Dist2D(Ground, Spot) < DredgerRules::ClaimRadius);
	const FVector Flat(Prey.X, Prey.Y, 0.0);
	OutFacts.bCrossesTarget = FMath::PointDistToSegment(Flat, FVector(Here.X, Here.Y, 0.0), FVector(Ground.X, Ground.Y, 0.0)) < CrossNear
		&& OutFacts.Travel > CrossNear;
	OutFacts.bCurrent = bHasSpot && !bAvoidCurrent && FVector::Dist2D(Ground, Spot) < 50.f;
	return true;
}

bool ADredgerRifleman::DevFindSpot()
{
	BeginSearch();
	StepSearch(Candidates.Num() + 1);
	return bHasSpot;
}

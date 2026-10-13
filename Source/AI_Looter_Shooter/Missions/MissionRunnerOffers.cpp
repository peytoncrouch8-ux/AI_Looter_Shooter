// UMissionRunner: missions picked up from their givers (EMissionStart::Offered), Borderlands' way. Once what comes first is
// finished such a mission is offered, not started: a ! over its giver, and the offer card when the player talks to them,
// reads their board or picks up their artifact. Accepting it records it in the campaign and starts it; from then on it
// starts in its area's level by itself, as an automatic mission does.

#include "Missions/MissionRunner.h"
#include "AI_Looter_Shooter.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionObjective.h"
#include "Missions/MissionTargets.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

// ---------------------------------------------------------------------------
// Queries
// ---------------------------------------------------------------------------

bool UMissionRunner::IsAccepted(FName MissionId) const
{
	if (MissionId.IsNone())
	{
		return false;
	}
	// Recorded as taken; or played already in a save from before missions were offered: the campaign's main mission, one
	// waiting for its turn-in, one running here. None of those is offered again.
	const FCampaignRecord& Campaign = GetCampaign();
	return Campaign.IsAccepted(MissionId) || Campaign.ActiveMission == MissionId || Campaign.IsReadyToTurnIn(MissionId)
		|| IsRunning(MissionId);
}

bool UMissionRunner::IsAvailable(FName MissionId) const
{
	const UMissionDefinition* Mission = FindDefinition(MissionId);
	return Mission && Mission->IsOffered() && !Mission->Steps.IsEmpty() && !IsAccepted(MissionId) && CanStart(*Mission);
}

TArray<const UMissionDefinition*> UMissionRunner::GetOffersAt(const AActor* Giver) const
{
	TArray<const UMissionDefinition*> Offers;
	if (!Giver)
	{
		return Offers;
	}
	for (const UMissionDefinition* Mission : Definitions)
	{
		if (Mission && Mission->IsOffered() && Giver->ActorHasTag(Mission->Offer.GiverTag) && IsAvailable(Mission->GetMissionId()))
		{
			Offers.Add(Mission);
		}
	}
	// The story first: someone with a main mission and a side job for the player offers the main one first.
	Offers.StableSort([](const UMissionDefinition& A, const UMissionDefinition& B)
	{
		return A.Kind == EMissionKind::Main && B.Kind != EMissionKind::Main;
	});
	return Offers;
}

AActor* UMissionRunner::FindOfferGiverActor(const UMissionDefinition& Mission) const
{
	if (Mission.Offer.GiverTag.IsNone())
	{
		return nullptr;
	}
	FMissionActorFilter Giver;
	Giver.ActorTag = Mission.Offer.GiverTag;
	const FMissionContext Context = MakeContext();
	const TOptional<FVector> From = Context.GetPlayerLocation();
	AActor* Nearest = nullptr;
	double NearestDistance = TNumericLimits<double>::Max();
	// Shown ones only: someone the story hasn't brought out yet, or an artifact already taken, offers nothing.
	MissionTargets::ForEach(Context.World, Giver, [&](AActor& Candidate)
	{
		if (Candidate.IsHidden())
		{
			return;
		}
		const double Distance = From.IsSet() ? FVector::DistSquared2D(Candidate.GetActorLocation(), *From) : 0.0;
		if (!Nearest || Distance < NearestDistance)
		{
			Nearest = &Candidate;
			NearestDistance = Distance;
		}
	});
	return Nearest;
}

TOptional<FVector> UMissionRunner::FindOfferGiver(const UMissionDefinition& Mission) const
{
	const AActor* Giver = FindOfferGiverActor(Mission);
	return Giver ? TOptional<FVector>(Giver->GetActorLocation()) : TOptional<FVector>();
}

TOptional<FVector> UMissionRunner::GetPlayerLocation() const
{
	return MakeContext().GetPlayerLocation();
}

// ---------------------------------------------------------------------------
// Accepting
// ---------------------------------------------------------------------------

bool UMissionRunner::AcceptMission(FName MissionId)
{
	UMissionDefinition* Mission = FindDefinition(MissionId);
	if (!bActive || !Mission || IsCompleted(MissionId))
	{
		return false;
	}
	FCampaignRecord& Campaign = GetCampaign();
	if (IsRunning(MissionId))
	{
		// Under way already (the console offered it as it ran): only recorded.
		if (Campaign.Accept(MissionId))
		{
			AskToSave();
			bChanged = true;
		}
		AfterChange();
		return true;
	}
	if (Mission->Steps.IsEmpty() || !CanStart(*Mission))
	{
		UE_LOG(LogLooter, Log, TEXT("Mission %s can't be taken now (%s)."), *MissionId.ToString(),
			Mission->Steps.IsEmpty() ? TEXT("it has no steps") : !IsInArea(*Mission) ? TEXT("it's played in another area")
			: TEXT("what comes before it isn't finished"));
		return false;
	}
	Campaign.Accept(MissionId);
	AskToSave();
	UE_LOG(LogLooter, Log, TEXT("Mission %s accepted from %s"), *MissionId.ToString(),
		Mission->IsOffered() ? *Mission->GetOfferGiverName().ToString() : TEXT("the console"));
	StartRun(*Mission, 0);
	// Its line goes in the display now: with nothing tracked it's what the minimap and the tracker follow (FMissionBook).
	AfterChange();
	return true;
}

// ---------------------------------------------------------------------------
// Starting by itself
// ---------------------------------------------------------------------------

bool UMissionRunner::StartsByItself(const UMissionDefinition& Mission) const
{
	return Mission.Start == EMissionStart::Automatic || (Mission.IsOffered() && GetCampaign().IsAccepted(Mission.GetMissionId()));
}

void UMissionRunner::UpgradeAccepted(const UMissionDefinition& Mission)
{
	if (Mission.IsOffered() && GetCampaign().Accept(Mission.GetMissionId()))
	{
		UE_LOG(LogLooter, Log, TEXT("Mission %s: played already, so it's recorded as taken from %s."), *Mission.GetMissionId().ToString(),
			*Mission.GetOfferGiverName().ToString());
		AskToSave();
	}
}

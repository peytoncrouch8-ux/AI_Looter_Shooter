#include "Missions/MissionTalk.h"
#include "Missions/MissionDefinition.h"
#include "Story/CaptionSubsystem.h"
#include "Story/SpeakerPointComponent.h"
#include "GameFramework/Actor.h"

namespace
{
	USpeakerPointComponent* SpeakerOf(AActor* Giver)
	{
		return Giver ? Giver->FindComponentByClass<USpeakerPointComponent>() : nullptr;
	}
}

FText MissionTalk::GiverSpeaker(const UMissionDefinition& Mission, const AActor* Giver)
{
	const USpeakerPointComponent* Point = Giver ? Giver->FindComponentByClass<USpeakerPointComponent>() : nullptr;
	if (Point && !Point->SpeakerName.IsEmpty())
	{
		return Point->SpeakerName;
	}
	// Away from their point (Hob between perches) or a board: the name the offer gives them.
	return Mission.GetOfferGiverName();
}

TArray<FStoryLine> MissionTalk::InGiverName(TArray<FStoryLine> Lines, const UMissionDefinition& Mission, const AActor* Giver)
{
	const FText Speaker = GiverSpeaker(Mission, Giver);
	for (FStoryLine& Line : Lines)
	{
		if (Line.Speaker.IsEmpty())
		{
			Line.Speaker = Speaker;
		}
	}
	return Lines;
}

int32 MissionTalk::SayOnTheMove(const UObject* WorldContext, const UMissionDefinition& Mission, const AActor* Giver)
{
	TArray<FStoryLine> Said = Mission.Offer.Lines;
	Said.Append(Mission.Offer.AcceptLines);
	UCaptionSubsystem* Captions = UCaptionSubsystem::Get(WorldContext);
	if (!Captions || Said.IsEmpty())
	{
		return 0;
	}
	// Queued: the turn-in that unlocked it is still being said, and finishes first.
	return Captions->Play(InGiverName(MoveTemp(Said), Mission, Giver), ECaptionPlay::Queue);
}

int32 MissionTalk::SayAccepted(const UObject* WorldContext, const UMissionDefinition& Mission, AActor* Giver)
{
	if (USpeakerPointComponent* Point = SpeakerOf(Giver))
	{
		// Their offer's words stop where they are and the accept lines come at once, Borderlands' way.
		return Point->SayInstead(Mission.Offer.AcceptLines);
	}
	UCaptionSubsystem* Captions = UCaptionSubsystem::Get(WorldContext);
	if (!Captions || Mission.Offer.AcceptLines.IsEmpty())
	{
		return 0;
	}
	// A board or a found thing says nothing as it offers: its accept lines (a remark of Hob's) cut nobody short.
	return Captions->Play(InGiverName(Mission.Offer.AcceptLines, Mission, Giver), ECaptionPlay::Queue);
}

void MissionTalk::StopGiver(AActor* Giver)
{
	if (USpeakerPointComponent* Point = SpeakerOf(Giver))
	{
		Point->StopTalking();
	}
}

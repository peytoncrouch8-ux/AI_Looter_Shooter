#include "World/GraveMarkRules.h"
#include "Session/CampaignRecord.h"

namespace GraveMarkRules
{
	bool IsMarked(const FCampaignRecord& Campaign, FName Marker)
	{
		return !Marker.IsNone() && Campaign.MarkedGraves.Contains(Marker);
	}

	bool IsSavePoint(const FCampaignRecord& Campaign, FName Area, FName Marker)
	{
		return !Marker.IsNone() && Campaign.GetLastGrave(Area) == Marker;
	}

	EGraveMarkBlock Check(const FGraveMarkSenses& Senses)
	{
		// The reason that is most final first, so the console names the one that matters.
		if (!Senses.bMarkOnApproach)
		{
			return EGraveMarkBlock::Off;
		}
		if (Senses.bLanding)
		{
			return EGraveMarkBlock::Landing;
		}
		if (!Senses.bHasPlayer)
		{
			return EGraveMarkBlock::NoPlayer;
		}
		if (Senses.bDying)
		{
			return EGraveMarkBlock::Dying;
		}
		if (Senses.bInScene)
		{
			return EGraveMarkBlock::Scene;
		}
		if (Senses.bTravelling)
		{
			return EGraveMarkBlock::Travelling;
		}
		if (!Senses.bStoryAllows)
		{
			return EGraveMarkBlock::NotYet;
		}
		if (Senses.Distance > Senses.Radius)
		{
			return EGraveMarkBlock::TooFar;
		}
		if (!Senses.bClearLine)
		{
			return EGraveMarkBlock::NoSight;
		}
		return EGraveMarkBlock::None;
	}

	EGraveMark Outcome(bool bMarkedBefore, bool bIsSavePoint)
	{
		if (!bMarkedBefore)
		{
			return EGraveMark::First;
		}
		return bIsSavePoint ? EGraveMark::None : EGraveMark::Again;
	}

	EGraveMark Record(FCampaignRecord& Campaign, FName Area, FName Marker)
	{
		if (Marker.IsNone())
		{
			return EGraveMark::None;
		}
		const EGraveMark Result = Outcome(IsMarked(Campaign, Marker), IsSavePoint(Campaign, Area, Marker));
		Campaign.MarkedGraves.AddUnique(Marker);
		Campaign.SetLastGrave(Area, Marker);
		return Result;
	}

	bool Forget(FCampaignRecord& Campaign, FName Area, FName Marker)
	{
		if (Marker.IsNone())
		{
			return false;
		}
		const bool bWasMarked = Campaign.MarkedGraves.Remove(Marker) > 0;
		if (IsSavePoint(Campaign, Area, Marker))
		{
			Campaign.LastGraves.Remove(Area);
		}
		return bWasMarked;
	}

	float FlareSeconds(EGraveMark Kind)
	{
		switch (Kind)
		{
		case EGraveMark::First:
			return FirstFlareSeconds;
		case EGraveMark::Again:
			return LiftSeconds;
		default:
			return 0.f;
		}
	}

	float FlarePulse(float Age, EGraveMark Kind)
	{
		const float Length = FlareSeconds(Kind);
		if (Length <= 0.f || Age < 0.f || Age >= Length)
		{
			return 0.f;
		}
		if (Age < FlareRiseSeconds)
		{
			// It catches: up to the peak in a breath.
			return FMath::InterpEaseOut(0.f, 1.f, Age / FlareRiseSeconds, 2.f);
		}
		// Then it eases down, slowly at first and settling into the rest light of a flame that has taken.
		const float Fall = (Age - FlareRiseSeconds) / (Length - FlareRiseSeconds);
		return FMath::Square(1.f - Fall);
	}

	FString Describe(EGraveMarkBlock Block)
	{
		switch (Block)
		{
		case EGraveMarkBlock::Off:
			return TEXT("it doesn't mark on approach");
		case EGraveMarkBlock::Landing:
			return TEXT("it is a trip's landing");
		case EGraveMarkBlock::NoPlayer:
			return TEXT("no living player");
		case EGraveMarkBlock::Dying:
			return TEXT("the player is down");
		case EGraveMarkBlock::Scene:
			return TEXT("a scene is playing");
		case EGraveMarkBlock::Travelling:
			return TEXT("a trip or a fast travel is under way");
		case EGraveMarkBlock::NotYet:
			return TEXT("its mission isn't finished");
		case EGraveMarkBlock::TooFar:
			return TEXT("too far");
		case EGraveMarkBlock::NoSight:
			return TEXT("no clear line to its lantern");
		default:
			return FString();
		}
	}
}

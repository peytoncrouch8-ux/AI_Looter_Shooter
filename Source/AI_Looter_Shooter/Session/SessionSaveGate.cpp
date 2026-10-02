#include "Session/SessionSaveGate.h"

bool FSessionSaveGate::Allows(ESessionSaveReason Reason) const
{
	if (bTravelling)
	{
		return false;
	}
	switch (Reason)
	{
	case ESessionSaveReason::Autosave:
	case ESessionSaveReason::Soon:
		return Holds.IsEmpty();
	case ESessionSaveReason::Asked:
	case ESessionSaveReason::LevelEnd:
		return true;
	}
	return true;
}

void FSessionSaveGate::BeginTrip(const FString& Destination)
{
	bTravelling = true;
	TripDestination = Destination;
}

void FSessionSaveGate::Hold(FName Reason)
{
	if (!Reason.IsNone())
	{
		Holds.AddUnique(Reason);
	}
}

bool FSessionSaveGate::Release(FName Reason)
{
	return Holds.Remove(Reason) > 0 && Holds.IsEmpty();
}

void FSessionSaveGate::Reset()
{
	Holds.Reset();
	TripDestination.Reset();
	bTravelling = false;
}

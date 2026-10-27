// MissionGates: the short hold a mission's key actions take, and the story conditions that open a step's things.

#include "Missions/MissionGates.h"
#include "Interaction/InteractionTypes.h"
#include "Missions/MissionRunner.h"
#include "Story/StoryCondition.h"
#include "HAL/IConsoleManager.h"

namespace
{
	/**
	 * As long as a wanted poster's tear (0.6 s): long enough that a mashed key never takes or reads a step's thing, short
	 * enough not to drag (the bell's 1.2 s and a fitting's 1.5 s are the long, physical ones).
	 */
	constexpr float DefaultHoldSeconds = 0.6f;

	TAutoConsoleVariable<float> CVarHoldSeconds(TEXT("Looter.Missions.HoldSeconds"), DefaultHoldSeconds,
		TEXT("How long Interact is held (s) to take a mission item or do a step's reading (Back Pay's tags, the payroll book). "
			"0: a tap, as before 2026-10-27."), ECVF_Default);
}

float MissionGates::HoldSeconds()
{
	return FMath::Max(CVarHoldSeconds.GetValueOnGameThread(), 0.f);
}

bool MissionGates::IsMet(const FStoryCondition& Condition, const UObject* WorldContext)
{
	if (Condition.IsEmpty())
	{
		return true;
	}
	const UMissionRunner* Runner = UMissionRunner::Get(WorldContext);
	return Runner && Condition.IsMet(Runner->GetCampaign(), Runner);
}

bool MissionGates::AnyMet(const TArray<FStoryCondition>& Conditions, const UObject* WorldContext)
{
	if (Conditions.IsEmpty())
	{
		return true;
	}
	const UMissionRunner* Runner = UMissionRunner::Get(WorldContext);
	if (!Runner)
	{
		return false;
	}
	return Conditions.ContainsByPredicate([Runner](const FStoryCondition& When)
	{
		return When.IsEmpty() || When.IsMet(Runner->GetCampaign(), Runner);
	});
}

void MissionGates::SetUse(FInteractionOptions& Options, const FText& Words, bool bKeyAction)
{
	const float Hold = bKeyAction ? HoldSeconds() : 0.f;
	Options.bHold = Hold > 0.f;
	Options.bTap = !Options.bHold;
	Options.HoldSeconds = Hold;
	Options.TapPrompt = Options.bTap ? Words : FText::GetEmpty();
	Options.HoldPrompt = Options.bHold ? Words : FText::GetEmpty();
}

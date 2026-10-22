// CampaignStepMigration: a campaign record from before 2026-10-13's missions moved onto today's (the steps that moved, the
// side missions that used to start by themselves).

#include "Session/CampaignStepMigration.h"
#include "AI_Looter_Shooter.h"
#include "Session/CampaignRecord.h"

namespace
{
	const FName Main3(TEXT("Main3"));
	const FName Main4(TEXT("Main4"));
	const FName Main5(TEXT("Main5"));
	const FName Side1(TEXT("Side1"));
	const FName Side2(TEXT("Side2"));
	const FName Side3(TEXT("Side3"));
	const FName KnockingBelow(TEXT("KnockingBelow"));
	const FName DeadLift(TEXT("DeadLift"));

	/** A mission whose steps moved: where each old step is in today's list, the last entry one past the old last step. */
	struct FMovedSteps
	{
		FName Mission;
		TArray<int32> NewStepOf;
	};

	const TArray<FMovedSteps>& MovedSteps()
	{
		static const TArray<FMovedSteps> All = {
			// Main 4 was the chapel, the yard, the bell, the Reliquary; the knock at the vestry door went in as step 2. Old
			// step 4 is one past the old last: waiting for Aldana (or, in a save from before turn-ins, the talk with him that
			// is the turn-in now), so one past today's last, waiting for him.
			{ Main4, { 0, 1, 3, 4, 5 } },
			// Side 2 was Amos's talk, the bales, the hands; his talk is his offer now and the bales step 0, the road talk step
			// 1, the hands still step 2. (A side mission's step isn't kept, so only its optional objectives' keys would ever
			// move; none were kept before 2026-10-13. Listed so a record naming it reads right.)
			{ Side2, { 0, 0, 2, 3 } },
		};
		return All;
	}

	/** The side missions that started by themselves before the givers, and the main mission whose end started them. */
	struct FRanByItself
	{
		FName Mission;
		FName After;
	};

	const FRanByItself RanByItself[] = {
		{ Side1, Main3 },
		{ Side2, Main4 },
		{ Side3, Main5 },
	};

	/** "Main4.3.1": a kept optional objective's mission, step and index (FCampaignRecord::OptionalKey); false when it isn't one. */
	bool ParseOptionalKey(FName Key, FName& OutMission, int32& OutStep, int32& OutIndex)
	{
		TArray<FString> Parts;
		Key.ToString().ParseIntoArray(Parts, TEXT("."));
		if (Parts.Num() != 3 || !Parts[1].IsNumeric() || !Parts[2].IsNumeric())
		{
			return false;
		}
		OutMission = FName(*Parts[0]);
		OutStep = FCString::Atoi(*Parts[1]);
		OutIndex = FCString::Atoi(*Parts[2]);
		return true;
	}
}

int32 CampaignStepMigration::MapStepBeforeTalkCut(FName Mission, int32 OldStep)
{
	// Each old step's place in today's list, the last entry one past the old last step (waiting for the turn-in).
	static const TArray<int32> SideTwo = { 0, 1, 1, 2 };
	static const TArray<int32> Knocking = { 0, 0, 1, 2, 3, 4, 4, 5 };
	const TArray<int32>* Moved = Mission == Side2 ? &SideTwo : Mission == KnockingBelow ? &Knocking : nullptr;
	if (!Moved || OldStep < 0)
	{
		return OldStep;
	}
	if (Moved->IsValidIndex(OldStep))
	{
		return (*Moved)[OldStep];
	}
	const int32 Last = Moved->Num() - 1;
	return OldStep + ((*Moved)[Last] - Last);
}

FString CampaignStepMigration::UpgradeTalkCut(FCampaignRecord& Campaign)
{
	TArray<FString> Moved;
	if (!Campaign.ActiveMission.IsNone())
	{
		const int32 Old = Campaign.ActiveMissionStep;
		const int32 New = MapStepBeforeTalkCut(Campaign.ActiveMission, Old);
		if (New != Old)
		{
			Campaign.ActiveMissionStep = New;
			Moved.Add(FString::Printf(TEXT("%s step %d is step %d now"), *Campaign.ActiveMission.ToString(), Old + 1, New + 1));
		}
	}
	TArray<FName> Optionals;
	for (const FName Key : Campaign.OptionalDone)
	{
		FName Mission;
		int32 Step = 0;
		int32 Index = 0;
		if (!ParseOptionalKey(Key, Mission, Step, Index))
		{
			Optionals.AddUnique(Key);
			continue;
		}
		// Dead Lift's first step had Odell's report as its optional (objective 1); it has none now.
		if (Mission == DeadLift && Step == 0)
		{
			Moved.Add(FString::Printf(TEXT("optional %s gone (no longer an objective)"), *Key.ToString()));
			continue;
		}
		const FName Now = FCampaignRecord::OptionalKey(Mission, MapStepBeforeTalkCut(Mission, Step), Index);
		if (Now != Key)
		{
			Moved.Add(FString::Printf(TEXT("optional %s is %s now"), *Key.ToString(), *Now.ToString()));
		}
		Optionals.AddUnique(Now);
	}
	Campaign.OptionalDone = MoveTemp(Optionals);
	// The retired items: never a mission's again (their ids are never reused).
	for (const TCHAR* Retired : { TEXT("WinchCrank"), TEXT("CoalSack") })
	{
		if (Campaign.CarriedItems.Remove(FName(Retired)) > 0)
		{
			Moved.Add(FString::Printf(TEXT("%s no longer carried"), Retired));
		}
	}
	const FString Said = FString::Join(Moved, TEXT("; "));
	if (!Said.IsEmpty())
	{
		UE_LOG(LogLooter, Log, TEXT("Campaign record from before 2026-10-21's cut of the fetches: %s."), *Said);
	}
	return Said;
}

FDateTime CampaignStepMigration::GiversSince()
{
	return FDateTime(2026, 10, 13, 9, 30, 25);
}

FDateTime CampaignStepMigration::StepListsSince()
{
	return FDateTime(2026, 10, 13, 17, 28, 32);
}

int32 CampaignStepMigration::MapOldStep(FName Mission, int32 OldStep)
{
	const FMovedSteps* Moved = MovedSteps().FindByPredicate([Mission](const FMovedSteps& Each) { return Each.Mission == Mission; });
	if (!Moved || OldStep < 0 || Moved->NewStepOf.IsEmpty())
	{
		return OldStep;
	}
	if (Moved->NewStepOf.IsValidIndex(OldStep))
	{
		return Moved->NewStepOf[OldStep];
	}
	// Past the table (a console's step): moved as the last one was.
	const int32 Last = Moved->NewStepOf.Num() - 1;
	return OldStep + (Moved->NewStepOf[Last] - Last);
}

FString CampaignStepMigration::Upgrade(FCampaignRecord& Campaign, const FDateTime& Saved)
{
	TArray<FString> Moved;
	if (Saved < StepListsSince())
	{
		// The story mission's step (only the main one is kept; ResumeRecorded plays it on from there).
		if (!Campaign.ActiveMission.IsNone())
		{
			const int32 Old = Campaign.ActiveMissionStep;
			const int32 New = MapOldStep(Campaign.ActiveMission, Old);
			if (New != Old)
			{
				Campaign.ActiveMissionStep = New;
				Moved.Add(FString::Printf(TEXT("%s step %d is step %d now"), *Campaign.ActiveMission.ToString(), Old + 1, New + 1));
			}
		}
		// The optional objectives done, kept by their step: moved with it, each once.
		TArray<FName> Optionals;
		for (const FName Key : Campaign.OptionalDone)
		{
			FName Mission;
			int32 Step = 0;
			int32 Index = 0;
			const FName Now = ParseOptionalKey(Key, Mission, Step, Index)
				? FCampaignRecord::OptionalKey(Mission, MapOldStep(Mission, Step), Index) : Key;
			if (Now != Key)
			{
				Moved.Add(FString::Printf(TEXT("optional %s is %s now"), *Key.ToString(), *Now.ToString()));
			}
			Optionals.AddUnique(Now);
		}
		Campaign.OptionalDone = MoveTemp(Optionals);
	}
	if (Saved < GiversSince())
	{
		// Running by itself in the old save once its main mission was done: taken, so it plays on, not offered again.
		for (const FRanByItself& Each : RanByItself)
		{
			if (Campaign.HasCompleted(Each.After) && Campaign.Accept(Each.Mission))
			{
				Moved.Add(FString::Printf(TEXT("%s, running by itself then, taken"), *Each.Mission.ToString()));
			}
		}
	}
	const FString Said = FString::Join(Moved, TEXT("; "));
	if (!Said.IsEmpty())
	{
		UE_LOG(LogLooter, Log, TEXT("Campaign record from before 2026-10-13's missions (saved %s): %s."), *Saved.ToString(), *Said);
	}
	return Said;
}

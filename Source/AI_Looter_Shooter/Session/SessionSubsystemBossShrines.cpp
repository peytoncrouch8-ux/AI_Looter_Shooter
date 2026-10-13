// USessionSubsystem: what the vigil stones (World/BossShrine) keep of their bosses in the session: each one's last defeat,
// for its call back five minutes of play after it, or at once once the session has been loaded again (a Save & Quit since,
// counted by ULooterSessionSave::SessionLoads), and whether its stone has risen.

#include "Session/SessionSubsystem.h"
#include "AI_Looter_Shooter.h"
#include "Session/BossRecallRecord.h"
#include "Session/SessionSave.h"
#include "Engine/World.h"

const FBossRecallRecord* USessionSubsystem::FindBossRecall(FName BossKey) const
{
	return Current && !BossKey.IsNone() ? Current->BossRecalls.Find(BossKey) : nullptr;
}

void USessionSubsystem::NoteBossRecallDefeat(UWorld* World, FName BossKey, int32 Level)
{
	if (BossKey.IsNone())
	{
		return;
	}
	// Played without a session, the record lives in memory for this play (and its trips), as the story does.
	GetCampaign();
	if (!Current)
	{
		return;
	}
	FBossRecallRecord& Record = Current->BossRecalls.FindOrAdd(BossKey);
	Record.DefeatedAt = GetPlayedSecondsNow(World);
	Record.DefeatedOn = FDateTime::Now();
	Record.LoadsAtDefeat = Current->SessionLoads;
	Record.Level = Level;
	++Record.Defeats;
	UE_LOG(LogLooter, Log, TEXT("Vigil stone: %s laid to rest at %.1f min of play (load %d, defeat %d)."), *BossKey.ToString(),
		Record.DefeatedAt / 60.0, Record.LoadsAtDefeat, Record.Defeats);
	SaveSoon();
}

void USessionSubsystem::NoteBossShrineRisen(FName BossKey)
{
	GetCampaign();
	if (!Current || BossKey.IsNone())
	{
		return;
	}
	FBossRecallRecord& Record = Current->BossRecalls.FindOrAdd(BossKey);
	if (!Record.bRisen)
	{
		Record.bRisen = true;
		SaveSoon();
	}
}

int32 USessionSubsystem::GetSessionLoads() const
{
	return Current ? Current->SessionLoads : 0;
}

int32 USessionSubsystem::ForgetBossRecalls(FName BossKey)
{
	if (!Current)
	{
		return 0;
	}
	int32 Forgotten = 0;
	if (BossKey.IsNone())
	{
		Forgotten = Current->BossRecalls.Num();
		Current->BossRecalls.Reset();
	}
	else
	{
		Forgotten = Current->BossRecalls.Remove(BossKey);
	}
	if (Forgotten > 0)
	{
		SaveSoon();
	}
	return Forgotten;
}

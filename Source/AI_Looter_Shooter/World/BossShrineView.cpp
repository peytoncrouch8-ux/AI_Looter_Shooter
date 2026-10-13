// ABossShrine: what a vigil stone says of its boss: its name and level, its menu's words and numbers, the map pin's line.

#include "World/BossShrine.h"
#include "Bosses/BossComponent.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/CreatureRankSettings.h"
#include "Creatures/EncounterSpawner.h"
#include "Areas/AreaRulesSubsystem.h"
#include "Progression/PlayerProgressionSubsystem.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Math/RandomStream.h"

#define LOCTEXT_NAMESPACE "LooterBossShrine"

FText ABossShrine::GetBossName() const
{
	if (!BossName.IsEmpty())
	{
		return BossName;
	}
	const ACreatureBase* Boss = PlacedBoss ? PlacedBoss.Get() : (BossClass ? GetDefault<ACreatureBase>(BossClass) : nullptr);
	const UBossComponent* Fight = PlacedBoss ? PlacedBoss->FindComponentByClass<UBossComponent>() : nullptr;
	if (Fight && !Fight->BossName.IsEmpty())
	{
		return Fight->BossName;
	}
	return Boss ? Boss->DisplayName : FText::FromName(BossKey);
}

int32 ABossShrine::GetBossLevel() const
{
	// Out there: as it is.
	if (CallKind == EBossShrineCall::Placed && PlacedBoss && !PlacedBoss->IsDead() && IsBossPresent())
	{
		return PlacedBoss->Level;
	}
	if (CallKind == EBossShrineCall::Lair && Lair)
	{
		for (const ACreatureBase* Each : Lair->GetAliveCreatures())
		{
			if (Each && IsOwnBoss(*Each))
			{
				return Each->Level;
			}
		}
	}
	// As it would rise now: its kind's set level in its area, or the levels where it stands, and its rank's levels on top.
	const ACreatureBase* Kind = BossClass ? GetDefault<ACreatureBase>(BossClass) : nullptr;
	if (!Kind)
	{
		const FBossRecallRecord* Record = GetRecord();
		return Record ? Record->Level : 0;
	}
	const UWorld* World = GetWorld();
	UAreaRulesSubsystem* Rules = World ? World->GetSubsystem<UAreaRulesSubsystem>() : nullptr;
	const FRandomStream NoRoll(0);
	const int32 Own = UAreaRulesSubsystem::RollLevelIn(Rules ? Rules->GetArea() : nullptr, GetArenaSpot(), BossClass, Kind->Level, false,
		NoRoll);
	return FMath::Max(Own + UCreatureRankSettings::Get(Kind->StartingRank).LevelOffset, 1);
}

FBossShrineView ABossShrine::GetView() const
{
	FBossShrineView View;
	const FBossRecallRecord* Record = GetRecord();
	// Read live, so the countdown runs by the frame while the menu is open.
	const BossShrineRules::FInputs In = ReadInputs();
	const BossShrineRules::FVerdict Now = BossShrineRules::Evaluate(In);
	View.State = Now.State;
	View.SecondsLeft = Now.SecondsLeft;
	View.BossName = GetBossName();
	View.Level = GetBossLevel();
	const bool bThisLoad = Record && !BossShrineRules::IsLoadedSince(In.LoadsAtDefeat, In.LoadsNow);
	View.LastKill = BossShrineRules::LastKillText(In.DefeatedAt, In.PlayedNow, Record ? Record->DefeatedOn : FDateTime(),
		FDateTime::Now(), bThisLoad);
	// How many times: the Ledger's count of its kind (every defeat since the first), else what the stone kept.
	int32 Counted = 0;
	const UWorld* World = GetWorld();
	const APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	const ULocalPlayer* Local = PC ? PC->GetLocalPlayer() : nullptr;
	if (const UPlayerProgressionSubsystem* Progress = Local ? Local->GetSubsystem<UPlayerProgressionSubsystem>() : nullptr)
	{
		Counted = BossClass ? Progress->GetDefeated(BossClass) : 0;
	}
	View.TimesBeaten = FMath::Max(Counted, Record ? Record->Defeats : 0);
	View.LootHint = LootHint;
	View.Status = BossShrineRules::StatusText(View.State, View.BossName, View.SecondsLeft);
	return View;
}

FText ABossShrine::GetMapDetail() const
{
	const FText Name = GetBossName();
	switch (Verdict.State)
	{
	case EBossShrineState::Ready:
		return FText::Format(LOCTEXT("MapReady", "{0}: ready to call back"), Name);
	case EBossShrineState::Waiting:
		return FText::Format(LOCTEXT("MapWaiting", "{0}: ready in {1}"), Name, BossShrineRules::Countdown(Verdict.SecondsLeft));
	case EBossShrineState::Present:
		return FText::Format(LOCTEXT("MapPresent", "{0}: out there now"), Name);
	case EBossShrineState::NotYet:
		return FText::Format(LOCTEXT("MapNotYet", "{0}: not yet"), Name);
	case EBossShrineState::Dormant:
	default:
		return Name;
	}
}

#undef LOCTEXT_NAMESPACE

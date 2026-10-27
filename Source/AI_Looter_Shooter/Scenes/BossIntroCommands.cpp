// Developer console commands for the boss intros (not in shipping builds). Their cvars (Looter.BossIntro,
// Looter.BossIntro.Replay) are BossIntro.cpp's.

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "AI_Looter_Shooter.h"
#include "Areas/AreaTravelSubsystem.h"
#include "Bosses/AbelKeeper.h"
#include "Bosses/BossComponent.h"
#include "Creatures/ForemanCreature.h"
#include "Creatures/GravemotherCreature.h"
#include "Scenes/BossIntro.h"
#include "Session/CampaignRecord.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"

namespace
{
	/** The game world the command is for: the one it was typed in, or the running PIE session when typed in the editor. */
	UWorld* FindGameWorld(UWorld* World)
	{
		if (World && World->IsGameWorld())
		{
			return World;
		}
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (Context.World() && Context.World()->IsGameWorld())
			{
				return Context.World();
			}
		}
		return nullptr;
	}

	/** The living one of Type nearest Near. */
	template <typename TBoss>
	TBoss* FindNearest(UWorld& World, const FVector& Near)
	{
		TBoss* Best = nullptr;
		double BestDistance = TNumericLimits<double>::Max();
		for (TActorIterator<TBoss> It(&World); It; ++It)
		{
			const double Distance = FVector::DistSquared(It->GetActorLocation(), Near);
			if (!It->IsDead() && Distance < BestDistance)
			{
				Best = *It;
				BestDistance = Distance;
			}
		}
		return Best;
	}

	/** Its intro now: on a fight already on, at once; else its fight starts with the player and the hook plays it. */
	template <typename TBoss>
	bool PlayOn(UWorld& World, APawn& Player, EBossIntro Kind)
	{
		TBoss* Boss = FindNearest<TBoss>(World, Player.GetActorLocation());
		UBossComponent* Fight = Boss ? Boss->GetBoss() : nullptr;
		if (!Fight)
		{
			return false;
		}
		if (FCampaignRecord* Campaign = UAreaTravelSubsystem::FindCampaign(&World))
		{
			Campaign->ClearFlag(BossIntro::SeenFlag(Kind));
		}
		if (Fight->IsFighting())
		{
			return BossIntro::PlayNow(*Boss);
		}
		Fight->StartFight(&Player);
		return Fight->IsFighting();
	}

	void PlayCommand(const TArray<FString>& Args, UWorld* World)
	{
		UWorld* Played = FindGameWorld(World);
		const APlayerController* Controller = Played ? Played->GetFirstPlayerController() : nullptr;
		APawn* Player = Controller ? Controller->GetPawn() : nullptr;
		EBossIntro Kind = EBossIntro::Gravemother;
		if (!Played || !Player || Args.Num() < 1 || !BossIntro::ParseKind(Args[0], Kind))
		{
			UE_LOG(LogLooter, Display, TEXT("Looter.BossIntro.Play <Gravemother|Foreman|Abel>: in a running game, near the boss "
				"(Abel outside Main 6: Looter.BossIntro.Reset, then Looter.Abel.Fight)."));
			return;
		}
		bool bStarted = false;
		switch (Kind)
		{
		case EBossIntro::Gravemother:
			bStarted = PlayOn<AGravemotherCreature>(*Played, *Player, Kind);
			break;
		case EBossIntro::Foreman:
			bStarted = PlayOn<AForemanCreature>(*Played, *Player, Kind);
			break;
		case EBossIntro::Abel:
		default:
			bStarted = PlayOn<AAbelKeeper>(*Played, *Player, Kind);
			break;
		}
		UE_LOG(LogLooter, Display, TEXT("Looter.BossIntro.Play %s: %s"), BossIntro::KindName(Kind), bStarted
			? TEXT("its fight is on (the intro plays unless a rematch, scenes off or Looter.BossIntro 0).")
			: TEXT("no such boss here, or its fight couldn't start."));
	}

	void ResetCommand(const TArray<FString>& Args, UWorld* World)
	{
		UWorld* Played = FindGameWorld(World);
		FCampaignRecord* Campaign = Played ? UAreaTravelSubsystem::FindCampaign(Played) : nullptr;
		if (!Campaign)
		{
			UE_LOG(LogLooter, Display, TEXT("Looter.BossIntro.Reset: no story running."));
			return;
		}
		int32 Cleared = 0;
		for (const EBossIntro Kind : { EBossIntro::Gravemother, EBossIntro::Foreman, EBossIntro::Abel })
		{
			Cleared += Campaign->ClearFlag(BossIntro::SeenFlag(Kind)) ? 1 : 0;
		}
		UE_LOG(LogLooter, Display, TEXT("Looter.BossIntro.Reset: %d intro(s) due again."), Cleared);
	}

	FAutoConsoleCommandWithWorldAndArgs PlayCmd(
		TEXT("Looter.BossIntro.Play"),
		TEXT("Looter.BossIntro.Play <Gravemother|Foreman|Abel>: the nearest such boss's intro, seen or not (starts its fight)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&PlayCommand));

	FAutoConsoleCommandWithWorldAndArgs ResetCmd(
		TEXT("Looter.BossIntro.Reset"),
		TEXT("Looter.BossIntro.Reset: every boss intro due again in this session."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ResetCommand));
}

#endif

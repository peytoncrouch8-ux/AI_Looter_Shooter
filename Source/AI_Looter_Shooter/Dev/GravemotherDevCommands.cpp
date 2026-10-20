// Developer console commands for the Gravemother's Barrow fight (not in shipping builds): each of her moves on cue, her
// stone and health set, to watch them in a play session (Looter.SpawnCreature Gravemother, slomo, screenshots).

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "AI_Looter_Shooter.h"
#include "Bosses/BossComponent.h"
#include "Combat/HealthComponent.h"
#include "Creatures/GravemotherCreature.h"
#include "Creatures/GravemotherStone.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	UWorld* GravemotherGameWorld(UWorld* World)
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

	/** The Gravemother nearest the player, alive. */
	AGravemotherCreature* NearestMother(UWorld* World, APlayerController*& OutPlayer)
	{
		OutPlayer = World ? World->GetFirstPlayerController() : nullptr;
		const APawn* Pawn = OutPlayer ? OutPlayer->GetPawn() : nullptr;
		AGravemotherCreature* Best = nullptr;
		double BestDistance = TNumericLimits<double>::Max();
		for (TActorIterator<AGravemotherCreature> It(World); It; ++It)
		{
			const double Distance = Pawn ? FVector::DistSquared(It->GetActorLocation(), Pawn->GetActorLocation()) : 0.0;
			if (!It->IsDead() && Distance < BestDistance)
			{
				Best = *It;
				BestDistance = Distance;
			}
		}
		return Best;
	}

	/** Takes Actor's health down to Share of its most as the player's damage (its events, phases and states follow). */
	void HurtToShare(AActor* Actor, float Share, APlayerController* By)
	{
		UHealthComponent* Health = Actor ? Actor->FindComponentByClass<UHealthComponent>() : nullptr;
		if (!Health)
		{
			return;
		}
		const float Wanted = Health->GetMaxHealth() * FMath::Clamp(Share, 0.f, 1.f);
		const float Damage = Health->GetHealth() - Wanted;
		if (Damage > 0.f)
		{
			UGameplayStatics::ApplyDamage(Actor, Damage, By, By ? By->GetPawn() : nullptr, UDamageType::StaticClass());
		}
	}

	void GravemotherDo(const TArray<FString>& Args, UWorld* World)
	{
		UWorld* GameWorld = GravemotherGameWorld(World);
		APlayerController* Player = nullptr;
		AGravemotherCreature* Mother = GameWorld ? NearestMother(GameWorld, Player) : nullptr;
		if (!Mother || Args.IsEmpty())
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Gravemother <Fight|Clod|Furrow|Quake|Unearth|Stun|Break|Stone <%%>|Health <%%>|Pace <0-2>|Hold [0|1]>: ")
				TEXT("needs a living Gravemother in a game (Looter.SpawnCreature Gravemother)."));
			return;
		}
		APawn* Pawn = Player ? Player->GetPawn() : nullptr;
		const FString What = Args[0];
		const float Value = Args.Num() > 1 ? FCString::Atof(*Args[1]) : 0.f;
		// Everything but setting a number starts her fight on the player first.
		auto Fight = [Mother, Pawn]()
		{
			if (Pawn)
			{
				Mother->AlertTo(Pawn);
				if (UBossComponent* Boss = Mother->GetBoss(); Boss && !Boss->IsFighting())
				{
					Boss->StartFight(Pawn);
				}
			}
		};
		if (What.Equals(TEXT("Fight"), ESearchCase::IgnoreCase))
		{
			Fight();
		}
		else if (What.Equals(TEXT("Clod"), ESearchCase::IgnoreCase))
		{
			Fight();
			Mother->ReadyClod();
		}
		else if (What.Equals(TEXT("Furrow"), ESearchCase::IgnoreCase))
		{
			Fight();
			Mother->ReadyFurrow();
		}
		else if (What.Equals(TEXT("Quake"), ESearchCase::IgnoreCase))
		{
			Fight();
			Mother->ApplyPace(FMath::Max(Mother->GetPacePhase(), 1));
			Mother->ReadyQuake();
		}
		else if (What.Equals(TEXT("Unearth"), ESearchCase::IgnoreCase))
		{
			Fight();
			Mother->CallBrood();
		}
		else if (What.Equals(TEXT("Stun"), ESearchCase::IgnoreCase))
		{
			Fight();
			Mother->CounterClod(Player);
		}
		else if (What.Equals(TEXT("Break"), ESearchCase::IgnoreCase))
		{
			Fight();
			if (AGravemotherStone* Stone = Mother->GetStone())
			{
				Stone->Break(false);
			}
		}
		else if (What.Equals(TEXT("Stone"), ESearchCase::IgnoreCase))
		{
			HurtToShare(Mother->GetStone(), Value / 100.f, Player);
		}
		else if (What.Equals(TEXT("Health"), ESearchCase::IgnoreCase))
		{
			HurtToShare(Mother, Value / 100.f, Player);
		}
		else if (What.Equals(TEXT("Pace"), ESearchCase::IgnoreCase))
		{
			Mother->ApplyPace(FMath::RoundToInt32(Value));
		}
		else if (What.Equals(TEXT("Hold"), ESearchCase::IgnoreCase))
		{
			// Staging for a look (screenshots): she stands where she is but still makes her moves; Hold 0 lets her walk
			// again (her pace's chase; a phase change or a reel also gives it back).
			const bool bHold = Args.Num() < 2 || Value != 0.f;
			Mother->WalkSpeed = bHold ? 0.f : GetDefault<AGravemotherCreature>()->WalkSpeed;
			if (bHold)
			{
				Mother->ChaseSpeed = 0.f;
			}
			else
			{
				Mother->ApplyPace(Mother->GetPacePhase());
			}
		}
		else
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Gravemother: no move called '%s'."), *What);
			return;
		}
		UE_LOG(LogLooter, Display, TEXT("Looter.Gravemother %s: done (%s)."), *What, *Mother->GetName());
	}

	FAutoConsoleCommandWithWorldAndArgs GravemotherCommand(TEXT("Looter.Gravemother"),
		TEXT("Looter.Gravemother <Fight|Clod|Furrow|Quake|Unearth|Stun|Break|Stone <%>|Health <%>|Pace <0-2>|Hold [0|1]>: the nearest Gravemother's ")
		TEXT("fight on the player, a move readied for her next attack (Clod, Furrow, Quake), her mounds raised now (Unearth), stunned ")
		TEXT("(Stun), her stone cracked off (Break) or set to a share of its health (Stone), her health set (Health), her pace, held in place for a look (Hold)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&GravemotherDo));
}

#endif

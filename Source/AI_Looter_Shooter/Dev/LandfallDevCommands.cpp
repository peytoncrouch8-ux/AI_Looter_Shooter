// Developer console commands for Landfall (Main 1): jump to a step, put the Lamplighter in a state, clear her deck (not in
// shipping builds).
//   Looter.Landfall [step 0-8]   Main 1 at that step with the ship and the player where it begins (0 sailing in, 1 the
//                                grapples biting and the boarders coming, 2 the deck clear, 3 the hard landing, 4 moored
//                                before the anchor, 5 the climb, 6 at the spyglass, 7 the trail, 8 near the farm)
//   Looter.Landfall.Ship <Sailing|Grappled|Breaking|Landing|Moored|Anchored>   the ship alone, the mission untouched
//   Looter.Landfall.ClearDeck   every boarder gone: the deck is clear (Landfall.DeckClear)

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "AI_Looter_Shooter.h"
#include "Missions/MissionRunner.h"
#include "Scenes/SceneSubsystem.h"
#include "World/LamplighterRules.h"
#include "World/LamplighterShip.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"

namespace
{
	/** The game world the command is for: the one it was typed in, or the running PIE session when typed in the editor. */
	UWorld* FindLandfallWorld(UWorld* World)
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

	ALamplighterShip* FindShipOrSay(UWorld* World, const TCHAR* Command)
	{
		ALamplighterShip* Ship = ALamplighterShip::Find(World);
		if (!Ship)
		{
			UE_LOG(LogLooter, Warning, TEXT("%s: there's no Lamplighter in this level (Ransom's Rest: build_area_landfall.py places her)."), Command);
		}
		return Ship;
	}

	/** The first actor carrying Tag, or null. */
	AActor* FindTagged(UWorld* World, FName Tag)
	{
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			if (It->ActorHasTag(Tag))
			{
				return *It;
			}
		}
		return nullptr;
	}

	/** Stands the player at Where (their feet), facing Toward. */
	void PutPlayer(UWorld* World, const FVector& Where, const FVector& Toward)
	{
		APlayerController* Controller = World->GetFirstPlayerController();
		APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
		if (!Pawn)
		{
			return;
		}
		const FRotator Facing(0.0, (Toward - Where).Rotation().Yaw, 0.0);
		Pawn->TeleportTo(Where + FVector(0.0, 0.0, Pawn->GetDefaultHalfHeight() + 5.0), Facing, false, true);
		Controller->SetControlRotation(Facing);
	}

	/** Main 1 at Step: moved there while it runs, else started there. */
	void SetMainStep(UWorld* World, int32 Step)
	{
		UMissionRunner* Runner = UMissionRunner::Get(World);
		const FName Main = LandfallRules::MissionId();
		if (!Runner)
		{
			return;
		}
		const bool bSet = Runner->IsRunning(Main) ? Runner->SetStep(Main, Step) : Runner->StartMission(Main, Step, /*bForce*/ true);
		if (!bSet)
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Landfall: Main 1 couldn't be put at step %d (its mission asset: create_mission_assets.py)."), Step);
		}
	}

	void LandfallCommand(const TArray<FString>& Args, UWorld* World)
	{
		UWorld* GameWorld = FindLandfallWorld(World);
		ALamplighterShip* Ship = GameWorld ? FindShipOrSay(GameWorld, TEXT("Looter.Landfall")) : nullptr;
		if (!Ship)
		{
			return;
		}
		const int32 Step = FMath::Clamp(Args.Num() > 0 ? FCString::Atoi(*Args[0]) : 0, 0, LandfallRules::StepPorch);
		if (USceneSubsystem* Scenes = USceneSubsystem::Get(GameWorld); Scenes && Scenes->IsPlaying())
		{
			Scenes->SkipScene();
		}
		SetMainStep(GameWorld, Step);
		switch (Step)
		{
		case LandfallRules::StepProw:
			Ship->SetFlight(EShipFlight::Sailing);
			break;
		case LandfallRules::StepDeckFight:
			Ship->SetFlight(EShipFlight::Grappled);
			break;
		case LandfallRules::StepGrapples:
			Ship->SetFlight(EShipFlight::Grappled);
			Ship->ClearDeck();
			break;
		case LandfallRules::StepLanding:
			Ship->SetFlight(EShipFlight::Landing);
			break;
		case LandfallRules::StepAnchor:
			Ship->SetFlight(EShipFlight::Moored);
			Ship->PutPlayerOnDeck();
			break;
		default:
		{
			Ship->SetFlight(EShipFlight::Moored);
			Ship->SetAnchored(/*bPlayed*/ false);
			// Where the step begins: off her gangway for the climb, at the spyglass for the look and the trail, near the farm.
			const FName Near = Step >= LandfallRules::StepPorch ? FName(TEXT("Speaker_Delia")) : Step >= LandfallRules::StepSpyglass
				? LandfallRules::SpyglassTag() : NAME_None;
			if (AActor* Spot = Near.IsNone() ? nullptr : FindTagged(GameWorld, Near))
			{
				const FVector At = Spot->GetActorLocation();
				const FVector Back = Spot->GetActorForwardVector() * (Step >= LandfallRules::StepPorch ? -1200.0 : -180.0);
				PutPlayer(GameWorld, At + Back, At);
			}
			else
			{
				const float Side = Ship->GangwaySide >= 0.f ? 1.f : -1.f;
				const FVector Off = Ship->DeckPoint(FVector(-25.0, Side * (ALamplighterShip::RailOut + 500.0), 0.0));
				PutPlayer(GameWorld, Off, Ship->GetActorLocation());
			}
			break;
		}
		}
		UE_LOG(LogLooter, Display, TEXT("Looter.Landfall: Main 1 at step %d, the Lamplighter %s."), Step, *UEnum::GetValueAsString(Ship->GetFlight()));
	}

	void ShipCommand(const TArray<FString>& Args, UWorld* World)
	{
		UWorld* GameWorld = FindLandfallWorld(World);
		ALamplighterShip* Ship = GameWorld ? FindShipOrSay(GameWorld, TEXT("Looter.Landfall.Ship")) : nullptr;
		if (!Ship)
		{
			return;
		}
		const FString Wanted = Args.Num() > 0 ? Args[0] : FString();
		struct FNamed { const TCHAR* Name; EShipFlight State; };
		const FNamed States[] = { { TEXT("Sailing"), EShipFlight::Sailing }, { TEXT("Grappled"), EShipFlight::Grappled },
			{ TEXT("Breaking"), EShipFlight::Breaking }, { TEXT("Landing"), EShipFlight::Landing }, { TEXT("Moored"), EShipFlight::Moored },
			{ TEXT("Anchored"), EShipFlight::Moored } };
		for (const FNamed& Named : States)
		{
			if (Wanted.Equals(Named.Name, ESearchCase::IgnoreCase))
			{
				Ship->SetFlight(Named.State);
				if (Wanted.Equals(TEXT("Anchored"), ESearchCase::IgnoreCase))
				{
					Ship->SetAnchored(/*bPlayed*/ true);
				}
				UE_LOG(LogLooter, Display, TEXT("Looter.Landfall.Ship: %s."), *UEnum::GetValueAsString(Ship->GetFlight()));
				return;
			}
		}
		UE_LOG(LogLooter, Warning, TEXT("Usage: Looter.Landfall.Ship <Sailing|Grappled|Breaking|Landing|Moored|Anchored> (now %s)."),
			*UEnum::GetValueAsString(Ship->GetFlight()));
	}

	void ClearDeckCommand(const TArray<FString>& Args, UWorld* World)
	{
		UWorld* GameWorld = FindLandfallWorld(World);
		ALamplighterShip* Ship = GameWorld ? FindShipOrSay(GameWorld, TEXT("Looter.Landfall.ClearDeck")) : nullptr;
		if (Ship)
		{
			Ship->ClearDeck();
			UE_LOG(LogLooter, Display, TEXT("Looter.Landfall.ClearDeck: %s."), Ship->IsDeckClear() ? TEXT("the deck is clear")
				: TEXT("nothing to clear (she isn't grappled)"));
		}
	}

	FAutoConsoleCommandWithWorldAndArgs LandfallCommandRegistration(
		TEXT("Looter.Landfall"),
		TEXT("Landfall (Main 1) at a step, the Lamplighter and the player where it begins: Looter.Landfall [0-8] (0 sailing in, 1 the boarders, 2 the deck clear, 3 the hard landing, 4 moored, 5 the climb, 6 the spyglass, 7 the trail, 8 the farm)"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&LandfallCommand));

	FAutoConsoleCommandWithWorldAndArgs ShipCommandRegistration(
		TEXT("Looter.Landfall.Ship"),
		TEXT("Puts the Lamplighter in a state (the mission untouched): Looter.Landfall.Ship <Sailing|Grappled|Breaking|Landing|Moored|Anchored>"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ShipCommand));

	FAutoConsoleCommandWithWorldAndArgs ClearDeckRegistration(
		TEXT("Looter.Landfall.ClearDeck"),
		TEXT("Every boarder on the Lamplighter's deck gone: the deck is clear (the mission hears Landfall.DeckClear)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ClearDeckCommand));
}

#endif

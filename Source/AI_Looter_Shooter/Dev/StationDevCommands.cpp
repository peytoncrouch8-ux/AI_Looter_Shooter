// Developer console commands for the station boards, Skyreach's skiff jetty and leaving Skyreach (not in shipping builds).

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "AI_Looter_Shooter.h"
#include "Areas/AreaDefinition.h"
#include "Areas/AreaTravelSubsystem.h"
#include "Areas/StationBoard.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Progression/PlayerProgressionSubsystem.h"
#include "Session/CampaignRecord.h"
#include "Session/SessionSubsystem.h"
#include "Tutorial/TutorialDirector.h"
#include "UI/HUD/LooterHUD.h"
#include "Affixes/WeaponRollLibrary.h"
#include "Weapons/WeaponDefinition.h"
#include "World/SkiffJetty.h"
#include "World/TrainStation.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"

namespace
{
	/** The game world the command is for: the one it was typed in, or the running play session when typed in the editor. */
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

	APawn* FindPlayerPawn(const UWorld* World)
	{
		const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
		return Controller ? Controller->GetPawn() : nullptr;
	}

	ASkiffJetty* FindJetty(UWorld* World)
	{
		if (!World)
		{
			return nullptr;
		}
		TActorIterator<ASkiffJetty> It(World);
		return It ? *It : nullptr;
	}

	/** The board nearest the player: a jetty's or a station's (null: none in the level). */
	AActor* FindNearestBoard(UWorld* World, const FVector& From)
	{
		AActor* Nearest = nullptr;
		if (!World)
		{
			return Nearest;
		}
		double NearestDistance = TNumericLimits<double>::Max();
		auto Consider = [&](AActor* Board)
		{
			const double Distance = FVector::DistSquared(Board->GetActorLocation(), From);
			if (Distance < NearestDistance)
			{
				Nearest = Board;
				NearestDistance = Distance;
			}
		};
		for (TActorIterator<ASkiffJetty> It(World); It; ++It)
		{
			Consider(*It);
		}
		for (TActorIterator<ATrainStation> It(World); It; ++It)
		{
			Consider(*It);
		}
		return Nearest;
	}

	FString DescribeLine(const FStationBoardLine& Line)
	{
		if (Line.Kind == EStationLine::NextMission)
		{
			return FString::Printf(TEXT("%s (the blank line: %s opens the next area)"), *Line.Name.ToString(), *Line.MissionId.ToString());
		}
		TArray<FString> Notes;
		Notes.Add(Line.Kind == EStationLine::Practice ? TEXT("practice") : TEXT("area"));
		if (Line.bFirstCastOff)
		{
			Notes.Add(TEXT("the first cast-off"));
		}
		if (Line.bHere)
		{
			Notes.Add(TEXT("here"));
		}
		if (!Line.bLevelBuilt)
		{
			Notes.Add(TEXT("level not in the game yet"));
		}
		return FString::Printf(TEXT("%s -> %s at %s (%s)"), *Line.Name.ToString(), *Line.AreaId.ToString(),
			Line.Landing.IsNone() ? TEXT("its start") : *Line.Landing.ToString(), *FString::Join(Notes, TEXT(", ")));
	}

	/** Looter.Station.Board: the station board, as the nearest jetty or station would open it (else the level's own). */
	void OpenBoard(const TArray<FString>& Args, UWorld* World)
	{
		UWorld* GameWorld = FindGameWorld(World);
		const APawn* Pawn = FindPlayerPawn(GameWorld);
		ALooterHUD* HUD = ALooterHUD::FindFor(Pawn);
		if (!Pawn || !HUD)
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Station.Board: start the game first (it needs a player and their HUD)."));
			return;
		}
		AActor* Board = FindNearestBoard(GameWorld, Pawn->GetActorLocation());
		FStationBoardWords Words = FStationBoardWords::Station();
		if (const ASkiffJetty* Jetty = Cast<ASkiffJetty>(Board))
		{
			Words = Jetty->GetBoardWords();
		}
		else if (const ATrainStation* Station = Cast<ATrainStation>(Board))
		{
			Words = Station->GetBoardWords();
		}
		if (!HUD->OpenStationBoard(Board, Words))
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Station.Board: the board couldn't open (the pause menu is up?)."));
		}
	}

	/** Looter.Station.CastOff: the first cast-off from the level's jetty (the ride), or straight on behind the white. */
	void CastOff(const TArray<FString>& Args, UWorld* World)
	{
		UWorld* GameWorld = FindGameWorld(World);
		UAreaTravelSubsystem* Travel = UAreaTravelSubsystem::Get(GameWorld);
		if (!Travel)
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Station.CastOff: start the game first."));
			return;
		}
		if (ASkiffJetty* Jetty = FindJetty(GameWorld))
		{
			if (Jetty->CastOff())
			{
				return;
			}
			UE_LOG(LogLooter, Warning, TEXT("Looter.Station.CastOff: the jetty is casting off already, or has no skiff."));
			return;
		}
		UE_LOG(LogLooter, Log, TEXT("Looter.Station.CastOff: no jetty here, so straight on to the story's first arrival."));
		Travel->LeaveForFirstArrival();
	}

	/** Looter.Station.Gangplank up|down: the level's jetty's plank, whatever the story says, until the level ends. */
	void Gangplank(const TArray<FString>& Args, UWorld* World)
	{
		ASkiffJetty* Jetty = FindJetty(FindGameWorld(World));
		const bool bDown = Args.Num() == 1 && Args[0].Equals(TEXT("down"), ESearchCase::IgnoreCase);
		const bool bUp = Args.Num() == 1 && Args[0].Equals(TEXT("up"), ESearchCase::IgnoreCase);
		if (!Jetty || (!bDown && !bUp))
		{
			UE_LOG(LogLooter, Warning, TEXT("Usage (in a level with the skiff jetty): Looter.Station.Gangplank up|down"));
			return;
		}
		Jetty->ForceGangplank(bDown);
		UE_LOG(LogLooter, Log, TEXT("Looter.Station.Gangplank: %s"), bDown ? TEXT("down (the bell rings)") : TEXT("up"));
	}

	/**
	 * Looter.Station.SkipTutorial [stay]: "Skip the tutorial" in the session being played: the tutorial done, the first
	 * cast-off recorded, a Common Bullpup in hand when the player carries no gun, then the story's first arrival (or,
	 * with stay, right here: Skyreach as a practice island).
	 */
	void SkipTutorial(const TArray<FString>& Args, UWorld* World)
	{
		UWorld* GameWorld = FindGameWorld(World);
		FCampaignRecord* Campaign = UAreaTravelSubsystem::FindCampaign(GameWorld);
		APawn* Pawn = FindPlayerPawn(GameWorld);
		if (!Campaign || !Pawn)
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Station.SkipTutorial: start the game first."));
			return;
		}
		for (TActorIterator<ATutorialDirector> It(GameWorld); It; ++It)
		{
			if (It->GetCurrentStep() != INDEX_NONE)
			{
				It->Skip();
			}
		}
		const UGameInstance* GameInstance = GameWorld->GetGameInstance();
		const ULocalPlayer* Player = GameInstance ? GameInstance->GetFirstGamePlayer() : nullptr;
		if (UPlayerProgressionSubsystem* Progression = Player ? Player->GetSubsystem<UPlayerProgressionSubsystem>() : nullptr)
		{
			Progression->SetTutorialDone(true);
		}
		StationBoard::RecordFirstCastOff(*Campaign);

		// The coffin's Bullpup, for a player who carries no gun.
		UWeaponManagerComponent* Manager = Pawn->FindComponentByClass<UWeaponManagerComponent>();
		UWeaponDefinition* Bullpup = USessionSubsystem::LoadBullpup();
		if (Manager && Bullpup && Manager->GetWeapons().IsEmpty())
		{
			const FWeaponInstanceData Gun = UWeaponRollLibrary::RollWeaponWithRarity(Bullpup, EWeaponRarity::Common, 1);
			if (Manager->GiveWeapon(Gun))
			{
				Manager->AddAmmo(Bullpup->AmmoType, Gun.Stats.MagazineSize * Bullpup->StartingReserveMagazines);
			}
		}
		UE_LOG(LogLooter, Log, TEXT("Looter.Station.SkipTutorial: the tutorial is done and the first cast-off recorded."));
		if (USessionSubsystem* Sessions = USessionSubsystem::Get(GameWorld))
		{
			Sessions->SaveSoon();
		}

		const bool bStay = Args.Num() == 1 && Args[0].Equals(TEXT("stay"), ESearchCase::IgnoreCase);
		UAreaTravelSubsystem* Travel = UAreaTravelSubsystem::Get(GameWorld);
		if (!bStay && Travel)
		{
			Travel->LeaveForFirstArrival();
		}
	}

	/** Looter.Station.Lines: the board's lines here, as the story stands. */
	void ListLines(const TArray<FString>& Args, UWorld* World)
	{
		UWorld* GameWorld = FindGameWorld(World);
		if (!GameWorld)
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Station.Lines: start the game first."));
			return;
		}
		TArray<UAreaDefinition*> Areas;
		const TArray<FStationBoardLine> Lines = UAreaTravelSubsystem::GatherBoardLines(GameWorld, Areas);
		const FCampaignRecord* Campaign = UAreaTravelSubsystem::FindCampaign(GameWorld);
		UE_LOG(LogLooter, Log, TEXT("Station board here (%s): %d lines; first cast-off %s, %d areas opened"),
			*USessionSubsystem::AreaName(USessionSubsystem::MapOf(GameWorld)), Lines.Num(),
			Campaign && Campaign->bFirstCastOff ? TEXT("made") : TEXT("not yet"), Campaign ? Campaign->OpenedAreas.Num() : 0);
		for (int32 Index = 0; Index < Lines.Num(); ++Index)
		{
			UE_LOG(LogLooter, Log, TEXT("  %d. %s"), Index + 1, *DescribeLine(Lines[Index]));
		}
	}

	FAutoConsoleCommandWithWorldAndArgs OpenBoardCommand(
		TEXT("Looter.Station.Board"),
		TEXT("Opens the station board, as the nearest jetty or station would (else the level's own)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&OpenBoard));

	FAutoConsoleCommandWithWorldAndArgs CastOffCommand(
		TEXT("Looter.Station.CastOff"),
		TEXT("The first cast-off from the level's skiff jetty: the ride, then the story's first arrival (without a jetty or a ride, straight on behind the white)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&CastOff));

	FAutoConsoleCommandWithWorldAndArgs GangplankCommand(
		TEXT("Looter.Station.Gangplank"),
		TEXT("Raises or lowers the skiff jetty's gangplank whatever the story says: Looter.Station.Gangplank up|down"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Gangplank));

	FAutoConsoleCommandWithWorldAndArgs SkipTutorialCommand(
		TEXT("Looter.Station.SkipTutorial"),
		TEXT("Skip the tutorial in the session being played (the first cast-off, a Bullpup if unarmed), then the story's first arrival: Looter.Station.SkipTutorial [stay]"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&SkipTutorial));

	FAutoConsoleCommandWithWorldAndArgs ListLinesCommand(
		TEXT("Looter.Station.Lines"),
		TEXT("Lists the station board's lines here, as the story stands."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ListLines));
}

#endif

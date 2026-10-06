// Developer console commands for scenes and the transition screen (not in shipping builds).

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "AI_Looter_Shooter.h"
#include "Scenes/SceneSubsystem.h"
#include "Scenes/SkiffRide.h"
#include "Scenes/TransitionScreenSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
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

	/**
	 * The skiff nearest the player: an actor carrying a skiff mesh with a Deck socket (the jetty's skiff, or a
	 * SM_Skiff_A_Packet placed to try the ride). Gangplanks and other skiff parts have no deck, so they never count.
	 */
	AActor* FindSkiff(UWorld* World)
	{
		const APlayerController* Controller = World->GetFirstPlayerController();
		const APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
		const FVector From = Pawn ? Pawn->GetActorLocation() : FVector::ZeroVector;
		AActor* Nearest = nullptr;
		double NearestDistance = TNumericLimits<double>::Max();
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			TInlineComponentArray<UStaticMeshComponent*> Meshes(*It);
			const bool bSkiff = Meshes.ContainsByPredicate([](const UStaticMeshComponent* Mesh)
			{
				const UStaticMesh* Model = Mesh ? Mesh->GetStaticMesh() : nullptr;
				return Model && Model->GetName().Contains(TEXT("Skiff")) && Mesh->DoesSocketExist(SkiffRide::DeckSocket());
			});
			const double Distance = FVector::Distance(It->GetActorLocation(), From);
			if (bSkiff && Distance < NearestDistance)
			{
				Nearest = *It;
				NearestDistance = Distance;
			}
		}
		return Nearest;
	}

	/** Looter.Scene.Skip: the scene playing goes to its end, its moments all happening. */
	void SkipCommand(const TArray<FString>& Args, UWorld* World)
	{
		USceneSubsystem* Scenes = USceneSubsystem::Get(FindGameWorld(World));
		if (!Scenes || !Scenes->IsPlaying())
		{
			UE_LOG(LogLooter, Display, TEXT("Looter.Scene.Skip: no scene is playing."));
			return;
		}
		Scenes->SkipScene();
	}

	/**
	 * Looter.Scene.Ride: the first cast-off's ride on the nearest skiff. Nothing travels: at full white the skiff goes back
	 * to its moorings, the player back where they stood, and the white is revealed with the title card.
	 */
	void RideCommand(const TArray<FString>& Args, UWorld* World)
	{
		UWorld* GameWorld = FindGameWorld(World);
		USceneSubsystem* Scenes = USceneSubsystem::Get(GameWorld);
		if (!Scenes)
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Scene.Ride: start the game first."));
			return;
		}
		AActor* Skiff = FindSkiff(GameWorld);
		if (!Skiff)
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Scene.Ride: there's no skiff in the level (an actor with a skiff mesh that has a Deck socket)."));
			return;
		}
		const TWeakObjectPtr<USceneSubsystem> WeakScenes = Scenes;
		const bool bPlays = Scenes->PlaySkiffRide(Skiff, FOnSceneMoment::CreateLambda([WeakScenes]()
		{
			if (USceneSubsystem* Played = WeakScenes.Get())
			{
				Played->ReturnFromScene(UTransitionScreenSubsystem::GameTitle());
			}
		}));
		if (bPlays)
		{
			UE_LOG(LogLooter, Display, TEXT("Looter.Scene.Ride: riding %s; no trip at the end."), *Skiff->GetName());
		}
		else
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Scene.Ride: it doesn't play (a scene is playing, or scenes are off: Looter.Scenes 2 forces them on)."));
		}
	}

	/** Looter.Scene.Title [text]: the white held, then revealed with the title (REVENANT, or the words given). */
	void TitleCommand(const TArray<FString>& Args, UWorld* World)
	{
		UTransitionScreenSubsystem* Screen = UTransitionScreenSubsystem::Get(FindGameWorld(World));
		if (!Screen)
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Scene.Title: start the game first."));
			return;
		}
		const FString Words = FString::Join(Args, TEXT(" "));
		Screen->HoldWhite();
		Screen->Reveal(Words.IsEmpty() ? UTransitionScreenSubsystem::GameTitle() : FText::FromString(Words.ToUpper()));
	}

	FAutoConsoleCommandWithWorldAndArgs SkipRegistration(
		TEXT("Looter.Scene.Skip"),
		TEXT("Skips the scene playing to its end; its moments (travel among them) still happen."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&SkipCommand));

	FAutoConsoleCommandWithWorldAndArgs RideRegistration(
		TEXT("Looter.Scene.Ride"),
		TEXT("Plays the first cast-off's skiff ride on the nearest skiff, then (with no trip) puts everything back and reveals the white with the title."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&RideCommand));

	FAutoConsoleCommandWithWorldAndArgs TitleRegistration(
		TEXT("Looter.Scene.Title"),
		TEXT("Holds the white, then reveals it with a title rising through it: Looter.Scene.Title [text] (REVENANT by default)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&TitleCommand));
}

#endif

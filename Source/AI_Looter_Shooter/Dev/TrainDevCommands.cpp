// Developer console commands for the train and Main 7, "The Lantern Leans" (not in shipping builds): the train's two
// shots, its look and its running gear, Delia's hand-off and the lantern's leaning flame.

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "AI_Looter_Shooter.h"
#include "Areas/AreaLandings.h"
#include "Scenes/SceneSubsystem.h"
#include "Scenes/TrainShots.h"
#include "Story/DoorHandoff.h"
#include "World/LanternFlame.h"
#include "World/Train.h"
#include "World/TrainStation.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
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

	/** The level's train, logged when there's none. */
	ATrain* FindTrain(UWorld* World, const TCHAR* Command)
	{
		ATrain* Train = ATrain::FindIn(World);
		if (!Train)
		{
			UE_LOG(LogLooter, Warning, TEXT("%s: start the game in a level with the train (Ransom's Rest's depot)."), Command);
		}
		return Train;
	}

	/** Where a trip by Train lands the player: its station's landing on the platform, else the hearse car's door. */
	FTransform LandingFor(UWorld* World, const ATrain& Train)
	{
		ATrainStation* Nearest = nullptr;
		double NearestDistance = TNumericLimits<double>::Max();
		for (TActorIterator<ATrainStation> It(World); It; ++It)
		{
			const double Distance = FVector::DistSquared(It->GetActorLocation(), Train.GetActorLocation());
			if (Distance < NearestDistance)
			{
				Nearest = *It;
				NearestDistance = Distance;
			}
		}
		if (Nearest)
		{
			const FTransform Spot = AreaLandings::GetSpot(Nearest, Nearest->LandingName);
			return FTransform(FRotator(0.0, Spot.Rotator().Yaw, 0.0), Spot.GetLocation());
		}
		return Train.GetArrivalSpot();
	}

	/** Looter.Train.Depart: the departure shot, with no trip after it: the train comes back and the black lifts. */
	void Depart(const TArray<FString>& Args, UWorld* World)
	{
		UWorld* GameWorld = FindGameWorld(World);
		ATrain* Train = FindTrain(GameWorld, TEXT("Looter.Train.Depart"));
		USceneSubsystem* Scenes = USceneSubsystem::Get(GameWorld);
		if (!Train || !Scenes)
		{
			return;
		}
		if (!Scenes->Play(TrainShots::MakeDeparture(*Scenes, *Train, TFunction<void()>(), /*bTripFollows*/ false)))
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Train.Depart: it can't play now (a scene is playing, or scenes are off: Looter.Scenes 2)."));
		}
	}

	/** Looter.Train.Arrive: the arrival shot, the player handed back at the station's landing by the hearse car's door. */
	void Arrive(const TArray<FString>& Args, UWorld* World)
	{
		UWorld* GameWorld = FindGameWorld(World);
		ATrain* Train = FindTrain(GameWorld, TEXT("Looter.Train.Arrive"));
		USceneSubsystem* Scenes = USceneSubsystem::Get(GameWorld);
		if (!Train || !Scenes)
		{
			return;
		}
		if (!Scenes->Play(TrainShots::MakeArrival(*Scenes, *Train, LandingFor(GameWorld, *Train))))
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Train.Arrive: it can't play now (a scene is playing, or scenes are off: Looter.Scenes 2)."));
		}
	}

	/** Looter.Train.Warm [1|0]: steam up or cold, whatever the story says, until the level ends; no word: as the story says. */
	void Warm(const TArray<FString>& Args, UWorld* World)
	{
		ATrain* Train = FindTrain(FindGameWorld(World), TEXT("Looter.Train.Warm"));
		if (!Train)
		{
			return;
		}
		if (Args.IsEmpty())
		{
			Train->RefreshStory();
		}
		else
		{
			Train->HoldLook(Args[0] != TEXT("0"));
		}
		UE_LOG(LogLooter, Log, TEXT("Looter.Train.Warm: %s (the story says %s)"), Train->IsWarm() ? TEXT("warm") : TEXT("cold"),
			*Train->WarmWhen.Describe());
	}

	/** Looter.Train.Move <metres>: slides the train that far along its track from the platform, its wheels turning. */
	void Move(const TArray<FString>& Args, UWorld* World)
	{
		ATrain* Train = FindTrain(FindGameWorld(World), TEXT("Looter.Train.Move"));
		if (!Train)
		{
			return;
		}
		const float Metres = Args.IsEmpty() ? 0.f : FCString::Atof(*Args[0]);
		Train->SetTravel(Metres * 100.f);
		UE_LOG(LogLooter, Log, TEXT("Looter.Train.Move: %.1f m out along the track (0 puts it back)."), Metres);
	}

	/** Looter.Train.Info: the train as it's put together and how it stands. */
	void Info(const TArray<FString>& Args, UWorld* World)
	{
		const ATrain* Train = FindTrain(FindGameWorld(World), TEXT("Looter.Train.Info"));
		if (!Train)
		{
			return;
		}
		UE_LOG(LogLooter, Log, TEXT("Train %s: cars at %.0f, %.0f and %.0f cm; %d axles (%d on drivers); %s, the hearse car's door %s; %.0f cm out."),
			*Train->GetActorNameOrLabel(), Train->GetCarOffset(0), Train->GetCarOffset(1), Train->GetCarOffset(2), Train->GetNumAxles(),
			Train->GetNumDriverAxles(), Train->IsWarm() ? TEXT("warm") : TEXT("cold"), Train->IsDoorOpen() ? TEXT("open") : TEXT("shut"),
			Train->GetTravel());
	}

	/** Looter.Story.Handoff [force]: Delia's hand-off now, as Main 7's first step ending does (force: again, once done). */
	void Handoff(const TArray<FString>& Args, UWorld* World)
	{
		UWorld* GameWorld = FindGameWorld(World);
		ADoorHandoff* Handing = nullptr;
		if (GameWorld)
		{
			TActorIterator<ADoorHandoff> First(GameWorld);
			Handing = First ? *First : nullptr;
		}
		if (!Handing)
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Story.Handoff: start the game on Ransom's Rest (its farmhouse has the hand-off)."));
			return;
		}
		const bool bForce = Args.Num() == 1 && Args[0].Equals(TEXT("force"), ESearchCase::IgnoreCase);
		if (Handing->HandOut(bForce))
		{
			UE_LOG(LogLooter, Log, TEXT("Looter.Story.Handoff: %s comes out through the door."), *Handing->FindGunId().ToString());
		}
		else
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Story.Handoff: not handed out (it's under way or done: add force; or no named gun)."));
		}
	}

	/** Looter.Story.Lean [lit|out]: the lantern's flame lit or out, whatever the story says; with no word, its state. */
	void Lean(const TArray<FString>& Args, UWorld* World)
	{
		UWorld* GameWorld = FindGameWorld(World);
		int32 Found = 0;
		if (!GameWorld)
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Story.Lean: start the game first."));
			return;
		}
		for (TActorIterator<ALanternFlame> It(GameWorld); It; ++It)
		{
			++Found;
			if (Args.Num() == 1)
			{
				It->SetLit(Args[0].Equals(TEXT("lit"), ESearchCase::IgnoreCase));
			}
			const FVector Axis = It->GetFlameAxis();
			UE_LOG(LogLooter, Log, TEXT("Looter.Story.Lean: %s %s, leaning toward bearing %.0f (axis %.2f, %.2f, %.2f); the story: %s"),
				*It->GetActorNameOrLabel(), It->IsLit() ? TEXT("lit") : TEXT("out"), It->Bearing, Axis.X, Axis.Y, Axis.Z,
				*It->ShownWhen.Describe());
		}
		if (Found == 0)
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Story.Lean: no lantern flame here (start the game on Ransom's Rest)."));
		}
	}

	FAutoConsoleCommandWithWorldAndArgs DepartCommand(
		TEXT("Looter.Train.Depart"),
		TEXT("The train's departure shot (it pulls out, then black), with no trip: the train comes back and the black lifts."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Depart));

	FAutoConsoleCommandWithWorldAndArgs ArriveCommand(
		TEXT("Looter.Train.Arrive"),
		TEXT("The train's arrival shot (out of the black it backs in to the platform), the player handed back at the hearse car's door."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Arrive));

	FAutoConsoleCommandWithWorldAndArgs WarmCommand(
		TEXT("Looter.Train.Warm"),
		TEXT("Steam up (1) or cold and shut (0) whatever the story says, until the level ends; no word: as the story says. Looter.Train.Warm [1|0]"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Warm));

	FAutoConsoleCommandWithWorldAndArgs MoveCommand(
		TEXT("Looter.Train.Move"),
		TEXT("Slides the train along its track from the platform, its wheels and rods turning: Looter.Train.Move <metres> (0 puts it back)"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Move));

	FAutoConsoleCommandWithWorldAndArgs InfoCommand(
		TEXT("Looter.Train.Info"),
		TEXT("Prints how the train is put together and how it stands (cars, axles, warm or cold, the hearse car's door)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Info));

	FAutoConsoleCommandWithWorldAndArgs HandoffCommand(
		TEXT("Looter.Story.Handoff"),
		TEXT("Delia's hand-off now, as Main 7's first step ending does: the door opens a crack, Heirloom comes out. Looter.Story.Handoff [force]"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Handoff));

	FAutoConsoleCommandWithWorldAndArgs LeanCommand(
		TEXT("Looter.Story.Lean"),
		TEXT("The Keeper's Lantern's leaning flame lit or out whatever the story says, or its state: Looter.Story.Lean [lit|out]"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Lean));
}

#endif

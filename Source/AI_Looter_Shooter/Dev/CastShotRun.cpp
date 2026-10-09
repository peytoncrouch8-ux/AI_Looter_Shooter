// FCastShotRun's steps and clock (CastShotRun.h): setting each scene up, letting it run, stopping time, the pictures,
// and putting everything back. CastShotRunNumbers.cpp takes the numbers. Developer builds only.

#include "Dev/CastShotRun.h"
#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "AI_Looter_Shooter.h"
#include "Creatures/CreatureBase.h"
#include "Player/PlayerViewComponent.h"
#include "Story/AmosWhitlock.h"
#include "Camera/CameraActor.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/HUD.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformMisc.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

namespace
{
	namespace Scene = CastShotScene;

	// Real seconds, the run's own clock.
	constexpr float PlayerWaitSeconds = 120.f;
	/** After setting up: the level's first frames, streaming and the exposure. */
	constexpr float WarmupSeconds = 5.f;
	/** A fresh creature stands this long before its state starts (its pose and feet settle). */
	constexpr float SettleSeconds = 0.4f;
	/** After the camera moves (a cut): the frames anti-aliasing needs to settle. Then the gap after a picture's request. */
	constexpr float CameraSettleSeconds = 0.35f;
	constexpr float ShotGapSeconds = 0.3f;
	/** A pack closes on the player this long; the player walks this long; a character stands this long. */
	constexpr float PackSeconds = 2.5f;
	constexpr float PlayerWalkSeconds = 1.6f;
	constexpr float CharacterSeconds = 0.6f;
	/** The fastest the player walks (cm/s, a little over its 510): its walk's way must be clear this far a second. */
	constexpr float PlayerTopSpeed = 600.f;
	/** Time stopped for the pictures (the engine's slowest). */
	constexpr float Stopped = 0.0001f;
	/** The level's own creatures this near a spot (cm) are cleared away. */
	constexpr float ClearRadius = 6000.f;
	/** The player waits this far behind a spot, out of the pictures, unless a state calls them over (cm). */
	constexpr float PlayerAway = 3000.f;
}

FCastShotRun::FCastShotRun(bool bInQuit, TArray<FString> InOnly, TOptional<FVector> InFlat, TOptional<FVector> InSlope, FString InTag)
	: bQuit(bInQuit), Only(MoveTemp(InOnly)), GivenFlat(InFlat), GivenSlope(InSlope), Tag(MoveTemp(InTag))
{
}

FString FCastShotRun::Directory(const FString& Tag)
{
	const FString Base = FPaths::ProjectSavedDir() / TEXT("Screenshots/CastShots");
	return Tag.IsEmpty() ? Base : Base / Tag;
}

bool FCastShotRun::Tick(float DeltaTime)
{
	Clock += DeltaTime;
	if (Phase == EPhase::Waiting)
	{
		return TickWaiting();
	}
	UWorld* World = WorldPtr.Get();
	APawn* Pawn = PawnPtr.Get();
	if (!World || !Pawn || !CameraPtr.IsValid())
	{
		UE_LOG(LogLooter, Warning, TEXT("Looter.CastShots: the world or the player went away; stopped."));
		return Finish(false);
	}
	switch (Phase)
	{
	case EPhase::Warmup:
		return Clock >= WarmupSeconds ? BeginStep(*World, 0) : true;
	case EPhase::Settle:
		return TickSettle(*World, *Pawn);
	case EPhase::Run:
		return TickRun(*World, *Pawn);
	case EPhase::Shoot:
		return TickShoot(*World);
	default:
		return true;
	}
}

bool FCastShotRun::TickWaiting()
{
	// The level may still be loading when the command runs (-ExecCmds): look again each frame.
	UWorld* Found = nullptr;
	for (const FWorldContext& Context : GEngine->GetWorldContexts())
	{
		if (Context.World() && Context.World()->IsGameWorld() && Context.World()->GetFirstPlayerController())
		{
			Found = Context.World();
		}
	}
	APlayerController* Controller = Found ? Found->GetFirstPlayerController() : nullptr;
	if (Controller && Controller->GetPawn() && Found->HasBegunPlay())
	{
		Prepare(*Found, *Controller);
		return true;
	}
	if (Clock > PlayerWaitSeconds)
	{
		UE_LOG(LogLooter, Warning, TEXT("Looter.CastShots: no player after %.0f s (start a level, not the menu); stopped."), PlayerWaitSeconds);
		return Finish(false);
	}
	return true;
}

void FCastShotRun::Prepare(UWorld& World, APlayerController& Controller)
{
	WorldPtr = &World;
	ControllerPtr = &Controller;
	PawnPtr = Controller.GetPawn();
	APawn& Pawn = *Controller.GetPawn();
	Pawn.SetCanBeDamaged(false);
	ShowPlayer(Pawn, false);
	if (Controller.MyHUD)
	{
		Controller.MyHUD->bShowHUD = false;
	}
	// Motion blur smears the frame after the camera jumps.
	if (IConsoleVariable* Blur = IConsoleManager::Get().FindConsoleVariable(TEXT("r.MotionBlurQuality")))
	{
		OldMotionBlur = Blur->GetInt();
		Blur->Set(0, ECVF_SetByConsole);
	}
	FActorSpawnParameters Params;
	Params.ObjectFlags |= RF_Transient;
	CameraPtr = World.SpawnActor<ACameraActor>(Params);
	Controller.SetViewTarget(CameraPtr.Get());

	Spots = Scene::FindSpots(World, Pawn.GetActorLocation(), GivenFlat.GetPtrOrNull(), GivenSlope.GetPtrOrNull());
	Subjects = Scene::Subjects(World, Only);
	for (int32 Index = 0; Index < Subjects.Num(); ++Index)
	{
		for (const Scene::EPlace Place : Subjects[Index].Places)
		{
			if ((Place == Scene::EPlace::Flat && !Spots.bFlat) || (Place == Scene::EPlace::Slope && !Spots.bSlope))
			{
				continue;
			}
			for (const Scene::EState State : Subjects[Index].States)
			{
				Steps.Add({ Index, Place, State });
			}
		}
	}
	Rows.Add(TEXT("picture,who,place,state,metric,value,detail,past_mark"));
	UE_LOG(LogLooter, Display, TEXT("Looter.CastShots: %s, %d subjects, %d picture sets; flat ground %s, slope %s (%.0f degrees); pictures in %s."),
		*World.GetMapName(), Subjects.Num(), Steps.Num(), Spots.bFlat ? *Spots.Flat.ToCompactString() : TEXT("none found"),
		Spots.bSlope ? *Spots.Slope.ToCompactString() : TEXT("none found"), Spots.SlopeDegrees, *Directory(Tag));
	Phase = EPhase::Warmup;
	Clock = 0.f;
}

void FCastShotRun::ShowPlayer(APawn& Pawn, bool bShow)
{
	TArray<AActor*> Carried;
	Pawn.GetAttachedActors(Carried, /*bResetArray*/ true, /*bRecursivelyIncludeAttachedActors*/ true);
	Carried.Add(&Pawn);
	for (AActor* Each : Carried)
	{
		Each->SetActorHiddenInGame(!bShow);
	}
}

void FCastShotRun::EndStep()
{
	for (const TWeakObjectPtr<AActor>& Each : Spawned)
	{
		if (AActor* Actor = Each.Get())
		{
			Actor->Destroy();
		}
	}
	Spawned.Reset();
	if (AActor* Character = Restore.Get())
	{
		Character->SetActorHiddenInGame(bRestoreHidden);
		Character->SetActorTickEnabled(bRestoreTicking);
	}
	Restore.Reset();
	if (APawn* Pawn = PawnPtr.Get())
	{
		if (UPlayerViewComponent* View = Pawn->FindComponentByClass<UPlayerViewComponent>())
		{
			View->SetViewMode(EPlayerViewMode::FirstPerson);
		}
		ShowPlayer(*Pawn, false);
	}
	Actors.Reset();
}

void FCastShotRun::ClearAround(UWorld& World, const FVector& Spot)
{
	// A creature photographed where it stands (Abel) stays.
	for (TActorIterator<ACreatureBase> It(&World); It; ++It)
	{
		ACreatureBase* Creature = *It;
		const bool bKept = Spawned.ContainsByPredicate([Creature](const TWeakObjectPtr<AActor>& Each) { return Each.Get() == Creature; })
			|| Subjects.ContainsByPredicate([Creature](const Scene::FSubject& Each) { return Each.Found.Get() == Creature; });
		if (!bKept && FVector::Dist2D(Creature->GetActorLocation(), Spot) < ClearRadius)
		{
			Creature->Destroy();
		}
	}
}

bool FCastShotRun::BeginStep(UWorld& World, int32 Index)
{
	EndStep();
	if (!Steps.IsValidIndex(Index))
	{
		return Finish(true);
	}
	StepIndex = Index;
	const FStep& Step = Steps[Index];
	const Scene::FSubject& Subject = Subjects[Step.Subject];
	APawn& Pawn = *PawnPtr.Get();
	Clock = 0.f;
	Slide = CastShotProbe::FFootSlide();
	Pop = CastShotProbe::FPopWatch();
	WorstUnder = WorstPack = PackClock = 0.f;
	WorstUnderPart.Reset();
	WorstPackDetail.Reset();

	const bool bSlope = Step.Place == Scene::EPlace::Slope;
	const FVector Spot = bSlope ? Spots.Slope : Spots.Flat;
	// On the slope it stands across it and heads up it on a diagonal; on the flat, the same way every time.
	Ahead = bSlope ? Spots.Uphill.RotateAngleAxis(45.f, FVector::UpVector) : FVector(1.0, 0.0, 0.0).RotateAngleAxis(30.f, FVector::UpVector);
	const bool bMoves = Step.State == Scene::EState::Walk || Step.State == Scene::EState::Chase || Step.State == Scene::EState::Pack;
	const FVector Facing = bMoves || !bSlope ? Ahead : Spots.Uphill.RotateAngleAxis(90.f, FVector::UpVector);
	const float Yaw = static_cast<float>(Facing.Rotation().Yaw);
	if (Step.Place != Scene::EPlace::InPlace)
	{
		ClearAround(World, Spot);
		FVector Away;
		if (Scene::GroundAt(World, Spot - Ahead * PlayerAway, Away))
		{
			Pawn.SetActorLocation(Away + FVector(0.0, 0.0, 120.0), false, nullptr, ETeleportType::TeleportPhysics);
			Scene::StopMoving(Pawn);
		}
	}

	if (Subject.bPlayer)
	{
		// The player's own body, from outside, in the third-person view's body animation; walking, along a clear way.
		ShowPlayer(Pawn, true);
		if (UPlayerViewComponent* View = Pawn.FindComponentByClass<UPlayerViewComponent>())
		{
			View->SetViewMode(EPlayerViewMode::ThirdPerson);
		}
		const float WalkDistance = Step.State == Scene::EState::Walk ? PlayerWalkSeconds * PlayerTopSpeed + 150.f : 0.f;
		Scene::StandPlayer(World, Pawn, ControllerPtr.Get(), Spot, Yaw, WalkDistance, Ahead);
		Actors.Add(&Pawn);
		return StartRun(World, Step.State == Scene::EState::Walk ? PlayerWalkSeconds : 1.f);
	}
	if (AActor* Character = Subject.Found.Get())
	{
		// Shown and posing for its pictures whatever the story says, and put back after.
		Restore = Character;
		bRestoreHidden = Character->IsHidden();
		bRestoreTicking = Character->IsActorTickEnabled();
		Character->SetActorHiddenInGame(false);
		Character->SetActorTickEnabled(true);
		if (AAmosWhitlock* Amos = Cast<AAmosWhitlock>(Character))
		{
			if (Step.State == Scene::EState::Sit)
			{
				Amos->SitNow(/*bAtOnce*/ true);
			}
			else
			{
				Amos->LeanNow();
			}
		}
		Actors.Add(Character);
		return StartRun(World, CharacterSeconds);
	}
	if (Subject.PackCount > 0)
	{
		// A loose crowd across the way, closing together on the player 14 m ahead.
		const FVector Across = FVector::CrossProduct(FVector::UpVector, Ahead);
		for (int32 Member = 0; Member < Subject.PackCount; ++Member)
		{
			FVector Feet;
			const FVector Place = Spot + Across * ((Member % 3 - 1) * 260.0) - Ahead * ((Member / 3) * 300.0);
			if (ACreatureBase* Creature = Scene::GroundAt(World, Place, Feet) ? Scene::Spawn(World, Subject, Feet, Yaw) : nullptr)
			{
				Spawned.Add(Creature);
				Actors.Add(Creature);
			}
		}
		FVector Prey;
		if (Scene::GroundAt(World, Spot + Ahead * 1400.0, Prey))
		{
			Pawn.SetActorLocation(Prey + FVector(0.0, 0.0, 120.0), false, nullptr, ETeleportType::TeleportPhysics);
		}
		for (const TWeakObjectPtr<AActor>& Each : Actors)
		{
			if (ACreatureBase* Creature = Cast<ACreatureBase>(Each.Get()))
			{
				Creature->SetPassive(false);
				Creature->DevPutInState(ECreatureState::Chase, &Pawn);
			}
		}
		return StartRun(World, PackSeconds);
	}
	if (ACreatureBase* Creature = Scene::Spawn(World, Subject, Spot, Yaw))
	{
		Spawned.Add(Creature);
		Actors.Add(Creature);
		Phase = EPhase::Settle;
		// The camera on it already, so the exposure settles while it stands.
		FrameNow(World, 0);
		return true;
	}
	UE_LOG(LogLooter, Warning, TEXT("Looter.CastShots: %s couldn't be set up on the %s ground; skipped."), *Subject.Name, Scene::PlaceName(Step.Place));
	return BeginStep(World, Index + 1);
}

bool FCastShotRun::StartRun(UWorld& World, float Seconds)
{
	RunSeconds = Seconds;
	Phase = EPhase::Run;
	Clock = 0.f;
	FrameNow(World, 0);
	return true;
}

bool FCastShotRun::TickSettle(UWorld& World, APawn& Pawn)
{
	ACreatureBase* Creature = Actors.IsEmpty() ? nullptr : Cast<ACreatureBase>(Actors[0].Get());
	if (!Creature)
	{
		return BeginStep(World, StepIndex + 1);
	}
	if (Clock < SettleSeconds)
	{
		return true;
	}
	const FStep& Step = Steps[StepIndex];
	const float Seconds = Scene::Start(World, *Creature, Step.State, Pawn, Ahead);
	if (Seconds < 0.f)
	{
		UE_LOG(LogLooter, Warning, TEXT("Looter.CastShots: %s %s couldn't be made here (no ground for the player); skipped."),
			*Subjects[Step.Subject].Name, Scene::StateName(Step.State));
		return BeginStep(World, StepIndex + 1);
	}
	return StartRun(World, Seconds);
}

bool FCastShotRun::TickRun(UWorld& World, APawn& Pawn)
{
	const float Delta = World.GetDeltaSeconds();
	const FStep& Step = Steps[StepIndex];
	if (Subjects[Step.Subject].bPlayer && Step.State == Scene::EState::Walk)
	{
		Pawn.AddMovementInput(Ahead, 1.f);
	}
	SampleBody(World, Delta);
	PackClock += Delta;
	if (Actors.Num() > 1 && PackClock >= 0.25f)
	{
		PackClock = 0.f;
		SamplePack();
	}
	if (Clock < RunSeconds)
	{
		return true;
	}
	// Time stops: the numbers at this moment, then the pictures.
	UGameplayStatics::SetGlobalTimeDilation(&World, Stopped);
	MeasureNow(World);
	Phase = EPhase::Shoot;
	Angle = 0;
	Clock = 0.f;
	bShotRequested = false;
	FrameNow(World, Angle);
	return true;
}

void FCastShotRun::FrameNow(UWorld& World, int32 Which)
{
	TArray<const AActor*> Bodies;
	for (const TWeakObjectPtr<AActor>& Each : Actors)
	{
		if (Each.IsValid())
		{
			Bodies.Add(Each.Get());
		}
	}
	ACameraActor* Camera = CameraPtr.Get();
	if (Bodies.IsEmpty() || !Camera)
	{
		return;
	}
	TArray<const AActor*> Ignored = Bodies;
	Ignored.Add(PawnPtr.Get());
	Scene::Frame(World, *Camera, Scene::BoundsOf(Bodies), static_cast<float>(Bodies[0]->GetActorRotation().Yaw), Which, Ignored);
	if (APlayerController* Controller = ControllerPtr.Get())
	{
		Controller->SetViewTarget(Camera);
		if (Controller->PlayerCameraManager)
		{
			Controller->PlayerCameraManager->SetGameCameraCutThisFrame();
		}
	}
}

bool FCastShotRun::TickShoot(UWorld& World)
{
	// Kept stopped: a hit-stop or a scene may have set time going again.
	UGameplayStatics::SetGlobalTimeDilation(&World, Stopped);
	if (!bShotRequested)
	{
		if (Clock < CameraSettleSeconds)
		{
			return true;
		}
		const FStep& Step = Steps[StepIndex];
		const FString File = Directory(Tag) / FString::Printf(TEXT("%03d_%s-%s_%s_%s.png"), ++Pictures, *Subjects[Step.Subject].Name,
			Scene::PlaceName(Step.Place), Scene::StateName(Step.State), Scene::AngleName(Angle));
		// Without the UI: the bodies, not the tags over them.
		FScreenshotRequest::RequestScreenshot(File, /*bInShowUI*/ false, /*bAddFilenameSuffix*/ false);
		bShotRequested = true;
		ShotAt = Clock;
		return true;
	}
	if (Clock < ShotAt + ShotGapSeconds)
	{
		return true;
	}
	bShotRequested = false;
	Clock = 0.f;
	if (++Angle < Scene::AngleCount)
	{
		FrameNow(World, Angle);
		return true;
	}
	UGameplayStatics::SetGlobalTimeDilation(&World, 1.f);
	return BeginStep(World, StepIndex + 1);
}

bool FCastShotRun::Finish(bool bCompleted)
{
	if (UWorld* World = WorldPtr.Get())
	{
		UGameplayStatics::SetGlobalTimeDilation(World, 1.f);
	}
	EndStep();
	if (APawn* Pawn = PawnPtr.Get())
	{
		ShowPlayer(*Pawn, true);
	}
	if (APlayerController* Controller = ControllerPtr.Get())
	{
		Controller->SetViewTarget(Controller->GetPawn());
		if (Controller->MyHUD)
		{
			Controller->MyHUD->bShowHUD = true;
		}
	}
	if (AActor* Camera = CameraPtr.Get())
	{
		Camera->Destroy();
	}
	IConsoleVariable* Blur = IConsoleManager::Get().FindConsoleVariable(TEXT("r.MotionBlurQuality"));
	if (Blur && OldMotionBlur >= 0)
	{
		Blur->Set(OldMotionBlur, ECVF_SetByConsole);
	}
	const FString Csv = Directory(Tag) / TEXT("numbers.csv");
	if (Rows.Num() > 1)
	{
		FFileHelper::SaveStringArrayToFile(Rows, *Csv);
	}
	UE_LOG(LogLooter, Display, TEXT("Looter.CastShots: %s, %d pictures, %d numbers past their marks; numbers in %s."),
		bCompleted ? TEXT("done") : TEXT("stopped"), Pictures, Marked, Rows.Num() > 1 ? *Csv : TEXT("(none taken)"));
	if (bQuit)
	{
		FPlatformMisc::RequestExit(false, TEXT("Looter.CastShots"));
	}
	return false;
}

#endif

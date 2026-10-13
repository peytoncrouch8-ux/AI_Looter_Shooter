// AOreLift: when it runs and why not, the call, the ride and the rider (laid on the far landing for the ride, let go
// there), the bell pulls, the autosave held. The run under way (the cage hung where the motion has it, its sounds) is
// OreLiftUnderWay.cpp's.

#include "World/OreLift.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Combat/HealthComponent.h"
#include "Missions/MissionObjective.h"
#include "Missions/MissionRunner.h"
#include "Scenes/OreLiftRide.h"
#include "Scenes/SceneSubsystem.h"
#include "Session/CampaignRecord.h"
#include "Session/SessionSubsystem.h"
#include "Story/CaptionSubsystem.h"
#include "Story/StoryCondition.h"
#include "World/OreLiftBellPull.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"

namespace
{
	/** What the autosave waits for while someone rides (USessionSubsystem::HoldSaves). */
	const FName OreLiftSaveHold(TEXT("OreLift"));

	EOreLiftEnd OtherEnd(EOreLiftEnd End)
	{
		return End == EOreLiftEnd::Bottom ? EOreLiftEnd::Top : EOreLiftEnd::Bottom;
	}

	const TCHAR* EndName(EOreLiftEnd End)
	{
		return End == EOreLiftEnd::Bottom ? TEXT("Bottom") : TEXT("Top");
	}

	bool IsDead(const AActor* Who)
	{
		const UHealthComponent* Health = Who ? Who->FindComponentByClass<UHealthComponent>() : nullptr;
		return Health && Health->IsDead();
	}
}

// ---------------------------------------------------------------------------
// When it runs
// ---------------------------------------------------------------------------

bool AOreLift::IsOpenFor(const FCampaignRecord& Campaign, const UMissionRunner* Runner, FName Mission, int32 AtStep, FName Flag)
{
	if (Mission.IsNone() && Flag.IsNone())
	{
		return true;
	}
	if (Campaign.HasFlag(Flag))
	{
		return true;
	}
	if (Mission.IsNone())
	{
		return false;
	}
	// Ridden, its mission waits for its turn-in at the lift head or is done: it runs both ways from then on.
	if (Campaign.HasCompleted(Mission) || Campaign.IsReadyToTurnIn(Mission))
	{
		return true;
	}
	// Under way at the step that rides it (the first ride) or later.
	FStoryCondition Riding;
	Riding.DuringMission = Mission;
	Riding.FromStep = FMath::Max(AtStep, 0);
	return Riding.IsMet(Campaign, Runner);
}

bool AOreLift::IsOpen() const
{
	if (bForcedOpen)
	{
		return true;
	}
	const UMissionRunner* Runner = UMissionRunner::Get(this);
	if (!Runner || !Runner->IsActive())
	{
		// No story here (the main menu's backdrop, a test level): only a lift with no gate runs.
		return OpensWithMission.IsNone() && OpenFlag.IsNone();
	}
	return IsOpenFor(Runner->GetCampaign(), Runner, OpensWithMission, OpensAtStep, OpenFlag);
}

FText AOreLift::GetClosedReason() const
{
	if (IsOpen())
	{
		return FText::GetEmpty();
	}
	// Its mission under way but not at the ride yet: no steam. Before it, a dead lift.
	const UMissionRunner* Runner = UMissionRunner::Get(this);
	const bool bUnderWay = Runner && Runner->IsActive() && !OpensWithMission.IsNone()
		&& (Runner->IsRunning(OpensWithMission) || Runner->GetCampaign().ActiveMission == OpensWithMission);
	return bUnderWay ? ClosedSteamLine : ClosedColdLine;
}

EOreLiftBell AOreLift::GetBellState(EOreLiftEnd End) const
{
	if (!IsOpen())
	{
		return EOreLiftBell::Closed;
	}
	if (IsMoving())
	{
		return EOreLiftBell::Busy;
	}
	return CageEnd == End ? EOreLiftBell::Ride : EOreLiftBell::Call;
}

bool AOreLift::IsFirstRide() const
{
	const UMissionRunner* Runner = UMissionRunner::Get(this);
	return Runner && Runner->IsActive() && !RiddenFlag.IsNone() && !Runner->GetCampaign().HasFlag(RiddenFlag);
}

void AOreLift::RegisterBellPull(AOreLiftBellPull& Pull, EOreLiftEnd End)
{
	(End == EOreLiftEnd::Bottom ? BottomPull : TopPull) = &Pull;
}

AOreLiftBellPull* AOreLift::FindBellPull(EOreLiftEnd End) const
{
	return (End == EOreLiftEnd::Bottom ? BottomPull : TopPull).Get();
}

void AOreLift::RingClosed(EOreLiftEnd End)
{
	const AOreLiftBellPull* Pull = FindBellPull(End);
	LooterSound::PlayAt(this, LooterSoundCue::OreLift::BellDead, Pull ? Pull->GetActorLocation() : GetRiderSpot(End));
	const FText Reason = GetClosedReason();
	UCaptionSubsystem* Captions = UCaptionSubsystem::Get(this);
	if (Captions && !Reason.IsEmpty())
	{
		Captions->Play({ FStoryLine::Make(FText::GetEmpty(), Reason) }, ECaptionPlay::Interrupt);
	}
}

// ---------------------------------------------------------------------------
// Calling and riding
// ---------------------------------------------------------------------------

bool AOreLift::CallTo(EOreLiftEnd End)
{
	if (!IsOpen() || IsMoving() || CageEnd == End)
	{
		return false;
	}
	BeginRun(CageEnd, /*bRide*/ false);
	// The far landing's bell answers the pull: the cage is coming.
	if (AOreLiftBellPull* Far = FindBellPull(RunFrom))
	{
		Far->Ring(LooterSoundCue::OreLift::BellAnswer);
	}
	UE_LOG(LogLooter, Log, TEXT("%s: the cage is called to the %s landing (%.1f s)."), *GetActorNameOrLabel(), EndName(End), Run.GetDuration());
	RefreshTick();
	return true;
}

bool AOreLift::Ride(EOreLiftEnd From, AActor* InRider)
{
	const TCHAR* Refused = !IsValid(InRider) ? TEXT("no rider")
		: IsDead(InRider) ? TEXT("the rider is dead")
		: !IsOpen() ? TEXT("it doesn't run yet")
		: IsMoving() ? TEXT("the cage is on the move")
		: CageEnd != From ? TEXT("the cage isn't at that landing")
		: nullptr;
	USceneSubsystem* Scenes = USceneSubsystem::Get(this);
	const bool bScenes = Scenes && USceneSubsystem::AreScenesOn(GetWorld());
	if (!Refused && bScenes && Scenes->IsPlaying())
	{
		Refused = TEXT("a scene is playing");
	}
	if (Refused)
	{
		UE_LOG(LogLooter, Log, TEXT("%s: no ride from the %s landing: %s."), *GetActorNameOrLabel(), EndName(From), Refused);
		return false;
	}

	const bool bFirst = IsFirstRide();
	Rider = InRider;
	bRiderLost = false;
	BeginRun(From, /*bRide*/ true);
	// The session never saves the player halfway up: it waits for the far landing.
	HoldSaves();
	UE_LOG(LogLooter, Log, TEXT("%s: %s rides from the %s landing%s."), *GetActorNameOrLabel(), *InRider->GetActorNameOrLabel(), EndName(From),
		bFirst ? TEXT(" (the first ride)") : TEXT(""));

	if (bScenes && Scenes->Play(OreLiftRide::Make(*Scenes, *this, *InRider, bFirst)))
	{
		TellMissions(FMissionEvent::BoardEvent(Vehicle));
		RefreshTick();
		return true;
	}
	// No ride plays (scenes off in tour and perf runs): there at once.
	TellMissions(FMissionEvent::BoardEvent(Vehicle));
	ParkRider(OtherEnd(From));
	SetRunTime(Run.GetDuration(), /*bQuiet*/ true);
	FinishRide();
	return true;
}

void AOreLift::ParkRider(EOreLiftEnd End)
{
	AActor* Who = Rider.Get();
	if (!Who)
	{
		return;
	}
	// Standing on the cage's floor where it will rest (a pawn's location is its middle), facing the way they look.
	const APawn* Pawn = Cast<APawn>(Who);
	const AController* Controller = Pawn ? Pawn->GetController() : nullptr;
	const double Yaw = Controller ? Controller->GetControlRotation().Yaw : Who->GetActorRotation().Yaw;
	const FVector Middle = GetRiderSpot(End) + FVector(0.0, 0.0, Who->GetSimpleCollisionHalfHeight() + 2.0);
	Who->TeleportTo(Middle, FRotator(0.0, Yaw, 0.0), /*bIsATest*/ false, /*bNoCheck*/ true);
	// Held still there until the cage comes (a scene has them unseen too): no walking and no falling. After the teleport,
	// which would have them falling, as it finds no floor under the empty landing's cage spot yet. They stay solid: should
	// anything set them moving, they land on the landing rather than drop through the world.
	if (ACharacter* Character = Cast<ACharacter>(Who))
	{
		if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
		{
			Movement->StopMovementImmediately();
			Movement->DisableMovement();
		}
	}
}

void AOreLift::FinishRide()
{
	if (!bRiding)
	{
		return;
	}
	const EOreLiftEnd To = OtherEnd(RunFrom);
	bRiding = false;
	SetResting(To);

	// Let go on the far landing: their own feet again, on the cage's floor.
	AActor* Who = Rider.Get();
	const bool bArrived = Who && !bRiderLost && !IsDead(Who);
	if (Who)
	{
		if (ACharacter* Character = Cast<ACharacter>(Who))
		{
			if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
			{
				Movement->SetMovementMode(Movement->DefaultLandMovementMode);
			}
		}
	}
	Rider.Reset();
	bRiderLost = false;
	ReleaseSaves();

	if (!bArrived)
	{
		// A death on the way (a scene holds the player unhurt, so only the console's): they lie on the far landing, and the
		// death wakes them at a grave as any death does. Nobody arrived, so the missions hear nothing.
		UE_LOG(LogLooter, Warning, TEXT("%s: the ride ended without its rider; the cage rests at the %s landing."), *GetActorNameOrLabel(), EndName(To));
		return;
	}
	UMissionRunner* Runner = UMissionRunner::Get(this);
	if (Runner && Runner->IsActive() && !RiddenFlag.IsNone() && Runner->GetCampaign().SetFlag(RiddenFlag))
	{
		if (USessionSubsystem* Sessions = USessionSubsystem::Get(this))
		{
			Sessions->SaveSoon();
		}
	}
	if (AOreLiftBellPull* Landing = FindBellPull(To))
	{
		Landing->Ring(LooterSoundCue::OreLift::BellArrive);
	}
	TellMissions(FName(*FString::Printf(TEXT("%s.%s"), *ArrivedEvent, EndName(To))));
	UE_LOG(LogLooter, Log, TEXT("%s: arrived at the %s landing."), *GetActorNameOrLabel(), EndName(To));
}

void AOreLift::SetResting(EOreLiftEnd End)
{
	StopLoops();
	CageEnd = End;
	RunFrom = End;
	bCalled = false;
	RunTime = -1.f;
	Sway = FOreLiftSway();
	SwingState = FOreLiftSwingState();
	SwayTurn = FQuat::Identity;
	Cage->SetRelativeTransform(End == EOreLiftEnd::Bottom ? BottomRest : TopRest);
	LastShiver = 0.f;
	LayCable(0.f);
	RefreshTick();
}

void AOreLift::Advance(float DeltaSeconds)
{
	if (bCalled)
	{
		SetRunTime(RunTime + DeltaSeconds, /*bQuiet*/ false);
		if (RunTime >= Run.GetDuration())
		{
			const EOreLiftEnd To = OtherEnd(RunFrom);
			SetResting(To);
			if (AOreLiftBellPull* Landing = FindBellPull(To))
			{
				Landing->Ring(LooterSoundCue::OreLift::BellArrive);
			}
			UE_LOG(LogLooter, Log, TEXT("%s: the cage waits at the %s landing."), *GetActorNameOrLabel(), EndName(To));
		}
	}
	if (bRiding && !bRiderLost)
	{
		const AActor* Who = Rider.Get();
		if (!Who || IsDead(Who))
		{
			// Never left hanging: the ride is over at once, the cage at the far landing with the body.
			bRiderLost = true;
			UE_LOG(LogLooter, Warning, TEXT("%s: the rider %s mid-ride; the ride ends at once."), *GetActorNameOrLabel(), Who ? TEXT("died") : TEXT("is gone"));
			USceneSubsystem* Scenes = USceneSubsystem::Get(this);
			if (Scenes && Scenes->GetPlayingName() == OreLiftRide::SceneName())
			{
				Scenes->SkipScene();
			}
			else
			{
				ParkRider(OtherEnd(RunFrom));
				SetRunTime(Run.GetDuration(), /*bQuiet*/ true);
				FinishRide();
			}
		}
	}
	RefreshTick();
}

// ---------------------------------------------------------------------------
// The rest
// ---------------------------------------------------------------------------

void AOreLift::HoldSaves()
{
	if (bHoldingSaves)
	{
		return;
	}
	if (USessionSubsystem* Sessions = USessionSubsystem::Get(this))
	{
		Sessions->HoldSaves(OreLiftSaveHold);
	}
	bHoldingSaves = true;
}

void AOreLift::ReleaseSaves()
{
	if (!bHoldingSaves)
	{
		return;
	}
	// What waited saves now, a few seconds on, with the player on the far landing.
	if (USessionSubsystem* Sessions = USessionSubsystem::Get(this))
	{
		Sessions->ReleaseSaves(OreLiftSaveHold);
	}
	bHoldingSaves = false;
}

void AOreLift::RefreshTick()
{
	SetActorTickEnabled(IsMoving());
}

void AOreLift::TellMissions(FName Name)
{
	if (UMissionRunner* Runner = UMissionRunner::Get(this))
	{
		Runner->NotifyEvent(FMissionEvent::Named(Name, this));
	}
}

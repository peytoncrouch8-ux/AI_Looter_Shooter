// ADoorHandoff: a named gun handed out through a door opened a crack, once, as the step that asks for the talk is done.

#include "Story/DoorHandoff.h"
#include "AI_Looter_Shooter.h"
#include "Affixes/WeaponRollLibrary.h"
#include "Loot/LootFanfareSubsystem.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionObjective.h"
#include "Missions/MissionRewards.h"
#include "Missions/MissionRunner.h"
#include "Progression/PlayerProgressionSubsystem.h"
#include "Weapons/NamedWeaponDefinition.h"
#include "Weapons/WeaponBase.h"
#include "Components/SceneComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

namespace
{
	/** The level of the player the gun is handed to: the local player's, 1 without one (a test level). */
	int32 PlayerLevel(const UWorld* World)
	{
		const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
		const ULocalPlayer* Player = GameInstance ? GameInstance->GetFirstGamePlayer() : nullptr;
		const UPlayerProgressionSubsystem* Progression = Player ? Player->GetSubsystem<UPlayerProgressionSubsystem>() : nullptr;
		return Progression ? FMath::Max(Progression->GetLevel(), 1) : 1;
	}

	/** The player's body, whose backpack may hold the gun already; null without one (a test level). */
	const AActor* PlayerBody(const UWorld* World)
	{
		const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
		return Controller ? Controller->GetPawn() : nullptr;
	}
}

ADoorHandoff::ADoorHandoff()
{
	// Only while the door moves.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	USceneComponent* Spot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Spot->SetMobility(EComponentMobility::Static);
	RootComponent = Spot;
}

void ADoorHandoff::BeginPlay()
{
	Super::BeginPlay();
	if (UMissionRunner* Runner = UMissionRunner::Get(this))
	{
		BoundRunner = Runner;
		ChangedHandle = Runner->OnChanged.AddUObject(this, &ADoorHandoff::HandleMissionsChanged);
		EventHandle = Runner->OnEvent.AddUObject(this, &ADoorHandoff::HandleMissionEvent);
	}
	// Where the story stands as the level begins: only a step ending from here on hands anything out. A session that's past
	// it has had its beat: the gun is in hand, or on the porch where it was saved, or (saved before it came out) the turn-in
	// gives it.
	LastStep = ReadStep(bLastFinished);
	if (bLastFinished || LastStep > Step)
	{
		State = EDoorHandoffState::Done;
	}
}

void ADoorHandoff::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UMissionRunner* Runner = BoundRunner.Get())
	{
		Runner->OnChanged.Remove(ChangedHandle);
		Runner->OnEvent.Remove(EventHandle);
	}
	BoundRunner.Reset();
	ChangedHandle.Reset();
	EventHandle.Reset();
	Super::EndPlay(EndPlayReason);
}

void ADoorHandoff::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Advance(DeltaSeconds);
}

// ---------------------------------------------------------------------------
// When
// ---------------------------------------------------------------------------

int32 ADoorHandoff::ReadStep(bool& bOutFinished) const
{
	const UMissionRunner* Runner = UMissionRunner::Get(this);
	bOutFinished = Runner && Runner->IsCompleted(Mission);
	return Runner ? Runner->GetStep(Mission) : INDEX_NONE;
}

void ADoorHandoff::HandleMissionsChanged()
{
	bool bFinished = false;
	const int32 Now = ReadStep(bFinished);
	// Its step done here and now: the mission was on it, and has gone past it (waiting for its turn-in at Tilly's), or
	// finished from it (the console's way).
	const bool bWasOnStep = LastStep == Step && !bLastFinished;
	const bool bPast = bFinished || Now > Step;
	LastStep = Now;
	bLastFinished = bFinished;
	// The gun gone from the porch with no use heard (brought to the player as Main 7 was turned in): the door shuts too.
	if (State == EDoorHandoffState::Out && !IsGunOut())
	{
		StartClosing();
		return;
	}
	if (State != EDoorHandoffState::Waiting || !bPast)
	{
		return;
	}
	if (bWasOnStep && !bFinished)
	{
		HandOut();
	}
	else
	{
		// Past it without this level seeing it done (a session resumed on a later step): the gun is the session's already.
		// Or finished from it (the console): its turn-in gave the gun (MissionRewards::EnsureNamedGun).
		State = EDoorHandoffState::Done;
	}
}

void ADoorHandoff::HandleMissionEvent(const FMissionEvent& Event)
{
	// Anything used may be the gun taken (picked up, or stashed and gone): once it isn't out any more, the door shuts.
	if (State == EDoorHandoffState::Out && Event.Name == FMissionEvent::Interact && !IsGunOut())
	{
		StartClosing();
	}
}

FName ADoorHandoff::FindGunId() const
{
	if (!NamedGun.IsNone())
	{
		return NamedGun;
	}
	const UMissionRunner* Runner = UMissionRunner::Get(this);
	const UMissionDefinition* Definition = Runner ? Runner->FindDefinition(Mission) : nullptr;
	return Definition && Definition->Rewards.bNamedGunByHand ? Definition->Rewards.NamedGun : FName();
}

UNamedWeaponDefinition* ADoorHandoff::FindGun() const
{
	const FName GunId = FindGunId();
	UNamedWeaponDefinition* Named = GunId.IsNone() ? nullptr : UNamedWeaponDefinition::FindByName(GunId.ToString());
	if (!Named || !Named->Weapon)
	{
		UE_LOG(LogLooter, Warning, TEXT("%s: no named gun to hand out (%s; %s's rewards name one given by hand, made by "
			"Tools/Unreal/create_named_weapons.py)."), *GetActorNameOrLabel(), GunId.IsNone() ? TEXT("none named") : *GunId.ToString(),
			*Mission.ToString());
		return nullptr;
	}
	return Named;
}

// ---------------------------------------------------------------------------
// The beat
// ---------------------------------------------------------------------------

bool ADoorHandoff::HandOut(bool bForce)
{
	if (State != EDoorHandoffState::Waiting && !(bForce && State == EDoorHandoffState::Done))
	{
		return false;
	}
	if (!FindGun())
	{
		State = EDoorHandoffState::Done;
		return false;
	}
	// Words first: nothing is in the world until the gun comes out, so there's nothing to take unseen while she speaks.
	State = EDoorHandoffState::Handing;
	Clock = 0.f;
	Gun.Reset();
	bForced = bForce;
	if (Door && Door->GetRootComponent())
	{
		DoorShut = Door->GetActorQuat();
		bDoorShutKnown = true;
	}
	SetActorTickEnabled(true);
	UE_LOG(LogLooter, Log, TEXT("%s: %s to be handed out through the door."), *GetActorNameOrLabel(), *FindGunId().ToString());
	return true;
}

bool ADoorHandoff::BringGunOut()
{
	UNamedWeaponDefinition* Named = FindGun();
	if (!Named)
	{
		return false;
	}
	// One of a kind: the session has it already when Main 7 was turned in before her words were done (the turn-in gave it).
	if (!bForced && MissionRewards::FindNamedGun(GetWorld(), Named, PlayerBody(GetWorld())).IsAnywhere())
	{
		UE_LOG(LogLooter, Log, TEXT("%s: %s is the player's already: nothing comes out."), *GetActorNameOrLabel(), *Named->GetNamedId().ToString());
		return false;
	}
	// Out through the crack and down onto the porch, a world drop like any loot: its beam, and its rarity heard and its beam
	// flaring as it lands (an Epic announced).
	const FTransform Frame = GetActorTransform();
	const FVector Where = Frame.TransformPosition(OutOffset);
	AWeaponBase* Spawned = UWeaponRollLibrary::SpawnWeapon(this, Named->MakeInstance(PlayerLevel(GetWorld())),
		FTransform(FRotator(0.f, Frame.Rotator().Yaw, 0.f), Where));
	if (!Spawned)
	{
		return false;
	}
	Spawned->Toss(Frame.TransformVectorNoScale(OutVelocity));
	ULootFanfareSubsystem::ExpectLanding(Spawned);
	Gun = Spawned;
	UE_LOG(LogLooter, Log, TEXT("%s: %s handed out through the door."), *GetActorNameOrLabel(), *Named->GetNamedId().ToString());
	return true;
}

void ADoorHandoff::Advance(float DeltaSeconds)
{
	switch (State)
	{
	case EDoorHandoffState::Handing:
		Clock += DeltaSeconds;
		SetDoorAngle(CrackDegrees * FMath::SmoothStep(OpenAfter, OpenAfter + SwingSeconds, Clock));
		if (Clock >= GunOutAfter)
		{
			SetDoorAngle(CrackDegrees);
			if (BringGunOut())
			{
				State = EDoorHandoffState::Out;
				// Still until it's taken: the missions' Interact event says when.
				SetActorTickEnabled(false);
			}
			else
			{
				// Nothing to come out (the session has it): the door shuts again.
				StartClosing();
			}
		}
		break;
	case EDoorHandoffState::Out:
		if (!IsGunOut())
		{
			StartClosing();
		}
		break;
	case EDoorHandoffState::Closing:
	{
		Clock += DeltaSeconds;
		const float Shut = FMath::SmoothStep(CloseAfterTaken, CloseAfterTaken + SwingSeconds, Clock);
		SetDoorAngle(ClosingFrom * (1.f - Shut));
		if (Clock >= CloseAfterTaken + SwingSeconds)
		{
			SetDoorAngle(0.f);
			State = EDoorHandoffState::Done;
			SetActorTickEnabled(false);
		}
		break;
	}
	default:
		SetActorTickEnabled(false);
		break;
	}
}

void ADoorHandoff::StartClosing()
{
	State = EDoorHandoffState::Closing;
	Clock = 0.f;
	ClosingFrom = DoorAngle;
	SetActorTickEnabled(true);
}

void ADoorHandoff::SetDoorAngle(float Degrees)
{
	DoorAngle = Degrees;
	USceneComponent* Hinge = Door ? Door->GetRootComponent() : nullptr;
	if (!Hinge || !bDoorShutKnown)
	{
		return;
	}
	// Placed fixed with the house: it moves only now.
	if (Hinge->Mobility != EComponentMobility::Movable)
	{
		Hinge->SetMobility(EComponentMobility::Movable);
	}
	// About its own up axis, the hinge line at its foot.
	Door->SetActorRotation(DoorShut * FQuat(FVector::UpVector, FMath::DegreesToRadians(Degrees)), ETeleportType::TeleportPhysics);
}

AWeaponBase* ADoorHandoff::GetGun() const
{
	return Gun.Get();
}

bool ADoorHandoff::IsGunOut() const
{
	return Gun.IsValid() && Gun->IsPickup() && !Gun->IsActorBeingDestroyed();
}

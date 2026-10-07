// ADoorHandoff: a named gun handed out through a door opened a crack, once, as the step that asks for the talk is done.

#include "Story/DoorHandoff.h"
#include "AI_Looter_Shooter.h"
#include "Affixes/WeaponRollLibrary.h"
#include "Loot/LootTossComponent.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionObjective.h"
#include "Missions/MissionRunner.h"
#include "Progression/PlayerProgressionSubsystem.h"
#include "Weapons/NamedWeaponDefinition.h"
#include "Weapons/WeaponBase.h"
#include "Components/SceneComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/RotatingMovementComponent.h"

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
	// it has the gun already (in hand, or on the porch where it was saved).
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
	// Its step done here and now: the mission was on it, and has gone past it (or finished from it, the console's way).
	const bool bWasOnStep = LastStep == Step && !bLastFinished;
	const bool bPast = bFinished || Now > Step;
	LastStep = Now;
	bLastFinished = bFinished;
	if (State != EDoorHandoffState::Waiting || !bPast)
	{
		return;
	}
	if (bWasOnStep)
	{
		HandOut();
	}
	else
	{
		// Past it without this level seeing it done (a session resumed on a later step): the gun is the session's already.
		State = EDoorHandoffState::Done;
	}
}

void ADoorHandoff::HandleMissionEvent(const FMissionEvent& Event)
{
	// Anything used may be the gun taken (picked up, or stashed and gone): once it isn't held out any more, the door shuts.
	if (State == EDoorHandoffState::Holding && Event.Name == FMissionEvent::Interact && !IsHeldOut())
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

// ---------------------------------------------------------------------------
// The beat
// ---------------------------------------------------------------------------

bool ADoorHandoff::HandOut(bool bForce)
{
	if (State != EDoorHandoffState::Waiting && !(bForce && State == EDoorHandoffState::Done))
	{
		return false;
	}
	SpawnGun();
	if (!Gun.IsValid())
	{
		State = EDoorHandoffState::Done;
		return false;
	}
	State = EDoorHandoffState::Handing;
	Clock = 0.f;
	if (Door && Door->GetRootComponent())
	{
		DoorShut = Door->GetActorQuat();
		bDoorShutKnown = true;
	}
	SetActorTickEnabled(true);
	UE_LOG(LogLooter, Log, TEXT("%s: %s handed out through the door."), *GetActorNameOrLabel(), *FindGunId().ToString());
	return true;
}

void ADoorHandoff::SpawnGun()
{
	const FName GunId = FindGunId();
	UNamedWeaponDefinition* Named = GunId.IsNone() ? nullptr : UNamedWeaponDefinition::FindByName(GunId.ToString());
	if (!Named || !Named->Weapon)
	{
		UE_LOG(LogLooter, Warning, TEXT("%s: no named gun to hand out (%s; %s's rewards name one given by hand, made by "
			"Tools/Unreal/create_named_weapons.py)."), *GetActorNameOrLabel(), GunId.IsNone() ? TEXT("none named") : *GunId.ToString(),
			*Mission.ToString());
		return;
	}
	const FTransform Held = FTransform(HeldRotation, HeldOffset) * GetActorTransform();
	AWeaponBase* Spawned = UWeaponRollLibrary::SpawnWeapon(this, Named->MakeInstance(PlayerLevel(GetWorld())), Held);
	if (!Spawned)
	{
		return;
	}
	// Loot, so the player takes it as any gun and a save keeps it; but held at the crack: not falling, not spinning.
	Spawned->Toss(FVector::ZeroVector);
	if (ULootTossComponent* Toss = Spawned->FindComponentByClass<ULootTossComponent>())
	{
		Toss->StopMovementImmediately();
		Toss->Deactivate();
	}
	if (URotatingMovementComponent* Spin = Spawned->FindComponentByClass<URotatingMovementComponent>())
	{
		Spin->Deactivate();
	}
	Spawned->SetActorTransform(Held, /*bSweep*/ false, nullptr, ETeleportType::TeleportPhysics);
	Spawned->SetActorHiddenInGame(true);
	Gun = Spawned;
	bGunShown = false;
}

void ADoorHandoff::Advance(float DeltaSeconds)
{
	switch (State)
	{
	case EDoorHandoffState::Handing:
		Clock += DeltaSeconds;
		SetDoorAngle(CrackDegrees * FMath::SmoothStep(OpenAfter, OpenAfter + SwingSeconds, Clock));
		if (!Gun.IsValid() || !Gun->IsPickup())
		{
			// Taken before it was shown (it's loot from the first): nothing to hold out.
			StartClosing();
		}
		else if (Clock >= GunOutAfter)
		{
			SetDoorAngle(CrackDegrees);
			Gun->SetActorHiddenInGame(false);
			bGunShown = true;
			State = EDoorHandoffState::Holding;
			// Still until it's taken: the missions' Interact event says when.
			SetActorTickEnabled(false);
		}
		break;
	case EDoorHandoffState::Holding:
		if (!IsHeldOut())
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

bool ADoorHandoff::IsHeldOut() const
{
	return bGunShown && Gun.IsValid() && Gun->IsPickup() && !Gun->IsActorBeingDestroyed();
}

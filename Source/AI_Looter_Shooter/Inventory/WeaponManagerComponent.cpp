#include "Inventory/WeaponManagerComponent.h"
#include "Player/PlayerViewComponent.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponDefinition.h"
#include "Affixes/WeaponRollLibrary.h"
#include "Settings/KeyBindingSubsystem.h"
#include "AI_Looter_Shooter.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedPlayerInput.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"

UWeaponManagerComponent::UWeaponManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UWeaponManagerComponent::BeginPlay()
{
	Super::BeginPlay();

	if (APawn* Pawn = Cast<APawn>(GetOwner()))
	{
		Pawn->ReceiveControllerChangedDelegate.AddDynamic(this, &UWeaponManagerComponent::HandleControllerChanged);
		SetupInput(Pawn->GetController());
	}

	for (const FStartingWeapon& Starting : StartingWeapons)
	{
		if (!Starting.Definition)
		{
			continue;
		}
		const FWeaponInstanceData Instance = Starting.bRandomRarity
			? UWeaponRollLibrary::RollWeapon(Starting.Definition, Starting.Level)
			: UWeaponRollLibrary::RollWeaponWithRarity(Starting.Definition, Starting.Rarity, Starting.Level);
		if (GiveWeapon(Instance))
		{
			// Starting weapons come with a few magazines; everything after that comes from ammo drops.
			AddAmmo(Starting.Definition->AmmoType, Instance.Stats.MagazineSize * Starting.Definition->StartingReserveMagazines);
		}
	}
}

// ---------------------------------------------------------------------------
// Ammo
// ---------------------------------------------------------------------------

int32 UWeaponManagerComponent::GetAmmo(EAmmoType Type) const
{
	return LooterAmmo::IsValid(Type) ? AmmoPool[static_cast<int32>(Type)] : 0;
}

int32 UWeaponManagerComponent::GetMaxAmmo(EAmmoType Type) const
{
	return LooterAmmo::IsValid(Type) ? LooterAmmo::GetInfo(Type).MaxCarried : 0;
}

int32 UWeaponManagerComponent::AddAmmo(EAmmoType Type, int32 Amount)
{
	if (!LooterAmmo::IsValid(Type) || Amount <= 0)
	{
		return 0;
	}
	int32& Carried = AmmoPool[static_cast<int32>(Type)];
	const int32 Added = FMath::Min(Amount, FMath::Max(GetMaxAmmo(Type) - Carried, 0));
	if (Added > 0)
	{
		Carried += Added;
		OnAmmoChanged.Broadcast(Type, Carried);
	}
	return Added;
}

int32 UWeaponManagerComponent::TakeAmmo(EAmmoType Type, int32 Amount)
{
	if (!LooterAmmo::IsValid(Type) || Amount <= 0)
	{
		return 0;
	}
	int32& Carried = AmmoPool[static_cast<int32>(Type)];
	const int32 Taken = FMath::Min(Amount, Carried);
	if (Taken > 0)
	{
		Carried -= Taken;
		OnAmmoChanged.Broadcast(Type, Carried);
	}
	return Taken;
}

void UWeaponManagerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	TeardownInput();

	if (APawn* Pawn = Cast<APawn>(GetOwner()))
	{
		Pawn->ReceiveControllerChangedDelegate.RemoveDynamic(this, &UWeaponManagerComponent::HandleControllerChanged);
	}

	for (AWeaponBase* Weapon : Weapons)
	{
		if (IsValid(Weapon))
		{
			Weapon->Destroy();
		}
	}
	Weapons.Reset();

	Super::EndPlay(EndPlayReason);
}

// ---------------------------------------------------------------------------
// Firing passthrough
// ---------------------------------------------------------------------------

void UWeaponManagerComponent::StartFire()
{
	if (AWeaponBase* Weapon = GetActiveWeapon())
	{
		Weapon->StartFire();
	}
}

void UWeaponManagerComponent::StopFire()
{
	if (AWeaponBase* Weapon = GetActiveWeapon())
	{
		Weapon->StopFire();
	}
}

void UWeaponManagerComponent::Reload()
{
	if (AWeaponBase* Weapon = GetActiveWeapon())
	{
		Weapon->Reload();
	}
}

// ---------------------------------------------------------------------------
// Input
// ---------------------------------------------------------------------------

void UWeaponManagerComponent::HandleControllerChanged(APawn* Pawn, AController* OldController, AController* NewController)
{
	SetupInput(NewController);
}

void UWeaponManagerComponent::SetupInput(AController* Controller)
{
	TeardownInput();

	// Use the player's rebound copy of the weapon controls when available.
	UKeyBindingSubsystem* Bindings = FPawnInputBinding::GetBindings(Controller);
	UInputMappingContext* Context = Bindings ? Bindings->GetRuntimeContext(InputMappingContext) : InputMappingContext.Get();
	UEnhancedInputComponent* Input = InputBinding.Setup(GetOwner(), Controller, Context, InputMappingPriority);
	if (!Input)
	{
		return;
	}

	if (FireAction)
	{
		Input->BindAction(FireAction, ETriggerEvent::Started, this, &UWeaponManagerComponent::StartFire);
		Input->BindAction(FireAction, ETriggerEvent::Completed, this, &UWeaponManagerComponent::StopFire);
		Input->BindAction(FireAction, ETriggerEvent::Canceled, this, &UWeaponManagerComponent::StopFire);
	}
	if (ReloadAction)
	{
		Input->BindAction(ReloadAction, ETriggerEvent::Started, this, &UWeaponManagerComponent::Reload);
	}
	if (NextWeaponAction)
	{
		Input->BindAction(NextWeaponAction, ETriggerEvent::Started, this, &UWeaponManagerComponent::NextWeapon);
	}
	if (PreviousWeaponAction)
	{
		Input->BindAction(PreviousWeaponAction, ETriggerEvent::Started, this, &UWeaponManagerComponent::PreviousWeapon);
	}
	if (DropWeaponAction)
	{
		Input->BindAction(DropWeaponAction, ETriggerEvent::Started, this, &UWeaponManagerComponent::HandleDropInput);
	}
	if (InteractAction)
	{
		Input->BindAction(InteractAction, ETriggerEvent::Started, this, &UWeaponManagerComponent::HandleInteractPressed);
		Input->BindAction(InteractAction, ETriggerEvent::Completed, this, &UWeaponManagerComponent::HandleInteractReleased);
		Input->BindAction(InteractAction, ETriggerEvent::Canceled, this, &UWeaponManagerComponent::HandleInteractReleased);
	}

	// Loot labels and pickup focus only matter to the local player.
	GetWorld()->GetTimerManager().SetTimer(PickupFocusTimer, this, &UWeaponManagerComponent::UpdatePickupFocus, 0.1f, true);
}

void UWeaponManagerComponent::TeardownInput()
{
	// Losing input (death, unpossess) means the release event may never arrive.
	StopFire();
	InputBinding.Teardown();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PickupFocusTimer);
		World->GetTimerManager().ClearTimer(PickupHoldTimer);
	}
	PressedPickup.Reset();
	for (const TWeakObjectPtr<AWeaponBase>& Weapon : LabeledPickups)
	{
		if (Weapon.IsValid())
		{
			Weapon->SetLabelState(false, false);
		}
	}
	LabeledPickups.Reset();
	FocusedPickup.Reset();
}

void UWeaponManagerComponent::HandleDropInput()
{
	DropActiveWeapon();
}

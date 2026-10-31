#include "Inventory/WeaponManagerComponent.h"
#include "Combat/HealthComponent.h"
#include "Interaction/InteractionComponent.h"
#include "Player/PlayerLocomotionComponent.h"
#include "Player/PlayerViewComponent.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponDefinition.h"
#include "Affixes/WeaponRollLibrary.h"
#include "Settings/KeyBindingSubsystem.h"
#include "Shields/PlayerShieldComponent.h"
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
		// A pawn that carries guns and has health wears a ward (SHIELD, 2026-10-22): ALooterCharacter makes the component,
		// and a character class made before it gets one here, before the starting weapons below bring its starting ward.
		if (Pawn->FindComponentByClass<UHealthComponent>())
		{
			UPlayerShieldComponent::FindOrAdd(*Pawn);
		}
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
	StopWatchingSlotProgress();

	if (APawn* Pawn = Cast<APawn>(GetOwner()))
	{
		Pawn->ReceiveControllerChangedDelegate.RemoveDynamic(this, &UWeaponManagerComponent::HandleControllerChanged);
	}

	// A swap under way goes with it (its timer would wake on guns already gone).
	StopSwap();
	// The guns go with it: said aloud unless the level or play is ending (a rebuilt pawn once took them unseen).
	ReportEndPlay(EndPlayReason);
	for (AWeaponBase* Weapon : Weapons)
	{
		ReleaseGun(Weapon);
	}
	Weapons.Reset();

	Super::EndPlay(EndPlayReason);
}

// ---------------------------------------------------------------------------
// Firing passthrough
// ---------------------------------------------------------------------------

void UWeaponManagerComponent::StartFire()
{
	// Mid-swap the gun in hand is on its way out of view (locked too): a press then is for the next gun, pressed again once
	// it's up (its first shot waits for its draw).
	if (IsSwapping() || UPlayerLocomotionComponent::IsSwimmingActor(GetOwner()))
	{
		return;
	}
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
	// No reload of a gun being put away.
	if (IsSwapping() || UPlayerLocomotionComponent::IsSwimmingActor(GetOwner()))
	{
		return;
	}
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
	// Remembered past an unpossess: the pawn is destroyed after its controller lets go (ReportEndPlay).
	bHeldByPlayer = bHeldByPlayer || (Controller && Controller->IsLocalPlayerController());
	// The player's slots open with their level and the story: heard from here on, for the HUD's moment (WeaponManagerSlotGate.cpp).
	if (Controller && Controller->IsLocalPlayerController())
	{
		WatchSlotProgress(Controller);
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
	// The interact key (mapped in these controls) is the interaction component's: it finds the loot looked at and hands
	// a tap or a hold on it back here (UseOffer).

	// The number keys take a slot's weapon in hand (an empty slot, or the one in hand, does nothing; a shut one says how it
	// opens). They come from the key bindings' own context, which this adds and removes with the rest of the weapon controls.
	UEnhancedInputComponent* SlotInput = Bindings ? SlotInputBinding.Setup(GetOwner(), Controller, Bindings->GetWeaponSlotContext(), InputMappingPriority) : nullptr;
	for (int32 SlotIndex = 0; SlotInput && SlotIndex < UKeyBindingSubsystem::NumWeaponSlotKeys; ++SlotIndex)
	{
		if (const UInputAction* SlotAction = Bindings->GetWeaponSlotAction(SlotIndex))
		{
			SlotInput->BindAction(SlotAction, ETriggerEvent::Started, this, &UWeaponManagerComponent::HandleSlotKey, SlotIndex);
		}
	}

	// Loot labels only matter to the local player. The focused one's label follows the interaction component's focus.
	GetWorld()->GetTimerManager().SetTimer(LootLabelTimer, this, &UWeaponManagerComponent::UpdateLootLabels, 0.1f, true);
	if (UInteractionComponent* Interaction = GetOwner()->FindComponentByClass<UInteractionComponent>())
	{
		Interaction->OnFocusChanged.AddUniqueDynamic(this, &UWeaponManagerComponent::HandleInteractionFocusChanged);
		LabelFocus = Interaction;
	}
}

void UWeaponManagerComponent::TeardownInput()
{
	// Losing input (death, unpossess) means the release event may never arrive.
	StopFire();
	InputBinding.Teardown();
	SlotInputBinding.Teardown();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(LootLabelTimer);
	}
	if (UInteractionComponent* Interaction = LabelFocus.Get())
	{
		Interaction->OnFocusChanged.RemoveDynamic(this, &UWeaponManagerComponent::HandleInteractionFocusChanged);
	}
	LabelFocus.Reset();
	for (const TWeakObjectPtr<AWeaponBase>& Weapon : LabeledPickups)
	{
		if (Weapon.IsValid())
		{
			Weapon->SetLabelState(false, false);
		}
	}
	LabeledPickups.Reset();
}

void UWeaponManagerComponent::HandleDropInput()
{
	DropActiveWeapon();
}

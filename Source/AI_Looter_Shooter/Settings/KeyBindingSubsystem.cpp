#include "Settings/KeyBindingSubsystem.h"
#include "AI_Looter_Shooter.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	const TCHAR* KeyBindingsSaveSlot = TEXT("KeyBindings");
	constexpr int32 GlobalContextPriority = 100;

	template <typename T>
	T* LoadAsset(const TCHAR* Path)
	{
		T* Asset = LoadObject<T>(nullptr, Path);
		if (!Asset)
		{
			UE_LOG(LogLooter, Warning, TEXT("Key bindings: could not load %s"), Path);
		}
		return Asset;
	}
}

void UKeyBindingSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	SaveData = Cast<ULooterKeyBindingsSave>(UGameplayStatics::LoadGameFromSlot(KeyBindingsSaveSlot, 0));
	if (!SaveData)
	{
		SaveData = NewObject<ULooterKeyBindingsSave>(this);
	}

	BuildGlobalContext();
	BuildCharacterContext();
	WeaponSlotContext = BuildWeaponSlotContext(this, WeaponSlotActions);

	UInputMappingContext* DefaultContext = LoadAsset<UInputMappingContext>(TEXT("/Game/Input/IMC_Default.IMC_Default"));
	UInputMappingContext* WeaponContext = LoadAsset<UInputMappingContext>(TEXT("/Game/Input/IMC_Weapons.IMC_Weapons"));
	const UInputAction* Move = LoadAsset<UInputAction>(TEXT("/Game/Input/Actions/IA_Move.IA_Move"));
	const UInputAction* Jump = LoadAsset<UInputAction>(TEXT("/Game/Input/Actions/IA_Jump.IA_Jump"));
	const UInputAction* Fire = LoadAsset<UInputAction>(TEXT("/Game/Input/Actions/IA_Fire.IA_Fire"));
	const UInputAction* Reload = LoadAsset<UInputAction>(TEXT("/Game/Input/Actions/IA_Reload.IA_Reload"));
	const UInputAction* Next = LoadAsset<UInputAction>(TEXT("/Game/Input/Actions/IA_NextWeapon.IA_NextWeapon"));
	const UInputAction* Previous = LoadAsset<UInputAction>(TEXT("/Game/Input/Actions/IA_PreviousWeapon.IA_PreviousWeapon"));
	const UInputAction* Drop = LoadAsset<UInputAction>(TEXT("/Game/Input/Actions/IA_DropWeapon.IA_DropWeapon"));
	const UInputAction* Interact = LoadAsset<UInputAction>(TEXT("/Game/Input/Actions/IA_Interact.IA_Interact"));

	// Order here is the order shown in the settings menu. Only keyboard/mouse keys are listed; gamepad keeps its defaults.
	AddBinding(TEXT("MoveForward"), TEXT("Move forward"), TEXT("Movement"), DefaultContext, Move, EKeys::W);
	AddBinding(TEXT("MoveBackward"), TEXT("Move backward"), TEXT("Movement"), DefaultContext, Move, EKeys::S);
	AddBinding(TEXT("MoveLeft"), TEXT("Move left"), TEXT("Movement"), DefaultContext, Move, EKeys::A);
	AddBinding(TEXT("MoveRight"), TEXT("Move right"), TEXT("Movement"), DefaultContext, Move, EKeys::D);
	AddBinding(TEXT("Jump"), TEXT("Jump"), TEXT("Movement"), DefaultContext, Jump, EKeys::SpaceBar);
	AddBinding(TEXT("Sprint"), TEXT("Sprint"), TEXT("Movement"), CharacterContext, SprintAction, EKeys::LeftShift, true);
	AddBinding(TEXT("Crouch"), TEXT("Crouch"), TEXT("Movement"), CharacterContext, CrouchAction, EKeys::LeftControl, true);

	AddBinding(TEXT("Fire"), TEXT("Fire"), TEXT("Combat"), WeaponContext, Fire, EKeys::LeftMouseButton);
	AddBinding(TEXT("Aim"), TEXT("Aim down sights"), TEXT("Combat"), CharacterContext, AimAction, EKeys::RightMouseButton, true);
	AddBinding(TEXT("Reload"), TEXT("Reload"), TEXT("Combat"), WeaponContext, Reload, EKeys::R);
	AddBinding(TEXT("NextWeapon"), TEXT("Next weapon"), TEXT("Combat"), WeaponContext, Next, EKeys::MouseScrollUp);
	AddBinding(TEXT("PreviousWeapon"), TEXT("Previous weapon"), TEXT("Combat"), WeaponContext, Previous, EKeys::MouseScrollDown);
	for (int32 SlotIndex = 0; SlotIndex < WeaponSlotActions.Num(); ++SlotIndex)
	{
		AddBinding(WeaponSlotBindingId(SlotIndex), *FString::Printf(TEXT("Weapon slot %d"), SlotIndex + 1), TEXT("Combat"), WeaponSlotContext,
			WeaponSlotActions[SlotIndex], DefaultWeaponSlotKey(SlotIndex));
	}
	AddBinding(TEXT("DropWeapon"), TEXT("Drop weapon"), TEXT("Combat"), WeaponContext, Drop, EKeys::G);
	AddBinding(TEXT("Interact"), TEXT("Pick up / interact"), TEXT("Combat"), WeaponContext, Interact, EKeys::E);

	AddBinding(TEXT("ToggleView"), TEXT("Camera view (1st / 3rd person)"), TEXT("Camera"), CharacterContext, ToggleViewAction, EKeys::F5);

	AddBinding(TEXT("Inventory"), TEXT("Inventory"), TEXT("Menus"), GlobalContext, InventoryAction, EKeys::Tab);
	AddBinding(TEXT("InventoryAlt"), TEXT("Inventory (alternate)"), TEXT("Menus"), GlobalContext, InventoryAction, EKeys::I);

	// Forget saved keys for controls that no longer exist, so old saves don't carry them forever.
	for (auto It = SaveData->Overrides.CreateIterator(); It; ++It)
	{
		if (!Bindings.ContainsByPredicate([&It](const FRebindableKey& B) { return B.Id == It->Key; }))
		{
			It.RemoveCurrent();
		}
	}
}

void UKeyBindingSubsystem::BuildGlobalContext()
{
	// Built in code so these always exist, no matter which level or pawn is active.
	PauseAction = NewObject<UInputAction>(this, TEXT("IA_Pause"));
	InventoryAction = NewObject<UInputAction>(this, TEXT("IA_Inventory"));

	GlobalContext = NewObject<UInputMappingContext>(this, TEXT("IMC_Global"));
	// Escape always opens the menu (not rebindable) so players can never lock themselves out.
	GlobalContext->MapKey(PauseAction, EKeys::Escape);
	GlobalContext->MapKey(PauseAction, EKeys::Gamepad_Special_Right);
	GlobalContext->MapKey(InventoryAction, EKeys::Tab);
	GlobalContext->MapKey(InventoryAction, EKeys::I);
	GlobalContext->MapKey(InventoryAction, EKeys::Gamepad_Special_Left);
}

void UKeyBindingSubsystem::BuildCharacterContext()
{
	SprintAction = NewObject<UInputAction>(this, TEXT("IA_Sprint"));
	CrouchAction = NewObject<UInputAction>(this, TEXT("IA_Crouch"));
	ToggleViewAction = NewObject<UInputAction>(this, TEXT("IA_ToggleView"));
	AimAction = NewObject<UInputAction>(this, TEXT("IA_Aim"));

	CharacterContext = NewObject<UInputMappingContext>(this, TEXT("IMC_Character"));
	CharacterContext->MapKey(SprintAction, EKeys::LeftShift);
	CharacterContext->MapKey(CrouchAction, EKeys::LeftControl);
	CharacterContext->MapKey(ToggleViewAction, EKeys::F5);
	CharacterContext->MapKey(AimAction, EKeys::RightMouseButton);
	// Gamepad keeps fixed defaults: click the left stick to sprint, B to crouch, click the right stick to change view.
	CharacterContext->MapKey(SprintAction, EKeys::Gamepad_LeftThumbstick);
	CharacterContext->MapKey(CrouchAction, EKeys::Gamepad_FaceButton_Right);
	CharacterContext->MapKey(ToggleViewAction, EKeys::Gamepad_RightThumbstick);
	// Gamepads aim with the left trigger, as shooters do.
	CharacterContext->MapKey(AimAction, EKeys::Gamepad_LeftTrigger);
}

FName UKeyBindingSubsystem::WeaponSlotBindingId(int32 SlotIndex)
{
	// Slots count from 0, people (and the keys) from 1.
	return FName(*FString::Printf(TEXT("WeaponSlot%d"), SlotIndex + 1));
}

FKey UKeyBindingSubsystem::DefaultWeaponSlotKey(int32 SlotIndex)
{
	switch (SlotIndex)
	{
	case 0: return EKeys::One;
	case 1: return EKeys::Two;
	case 2: return EKeys::Three;
	default: return EKeys::Invalid;
	}
}

UInputMappingContext* UKeyBindingSubsystem::BuildWeaponSlotContext(UObject* Outer, TArray<TObjectPtr<UInputAction>>& OutActions)
{
	// Built in code like the character actions, so the number keys work without touching the weapon controls asset.
	UInputMappingContext* Context = NewObject<UInputMappingContext>(Outer, TEXT("IMC_WeaponSlots"));
	OutActions.Reset();
	for (int32 SlotIndex = 0; SlotIndex < NumWeaponSlotKeys; ++SlotIndex)
	{
		UInputAction* Action = NewObject<UInputAction>(Outer, FName(*FString::Printf(TEXT("IA_WeaponSlot%d"), SlotIndex + 1)));
		Context->MapKey(Action, DefaultWeaponSlotKey(SlotIndex));
		OutActions.Add(Action);
	}
	return Context;
}

const UInputAction* UKeyBindingSubsystem::GetWeaponSlotAction(int32 SlotIndex) const
{
	return WeaponSlotActions.IsValidIndex(SlotIndex) ? WeaponSlotActions[SlotIndex].Get() : nullptr;
}

void UKeyBindingSubsystem::AddBinding(FName Id, const TCHAR* Name, const TCHAR* Category, UInputMappingContext* Context, const UInputAction* Action,
	const FKey& DefaultKey, bool bSupportsToggle)
{
	if (!Context || !Action)
	{
		return;
	}

	const TArray<FEnhancedActionKeyMapping>& Mappings = Context->GetMappings();
	const int32 Index = Mappings.IndexOfByPredicate([Action, &DefaultKey](const FEnhancedActionKeyMapping& Mapping)
	{
		return Mapping.Action == Action && Mapping.Key == DefaultKey;
	});
	if (Index == INDEX_NONE)
	{
		UE_LOG(LogLooter, Warning, TEXT("Key bindings: %s has no %s mapping for %s"), *Context->GetName(), *DefaultKey.ToString(), *Action->GetName());
		return;
	}

	FRebindableKey Binding;
	Binding.Id = Id;
	Binding.DisplayName = FText::FromString(Name);
	Binding.Category = FText::FromString(Category);
	Binding.DefaultKey = DefaultKey;
	Binding.SourceContext = Context;
	Binding.MappingIndex = Index;
	Binding.bSupportsToggle = bSupportsToggle;
	Bindings.Add(Binding);
}

UEnhancedInputLocalPlayerSubsystem* UKeyBindingSubsystem::GetInputSubsystem() const
{
	return ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
}

UInputMappingContext* UKeyBindingSubsystem::GetRuntimeContext(UInputMappingContext* Source)
{
	if (!Source)
	{
		return nullptr;
	}
	if (TObjectPtr<UInputMappingContext>* Existing = RuntimeContexts.Find(Source))
	{
		return *Existing;
	}

	// Duplicate so player keys never modify the shared asset (important in the editor, where PIE uses the same assets).
	UInputMappingContext* Copy = DuplicateObject<UInputMappingContext>(Source, this);
	RuntimeContexts.Add(Source, Copy);
	for (const FRebindableKey& Binding : Bindings)
	{
		if (Binding.SourceContext == Source)
		{
			ApplyBinding(Binding);
		}
	}
	return Copy;
}

void UKeyBindingSubsystem::ApplyBinding(const FRebindableKey& Binding)
{
	TObjectPtr<UInputMappingContext>* Copy = RuntimeContexts.Find(Binding.SourceContext);
	if (Copy && (*Copy)->GetMappings().IsValidIndex(Binding.MappingIndex))
	{
		(*Copy)->GetMapping(Binding.MappingIndex).Key = GetKey(Binding.Id);
	}
}

void UKeyBindingSubsystem::SyncContexts()
{
	UEnhancedInputLocalPlayerSubsystem* Input = GetInputSubsystem();
	if (!Input)
	{
		return;
	}

	// Anything that added an original asset (e.g. the template's IMC_Default) gets the player's copy instead.
	TArray<UInputMappingContext*> Sources;
	for (const FRebindableKey& Binding : Bindings)
	{
		Sources.AddUnique(Binding.SourceContext);
	}
	for (UInputMappingContext* Source : Sources)
	{
		int32 Priority = 0;
		if (Source != GlobalContext && Input->HasMappingContext(Source, Priority))
		{
			Input->RemoveMappingContext(Source);
			Input->AddMappingContext(GetRuntimeContext(Source), Priority);
		}
	}

	UInputMappingContext* Global = GetRuntimeContext(GlobalContext);
	if (Global && !Input->HasMappingContext(Global))
	{
		Input->AddMappingContext(Global, GlobalContextPriority);
	}
}

FKey UKeyBindingSubsystem::GetKey(FName Id) const
{
	if (SaveData)
	{
		if (const FKey* Override = SaveData->Overrides.Find(Id))
		{
			return *Override;
		}
	}
	const FRebindableKey* Binding = Bindings.FindByPredicate([Id](const FRebindableKey& B) { return B.Id == Id; });
	return Binding ? Binding->DefaultKey : EKeys::Invalid;
}

FText UKeyBindingSubsystem::GetDisplayName(FName Id) const
{
	const FRebindableKey* Binding = Bindings.FindByPredicate([Id](const FRebindableKey& B) { return B.Id == Id; });
	return Binding ? Binding->DisplayName : FText::FromName(Id);
}

void UKeyBindingSubsystem::SetKey(FName Id, const FKey& Key)
{
	const FRebindableKey* Binding = Bindings.FindByPredicate([Id](const FRebindableKey& B) { return B.Id == Id; });
	if (!Binding || !Key.IsValid())
	{
		return;
	}

	if (Key == Binding->DefaultKey)
	{
		SaveData->Overrides.Remove(Id);
	}
	else
	{
		SaveData->Overrides.Add(Id, Key);
	}

	ApplyBinding(*Binding);
	if (UEnhancedInputLocalPlayerSubsystem* Input = GetInputSubsystem())
	{
		Input->RequestRebuildControlMappings();
	}
	Save();
}

void UKeyBindingSubsystem::ResetKey(FName Id)
{
	const FRebindableKey* Binding = Bindings.FindByPredicate([Id](const FRebindableKey& B) { return B.Id == Id; });
	if (Binding)
	{
		SetKey(Id, Binding->DefaultKey);
	}
}

bool UKeyBindingSubsystem::IsToggleMode(FName Id) const
{
	return SaveData && SaveData->ToggleBindings.Contains(Id);
}

void UKeyBindingSubsystem::SetToggleMode(FName Id, bool bToggle)
{
	const FRebindableKey* Binding = Bindings.FindByPredicate([Id](const FRebindableKey& B) { return B.Id == Id; });
	if (!Binding || !Binding->bSupportsToggle || IsToggleMode(Id) == bToggle)
	{
		return;
	}

	if (bToggle)
	{
		SaveData->ToggleBindings.Add(Id);
	}
	else
	{
		SaveData->ToggleBindings.Remove(Id);
	}
	Save();
}

void UKeyBindingSubsystem::ResetAll()
{
	SaveData->Overrides.Reset();
	SaveData->ToggleBindings.Reset();
	for (const FRebindableKey& Binding : Bindings)
	{
		ApplyBinding(Binding);
	}
	if (UEnhancedInputLocalPlayerSubsystem* Input = GetInputSubsystem())
	{
		Input->RequestRebuildControlMappings();
	}
	Save();
}

FName UKeyBindingSubsystem::FindConflict(FName Id, const FKey& Key) const
{
	for (const FRebindableKey& Binding : Bindings)
	{
		if (Binding.Id != Id && GetKey(Binding.Id) == Key)
		{
			return Binding.Id;
		}
	}
	return NAME_None;
}

bool UKeyBindingSubsystem::IsInventoryKey(const FKey& Key) const
{
	return Key == GetKey(TEXT("Inventory")) || Key == GetKey(TEXT("InventoryAlt")) || Key == EKeys::Gamepad_Special_Left;
}

void UKeyBindingSubsystem::Save()
{
	UGameplayStatics::SaveGameToSlot(SaveData, KeyBindingsSaveSlot, 0);
}

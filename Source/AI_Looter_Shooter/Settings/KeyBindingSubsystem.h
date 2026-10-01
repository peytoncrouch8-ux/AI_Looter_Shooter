#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "KeyBindingSubsystem.generated.h"

class UEnhancedInputLocalPlayerSubsystem;
class UInputAction;
class UInputMappingContext;

/** One key the player can rebind in the settings menu. */
USTRUCT()
struct FRebindableKey
{
	GENERATED_BODY()

	FName Id;
	FText DisplayName;
	FText Category;
	FKey DefaultKey;

	/** The mapping context (asset or code-built) and the mapping inside it this row controls. */
	UPROPERTY()
	TObjectPtr<UInputMappingContext> SourceContext = nullptr;

	int32 MappingIndex = INDEX_NONE;

	/** The player can choose between holding the key and pressing it to toggle (sprint, crouch). */
	bool bSupportsToggle = false;
};

UCLASS()
class AI_LOOTER_SHOOTER_API ULooterKeyBindingsSave : public USaveGame
{
	GENERATED_BODY()

public:
	/** Binding id -> key the player chose. Missing entries use the default key. */
	UPROPERTY()
	TMap<FName, FKey> Overrides;

	/** Bindings the player switched from hold to toggle. */
	UPROPERTY()
	TSet<FName> ToggleBindings;
};

/**
 * Player key rebinding without touching input assets. Every mapping context the game uses is swapped
 * for a runtime copy with the player's keys applied; overrides are saved to the "KeyBindings" slot.
 * Also owns the always-on global actions (pause menu, inventory) and the code-built character actions
 * (sprint, crouch, toggle camera view).
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UKeyBindingSubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	const TArray<FRebindableKey>& GetBindings() const { return Bindings; }
	FKey GetKey(FName Id) const;
	void SetKey(FName Id, const FKey& Key);
	void ResetKey(FName Id);
	void ResetAll();

	/** Another binding already using this key, or NAME_None. */
	FName FindConflict(FName Id, const FKey& Key) const;
	FText GetDisplayName(FName Id) const;

	/** Hold (default) or toggle, for bindings that support both. */
	bool IsToggleMode(FName Id) const;
	void SetToggleMode(FName Id, bool bToggle);

	/** Runtime copy of a mapping context with the player's keys applied. Add this instead of the asset. */
	UInputMappingContext* GetRuntimeContext(UInputMappingContext* Source);

	/** Swaps any original contexts the input system is using for their runtime copies. Cheap; call every frame. */
	void SyncContexts();

	const UInputAction* GetPauseAction() const { return PauseAction; }
	const UInputAction* GetInventoryAction() const { return InventoryAction; }
	const UInputAction* GetSprintAction() const { return SprintAction; }
	const UInputAction* GetCrouchAction() const { return CrouchAction; }
	const UInputAction* GetToggleViewAction() const { return ToggleViewAction; }
	const UInputAction* GetAimAction() const { return AimAction; }

	/** The player's copy of the character controls (sprint, crouch, camera view). Added while the character is possessed. */
	UInputMappingContext* GetCharacterContext() { return GetRuntimeContext(CharacterContext); }

	/** Input priority the character components add GetCharacterContext() with. */
	static constexpr int32 CharacterContextPriority = 1;

	/** True if Key currently opens/closes the inventory. */
	bool IsInventoryKey(const FKey& Key) const;

private:
	void BuildGlobalContext();
	void BuildCharacterContext();
	void AddBinding(FName Id, const TCHAR* Name, const TCHAR* Category, UInputMappingContext* Context, const UInputAction* Action,
		const FKey& DefaultKey, bool bSupportsToggle = false);
	void ApplyBinding(const FRebindableKey& Binding);
	void Save();
	UEnhancedInputLocalPlayerSubsystem* GetInputSubsystem() const;

	UPROPERTY(Transient)
	TArray<FRebindableKey> Bindings;

	UPROPERTY(Transient)
	TMap<TObjectPtr<UInputMappingContext>, TObjectPtr<UInputMappingContext>> RuntimeContexts;

	UPROPERTY(Transient)
	TObjectPtr<ULooterKeyBindingsSave> SaveData;

	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> GlobalContext;

	UPROPERTY(Transient) TObjectPtr<UInputAction> PauseAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> InventoryAction;

	/** Character-only controls live in their own context so they only work while the player's character is possessed. */
	UPROPERTY(Transient) TObjectPtr<UInputMappingContext> CharacterContext;
	UPROPERTY(Transient) TObjectPtr<UInputAction> SprintAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> CrouchAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> ToggleViewAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> AimAction;
};

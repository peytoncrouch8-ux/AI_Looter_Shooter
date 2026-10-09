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
 * Also owns the always-on global actions (pause menu, inventory, map), the code-built character actions
 * (sprint, crouch, aim, melee, grenade, toggle camera view) and the number keys that take a weapon slot in hand.
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
	const UInputAction* GetMeleeAction() const { return MeleeAction; }
	const UInputAction* GetGrenadeAction() const { return GrenadeAction; }
	const UInputAction* GetMapAction() const { return MapAction; }

	// --- Map: M, and down on the D-pad on a gamepad (ALooterHUD binds it: the inventory opens on its map page) ---

	/** The map's rebindable binding: "Map". */
	static FName MapBindingId();

	/** M, as in most games with a map. */
	static FKey DefaultMapKey();

	/** Down on the D-pad: the View button, the usual one, opens the inventory, and up on the D-pad changes the camera view. */
	static FKey DefaultMapGamepadKey();

	/**
	 * Makes the map action in Outer and maps it in Context to its default keys (DefaultMapKey, DefaultMapGamepadKey).
	 * Static so tests can check the mapping the settings menu rebinds.
	 */
	static UInputAction* AddMapAction(UObject* Outer, UInputMappingContext& Context);

	/**
	 * True if Key is the map's keyboard key now (its page can close on it, as the inventory's pages close on
	 * IsInventoryKey); never the D-pad, which moves the page's selection.
	 */
	bool IsMapKey(const FKey& Key) const;

	// --- Melee: V, and a click of the right stick on a gamepad, Borderlands' way (UPlayerMeleeComponent binds it) ---

	/** The melee strike's rebindable binding: "Melee". */
	static FName MeleeBindingId();

	/** V: under the left hand's fingers beside the movement keys, and free (nothing else in the game uses it). */
	static FKey DefaultMeleeKey();

	/** The right stick's click, as in Borderlands (the camera view moved to the D-pad's up for it). */
	static FKey DefaultMeleeGamepadKey();

	/**
	 * Makes the melee action in Outer and maps it in Context to its default keys (DefaultMeleeKey, DefaultMeleeGamepadKey).
	 * Static so tests can check the mapping the settings menu rebinds.
	 */
	static UInputAction* AddMeleeAction(UObject* Outer, UInputMappingContext& Context);

	// --- Grenade: G, and the right bumper on a gamepad, Borderlands' way (UPlayerThrowComponent binds it) ---

	/** The grave-salt grenade's rebindable binding: "Grenade". */
	static FName GrenadeBindingId();

	/** G, as in Borderlands (Drop weapon, which had it, moved to DefaultDropWeaponKey). */
	static FKey DefaultGrenadeKey();

	/** The right bumper (free: the left one is kept for the ember powers to come). */
	static FKey DefaultGrenadeGamepadKey();

	/**
	 * Makes the grenade action in Outer and maps it in Context to its default keys (DefaultGrenadeKey,
	 * DefaultGrenadeGamepadKey). Static so tests can check the mapping the settings menu rebinds.
	 */
	static UInputAction* AddGrenadeAction(UObject* Outer, UInputMappingContext& Context);

	/**
	 * Drop weapon's default key: X, since G is the grenade's. The weapon controls asset still maps the action to G; the
	 * player's copy of it gets this key (the binding finds the asset's mapping by DropWeaponAssetKey).
	 */
	static FKey DefaultDropWeaponKey();
	static FKey DropWeaponAssetKey();

	/** The player's copy of the character controls (sprint, crouch, aim, melee, grenade, camera view). Added while the character is possessed. */
	UInputMappingContext* GetCharacterContext() { return GetRuntimeContext(CharacterContext); }

	/** Input priority the character components add GetCharacterContext() with. */
	static constexpr int32 CharacterContextPriority = 1;

	// --- Weapon slot keys: 1, 2 and 3 take that slot's weapon in hand (UWeaponManagerComponent binds them) ---

	/** One key per equip slot (UWeaponManagerComponent::MaxWeapons). */
	static constexpr int32 NumWeaponSlotKeys = 3;

	/** The rebindable binding of slot SlotIndex (from 0): "WeaponSlot1" for the first. */
	static FName WeaponSlotBindingId(int32 SlotIndex);

	/** The number key above the letters that matches the slot: One for the first. Invalid past NumWeaponSlotKeys. */
	static FKey DefaultWeaponSlotKey(int32 SlotIndex);

	/**
	 * Builds the weapon slot keys' mapping context in Outer: one action per slot (in OutActions, by slot), each mapped to
	 * its default key. Static so tests can check the mapping the settings menu rebinds.
	 */
	static UInputMappingContext* BuildWeaponSlotContext(UObject* Outer, TArray<TObjectPtr<UInputAction>>& OutActions);

	/** The action that takes slot SlotIndex (from 0) in hand, or null. */
	const UInputAction* GetWeaponSlotAction(int32 SlotIndex) const;

	/** The player's copy of the weapon slot keys, with their rebound keys. Added while a weapon carrier is possessed. */
	UInputMappingContext* GetWeaponSlotContext() { return GetRuntimeContext(WeaponSlotContext); }

	/** True if Key currently opens/closes the inventory. */
	bool IsInventoryKey(const FKey& Key) const;

private:
	void BuildGlobalContext();
	void BuildCharacterContext();
	/**
	 * A row of the settings menu's key list for Action's mapping in Context. The mapping is found by its key: DefaultKey,
	 * or AssetKey when the asset maps it to another key than the game's default (Drop weapon's G).
	 */
	void AddBinding(FName Id, const TCHAR* Name, const TCHAR* Category, UInputMappingContext* Context, const UInputAction* Action,
		const FKey& DefaultKey, bool bSupportsToggle = false, const FKey& AssetKey = EKeys::Invalid);
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
	UPROPERTY(Transient) TObjectPtr<UInputAction> MapAction;

	/** Character-only controls live in their own context so they only work while the player's character is possessed. */
	UPROPERTY(Transient) TObjectPtr<UInputMappingContext> CharacterContext;
	UPROPERTY(Transient) TObjectPtr<UInputAction> SprintAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> CrouchAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> ToggleViewAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> AimAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> MeleeAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> GrenadeAction;

	/** The weapon slot keys get their own context: the weapon carrier adds it, whatever the weapon controls asset holds. */
	UPROPERTY(Transient) TObjectPtr<UInputMappingContext> WeaponSlotContext;
	UPROPERTY(Transient) TArray<TObjectPtr<UInputAction>> WeaponSlotActions;
};

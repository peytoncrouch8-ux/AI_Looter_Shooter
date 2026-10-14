#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Affixes/WeaponRollLibrary.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Player/PlayerLocomotionComponent.h"
#include "Settings/KeyBindingSubsystem.h"
#include "Tests/LocomotionTestWorld.h"
#include "Weapons/WeaponDefinition.h"
#include "Weapons/WeaponTypes.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EnhancedActionKeyMapping.h"
#include "GameFramework/Character.h"
#include "InputAction.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "Subsystems/SubsystemCollection.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Script.h"
#include "UObject/StrongObjectPtr.h"

// The mouse wheel through the weapon slots (the user, 2026-10-14: "if I have weapon 2 equipped, and scroll up, it goes
// down"). The HUD's slot column has slot 1 on top, so the wheel moves along it: up to the slot above (2 -> 1), down to the
// one below (2 -> 3), round at the ends. Followed from the key to the gun in hand: the player's copy of the weapon
// controls (UKeyBindingSubsystem), the action it maps the wheel to, and what the real character's weapon manager does
// with that action. The gamepad's Y still steps forward.

namespace
{
	const FName WheelTestNextId(TEXT("NextWeapon"));
	const FName WheelTestPreviousId(TEXT("PreviousWeapon"));

	/** The action Key fires in Context, or null. */
	const UInputAction* WheelTestActionOn(const UInputMappingContext* Context, const FKey& Key)
	{
		if (!Context)
		{
			return nullptr;
		}
		for (const FEnhancedActionKeyMapping& Mapping : Context->GetMappings())
		{
			if (Mapping.Key == Key)
			{
				return Mapping.Action.Get();
			}
		}
		return nullptr;
	}

	const FRebindableKey* WheelTestRow(const UKeyBindingSubsystem& Keys, FName Id)
	{
		return Keys.GetBindings().FindByPredicate([Id](const FRebindableKey& Row) { return Row.Id == Id; });
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInputWheelCyclesTest, "Looter.Input.WheelCycles",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FInputWheelCyclesTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper TestLevel;
	UWeaponDefinition* Rifle = LoadObject<UWeaponDefinition>(nullptr, TEXT("/Game/Weapons/Data/DA_AssaultRifle.DA_AssaultRifle"));
	UInputMappingContext* WeaponControls = LoadObject<UInputMappingContext>(nullptr, TEXT("/Game/Input/IMC_Weapons.IMC_Weapons"));
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)) || !TestNotNull(TEXT("The rifle"), Rifle)
		|| !TestNotNull(TEXT("The weapon controls"), WeaponControls))
	{
		return false;
	}

	// The key bindings as a player's are made, on a local player of the test's own that never joins the game (nothing here
	// saves, and the input system is never asked for).
	ULocalPlayer* Player = GEngine ? NewObject<ULocalPlayer>(GEngine) : nullptr;
	if (!TestNotNull(TEXT("A local player to hold the key bindings"), Player))
	{
		return false;
	}
	FSubsystemCollection<ULocalPlayerSubsystem> Collection;
	const TStrongObjectPtr<UKeyBindingSubsystem> Keys(NewObject<UKeyBindingSubsystem>(Player));
	Keys->Initialize(Collection);

	// The settings menu's rows: wheel down for the next weapon, wheel up for the previous one.
	const FRebindableKey* NextRow = WheelTestRow(*Keys, WheelTestNextId);
	const FRebindableKey* PreviousRow = WheelTestRow(*Keys, WheelTestPreviousId);
	if (!TestNotNull(TEXT("A \"Next weapon\" row"), NextRow) || !TestNotNull(TEXT("A \"Previous weapon\" row"), PreviousRow))
	{
		return false;
	}
	TestTrue(TEXT("Next weapon: the wheel down by default"), NextRow->DefaultKey == EKeys::MouseScrollDown);
	TestTrue(TEXT("Previous weapon: the wheel up by default"), PreviousRow->DefaultKey == EKeys::MouseScrollUp);
	TestTrue(TEXT("...each its own key (no conflict)"), Keys->FindConflict(WheelTestNextId, NextRow->DefaultKey) == NAME_None
		&& Keys->FindConflict(WheelTestPreviousId, PreviousRow->DefaultKey) == NAME_None);

	// The real character's weapon manager and its three slots.
	FEditorScriptExecutionGuard RunActorEvents;
	UWorld* World = TestLevel.GetTestWorld();
	LocomotionTestWorld::SpawnBlock(World, FVector(0.0, 0.0, -50.0), FVector(2000.0, 2000.0, 100.0));
	const UPlayerLocomotionComponent* Locomotion = LocomotionTestWorld::SpawnPlayer(World, FVector::ZeroVector);
	UWeaponManagerComponent* Weapons = Locomotion ? LocomotionTestWorld::BodyOf(Locomotion)->FindComponentByClass<UWeaponManagerComponent>() : nullptr;
	if (!TestNotNull(TEXT("The player's weapon manager"), Weapons))
	{
		return false;
	}
	TestTrue(TEXT("Its next and previous actions are the controls' own"), Weapons->NextWeaponAction && Weapons->PreviousWeaponAction
		&& Weapons->NextWeaponAction != Weapons->PreviousWeaponAction);
	for (int32 Slot = Weapons->GetWeapons().Num(); Slot < UKeyBindingSubsystem::NumWeaponSlotKeys; ++Slot)
	{
		Weapons->GiveWeapon(UWeaponRollLibrary::RollWeaponWithRarity(Rifle, EWeaponRarity::Common, 1));
	}
	if (!TestEqual(TEXT("Three guns in the slots"), Weapons->GetWeapons().Num(), UKeyBindingSubsystem::NumWeaponSlotKeys))
	{
		return false;
	}

	// The player's copy of the controls (their keys put in): what each key fires, handed to the manager as its input does.
	if (Keys->GetKey(WheelTestNextId) != NextRow->DefaultKey || Keys->GetKey(WheelTestPreviousId) != PreviousRow->DefaultKey)
	{
		AddWarning(TEXT("This machine's saved key bindings move the weapon keys: the wheel's way through the slots wasn't followed."));
		return true;
	}
	const UInputMappingContext* PlayerControls = Keys->GetRuntimeContext(WeaponControls);
	auto Press = [&](const FKey& Key)
	{
		const UInputAction* Action = WheelTestActionOn(PlayerControls, Key);
		if (Action && Action == Weapons->NextWeaponAction.Get())
		{
			Weapons->NextWeapon();
		}
		else if (Action && Action == Weapons->PreviousWeaponAction.Get())
		{
			Weapons->PreviousWeapon();
		}
		else
		{
			AddError(FString::Printf(TEXT("%s fires neither weapon action (%s)"), *Key.ToString(), *GetNameSafe(Action)));
		}
		return Weapons->GetActiveSlot() + 1;
	};

	Weapons->EquipSlot(1);
	TestEqual(TEXT("Weapon 2 in hand, the wheel up: weapon 1, the slot above"), Press(EKeys::MouseScrollUp), 1);
	Weapons->EquipSlot(1);
	TestEqual(TEXT("Weapon 2 in hand, the wheel down: weapon 3, the slot below"), Press(EKeys::MouseScrollDown), 3);
	TestEqual(TEXT("Weapon 3, the wheel down: round to weapon 1"), Press(EKeys::MouseScrollDown), 1);
	TestEqual(TEXT("Weapon 1, the wheel up: round to weapon 3"), Press(EKeys::MouseScrollUp), 3);
	TestEqual(TEXT("...and up again: weapon 2"), Press(EKeys::MouseScrollUp), 2);

	// The gamepad's Y is the next weapon still, stepping forward through the slots.
	Weapons->EquipSlot(1);
	TestEqual(TEXT("A gamepad's Y from weapon 2: weapon 3"), Press(EKeys::Gamepad_FaceButton_Top), 3);
	return true;
}

#endif

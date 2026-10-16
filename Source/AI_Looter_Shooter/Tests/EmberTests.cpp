#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Combat/HealthComponent.h"
#include "Embers/EmberComponent.h"
#include "Embers/EmberDefinition.h"
#include "Embers/EmberTypes.h"
#include "Embers/HeroDefinition.h"
#include "Session/SessionSave.h"
#include "Settings/KeyBindingSubsystem.h"
#include "Tests/EmberProbePower.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/DamageType.h"
#include "InputAction.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Package.h"
#include "UObject/Script.h"
#include "UObject/UnrealType.h"

// The ember powers' core (Embers/EmberComponent.h): the hero's powers owned and the one equipped, the Ember key's gate,
// the two clocks run by hand, a death cutting a power short, the save and its round trip, and the key. The three powers
// themselves are Looter.Embers.Power.* (EmberPowerTests.cpp).

namespace
{
	constexpr float EmberFrame = 0.1f;

	UEmberDefinition* MakeEmber(const TCHAR* Id, float Cooldown, float Duration, bool bWithClass)
	{
		UEmberDefinition* Def = NewObject<UEmberDefinition>(GetTransientPackage());
		Def->Id = Id;
		Def->DisplayName = FText::FromString(Id);
		Def->CooldownSeconds = Cooldown;
		Def->DurationSeconds = Duration;
		Def->PowerClass = bWithClass ? UEmberProbePower::StaticClass() : nullptr;
		return Def;
	}

	/** Runs the component's own tick for Seconds, as the world would. */
	void RunEmbers(UEmberComponent* Embers, float Seconds)
	{
		const int32 Frames = FMath::RoundToInt(Seconds / EmberFrame);
		for (int32 Index = 0; Index < Frames; ++Index)
		{
			static_cast<UActorComponent*>(Embers)->TickComponent(EmberFrame, LEVELTICK_All, nullptr);
		}
	}

	UEmberProbePower* ProbeOf(const UEmberComponent* Embers, const TCHAR* Id)
	{
		return Cast<UEmberProbePower>(Embers->GetPowerFor(Id));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEmberComponentTest, "Looter.Embers.Component",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FEmberComponentTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();

	// A hero of three: an instant one, one that lasts two seconds, and one with no class (it can't fire).
	UHeroDefinition* Hero = NewObject<UHeroDefinition>(GetTransientPackage());
	Hero->Id = TEXT("TestHero");
	Hero->Powers.Add(MakeEmber(TEXT("Snap"), 4.f, 0.f, true));
	Hero->Powers.Add(MakeEmber(TEXT("Hold"), 6.f, 2.f, true));
	Hero->Powers.Add(MakeEmber(TEXT("Empty"), 3.f, 0.f, false));

	// A bare body with health and the component (no controller: only the world's own gate counts).
	AActor* Body = World->SpawnActor<AActor>();
	UHealthComponent* Health = NewObject<UHealthComponent>(Body);
	Health->bShowDamageNumbers = false;
	Health->RegisterComponent();
	UEmberComponent* Embers = NewObject<UEmberComponent>(Body);
	Embers->HeroDefinition = Hero;
	Embers->RegisterComponent();
	Body->DispatchBeginPlay();

	int32 Used = 0;
	int32 StateChanges = 0;
	EEmberDenied LastDenied = EEmberDenied::None;
	FName LastEquipped;
	Embers->OnUsed.AddLambda([&Used](FName) { ++Used; });
	Embers->OnStateChanged.AddLambda([&StateChanges](EEmberState) { ++StateChanges; });
	Embers->OnDenied.AddLambda([&LastDenied](EEmberDenied Why) { LastDenied = Why; });
	Embers->OnEquipped.AddLambda([&LastEquipped](FName Id) { LastEquipped = Id; });

	// Owned and equipped: the hero's three, the first equipped, ready; a power object for each with a class.
	TestTrue(TEXT("The hero bound"), Embers->GetHero() == Hero);
	TestEqual(TEXT("Three owned"), Embers->NumOwned(), 3);
	TestEqual(TEXT("The first equipped in a new game"), Embers->GetEquippedId(), FName(TEXT("Snap")));
	TestTrue(TEXT("...ready"), Embers->IsReady() && Embers->GetCooldownFraction() == 0.f);
	UEmberProbePower* Snap = ProbeOf(Embers, TEXT("Snap"));
	UEmberProbePower* Hold = ProbeOf(Embers, TEXT("Hold"));
	if (!TestNotNull(TEXT("Snap's power made"), Snap) || !TestNotNull(TEXT("Hold's power made"), Hold))
	{
		return false;
	}
	TestNull(TEXT("No power made for a definition with no class"), Embers->GetPowerFor(TEXT("Empty")));

	// The power's own say: a refusal fires nothing and starts no clock.
	Snap->Refusal = EEmberDenied::Refused;
	TestEqual(TEXT("A power that refuses: Refused"), Embers->TryUse(), EEmberDenied::Refused);
	TestTrue(TEXT("...told"), LastDenied == EEmberDenied::Refused);
	TestTrue(TEXT("...nothing fired, still ready"), Snap->Activations == 0 && Embers->IsReady());
	Snap->Refusal = EEmberDenied::None;

	// An instant power: fired and ended at once, into its cooldown (counted from the use).
	TestEqual(TEXT("Snap fires"), Embers->TryUse(), EEmberDenied::None);
	TestTrue(TEXT("...activated and ended at once"), Snap->Activations == 1 && Snap->Ends == 1 && !Snap->bLastInterrupted);
	TestEqual(TEXT("...with its own data"), Snap->LastDefinition, FName(TEXT("Snap")));
	TestEqual(TEXT("...level 1's damage scale with no player"), Snap->LastLevelScale, 1.f, 0.001f);
	TestEqual(TEXT("...used once"), Used, 1);
	TestTrue(TEXT("...cooling down, all of it to go"), Embers->GetState() == EEmberState::Cooldown && FMath::IsNearlyEqual(Embers->GetCooldownFraction(), 1.f));
	TestEqual(TEXT("A press now: Cooldown"), Embers->TryUse(), EEmberDenied::Cooldown);
	TestTrue(TEXT("...told"), LastDenied == EEmberDenied::Cooldown);

	// A swap mid-cooldown keeps the wait (held to the new power's own).
	TestTrue(TEXT("Hold equipped while Snap cools"), Embers->Equip(TEXT("Hold")));
	TestEqual(TEXT("...told"), LastEquipped, FName(TEXT("Hold")));
	TestEqual(TEXT("...the wait carried over"), Embers->GetCooldownLeft(), 4.f, 0.01f);
	TestFalse(TEXT("Equipping the one equipped changes nothing"), Embers->Equip(TEXT("Hold")));
	TestFalse(TEXT("Nothing the hero doesn't own"), Embers->Equip(TEXT("Nobody")));
	RunEmbers(Embers, 2.f);
	TestEqual(TEXT("Two seconds on, two left"), Embers->GetCooldownLeft(), 2.f, 0.05f);
	RunEmbers(Embers, 2.1f);
	TestTrue(TEXT("Its cooldown out: ready"), Embers->IsReady() && Embers->GetCooldownLeft() == 0.f);

	// A power with a duration: running, ticked, its arc counting down; nothing else while it runs.
	const int32 ChangesBefore = StateChanges;
	TestEqual(TEXT("Hold fires"), Embers->TryUse(), EEmberDenied::None);
	TestTrue(TEXT("...running"), Embers->IsActive() && Hold->IsRunning() && Embers->GetActivePower() == Hold);
	TestTrue(TEXT("...its arc full"), FMath::IsNearlyEqual(Embers->GetActiveFraction(), 1.f));
	TestTrue(TEXT("...the state change told"), StateChanges > ChangesBefore);
	TestEqual(TEXT("A press while it runs: Active"), Embers->TryUse(), EEmberDenied::Active);
	TestFalse(TEXT("No swap while it runs"), Embers->Equip(TEXT("Snap")));
	RunEmbers(Embers, 1.f);
	TestTrue(TEXT("A second in: ticked, half its arc left"), Hold->Ticks >= 9 && FMath::IsNearlyEqual(Embers->GetActiveFraction(), 0.5f, 0.06f));
	RunEmbers(Embers, 1.1f);
	TestTrue(TEXT("Its time up: ended, not cut short"), Hold->Ends == 1 && !Hold->bLastInterrupted && !Hold->IsRunning());
	TestTrue(TEXT("...cooling down the rest (counted from the use)"), Embers->GetState() == EEmberState::Cooldown
		&& FMath::IsNearlyEqual(Embers->GetCooldownLeft(), 3.9f, 0.15f));
	TestNull(TEXT("...nothing running"), Embers->GetActivePower());

	// One with no class: equips, can't fire.
	TestTrue(TEXT("Empty equips"), Embers->Equip(TEXT("Empty")));
	TestEqual(TEXT("...but has no power: NoPower"), Embers->TryUse(), EEmberDenied::NoPower);
	TestTrue(TEXT("Back to Hold"), Embers->Equip(TEXT("Hold")));

	// The save: the equipped one by id and its cooldown left; a load puts both back.
	FEmberSave Saved;
	Embers->SaveEmbers(Saved);
	TestEqual(TEXT("Saved: the equipped one"), Saved.Equipped, FName(TEXT("Hold")));
	TestEqual(TEXT("Saved: its cooldown left"), Saved.CooldownLeft, Embers->GetCooldownLeft(), 0.001f);
	TArray<uint8> Bytes;
	FMemoryWriter Writer(Bytes, true);
	FObjectAndNameAsStringProxyArchive WriteProxy(Writer, false);
	FEmberSave::StaticStruct()->SerializeItem(WriteProxy, &Saved, nullptr);
	FEmberSave Read;
	FMemoryReader Reader(Bytes, true);
	FObjectAndNameAsStringProxyArchive ReadProxy(Reader, true);
	FEmberSave::StaticStruct()->SerializeItem(ReadProxy, &Read, nullptr);
	TestTrue(TEXT("Read back as saved"), Read.Equipped == Saved.Equipped && FMath::IsNearlyEqual(Read.CooldownLeft, Saved.CooldownLeft));
	const FEmberSave Fresh;
	TestTrue(TEXT("A save from before the powers: none equipped, ready"), Fresh.Equipped.IsNone() && Fresh.CooldownLeft == 0.f);
	TestNotNull(TEXT("The session keeps bHasEmbers"), FindFProperty<FBoolProperty>(ULooterSessionSave::StaticClass(), TEXT("bHasEmbers")));
	TestNotNull(TEXT("The session keeps Embers"), FindFProperty<FStructProperty>(ULooterSessionSave::StaticClass(), TEXT("Embers")));

	FEmberSave SnapCooling;
	SnapCooling.Equipped = TEXT("Snap");
	SnapCooling.CooldownLeft = 2.5f;
	Embers->RestoreEmbers(SnapCooling);
	TestTrue(TEXT("Restored: Snap equipped, cooling, 2.5 s left"), Embers->GetEquippedId() == FName(TEXT("Snap"))
		&& Embers->GetState() == EEmberState::Cooldown && FMath::IsNearlyEqual(Embers->GetCooldownLeft(), 2.5f));
	FEmberSave Stale;
	Stale.Equipped = TEXT("Gone");
	Stale.CooldownLeft = 99.f;
	Embers->RestoreEmbers(Stale);
	TestTrue(TEXT("A power no longer owned: the hero's first, the wait held to its cooldown"), Embers->GetEquippedId() == FName(TEXT("Snap"))
		&& FMath::IsNearlyEqual(Embers->GetCooldownLeft(), 4.f));
	FEmberSave HoldReady;
	HoldReady.Equipped = TEXT("Hold");
	Embers->RestoreEmbers(HoldReady);
	TestTrue(TEXT("Restored ready"), Embers->GetEquippedId() == FName(TEXT("Hold")) && Embers->IsReady());

	// Death cuts a running power short, and a dead body fires nothing.
	TestEqual(TEXT("Hold fires again"), Embers->TryUse(), EEmberDenied::None);
	{
		FEditorScriptExecutionGuard RunActorEvents;
		Body->OnTakeAnyDamage.Broadcast(Body, 10000.f, GetDefault<UDamageType>(), nullptr, nullptr);
	}
	TestTrue(TEXT("The body is dead"), Health->IsDead());
	TestEqual(TEXT("A press while dead: Dead"), Embers->TryUse(), EEmberDenied::Dead);
	RunEmbers(Embers, EmberFrame);
	TestTrue(TEXT("...the running power cut short"), Hold->Ends == 2 && Hold->bLastInterrupted && !Embers->IsActive());

	// Interrupt with nothing running does nothing.
	const int32 EndsBefore = Hold->Ends;
	Embers->Interrupt();
	TestEqual(TEXT("Interrupt with nothing running ends nothing"), Hold->Ends, EndsBefore);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEmberKeysTest, "Looter.Embers.Keys",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FEmberKeysTest::RunTest(const FString& Parameters)
{
	// Q by default (rebindable, "Ember power" under Combat), the left bumper on a gamepad (the user's call, 2026-10-15).
	TestTrue(TEXT("Its binding is \"Ember\""), UKeyBindingSubsystem::EmberBindingId() == FName(TEXT("Ember")));
	TestTrue(TEXT("Q by default"), UKeyBindingSubsystem::DefaultEmberKey() == EKeys::Q);
	TestTrue(TEXT("The left bumper on a gamepad"), UKeyBindingSubsystem::DefaultEmberGamepadKey() == EKeys::Gamepad_LeftShoulder);
	for (const FKey& Taken : { UKeyBindingSubsystem::DefaultGrenadeKey(), UKeyBindingSubsystem::DefaultMeleeKey(),
		UKeyBindingSubsystem::DefaultDropWeaponKey(), UKeyBindingSubsystem::DefaultMapKey(), FKey(EKeys::E), FKey(EKeys::R) })
	{
		TestTrue(FString::Printf(TEXT("Not on %s"), *Taken.ToString()), UKeyBindingSubsystem::DefaultEmberKey() != Taken);
	}
	for (const FKey& Taken : { UKeyBindingSubsystem::DefaultGrenadeGamepadKey(), UKeyBindingSubsystem::DefaultMeleeGamepadKey(),
		UKeyBindingSubsystem::DefaultMapGamepadKey() })
	{
		TestTrue(FString::Printf(TEXT("Its pad key isn't %s"), *Taken.ToString()), UKeyBindingSubsystem::DefaultEmberGamepadKey() != Taken);
	}

	// The mapping the settings menu rebinds has to be there. A fresh outer, so repeated runs never share object names.
	UInputMappingContext* Context = NewObject<UInputMappingContext>(NewObject<ULooterKeyBindingsSave>());
	const UInputAction* Action = UKeyBindingSubsystem::AddEmberAction(Context, *Context);
	if (!TestNotNull(TEXT("The ember action"), Action))
	{
		return false;
	}
	for (const FKey& Key : { UKeyBindingSubsystem::DefaultEmberKey(), UKeyBindingSubsystem::DefaultEmberGamepadKey() })
	{
		TestTrue(FString::Printf(TEXT("Mapped to %s"), *Key.ToString()), Context->GetMappings().ContainsByPredicate([Action, &Key](const FEnhancedActionKeyMapping& Mapping)
		{
			return Mapping.Action == Action && Mapping.Key == Key;
		}));
	}
	return true;
}

#endif

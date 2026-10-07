#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Settings/ControlSettingsSubsystem.h"
#include "Settings/GraphicsSettingsSubsystem.h"
#include "Settings/KeyBindingSubsystem.h"
#include "Inventory/WeaponManagerComponent.h"
#include "InputAction.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"
#include "Kismet/GameplayStatics.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FQualityPresetsTest, "Looter.Settings.QualityPresets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FQualityPresetsTest::RunTest(const FString& Parameters)
{
	// The rules the presets were measured with (Docs/Performance.md): Lumen only on High and Epic, TSR only on Epic
	// (TAA below), and no Nanite below High, which the reference card pays about 2.5 ms for. Below High, shadows are
	// cascades rather than virtual shadow maps, and there is no screen-space or distance field occlusion.
	for (const EGraphicsQuality Quality : { EGraphicsQuality::Low, EGraphicsQuality::Medium, EGraphicsQuality::High, EGraphicsQuality::Epic })
	{
		const FString Name = UGraphicsSettingsSubsystem::QualityName(Quality);
		const TMap<FString, int32> Settings = UGraphicsSettingsSubsystem::QualitySettings(Quality);
		const bool bHighOrEpic = Quality == EGraphicsQuality::High || Quality == EGraphicsQuality::Epic;
		TestEqual(Name + TEXT(" Lumen lighting"), Settings.FindRef(TEXT("r.DynamicGlobalIlluminationMethod")), bHighOrEpic ? 1 : 0);
		TestEqual(Name + TEXT(" Lumen reflections"), Settings.FindRef(TEXT("r.ReflectionMethod")), bHighOrEpic ? 1 : 0);
		TestEqual(Name + TEXT(" anti-aliasing"), Settings.FindRef(TEXT("r.AntiAliasingMethod")), Quality == EGraphicsQuality::Epic ? 4 : 2);
		TestEqual(Name + TEXT(" Nanite"), Settings.FindRef(TEXT("r.Nanite")), bHighOrEpic ? 1 : 0);
		TestEqual(Name + TEXT(" virtual shadow maps"), Settings.FindRef(TEXT("r.Shadow.Virtual.Enable")), bHighOrEpic ? 1 : 0);
		TestEqual(Name + TEXT(" SSAO"), Settings.FindRef(TEXT("r.AmbientOcclusionLevels")), bHighOrEpic ? -1 : 0);
		TestEqual(Name + TEXT(" distance field AO"), Settings.FindRef(TEXT("r.DistanceFieldAO")), bHighOrEpic ? 1 : 0);
	}

	// A fresh install starts on the minimum spec, without motion blur, with the minimap and the FPS counter showing.
	TestTrue(TEXT("Starts on Medium"), GetDefault<ULooterGraphicsSave>()->Quality == EGraphicsQuality::Medium);
	TestFalse(TEXT("Starts without motion blur"), GetDefault<ULooterGraphicsSave>()->bMotionBlur);
	TestTrue(TEXT("Starts with the FPS counter"), GetDefault<ULooterGraphicsSave>()->bShowFrameRate);
	TestTrue(TEXT("Starts with the minimap"), GetDefault<ULooterGraphicsSave>()->bShowMinimap);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFieldOfViewSettingTest, "Looter.Settings.FieldOfView",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFieldOfViewSettingTest::RunTest(const FString& Parameters)
{
	using Graphics = UGraphicsSettingsSubsystem;

	// A fresh install, and a save from before the setting, keep the view the game was made with.
	TestEqual(TEXT("Defaults to 90 degrees"), Graphics::DefaultFieldOfView, 90.f);
	TestEqual(TEXT("A fresh save starts at the default"), GetDefault<ULooterGraphicsSave>()->FirstPersonFieldOfView, Graphics::DefaultFieldOfView);

	// The slider runs from 70 to 110 in whole degrees, and anything outside (a hand-edited save) is pulled back in.
	TestEqual(TEXT("Narrowest is 70"), Graphics::MinFieldOfView, 70.f);
	TestEqual(TEXT("Widest is 110"), Graphics::MaxFieldOfView, 110.f);
	TestEqual(TEXT("Too narrow clamps to 70"), Graphics::ClampFieldOfView(40.f), 70.f);
	TestEqual(TEXT("Too wide clamps to 110"), Graphics::ClampFieldOfView(170.f), 110.f);
	TestEqual(TEXT("The ends are kept"), Graphics::ClampFieldOfView(70.f), 70.f);
	TestEqual(TEXT("Inside the range is kept"), Graphics::ClampFieldOfView(103.f), 103.f);
	TestEqual(TEXT("Whole degrees"), Graphics::ClampFieldOfView(84.6f), 85.f);
	TestEqual(TEXT("The default is in range"), Graphics::ClampFieldOfView(Graphics::DefaultFieldOfView), Graphics::DefaultFieldOfView);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponSlotKeysTest, "Looter.Settings.WeaponSlotKeys",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWeaponSlotKeysTest::RunTest(const FString& Parameters)
{
	// One key per equip slot, the number keys above the letters by default.
	TestEqual(TEXT("A key for every slot"), UKeyBindingSubsystem::NumWeaponSlotKeys, GetDefault<UWeaponManagerComponent>()->MaxWeapons);
	const FKey Expected[] = { EKeys::One, EKeys::Two, EKeys::Three };
	TestEqual(TEXT("Three slot keys"), UKeyBindingSubsystem::NumWeaponSlotKeys, static_cast<int32>(UE_ARRAY_COUNT(Expected)));
	TestFalse(TEXT("No key past the last slot"), UKeyBindingSubsystem::DefaultWeaponSlotKey(UKeyBindingSubsystem::NumWeaponSlotKeys).IsValid());

	// The context the player gets: the settings menu rebinds each slot's mapping to its default key, so it has to be there.
	// A fresh outer, so repeated runs never share object names.
	TArray<TObjectPtr<UInputAction>> Actions;
	const UInputMappingContext* Context = UKeyBindingSubsystem::BuildWeaponSlotContext(NewObject<ULooterKeyBindingsSave>(), Actions);
	if (!TestNotNull(TEXT("Context built"), Context) || !TestEqual(TEXT("One action per slot"), Actions.Num(), UKeyBindingSubsystem::NumWeaponSlotKeys))
	{
		return false;
	}
	for (int32 SlotIndex = 0; SlotIndex < UKeyBindingSubsystem::NumWeaponSlotKeys; ++SlotIndex)
	{
		const FString Name = FString::Printf(TEXT("Slot %d"), SlotIndex + 1);
		const FKey Key = Expected[SlotIndex];
		TestTrue(Name + TEXT(" binding id"), UKeyBindingSubsystem::WeaponSlotBindingId(SlotIndex) == FName(*FString::Printf(TEXT("WeaponSlot%d"), SlotIndex + 1)));
		TestTrue(Name + TEXT(" default key"), UKeyBindingSubsystem::DefaultWeaponSlotKey(SlotIndex) == Key);
		const UInputAction* Action = Actions[SlotIndex];
		TestTrue(Name + TEXT(" mapped to its key"), Action && Context->GetMappings().ContainsByPredicate([Action, &Key](const FEnhancedActionKeyMapping& Mapping)
		{
			return Mapping.Action == Action && Mapping.Key == Key;
		}));
	}
	TestTrue(TEXT("A different action per slot"), Actions[0] != Actions[1] && Actions[1] != Actions[2] && Actions[0] != Actions[2]);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLookSensitivitySettingTest, "Looter.Settings.LookSensitivity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FLookSensitivitySettingTest::RunTest(const FString& Parameters)
{
	using Controls = UControlSettingsSubsystem;

	// A fresh install, and a save from before the setting, turn the view as the game was made.
	TestEqual(TEXT("Defaults to 1x"), Controls::DefaultLookSensitivity, 1.f);
	TestEqual(TEXT("A fresh save starts at the default"), GetDefault<ULooterControlsSave>()->LookSensitivity, Controls::DefaultLookSensitivity);

	// The slider runs from 0.1x to 3x in steps of 0.05, and anything outside (a hand-edited save) is pulled back in.
	TestEqual(TEXT("Slowest is 0.1x"), Controls::MinLookSensitivity, 0.1f);
	TestEqual(TEXT("Fastest is 3x"), Controls::MaxLookSensitivity, 3.f);
	TestEqual(TEXT("Too slow clamps to 0.1x"), Controls::ClampLookSensitivity(0.f), 0.1f);
	TestEqual(TEXT("Backwards clamps to 0.1x"), Controls::ClampLookSensitivity(-2.f), 0.1f);
	TestEqual(TEXT("Too fast clamps to 3x"), Controls::ClampLookSensitivity(12.f), 3.f);
	TestEqual(TEXT("The ends are kept"), Controls::ClampLookSensitivity(3.f), 3.f);
	TestEqual(TEXT("Inside the range is kept"), Controls::ClampLookSensitivity(1.75f), 1.75f);
	TestEqual(TEXT("Rounded to the slider's steps"), Controls::ClampLookSensitivity(1.23f), 1.25f);
	TestEqual(TEXT("Not a number: the default"), Controls::ClampLookSensitivity(std::numeric_limits<float>::quiet_NaN()), Controls::DefaultLookSensitivity);
	TestEqual(TEXT("The default is in range"), Controls::ClampLookSensitivity(Controls::DefaultLookSensitivity), Controls::DefaultLookSensitivity);

	// Saved and read back as the subsystem does, through a slot of the test's own so the player's setting is never touched.
	const FString Slot = TEXT("ControlSettings_AutomationTest");
	UGameplayStatics::DeleteGameInSlot(Slot, 0);
	ULooterControlsSave* Missing = Controls::LoadControls(Slot, nullptr);
	TestNotNull(TEXT("No save yet: a fresh set"), Missing);
	TestEqual(TEXT("...at the default"), Controls::LookSensitivityOf(Missing), Controls::DefaultLookSensitivity);
	TestEqual(TEXT("No save object at all: the default"), Controls::LookSensitivityOf(nullptr), Controls::DefaultLookSensitivity);

	ULooterControlsSave* Written = NewObject<ULooterControlsSave>();
	Written->LookSensitivity = 1.75f;
	if (!TestTrue(TEXT("Saved"), UGameplayStatics::SaveGameToSlot(Written, Slot, 0)))
	{
		return false;
	}
	const ULooterControlsSave* ReadBack = Controls::LoadControls(Slot, nullptr);
	TestTrue(TEXT("Read back a new copy"), ReadBack && ReadBack != Written);
	TestEqual(TEXT("...with the sensitivity saved"), Controls::LookSensitivityOf(ReadBack), 1.75f);

	// A hand-edited save can't spin the view: what's read back is clamped.
	Written->LookSensitivity = 40.f;
	UGameplayStatics::SaveGameToSlot(Written, Slot, 0);
	TestEqual(TEXT("A save past the end reads as 3x"), Controls::LookSensitivityOf(Controls::LoadControls(Slot, nullptr)), 3.f);
	UGameplayStatics::DeleteGameInSlot(Slot, 0);

	// Every turn of the view goes through it, mouse and stick alike: the look input scaled by the setting.
	const FVector2D Input(3.f, -1.5f);
	TestTrue(TEXT("1x leaves the turn as it was"), Controls::ScaleLookInput(Input, 1.f).Equals(Input));
	TestTrue(TEXT("2x turns twice as far"), Controls::ScaleLookInput(Input, 2.f).Equals(FVector2D(6.f, -3.f)));
	TestTrue(TEXT("0.5x turns half as far"), Controls::ScaleLookInput(Input, 0.5f).Equals(FVector2D(1.5f, -0.75f)));
	TestTrue(TEXT("Scaled within the range"), Controls::ScaleLookInput(Input, 50.f).Equals(Input * Controls::MaxLookSensitivity));
	// Anything without a local player (AI, tests) turns as the game was made.
	TestTrue(TEXT("No player: unchanged"), Controls::ScaleLookInputFor(nullptr, Input).Equals(Input));
	return true;
}

#endif

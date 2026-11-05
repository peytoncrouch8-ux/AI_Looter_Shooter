#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Settings/ControlSettingsSubsystem.h"
#include "Settings/GraphicsSettingsSubsystem.h"
#include "Settings/KeyBindingSubsystem.h"
#include "Inventory/WeaponManagerComponent.h"
#include "UI/Menus/SettingsMenuHelp.h"
#include "UI/Menus/SettingsMenuParts.h"
#include "UI/Menus/SettingsMenuWidget.h"
#include "UI/Style/LooterButton.h"
#include "UI/Style/LooterUIFocus.h"
#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "InputAction.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"
#include "Kismet/GameplayStatics.h"
#include "Tests/AutomationCommon.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FQualityPresetsTest, "Looter.Settings.QualityPresets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FQualityPresetsTest::RunTest(const FString& Parameters)
{
	// The rules the presets were measured with (Docs/Performance.md): Lumen and TSR only on Epic (TAA below), no Nanite
	// below High, which the reference card pays about 2.5 ms for; High reflects in screen space (2026-10-14: Lumen held
	// Epic at 22-28 fps there, High without it 53-71). Below High, shadows are cascades rather than virtual shadow maps,
	// and there is no screen-space or distance field occlusion; distance field occlusion only on Epic.
	for (const EGraphicsQuality Quality : { EGraphicsQuality::Low, EGraphicsQuality::Medium, EGraphicsQuality::High, EGraphicsQuality::Epic })
	{
		const FString Name = UGraphicsSettingsSubsystem::QualityName(Quality);
		const TMap<FString, float> Settings = UGraphicsSettingsSubsystem::QualitySettings(Quality);
		// The integer variables, as the console sets them.
		const auto Whole = [&Settings](const TCHAR* Variable) { return FMath::RoundToInt32(Settings.FindRef(Variable)); };
		const bool bHighOrEpic = Quality == EGraphicsQuality::High || Quality == EGraphicsQuality::Epic;
		const bool bEpic = Quality == EGraphicsQuality::Epic;
		TestEqual(Name + TEXT(" Lumen lighting"), Whole(TEXT("r.DynamicGlobalIlluminationMethod")), bEpic ? 1 : 0);
		TestEqual(Name + TEXT(" reflections"), Whole(TEXT("r.ReflectionMethod")), bEpic ? 1 : (bHighOrEpic ? 2 : 0));
		TestEqual(Name + TEXT(" anti-aliasing"), Whole(TEXT("r.AntiAliasingMethod")), bEpic ? 4 : 2);
		TestEqual(Name + TEXT(" Nanite"), Whole(TEXT("r.Nanite")), bHighOrEpic ? 1 : 0);
		TestEqual(Name + TEXT(" virtual shadow maps"), Whole(TEXT("r.Shadow.Virtual.Enable")), bHighOrEpic ? 1 : 0);
		TestEqual(Name + TEXT(" SSAO"), Whole(TEXT("r.AmbientOcclusionLevels")), bHighOrEpic ? -1 : 0);
		TestEqual(Name + TEXT(" distance field AO"), Whole(TEXT("r.DistanceFieldAO")), bEpic ? 1 : 0);
		// The art audit's step 1 (2026-11-04): Medium keeps its full-size textures (the mips cost 0.2 ms); its filtering is
		// 4x, High's and Epic's 8x (the user's yes on the evening of 2026-11-04: 8x cost the open views 0.1-0.4 ms over the
		// 120 fps line); creature LOD0 up close stays High's and Epic's; Low stays the engine's own.
		const bool bLow = Quality == EGraphicsQuality::Low;
		const bool bMedium = Quality == EGraphicsQuality::Medium;
		TestEqual(Name + TEXT(" texture mip bias"), Whole(TEXT("r.Streaming.MipBias")), bLow ? 16 : 0);
		TestEqual(Name + TEXT(" anisotropy"), Whole(TEXT("r.MaxAnisotropy")), bLow ? 1 : (bMedium ? 4 : 8));
		TestEqual(Name + TEXT(" skeletal LOD bias"), Whole(TEXT("r.SkeletalMeshLODBias")), bLow ? 2 : (bHighOrEpic ? 0 : 1));
		TestEqual(Name + TEXT(" streaming pool"), Whole(TEXT("r.Streaming.PoolSize")), bLow ? 400 : 1000);
		// Medium's static meshes take their next LOD a quarter sooner (0-0.25 ms in the heaviest views); the engine's own
		// distances elsewhere.
		TestEqual(Name + TEXT(" static mesh LOD distance"), Settings.FindRef(TEXT("r.StaticMeshLODDistanceScale")), bMedium ? 1.25f : 1.f);
		// The shrubs' shadows only on High and Epic (the user's pick, 2026-11-04: about 0.1 ms at the farm).
		TestEqual(Name + TEXT(" shrub shadows"), Whole(TEXT("Looter.Scatter.HighShadows")), bHighOrEpic ? 1 : 0);
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
	const FKey Expected[] = { EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four };
	TestEqual(TEXT("Four slot keys (the user, 2026-10-31: four guns)"), UKeyBindingSubsystem::NumWeaponSlotKeys, static_cast<int32>(UE_ARRAY_COUNT(Expected)));
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
	TestTrue(TEXT("No save object at all: the default"), (Controls::LookSensitivityOf(nullptr)) == Controls::DefaultLookSensitivity);

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGamepadControlsSettingTest, "Looter.Settings.GamepadControls",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGamepadControlsSettingTest::RunTest(const FString& Parameters)
{
	using Controls = UControlSettingsSubsystem;

	// Invert Y starts off and Aim assist on, for a fresh install and for a save from before they existed.
	TestFalse(TEXT("Invert Y starts off"), Controls::DefaultInvertY);
	TestTrue(TEXT("Aim assist starts on"), Controls::DefaultAimAssist);
	TestTrue(TEXT("A fresh save: Invert Y at the default"), (GetDefault<ULooterControlsSave>()->bInvertY) == Controls::DefaultInvertY);
	TestTrue(TEXT("A fresh save: Aim assist at the default"), (GetDefault<ULooterControlsSave>()->bAimAssist) == Controls::DefaultAimAssist);
	TestTrue(TEXT("No save object at all: Invert Y at the default"), (Controls::InvertYOf(nullptr)) == Controls::DefaultInvertY);
	TestTrue(TEXT("No save object at all: Aim assist at the default"), (Controls::AimAssistOf(nullptr)) == Controls::DefaultAimAssist);
	// A controller that isn't a local player's has no one at the pad: the view isn't inverted and nothing is assisted.
	TestFalse(TEXT("No local player: not inverted"), Controls::IsInvertYFor(nullptr));
	TestFalse(TEXT("No local player: no aim assist"), Controls::IsAimAssistFor(nullptr));

	// Saved and read back as the subsystem does, through a slot of the test's own, with the settings beside them untouched.
	const FString Slot = TEXT("ControlSettings_AutomationTest_Gamepad");
	UGameplayStatics::DeleteGameInSlot(Slot, 0);
	const ULooterControlsSave* Missing = Controls::LoadControls(Slot, nullptr);
	TestTrue(TEXT("No save yet: Invert Y at the default"), (Controls::InvertYOf(Missing)) == Controls::DefaultInvertY);
	TestTrue(TEXT("...and Aim assist"), (Controls::AimAssistOf(Missing)) == Controls::DefaultAimAssist);

	ULooterControlsSave* Written = NewObject<ULooterControlsSave>();
	Written->bInvertY = true;
	Written->bAimAssist = false;
	Written->LookSensitivity = 1.75f;
	Written->ControllerRumble = 0.35f;
	if (!TestTrue(TEXT("Saved"), UGameplayStatics::SaveGameToSlot(Written, Slot, 0)))
	{
		return false;
	}
	const ULooterControlsSave* ReadBack = Controls::LoadControls(Slot, nullptr);
	TestTrue(TEXT("Invert Y read back on"), Controls::InvertYOf(ReadBack));
	TestFalse(TEXT("Aim assist read back off"), Controls::AimAssistOf(ReadBack));
	TestEqual(TEXT("...with the look sensitivity beside them"), Controls::LookSensitivityOf(ReadBack), 1.75f);
	TestEqual(TEXT("...and the controller rumble"), Controls::ControllerRumbleOf(ReadBack), 0.35f);

	// And the other way round, so neither value is just the default coming back.
	Written->bInvertY = false;
	Written->bAimAssist = true;
	UGameplayStatics::SaveGameToSlot(Written, Slot, 0);
	const ULooterControlsSave* ReadAgain = Controls::LoadControls(Slot, nullptr);
	TestFalse(TEXT("Invert Y read back off"), Controls::InvertYOf(ReadAgain));
	TestTrue(TEXT("Aim assist read back on"), Controls::AimAssistOf(ReadAgain));
	UGameplayStatics::DeleteGameInSlot(Slot, 0);

	// Each row has its words (the Help test checks the sentences) and says its default.
	const SettingsMenu::FSettingHelp Invert = SettingsMenu::RowHelp(SettingsMenu::ESettingRow::InvertY);
	const SettingsMenu::FSettingHelp Assist = SettingsMenu::RowHelp(SettingsMenu::ESettingRow::AimAssist);
	TestEqual(TEXT("The Invert Y row is named"), Invert.Title, FString(TEXT("Invert Y")));
	TestEqual(TEXT("The Aim assist row is named"), Assist.Title, FString(TEXT("Aim assist")));
	TestTrue(TEXT("Invert Y says it starts off"), Invert.Detail.Contains(TEXT("Off")));
	TestTrue(TEXT("Aim assist says it starts on"), Assist.Detail.Contains(TEXT("On")));
	TestTrue(TEXT("Aim assist says it is the gamepad's"), Assist.Body.Contains(TEXT("gamepad")));
	return true;
}

namespace
{
	/** Puts the menus back under the mouse when the test ends, however it ends. */
	struct FGamepadRowsMouseAfter
	{
		FGamepadRowsMouseAfter() { LooterUI::SetMenuDevice(LooterUI::EMenuDevice::Gamepad); }
		~FGamepadRowsMouseAfter() { LooterUI::SetMenuDevice(LooterUI::EMenuDevice::Mouse); }
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGamepadControlsRowsTest, "Looter.Settings.GamepadRows",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGamepadControlsRowsTest::RunTest(const FString& Parameters)
{
	using namespace SettingsMenu;
	FGamepadRowsMouseAfter Device;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	USettingsMenuWidget* Menu = CreateWidget<USettingsMenuWidget>(TestLevel.GetTestWorld(), USettingsMenuWidget::StaticClass());
	if (!TestNotNull(TEXT("The menu"), Menu))
	{
		return false;
	}
	const TSharedRef<SWidget> MenuSlate = Menu->TakeWidget();
	Menu->Open(ESettingsMenuMode::MainMenu);
	Menu->AdvanceNav(0.f);
	auto Tap = [Menu](const FKey& Key)
	{
		Menu->HandleNavKey(Key);
		Menu->HandleNavKeyUp(Key);
		Menu->AdvanceNav(0.f);
	};

	// To the Controls page by the bumpers (Audio, Interface, Controls), the controller driving the page. (A first press only
	// brings the focus look up when the mouse drove the page; Up at the first row moves nothing.)
	Tap(EKeys::Gamepad_DPad_Up);
	for (int32 Step = 0; Step < 3; ++Step)
	{
		Tap(EKeys::Gamepad_RightShoulder);
	}
	if (!TestEqual(TEXT("The Controls page"), Menu->GetPage(), static_cast<int32>(ESettingsPage::Controls)))
	{
		return false;
	}

	// Down the page: the three sliders, then the new switches, then Reset all keys (the key list's first stop).
	TestEqual(TEXT("Starts on the look sensitivity"), Menu->GetShownHelpTitle(), RowHelp(ESettingRow::LookSensitivity).Title);
	Tap(EKeys::Gamepad_DPad_Down);
	TestEqual(TEXT("Down: camera shake"), Menu->GetShownHelpTitle(), RowHelp(ESettingRow::CameraShake).Title);
	Tap(EKeys::Gamepad_DPad_Down);
	TestEqual(TEXT("Down: controller rumble"), Menu->GetShownHelpTitle(), RowHelp(ESettingRow::ControllerRumble).Title);
	Tap(EKeys::Gamepad_DPad_Down);
	TestEqual(TEXT("Down: Invert Y, described"), Menu->GetShownHelpTitle(), RowHelp(ESettingRow::InvertY).Title);
	TestEqual(TEXT("...with its words"), Menu->GetShownHelpText(), RowHelp(ESettingRow::InvertY).Body);
	ULooterButton* Switch = Cast<ULooterButton>(Menu->GetFocusedWidget());
	if (TestNotNull(TEXT("...a switch under the focus"), Switch))
	{
		// A press lights the side pressed (with no local player the setting itself can't change, but the switch still answers).
		TestTrue(TEXT("The switch presses"), Switch->Press());
		TestTrue(TEXT("...and the side pressed is the one lit"), Switch->IsHighlighted());
	}
	Tap(EKeys::Gamepad_DPad_Down);
	TestEqual(TEXT("Down: Aim assist, described"), Menu->GetShownHelpTitle(), RowHelp(ESettingRow::AimAssist).Title);
	TestEqual(TEXT("...with its words"), Menu->GetShownHelpText(), RowHelp(ESettingRow::AimAssist).Body);
	Tap(EKeys::Gamepad_DPad_Down);
	const ULooterButton* ResetAll = Cast<ULooterButton>(Menu->GetFocusedWidget());
	TestTrue(TEXT("Down: Reset all keys, after the page's two switches"), ResetAll && ResetAll->Action == ActionResetAll);
	TestEqual(TEXT("...described"), Menu->GetShownHelpTitle(), RowHelp(ESettingRow::ResetAllKeys).Title);
	return true;
}

#endif

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Settings/GraphicsSettingsSubsystem.h"

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

	// A fresh install starts on the minimum spec, without motion blur.
	TestTrue(TEXT("Starts on Medium"), GetDefault<ULooterGraphicsSave>()->Quality == EGraphicsQuality::Medium);
	TestFalse(TEXT("Starts without motion blur"), GetDefault<ULooterGraphicsSave>()->bMotionBlur);
	return true;
}

#endif

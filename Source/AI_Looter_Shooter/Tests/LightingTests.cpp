#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "World/LightingState.h"
#include "World/LightingStates.h"
#include "World/LightingStateSubsystem.h"
#include "World/LightingTargets.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/Level.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/SkyLight.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionCollectionParameter.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "Misc/PackageName.h"
#include "Tests/AutomationCommon.h"

namespace
{
	const FName DuskName(TEXT("Dusk"));

	/** A dusk for the test level that differs from Day in everything a state sets. */
	FLightingState MakeTestDusk()
	{
		FLightingState Dusk;
		Dusk.Name = DuskName;
		Dusk.SunBearing = 252.f;
		Dusk.SunElevation = 4.f;
		Dusk.SunIntensity = 4.f;
		Dusk.SunTemperature = 3200.f;
		Dusk.ShadowDistance = 6000.f;
		Dusk.ShadowCascades = 1;
		Dusk.SkyIntensity = 0.6f;
		Dusk.SkyLuminance = FLinearColor(1.2f, 2.1f, 5.f);
		Dusk.Ozone = 3.5f;
		Dusk.FogDensity = 0.015f;
		Dusk.FogInscattering = FLinearColor(0.20f, 0.13f, 0.08f);
		Dusk.FogDirectionalInscattering = FLinearColor(0.95f, 0.45f, 0.15f);
		Dusk.FogDirectionalExponent = 6.f;
		Dusk.FogDirectionalStartDistance = 6000.f;
		Dusk.BackdropTint = FLinearColor(0.30f, 0.24f, 0.24f);
		Dusk.CloudTint = FLinearColor(0.50f, 0.33f, 0.28f);
		Dusk.ExposureBias = 0.5f;
		return Dusk;
	}

	/** Every value of a state against another's (the sun's bearing the short way round). */
	void TestStatesMatch(FAutomationTestBase& Test, const FString& What, const FLightingState& Actual, const FLightingState& Expected)
	{
		Test.TestTrue(FString::Printf(TEXT("%s: the sun at bearing %.2f (%.2f)"), *What, Expected.SunBearing, Actual.SunBearing),
			FMath::Abs(FMath::FindDeltaAngleDegrees(Actual.SunBearing, Expected.SunBearing)) < 0.05f);
		Test.TestEqual(What + TEXT(": the sun's elevation"), Actual.SunElevation, Expected.SunElevation, 0.05f);
		Test.TestEqual(What + TEXT(": the sun's intensity"), Actual.SunIntensity, Expected.SunIntensity, 1e-3f);
		Test.TestEqual(What + TEXT(": the sun's temperature"), Actual.SunTemperature, Expected.SunTemperature, 0.5f);
		Test.TestEqual(What + TEXT(": how far its shadows reach"), Actual.ShadowDistance, Expected.ShadowDistance, 0.5f);
		Test.TestEqual(What + TEXT(": its shadows' cascades"), Actual.ShadowCascades, Expected.ShadowCascades);
		Test.TestEqual(What + TEXT(": the sky light"), Actual.SkyIntensity, Expected.SkyIntensity, 1e-3f);
		Test.TestTrue(What + TEXT(": the sky's color"), Actual.SkyLuminance.Equals(Expected.SkyLuminance, 1e-3f));
		Test.TestEqual(What + TEXT(": the ozone"), Actual.Ozone, Expected.Ozone, 1e-3f);
		Test.TestEqual(What + TEXT(": the fog's density"), Actual.FogDensity, Expected.FogDensity, 1e-5f);
		Test.TestTrue(What + TEXT(": the haze's color"), Actual.FogInscattering.Equals(Expected.FogInscattering, 1e-3f));
		Test.TestTrue(What + TEXT(": the glow toward the sun"), Actual.FogDirectionalInscattering.Equals(Expected.FogDirectionalInscattering, 1e-3f));
		Test.TestEqual(What + TEXT(": the glow's exponent"), Actual.FogDirectionalExponent, Expected.FogDirectionalExponent, 1e-3f);
		Test.TestEqual(What + TEXT(": where the glow starts"), Actual.FogDirectionalStartDistance, Expected.FogDirectionalStartDistance, 0.5f);
		Test.TestTrue(What + TEXT(": the backdrop's tint"), Actual.BackdropTint.Equals(Expected.BackdropTint, 1e-3f));
		Test.TestTrue(What + TEXT(": the clouds' tint"), Actual.CloudTint.Equals(Expected.CloudTint, 1e-3f));
		Test.TestEqual(What + TEXT(": the exposure bias"), Actual.ExposureBias, Expected.ExposureBias, 1e-3f);
	}

	/** What the lights show against a state, in everything a switch sets on them. */
	void TestLightsShow(FAutomationTestBase& Test, const FString& What, const FLightingTargets& Lights, const FLightingState& Expected)
	{
		// The tints aren't on the lights: they come along from Expected and match by themselves.
		FLightingState Shown = Expected;
		Lights.Read(Shown);
		TestStatesMatch(Test, What, Shown, Expected);
	}

	/**
	 * A test level lit as a build lights one: a movable sun lighting the atmosphere, a movable sky light, height fog, an
	 * unbound post process volume, and the states actor naming them, all in the first state.
	 */
	ALightingStates* SpawnLitTestLevel(UWorld* World, const TArray<FLightingState>& LevelStates)
	{
		ADirectionalLight* Sun = World->SpawnActor<ADirectionalLight>();
		ASkyLight* Sky = World->SpawnActor<ASkyLight>();
		ASkyAtmosphere* Air = World->SpawnActor<ASkyAtmosphere>();
		AExponentialHeightFog* Fog = World->SpawnActor<AExponentialHeightFog>();
		APostProcessVolume* Post = World->SpawnActor<APostProcessVolume>();
		ALightingStates* Placed = World->SpawnActor<ALightingStates>();
		if (!Sun || !Sky || !Air || !Fog || !Post || !Placed || LevelStates.IsEmpty())
		{
			return nullptr;
		}
		Sun->GetComponent()->SetMobility(EComponentMobility::Movable);
		Sun->GetComponent()->SetAtmosphereSunLight(true);
		Sky->GetLightComponent()->SetMobility(EComponentMobility::Movable);
		Post->bUnbound = true;
		Placed->States = LevelStates;
		Placed->Sun = Sun;
		Placed->SkyLight = Sky;
		Placed->Atmosphere = Air;
		Placed->HeightFog = Fog;
		Placed->PostVolume = Post;
		FLightingTargets::Find(nullptr, Placed).Write(LevelStates[0]);
		return Placed;
	}

	/** The value a collection parameter has in a world, or magenta when it has none. */
	FLinearColor CollectionValue(UWorld* World, UMaterialParameterCollection* Collection, FName Parameter)
	{
		FLinearColor Value = FLinearColor(1.f, 0.f, 1.f);
		UMaterialParameterCollectionInstance* Instance = World && Collection ? World->GetParameterCollectionInstance(Collection) : nullptr;
		if (!Instance || !Instance->GetVectorParameterValue(Parameter, Value))
		{
			return FLinearColor(1.f, 0.f, 1.f);
		}
		return Value;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLightingDefaultsTest, "Looter.World.Lighting.Defaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FLightingDefaultsTest::RunTest(const FString& Parameters)
{
	// A default state is the tutorial island's afternoon as Tools/Unreal/build_area_environment.py places it (its
	// DEFAULTS), so a level built without a states block and a default Day agree: the island keeps its light.
	const FLightingState Day;
	TestTrue(TEXT("Named Day"), Day.Name == ALightingStates::DayState);
	TestEqual(TEXT("The sun at bearing 292, west-northwest"), Day.SunBearing, 292.f);
	TestEqual(TEXT("...38 degrees up"), Day.SunElevation, 38.f);
	TestEqual(TEXT("...7 lux"), Day.SunIntensity, 7.f);
	TestEqual(TEXT("...5300 K"), Day.SunTemperature, 5300.f);
	TestEqual(TEXT("Its shadows reach 100 m"), Day.ShadowDistance, 10000.f);
	TestEqual(TEXT("...in two cascades"), Day.ShadowCascades, 2);
	TestEqual(TEXT("The sky light at 1.2"), Day.SkyIntensity, 1.2f);
	TestTrue(TEXT("The sky's own color"), Day.SkyLuminance.Equals(FLinearColor::White));
	TestEqual(TEXT("...and Earth's ozone"), Day.Ozone, 1.f);
	TestEqual(TEXT("Fog at 0.03"), Day.FogDensity, 0.03f);
	TestTrue(TEXT("...the island's blue haze"), Day.FogInscattering.Equals(FLinearColor(0.20f, 0.29f, 0.44f)));
	TestTrue(TEXT("...with no glow toward the sun"), Day.FogDirectionalInscattering.Equals(FLinearColor::Black));
	TestEqual(TEXT("...and the engine's glow exponent"), Day.FogDirectionalExponent, 4.f);
	TestEqual(TEXT("...and start"), Day.FogDirectionalStartDistance, 10000.f);
	TestTrue(TEXT("The backdrop as built"), Day.BackdropTint.Equals(FLinearColor::White));
	TestTrue(TEXT("The clouds as built"), Day.CloudTint.Equals(FLinearColor::White));
	TestEqual(TEXT("Exposure bias 0.4"), Day.ExposureBias, 0.4f);

	// The sun turned as the build script turns it (pitched down by its elevation, facing away from its bearing), and back.
	const FRotator Turn = Day.GetSunRotation();
	TestEqual(TEXT("The sun's light pitches down 38 degrees"), Turn.Pitch, -38.0, 1e-4);
	TestEqual(TEXT("...facing bearing 112"), Turn.Yaw, 112.0, 1e-4);
	TestEqual(TEXT("...unrolled"), Turn.Roll, 0.0, 1e-4);
	for (const float Bearing : { 0.f, 90.f, 247.5f, 252.f, 359.f })
	{
		for (const float Elevation : { 4.f, 15.f, 38.f, 80.f })
		{
			FLightingState Placed;
			Placed.SunBearing = Bearing;
			Placed.SunElevation = Elevation;
			float BackBearing = -1.f;
			float BackElevation = -1.f;
			FLightingState::SunAnglesFromRotation(Placed.GetSunRotation(), BackBearing, BackElevation);
			TestTrue(FString::Printf(TEXT("Bearing %.1f, %.0f up, comes back as %.2f, %.2f up"), Bearing, Elevation, BackBearing, BackElevation),
				FMath::Abs(FMath::FindDeltaAngleDegrees(BackBearing, Bearing)) < 0.01f && FMath::IsNearlyEqual(BackElevation, Elevation, 0.01f));
		}
	}

	// Blending: the ends are the two states, halfway is halfway, and the sun goes the short way round.
	FLightingState From;
	From.SunBearing = 350.f;
	From.SunIntensity = 2.f;
	From.FogInscattering = FLinearColor::Black;
	From.ShadowCascades = 1;
	FLightingState To;
	To.Name = DuskName;
	To.SunBearing = 10.f;
	To.SunIntensity = 4.f;
	To.FogInscattering = FLinearColor(0.4f, 0.2f, 0.f);
	To.ShadowCascades = 3;
	To.Ozone = 3.f;
	const FLightingState Halfway = FLightingState::Blend(From, To, 0.5f);
	TestTrue(TEXT("A blend is named for where it goes"), Halfway.Name == DuskName);
	TestEqual(TEXT("At 0, where it set out"), FLightingState::Blend(From, To, 0.f).SunIntensity, 2.f);
	TestEqual(TEXT("At 1, where it goes"), FLightingState::Blend(From, To, 1.f).SunIntensity, 4.f);
	TestEqual(TEXT("Past the end, still there"), FLightingState::Blend(From, To, 2.f).SunIntensity, 4.f);
	TestEqual(TEXT("Halfway, halfway"), Halfway.SunIntensity, 3.f, 1e-4f);
	TestTrue(TEXT("...the colors too"), Halfway.FogInscattering.Equals(FLinearColor(0.2f, 0.1f, 0.f), 1e-4f));
	TestEqual(TEXT("...the ozone too"), Halfway.Ozone, 2.f, 1e-4f);
	TestEqual(TEXT("...and the cascades, whole"), Halfway.ShadowCascades, 2);
	TestTrue(FString::Printf(TEXT("Halfway from bearing 350 to 10 is north, not south (%.2f)"), Halfway.SunBearing),
		FMath::Abs(FMath::FindDeltaAngleDegrees(Halfway.SunBearing, 0.f)) < 0.01f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLightingSwitchTest, "Looter.World.Lighting.Switch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FLightingSwitchTest::RunTest(const FString& Parameters)
{
	// A level's states switched at once, refused when unknown, back to Day, and blended; and the collection the unlit
	// backdrop and clouds read.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	ULightingStateSubsystem* Lighting = ULightingStateSubsystem::Get(World);
	if (!TestNotNull(TEXT("The level has the lighting states' subsystem"), Lighting))
	{
		return false;
	}

	// A level without states keeps its light: there's nothing to switch to. (The refusals warn; 0 expects the warning at
	// least once.)
	AddExpectedMessagePlain(TEXT("it has no lighting states"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 0);
	TestFalse(TEXT("No states, so no Dusk"), Lighting->SetState(DuskName, ELightingSwitch::Instant));
	TestTrue(TEXT("...and no state in place"), Lighting->GetState().IsNone());

	const FLightingState Day;
	const FLightingState Dusk = MakeTestDusk();
	const ALightingStates* States = SpawnLitTestLevel(World, { Day, Dusk });
	if (!TestNotNull(TEXT("The test level is lit"), States))
	{
		return false;
	}
	const FLightingTargets Lights = FLightingTargets::Find(nullptr, States);
	TestTrue(FString::Printf(TEXT("Every light found (missing: %s)"), *Lights.DescribeMissing()), Lights.DescribeMissing().IsEmpty());
	TestTrue(TEXT("It starts in Day"), Lighting->GetState() == ALightingStates::DayState);
	TestTrue(TEXT("It has Day and Dusk"), Lighting->GetStateNames() == TArray<FName>({ ALightingStates::DayState, DuskName }));

	TArray<FLightingStateChange> Heard;
	const FDelegateHandle Listening = Lighting->OnChanged.AddLambda([&Heard](const FLightingStateChange& Change) { Heard.Add(Change); });

	// Dusk at once, as a scene's cut asks: every light as Dusk has it.
	TestTrue(TEXT("Dusk switched to"), Lighting->SetState(DuskName, ELightingSwitch::Instant));
	TestLightsShow(*this, TEXT("At dusk"), Lights, Dusk);
	// The sky's color correction is the sky's alone: the haze takes the sky's light without it.
	const FLinearColor FogShare = Lights.HeightFog->GetComponent()->SkyAtmosphereAmbientContributionColorScale;
	TestTrue(FString::Printf(TEXT("The fog's share of the sky's light undoes the sky's color (%s)"), *FogShare.ToString()),
		FMath::IsNearlyEqual(FogShare.R * Dusk.SkyLuminance.R, 1.f, 1e-3f) && FMath::IsNearlyEqual(FogShare.G * Dusk.SkyLuminance.G, 1.f, 1e-3f)
		&& FMath::IsNearlyEqual(FogShare.B * Dusk.SkyLuminance.B, 1.f, 1e-3f));
	TestTrue(TEXT("Dusk in place"), Lighting->GetState() == DuskName && !Lighting->IsSwitching());
	TestTrue(TEXT("Heard once, from Day to Dusk"), Heard.Num() == 1 && Heard[0].From == ALightingStates::DayState && Heard[0].To == DuskName
		&& Heard[0].How == ELightingSwitch::Instant);

	// An unknown state is refused, and nothing changes.
	AddExpectedMessagePlain(TEXT("has no state Night"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 0);
	TestFalse(TEXT("No Night here"), Lighting->SetState(FName(TEXT("Night")), ELightingSwitch::Instant));
	TestLightsShow(*this, TEXT("After asking for Night"), Lights, Dusk);
	TestTrue(TEXT("...still Dusk"), Lighting->GetState() == DuskName);
	TestEqual(TEXT("...and nobody told"), Heard.Num(), 1);

	// Asking for the state in place (in any case) does nothing.
	TestTrue(TEXT("Asking for dusk again is fine"), Lighting->SetState(FName(TEXT("dusk"))));
	TestEqual(TEXT("...and changes nothing"), Heard.Num(), 1);

	// Back to Day behind a fade, which with nobody to fade for switches at once: the level as built.
	TestTrue(TEXT("Day switched back to"), Lighting->SetState(FName(TEXT("day")), ELightingSwitch::Fade));
	TestLightsShow(*this, TEXT("Day again"), Lights, Day);
	TestTrue(TEXT("Day in place"), Lighting->GetState() == ALightingStates::DayState && !Lighting->IsSwitching());
	TestEqual(TEXT("Heard twice"), Heard.Num(), 2);

	// Blended over 2 s: halfway the light is between, nobody told yet; at the end, Dusk exactly, told once.
	TestTrue(TEXT("A blend to Dusk starts"), Lighting->SetState(DuskName, ELightingSwitch::Blend, 2.f));
	TestTrue(TEXT("...heading for Dusk"), Lighting->IsSwitching() && Lighting->GetPendingState() == DuskName);
	Lighting->Update(1.f);
	FLightingState Between;
	Lights.Read(Between);
	TestTrue(FString::Printf(TEXT("Halfway the sun's light is between (%.2f lux)"), Between.SunIntensity),
		Between.SunIntensity < Day.SunIntensity - 0.1f && Between.SunIntensity > Dusk.SunIntensity + 0.1f);
	TestTrue(FString::Printf(TEXT("...and so is its height (%.1f deg)"), Between.SunElevation),
		Between.SunElevation < Day.SunElevation - 1.f && Between.SunElevation > Dusk.SunElevation + 1.f);
	TestTrue(TEXT("Still Day until it lands"), Lighting->GetState() == ALightingStates::DayState);
	TestEqual(TEXT("...and nobody told yet"), Heard.Num(), 2);
	Lighting->Update(1.5f);
	TestLightsShow(*this, TEXT("Blended to dusk"), Lights, Dusk);
	TestTrue(TEXT("Dusk in place after the blend"), Lighting->GetState() == DuskName && !Lighting->IsSwitching());
	TestTrue(TEXT("Told at the end"), Heard.Num() == 3 && Heard[2].How == ELightingSwitch::Blend);

	// The collection carries Dusk's tints and fog colors to the materials that can't see the light, and Day's back.
	UMaterialParameterCollection* Collection = States->Collection.LoadSynchronous();
	if (TestNotNull(TEXT("MPC_Lighting exists (Tools/Unreal/build_world_materials.py M_Backdrop makes it)"), Collection))
	{
		TestTrue(TEXT("The backdrop's tint at dusk"), CollectionValue(World, Collection, ULightingStateSubsystem::BackdropTintParameter).Equals(Dusk.BackdropTint, 1e-4f));
		TestTrue(TEXT("The clouds' tint at dusk"), CollectionValue(World, Collection, ULightingStateSubsystem::CloudTintParameter).Equals(Dusk.CloudTint, 1e-4f));
		TestTrue(TEXT("The haze at dusk"), CollectionValue(World, Collection, ULightingStateSubsystem::FogInscatteringParameter).Equals(Dusk.FogInscattering, 1e-4f));
		TestTrue(TEXT("The glow at dusk"), CollectionValue(World, Collection, ULightingStateSubsystem::FogDirectionalParameter).Equals(Dusk.FogDirectionalInscattering, 1e-4f));
		Lighting->SetState(ALightingStates::DayState, ELightingSwitch::Instant);
		TestTrue(TEXT("By day, the backdrop as built"), CollectionValue(World, Collection, ULightingStateSubsystem::BackdropTintParameter).Equals(FLinearColor::White, 1e-4f));
		TestTrue(TEXT("...and the clouds"), CollectionValue(World, Collection, ULightingStateSubsystem::CloudTintParameter).Equals(FLinearColor::White, 1e-4f));
	}
	Lighting->OnChanged.Remove(Listening);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLightingFadeTest, "Looter.World.Lighting.Fade",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FLightingFadeTest::RunTest(const FString& Parameters)
{
	// Behind the fade (the console's way): the light stays as it was while the screen goes black, changes only once it's
	// black, holds black a couple of frames for the sky light's capture, then fades back in. Never seen changing.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	ULightingStateSubsystem* Lighting = ULightingStateSubsystem::Get(World);
	const FLightingState Day;
	const FLightingState Dusk = MakeTestDusk();
	ALightingStates* States = SpawnLitTestLevel(World, { Day, Dusk });
	APlayerController* Player = World->SpawnActor<APlayerController>();
	if (Player && !Player->PlayerCameraManager)
	{
		// A test world never readies its actors (no PostInitializeComponents): the controller joins the world's list and
		// takes its camera as play would, and counts as this machine's player (with no net driver, UE asks for a local
		// player otherwise), so the subsystem finds a camera to fade.
		World->AddController(Player);
		Player->SpawnPlayerCameraManager();
		Player->SetAsLocalPlayerController();
	}
	APlayerCameraManager* Camera = Player ? Player->PlayerCameraManager.Get() : nullptr;
	if (!TestNotNull(TEXT("Lighting"), Lighting) || !TestNotNull(TEXT("A lit level"), States) || !TestNotNull(TEXT("A player's camera to fade"), Camera))
	{
		return false;
	}
	// Only the fade matters here: no collection, so the test doesn't need the material build's.
	States->Collection.Reset();
	const FLightingTargets Lights = FLightingTargets::Find(nullptr, States);

	TestTrue(TEXT("Dusk asked for"), Lighting->SetState(DuskName, ELightingSwitch::Fade));
	TestTrue(TEXT("The screen starts fading to black, to hold there"), Camera->bEnableFading && FMath::IsNearlyEqual(Camera->FadeAlpha.Y, 1.0));
	TestTrue(TEXT("Switching, toward Dusk"), Lighting->IsSwitching() && Lighting->GetPendingState() == DuskName);
	Lighting->Update(Lighting->FadeOutSeconds * 0.5f);
	TestLightsShow(*this, TEXT("Halfway to black"), Lights, Day);
	TestTrue(TEXT("...still Day"), Lighting->GetState() == ALightingStates::DayState);

	Lighting->Update(Lighting->FadeOutSeconds * 0.6f);
	TestTrue(TEXT("Once black, Dusk is in place"), Lighting->GetState() == DuskName);
	TestLightsShow(*this, TEXT("Behind the black"), Lights, Dusk);
	TestTrue(TEXT("...held black"), Camera->bEnableFading && FMath::IsNearlyEqual(Camera->FadeAmount, 1.f));
	TestTrue(TEXT("...a while longer"), Lighting->IsSwitching());

	for (int32 Frame = 0; Frame < 4 && Lighting->IsSwitching(); ++Frame)
	{
		Lighting->Update(1.f / 60.f);
	}
	TestFalse(TEXT("Done after a couple of frames"), Lighting->IsSwitching());
	TestTrue(TEXT("The screen fades back in from black"), Camera->bEnableFading && FMath::IsNearlyEqual(Camera->FadeAlpha.X, 1.0) && FMath::IsNearlyEqual(Camera->FadeAlpha.Y, 0.0));
	TestEqual(TEXT("...over the fade-in's time"), Camera->FadeTimeRemaining, Lighting->FadeInSeconds, 1e-4f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLightingLevelsTest, "Looter.World.Lighting.Levels",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FLightingLevelsTest::RunTest(const FString& Parameters)
{
	// Each built level is lit as its Day: its lights show the state it starts in, so going back to Day after Dusk brings
	// back the level as built. The tutorial island has no states block, so its Day is a default state (the build
	// script's DEFAULTS), which is how it keeps its light.
	struct FBuiltLevel
	{
		const TCHAR* Map;
		bool bHasDusk;
	};
	const FBuiltLevel Built[] = {
		{ TEXT("/Game/Maps/Lvl_TutorialIsland"), false },
		{ TEXT("/Game/Maps/Dev/Lvl_TerrainTest"), true },
		{ TEXT("/Game/Maps/Lvl_RansomsRest"), true },
	};
	for (const FBuiltLevel& Each : Built)
	{
		const FString MapPackage(Each.Map);
		const FString Name = FPackageName::GetShortName(MapPackage);
		if (!FPackageName::DoesPackageExist(MapPackage))
		{
			AddInfo(FString::Printf(TEXT("%s isn't built: skipped."), *Name));
			continue;
		}
		const UWorld* Map = LoadObject<UWorld>(nullptr, *FString::Printf(TEXT("%s.%s"), *MapPackage, *Name));
		if (!TestNotNull(Name + TEXT(" loads"), Map) || !TestNotNull(Name + TEXT(" has a level"), Map->PersistentLevel.Get()))
		{
			continue;
		}
		const ALightingStates* States = nullptr;
		for (const AActor* Actor : Map->PersistentLevel->Actors)
		{
			States = States ? States : Cast<ALightingStates>(Actor);
		}
		if (!States && Each.bHasDusk)
		{
			AddWarning(FString::Printf(TEXT("%s has no lighting states yet: build it again (Tools/Unreal/build_area.py %s)."), *Name,
				*Name.RightChop(4)));
			continue;
		}
		const FLightingState Day = States && States->GetInitialState() ? *States->GetInitialState() : FLightingState();
		const FLightingTargets Lights = FLightingTargets::Find(Map->PersistentLevel.Get(), States);
		TestTrue(FString::Printf(TEXT("%s's lights are all there (missing: %s)"), *Name, *Lights.DescribeMissing()), Lights.DescribeMissing().IsEmpty());
		TestLightsShow(*this, Name + TEXT(" as built"), Lights, Day);
		if (!States)
		{
			continue;
		}
		TestTrue(Name + TEXT(" starts in Day"), Day.Name == ALightingStates::DayState);
		const TArray<FName> Names = States->GetStateNames();
		TSet<FName> Unique;
		Unique.Append(Names);
		TestEqual(Name + TEXT("'s states have names of their own"), Unique.Num(), Names.Num());
		if (Each.bHasDusk)
		{
			const FLightingState* Dusk = States->FindState(DuskName);
			if (TestNotNull(Name + TEXT(" has a Dusk"), Dusk))
			{
				TestTrue(FString::Printf(TEXT("%s's sun stands lower at dusk (%.1f deg against %.1f)"), *Name, Dusk->SunElevation, Day.SunElevation),
					Dusk->SunElevation < Day.SunElevation);
				TestFalse(Name + TEXT("'s backdrop is darker at dusk"), Dusk->BackdropTint.GetLuminance() >= Day.BackdropTint.GetLuminance());
			}
		}
		else
		{
			// A layout without states gets a Day equal to today's environment.
			TestStatesMatch(*this, Name + TEXT("'s Day against a default state"), Day, FLightingState());
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLightingBackdropTest, "Looter.World.Lighting.Backdrop",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FLightingBackdropTest::RunTest(const FString& Parameters)
{
	// The backdrop's silhouettes are unlit and never see the sun go down: M_Backdrop multiplies each layer's tint by the
	// state's from MPC_Lighting, which starts white (the backdrop as built) for every tint a state writes.
	UMaterialParameterCollection* Collection = LoadObject<UMaterialParameterCollection>(nullptr, ALightingStates::DefaultCollectionPath);
	if (!TestNotNull(TEXT("MPC_Lighting exists (Tools/Unreal/build_world_materials.py M_Backdrop makes it)"), Collection))
	{
		return false;
	}
	for (const FName Parameter : { ULightingStateSubsystem::BackdropTintParameter, ULightingStateSubsystem::CloudTintParameter,
		ULightingStateSubsystem::FogInscatteringParameter, ULightingStateSubsystem::FogDirectionalParameter })
	{
		bool bFound = false;
		const FLinearColor Default = Collection->GetVectorParameterDefaultValue(Parameter, bFound);
		TestTrue(FString::Printf(TEXT("MPC_Lighting has %s"), *Parameter.ToString()), bFound);
		if (Parameter == ULightingStateSubsystem::BackdropTintParameter || Parameter == ULightingStateSubsystem::CloudTintParameter)
		{
			TestTrue(FString::Printf(TEXT("...white to start (%s)"), *Default.ToString()), Default.Equals(FLinearColor::White, 1e-4f));
		}
	}

#if WITH_EDITORONLY_DATA
	const UMaterial* Backdrop = LoadObject<UMaterial>(nullptr, TEXT("/Game/Art/Materials/Masters/M_Backdrop.M_Backdrop"));
	if (TestNotNull(TEXT("M_Backdrop exists"), Backdrop))
	{
		bool bReadsTint = false;
		for (const TObjectPtr<UMaterialExpression>& Expression : Backdrop->GetExpressions())
		{
			const UMaterialExpressionCollectionParameter* Reads = Cast<UMaterialExpressionCollectionParameter>(Expression.Get());
			// Found by its id as well as its name: an id that doesn't match the collection's leaves the material unbuilt.
			bReadsTint |= Reads && Reads->Collection == Collection && Reads->ParameterName == ULightingStateSubsystem::BackdropTintParameter
				&& Reads->ParameterId.IsValid() && Reads->ParameterId == Collection->GetParameterId(ULightingStateSubsystem::BackdropTintParameter);
		}
		TestTrue(TEXT("M_Backdrop reads BackdropTint from MPC_Lighting"), bReadsTint);
	}
#endif
	return true;
}

#endif

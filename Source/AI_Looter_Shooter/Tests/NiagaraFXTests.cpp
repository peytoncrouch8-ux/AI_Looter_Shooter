#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Combat/GraveSaltCinderBed.h"
#include "Combat/GrenadeKindsTuning.h"
#include "Tests/LocomotionTestWorld.h"
#include "Weapons/NiagaraFX.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Misc/ScopeExit.h"
#include "Tests/AutomationCommon.h"

// The Niagara effects (round 5): Looter.Fx.Niagara picks the look, a system not built yet falls back to the code's, and
// the cinder bed draws its fire one way or the other for its whole life and burns out either way.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNiagaraFXSwitchTest, "Looter.Fx.Niagara",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FNiagaraFXSwitchTest::RunTest(const FString& Parameters)
{
	IConsoleVariable* Switch = IConsoleManager::Get().FindConsoleVariable(TEXT("Looter.Fx.Niagara"));
	if (!TestNotNull(TEXT("Looter.Fx.Niagara exists"), Switch))
	{
		return false;
	}
	const bool bWas = Switch->GetBool();
	ON_SCOPE_EXIT
	{
		Switch->Set(bWas, ECVF_SetByCode);
	};
	TestTrue(TEXT("Every effect has its path"), FString(NiagaraFX::PathOf(ENiagaraFX::CinderBed)).StartsWith(TEXT("/Game/Art/Effects/Niagara/"))
		&& FString(NiagaraFX::PathOf(ENiagaraFX::CinderBurst)).StartsWith(TEXT("/Game/Art/Effects/Niagara/")));

	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	LocomotionTestWorld::SpawnBlock(World, FVector(0.0, 0.0, -50.0), FVector(3000.0, 3000.0, 100.0));
	const FGrenadeKindTuning Kinds;
	constexpr float Frame = 1.f / 60.f;

	// Off: the code's look, whatever is built.
	Switch->Set(false, ECVF_SetByCode);
	TestFalse(TEXT("Off: no Niagara bed"), NiagaraFX::UseFor(ENiagaraFX::CinderBed));
	TestFalse(TEXT("Off: no Niagara burst"), NiagaraFX::UseFor(ENiagaraFX::CinderBurst));
	TestNull(TEXT("Off: nothing spawned"), NiagaraFX::SpawnAt(World, ENiagaraFX::CinderBurst, FVector::ZeroVector, FVector::UpVector, 1.f));
	AGraveSaltCinderBed* Coded = AGraveSaltCinderBed::Spawn(World, FVector(0.0, 0.0, 100.0), nullptr, 1.f);
	if (TestNotNull(TEXT("A bed"), Coded))
	{
		Coded->Step(Frame);
		TestFalse(TEXT("Off: the bed's fire is the code's"), Coded->IsDrawnByNiagara());
		Coded->Destroy();
	}

	// On: Niagara where it's built; the code's look where it isn't (a build before the script's run plays as before).
	Switch->Set(true, ECVF_SetByCode);
	const bool bBuilt = NiagaraFX::Find(ENiagaraFX::CinderBed) != nullptr;
	if (!bBuilt)
	{
		AddInfo(TEXT("NS_CinderBed isn't built yet (Tools/Unreal/build_niagara_fx.py): checking the fallback only"));
	}
	TestEqual(TEXT("On: the bed's Niagara look exactly when it's built"), NiagaraFX::UseFor(ENiagaraFX::CinderBed), bBuilt);
	AGraveSaltCinderBed* Bed = AGraveSaltCinderBed::Spawn(World, FVector(0.0, 0.0, 100.0), nullptr, 1.f);
	if (!TestNotNull(TEXT("A bed"), Bed))
	{
		return false;
	}
	Bed->Step(Frame);
	TestEqual(TEXT("On: the bed's fire is Niagara's when built, the code's when not"), Bed->IsDrawnByNiagara(), bBuilt);

	// Flipped mid-burn: the bed keeps the look it began with, through its dying down, and burns out on time.
	Switch->Set(!bBuilt, ECVF_SetByCode);
	for (float Time = 0.f; Time < Kinds.BedSeconds + 0.2f && IsValid(Bed) && !Bed->IsActorBeingDestroyed(); Time += Frame)
	{
		TestEqual(TEXT("...the same look all its life"), Bed->IsDrawnByNiagara(), bBuilt);
		Bed->Step(Frame);
	}
	TestTrue(TEXT("Burnt out and gone after its seconds"), !IsValid(Bed) || Bed->IsActorBeingDestroyed());
	return true;
}

#endif

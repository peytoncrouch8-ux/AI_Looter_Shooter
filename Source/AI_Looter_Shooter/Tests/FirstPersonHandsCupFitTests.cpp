#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Affixes/WeaponRollLibrary.h"
#include "Player/FirstPersonHandsGrips.h"
#include "Weapons/WeaponDefinition.h"
#include "Weapons/WeaponModelComponent.h"
#include "Weapons/WeaponParts.h"
#include "Weapons/WeaponTypes.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Tests/AutomationCommon.h"

// The support hand's cup fitted to the gun's own parts (FirstPersonHandsCupFit.cpp, 2026-10-28): every body and every
// magazine of every two-handed gun has an underside under the cup's spot (the AR's Marksman body, with no thumbhole loop,
// left the hand open in the air on the table's spot), and the hand moved onto it rests on it.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFirstPersonHandsCupFitTest, "Looter.Player.Hands.CupFit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFirstPersonHandsCupFitTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	AActor* Holder = TestLevel.GetTestWorld()->SpawnActor<AActor>();
	UWeaponModelComponent* Model = Holder ? NewObject<UWeaponModelComponent>(Holder) : nullptr;
	if (!TestNotNull(TEXT("A gun's model"), Model))
	{
		return false;
	}
	Holder->SetRootComponent(Model);
	Model->RegisterComponent();

	const TCHAR* const Guns[] = {
		TEXT("/Game/Weapons/Data/DA_AssaultRifle.DA_AssaultRifle"),
		TEXT("/Game/Weapons/Data/DA_PumpShotgun.DA_PumpShotgun"),
		TEXT("/Game/Weapons/Data/DA_SMG.DA_SMG"),
		TEXT("/Game/Weapons/Data/DA_Sniper.DA_Sniper"),
		TEXT("/Game/Weapons/Data/DA_LMG.DA_LMG"),
		TEXT("/Game/Weapons/Data/DA_Launcher.DA_Launcher"),
	};
	for (const TCHAR* Path : Guns)
	{
		UWeaponDefinition* Definition = LoadObject<UWeaponDefinition>(nullptr, Path);
		if (!TestNotNull(Path, Definition))
		{
			continue;
		}
		const FGunHands& Table = FirstPersonHandsGrips::For(Definition->Kind);
		const float TableBottom = static_cast<float>(Table.CupShape.Offset.Z) - Table.CupShape.HalfAcross;
		for (int32 Slot = 0; Slot < Definition->Parts.Num(); ++Slot)
		{
			const FName SlotName = Definition->Parts[Slot].Name;
			if (SlotName != TEXT("Body") && SlotName != TEXT("Magazine"))
			{
				continue;
			}
			for (const FWeaponPartOption& Option : Definition->Parts[Slot].Options)
			{
				FWeaponInstanceData Instance = UWeaponRollLibrary::RollWeaponWithRarity(Definition, EWeaponRarity::Rare, 1);
				Instance.Seed = 11;
				if (!Instance.Parts.IsValidIndex(Slot))
				{
					continue;
				}
				Instance.Parts[Slot] = Option.Key;
				const FString Name = FString::Printf(TEXT("%s %s=%s"), *Definition->GetName(), *SlotName.ToString(), *Option.Key.ToString());
				if (!TestTrue(Name + TEXT(" assembles"), Model->Assemble(Instance)))
				{
					continue;
				}
				const FHandsCupFit Fit = FirstPersonHandsGrips::FitCup(*Model, Definition->Kind);
				AddInfo(FString::Printf(TEXT("%s: underside %s (the table's %.2f), half width %.2f"), *Name, *Fit.Under.ToCompactString(), TableBottom, Fit.HalfWidth));
				if (!TestTrue(Name + TEXT(": an underside under the cup"), Fit.bFitted))
				{
					continue;
				}
				// The hand moved with its box: still the table's height over the box's bottom, the box's bottom on the underside.
				FGunHands Fitted = Table;
				FirstPersonHandsGrips::FitCupTo(Fitted, Fit);
				const float Bottom = static_cast<float>(Fitted.CupShape.Offset.Z) - Fitted.CupShape.HalfAcross;
				TestTrue(Name + TEXT(": the box's bottom on the underside"), FMath::IsNearlyEqual(Bottom, static_cast<float>(Fit.Under.Z), 0.01f));
				TestTrue(Name + TEXT(": the hand over it as in the table"), FMath::IsNearlyEqual(static_cast<float>(Fitted.CupHold.Z - Bottom),
					static_cast<float>(Table.CupHold.Z - TableBottom), 0.01f));
			}
		}
	}
	Holder->Destroy();
	return true;
}

#endif

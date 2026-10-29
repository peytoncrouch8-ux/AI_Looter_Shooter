#include "Misc/AutomationTest.h"
#include "Affixes/WeaponRollLibrary.h"
#include "Elements/ElementRules.h"
#include "Tests/LocomotionTestWorld.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponDefinition.h"
#include "Weapons/WeaponModelComponent.h"
#include "Weapons/WeaponParts.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"

// An SMG with an element helix lights its round count in that element's hue (the art's orange drowned the Soul-light
// part, 2026-10-22); a plain helix keeps the art's orange. Looter.Weapons.NewGuns.HelixColor.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNewGunsHelixColorTest, "Looter.Weapons.NewGuns.HelixColor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FNewGunsHelixColorTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	UWeaponDefinition* Definition = LoadObject<UWeaponDefinition>(nullptr, TEXT("/Game/Weapons/Data/DA_SMG.DA_SMG"));
	if (!TestNotNull(TEXT("The SMG's definition"), Definition))
	{
		return false;
	}
	int32 MagazineSlot = INDEX_NONE;
	for (int32 Index = 0; Index < Definition->Parts.Num(); ++Index)
	{
		if (Definition->Parts[Index].Name == FName(TEXT("Magazine")))
		{
			MagazineSlot = Index;
		}
	}
	if (!TestTrue(TEXT("It has a magazine slot"), MagazineSlot != INDEX_NONE))
	{
		return false;
	}
	struct FCase
	{
		const TCHAR* Key;
		ELooterElement Element;
	};
	const FCase Cases[] = { { TEXT("40"), ELooterElement::None }, { TEXT("Ember40"), ELooterElement::Ember },
		{ TEXT("Saint40"), ELooterElement::SoulLight }, { TEXT("Salt40"), ELooterElement::GraveSalt }, { TEXT("Thunder40"), ELooterElement::Thunder },
		{ TEXT("Rot40"), ELooterElement::Rot } };
	for (const FCase& Case : Cases)
	{
		FWeaponInstanceData Gun = UWeaponRollLibrary::RollWeaponWithRarity(Definition, EWeaponRarity::Rare, 1);
		TArray<FName> Keys = WeaponParts::PartKeys(WeaponParts::Pick(Gun));
		if (!TestTrue(TEXT("A key for every slot"), Keys.IsValidIndex(MagazineSlot)))
		{
			continue;
		}
		Keys[MagazineSlot] = Case.Key;
		Gun.Parts = Keys;
		AWeaponBase* Weapon = UWeaponRollLibrary::SpawnWeapon(World, Gun, FTransform::Identity);
		UWeaponModelComponent* Model = Weapon ? Weapon->FindComponentByClass<UWeaponModelComponent>() : nullptr;
		if (TestTrue(*FString::Printf(TEXT("%s built with its helix glow"), Case.Key), Model && Model->HasAmmoGauge()))
		{
			const FLinearColor Color = Model->GetAmmoGaugeColor();
			if (Case.Element == ELooterElement::None)
			{
				// The art's ember orange: red-led, little blue.
				TestTrue(*FString::Printf(TEXT("%s keeps the art's orange (%s)"), Case.Key, *Color.ToString()), Color.R > Color.G && Color.G > Color.B);
			}
			else
			{
				TestTrue(*FString::Printf(TEXT("%s glows in its element's hue (%s)"), Case.Key, *Color.ToString()),
					Color.Equals(ElementRules::FxColor(Case.Element), 0.01f));
			}
		}
		if (Weapon)
		{
			Weapon->Destroy();
		}
	}
	return true;
}

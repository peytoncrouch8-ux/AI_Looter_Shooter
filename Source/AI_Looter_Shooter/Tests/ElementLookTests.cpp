#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Affixes/WeaponRollLibrary.h"
#include "Elements/ElementRules.h"
#include "Weapons/WeaponDefinition.h"
#include "Weapons/WeaponModelComponent.h"
#include "Weapons/WeaponParts.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "HAL/IConsoleManager.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Tests/AutomationCommon.h"

// The gun's element on its looks (the user's call, 2026-10-27, "Glow parts take element"): a gun with an element part glows in
// its element's colour everywhere but its rarity accent, and the element part wears its element's finish on its own mesh
// (scorched, gilt, frosted) through the gun master's custom primitive data. Looter.Weapons.ElementLook.*.

namespace
{
	const TCHAR* const ElementGuns[] = { TEXT("DA_AssaultRifle"), TEXT("DA_PumpShotgun"), TEXT("DA_Revolver"), TEXT("DA_SMG"),
		TEXT("DA_Sniper"), TEXT("DA_LMG"), TEXT("DA_Launcher") };
	const ELooterElement Elements[] = { ELooterElement::None, ELooterElement::Ember, ELooterElement::SoulLight, ELooterElement::GraveSalt };

	bool CVarOn(const TCHAR* Name)
	{
		const IConsoleVariable* Variable = IConsoleManager::Get().FindConsoleVariable(Name);
		return Variable && Variable->GetBool();
	}

	float DataAt(const UPrimitiveComponent& Part, int32 Index)
	{
		const TArray<float>& Data = Part.GetCustomPrimitiveData().Data;
		return Data.IsValidIndex(Index) ? Data[Index] : 0.f;
	}

	/** The gun's saved part keys with Element's first part fitted (no element part at all for None); false if it has none. */
	bool KeysWith(const FWeaponInstanceData& Gun, ELooterElement Element, TArray<FName>& OutKeys)
	{
		const UWeaponDefinition& Definition = *Gun.Definition;
		OutKeys = WeaponParts::PartKeys(WeaponParts::Pick(Gun));
		bool bFitted = Element == ELooterElement::None;
		for (int32 Slot = 0; Slot < Definition.Parts.Num() && OutKeys.IsValidIndex(Slot); ++Slot)
		{
			const TArray<FWeaponPartOption>& Options = Definition.Parts[Slot].Options;
			const FWeaponPartOption* Picked = Options.FindByPredicate([&](const FWeaponPartOption& Option) { return Option.Key == OutKeys[Slot]; });
			if (Picked && Picked->Element != ELooterElement::None)
			{
				const FWeaponPartOption* Plain = Options.FindByPredicate([](const FWeaponPartOption& Option) { return Option.Element == ELooterElement::None && Option.Mesh; });
				OutKeys[Slot] = Plain ? Plain->Key : NAME_None;
			}
			if (!bFitted)
			{
				if (const FWeaponPartOption* Wanted = Options.FindByPredicate([Element](const FWeaponPartOption& Option) { return Option.Element == Element && Option.Mesh; }))
				{
					OutKeys[Slot] = Wanted->Key;
					bFitted = true;
				}
			}
		}
		return bFitted;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponElementLookRulesTest, "Looter.Weapons.ElementLook.Rules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWeaponElementLookRulesTest::RunTest(const FString& Parameters)
{
	const FName Rarity(TEXT("GunAccentGlow"));
	// Every glow of the gun kit takes the element; the rarity accent (and a part's own copy of it), paints and textures don't.
	for (const TCHAR* Slot : { TEXT("GunEmberGlow"), TEXT("GunCyanGlow"), TEXT("GunAmmoGlow"), TEXT("GunHeatGlow"), TEXT("GunSoulGlow"), TEXT("GunSaltGlow") })
	{
		TestTrue(FString::Printf(TEXT("%s takes the element"), Slot), UWeaponModelComponent::IsElementGlowSlot(Slot, Rarity));
	}
	for (const TCHAR* Slot : { TEXT("GunAccentGlow"), TEXT("GunAccentGlow_TripoBullpupBodyStandard"), TEXT("GunPolymerSand"),
		TEXT("GunPolymerSand_TripoBullpupMagazine30"), TEXT("GunTripoBullpupMagazine30"), TEXT("GunSteelBlued"), TEXT("GunSaltCrystal") })
	{
		TestFalse(FString::Printf(TEXT("%s keeps its own colour"), Slot), UWeaponModelComponent::IsElementGlowSlot(Slot, Rarity));
	}
	TestEqual(TEXT("The reticle follows its cvar"), UWeaponModelComponent::IsElementGlowSlot(TEXT("GunReticle"), Rarity),
		CVarOn(TEXT("Looter.Weapons.ElementLook.Reticle")));

	// Each element its own finish; none, nothing.
	const FWeaponElementFinish None = UWeaponModelComponent::ElementFinishFor(ELooterElement::None);
	TestEqual(TEXT("No element, no finish"), None.Kind, 0.f);
	TestEqual(TEXT("No element, no amount"), None.Amount, 0.f);
	if (CVarOn(TEXT("Looter.Weapons.ElementLook.Finish")))
	{
		const FWeaponElementFinish Ember = UWeaponModelComponent::ElementFinishFor(ELooterElement::Ember);
		const FWeaponElementFinish Soul = UWeaponModelComponent::ElementFinishFor(ELooterElement::SoulLight);
		const FWeaponElementFinish Salt = UWeaponModelComponent::ElementFinishFor(ELooterElement::GraveSalt);
		TestEqual(TEXT("Ember is scorched"), Ember.Kind, UWeaponModelComponent::FinishScorched);
		TestEqual(TEXT("Soul-light is gilt"), Soul.Kind, UWeaponModelComponent::FinishGilt);
		TestEqual(TEXT("Grave-salt is frosted"), Salt.Kind, UWeaponModelComponent::FinishFrosted);
		for (const FWeaponElementFinish* Finish : { &Ember, &Soul, &Salt })
		{
			TestTrue(TEXT("A finish shows"), Finish->Amount > 0.f && Finish->Amount <= 1.f);
			TestTrue(TEXT("A finish glows"), Finish->GlowStrength > 0.f);
		}
		TestTrue(TEXT("Scorched glows in Ember's hue"), Ember.Glow.Equals(ElementRules::FxColor(ELooterElement::Ember)));
		TestTrue(TEXT("Gilt glows in Soul-light's hue"), Soul.Glow.Equals(ElementRules::FxColor(ELooterElement::SoulLight)));
		TestTrue(TEXT("Frosted glows in Grave-salt's hue"), Salt.Glow.Equals(ElementRules::FxColor(ELooterElement::GraveSalt)));
		TestTrue(TEXT("Scorched is the darkest"), Ember.Tint.GetLuminance() < Soul.Tint.GetLuminance() && Ember.Tint.GetLuminance() < Salt.Tint.GetLuminance());
		TestTrue(TEXT("Gilt is warm"), Soul.Tint.R > Soul.Tint.B * 2.f);
		TestTrue(TEXT("Frosted is cold and pale"), Salt.Tint.B >= Salt.Tint.R && Salt.Tint.GetLuminance() > 0.5f);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponElementLookGunsTest, "Looter.Weapons.ElementLook.Guns",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWeaponElementLookGunsTest::RunTest(const FString& Parameters)
{
	// Every family with each element part fitted, and with none: the glows in the element's hue (the art's without one), the
	// rarity accent in the rarity's colour, and the element part (alone) in its finish.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	AActor* Holder = WorldWrapper.GetTestWorld()->SpawnActor<AActor>();
	UWeaponModelComponent* Model = NewObject<UWeaponModelComponent>(Holder);
	Holder->SetRootComponent(Model);
	Model->RegisterComponent();
	const bool bGlows = UWeaponModelComponent::ElementGlowsOn();
	int32 Recoloured = 0;
	int32 Finished = 0;

	for (const TCHAR* Asset : ElementGuns)
	{
		FWeaponInstanceData Gun;
		Gun.Definition = LoadObject<UWeaponDefinition>(nullptr, *FString::Printf(TEXT("/Game/Weapons/Data/%s.%s"), Asset, Asset));
		Gun.Rarity = EWeaponRarity::Legendary;
		Gun.Seed = 11;
		if (!TestNotNull(Asset, Gun.Definition.Get()))
		{
			continue;
		}
		const FName RaritySlot = Gun.Definition->RarityGlowSlot;
		for (const ELooterElement Element : Elements)
		{
			const FString Case = FString::Printf(TEXT("%s %s"), Asset, *UEnum::GetValueAsString(Element));
			FWeaponInstanceData Fitted = Gun;
			Fitted.Parts.Reset();
			TArray<FName> Keys;
			if (!KeysWith(Fitted, Element, Keys))
			{
				AddInfo(FString::Printf(TEXT("%s: no such part"), *Case));
				continue;
			}
			Fitted.Parts = Keys;
			if (!TestTrue(FString::Printf(TEXT("%s assembles"), *Case), Model->Assemble(Fitted)))
			{
				continue;
			}
			TestEqual(FString::Printf(TEXT("%s's element"), *Case), WeaponParts::ElementOf(WeaponParts::Pick(Fitted)), Element);
			const FLinearColor Hue = ElementRules::FxColor(Element);
			for (const UStaticMeshComponent* Part : Model->GetParts())
			{
				const TArray<FName> Slots = Part->GetMaterialSlotNames();
				for (int32 Material = 0; Material < Slots.Num(); ++Material)
				{
					const UMaterialInstanceDynamic* Instanced = Cast<UMaterialInstanceDynamic>(Part->GetMaterial(Material));
					FLinearColor Color = FLinearColor::Black;
					const bool bColored = Instanced && Instanced->GetVectorParameterValue(FHashedMaterialParameterInfo(TEXT("Color")), Color);
					if (Slots[Material] == RaritySlot)
					{
						TestTrue(FString::Printf(TEXT("%s: the rarity accent keeps the rarity's colour"), *Case),
							bColored && Color.Equals(UWeaponRollLibrary::GetRarityColor(Fitted.Definition, Fitted.Rarity), 0.001f));
					}
					else if (UWeaponModelComponent::IsElementGlowSlot(Slots[Material], RaritySlot))
					{
						if (Element != ELooterElement::None && bGlows)
						{
							TestTrue(FString::Printf(TEXT("%s: %s glows in the element's hue (%s)"), *Case, *Slots[Material].ToString(), *Color.ToString()),
								bColored && Color.Equals(Hue, 0.01f));
							++Recoloured;
						}
						else if (Element == ELooterElement::None && Instanced && Slots[Material] != UWeaponModelComponent::AmmoGlowSlot
							&& Slots[Material] != UWeaponModelComponent::HeatGlowSlot)
						{
							AddError(FString::Printf(TEXT("%s: %s was recoloured on a gun without an element"), *Case, *Slots[Material].ToString()));
						}
					}
				}
				// Only the element part wears a finish.
				const bool bElementPart = Part == Model->GetElementPart();
				const FWeaponElementFinish Finish = UWeaponModelComponent::ElementFinishFor(Element);
				if (bElementPart)
				{
					TestEqual(FString::Printf(TEXT("%s: the element part's finish"), *Case), DataAt(*Part, UWeaponModelComponent::ElementFinishDataIndex), Finish.Kind);
					TestTrue(FString::Printf(TEXT("%s: its finish's amount"), *Case),
						FMath::IsNearlyEqual(DataAt(*Part, UWeaponModelComponent::ElementTintDataIndex + 3), Finish.Amount));
					TestTrue(FString::Printf(TEXT("%s: its finish's glow"), *Case),
						FMath::IsNearlyEqual(DataAt(*Part, UWeaponModelComponent::ElementGlowDataIndex + 3), Finish.GlowStrength));
					Finished += Finish.Kind > 0.f ? 1 : 0;
				}
				else
				{
					TestEqual(FString::Printf(TEXT("%s: other parts as modeled"), *Case), DataAt(*Part, UWeaponModelComponent::ElementFinishDataIndex), 0.f);
				}
			}
			TestEqual(FString::Printf(TEXT("%s: an element part exactly when it has an element"), *Case),
				Model->GetElementPart() != nullptr, Element != ELooterElement::None);
		}
	}
	if (bGlows)
	{
		TestTrue(TEXT("Some glows took an element"), Recoloured > 0);
	}
	if (CVarOn(TEXT("Looter.Weapons.ElementLook.Finish")))
	{
		TestTrue(TEXT("Some parts wore a finish"), Finished > 0);
	}
	Model->Clear();
	return true;
}

#endif

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Affixes/WeaponRollLibrary.h"
#include "Weapons/WeaponDefinition.h"
#include "Weapons/WeaponModelComponent.h"
#include "Weapons/WeaponParts.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Tests/AutomationCommon.h"

namespace
{
	const UWeaponDefinition* LoadGun(const TCHAR* Name)
	{
		return LoadObject<UWeaponDefinition>(nullptr, *FString::Printf(TEXT("/Game/Weapons/Data/%s.%s"), Name, Name));
	}

	int32 SlotIndex(const UWeaponDefinition& Definition, FName Slot)
	{
		return Definition.Parts.IndexOfByPredicate([Slot](const FWeaponPartSlot& Part) { return Part.Name == Slot; });
	}

	const UStaticMesh* PickedMesh(const FWeaponLook& Look, int32 Slot)
	{
		return Look.Parts.IsValidIndex(Slot) && Look.Parts[Slot] ? Look.Parts[Slot]->Mesh.Get() : nullptr;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponPartPicksTest, "Looter.Weapons.Parts.Picks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWeaponPartPicksTest::RunTest(const FString& Parameters)
{
	// Each gun is made of parts its seed picks: the same seed always makes the same gun, every option turns up, and
	// rarity parts (the rifle's fins, the shotgun's shroud) come only with their rarity.
	struct FCase
	{
		const TCHAR* Asset;
		const TCHAR* RaritySlot;
		EWeaponRarity RaritySlotFrom;
	};
	const FCase Cases[] = { { TEXT("DA_AssaultRifle"), TEXT("Fins"), EWeaponRarity::Legendary }, { TEXT("DA_PumpShotgun"), TEXT("Shroud"), EWeaponRarity::Epic } };
	const EWeaponRarity Rarities[] = { EWeaponRarity::Common, EWeaponRarity::Uncommon, EWeaponRarity::Rare, EWeaponRarity::Epic, EWeaponRarity::Legendary };
	for (const FCase& Case : Cases)
	{
		const UWeaponDefinition* Definition = LoadGun(Case.Asset);
		if (!TestNotNull(Case.Asset, Definition))
		{
			continue;
		}
		TestTrue(FString::Printf(TEXT("%s has parts"), Case.Asset), Definition->Parts.Num() > 1);
		TestTrue(FString::Printf(TEXT("%s knows its kind"), Case.Asset), Definition->Kind != EWeaponKind::None);
		TestTrue(FString::Printf(TEXT("%s's reload part is one of its slots"), Case.Asset), SlotIndex(*Definition, Definition->ReloadSlot) != INDEX_NONE);
		const int32 RaritySlot = SlotIndex(*Definition, Case.RaritySlot);
		TestTrue(FString::Printf(TEXT("%s has its rarity slot"), Case.Asset), RaritySlot != INDEX_NONE);

		TSet<const UStaticMesh*> Seen;
		for (int32 Seed = 0; Seed < 300; ++Seed)
		{
			const FWeaponLook Common = WeaponParts::Pick(*Definition, Seed, EWeaponRarity::Common);
			for (const EWeaponRarity Rarity : Rarities)
			{
				const FWeaponLook Look = WeaponParts::Pick(*Definition, Seed, Rarity);
				const FWeaponLook Again = WeaponParts::Pick(*Definition, Seed, Rarity);
				if (!TestEqual(TEXT("One pick per slot"), Look.Parts.Num(), Definition->Parts.Num()))
				{
					return false;
				}
				for (int32 Slot = 0; Slot < Look.Parts.Num(); ++Slot)
				{
					const UStaticMesh* Mesh = PickedMesh(Look, Slot);
					Seen.Add(Mesh);
					if (PickedMesh(Again, Slot) != Mesh)
					{
						AddError(FString::Printf(TEXT("%s seed %d: slot %d picked differently twice"), Case.Asset, Seed, Slot));
					}
					if (Look.Parts[Slot] && Look.Parts[Slot]->MinRarity > Rarity)
					{
						AddError(FString::Printf(TEXT("%s seed %d: a part above its rarity"), Case.Asset, Seed));
					}
					// Rarity changes only the rarity parts.
					if (Slot != RaritySlot && PickedMesh(Common, Slot) != Mesh)
					{
						AddError(FString::Printf(TEXT("%s seed %d: slot %d changed with rarity"), Case.Asset, Seed, Slot));
					}
					if (Slot == RaritySlot && (Mesh != nullptr) != (Rarity >= Case.RaritySlotFrom))
					{
						AddError(FString::Printf(TEXT("%s seed %d: %s at the wrong rarity"), Case.Asset, Seed, Case.RaritySlot));
					}
					if (Slot != RaritySlot && !Mesh)
					{
						AddError(FString::Printf(TEXT("%s seed %d: slot %d left empty"), Case.Asset, Seed, Slot));
					}
				}
				TestTrue(TEXT("Every gun is painted"), Look.Colors.Contains(TEXT("GunPaint")));
			}
		}
		for (const FWeaponPartSlot& Slot : Definition->Parts)
		{
			for (const FWeaponPartOption& Option : Slot.Options)
			{
				TestTrue(FString::Printf(TEXT("%s: %s turns up"), Case.Asset, *GetNameSafe(Option.Mesh)), Seen.Contains(Option.Mesh.Get()));
			}
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponPartAssemblyTest, "Looter.Weapons.Parts.Assembly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWeaponPartAssemblyTest::RunTest(const FString& Parameters)
{
	// An assembled gun: its parts in place, the muzzle and hands where they go, its reload part moving the way the
	// reload does (the rifle's magazine out along its tilt, the shotgun's pump back), and its own paint and rarity glow.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	AActor* Holder = WorldWrapper.GetTestWorld()->SpawnActor<AActor>();
	UWeaponModelComponent* Model = NewObject<UWeaponModelComponent>(Holder);
	Holder->SetRootComponent(Model);
	Model->RegisterComponent();

	struct FCase
	{
		const TCHAR* Asset;
		EWeaponRarity Rarity;
		EWeaponReloadPart ReloadPart;
		FVector ReloadWay;
	};
	const FCase Cases[] = {
		{ TEXT("DA_AssaultRifle"), EWeaponRarity::Legendary, EWeaponReloadPart::Magazine, FRotator(10.f, 0.f, 0.f).RotateVector(FVector(0.f, 0.f, -1.f)) },
		{ TEXT("DA_PumpShotgun"), EWeaponRarity::Epic, EWeaponReloadPart::Pump, FVector(-1.f, 0.f, 0.f) } };
	for (const FCase& Case : Cases)
	{
		FWeaponInstanceData Instance;
		Instance.Definition = const_cast<UWeaponDefinition*>(LoadGun(Case.Asset));
		Instance.Rarity = Case.Rarity;
		Instance.Seed = 7;
		if (!TestNotNull(Case.Asset, Instance.Definition.Get()) || !TestTrue(FString::Printf(TEXT("%s assembles"), Case.Asset), Model->Assemble(Instance)))
		{
			continue;
		}
		// Every slot is filled at these rarities.
		TestEqual(FString::Printf(TEXT("%s parts"), Case.Asset), Model->GetParts().Num(), Instance.Definition->Parts.Num());
		TestTrue(FString::Printf(TEXT("%s muzzle ahead (%s)"), Case.Asset, *Model->GetMuzzle().ToString()), Model->GetMuzzle().X > 50.0);
		TestTrue(FString::Printf(TEXT("%s grip under the receiver"), Case.Asset), !Model->GetGrip().IsZero() && Model->GetGrip().Z < 0.0);
		TestTrue(FString::Printf(TEXT("%s foregrip ahead of the grip"), Case.Asset), Model->GetForegrip().X > Model->GetGrip().X + 20.0);
		TestTrue(FString::Printf(TEXT("%s center"), Case.Asset), Model->GetCenter().X > 0.0 && Model->GetCenter().X < Model->GetMuzzle().X);

		TestEqual(FString::Printf(TEXT("%s reload part"), Case.Asset), Model->GetReloadPart(), Case.ReloadPart);
		const UStaticMeshComponent* Moving = Model->GetParts()[SlotIndex(*Instance.Definition, Instance.Definition->ReloadSlot)];
		const FVector Rest = Moving->GetComponentLocation();
		Model->SetReloadTravel(10.f, true);
		const FVector Moved = (Moving->GetComponentLocation() - Rest) / 10.0;
		TestTrue(FString::Printf(TEXT("%s reload part moves its way (%s)"), Case.Asset, *Moved.ToString()), Moved.Equals(Case.ReloadWay, 0.01));
		Model->SetReloadTravel(0.f, true);

		// One paint and one rarity glow for the whole gun.
		const FWeaponLook Look = WeaponParts::Pick(*Instance.Definition, Instance.Seed, Instance.Rarity);
		int32 Painted = 0;
		int32 Glowing = 0;
		for (const UStaticMeshComponent* Part : Model->GetParts())
		{
			const TArray<FName> Slots = Part->GetMaterialSlotNames();
			for (int32 Material = 0; Material < Slots.Num(); ++Material)
			{
				const UMaterialInstanceDynamic* Instanced = Cast<UMaterialInstanceDynamic>(Part->GetMaterial(Material));
				FLinearColor Color;
				if (Slots[Material] == TEXT("GunPaint") && TestNotNull(TEXT("Paint is this gun's"), Instanced))
				{
					Instanced->GetVectorParameterValue(FHashedMaterialParameterInfo(TEXT("Color")), Color);
					TestTrue(TEXT("Paint color"), Color.Equals(Look.Colors.FindRef(TEXT("GunPaint")), 0.001f));
					++Painted;
				}
				if (Slots[Material] == TEXT("GunAccent") && TestNotNull(TEXT("Glow is this gun's"), Instanced))
				{
					Instanced->GetVectorParameterValue(FHashedMaterialParameterInfo(TEXT("Color")), Color);
					TestTrue(TEXT("Glows in its rarity's color"), Color.Equals(UWeaponRollLibrary::GetRarityColor(Instance.Definition, Instance.Rarity), 0.001f));
					++Glowing;
				}
			}
		}
		TestTrue(FString::Printf(TEXT("%s is painted"), Case.Asset), Painted > 0);
		TestTrue(FString::Printf(TEXT("%s glows"), Case.Asset), Glowing > 0);
	}
	Model->Clear();
	TestFalse(TEXT("Cleared"), Model->IsAssembled());
	return true;
}

#endif

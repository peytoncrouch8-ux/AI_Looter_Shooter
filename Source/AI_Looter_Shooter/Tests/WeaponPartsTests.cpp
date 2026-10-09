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
#include "UI/Style/WeaponText.h"
#include "UObject/Package.h"

namespace
{
	const TCHAR* const Guns[] = { TEXT("DA_AssaultRifle"), TEXT("DA_PumpShotgun"), TEXT("DA_Revolver") };
	const EWeaponRarity Rarities[] = { EWeaponRarity::Common, EWeaponRarity::Uncommon, EWeaponRarity::Rare, EWeaponRarity::Epic, EWeaponRarity::Legendary };

	const UWeaponDefinition* LoadGun(const TCHAR* Name)
	{
		return LoadObject<UWeaponDefinition>(nullptr, *FString::Printf(TEXT("/Game/Weapons/Data/%s.%s"), Name, Name));
	}

	int32 SlotIndex(const UWeaponDefinition& Definition, FName Slot)
	{
		return Definition.Parts.IndexOfByPredicate([Slot](const FWeaponPartSlot& Part) { return Part.Name == Slot; });
	}

	FName PickedKey(const FWeaponLook& Look, int32 Slot)
	{
		return Look.Parts.IsValidIndex(Slot) && Look.Parts[Slot] ? Look.Parts[Slot]->Key : NAME_None;
	}

	/** A made-up gun for testing the rules with known numbers. */
	UWeaponDefinition* MakeGun()
	{
		return NewObject<UWeaponDefinition>(GetTransientPackage());
	}

	/** Any mesh will do: a part only needs one to be picked. */
	FWeaponPartOption& AddOption(UWeaponDefinition& Gun, FName Slot, FName Key)
	{
		static UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
		FWeaponPartSlot* Found = Gun.Parts.FindByPredicate([Slot](const FWeaponPartSlot& Part) { return Part.Name == Slot; });
		if (!Found)
		{
			Found = &Gun.Parts.AddDefaulted_GetRef();
			Found->Name = Slot;
		}
		FWeaponPartOption& Option = Found->Options.AddDefaulted_GetRef();
		Option.Key = Key;
		Option.Mesh = Cube;
		return Option;
	}

	FWeaponStatRange Percent(float Min, float Max)
	{
		FWeaponStatRange Range;
		Range.Min = Min;
		Range.Max = Max;
		return Range;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponPartPicksTest, "Looter.Weapons.Parts.Picks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWeaponPartPicksTest::RunTest(const FString& Parameters)
{
	// Each gun is made of parts its seed picks: the same seed always makes the same gun, every option turns up, a part
	// never comes below its rarity (rarity unlocks the better ones), every slot is filled, a part that needs a long
	// enough barrel only comes with one, and every paint gets a color.
	for (const TCHAR* Asset : Guns)
	{
		const UWeaponDefinition* Definition = LoadGun(Asset);
		if (!TestNotNull(Asset, Definition))
		{
			continue;
		}
		TestTrue(FString::Printf(TEXT("%s has parts"), Asset), Definition->Parts.Num() > 1);
		TestTrue(FString::Printf(TEXT("%s knows its kind"), Asset), Definition->Kind != EWeaponKind::None);
		TestTrue(FString::Printf(TEXT("%s's reload part is one of its slots"), Asset), SlotIndex(*Definition, Definition->ReloadSlot) != INDEX_NONE);
		TestTrue(FString::Printf(TEXT("%s has paints"), Asset), !Definition->Paints.IsEmpty());
		for (const FWeaponPartSlot& Slot : Definition->Parts)
		{
			TSet<FName> Keys;
			for (const FWeaponPartOption& Option : Slot.Options)
			{
				TestFalse(FString::Printf(TEXT("%s %s: every option has its own key"), Asset, *Slot.Name.ToString()), Option.Key.IsNone() || Keys.Contains(Option.Key));
				TestNotNull(FString::Printf(TEXT("%s %s %s has a mesh"), Asset, *Slot.Name.ToString(), *Option.Key.ToString()), Option.Mesh.Get());
				Keys.Add(Option.Key);
			}
		}

		TSet<const FWeaponPartOption*> Seen;
		for (int32 Seed = 0; Seed < 400; ++Seed)
		{
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
					const FWeaponPartOption* Part = Look.Parts[Slot];
					Seen.Add(Part);
					if (Again.Parts[Slot] != Part)
					{
						AddError(FString::Printf(TEXT("%s seed %d: slot %d picked differently twice"), Asset, Seed, Slot));
					}
					if (!Part)
					{
						AddError(FString::Printf(TEXT("%s seed %d: slot %d left empty"), Asset, Seed, Slot));
						continue;
					}
					if (Part->MinRarity > Rarity)
					{
						AddError(FString::Printf(TEXT("%s seed %d: %s above its rarity"), Asset, Seed, *Part->Key.ToString()));
					}
					const int32 Needed = Part->Requires.Slot.IsNone() ? INDEX_NONE : SlotIndex(*Definition, Part->Requires.Slot);
					if (!Part->Requires.Slot.IsNone() && (Needed == INDEX_NONE || Needed >= Slot || !Look.Parts[Needed]
						|| Look.Parts[Needed]->Length < Part->Requires.MinLength))
					{
						AddError(FString::Printf(TEXT("%s seed %d: %s without what it needs"), Asset, Seed, *Part->Key.ToString()));
					}
				}
				for (const FWeaponPaint& Paint : Definition->Paints)
				{
					TestTrue(FString::Printf(TEXT("%s is painted %s"), Asset, *Paint.Slot.ToString()), Look.Colors.Contains(Paint.Slot));
				}
			}
		}
		for (const FWeaponPartSlot& Slot : Definition->Parts)
		{
			for (const FWeaponPartOption& Option : Slot.Options)
			{
				TestTrue(FString::Printf(TEXT("%s: %s %s turns up"), Asset, *Slot.Name.ToString(), *Option.Key.ToString()), Seen.Contains(&Option));
			}
		}
	}

	// The rule itself: a long tube only on a long enough barrel, and it still turns up when one is picked.
	UWeaponDefinition* Gun = MakeGun();
	AddOption(*Gun, TEXT("Barrel"), TEXT("Short")).Length = 38.f;
	AddOption(*Gun, TEXT("Barrel"), TEXT("Long")).Length = 50.f;
	FWeaponPartOption& Tube = AddOption(*Gun, TEXT("Magazine"), TEXT("Tube8"));
	Tube.Requires.Slot = TEXT("Barrel");
	Tube.Requires.MinLength = 46.f;
	AddOption(*Gun, TEXT("Magazine"), TEXT("Tube6"));
	int32 LongTubes = 0;
	for (int32 Seed = 0; Seed < 300; ++Seed)
	{
		const FWeaponLook Look = WeaponParts::Pick(*Gun, Seed, EWeaponRarity::Common);
		if (PickedKey(Look, 1) == TEXT("Tube8"))
		{
			++LongTubes;
			TestEqual(TEXT("The long tube comes on the long barrel"), PickedKey(Look, 0), FName(TEXT("Long")));
		}
	}
	TestTrue(TEXT("The long tube turns up"), LongTubes > 30);
	Gun->MarkAsGarbage();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponPartSavedTest, "Looter.Weapons.Parts.Saved",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWeaponPartSavedTest::RunTest(const FString& Parameters)
{
	// A gun keeps the parts it dropped with: options added to the lists later don't change it, a saved part that's gone is
	// picked again, and a rolled gun saves one key per slot.
	for (const TCHAR* Asset : Guns)
	{
		const UWeaponDefinition* Definition = LoadGun(Asset);
		if (!TestNotNull(Asset, Definition))
		{
			continue;
		}
		const FWeaponInstanceData Rolled = UWeaponRollLibrary::RollWeaponWithRarity(const_cast<UWeaponDefinition*>(Definition), EWeaponRarity::Rare, 3);
		TestEqual(FString::Printf(TEXT("%s saves a key per slot"), Asset), Rolled.Parts.Num(), Definition->Parts.Num());
		TestTrue(FString::Printf(TEXT("%s saves what it shows"), Asset), WeaponParts::PartKeys(WeaponParts::Pick(Rolled)) == Rolled.Parts);

		// New options at the front of every slot would shift every seeded pick; the saved gun doesn't notice.
		UWeaponDefinition* Grown = DuplicateObject(Definition, GetTransientPackage());
		for (FWeaponPartSlot& Slot : Grown->Parts)
		{
			FWeaponPartOption Added = Slot.Options[0];
			Added.Key = TEXT("AddedLater");
			Added.Weight = 50.f;
			Slot.Options.Insert(Added, 0);
		}
		for (int32 Seed = 0; Seed < 50; ++Seed)
		{
			const TArray<FName> Saved = WeaponParts::PartKeys(WeaponParts::Pick(*Definition, Seed, EWeaponRarity::Epic));
			TestTrue(TEXT("Saved parts survive new options"), WeaponParts::PartKeys(WeaponParts::Pick(*Grown, Seed, EWeaponRarity::Epic, Saved)) == Saved);
			// The stats roll the same as well (each part rolls from the seed and its own key).
			const FWeaponStats Before = UWeaponRollLibrary::ComputeStatsWithParts(Definition, EWeaponRarity::Epic, 1, Seed, Saved);
			const FWeaponStats After = UWeaponRollLibrary::ComputeStatsWithParts(Grown, EWeaponRarity::Epic, 1, Seed, Saved);
			TestTrue(TEXT("Saved stats survive new options"), FMath::IsNearlyEqual(Before.Damage, After.Damage) && FMath::IsNearlyEqual(Before.Spread, After.Spread)
				&& Before.MagazineSize == After.MagazineSize && FMath::IsNearlyEqual(Before.Handling, After.Handling));

			TArray<FName> Lost = Saved;
			Lost[0] = TEXT("RemovedSinceItDropped");
			const TArray<FName> Repaired = WeaponParts::PartKeys(WeaponParts::Pick(*Definition, Seed, EWeaponRarity::Epic, Lost));
			TestFalse(TEXT("A lost part is picked again"), Repaired[0].IsNone() || Repaired[0] == Lost[0]);
			for (int32 Slot = 1; Slot < Saved.Num(); ++Slot)
			{
				TestEqual(TEXT("The other parts stay"), Repaired[Slot], Saved[Slot]);
			}
		}
		Grown->MarkAsGarbage();
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponPartRangesTest, "Looter.Weapons.Parts.Ranges",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWeaponPartRangesTest::RunTest(const FString& Parameters)
{
	// Each part's stats are ranges: every gun rolls its own amount within them, the same gun always the same, and one
	// part's roll never depends on the other parts.
	UWeaponDefinition* Gun = MakeGun();
	FWeaponPartOption& Barrel = AddOption(*Gun, TEXT("Barrel"), TEXT("Heavy"));
	Barrel.Stats.Damage = Percent(10.f, 20.f);
	Barrel.Stats.Handling = Percent(-12.f, -6.f);
	AddOption(*Gun, TEXT("Sight"), TEXT("Red"));
	AddOption(*Gun, TEXT("Sight"), TEXT("Holo")).Stats.Accuracy = Percent(5.f, 5.f);

	float Lowest = 100.f;
	float Highest = -100.f;
	for (int32 Seed = 0; Seed < 300; ++Seed)
	{
		const FWeaponPartTotals Totals = WeaponParts::CombinedStats(WeaponParts::Pick(*Gun, Seed, EWeaponRarity::Common), Seed);
		const FWeaponPartTotals Again = WeaponParts::CombinedStats(WeaponParts::Pick(*Gun, Seed, EWeaponRarity::Common), Seed);
		TestTrue(TEXT("Damage within its range"), Totals.Damage >= 10.f && Totals.Damage <= 20.f);
		TestTrue(TEXT("Handling within its range"), Totals.Handling >= -12.f && Totals.Handling <= -6.f);
		TestEqual(TEXT("The same gun rolls the same"), Totals.Damage, Again.Damage);
		Lowest = FMath::Min(Lowest, Totals.Damage);
		Highest = FMath::Max(Highest, Totals.Damage);

		// The barrel rolls the same whichever sight the gun has.
		const TArray<FName> Red = { TEXT("Heavy"), TEXT("Red") };
		const TArray<FName> Holo = { TEXT("Heavy"), TEXT("Holo") };
		const FWeaponPartTotals WithRed = WeaponParts::CombinedStats(WeaponParts::Pick(*Gun, Seed, EWeaponRarity::Common, Red), Seed);
		const FWeaponPartTotals WithHolo = WeaponParts::CombinedStats(WeaponParts::Pick(*Gun, Seed, EWeaponRarity::Common, Holo), Seed);
		TestEqual(TEXT("One part's roll doesn't move another's"), WithRed.Damage, WithHolo.Damage);
		TestEqual(TEXT("A fixed range is fixed"), WithHolo.Accuracy, 5.f);
	}
	TestTrue(FString::Printf(TEXT("Guns spread over the whole range (%.1f-%.1f)"), Lowest, Highest), Lowest < 11.f && Highest > 19.f);
	Gun->MarkAsGarbage();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponPartCapsTest, "Looter.Weapons.Parts.Caps",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWeaponPartCapsTest::RunTest(const FString& Parameters)
{
	// The parts' percentages add up, capped per stat; magazines and sights set capacity and zoom outright (the later
	// part wins).
	UWeaponDefinition* Gun = MakeGun();
	AddOption(*Gun, TEXT("Body"), TEXT("Big"));
	AddOption(*Gun, TEXT("Barrel"), TEXT("Bigger"));
	FWeaponPartOption& First = Gun->Parts[0].Options[0];
	FWeaponPartOption& Second = Gun->Parts[1].Options[0];
	for (FWeaponPartOption* Part : { &First, &Second })
	{
		Part->Stats.Damage = Percent(25.f, 25.f);
		Part->Stats.Accuracy = Percent(-25.f, -25.f);
		Part->Stats.Range = Percent(40.f, 40.f);
		Part->Stats.FireRate = Percent(-15.f, -15.f);
		Part->Stats.Reload = Percent(-30.f, -30.f);
		Part->Stats.Recoil = Percent(30.f, 30.f);
		Part->Stats.Handling = Percent(-5.f, -5.f);
	}
	First.Stats.Magazine = 40;
	First.Stats.Zoom = 2.f;
	Second.Stats.Magazine = 60;

	const FWeaponPartTotals Totals = WeaponParts::CombinedStats(WeaponParts::Pick(*Gun, 1, EWeaponRarity::Common), 1);
	TestEqual(TEXT("Damage capped"), Totals.Damage, WeaponParts::DamageCap.Max);
	TestEqual(TEXT("Accuracy capped"), Totals.Accuracy, WeaponParts::AccuracyCap.Min);
	TestEqual(TEXT("Range capped"), Totals.Range, WeaponParts::RangeCap.Max);
	TestEqual(TEXT("Fire rate capped"), Totals.FireRate, WeaponParts::FireRateCap.Min);
	TestEqual(TEXT("Reload capped"), Totals.Reload, WeaponParts::ReloadCap.Min);
	TestEqual(TEXT("Recoil capped"), Totals.Recoil, WeaponParts::RecoilCap.Max);
	TestEqual(TEXT("Handling under its cap adds up"), Totals.Handling, -10.f);
	TestEqual(TEXT("The later magazine sets the capacity"), Totals.Magazine, 60);
	TestEqual(TEXT("The sight's zoom"), Totals.Zoom, 2.f);
	Gun->MarkAsGarbage();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponPartAssemblyTest, "Looter.Weapons.Parts.Assembly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWeaponPartAssemblyTest::RunTest(const FString& Parameters)
{
	// An assembled gun: its parts in place, the muzzle and hands where they go, a sight to aim through, its reload part
	// moving the way the reload does (the rifle's magazine down out of its well, the shotgun's pump back), and its own
	// paint and rarity glow.
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
		EWeaponReloadPart ReloadPart;
		FVector ReloadWay;
	};
	const FCase Cases[] = {
		{ TEXT("DA_AssaultRifle"), EWeaponReloadPart::Magazine, FVector(0.f, 0.f, -1.f) },
		{ TEXT("DA_PumpShotgun"), EWeaponReloadPart::Pump, FVector(-1.f, 0.f, 0.f) } };
	for (const FCase& Case : Cases)
	{
		for (const EWeaponRarity Rarity : { EWeaponRarity::Common, EWeaponRarity::Legendary })
		{
			FWeaponInstanceData Instance;
			Instance.Definition = const_cast<UWeaponDefinition*>(LoadGun(Case.Asset));
			Instance.Rarity = Rarity;
			Instance.Seed = 7;
			if (!TestNotNull(Case.Asset, Instance.Definition.Get()) || !TestTrue(FString::Printf(TEXT("%s assembles"), Case.Asset), Model->Assemble(Instance)))
			{
				continue;
			}
			TestEqual(FString::Printf(TEXT("%s parts"), Case.Asset), Model->GetParts().Num(), Instance.Definition->Parts.Num());
			TestTrue(FString::Printf(TEXT("%s muzzle ahead (%s)"), Case.Asset, *Model->GetMuzzle().ToString()), Model->GetMuzzle().X > 50.0);
			TestTrue(FString::Printf(TEXT("%s grip under the receiver"), Case.Asset), !Model->GetGrip().IsZero() && Model->GetGrip().Z < 0.0);
			TestTrue(FString::Printf(TEXT("%s foregrip ahead of the grip"), Case.Asset), Model->GetForegrip().X > Model->GetGrip().X + 20.0);
			TestTrue(FString::Printf(TEXT("%s center"), Case.Asset), Model->GetCenter().X > 0.0 && Model->GetCenter().X < Model->GetMuzzle().X);
			// The sight is on top, behind the muzzle, on the gun's center line.
			const FVector Aim = Model->GetAimPoint();
			TestTrue(FString::Printf(TEXT("%s aims over the gun (%s)"), Case.Asset, *Aim.ToString()), Aim.Z > Model->GetGrip().Z && Aim.X < Model->GetMuzzle().X
				&& FMath::Abs(Aim.Y) < 1.0);

			TestEqual(FString::Printf(TEXT("%s reload part"), Case.Asset), Model->GetReloadPart(), Case.ReloadPart);
			const UStaticMeshComponent* Moving = Model->GetParts()[SlotIndex(*Instance.Definition, Instance.Definition->ReloadSlot)];
			const FVector Rest = Moving->GetComponentLocation();
			Model->SetReloadTravel(10.f, true);
			const FVector Moved = (Moving->GetComponentLocation() - Rest) / 10.0;
			TestTrue(FString::Printf(TEXT("%s reload part moves its way (%s)"), Case.Asset, *Moved.ToString()), (Moved | Case.ReloadWay) > 0.9);
			Model->SetReloadTravel(0.f, true);

			// Every paint is this gun's color, and its rarity glows.
			const FWeaponLook Look = WeaponParts::Pick(Instance);
			TSet<FName> Painted;
			int32 Glowing = 0;
			for (const UStaticMeshComponent* Part : Model->GetParts())
			{
				const TArray<FName> Slots = Part->GetMaterialSlotNames();
				for (int32 Material = 0; Material < Slots.Num(); ++Material)
				{
					const UMaterialInstanceDynamic* Instanced = Cast<UMaterialInstanceDynamic>(Part->GetMaterial(Material));
					FLinearColor Color;
					if (const FLinearColor* Wanted = Look.Colors.Find(Slots[Material]))
					{
						const bool bColored = Instanced && (Instanced->GetVectorParameterValue(FHashedMaterialParameterInfo(TEXT("Tint")), Color)
							|| Instanced->GetVectorParameterValue(FHashedMaterialParameterInfo(TEXT("Color")), Color));
						TestTrue(FString::Printf(TEXT("%s %s is this gun's color"), Case.Asset, *Slots[Material].ToString()), bColored && Color.Equals(*Wanted, 0.001f));
						Painted.Add(Slots[Material]);
					}
					if (Slots[Material] == Instance.Definition->RarityGlowSlot && TestNotNull(TEXT("Glow is this gun's"), Instanced))
					{
						Instanced->GetVectorParameterValue(FHashedMaterialParameterInfo(TEXT("Color")), Color);
						TestTrue(TEXT("Glows in its rarity's color"), Color.Equals(UWeaponRollLibrary::GetRarityColor(Instance.Definition, Instance.Rarity), 0.001f));
						++Glowing;
					}
				}
			}
			TestTrue(FString::Printf(TEXT("%s is painted"), Case.Asset), !Painted.IsEmpty());
			TestTrue(FString::Printf(TEXT("%s glows"), Case.Asset), Glowing > 0);

			// Every part carries the gun's wear for the gun master.
			const float Wear = WeaponParts::Wear(Instance);
			for (const UStaticMeshComponent* Part : Model->GetParts())
			{
				const TArray<float>& Data = Part->GetCustomPrimitiveData().Data;
				TestTrue(FString::Printf(TEXT("%s part carries its wear"), Case.Asset), Data.IsValidIndex(WeaponParts::WearDataIndex)
					&& FMath::IsNearlyEqual(Data[WeaponParts::WearDataIndex], Wear));
			}
		}
	}
	Model->Clear();
	TestFalse(TEXT("Cleared"), Model->IsAssembled());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponWearTest, "Looter.Weapons.Parts.Wear",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWeaponWearTest::RunTest(const FString& Parameters)
{
	// Each gun rolls how worn it looks from its seed: the same gun always the same, commons scuffed, legendaries nearly clean.
	double CommonTotal = 0.0;
	double LegendaryTotal = 0.0;
	const int32 NumGuns = 200;
	for (int32 Seed = 0; Seed < NumGuns; ++Seed)
	{
		FWeaponInstanceData Gun;
		Gun.Seed = Seed;
		Gun.Rarity = EWeaponRarity::Common;
		const float Common = WeaponParts::Wear(Gun);
		TestEqual(TEXT("The same gun wears the same"), WeaponParts::Wear(Gun), Common);
		TestTrue(TEXT("Commons are worn"), Common >= 0.45f && Common <= 1.f);
		Gun.Rarity = EWeaponRarity::Legendary;
		const float Legendary = WeaponParts::Wear(Gun);
		TestTrue(TEXT("Legendaries are nearly clean"), Legendary >= 0.f && Legendary <= 0.25f);
		CommonTotal += Common;
		LegendaryTotal += Legendary;
	}
	TestTrue(TEXT("Commons are more worn than legendaries"), CommonTotal / NumGuns > LegendaryTotal / NumGuns + 0.4);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponPartStatsTest, "Looter.Weapons.Parts.Stats",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWeaponPartStatsTest::RunTest(const FString& Parameters)
{
	// A gun's parts carry its affixes: its stats are the roll it would get without parts, scaled by its parts' capped
	// percentages, with the capacity and zoom its parts set; its highest-priority named part names it.
	auto Near = [](float A, float B) { return FMath::IsNearlyEqual(A, B, FMath::Abs(B) * 1.e-4f + 1.e-4f); };
	auto Scale = [](float Percent) { return FMath::Max(1.f + Percent * 0.01f, 0.05f); };
	for (const TCHAR* Asset : Guns)
	{
		const UWeaponDefinition* Definition = LoadGun(Asset);
		if (!TestNotNull(Asset, Definition))
		{
			continue;
		}
		UWeaponDefinition* Bare = DuplicateObject(Definition, GetTransientPackage());
		Bare->Parts.Reset();
		bool bPartsMatter = false;
		bool bZooms = false;
		for (int32 Seed = 0; Seed < 100; ++Seed)
		{
			for (const EWeaponRarity Rarity : Rarities)
			{
				const FWeaponStats Stats = UWeaponRollLibrary::ComputeStats(Definition, Rarity, 1, Seed);
				const FWeaponStats Plain = UWeaponRollLibrary::ComputeStats(Bare, Rarity, 1, Seed);
				const FWeaponPartTotals Parts = WeaponParts::CombinedStats(WeaponParts::Pick(*Definition, Seed, Rarity), Seed);
				const int32 Magazine = FMath::Max(1, FMath::RoundToInt(Parts.Magazine * Definition->GetRarityInfo(Rarity).MagazineMultiplier));
				if (!Near(Stats.Damage, Plain.Damage * Scale(Parts.Damage)) || !Near(Stats.FireRate, Plain.FireRate * Scale(Parts.FireRate))
					|| !Near(Stats.ReloadTime, Plain.ReloadTime * Scale(Parts.Reload)) || !Near(Stats.Spread, Plain.Spread / Scale(Parts.Accuracy))
					|| !Near(Stats.Range, Plain.Range * Scale(Parts.Range)) || !Near(Stats.Recoil, Plain.Recoil * Scale(Parts.Recoil))
					|| !Near(Stats.Handling, Plain.Handling * Scale(Parts.Handling)) || (Parts.Magazine > 0 && Stats.MagazineSize != Magazine)
					|| (Parts.Zoom > 0.f && !Near(Stats.Zoom, FMath::Max(Parts.Zoom, 1.f))))
				{
					AddError(FString::Printf(TEXT("%s seed %d: the stats aren't the part-less roll changed by the parts"), Asset, Seed));
				}
				TestTrue(TEXT("Every gun's magazine is a part"), Parts.Magazine > 0);
				bPartsMatter |= !Near(Parts.Damage, 0.f) || !Near(Parts.Accuracy, 0.f);
				bZooms |= Stats.Zoom > 1.5f;
			}
		}
		TestTrue(FString::Printf(TEXT("%s: parts change stats"), Asset), bPartsMatter);
		TestTrue(FString::Printf(TEXT("%s: some sights magnify"), Asset), bZooms);
		Bare->MarkAsGarbage();

		// Names: the word of the gun's highest-priority part, then the kind of gun; never the rarity (its color says it).
		for (int32 Seed = 0; Seed < 50; ++Seed)
		{
			FWeaponInstanceData Gun;
			Gun.Definition = const_cast<UWeaponDefinition*>(Definition);
			Gun.Seed = Seed;
			Gun.Rarity = EWeaponRarity::Epic;
			const FWeaponLook Look = WeaponParts::Pick(Gun);
			const FWeaponPartOption* Namer = nullptr;
			for (const FWeaponPartOption* Part : Look.Parts)
			{
				Namer = Part && !Part->NamePrefix.IsEmpty() && (!Namer || Part->NamePriority > Namer->NamePriority) ? Part : Namer;
			}
			const FString Name = LooterWeaponText::Name(Gun);
			TestTrue(FString::Printf(TEXT("%s is named by its parts"), *Name), !Namer || Name.Contains(Namer->NamePrefix.ToString()));
			TestFalse(FString::Printf(TEXT("%s has no rarity word"), *Name), Name.Contains(UEnum::GetDisplayValueAsText(Gun.Rarity).ToString()));
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponRangeFalloffTest, "Looter.Weapons.RangeFalloff",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWeaponRangeFalloffTest::RunTest(const FString& Parameters)
{
	// Full damage out to the gun's range, then down to half at twice the range, and no lower.
	FWeaponStats Stats;
	Stats.Range = 2000.f;
	TestEqual(TEXT("Close"), Stats.DamageAtDistance(100.f), 1.f);
	TestEqual(TEXT("At its range"), Stats.DamageAtDistance(2000.f), 1.f);
	TestEqual(TEXT("Halfway out"), Stats.DamageAtDistance(3000.f), 0.75f);
	TestEqual(TEXT("Twice its range"), Stats.DamageAtDistance(4000.f), FWeaponStats::RangeFalloffFloor);
	TestEqual(TEXT("Beyond"), Stats.DamageAtDistance(5900.f), FWeaponStats::RangeFalloffFloor);
	return true;
}

#endif

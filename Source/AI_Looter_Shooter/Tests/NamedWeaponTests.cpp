#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Affixes/WeaponRollLibrary.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionEventObjectives.h"
#include "Missions/MissionRewards.h"
#include "Missions/MissionRunner.h"
#include "Session/CampaignRecord.h"
#include "Session/SessionSave.h"
#include "Session/SessionSubsystem.h"
#include "Tests/MissionTestWorld.h"
#include "UI/Style/WeaponText.h"
#include "UI/World/WeaponLabelWidget.h"
#include "Weapons/NamedWeaponDefinition.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponDefinition.h"
#include "Weapons/WeaponParts.h"
#include "Blueprint/UserWidget.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Package.h"

using namespace MissionTestWorld;

namespace
{
	const TCHAR* const HeirloomPath = TEXT("/Game/Data/Weapons/DA_Named_Heirloom.DA_Named_Heirloom");
	const TCHAR* const ShotgunPath = TEXT("/Game/Weapons/Data/DA_PumpShotgun.DA_PumpShotgun");

	bool Near(double A, double B)
	{
		return FMath::IsNearlyEqual(A, B, FMath::Abs(B) * 1.e-4 + 1.e-4);
	}

	FWeaponStatRange Percent(float Min, float Max)
	{
		FWeaponStatRange Range;
		Range.Min = Min;
		Range.Max = Max;
		return Range;
	}

	/** A part on a made-up gun. Any mesh will do: a part only needs one to be picked. Set it up before adding the next. */
	FWeaponPartOption& AddPart(UWeaponDefinition& Gun, FName Slot, FName Key, EWeaponRarity MinRarity = EWeaponRarity::Common)
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
		Option.MinRarity = MinRarity;
		return Option;
	}

	/**
	 * A made-up gun with known numbers: round base stats with 10% variance, and three slots. The Fine body comes on Rare
	 * guns and up and moves four stats (two where less is better), the Long barrel adds accuracy, and the Tube needs a
	 * barrel of 46 cm or more.
	 */
	UWeaponDefinition* MakeGun()
	{
		UWeaponDefinition* Gun = NewObject<UWeaponDefinition>(GetTransientPackage());
		Gun->DisplayName = FText::FromString(TEXT("Test Gun"));
		Gun->StatVariance = 0.1f;
		Gun->BaseStats.Damage = 10.f;
		Gun->BaseStats.FireRate = 100.f;
		Gun->BaseStats.MagazineSize = 10;
		Gun->BaseStats.ReloadTime = 2.f;
		Gun->BaseStats.Spread = 2.f;
		Gun->BaseStats.Range = 1000.f;
		AddPart(*Gun, TEXT("Body"), TEXT("Plain"));
		FWeaponPartOption& Fine = AddPart(*Gun, TEXT("Body"), TEXT("Fine"), EWeaponRarity::Rare);
		Fine.Stats.Damage = Percent(10.f, 20.f);
		Fine.Stats.Reload = Percent(10.f, 30.f);
		Fine.Stats.Recoil = Percent(-20.f, -10.f);
		Fine.Stats.Handling = Percent(-12.f, -6.f);
		Fine.NamePrefix = FText::FromString(TEXT("Fine"));
		Fine.NamePriority = 10;
		FWeaponPartOption& Long = AddPart(*Gun, TEXT("Barrel"), TEXT("Long"));
		Long.Length = 50.f;
		Long.Stats.Accuracy = Percent(4.f, 8.f);
		AddPart(*Gun, TEXT("Barrel"), TEXT("Short")).Length = 30.f;
		FWeaponPartOption& Tube = AddPart(*Gun, TEXT("Magazine"), TEXT("Tube"));
		Tube.Stats.Magazine = 8;
		Tube.Requires.Slot = TEXT("Barrel");
		Tube.Requires.MinLength = 46.f;
		AddPart(*Gun, TEXT("Magazine"), TEXT("Box")).Stats.Magazine = 5;
		return Gun;
	}

	/** DA_Named_Heirloom, found by its id as missions and the console find it; null, with an error saying how to make it. */
	UNamedWeaponDefinition* LoadHeirloom(FAutomationTestBase& Test)
	{
		UNamedWeaponDefinition* Heirloom = UNamedWeaponDefinition::FindByName(TEXT("Heirloom"));
		if (!Heirloom)
		{
			Test.AddError(TEXT("DA_Named_Heirloom isn't in /Game/Data/Weapons: run Tools/Unreal/create_named_weapons.py in the editor."));
		}
		return Heirloom;
	}

	/** The same stats, within float rounding (a copy rebuilt is computed the same way). */
	bool SameStats(const FWeaponStats& A, const FWeaponStats& B)
	{
		return Near(A.Damage, B.Damage) && Near(A.FireRate, B.FireRate) && A.MagazineSize == B.MagazineSize && Near(A.ReloadTime, B.ReloadTime)
			&& Near(A.Spread, B.Spread) && Near(A.Range, B.Range) && Near(A.Recoil, B.Recoil) && Near(A.Handling, B.Handling) && Near(A.Zoom, B.Zoom);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNamedWeaponRulesTest, "Looter.Weapons.Named.Rules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FNamedWeaponRulesTest::RunTest(const FString& Parameters)
{
	// A named gun's stats aren't rolled: its parts' percentages and its kind's variance sit at its fixed quality, 1 the
	// better end of each range (the low end where less is better), whatever its seed, and rolled guns land between the
	// ends. A named gun made of those parts: every copy the same gun, named and lined as itself, worn as it says; the
	// same parts unnamed are an ordinary gun. FindProblems catches parts that don't exist, don't come at its rarity or
	// don't fit. On a made-up gun, so the numbers are known.
	UWeaponDefinition* Gun = MakeGun();
	const TArray<FName> Fine = { TEXT("Fine"), TEXT("Long"), TEXT("Tube") };
	const FWeaponLook Look = WeaponParts::Pick(*Gun, 3, EWeaponRarity::Rare, Fine);
	TestTrue(TEXT("The parts asked for"), WeaponParts::PartKeys(Look) == Fine);

	const FWeaponPartTotals Best = WeaponParts::CombinedStats(Look, 3, 1.f);
	TestTrue(TEXT("At 1: the top where more is better"), Near(Best.Damage, 20.0) && Near(Best.Accuracy, 8.0) && Near(Best.Handling, -6.0));
	TestTrue(TEXT("...and the bottom where less is better"), Near(Best.Reload, 10.0) && Near(Best.Recoil, -20.0));
	const FWeaponPartTotals Worst = WeaponParts::CombinedStats(Look, 3, 0.f);
	TestTrue(TEXT("At 0: the other ends"), Near(Worst.Damage, 10.0) && Near(Worst.Accuracy, 4.0) && Near(Worst.Handling, -12.0)
		&& Near(Worst.Reload, 30.0) && Near(Worst.Recoil, -10.0));
	const FWeaponPartTotals Middle = WeaponParts::CombinedStats(Look, 3, 0.5f);
	TestTrue(TEXT("At a half: the middles"), Near(Middle.Damage, 15.0) && Near(Middle.Reload, 20.0) && Near(Middle.Recoil, -15.0));
	const FWeaponPartTotals OtherSeed = WeaponParts::CombinedStats(Look, 9999, 0.5f);
	TestTrue(TEXT("The seed doesn't move a fixed point"), Near(OtherSeed.Damage, Middle.Damage) && Near(OtherSeed.Reload, Middle.Reload));
	TestEqual(TEXT("The tube sets the capacity"), Best.Magazine, 8);

	// The whole gun: at 1 the kind's variance gives 10% more damage and fire rate, 10% quicker reloads, 10% tighter spread.
	const FWeaponRarityInfo& Rare = Gun->GetRarityInfo(EWeaponRarity::Rare);
	const FWeaponStats Top = UWeaponRollLibrary::ComputeStatsWithParts(Gun, EWeaponRarity::Rare, 1, 3, Fine, 1.f);
	TestTrue(TEXT("At 1: damage"), Near(Top.Damage, 10.0 * 1.1 * Rare.DamageMultiplier * 1.2));
	TestTrue(TEXT("At 1: fire rate"), Near(Top.FireRate, 100.0 * 1.1 * Rare.FireRateMultiplier));
	TestTrue(TEXT("At 1: the quickest reload"), Near(Top.ReloadTime, 2.0 * 0.9 * Rare.ReloadTimeMultiplier * 1.1));
	TestTrue(TEXT("At 1: the tightest spread"), Near(Top.Spread, 2.0 * 0.9 * Rare.SpreadMultiplier / 1.08));
	TestEqual(TEXT("The tube's shells, scaled by rarity"), Top.MagazineSize, FMath::RoundToInt(8.f * Rare.MagazineMultiplier));
	const FWeaponStats Low = UWeaponRollLibrary::ComputeStatsWithParts(Gun, EWeaponRarity::Rare, 1, 3, Fine, 0.f);
	const FWeaponStats Mid = UWeaponRollLibrary::ComputeStatsWithParts(Gun, EWeaponRarity::Rare, 1, 3, Fine, 0.5f);
	TestTrue(TEXT("Better quality, more damage"), Low.Damage < Mid.Damage && Mid.Damage < Top.Damage);
	TestTrue(TEXT("...quicker reloads and tighter spread"), Low.ReloadTime > Mid.ReloadTime && Mid.ReloadTime > Top.ReloadTime
		&& Low.Spread > Mid.Spread && Mid.Spread > Top.Spread);
	TestTrue(TEXT("Any seed, the same stats"), SameStats(UWeaponRollLibrary::ComputeStatsWithParts(Gun, EWeaponRarity::Rare, 1, 777, Fine, 0.5f), Mid));
	for (int32 Seed = 0; Seed < 200; ++Seed)
	{
		const FWeaponStats Rolled = UWeaponRollLibrary::ComputeStatsWithParts(Gun, EWeaponRarity::Rare, 1, Seed, Fine);
		if (Rolled.Damage < Low.Damage - 1.e-3f || Rolled.Damage > Top.Damage + 1.e-3f
			|| Rolled.ReloadTime < Top.ReloadTime - 1.e-3f || Rolled.ReloadTime > Low.ReloadTime + 1.e-3f)
		{
			AddError(FString::Printf(TEXT("Seed %d rolled outside the fixed ends"), Seed));
		}
	}

	// A named gun of those parts, at the top of its ranges (in a scratch package, so it never meets the project's assets).
	UNamedWeaponDefinition* Named = NewObject<UNamedWeaponDefinition>(CreatePackage(nullptr), TEXT("DA_Named_TestKeepsake"), RF_Transient);
	Named->Weapon = Gun;
	Named->DisplayName = FText::FromString(TEXT("Keepsake"));
	Named->FlavorText = FText::FromString(TEXT("Mind the step."));
	Named->Rarity = EWeaponRarity::Rare;
	Named->Parts.Add(TEXT("Body"), TEXT("Fine"));
	Named->Parts.Add(TEXT("Barrel"), TEXT("Long"));
	Named->Parts.Add(TEXT("Magazine"), TEXT("Tube"));
	Named->StatQuality = 1.f;
	Named->Wear = 0.3f;
	Named->Seed = 11;
	TestEqual(TEXT("Its id"), Named->GetNamedId(), FName(TEXT("TestKeepsake")));
	const TArray<FString> Problems = Named->FindProblems();
	TestTrue(FString::Printf(TEXT("Sound (%s)"), *FString::Join(Problems, TEXT("; "))), Problems.IsEmpty());

	const FWeaponInstanceData Copy = Named->MakeInstance(5);
	TestTrue(TEXT("A copy: its kind, its rarity, the level asked for, its seed"), Copy.Definition == Gun && Copy.Named == Named
		&& Copy.Rarity == EWeaponRarity::Rare && Copy.Level == 5 && Copy.Seed == 11);
	TestTrue(TEXT("Its parts, in its kind's slot order"), Copy.Parts == Fine);
	TestTrue(TEXT("Built from exactly those"), WeaponParts::PartKeys(WeaponParts::Pick(Copy)) == Fine);
	TestTrue(TEXT("Its stats at its quality"), SameStats(Copy.Stats, UWeaponRollLibrary::ComputeStatsWithParts(Gun, EWeaponRarity::Rare, 5, 11, Fine, 1.f)));
	TestTrue(TEXT("...which a gun's stats are rebuilt as"), SameStats(UWeaponRollLibrary::ComputeInstanceStats(Copy), Copy.Stats));
	TestTrue(TEXT("Every copy is the same gun"), SameStats(Named->MakeInstance(5).Stats, Copy.Stats) && Named->MakeInstance(5).Parts == Copy.Parts);
	TestTrue(TEXT("Only its level follows the player"), Named->MakeInstance(1).Stats.Damage < Copy.Stats.Damage);
	TestEqualSensitive(TEXT("Its own name"), LooterWeaponText::Name(Copy), FString(TEXT("Keepsake")));
	TestEqualSensitive(TEXT("Its line"), LooterWeaponText::FlavorLine(Copy), FString(TEXT("Mind the step.")));
	TestEqual(TEXT("Its own wear"), WeaponParts::Wear(Copy), 0.3f);

	// The same gun unnamed: named by its parts, no line, stats and wear rolled from its seed like any gun's.
	FWeaponInstanceData Plain = Copy;
	Plain.Named = nullptr;
	TestEqualSensitive(TEXT("Unnamed: named by its parts"), LooterWeaponText::Name(Plain), FString(TEXT("Fine Test Gun")));
	TestTrue(TEXT("Unnamed: no line"), LooterWeaponText::FlavorLine(Plain).IsEmpty());
	const FWeaponStats Seeded = UWeaponRollLibrary::ComputeStatsWithParts(Gun, EWeaponRarity::Rare, 5, 11, Fine);
	const FWeaponStats Rebuilt = UWeaponRollLibrary::ComputeInstanceStats(Plain);
	TestTrue(TEXT("Unnamed: rolled from its seed"), Rebuilt.Damage == Seeded.Damage && Rebuilt.ReloadTime == Seeded.ReloadTime
		&& Rebuilt.Spread == Seeded.Spread);
	const float SeedWear = WeaponParts::Wear(Plain);
	TestTrue(TEXT("Unnamed: worn as a rare gun's seed rolls"), SeedWear >= 0.15f && SeedWear <= 0.65f);
	Named->Wear = -1.f;
	TestEqual(TEXT("No wear of its own: its seed's"), WeaponParts::Wear(Copy), SeedWear);
	Named->Wear = 0.3f;

	// The names it answers to.
	TestTrue(TEXT("Its id, in any case or spacing, with or without its asset's prefix, or its display name"), Named->IsNamed(TEXT("TestKeepsake"))
		&& Named->IsNamed(TEXT("test keepsake")) && Named->IsNamed(TEXT("DA_Named_TestKeepsake")) && Named->IsNamed(TEXT("Keepsake")));
	TestFalse(TEXT("Not part of a name, nor nothing"), Named->IsNamed(TEXT("Keeps")) || Named->IsNamed(TEXT("")));

	// What FindProblems catches, one at a time.
	auto Says = [Named](const TCHAR* Words)
	{
		return Named->FindProblems().ContainsByPredicate([Words](const FString& Problem) { return Problem.Contains(Words); });
	};
	Named->Parts.Add(TEXT("Body"), TEXT("Gold"));
	TestTrue(TEXT("A part its slot doesn't have"), Says(TEXT("no part Gold")));
	Named->Parts.Add(TEXT("Body"), TEXT("Fine"));
	Named->Rarity = EWeaponRarity::Uncommon;
	TestTrue(TEXT("A part that doesn't come at its rarity"), Says(TEXT("comes on Rare guns")));
	Named->Rarity = EWeaponRarity::Rare;
	Named->Parts.Add(TEXT("Barrel"), TEXT("Short"));
	TestTrue(TEXT("A tube on a barrel too short for it"), Says(TEXT("needs a barrel of at least 46 cm")));
	Named->Parts.Add(TEXT("Barrel"), TEXT("Long"));
	Named->Parts.Remove(TEXT("Magazine"));
	TestTrue(TEXT("A slot left without a part"), Says(TEXT("no part for the Magazine slot")));
	TestTrue(TEXT("...which a copy would leave empty"), Named->GetPartKeys().Num() == 3 && Named->GetPartKeys()[2].IsNone());
	Named->Parts.Add(TEXT("Magazine"), TEXT("Tube"));
	Named->Parts.Add(TEXT("Scope"), TEXT("Big"));
	TestTrue(TEXT("A slot its kind doesn't have"), Says(TEXT("no Scope slot")));
	Named->Parts.Remove(TEXT("Scope"));
	Named->DisplayName = FText::GetEmpty();
	TestTrue(TEXT("No name"), Says(TEXT("no name")));
	Named->DisplayName = FText::FromString(TEXT("Keepsake"));
	TestTrue(TEXT("Mended, it's sound again"), Named->FindProblems().IsEmpty());

	Named->MarkAsGarbage();
	Gun->MarkAsGarbage();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNamedWeaponHeirloomTest, "Looter.Weapons.Named.Heirloom",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FNamedWeaponHeirloomTest::RunTest(const FString& Parameters)
{
	// Heirloom, Abel's Ranchhand (Main 7's reward): a named Epic pump whose fixed parts all exist, come at Epic and fit,
	// called Heirloom with the line "Hold the door.". A copy is that gun at the level asked for; spawned it keeps its
	// name and fixed stats, and its loot label shows the name, and the line under it when looked at. The same parts
	// unnamed make an ordinary shotgun, named by its parts as before, and an ordinary gun's label has no line.
	UNamedWeaponDefinition* Heirloom = LoadHeirloom(*this);
	UWeaponDefinition* Shotgun = LoadObject<UWeaponDefinition>(nullptr, ShotgunPath);
	if (!Heirloom || !TestNotNull(TEXT("The pump shotgun"), Shotgun))
	{
		return false;
	}
	TestEqual(TEXT("Where it's kept"), Heirloom->GetPathName(), FString(HeirloomPath));
	TestTrue(TEXT("A Ranchhand pump"), Heirloom->Weapon == Shotgun);
	TestTrue(TEXT("Epic"), Heirloom->Rarity == EWeaponRarity::Epic);
	TestEqualSensitive(TEXT("Its name"), Heirloom->DisplayName.ToString(), FString(TEXT("Heirloom")));
	TestEqualSensitive(TEXT("Its line"), Heirloom->FlavorText.ToString(), FString(TEXT("Hold the door.")));
	for (const FString& Problem : Heirloom->FindProblems())
	{
		AddError(FString::Printf(TEXT("Heirloom: %s"), *Problem));
	}
	TestEqual(TEXT("A part for every slot: nothing left to chance"), Heirloom->Parts.Num(), Shotgun->Parts.Num());
	// The parts of its spectral twin (SM_AbelPump), as chosen at step 23; the sight among them, so it aims through one.
	struct FChosenPart
	{
		const TCHAR* Slot;
		const TCHAR* Key;
	};
	const FChosenPart Chosen[] = { { TEXT("Body"), TEXT("Heritage") }, { TEXT("Barrel"), TEXT("Trap") }, { TEXT("Muzzle"), TEXT("Crown") },
		{ TEXT("Magazine"), TEXT("Tube6") }, { TEXT("Sight"), TEXT("Flip") }, { TEXT("Stock"), TEXT("Field") }, { TEXT("Pump"), TEXT("Walnut") } };
	for (const FChosenPart& Part : Chosen)
	{
		const FName Has = Heirloom->Parts.FindRef(FName(Part.Slot));
		TestEqual(*FString::Printf(TEXT("Its %s"), Part.Slot), Has, FName(Part.Key));
	}
	TestTrue(TEXT("Its stats at a point of their ranges"), Heirloom->StatQuality >= 0.f && Heirloom->StatQuality <= 1.f);
	TestTrue(TEXT("Its own wear"), Heirloom->Wear >= 0.f && Heirloom->Wear <= 1.f);

	const FWeaponInstanceData Gun = Heirloom->MakeInstance(9);
	TestTrue(TEXT("A copy: the pump, Heirloom, Epic, level 9"), Gun.Definition == Shotgun && Gun.Named == Heirloom
		&& Gun.Rarity == EWeaponRarity::Epic && Gun.Level == 9 && Gun.Seed == Heirloom->Seed);
	TestTrue(TEXT("Its parts, every slot filled"), Gun.Parts == Heirloom->GetPartKeys() && !Gun.Parts.Contains(NAME_None));
	TestTrue(TEXT("Built from exactly those"), WeaponParts::PartKeys(WeaponParts::Pick(Gun)) == Gun.Parts);
	TestEqualSensitive(TEXT("Called Heirloom"), LooterWeaponText::Name(Gun), FString(TEXT("Heirloom")));
	TestEqualSensitive(TEXT("Hold the door."), LooterWeaponText::FlavorLine(Gun), FString(TEXT("Hold the door.")));
	TestEqual(TEXT("Worn as it says"), WeaponParts::Wear(Gun), Heirloom->Wear);
	TestTrue(TEXT("Its stats at its quality"), SameStats(Gun.Stats, UWeaponRollLibrary::ComputeStatsWithParts(Shotgun, EWeaponRarity::Epic, 9,
		Gun.Seed, Gun.Parts, Heirloom->StatQuality)));
	TestTrue(TEXT("Every copy the same"), SameStats(Heirloom->MakeInstance(9).Stats, Gun.Stats));

	// The same parts unnamed: an ordinary shotgun, named by its parts as any is; a freshly rolled one too.
	FWeaponInstanceData Plain = Gun;
	Plain.Named = nullptr;
	const FText Word = WeaponParts::NamePrefix(WeaponParts::Pick(Plain));
	const FString Kind = Shotgun->DisplayName.ToString();
	TestFalse(TEXT("Its parts have a word of their own (the barrel's Trap), which its name wins over"), Word.IsEmpty());
	TestEqual(TEXT("Unnamed: its parts' name"), LooterWeaponText::Name(Plain), Word.IsEmpty() ? Kind : Word.ToString() + TEXT(" ") + Kind);
	TestTrue(TEXT("Unnamed: no line"), LooterWeaponText::FlavorLine(Plain).IsEmpty());
	const FWeaponInstanceData Rolled = UWeaponRollLibrary::RollWeaponWithRarity(Shotgun, EWeaponRarity::Epic, 9);
	TestTrue(TEXT("A rolled shotgun is no named gun"), Rolled.Named == nullptr && LooterWeaponText::FlavorLine(Rolled).IsEmpty()
		&& LooterWeaponText::Name(Rolled).EndsWith(Kind, ESearchCase::CaseSensitive));

	// Spawned: still Heirloom, its stats rebuilt at its quality; its loot label.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	AWeaponBase* Spawned = UWeaponRollLibrary::SpawnWeapon(World, Gun, FTransform::Identity);
	AWeaponBase* Ordinary = UWeaponRollLibrary::SpawnWeapon(World, Rolled, FTransform(FVector(200.0, 0.0, 0.0)));
	if (!TestNotNull(TEXT("Heirloom spawned"), Spawned) || !TestNotNull(TEXT("A rolled shotgun spawned"), Ordinary))
	{
		return false;
	}
	TestTrue(TEXT("Spawned, it's still Heirloom"), Spawned->GetInstance().Named == Heirloom);
	TestEqualSensitive(TEXT("...by name"), Spawned->GetDisplayName().ToString(), FString(TEXT("Heirloom")));
	TestTrue(TEXT("...with its fixed stats"), SameStats(Spawned->GetStats(), Gun.Stats));

	UWeaponLabelWidget* Label = CreateWidget<UWeaponLabelWidget>(World, UWeaponLabelWidget::StaticClass());
	UWeaponLabelWidget* OrdinaryLabel = CreateWidget<UWeaponLabelWidget>(World, UWeaponLabelWidget::StaticClass());
	if (!TestNotNull(TEXT("A loot label"), Label) || !TestNotNull(TEXT("Another"), OrdinaryLabel))
	{
		return false;
	}
	Label->SetWeapon(Spawned);
	Label->TakeWidget();
	TestEqualSensitive(TEXT("The label: its name"), Label->GetNameText().ToString(), FString(TEXT("HEIRLOOM")));
	TestFalse(TEXT("From afar, the name alone"), Label->IsFlavorShown());
	Label->SetFocused(true);
	TestTrue(TEXT("Looked at: its line under the name"), Label->IsFlavorShown());
	TestEqualSensitive(TEXT("...as written"), Label->GetFlavorText().ToString(), FString(TEXT("Hold the door.")));
	OrdinaryLabel->SetWeapon(Ordinary);
	OrdinaryLabel->TakeWidget();
	OrdinaryLabel->SetFocused(true);
	TestEqualSensitive(TEXT("An ordinary gun's label: its parts' name"), OrdinaryLabel->GetNameText().ToString(), LooterWeaponText::Name(Rolled).ToUpper());
	TestFalse(TEXT("...and no line"), OrdinaryLabel->IsFlavorShown());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNamedWeaponSaveTest, "Looter.Weapons.Named.Save",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FNamedWeaponSaveTest::RunTest(const FString& Parameters)
{
	// Heirloom goes through a session's save as any gun does (in hand, in the backpack, lying on the ground) and comes back
	// Heirloom: the named gun it is, by name, with its line, parts and fixed stats. A rolled gun beside it comes back as it
	// was, unnamed, which is how every gun saved before named guns existed reads.
	UNamedWeaponDefinition* Heirloom = LoadHeirloom(*this);
	UWeaponDefinition* Shotgun = Heirloom ? Heirloom->Weapon.Get() : nullptr;
	if (!Heirloom || !TestNotNull(TEXT("Its kind"), Shotgun))
	{
		return false;
	}
	FWeaponInstanceData Held = Heirloom->MakeInstance(7);
	Held.SavedMagazine = 4;
	const FWeaponInstanceData Packed = Heirloom->MakeInstance(3);
	const FWeaponInstanceData Rolled = UWeaponRollLibrary::RollWeaponWithRarity(Shotgun, EWeaponRarity::Epic, 7);
	const FString MapPath(TEXT("/Game/Maps/Lvl_RansomsRest"));

	ULooterSessionSave* Save = NewObject<ULooterSessionSave>();
	Save->Version = ULooterSessionSave::CurrentVersion;
	Save->bHasInventory = true;
	Save->Inventory.Equipped = { Held, Rolled };
	Save->Inventory.ActiveSlot = 0;
	Save->Inventory.Backpack = { Packed };
	FSavedLootWeapon& Lying = Save->FindOrAddWorld(MapPath).LootWeapons.AddDefaulted_GetRef();
	Lying.Weapon = Heirloom->MakeInstance(9);
	Lying.Transform = FTransform(FVector(100.0, 200.0, 300.0));

	TArray<uint8> Bytes;
	const ULooterSessionSave* Read = UGameplayStatics::SaveGameToMemory(Save, Bytes) ? USessionSubsystem::ReadSave(Bytes) : nullptr;
	if (!TestNotNull(TEXT("Written and read back"), Read) || !TestEqual(TEXT("Both guns in hand's slots"), Read->Inventory.Equipped.Num(), 2))
	{
		return false;
	}
	const FWeaponInstanceData& Back = Read->Inventory.Equipped[0];
	TestTrue(TEXT("Heirloom comes back as the named gun it is"), Back.Named == Heirloom && Back.Definition == Shotgun);
	TestEqualSensitive(TEXT("...by its name"), LooterWeaponText::Name(Back), FString(TEXT("Heirloom")));
	TestEqualSensitive(TEXT("...with its line"), LooterWeaponText::FlavorLine(Back), FString(TEXT("Hold the door.")));
	TestTrue(TEXT("...its parts, rarity, level, seed and magazine"), Back.Parts == Held.Parts && Back.Rarity == EWeaponRarity::Epic
		&& Back.Level == 7 && Back.Seed == Held.Seed && Back.SavedMagazine == 4);
	TestTrue(TEXT("...and its fixed stats when rebuilt"), SameStats(UWeaponRollLibrary::ComputeInstanceStats(Back), Held.Stats));
	TestTrue(TEXT("Heirloom in the backpack"), Read->Inventory.Backpack.Num() == 1 && Read->Inventory.Backpack[0].Named == Heirloom
		&& Read->Inventory.Backpack[0].Level == 3);
	const FSavedMapWorld* Kept = Read->FindWorld(MapPath);
	TestTrue(TEXT("Heirloom lying on the ground"), Kept && Kept->LootWeapons.Num() == 1 && Kept->LootWeapons[0].Weapon.Named == Heirloom
		&& LooterWeaponText::Name(Kept->LootWeapons[0].Weapon) == TEXT("Heirloom"));

	const FWeaponInstanceData& Ordinary = Read->Inventory.Equipped[1];
	TestTrue(TEXT("The rolled gun comes back unnamed"), Ordinary.Named == nullptr && Ordinary.Seed == Rolled.Seed && Ordinary.Parts == Rolled.Parts);
	TestEqual(TEXT("...under the name its parts give it"), LooterWeaponText::Name(Ordinary), LooterWeaponText::Name(Rolled));
	TestTrue(TEXT("...with no line"), LooterWeaponText::FlavorLine(Ordinary).IsEmpty());
	TestTrue(TEXT("...and its rolled stats"), SameStats(UWeaponRollLibrary::ComputeInstanceStats(Ordinary), Rolled.Stats));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNamedWeaponRewardTest, "Looter.Weapons.Named.Reward",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FNamedWeaponRewardTest::RunTest(const FString& Parameters)
{
	// A mission whose rewards name a named gun gives it as it finishes: Heirloom lies at the player's feet as loot, at their
	// level, once (finished again, nothing more). The Missions page names it, and a reward naming a named gun that
	// doesn't exist gives nothing and says so.
	UNamedWeaponDefinition* Heirloom = LoadHeirloom(*this);
	if (!Heirloom)
	{
		return false;
	}
	FMissionRewards Words;
	Words.NamedGun = TEXT("Heirloom");
	TestFalse(TEXT("A named gun is a reward"), Words.IsEmpty());
	const TArray<FString> Lines = MissionRewards::Describe(Words);
	TestTrue(TEXT("The Missions page names it"), Lines.Num() == 1 && Lines[0].Equals(TEXT("Named gun: Heirloom"), ESearchCase::CaseSensitive));

	FCampaignRecord Campaign;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	if (!TestNotNull(TEXT("The level has a mission runner"), Runner))
	{
		return false;
	}
	const FName LeansId(TEXT("TestLanternLeans"));
	UPackage* Scratch = CreatePackage(nullptr);
	UMissionDefinition* Leans = NewMission(Scratch, TEXT("TestLanternLeans"), EMissionKind::Main, EMissionStart::Automatic, TEXT("TestValley"));
	AddObjective<UMissionEventObjective>(Leans, 0)->Event = TEXT("Board.Read");
	Leans->Rewards.NamedGun = TEXT("Heirloom");
	AActor* Player = SpawnMarker(World, FVector(500.0, 0.0, 100.0));
	if (!TestNotNull(TEXT("The player's stand-in"), Player))
	{
		return false;
	}
	auto GivenHeirlooms = [World, Heirloom]()
	{
		TArray<AWeaponBase*> Found;
		for (TActorIterator<AWeaponBase> It(World); It; ++It)
		{
			if (It->IsPickup() && It->GetInstance().Named == Heirloom)
			{
				Found.Add(*It);
			}
		}
		return Found;
	};

	Runner->BeginForTesting({ Leans }, Campaign, Player, TEXT("TestValley"));
	Runner->Update(0.f);
	TestTrue(TEXT("The mission runs"), Runner->IsRunning(LeansId));
	TestEqual(TEXT("Nothing given before it's done"), GivenHeirlooms().Num(), 0);
	Runner->NotifyEvent(FMissionEvent::Named(TEXT("Board.Read")));
	TestTrue(TEXT("Finished"), Campaign.HasCompleted(LeansId));
	const TArray<AWeaponBase*> Given = GivenHeirlooms();
	if (TestEqual(TEXT("Heirloom given"), Given.Num(), 1))
	{
		const FWeaponInstanceData& Gun = Given[0]->GetInstance();
		TestTrue(TEXT("...as itself: its kind, rarity and parts"), Gun.Definition == Heirloom->Weapon && Gun.Rarity == Heirloom->Rarity
			&& Gun.Parts == Heirloom->GetPartKeys());
		TestEqual(TEXT("...at the player's level (1: no progress in a test level)"), Gun.Level, 1);
		TestTrue(TEXT("...at their feet"), FVector::Dist2D(Given[0]->GetActorLocation(), Player->GetActorLocation()) < 300.0);
	}
	TestTrue(TEXT("Finished again from the console"), Runner->CompleteMission(LeansId));
	TestEqual(TEXT("Given once"), GivenHeirlooms().Num(), 1);

	// A reward naming a named gun that doesn't exist gives nothing, and says so; nor is anything given with no player.
	FMissionRewards Unknown;
	Unknown.NamedGun = TEXT("NoSuchGun");
	AddExpectedMessagePlain(TEXT("no named gun NoSuchGun"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);
	TestNull(TEXT("An unknown named gun isn't given"), MissionRewards::DropNamedGun(World, Unknown, 5, Player));
	TestNull(TEXT("Nothing without a player"), MissionRewards::DropNamedGun(World, Words, 5, nullptr));
	return true;
}

#endif

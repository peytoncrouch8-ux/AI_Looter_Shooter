#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Affixes/WeaponMakers.h"
#include "Weapons/WeaponDefinition.h"
#include "Weapons/WeaponModelComponent.h"
#include "Weapons/WeaponParts.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "HAL/IConsoleManager.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "UObject/Package.h"
#include "Tests/AutomationCommon.h"

// The gun's field paint and its element's stains (the user's call, 2026-10-27; Docs/Work/2026-10-27-GunPatterns/CONTRACT.md):
// a pattern rolled from the gun's seed, rarity and maker into every part's custom primitive data 29-35 (rarer is fancier, the
// wholly glowing parts plain), and the element part's finish spread lightly over the rest of the gun. Looter.Weapons.Pattern.*.

namespace
{
	const TCHAR* const PatternGuns[] = { TEXT("DA_AssaultRifle"), TEXT("DA_PumpShotgun"), TEXT("DA_Revolver"), TEXT("DA_SMG"),
		TEXT("DA_Sniper"), TEXT("DA_LMG"), TEXT("DA_Launcher") };
	const EWeaponRarity PatternRarities[] = { EWeaponRarity::Common, EWeaponRarity::Uncommon, EWeaponRarity::Rare, EWeaponRarity::Epic,
		EWeaponRarity::Legendary };
	const ELooterElement PatternElements[] = { ELooterElement::Ember, ELooterElement::SoulLight, ELooterElement::GraveSalt };
	constexpr int32 PatternRolls = 200;

	UWeaponDefinition* LoadPatternGun(const TCHAR* Asset)
	{
		return LoadObject<UWeaponDefinition>(nullptr, *FString::Printf(TEXT("/Game/Weapons/Data/%s.%s"), Asset, Asset));
	}

	/** A rolled gun as the loot makes one: its maker from its seed. */
	FWeaponInstanceData PatternGun(UWeaponDefinition* Definition, EWeaponRarity Rarity, int32 Seed)
	{
		FWeaponInstanceData Gun;
		Gun.Definition = Definition;
		Gun.Rarity = Rarity;
		Gun.Seed = Seed;
		Gun.Maker = WeaponMakers::Roll(Definition, Seed, Rarity);
		return Gun;
	}

	float PatternDataAt(const UPrimitiveComponent& Part, int32 Index)
	{
		const TArray<float>& Data = Part.GetCustomPrimitiveData().Data;
		return Data.IsValidIndex(Index) ? Data[Index] : 0.f;
	}

	/** The contract's kinds per rarity (the legendary's maker design added by the caller). */
	TArray<float> AllowedKinds(EWeaponRarity Rarity)
	{
		switch (Rarity)
		{
		case EWeaponRarity::Common: return { 1.f, 2.f };
		case EWeaponRarity::Uncommon: return { 1.f, 2.f, 3.f };
		case EWeaponRarity::Rare: return { 3.f, 4.f, 5.f, 6.f };
		case EWeaponRarity::Epic: return { 4.f, 5.f, 6.f, 7.f };
		case EWeaponRarity::Legendary:
		default: return { 7.f };
		}
	}

	/** The gun's saved part keys with Element's first part fitted and no other element part; false if it has none. */
	bool KeysWithElementPart(const FWeaponInstanceData& Gun, ELooterElement Element, TArray<FName>& OutKeys)
	{
		const UWeaponDefinition& Definition = *Gun.Definition;
		OutKeys = WeaponParts::PartKeys(WeaponParts::Pick(Gun));
		bool bFitted = false;
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponPatternRarityTest, "Looter.Weapons.Pattern.Rarity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWeaponPatternRarityTest::RunTest(const FString& Parameters)
{
	// Rarer is fancier: each rarity rolls only its kinds (every one of them over many seeds), subtle on commons, bold on
	// legendaries, its tint from the maker's palette and never lost on the shell.
	UWeaponDefinition* Definition = LoadPatternGun(TEXT("DA_AssaultRifle"));
	if (!TestNotNull(TEXT("DA_AssaultRifle"), Definition))
	{
		return false;
	}
	for (const EWeaponRarity Rarity : PatternRarities)
	{
		const FString Name = UEnum::GetValueAsString(Rarity);
		TSet<float> Seen;
		for (int32 Roll = 0; Roll < PatternRolls; ++Roll)
		{
			const FWeaponInstanceData Gun = PatternGun(Definition, Rarity, Roll * 7919 + 13);
			const FWeaponLook Look = WeaponParts::Pick(Gun);
			const FWeaponPattern Pattern = UWeaponModelComponent::PickPattern(Gun, Look);
			TArray<float> Allowed = AllowedKinds(Rarity);
			if (Rarity == EWeaponRarity::Legendary)
			{
				const FWeaponMaker* Maker = WeaponMakers::Of(Gun);
				Allowed.AddUnique(UWeaponModelComponent::MakerPatternKind(Maker ? Maker->Key : NAME_None));
			}
			if (!Allowed.Contains(Pattern.Kind))
			{
				AddError(FString::Printf(TEXT("%s seed %d rolled kind %.0f"), *Name, Gun.Seed, Pattern.Kind));
				continue;
			}
			Seen.Add(Pattern.Kind);
			TestTrue(FString::Printf(TEXT("%s: scale within 4-30 cm (%.1f)"), *Name, Pattern.Scale), Pattern.Scale >= 4.f && Pattern.Scale <= 30.f);
			TestTrue(FString::Printf(TEXT("%s: seed within 0-1 (%.3f)"), *Name, Pattern.Seed), Pattern.Seed >= 0.f && Pattern.Seed < 1.f);
			TestTrue(FString::Printf(TEXT("%s: subtle to bold (%.2f)"), *Name, Pattern.Amount), Pattern.Amount >= 0.25f && Pattern.Amount <= 0.8f);
			if (Rarity == EWeaponRarity::Common)
			{
				TestTrue(FString::Printf(TEXT("Common amount 0.3-0.5 (%.2f)"), Pattern.Amount), Pattern.Amount >= 0.3f && Pattern.Amount <= 0.5f);
			}
			if (Rarity == EWeaponRarity::Legendary)
			{
				TestTrue(FString::Printf(TEXT("Legendary amount 0.6+ (%.2f)"), Pattern.Amount), Pattern.Amount >= 0.6f);
			}
			const TArray<FLinearColor> Palette = UWeaponModelComponent::PatternPalette(Look);
			TestTrue(FString::Printf(TEXT("%s: the tint is the maker's palette's (%s)"), *Name, *Pattern.Tint.ToString()),
				Palette.ContainsByPredicate([&Pattern](const FLinearColor& Color) { return Color.Equals(Pattern.Tint, 0.001f); }));
			const FLinearColor* Shell = Look.Colors.Find(TEXT("GunPolymerSand"));
			if (Shell)
			{
				TestTrue(FString::Printf(TEXT("%s: the tint shows on the shell"), *Name),
					FMath::Abs(Pattern.Tint.GetLuminance() - Shell->GetLuminance()) >= 0.02f);
			}
		}
		if (Rarity != EWeaponRarity::Legendary)
		{
			TestEqual(FString::Printf(TEXT("%s rolls each of its kinds"), *Name), Seen.Num(), AllowedKinds(Rarity).Num());
		}
		else
		{
			TestTrue(TEXT("Legendaries roll clean bold"), Seen.Contains(UWeaponModelComponent::PatternBold));
			TestTrue(TEXT("...and their makers' own designs"), Seen.Num() > 1);
		}
	}

	// Each maker's palette is its own: its trim, its shell's shades.
	TSet<FString> Darks;
	for (const FWeaponMaker& Maker : WeaponMakers::All())
	{
		FWeaponInstanceData Gun = PatternGun(Definition, EWeaponRarity::Rare, 77);
		Gun.Maker = Maker.Key;
		const FWeaponLook Look = WeaponParts::Pick(Gun);
		const TArray<FLinearColor> Palette = UWeaponModelComponent::PatternPalette(Look);
		TestEqual(FString::Printf(TEXT("%s: five tints"), *Maker.Key.ToString()), Palette.Num(), 5);
		if (const FLinearColor* Trim = Look.Colors.Find(TEXT("GunBrass")))
		{
			TestTrue(FString::Printf(TEXT("%s: its accent is its trim"), *Maker.Key.ToString()), Palette.Num() > 2 && Palette[2].Equals(*Trim, 0.001f));
		}
		if (!Palette.IsEmpty())
		{
			Darks.Add(Palette[0].ToString());
		}
		TestTrue(FString::Printf(TEXT("%s has a legendary design"), *Maker.Key.ToString()), UWeaponModelComponent::MakerPatternKind(Maker.Key) > 0.f);
	}
	TestTrue(TEXT("The makers' palettes differ"), Darks.Num() > 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponPatternDeterminismTest, "Looter.Weapons.Pattern.Determinism",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWeaponPatternDeterminismTest::RunTest(const FString& Parameters)
{
	// The same gun always wears the same pattern (nothing saved); other seeds wear others; a forced kind keeps the roll.
	UWeaponDefinition* Definition = LoadPatternGun(TEXT("DA_AssaultRifle"));
	if (!TestNotNull(TEXT("DA_AssaultRifle"), Definition))
	{
		return false;
	}
	TSet<FString> Different;
	for (const EWeaponRarity Rarity : PatternRarities)
	{
		for (int32 Seed : { 1, 42, 9001, -77, 123456 })
		{
			const FWeaponInstanceData Gun = PatternGun(Definition, Rarity, Seed);
			const FWeaponLook Look = WeaponParts::Pick(Gun);
			const FWeaponPattern First = UWeaponModelComponent::PickPattern(Gun, Look);
			const FWeaponPattern Again = UWeaponModelComponent::PickPattern(Gun, WeaponParts::Pick(Gun));
			const FString Case = FString::Printf(TEXT("%s seed %d"), *UEnum::GetValueAsString(Rarity), Seed);
			TestEqual(Case + TEXT(": kind"), Again.Kind, First.Kind);
			TestEqual(Case + TEXT(": scale"), Again.Scale, First.Scale);
			TestEqual(Case + TEXT(": seed"), Again.Seed, First.Seed);
			TestEqual(Case + TEXT(": amount"), Again.Amount, First.Amount);
			TestTrue(Case + TEXT(": tint"), Again.Tint.Equals(First.Tint, KINDA_SMALL_NUMBER));
			Different.Add(FString::Printf(TEXT("%.0f %.3f %.3f"), First.Kind, First.Seed, First.Amount));

			// Forced (Looter.Weapons.Pattern.Force): that kind, the same draws, at least half strength; 0 shows none.
			for (int32 Force = 1; Force < UWeaponModelComponent::PatternKindCount; ++Force)
			{
				const FWeaponPattern Forced = UWeaponModelComponent::PickPattern(Gun, Look, Force);
				TestEqual(Case + FString::Printf(TEXT(": forced %d"), Force), Forced.Kind, static_cast<float>(Force));
				TestEqual(Case + TEXT(": forcing keeps the seed"), Forced.Seed, First.Seed);
				TestTrue(Case + TEXT(": a forced kind shows"), Forced.Amount >= 0.5f && Forced.Scale >= 4.f && Forced.Scale <= 30.f);
			}
			const FWeaponPattern None = UWeaponModelComponent::PickPattern(Gun, Look, 0);
			TestTrue(Case + TEXT(": forced none is plain"), None.Kind == 0.f && None.Amount == 0.f);
		}
	}
	TestTrue(TEXT("Different seeds wear different patterns"), Different.Num() > 10);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponPatternGlowTest, "Looter.Weapons.Pattern.Glow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWeaponPatternGlowTest::RunTest(const FString& Parameters)
{
	// Glows never take a pattern: the rarity accent (and a part's own copy of it), every Gun*Glow and the reticle.
	const FName Rarity(TEXT("GunAccentGlow"));
	for (const TCHAR* Slot : { TEXT("GunAccentGlow"), TEXT("GunAccentGlow_TripoBullpupBodyStandard"), TEXT("GunEmberGlow"), TEXT("GunCyanGlow"),
		TEXT("GunAmmoGlow"), TEXT("GunHeatGlow"), TEXT("GunSoulGlow"), TEXT("GunSaltGlow"), TEXT("GunReticle") })
	{
		TestTrue(FString::Printf(TEXT("%s is a glow"), Slot), UWeaponModelComponent::IsGlowSlot(Slot, Rarity));
	}
	for (const TCHAR* Slot : { TEXT("GunPolymerSand"), TEXT("GunPolymerSand_TripoBullpupMagazine30"), TEXT("GunSteelBlued"), TEXT("GunBrass"),
		TEXT("GunSalvage"), TEXT("GunSaltCrystal"), TEXT("GunTripoBullpupMagazine30") })
	{
		TestFalse(FString::Printf(TEXT("%s takes the pattern"), Slot), UWeaponModelComponent::IsGlowSlot(Slot, Rarity));
	}
	TestFalse(TEXT("No slot, no glow"), UWeaponModelComponent::IsGlowSlot(NAME_None, Rarity));

	// A plain pattern writes zeros; a shown one its values, each part's seed moved on by its index.
	UStaticMeshComponent* Part = NewObject<UStaticMeshComponent>(GetTransientPackage());
	UWeaponModelComponent::ShowPattern(*Part, FWeaponPattern(), 3);
	for (int32 Index = UWeaponModelComponent::PatternKindDataIndex; Index <= UWeaponModelComponent::PatternAmountDataIndex; ++Index)
	{
		TestEqual(FString::Printf(TEXT("Plain: data %d is zero"), Index), PatternDataAt(*Part, Index), 0.f);
	}
	FWeaponPattern Shown;
	Shown.Kind = UWeaponModelComponent::PatternStripes;
	Shown.Scale = 12.f;
	Shown.Seed = 0.25f;
	Shown.Tint = FLinearColor(0.1f, 0.2f, 0.3f);
	Shown.Amount = 0.45f;
	UWeaponModelComponent::ShowPattern(*Part, Shown, 0);
	TestEqual(TEXT("Kind at 29"), PatternDataAt(*Part, 29), Shown.Kind);
	TestEqual(TEXT("Scale at 30"), PatternDataAt(*Part, 30), Shown.Scale);
	TestTrue(TEXT("Seed at 31"), FMath::IsNearlyEqual(PatternDataAt(*Part, 31), Shown.Seed));
	TestTrue(TEXT("Tint at 32-34"), FMath::IsNearlyEqual(PatternDataAt(*Part, 32), 0.1f) && FMath::IsNearlyEqual(PatternDataAt(*Part, 33), 0.2f)
		&& FMath::IsNearlyEqual(PatternDataAt(*Part, 34), 0.3f));
	TestTrue(TEXT("Amount at 35"), FMath::IsNearlyEqual(PatternDataAt(*Part, 35), Shown.Amount));
	UWeaponModelComponent::ShowPattern(*Part, Shown, 1);
	TestFalse(TEXT("The next part's seed differs"), FMath::IsNearlyEqual(PatternDataAt(*Part, 31), Shown.Seed));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponPatternPartsTest, "Looter.Weapons.Pattern.Parts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWeaponPatternPartsTest::RunTest(const FString& Parameters)
{
	// Every family at every rarity: each part carries the gun's pattern, but a part that is wholly glow stays plain.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	AActor* Holder = WorldWrapper.GetTestWorld()->SpawnActor<AActor>();
	UWeaponModelComponent* Model = NewObject<UWeaponModelComponent>(Holder);
	Holder->SetRootComponent(Model);
	Model->RegisterComponent();
	int32 Patterned = 0;
	for (const TCHAR* Asset : PatternGuns)
	{
		UWeaponDefinition* Definition = LoadPatternGun(Asset);
		if (!TestNotNull(Asset, Definition))
		{
			continue;
		}
		for (const EWeaponRarity Rarity : PatternRarities)
		{
			const FWeaponInstanceData Gun = PatternGun(Definition, Rarity, 31);
			const FString Case = FString::Printf(TEXT("%s %s"), Asset, *UEnum::GetValueAsString(Rarity));
			if (!TestTrue(Case + TEXT(" assembles"), Model->Assemble(Gun)))
			{
				continue;
			}
			const FWeaponPattern Expected = UWeaponModelComponent::PatternFor(Gun, WeaponParts::Pick(Gun));
			for (const UStaticMeshComponent* Part : Model->GetParts())
			{
				const FString PartName = Part->GetStaticMesh() ? Part->GetStaticMesh()->GetName() : TEXT("?");
				TestTrue(FString::Printf(TEXT("%s %s: carries the pattern's data"), *Case, *PartName),
					Part->GetCustomPrimitiveData().Data.Num() > UWeaponModelComponent::PatternAmountDataIndex);
				if (UWeaponModelComponent::IsWhollyGlow(*Part, Definition->RarityGlowSlot))
				{
					TestEqual(FString::Printf(TEXT("%s %s: a glow takes no pattern"), *Case, *PartName), PatternDataAt(*Part, 29), 0.f);
					TestEqual(FString::Printf(TEXT("%s %s: ...nor amount"), *Case, *PartName), PatternDataAt(*Part, 35), 0.f);
					continue;
				}
				TestEqual(FString::Printf(TEXT("%s %s: the gun's kind"), *Case, *PartName), PatternDataAt(*Part, 29), Expected.Kind);
				TestTrue(FString::Printf(TEXT("%s %s: the gun's amount"), *Case, *PartName), FMath::IsNearlyEqual(PatternDataAt(*Part, 35), Expected.Amount));
				TestTrue(FString::Printf(TEXT("%s %s: the gun's scale"), *Case, *PartName), FMath::IsNearlyEqual(PatternDataAt(*Part, 30), Expected.Scale));
				TestTrue(FString::Printf(TEXT("%s %s: the gun's tint"), *Case, *PartName), FMath::IsNearlyEqual(PatternDataAt(*Part, 32), Expected.Tint.R)
					&& FMath::IsNearlyEqual(PatternDataAt(*Part, 33), Expected.Tint.G) && FMath::IsNearlyEqual(PatternDataAt(*Part, 34), Expected.Tint.B));
				const float Seed = PatternDataAt(*Part, 31);
				TestTrue(FString::Printf(TEXT("%s %s: its seed within 0-1"), *Case, *PartName), Seed >= 0.f && Seed < 1.f);
				Patterned += Expected.Kind > 0.f ? 1 : 0;
			}
		}
	}
	const IConsoleVariable* PatternOn = IConsoleManager::Get().FindConsoleVariable(TEXT("Looter.Weapons.Pattern"));
	const IConsoleVariable* PatternForce = IConsoleManager::Get().FindConsoleVariable(TEXT("Looter.Weapons.Pattern.Force"));
	if (PatternOn && PatternOn->GetBool() && (!PatternForce || PatternForce->GetInt() != 0))
	{
		TestTrue(TEXT("Some parts wore a pattern"), Patterned > 0);
	}
	Model->Clear();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponPatternElementSpreadTest, "Looter.Weapons.Pattern.ElementSpread",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWeaponPatternElementSpreadTest::RunTest(const FString& Parameters)
{
	// The element part wears its finish whole; every other part (but a wholly glowing one) the same finish at the spread's share.
	const float Spread = UWeaponModelComponent::ElementSpread();
	TestTrue(TEXT("The spread is a share"), Spread >= 0.f && Spread <= 1.f);

	// Scaled on its own: the amount and the glow by the share, the kind and colours kept; zero clears it.
	UStaticMeshComponent* Loose = NewObject<UStaticMeshComponent>(GetTransientPackage());
	const FWeaponElementFinish Ember = UWeaponModelComponent::ElementFinishFor(ELooterElement::Ember);
	UWeaponModelComponent::ShowElementFinish(*Loose, ELooterElement::Ember, 0.25f);
	TestEqual(TEXT("Stains keep the kind"), PatternDataAt(*Loose, UWeaponModelComponent::ElementFinishDataIndex), Ember.Kind);
	TestTrue(TEXT("Stains at a quarter"), FMath::IsNearlyEqual(PatternDataAt(*Loose, UWeaponModelComponent::ElementTintDataIndex + 3), Ember.Amount * 0.25f));
	TestTrue(TEXT("Stains glow at a quarter"), FMath::IsNearlyEqual(PatternDataAt(*Loose, UWeaponModelComponent::ElementGlowDataIndex + 3), Ember.GlowStrength * 0.25f));
	UWeaponModelComponent::ShowElementFinish(*Loose, ELooterElement::Ember, 0.f);
	TestEqual(TEXT("No share, no stains"), PatternDataAt(*Loose, UWeaponModelComponent::ElementFinishDataIndex), 0.f);

	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	AActor* Holder = WorldWrapper.GetTestWorld()->SpawnActor<AActor>();
	UWeaponModelComponent* Model = NewObject<UWeaponModelComponent>(Holder);
	Holder->SetRootComponent(Model);
	Model->RegisterComponent();
	int32 Stained = 0;
	for (const TCHAR* Asset : PatternGuns)
	{
		UWeaponDefinition* Definition = LoadPatternGun(Asset);
		if (!TestNotNull(Asset, Definition))
		{
			continue;
		}
		for (const ELooterElement Element : PatternElements)
		{
			const FString Case = FString::Printf(TEXT("%s %s"), Asset, *UEnum::GetValueAsString(Element));
			FWeaponInstanceData Gun = PatternGun(Definition, EWeaponRarity::Legendary, 11);
			TArray<FName> Keys;
			if (!KeysWithElementPart(Gun, Element, Keys))
			{
				AddInfo(FString::Printf(TEXT("%s: no such part"), *Case));
				continue;
			}
			Gun.Parts = Keys;
			if (!TestTrue(Case + TEXT(" assembles"), Model->Assemble(Gun)))
			{
				continue;
			}
			if (!TestTrue(Case + TEXT(" has its element part"), Model->GetElementPart() != nullptr))
			{
				continue;
			}
			const FWeaponElementFinish Finish = UWeaponModelComponent::ElementFinishFor(Element);
			for (const UStaticMeshComponent* Part : Model->GetParts())
			{
				const FString PartName = Part->GetStaticMesh() ? Part->GetStaticMesh()->GetName() : TEXT("?");
				const float Kind = PatternDataAt(*Part, UWeaponModelComponent::ElementFinishDataIndex);
				const float Amount = PatternDataAt(*Part, UWeaponModelComponent::ElementTintDataIndex + 3);
				const float Glow = PatternDataAt(*Part, UWeaponModelComponent::ElementGlowDataIndex + 3);
				if (Part == Model->GetElementPart())
				{
					TestEqual(FString::Printf(TEXT("%s: the element part's own finish"), *Case), Kind, Finish.Kind);
					TestTrue(FString::Printf(TEXT("%s: whole"), *Case), FMath::IsNearlyEqual(Amount, Finish.Amount));
				}
				else if (UWeaponModelComponent::IsWhollyGlow(*Part, Definition->RarityGlowSlot) || Finish.Kind <= 0.f || Spread <= 0.f)
				{
					TestEqual(FString::Printf(TEXT("%s %s: no stains"), *Case, *PartName), Kind, 0.f);
				}
				else
				{
					TestEqual(FString::Printf(TEXT("%s %s: stained with its element"), *Case, *PartName), Kind, Finish.Kind);
					TestTrue(FString::Printf(TEXT("%s %s: lightly (%.3f)"), *Case, *PartName, Amount), FMath::IsNearlyEqual(Amount, Finish.Amount * Spread));
					TestTrue(FString::Printf(TEXT("%s %s: dimly (%.3f)"), *Case, *PartName, Glow), FMath::IsNearlyEqual(Glow, Finish.GlowStrength * Spread));
					TestTrue(FString::Printf(TEXT("%s %s: lighter than the element part"), *Case, *PartName), Amount <= Finish.Amount);
					++Stained;
				}
			}
		}
	}
	if (Spread > 0.f && UWeaponModelComponent::ElementFinishFor(ELooterElement::Ember).Kind > 0.f)
	{
		TestTrue(TEXT("Some parts took stains"), Stained > 0);
	}
	Model->Clear();
	return true;
}

#endif

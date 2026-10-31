#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Combat/HealthComponent.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Shields/PlayerShieldComponent.h"
#include "Shields/ShieldDefinition.h"
#include "Shields/ShieldRules.h"
#include "Shields/ShieldText.h"
#include "Tests/AutomationCommon.h"
#include "Tests/RecoveryTestWorld.h"
#include "UI/Cards/WardCardRules.h"
#include "UI/Cards/WardCardWidget.h"
#include "UI/World/ShieldLabelWidget.h"
#include "Weapons/WeaponDefinition.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/Character.h"

// The ward card (the user, 2026-10-31: "Wards should have a similar UI to the weapons"): its rules (rows, arrows, lines,
// keys), the widget, and the ward's label on the ground opening it when looked at, as a gun's label opens the gun card.

namespace
{
	constexpr EAutomationTestFlags WardCardTestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;

	FShieldStats CardTestStats(float Capacity, float Delay, float Rate)
	{
		FShieldStats Stats;
		Stats.Capacity = Capacity;
		Stats.RechargeDelay = Delay;
		Stats.RechargeRate = Rate;
		return Stats;
	}

	TArray<FString> Words(const TArray<GunCardRules::FKeyHint>& Hints)
	{
		TArray<FString> Out;
		for (const GunCardRules::FKeyHint& Hint : Hints)
		{
			Out.Add(Hint.Cap.IsEmpty() ? Hint.Word : FString::Printf(TEXT("[%s] %s"), *Hint.Cap, *Hint.Word));
		}
		return Out;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWardCardRulesTest, "Looter.UI.WardCard.Rules", WardCardTestFlags)

bool FWardCardRulesTest::RunTest(const FString& Parameters)
{
	using WardCardRules::EStat;
	using LoadoutRules::EChange;
	// The rows: capacity, recharge delay, recharge rate, in ShieldText's numbers.
	const FShieldStats Mine = CardTestStats(52.f, 2.9f, 15.f);
	TestEqual(TEXT("Three stat rows"), WardCardRules::CardStats().Num(), 3);
	TestEqual(TEXT("Capacity row"), WardCardRules::StatLabel(EStat::Capacity) + TEXT(" ") + WardCardRules::StatValueText(EStat::Capacity, Mine), FString(TEXT("Capacity 52")));
	TestEqual(TEXT("Delay row"), WardCardRules::StatLabel(EStat::Delay) + TEXT(" ") + WardCardRules::StatValueText(EStat::Delay, Mine), FString(TEXT("Recharge delay 2.9 s")));
	TestEqual(TEXT("Rate row"), WardCardRules::StatLabel(EStat::Rate) + TEXT(" ") + WardCardRules::StatValueText(EStat::Rate, Mine), FString(TEXT("Recharge rate 15 / s")));

	// The arrows against the ward worn: more holds better, a shorter wait better, a faster fill better; within 3% the same.
	const FShieldStats Worn = CardTestStats(40.f, 3.2f, 18.f);
	TestTrue(TEXT("More capacity: better"), WardCardRules::CompareStat(EStat::Capacity, Mine, Worn) == EChange::Better);
	TestTrue(TEXT("A shorter wait: better"), WardCardRules::CompareStat(EStat::Delay, Mine, Worn) == EChange::Better);
	TestTrue(TEXT("A slower fill: worse"), WardCardRules::CompareStat(EStat::Rate, Mine, Worn) == EChange::Worse);
	TestTrue(TEXT("Within 3%: the same"), WardCardRules::CompareStat(EStat::Capacity, CardTestStats(40.6f, 3.2f, 18.f), Worn) == EChange::Same);
	TestTrue(TEXT("...and a wait a twentieth shorter is the same"), WardCardRules::CompareStat(EStat::Delay, CardTestStats(40.f, 3.16f, 18.f), Worn) == EChange::Same);
	TestEqual(TEXT("The arrows' words"), WardCardRules::ArrowText(EChange::Better) + WardCardRules::ArrowText(EChange::Worse) + WardCardRules::ArrowText(EChange::Same),
		FString(TEXT("▲▼")));

	// The build and trait rows, and the lines: the trait's effect first, then the build's lean.
	UShieldDefinition* Definition = NewObject<UShieldDefinition>();
	const FShieldInstanceData Plain = ShieldRules::MakeWith(Definition, EWeaponRarity::Common, 1, 5, EShieldBuild::Standard, EShieldTrait::None);
	const FShieldInstanceData Reaping = ShieldRules::MakeWith(Definition, EWeaponRarity::Epic, 4, 6, EShieldBuild::Quickset, EShieldTrait::Harvest);
	TestTrue(TEXT("A plain ward: its build row alone"), WardCardRules::ExtraRows(Plain).Num() == 1 && WardCardRules::ExtraRows(Plain)[0].Value == TEXT("Ward Box"));
	const TArray<WardCardRules::FExtraRow> Rows = WardCardRules::ExtraRows(Reaping);
	TestTrue(TEXT("A trait: its row too, in the trait's colour"), Rows.Num() == 2 && Rows[1].bTrait && Rows[1].Value == TEXT("Harvest"));
	TestTrue(TEXT("A plain ward has no lines"), WardCardRules::Lines(Plain).IsEmpty());
	const TArray<FString> Lines = WardCardRules::Lines(Reaping);
	TestTrue(TEXT("Trait first, then the build"), Lines.Num() == 2 && Lines[0] == ShieldText::TraitLine(Reaping) && Lines[1] == WardCardRules::BuildLine(EShieldBuild::Quickset));
	TestEqual(TEXT("Bulwark's lean, said"), WardCardRules::BuildLine(EShieldBuild::Bulwark), FString(TEXT("Bulwark: holds 20% more, waits 10% longer, refills 20% slower.")));
	TestEqual(TEXT("Quickset's lean, said"), WardCardRules::BuildLine(EShieldBuild::Quickset), FString(TEXT("Quickset: holds 15% less, wakes 30% sooner, refills 15% faster.")));
	TestTrue(TEXT("A Ward Box has no lean"), WardCardRules::BuildLine(EShieldBuild::Standard).IsEmpty());
	TestEqual(TEXT("Over the name: its rarity's word"), WardCardRules::TopWord(Reaping), FString(TEXT("EPIC WARD")));
	TestEqual(TEXT("Better than the worn: UPGRADE"), WardCardRules::VerdictWord(Reaping, &Plain), FString(TEXT("UPGRADE")));
	TestTrue(TEXT("...worse, or none worn: nothing"), WardCardRules::VerdictWord(Plain, &Reaping).IsEmpty() && WardCardRules::VerdictWord(Reaping, nullptr).IsEmpty());

	// The keys, as a gun's: none worn a tap wears; worn a tap to the backpack and a hold wears; full a tap swaps.
	FWardCardKeys Keys;
	TestTrue(TEXT("None worn"), Words(WardCardRules::KeyHints(Keys)) == TArray<FString>({ TEXT("[E] WEAR") }));
	Keys.bWorn = true;
	TestTrue(TEXT("Worn, room"), Words(WardCardRules::KeyHints(Keys)) == TArray<FString>({ TEXT("[E] TO BACKPACK"), TEXT("[HOLD E] WEAR") }));
	Keys.bBackpackFull = true;
	TestTrue(TEXT("Worn, the backpack full"), Words(WardCardRules::KeyHints(Keys)) == TArray<FString>({ TEXT("[E] SWAP"), TEXT("BACKPACK FULL") }));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWardCardWidgetTest, "Looter.UI.WardCard.Widget", WardCardTestFlags)

bool FWardCardWidgetTest::RunTest(const FString& Parameters)
{
	// The widget: a ward's card against the worn one, its intro counting the capacity up, keys only on a world card.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWardCardWidget* Card = CreateWidget<UWardCardWidget>(WorldWrapper.GetTestWorld(), UWardCardWidget::StaticClass());
	if (!TestNotNull(TEXT("A ward card"), Card))
	{
		return false;
	}
	Card->TakeWidget();
	UShieldDefinition* Definition = NewObject<UShieldDefinition>();
	const FShieldInstanceData Worn = ShieldRules::MakeWith(Definition, EWeaponRarity::Common, 1, 5, EShieldBuild::Standard, EShieldTrait::None);
	const FShieldInstanceData Found = ShieldRules::MakeWith(Definition, EWeaponRarity::Legendary, 1, 6, EShieldBuild::Bulwark, EShieldTrait::SaintsGrace);
	TestTrue(TEXT("A new ward: true (its intro may play)"), Card->SetWard(Found, &Worn, EWardCardMode::Loadout));
	TestFalse(TEXT("...the same again: false"), Card->SetWard(Found, &Worn, EWardCardMode::Loadout));
	TestEqual(TEXT("Its name"), Card->GetNameText().ToString(), ShieldText::Name(Found).ToUpper());
	TestEqual(TEXT("...its rarity over it"), Card->GetTopText().ToString(), FString(TEXT("LEGENDARY WARD")));
	TestTrue(TEXT("Compared: a bigger ward's capacity arrow is up"), Card->HasCompare() && Card->GetStatArrowText(0).ToString() == TEXT("▲"));
	TestEqual(TEXT("...an upgrade"), Card->GetVerdictText().ToString(), FString(TEXT("UPGRADE")));
	TestEqual(TEXT("Its lines: trait, then build"), Card->GetLines().Num(), 2);
	Card->SetKeys(FWardCardKeys());
	TestTrue(TEXT("In the loadout: no keys"), Card->GetKeyCaps().IsEmpty());

	Card->PlayIntro(false);
	TestEqual(TEXT("The intro starts the capacity at nothing"), Card->GetStatValueText(0).ToString(), FString(TEXT("0")));
	Card->AdvanceIntro(1.f);
	TestEqual(TEXT("...and counts it up to the ward's"), Card->GetStatValueText(0).ToString(), ShieldText::CapacityString(Found.Stats));
	TestFalse(TEXT("...done"), Card->IsIntroPlaying());

	TestTrue(TEXT("A world card is another card (its intro again)"), Card->SetWard(Found, nullptr, EWardCardMode::World));
	TestFalse(TEXT("...nothing worn: no compare"), Card->HasCompare());
	Card->SetKeys(FWardCardKeys());
	TestTrue(TEXT("...its keys"), Card->GetKeyWords() == TArray<FString>({ TEXT("WEAR") }));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWardWorldCardTest, "Looter.UI.WorldCard.Ward", WardCardTestFlags)

bool FWardWorldCardTest::RunTest(const FString& Parameters)
{
	// A ward on the ground: from afar its name; looked at, the ward card over it with the take keys from the ward worn and the
	// backpack's shared room (a gun in the backpack fills it as a ward would).
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	UHealthComponent* Health = nullptr;
	ACharacter* Player = RecoveryTestWorld::SpawnPlayer(World, FVector::ZeroVector, Health);
	UPlayerShieldComponent* Shield = Player ? UPlayerShieldComponent::FindOrAdd(*Player) : nullptr;
	if (!TestNotNull(TEXT("A ward component"), Shield))
	{
		return false;
	}
	UWeaponManagerComponent* Inventory = NewObject<UWeaponManagerComponent>(Player);
	Inventory->RegisterComponent();
	Inventory->BackpackCapacity = 1;
	UShieldDefinition* Definition = NewObject<UShieldDefinition>();
	const FShieldInstanceData Ground = ShieldRules::MakeWith(Definition, EWeaponRarity::Epic, 2, 8, EShieldBuild::Standard, EShieldTrait::Barbwire);

	UShieldLabelWidget* Label = CreateWidget<UShieldLabelWidget>(World, UShieldLabelWidget::StaticClass());
	if (!TestNotNull(TEXT("A ward's label"), Label))
	{
		return false;
	}
	Label->SetWard(Ground, nullptr);
	Label->TakeWidget();
	TestFalse(TEXT("Not looked at: no card"), Label->IsCardShown());
	TestEqual(TEXT("...its name"), Label->GetNameText().ToString(), ShieldText::Name(Ground).ToUpper());
	TestEqual(TEXT("...its trait's name under it"), Label->GetTraitText().ToString(), FString(TEXT("BARBWIRE")));

	Label->SetFocused(true);
	Label->UpdateCard(Shield, nullptr);
	UWardCardWidget* Card = Label->GetCard();
	TestTrue(TEXT("Looked at: the ward card, a world card"), Label->IsCardShown() && Card && Card->GetMode() == EWardCardMode::World);
	TestTrue(TEXT("None worn: nothing compared, a tap wears"), Card && !Card->HasCompare() && Card->GetKeyWords() == TArray<FString>({ TEXT("WEAR") }));

	Shield->Equip(ShieldRules::MakeWith(Definition, EWeaponRarity::Common, 1, 9, EShieldBuild::Standard, EShieldTrait::None), true);
	Label->UpdateCard(Shield, nullptr);
	TestTrue(TEXT("One worn: compared with it"), Card && Card->HasCompare());
	TestTrue(TEXT("...a tap to the backpack, a hold wears"), Card && Card->GetKeyWords() == TArray<FString>({ TEXT("TO BACKPACK"), TEXT("WEAR") })
		&& Card->GetKeyCaps() == TArray<FString>({ TEXT("E"), TEXT("HOLD E") }));

	FWeaponInstanceData Gun;
	Gun.Definition = NewObject<UWeaponDefinition>();
	TestTrue(TEXT("A gun fills the backpack's one place"), Inventory->AddToBackpack(Gun) && !Inventory->HasBackpackRoom());
	Label->UpdateCard(Shield, nullptr);
	TestTrue(TEXT("...the tap swaps, the backpack full"), Card && Card->GetKeyWords() == TArray<FString>({ TEXT("SWAP"), TEXT("BACKPACK FULL") }));

	Label->SetFocused(false);
	TestFalse(TEXT("Looked away: no card"), Label->IsCardShown());
	return true;
}

#endif

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "UI/Cards/GunCardRules.h"
#include "UI/Cards/GunCardStyle.h"
#include "UI/Cards/GunCardWidget.h"
#include "UI/Cards/LooterCardBackdrop.h"
#include "Audio/LooterSoundCues.h"
#include "Weapons/WeaponDefinition.h"
#include "Weapons/WeaponTypes.h"
#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"

// The gun card ("Wide Sweep", the user's pick 2026-10-28): its rules, its shapes and the card itself.

namespace
{
	FWeaponInstanceData MakeGun(UWeaponDefinition* Definition, FName Maker, EWeaponRarity Rarity, float Damage, int32 Pellets, float FireRate,
		int32 Magazine, float Reload, int32 Seed)
	{
		FWeaponInstanceData Gun;
		Gun.Definition = Definition;
		Gun.Maker = Maker;
		Gun.Rarity = Rarity;
		Gun.Level = 6;
		Gun.Seed = Seed;
		Gun.Stats.Damage = Damage;
		Gun.Stats.PelletsPerShot = Pellets;
		Gun.Stats.FireRate = FireRate;
		Gun.Stats.MagazineSize = Magazine;
		Gun.Stats.ReloadTime = Reload;
		return Gun;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunCardRulesTest, "Looter.UI.GunCard.Rules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunCardRulesTest::RunTest(const FString& Parameters)
{
	// The stat list: six rows, a scoped gun's zoom a seventh.
	FWeaponStats Plain;
	Plain.Damage = 21.f;
	Plain.PelletsPerShot = 8;
	Plain.FireRate = 66.f;
	Plain.MagazineSize = 10;
	Plain.ReloadTime = 1.7f;
	Plain.Handling = 0.7f;
	Plain.Zoom = 1.2f;
	const TArray<LoadoutRules::EStat> Stats = GunCardRules::CardStats(Plain);
	if (TestEqual(TEXT("Six rows"), Stats.Num(), 6))
	{
		TestEqual(TEXT("Damage first"), GunCardRules::StatLabel(Stats[0]), FString(TEXT("Damage")));
		TestEqual(TEXT("...magazine last"), GunCardRules::StatLabel(Stats[5]), FString(TEXT("Magazine")));
	}
	TestEqual(TEXT("A shotgun's damage with its pellets"), GunCardRules::StatValueText(LoadoutRules::EStat::Damage, Plain), FString(TEXT("21 × 8")));
	TestEqual(TEXT("Fire rate in rpm"), GunCardRules::StatValueText(LoadoutRules::EStat::FireRate, Plain), FString(TEXT("66 rpm")));
	TestEqual(TEXT("Handling in percent"), GunCardRules::StatValueText(LoadoutRules::EStat::Handling, Plain), FString(TEXT("70%")));
	TestEqual(TEXT("Reload in seconds"), GunCardRules::StatValueText(LoadoutRules::EStat::Reload, Plain), FString(TEXT("1.7 s")));
	FWeaponStats Scoped = Plain;
	Scoped.Zoom = 6.f;
	TestEqual(TEXT("A scope adds no row (zoom is no card stat since 2026-10-29)"), GunCardRules::CardStats(Scoped).Num(), 6);
	TestEqual(TEXT("...as 6x"), GunCardRules::StatValueText(LoadoutRules::EStat::Zoom, Scoped), FString(TEXT("6×")));
	TestEqual(TEXT("An upgrade boasts"), GunCardRules::VerdictWord(LoadoutRules::EVerdict::Upgrade), FString(TEXT("UPGRADE")));
	TestTrue(TEXT("Similar says nothing"), GunCardRules::VerdictWord(LoadoutRules::EVerdict::Similar).IsEmpty());
	TestTrue(TEXT("Weaker says nothing (the arrows do)"), GunCardRules::VerdictWord(LoadoutRules::EVerdict::Weaker).IsEmpty());

	// The take keys, as the weapon manager takes loot.
	FGunCardKeys Keys;
	Keys.Key = TEXT("E");
	TArray<GunCardRules::FKeyHint> Hints = GunCardRules::KeyHints(Keys);
	if (TestEqual(TEXT("A free slot: take and equip"), Hints.Num(), 2))
	{
		TestEqual(TEXT("...E"), Hints[0].Cap, FString(TEXT("E")));
		TestEqual(TEXT("...TAKE"), Hints[0].Word, FString(TEXT("TAKE")));
		TestEqual(TEXT("...HOLD E EQUIP"), Hints[1].Word, FString(TEXT("EQUIP")));
	}
	Keys.bSlotsFull = true;
	Hints = GunCardRules::KeyHints(Keys);
	if (TestEqual(TEXT("Slots full: tap and hold"), Hints.Num(), 2))
	{
		TestEqual(TEXT("...the tap stashes it"), Hints[0].Word, FString(TEXT("TO BACKPACK")));
		TestEqual(TEXT("...the hold's cap"), Hints[1].Cap, FString(TEXT("HOLD E")));
		TestTrue(TEXT("...fills as the hold runs"), Hints[1].bHold);
	}
	Keys.bBackpackFull = true;
	Hints = GunCardRules::KeyHints(Keys);
	if (TestEqual(TEXT("Both full: swap and a warning"), Hints.Num(), 2))
	{
		TestEqual(TEXT("...the tap swaps"), Hints[0].Word, FString(TEXT("SWAP")));
		TestTrue(TEXT("...the warning has no cap"), Hints[1].Cap.IsEmpty());
	}

	// The intro: snaps open with an overshoot, rows one after another, the count, the gem's pop.
	TestNearlyEqual(TEXT("Starts folded"), GunCardRules::UnfoldScale(0.f), 0.15f, 0.001f);
	TestTrue(TEXT("Overshoots"), GunCardRules::UnfoldScale(0.08f) > 1.f);
	TestNearlyEqual(TEXT("Settles whole"), GunCardRules::UnfoldScale(GunCardRules::UnfoldSeconds), 1.f, 0.001f);
	TestTrue(TEXT("The first row comes in before the second"), GunCardRules::RowShare(0.12f, 0) > GunCardRules::RowShare(0.12f, 1));
	TestNearlyEqual(TEXT("Every row in by the end"), GunCardRules::RowShare(GunCardRules::IntroSeconds, 6), 1.f, 0.001f);
	TestNearlyEqual(TEXT("The count done"), GunCardRules::CountShare(GunCardRules::CountSeconds), 1.f, 0.001f);
	TestNearlyEqual(TEXT("The gem at rest at first"), GunCardRules::GemPop(0.f), 1.f, 0.001f);
	TestTrue(TEXT("...it pops as the card lands"), GunCardRules::GemPop(0.38f) > 1.05f);

	// The tag: eleven corners (shoulders, notch, point), its gem over the notch.
	const TArray<FVector2f> Tag = LooterBackdrop::TagOutline(FVector2f(470.f, 500.f));
	TestEqual(TEXT("The tag's corners"), Tag.Num(), 11);
	TestNearlyEqual(TEXT("...its point at the foot's middle"), Tag[9].X, 235.f, 0.01f);
	TestNearlyEqual(TEXT("...the gem centred"), LooterBackdrop::TagGemCenter(FVector2f(470.f, 500.f)).X, 235.f, 0.01f);

	// The shapes.
	const TArray<FVector2f> Card = LooterBackdrop::CardOutline(FVector2f(420.f, 200.f));
	TestEqual(TEXT("A card has its two cuts (six corners)"), Card.Num(), 6);
	TestEqual(TEXT("A row has one (five)"), LooterBackdrop::RowOutline(FVector2f(470.f, 52.f)).Num(), 5);
	const TArray<FVector2f> Sweep = LooterBackdrop::SweepOutline(420.f);
	TestNearlyEqual(TEXT("The sweep ends at the mockup's 276 px"), LooterBackdrop::SweepLength(420.f), 276.f, 0.5f);
	TestNearlyEqual(TEXT("...its curve reaches the foot there"), Sweep[Sweep.Num() - 2].Y, LooterBackdrop::SweepHeight, 0.01f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunCardStyleTest, "Looter.UI.GunCard.Style",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunCardStyleTest::RunTest(const FString& Parameters)
{
	// Every maker has its own look and sound; a gun with none is plain.
	struct FExpect { const TCHAR* Key; GunCardStyle::EDeco Deco; const TCHAR* Cue; };
	const FExpect Makers[] = {
		{ TEXT("Roundhouse"), GunCardStyle::EDeco::Rivets, LooterSoundCue::Card::Maker::Roundhouse },
		{ TEXT("SaintMaud"), GunCardStyle::EDeco::Gilt, LooterSoundCue::Card::Maker::SaintMaud },
		{ TEXT("Windfall"), GunCardStyle::EDeco::Patch, LooterSoundCue::Card::Maker::Windfall },
		{ TEXT("Meridian"), GunCardStyle::EDeco::Ticks, LooterSoundCue::Card::Maker::Meridian },
	};
	for (const FExpect& Expect : Makers)
	{
		const GunCardStyle::FStyle& Style = GunCardStyle::ForMaker(Expect.Key);
		TestEqual(FString::Printf(TEXT("%s: its own look"), Expect.Key), Style.Key, FName(Expect.Key));
		TestTrue(FString::Printf(TEXT("%s: its word"), Expect.Key), !Style.Word.IsEmpty());
		TestTrue(FString::Printf(TEXT("%s: its mark"), Expect.Key), !Style.MarkIcon.IsNone());
		TestTrue(FString::Printf(TEXT("%s: its deco"), Expect.Key), Style.Deco == Expect.Deco);
		TestEqual(FString::Printf(TEXT("%s: its sound"), Expect.Key), Style.OpenCue, FName(Expect.Cue));
		TestTrue(FString::Printf(TEXT("%s: colours on its sweep"), Expect.Key), Style.Plates.Num() >= 2);
	}
	TestEqual(TEXT("Windfall's sweep is three plates"), GunCardStyle::ForMaker(TEXT("Windfall")).Plates.Num(), 3);
	TestTrue(TEXT("An unknown maker is plain"), GunCardStyle::ForMaker(TEXT("Nobody")).Key.IsNone());
	TestTrue(TEXT("...with no sound of its own"), GunCardStyle::Plain().OpenCue.IsNone());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunCardWidgetTest, "Looter.UI.GunCard.Widget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunCardWidgetTest::RunTest(const FString& Parameters)
{
	UWeaponDefinition* Shotgun = LoadObject<UWeaponDefinition>(nullptr, TEXT("/Game/Weapons/Data/DA_PumpShotgun.DA_PumpShotgun"));
	if (!TestNotNull(TEXT("The pump shotgun's data"), Shotgun))
	{
		return false;
	}
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UGunCardWidget* Card = CreateWidget<UGunCardWidget>(TestLevel.GetTestWorld(), UGunCardWidget::StaticClass());
	if (!TestNotNull(TEXT("The card"), Card))
	{
		return false;
	}
	Card->TakeWidget();

	const FWeaponInstanceData Loot = MakeGun(Shotgun, TEXT("Roundhouse"), EWeaponRarity::Rare, 14.f, 8, 70.f, 8, 2.2f, 101);
	const FWeaponInstanceData InHand = MakeGun(Shotgun, TEXT("Meridian"), EWeaponRarity::Uncommon, 9.f, 8, 80.f, 6, 2.6f, 202);
	TestTrue(TEXT("A first gun is new"), Card->SetGun(Loot, &InHand, EGunCardMode::World));
	TestFalse(TEXT("...the same again isn't"), Card->SetGun(Loot, &InHand, EGunCardMode::World));
	TestEqual(TEXT("The maker's word on its badge"), Card->GetMakerText().ToString(), FString(TEXT("ROUNDHOUSE")));
	TestEqual(TEXT("Six rows"), Card->GetStatCount(), 6);
	TestEqual(TEXT("Fire rate's value"), Card->GetStatValueText(4).ToString(), FString(TEXT("70 rpm")));
	TestEqual(TEXT("...the magazine's"), Card->GetStatValueText(5).ToString(), FString(TEXT("8")));
	TestEqual(TEXT("...the reload's"), Card->GetStatValueText(3).ToString(), FString(TEXT("2.2 s")));
	TestEqual(TEXT("A better shotgun is an upgrade"), Card->GetVerdictText().ToString(), FString(TEXT("UPGRADE")));
	TestTrue(TEXT("...compared: its damage arrow is up"), Card->HasCompare() && Card->GetStatArrowText(0).ToString().Equals(TEXT("▲")));
	TestTrue(TEXT("...a maker's trait line"), Card->GetLines().ContainsByPredicate([](const FString& Line) { return Line.Contains(TEXT("Rolling Stock")); }));
	TestEqual(TEXT("It opens with its maker's sound"), Card->GetMakerCue(), FName(LooterSoundCue::Card::Maker::Roundhouse));

	FGunCardKeys Keys;
	Keys.bSlotsFull = true;
	Keys.Hold = 0.5f;
	Card->SetKeys(Keys);
	const TArray<FString> Caps = Card->GetKeyCaps();
	TestEqual(TEXT("A world card's keys"), Caps.Num(), 2);
	TestNearlyEqual(TEXT("...the hold cap half full"), Card->GetHoldFill(), 0.5f, 0.001f);

	// The intro counts the damage up and ends whole.
	Card->PlayIntro(false);
	TestTrue(TEXT("The intro runs"), Card->IsIntroPlaying());
	TestEqual(TEXT("...counting from nothing"), Card->GetDamageText().ToString(), FString(TEXT("0 × 8")));
	for (int32 Step = 0; Step < 20; ++Step)
	{
		Card->AdvanceIntro(0.05f);
	}
	TestFalse(TEXT("...and ends"), Card->IsIntroPlaying());
	TestEqual(TEXT("...on the gun's damage"), Card->GetDamageText().ToString(), FString(TEXT("14 × 8")));

	// In hand: nothing compared, no keys.
	TestTrue(TEXT("Another mode is new"), Card->SetGun(InHand, nullptr, EGunCardMode::InHand));
	TestTrue(TEXT("No verdict in hand"), Card->GetVerdictText().IsEmpty());
	TestEqual(TEXT("No keys in hand"), Card->GetKeyCaps().Num(), 0);
	TestFalse(TEXT("No arrows in hand"), Card->HasCompare() || !Card->GetStatArrowText(0).IsEmpty());

	// Like with like: a rifle in hand isn't held against a shotgun; a carried shotgun is.
	UWeaponDefinition* Rifle = LoadObject<UWeaponDefinition>(nullptr, TEXT("/Game/Weapons/Data/DA_AssaultRifle.DA_AssaultRifle"));
	if (TestNotNull(TEXT("The rifle's data"), Rifle))
	{
		const FWeaponInstanceData Held = MakeGun(Rifle, TEXT("Meridian"), EWeaponRarity::Common, 30.f, 1, 540.f, 30, 2.2f, 303);
		TestNull(TEXT("A rifle in hand: no compare for a shotgun"), GunCardRules::PickCompare(Loot, &Held, {}));
		const FWeaponInstanceData* Carried[] = { &InHand };
		TestTrue(TEXT("...a shotgun carried: that one"), GunCardRules::PickCompare(Loot, &Held, Carried) == &InHand);
	}
	return true;
}

#endif

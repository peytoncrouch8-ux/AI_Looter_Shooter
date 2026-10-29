#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "UI/Cards/GunCardRules.h"
#include "UI/Cards/GunCardStyle.h"
#include "UI/Cards/GunCardWidget.h"
#include "UI/Cards/LooterCardBackdrop.h"
#include "UI/Cards/LooterSegmentBar.h"
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
	// Only what decides a pick: three bars.
	const TConstArrayView<LoadoutRules::EStat> Stats = GunCardRules::CardStats();
	if (TestEqual(TEXT("Three bars"), Stats.Num(), 3))
	{
		TestEqual(TEXT("Fire rate first"), GunCardRules::StatLabel(Stats[0]), FString(TEXT("FIRE RATE")));
		TestEqual(TEXT("then magazine"), GunCardRules::StatLabel(Stats[1]), FString(TEXT("MAGAZINE")));
		TestEqual(TEXT("then reload"), GunCardRules::StatLabel(Stats[2]), FString(TEXT("RELOAD")));
	}
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

	// The intro: snaps open with an overshoot, bars one after another, the count, the shine.
	TestNearlyEqual(TEXT("Starts folded"), GunCardRules::UnfoldScale(0.f), 0.15f, 0.001f);
	TestTrue(TEXT("Overshoots"), GunCardRules::UnfoldScale(0.08f) > 1.f);
	TestNearlyEqual(TEXT("Settles whole"), GunCardRules::UnfoldScale(GunCardRules::UnfoldSeconds), 1.f, 0.001f);
	TestTrue(TEXT("The first bar fills before the second"), GunCardRules::BarShare(0.15f, 0) > GunCardRules::BarShare(0.15f, 1));
	TestNearlyEqual(TEXT("Every bar full by the end"), GunCardRules::BarShare(GunCardRules::IntroSeconds, 2), 1.f, 0.001f);
	TestNearlyEqual(TEXT("The count done"), GunCardRules::CountShare(GunCardRules::CountSeconds), 1.f, 0.001f);
	TestTrue(TEXT("No shine at first"), GunCardRules::ShinePosition(0.1f) < 0.f);
	TestTrue(TEXT("The shine crosses midway"), GunCardRules::ShinePosition(0.5f) > 0.f && GunCardRules::ShinePosition(0.5f) < 1.f);

	// The bar's segments.
	TestEqual(TEXT("Nothing lights none"), LooterSegmentBarRules::LitSegments(0.f), 0);
	TestEqual(TEXT("A sliver lights one"), LooterSegmentBarRules::LitSegments(0.01f), 1);
	TestEqual(TEXT("Half lights five"), LooterSegmentBarRules::LitSegments(0.5f), 5);
	TestEqual(TEXT("Full lights ten"), LooterSegmentBarRules::LitSegments(1.f), 10);

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
	TestEqual(TEXT("The maker's word on the sweep"), Card->GetMakerText().ToString(), FString(TEXT("ROUNDHOUSE")));
	TestEqual(TEXT("Three bars"), Card->GetStatCount(), 3);
	TestEqual(TEXT("A better shotgun is an upgrade"), Card->GetVerdictText().ToString(), FString(TEXT("UPGRADE")));
	TestTrue(TEXT("...the bars show the gun in hand's tick"), Card->GetBar(0) && Card->GetBar(0)->GetTick() >= 0.f);
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
	TestEqual(TEXT("...counting from nothing"), Card->GetDamageText().ToString(), FString(TEXT("0")));
	for (int32 Step = 0; Step < 20; ++Step)
	{
		Card->AdvanceIntro(0.05f);
	}
	TestFalse(TEXT("...and ends"), Card->IsIntroPlaying());
	TestEqual(TEXT("...on the gun's damage"), Card->GetDamageText().ToString(), FString(TEXT("14")));

	// In hand: nothing compared, no keys.
	TestTrue(TEXT("Another mode is new"), Card->SetGun(InHand, nullptr, EGunCardMode::InHand));
	TestTrue(TEXT("No verdict in hand"), Card->GetVerdictText().IsEmpty());
	TestEqual(TEXT("No keys in hand"), Card->GetKeyCaps().Num(), 0);
	TestTrue(TEXT("No tick in hand"), Card->GetBar(0) && Card->GetBar(0)->GetTick() < 0.f);
	return true;
}

#endif

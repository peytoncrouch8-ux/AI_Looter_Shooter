#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Affixes/WeaponRollLibrary.h"
#include "Audio/LooterSoundCues.h"
#include "Loot/LootDropMoments.h"
#include "Loot/LootFanfareSubsystem.h"
#include "UI/World/WeaponLabelWidget.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponDefinition.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Tests/AutomationCommon.h"

// The bigger drop moments (Looter.Loot.DropMoments.*, Phase 12.3): what each rarity's drop does (taller beams each tier,
// stingers, an Epic's spin and ring, a Legendary's sting at the drop, slow-down, column, name pop and landing impact), the
// name pop's motion, and a Legendary dropped and landed in a test level. The rules read the cvars' defaults.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDropMomentRulesTest, "Looter.Loot.DropMoments.Rules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDropMomentRulesTest::RunTest(const FString& Parameters)
{
	using LootDropMoments::Defaults;
	const FDropMoment Common = Defaults(EWeaponRarity::Common);
	const FDropMoment Uncommon = Defaults(EWeaponRarity::Uncommon);
	const FDropMoment Rare = Defaults(EWeaponRarity::Rare);
	const FDropMoment Epic = Defaults(EWeaponRarity::Epic);
	const FDropMoment Legendary = Defaults(EWeaponRarity::Legendary);

	TestEqual(TEXT("A Common has no beam"), Common.BeamHeight, 0.f);
	TestTrue(TEXT("Each tier's beam is taller"), Uncommon.BeamHeight > 0.f && Rare.BeamHeight > Uncommon.BeamHeight
		&& Epic.BeamHeight > Rare.BeamHeight && Legendary.BeamHeight > Epic.BeamHeight);
	TestTrue(TEXT("...taller than before the drop moments (350 / 650 / 1000 / 1600)"), Uncommon.BeamHeight > 350.f && Rare.BeamHeight > 650.f
		&& Epic.BeamHeight > 1000.f && Legendary.BeamHeight > 1600.f);
	TestTrue(TEXT("A Common has no sting"), Common.Stinger.IsNone());
	TestEqual(TEXT("A Rare's sting"), Rare.Stinger, FName(LooterSoundCue::DropRare));
	TestEqual(TEXT("An Epic's"), Epic.Stinger, FName(LooterSoundCue::DropEpic));
	TestEqual(TEXT("A Legendary's"), Legendary.Stinger, FName(LooterSoundCue::DropLegendary));
	TestTrue(TEXT("Only a Legendary's plays as it drops"), Legendary.bStingerAtDrop && !Epic.bStingerAtDrop && !Rare.bStingerAtDrop);
	TestEqual(TEXT("...and lands with its impact"), Legendary.LandCue, FName(LootDropMoments::LegendaryLandCue));
	TestTrue(TEXT("Epic and up: heard flat"), Epic.bStingerFlat && Legendary.bStingerFlat && !Rare.bStingerFlat);

	TestTrue(TEXT("Epic and up spin as they land"), Epic.SpinRate > 0.f && Legendary.SpinRate > 0.f && Rare.SpinRate == 0.f);
	TestTrue(TEXT("...and ring the ground, a Legendary's wider"), Epic.RingRadius > 0.f && Legendary.RingRadius > Epic.RingRadius
		&& Rare.RingRadius == 0.f);
	TestEqual(TEXT("A Legendary slows the world 0.25 s"), Legendary.SlowSeconds, 0.25f, 0.0001f);
	TestTrue(TEXT("...to under half speed"), Legendary.SlowDilation > 0.f && Legendary.SlowDilation < 0.5f);
	TestTrue(TEXT("Nothing else slows it"), Epic.SlowSeconds == 0.f && Rare.SlowSeconds == 0.f);
	TestTrue(TEXT("A Legendary's column stands 40 m and more (seen from 60 m)"), Legendary.ColumnHeight >= 4000.f && Legendary.ColumnRadius > 0.f);
	TestTrue(TEXT("...its name pops up over it"), Legendary.NamePopSeconds > 0.f && Epic.NamePopSeconds == 0.f);
	TestTrue(TEXT("...and motes rise"), Legendary.Motes > 0 && Epic.Motes == 0);

	TestTrue(TEXT("A drop may slow the world from full speed"), LootDropMoments::MaySlowWorld(1.f));
	TestFalse(TEXT("...never inside another slow (Sundown, a boss's death)"), LootDropMoments::MaySlowWorld(0.25f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDropMomentPopTest, "Looter.Loot.DropMoments.NamePop",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDropMomentPopTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("The name pops up from small"), UWeaponLabelWidget::PopScaleAt(0.f) < 0.5f);
	TestTrue(TEXT("...past its size"), UWeaponLabelWidget::PopScaleAt(0.12f) > 1.15f);
	TestEqual(TEXT("...settling to it"), UWeaponLabelWidget::PopScaleAt(1.f), 1.f, 0.0001f);
	TestEqual(TEXT("Full while it shows"), UWeaponLabelWidget::PopOpacityAt(1.f, 4.5f), 1.f);
	TestTrue(TEXT("...fading at its end"), UWeaponLabelWidget::PopOpacityAt(4.2f, 4.5f) < 1.f);
	TestEqual(TEXT("...gone at its end"), UWeaponLabelWidget::PopOpacityAt(4.5f, 4.5f), 0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDropMomentLegendaryTest, "Looter.Loot.DropMoments.Legendary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDropMomentLegendaryTest::RunTest(const FString& Parameters)
{
	FEditorScriptExecutionGuard RunEvents;
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	ULootFanfareSubsystem* Fanfare = World->GetSubsystem<ULootFanfareSubsystem>();
	UWeaponDefinition* Rifle = LoadObject<UWeaponDefinition>(nullptr, TEXT("/Game/Weapons/Data/DA_AssaultRifle.DA_AssaultRifle"));
	if (!TestNotNull(TEXT("The fanfare runs in a test level"), Fanfare) || !TestNotNull(TEXT("The rifle loads"), Rifle))
	{
		return false;
	}
	// A plain Legendary (a cursed one keeps to its guttering).
	FWeaponInstanceData Instance = UWeaponRollLibrary::RollWeaponWithRarity(Rifle, EWeaponRarity::Legendary, 1);
	Instance.Curse = NAME_None;
	AWeaponBase* Gun = UWeaponRollLibrary::SpawnWeapon(World, Instance, FTransform(FVector(0.0, 0.0, 50.0)));
	if (!TestNotNull(TEXT("A Legendary rifle"), Gun))
	{
		return false;
	}
	Gun->OnDropped();
	TestTrue(TEXT("It lies as loot"), Gun->IsPickup());
	const FDropMoment Moment = LootDropMoments::Defaults(EWeaponRarity::Legendary);

	ULootFanfareSubsystem::ExpectLanding(Gun);
	TestTrue(TEXT("Its drop slows the world"), Fanfare->IsSlowingWorld());
	TestEqual(TEXT("...to its speed"), UGameplayStatics::GetGlobalTimeDilation(World), Moment.SlowDilation, 0.001f);

	// Its ticks come slowed; a quarter second of real time ends it, the world back to full speed.
	Fanfare->Advance(Moment.SlowSeconds * Moment.SlowDilation * 1.05f);
	TestFalse(TEXT("The slow-down ends after its time"), Fanfare->IsSlowingWorld());
	TestEqual(TEXT("...the world back to full speed"), UGameplayStatics::GetGlobalTimeDilation(World), 1.f, 0.001f);

	// Landed (its toss is done or it never had one): the shows begin.
	for (int32 Frame = 0; Frame < 3 && Fanfare->NumExpected() > 0; ++Frame)
	{
		Fanfare->Advance(1.f / 30.f);
	}
	if (Fanfare->NumExpected() > 0)
	{
		Fanfare->Advance(ULootFanfareSubsystem::LandingTimeout);
	}
	TestEqual(TEXT("It landed"), Fanfare->NumExpected(), 0);
	TestEqual(TEXT("...a column stands over it"), Fanfare->NumColumns(), 1);
	TestEqual(TEXT("...its name pops up"), Fanfare->NumNamePops(), 1);
	TestEqual(TEXT("...it spins"), Fanfare->NumSpinning(), 1);
	TestEqual(TEXT("...a ring runs out"), Fanfare->NumRings(), 1);

	Fanfare->Advance(FMath::Max(Moment.SpinSeconds, Moment.RingSeconds) + 0.1f);
	TestEqual(TEXT("The spin settles"), Fanfare->NumSpinning(), 0);
	TestEqual(TEXT("The ring fades"), Fanfare->NumRings(), 0);
	TestEqual(TEXT("The column stands while it lies there"), Fanfare->NumColumns(), 1);

	// Taken: its column and name go with it.
	Gun->Destroy();
	Fanfare->Advance(1.f / 30.f);
	TestEqual(TEXT("Taken, its column goes"), Fanfare->NumColumns(), 0);
	TestEqual(TEXT("...and its name"), Fanfare->NumNamePops(), 0);

	// Inside another slow (Sundown's quarter speed), a drop leaves the clock alone.
	UGameplayStatics::SetGlobalTimeDilation(World, 0.25f);
	AWeaponBase* Second = UWeaponRollLibrary::SpawnWeapon(World, Instance, FTransform(FVector(300.0, 0.0, 50.0)));
	if (Second)
	{
		Second->OnDropped();
		ULootFanfareSubsystem::ExpectLanding(Second);
		TestFalse(TEXT("No slow-down inside another"), Fanfare->IsSlowingWorld());
		TestEqual(TEXT("...the other's speed kept"), UGameplayStatics::GetGlobalTimeDilation(World), 0.25f, 0.001f);
		Second->Destroy();
	}
	UGameplayStatics::SetGlobalTimeDilation(World, 1.f);
	return true;
}

#endif

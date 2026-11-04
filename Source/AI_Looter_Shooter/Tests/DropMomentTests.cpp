#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Affixes/WeaponRollLibrary.h"
#include "Audio/LooterSoundCues.h"
#include "Loot/LootBeamCrowd.h"
#include "Loot/LootDropMoments.h"
#include "Loot/LootFanfareSubsystem.h"
#include "UI/World/WeaponLabelWidget.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponDefinition.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Tests/AutomationCommon.h"

// The bigger drop moments (Looter.Loot.DropMoments.*, Phase 12.3): what each rarity's drop does (taller beams each tier,
// a Common's short one and soft landing since 2026-11-03, stingers heard flat from Uncommon up, an Epic's spin and ring, a
// Legendary's sting at the drop, burst, column, name pop and landing impact), the name pop's motion, a Common dropped in
// a test level (its beam up), and a Legendary dropped and landed there: the world never slowed (the user, 2026-10-27: the
// slow read as a stutter), its burst thrown, its column and name taken from the pools and given back. The rules read the
// cvars' defaults.

namespace
{
	/** A console variable set for a test and put back after it. */
	class FDropScopedCVar
	{
	public:
		FDropScopedCVar(const TCHAR* Name, const TCHAR* Value)
			: Variable(IConsoleManager::Get().FindConsoleVariable(Name))
		{
			if (Variable)
			{
				Previous = Variable->GetString();
				Variable->Set(Value, ECVF_SetByConsole);
			}
		}
		~FDropScopedCVar()
		{
			if (Variable)
			{
				Variable->Set(*Previous, ECVF_SetByConsole);
			}
		}

	private:
		IConsoleVariable* Variable = nullptr;
		FString Previous;
	};
}

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

	// Every drop seen (2026-11-03): a Common's short beam, 2.5 m, over the ammo's 1.5 m white ones and under an Uncommon's.
	TestEqual(TEXT("A Common's beam: 2.5 m"), Common.BeamHeight, 250.f);
	TestTrue(TEXT("...fainter and thinner than an Uncommon's"), Common.BeamGlow < Uncommon.BeamGlow && Common.BeamRadius < Uncommon.BeamRadius);
	TestTrue(TEXT("Each tier's beam is taller"), Common.BeamHeight > 150.f && Uncommon.BeamHeight > Common.BeamHeight && Rare.BeamHeight > Uncommon.BeamHeight
		&& Epic.BeamHeight > Rare.BeamHeight && Legendary.BeamHeight > Epic.BeamHeight);
	TestTrue(TEXT("...taller than before the drop moments (350 / 650 / 1000 / 1600)"), Uncommon.BeamHeight > 350.f && Rare.BeamHeight > 650.f
		&& Epic.BeamHeight > 1000.f && Legendary.BeamHeight > 1600.f);
	// ...and heard: a Common's soft landing, the Uncommon's glint quieter and lower, from where it fell.
	TestEqual(TEXT("A Common's soft landing: the Uncommon's glint"), Common.Stinger, FName(LooterSoundCue::DropUncommon));
	TestTrue(TEXT("...quieter and lower"), Common.StingerVolume > 0.f && Common.StingerVolume < 1.f && Common.StingerPitch < 1.f);
	TestTrue(TEXT("...the rarer ones at their own"), Uncommon.StingerVolume == 1.f && Rare.StingerVolume == 1.f && Legendary.StingerPitch == 1.f);
	TestEqual(TEXT("An Uncommon's sting"), Uncommon.Stinger, FName(LooterSoundCue::DropUncommon));
	TestEqual(TEXT("A Rare's sting"), Rare.Stinger, FName(LooterSoundCue::DropRare));
	TestEqual(TEXT("An Epic's"), Epic.Stinger, FName(LooterSoundCue::DropEpic));
	TestEqual(TEXT("A Legendary's"), Legendary.Stinger, FName(LooterSoundCue::DropLegendary));
	TestTrue(TEXT("Only a Legendary's plays as it drops"), Legendary.bStingerAtDrop && !Epic.bStingerAtDrop && !Rare.bStingerAtDrop && !Common.bStingerAtDrop);
	TestEqual(TEXT("...and lands with its impact"), Legendary.LandCue, FName(LootDropMoments::LegendaryLandCue));
	// Uncommon and up heard flat by the player (a Rare's sting had been lost 20 m off under fire); a Common's where it fell.
	TestTrue(TEXT("Uncommon and up: heard flat"), Uncommon.bStingerFlat && Rare.bStingerFlat && Epic.bStingerFlat && Legendary.bStingerFlat);
	TestFalse(TEXT("...a Common's from where it fell"), Common.bStingerFlat);

	TestTrue(TEXT("Epic and up spin as they land"), Epic.SpinRate > 0.f && Legendary.SpinRate > 0.f && Rare.SpinRate == 0.f);
	TestTrue(TEXT("...and ring the ground, a Legendary's wider"), Epic.RingRadius > 0.f && Legendary.RingRadius > Epic.RingRadius
		&& Rare.RingRadius == 0.f);
	TestTrue(TEXT("A Legendary bursts as it lands, nothing else does"), Legendary.bBurst && !Epic.bBurst && !Rare.bBurst && !Common.bBurst);
	TestTrue(TEXT("A Legendary's column stands 40 m and more (seen from 60 m)"), Legendary.ColumnHeight >= 4000.f && Legendary.ColumnRadius > 0.f);
	TestTrue(TEXT("...its name pops up over it"), Legendary.NamePopSeconds > 0.f && Epic.NamePopSeconds == 0.f);
	TestTrue(TEXT("...and motes rise"), Legendary.Motes > 0 && Epic.Motes == 0);

	// The burst: big enough to read from 40 m (rings across 10 m and more, a star metres wide, sparks kept two pixels wide),
	// a galaxy's arms of star sparks and a fountain, short (under 2 s), in budget (a few hundred sparks at most).
	const FLegendaryBurst Burst = LootDropMoments::DefaultBurst();
	TestTrue(TEXT("The burst is on"), Burst.Scale > 0.f && Burst.Glow > 0.f);
	TestTrue(TEXT("...its rings run out 10 m and more"), Burst.RingReach >= 1000.f && Burst.OuterReach > Burst.RingReach);
	TestTrue(TEXT("...its star is metres across"), Burst.StarSize >= 500.f && Burst.FlashSize > 0.f && Burst.CoreSize > 0.f);
	TestTrue(TEXT("...a tilted ring: the galaxy's disc"), Burst.TiltReach > 0.f && Burst.TiltDegrees > 0.f && Burst.TiltDegrees < 90.f);
	TestTrue(TEXT("...spiral arms of star sparks, winding"), Burst.Arms >= 2 && Burst.ArmSparks > 0 && Burst.ArmTurns > 0.f && Burst.ArmReach >= 1000.f);
	TestTrue(TEXT("...a fountain of them"), Burst.Fountain > 0 && Burst.FountainSpeed > 0.f);
	TestTrue(TEXT("...each spark at least about two pixels at 1080p"), Burst.SparkMinAngle >= 0.002f);
	TestTrue(TEXT("...all of it over in under 2 s"), FMath::Max3(Burst.ArmSeconds * 1.15f, Burst.FountainSeconds * 1.15f, Burst.OuterSeconds) < 2.f);
	TestTrue(TEXT("...a few hundred sparks at most"), Burst.Arms * Burst.ArmSparks + Burst.Fountain <= 200);
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDropMomentCommonTest, "Looter.Loot.DropMoments.Common",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDropMomentCommonTest::RunTest(const FString& Parameters)
{
	FDropScopedCVar Beam(TEXT("Looter.Loot.Drop.Beam.Common"), TEXT("250"));
	FDropScopedCVar Flat(TEXT("Looter.Loot.Drop.Stinger.FlatFrom"), TEXT("1"));
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
	FWeaponInstanceData Instance = UWeaponRollLibrary::RollWeaponWithRarity(Rifle, EWeaponRarity::Common, 1);
	Instance.Curse = NAME_None;
	AWeaponBase* Gun = UWeaponRollLibrary::SpawnWeapon(World, Instance, FTransform(FVector(0.0, 0.0, 50.0)));
	if (!TestNotNull(TEXT("A Common rifle"), Gun))
	{
		return false;
	}
	Gun->OnDropped();
	// Lying as loot, its beam stands 2.5 m: seen, and it stands for its spot among the small drops' beams.
	const UStaticMeshComponent* Shown = Gun->GetLootBeam();
	TestTrue(TEXT("A Common lying as loot shows its beam"), Gun->IsPickup() && LootBeamCrowd::ShowsBeam(Shown));
	TestEqual(TEXT("...2.5 m tall"), Shown ? static_cast<float>(Shown->GetRelativeScale3D().Z) * 100.f : 0.f, 250.f, 1.f);

	// It lands quietly otherwise: no flare, no spin, no ring; its soft sting from where it fell, a Rare's heard flat.
	ULootFanfareSubsystem::ExpectLanding(Gun);
	Fanfare->Advance(ULootFanfareSubsystem::LandingTimeout);
	TestEqual(TEXT("It landed"), Fanfare->NumExpected(), 0);
	TestTrue(TEXT("...no flare, no spin, no ring"), Fanfare->NumFlaring() == 0 && Fanfare->NumSpinning() == 0 && Fanfare->NumRings() == 0);
	TestFalse(TEXT("A Common's sting from where it fell"), ULootFanfareSubsystem::IsDropCueFlat(EWeaponRarity::Common));
	TestTrue(TEXT("...an Uncommon's and a Rare's flat, for the player"), ULootFanfareSubsystem::IsDropCueFlat(EWeaponRarity::Uncommon)
		&& ULootFanfareSubsystem::IsDropCueFlat(EWeaponRarity::Rare));

	// Off (Looter.Loot.Drop.Beam.Common 0, as before): a Common lies dark and stands for nothing.
	{
		FDropScopedCVar Dark(TEXT("Looter.Loot.Drop.Beam.Common"), TEXT("0"));
		AWeaponBase* Unseen = UWeaponRollLibrary::SpawnWeapon(World, Instance, FTransform(FVector(500.0, 0.0, 50.0)));
		if (TestNotNull(TEXT("Another Common rifle"), Unseen))
		{
			Unseen->OnDropped();
			TestFalse(TEXT("Off: no beam, and none standing for its spot"), LootBeamCrowd::ShowsBeam(Unseen->GetLootBeam()));
			Unseen->Destroy();
		}
	}
	Gun->Destroy();
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

	// The pools, as play beginning builds them (a test level doesn't begin play).
	Fanfare->WarmPools();
	TestEqual(TEXT("Columns ready before anything lands"), Fanfare->NumPooledColumns(), ULootFanfareSubsystem::PoolSize);
	TestEqual(TEXT("...and name pops"), Fanfare->NumPooledNamePops(), ULootFanfareSubsystem::PoolSize);

	ULootFanfareSubsystem::ExpectLanding(Gun);
	TestEqual(TEXT("Its drop never slows the world (it read as a stutter)"), UGameplayStatics::GetGlobalTimeDilation(World), 1.f, 0.001f);

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
	TestEqual(TEXT("...the world still at full speed"), UGameplayStatics::GetGlobalTimeDilation(World), 1.f, 0.001f);
	TestEqual(TEXT("...a column stands over it"), Fanfare->NumColumns(), 1);
	TestEqual(TEXT("...its name pops up"), Fanfare->NumNamePops(), 1);
	TestEqual(TEXT("...both from the pools, none made"), Fanfare->NumPooledColumns() + Fanfare->NumPooledNamePops(),
		2 * ULootFanfareSubsystem::PoolSize - 2);
	TestEqual(TEXT("...it spins"), Fanfare->NumSpinning(), 1);
	TestEqual(TEXT("...a ring runs out"), Fanfare->NumRings(), 1);
	const FLegendaryBurst Burst = LootDropMoments::Burst();
	TestTrue(FString::Printf(TEXT("...it bursts: %d star sparks flying"), Fanfare->NumBurstSparks()), Fanfare->NumBurstSparks() > 0
		|| Burst.Scale <= 0.f);

	Fanfare->Advance(FMath::Max(Moment.SpinSeconds, Moment.RingSeconds) + 0.1f);
	TestEqual(TEXT("The spin settles"), Fanfare->NumSpinning(), 0);
	TestEqual(TEXT("The ring fades"), Fanfare->NumRings(), 0);
	TestEqual(TEXT("The column stands while it lies there"), Fanfare->NumColumns(), 1);
	Fanfare->Advance(FMath::Max(Burst.ArmSeconds, Burst.FountainSeconds) * 1.2f);
	TestEqual(TEXT("The burst's sparks are gone"), Fanfare->NumBurstSparks(), 0);

	// Taken: its column and name go back to the pools, hidden.
	Gun->Destroy();
	Fanfare->Advance(1.f / 30.f);
	TestEqual(TEXT("Taken, its column goes"), Fanfare->NumColumns(), 0);
	TestEqual(TEXT("...and its name"), Fanfare->NumNamePops(), 0);
	TestEqual(TEXT("...both back in the pools"), Fanfare->NumPooledColumns() + Fanfare->NumPooledNamePops(), 2 * ULootFanfareSubsystem::PoolSize);

	// Three Legendaries lying at once: the pools lend what they have and make the one more.
	TArray<AWeaponBase*> Shower;
	for (int32 Index = 0; Index < ULootFanfareSubsystem::PoolSize + 1; ++Index)
	{
		if (AWeaponBase* Each = UWeaponRollLibrary::SpawnWeapon(World, Instance, FTransform(FVector(300.0 * (Index + 1), 0.0, 50.0))))
		{
			Each->OnDropped();
			ULootFanfareSubsystem::ExpectLanding(Each);
			Shower.Add(Each);
		}
	}
	Fanfare->Advance(ULootFanfareSubsystem::LandingTimeout);
	TestEqual(TEXT("A shower of Legendaries: a column over each"), Fanfare->NumColumns(), Shower.Num());
	for (AWeaponBase* Each : Shower)
	{
		Each->Destroy();
	}
	Fanfare->Advance(1.f / 30.f);
	TestEqual(TEXT("...each given back"), Fanfare->NumPooledColumns(), Shower.Num());
	TestEqual(TEXT("The world never slowed"), UGameplayStatics::GetGlobalTimeDilation(World), 1.f, 0.001f);
	return true;
}

#endif

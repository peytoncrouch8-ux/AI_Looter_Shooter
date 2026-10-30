#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Affixes/WeaponRollLibrary.h"
#include "Bosses/AbelRules.h"
#include "Bosses/BossComponent.h"
#include "Bosses/BossTestSpider.h"
#include "Bosses/BossTypes.h"
#include "Combat/HealthComponent.h"
#include "Combat/MovementSlowComponent.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/ForemanFight.h"
#include "Creatures/GravemotherFight.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Loot/LootLibrary.h"
#include "Tests/CreatureTraitTestWorld.h"
#include "UI/Cards/GunCardRules.h"
#include "Weapons/LegendaryDrops.h"
#include "Weapons/LegendaryEffectComponent.h"
#include "Weapons/LegendaryLantern.h"
#include "Weapons/LegendaryRules.h"
#include "Weapons/LegendarySubsystem.h"
#include "Weapons/LegendaryTuning.h"
#include "Weapons/NamedWeaponDefinition.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponDefinition.h"
#include "Weapons/WeaponTypes.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Math/RandomStream.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Package.h"

// The unique legendaries (Looter.Weapons.Legendary.*; the user's call, 2026-10-29 night): each one's rules (Gravesilk's
// silk and root, Company Scrip's burst and refund, Lamplighter's scoped kill and lantern), its card line, its boss's 10%
// drop, and the bosses naming their own. The rules read a default-made FLegendaryTuning (the cvars' defaults); the play
// parts run in test levels, which never tick: their clock stands still, so everything there happens "at once".

namespace
{
	/** A made-up named gun with Signature, on Weapon (a scratch one when none is given), in its own scratch package. */
	UNamedWeaponDefinition* MakeNamed(ELegendarySignature Signature, UWeaponDefinition* Weapon = nullptr)
	{
		UNamedWeaponDefinition* Named = NewObject<UNamedWeaponDefinition>(CreatePackage(nullptr), NAME_None, RF_Transient);
		Named->Weapon = Weapon ? Weapon : NewObject<UWeaponDefinition>(GetTransientPackage(), NAME_None, RF_Transient);
		Named->DisplayName = FText::FromString(TEXT("Test Legendary"));
		Named->Rarity = EWeaponRarity::Legendary;
		Named->Signature = Signature;
		return Named;
	}

	/** Of Kills kills rolled from a seeded stream, how many drop Named (each kill its own roll). */
	int32 CountDrops(UNamedWeaponDefinition* Named, int32 Kills, int32 Seed)
	{
		const FLegendaryTuning Tuning;
		FRandomStream Random(Seed);
		int32 Drops = 0;
		for (int32 Kill = 0; Kill < Kills; ++Kill)
		{
			FLootRoll Roll;
			Drops += LegendaryDrops::AddDedicated(Named, 5, Random.GetFraction(), Tuning, Roll) ? 1 : 0;
		}
		return Drops;
	}

	/** Its boss's chance, the drop near it over many kills, and its card line. */
	void CheckDropAndCard(FAutomationTestBase& Test, ELegendarySignature Signature)
	{
		const FLegendaryTuning Tuning;
		Test.TestEqual(TEXT("Its boss's chance: 10% a kill"), LegendaryRules::DropChance(Signature, Tuning), 0.1f, 1.e-6f);
		// 20,000 kills: 2,000 expected, 42 the spread; 200 either side is nearly five of it.
		const int32 Drops = CountDrops(MakeNamed(Signature), 20000, 2029);
		Test.TestTrue(FString::Printf(TEXT("...about one kill in ten over many (%d of 20000)"), Drops), Drops > 1800 && Drops < 2200);
		FWeaponInstanceData Gun;
		Gun.Named = MakeNamed(Signature);
		const FString Line = LegendaryRules::Description(Signature, Tuning);
		Test.TestFalse(TEXT("It has a card line"), Line.IsEmpty());
		Test.TestTrue(TEXT("...and the card shows it"), GunCardRules::Lines(Gun).Contains(Line));
	}

	/** A holder as the game has one: a pawn with health and a weapon manager, begun play (as DeadHandTests'). */
	APawn* SpawnHolder(UWorld* World, const FVector& Where, UHealthComponent*& OutHealth, UWeaponManagerComponent*& OutWeapons)
	{
		APawn* Holder = World->SpawnActor<APawn>(Where, FRotator::ZeroRotator);
		if (!Holder)
		{
			return nullptr;
		}
		OutHealth = NewObject<UHealthComponent>(Holder, TEXT("Health"));
		OutHealth->bShowDamageNumbers = false;
		Holder->AddInstanceComponent(OutHealth);
		OutHealth->RegisterComponent();
		OutWeapons = NewObject<UWeaponManagerComponent>(Holder, TEXT("Weapons"));
		Holder->AddInstanceComponent(OutWeapons);
		OutWeapons->RegisterComponent();
		OutWeapons->MaxWeapons = 3;
		Holder->DispatchBeginPlay();
		return Holder;
	}

	/** A loose gun of Kind's asset, named as a unique legendary with Signature, held by Holder. */
	AWeaponBase* SpawnLegendary(FAutomationTestBase& Test, UWorld* World, const TCHAR* GunPath, ELegendarySignature Signature, APawn* Holder)
	{
		UWeaponDefinition* Kind = LoadObject<UWeaponDefinition>(nullptr, GunPath);
		if (!Test.TestNotNull(TEXT("The kind of gun loads"), Kind))
		{
			return nullptr;
		}
		FWeaponInstanceData Instance = UWeaponRollLibrary::RollWeaponWithRarity(Kind, EWeaponRarity::Legendary, 5);
		Instance.Named = MakeNamed(Signature, Kind);
		AWeaponBase* Gun = UWeaponRollLibrary::SpawnWeapon(World, Instance, FTransform::Identity);
		if (Gun && Holder)
		{
			Gun->SetOwner(Holder);
		}
		return Gun;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLegendaryGravesilkTest, "Looter.Weapons.Legendary.Gravesilk",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FLegendaryGravesilkTest::RunTest(const FString& Parameters)
{
	using LegendaryRules::ESilk;
	const FLegendaryTuning Tuning;
	// Six stacks root it for a second; the stacks are spent.
	LegendaryRules::FSilk Silk;
	bool bLaid = true;
	for (int32 Hit = 0; Hit < 5; ++Hit)
	{
		bLaid &= LegendaryRules::LaySilk(Silk, 10.0 + Hit * 0.1, false, Tuning) == ESilk::Laid;
	}
	TestTrue(TEXT("Five hits lay five stacks"), bLaid && LegendaryRules::StacksAt(Silk, 10.5, Tuning) == 5);
	TestTrue(TEXT("The sixth roots it"), LegendaryRules::LaySilk(Silk, 10.5, false, Tuning) == ESilk::Rooted);
	TestEqual(TEXT("...spending the stacks"), LegendaryRules::StacksAt(Silk, 10.5, Tuning), 0);
	TestTrue(TEXT("Rooted for its second"), LegendaryRules::IsRooted(Silk, 11.4) && !LegendaryRules::IsRooted(Silk, 11.5));
	// Nothing takes while rooted or in the gap after; then it starts again.
	TestTrue(TEXT("No silk while rooted"), LegendaryRules::LaySilk(Silk, 11.f, false, Tuning) == ESilk::None);
	TestTrue(TEXT("...nor in the gap after"), LegendaryRules::LaySilk(Silk, 11.5 + Tuning.GravesilkRootGap - 0.1, false, Tuning) == ESilk::None);
	TestTrue(TEXT("...then it takes again"), LegendaryRules::LaySilk(Silk, 11.5 + Tuning.GravesilkRootGap + 0.1, false, Tuning) == ESilk::Laid);
	// Silk falls away a while after the last hit.
	LegendaryRules::FSilk Fading;
	LegendaryRules::LaySilk(Fading, 0.0, false, Tuning);
	LegendaryRules::LaySilk(Fading, 1.0, false, Tuning);
	TestEqual(TEXT("Two stacks on it"), LegendaryRules::StacksAt(Fading, 1.0 + Tuning.GravesilkSilkSeconds - 0.1, Tuning), 2);
	TestEqual(TEXT("...fallen away after its seconds"), LegendaryRules::StacksAt(Fading, 1.0 + Tuning.GravesilkSilkSeconds + 0.1, Tuning), 0);
	LegendaryRules::LaySilk(Fading, 1.0 + Tuning.GravesilkSilkSeconds + 0.1, false, Tuning);
	TestEqual(TEXT("...and the next hit starts from one"), Fading.Stacks, 1);
	// Never a boss.
	LegendaryRules::FSilk Boss;
	bool bBossTook = false;
	for (int32 Hit = 0; Hit < 20; ++Hit)
	{
		bBossTook |= LegendaryRules::LaySilk(Boss, Hit * 0.05, true, Tuning) != ESilk::None;
	}
	TestFalse(TEXT("A boss takes no silk"), bBossTook);
	CheckDropAndCard(*this, ELegendarySignature::Gravesilk);

	// In a level: a spider rooted (its walk held at the slow's floor), a boss never.
	FEditorScriptExecutionGuard RunEvents;
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	CreatureTraitTestWorld::SpawnFloor(World);
	ULegendarySubsystem* Legendaries = World->GetSubsystem<ULegendarySubsystem>();
	ACreatureBase* Spider = CreatureTraitTestWorld::SpawnSpider(World, FVector::ZeroVector);
	UBossComponent* BossComponent = BossTestSpider::Spawn(World, FVector(0.0, 3000.0, 0.0), 0.f);
	ACreatureBase* BossSpider = BossComponent ? Cast<ACreatureBase>(BossComponent->GetOwner()) : nullptr;
	if (!TestNotNull(TEXT("The legendaries run in a test level"), Legendaries) || !TestNotNull(TEXT("A spider"), Spider)
		|| !TestNotNull(TEXT("A boss"), BossSpider))
	{
		return false;
	}
	ESilk Last = ESilk::None;
	for (int32 Hit = 0; Hit < Tuning.GravesilkStacks; ++Hit)
	{
		Last = Legendaries->LaySilk(*Spider);
	}
	TestTrue(TEXT("Six hits root the spider"), Last == ESilk::Rooted && Legendaries->IsRooted(Spider));
	const UMovementSlowComponent* Slow = Spider->FindComponentByClass<UMovementSlowComponent>();
	TestTrue(TEXT("...held where it stands"), Slow && Slow->IsSlowed() && Slow->GetMultiplier() <= Tuning.GravesilkRootShare + 0.001f);
	for (int32 Hit = 0; Hit < Tuning.GravesilkStacks * 2; ++Hit)
	{
		Legendaries->LaySilk(*BossSpider);
	}
	TestFalse(TEXT("The boss never"), Legendaries->IsRooted(BossSpider));
	TestEqual(TEXT("...nor carries silk"), Legendaries->SilkOn(BossSpider), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLegendaryCompanyScripTest, "Looter.Weapons.Legendary.CompanyScrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FLegendaryCompanyScripTest::RunTest(const FString& Parameters)
{
	const FLegendaryTuning Tuning;
	TestEqual(TEXT("No kills: its own fire rate"), LegendaryRules::ScripFireRateScale(0, Tuning), 1.f);
	TestEqual(TEXT("A kill: +4%"), LegendaryRules::ScripFireRateScale(1, Tuning), 1.04f, 1.e-5f);
	TestEqual(TEXT("Eight: +32%"), LegendaryRules::ScripFireRateScale(8, Tuning), 1.32f, 1.e-5f);
	TestEqual(TEXT("...and no more past eight"), LegendaryRules::ScripFireRateScale(12, Tuning), 1.32f, 1.e-5f);
	TestEqual(TEXT("A ninth kill adds nothing"), LegendaryRules::ScripAddKill(8, Tuning), 8);
	TestTrue(TEXT("A burst holds between rounds, lapses after a pause"), LegendaryRules::ScripBurstHolds(0.2, Tuning)
		&& !LegendaryRules::ScripBurstHolds(Tuning.CompanyScripBurstGap + 0.1, Tuning));
	TestEqual(TEXT("A reload pays back 20% of the magazine"), LegendaryRules::ScripRefund(150, 150, Tuning), 30);
	TestEqual(TEXT("...of a smaller one too"), LegendaryRules::ScripRefund(80, 100, Tuning), 20);
	TestEqual(TEXT("...never more than it took"), LegendaryRules::ScripRefund(10, 150, Tuning), 10);
	TestEqual(TEXT("...and nothing for nothing taken"), LegendaryRules::ScripRefund(0, 150, Tuning), 0);
	CheckDropAndCard(*this, ELegendarySignature::CompanyScrip);

	// On a gun: kills while the trigger is held quicken it; letting go ends it; a reload's pay reaches the pool.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	UHealthComponent* Health = nullptr;
	UWeaponManagerComponent* Weapons = nullptr;
	APawn* Holder = SpawnHolder(World, FVector::ZeroVector, Health, Weapons);
	AWeaponBase* Gun = SpawnLegendary(*this, World, TEXT("/Game/Weapons/Data/DA_LMG.DA_LMG"), ELegendarySignature::CompanyScrip, Holder);
	if (!TestNotNull(TEXT("A holder"), Holder) || !TestNotNull(TEXT("Company Scrip"), Gun))
	{
		return false;
	}
	const double Now = World->GetTimeSeconds();
	const float Base = Gun->GetShotInterval();
	ULegendaryEffectComponent* Legendary = ULegendaryEffectComponent::For(*Gun);
	TestFalse(TEXT("A kill before any shot counts for nothing"), Legendary->NoteKill(Now));
	Legendary->NoteShot(Now, 0.f);
	for (int32 Kill = 0; Kill < 3; ++Kill)
	{
		Legendary->NoteKill(Now);
	}
	TestEqual(TEXT("Three kills in the burst"), Legendary->GetStacks(Now), 3);
	TestEqual(TEXT("...shoot 12% faster"), Gun->GetShotInterval(), Base / 1.12f, Base * 1.e-4f);
	Gun->StopFire();
	TestEqual(TEXT("Let go: the burst is over"), Legendary->GetStacks(Now), 0);
	TestEqual(TEXT("...and its own rate again"), Gun->GetShotInterval(), Base, Base * 1.e-4f);

	const EAmmoType Ammo = Gun->GetAmmoType();
	Weapons->AddAmmo(Ammo, 100);
	const int32 Before = Weapons->GetAmmo(Ammo);
	const int32 Expected = FMath::Min(LegendaryRules::ScripRefund(150, Gun->GetInstance().Stats.MagazineSize, Tuning), Weapons->GetMaxAmmo(Ammo) - Before);
	const int32 Paid = ULegendaryEffectComponent::RefundReload(*Gun, Weapons, 150);
	TestEqual(TEXT("A full reload pays its share back"), Paid, Expected);
	TestEqual(TEXT("...into the pool"), Weapons->GetAmmo(Ammo), Before + Paid);
	AWeaponBase* Plain = SpawnLegendary(*this, World, TEXT("/Game/Weapons/Data/DA_LMG.DA_LMG"), ELegendarySignature::None, Holder);
	TestEqual(TEXT("Any other gun pays nothing"), Plain ? ULegendaryEffectComponent::RefundReload(*Plain, Weapons, 150) : -1, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLegendaryLamplighterTest, "Looter.Weapons.Legendary.Lamplighter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FLegendaryLamplighterTest::RunTest(const FString& Parameters)
{
	const FLegendaryTuning Tuning;
	TestTrue(TEXT("Scoped: the sights most of the way up"), LegendaryRules::IsScopedShot(1.f, Tuning) && LegendaryRules::IsScopedShot(Tuning.LamplighterScopedFrom, Tuning));
	TestTrue(TEXT("...not from the hip"), !LegendaryRules::IsScopedShot(0.f, Tuning) && !LegendaryRules::IsScopedShot(Tuning.LamplighterScopedFrom - 0.05f, Tuning));
	TestEqual(TEXT("2% of max health a second"), LegendaryRules::LanternHeal(250.f, 1.f, Tuning), 5.f, 1.e-4f);
	TestEqual(TEXT("...a frame's share of it"), LegendaryRules::LanternHeal(250.f, 0.1f, Tuning), 0.5f, 1.e-4f);
	TestTrue(TEXT("Within 4 m, not past"), LegendaryRules::InLanternReach(FVector::ZeroVector, FVector(399.0, 0.0, 0.0), Tuning.LamplighterRadius)
		&& !LegendaryRules::InLanternReach(FVector::ZeroVector, FVector(401.0, 0.0, 0.0), Tuning.LamplighterRadius));
	TestTrue(TEXT("It lights, burns and gutters out"), LegendaryRules::LanternGlow(0.3f, 5.f) == 1.f && LegendaryRules::LanternGlow(4.75f, 5.f) < 1.f
		&& LegendaryRules::LanternGlow(5.f, 5.f) == 0.f);
	CheckDropAndCard(*this, ELegendarySignature::Lamplighter);

	FEditorScriptExecutionGuard RunEvents;
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	CreatureTraitTestWorld::SpawnFloor(World);
	ULegendarySubsystem* Legendaries = World->GetSubsystem<ULegendarySubsystem>();
	UHealthComponent* Health = nullptr;
	UWeaponManagerComponent* Weapons = nullptr;
	APawn* Holder = SpawnHolder(World, FVector(100.0, 0.0, 90.0), Health, Weapons);
	AWeaponBase* Gun = SpawnLegendary(*this, World, TEXT("/Game/Weapons/Data/DA_Sniper.DA_Sniper"), ELegendarySignature::Lamplighter, Holder);
	ACreatureBase* Spider = CreatureTraitTestWorld::SpawnSpider(World, FVector(600.0, 0.0, 0.0));
	if (!TestNotNull(TEXT("The legendaries run in a test level"), Legendaries) || !TestNotNull(TEXT("A holder"), Holder)
		|| !TestNotNull(TEXT("Lamplighter"), Gun) || !TestNotNull(TEXT("A spider"), Spider))
	{
		return false;
	}
	// A kill from the hip lights nothing; a scoped one lights a lantern.
	const double Now = World->GetTimeSeconds();
	ULegendaryEffectComponent* Legendary = ULegendaryEffectComponent::For(*Gun);
	Legendary->NoteShot(Now, 0.f);
	Legendaries->NoteKill(*Gun, *Spider);
	TestEqual(TEXT("A hip kill: no lantern"), Legendaries->NumLanterns(), 0);
	Legendary->NoteShot(Now, 1.f);
	Legendaries->NoteKill(*Gun, *Spider);
	TestEqual(TEXT("A scoped kill: a lantern"), Legendaries->NumLanterns(), 1);

	// It heals its holder in reach, 2% of their health a second; a second lantern never heals on top of it.
	ALegendaryLantern* Lantern = Legendaries->LightLantern(FVector::ZeroVector, Holder);
	ALegendaryLantern* Other = Legendaries->LightLantern(FVector::ZeroVector, Holder);
	if (!TestNotNull(TEXT("Lanterns light"), Lantern) || !TestNotNull(TEXT("...two of them"), Other))
	{
		return false;
	}
	Health->Drain(Health->GetMaxHealth() * 0.5f);
	const float Hurt = Health->GetHealth();
	Lantern->Advance(1.f);
	TestEqual(TEXT("A second in its light heals 2%"), Health->GetHealth() - Hurt, Health->GetMaxHealth() * Tuning.LamplighterHealShare, 0.01f);
	Other->Advance(1.f);
	TestEqual(TEXT("...and the other lantern adds nothing that frame"), Other->GetHealed(), 0.f);
	Holder->SetActorLocation(FVector(1000.0, 0.0, 90.0));
	const float Away = Health->GetHealth();
	Lantern->Advance(1.f);
	TestEqual(TEXT("Out of its reach: nothing"), Health->GetHealth(), Away);
	TestTrue(TEXT("At most three burn at once"), Legendaries->NumLanterns() <= Tuning.LamplighterMaxLanterns);
	Lantern->Advance(Tuning.LamplighterSeconds);
	TestTrue(TEXT("It goes out at its end"), !IsValid(Lantern) || Lantern->IsActorBeingDestroyed());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLegendaryDropsTest, "Looter.Weapons.Legendary.Drops",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FLegendaryDropsTest::RunTest(const FString& Parameters)
{
	// Each boss names its own.
	TestTrue(TEXT("The Gravemother's: Gravesilk"), GravemotherFight::MakeLootShower().DedicatedLegendary == FName(LegendaryDrops::GravesilkId));
	TestTrue(TEXT("The Foreman's: Company Scrip"), ForemanFight::MakeLootShower().DedicatedLegendary == FName(LegendaryDrops::CompanyScripId));
	TestTrue(TEXT("Abel's: Lamplighter"), AbelRules::MakeLootShower().DedicatedLegendary == FName(LegendaryDrops::LamplighterId));
	TestTrue(TEXT("Any other shower: none"), FBossLootShowerSettings().DedicatedLegendary.IsNone());

	// One a kill, under its chance only; a named gun without a trick never drops this way.
	const FLegendaryTuning Tuning;
	UNamedWeaponDefinition* Named = MakeNamed(ELegendarySignature::Gravesilk);
	FLootRoll Roll;
	TestTrue(TEXT("A roll under the chance drops it"), LegendaryDrops::AddDedicated(Named, 5, 0.05f, Tuning, Roll));
	TestFalse(TEXT("...once a kill"), LegendaryDrops::AddDedicated(Named, 5, 0.f, Tuning, Roll));
	TestEqual(TEXT("...one gun"), Roll.Weapons.Num(), 1);
	TestTrue(TEXT("...the named gun itself, Legendary, at the boss's level"), Roll.Weapons.Num() == 1 && Roll.Weapons[0].Named == Named
		&& Roll.Weapons[0].Rarity == EWeaponRarity::Legendary && Roll.Weapons[0].Level == 5);
	FLootRoll Missed;
	TestFalse(TEXT("A roll over it: nothing"), LegendaryDrops::AddDedicated(Named, 5, 0.1f, Tuning, Missed));
	TestFalse(TEXT("A named gun with no trick: nothing"), LegendaryDrops::AddDedicated(MakeNamed(ELegendarySignature::None), 5, 0.f, Tuning, Missed));
	FRandomStream Random(7);
	TestFalse(TEXT("A boss with none named: nothing"), LegendaryDrops::RollForBoss(NAME_None, 5, Random, Missed));
	TestEqual(TEXT("...and nothing added"), Missed.Weapons.Num(), 0);

	// The three guns as the asset script made them (Tools/Unreal/create_named_weapons.py).
	struct FExpected
	{
		const TCHAR* Id;
		ELegendarySignature Signature;
		EWeaponKind Kind;
		const TCHAR* Line;
	};
	const FExpected Guns[] = {
		{ LegendaryDrops::GravesilkId, ELegendarySignature::Gravesilk, EWeaponKind::SMG, TEXT("Lie still. She's coming.") },
		{ LegendaryDrops::CompanyScripId, ELegendarySignature::CompanyScrip, EWeaponKind::LMG, TEXT("Paid in full. Eventually.") },
		{ LegendaryDrops::LamplighterId, ELegendarySignature::Lamplighter, EWeaponKind::Sniper, TEXT("Leave a light on.") },
	};
	for (const FExpected& Expected : Guns)
	{
		const UNamedWeaponDefinition* Asset = UNamedWeaponDefinition::FindByName(Expected.Id);
		if (!Asset)
		{
			AddError(FString::Printf(TEXT("DA_Named_%s isn't in /Game/Data/Weapons: run Tools/Unreal/create_named_weapons.py in the editor."), Expected.Id));
			continue;
		}
		TestTrue(FString::Printf(TEXT("%s: Legendary, its trick, its kind"), Expected.Id), Asset->Rarity == EWeaponRarity::Legendary
			&& Asset->Signature == Expected.Signature && Asset->Weapon && Asset->Weapon->Kind == Expected.Kind);
		TestEqual(FString::Printf(TEXT("%s: its line"), Expected.Id), Asset->FlavorText.ToString(), FString(Expected.Line));
		const TArray<FString> Problems = Asset->FindProblems();
		TestTrue(FString::Printf(TEXT("%s: its parts fit (%s)"), Expected.Id, *FString::Join(Problems, TEXT("; "))), Problems.IsEmpty());
	}
	return true;
}

#endif

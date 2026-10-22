#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Bosses/BossLootShower.h"
#include "Combat/HealthComponent.h"
#include "HAL/IConsoleManager.h"
#include "Loot/LootLibrary.h"
#include "Loot/LootTable.h"
#include "Shields/PlayerShieldComponent.h"
#include "Shields/ShieldDefinition.h"
#include "Shields/ShieldRules.h"
#include "Tests/AutomationCommon.h"
#include "Tests/RecoveryTestWorld.h"
#include "UI/HUD/HudPortraitWidget.h"
#include "UI/HUD/HudWardHit.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/Character.h"
#include "Weapons/WeaponDefinition.h"

// The ward's loose ends (SHIELD-POLISH, 2026-10-22): how the HUD tells a hit the ward took from a wound (cyan edges and
// glint, no red, no flinch; a break both), and a boss's shower throwing its wards with its guns, the rarest last.

namespace
{
	constexpr EAutomationTestFlags ShieldHudTestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;

	HudWardHit::FSeen ShieldHudSeen(uint32 Hits, uint32 Breaks, float ChargeBefore, float HealthLost)
	{
		HudWardHit::FSeen Seen;
		Seen.WardHits = Hits;
		Seen.WardBreaks = Breaks;
		Seen.ChargeBefore = ChargeBefore;
		Seen.HealthLost = HealthLost;
		return Seen;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShieldHudHitRuleTest, "Looter.Shields.HudHit.Rule", ShieldHudTestFlags)

bool FShieldHudHitRuleTest::RunTest(const FString& Parameters)
{
	using HudWardHit::Decide;
	using HudWardHit::FReaction;
	auto Is = [](const FReaction& Reaction, bool bWard, bool bWound) { return Reaction.bWard == bWard && Reaction.bWound == bWound; };

	TestTrue(TEXT("Nothing happened: nothing shows"), Is(Decide(ShieldHudSeen(0, 0, 40.f, 0.f)), false, false));
	TestTrue(TEXT("A hit the ward took whole: cyan, no red, no flinch"), Is(Decide(ShieldHudSeen(1, 0, 40.f, 0.f)), true, false));
	TestTrue(TEXT("A shotgun's pellets on the ward: the same"), Is(Decide(ShieldHudSeen(8, 0, 40.f, 0.f)), true, false));
	TestTrue(TEXT("The breaking hit, some of it through: both"), Is(Decide(ShieldHudSeen(1, 1, 10.f, 5.f)), true, true));
	TestTrue(TEXT("The breaking hit, none of it through: both all the same (the break is the warning)"),
		Is(Decide(ShieldHudSeen(1, 1, 10.f, 0.f)), true, true));
	TestTrue(TEXT("A hit on an empty ward: health's, red only"), Is(Decide(ShieldHudSeen(1, 0, 0.f, 12.f)), false, true));
	TestTrue(TEXT("No ward: red, as before"), Is(Decide(ShieldHudSeen(0, 0, 0.f, 20.f)), false, true));
	TestTrue(TEXT("Its refill's first sliver soaking a hit: cyan"), Is(Decide(ShieldHudSeen(1, 0, 0.f, 0.f)), true, false));
	TestTrue(TEXT("Health's rounding wobble: nothing"), Is(Decide(ShieldHudSeen(0, 0, 0.f, HudWardHit::HealthStep * 0.5f)), false, false));
	TestTrue(TEXT("A drain on health while the ward soaks a hit: both"), Is(Decide(ShieldHudSeen(1, 0, 30.f, 3.f)), true, true));

	// The feel numbers: lighter and quicker than the red (0.85 over 0.55 s), every one a cvar.
	const HudWardHit::FTuning Tuning;
	TestTrue(TEXT("The cyan edges are lighter than the red and fade sooner"), Tuning.EdgePeak < 0.85f && Tuning.EdgeSeconds < 0.55f
		&& Tuning.EdgePeak > 0.f && Tuning.EdgeSeconds > 0.f);
	TestTrue(TEXT("The portrait's glint is lighter than its red flash (0.6)"), Tuning.GlintPeak > 0.f && Tuning.GlintPeak < 0.6f && Tuning.GlintSeconds > 0.f);
	for (const TCHAR* Name : { TEXT("Looter.Shield.Hud.EdgeSeconds"), TEXT("Looter.Shield.Hud.EdgePeak"), TEXT("Looter.Shield.Hud.GlintSeconds"),
		TEXT("Looter.Shield.Hud.GlintPeak") })
	{
		TestNotNull(FString::Printf(TEXT("%s exists"), Name), IConsoleManager::Get().FindConsoleVariable(Name));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShieldHudHitWatchTest, "Looter.Shields.HudHit.Watch", ShieldHudTestFlags)

bool FShieldHudHitWatchTest::RunTest(const FString& Parameters)
{
	// The HUD's watch on a real health component with a ward in front of it, a look a frame.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	UHealthComponent* Health = nullptr;
	ACharacter* Player = RecoveryTestWorld::SpawnPlayer(World, FVector::ZeroVector, Health);
	UPlayerShieldComponent* Shield = Player ? UPlayerShieldComponent::FindOrAdd(*Player) : nullptr;
	if (!TestNotNull(TEXT("Player stand-in"), Player) || !TestNotNull(TEXT("Its health"), Health) || !TestNotNull(TEXT("Its ward"), Shield))
	{
		return false;
	}
	FHudWardHitWatch Watch;
	TestFalse(TEXT("The first look: nothing to react to"), Watch.Look(Player).Any());
	TestTrue(TEXT("...and it found the health and the ward"), Watch.GetHealth() == Health && Watch.GetShield() == Shield);

	// No ward worn: a hit is a wound.
	RecoveryTestWorld::Hurt(Player, 10.f);
	HudWardHit::FReaction Seen = Watch.Look(Player);
	TestTrue(TEXT("No ward: the hit is red"), Seen.bWound && !Seen.bWard);
	Health->ResetHealth();
	Watch.Look(Player);

	Shield->Equip(ShieldRules::MakeWith(NewObject<UShieldDefinition>(), EWeaponRarity::Common, 1, 7, EShieldBuild::Standard, EShieldTrait::None), true);
	TestFalse(TEXT("Put on: nothing"), Watch.Look(Player).Any());

	RecoveryTestWorld::Hurt(Player, 20.f);
	Seen = Watch.Look(Player);
	TestTrue(TEXT("A 20 hit on a full ward: cyan, no red"), Seen.bWard && !Seen.bWound);
	TestFalse(TEXT("...once: the next look sees nothing new"), Watch.Look(Player).Any());

	RecoveryTestWorld::Hurt(Player, Shield->GetCharge() + 20.f);
	Seen = Watch.Look(Player);
	TestTrue(TEXT("The breaking hit: both"), Seen.bWard && Seen.bWound && Shield->IsBroken());

	RecoveryTestWorld::Hurt(Player, 5.f);
	Seen = Watch.Look(Player);
	TestTrue(TEXT("Broken: the next hit is red only"), Seen.bWound && !Seen.bWard);

	// Another pawn (none, then this one again, a respawn's way): taken as it is, no reaction for what came between.
	TestFalse(TEXT("No pawn: nothing"), Watch.Look(nullptr).Any());
	RecoveryTestWorld::Hurt(Player, 5.f);
	TestFalse(TEXT("Back on a pawn: nothing that first look"), Watch.Look(Player).Any());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShieldHudPortraitGlintTest, "Looter.HUD.PortraitWardGlint", ShieldHudTestFlags)

bool FShieldHudPortraitGlintTest::RunTest(const FString& Parameters)
{
	// A ward hit's glint in the portrait's window starts at its peak (and none before), through the red flash's own picture.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UHudPortraitWidget* Portrait = CreateWidget<UHudPortraitWidget>(TestLevel.GetTestWorld(), UHudPortraitWidget::StaticClass());
	if (!TestNotNull(TEXT("The portrait window"), Portrait))
	{
		return false;
	}
	const TSharedRef<SWidget> PortraitSlate = Portrait->TakeWidget();
	TestEqual(TEXT("Calm: no glint"), Portrait->GetWardGlint(), 0.f);
	Portrait->PlayWardHit();
	TestEqual(TEXT("A ward hit: the glint at its peak"), Portrait->GetWardGlint(), HudWardHit::FTuning::Live().GlintPeak, 0.001f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShieldBossShowerTest, "Looter.Shields.BossShower", ShieldHudTestFlags)

bool FShieldBossShowerTest::RunTest(const FString& Parameters)
{
	// The order: the ammo, then guns and wards together by rarity (a ward before a gun of its rarity), the rarest last.
	FLootRoll Roll;
	Roll.Ammo.AddDefaulted(1);
	Roll.Weapons.AddDefaulted_GetRef().Rarity = EWeaponRarity::Epic;
	Roll.Weapons.AddDefaulted_GetRef().Rarity = EWeaponRarity::Common;
	Roll.Shields.AddDefaulted_GetRef().Rarity = EWeaponRarity::Rare;
	Roll.Shields.AddDefaulted_GetRef().Rarity = EWeaponRarity::Common;
	TestTrue(TEXT("Ammo, the Common ward, the Common gun, the Rare ward, the Epic gun"),
		ABossLootShower::Order(Roll) == TArray<int32>({ -1, 3, 1, 2, 0 }));
	TestEqual(TEXT("Pieces past the guns are wards"), ABossLootShower::WardIndexOf(Roll, 3), 1);
	TestEqual(TEXT("...the first of them"), ABossLootShower::WardIndexOf(Roll, 2), 0);
	TestEqual(TEXT("A gun is no ward"), ABossLootShower::WardIndexOf(Roll, 1), static_cast<int32>(INDEX_NONE));
	TestEqual(TEXT("An ammo box is no ward"), ABossLootShower::WardIndexOf(Roll, -1), static_cast<int32>(INDEX_NONE));
	TestEqual(TEXT("Past the wards: nothing"), ABossLootShower::WardIndexOf(Roll, 4), static_cast<int32>(INDEX_NONE));

	// The plan: a ward-dropping boss table's picks split into guns and wards, every piece in the order once.
	ULootTable* Table = NewObject<ULootTable>();
	FLootTableEntry Entry;
	Entry.Weapon = NewObject<UWeaponDefinition>();
	Table->Entries.Add(Entry);
	Table->WeaponDropChance = 1.f;
	Table->MinWeaponDrops = 4;
	Table->MaxWeaponDrops = 4;
	Table->AmmoDropChance = 0.f;
	Table->bDropsShields = true;
	constexpr int32 Bonus = 2;
	FRandomStream Random(20261022);
	int32 Wards = 0;
	bool bCounts = true;
	bool bOrders = true;
	bool bRarestLast = true;
	for (int32 Kill = 0; Kill < 200; ++Kill)
	{
		const FLootRoll Planned = ABossLootShower::Plan(Table, 5, 0.f, Random, {}, true, Bonus);
		Wards += Planned.Shields.Num();
		bCounts &= Planned.Weapons.Num() + Planned.Shields.Num() == 4 && Planned.Ammo.Num() == Bonus;
		const TArray<int32> Pieces = ABossLootShower::Order(Planned);
		TSet<int32> Seen;
		Seen.Append(Pieces);
		bOrders &= Pieces.Num() == 4 + Bonus && Seen.Num() == Pieces.Num();
		// The last piece is of the roll's top rarity.
		EWeaponRarity Top = EWeaponRarity::Common;
		for (const FWeaponInstanceData& Gun : Planned.Weapons)
		{
			Top = FMath::Max(Top, Gun.Rarity);
		}
		for (const FShieldInstanceData& Ward : Planned.Shields)
		{
			Top = FMath::Max(Top, Ward.Rarity);
		}
		const int32 Last = Pieces.Last();
		const int32 LastWard = ABossLootShower::WardIndexOf(Planned, Last);
		const EWeaponRarity LastRarity = LastWard != INDEX_NONE ? Planned.Shields[LastWard].Rarity : Planned.Weapons[Last].Rarity;
		bRarestLast &= LastRarity == Top;
	}
	TestTrue(TEXT("Every shower: four picks (guns or wards) and its bonus boxes"), bCounts);
	TestTrue(TEXT("...each piece in the order once"), bOrders);
	TestTrue(TEXT("...the rarest last"), bRarestLast);
	TestTrue(FString::Printf(TEXT("...and wards among them (%d in 800 picks)"), Wards), Wards > 0);
	return true;
}

#endif

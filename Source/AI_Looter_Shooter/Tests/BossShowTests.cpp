#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

// Every boss's show (UBossComponent, BossComponentShow.cpp): the stagger's rules and its course in a fight, the camera's
// shake, the loot shower's plan and order (and its taking over the loot drop's toss), the bar's intro, callouts and last
// word, and the death that leaves the world's clock alone in a test level.

#include "Bosses/BossCameraShake.h"
#include "Bosses/BossComponent.h"
#include "Bosses/BossLootShower.h"
#include "Bosses/BossRules.h"
#include "Bosses/BossTestSpider.h"
#include "Combat/HealthComponent.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/CreatureRankSettings.h"
#include "Loot/LootDropComponent.h"
#include "Loot/LootTable.h"
#include "Tests/BossTestWorld.h"
#include "UI/HUD/HudBossBarWidget.h"
#include "Blueprint/UserWidget.h"
#include "Camera/CameraTypes.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "Math/RandomStream.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Package.h"

using namespace BossTestWorld;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBossShowRulesTest, "Looter.Bosses.Show.Rules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBossShowRulesTest::RunTest(const FString& Parameters)
{
	// The stagger: 5% of 10,000 health in crits staggers it; a crit adds its share; a full build-up drains over its seconds.
	FBossStagger Stagger;
	Stagger.CritShare = 0.05f;
	Stagger.DrainSeconds = 4.f;
	TestNearlyEqual(TEXT("A 250 crit on a 10,000 boss: half way to a stagger"), BossRules::AddCritToStagger(0.f, 250.f, 10000.f, Stagger), 0.5f, 0.001f);
	TestNearlyEqual(TEXT("...and 300 more: staggered, never past full"), BossRules::AddCritToStagger(0.5f, 300.f, 10000.f, Stagger), 1.f, 0.001f);
	TestNearlyEqual(TEXT("Full, two seconds later: half"), BossRules::DrainStagger(1.f, 2.f, Stagger), 0.5f, 0.001f);
	TestNearlyEqual(TEXT("...and never below nothing"), BossRules::DrainStagger(0.1f, 2.f, Stagger), 0.f, 0.001f);
	FBossStagger Never;
	TestNearlyEqual(TEXT("A boss that never staggers builds nothing"), BossRules::AddCritToStagger(0.f, 5000.f, 10000.f, Never), 0.f, 0.001f);

	// The shower's throws: every piece lands between its reaches, the next well away from the last, all round the ring.
	FBossLootShowerSettings Shower;
	Shower.MinReach = 160.f;
	Shower.MaxReach = 430.f;
	Shower.UpSpeed = 780.f;
	int32 Quadrants[4] = { 0, 0, 0, 0 };
	for (int32 Index = 0; Index < 16; ++Index)
	{
		const float Reach = BossRules::ShowerReach(Index, Shower);
		TestTrue(FString::Printf(TEXT("Piece %d lands between the reaches (%.0f cm)"), Index, Reach), Reach >= Shower.MinReach && Reach <= Shower.MaxReach);
		if (Index > 0)
		{
			TestTrue(TEXT("...well away from the one before"), FMath::Abs(Reach - BossRules::ShowerReach(Index - 1, Shower)) > 0.1f * (Shower.MaxReach - Shower.MinReach));
		}
		const FVector Throw = BossRules::ShowerThrow(Index, 30.f, Shower, 980.f);
		TestNearlyEqual(TEXT("...thrown up at its speed"), static_cast<float>(Throw.Z), Shower.UpSpeed, 0.01f);
		// Up and back down takes 2U/g; out at the throw's flat speed for that long is the reach.
		const float Landed = static_cast<float>(Throw.Size2D()) * 2.f * Shower.UpSpeed / 980.f;
		TestNearlyEqual(TEXT("...and lands at its reach on level ground"), Landed, Reach, 1.f);
		if (Index < 8)
		{
			++Quadrants[(Throw.X >= 0.0 ? 0 : 1) + (Throw.Y >= 0.0 ? 0 : 2)];
		}
	}
	TestTrue(TEXT("The first eight pieces fly every way round"), Quadrants[0] > 0 && Quadrants[1] > 0 && Quadrants[2] > 0 && Quadrants[3] > 0);

	// The camera's shake: full near, nothing past its reach, weaker the farther.
	TestNearlyEqual(TEXT("A kick right by the player: full"), BossCameraShake::Falloff(0.8f, 100.f, 4500.f), 0.8f, 0.001f);
	TestNearlyEqual(TEXT("...past its reach: nothing"), BossCameraShake::Falloff(0.8f, 4600.f, 4500.f), 0.f, 0.001f);
	TestTrue(TEXT("...weaker the farther"), BossCameraShake::Falloff(0.8f, 2000.f, 4500.f) > BossCameraShake::Falloff(0.8f, 3000.f, 4500.f));

	// The camera modifier: a shake dies away as the square of its time left, and kicks stack only so far.
	UCameraShakeModifier* Shaker = NewObject<UCameraShakeModifier>(GetTransientPackage());
	Shaker->AddShake(0.5f, 1.f);
	TestNearlyEqual(TEXT("A half-strength kick: half"), Shaker->GetAmount(), 0.5f, 0.001f);
	FMinimalViewInfo View;
	View.Rotation = FRotator::ZeroRotator;
	View.Location = FVector::ZeroVector;
	TestFalse(TEXT("It never stops the modifiers after it"), Shaker->ModifyCamera(0.5f, View));
	TestNearlyEqual(TEXT("Half way through: a quarter of it"), Shaker->GetAmount(), 0.125f, 0.001f);
	Shaker->ModifyCamera(0.6f, View);
	TestNearlyEqual(TEXT("Over: still"), Shaker->GetAmount(), 0.f, 0.001f);
	for (int32 Kick = 0; Kick < 10; ++Kick)
	{
		Shaker->AddShake(1.f, 2.f);
	}
	TestNearlyEqual(TEXT("Kicks stack no higher than its most"), Shaker->GetAmount(), UCameraShakeModifier::MaxAmount, 0.001f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBossLootShowerTest, "Looter.Bosses.Show.LootShower",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBossLootShowerTest::RunTest(const FString& Parameters)
{
	// The order pieces fly: the ammo, then the guns from the least rare to the rarest.
	FLootRoll Roll;
	for (const EWeaponRarity Rarity : { EWeaponRarity::Epic, EWeaponRarity::Common, EWeaponRarity::Legendary })
	{
		Roll.Weapons.AddDefaulted_GetRef().Rarity = Rarity;
	}
	Roll.Ammo.AddDefaulted(2);
	TestTrue(TEXT("Ammo first, then the guns, the rarest last"), ABossLootShower::Order(Roll) == TArray<int32>({ -1, -2, 1, 0, 2 }));

	// What a shower throws: its table rolled as a kill's, plus its bonus boxes, each a chest's full box.
	for (const ECreatureRank Rank : { ECreatureRank::Boss, ECreatureRank::Legendary })
	{
		const ULootTable* Table = UCreatureRankSettings::GetLootTable(Rank);
		if (!TestNotNull(TEXT("The rank's loot table"), Table))
		{
			return false;
		}
		const TCHAR* Name = Rank == ECreatureRank::Boss ? TEXT("Boss") : TEXT("Legendary");
		constexpr int32 Bonus = 3;
		FRandomStream Random(20261008);
		bool bCounts = true;
		bool bBonusFull = true;
		for (int32 Kill = 0; Kill < 40; ++Kill)
		{
			const FLootRoll Planned = ABossLootShower::Plan(Table, 9, 0.f, Random, {}, true, Bonus);
			const int32 Guns = Planned.Weapons.Num();
			const int32 Ammo = Planned.Ammo.Num();
			bCounts &= (Table->Entries.IsEmpty() || (Guns >= Table->MinWeaponDrops && Guns <= Table->MaxWeaponDrops))
				&& Ammo >= Table->MinAmmoDrops + Bonus && Ammo <= Table->MaxAmmoDrops + Bonus;
			for (int32 Box = Ammo - Bonus; Box < Ammo; ++Box)
			{
				bBonusFull &= Planned.Ammo.IsValidIndex(Box) && Planned.Ammo[Box].Amount == LooterLoot::ChestAmmoAmount;
			}
		}
		TestTrue(FString::Printf(TEXT("%s: every shower is its table's guns and ammo, and %d boxes more"), Name, Bonus), bCounts);
		TestTrue(FString::Printf(TEXT("%s: the bonus boxes are full"), Name), bBonusFull);
		if (Rank == ECreatureRank::Boss && !Table->Entries.IsEmpty())
		{
			TestEqual(TEXT("Boss: three guns, as Abel's design has them"), ABossLootShower::Plan(Table, 9, 0.f, Random, {}, true, Bonus).Weapons.Num(), 3);
		}
		const FLootRoll NoGuns = ABossLootShower::Plan(Table, 9, 0.f, Random, {}, false, Bonus);
		TestTrue(FString::Printf(TEXT("%s: where guns don't drop, only the ammo"), Name), NoGuns.Weapons.IsEmpty() && NoGuns.Ammo.Num() >= Bonus);
	}

	// In a fight: the boss's shower takes over its loot drop's toss, and its death starts the shower (no NoLoot here).
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	ACharacter* Player = SpawnPlayer(World, FVector(600.0, 0.0, 0.0));
	UBossComponent* Boss = BossTestSpider::Spawn(World, FVector::ZeroVector, 0.f);
	ULootDropComponent* Loot = Boss ? Boss->GetOwner()->FindComponentByClass<ULootDropComponent>() : nullptr;
	if (!TestNotNull(TEXT("Player stand-in"), Player) || !TestNotNull(TEXT("Test boss and its loot"), Loot))
	{
		return false;
	}
	TestFalse(TEXT("Its loot drop doesn't toss it: the shower throws it"), Loot->bDropOnDeath);
	Boss->StartFight(Player);
	Hurt(Boss->GetOwner(), 1.0e7f);
	TestTrue(TEXT("Its death wins the fight"), Boss->IsWon());
	TArray<ABossLootShower*> Showers;
	for (TActorIterator<ABossLootShower> It(World); It; ++It)
	{
		Showers.Add(*It);
	}
	if (TestEqual(TEXT("One shower starts"), Showers.Num(), 1))
	{
		const FLootRoll& Thrown = Showers[0]->GetRoll();
		TestEqual(TEXT("...its pieces are the roll's guns and ammo"), Showers[0]->NumPieces(), Thrown.Weapons.Num() + Thrown.Ammo.Num());
		TestTrue(TEXT("...with its bonus ammo"), Thrown.Ammo.Num() >= Boss->LootShower.BonusAmmo);
		TestEqual(TEXT("...nothing thrown before its delay"), Showers[0]->NumThrown(), 0);
		Showers[0]->Destroy();
	}

	// With the tests' NoLoot, nothing at all.
	UBossComponent* Quiet = BossTestSpider::Spawn(World, FVector(0.0, 5000.0, 0.0), 0.f);
	ACharacter* Other = SpawnPlayer(World, FVector(600.0, 5000.0, 0.0));
	if (TestNotNull(TEXT("Second test boss"), Quiet) && TestNotNull(TEXT("Second stand-in"), Other))
	{
		Quiet->StartFight(Other);
		Kill(Quiet->GetOwner());
		int32 Count = 0;
		for (TActorIterator<ABossLootShower> It(World); It; ++It)
		{
			Count += IsValid(*It) ? 1 : 0;
		}
		TestEqual(TEXT("NoLoot: no shower"), Count, 0);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBossStaggerTest, "Looter.Bosses.Show.Stagger",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBossStaggerTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	ACharacter* Player = nullptr;
	UBossComponent* Boss = StartTestFight(*this, WorldWrapper.GetTestWorld(), Player);
	if (!Boss)
	{
		return false;
	}
	Boss->Stagger.Seconds = 2.5f;
	TArray<bool> Heard;
	Boss->OnStaggered.AddLambda([&Heard](bool bStaggered) { Heard.Add(bStaggered); });

	// Staggered for its seconds, once at a time; then it's over, said both times.
	TestTrue(TEXT("In its fight it can be staggered"), Boss->BeginStagger() && Boss->IsStaggered());
	TestFalse(TEXT("...not twice at once"), Boss->BeginStagger());
	Boss->TickFight(2.4f);
	TestTrue(TEXT("2.4 s into a 2.5 s stagger: still"), Boss->IsStaggered());
	Boss->TickFight(0.2f);
	TestFalse(TEXT("2.6 s: over"), Boss->IsStaggered());
	TestTrue(TEXT("...heard as it began and as it ended"), Heard == TArray<bool>({ true, false }));

	// Its creature's say: no stagger while it can't.
	Boss->CanStagger.BindLambda([]() { return false; });
	TestFalse(TEXT("Its creature says no: none"), Boss->BeginStagger());
	Boss->CanStagger.Unbind();

	// Not while it can't be hurt.
	HurtTo(*Boss, 0.5f);
	TestTrue(TEXT("Under half the test boss can't be hurt"), Boss->IsUntargetable());
	TestFalse(TEXT("...nor staggered"), Boss->BeginStagger());
	KillAdds(*Boss);
	Boss->TickFight(0.1f);

	// The fight starting over ends a stagger under way.
	TestTrue(TEXT("Hurtable again: staggered"), Boss->BeginStagger());
	Boss->ResetFight();
	TestFalse(TEXT("A reset ends it"), Boss->IsStaggered());
	TestNearlyEqual(TEXT("...with nothing built up"), Boss->GetStaggerBuildUp(), 0.f, 0.001f);
	TestFalse(TEXT("Out of its fight, none"), Boss->BeginStagger());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBossBarShowTest, "Looter.Bosses.Show.Bar",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBossBarShowTest::RunTest(const FString& Parameters)
{
	// The title spelled out a letter at a time, in capitals, from a moment in.
	const FText Title = FText::FromString(TEXT("Brood of the Sink"));
	TestTrue(TEXT("At the intro's start: nothing yet"), UHudBossBarWidget::MakeIntroTitle(Title, 0.f).IsEmpty());
	const FString Partway = UHudBossBarWidget::MakeIntroTitle(Title, 0.5f).ToString();
	TestTrue(FString::Printf(TEXT("Half a second in: its first letters (%s)"), *Partway), Partway.Len() > 0 && Partway.Len() < 17
		&& FString(TEXT("BROOD OF THE SINK")).StartsWith(Partway));
	TestEqual(TEXT("Well in: all of it"), UHudBossBarWidget::MakeIntroTitle(Title, 3.f).ToString(), FString(TEXT("BROOD OF THE SINK")));

	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UHudBossBarWidget* Bar = CreateWidget<UHudBossBarWidget>(WorldWrapper.GetTestWorld(), UHudBossBarWidget::StaticClass());
	if (!TestNotNull(TEXT("The boss bar"), Bar))
	{
		return false;
	}
	Bar->SetBoss(FText::FromString(TEXT("The Gravemother")), 8, UCreatureRankSettings::Get(ECreatureRank::Legendary).Color, { 1.f, 0.66f, 0.33f });
	Bar->TakeWidget();
	Bar->SetPhase(0, FText::FromString(TEXT("Fresh Meat")));
	Bar->SetTitle(Title);
	Bar->PlayIntro();
	TestTrue(TEXT("The intro plays"), Bar->IsIntroPlaying());
	TestNearlyEqual(TEXT("...its fill runs up from empty"), Bar->GetIntroSweep(), 0.f, 0.001f);
	TestNearlyEqual(TEXT("...though the health it shows is full"), Bar->GetFraction(), 1.f, 0.001f);
	TestTrue(TEXT("...the title's line not yet spelled"), Bar->GetPhaseText().IsEmpty());

	// A hit flashes it; a callout takes the line; the death's word stays.
	Bar->SetHealth(90.f, 100.f);
	TestTrue(TEXT("A hit flashes the fill"), Bar->GetHitFlash() > 0.f);
	Bar->ShowCallout(FText::FromString(TEXT("Staggered")), 2.5f);
	TestEqual(TEXT("A callout takes the line"), Bar->GetPhaseText().ToString(), FString(TEXT("STAGGERED")));
	Bar->PlayDefeated(FText::FromString(TEXT("Slain")));
	TestTrue(TEXT("Its death: the bar says so"), Bar->IsDefeated() && Bar->GetPhaseText().ToString() == TEXT("SLAIN"));
	TestFalse(TEXT("...and the intro's over"), Bar->IsIntroPlaying());
	Bar->ShowCallout(FText::FromString(TEXT("Staggered")), 2.5f);
	TestEqual(TEXT("...no callout after it"), Bar->GetPhaseText().ToString(), FString(TEXT("SLAIN")));
	Bar->PlayDefeated(FText::GetEmpty());
	TestEqual(TEXT("With no word of its own: DEFEATED"), Bar->GetPhaseText().ToString(), FString(TEXT("DEFEATED")));

	// A new fight: none of it left.
	Bar->SetBoss(FText::FromString(TEXT("The Gravemother")), 8, UCreatureRankSettings::Get(ECreatureRank::Legendary).Color, { 1.f, 0.66f, 0.33f });
	Bar->SetPhase(0, FText::FromString(TEXT("Fresh Meat")));
	TestTrue(TEXT("A new fight: not beaten, no intro, its phase's name"), !Bar->IsDefeated() && !Bar->IsIntroPlaying()
		&& Bar->GetPhaseText().ToString() == TEXT("FRESH MEAT"));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBossDeathShowTest, "Looter.Bosses.Show.Death",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBossDeathShowTest::RunTest(const FString& Parameters)
{
	// Its death's slow beat is a game's: a test level's clock is left alone. The show's defaults are every boss's.
	const FBossShow Defaults;
	TestTrue(TEXT("By default the world slows at a boss's death, for under a second"), Defaults.DeathSlowMo < 1.f && Defaults.DeathSlowSeconds > 0.f
		&& Defaults.DeathSlowSeconds < 1.f);
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	ACharacter* Player = nullptr;
	UBossComponent* Boss = StartTestFight(*this, World, Player);
	if (!Boss)
	{
		return false;
	}
	Kill(Boss->GetOwner());
	TestTrue(TEXT("Its death wins the fight"), Boss->IsWon());
	TestFalse(TEXT("...and leaves a test level's clock alone"), Boss->IsDeathSlowOn());
	TestFalse(TEXT("...no stagger left"), Boss->IsStaggered());
	return true;
}

#endif

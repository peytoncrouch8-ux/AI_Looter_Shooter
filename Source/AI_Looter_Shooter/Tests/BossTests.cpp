#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Bosses/BossComponent.h"
#include "Bosses/BossRules.h"
#include "Bosses/BossSeal.h"
#include "Bosses/BossTestSpider.h"
#include "Combat/EnemyProjectileSubsystem.h"
#include "Combat/HealthComponent.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/CreatureRankSettings.h"
#include "Creatures/SpiderCreature.h"
#include "Tests/BossTestWorld.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Tests/AutomationCommon.h"

using namespace BossTestWorld;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBossPhasesTest, "Looter.Bosses.Phases",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBossPhasesTest::RunTest(const FString& Parameters)
{
	// Phases by the share of health left: highest first, the first from full health, a share on the line counting.
	const TArray<FBossPhase> Ordered = BossRules::Ordered(BossTestSpider::MakePhases(3));
	if (!TestEqual(TEXT("The test boss has three phases"), Ordered.Num(), 3))
	{
		return false;
	}
	TestEqual(TEXT("Full health: the first"), BossRules::PhaseAt(Ordered, 1.f), 0);
	TestEqual(TEXT("Just over half: still the first"), BossRules::PhaseAt(Ordered, 0.51f), 0);
	TestEqual(TEXT("Exactly half: the second"), BossRules::PhaseAt(Ordered, 0.5f), 1);
	TestEqual(TEXT("A third: the second"), BossRules::PhaseAt(Ordered, 0.33f), 1);
	TestEqual(TEXT("A quarter: the third"), BossRules::PhaseAt(Ordered, 0.25f), 2);
	TestEqual(TEXT("Nothing left: the third"), BossRules::PhaseAt(Ordered, 0.f), 2);

	TArray<FBossPhase> Mixed;
	for (const TPair<const TCHAR*, float>& Data : { TPair<const TCHAR*, float>(TEXT("C"), 0.25f), TPair<const TCHAR*, float>(TEXT("A"), 0.9f),
		TPair<const TCHAR*, float>(TEXT("B"), 0.6f) })
	{
		FBossPhase& Added = Mixed.AddDefaulted_GetRef();
		Added.Name = FText::FromString(Data.Key);
		Added.HealthShare = Data.Value;
	}
	const TArray<FBossPhase> Sorted = BossRules::Ordered(Mixed);
	TestTrue(TEXT("Written in any order, they play highest first"), Sorted.Num() == 3 && Sorted[0].Name.ToString() == TEXT("A")
		&& Sorted[1].Name.ToString() == TEXT("B") && Sorted[2].Name.ToString() == TEXT("C"));
	TestEqual(TEXT("The first starts at full health whatever it says"), Sorted[0].HealthShare, 1.f);
	TestEqual(TEXT("No phases make one"), BossRules::Ordered(TArray<FBossPhase>()).Num(), 1);
	TestEqual(TEXT("Five phases asked for"), BossTestSpider::MakePhases(5).Num(), 5);
	TestEqual(TEXT("Too many asked for: five"), BossTestSpider::MakePhases(9).Num(), BossTestSpider::MaxPhases);

	// In a fight: each phase starts as its line is crossed, once, in order, and never goes back.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	ACharacter* Player = SpawnPlayer(World, FVector(600.0, 0.0, 0.0));
	UBossComponent* Boss = BossTestSpider::Spawn(World, FVector::ZeroVector, 0.f, 3);
	if (!TestNotNull(TEXT("Player stand-in"), Player) || !TestNotNull(TEXT("Test boss"), Boss))
	{
		return false;
	}
	NoLoot(Boss->GetOwner());
	TArray<int32> Started;
	Boss->OnPhaseChanged.AddLambda([&Started](int32 NewPhase, int32 OldPhase) { Started.Add(NewPhase); });
	Boss->StartFight(Player);
	TestTrue(TEXT("The fight starts in the first phase"), Boss->IsFighting() && Boss->GetPhase() == 0 && Started.Num() == 1);
	TestTrue(TEXT("Its bar's ticks are its phases' shares"), Boss->GetPhaseShares() == TArray<float>({ 1.f, 0.5f, 0.25f }));
	TestTrue(TEXT("The boss is a Boss-rank brown spider"), Cast<ASpiderCreature>(Boss->GetOwner()) && Boss->GetCreature()->GetRank() == ECreatureRank::Boss);
	TestFalse(TEXT("A boss's own tag steps aside for its bar"), Boss->GetCreature()->bShowsHealthTag);

	HurtTo(*Boss, 0.5f, /*bOver*/ true);
	TestEqual(TEXT("Just over half: still the first phase"), Boss->GetPhase(), 0);
	HurtTo(*Boss, 0.5f);
	TestEqual(TEXT("Under half: the second phase"), Boss->GetPhase(), 1);
	TestTrue(TEXT("...which casts its spell"), Boss->IsUntargetable());
	TestEqual(TEXT("...and sends its brood"), Boss->NumAliveAdds(), BossTestSpider::Brood().Count);

	KillAdds(*Boss);
	Boss->TickFight(0.1f);
	TestFalse(TEXT("Its brood dead, it can be hurt again"), Boss->IsUntargetable());
	HurtTo(*Boss, 0.25f, /*bOver*/ true);
	TestEqual(TEXT("Just over a quarter: still the second phase"), Boss->GetPhase(), 1);
	HurtTo(*Boss, 0.25f);
	TestEqual(TEXT("Under a quarter: the third phase"), Boss->GetPhase(), 2);
	TestTrue(TEXT("Each phase started once, in order"), Started == TArray<int32>({ 0, 1, 2 }));

	// The third phase spits venom: its first volley 1.5 s in, after a 0.6 s tell, five pellets at the player.
	UEnemyProjectileSubsystem* Shots = World->GetSubsystem<UEnemyProjectileSubsystem>();
	if (TestNotNull(TEXT("The level has enemy pellets"), Shots))
	{
		Boss->TickFight(1.f);
		TestFalse(TEXT("No volley before its time"), Boss->IsVolleyWindingUp() || Shots->NumShotsFrom(Boss->GetOwner()) > 0);
		Boss->TickFight(0.6f);
		TestTrue(TEXT("Its tell glows first"), Boss->IsVolleyWindingUp() && Shots->NumShotsFrom(Boss->GetOwner()) == 0);
		Boss->TickFight(0.7f);
		TestEqual(TEXT("Then five pellets fly"), Shots->NumShotsFrom(Boss->GetOwner()), BossTestSpider::VenomVolley().Pellets);
	}

	// Healed, it doesn't go back to an earlier phase.
	Boss->GetBossHealth()->SetHealth(Boss->GetBossHealth()->GetMaxHealth());
	Boss->TickFight(0.1f);
	TestEqual(TEXT("Healed, it stays in the third phase"), Boss->GetPhase(), 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBossUntargetableTest, "Looter.Bosses.Untargetable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBossUntargetableTest::RunTest(const FString& Parameters)
{
	// When a spell ends: its time, or its adds all dead with no wave of its phase still to come; or only the boss's code.
	FBossUntargetable UntilAdds;
	UntilAdds.Seconds = 45.f;
	UntilAdds.bUntilAddsDie = true;
	TestFalse(TEXT("Adds alive: it holds"), BossRules::UntargetableEnds(UntilAdds, 10.f, 1, false));
	TestTrue(TEXT("Adds dead: it ends"), BossRules::UntargetableEnds(UntilAdds, 10.f, 0, false));
	TestFalse(TEXT("Adds dead but a wave still to come: it holds"), BossRules::UntargetableEnds(UntilAdds, 10.f, 0, true));
	TestTrue(TEXT("Its time is up: it ends, adds or not"), BossRules::UntargetableEnds(UntilAdds, 45.f, 3, false));
	FBossUntargetable Scripted;
	Scripted.bUntilAddsDie = false;
	TestFalse(TEXT("No time and no adds to wait for: only the boss's code ends it"), BossRules::UntargetableEnds(Scripted, 1000.f, 0, false));

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
	ACreatureBase* Creature = Boss->GetCreature();
	UHealthComponent* Health = Boss->GetBossHealth();
	const bool bShowedNumbers = Health->bShowDamageNumbers;
	HurtTo(*Boss, 0.5f);
	if (!TestTrue(TEXT("Under half the test boss can't be hurt"), Boss->IsUntargetable()))
	{
		return false;
	}

	// Hits land and take nothing, with no numbers to say otherwise; it withdraws and hunts nobody.
	const float Before = Health->GetHealth();
	Hurt(Creature, 500.f);
	TestEqual(TEXT("A hit takes nothing"), Health->GetHealth(), Before);
	TestFalse(TEXT("...and shows no number"), Health->bShowDamageNumbers);
	TestTrue(TEXT("It's held back from the fight"), Creature->IsPassive());
	Boss->TickFight(10.f);
	TestTrue(TEXT("10 s on, its brood alive: still untargetable"), Boss->IsUntargetable());

	TArray<ACreatureBase*> Brood = Boss->GetAliveAdds();
	for (int32 Index = 0; Index + 1 < Brood.Num(); ++Index)
	{
		Kill(Brood[Index]);
	}
	Boss->TickFight(0.1f);
	TestTrue(TEXT("One spiderling left: still untargetable"), Boss->IsUntargetable());
	KillAdds(*Boss);
	Boss->TickFight(0.1f);
	TestFalse(TEXT("The brood dead: it can be hurt"), Boss->IsUntargetable());
	Hurt(Creature, 100.f);
	TestNearlyEqual(TEXT("A hit takes its damage again"), Health->GetHealth(), Before - 100.f, 0.01f);
	TestTrue(TEXT("Numbers as the boss had them"), Health->bShowDamageNumbers == bShowedNumbers);
	TestFalse(TEXT("Not invulnerable"), Health->bInvulnerable);
	TestFalse(TEXT("Back in the fight"), Creature->IsPassive());

	// A spell for a time only ends when its time is up; the boss's code can end one at once.
	FBossUntargetable Timed;
	Timed.Seconds = 5.f;
	Timed.bUntilAddsDie = false;
	Boss->BeginUntargetable(Timed);
	Boss->TickFight(4.9f);
	TestTrue(TEXT("4.9 s into a 5 s spell: untargetable"), Boss->IsUntargetable());
	Boss->TickFight(0.2f);
	TestFalse(TEXT("5.1 s: over"), Boss->IsUntargetable());
	Boss->BeginUntargetable(Scripted);
	Boss->TickFight(60.f);
	TestTrue(TEXT("A scripted spell lasts"), Boss->IsUntargetable());
	Boss->EndUntargetable();
	TestFalse(TEXT("...until the boss's code ends it"), Boss->IsUntargetable());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBossAddsTest, "Looter.Bosses.Adds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBossAddsTest::RunTest(const FString& Parameters)
{
	// How many rise: what the wave asks for, up to its cap and the boss's (never past 10) less those alive.
	TestEqual(TEXT("Three asked, none alive"), BossRules::AddsToSpawn(3, 0, 0, 10), 3);
	TestEqual(TEXT("Three asked, nine alive: one"), BossRules::AddsToSpawn(3, 9, 0, 10), 1);
	TestEqual(TEXT("Ten alive: none"), BossRules::AddsToSpawn(3, 10, 0, 10), 0);
	TestEqual(TEXT("Two every 25 s, at most four: three alive, one rises"), BossRules::AddsToSpawn(2, 3, 4, 10), 1);
	TestEqual(TEXT("A boss's cap past 10 is 10"), BossRules::AddsToSpawn(15, 0, 0, 25), BossRules::MaxAliveAdds);
	TestEqual(TEXT("A wave's cap past the boss's is the boss's"), BossRules::AddsToSpawn(8, 0, 9, 6), 6);
	const TArray<FVector> Ring = BossRules::RingPoints(FVector(0.0, 0.0, 50.0), 600.f, 4, 0.f);
	TestTrue(TEXT("Spots evenly round the boss, level with it"), Ring.Num() == 4 && Ring[0].Equals(FVector(600.0, 0.0, 50.0), 0.1)
		&& Ring[1].Equals(FVector(0.0, 600.0, 50.0), 0.1) && Ring[2].Equals(FVector(-600.0, 0.0, 50.0), 0.1));

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
	const FBossAddWave Brood = BossTestSpider::Brood();
	TestEqual(TEXT("A wave of three rises"), Boss->SpawnWave(Brood), 3);
	TArray<ACreatureBase*> Risen = Boss->GetAliveAdds();
	if (!TestEqual(TEXT("Three adds alive"), Risen.Num(), 3))
	{
		return false;
	}
	for (const ACreatureBase* Add : Risen)
	{
		TestTrue(TEXT("A spiderling: a Basic brown spider at its wave's size"), Add->IsA<ASpiderCreature>() && Add->GetRank() == ECreatureRank::Basic
			&& FMath::IsNearlyEqual(Add->GetSizeScale(), Brood.BodyScale, 0.001f));
		TestFalse(TEXT("It never comes back once killed"), Add->WillRespawn());
		TestTrue(TEXT("It's set on the player"), Add->GetCreatureState() == ECreatureState::Chase);
		TestTrue(TEXT("It rose round the boss"), FMath::IsNearlyEqual(FVector::Dist2D(Add->GetActorLocation(), Boss->GetOwner()->GetActorLocation()),
			static_cast<double>(Brood.Radius), 5.0));
	}

	// Capped at ten alive, whatever the waves ask.
	FBossAddWave Big = Brood;
	Big.Count = 10;
	TestEqual(TEXT("A wave of ten with three alive: seven rise"), Boss->SpawnWave(Big), 7);
	TestEqual(TEXT("Ten alive"), Boss->NumAliveAdds(), BossRules::MaxAliveAdds);
	TestEqual(TEXT("At ten, none rise"), Boss->SpawnWave(Brood), 0);

	// The dead stay dead: nothing brings them back, and a later wave only tops the living up.
	ACreatureBase* Fallen = Risen[0];
	Kill(Fallen);
	Boss->TickFight(0.1f);
	TestTrue(TEXT("Killed, it's dead"), Fallen->IsDead());
	TestEqual(TEXT("Nine alive"), Boss->NumAliveAdds(), 9);
	TestFalse(TEXT("Not counted among the living"), Boss->GetAliveAdds().Contains(Fallen));
	TestFalse(TEXT("And it won't come back"), Fallen->WillRespawn());
	TestEqual(TEXT("A wave tops the living up to ten"), Boss->SpawnWave(Brood), 1);

	// Gone with the fight.
	TArray<TWeakObjectPtr<ACreatureBase>> Living;
	for (ACreatureBase* Add : Boss->GetAliveAdds())
	{
		Living.Add(Add);
	}
	Boss->ResetFight();
	TestEqual(TEXT("A reset takes the adds"), Boss->NumAliveAdds(), 0);
	TestFalse(TEXT("...every one of them"), Living.ContainsByPredicate([](const TWeakObjectPtr<ACreatureBase>& Add) { return Add.IsValid(); }));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBossResetTest, "Looter.Bosses.Reset",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBossResetTest::RunTest(const FString& Parameters)
{
	// The player's death starts the fight over: the boss heals and goes home, its adds go, its wall drops, its bar goes.
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
	ACreatureBase* Creature = Boss->GetCreature();
	UHealthComponent* Health = Boss->GetBossHealth();
	const FVector Home = Creature->GetHome().GetLocation();
	TestTrue(TEXT("Its bar is up"), Boss->IsBarShown());
	ABossSeal* Wall = Boss->GetActiveSeal();
	TestTrue(TEXT("Its wall closed round the player and the boss"), Wall && Wall->IsRaised() && Wall->IsInside(Player->GetActorLocation(), 0.f));

	// Mid-fight: led off its spot, in its second phase, its spell on and its brood out.
	Creature->SetActorLocation(Home + FVector(300.0, 200.0, 0.0));
	HurtTo(*Boss, 0.5f);
	TArray<TWeakObjectPtr<ACreatureBase>> Brood;
	for (ACreatureBase* Add : Boss->GetAliveAdds())
	{
		Brood.Add(Add);
	}
	TestTrue(TEXT("Mid-fight: second phase, untargetable, three adds"), Boss->GetPhase() == 1 && Boss->IsUntargetable() && Brood.Num() == 3);

	int32 Resets = 0;
	Boss->OnFightReset.AddLambda([&Resets]() { ++Resets; });
	Hurt(Player, 1.0e7f);
	TestTrue(TEXT("The player died"), Player->FindComponentByClass<UHealthComponent>()->IsDead());
	TestEqual(TEXT("Their death started the fight over, once"), Resets, 1);
	TestFalse(TEXT("No fight"), Boss->IsFighting());
	TestEqual(TEXT("The boss healed"), Health->GetHealth(), Health->GetMaxHealth());
	TestTrue(TEXT("...and stands on its spot again"), Creature->GetActorLocation().Equals(Home, 1.0));
	TestFalse(TEXT("...its spell gone"), Boss->IsUntargetable() || Health->bInvulnerable);
	TestTrue(TEXT("...and it waits there, hunting nobody"), Creature->IsPassive() && Creature->GetCreatureState() == ECreatureState::Idle);
	TestTrue(TEXT("...back before its first phase"), Boss->GetPhase() <= 0);
	TestEqual(TEXT("Its adds are gone"), Boss->NumAliveAdds(), 0);
	TestFalse(TEXT("...every one"), Brood.ContainsByPredicate([](const TWeakObjectPtr<ACreatureBase>& Add) { return Add.IsValid(); }));
	TestFalse(TEXT("Its wall dropped"), Wall && Wall->IsRaised());
	if (Wall && Wall->GetWalls().Num() > 0)
	{
		TestTrue(TEXT("...and lets everyone through"), Wall->GetWalls()[0]->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
	}
	TestFalse(TEXT("Its bar is down"), Boss->IsBarShown());
	TestTrue(TEXT("Its own tag is back"), Creature->bShowsHealthTag);

	// The player back on their feet: nothing but the fight's start sets the boss on them (not a pack's call), and the fight
	// can start again from the top.
	Player->FindComponentByClass<UHealthComponent>()->ResetHealth();
	Creature->AlertTo(Player);
	TestTrue(TEXT("Called to the player, it still waits"), Creature->GetCreatureState() == ECreatureState::Idle);
	Boss->StartFight(Player);
	TestTrue(TEXT("Fought again: its first phase, its wall closed"), Boss->IsFighting() && Boss->GetPhase() == 0 && Wall && Wall->IsRaised());
	TestTrue(TEXT("...and it's after the player"), !Creature->IsPassive() && Creature->GetCreatureState() == ECreatureState::Chase);

	// Won: the boss's death ends it for good.
	int32 Wins = 0;
	Boss->OnFightWon.AddLambda([&Wins]() { ++Wins; });
	Hurt(Creature, 1.0e7f);
	TestTrue(TEXT("Its death wins the fight"), Boss->IsWon() && !Boss->IsFighting() && Wins == 1);
	TestFalse(TEXT("A boss never comes back"), Creature->WillRespawn());
	TestFalse(TEXT("Its wall dropped"), Wall && Wall->IsRaised());
	TestTrue(TEXT("Its bar shows it empty a moment"), Boss->IsBarShown());
	Boss->TickFight(UBossComponent::BarHideDelay + 0.1f);
	TestFalse(TEXT("...then goes"), Boss->IsBarShown());
	Boss->StartFight(Player);
	TestFalse(TEXT("A won fight doesn't start again"), Boss->IsFighting());
	return true;
}

#endif

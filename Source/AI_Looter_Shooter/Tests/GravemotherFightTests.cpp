#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

// The Gravemother's boss fight (GravemotherFight, GravemotherCreatureMoves.cpp): her phases and pace, her fight starting as
// she turns on a player with a roar, her brood as her boss's adds by phase, the quake, the spit, her reel when staggered,
// the window a missed charge leaves and her fury's burning crack, and her fight standing down when the player is gone.

#include "Bosses/BossComponent.h"
#include "Combat/EnemyProjectileSubsystem.h"
#include "Combat/HealthComponent.h"
#include "Creatures/GravemotherCreature.h"
#include "Creatures/GravemotherFight.h"
#include "Creatures/GroundCrack.h"
#include "Tests/BossTestWorld.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Tests/AutomationCommon.h"

namespace
{
	constexpr float FightStep = 0.05f;

	/** The Gravemother as her lair spawns her (Legendary), begun, dropping no loot. */
	AGravemotherCreature* SpawnFightingMother(UWorld* World, const FVector& Feet)
	{
		ACreatureBase::FRuntimeSpawn Spawn;
		Spawn.Rank = ECreatureRank::Legendary;
		Spawn.Level = 1;
		AGravemotherCreature* Mother = Cast<AGravemotherCreature>(
			ACreatureBase::SpawnAtRuntime(World, AGravemotherCreature::StaticClass(), Feet, 0.f, Spawn));
		if (Mother && !Mother->HasActorBegunPlay())
		{
			Mother->DispatchBeginPlay();
		}
		BossTestWorld::NoLoot(Mother);
		return Mother;
	}

	/** Moves her on a frame at a time until Done holds (at most Limit seconds); the seconds it took, or negative. */
	float TickUntil(AGravemotherCreature* Mother, float Limit, TFunctionRef<bool()> Done)
	{
		for (float Time = 0.f; Mother && Time <= Limit; Time += FightStep)
		{
			if (Done())
			{
				return Time;
			}
			Mother->Tick(FightStep);
		}
		return -1.f;
	}

	void TickFor(AGravemotherCreature* Mother, float Seconds)
	{
		for (float Time = 0.f; Mother && Time < Seconds - KINDA_SMALL_NUMBER; Time += FightStep)
		{
			Mother->Tick(FightStep);
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGravemotherFightRulesTest, "Looter.Creatures.Gravemother.Fight.Rules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGravemotherFightRulesTest::RunTest(const FString& Parameters)
{
	// Three phases at her brood's calls, named on her bar.
	const TArray<FBossPhase> Phases = GravemotherFight::MakePhases();
	if (!TestEqual(TEXT("Three phases"), Phases.Num(), 3))
	{
		return false;
	}
	TestTrue(TEXT("\"Fresh Meat\" from full, \"The Brood Wakes\" at 66%, \"Mother's Fury\" at 33%"),
		Phases[0].Name.ToString() == TEXT("Fresh Meat") && Phases[1].Name.ToString() == TEXT("The Brood Wakes")
		&& Phases[2].Name.ToString() == TEXT("Mother's Fury") && Phases[0].HealthShare == 1.f
		&& FMath::IsNearlyEqual(Phases[1].HealthShare, Gravemother::BroodCallShare(0)) && FMath::IsNearlyEqual(Phases[2].HealthShare, Gravemother::BroodCallShare(1)));

	// Her pace: the quake from the second, everything quicker and the cracks burning in the third.
	const GravemotherFight::FPace First = GravemotherFight::PaceFor(0);
	const GravemotherFight::FPace Second = GravemotherFight::PaceFor(1);
	const GravemotherFight::FPace Third = GravemotherFight::PaceFor(2);
	TestTrue(TEXT("First: bite, charge and venom; no quake, no burning"), !First.bQuake && !First.bBurningCracks && First.SpitPellets >= 3);
	TestTrue(TEXT("Second: the quake, a charge sooner"), Second.bQuake && Second.ChargeCooldownScale < First.ChargeCooldownScale);
	TestTrue(TEXT("Third: quicker in everything, a wider spit, her cracks burning"), Third.bQuake && Third.bBurningCracks
		&& Third.ChaseScale > 1.f && Third.ChargeCooldownScale < Second.ChargeCooldownScale && Third.SpitPellets > First.SpitPellets
		&& Third.SpitCooldown < First.SpitCooldown);
	const AGravemotherCreature* Defaults = GetDefault<AGravemotherCreature>();
	TestTrue(TEXT("...her telegraph still over a second at its quickest"), Defaults->ChargeTelegraph * Third.ChargeTelegraphScale >= 1.2f);
	TestTrue(TEXT("Her venom: slow pellets from her fangs"), GravemotherFight::Spit(5, 16.f).Speed < 1300.f && GravemotherFight::Spit(5, 16.f).Pellets == 5);

	// Her boss: she lives as a spider until she turns on someone; her brood die with her; her loot showers; the lair, not
	// her boss, keeps her death.
	const UBossComponent* Boss = Defaults->GetBoss();
	if (!TestNotNull(TEXT("Her boss"), Boss))
	{
		return false;
	}
	TestTrue(TEXT("She waits for nobody: her fight starts as she turns on a player (a hit turns her, from her own ground)"),
		!Boss->bWaitsPassive && Boss->bStartWhenHunting && !Boss->bStartWhenHurt);
	TestTrue(TEXT("...stands down a while after the player leaves her ground, with no wall"), Boss->StandDownSeconds > 0.f
		&& Boss->SealRadius == 0.f && !Boss->Seal && Boss->LeashRadius == 0.f);
	TestTrue(TEXT("...her brood die with her; her lair, not her boss, records her death"), Boss->bAddsDieWithBoss && Boss->BossId.IsNone());
	TestEqual(TEXT("...titled under her name"), Boss->Show.Title.ToString(), FString(TEXT("Brood of the Sink")));
	TestTrue(TEXT("...her weak spots stagger her, her loot showers with ammo to spare"), Boss->Stagger.CritShare > 0.f && Boss->LootShower.bEnabled
		&& Boss->LootShower.BonusAmmo > 0);
	TestEqual(TEXT("...her phases"), Boss->Phases.Num(), 3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGravemotherFightPhasesTest, "Looter.Creatures.Gravemother.Fight.Phases",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGravemotherFightPhasesTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	ACharacter* Player = BossTestWorld::SpawnPlayer(World, FVector(1000.0, 0.0, 0.0));
	AGravemotherCreature* Mother = SpawnFightingMother(World, FVector::ZeroVector);
	UBossComponent* Boss = Mother ? Mother->GetBoss() : nullptr;
	if (!TestNotNull(TEXT("Player stand-in"), Player) || !TestNotNull(TEXT("The Gravemother and her boss"), Boss))
	{
		return false;
	}
	TestFalse(TEXT("Before her fight she isn't held back: she lives in her den as a spider"), Mother->IsPassive());
	TestFalse(TEXT("...and nothing has started"), Boss->IsFighting());

	// She turns on the player: her fight starts, and she rears and screams.
	Mother->AlertTo(Player);
	Boss->TickFight(FightStep);
	TestTrue(TEXT("Turned on the player: her fight is on, in its first phase"), Boss->IsFighting() && Boss->GetPhase() == 0);
	TestTrue(TEXT("...her roar is next"), Mother->IsRoarWanted());
	Mother->Tick(FightStep);
	TestTrue(TEXT("She rears and screams (from wherever the player is)"), Mother->GetCreatureState() == ECreatureState::Attack
		&& Mother->GetMove() == EGravemotherMove::Roar && FMath::IsNearlyEqual(Mother->AttackWindup, Mother->RoarWindup));
	TickFor(Mother, Mother->RoarWindup + Mother->RoarRecovery + 0.1f);
	TestTrue(TEXT("...then she's up again"), Mother->GetCreatureState() != ECreatureState::Attack && !Mother->IsRoarWanted());

	// Her brood by phase: four at two thirds and four more at a third, her boss's adds; a roar with each.
	BossTestWorld::HurtTo(*Boss, Gravemother::BroodCallShare(0));
	TestEqual(TEXT("At 66%: \"The Brood Wakes\""), Boss->GetPhase(), 1);
	TestEqual(TEXT("...four spiderlings, her boss's adds"), Boss->NumAliveAdds(), Gravemother::BroodSize);
	TestTrue(TEXT("...she screams again, and has the quake now"), Mother->IsRoarWanted() && Mother->GetPace().bQuake && Mother->GetPacePhase() == 1);
	const float ChaseBefore = Mother->ChaseSpeed;
	BossTestWorld::HurtTo(*Boss, Gravemother::BroodCallShare(1));
	TestEqual(TEXT("At 33%: \"Mother's Fury\""), Boss->GetPhase(), 2);
	TestEqual(TEXT("...four more"), Boss->NumAliveAdds(), 2 * Gravemother::BroodSize);
	TestTrue(TEXT("...quicker, her cracks burning"), Mother->GetPace().bBurningCracks && Mother->ChaseSpeed > ChaseBefore);

	// Her death wins it; her brood go with her.
	BossTestWorld::Kill(Mother);
	TestTrue(TEXT("Killed: her fight is won"), Boss->IsWon() && !Boss->IsFighting());
	TestEqual(TEXT("...her brood no longer her adds"), Boss->NumAliveAdds(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGravemotherFightMovesTest, "Looter.Creatures.Gravemother.Fight.Moves",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGravemotherFightMovesTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	UEnemyProjectileSubsystem* Shots = World->GetSubsystem<UEnemyProjectileSubsystem>();

	// The Gravequake (from her second phase): at a player hugging her she rears while a ring of cracks spreads out to its
	// reach, then slams; the ring closes and it cools down.
	ACharacter* Hugger = BossTestWorld::SpawnPlayer(World, FVector(300.0, 0.0, 0.0));
	AGravemotherCreature* Quaker = SpawnFightingMother(World, FVector::ZeroVector);
	if (!TestNotNull(TEXT("Player stand-in"), Hugger) || !TestNotNull(TEXT("The Gravemother"), Quaker) || !TestNotNull(TEXT("Pellets"), Shots))
	{
		return false;
	}
	Quaker->ApplyPace(1);
	Quaker->ReadyQuake();
	Quaker->AlertTo(Hugger);
	Quaker->Tick(FightStep);
	TestTrue(TEXT("3 m off in her second phase: the quake"), Quaker->GetCreatureState() == ECreatureState::Attack
		&& Quaker->GetMove() == EGravemotherMove::Quake && FMath::IsNearlyEqual(Quaker->AttackWindup, Quaker->QuakeWindup));
	AGroundCrack* Ring = Quaker->GetQuakeCrack();
	TestTrue(TEXT("...a ring of cracks spreading under her"), Ring && Ring->NumBurstCracks() >= 6);
	TestTrue(TEXT("...the player inside its reach"), Quaker->IsInQuake(Hugger->GetActorLocation()));
	TestFalse(TEXT("...one 8 m off isn't"), Quaker->IsInQuake(FVector(800.0, 0.0, 0.0)));
	TickFor(Quaker, Quaker->QuakeWindup + 0.05f);
	TestTrue(TEXT("At its wind-up's end the slam: the ring closes, the quake cools down"), Ring && Ring->IsClosing() && !Quaker->IsQuakeReady()
		&& Quaker->GetQuakeCrack() == nullptr);

	// Her venom (in her fight): at a player out past her charge's reach, after her roar, she rears with the glow at her
	// fangs and spits a cone of pellets.
	ACharacter* Keeper = BossTestWorld::SpawnPlayer(World, FVector(1800.0, 8000.0, 0.0));
	AGravemotherCreature* Spitter = SpawnFightingMother(World, FVector(0.0, 8000.0, 0.0));
	if (!TestNotNull(TEXT("Second stand-in"), Keeper) || !TestNotNull(TEXT("Second Gravemother"), Spitter))
	{
		return false;
	}
	Spitter->GetBoss()->StartFight(Keeper);
	const float SpitAt = TickUntil(Spitter, 5.f, [Spitter]()
	{
		return Spitter->GetMove() == EGravemotherMove::Spit && Spitter->GetCreatureState() == ECreatureState::Attack;
	});
	TestTrue(FString::Printf(TEXT("18 m off: after her roar, she rears to spit (%.2f s)"), SpitAt), SpitAt >= 0.f);
	TestEqual(TEXT("...nothing flies during the tell"), Shots->NumShotsFrom(Spitter), 0);
	TickFor(Spitter, Spitter->SpitWindup + 0.05f);
	TestEqual(TEXT("...then her venom flies"), Shots->NumShotsFrom(Spitter), Spitter->GetPace().SpitPellets);
	TestFalse(TEXT("...and cools down"), Spitter->IsSpitReady());

	// Staggered she reels: whatever she was at is broken off (here the spit's recovery), she's held still, and she's up again
	// once it's over.
	UBossComponent* Boss = Spitter->GetBoss();
	TestTrue(TEXT("Still in her spit's recovery"), Spitter->GetCreatureState() == ECreatureState::Attack);
	TestTrue(TEXT("Staggered"), Boss->BeginStagger() && Spitter->IsReeling());
	TestNearlyEqual(TEXT("...held still"), Spitter->ChaseSpeed, 0.f, 0.001f);
	TickFor(Spitter, 0.2f);
	TestTrue(TEXT("...her attack broken off"), Spitter->GetCreatureState() != ECreatureState::Attack);
	Boss->TickFight(Boss->Stagger.Seconds + 0.1f);
	TestTrue(TEXT("Over: up again, on the move"), !Spitter->IsReeling() && Spitter->ChaseSpeed > 0.f);

	// A charge that runs nobody down leaves her forelegs in the dirt longer (the window), and in her fury its crack burns.
	ACharacter* Dodger = BossTestWorld::SpawnPlayer(World, FVector(1000.0, -8000.0, 0.0));
	AGravemotherCreature* Charger = SpawnFightingMother(World, FVector(0.0, -8000.0, 0.0));
	if (!TestNotNull(TEXT("Third stand-in"), Dodger) || !TestNotNull(TEXT("Third Gravemother"), Charger))
	{
		return false;
	}
	Charger->ApplyPace(2);
	Charger->AlertTo(Dodger);
	Charger->ReadyCharge();
	const float DashAt = TickUntil(Charger, 3.f, [Charger]() { return Charger->GetChargeState() == EGravemotherCharge::Dash; });
	if (!TestTrue(TEXT("Her fury's charge: the dash"), DashAt >= 0.f))
	{
		return false;
	}
	// The player sidesteps well off her line; she's carried down it as her movement would carry her.
	const FVector Start = Charger->GetChargeStart();
	const FVector Line = Charger->GetChargeDirection();
	Dodger->SetActorLocation(Start + FVector(-Line.Y, Line.X, 0.0) * 1500.0);
	const float Speed = Charger->ChargeSpeed * Charger->GetSizeScale();
	float Covered = 0.f;
	while (Covered < Charger->GetChargeLength() + Speed * FightStep && Charger->GetChargeState() == EGravemotherCharge::Dash)
	{
		Covered = FMath::Min(Covered + Speed * FightStep, Charger->GetChargeLength());
		Charger->SetActorLocation(Start + Line * Covered);
		Charger->Tick(FightStep);
	}
	TestTrue(TEXT("Dodged: she slams down, nobody run down"), Charger->GetChargeState() == EGravemotherCharge::Recover && !Charger->HasChargeHit());
	TickFor(Charger, Charger->ChargeRecover + 0.3f);
	TestTrue(TEXT("...still down past a hit's recovery: the window to punish"), Charger->GetChargeState() == EGravemotherCharge::Recover);
	const float UpAt = TickUntil(Charger, Charger->ChargeMissRecover + 0.5f, [Charger]() { return Charger->GetChargeState() == EGravemotherCharge::None; });
	TestTrue(TEXT("...then up"), UpAt >= 0.f);
	TestNotNull(TEXT("In her fury the crack she ran down burns"), Charger->GetBurningCrack());
	TickFor(Charger, Charger->BurnSeconds + 0.2f);
	TestNull(TEXT("...for a while"), Charger->GetBurningCrack());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGravemotherStandDownTest, "Looter.Creatures.Gravemother.Fight.StandDown",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGravemotherStandDownTest::RunTest(const FString& Parameters)
{
	// The player gone off her ground a while: her fight stands down where it is, with no healing and no trip home.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	ACharacter* Player = BossTestWorld::SpawnPlayer(World, FVector(1000.0, 0.0, 0.0));
	AGravemotherCreature* Mother = SpawnFightingMother(World, FVector::ZeroVector);
	UBossComponent* Boss = Mother ? Mother->GetBoss() : nullptr;
	if (!TestNotNull(TEXT("Player stand-in"), Player) || !TestNotNull(TEXT("Her boss"), Boss))
	{
		return false;
	}
	Boss->StartFight(Player);
	BossTestWorld::HurtTo(*Boss, 0.8f);
	const float Wounded = Boss->GetBossHealth()->GetHealth();
	Player->SetActorLocation(FVector(20000.0, 0.0, 0.0));
	// (Her roar plays out first: a creature mid-attack keeps its target until the attack is over.)
	TickFor(Mother, Mother->RoarWindup + Mother->RoarRecovery + 0.6f);
	TestNull(TEXT("The player gone far off: she lets them go"), Mother->GetTarget());
	Boss->TickFight(Boss->StandDownSeconds * 0.5f);
	TestTrue(TEXT("A moment later her fight is still on"), Boss->IsFighting());
	Boss->TickFight(Boss->StandDownSeconds * 0.5f + 0.2f);
	TestTrue(TEXT("A while later it stands down, not won"), !Boss->IsFighting() && !Boss->IsWon());
	TestNearlyEqual(TEXT("...her wounds kept"), Boss->GetBossHealth()->GetHealth(), Wounded, 0.5f);
	TestFalse(TEXT("...and she's free (not held back), as she was"), Mother->IsPassive());
	return true;
}

#endif

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

// The Gravemother's boss fight (GravemotherFight, GravemotherCreatureMoves.cpp): her phases and pace (the user's +25% in her
// last third, her tells never shortened), her fight starting as she turns on a player with a roar, her brood as her boss's
// adds by phase (through her roar's mounds), the quake, her reel when staggered (a clod in her grip dropped), the window a
// missed plough leaves, and her fight standing down when the player is gone.

#include "Bosses/BossComponent.h"
#include "Combat/HealthComponent.h"
#include "Creatures/GravemotherClod.h"
#include "Creatures/GravemotherCreature.h"
#include "Creatures/GravemotherFight.h"
#include "Creatures/GroundCrack.h"
#include "Tests/BossTestWorld.h"
#include "Tests/GravemotherTestWorld.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Tests/AutomationCommon.h"

using namespace GravemotherTestWorld;

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

	// Her pace: the quake from the second; every move sooner in the third, and a quarter faster on her feet (the user's +25%)
	// only then.
	const GravemotherFight::FPace First = GravemotherFight::PaceFor(0);
	const GravemotherFight::FPace Second = GravemotherFight::PaceFor(1);
	const GravemotherFight::FPace Third = GravemotherFight::PaceFor(2);
	TestTrue(TEXT("First: bite, furrow and clod; no quake, her own speed"), !First.bQuake && First.SpeedScale == 1.f);
	TestTrue(TEXT("Second: the quake, a furrow and a clod sooner, her own speed"), Second.bQuake && Second.FurrowCooldownScale < First.FurrowCooldownScale
		&& Second.ClodCooldown < First.ClodCooldown && Second.SpeedScale == 1.f);
	TestTrue(TEXT("Third: everything sooner, a quarter faster"), Third.bQuake && Third.FurrowCooldownScale < Second.FurrowCooldownScale
		&& Third.ClodCooldown < Second.ClodCooldown && FMath::IsNearlyEqual(Third.SpeedScale, 1.25f));
	const AGravemotherCreature* Defaults = GetDefault<AGravemotherCreature>();
	TestTrue(TEXT("Her tells are the user's, at every pace: the furrow's 2 s, the clod's 1.2 s"),
		FMath::IsNearlyEqual(Defaults->Furrows.Tell, 2.f) && FMath::IsNearlyEqual(Defaults->Clods.Tell, 1.2f));

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
	AGravemotherCreature* Mother = SpawnMother(World, FVector::ZeroVector);
	UBossComponent* Boss = Mother ? Mother->GetBoss() : nullptr;
	if (!TestNotNull(TEXT("Player stand-in"), Player) || !TestNotNull(TEXT("The Gravemother and her boss"), Boss))
	{
		return false;
	}
	TestFalse(TEXT("Before her fight she isn't held back: she lives in her den as a spider"), Mother->IsPassive());
	TestFalse(TEXT("...and nothing has started"), Boss->IsFighting());

	// She turns on the player: her fight starts, and she rears and screams.
	Mother->AlertTo(Player);
	Boss->TickFight(Step);
	TestTrue(TEXT("Turned on the player: her fight is on, in its first phase"), Boss->IsFighting() && Boss->GetPhase() == 0);
	TestTrue(TEXT("...her roar is next"), Mother->IsRoarWanted());
	Mother->Tick(Step);
	TestTrue(TEXT("She rears and screams (from wherever the player is)"), Mother->GetCreatureState() == ECreatureState::Attack
		&& Mother->GetMove() == EGravemotherMove::Roar && FMath::IsNearlyEqual(Mother->AttackWindup, Mother->RoarWindup));
	TickFor(Mother, Mother->RoarWindup + Mother->RoarRecovery + 0.1f);
	TestTrue(TEXT("...then she's up again"), Mother->GetCreatureState() != ECreatureState::Attack && !Mother->IsRoarWanted());

	// Her brood by phase: at two thirds she screams and slams the floor, four mounds bulge, four spiderlings dig out: her
	// boss's adds.
	BossTestWorld::HurtTo(*Boss, Gravemother::BroodCallShare(0));
	TestEqual(TEXT("At 66%: \"The Brood Wakes\""), Boss->GetPhase(), 1);
	TestTrue(TEXT("...she screams again, and has the quake now"), Mother->IsRoarWanted() && Mother->GetPace().bQuake && Mother->GetPacePhase() == 1);
	const float SlamAt = TickUntil(Mother, 4.f, [Mother]() { return Mother->GetMounds().Num() > 0; });
	TestTrue(*FString::Printf(TEXT("...her roar's slam raises the mounds (%.2f s)"), SlamAt), SlamAt >= 0.f
		&& Mother->GetMounds().Num() == Gravemother::BroodSize);
	AdvanceMounds(*Mother, Mother->Unearth.BulgeSeconds + Step * 2.f);
	TestEqual(TEXT("...four spiderlings, her boss's adds"), Boss->NumAliveAdds(), Gravemother::BroodSize);
	const float ChaseBefore = Mother->ChaseSpeed;
	TestTrue(TEXT("Her stone still on"), Mother->IsStoneOn());

	// Her last third: her stone cracks off by itself, four more, and she runs a quarter faster.
	BossTestWorld::HurtTo(*Boss, Gravemother::BroodCallShare(1));
	TestEqual(TEXT("At 33%: \"Mother's Fury\""), Boss->GetPhase(), 2);
	TestFalse(TEXT("...her stone cracks off by itself"), Mother->IsStoneOn());
	TestNearlyEqual(TEXT("...a quarter faster on her feet"), Mother->ChaseSpeed, ChaseBefore * 1.25f, 0.5f);
	TickUntil(Mother, 4.f, [Mother]() { return Mother->GetMounds().Num() > Gravemother::BroodSize; });
	AdvanceMounds(*Mother, Mother->Unearth.BulgeSeconds + Step * 2.f);
	TestEqual(TEXT("...four more"), Boss->NumAliveAdds(), 2 * Gravemother::BroodSize);

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

	// The Gravequake (from her second phase): at a player hugging her she rears while a ring of cracks spreads out to its
	// reach, then slams; the ring closes and it cools down.
	ACharacter* Hugger = BossTestWorld::SpawnPlayer(World, FVector(300.0, 0.0, 0.0));
	AGravemotherCreature* Quaker = SpawnMother(World, FVector::ZeroVector);
	if (!TestNotNull(TEXT("Player stand-in"), Hugger) || !TestNotNull(TEXT("The Gravemother"), Quaker))
	{
		return false;
	}
	Quaker->ApplyPace(1);
	Quaker->ReadyQuake();
	Quaker->AlertTo(Hugger);
	Quaker->Tick(Step);
	TestTrue(TEXT("3 m off in her second phase: the quake"), Quaker->GetCreatureState() == ECreatureState::Attack
		&& Quaker->GetMove() == EGravemotherMove::Quake && FMath::IsNearlyEqual(Quaker->AttackWindup, Quaker->QuakeWindup));
	AGroundCrack* Ring = Quaker->GetQuakeCrack();
	TestTrue(TEXT("...a ring of cracks spreading under her"), Ring && Ring->NumBurstCracks() >= 6);
	TestTrue(TEXT("...the player inside its reach"), Quaker->IsInQuake(Hugger->GetActorLocation()));
	TestFalse(TEXT("...one 8 m off isn't"), Quaker->IsInQuake(FVector(800.0, 0.0, 0.0)));
	TickFor(Quaker, Quaker->QuakeWindup + 0.05f);
	TestTrue(TEXT("At its wind-up's end the slam: the ring closes, the quake cools down"), Ring && Ring->IsClosing() && !Quaker->IsQuakeReady()
		&& Quaker->GetQuakeCrack() == nullptr);

	// Staggered she reels: whatever she was at is broken off (here her clod's tell: it falls from her grip), she's held
	// still, and she's up again once it's over.
	SpawnFloor(World, FVector(900.0, 8000.0, 0.0), 2500.0);
	ACharacter* Keeper = BossTestWorld::SpawnPlayer(World, FVector(1800.0, 8000.0, 90.0));
	AGravemotherCreature* Lobber = SpawnMother(World, FVector(0.0, 8000.0, 0.0));
	if (!TestNotNull(TEXT("Second stand-in"), Keeper) || !TestNotNull(TEXT("Second Gravemother"), Lobber))
	{
		return false;
	}
	UBossComponent* Boss = Lobber->GetBoss();
	Boss->StartFight(Keeper);
	Lobber->AlertTo(Keeper);
	const float TellAt = TickUntil(Lobber, 5.f, [Lobber]() { return Lobber->GetHeldClod() != nullptr; });
	TestTrue(FString::Printf(TEXT("18 m off: after her roar, a clod in her grip (%.2f s)"), TellAt), TellAt >= 0.f);
	AGravemotherClod* Held = Lobber->GetHeldClod();
	TestTrue(TEXT("Staggered"), Boss->BeginStagger() && Lobber->IsReeling());
	TestNearlyEqual(TEXT("...held still"), Lobber->ChaseSpeed, 0.f, 0.001f);
	TickFor(Lobber, 0.2f);
	TestTrue(TEXT("...her attack broken off"), Lobber->GetCreatureState() != ECreatureState::Attack);
	TestTrue(TEXT("...her clod fallen from her grip"), Lobber->GetHeldClod() == nullptr && (!Held || Held->WasDropped()));
	Boss->TickFight(Boss->Stagger.Seconds + 0.1f);
	TestTrue(TEXT("Over: up again, on the move"), !Lobber->IsReeling() && Lobber->ChaseSpeed > 0.f);

	// A plough that runs nobody down leaves her forelegs in the dirt longer (the window).
	ACharacter* Dodger = BossTestWorld::SpawnPlayer(World, FVector(1000.0, -8000.0, 0.0));
	AGravemotherCreature* Plougher = SpawnMother(World, FVector(0.0, -8000.0, 0.0));
	if (!TestNotNull(TEXT("Third stand-in"), Dodger) || !TestNotNull(TEXT("Third Gravemother"), Plougher))
	{
		return false;
	}
	Plougher->ApplyPace(2);
	Plougher->AlertTo(Dodger);
	Plougher->ReadyFurrow();
	const float PloughAt = TickUntil(Plougher, 3.f, [Plougher]() { return Plougher->GetFurrowState() == EGravemotherFurrow::Plough; });
	if (!TestTrue(TEXT("Her fury's furrow: the plough, after the full 2 s tell"), PloughAt >= Plougher->Furrows.Tell - Step))
	{
		return false;
	}
	TestNearlyEqual(TEXT("...a quarter faster in her fury"), Plougher->GetPloughSpeed(), Plougher->Furrows.Speed * Plougher->GetSizeScale() * 1.25f, 1.f);
	// The player sidesteps well off her line; she's carried down it as her movement would carry her.
	const FVector Start = Plougher->GetFurrowStart();
	const FVector Line = Plougher->GetFurrowDirection();
	Dodger->SetActorLocation(Start + FVector(-Line.Y, Line.X, 0.0) * 1500.0);
	const float Speed = Plougher->GetPloughSpeed();
	float Covered = 0.f;
	while (Covered < Plougher->GetFurrowLength() + Speed * Step && Plougher->GetFurrowState() == EGravemotherFurrow::Plough)
	{
		Covered = FMath::Min(Covered + Speed * Step, Plougher->GetFurrowLength());
		Plougher->SetActorLocation(Start + Line * Covered);
		Plougher->Tick(Step);
	}
	TestTrue(TEXT("Dodged: she slams down, nobody run down"), Plougher->GetFurrowState() == EGravemotherFurrow::Recover && !Plougher->HasFurrowHit());
	TickFor(Plougher, Plougher->Furrows.Recover + 0.3f);
	TestTrue(TEXT("...still down past a hit's recovery: the window to punish"), Plougher->GetFurrowState() == EGravemotherFurrow::Recover);
	const float UpAt = TickUntil(Plougher, Plougher->Furrows.MissRecover + 0.5f, [Plougher]() { return Plougher->GetFurrowState() == EGravemotherFurrow::None; });
	TestTrue(TEXT("...then up"), UpAt >= 0.f);
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
	AGravemotherCreature* Mother = SpawnMother(World, FVector::ZeroVector);
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

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Combat/HealthComponent.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/CreatureRallied.h"
#include "Creatures/CreatureTraitComponent.h"
#include "Creatures/CreatureTraitRules.h"
#include "Creatures/CreatureTraitSettings.h"
#include "Tests/BossTestWorld.h"
#include "Tests/CreatureTraitTestWorld.h"
#include "Tests/EncounterTestWorld.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Script.h"

// The creature traits that reach past the creature's own body (UCreatureTraitComponent): Rallying (the pack it holds),
// Grave-burst (the player it catches), Phasing (where it blinks to, on a floor), and a new life starting every trait
// afresh. The rules and the traits on its own body: CreatureTraitTests.cpp.

using namespace CreatureTraitTestWorld;

// ---------------------------------------------------------------------------
// Rallying
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCreatureTraitRallyTest, "Looter.Creatures.Traits.Rally",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCreatureTraitRallyTest::RunTest(const FString& Parameters)
{
	FEditorScriptExecutionGuard RunEvents;
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	const FRallyingTraitTuning& Tuning = Settings().Rallying;
	ACreatureBase* Rallier = SpawnSpider(World, FVector::ZeroVector);
	ACreatureBase* Near = SpawnSpider(World, FVector(500.0, 0.0, 0.0));
	ACreatureBase* Far = SpawnSpider(World, FVector(Tuning.Radius + 1200.0, 0.0, 0.0));
	ACreatureBase* Second = SpawnSpider(World, FVector(300.0, 300.0, 0.0));
	UHealthComponent* NearHealth = HealthOf(Near);
	if (!TestNotNull(TEXT("A rallier"), Rallier) || !TestNotNull(TEXT("One near it"), NearHealth) || !TestNotNull(TEXT("One far off"), Far)
		|| !TestNotNull(TEXT("A second rallier"), Second))
	{
		return false;
	}
	const float NearChase = Near->ChaseSpeed;
	UCreatureTraitComponent* Traits = Give(*Rallier, { ECreatureTrait::Rallying });
	Traits->Advance(Frame);
	const UCreatureRallied* Rallied = UCreatureRallied::Find(*Near);
	TestNotNull(TEXT("The one 5 m off is rallied"), Rallied);
	TestEqual(TEXT("...10% faster"), Near->ChaseSpeed, NearChase * Tuning.SpeedScale, 0.01f);
	TestEqual(TEXT("...15% less of each hit"), NearHealth->GetDamageTakenScale(), Tuning.DamageTakenScale, 0.0001f);
	TestNull(TEXT("The one far off isn't"), UCreatureRallied::Find(*Far));
	TestNull(TEXT("It doesn't rally itself"), UCreatureRallied::Find(*Rallier));
	const float Before = NearHealth->GetHealth();
	Hurt(Near, 100.f);
	TestEqual(TEXT("A hit for 100 takes 85"), Before - NearHealth->GetHealth(), 100.f * Tuning.DamageTakenScale, 0.01f);

	// A second rallier near it: it counts once.
	UCreatureTraitComponent* SecondTraits = Give(*Second, { ECreatureTrait::Rallying });
	SecondTraits->Advance(Frame);
	Rallied = UCreatureRallied::Find(*Near);
	TestTrue(TEXT("Two ralliers hold it"), Rallied && Rallied->NumRalliers() == 2);
	TestEqual(TEXT("...still only 10% faster"), Near->ChaseSpeed, NearChase * Tuning.SpeedScale, 0.01f);

	// The first dies: the second still holds it; the second's taken away: it's let go, its pace and its hits as before.
	BossTestWorld::Kill(Rallier);
	Rallied = UCreatureRallied::Find(*Near);
	TestTrue(TEXT("Its rallier dead, the other still holds it"), Rallied && Rallied->NumRalliers() == 1);
	SecondTraits->SetTraits(TArray<ECreatureTrait>());
	TestNull(TEXT("No rallier left: let go"), UCreatureRallied::Find(*Near));
	TestEqual(TEXT("...its own pace"), Near->ChaseSpeed, NearChase, 0.01f);
	TestEqual(TEXT("...its hits whole again"), NearHealth->GetDamageTakenScale(), 1.f, 0.0001f);
	return true;
}

// ---------------------------------------------------------------------------
// Grave-burst
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCreatureTraitGraveBurstTest, "Looter.Creatures.Traits.GraveBurst",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCreatureTraitGraveBurstTest::RunTest(const FString& Parameters)
{
	const FGraveBurstTraitTuning& Tuning = Settings().GraveBurst;
	TestEqual(TEXT("Full at its heart"), CreatureTraitRules::BurstShare(0.f, Tuning), 1.f);
	TestEqual(TEXT("Full within its full radius"), CreatureTraitRules::BurstShare(Tuning.FullRadius, Tuning), 1.f);
	TestEqual(TEXT("Its edge share at the edge"), CreatureTraitRules::BurstShare(Tuning.Radius, Tuning), Tuning.EdgeShare, 0.0001f);
	TestEqual(TEXT("Nothing past it"), CreatureTraitRules::BurstShare(Tuning.Radius + 1.f, Tuning), 0.f);

	FEditorScriptExecutionGuard RunEvents;
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	SpawnFloor(World);
	ACreatureBase* Spider = SpawnSpider(World, FVector::ZeroVector);
	ACreatureBase* Away = SpawnSpider(World, FVector(0.0, 3000.0, 0.0));
	ACharacter* Player = EncounterTestWorld::SpawnPlayer(World, FVector(200.0, 0.0, 90.0));
	if (!TestNotNull(TEXT("A spider"), Spider) || !TestNotNull(TEXT("One far off"), Away) || !TestNotNull(TEXT("A player 2 m from it"), Player))
	{
		return false;
	}
	UCreatureTraitComponent* Traits = Give(*Spider, { ECreatureTrait::GraveBurst });
	const float Bite = Spider->AttackDamage;
	const FVector BaseScale = Spider->GetMesh()->GetRelativeScale3D();
	TestFalse(TEXT("Alive, no swell"), Traits->IsSwelling());
	BossTestWorld::Kill(Spider);
	TestTrue(TEXT("Dead, it swells"), Traits->IsSwelling());
	Traits->Advance(Tuning.SwellSeconds * 0.5f);
	TestTrue(TEXT("Half its swell: not yet burst"), Traits->IsSwelling() && !Traits->HasBurst());
	TestTrue(TEXT("...its body bigger"), Spider->GetMesh()->GetRelativeScale3D().X > BaseScale.X * 1.05);
	Traits->Advance(Tuning.SwellSeconds * 0.6f);
	TestTrue(TEXT("Its swell out: it bursts"), Traits->HasBurst() && !Traits->IsSwelling());
	TestTrue(TEXT("...its body gone in the blast"), Spider->IsHidden());
	TestTrue(TEXT("...its size put back for its next life"), Spider->GetMesh()->GetRelativeScale3D().Equals(BaseScale, 0.001));
	const TArray<FCreatureBurstVictim>& Caught = Traits->GetLastBurst();
	if (TestEqual(TEXT("The player 2 m off is caught"), Caught.Num(), 1))
	{
		TestTrue(TEXT("...the player"), Caught[0].Actor.Get() == Player);
		TestTrue(TEXT("...within its reach"), Caught[0].Distance <= Tuning.Radius);
		TestEqual(TEXT("...for its share of 1.5 bites at that distance"), Caught[0].Damage,
			Bite * Tuning.DamageShare * CreatureTraitRules::BurstShare(Caught[0].Distance, Tuning), 0.01f);
	}

	// One 30 m away bursts with nobody near.
	UCreatureTraitComponent* AwayTraits = Give(*Away, { ECreatureTrait::GraveBurst });
	BossTestWorld::Kill(Away);
	AwayTraits->Advance(Tuning.SwellSeconds + Frame);
	TestTrue(TEXT("The far one bursts"), AwayTraits->HasBurst());
	TestEqual(TEXT("...catching nobody"), AwayTraits->GetLastBurst().Num(), 0);
	return true;
}

// ---------------------------------------------------------------------------
// Phasing
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCreatureTraitPhasingTest, "Looter.Creatures.Traits.Phasing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCreatureTraitPhasingTest::RunTest(const FString& Parameters)
{
	const FPhasingTraitTuning& Tuning = Settings().Phasing;
	// Where a blink may land: aside first (square to the hitter's line), the longest first, both sides, all 4-6 m off.
	const FVector Away(1.0, 0.0, 0.0);
	const TArray<FVector> Spots = CreatureTraitRules::BlinkCandidates(FVector::ZeroVector, Away, true, Tuning);
	TestEqual(TEXT("Three angles, three distances, two sides"), Spots.Num(), 18);
	if (Spots.Num() > 0)
	{
		TestEqual(TEXT("The first is square aside"), FVector::DotProduct(Spots[0].GetSafeNormal(), Away), 0.0, 0.001);
		TestEqual(TEXT("...at the longest"), Spots[0].Size2D(), static_cast<double>(Tuning.MaxDistance), 0.1);
	}
	bool bInRange = true;
	for (const FVector& Spot : Spots)
	{
		bInRange &= Spot.Size2D() >= Tuning.MinDistance - 0.1 && Spot.Size2D() <= Tuning.MaxDistance + 0.1;
	}
	TestTrue(TEXT("Every spot 4-6 m off"), bInRange);
	TestFalse(TEXT("A tenth of its health in a second is no hard hit under 8%..."), CreatureTraitRules::IsHardHit(7.f, 100.f, 0.08f));
	TestTrue(TEXT("...8% is"), CreatureTraitRules::IsHardHit(8.f, 100.f, 0.08f));

	FEditorScriptExecutionGuard RunEvents;
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	SpawnFloor(World);
	ACreatureBase* Spider = SpawnSpider(World, FVector::ZeroVector);
	ACharacter* Player = EncounterTestWorld::SpawnPlayer(World, FVector(1200.0, 0.0, 90.0));
	UHealthComponent* Health = HealthOf(Spider);
	if (!TestNotNull(TEXT("A spider with health"), Health) || !TestNotNull(TEXT("A player 12 m off"), Player))
	{
		return false;
	}
	UCreatureTraitComponent* Traits = Give(*Spider, { ECreatureTrait::Phasing });
	const float Max = Health->GetMaxHealth();
	const FVector Start = Spider->GetActorLocation();
	Hurt(Spider, Max * 0.05f);
	Traits->Advance(Frame);
	TestFalse(TEXT("5% of its health: it holds its ground"), Traits->IsBlinking());
	Hurt(Spider, Max * 0.05f);
	Traits->Advance(Frame);
	TestTrue(TEXT("10% within a second: it blinks"), Traits->IsBlinking());
	TestTrue(TEXT("...gone from sight"), Spider->IsHidden());
	const FVector Moved = Spider->GetActorLocation() - Start;
	TestTrue(FString::Printf(TEXT("...4-6 m away (%.0f cm)"), Moved.Size2D()), Moved.Size2D() >= Tuning.MinDistance - 1.0 && Moved.Size2D() <= Tuning.MaxDistance + 1.0);
	const FVector FromPlayer = (Start - Player->GetActorLocation()).GetSafeNormal2D();
	TestTrue(TEXT("...aside of whoever hit it, not toward or straight away"), FMath::Abs(FVector::DotProduct(Moved.GetSafeNormal2D(), FromPlayer)) < 0.45);
	Traits->Advance(Tuning.GoneSeconds + Frame);
	TestFalse(TEXT("Back after its moment gone"), Traits->IsBlinking() || Spider->IsHidden());
	TestEqual(TEXT("One blink"), Traits->GetBlinkCount(), 1);

	Hurt(Spider, Max * 0.2f);
	Traits->Advance(Frame);
	TestEqual(TEXT("Hit hard again at once: no blink (every 4 s at most)"), Traits->GetBlinkCount(), 1);
	Traits->Advance(Tuning.Cooldown);
	Hurt(Spider, Max * 0.1f);
	Traits->Advance(Frame);
	TestEqual(TEXT("After its wait, hit hard: it blinks again"), Traits->GetBlinkCount(), 2);
	Traits->Advance(Tuning.GoneSeconds + Frame);

	Traits->Advance(Tuning.Cooldown);
	Spider->DevPutInState(ECreatureState::Attack, Player);
	Hurt(Spider, Max * 0.1f);
	Traits->Advance(Frame);
	TestEqual(TEXT("Mid-attack it never blinks"), Traits->GetBlinkCount(), 2);
	Spider->DevPutInState(ECreatureState::Chase, Player);
	Traits->Advance(Frame);
	TestEqual(TEXT("...but blinks as the attack ends (the hard hit kept)"), Traits->GetBlinkCount(), 3);
	return true;
}

// ---------------------------------------------------------------------------
// A new life
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCreatureTraitNewLifeTest, "Looter.Creatures.Traits.NewLife",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCreatureTraitNewLifeTest::RunTest(const FString& Parameters)
{
	// Coming back (its health reset, as a respawn does) starts every trait afresh; taking its traits undoes them all.
	FEditorScriptExecutionGuard RunEvents;
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	ACreatureBase* Spider = SpawnSpider(WorldWrapper.GetTestWorld(), FVector::ZeroVector);
	UHealthComponent* Health = HealthOf(Spider);
	if (!TestNotNull(TEXT("A spider with health"), Health))
	{
		return false;
	}
	const float Chase = Spider->ChaseSpeed;
	const float Fleet = Settings().Fleet.SpeedScale;
	const float Rage = Settings().Vengeful.SpeedScale;
	UCreatureTraitComponent* Traits = Give(*Spider, { ECreatureTrait::Warded, ECreatureTrait::Vengeful, ECreatureTrait::Fleet });
	TestEqual(TEXT("Three traits"), Traits->GetTraits().Num(), 3);
	Traits->SetTraits(TArray<ECreatureTrait>({ ECreatureTrait::Warded, ECreatureTrait::Vengeful, ECreatureTrait::Fleet }));
	TestEqual(TEXT("The same ones again change nothing (Fleet not twice)"), Spider->ChaseSpeed, Chase * Fleet, 0.01f);
	const float Max = Health->GetMaxHealth();
	// Through the ward (30%) to a quarter of its health: the ward broken, it enraged.
	Hurt(Spider, Max * (0.75f + Settings().Warded.HealthShare));
	TestFalse(TEXT("Ward broken"), Traits->IsWardUp());
	TestTrue(TEXT("...enraged"), Traits->IsEnraged());
	TestEqual(TEXT("...Fleet and rage together"), Spider->ChaseSpeed, Chase * Fleet * Rage, 0.01f);
	BossTestWorld::Kill(Spider);
	TestEqual(TEXT("Dead: its rage gone, Fleet kept"), Spider->ChaseSpeed, Chase * Fleet, 0.01f);
	Health->ResetHealth();
	TestTrue(TEXT("A new life: a full ward"), Traits->GetWardFraction() == 1.f && Health->Shield.IsBound());
	TestFalse(TEXT("...calm"), Traits->IsEnraged());
	TestEqual(TEXT("...Fleet once"), Spider->ChaseSpeed, Chase * Fleet, 0.01f);
	Traits->SetTraits(TArray<ECreatureTrait>());
	TestEqual(TEXT("Its traits taken: its own pace"), Spider->ChaseSpeed, Chase, 0.01f);
	TestFalse(TEXT("...no shield"), Health->Shield.IsBound());
	return true;
}

#endif

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Combat/HealthComponent.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/FightDirectorSubsystem.h"
#include "Creatures/FightTuning.h"
#include "Creatures/UnpaidCreature.h"
#include "Creatures/UnpaidFightRules.h"
#include "Creatures/UnpaidFightTuning.h"
#include "Tests/EncounterTestWorld.h"
#include "Tests/LocomotionTestWorld.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "HAL/IConsoleManager.h"
#include "Tests/AutomationCommon.h"

// The Unpaid as a pack with a head (FIGHTS.md lane 5, 2026-10-29; Creatures/UnpaidCreatureFadeBack.cpp): a hurt Basic fades
// back once a life to the ring's outer edge, waits, and comes on again; a led pack's rally is its leader's alone, none in
// the lull after its fall, and the packmates' coals gutter; a camping player is punished (no lead, a wider lunge, cinders
// sooner, more flanks); a step that lands right by its target holds longer (the chapel's dais).

namespace
{
	/** A test level with a fight director in it (one is made only in played worlds unless a test lets it). */
	struct FFadeLevel
	{
		FTestWorldWrapper Wrapper;
		bool Create()
		{
			UFightDirectorSubsystem::bAllowInTestWorlds = true;
			const bool bMade = Wrapper.CreateTestWorld(EWorldType::EditorPreview);
			UFightDirectorSubsystem::bAllowInTestWorlds = false;
			return bMade;
		}
	};

	/** Sets a cvar for one test and puts back what it was (the user may have tuned it in this editor). */
	struct FFadeCVar
	{
		IConsoleVariable* Variable = nullptr;
		FString Was;
		FFadeCVar(const TCHAR* Name, const TCHAR* Value)
		{
			Variable = IConsoleManager::Get().FindConsoleVariable(Name);
			if (Variable)
			{
				Was = Variable->GetString();
				Variable->Set(Value, ECVF_SetByConsole);
			}
		}
		~FFadeCVar()
		{
			if (Variable)
			{
				Variable->Set(*Was, ECVF_SetByConsole);
			}
		}
	};

	AUnpaidCreature* SpawnUnpaid(UWorld* World, const FVector& Feet)
	{
		return Cast<AUnpaidCreature>(EncounterTestWorld::SpawnCreature(World, AUnpaidCreature::StaticClass(), Feet));
	}

	constexpr float Frame = 1.f / 60.f;

	/** One frame of an Unpaid's own (its movement isn't ticked: it stays where it stands unless it blinks). */
	void TickUnpaid(UWorld& World, AUnpaidCreature& Unpaid)
	{
		World.TimeSeconds += Frame;
		Unpaid.ConsumeMovementInputVector();
		Unpaid.Tick(Frame);
	}

	void SetHealthShare(AUnpaidCreature& Unpaid, float Share)
	{
		if (UHealthComponent* Health = Unpaid.FindComponentByClass<UHealthComponent>())
		{
			Health->SetHealth(Health->GetMaxHealth() * Share);
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnpaidKitFadeBackTest, "Looter.Creatures.Unpaid.Kit.FadeBack",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FUnpaidKitFadeBackTest::RunTest(const FString& Parameters)
{
	// Its numbers: now and then, only badly hurt, back to the ring's outer edge, a short wait (a window, not a retreat).
	const FUnpaidFightTuning Tuning;
	const FFightTuning Fight;
	TestTrue(TEXT("A chance, not a rule"), Tuning.FadeBack > 0.f && Tuning.FadeBack < 1.f);
	TestTrue(TEXT("Only badly hurt"), Tuning.FadeBackBelow > 0.f && Tuning.FadeBackBelow <= 0.5f);
	TestTrue(TEXT("Out past the ring"), Tuning.FadeBackNear > Fight.RingRadius && Tuning.FadeBackFar >= Tuning.FadeBackNear);
	TestTrue(TEXT("...but still in the fight"), Tuning.FadeBackFar <= Tuning.FlankMaxRange);
	TestTrue(TEXT("A short wait"), Tuning.FadeBackWait >= 1.f && Tuning.FadeBackWait <= 3.f);

	// Who may: a hurt Basic, once a life.
	TestTrue(TEXT("A Basic at 30%"), UnpaidFightRules::MayFadeBack(true, 0.3f, 0.35f, false));
	TestFalse(TEXT("A Restless never"), UnpaidFightRules::MayFadeBack(false, 0.3f, 0.35f, false));
	TestFalse(TEXT("Not at half health"), UnpaidFightRules::MayFadeBack(true, 0.5f, 0.35f, false));
	TestFalse(TEXT("Once a life"), UnpaidFightRules::MayFadeBack(true, 0.3f, 0.35f, true));
	TestFalse(TEXT("Not dead"), UnpaidFightRules::MayFadeBack(true, 0.f, 0.35f, false));

	// Where: away from them on its own side, in the band, straight back first.
	const TArray<FVector> Spots = UnpaidFightRules::FadeBackSpots(FVector(200.0, 0.0, 30.0), FVector::ZeroVector, 500.f, 650.f, 30.f);
	if (TestEqual(TEXT("Fifteen spots to try"), Spots.Num(), 15))
	{
		TestTrue(TEXT("First straight back, in the band's middle"), Spots[0].Equals(FVector(575.0, 0.0, 30.0), 0.5));
		for (const FVector& Spot : Spots)
		{
			const float Distance = static_cast<float>(Spot.Size2D());
			TestTrue(FString::Printf(TEXT("%.0f cm off: in the band"), Distance), Distance >= 499.f && Distance <= 651.f);
			TestTrue(TEXT("...on its own side (within 50 degrees of straight back)"),
				FVector::DotProduct(Spot.GetSafeNormal2D(), FVector::ForwardVector) >= FMath::Cos(FMath::DegreesToRadians(50.5f)));
			TestNearlyEqual(TEXT("...at its feet's height"), static_cast<float>(Spot.Z), 30.f, 0.01f);
		}
	}

	// In play: a hurt Basic at its turn's end blinks out to the edge, waits there, then comes on; once a life.
	FFadeLevel Level;
	if (!TestTrue(TEXT("Test level made"), Level.Create()))
	{
		return false;
	}
	UWorld* World = Level.Wrapper.GetTestWorld();
	FEditorScriptExecutionGuard RunActorEvents;
	FFadeCVar KitOn(TEXT("Looter.Unpaid.Kit"), TEXT("1"));
	LocomotionTestWorld::SpawnBlock(World, FVector(0.0, 0.0, -50.0), FVector(6000.0, 6000.0, 100.0));
	ACharacter* Player = EncounterTestWorld::SpawnPlayer(World, FVector(0.0, 0.0, 90.0));
	AUnpaidCreature* Unpaid = SpawnUnpaid(World, FVector(250.0, 0.0, 0.0));
	AUnpaidCreature* Healthy = SpawnUnpaid(World, FVector(-250.0, 0.0, 0.0));
	if (!TestNotNull(TEXT("Player stand-in"), Player) || !TestNotNull(TEXT("Unpaid spawned"), Unpaid) || !TestNotNull(TEXT("..."), Healthy))
	{
		return false;
	}
	Healthy->DevPutInState(ECreatureState::Chase, Player);
	TestFalse(TEXT("At full health it fights on"), Healthy->TryFadeBack(false));

	// A Basic, as its rules ask (a test spawn rolls no rank, but say so).
	Unpaid->SetRank(ECreatureRank::Basic);
	Unpaid->DevPutInState(ECreatureState::Chase, Player);
	SetHealthShare(*Unpaid, 0.3f);
	if (!TestTrue(TEXT("Hurt, it fades back"), Unpaid->TryFadeBack(false)))
	{
		return false;
	}
	TestTrue(TEXT("...fading out"), Unpaid->IsPhasing() && Unpaid->IsFadingBack());
	for (int32 Index = 0; Index < 120 && Unpaid->IsPhasing(); ++Index)
	{
		TickUnpaid(*World, *Unpaid);
	}
	TestFalse(TEXT("Back in within two seconds"), Unpaid->IsPhasing());
	const float Off = static_cast<float>(FVector::Dist2D(Unpaid->GetActorLocation(), Player->GetActorLocation()));
	TestTrue(FString::Printf(TEXT("At the ring's edge (%.0f cm off)"), Off), Off >= Tuning.FadeBackNear - 30.f && Off <= Tuning.FadeBackFar + 30.f);
	TestTrue(TEXT("...on its own side"), Unpaid->GetActorLocation().X > 0.0);
	TestTrue(TEXT("...waiting there"), Unpaid->IsFadingBack());
	TestFalse(TEXT("...without stepping in again"), Unpaid->IsPhasing());
	const int32 WaitFrames = FMath::CeilToInt((Tuning.FadeBackWait + 0.3f) / Frame);
	for (int32 Index = 0; Index < WaitFrames; ++Index)
	{
		TickUnpaid(*World, *Unpaid);
	}
	TestFalse(TEXT("Its wait done, it comes on again"), Unpaid->IsFadingBack());
	TestFalse(TEXT("Once a life"), Unpaid->TryFadeBack(false));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnpaidKitLeaderTest, "Looter.Creatures.Unpaid.Kit.Leader",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FUnpaidKitLeaderTest::RunTest(const FString& Parameters)
{
	// Who screams: a leaderless pack as before; a led pack its leader alone, more often; none in the lull after its fall.
	const FUnpaidFightTuning Tuning;
	TestNearlyEqual(TEXT("Leaderless: anyone, the usual cooldown"), UnpaidFightRules::RallyCooldownFor(false, false, false, 16.f, 12.f), 16.f, 0.001f);
	TestNearlyEqual(TEXT("Led: the leader, its own cooldown"), UnpaidFightRules::RallyCooldownFor(true, true, false, 16.f, 12.f), 12.f, 0.001f);
	TestTrue(TEXT("Led: never a packmate"), UnpaidFightRules::RallyCooldownFor(true, false, false, 16.f, 12.f) < 0.f);
	TestTrue(TEXT("In the lull: nobody"), UnpaidFightRules::RallyCooldownFor(true, true, true, 16.f, 12.f) < 0.f);
	TestTrue(TEXT("...not even a leaderless one"), UnpaidFightRules::RallyCooldownFor(false, false, true, 16.f, 12.f) < 0.f);
	TestTrue(TEXT("A leader screams sooner than a leaderless pack"), Tuning.LeaderRallyCooldown < Tuning.RallyCooldown);
	TestTrue(TEXT("...but never inside its own frenzy and the next"), Tuning.LeaderRallyCooldown >= 2.f * Tuning.FrenzySeconds);
	TestTrue(TEXT("The gutter: a moment, its tells low"), Tuning.GutterSeconds > 0.f && Tuning.GutterSeconds <= 2.f
		&& Tuning.GutterFlare < Tuning.TellFlare);

	// Its leader down: a packmate's scream dies in its throat and its coal gutters a moment.
	FFadeLevel Level;
	if (!TestTrue(TEXT("Test level made"), Level.Create()))
	{
		return false;
	}
	UWorld* World = Level.Wrapper.GetTestWorld();
	FEditorScriptExecutionGuard RunActorEvents;
	FFadeCVar KitOn(TEXT("Looter.Unpaid.Kit"), TEXT("1"));
	LocomotionTestWorld::SpawnBlock(World, FVector(0.0, 0.0, -50.0), FVector(6000.0, 6000.0, 100.0));
	ACharacter* Player = EncounterTestWorld::SpawnPlayer(World, FVector(0.0, 0.0, 90.0));
	AUnpaidCreature* Leader = SpawnUnpaid(World, FVector(900.0, 0.0, 0.0));
	AUnpaidCreature* Packmate = SpawnUnpaid(World, FVector(900.0, 400.0, 0.0));
	if (!TestNotNull(TEXT("Player stand-in"), Player) || !TestNotNull(TEXT("Leader"), Leader) || !TestNotNull(TEXT("Packmate"), Packmate))
	{
		return false;
	}
	Packmate->DevPutInState(ECreatureState::Chase, Player);
	TestTrue(TEXT("The packmate screams"), Packmate->StartKitAction(EUnpaidAction::Rally));
	Packmate->OnPackLeaderDown(*Leader);
	TestTrue(TEXT("...its leader falls: the scream dies"), Packmate->GetKitAction() == EUnpaidAction::None);
	TestTrue(TEXT("...its coal gutters"), Packmate->IsGuttering());
	const int32 GutterFrames = FMath::CeilToInt((Tuning.GutterSeconds + 0.1f) / Frame);
	for (int32 Index = 0; Index < GutterFrames; ++Index)
	{
		TickUnpaid(*World, *Packmate);
	}
	TestFalse(TEXT("...for a moment only"), Packmate->IsGuttering());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnpaidKitCampingTest, "Looter.Creatures.Unpaid.Kit.Camping",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FUnpaidKitCampingTest::RunTest(const FString& Parameters)
{
	// A moving player sees the plain numbers; a camping one no lead, a wider lunge, cinders sooner, more flanks.
	const FUnpaidFightTuning Tuning;
	const FUnpaidFightTuning Moving = Tuning.ForCamping(false);
	TestNearlyEqual(TEXT("Moving: the lead as it is"), Moving.LeadShare, Tuning.LeadShare, 0.0001f);
	TestNearlyEqual(TEXT("...the lunge as it is"), Moving.LungeHitRadius, Tuning.LungeHitRadius, 0.0001f);
	TestNearlyEqual(TEXT("...the cinders as they are"), Moving.CinderGap, Tuning.CinderGap, 0.0001f);
	TestNearlyEqual(TEXT("...the flanks as they are"), Moving.FlankChance, Tuning.FlankChance, 0.0001f);
	const FUnpaidFightTuning Camping = Tuning.ForCamping(true);
	TestNearlyEqual(TEXT("Camping: no lead (a still target needs none)"), Camping.LeadShare, 0.f, 0.0001f);
	TestNearlyEqual(TEXT("...a lunge 20 cm wider"), Camping.LungeHitRadius, Tuning.LungeHitRadius + 20.f, 0.0001f);
	TestNearlyEqual(TEXT("...cinders at 0.6 of the gap"), Camping.CinderGap, Tuning.CinderGap * 0.6f, 0.0001f);
	TestNearlyEqual(TEXT("...flanks at 0.6"), Camping.FlankChance, 0.6f, 0.0001f);
	TestTrue(TEXT("...still a fair cinder gap (a second or more)"), Camping.CinderGap >= 1.f);
	FUnpaidFightTuning Off = Tuning;
	Off.CampPunish = 0;
	TestNearlyEqual(TEXT("CampPunish 0: as it was"), Off.ForCamping(true).LungeHitRadius, Tuning.LungeHitRadius, 0.0001f);

	// The dais fix: a step that lands right by its target holds longer; elsewhere the beat it always had.
	TestNearlyEqual(TEXT("Landed 1.4 m off: the long hold"), UnpaidFightRules::ArrivalHoldFor(140.f, 0.6f, 0.8f, 150.f), 0.8f, 0.0001f);
	TestNearlyEqual(TEXT("...at exactly 1.5 m too (the to-sight ring)"), UnpaidFightRules::ArrivalHoldFor(150.f, 0.6f, 0.8f, 150.f), 0.8f, 0.0001f);
	TestNearlyEqual(TEXT("Landed 3 m off: the usual"), UnpaidFightRules::ArrivalHoldFor(300.f, 0.6f, 0.8f, 150.f), 0.6f, 0.0001f);
	TestTrue(TEXT("The long hold is longer, under a second"), Tuning.ArrivalHoldNear > Tuning.ArrivalHold && Tuning.ArrivalHoldNear < 1.f);

	// Nobody fought: the director reads no camping.
	FFadeLevel Level;
	if (!TestTrue(TEXT("Test level made"), Level.Create()))
	{
		return false;
	}
	UWorld* World = Level.Wrapper.GetTestWorld();
	FEditorScriptExecutionGuard RunActorEvents;
	ACharacter* Player = EncounterTestWorld::SpawnPlayer(World, FVector(0.0, 0.0, 90.0));
	UFightDirectorSubsystem* Director = World->GetSubsystem<UFightDirectorSubsystem>();
	if (TestNotNull(TEXT("A director"), Director) && TestNotNull(TEXT("Player stand-in"), Player))
	{
		TestFalse(TEXT("No fight, no camping"), Director->GetPlayerRead(*Player).IsCamping());
	}
	return true;
}

#endif

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Creatures/FightRules.h"
#include "Creatures/PackRules.h"
#include "Creatures/UnpaidCreature.h"
#include "Creatures/UnpaidLifeTuning.h"
#include "Creatures/UnpaidRiseRules.h"
#include "Tests/EncounterTestWorld.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Tests/AutomationCommon.h"

// The Unpaid's rise out of the ground (2026-10-31; Creatures/UnpaidRiseRules, UnpaidCreatureRise.cpp): its phases and their
// timings, how deep it is through them (a hand first, two heaves, a settle), the surround's pattern (the first ahead of the
// player, the next behind, the sides, the gaps; each a beat after the one before; never two in one grave nor one under their
// feet), the burst's slots, and a pack rising round a player in a test level (where each comes up, buried until its turn, the
// hand breaking the ground before the head, up and whole at the end).

namespace
{
	constexpr float Frame = 1.f / 60.f;

	/** Angle apart (degrees, 0-180). */
	float Apart(float A, float B)
	{
		return FMath::Abs(FMath::FindDeltaAngleDegrees(A, B));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnpaidRiseRulesTest, "Looter.Creatures.Unpaid.Rise.Rules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FUnpaidRiseRulesTest::RunTest(const FString& Parameters)
{
	using P = EUnpaidRisePhase;
	// The quick rise (RiseIn: a boss's adds, a toll's dead) is the class's RiseSeconds long, with no wait and no rumble.
	const FUnpaidRiseTiming Quick = UnpaidRiseRules::Timing(EUnpaidRiseStyle::Quick, 3.f, 1.f);
	TestNearlyEqual(TEXT("A quick rise is RiseSeconds long"), Quick.Length(), AUnpaidCreature::RiseSeconds, 0.001f);
	TestEqual(TEXT("...with no wait"), Quick.Buried, 0.f);
	TestEqual(TEXT("...and no rumble"), Quick.Rumble, 0.f);
	float Into = 0.f;
	TestTrue(TEXT("...its hand breaking the ground at once"), UnpaidRiseRules::PhaseAt(0.f, Quick, Into) == P::Breach);

	// An ambush's: buried for its stagger, the ground rumbling over its wait's last moments (never longer than the wait).
	const FUnpaidRiseTiming Ambush = UnpaidRiseRules::Timing(EUnpaidRiseStyle::Ambush, 1.2f, 0.7f);
	TestNearlyEqual(TEXT("An ambush's waits its stagger"), Ambush.Buried, 1.2f, 0.001f);
	TestNearlyEqual(TEXT("...the rumble its last 0.7 s"), Ambush.Rumble, 0.7f, 0.001f);
	TestNearlyEqual(TEXT("A rumble never outlasts the wait"), UnpaidRiseRules::Timing(EUnpaidRiseStyle::Ambush, 0.3f, 0.7f).Rumble, 0.3f, 0.001f);
	TestFalse(TEXT("No rumble early in the wait"), UnpaidRiseRules::IsRumbling(0.2f, Ambush));
	TestTrue(TEXT("...the rumble just before its hand"), UnpaidRiseRules::IsRumbling(1.1f, Ambush));
	TestFalse(TEXT("...and none once the hand is out"), UnpaidRiseRules::IsRumbling(1.3f, Ambush));
	TestTrue(TEXT("Its brain waits past the haul, into the stand"), Ambush.BrainFree() > Ambush.Buried + Ambush.Breach + Ambush.Haul
		&& Ambush.BrainFree() < Ambush.Length());

	// The phases in order, each once, and None after.
	P Last = P::None;
	TArray<P> Seen;
	for (float Time = 0.f; Time < Ambush.Length() + 0.2f; Time += 0.01f)
	{
		const P Now = UnpaidRiseRules::PhaseAt(Time, Ambush, Into);
		TestTrue(TEXT("Into a phase stays in 0-1"), Into >= 0.f && Into <= 1.f);
		if (Now != Last)
		{
			Seen.Add(Now);
			Last = Now;
		}
	}
	TestTrue(TEXT("Buried, the breach, the haul, the stand, then up"), Seen.Num() == 5 && Seen[0] == P::Buried && Seen[1] == P::Breach
		&& Seen[2] == P::Haul && Seen[3] == P::Stand && Seen[4] == P::None);
	TestTrue(TEXT("Before its start: none"), UnpaidRiseRules::PhaseAt(-0.1f, Ambush, Into) == P::None);

	// How deep: buried deep, only rising from there (the haul's gather holds), a touch past its hang and settled at 0.
	for (const EUnpaidRiseStyle Style : { EUnpaidRiseStyle::Ambush, EUnpaidRiseStyle::Quick })
	{
		const FUnpaidRiseTiming Timing = UnpaidRiseRules::Timing(Style, 1.f, 0.5f);
		const TCHAR* Name = Style == EUnpaidRiseStyle::Ambush ? TEXT("ambush") : TEXT("quick");
		float Previous = -1000.f;
		float Highest = -1000.f;
		bool bRises = true;
		for (float Time = 0.f; Time < Timing.Length(); Time += 0.005f)
		{
			const P Now = UnpaidRiseRules::PhaseAt(Time, Timing, Into);
			const float Sink = UnpaidRiseRules::Sink(Style, Now, Into);
			// Rising all the way through the climb; only the stand's settle comes back down (by its overshoot).
			if (Now != P::Stand)
			{
				bRises &= Sink >= Previous - 0.01f;
			}
			Highest = FMath::Max(Highest, Sink);
			Previous = Sink;
		}
		TestTrue(FString::Printf(TEXT("%s: it only rises through the climb"), Name), bRises);
		TestTrue(FString::Printf(TEXT("%s: a touch past its hang at most (%.1f cm)"), Name, Highest), Highest > 0.f && Highest <= 6.f);
		TestNearlyEqual(FString::Printf(TEXT("%s: settled at its hang"), Name), UnpaidRiseRules::Sink(Style, P::Stand, 1.f), 0.f, 0.01f);
		TestNearlyEqual(FString::Printf(TEXT("%s: up once it's done"), Name), UnpaidRiseRules::Sink(Style, P::None, 0.f), 0.f, 0.01f);
	}
	TestNearlyEqual(TEXT("Buried as deep as its rules say"), UnpaidRiseRules::Sink(EUnpaidRiseStyle::Ambush, P::Buried, 0.5f),
		-UnpaidRiseRules::BuriedDepth, 0.01f);
	TestNearlyEqual(TEXT("A quick rise starts shallower"), UnpaidRiseRules::Sink(EUnpaidRiseStyle::Quick, P::Breach, 0.f),
		-UnpaidRiseRules::QuickDepth, 0.01f);
	TestTrue(TEXT("Its breach brings it up only a little (the hand, not the head)"),
		UnpaidRiseRules::Sink(EUnpaidRiseStyle::Ambush, P::Breach, 1.f) < -150.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnpaidRiseSurroundTest, "Looter.Creatures.Unpaid.Rise.Surround",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FUnpaidRiseSurroundTest::RunTest(const FString& Parameters)
{
	// The pattern: the first ahead (they see the first hand come up), the second behind (turn round!), then the sides.
	TestNearlyEqual(TEXT("The first rises where they look"), UnpaidRiseRules::PatternOffset(0), 0.f, 0.01f);
	TestNearlyEqual(TEXT("The second behind them"), Apart(UnpaidRiseRules::PatternOffset(1), 0.f), 180.f, 0.01f);
	TestTrue(TEXT("The third and fourth at their sides, one each"), Apart(UnpaidRiseRules::PatternOffset(2), 0.f) >= 80.f
		&& Apart(UnpaidRiseRules::PatternOffset(3), 0.f) >= 80.f && FMath::Sign(UnpaidRiseRules::PatternOffset(2)) != FMath::Sign(UnpaidRiseRules::PatternOffset(3)));
	for (int32 Slot = 0; Slot < UnpaidRiseRules::PatternSlots; ++Slot)
	{
		for (int32 Other = Slot + 1; Other < UnpaidRiseRules::PatternSlots; ++Other)
		{
			TestTrue(FString::Printf(TEXT("Slots %d and %d come up 40 degrees or more apart"), Slot, Other),
				Apart(UnpaidRiseRules::PatternOffset(Slot), UnpaidRiseRules::PatternOffset(Other)) >= 40.f - 0.01f);
		}
		if (Slot > 0)
		{
			// The stagger walks round them: no two in a row side by side.
			TestTrue(FString::Printf(TEXT("Slot %d is 80 degrees or more round from the one before"), Slot),
				Apart(UnpaidRiseRules::PatternOffset(Slot), UnpaidRiseRules::PatternOffset(Slot - 1)) >= 80.f);
		}
	}
	TestTrue(TEXT("Round again, half a gap round"), Apart(UnpaidRiseRules::PatternOffset(UnpaidRiseRules::PatternSlots), 0.f) > 10.f);
	TestNearlyEqual(TEXT("The bearing: their view, the pattern and the burst's turn"), UnpaidRiseRules::SurroundBearing(1, 90.f, 10.f),
		FMath::UnwindDegrees(90.f + 180.f + 10.f), 0.01f);

	// How far: every other one a little farther, farther again each time round.
	TestNearlyEqual(TEXT("Slot 0 at the radius"), UnpaidRiseRules::SurroundRadius(0, 480.f), 480.f, 0.01f);
	TestTrue(TEXT("Slot 1 a little farther"), UnpaidRiseRules::SurroundRadius(1, 480.f) > 480.f && UnpaidRiseRules::SurroundRadius(1, 480.f) < 600.f);
	TestTrue(TEXT("Round again, farther"), UnpaidRiseRules::SurroundRadius(UnpaidRiseRules::PatternSlots, 480.f) > UnpaidRiseRules::SurroundRadius(0, 480.f));

	// The stagger: each a gap after the one before; its jitter never as much as a gap.
	for (int32 Slot = 1; Slot < 8; ++Slot)
	{
		TestTrue(FString::Printf(TEXT("Slot %d breaks the ground after slot %d, however they roll"), Slot, Slot - 1),
			UnpaidRiseRules::StaggerDelay(Slot, 0.35f, 0.45f, 0.f) > UnpaidRiseRules::StaggerDelay(Slot - 1, 0.35f, 0.45f, 1.f));
	}
	TestNearlyEqual(TEXT("The first after First"), UnpaidRiseRules::StaggerDelay(0, 0.35f, 0.45f, 0.f), 0.35f, 0.001f);

	// Where it tries: its own spot first, then round and nearer and farther, all at the player's feet.
	const FVector Feet(100.0, 200.0, 50.0);
	const TArray<FVector> Tries = UnpaidRiseRules::SurroundCandidates(Feet, 90.f, 500.f);
	TestTrue(TEXT("Several spots to try"), Tries.Num() >= 5);
	if (Tries.Num() > 0)
	{
		TestTrue(TEXT("...its own first: 5 m on its bearing"), Tries[0].Equals(Feet + FVector(0.0, 500.0, 0.0), 0.1));
	}
	for (const FVector& Each : Tries)
	{
		const double Out = FVector::Dist2D(Each, Feet);
		TestTrue(TEXT("...each within 0.8-1.2 of the radius, at their feet's height"), Out >= 399.0 && Out <= 601.0 && FMath::IsNearlyEqual(Each.Z, Feet.Z));
	}
	// Never under their feet, never two in one grave.
	const TArray<FVector> Taken = { FVector(500.0, 0.0, 0.0) };
	TestFalse(TEXT("Too near the player"), UnpaidRiseRules::IsClear(FVector(200.0, 0.0, 0.0), {}, 230.f, FVector::ZeroVector, 320.f));
	TestFalse(TEXT("Too near another of the burst"), UnpaidRiseRules::IsClear(FVector(500.0, 150.0, 0.0), Taken, 230.f, FVector::ZeroVector, 320.f));
	TestTrue(TEXT("Clear of both"), UnpaidRiseRules::IsClear(FVector(0.0, 500.0, 0.0), Taken, 230.f, FVector::ZeroVector, 320.f));

	// The burst: slots in turn while they come close together, a fresh burst (where they look now) after a lull.
	FUnpaidRiseBurst Burst;
	TestEqual(TEXT("The first slot"), UnpaidRiseRules::ClaimSlot(Burst, 10.0, 45.f, 1.5f, 0.5f, 20.f), 0);
	TestEqual(TEXT("...the next"), UnpaidRiseRules::ClaimSlot(Burst, 10.0, 0.f, 1.5f, 0.5f, 20.f), 1);
	TestEqual(TEXT("...and the next, a moment on"), UnpaidRiseRules::ClaimSlot(Burst, 11.0, 0.f, 1.5f, 0.5f, 20.f), 2);
	TestNearlyEqual(TEXT("...all round where they looked as it began"), Burst.ViewYaw, 45.f, 0.01f);
	TestEqual(TEXT("After a lull, a fresh burst"), UnpaidRiseRules::ClaimSlot(Burst, 14.0, 120.f, 1.5f, 1.f, 20.f), 0);
	TestTrue(TEXT("...round where they look now, turned at most its turn"), FMath::IsNearlyEqual(Burst.ViewYaw, 120.f)
		&& FMath::Abs(Burst.Turn) <= 20.f + 0.01f && Burst.Spots.IsEmpty());

	// A pack of six round a player looking along +X: the first in front, at least one behind them, none nearer each other
	// than the spacing, all off their feet.
	const FUnpaidLifeTuning Tuning;
	TArray<FVector> Ring;
	int32 Behind = 0;
	for (int32 Slot = 0; Slot < 6; ++Slot)
	{
		const float Radians = FMath::DegreesToRadians(UnpaidRiseRules::SurroundBearing(Slot, 0.f, 0.f));
		const float Out = UnpaidRiseRules::SurroundRadius(Slot, Tuning.RiseRadius);
		const FVector Spot(FMath::Cos(Radians) * Out, FMath::Sin(Radians) * Out, 0.f);
		Behind += FightRules::Frontness(PackRules::BearingAround(FVector::ZeroVector, Spot), 0.f) < 0.25f ? 1 : 0;
		TestTrue(FString::Printf(TEXT("Slot %d clear of the player and the others"), Slot),
			UnpaidRiseRules::IsClear(Spot, Ring, Tuning.RiseSpacing, FVector::ZeroVector, Tuning.RiseClear));
		Ring.Add(Spot);
	}
	TestTrue(TEXT("The first of six in front of them"), FightRules::Frontness(PackRules::BearingAround(FVector::ZeroVector, Ring[0]), 0.f) > 0.95f);
	TestTrue(FString::Printf(TEXT("...and some behind them (%d)"), Behind), Behind >= 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnpaidRisePlayTest, "Looter.Creatures.Unpaid.Rise.Play",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FUnpaidRisePlayTest::RunTest(const FString& Parameters)
{
	// Six of the dead rise round a player on open ground, as an ambush that springs round them (RiseAround) has them.
	FTestWorldWrapper Wrapper;
	if (!TestTrue(TEXT("Test world created"), Wrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = Wrapper.GetTestWorld();
	EncounterTestWorld::SpawnBlocks(World, { FBox(FVector(-4000.0, -4000.0, -100.0), FVector(4000.0, 4000.0, 0.0)) });
	ACharacter* Player = EncounterTestWorld::SpawnPlayer(World, FVector(0.0, 0.0, 100.0));
	if (!TestNotNull(TEXT("Player stand-in"), Player))
	{
		return false;
	}
	TArray<AUnpaidCreature*> Dead;
	for (int32 Index = 0; Index < 6; ++Index)
	{
		AUnpaidCreature* Unpaid = Cast<AUnpaidCreature>(EncounterTestWorld::SpawnCreature(World, AUnpaidCreature::StaticClass(),
			FVector(1500.0, -500.0 + 200.0 * Index, 0.0)));
		if (!TestNotNull(TEXT("Unpaid spawned"), Unpaid))
		{
			return false;
		}
		Unpaid->RiseAmbush(Player, /*bSurround*/ true);
		Unpaid->AlertTo(Player);
		Dead.Add(Unpaid);
	}
	const FUnpaidLifeTuning Tuning;
	const FVector Center = Player->GetActorLocation();
	TArray<FVector> Spots;
	int32 Behind = 0;
	for (int32 Index = 0; Index < Dead.Num(); ++Index)
	{
		const AUnpaidCreature* Unpaid = Dead[Index];
		const FVector Here = Unpaid->GetActorLocation();
		const double Out = FVector::Dist2D(Here, Center);
		TestTrue(FString::Printf(TEXT("Rising round the player, 4-8 m off (%.1f m)"), Out / 100.0), Out >= 380.0 && Out <= 800.0);
		TestTrue(TEXT("...gone under the ground, rising (fading in)"), Unpaid->IsRising() && Unpaid->IsPhasing() && Unpaid->GetPhase() >= 0.99f);
		TestTrue(TEXT("...after the player"), Unpaid->GetTarget() == Player);
		Behind += FightRules::Frontness(PackRules::BearingAround(Center, Here), static_cast<float>(Player->GetViewRotation().Yaw)) < 0.3f ? 1 : 0;
		Spots.Add(Here);
	}
	TestTrue(TEXT("Round them, not in one row: some behind them"), Behind >= 1);
	TestTrue(TEXT("...none in one grave"), EncounterTestWorld::AllApart(Spots, Tuning.RiseSpacing - 1.0));

	// They break the ground one after another, each staying gone until its turn; at the end every one is up and whole.
	TArray<float> BrokeAt;
	BrokeAt.Init(-1.f, Dead.Num());
	// When each was out of its rise and whole: noted then, since a creature stuck in a test level (which never moves it) then
	// phase-steps toward the player, and a phase-step's fade isn't a rise.
	TArray<float> WholeAt;
	WholeAt.Init(-1.f, Dead.Num());
	const bool bModel = Dead[0]->GetMesh()->GetSkeletalMeshAsset() != nullptr && Dead[0]->GetBonePose().Num() >= 20;
	bool bHandFirst = !bModel;
	const FName Hands[2] = { Dead[0]->Rig.LeftArm.Hand, Dead[0]->Rig.RightArm.Hand };
	const FName Head = Dead[0]->Rig.Head;
	for (float Time = 0.f; Time < 6.f; Time += Frame)
	{
		for (int32 Index = 0; Index < Dead.Num(); ++Index)
		{
			AUnpaidCreature* Unpaid = Dead[Index];
			Unpaid->Tick(Frame);
			if (BrokeAt[Index] < 0.f && Unpaid->GetPhase() < 0.99f)
			{
				BrokeAt[Index] = Time;
			}
			if (BrokeAt[Index] >= 0.f && WholeAt[Index] < 0.f && !Unpaid->IsRising() && Unpaid->GetPhase() <= 0.01f)
			{
				WholeAt[Index] = Time;
			}
			// Its hand out of the ground while its head is still under it, at some moment of its breach (component space: Z 0 is
			// the ground under it).
			if (bModel && Index == 0 && !bHandFirst)
			{
				float HandZ = -1000.f;
				float HeadZ = 1000.f;
				for (const FCreatureBonePose& Bone : Unpaid->GetBonePose())
				{
					if (Bone.Bone == Hands[0] || Bone.Bone == Hands[1])
					{
						HandZ = FMath::Max(HandZ, static_cast<float>(Bone.Transform.GetLocation().Z));
					}
					else if (Bone.Bone == Head)
					{
						HeadZ = static_cast<float>(Bone.Transform.GetLocation().Z);
					}
				}
				bHandFirst = HandZ > 0.f && HeadZ < 0.f;
			}
		}
	}
	TestTrue(TEXT("A hand breaks the ground before the head"), bHandFirst);
	if (!bModel)
	{
		AddWarning(TEXT("SK_Unpaid isn't imported: the hand-first check is skipped."));
	}
	TArray<float> Order = BrokeAt;
	for (int32 Index = 0; Index < Dead.Num(); ++Index)
	{
		TestTrue(FString::Printf(TEXT("%d broke the ground (at %.2f s)"), Index, BrokeAt[Index]), BrokeAt[Index] >= 0.f);
		TestTrue(FString::Printf(TEXT("...and was up and whole within its rise (at %.2f s)"), WholeAt[Index]),
			WholeAt[Index] >= BrokeAt[Index] && WholeAt[Index] - BrokeAt[Index] <= UnpaidRiseRules::AmbushBreach + UnpaidRiseRules::AmbushHaul
				+ UnpaidRiseRules::AmbushStand + 0.1f);
	}
	Order.Sort();
	for (int32 Index = 1; Index < Order.Num(); ++Index)
	{
		TestTrue(FString::Printf(TEXT("Staggered: one after another (%.2f s after %.2f s)"), Order[Index], Order[Index - 1]),
			Order[Index] - Order[Index - 1] >= 0.25f);
	}
	TestTrue(FString::Printf(TEXT("The first a moment after the spring (%.2f s), with its rumble first"), Order[0]),
		Order[0] >= Tuning.RiseFirst - 0.05f && Order[0] <= Tuning.RiseFirst + Tuning.RiseGap);

	// The quick rise (a boss's adds): up at once, whole a beat later, its brain free by then.
	AUnpaidCreature* Add = Cast<AUnpaidCreature>(EncounterTestWorld::SpawnCreature(World, AUnpaidCreature::StaticClass(), FVector(-1500.0, 0.0, 0.0)));
	if (TestNotNull(TEXT("An add"), Add))
	{
		Add->RiseIn();
		TestTrue(TEXT("Its rise starts fading in at once"), Add->IsRising() && Add->IsPhasing() && Add->GetPhase() > 0.5f);
		for (float Time = 0.f; Time < AUnpaidCreature::RiseSeconds + 0.2f; Time += Frame)
		{
			Add->Tick(Frame);
		}
		TestFalse(TEXT("...up within RiseSeconds"), Add->IsRising() || Add->IsPhasing());
	}
	return true;
}

#endif

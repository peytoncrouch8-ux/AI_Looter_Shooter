#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Embers/EmberComponent.h"
#include "Embers/Powers/EmberFX.h"
#include "Embers/Powers/EmberFiftyTwoPickup.h"
#include "Player/FirstPersonHandsMotion.h"
#include "Tests/EmberHeroPowerBench.h"
#include "Tests/AutomationCommon.h"

// The ember powers' cast and Odessa's cards (the user's notes 27 and 6, 2026-10-27): the hands' cast pose by kind and its
// clock (never more than 0.15 s before the power), and Fifty-Two Pickup's real cards: their size, their turn in flight,
// stuck in what they hit.

using namespace EmberHeroBench;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEmberCastPoseTest, "Looter.Embers.Cast.Pose",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FEmberCastPoseTest::RunTest(const FString& Parameters)
{
	using namespace FirstPersonHandsMotion;
	// The lead: a power never waits more than 0.15 s on its cast.
	TestTrue(TEXT("The cast's lead is short"), UEmberComponent::CastLeadSeconds() >= 0.f && UEmberComponent::CastLeadSeconds() <= 0.15f);
	// The kinds by power.
	TestTrue(TEXT("Fifty-Two Pickup flings"), CastKindOf(TEXT("FiftyTwoPickup")) == EHandsCast::Fling);
	TestTrue(TEXT("Iron Hide clenches"), CastKindOf(TEXT("IronHide")) == EHandsCast::Clench);
	TestTrue(TEXT("Grave Ward plants"), CastKindOf(TEXT("GraveWard")) == EHandsCast::Plant);
	TestTrue(TEXT("Rail Spike pushes"), CastKindOf(TEXT("RailSpike")) == EHandsCast::Push);
	TestTrue(TEXT("Anything else pushes"), CastKindOf(TEXT("NoSuchPower")) == EHandsCast::Push);

	// A support hand on a foregrip, low and right of the middle.
	const FHandPlacement OnGun = FHandPlacement::Make(FVector(50.f, 12.f, -20.f), FVector(1.f, 0.f, 0.f), FVector(0.f, 0.3f, 1.f),
		FHandFingers::Of(EHandGrip::Forend));
	const float Lead = 0.1f;
	for (const EHandsCast Kind : { EHandsCast::Fling, EHandsCast::Push, EHandsCast::Clench, EHandsCast::Plant })
	{
		const FString Name = FString::Printf(TEXT("Kind %d"), static_cast<int32>(Kind));
		TestTrue(Name + TEXT(": on the gun before"), SupportInCast(OnGun, Kind, -0.01f, Lead).Hold.Equals(OnGun.Hold, 0.01f));
		TestTrue(Name + TEXT(": gathered at the release"), SupportInCast(OnGun, Kind, Lead, Lead).Hold.Equals(CastRaised(Kind).Hold, 0.5f));
		TestTrue(Name + TEXT(": off the gun by then"), FVector::Dist(SupportInCast(OnGun, Kind, Lead, Lead).Hold, OnGun.Hold) > 15.f);
		TestTrue(Name + TEXT(": the raised hand on the left of the view"), CastRaised(Kind).Hold.Y < 0.f);
		const FHandPlacement Out = SupportInCast(OnGun, Kind, Lead + 0.08f, Lead);
		TestTrue(Name + TEXT(": released out where it goes"), FVector::Dist(Out.Hold, CastReleased(Kind).Hold) < 4.f);
		TestTrue(Name + TEXT(": back on the gun after"), SupportInCast(OnGun, Kind, CastSeconds(Lead) + 0.01f, Lead).Hold.Equals(OnGun.Hold, 0.01f));
		// Never a jump: under 2.5 cm in a millisecond (the flare is the quickest part).
		float Worst = 0.f;
		FVector Last = SupportInCast(OnGun, Kind, 0.f, Lead).Hold;
		for (float Time = 0.001f; Time < CastSeconds(Lead) + 0.02f; Time += 0.001f)
		{
			const FVector Now = SupportInCast(OnGun, Kind, Time, Lead).Hold;
			Worst = FMath::Max(Worst, static_cast<float>(FVector::Dist(Now, Last)));
			Last = Now;
		}
		TestTrue(FString::Printf(TEXT("%s: no jump (%.2f cm in a millisecond)"), *Name, Worst), Worst < 2.5f);
	}
	// A power used with no lead (a console use) shows the cast from its release, from the gun.
	TestTrue(TEXT("No lead: flared out from the gun"), FVector::Dist(SupportInCast(OnGun, EHandsCast::Push, 0.03f, 0.f).Hold, OnGun.Hold) > 5.f);
	TestTrue(TEXT("The whole cast under half a second"), CastSeconds(Lead) < 0.5f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEmberCardsTest, "Looter.Embers.Cards",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FEmberCardsTest::RunTest(const FString& Parameters)
{
	// Readable cards: drawn at least twice a real card's size, and the hit reach about the drawn card's half width.
	TestTrue(TEXT("Cards drawn big enough to read"), EmberFX::CardSize() >= 2.f);
	TestTrue(TEXT("...their hit reach close to their size"), UEmberFiftyTwoPickup::CardRadius >= 0.8f * EmberFX::CardShort * EmberFX::CardSize() * 0.5f);
	// Their turn in flight: the face tipped back toward the thrower however it flutters, the long side in the face's plane.
	const FVector Velocity(3000.f, 400.f, 200.f);
	const FVector Back = -Velocity.GetSafeNormal();
	float Least = 1.f;
	for (float Flutter = 0.f; Flutter < UE_TWO_PI; Flutter += 0.2f)
	{
		const FQuat Turn = EmberFX::FlyingCardTurn(Velocity, Flutter * 3.f, Flutter, 0.3f);
		Least = FMath::Min(Least, static_cast<float>(FVector::DotProduct(Turn.GetAxisZ(), Back)));
		TestTrue(TEXT("The long side lies in the face"), FMath::Abs(FVector::DotProduct(Turn.GetAxisX(), Turn.GetAxisZ())) < 0.01f);
	}
	TestTrue(FString::Printf(TEXT("The face always turned back toward the thrower (%.2f)"), Least), Least > 0.25f);

	// Thrown at a spider 3 m off: they cut it and stick in it (and the floor), then are gone.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	SpawnFloor(World);
	FEditorScriptExecutionGuard RunEvents;
	const FBench Bench = SpawnBench(World, FVector(-1500.0, 0.0, 0.0));
	ASpiderCreature* Close = SpawnSpider(World, FVector(-1200.0, 0.0, 0.0));
	if (!TestNotNull(TEXT("The player"), Bench.Body) || !TestNotNull(TEXT("A spider"), Close))
	{
		return false;
	}
	UEmberDefinition* Def = MakeDefinition(TEXT("FiftyTwoPickup"), 0.f, 8.f, 1400.f, 0.f, 0.f);
	UEmberFiftyTwoPickup* Power = NewObject<UEmberFiftyTwoPickup>(Bench.Embers);
	const FEmberContext Context = MakeContext(Bench, Def);
	Power->Activate(Context);
	Power->End(Context, false);
	Power->StepCards(Frame);
	TestTrue(FString::Printf(TEXT("Drawn as cards (%d)"), Power->NumDrawn()), Power->NumDrawn() > 40);
	for (int32 Step = 0; Step < 8; ++Step)
	{
		Power->StepCards(Frame);
	}
	TestTrue(FString::Printf(TEXT("Cards stuck in the spider (%d)"), Power->NumInBodies()), Power->NumInBodies() >= 5);
	TestEqual(TEXT("...all of the cuts at first"), Power->NumInBodies(), Power->NumCuts());
	for (int32 Step = 0; Step < 150; ++Step)
	{
		Power->StepCards(Frame);
	}
	TestEqual(TEXT("Seconds on, none still stuck"), Power->NumStuck(), 0);
	TestEqual(TEXT("...and none drawn"), Power->NumDrawn(), 0);
	return true;
}

#endif

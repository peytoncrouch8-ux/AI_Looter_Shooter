#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "World/HouseLights.h"
#include "World/WindowLifeCard.h"
#include "World/WindowLifeRules.h"
#include "World/WindowLifeSubsystem.h"
#include "World/WindowShutter.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "Tests/AutomationCommon.h"

namespace
{
	const FName Dusk(TEXT("Dusk"));
	const FName Day(TEXT("Day"));

	/** One event seen in a run: its kind, where, when. */
	struct FSeen
	{
		EWindowLifeKind Kind = EWindowLifeKind::None;
		const AActor* Place = nullptr;
		double Time = 0.0;
	};

	/**
	 * Looks from Eyes along View every LookSeconds for Seconds from Start, moving the card on as the game's tick would.
	 * Returns what played.
	 */
	TArray<FSeen> Watch(UWindowLifeSubsystem& Life, const FVector& Eyes, const FVector& View, double Start, double Seconds)
	{
		TArray<FSeen> Seen;
		for (double Now = Start; Now < Start + Seconds; Now += WindowLifeRules::LookSeconds)
		{
			if (AWindowLifeCard* Card = Life.GetCard())
			{
				Card->Advance(WindowLifeRules::LookSeconds);
			}
			const EWindowLifeKind Kind = Life.Check(Eyes, View, Now);
			if (Kind != EWindowLifeKind::None)
			{
				Seen.Add({ Kind, Life.GetLastPlace(), Now });
			}
		}
		return Seen;
	}

	/** A lived-in house of Mesh at Where, turned Yaw, with its lights begun (their household's, the model's). */
	AHouseLights* SpawnHouse(UWorld* World, UStaticMesh* Mesh, const FVector& Where, float Yaw)
	{
		AStaticMeshActor* House = World->SpawnActor<AStaticMeshActor>(Where, FRotator(0.0, Yaw, 0.0));
		if (!House)
		{
			return nullptr;
		}
		House->GetStaticMeshComponent()->SetStaticMesh(Mesh);
		AHouseLights* Lights = World->SpawnActor<AHouseLights>(Where + FVector(0.0, 0.0, 200.0), FRotator(0.0, Yaw, 0.0));
		if (Lights)
		{
			Lights->House = House;
			Lights->DispatchBeginPlay();
		}
		return Lights;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWindowLifeRulesTest, "Looter.World.WindowLife.Rules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWindowLifeRulesTest::RunTest(const FString& Parameters)
{
	using namespace WindowLifeRules;
	// Seen: across the street, looked at, the window facing the player.
	const FVector Window(0.0, 0.0, 200.0);
	const FVector Out(1.0, 0.0, 0.0);
	auto From = [&Window](const FVector& Eyes) { return (Window - Eyes).GetSafeNormal(); };
	const FVector Street(1200.0, 0.0, 170.0);
	TestTrue(TEXT("From across the street, looking at it: seen"), Sees(Street, From(Street), Window, Out));
	const FVector Close(300.0, 0.0, 170.0);
	TestFalse(TEXT("Too close (3 m): somebody inside isn't a glimpse"), Sees(Close, From(Close), Window, Out));
	const FVector Away(4000.0, 0.0, 170.0);
	TestFalse(TEXT("Too far (40 m)"), Sees(Away, From(Away), Window, Out));
	TestFalse(TEXT("Looking away"), Sees(Street, -From(Street), Window, Out));
	TestFalse(TEXT("Looking well to the side (60 degrees off)"), Sees(Street, From(Street).RotateAngleAxis(60.0, FVector::UpVector), Window, Out));
	const FVector Behind(-1200.0, 0.0, 170.0);
	TestFalse(TEXT("From behind the house"), Sees(Behind, From(Behind), Window, Out));
	const FVector Slant(1200.0, 1100.0, 170.0);
	TestTrue(TEXT("From down the street at a slant"), Sees(Slant, From(Slant), Window, Out));
	const FVector EdgeOn(200.0, 1500.0, 170.0);
	TestFalse(TEXT("Edge-on, along the wall"), Sees(EdgeOn, From(EdgeOn), Window, Out));

	// By day only curtains (and shutters left open); at dusk, shadows too.
	TestFalse(TEXT("No shadow by day"), Allows(EWindowLifeKind::Shadow, false));
	TestTrue(TEXT("Curtains by day"), Allows(EWindowLifeKind::Curtain, false));
	TestTrue(TEXT("Shadows at dusk"), Allows(EWindowLifeKind::Shadow, true));
	FRandomStream Random(20261014);
	const EWindowLifeKind Both[] = { EWindowLifeKind::Shadow, EWindowLifeKind::Curtain };
	int32 DayShadows = 0;
	int32 DuskShadows = 0;
	int32 DuskCurtains = 0;
	for (int32 Roll = 0; Roll < 2000; ++Roll)
	{
		DayShadows += PickKind(MakeArrayView(Both), false, Random) == EWindowLifeKind::Shadow ? 1 : 0;
		const EWindowLifeKind AtDusk = PickKind(MakeArrayView(Both), true, Random);
		DuskShadows += AtDusk == EWindowLifeKind::Shadow ? 1 : 0;
		DuskCurtains += AtDusk == EWindowLifeKind::Curtain ? 1 : 0;
	}
	TestEqual(TEXT("By day: never a shadow"), DayShadows, 0);
	TestTrue(FString::Printf(TEXT("At dusk: shadows and curtains by their weights (%d, %d)"), DuskShadows, DuskCurtains),
		FMath::Abs(static_cast<float>(DuskShadows) / 2000.f - ShadowWeight / (ShadowWeight + CurtainWeight)) < 0.05f
		&& DuskShadows + DuskCurtains == 2000);

	// Rare, and one at a time: busy for the event, then a gap; each house rests a while.
	FWindowLifeClock Clock;
	// Two houses: any concrete object stands for one (UObject itself is abstract).
	UObject* First = NewObject<UStaticMeshComponent>(GetTransientPackage());
	UObject* Second = NewObject<UStaticMeshComponent>(GetTransientPackage());
	TestTrue(TEXT("At first anything may play"), Clock.MayPlay(FObjectKey(First), 100.0));
	Clock.Played(FObjectKey(First), 100.0, 3.f, Random);
	TestFalse(TEXT("Not while one plays"), Clock.MayPlay(FObjectKey(Second), 102.0));
	TestFalse(TEXT("...nor in the gap after it"), Clock.MayPlay(FObjectKey(Second), 103.0 + GapMin - 0.1));
	TestTrue(TEXT("Another house after the gap"), Clock.MayPlay(FObjectKey(Second), 103.0 + GapMax + 0.1));
	TestFalse(TEXT("The same house rests"), Clock.MayPlay(FObjectKey(First), 100.0 + HouseRestMin - 0.1));
	TestTrue(TEXT("...for at most its longest rest"), Clock.MayPlay(FObjectKey(First), 100.0 + HouseRestMax + 0.1));

	// A shadow: out of the wall at one side, across, into the wall at the other, faded at both ends; one that stops
	// stands near the middle; a curtain is pulled in and settles back where it hung.
	for (int32 Each = 0; Each < 40; ++Each)
	{
		const FWindowLifeEvent Shadow = MakeEvent(EWindowLifeKind::Shadow, true, Random);
		const FWindowLifePose Start = PoseAt(Shadow, 0.f);
		const FWindowLifePose Middle = PoseAt(Shadow, Shadow.Seconds * 0.5f);
		const FWindowLifePose End = PoseAt(Shadow, Shadow.Seconds);
		TestTrue(TEXT("A shadow starts and ends unseen"), Start.Strength < 0.01f && End.Strength < 0.01f);
		TestTrue(TEXT("...is plain in the middle"), Middle.Strength > 0.7f * ShadowDensity);
		TestTrue(TEXT("...starts past the pane on its side"), FMath::IsNearlyEqual(Start.Slide, Shadow.Side * SlideFar, 0.5f));
		TestTrue(TEXT("...and never past where it started"), FMath::Abs(Middle.Slide) <= SlideFar);
		if (Shadow.bLingers)
		{
			TestTrue(TEXT("One that stops stands near the middle"), FMath::Abs(Middle.Slide) <= SlideNear + 3.f);
			TestTrue(TEXT("...and goes back the way it came"), FMath::IsNearlyEqual(End.Slide, Shadow.Side * SlideFar, 0.5f));
		}
		else
		{
			TestTrue(TEXT("One walking through ends on the other side"), FMath::IsNearlyEqual(End.Slide, -Shadow.Side * SlideFar, 0.5f));
		}
		const FWindowLifeEvent Curtain = MakeEvent(EWindowLifeKind::Curtain, false, Random);
		float Furthest = 0.f;
		for (float Time = 0.f; Time <= Curtain.Seconds; Time += 0.02f)
		{
			Furthest = FMath::Max(Furthest, PoseAt(Curtain, Time).Swing);
		}
		TestTrue(TEXT("A curtain is pulled in no further than its reach"), Furthest <= Curtain.Reach + 1e-3f && Furthest > 0.5f * Curtain.Reach);
		TestTrue(TEXT("...and hangs where it was at the end"), FMath::IsNearlyEqual(PoseAt(Curtain, Curtain.Seconds).Slide, Curtain.Side * CurtainRest, 0.5f));
	}
	TestEqual(TEXT("A shadow's steps and a curtain's swish are heard"), CueFor(EWindowLifeKind::Shadow), FName(StepsCue));
	TestEqual(TEXT("..."), CueFor(EWindowLifeKind::Curtain), FName(CurtainCue));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWindowLifeWindowsTest, "Looter.World.WindowLife.Windows",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWindowLifeWindowsTest::RunTest(const FString& Parameters)
{
	// The saltbox (SM_RR_Home_A) has four lamplit windows; looked at from across the street a shadow or a curtain shows at
	// one now and then: rarely, one at a time, curtains only by day, never from too near or too far or looking away.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UWindowLifeSubsystem* Life = UWindowLifeSubsystem::Get(World);
	UStaticMesh* Saltbox = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Art/Buildings/SM_RR_Home_A.SM_RR_Home_A"));
	UStaticMesh* PorchHouse = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Art/Buildings/SM_RR_Home_H.SM_RR_Home_H"));
	if (!TestNotNull(TEXT("Window life"), Life) || !TestNotNull(TEXT("SM_RR_Home_A"), Saltbox) || !TestNotNull(TEXT("SM_RR_Home_H"), PorchHouse))
	{
		return false;
	}
	TestNotNull(TEXT("M_WindowLife is made (Tools/Unreal/build_window_life_material.py)"),
		LoadObject<UMaterialInterface>(nullptr, AWindowLifeCard::MaterialPath, nullptr, LOAD_NoWarn | LOAD_Quiet));
	Life->SetRandomSeed(20261014);
	// The test level has nothing to hide a window but the houses, and those never stand between.
	Life->SetSightChecks(false);
	AHouseLights* Haskett = SpawnHouse(World, Saltbox, FVector::ZeroVector, 0.f);
	if (!TestNotNull(TEXT("The saltbox's lights"), Haskett)
		|| !TestEqual(TEXT("Its four lamplit windows (SOCKET_Window_1-4: re-import Homestead.py)"), Haskett->NumLitWindows(), 4))
	{
		return false;
	}
	TestEqual(TEXT("It joined the window life"), Life->NumHouses(), 1);
	for (int32 Index = 0; Index < Haskett->NumLitWindows(); ++Index)
	{
		FTransform Window;
		if (TestTrue(TEXT("Each window has its place"), Haskett->GetLitWindow(Index, Window)))
		{
			const FVector Out = Window.GetUnitAxis(EAxis::X);
			const FVector FromMiddle = (Window.GetLocation() - Haskett->House->GetComponentsBoundingBox().GetCenter()).GetSafeNormal2D();
			TestTrue(TEXT("...facing out of the house, level"), FMath::Abs(Out.Z) < 0.01f && FVector::DotProduct(Out, FromMiddle) > 0.3f);
		}
	}

	FTransform Front;
	Haskett->GetLitWindow(0, Front);
	const FVector Out = Front.GetUnitAxis(EAxis::X);
	const FVector Eyes = Front.GetLocation() + Out * 1200.0 - FVector(0.0, 0.0, 30.0);
	const FVector Look = (Front.GetLocation() - Eyes).GetSafeNormal();
	constexpr double Minutes = 20.0;

	// By day: curtains now and then, never a shadow; each a good while after the last at this house.
	Haskett->ApplyState(Day);
	TArray<FSeen> Seen = Watch(*Life, Eyes, Look, 1000.0, Minutes * 60.0);
	TestTrue(FString::Printf(TEXT("By day, now and then (%d in %.0f minutes)"), Seen.Num(), Minutes), Seen.Num() >= 4);
	TestTrue(TEXT("...rarely: a house at most every 30 s"), Seen.Num() <= FMath::CeilToInt(Minutes * 60.0 / WindowLifeRules::HouseRestMin) + 1);
	bool bCurtainsOnly = true;
	bool bRested = true;
	for (int32 Index = 0; Index < Seen.Num(); ++Index)
	{
		bCurtainsOnly &= Seen[Index].Kind == EWindowLifeKind::Curtain;
		bRested &= Index == 0 || Seen[Index].Time - Seen[Index - 1].Time >= WindowLifeRules::HouseRestMin - 0.01;
	}
	TestTrue(TEXT("...curtains only"), bCurtainsOnly);
	TestTrue(TEXT("...the house resting between"), bRested);

	// At dusk: shadows too, and the card shows them at the window.
	Haskett->ApplyState(Dusk);
	Seen = Watch(*Life, Eyes, Look, 3000.0, Minutes * 60.0);
	const int32 Shadows = Seen.FilterByPredicate([](const FSeen& Each) { return Each.Kind == EWindowLifeKind::Shadow; }).Num();
	TestTrue(FString::Printf(TEXT("At dusk, shadows cross the glass (%d of %d)"), Shadows, Seen.Num()), Shadows >= 1);
	if (Life->PlayAt(Haskett, 0, EWindowLifeKind::Shadow, 9000.0))
	{
		const AWindowLifeCard* Card = Life->GetCard();
		TestTrue(TEXT("The card plays at the window, just off the glass"), Card && Card->IsPlaying()
			&& FVector::Dist(Card->GetActorLocation(), Front.GetLocation()) < 2.0 && Card->Card->IsVisible());
		AWindowLifeCard* Playing = Life->GetCard();
		Playing->Advance(Playing->GetEvent().Seconds * 0.5f);
		TestTrue(TEXT("...plain halfway"), Playing->GetPose().Strength > 0.5f);
		Playing->Advance(Playing->GetEvent().Seconds);
		TestFalse(TEXT("...and hidden once it's over"), Playing->IsPlaying() || Playing->Card->IsVisible());
	}
	else
	{
		AddError(TEXT("A shadow couldn't play at the saltbox's window (no M_WindowLife?)"));
	}

	// Too near, too far, looking away: nothing.
	const FVector Near = Front.GetLocation() + Out * 300.0;
	const FVector Far = Front.GetLocation() + Out * 4000.0;
	int32 Wrong = Watch(*Life, Near, (Front.GetLocation() - Near).GetSafeNormal(), 10000.0, 300.0).Num();
	Wrong += Watch(*Life, Far, (Front.GetLocation() - Far).GetSafeNormal(), 11000.0, 300.0).Num();
	Wrong += Watch(*Life, Eyes, -Look, 12000.0, 300.0).Num();
	TestEqual(TEXT("Nothing from too near, too far, or looking away"), Wrong, 0);

	// Two houses in view: never two at once, with a pause between any two.
	AHouseLights* Mercer = SpawnHouse(World, PorchHouse, Front.GetLocation() + FVector(0.0, 900.0, -200.0), 0.f);
	if (TestNotNull(TEXT("The porch house's lights"), Mercer) && TestTrue(TEXT("...with its lamplit windows"), Mercer->NumLitWindows() > 0))
	{
		Mercer->ApplyState(Dusk);
		const FVector Between = Front.GetLocation() + FVector(1500.0, 450.0, -30.0);
		Seen = Watch(*Life, Between, (Front.GetLocation() + FVector(0.0, 450.0, 0.0) - Between).GetSafeNormal(), 20000.0, Minutes * 60.0);
		bool bApart = true;
		TSet<const AActor*> Places;
		for (int32 Index = 1; Index < Seen.Num(); ++Index)
		{
			bApart &= Seen[Index].Time - Seen[Index - 1].Time >= WindowLifeRules::GapMin;
		}
		for (const FSeen& Each : Seen)
		{
			Places.Add(Each.Place);
		}
		TestTrue(FString::Printf(TEXT("One at a time, a pause between (%d events)"), Seen.Num()), bApart && Seen.Num() >= 4);
		TestEqual(TEXT("...at both houses"), Places.Num(), 2);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWindowLifeShutterTest, "Looter.World.WindowLife.Shutter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWindowLifeShutterTest::RunTest(const FString& Parameters)
{
	// A shutter left open (before Main 3) swings shut now and then as the player looks over from across the street, past
	// where it would slam by itself; once shut it's never chosen again.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UWindowLifeSubsystem* Life = UWindowLifeSubsystem::Get(World);
	AWindowShutter* Shutter = World->SpawnActor<AWindowShutter>(FVector(0.0, 0.0, 250.0), FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("Window life"), Life) || !TestNotNull(TEXT("A shutter"), Shutter))
	{
		return false;
	}
	Shutter->DispatchBeginPlay();
	TestTrue(TEXT("It hangs open"), Shutter->GetState() == EWindowShutterState::Open);
	Life->SetRandomSeed(3);
	Life->SetSightChecks(false);
	Life->RefreshShutters();
	TestEqual(TEXT("The level's shutter is found"), Life->NumShutters(), 1);

	// Looking over at it from 20 m, by day.
	const FVector Eyes(2000.0, 300.0, 170.0);
	const FVector Look = (Shutter->GetActorLocation() - Eyes).GetSafeNormal();
	const TArray<FSeen> Seen = Watch(*Life, Eyes, Look, 1000.0, 600.0);
	if (TestEqual(TEXT("It swings shut once"), Seen.Num(), 1))
	{
		TestTrue(TEXT("...a shutter event"), Seen[0].Kind == EWindowLifeKind::Shutter && Seen[0].Place == Shutter);
	}
	TestTrue(TEXT("...on its way shut"), Shutter->GetState() == EWindowShutterState::Noticed || Shutter->GetState() == EWindowShutterState::Slamming);
	for (int32 Step = 0; Step < 60; ++Step)
	{
		Shutter->AdvanceSlam(1.f / 30.f);
	}
	TestTrue(TEXT("...and shut"), Shutter->IsShut());
	TestEqual(TEXT("Shut, it's never chosen again"), Watch(*Life, Eyes, Look, 2000.0, 300.0).Num(), 0);
	return true;
}

#endif

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Bestiary/BestiaryEntry.h"
#include "Creatures/DredgerHoseMan.h"
#include "Creatures/DredgerRifleman.h"
#include "Creatures/EncounterGroup.h"
#include "Creatures/EncounterSpawner.h"
#include "Creatures/EncounterSubsystem.h"
#include "Creatures/UnpaidCreature.h"
#include "Missions/MissionRunner.h"
#include "Story/CaptionSubsystem.h"
#include "Story/VaneBroadcast.h"
#include "Tests/EncounterTestWorld.h"
#include "Tests/MissionTestWorld.h"
#include "World/CompanyBanner.h"
#include "World/CompanyHoseCart.h"
#include "World/CompanyLoudspeaker.h"
#include "World/CompanySign.h"
#include "World/GangSkiffFlyBy.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/HitResult.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Script.h"

// Vane Consolidated's things in the Rest (lane MAIN23, Mains 2-3 of the Wardens story, 2026-11-01): the loudspeakers whose
// horns can be shot, the hose-cart that drinks the dead, the banner over Main Street, the signs and chalk, and Lucky Ned's
// skiff leaving. Their missions' flows are FerrymansPrice's (FerrymansPriceTests.cpp) and UnderNewManagement's.

namespace
{
	// A test level never readies its actors for play, and until one has, AActor::ProcessEvent drops every event bound to an
	// actor's own functions (the cart's and banner's OnTakePointDamage, the loudspeaker's OnDeath): the damage lands, its
	// handler never hears it. Each hit below lets them run, as they do in the game (BossTestWorld::Hurt).

	/** A shot on Part of Victim, as the bullet subsystem deals one (point damage with its hit). */
	void ShootPart(AActor* Victim, UPrimitiveComponent* Part, float Damage)
	{
		FEditorScriptExecutionGuard RunActorEvents;
		FHitResult Hit;
		Hit.Component = Part;
		Hit.Location = Part ? Part->GetComponentLocation() : FVector::ZeroVector;
		Hit.ImpactPoint = Hit.Location;
		UGameplayStatics::ApplyPointDamage(Victim, Damage, FVector::ForwardVector, Hit, nullptr, nullptr, UDamageType::StaticClass());
	}

	/** Damage as a shot deals it with no hit (MissionTestWorld::Hurt), its actor's own events let through. */
	void HurtActor(AActor* Victim, float Damage)
	{
		FEditorScriptExecutionGuard RunActorEvents;
		MissionTestWorld::Hurt(Victim, Damage, nullptr);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCompanyLoudspeakerCutOffTest, "Looter.World.CompanyLoudspeaker.CutOff",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCompanyLoudspeakerCutOffTest::RunTest(const FString& Parameters)
{
	// A horn shot out cuts Vane mid-word (MAINS_2_7.md: "cuts him at 'your bi-'"): four fifths in, a third of that word, a dash.
	const FString Dash = FString::Chr(static_cast<TCHAR>(0x2014));
	TestEqual(TEXT("\"I'm adding it to your bill.\" cut at \"your bi-\""), ACompanyLoudspeaker::CutOff(TEXT("I'm adding it to your bill.")),
		FString(TEXT("I'm adding it to your bi")) + Dash);
	const FString Long = ACompanyLoudspeaker::CutOff(TEXT("Your dead are being counted. Please don't make me count you."));
	TestTrue(TEXT("A long line keeps most of its words and ends inside one"), Long.EndsWith(Dash) && Long.Len() > 30 && Long.Len() < 60);
	TestEqual(TEXT("One short word keeps itself"), ACompanyLoudspeaker::CutOff(TEXT("I")), FString(TEXT("I")) + Dash);
	TestEqual(TEXT("Nothing said: only the dash"), ACompanyLoudspeaker::CutOff(TEXT("   ")), Dash);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCompanyLoudspeakerSilenceTest, "Looter.World.CompanyLoudspeaker.Silence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCompanyLoudspeakerSilenceTest::RunTest(const FString& Parameters)
{
	// Vane talks from it; enough damage on its horns silences it for the visit (a silenced horn says nothing); a voice only
	// can't be shot.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	ACompanyLoudspeaker* Horn = World->SpawnActor<ACompanyLoudspeaker>(FVector::ZeroVector, FRotator::ZeroRotator);
	ACompanyLoudspeaker* Voice = World->SpawnActor<ACompanyLoudspeaker>(FVector(2000.0, 0.0, 0.0), FRotator::ZeroRotator);
	if (!Horn || !Voice)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	Horn->Tags.Add(TEXT("Loudspeaker_Test"));
	Voice->bShowPost = false;
	Horn->DispatchBeginPlay();
	Voice->DispatchBeginPlay();
	TestTrue(TEXT("Tagged as every loudspeaker is, and an obstacle on the minimap"), Horn->ActorHasTag(ACompanyLoudspeaker::LoudspeakerTag)
		&& Horn->ActorHasTag(TEXT("Obstacle")));
	TestTrue(TEXT("Found by its tag"), ACompanyLoudspeaker::FindTagged(World, TEXT("Loudspeaker_Test")) == Horn);
	const TArray<FStoryLine> Words = { FStoryLine::Make(UVaneBroadcast::SpeakerName(), FText::FromString(TEXT("That cart is Company property."))) };
	TestTrue(TEXT("Vane talks from it"), Horn->Broadcast(Words));
	TestTrue(TEXT("...on the air"), UVaneBroadcast::IsSpeaking(World));
	TestTrue(TEXT("Its horns are a target"), Horn->CanBeDamaged() && Horn->HornsHit->IsCollisionEnabled());
	HurtActor(Horn, Horn->HornsHealth * 2.f);
	TestTrue(TEXT("Shot out: silenced"), Horn->IsSilenced());
	TestFalse(TEXT("...Vane cut off"), Horn->IsOnAir());
	TestFalse(TEXT("...a silenced horn says nothing"), Horn->Broadcast(Words));
	TestFalse(TEXT("...its horns gone"), Horn->HornLeft->IsVisible() || Horn->HornsHit->IsCollisionEnabled());
	TestFalse(TEXT("...and once is enough"), Horn->Silence(nullptr));
	TestFalse(TEXT("A voice only can't be shot out"), Voice->CanBeDamaged() || Voice->Silence(nullptr));
	TestTrue(TEXT("...but Vane talks from it"), Voice->Broadcast(Words));
	UVaneBroadcast::Stop(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCompanyPropsBestiaryTest, "Looter.World.CompanyProps.NotInBestiary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCompanyPropsBestiaryTest::RunTest(const FString& Parameters)
{
	// Only kinds with a bestiary page are met when shot or killed (the play check of 2026-11-01: a loudspeaker's horns opened
	// a bestiary entry): the Company's men have pages, its props that take shots don't.
	TestTrue(TEXT("A Dredger rifleman has a page"), UBestiaryEntry::HasPageFor(ADredgerRifleman::StaticClass()));
	TestTrue(TEXT("...and a hose-man"), UBestiaryEntry::HasPageFor(ADredgerHoseMan::StaticClass()));
	TestFalse(TEXT("A loudspeaker hasn't"), UBestiaryEntry::HasPageFor(ACompanyLoudspeaker::StaticClass()));
	TestFalse(TEXT("...nor a hose-cart"), UBestiaryEntry::HasPageFor(ACompanyHoseCart::StaticClass()));
	TestFalse(TEXT("...nor a banner"), UBestiaryEntry::HasPageFor(ACompanyBanner::StaticClass()));
	TestFalse(TEXT("...nor anything at all"), UBestiaryEntry::HasPageFor(AActor::StaticClass()) || UBestiaryEntry::HasPageFor(nullptr));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVaneBroadcastQuietTest, "Looter.World.CompanyLoudspeaker.WaitsForQuiet",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FVaneBroadcastQuietTest::RunTest(const FString& Parameters)
{
	// Story lines never overlap (the play check of 2026-11-01: the fly-by's radio broke in over Hob's road line): Vane waits,
	// silent, for Hob's line to end, then comes on; with nobody talking he's on at once; a line that never ends holds him only
	// so long, and even then his words queue behind it, never cutting it off.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UCaptionSubsystem* Captions = UCaptionSubsystem::Get(World);
	UVaneBroadcast* Radio = UVaneBroadcast::Get(World);
	if (!TestNotNull(TEXT("Captions"), Captions) || !TestNotNull(TEXT("Vane's radio"), Radio))
	{
		return false;
	}
	const FText Hob = FText::FromString(TEXT("Hob"));
	const TArray<FStoryLine> Vane = { FStoryLine::Make(UVaneBroadcast::SpeakerName(), FText::FromString(TEXT("Residents, remain indoors.")), 2.f) };
	constexpr float Frame = 1.f / 30.f;
	// Runs both clocks for Seconds; true if a line of Hob's was ever cut short. bVaneHeard: Vane's line came on screen.
	auto Run = [&](float Seconds, bool& bVaneHeard)
	{
		bool bOverlap = false;
		for (float Clock = 0.f; Clock < Seconds; Clock += Frame)
		{
			Radio->Advance(Frame);
			Captions->Update(Frame);
			const FCaptionEntry* Current = Captions->GetCurrent();
			if (!Current)
			{
				continue;
			}
			const bool bHob = Current->Line.Speaker.EqualTo(Hob);
			bOverlap |= bHob && Current->bCut;
			bVaneHeard |= Current->Line.Speaker.EqualTo(UVaneBroadcast::SpeakerName());
		}
		return bOverlap;
	};

	const int32 Road = Captions->Play({ FStoryLine::Make(Hob, FText::FromString(TEXT("Company men on the road. Mind them.")), 3.f) }, ECaptionPlay::Queue);
	UVaneBroadcast::Say(World, Vane);
	TestTrue(TEXT("Hob talking: Vane is on the air but waits, silent"), UVaneBroadcast::IsSpeaking(World) && Radio->IsWaitingForQuiet());
	bool bHeard = false;
	bool bCut = Run(0.5f, bHeard);
	TestTrue(TEXT("...Hob still talking, Vane not heard"), Captions->IsPlaying(Road) && !bHeard && Radio->IsWaitingForQuiet());
	bCut |= Run(12.f, bHeard);
	TestFalse(TEXT("Hob's line is never cut short"), bCut);
	TestTrue(TEXT("...and Vane comes on after it, and ends"), bHeard && !Radio->IsOnAir());

	UVaneBroadcast::Say(World, Vane);
	TestFalse(TEXT("Nobody talking: on at once"), Radio->IsWaitingForQuiet());
	UVaneBroadcast::Stop(World);

	Captions->Play({ FStoryLine::Make(Hob, FText::FromString(TEXT("A very long remark.")), 6.f) }, ECaptionPlay::Queue);
	const int32 Long = Captions->Play({ FStoryLine::Make(Hob, FText::FromString(TEXT("And another, longer.")), 6.f) }, ECaptionPlay::Queue);
	UVaneBroadcast::Say(World, Vane);
	bool bLongHeard = false;
	const bool bLongCut = Run(UVaneBroadcast::QuietWaitSeconds + 1.f, bLongHeard);
	TestTrue(TEXT("A long talk holds him only so long (the crackle comes)"), Radio->IsOnAir() && !Radio->IsWaitingForQuiet());
	TestTrue(TEXT("...and his words queue behind it: nothing cut, nothing dropped"), !bLongCut && (bLongHeard || Captions->IsPlaying(Long)));
	UVaneBroadcast::Stop(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCompanyHoseCartTest, "Looter.World.HoseCart.Stop",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCompanyHoseCartTest::RunTest(const FString& Parameters)
{
	// A cart with no story pumps and can be stopped. Shots elsewhere do nothing to its tank; enough on the tank burst it:
	// stopped, its spill out, and its crew's next wave brought at the spill's spots. A shut valve stops another the same way.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	ACharacter* Player = EncounterTestWorld::SpawnPlayer(World, FVector(-600.0, 0.0, 0.0));
	AEncounterSpawner* Crew = EncounterTestWorld::SpawnSpawner(World, FVector(1000.0, 0.0, 0.0), [](AEncounterSpawner& Setup)
	{
		FEncounterGroup Spill = EncounterTestWorld::MakeGroup(AUnpaidCreature::StaticClass(), 2);
		Spill.FirstWave = 2;
		Spill.LastWave = 2;
		FEncounterGroup Men = EncounterTestWorld::MakeGroup(AUnpaidCreature::StaticClass(), 1);
		Men.FirstWave = 1;
		Men.LastWave = 1;
		Setup.SpawnerId = TEXT("TestCrew");
		Setup.Groups = { Men, Spill };
		Setup.NumWaves = 2;
		Setup.WaveInterval = 0.f;
		Setup.bWaitForClear = false;
		Setup.bSpawnOnApproach = false;
	});
	ACompanyHoseCart* Cart = World->SpawnActor<ACompanyHoseCart>(FVector::ZeroVector, FRotator::ZeroRotator);
	ACompanyHoseCart* Other = World->SpawnActor<ACompanyHoseCart>(FVector(0.0, 1500.0, 0.0), FRotator::ZeroRotator);
	if (!Player || !Crew || !Cart || !Other)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	Cart->CrewSpawnerId = TEXT("TestCrew");
	Cart->SpillSpots = { FVector(200.0, 0.0, 0.0), FVector(-200.0, 0.0, 0.0) };
	Cart->DispatchBeginPlay();
	Other->DispatchBeginPlay();
	TestTrue(TEXT("Tagged as a hose-cart and an obstacle (cover)"), Cart->ActorHasTag(ACompanyHoseCart::HoseCartTag) && Cart->ActorHasTag(TEXT("Obstacle")));
	TestTrue(TEXT("No story: it pumps, and can be stopped"), Cart->GetCartState() == EHoseCartState::Pumping && Cart->IsLive());

	ShootPart(Cart, Cart->Bed, 500.f);
	TestTrue(TEXT("A shot on its bed doesn't touch the tank"), Cart->GetTankDamage() == 0.f && Cart->GetCartState() == EHoseCartState::Pumping);
	ShootPart(Cart, Cart->TankHit, Cart->TankHealth * 0.4f);
	TestTrue(TEXT("A shot on the tank holes it, still pumping"), Cart->GetTankDamage() > 0.f && Cart->GetCartState() == EHoseCartState::Pumping);
	ShootPart(Cart, Cart->TankHit, Cart->TankHealth);
	TestTrue(TEXT("Enough on the tank: it bursts, stopped"), Cart->GetCartState() == EHoseCartState::Stopped && Cart->WasBurst());
	TestTrue(TEXT("...its spill out"), Cart->Spill->IsVisible());
	TestEqual(TEXT("...the crew came first (it was stopped before they did), then the spill's wave"), Crew->GetWavesStarted(), 2);
	if (TestEqual(TEXT("...at the spill's spots"), Crew->SpawnPoints.Num(), 2))
	{
		TestEqual(TEXT("...the first of them by the cart, as the spawner keeps its spots (relative to it)"), Crew->SpawnPoints[0].X, -800.0);
	}
	TestFalse(TEXT("Nothing more to stop"), Cart->StopPumping(nullptr, false));
	TestTrue(TEXT("...no valve to turn"), !Cart->IsLive());

	// The other: its valve shut (the hold's end), no crew of its own.
	TestTrue(TEXT("The other cart's valve shut"), Other->StopPumping(nullptr, /*bBurst*/ false));
	TestTrue(TEXT("...stopped, not burst"), Other->GetCartState() == EHoseCartState::Stopped && !Other->WasBurst());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCompanyHoseCartStoryTest, "Looter.World.HoseCart.Story",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCompanyHoseCartStoryTest::RunTest(const FString& Parameters)
{
	// Its story: away before it's shown, pumping but not stoppable before its step, stoppable in it, stopped after.
	FCampaignRecord Campaign;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	AActor* Listener = MissionTestWorld::SpawnMarker(World, FVector::ZeroVector);
	ACompanyHoseCart* Cart = World->SpawnActor<ACompanyHoseCart>(FVector(400.0, 0.0, 0.0), FRotator::ZeroRotator);
	if (!Runner || !Listener || !Cart)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	const FName Main2(TEXT("Main2"));
	Cart->ShownWhen.DuringMission = Main2;
	Cart->PumpingWhen.DuringMission = Main2;
	Cart->PumpingWhen.FromStep = 1;
	Cart->PumpingWhen.BeforeStep = 2;
	FStoryCondition Past;
	Past.DuringMission = Main2;
	Past.FromStep = 2;
	Cart->StoppedWhen = { Past };
	Campaign.ActiveMission = TEXT("Main1");
	Runner->BeginForTesting({}, Campaign, Listener, TEXT("TestValley"));
	Cart->DispatchBeginPlay();
	TestTrue(TEXT("Before Main 2: away (hidden)"), Cart->GetCartState() == EHoseCartState::Away && Cart->IsHidden());
	Campaign.Complete(TEXT("Main1"));
	Campaign.ActiveMission = Main2;
	Campaign.ActiveMissionStep = 0;
	Cart->RefreshStory();
	TestTrue(TEXT("Main 2's way out: pumping, not stoppable"), Cart->GetCartState() == EHoseCartState::Pumping && !Cart->IsLive()
		&& !Cart->IsHidden());
	ShootPart(Cart, Cart->TankHit, Cart->TankHealth * 2.f);
	TestTrue(TEXT("...a tank shot from the approach holds"), Cart->GetCartState() == EHoseCartState::Pumping);
	Campaign.ActiveMissionStep = 1;
	Cart->RefreshStory();
	TestTrue(TEXT("Its step: stoppable"), Cart->IsLive());
	ShootPart(Cart, Cart->TankHit, 1.f);
	TestTrue(TEXT("...the holed tank bursts at the next shot"), Cart->GetCartState() == EHoseCartState::Stopped && Cart->WasBurst());

	ACompanyHoseCart* Later = World->SpawnActor<ACompanyHoseCart>(FVector(-400.0, 0.0, 0.0), FRotator::ZeroRotator);
	if (TestNotNull(TEXT("A level loaded past the cart"), Later))
	{
		Later->ShownWhen = Cart->ShownWhen;
		Later->PumpingWhen = Cart->PumpingWhen;
		Later->StoppedWhen = Cart->StoppedWhen;
		Campaign.ActiveMissionStep = 2;
		Later->DispatchBeginPlay();
		TestTrue(TEXT("...finds it stopped, spilled, from the start"), Later->GetCartState() == EHoseCartState::Stopped && Later->Spill->IsVisible());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCompanyBannerTest, "Looter.World.CompanyBanner.Ropes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCompanyBannerTest::RunTest(const FString& Parameters)
{
	// A shot on a rope cuts it, its end sags; both cut, it falls flat into the street and its flag is kept; a cut rope or a
	// fallen banner takes no more.
	FCampaignRecord Campaign;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	AActor* Listener = MissionTestWorld::SpawnMarker(World, FVector::ZeroVector);
	ACompanyBanner* Banner = World->SpawnActor<ACompanyBanner>(FVector(0.0, 0.0, 500.0), FRotator::ZeroRotator);
	if (!Runner || !Listener || !Banner)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	Banner->Text = FText::FromString(TEXT("UNDER NEW MANAGEMENT"));
	Banner->DownFlag = TEXT("Test.BannerDown");
	Runner->BeginForTesting({}, Campaign, Listener, TEXT("TestValley"));
	Banner->DispatchBeginPlay();
	TestTrue(TEXT("Strung up, both ropes whole"), !Banner->IsHidden() && Banner->NumCut() == 0 && !Banner->IsDown());
	TestTrue(TEXT("...its words on both faces"), Banner->WordsFront->Text.ToString() == TEXT("UNDER NEW MANAGEMENT")
		&& Banner->WordsBack->Text.ToString() == TEXT("UNDER NEW MANAGEMENT"));
	const double RestZ = Banner->ClothFrame->GetRelativeLocation().Z;
	ShootPart(Banner, Banner->RopeLeftHit, 10.f);
	TestEqual(TEXT("A shot on the left rope cuts it"), Banner->NumCut(), 1);
	Banner->Advance(2.f);
	TestTrue(TEXT("...its end sags, the cloth still up"), !Banner->IsDown() && Banner->ClothFrame->GetRelativeLocation().Z < RestZ);
	TestFalse(TEXT("...a cut rope takes no more"), Banner->CutRope(0, nullptr));
	ShootPart(Banner, Banner->RopeRightHit, 10.f);
	Banner->Advance(2.f);
	TestTrue(TEXT("Both cut: it falls into the street"), Banner->IsDown() && Banner->NumCut() == 2);
	TestTrue(TEXT("...lying on it"), FMath::IsNearlyEqual(Banner->ClothFrame->GetRelativeLocation().Z, -static_cast<double>(Banner->StreetBelow), 20.0));
	TestTrue(TEXT("...the story keeps it down"), Campaign.HasFlag(TEXT("Test.BannerDown")));
	TestFalse(TEXT("...nothing more to cut"), Banner->CutRope(1, nullptr));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCompanySignTest, "Looter.World.CompanySign.Look",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCompanySignTest::RunTest(const FString& Parameters)
{
	// A board stands on its posts with its words on its face; chalk is the words alone, slanted.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	ACompanySign* Board = World->SpawnActor<ACompanySign>(FVector::ZeroVector, FRotator::ZeroRotator);
	ACompanySign* Chalk = World->SpawnActor<ACompanySign>(FVector(500.0, 0.0, 0.0), FRotator::ZeroRotator);
	if (!Board || !Chalk)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	Board->Text = FText::FromString(TEXT("VANE CONSOLIDATED"));
	Chalk->Text = FText::FromString(TEXT("ASSESSED"));
	Chalk->Look = ECompanySignLook::Chalk;
	Chalk->Slant = 6.f;
	// The barrier across the gate: a long, thin board (build_area_main3 BARRIER_SIZE), never the signboard model.
	ACompanySign* Barrier = World->SpawnActor<ACompanySign>(FVector(-500.0, 0.0, 0.0), FRotator::ZeroRotator);
	if (!Barrier)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	Barrier->BoardSize = FVector2D(900.0, 14.0);
	Board->DispatchBeginPlay();
	Chalk->DispatchBeginPlay();
	Barrier->DispatchBeginPlay();
	// A signboard-sized board is the Tripo model once it's imported (its own posts: the stand-in's are hidden), else the
	// cube on the two cylinder posts (lanes/TRIPO_M23.md).
	auto MeshPath = [](const ACompanySign* Sign)
	{
		const UStaticMesh* Mesh = Sign->Board->GetStaticMesh();
		return Mesh ? Mesh->GetPathName() : FString();
	};
	const bool bModel = MeshPath(Board) == ACompanySign::BoardModelPath;
	TestTrue(TEXT("A board: board, posts and words"), Board->Board->IsVisible() && Board->PostLeft->IsVisible() != bModel
		&& Board->PostRight->IsVisible() != bModel && Board->Words->Text.ToString() == TEXT("VANE CONSOLIDATED"));
	TestTrue(TEXT("The barrier: a pole (the cube) on the stand-in's posts, never the signboard model"),
		MeshPath(Barrier) != ACompanySign::BoardModelPath && Barrier->Board->IsVisible() && Barrier->PostLeft->IsVisible()
		&& Barrier->PostRight->IsVisible());
	TestTrue(TEXT("...its words over the board's middle"), FMath::IsNearlyEqual(Board->Words->GetRelativeLocation().Z,
		Board->BoardLift + Board->BoardSize.Y * 0.5, 1.0));
	TestTrue(TEXT("Chalk: the words alone, slanted"), !Chalk->Board->IsVisible() && !Chalk->PostLeft->IsVisible()
		&& FMath::IsNearlyEqual(Chalk->Words->GetRelativeRotation().Roll, 6.0, 0.1));
	TestTrue(TEXT("No story: always there"), Board->ShouldShow() && !Board->IsHidden());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGangSkiffFlyByTest, "Looter.World.GangSkiffFlyBy.Flight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGangSkiffFlyByTest::RunTest(const FString& Parameters)
{
	// Hidden until it casts off; flies its course in its time; gone at the end, its flag kept so it never flies again.
	FCampaignRecord Campaign;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	AActor* Listener = MissionTestWorld::SpawnMarker(World, FVector::ZeroVector);
	AGangSkiffFlyBy* Skiff = World->SpawnActor<AGangSkiffFlyBy>(FVector::ZeroVector, FRotator::ZeroRotator);
	if (!Runner || !Listener || !Skiff)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	Skiff->Course = { FVector(0.0, 0.0, 5000.0), FVector(-6000.0, -2000.0, 6000.0), FVector(-15000.0, 0.0, 7000.0),
		FVector(-30000.0, -3000.0, 8000.0) };
	Skiff->Seconds = 20.f;
	Skiff->DoneFlag = TEXT("Test.NedLeft");
	Runner->BeginForTesting({}, Campaign, Listener, TEXT("TestValley"));
	Skiff->DispatchBeginPlay();
	TestTrue(TEXT("Hidden until it casts off"), Skiff->IsHidden() && !Skiff->IsFlying() && !Skiff->HasFlown());
	TestTrue(TEXT("Its course is the cold open's, built from its points"), Skiff->GetCourse().IsBuilt()
		&& FMath::IsNearlyEqual(Skiff->GetCourse().GetDuration(), 20.f, 0.5f));
	TestTrue(TEXT("Casts off"), Skiff->Begin() && Skiff->IsFlying() && !Skiff->IsHidden());
	const FVector Start = Skiff->GetActorLocation();
	Skiff->Advance(10.f);
	TestTrue(TEXT("...halfway, well along its way south"), FVector::Dist(Skiff->GetActorLocation(), Start) > 5000.0 && Skiff->IsFlying());
	Skiff->Advance(15.f);
	TestTrue(TEXT("Out of the valley: gone, flown"), !Skiff->IsFlying() && Skiff->HasFlown() && Skiff->IsHidden());
	TestTrue(TEXT("...its flag kept"), Campaign.HasFlag(TEXT("Test.NedLeft")));

	AGangSkiffFlyBy* Again = World->SpawnActor<AGangSkiffFlyBy>(FVector::ZeroVector, FRotator::ZeroRotator);
	if (TestNotNull(TEXT("A later visit"), Again))
	{
		Again->Course = Skiff->Course;
		Again->DoneFlag = Skiff->DoneFlag;
		Again->DispatchBeginPlay();
		TestTrue(TEXT("...finds it flown already"), Again->HasFlown());
	}
	return true;
}

#endif

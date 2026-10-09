#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Audio/LooterSoundCues.h"
#include "Session/CampaignRecord.h"
#include "Story/CaptionSubsystem.h"
#include "Story/SpeakerPointComponent.h"
#include "World/HouseLights.h"
#include "World/TownLifeDoor.h"
#include "World/TownLifeRules.h"
#include "World/TownLifeSubsystem.h"
#include "World/WindowShutter.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"

namespace
{
	const FName Pruitt(TEXT("Pruitt"));
	const FName CottageSouth(TEXT("CottageSouth"));

	/** Every town life cue, as the header names them. */
	TArray<FName> TownLifeCues()
	{
		using namespace LooterSoundCue::TownLife;
		return { Voices, Cough, MusicBox, DogFar, Latch, Creak, Hush, Workshop };
	}

	/** A townsfolk's door at Where, for Household, begun as play begins it. */
	ATownLifeDoor* SpawnDoor(UWorld* World, const FVector& Where, FName Household)
	{
		ATownLifeDoor* Door = World->SpawnActor<ATownLifeDoor>(Where, FRotator::ZeroRotator);
		if (Door)
		{
			Door->Household = Household;
			Door->DispatchBeginPlay();
		}
		return Door;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTownLifeHouseholdsTest, "Looter.World.TownLife.Households",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTownLifeHouseholdsTest::RunTest(const FString& Parameters)
{
	// Each lived-in house has sounds of its own, all of them town life cues; who lives where follows the story.
	const TArray<FName> Cues = TownLifeCues();
	TestTrue(TEXT("The households"), TownLifeRules::Households().Num() >= 5);
	for (const TownLifeRules::FHousehold& Household : TownLifeRules::Households())
	{
		TestTrue(FString::Printf(TEXT("%s makes some sound"), *Household.Id.ToString()), Household.Sounds.Num() >= 3);
		for (const FName& Sound : Household.Sounds)
		{
			TestTrue(FString::Printf(TEXT("%s: %s is a town life cue"), *Household.Id.ToString(), *Sound.ToString()), Cues.Contains(Sound));
		}
		TestTrue(TEXT("Voices in a human range"), Household.VoicePitch > 0.8f && Household.VoicePitch < 1.3f);
	}
	// Delia lives alone and Tilly works alone: no voices talking from either house; the music box is the child's house's.
	const TownLifeRules::FHousehold* Delia = TownLifeRules::FindHousehold(TEXT("Ransom"));
	const TownLifeRules::FHousehold* Tilly = TownLifeRules::FindHousehold(TEXT("Bright"));
	const TownLifeRules::FHousehold* Child = TownLifeRules::FindHousehold(CottageSouth);
	if (TestNotNull(TEXT("Delia's house"), Delia) && TestNotNull(TEXT("Tilly's shop"), Tilly) && TestNotNull(TEXT("The child's cottage"), Child))
	{
		TestFalse(TEXT("Nobody talking at Delia's"), Delia->Sounds.Contains(FName(LooterSoundCue::TownLife::Voices)));
		TestFalse(TEXT("No dog barking from Delia's or Tilly's"), Delia->bKeepsDog || Tilly->bKeepsDog);
		TestTrue(TEXT("...the dog is a town family's"), Child->bKeepsDog);
		TestTrue(TEXT("Tilly at her work"), Tilly->Sounds.Contains(FName(LooterSoundCue::TownLife::Workshop)));
		TestTrue(TEXT("A music box at the child's"), Child->Sounds.Contains(FName(LooterSoundCue::TownLife::MusicBox)));
	}
	TestNull(TEXT("No such household"), TownLifeRules::FindHousehold(TEXT("Nobody")));

	// A house's model names its household when its lights don't; the tutorial island's houses have nobody in them.
	TestEqual(TEXT("Delia's farmhouse"), TownLifeRules::HouseholdForMesh(TEXT("SM_Farmhouse_Ransom")), FName(TEXT("Ransom")));
	TestEqual(TEXT("Tilly's shop"), TownLifeRules::HouseholdForMesh(TEXT("SM_FalseFront_Undertaker")), FName(TEXT("Bright")));
	TestEqual(TEXT("Pruitt's store"), TownLifeRules::HouseholdForMesh(TEXT("SM_FalseFront_Store")), Pruitt);
	TestEqual(TEXT("A settler's cottage"), TownLifeRules::HouseholdForMesh(TEXT("SM_Cottage_Ransom")), FName(TEXT("Cottage")));
	TestTrue(TEXT("The empty saloon and sheriff's office, and Skyreach's houses: nobody"),
		TownLifeRules::HouseholdForMesh(TEXT("SM_FalseFront_Saloon")).IsNone() && TownLifeRules::HouseholdForMesh(TEXT("SM_FalseFront_Sheriff")).IsNone()
		&& TownLifeRules::HouseholdForMesh(TEXT("SM_Cottage")).IsNone() && TownLifeRules::HouseholdForMesh(TEXT("SM_Farmhouse")).IsNone());
	TestTrue(TEXT("A cough is a voice, a latch isn't"), TownLifeRules::IsVoice(LooterSoundCue::TownLife::Cough)
		&& !TownLifeRules::IsVoice(LooterSoundCue::TownLife::Latch));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTownLifeClockTest, "Looter.World.TownLife.Clock",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTownLifeClockTest::RunTest(const FString& Parameters)
{
	FRandomStream Random(20261009);
	FTownLifeClock Clock;

	// A first pass is fresh; lingering isn't; away long enough and it's fresh again.
	TestTrue(TEXT("Never near: fresh"), Clock.IsFreshVisit(Pruitt, 100.0));
	Clock.MarkNear(Pruitt, 100.0);
	TestFalse(TEXT("Lingering: not fresh"), Clock.IsFreshVisit(Pruitt, 101.0));
	TestTrue(TEXT("Back after a while: fresh"), Clock.IsFreshVisit(Pruitt, 100.0 + TownLifeRules::AwaySeconds + 1.0));

	// A fresh pass is heard often, a lingering look rarely.
	int32 Fresh = 0;
	int32 Again = 0;
	constexpr int32 Rolls = 4000;
	for (int32 Roll = 0; Roll < Rolls; ++Roll)
	{
		Fresh += Clock.WantsSound(Pruitt, 200.0, true, Random) ? 1 : 0;
		Again += Clock.WantsSound(Pruitt, 200.0, false, Random) ? 1 : 0;
	}
	TestTrue(TEXT("A fresh pass: about its chance"), FMath::Abs(static_cast<float>(Fresh) / Rolls - TownLifeRules::FirstChance) < 0.03f);
	TestTrue(TEXT("Lingering: about its chance"), FMath::Abs(static_cast<float>(Again) / Rolls - TownLifeRules::AgainChance) < 0.02f);

	// After a sound: nothing from anywhere for the pause, nothing from that house for its rest.
	Clock.Played(Pruitt, 300.0, Random);
	TestTrue(TEXT("The pause after a sound"), Clock.NextSound >= 300.0 + TownLifeRules::GapMin && Clock.NextSound <= 300.0 + TownLifeRules::GapMax);
	bool bQuiet = true;
	for (int32 Roll = 0; Roll < 200; ++Roll)
	{
		bQuiet &= !Clock.WantsSound(CottageSouth, 300.0 + TownLifeRules::GapMin - 0.5, true, Random);
		bQuiet &= !Clock.WantsSound(Pruitt, 300.0 + TownLifeRules::HouseRestMin - 0.5, true, Random);
	}
	TestTrue(TEXT("No house during the pause, nor that house during its rest"), bQuiet);
	bool bHeardOther = false;
	for (int32 Roll = 0; Roll < 50 && !bHeardOther; ++Roll)
	{
		bHeardOther = Clock.WantsSound(CottageSouth, 300.0 + TownLifeRules::GapMax + 0.5, true, Random);
	}
	TestTrue(TEXT("Another house after the pause"), bHeardOther);

	// A house never makes the same sound twice running.
	const TownLifeRules::FHousehold* Child = TownLifeRules::FindHousehold(CottageSouth);
	if (TestNotNull(TEXT("The child's cottage"), Child))
	{
		FName Last;
		bool bNoRepeat = true;
		TSet<FName> Heard;
		for (int32 Pick = 0; Pick < 500; ++Pick)
		{
			const FName Sound = Clock.PickSound(*Child, Random);
			bNoRepeat &= Sound != Last;
			Last = Sound;
			Heard.Add(Sound);
		}
		TSet<FName> Own;
		for (const FName& Sound : Child->Sounds)
		{
			Own.Add(Sound);
		}
		TestTrue(TEXT("Never the same twice running"), bNoRepeat);
		TestEqual(TEXT("...and all of its sounds come"), Heard.Num(), Own.Num());
	}

	// The dog rests a good while after it barks; mutters pause between them.
	Clock.DogBarked(400.0, Random);
	TestFalse(TEXT("The dog rests"), Clock.WantsDog(400.0 + TownLifeRules::DogRestMin - 1.0, Random));
	Clock.Muttered(500.0, Random);
	TestFalse(TEXT("No mutter right after one"), Clock.MayMutter(500.0 + TownLifeRules::MutterGapMin - 1.0));
	TestTrue(TEXT("...and one may come after the pause"), Clock.MayMutter(500.0 + TownLifeRules::MutterGapMax + 0.1));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTownLifeMuttersTest, "Looter.World.TownLife.Mutters",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTownLifeMuttersTest::RunTest(const FString& Parameters)
{
	// Each townsfolk's door mutters lines for where the story is: afraid of the corpse, then the gate won (Main 3), the
	// bell rung (Main 4), Abel at rest (Main 6).
	int32 Total = 0;
	for (const FName Household : { Pruitt, FName(TEXT("CottageNorth")), CottageSouth })
	{
		const TArray<FSpeakerTopic> Topics = TownLifeRules::MutterTopicsFor(Household);
		if (!TestEqual(FString::Printf(TEXT("%s: four parts of the story"), *Household.ToString()), Topics.Num(), 4))
		{
			continue;
		}
		TestTrue(TEXT("Latest first: after Main 6"), Topics[0].When.AfterMissions.Contains(FName(TEXT("Main6"))));
		TestTrue(TEXT("...then after Main 4"), Topics[1].When.AfterMissions.Contains(FName(TEXT("Main4"))));
		TestTrue(TEXT("...then after Main 3"), Topics[2].When.AfterMissions.Contains(FName(TEXT("Main3"))));
		TestTrue(TEXT("...and the town afraid, always"), Topics[3].When.IsEmpty());
		for (const FSpeakerTopic& Topic : Topics)
		{
			for (const FStoryLine& Line : Topic.Lines)
			{
				++Total;
				TestFalse(TEXT("Every mutter names who it is (a woman, a child...)"), Line.Speaker.IsEmpty());
				TestTrue(FString::Printf(TEXT("One line, short: \"%s\""), *Line.Text.ToString()), Line.Text.ToString().Len() <= 90);
			}
		}

		// Which applies as the story goes on.
		auto SaidNow = [&Topics](const FCampaignRecord& Campaign)
		{
			for (int32 Index = 0; Index < Topics.Num(); ++Index)
			{
				if (Topics[Index].When.IsMet(Campaign))
				{
					return Index;
				}
			}
			return int32(INDEX_NONE);
		};
		FCampaignRecord Campaign;
		TestEqual(TEXT("Before the gate: afraid"), SaidNow(Campaign), 3);
		Campaign.Complete(TEXT("Main3"));
		TestEqual(TEXT("After Main 3: the gate won"), SaidNow(Campaign), 2);
		Campaign.Complete(TEXT("Main4"));
		TestEqual(TEXT("After Main 4: the bell"), SaidNow(Campaign), 1);
		Campaign.Complete(TEXT("Main5"));
		Campaign.Complete(TEXT("Main6"));
		TestEqual(TEXT("After Main 6: Abel at rest"), SaidNow(Campaign), 0);
	}
	TestTrue(TEXT("About a dozen lines or more"), Total >= 12);
	TestTrue(TEXT("Nobody mutters from a house with no lines (Delia's, Tilly's: their own speaker points)"),
		TownLifeRules::MutterTopicsFor(TEXT("Ransom")).IsEmpty() && TownLifeRules::MutterTopicsFor(TEXT("Bright")).IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTownLifeDoorTest, "Looter.World.TownLife.Door",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTownLifeDoorTest::RunTest(const FString& Parameters)
{
	// Pruitt's door: nobody to talk to, but they mutter as the player passes, one line at a time into silence, each once,
	// and the house is heard through it now and then; a shutter on the store joins its household.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UTownLifeSubsystem* TownLife = UTownLifeSubsystem::Get(World);
	UCaptionSubsystem* Captions = UCaptionSubsystem::Get(World);
	if (!TestNotNull(TEXT("The town's life"), TownLife) || !TestNotNull(TEXT("Captions"), Captions))
	{
		return false;
	}
	TownLife->SetRandomSeed(20261009);
	ATownLifeDoor* Door = SpawnDoor(World, FVector(0.0, 0.0, 150.0), Pruitt);
	if (!TestNotNull(TEXT("Pruitt's door"), Door))
	{
		return false;
	}
	USpeakerPointComponent* Point = Door->SpeakerPoint;
	TestEqual(TEXT("It joined as a source"), TownLife->NumSources(), 1);
	TestEqual(TEXT("...and as a door that mutters"), TownLife->NumMutterPoints(), 1);
	TestEqual(TEXT("...of the Pruitts"), TownLife->GetHouseholdOf(Door), Pruitt);
	TestFalse(TEXT("Nobody to talk to"), Point->CanTalk());

	// Muttered into silence, a line at a time, each once, with a rest between.
	if (TestTrue(TEXT("The first mutter"), Point->Mutter(0.0)))
	{
		const FCaptionEntry* Shown = Captions->GetCurrent();
		TestTrue(TEXT("...as a caption"), Shown != nullptr);
		TestEqual(TEXT("...its first line"), Point->GetLastMutter().ToString(), FString(TEXT("Bar the door, Hollis. It's coming up the street.")));
		if (Shown)
		{
			TestEqual(TEXT("...naming who"), Shown->Line.Speaker.ToString(), FString(TEXT("A woman, inside")));
		}
	}
	Captions->Clear();
	Captions->Update(1.f);
	TestFalse(TEXT("Not again before its rest"), Point->Mutter(10.0));
	Captions->Play(TArray<FStoryLine>{ FStoryLine::Make(FText::FromString(TEXT("Hob")), FText::FromString(TEXT("Morning, sunshine."))) },
		ECaptionPlay::Queue);
	TestFalse(TEXT("Never over somebody else's line"), Point->Mutter(Point->MutterRest + 1.0));
	Captions->Clear();
	Captions->Update(1.f);
	TestTrue(TEXT("The second line, into silence after its rest"), Point->Mutter(Point->MutterRest + 1.0));
	TestEqual(TEXT("...the next one"), Point->GetLastMutter().ToString(), FString(TEXT("Lord. That's the Ransom child. Was the Ransom child.")));
	Captions->Clear();
	Captions->Update(1.f);
	TestFalse(TEXT("Both said this visit: no more"), Point->CanMutter(Point->MutterRest * 3.0));

	// A shutter on the store, naming nobody, joins the nearest household; it's only heard through once it's shut.
	AWindowShutter* Shutter = World->SpawnActor<AWindowShutter>(FVector(400.0, 300.0, 300.0), FRotator::ZeroRotator);
	if (TestNotNull(TEXT("A shutter"), Shutter))
	{
		Shutter->DispatchBeginPlay();
		TestEqual(TEXT("The shutter is the Pruitts'"), TownLife->GetHouseholdOf(Shutter), Pruitt);
	}
	// A cottage's lights naming their household.
	AHouseLights* Lights = World->SpawnActor<AHouseLights>(FVector(8000.0, 0.0, 200.0), FRotator::ZeroRotator);
	if (TestNotNull(TEXT("A cottage's lights"), Lights))
	{
		Lights->Household = CottageSouth;
		Lights->DispatchBeginPlay();
		TestEqual(TEXT("The lights are the child's house"), TownLife->GetHouseholdOf(Lights), CottageSouth);
	}

	// Lingering by the door for three minutes: the house is heard, but rarely (one house rests a while between sounds).
	int32 Sounds = 0;
	double LastNext = TownLife->GetClock().NextSound;
	const FVector Passing(300.0, 0.0, 150.0);
	constexpr double Lingering = 180.0;
	for (double Now = 1000.0; Now < 1000.0 + Lingering; Now += TownLifeRules::CheckSeconds)
	{
		TownLife->Check(Passing, Now);
		if (TownLife->GetClock().NextSound != LastNext)
		{
			++Sounds;
			LastNext = TownLife->GetClock().NextSound;
			TestEqual(TEXT("Heard from the Pruitts' house"), TownLife->GetLastHousehold(), Pruitt);
			TestTrue(TEXT("...one of their sounds"), TownLifeRules::FindHousehold(Pruitt)->Sounds.Contains(TownLife->GetLastSound()));
		}
	}
	TestTrue(TEXT("The house is heard while the player lingers"), Sounds >= 1);
	TestTrue(TEXT("...but rarely"), Sounds <= Lingering / TownLifeRules::HouseRestMin + 1.0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTownLifeDogTest, "Looter.World.TownLife.Dog",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTownLifeDogTest::RunTest(const FString& Parameters)
{
	// In town but away from the houses, a dog barks far off now and then, from a house well away, never from the
	// house the player stands at; a house that far is never heard itself.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UTownLifeSubsystem* TownLife = UTownLifeSubsystem::Get(World);
	if (!TestNotNull(TEXT("The town's life"), TownLife))
	{
		return false;
	}
	TownLife->SetRandomSeed(7);
	ATownLifeDoor* Door = SpawnDoor(World, FVector(3000.0, 0.0, 150.0), CottageSouth);
	if (!TestNotNull(TEXT("A cottage's door"), Door))
	{
		return false;
	}
	int32 Barks = 0;
	bool bOnlyTheDog = true;
	double LastDog = TownLife->GetClock().NextDog;
	for (double Now = 0.0; Now < 1200.0; Now += TownLifeRules::CheckSeconds)
	{
		TownLife->Check(FVector::ZeroVector, Now);
		if (TownLife->GetClock().NextDog != LastDog)
		{
			LastDog = TownLife->GetClock().NextDog;
			++Barks;
			TestTrue(TEXT("The dog from the far house"), TownLife->GetLastSoundAt().Equals(Door->GetActorLocation(), 1.0));
		}
		const FName Sound = TownLife->GetLastSound();
		bOnlyTheDog &= Sound.IsNone() || Sound == FName(LooterSoundCue::TownLife::DogFar);
	}
	TestTrue(TEXT("The dog barks now and then over twenty minutes"), Barks >= 2);
	TestTrue(TEXT("...resting between"), Barks <= 1200.0 / TownLifeRules::DogRestMin + 1.0);
	TestTrue(TEXT("A house 30 m off is never heard itself"), bOnlyTheDog);
	return true;
}

#endif

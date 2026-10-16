#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Missions/MissionDefinition.h"
#include "Missions/MissionEventObjectives.h"
#include "Missions/MissionRunner.h"
#include "Scenes/ColdOpenCast.h"
#include "Session/CampaignRecord.h"
#include "Story/SpeakerPoint.h"
#include "Story/SpeakerPointComponent.h"
#include "Story/StoryLine.h"
#include "Tests/MissionTestWorld.h"
#include "World/FarSignalLight.h"
#include "World/ReadableProp.h"
#include "World/WantedPoster.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Package.h"

// The seeds in Ransom's Rest (Docs/Story.md, "Seeds in Ransom's Rest"): a Ledger page that opens its town's page when read,
// a talk said once (Aldana's), the spark in the Deacon's hand, the second light that burns only at dusk, and the Seven on
// the Rangers' board crossed out face by face.

using namespace MissionTestWorld;

namespace
{
	FStoryLine SeedLine(const TCHAR* Words)
	{
		return FStoryLine::Make(FText::GetEmpty(), FText::FromString(Words), 2.f);
	}

	/** A test level whose story is running: the runner begun on one mission that waits on nothing these tests send. */
	struct FSeedLevel
	{
		FTestWorldWrapper Wrapper;
		UWorld* World = nullptr;
		UMissionRunner* Runner = nullptr;
		AActor* Player = nullptr;

		bool Build(FCampaignRecord& Campaign)
		{
			if (!Wrapper.CreateTestWorld(EWorldType::EditorPreview))
			{
				return false;
			}
			World = Wrapper.GetTestWorld();
			Runner = World->GetSubsystem<UMissionRunner>();
			Player = SpawnMarker(World, FVector::ZeroVector);
			if (!Runner || !Player)
			{
				return false;
			}
			UMissionDefinition* Waiting = NewMission(CreatePackage(nullptr), TEXT("TestSeeds"), EMissionKind::Side, EMissionStart::Automatic, NAME_None);
			AddObjective<UMissionEventObjective>(Waiting, 0)->Event = TEXT("Test.Seeds.Never");
			Runner->BeginForTesting({ Waiting }, Campaign, Player);
			Runner->Update(0.f);
			return true;
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSeedReadableFlagTest, "Looter.Seeds.ReadableFlag",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSeedReadableFlagTest::RunTest(const FString& Parameters)
{
	FCampaignRecord Campaign;
	FSeedLevel Level;
	if (!TestTrue(TEXT("Test level made"), Level.Build(Campaign)))
	{
		return false;
	}
	AReadableProp* Page = Level.World->SpawnActor<AReadableProp>(FVector(200.0, 0.0, 100.0), FRotator::ZeroRotator);
	AReadableProp* Plain = Level.World->SpawnActor<AReadableProp>(FVector(200.0, 300.0, 100.0), FRotator::ZeroRotator);
	if (!TestTrue(TEXT("Two readables placed"), Page && Plain))
	{
		return false;
	}
	const FName PageFlag(TEXT("Ledger.Page.TheNeedles"));
	Page->ReadableId = TEXT("LedgerPage_TheNeedles");
	Page->Title = FText::FromString(TEXT("A loose ledger page"));
	Page->Text = FText::FromString(TEXT("THE NEEDLES. Saint ... owed ..."));
	Page->SetsFlag = PageFlag;
	Plain->ReadableId = TEXT("TestPlain");
	Plain->Lines = { SeedLine(TEXT("Nothing here.")) };
	Page->DispatchBeginPlay();
	Plain->DispatchBeginPlay();

	TestFalse(TEXT("The town's page is shut before the leaf is read"), Campaign.HasFlag(PageFlag));
	TestTrue(TEXT("Read"), Page->Read(Level.Player));
	TestTrue(TEXT("...its flag set: the Ledger's town page opens"), Campaign.HasFlag(PageFlag) && Level.Runner->HasFlag(PageFlag));
	const int32 FlagsAfterFirst = Campaign.StoryFlags.Num();
	TestTrue(TEXT("Read again"), Page->Read(Level.Player));
	TestEqual(TEXT("...sets nothing new"), Campaign.StoryFlags.Num(), FlagsAfterFirst);
	TestTrue(TEXT("A readable with no flag reads"), Plain->Read(Level.Player));
	TestEqual(TEXT("...and sets none"), Campaign.StoryFlags.Num(), FlagsAfterFirst);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSeedTopicFlagTest, "Looter.Seeds.TopicFlag",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSeedTopicFlagTest::RunTest(const FString& Parameters)
{
	FCampaignRecord Campaign;
	FSeedLevel Level;
	if (!TestTrue(TEXT("Test level made"), Level.Build(Campaign)))
	{
		return false;
	}
	ASpeakerPoint* Door = Level.World->SpawnActor<ASpeakerPoint>(FVector(150.0, 0.0, 0.0), FRotator(0.0, 180.0, 0.0));
	if (!TestNotNull(TEXT("A vestry door"), Door))
	{
		return false;
	}
	// Aldana's way: the story of the first keeper once, above what he says the rest of the time.
	const FName Told(TEXT("Ada.Told"));
	FSpeakerTopic Once;
	Once.When.WithoutFlags = { Told };
	Once.Lines = { SeedLine(TEXT("She gave it up, the first keeper.")) };
	Once.SetsFlag = Told;
	FSpeakerTopic Usual;
	Usual.Lines = { SeedLine(TEXT("The bell's quiet tonight.")) };
	Door->SpeakerPoint->SpeakerName = FText::FromString(TEXT("Father Aldana"));
	Door->SpeakerPoint->Topics = { Once, Usual };
	Door->DispatchBeginPlay();

	int32 Topic = INDEX_NONE;
	Door->SpeakerPoint->GetLinesNow(&Topic);
	TestEqual(TEXT("Before it's told: the story"), Topic, 0);
	TestFalse(TEXT("...not told yet"), Campaign.HasFlag(Told));
	TestTrue(TEXT("Talked to"), Door->SpeakerPoint->Talk(Level.Player));
	TestTrue(TEXT("Saying it sets its flag"), Campaign.HasFlag(Told));
	Door->SpeakerPoint->GetLinesNow(&Topic);
	TestEqual(TEXT("...so the next talk is the usual"), Topic, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSeedSparkTest, "Looter.Seeds.Spark",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSeedSparkTest::RunTest(const FString& Parameters)
{
	// The curve: rest, a flare to three times in 0.15 s, down to 0.35 over 1.2 s, back to rest over 2 s.
	constexpr float Near = 0.01f;
	TestEqual(TEXT("At rest before"), AColdOpenCast::EmberFlareAt(-1.f), 1.f, Near);
	TestEqual(TEXT("At rest as it starts"), AColdOpenCast::EmberFlareAt(0.f), 1.f, Near);
	TestEqual(TEXT("Three times at its peak"), AColdOpenCast::EmberFlareAt(0.15f), 3.f, Near);
	TestEqual(TEXT("A third of rest once the shot's rung"), AColdOpenCast::EmberFlareAt(1.35f), 0.35f, Near);
	TestEqual(TEXT("Back at rest after"), AColdOpenCast::EmberFlareAt(3.35f), 1.f, Near);
	TestEqual(TEXT("...and stays"), AColdOpenCast::EmberFlareAt(10.f), 1.f, Near);
	TestEqual(TEXT("It lasts 3.35 s"), AColdOpenCast::GetFlareSeconds(), 3.35f, Near);
	float Highest = 0.f;
	float Lowest = 10.f;
	float Previous = AColdOpenCast::EmberFlareAt(0.f);
	bool bSmooth = true;
	for (float Seconds = 0.f; Seconds <= 4.f; Seconds += 1.f / 120.f)
	{
		const float Now = AColdOpenCast::EmberFlareAt(Seconds);
		Highest = FMath::Max(Highest, Now);
		Lowest = FMath::Min(Lowest, Now);
		// No jump bigger than the flare's own rise in one frame at 120 fps.
		bSmooth = bSmooth && FMath::Abs(Now - Previous) < 0.4f;
		Previous = Now;
	}
	TestTrue(TEXT("Never past three, never under 0.35"), Highest <= 3.f + Near && Lowest >= 0.35f - Near);
	TestTrue(TEXT("...and never jumps"), bSmooth);

	// On a cast: the Deacon's shot sparks it, the cast's own clock runs it.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	AColdOpenCast* Props = TestLevel.GetTestWorld()->SpawnActor<AColdOpenCast>();
	if (!TestNotNull(TEXT("A cast"), Props))
	{
		return false;
	}
	Props->Tick(1.f);
	TestEqual(TEXT("No spark before the shot"), Props->GetEmberFlare(), 1.f, Near);
	Props->FlareEmber();
	Props->Tick(0.15f);
	TestEqual(TEXT("Flared"), Props->GetEmberFlare(), 3.f, Near);
	Props->Tick(1.2f);
	TestEqual(TEXT("Dimmed"), Props->GetEmberFlare(), 0.35f, Near);
	Props->Tick(2.f);
	TestEqual(TEXT("At rest again"), Props->GetEmberFlare(), 1.f, Near);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSeedSteadyLightTest, "Looter.Seeds.SteadyLight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSeedSteadyLightTest::RunTest(const FString& Parameters)
{
	// The breath: 0.5 Hz, 8% either way.
	constexpr float Near = 0.005f;
	TestEqual(TEXT("At its middle as it starts"), AFarSignalLight::BreathAt(0.f, 0.5f, 0.08f), 1.f, Near);
	TestEqual(TEXT("Brightest a quarter way through"), AFarSignalLight::BreathAt(0.5f, 0.5f, 0.08f), 1.08f, Near);
	TestEqual(TEXT("Dimmest three quarters through"), AFarSignalLight::BreathAt(1.5f, 0.5f, 0.08f), 0.92f, Near);
	TestTrue(TEXT("No state asked: any state, or none"), AFarSignalLight::StateAllows(NAME_None, TEXT("Day")) && AFarSignalLight::StateAllows(NAME_None, NAME_None));
	TestTrue(TEXT("Dusk asked: at dusk, any case"), AFarSignalLight::StateAllows(TEXT("Dusk"), TEXT("dusk")));
	TestFalse(TEXT("...not by day"), AFarSignalLight::StateAllows(TEXT("Dusk"), TEXT("Day")));
	TestFalse(TEXT("...nor in a level without states"), AFarSignalLight::StateAllows(TEXT("Dusk"), NAME_None));

	// The second light: shown after the story's point (a flag here), and only at dusk.
	FCampaignRecord Campaign;
	FSeedLevel Level;
	if (!TestTrue(TEXT("Test level made"), Level.Build(Campaign)))
	{
		return false;
	}
	AFarSignalLight* Light = Level.World->SpawnActor<AFarSignalLight>(FVector(-900000.0, 0.0, 4000.0), FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("The second light"), Light))
	{
		return false;
	}
	const FName After(TEXT("Test.Seeds.AfterGravewind"));
	Light->ShownWhen.Reset();
	Light->ShownFlag = After;
	Light->ShownState = TEXT("Dusk");
	Light->bSteady = true;
	Light->DispatchBeginPlay();
	Light->ApplyLightingState(TEXT("Dusk"));
	TestFalse(TEXT("Dark at dusk before the story's point"), Light->IsShown());
	Level.Runner->SetFlag(After);
	TestTrue(TEXT("Burns at dusk after it (the story's change shows it)"), Light->IsShown());
	float Brightest = 0.f;
	float Darkest = 10.f;
	for (int32 Frame = 0; Frame < 60; ++Frame)
	{
		Light->Advance(1.f / 15.f);
		Brightest = FMath::Max(Brightest, Light->GetKey());
		Darkest = FMath::Min(Darkest, Light->GetKey());
	}
	TestTrue(TEXT("...steady: never out, only breathing"), Darkest > 0.9f && Brightest < 1.1f && Brightest - Darkest > 0.08f);
	Light->ApplyLightingState(TEXT("Day"));
	TestFalse(TEXT("Gone by day"), Light->IsShown());
	Light->ApplyLightingState(TEXT("Dusk"));
	TestTrue(TEXT("Back at the next dusk"), Light->IsShown());
	Campaign.ClearFlag(After);
	Light->RefreshStory();
	TestFalse(TEXT("The story's say still counts at dusk"), Light->IsShown());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSeedSevenPosterTest, "Looter.Seeds.SevenPoster",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSeedSevenPosterTest::RunTest(const FString& Parameters)
{
	FCampaignRecord Campaign;
	FSeedLevel Level;
	if (!TestTrue(TEXT("Test level made"), Level.Build(Campaign)))
	{
		return false;
	}
	AWantedPoster* Seven = Level.World->SpawnActor<AWantedPoster>(FVector(200.0, 0.0, 150.0), FRotator(0.0, 180.0, 0.0));
	if (!TestNotNull(TEXT("The Seven"), Seven))
	{
		return false;
	}
	Seven->SetVariant(EWantedPosterVariant::Seven);
	Seven->DispatchBeginPlay();

	TestTrue(TEXT("Tagged as the Seven, not as a wanted poster"), Seven->ActorHasTag(AWantedPoster::SevenTag) && !Seven->ActorHasTag(AWantedPoster::WantedTag));
	TestTrue(TEXT("Its own cell"), Seven->GetWholeCell() == Seven->Atlas.Seven && Seven->GetShownCell() == Seven->Atlas.Seven);
	TestEqual(TEXT("A face's place for each of the seven"), Seven->Atlas.SevenFaces.Num(), AWantedPoster::SevenCount);
	TestEqual(TEXT("A flag for each of the seven"), Seven->CrossedFlags.Num(), AWantedPoster::SevenCount);
	TestTrue(TEXT("The Deacon's first, by his ember"), Seven->CrossedFlags.Num() > 0 && Seven->CrossedFlags[0] == FName(TEXT("Ember.Paid.Deacon")));
	TestFalse(TEXT("Never torn"), Seven->CanTear() || Seven->Tear(Level.Player) || Seven->IsTorn());
	TestFalse(TEXT("Nothing to read"), Seven->CanRead() || Seven->Read(Level.Player));
	bool bAnyCrossed = false;
	for (int32 Face = 0; Face < AWantedPoster::SevenCount; ++Face)
	{
		bAnyCrossed = bAnyCrossed || Seven->IsFaceCrossShown(Face);
	}
	TestFalse(TEXT("No face crossed before an ember's paid"), bAnyCrossed);

	// Ira's ember paid (a later mission's flag): his face, and only his, crossed out as the story changes.
	Level.Runner->SetFlag(Seven->CrossedFlags[2]);
	TestTrue(TEXT("Ira crossed out"), Seven->IsFaceCrossedNow(2) && Seven->IsFaceCrossShown(2));
	TestFalse(TEXT("...the Deacon not"), Seven->IsFaceCrossShown(0));
	TestFalse(TEXT("...nor Tobias"), Seven->IsFaceCrossShown(6));
	TestFalse(TEXT("...and still never torn"), Seven->IsTorn());
	return true;
}

#endif

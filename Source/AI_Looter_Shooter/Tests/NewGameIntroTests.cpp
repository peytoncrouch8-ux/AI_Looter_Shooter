#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Missions/MissionRunner.h"
#include "Scenes/SceneSubsystem.h"
#include "Session/CampaignRecord.h"
#include "Story/NewGameIntro.h"
#include "Story/StoryLineSet.h"
#include "Tests/MissionTestWorld.h"
#include "UI/Story/IntroWidget.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Misc/PackageName.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Package.h"

namespace
{
	constexpr float Frame = 1.f / 30.f;
	/** Times in the script are tenths of a second: this much slack for float sums. */
	constexpr float TimeSlack = 0.01f;

	/** A console variable set for a test, and put back after it. */
	class FScopedCVar
	{
	public:
		FScopedCVar(const TCHAR* Name, int32 Value)
			: Variable(IConsoleManager::Get().FindConsoleVariable(Name))
		{
			if (Variable)
			{
				Previous = Variable->GetString();
				Variable->Set(Value, ECVF_SetByConsole);
			}
		}

		~FScopedCVar()
		{
			if (Variable)
			{
				Variable->Set(*Previous, ECVF_SetByConsole);
			}
		}

	private:
		IConsoleVariable* Variable = nullptr;
		FString Previous;
	};

	void TickScenes(USceneSubsystem& Scenes, float Seconds)
	{
		for (float Done = 0.f; Done < Seconds - KINDA_SMALL_NUMBER; Done += Frame)
		{
			Scenes.Tick(FMath::Min(Frame, Seconds - Done));
		}
	}

	/** The words of Text, split on anything but letters and apostrophes. */
	TArray<FString> WordsOf(const FString& Text)
	{
		TArray<FString> Words;
		FString Word;
		for (const TCHAR Char : Text)
		{
			if (FChar::IsAlpha(Char) || Char == TEXT('\''))
			{
				Word.AppendChar(Char);
			}
			else if (!Word.IsEmpty())
			{
				Words.Add(Word);
				Word.Reset();
			}
		}
		if (!Word.IsEmpty())
		{
			Words.Add(Word);
		}
		return Words;
	}

	/** One part's captions: in order, apart, each held a reader's time, all before LastAt. */
	void CheckCaptions(FAutomationTestBase& Test, const TCHAR* Part, const TArray<FIntroCaption>& Captions, float LastAt, TArray<int32>& UsedLines)
	{
		const TArray<FStoryLine>& Lines = NewGameIntro::ScriptLines();
		float FreeAt = 0.f;
		for (const FIntroCaption& Caption : Captions)
		{
			const FString Where = FString::Printf(TEXT("%s, the caption at %.1f s"), Part, Caption.At);
			Test.TestTrue(Where + TEXT(" comes after the one before has gone"), Caption.At >= FreeAt - TimeSlack);
			FreeAt = Caption.At + Caption.Seconds;
			Test.TestTrue(Where + TEXT(" is gone before the white rises"), FreeAt <= LastAt + TimeSlack);
			if (Caption.Card != EIntroCard::None)
			{
				Test.TestTrue(Where + TEXT(": a card stays at least 3 s"), Caption.Seconds >= 3.f);
				Test.TestTrue(Where + TEXT(": a card is a card, not a line"), Caption.Line == INDEX_NONE);
				continue;
			}
			if (!Test.TestTrue(Where + TEXT(" names a written line"), Lines.IsValidIndex(Caption.Line)))
			{
				continue;
			}
			UsedLines.Add(Caption.Line);
			Test.TestTrue(Where + FString::Printf(TEXT(" is up long enough to read (%.2f s for %.2f s)"), Caption.Seconds,
				Lines[Caption.Line].GetSeconds()), Caption.Seconds >= Lines[Caption.Line].GetSeconds() - TimeSlack);
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIntroScriptTest, "Looter.Intro.Script",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FIntroScriptTest::RunTest(const FString& Parameters)
{
	using namespace NewGameIntro;

	// The words: narration (no speaker, shown with Subtitles off), none of Ellis's family's (the same for every hero).
	const TArray<FStoryLine>& Lines = ScriptLines();
	TestEqual(TEXT("Eleven lines"), Lines.Num(), 11);
	const TArray<FString> Family = { TEXT("Pa"), TEXT("Pa's"), TEXT("El"), TEXT("Ellis"), TEXT("Grandma"), TEXT("grandmother"),
		TEXT("Abel"), TEXT("Delia"), TEXT("kid") };
	for (const FStoryLine& Line : Lines)
	{
		const FString Words = Line.Text.ToString();
		TestFalse(FString::Printf(TEXT("\"%s\" has words"), *Words), Words.IsEmpty());
		TestTrue(FString::Printf(TEXT("\"%s\" is narration (no speaker)"), *Words), Line.Speaker.IsEmpty());
		for (const FString& Word : WordsOf(Words))
		{
			TestFalse(FString::Printf(TEXT("\"%s\" names no family (%s)"), *Words, *Word), Family.ContainsByPredicate(
				[&Word](const FString& Name) { return Name.Equals(Word, ESearchCase::IgnoreCase); }));
		}
	}

	// The captions: each line once, each held long enough, one thing at a time, none under the white or a cut's dip.
	TArray<int32> Used;
	CheckCaptions(*this, TEXT("Teropa"), PlanetCaptions(), PlanetWhiteAt, Used);
	CheckCaptions(*this, TEXT("Ransom's Rest"), RestCaptions(), RestWhiteAt, Used);
	for (int32 Index = 0; Index < Lines.Num(); ++Index)
	{
		TestEqual(FString::Printf(TEXT("Line %d is shown once"), Index), Used.FilterByPredicate([Index](int32 Line) { return Line == Index; }).Num(), 1);
	}
	TestTrue(TEXT("The hero's name card is in the Rest's part"), RestCaptions().ContainsByPredicate([](const FIntroCaption& Caption)
		{ return Caption.Card == EIntroCard::Hero; }));
	for (const EIntroCard Card : { EIntroCard::Teropa, EIntroCard::Reaches, EIntroCard::Rest })
	{
		TestFalse(TEXT("Each place card has its name"), CardTitle(Card).IsEmpty());
	}

	// The length the user asked for (60-90 s), either way it plays.
	const float Total = PlanetSeconds + RestSeconds;
	TestTrue(FString::Printf(TEXT("%.1f s in all: 60-90 s"), Total), Total >= ShortestTotal && Total <= LongestTotal);
	TestEqual(TEXT("Westward in the planet's place keeps the length"), RestSceneSeconds(true), Total);
	TestTrue(TEXT("The white rises within each part"), PlanetWhiteAt + WhiteSeconds <= PlanetSeconds + TimeSlack && RestWhiteAt + WhiteSeconds <= RestSeconds + TimeSlack);

	// The shots: back to back from 0 to the end, Westward first when asked; no caption inside a cut's dip.
	for (const bool bWestward : { false, true })
	{
		const TArray<FIntroMove> Moves = RestMoves(bWestward);
		if (!TestTrue(TEXT("Shots"), Moves.Num() >= 4))
		{
			continue;
		}
		TestEqual(TEXT("Westward opens only when asked"), Moves[0].Name == FName(TEXT("Westward")), bWestward);
		float At = 0.f;
		for (const FIntroMove& Move : Moves)
		{
			TestTrue(FString::Printf(TEXT("%s starts as the shot before ends"), *Move.Name.ToString()), FMath::IsNearlyEqual(Move.Start, At, TimeSlack));
			TestTrue(FString::Printf(TEXT("%s moves"), *Move.Name.ToString()), Move.Duration > 0.f && !Move.FromEye.Equals(Move.ToEye, 1.0));
			TestTrue(FString::Printf(TEXT("%s's lens is a lens"), *Move.Name.ToString()), Move.FromFov > 30.f && Move.ToFov > 30.f
				&& Move.FromFov < 100.f && Move.ToFov < 100.f);
			At = Move.Start + Move.Duration;
		}
		TestTrue(TEXT("The shots run to the scene's end"), FMath::IsNearlyEqual(At, RestSceneSeconds(bWestward), TimeSlack));
	}
	for (const FIntroMove& Move : RestMoves(false))
	{
		if (Move.Start <= 0.f)
		{
			continue;
		}
		for (const FIntroCaption& Caption : RestCaptions())
		{
			const bool bClear = Caption.At + Caption.Seconds <= Move.Start - CutDipSeconds + TimeSlack || Caption.At >= Move.Start + CutDipSeconds - TimeSlack;
			TestTrue(FString::Printf(TEXT("The caption at %.1f s is clear of the cut at %.1f s"), Caption.At, Move.Start), bClear);
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIntroDueTest, "Looter.Intro.Due",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FIntroDueTest::RunTest(const FString& Parameters)
{
	FCampaignRecord Campaign;
	TestFalse(TEXT("Before the story begins: not due"), NewGameIntro::IsDue(Campaign));
	Campaign.bFirstCastOff = true;
	TestTrue(TEXT("A new game's first arrival: due"), NewGameIntro::IsDue(Campaign));
	Campaign.SetFlag(NewGameIntro::SeenFlag());
	TestFalse(TEXT("Seen: never again"), NewGameIntro::IsDue(Campaign));
	Campaign.ClearFlag(NewGameIntro::SeenFlag());
	Campaign.bColdOpenSeen = true;
	TestFalse(TEXT("A session past its cold open never sees it"), NewGameIntro::IsDue(Campaign));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIntroFramingTest, "Looter.Intro.Framing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FIntroFramingTest::RunTest(const FString& Parameters)
{
	using namespace NewGameIntro;
	TestEqual(TEXT("Ease starts at 0"), Ease(0.f), 0.f);
	TestEqual(TEXT("...ends at 1"), Ease(1.f), 1.f);
	TestTrue(TEXT("...passes the middle at the middle"), FMath::IsNearlyEqual(Ease(0.5f), 0.5f, 0.001f));
	TestTrue(TEXT("...and never goes back"), Ease(0.3f) < Ease(0.31f));

	// A move starts and ends where it says.
	const FIntroMove Move = RestMoves(false)[0];
	TestTrue(TEXT("A move starts at its first eye"), ViewAlong(Move, 0.f).Location.Equals(Move.FromEye, 0.1));
	TestTrue(TEXT("...and ends at its last, looking at its last mark"), ViewAlong(Move, 1.f).Location.Equals(Move.ToEye, 0.1)
		&& ViewAlong(Move, 1.f).Rotation.Vector().Equals((Move.ToLook - Move.ToEye).GetSafeNormal(), 0.001));

	// The grave: from past its foot to a standing eye over it, looking at the headboard.
	const FVector Board(1000.0, 2000.0, 300.0);
	const FVector Foot(1000.0, 2250.0, 300.0);
	const FIntroMove Grave = GraveMove(Board, Foot, 37.f, 13.5f);
	const FIntroView End = ViewAlong(Grave, 1.f);
	const double Near = FVector::Dist2D(End.Location, Board);
	TestTrue(FString::Printf(TEXT("The grave shot ends close (%.0f cm)"), Near), Near > 300.0 && Near < 600.0);
	TestTrue(TEXT("...from the foot's side"), FVector::DotProduct(End.Location - Board, Foot - Board) > 0.0);
	TestTrue(TEXT("...above it, looking down at the board"), End.Location.Z > Board.Z && End.Rotation.Pitch < 0.0);

	// The planet: the menu's view first, then never inside the planet or jumping, ending on the planet's disc.
	FPlanetFrame Planet;
	Planet.bFound = true;
	Planet.Center = FVector::ZeroVector;
	Planet.Radius = 100000.f;
	Planet.ToSun = FVector(0.0, -1.0, 0.1).GetSafeNormal();
	FIntroView Base;
	Base.Location = FVector(-600000.0, 0.0, 50000.0);
	Base.Rotation = (FVector(0.0, -160000.0, 0.0) - Base.Location).Rotation();
	Base.Fov = 60.f;
	for (const FVector& Sun : { Planet.ToSun, (Base.Location - Planet.Center).GetSafeNormal() })
	{
		Planet.ToSun = Sun;
		const FIntroView First = PlanetView(Base, Planet, 0.f);
		TestTrue(TEXT("It starts on the menu's own view"), First.Location.Equals(Base.Location, 1.0) && First.Rotation.Equals(Base.Rotation, 0.5f));
		FIntroView Last = First;
		bool bOutside = true;
		bool bSmooth = true;
		for (float Time = Frame; Time <= PlanetSeconds + TimeSlack; Time += Frame)
		{
			const FIntroView View = PlanetView(Base, Planet, Time);
			bOutside &= FVector::Dist(View.Location, Planet.Center) >= Planet.Radius * 1.6;
			bSmooth &= FVector::Dist(View.Location, Last.Location) < Base.Location.Size() * 0.02 && !View.Location.ContainsNaN();
			Last = View;
		}
		TestTrue(TEXT("The camera stays well outside the planet"), bOutside);
		TestTrue(TEXT("...and never jumps"), bSmooth);
		const FVector ToCenter = (Planet.Center - Last.Location).GetSafeNormal();
		const double Disc = FMath::Asin(Planet.Radius / FVector::Dist(Last.Location, Planet.Center));
		TestTrue(TEXT("It ends looking at the planet"), FMath::Acos(FVector::DotProduct(ToCenter, Last.Rotation.Vector())) <= Disc);
		TestTrue(TEXT("...the lens a little narrower"), Last.Fov < Base.Fov);
	}
	// No planet found: the menu's view turns a little and narrows, in place.
	const FIntroView Blind = PlanetView(Base, FPlanetFrame(), PlanetSeconds);
	TestTrue(TEXT("With no planet the camera stays put"), Blind.Location.Equals(Base.Location, 0.1));
	TestTrue(TEXT("...and narrows its lens"), Blind.Fov < Base.Fov * 0.8f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIntroSkipKeysTest, "Looter.Intro.SkipKeys",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FIntroSkipKeysTest::RunTest(const FString& Parameters)
{
	// The fades.
	TestEqual(TEXT("Nothing before a caption"), IntroFade::Alpha(-0.1f, 4.f, 0.5f, 0.5f), 0.f);
	TestTrue(TEXT("Half in, half up"), FMath::IsNearlyEqual(IntroFade::Alpha(0.25f, 4.f, 0.5f, 0.5f), 0.5f));
	TestEqual(TEXT("Held full"), IntroFade::Alpha(2.f, 4.f, 0.5f, 0.5f), 1.f);
	TestTrue(TEXT("Going out at the end"), IntroFade::Alpha(3.9f, 4.f, 0.5f, 0.5f) < 0.3f);
	TestEqual(TEXT("Gone after"), IntroFade::Alpha(4.f, 4.f, 0.5f, 0.5f), 0.f);

	// The scene's skip rules, in the menu: the key that picked the hero does nothing; a held key or two Escapes skip.
	{
		FIntroSkipKeys Keys;
		Keys.HoldDown(TEXT("E"));
		Keys.Tick(USceneSubsystem::SkipHoldSeconds + 0.1f);
		TestFalse(TEXT("A key down in the first moment does nothing"), Keys.WantsSkip());
	}
	{
		FIntroSkipKeys Keys;
		Keys.Tick(1.f);
		Keys.HoldDown(TEXT("ENTER"));
		Keys.Tick(USceneSubsystem::SkipHoldSeconds * 0.5f);
		TestTrue(TEXT("Holding shows the prompt, half full"), Keys.IsPromptShown() && FMath::IsNearlyEqual(Keys.GetHold(), 0.5f, 0.01f)
			&& Keys.GetKey() == TEXT("ENTER") && Keys.GetLead() == TEXT("HOLD"));
		Keys.HoldUp();
		TestFalse(TEXT("Let go early: no skip"), Keys.WantsSkip());
		Keys.HoldDown(TEXT("E"));
		Keys.Tick(USceneSubsystem::SkipHoldSeconds + 0.05f);
		TestTrue(TEXT("Held a second: skipped"), Keys.WantsSkip());
	}
	{
		FIntroSkipKeys Keys;
		Keys.Tick(1.f);
		Keys.Escape();
		TestTrue(TEXT("One Escape asks again"), !Keys.WantsSkip() && Keys.GetLead() == TEXT("PRESS") && Keys.GetKey() == TEXT("ESC"));
		Keys.Tick(USceneSubsystem::SkipPromptSeconds + 0.1f);
		Keys.Escape();
		TestFalse(TEXT("Too late for the second: it asks again"), Keys.WantsSkip());
		Keys.Tick(0.5f);
		Keys.Escape();
		TestTrue(TEXT("Two in time: skipped"), Keys.WantsSkip());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIntroRestTest, "Looter.Intro.Rest",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FIntroRestTest::RunTest(const FString& Parameters)
{
	FScopedCVar Scenes(TEXT("Looter.Scenes"), 2);
	FScopedCVar On(TEXT("Looter.Intro"), 1);
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	USceneSubsystem* Played = World->GetSubsystem<USceneSubsystem>();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	AActor* Player = MissionTestWorld::SpawnMarker(World, FVector::ZeroVector);
	if (!TestNotNull(TEXT("Scenes"), Played) || !TestNotNull(TEXT("Missions"), Runner) || !Player)
	{
		return false;
	}
	FCampaignRecord Campaign;
	Runner->BeginForTesting({}, Campaign, Player, TEXT("TestValley"));

	int32 Thens = 0;
	TestFalse(TEXT("Before the story begins nothing plays"), NewGameIntro::PlayRestIfDue(*World, [&Thens]() { ++Thens; }));
	Campaign.bFirstCastOff = true;

	// Played through: Westward first here (no menu played the planet), the whole length, then Then once, recorded seen.
	TestTrue(TEXT("A new game's first arrival plays it"), NewGameIntro::PlayRestIfDue(*World, [&Thens]() { ++Thens; }));
	TestTrue(TEXT("...as the scene playing"), Played->IsPlaying() && Played->GetPlayingName() == NewGameIntro::SceneName());
	TestFalse(TEXT("A second call while it plays starts nothing"), NewGameIntro::PlayRestIfDue(*World, [&Thens]() { ++Thens; }));
	TickScenes(*Played, NewGameIntro::RestSceneSeconds(true) - 1.f);
	TestTrue(TEXT("Still playing a second before its end (Westward first)"), Played->IsPlaying() && Thens == 0);
	TickScenes(*Played, 1.5f);
	TestFalse(TEXT("Ended"), Played->IsPlaying() && Played->GetPlayingName() == NewGameIntro::SceneName());
	TestEqual(TEXT("...Then once"), Thens, 1);
	TestTrue(TEXT("...recorded seen"), Campaign.HasFlag(NewGameIntro::SeenFlag()));
	TestFalse(TEXT("...and never again"), NewGameIntro::PlayRestIfDue(*World, [&Thens]() { ++Thens; }));

	// Skipped: Then all the same, and recorded.
	Played->ReturnFromScene();
	Campaign.ClearFlag(NewGameIntro::SeenFlag());
	TestTrue(TEXT("Due again, it plays"), NewGameIntro::PlayRestIfDue(*World, [&Thens]() { ++Thens; }));
	TickScenes(*Played, 3.f);
	Played->SkipScene();
	TestEqual(TEXT("A skip runs Then"), Thens, 2);
	TestTrue(TEXT("...and records it seen"), Campaign.HasFlag(NewGameIntro::SeenFlag()));

	// Off: passed, and recorded so it never waits.
	Played->ReturnFromScene();
	Campaign.ClearFlag(NewGameIntro::SeenFlag());
	{
		FScopedCVar Off(TEXT("Looter.Intro"), 0);
		TestFalse(TEXT("Looter.Intro 0 plays nothing"), NewGameIntro::PlayRestIfDue(*World, [&Thens]() { ++Thens; }));
	}
	TestTrue(TEXT("...and passes it"), Campaign.HasFlag(NewGameIntro::SeenFlag()) && Thens == 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIntroLinesTest, "Looter.Intro.Lines",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FIntroLinesTest::RunTest(const FString& Parameters)
{
	// The line set a writer edits (create_story_lines.py) says what the code wrote, line for line.
	const FString Path = NewGameIntro::LinesAssetPath();
	if (!FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(Path)))
	{
		AddWarning(FString::Printf(TEXT("%s isn't made yet (create_story_lines.py): the written words play. Skipped."), *Path));
		return true;
	}
	const UStoryLineSet* Set = LoadObject<UStoryLineSet>(nullptr, *Path);
	const TArray<FStoryLine>& Written = NewGameIntro::ScriptLines();
	if (!TestNotNull(TEXT("The line set loads"), Set) || !TestEqual(TEXT("...with a line for every slot"), Set->Lines.Num(), Written.Num()))
	{
		return false;
	}
	for (int32 Index = 0; Index < Written.Num(); ++Index)
	{
		TestEqual(FString::Printf(TEXT("Line %d"), Index), Set->Lines[Index].Text.ToString(), Written[Index].Text.ToString());
	}
	return true;
}

#endif

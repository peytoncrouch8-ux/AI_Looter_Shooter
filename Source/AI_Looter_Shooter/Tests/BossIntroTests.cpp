#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Scenes/BossIntro.h"
#include "Scenes/BossIntroFreeze.h"
#include "Scenes/SceneSubsystem.h"
#include "Session/CampaignRecord.h"
#include "Story/StoryLineSet.h"
#include "Tests/MissionTestWorld.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Misc/PackageName.h"
#include "Tests/AutomationCommon.h"

namespace
{
	constexpr float Frame = 1.f / 30.f;
	/** Times in the scripts are tenths of a second: this much slack for float sums. */
	constexpr float TimeSlack = 0.01f;

	const EBossIntro AllKinds[] = { EBossIntro::Gravemother, EBossIntro::Foreman, EBossIntro::Abel };

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

	/** How long a caption slot is up: its line's reading time, or the boss's own line's. */
	float CaptionSeconds(EBossIntro Kind, const FBossIntroCaption& Caption)
	{
		const TArray<FStoryLine>& Lines = BossIntro::ScriptLines(Kind);
		return Caption.Line == BossIntro::OwnLine ? BossIntro::OwnLineSeconds
			: (Lines.IsValidIndex(Caption.Line) ? Lines[Caption.Line].GetSeconds() : 0.f);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBossIntroScriptTest, "Looter.Scenes.BossIntro.Script",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBossIntroScriptTest::RunTest(const FString& Parameters)
{
	using namespace BossIntro;
	for (const EBossIntro Kind : AllKinds)
	{
		const FBossIntroScript& Plan = Script(Kind);
		const FString Name = KindName(Kind);

		// Short (the user's 8-15 s), its beats in order, its card read in full before it ends.
		TestTrue(FString::Printf(TEXT("%s: %.1f s, 8-15 s"), *Name, Plan.Seconds), Plan.Seconds >= ShortestSeconds && Plan.Seconds <= LongestSeconds);
		TestTrue(Name + TEXT(": the boss goes live before the hand back"), Plan.LiveAt > 0.f && Plan.LiveAt < Plan.HandBackAt);
		TestTrue(Name + TEXT(": the hand back eases to the end"), FMath::IsNearlyEqual(Plan.HandBackAt + HandBackSeconds, Plan.Seconds, TimeSlack));
		// Fair: long enough to see its entrance, not so long it's deep into its fight while the player is held.
		const float LiveSeconds = Plan.Seconds - Plan.LiveAt;
		TestTrue(FString::Printf(TEXT("%s: %.1f s live on camera, 2.5-5 s"), *Name, LiveSeconds), LiveSeconds >= 2.5f && LiveSeconds <= 5.f);
		TestTrue(Name + TEXT(": the card comes with the reveal"), Plan.CardAt >= Plan.LiveAt - TimeSlack && Plan.CardAt <= Plan.LiveAt + 1.f);
		TestTrue(Name + TEXT(": the card stays 3 s and is gone by the end"), Plan.CardSeconds >= 3.f && Plan.CardAt + Plan.CardSeconds <= Plan.Seconds + TimeSlack);
		TestNotNull(*(Name + TEXT(": music")), Plan.Music);
		TestFalse(Name + TEXT(": a name on its card"), CardTitle(Kind).IsEmpty() || CardSub(Kind).IsEmpty());
		TestEqual(Name + TEXT(": the card's in capitals"), CardTitle(Kind).ToString(), CardTitle(Kind).ToString().ToUpper());

		// The shots: back to back from 0 to the end, each one moving, real lenses.
		float At = 0.f;
		for (const FBossIntroShot& Shot : Plan.Shots)
		{
			const FString Where = Name + TEXT(", ") + Shot.Name.ToString();
			TestTrue(Where + TEXT(" starts as the shot before ends"), FMath::IsNearlyEqual(Shot.Start, At, TimeSlack));
			TestTrue(Where + TEXT(" moves"), Shot.Duration > 0.f && !Shot.FromEye.Offset.Equals(Shot.ToEye.Offset, 1.0));
			TestTrue(Where + TEXT("'s lens is a lens"), Shot.FromFov > 30.f && Shot.ToFov > 30.f && Shot.FromFov < 100.f && Shot.ToFov < 100.f);
			At = Shot.Start + Shot.Duration;
		}
		TestTrue(Name + TEXT(": the shots run to the end"), Plan.Shots.Num() >= 2 && FMath::IsNearlyEqual(At, Plan.Seconds, TimeSlack));

		// The captions: one at a time, each line once, none in a cut's dip, all said by the end.
		TArray<int32> Used;
		float FreeAt = 0.f;
		for (const FBossIntroCaption& Caption : Plan.Captions)
		{
			const FString Where = FString::Printf(TEXT("%s, the caption at %.1f s"), *Name, Caption.At);
			TestTrue(Where + TEXT(" comes after the one before has gone"), Caption.At >= FreeAt - TimeSlack);
			FreeAt = Caption.At + CaptionSeconds(Kind, Caption);
			TestTrue(Where + TEXT(" is said by the end"), FreeAt <= Plan.Seconds + TimeSlack);
			TestTrue(Where + TEXT(" says something"), CaptionSeconds(Kind, Caption) > 0.f);
			Used.Add(Caption.Line);
			for (const FBossIntroShot& Shot : Plan.Shots)
			{
				if (Shot.Start > 0.f)
				{
					TestTrue(Where + TEXT(" doesn't start in a cut's dip"), Caption.At <= Shot.Start - CutDipSeconds - TimeSlack
						|| Caption.At >= Shot.Start + CutDipSeconds - TimeSlack);
				}
			}
		}
		for (int32 Index = 0; Index < ScriptLines(Kind).Num(); ++Index)
		{
			TestEqual(FString::Printf(TEXT("%s: line %d is said once"), *Name, Index), Used.FilterByPredicate([Index](int32 Line) { return Line == Index; }).Num(), 1);
		}

		// The Gravemother's burrow: the dive, then the line of bursts, then her burst out.
		if (Plan.DiveAt >= 0.f)
		{
			TestTrue(Name + TEXT(": the dive comes before the burst"), Plan.DiveAt < Plan.TrailFrom && Plan.TrailTo < Plan.LiveAt);
			TestTrue(Name + TEXT(": a line of bursts"), Plan.TrailPuffs >= 3);
			TestTrue(Name + TEXT(": the burst framed"), Plan.Shots.Last().FromLook.Anchor == EBossIntroAnchor::Burst);
		}
	}
	TestEqual(TEXT("The Gravemother's is the one that dives"), Script(EBossIntro::Gravemother).DiveAt >= 0.f && Script(EBossIntro::Foreman).DiveAt < 0.f
		&& Script(EBossIntro::Abel).DiveAt < 0.f, true);
	TestTrue(TEXT("The Foreman's comes out of the dark"), Script(EBossIntro::Foreman).Shots[0].FromExposure < -1.f
		&& FMath::IsNearlyZero(Script(EBossIntro::Foreman).Shots.Last().ToExposure));
	TestTrue(TEXT("Abel says his own line on camera"), Script(EBossIntro::Abel).bClearCaptions && Script(EBossIntro::Abel).Captions.ContainsByPredicate(
		[](const FBossIntroCaption& Caption) { return Caption.Line == BossIntro::OwnLine; }));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBossIntroWordsTest, "Looter.Scenes.BossIntro.Words",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBossIntroWordsTest::RunTest(const FString& Parameters)
{
	// Spoken lines (a speaker each), and none of Ellis's family's: the same for every hero (Pipeline/Story.md).
	const TArray<FString> Family = { TEXT("Pa"), TEXT("Pa's"), TEXT("El"), TEXT("Ellis"), TEXT("Grandma"), TEXT("grandmother"),
		TEXT("Delia"), TEXT("kid") };
	for (const EBossIntro Kind : AllKinds)
	{
		for (const FStoryLine& Line : BossIntro::ScriptLines(Kind))
		{
			const FString Words = Line.Text.ToString();
			TestFalse(FString::Printf(TEXT("\"%s\" has words"), *Words), Words.IsEmpty());
			TestFalse(FString::Printf(TEXT("\"%s\" has a speaker"), *Words), Line.Speaker.IsEmpty());
			TestTrue(FString::Printf(TEXT("\"%s\" is short (a caption on the move)"), *Words), Words.Len() <= 60);
			for (const FString& Word : WordsOf(Words))
			{
				TestFalse(FString::Printf(TEXT("\"%s\" names no family (%s)"), *Words, *Word), Family.ContainsByPredicate(
					[&Word](const FString& Name) { return Name.Equals(Word, ESearchCase::IgnoreCase); }));
			}
		}
		EBossIntro Parsed = EBossIntro::Abel;
		TestTrue(TEXT("Its name reads back"), BossIntro::ParseKind(FString(BossIntro::KindName(Kind)).ToLower(), Parsed) && Parsed == Kind);
		TestTrue(TEXT("Its scene and flag are its own"), BossIntro::SceneName(Kind) != BossIntro::SeenFlag(Kind));
	}
	EBossIntro Unused = EBossIntro::Abel;
	TestFalse(TEXT("Nothing else reads as a boss"), BossIntro::ParseKind(TEXT("Sexton"), Unused));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBossIntroDueTest, "Looter.Scenes.BossIntro.Due",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBossIntroDueTest::RunTest(const FString& Parameters)
{
	FScopedCVar On(TEXT("Looter.BossIntro"), 1);
	FScopedCVar Replay(TEXT("Looter.BossIntro.Replay"), 0);
	FCampaignRecord Campaign;
	TestTrue(TEXT("A first fight in a session: due"), BossIntro::IsDue(&Campaign, EBossIntro::Foreman, false));
	TestFalse(TEXT("A vigil stone's rematch: never"), BossIntro::IsDue(&Campaign, EBossIntro::Foreman, true));
	Campaign.SetFlag(BossIntro::SeenFlag(EBossIntro::Foreman));
	TestFalse(TEXT("Seen in this session: not again"), BossIntro::IsDue(&Campaign, EBossIntro::Foreman, false));
	TestTrue(TEXT("...each boss its own"), BossIntro::IsDue(&Campaign, EBossIntro::Gravemother, false));
	TestTrue(TEXT("No story running: due"), BossIntro::IsDue(nullptr, EBossIntro::Abel, false));
	{
		FScopedCVar Again(TEXT("Looter.BossIntro.Replay"), 1);
		TestTrue(TEXT("Replay: seen or not"), BossIntro::IsDue(&Campaign, EBossIntro::Foreman, false));
		TestFalse(TEXT("...never a rematch"), BossIntro::IsDue(&Campaign, EBossIntro::Foreman, true));
	}
	{
		FScopedCVar Off(TEXT("Looter.BossIntro"), 0);
		TestFalse(TEXT("Off: never"), BossIntro::IsDue(&Campaign, EBossIntro::Abel, false));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBossIntroFramingTest, "Looter.Scenes.BossIntro.Framing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBossIntroFramingTest::RunTest(const FString& Parameters)
{
	using namespace BossIntro;
	TestEqual(TEXT("Ease starts at 0"), Ease(0.f), 0.f);
	TestEqual(TEXT("...ends at 1"), Ease(1.f), 1.f);
	TestTrue(TEXT("...and never goes back"), Ease(0.3f) < Ease(0.31f));

	// The frame: forward from the boss to the player, flat; right square to it; points placed along them.
	const FVector Boss(1000.0, 0.0, 100.0);
	const FVector Eye(1000.0, 2000.0, 400.0);
	const FBossIntroFrame BossFrame = FBossIntroFrame::Make(Boss, Eye, Boss, FVector::ForwardVector);
	TestTrue(TEXT("Forward runs to the player, flat"), BossFrame.Forward.Equals(FVector(0.0, 1.0, 0.0), 0.001));
	TestTrue(TEXT("Right is square to it"), FMath::IsNearlyZero(FVector::DotProduct(BossFrame.Forward, BossFrame.Right), 0.001) && FMath::IsNearlyZero(BossFrame.Right.Z, 0.001));
	FBossIntroPoint Behind;
	Behind.Anchor = EBossIntroAnchor::Player;
	Behind.Offset = FVector(100.0, 0.0, 50.0);
	TestTrue(TEXT("A point behind the player is farther from the boss"), BossFrame.Resolve(Behind).Equals(Eye + FVector(0.0, 100.0, 50.0), 0.1));
	const FBossIntroFrame Over = FBossIntroFrame::Make(Boss, Boss + FVector(0.0, 0.0, 300.0), Boss, FVector(-1.0, 0.0, 0.0));
	TestTrue(TEXT("A player right over it: framed from the way it faces"), Over.Forward.Equals(FVector(-1.0, 0.0, 0.0), 0.001));

	// A shot starts and ends where it says, looking at its marks.
	const FBossIntroShot& Shot = Script(EBossIntro::Abel).Shots[0];
	const FBossIntroView First = ViewAlong(Shot, BossFrame, 0.f);
	const FBossIntroView Last = ViewAlong(Shot, BossFrame, 1.f);
	TestTrue(TEXT("A shot starts at its first eye"), First.Location.Equals(BossFrame.Resolve(Shot.FromEye), 0.1));
	TestTrue(TEXT("...and ends at its last, looking at its last mark"), Last.Location.Equals(BossFrame.Resolve(Shot.ToEye), 0.1)
		&& Last.Rotation.Vector().Equals((BossFrame.Resolve(Shot.ToLook) - Last.Location).GetSafeNormal(), 0.001));
	FBossIntroShot Tracked = Shot;
	Tracked.bTrackBoss = true;
	const FVector Moved(0.0, 300.0, 0.0);
	TestTrue(TEXT("A tracking shot's look follows the boss"), ViewAlong(Tracked, BossFrame, 1.f, Moved).Rotation.Vector()
		.Equals((BossFrame.Resolve(Shot.ToLook) + Moved - Last.Location).GetSafeNormal(), 0.001));

	// The boom: clear, it stays; blocked, it comes in front of what's in the way, never nearer than the shortest.
	const FVector Look(0.0, 0.0, 0.0);
	const FVector Far(1000.0, 0.0, 0.0);
	TestTrue(TEXT("Nothing in the way: the eye stays"), BoomClip(Look, Far, 1.f).Equals(Far, 0.1));
	TestTrue(TEXT("A wall halfway: the eye in front of it"), BoomClip(Look, Far, 0.5f).Equals(FVector(500.0 - BoomMargin, 0.0, 0.0), 0.1));
	TestTrue(TEXT("A wall right there: never nearer than the shortest"), BoomClip(Look, Far, 0.05f).Equals(FVector(BoomShortest, 0.0, 0.0), 0.1));

	// The Gravemother's burst: on her way to the player, kept off them, on her own ground.
	const FVector Her(0.0, 0.0, 0.0);
	const FVector Home(0.0, 0.0, 0.0);
	const FVector Burst = BurstAlong(Her, FVector(3000.0, 0.0, 50.0), Home);
	TestTrue(FString::Printf(TEXT("A share of the way (%.0f cm)"), Burst.X), FMath::IsNearlyEqual(Burst.X, 3000.0 * BurstShare, 1.0) && FMath::IsNearlyZero(Burst.Y, 0.1));
	TestTrue(TEXT("...at her height"), FMath::IsNearlyZero(Burst.Z, 0.1));
	const FVector Near = BurstAlong(Her, FVector(1500.0, 0.0, 0.0), Home);
	TestTrue(TEXT("A player near her: kept their distance"), FVector::Dist2D(Near, FVector(1500.0, 0.0, 0.0)) >= BurstMinFromPlayer - 1.0);
	const FVector OnHer = BurstAlong(Her, FVector(500.0, 0.0, 0.0), Home);
	TestTrue(TEXT("A player right on her: she comes up where she went down"), OnHer.Equals(Her, 0.1));
	const FVector Far2 = BurstAlong(FVector(1400.0, 0.0, 0.0), FVector(9000.0, 0.0, 0.0), Home);
	TestTrue(TEXT("Never off her own ground"), FVector::Dist2D(Far2, Home) <= BurstMaxFromHome + 1.0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBossIntroPlayTest, "Looter.Scenes.BossIntro.Play",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBossIntroPlayTest::RunTest(const FString& Parameters)
{
	FScopedCVar Scenes(TEXT("Looter.Scenes"), 2);
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	USceneSubsystem* Played = World->GetSubsystem<USceneSubsystem>();
	AActor* Boss = MissionTestWorld::SpawnMarker(World, FVector(0.0, 0.0, 100.0));
	AActor* Bystander = MissionTestWorld::SpawnMarker(World, FVector(500.0, 0.0, 100.0));
	if (!TestNotNull(TEXT("Scenes"), Played) || !Boss || !Bystander)
	{
		return false;
	}

	// The freeze: held at the freeze's pace, given back as it was, held once however often asked.
	{
		Bystander->CustomTimeDilation = 0.5f;
		FBossIntroFreeze Freeze;
		Freeze.Hold(*Bystander, BossIntro::FreezeDilation);
		Freeze.Hold(*Bystander, BossIntro::FreezeDilation);
		TestTrue(TEXT("Held still"), FMath::IsNearlyEqual(Bystander->CustomTimeDilation, BossIntro::FreezeDilation) && Freeze.Num() == 1);
		Freeze.Release(Bystander);
		TestEqual(TEXT("Given back its own pace"), Bystander->CustomTimeDilation, 0.5f);
		Freeze.Hold(*Bystander, 0.f);
		TestTrue(TEXT("Never quite 0"), Bystander->CustomTimeDilation > 0.f);
		Freeze.ReleaseAll();
		TestEqual(TEXT("All let go"), Bystander->CustomTimeDilation, 0.5f);
		Bystander->CustomTimeDilation = 1.f;
	}

	// Played through: the boss held until its Live moment, then its own; the other moments in order; the end once.
	for (const bool bSkip : { false, true })
	{
		const EBossIntro Kind = bSkip ? EBossIntro::Gravemother : EBossIntro::Foreman;
		const FBossIntroScript& Plan = BossIntro::Script(Kind);
		int32 Dives = 0;
		int32 Puffs = 0;
		int32 Lives = 0;
		int32 Ends = 0;
		bool bEndSkipped = false;
		float DilationAtLive = -1.f;
		FBossIntroActs Acts;
		Acts.Dive = [&Dives](bool) { ++Dives; };
		Acts.Trail = [&Puffs](int32, const FVector&, bool) { ++Puffs; };
		Acts.Live = [&Lives, &DilationAtLive, Boss](bool) { ++Lives; DilationAtLive = Boss->CustomTimeDilation; };
		Acts.Ended = [&Ends, &bEndSkipped](bool bSkipped) { ++Ends; bEndSkipped = bSkipped; };
		const FString Name = BossIntro::KindName(Kind);
		if (!TestTrue(Name + TEXT(" plays"), BossIntro::Play(Kind, *Boss, nullptr, MoveTemp(Acts))))
		{
			continue;
		}
		TestTrue(Name + TEXT(" is the scene playing"), Played->IsPlaying() && Played->GetPlayingName() == BossIntro::SceneName(Kind));
		TestTrue(Name + TEXT(": the boss is held at once"), FMath::IsNearlyEqual(Boss->CustomTimeDilation, BossIntro::FreezeDilation));
		FBossIntroActs Again;
		TestFalse(Name + TEXT(": a second while it plays starts nothing"), BossIntro::Play(Kind, *Boss, nullptr, MoveTemp(Again)));
		if (bSkip)
		{
			TickScenes(*Played, 1.f);
			Played->SkipScene();
		}
		else
		{
			TickScenes(*Played, Plan.LiveAt - 0.2f);
			TestTrue(Name + TEXT(": still held just before it goes live"), FMath::IsNearlyEqual(Boss->CustomTimeDilation, BossIntro::FreezeDilation) && Lives == 0);
			TickScenes(*Played, 0.4f);
			TestEqual(Name + TEXT(": live once"), Lives, 1);
			TestEqual(Name + TEXT(": its own pace back for its entrance"), DilationAtLive, 1.f);
			TickScenes(*Played, Plan.Seconds - Plan.LiveAt);
		}
		TestFalse(Name + TEXT(": ended"), Played->IsPlaying() && Played->GetPlayingName() == BossIntro::SceneName(Kind));
		TestEqual(Name + TEXT(": the end once"), Ends, 1);
		TestEqual(Name + TEXT(": ...played or skipped as it was"), bEndSkipped, bSkip);
		TestEqual(Name + TEXT(": live once, played or skipped"), Lives, 1);
		TestEqual(Name + TEXT(": the dives its script has"), Dives, Plan.DiveAt >= 0.f ? 1 : 0);
		TestEqual(Name + TEXT(": the line of bursts its script has"), Puffs, Plan.TrailPuffs);
		TestEqual(Name + TEXT(": the boss runs on its own clock after"), Boss->CustomTimeDilation, 1.f);
	}

	// Scenes off (a tour, a perf run): nothing plays, nothing is held.
	{
		FScopedCVar Off(TEXT("Looter.Scenes"), 0);
		FBossIntroActs Acts;
		TestFalse(TEXT("Scenes off: no intro"), BossIntro::Play(EBossIntro::Abel, *Boss, nullptr, MoveTemp(Acts)));
		TestEqual(TEXT("...and the boss isn't held"), Boss->CustomTimeDilation, 1.f);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBossIntroLinesTest, "Looter.Scenes.BossIntro.Lines",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBossIntroLinesTest::RunTest(const FString& Parameters)
{
	// The line sets a writer edits (create_story_lines.py) say what the code wrote, line for line.
	for (const EBossIntro Kind : AllKinds)
	{
		const FString Path = BossIntro::LinesAssetPath(Kind);
		if (!FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(Path)))
		{
			AddWarning(FString::Printf(TEXT("%s isn't made yet (create_story_lines.py): the written words play. Skipped."), *Path));
			continue;
		}
		const UStoryLineSet* Set = LoadObject<UStoryLineSet>(nullptr, *Path);
		const TArray<FStoryLine>& Written = BossIntro::ScriptLines(Kind);
		if (!TestNotNull(TEXT("The line set loads"), Set) || !TestEqual(TEXT("...with a line for every slot"), Set->Lines.Num(), Written.Num()))
		{
			continue;
		}
		for (int32 Index = 0; Index < Written.Num(); ++Index)
		{
			TestEqual(FString::Printf(TEXT("%s line %d"), BossIntro::KindName(Kind), Index), Set->Lines[Index].Text.ToString(), Written[Index].Text.ToString());
		}
	}
	return true;
}

#endif

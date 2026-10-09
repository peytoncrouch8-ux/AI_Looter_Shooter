#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Audio/CreatureVoiceBarks.h"
#include "Audio/CreatureVoiceComponent.h"
#include "Audio/CreatureVoiceDirector.h"
#include "Audio/LooterSoundCues.h"
#include "Creatures/CreatureRank.h"
#include "Creatures/SpiderCreature.h"
#include "Creatures/UnpaidCreature.h"
#include "Tests/BossTestWorld.h"
#include "UI/World/CreatureBarkActor.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"

namespace
{
	const ECreatureBark AllSituations[] = { ECreatureBark::Idle, ECreatureBark::Hurt, ECreatureBark::Spot,
		ECreatureBark::PackmateDeath, ECreatureBark::Death };

	/** A creature in a test world, started as play starts it (a test world never begins play itself), dropping no loot. */
	template <typename TCreature>
	TCreature* SpawnCreature(UWorld* World, const FVector& Where, ECreatureRank Rank = ECreatureRank::Basic)
	{
		TCreature* Creature = World->SpawnActor<TCreature>(Where, FRotator::ZeroRotator);
		if (Creature)
		{
			Creature->StartingRank = Rank;
			Creature->DispatchBeginPlay();
			BossTestWorld::NoLoot(Creature);
		}
		return Creature;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCreatureBarksLinesTest, "Looter.Creatures.Barks.Lines",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCreatureBarksLinesTest::RunTest(const FString& Parameters)
{
	// About forty lines, each situation with plain ones for any Unpaid and angrier ones for the ranked souls, every line
	// short enough to read over a creature in a fight (a few wrapped lines at most), and none twice.
	const TConstArrayView<FCreatureBarkLine> Lines = CreatureBarks::AllLines();
	TestTrue(TEXT("About forty lines"), Lines.Num() >= 36 && Lines.Num() <= 60);
	TSet<FString> Seen;
	for (const FCreatureBarkLine& Line : Lines)
	{
		const FString Text = Line.Text;
		TestFalse(TEXT("No empty line"), Text.IsEmpty());
		TestTrue(FString::Printf(TEXT("Short enough to read: \"%s\""), *Text), Text.Len() <= 60);
		TestFalse(FString::Printf(TEXT("Said once in the table: \"%s\""), *Text), Seen.Contains(Text));
		Seen.Add(Text);
	}
	for (const ECreatureBark Situation : AllSituations)
	{
		int32 Plain = 0;
		int32 Angry = 0;
		for (const FCreatureBarkLine& Line : Lines)
		{
			if (Line.Situation == Situation)
			{
				(Line.bAngry ? Angry : Plain) += 1;
			}
		}
		const int32 Kind = static_cast<int32>(Situation);
		TestTrue(FString::Printf(TEXT("Situation %d: at least three plain lines"), Kind), Plain >= 3);
		TestTrue(FString::Printf(TEXT("Situation %d: at least two angry lines"), Kind), Angry >= 2);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCreatureBarksLineChoiceTest, "Looter.Creatures.Barks.LineChoice",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCreatureBarksLineChoiceTest::RunTest(const FString& Parameters)
{
	const TConstArrayView<FCreatureBarkLine> Lines = CreatureBarks::AllLines();
	FRandomStream Random(20261009);
	for (const ECreatureBark Situation : AllSituations)
	{
		const int32 Kind = static_cast<int32>(Situation);
		// A Basic soul only grumbles: plain lines of the situation, never an angry one.
		bool bBasicRight = true;
		for (int32 Roll = 0; Roll < 300; ++Roll)
		{
			const int32 Picked = CreatureBarks::PickLine(Situation, ECreatureRank::Basic, Random);
			bBasicRight &= Lines.IsValidIndex(Picked) && Lines[Picked].Situation == Situation && !Lines[Picked].bAngry;
		}
		TestTrue(FString::Printf(TEXT("Situation %d: a Basic Unpaid says its plain lines"), Kind), bBasicRight);

		// Restless and Gravebound souls are angry most of the time, and still say the situation's lines.
		for (const ECreatureRank Rank : { ECreatureRank::Rare, ECreatureRank::Epic })
		{
			int32 Angry = 0;
			bool bSituationRight = true;
			constexpr int32 Rolls = 1000;
			for (int32 Roll = 0; Roll < Rolls; ++Roll)
			{
				const int32 Picked = CreatureBarks::PickLine(Situation, Rank, Random);
				bSituationRight &= Lines.IsValidIndex(Picked) && Lines[Picked].Situation == Situation;
				Angry += Lines.IsValidIndex(Picked) && Lines[Picked].bAngry ? 1 : 0;
			}
			const float Share = static_cast<float>(Angry) / Rolls;
			TestTrue(FString::Printf(TEXT("Situation %d, rank %d: its own situation's lines"), Kind, static_cast<int32>(Rank)), bSituationRight);
			TestTrue(FString::Printf(TEXT("Situation %d, rank %d: mostly angry (%.2f)"), Kind, static_cast<int32>(Rank), Share),
				FMath::Abs(Share - CreatureBarks::AngryShare) < 0.08f);
		}
	}

	// A boss has its own words: it never barks.
	TestEqual(TEXT("A boss says nothing"), CreatureBarks::PickLine(ECreatureBark::Spot, ECreatureRank::Boss, Random), INDEX_NONE);
	TestFalse(TEXT("...and its rank can't bark"), CreatureBarks::CanBarkRank(ECreatureRank::Boss));

	// Lines said lately aren't said again while another is left; with every one of them said lately, one comes anyway.
	TArray<int32> PlainIdle;
	for (int32 Index = 0; Index < Lines.Num(); ++Index)
	{
		if (Lines[Index].Situation == ECreatureBark::Idle && !Lines[Index].bAngry)
		{
			PlainIdle.Add(Index);
		}
	}
	TArray<int32> Recent = PlainIdle;
	const int32 Left = Recent.Pop();
	bool bOnlyLeft = true;
	for (int32 Roll = 0; Roll < 100; ++Roll)
	{
		bOnlyLeft &= CreatureBarks::PickLine(ECreatureBark::Idle, ECreatureRank::Basic, Random, Recent) == Left;
	}
	TestTrue(TEXT("Recent lines are skipped: the one left is said"), bOnlyLeft);
	TestTrue(TEXT("Every line said lately: one comes anyway"),
		PlainIdle.Contains(CreatureBarks::PickLine(ECreatureBark::Idle, ECreatureRank::Basic, Random, PlainIdle)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCreatureBarksCooldownsTest, "Looter.Creatures.Barks.Cooldowns",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCreatureBarksCooldownsTest::RunTest(const FString& Parameters)
{
	using EVerdict = FCreatureBarkBoard::EVerdict;
	FCreatureBarkBoard Board;
	const FVector Here(0.0, 0.0, 0.0);
	const FVector Listener(800.0, 0.0, 0.0);
	TestTrue(TEXT("Nothing said yet: a spot may come"), Board.Check(1, ECreatureBark::Spot, Here, Listener, 0.0) == EVerdict::Allowed);

	// A spot on screen for 2 s from 0.
	Board.Start(1, ECreatureBark::Spot, Here, 0.0, 2.f);
	TestTrue(TEXT("It shows"), Board.IsShowing(1.0));
	TestTrue(TEXT("...said by its speaker"), Board.GetSpeaker(1.0) == 1u);

	// After it leaves the screen, a pause before any other.
	const double End = 2.0;
	TestTrue(TEXT("Right after it ends: the pause"), Board.Check(2, ECreatureBark::Hurt, Here, Listener, End + 0.5) == EVerdict::Cooling);
	TestTrue(TEXT("The pause over: a hurt may come"),
		Board.Check(2, ECreatureBark::Hurt, Here, Listener, End + CreatureBarks::GapAfter + 0.1) == EVerdict::Allowed);

	// Each kind rests a while after it's said: another spot only after the spots' rest.
	const double SpotRest = CreatureBarks::SituationCooldown(ECreatureBark::Spot);
	TestTrue(TEXT("Spot rest not over"), Board.Check(2, ECreatureBark::Spot, Here, Listener, SpotRest - 1.0) == EVerdict::Cooling);
	TestTrue(TEXT("Spot rest over"), Board.Check(2, ECreatureBark::Spot, Here, Listener, SpotRest + 0.1) == EVerdict::Allowed);

	// Last words and a fallen packmate are events: they don't wait out the pause after a bark (only their own rest).
	TestTrue(TEXT("Last words right after a bark"), Board.Check(3, ECreatureBark::Death, Here, Listener, End + 0.2) == EVerdict::Allowed);
	TestTrue(TEXT("A packmate's fall right after a bark"),
		Board.Check(3, ECreatureBark::PackmateDeath, Here, Listener, End + 0.2) == EVerdict::Allowed);
	Board.Start(3, ECreatureBark::Death, Here, End + 0.2, 2.f);
	const double DeathRest = CreatureBarks::SituationCooldown(ECreatureBark::Death);
	TestTrue(TEXT("Another's last words wait out the deaths' rest"),
		Board.Check(4, ECreatureBark::Death, Here, Listener, End + 0.2 + 2.5) == EVerdict::Cooling);
	TestTrue(TEXT("...and then come"), Board.Check(4, ECreatureBark::Death, Here, Listener, End + 0.2 + DeathRest + 0.1) == EVerdict::Allowed);

	// The rules' numbers: spots often, hurts now and then, mutters rarely and rested longest.
	TestTrue(TEXT("A spot is likelier than a hurt"), CreatureBarks::ChanceFor(ECreatureBark::Spot) > CreatureBarks::ChanceFor(ECreatureBark::Hurt));
	for (const ECreatureBark Situation : AllSituations)
	{
		TestTrue(TEXT("Every chance is a chance"), CreatureBarks::ChanceFor(Situation) > 0.f && CreatureBarks::ChanceFor(Situation) < 1.f);
		TestTrue(TEXT("Mutters rest longest"), CreatureBarks::SituationCooldown(ECreatureBark::Idle) >= CreatureBarks::SituationCooldown(Situation));
	}
	TestTrue(TEXT("A creature rests a while after its own bark"), CreatureBarks::SpeakerRest >= 5.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCreatureBarksOneAtATimeTest, "Looter.Creatures.Barks.OneAtATime",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCreatureBarksOneAtATimeTest::RunTest(const FString& Parameters)
{
	using EVerdict = FCreatureBarkBoard::EVerdict;
	FCreatureBarkBoard Board;
	const FVector Listener(0.0, 0.0, 0.0);
	const FVector Near(1000.0, 0.0, 0.0);
	const FVector Far(CreatureBarks::HearRadius + 100.0, 0.0, 0.0);

	// Farther than the hearing radius, nothing is said at all.
	TestTrue(TEXT("Too far to hear"), Board.Check(1, ECreatureBark::Spot, Far, Listener, 0.0) == EVerdict::TooFar);

	// One mutter on screen: nothing else of its weight or less while it shows.
	Board.Start(1, ECreatureBark::Idle, Near, 0.0, 3.f);
	TestTrue(TEXT("Another mutter waits"), Board.Check(2, ECreatureBark::Idle, Near, Listener, 1.0) == EVerdict::Busy);
	// A hurt matters more, but only cuts in once the mutter has been read a moment.
	TestTrue(TEXT("A hurt right away waits"), Board.Check(2, ECreatureBark::Hurt, Near, Listener, 0.2) == EVerdict::Busy);
	TestTrue(TEXT("A hurt after a moment cuts in"),
		Board.Check(2, ECreatureBark::Hurt, Near, Listener, CreatureBarks::MinShownSeconds + 0.1) == EVerdict::Allowed);

	// A spot on screen: a hurt matters less and waits; last words cut in.
	Board.Start(2, ECreatureBark::Spot, Near, 1.0, 3.f);
	TestTrue(TEXT("The spot replaced the mutter"), Board.GetSpeaker(1.5) == 2u);
	TestTrue(TEXT("A hurt waits behind a spot"), Board.Check(3, ECreatureBark::Hurt, Near, Listener, 2.0) == EVerdict::Busy);
	TestTrue(TEXT("Last words cut in over a spot"), Board.Check(3, ECreatureBark::Death, Near, Listener, 2.0) == EVerdict::Allowed);
	// A creature's own last words replace what it was saying at once.
	TestTrue(TEXT("Its own last words at once"), Board.Check(2, ECreatureBark::Death, Near, Listener, 1.1) == EVerdict::Allowed);

	// The priorities in order.
	TestTrue(TEXT("Hurt over mutter"), CreatureBarks::Priority(ECreatureBark::Hurt) > CreatureBarks::Priority(ECreatureBark::Idle));
	TestTrue(TEXT("Spot over hurt"), CreatureBarks::Priority(ECreatureBark::Spot) > CreatureBarks::Priority(ECreatureBark::Hurt));
	TestTrue(TEXT("Packmate over spot"), CreatureBarks::Priority(ECreatureBark::PackmateDeath) > CreatureBarks::Priority(ECreatureBark::Spot));
	TestTrue(TEXT("Last words over all"), CreatureBarks::Priority(ECreatureBark::Death) > CreatureBarks::Priority(ECreatureBark::PackmateDeath));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCreatureBarksMurmurTest, "Looter.Creatures.Barks.Murmur",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCreatureBarksMurmurTest::RunTest(const FString& Parameters)
{
	// Syllables as a reader would say them.
	TestEqual(TEXT("Hey"), CreatureBarks::CountSyllables(TEXT("Hey")), 1);
	TestEqual(TEXT("home (a silent e)"), CreatureBarks::CountSyllables(TEXT("home")), 1);
	TestEqual(TEXT("the"), CreatureBarks::CountSyllables(TEXT("the")), 1);
	TestEqual(TEXT("little"), CreatureBarks::CountSyllables(TEXT("little")), 2);
	TestEqual(TEXT("Ransom"), CreatureBarks::CountSyllables(TEXT("Ransom")), 2);
	TestEqual(TEXT("Ma'll"), CreatureBarks::CountSyllables(TEXT("Ma'll")), 1);
	TestEqual(TEXT("Finally"), CreatureBarks::CountSyllables(TEXT("Finally")), 3);

	// One murmured syllable for each, in order, a syllable's time apart within a phrase.
	const TArray<FMurmurGrain> Shout = CreatureBarks::BuildMurmur(TEXT("Hey! You owe me!"), 7);
	TestEqual(TEXT("A syllable each"), Shout.Num(), 4);
	bool bInOrder = true;
	for (int32 Index = 1; Index < Shout.Num(); ++Index)
	{
		bInOrder &= Shout[Index].Time > Shout[Index - 1].Time;
	}
	TestTrue(TEXT("In order"), bInOrder);
	TestTrue(TEXT("A shout is louder"), Shout[0].Volume > 1.f);

	// Held at an ellipsis far longer than between words.
	const TArray<FMurmurGrain> Trailing = CreatureBarks::BuildMurmur(TEXT("Tell Ma... I tried to get home."), 7);
	if (TestEqual(TEXT("Tell Ma I tried to get home: seven syllables"), Trailing.Num(), 7))
	{
		const float WordGap = Trailing[1].Time - Trailing[0].Time;
		const float Held = Trailing[2].Time - Trailing[1].Time;
		TestTrue(TEXT("Held at the dots"), Held > WordGap + 0.25f);
		// A statement falls; the line's last syllable trails off quieter.
		TestTrue(TEXT("It falls through the line"), Trailing.Last().Pitch < Trailing[0].Pitch);
		TestTrue(TEXT("...and trails off"), Trailing.Last().Volume < 1.f);
	}

	// A question lifts at its end.
	const TArray<FMurmurGrain> Question = CreatureBarks::BuildMurmur(TEXT("Did they cross?"), 7);
	if (TestEqual(TEXT("Did they cross: three syllables"), Question.Num(), 3))
	{
		TestTrue(TEXT("A question rises at its end"), Question.Last().Pitch > Question[1].Pitch);
	}

	// A long line keeps to the cap, and the same line and seed murmur the same.
	const TArray<FMurmurGrain> Long = CreatureBarks::BuildMurmur(
		TEXT("Somebody owes me for a mule, and a plough, and forty acres of wet hay, and a Sunday shirt besides."), 7);
	TestTrue(TEXT("At most MaxGrains syllables"), Long.Num() <= CreatureBarks::MaxGrains && Long.Num() >= 12);
	const TArray<FMurmurGrain> Again = CreatureBarks::BuildMurmur(TEXT("Tell Ma... I tried to get home."), 7);
	TestTrue(TEXT("The same line murmurs the same"), Again.Num() == Trailing.Num() && Again.Last().Time == Trailing.Last().Time);

	// On screen long enough to read and past the murmur's end, never lingering.
	const FString Line = TEXT("Tell Ma... I tried to get home.");
	const float MurmurEnd = Trailing.Last().Time + 0.25f;
	const float Shown = CreatureBarks::DisplaySeconds(Line, MurmurEnd);
	TestTrue(TEXT("Shown past the murmur"), Shown >= MurmurEnd + 0.69f || Shown >= 4.5f - KINDA_SMALL_NUMBER);
	TestTrue(TEXT("Never lingers"), Shown <= 4.5f && CreatureBarks::DisplaySeconds(TEXT("Hey"), 0.2f) >= 1.6f);

	// Each Unpaid has a voice of its own, within its kind's range.
	const float A = CreatureBarks::VoicePitchFor(TEXT("UnpaidCreature_1"));
	const float B = CreatureBarks::VoicePitchFor(TEXT("UnpaidCreature_2"));
	TestTrue(TEXT("Voices in range"), A >= 0.88f && A <= 1.14f && B >= 0.88f && B <= 1.14f);
	TestEqual(TEXT("The same creature, the same voice"), CreatureBarks::VoicePitchFor(TEXT("UnpaidCreature_1")), A);
	TestTrue(TEXT("Whispered: mutters and last words"), CreatureBarks::IsWhispered(ECreatureBark::Idle)
		&& CreatureBarks::IsWhispered(ECreatureBark::Death) && !CreatureBarks::IsWhispered(ECreatureBark::Spot));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCreatureBarksDirectorTest, "Looter.Creatures.Barks.Director",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCreatureBarksDirectorTest::RunTest(const FString& Parameters)
{
	// Two Unpaid and a spider near the player: only the Unpaid have words; one bark goes up over its speaker while it
	// murmurs it, and the other's waits; out of hearing, nothing is said.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UCreatureVoiceDirector* Director = UCreatureVoiceDirector::Get(World);
	if (!TestNotNull(TEXT("The level's creature voices"), Director))
	{
		return false;
	}
	Director->SetRandomSeed(20261009);
	AStaticMeshActor* NearListener = World->SpawnActor<AStaticMeshActor>(FVector(600.0, 0.0, 0.0), FRotator::ZeroRotator);
	AStaticMeshActor* FarListener = World->SpawnActor<AStaticMeshActor>(FVector(9000.0, 0.0, 0.0), FRotator::ZeroRotator);
	Director->SetTestListener(NearListener);

	AUnpaidCreature* First = SpawnCreature<AUnpaidCreature>(World, FVector(0.0, 0.0, 0.0));
	AUnpaidCreature* Second = SpawnCreature<AUnpaidCreature>(World, FVector(0.0, 500.0, 0.0), ECreatureRank::Rare);
	ASpiderCreature* Spider = SpawnCreature<ASpiderCreature>(World, FVector(0.0, -600.0, 0.0));
	if (!TestNotNull(TEXT("An Unpaid"), First) || !TestNotNull(TEXT("A Restless Unpaid"), Second) || !TestNotNull(TEXT("A spider"), Spider))
	{
		return false;
	}
	UCreatureVoiceComponent* FirstVoice = First->FindComponentByClass<UCreatureVoiceComponent>();
	UCreatureVoiceComponent* SecondVoice = Second->FindComponentByClass<UCreatureVoiceComponent>();
	UCreatureVoiceComponent* SpiderVoice = Spider->FindComponentByClass<UCreatureVoiceComponent>();
	if (!TestNotNull(TEXT("Voices"), FirstVoice) || !SecondVoice || !SpiderVoice)
	{
		return false;
	}
	TestTrue(TEXT("Their voices joined the level's"), Director->NumVoices() >= 3);
	TestTrue(TEXT("An Unpaid has words"), FirstVoice->CanBark());
	TestFalse(TEXT("A spider has none"), SpiderVoice->CanBark());
	TestTrue(TEXT("...only its idle call"), SpiderVoice->HasIdleCall() && !FirstVoice->HasIdleCall());
	TestFalse(TEXT("A spider can't be made to bark"), Director->RequestBark(*SpiderVoice, ECreatureBark::Spot));

	// The first spots the player: its line goes up over it and it murmurs it, voiced, a syllable at a time.
	if (TestTrue(TEXT("The first's spot is said"), Director->RequestBark(*FirstVoice, ECreatureBark::Spot)))
	{
		const int32 Line = Director->GetLastLine();
		const TConstArrayView<FCreatureBarkLine> Lines = CreatureBarks::AllLines();
		if (TestTrue(TEXT("A line was picked"), Lines.IsValidIndex(Line)))
		{
			TestTrue(TEXT("...a spot's"), Lines[Line].Situation == ECreatureBark::Spot);
			TestFalse(TEXT("...a plain one, from a Basic soul"), Lines[Line].bAngry);
			ACreatureBarkActor* Words = Director->GetBarkActor();
			if (TestNotNull(TEXT("Its words float over it"), Words))
			{
				TestTrue(TEXT("...showing"), Words->IsShowing());
				TestTrue(TEXT("...over the first"), Words->GetSpeaker() == First);
				TestEqual(TEXT("...its line"), Words->GetLine().ToString(), FString(Lines[Line].Text));
				TestFalse(TEXT("...said aloud, not muttered"), Words->IsMuttered());
			}
			TestEqual(TEXT("Its murmur: a syllable for each of the line's"), FirstVoice->GetMurmur().Num(),
				CreatureBarks::BuildMurmur(Lines[Line].Text, 0).Num());
			TestEqual(TEXT("...voiced"), FirstVoice->GetMurmurCue(), FName(LooterSoundCue::Voice::UnpaidMurmur));
		}
	}

	// One at a time: the second's spot waits while the first's is on screen.
	TestFalse(TEXT("The second's spot waits"), Director->RequestBark(*SecondVoice, ECreatureBark::Spot));
	TestTrue(TEXT("...the words stay over the first"), Director->GetBarkActor() && Director->GetBarkActor()->GetSpeaker() == First);

	// Out of hearing, nothing is said, not even last words.
	Director->SetTestListener(FarListener);
	TestFalse(TEXT("Out of hearing: no last words"), Director->RequestBark(*SecondVoice, ECreatureBark::Death));

	// The words fade out and go (the bark actor's own clock, which a test level doesn't tick).
	if (ACreatureBarkActor* Words = Director->GetBarkActor())
	{
		TestEqual(TEXT("Faded in from nothing"), ACreatureBarkActor::AlphaAt(0.f, 2.f), 0.f);
		TestEqual(TEXT("Fully shown in the middle"), ACreatureBarkActor::AlphaAt(1.f, 2.f), 1.f);
		Words->Advance(10.f);
		TestFalse(TEXT("Gone after its time"), Words->IsShowing());
	}
	return true;
}

#endif

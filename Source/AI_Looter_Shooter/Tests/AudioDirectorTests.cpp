#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Audio/AmbienceRules.h"
#include "Audio/LooterSoundBank.h"
#include "Audio/LooterSoundCues.h"
#include "Audio/LooterSoundRules.h"
#include "Audio/MusicRules.h"
#include "Creatures/CreatureRank.h"
#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Sound/SoundWave.h"
#include "World/LightingStateSubsystem.h"

namespace
{
	const FName DayName(TEXT("Day"));
	const FName DuskName(TEXT("Dusk"));

	/** The places' cues (the World.* ones the ambience plays), which belong to the Ambience class like the beds. */
	const TCHAR* const PlaceCues[] = {
		LooterSoundCue::Creek, LooterSoundCue::Pond, LooterSoundCue::Waterfall, LooterSoundCue::WindmillFan,
		LooterSoundCue::WindmillCreak, LooterSoundCue::ChapelBellHum, LooterSoundCue::TrainHiss, LooterSoundCue::RimWind,
		LooterSoundCue::SinkDrip, LooterSoundCue::SinkCreak, LooterSoundCue::TownHush, LooterSoundCue::TownCreak,
		LooterSoundCue::ShutterTap, LooterSoundCue::TownMurmur,
	};

	const TCHAR* const PlaceLoops[] = {
		LooterSoundCue::Creek, LooterSoundCue::Pond, LooterSoundCue::Waterfall, LooterSoundCue::WindmillFan,
		LooterSoundCue::ChapelBellHum, LooterSoundCue::TrainHiss, LooterSoundCue::RimWind, LooterSoundCue::SinkDrip,
		LooterSoundCue::TownHush,
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAudioCombatTimeoutTest, "Looter.Audio.Director.CombatTimeout",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FAudioCombatTimeoutTest::RunTest(const FString& Parameters)
{
	const float CalmAfter = MusicRules::CalmAfterSeconds;
	FCombatMemory Memory;
	TestFalse(TEXT("Nothing hunting: calm"), Memory.Update(false, 0.0, CalmAfter));
	TestFalse(TEXT("...and no fight"), Memory.bInCombat);

	// A creature turns on the player: the fight starts at once.
	TestTrue(TEXT("Hunted: the fight starts (a change)"), Memory.Update(true, 10.0, CalmAfter));
	TestTrue(TEXT("...in combat"), Memory.bInCombat);
	TestFalse(TEXT("Still hunted: no change"), Memory.Update(true, 11.0, CalmAfter));

	// It loses sight for a moment: the music doesn't drop out and come straight back.
	TestFalse(TEXT("Let go for 3 s: still a fight"), Memory.Update(false, 14.0, CalmAfter));
	TestTrue(TEXT("...in combat"), Memory.bInCombat);
	TestFalse(TEXT("Hunted again: the same fight"), Memory.Update(true, 15.0, CalmAfter));
	TestFalse(TEXT("5 s after the last let go: still a fight"), Memory.Update(false, 20.0, CalmAfter));
	TestTrue(TEXT("...in combat"), Memory.bInCombat);

	// CalmAfter seconds after the last one let go, it's over.
	TestTrue(TEXT("The grace runs out: calm (a change)"), Memory.Update(false, 15.0 + CalmAfter + 0.01, CalmAfter));
	TestFalse(TEXT("...no fight"), Memory.bInCombat);
	TestTrue(TEXT("The fight ran from the first turn to the last let-go"), FMath::IsNearlyEqual(Memory.FightSeconds(), 5.0, 1.e-6));
	TestTrue(TEXT("A grace of a few seconds, not long"), CalmAfter >= 3.f && CalmAfter <= 10.f);

	// The next fight starts its own clock.
	TestTrue(TEXT("A new fight"), Memory.Update(true, 60.0, CalmAfter));
	TestTrue(TEXT("...counted from its start"), FMath::IsNearlyEqual(Memory.FightSeconds(), 0.0, 1.e-6));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAudioMoodTest, "Looter.Audio.Director.MoodsAndDucking",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FAudioMoodTest::RunTest(const FString& Parameters)
{
	// A boss's fight over any fight, a fight over calm.
	TestTrue(TEXT("Calm"), MusicRules::Resolve(false, false) == EMusicMood::Calm);
	TestTrue(TEXT("Combat"), MusicRules::Resolve(false, true) == EMusicMood::Combat);
	TestTrue(TEXT("A boss's fight with its adds hunting is the boss's"), MusicRules::Resolve(true, true) == EMusicMood::Boss);
	TestTrue(TEXT("A boss's fight before anything hunts is still the boss's"), MusicRules::Resolve(true, false) == EMusicMood::Boss);
	TestTrue(TEXT("The Unpaid's boss plays Abel's theme"), MusicRules::ThemeFor(true) == EBossTheme::Keeper);
	TestTrue(TEXT("Any other plays the Gravemother's"), MusicRules::ThemeFor(false) == EBossTheme::Gravemother);

	// Ducking: under a scene and a pause the music dips, never to silence (a paused game keeps its music).
	TestEqual(TEXT("Nothing to duck under"), MusicRules::DuckFor(false, false), 1.f);
	TestEqual(TEXT("Under a scene"), MusicRules::DuckFor(true, false), MusicRules::SceneDuck);
	TestEqual(TEXT("Over a paused game"), MusicRules::DuckFor(false, true), MusicRules::PauseDuck);
	TestEqual(TEXT("Both: the deeper"), MusicRules::DuckFor(true, true), FMath::Min(MusicRules::SceneDuck, MusicRules::PauseDuck));
	TestTrue(TEXT("A scene's duck is real but not silence"), MusicRules::SceneDuck > 0.1f && MusicRules::SceneDuck < 0.7f);
	TestTrue(TEXT("A pause's duck is real but not silence"), MusicRules::PauseDuck > 0.1f && MusicRules::PauseDuck < 0.8f);
	TestTrue(TEXT("The theme dips under the combat layer, still heard"), MusicRules::UnderCombat > 0.3f && MusicRules::UnderCombat < 1.f);
	TestTrue(TEXT("A boss theme dips under a phase sting"), MusicRules::PhaseDuck < 1.f && MusicRules::PhaseDuckSeconds > 0.f);

	// The bed crossfades with the light: done under a fade by the time the picture's back, slow when the sun's watched.
	const float UnderFade = AmbienceRules::BedCrossfadeSeconds(ELightingSwitch::Fade);
	const float AtOnce = AmbienceRules::BedCrossfadeSeconds(ELightingSwitch::Instant);
	const float Watched = AmbienceRules::BedCrossfadeSeconds(ELightingSwitch::Blend);
	TestTrue(TEXT("Every crossfade takes a moment"), UnderFade > 0.f && AtOnce > 0.f && Watched > 0.f);
	TestTrue(TEXT("Under a fade it's quick"), UnderFade <= 2.f);
	TestTrue(TEXT("A watched sunset is the slowest"), Watched > UnderFade && Watched > AtOnce);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAudioBarLinesTest, "Looter.Audio.Director.BarLines",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FAudioBarLinesTest::RunTest(const FString& Parameters)
{
	// The grid: the next bar line at or after now; one just passed counts as now; before the grid starts, its start.
	TestTrue(TEXT("Mid bar: the next line"), FMath::IsNearlyEqual(MusicRules::NextBoundary(10.0, 0.0, 1.6), 11.2, 1.e-6));
	TestTrue(TEXT("On a line: now"), FMath::IsNearlyEqual(MusicRules::NextBoundary(11.2, 0.0, 1.6), 11.2, 1.e-6));
	TestTrue(TEXT("Just past a line: now, not a bar later"), FMath::IsNearlyEqual(MusicRules::NextBoundary(11.21, 0.0, 1.6), 11.21, 1.e-6));
	TestTrue(TEXT("Well past it: the next"), FMath::IsNearlyEqual(MusicRules::NextBoundary(11.3, 0.0, 1.6), 12.8, 1.e-6));
	TestTrue(TEXT("Before the grid: its start"), FMath::IsNearlyEqual(MusicRules::NextBoundary(0.5, 2.0, 1.6), 2.0, 1.e-6));
	TestTrue(TEXT("An offset grid"), FMath::IsNearlyEqual(MusicRules::NextBoundary(5.0, 1.0, 1.875), 6.625, 1.e-6));
	TestTrue(TEXT("No grid: now"), FMath::IsNearlyEqual(MusicRules::NextBoundary(5.0, 1.0, 0.0), 5.0, 1.e-6));

	// The pieces that layer share a bar, and the theme's length holds the combat layer's a whole number of times, so the
	// layer stays on the theme's grid however long they run together.
	const FMusicPiece Sky = MusicRules::ExploreFor(ELooterAudioArea::Skyreach);
	const FMusicPiece Rest = MusicRules::ExploreFor(ELooterAudioArea::RansomsRest);
	const FMusicPiece Fight = MusicRules::Combat();
	TestEqual(TEXT("Skyreach plays its own theme"), Sky.Cue, FName(LooterSoundCue::MusicSkyreachExplore));
	TestEqual(TEXT("The Rest plays its own"), Rest.Cue, FName(LooterSoundCue::MusicRansomsRestExplore));
	TestEqual(TEXT("A map of no area plays Skyreach's"), MusicRules::ExploreFor(ELooterAudioArea::Unknown).Cue, Sky.Cue);
	for (const FMusicPiece& Theme : { Sky, Rest })
	{
		TestEqual(FString::Printf(TEXT("%s shares the combat layer's bar"), *Theme.Cue.ToString()), Theme.BarSeconds, Fight.BarSeconds);
		TestEqual(TEXT("...and its beats"), Theme.BeatsPerBar, Fight.BeatsPerBar);
		TestEqual(TEXT("...and holds it a whole number of times"), Theme.Bars % Fight.Bars, 0);
	}
	TestTrue(TEXT("6/8 at 75: a bar of 1.6 s, two beats"), FMath::IsNearlyEqual(Fight.BarSeconds, 1.6f) && Fight.BeatsPerBar == 2);
	TestTrue(TEXT("Abel's: 4/4 at 128"), FMath::IsNearlyEqual(MusicRules::BossFor(EBossTheme::Keeper).BeatSeconds(), 60.f / 128.f));
	TestTrue(TEXT("The Gravemother's: 4/4 at 120"), FMath::IsNearlyEqual(MusicRules::BossFor(EBossTheme::Gravemother).BeatSeconds(), 0.5f));
	TestTrue(TEXT("The phase sting's hit is within its first second"), MusicRules::PhaseStingHit > 0.f && MusicRules::PhaseStingHit < 1.f);

	// Every piece is a Music cue, and once the bank is built each one's sound is exactly its bars long and loops.
	const ULooterSoundBank* Bank = ULooterSoundBank::Load();
	for (const FMusicPiece& Piece : MusicRules::AllPieces())
	{
		TestTrue(FString::Printf(TEXT("%s is a music cue"), *Piece.Cue.ToString()), Piece.Cue.ToString().StartsWith(TEXT("Music.")));
		const FLooterSoundCueEntry* Entry = Bank ? Bank->FindCue(Piece.Cue) : nullptr;
		const USoundWave* Wave = Entry && !Entry->Sounds.IsEmpty() ? Entry->Sounds[0].Get() : nullptr;
		if (!Wave)
		{
			continue;
		}
		TestTrue(FString::Printf(TEXT("%s loops"), *Piece.Cue.ToString()), Entry->bLoop);
		TestFalse(FString::Printf(TEXT("%s plays flat (2D)"), *Piece.Cue.ToString()), Entry->bSpatial);
		TestTrue(FString::Printf(TEXT("%s is %d bars of %.3f s (it's %.3f s)"), *Piece.Cue.ToString(), Piece.Bars, Piece.BarSeconds, Wave->Duration),
			FMath::IsNearlyEqual(Wave->Duration, static_cast<float>(Piece.LoopSeconds()), 0.01f));
	}
	if (!Bank)
	{
		AddWarning(TEXT("No sound bank yet: the pieces' lengths weren't checked against their sounds."));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAudioDirectorRulesTest, "Looter.Audio.Director.Rules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FAudioDirectorRulesTest::RunTest(const FString& Parameters)
{
	// Victory: a fight that ran a while and killed something; not a creature giving up, not a skirmish.
	TestTrue(TEXT("A real fight with a kill"), MusicRules::EarnsVictory(20.0, 2));
	TestFalse(TEXT("No kill: no victory"), MusicRules::EarnsVictory(60.0, 0));
	TestFalse(TEXT("Over in a moment: no fanfare"), MusicRules::EarnsVictory(MusicRules::VictoryMinFightSeconds - 1.0, 3));
	TestTrue(TEXT("Exactly long enough"), MusicRules::EarnsVictory(MusicRules::VictoryMinFightSeconds, 1));
	TestTrue(TEXT("Not every fight in a row"), MusicRules::VictoryCooldown >= 20.f);

	// Elites: Gravebound and Soulfed are announced; Basic and Restless aren't (too common); a boss has its own sting.
	TestFalse(TEXT("Basic"), MusicRules::IsElite(ECreatureRank::Basic));
	TestFalse(TEXT("Restless"), MusicRules::IsElite(ECreatureRank::Rare));
	TestTrue(TEXT("Gravebound"), MusicRules::IsElite(ECreatureRank::Epic));
	TestTrue(TEXT("Soulfed"), MusicRules::IsElite(ECreatureRank::Legendary));
	TestFalse(TEXT("A boss"), MusicRules::IsElite(ECreatureRank::Boss));

	// Calm: two or three plays, then a rest in its range.
	TestEqual(TEXT("A low roll: two plays"), MusicRules::CalmLoops(0.f), 2);
	TestEqual(TEXT("A high roll: three"), MusicRules::CalmLoops(1.f), 3);
	for (float Roll = 0.f; Roll <= 1.f; Roll += 0.125f)
	{
		const int32 Loops = MusicRules::CalmLoops(Roll);
		const float Rest = MusicRules::CalmRestSeconds(Roll);
		TestTrue(TEXT("Two or three plays"), Loops == 2 || Loops == 3);
		TestTrue(TEXT("A rest in its range"), Rest >= MusicRules::MinRestSeconds && Rest <= MusicRules::MaxRestSeconds);
	}
	TestTrue(TEXT("Rests are long enough to notice the world, short enough to miss the music"),
		MusicRules::MinRestSeconds >= 20.f && MusicRules::MaxRestSeconds <= 180.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAudioAmbienceBedsTest, "Looter.Audio.Ambience.Beds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FAudioAmbienceBedsTest::RunTest(const FString& Parameters)
{
	// The area from the map's name, however the editor or a path dresses it.
	TestTrue(TEXT("The Rest"), AmbienceRules::AreaForMap(TEXT("Lvl_RansomsRest")) == ELooterAudioArea::RansomsRest);
	TestTrue(TEXT("The Rest in play-in-editor"), AmbienceRules::AreaForMap(TEXT("UEDPIE_0_Lvl_RansomsRest")) == ELooterAudioArea::RansomsRest);
	TestTrue(TEXT("The tutorial island is Skyreach"), AmbienceRules::AreaForMap(TEXT("/Game/Maps/Lvl_TutorialIsland")) == ELooterAudioArea::Skyreach);
	TestTrue(TEXT("The old Skyreach map too"), AmbienceRules::AreaForMap(TEXT("UEDPIE_1_Lvl_Skyreach")) == ELooterAudioArea::Skyreach);
	TestTrue(TEXT("Any other map has none of its own"), AmbienceRules::AreaForMap(TEXT("Lvl_TerrainTest")) == ELooterAudioArea::Unknown);

	// The light: Dusk is evening; Day, none and anything else are day.
	TestTrue(TEXT("Dusk"), AmbienceRules::IsDusk(DuskName));
	TestFalse(TEXT("Day"), AmbienceRules::IsDusk(DayName));
	TestFalse(TEXT("No states"), AmbienceRules::IsDusk(NAME_None));

	// Each area's beds: both layers, the Rest's changing at dusk, a map of no area borrowing Skyreach's.
	const FAmbienceBed RestDay = AmbienceRules::BedFor(ELooterAudioArea::RansomsRest, DayName);
	const FAmbienceBed RestDusk = AmbienceRules::BedFor(ELooterAudioArea::RansomsRest, DuskName);
	const FAmbienceBed Sky = AmbienceRules::BedFor(ELooterAudioArea::Skyreach, DayName);
	TestEqual(TEXT("The Rest by day: dry wind"), RestDay.Air, FName(LooterSoundCue::RansomsRestDayAir));
	TestEqual(TEXT("...and cicadas"), RestDay.Life, FName(LooterSoundCue::RansomsRestDayLife));
	TestEqual(TEXT("At dusk: the Gravewind"), RestDusk.Air, FName(LooterSoundCue::RansomsRestDuskAir));
	TestEqual(TEXT("...and crickets"), RestDusk.Life, FName(LooterSoundCue::RansomsRestDuskLife));
	TestFalse(TEXT("Day and dusk differ"), RestDay == RestDusk);
	TestEqual(TEXT("Skyreach's breeze"), Sky.Air, FName(LooterSoundCue::SkyreachAir));
	TestEqual(TEXT("...and meadow"), Sky.Life, FName(LooterSoundCue::SkyreachLife));
	TestTrue(TEXT("No light named: the day's"), AmbienceRules::BedFor(ELooterAudioArea::RansomsRest, NAME_None) == RestDay);
	TestTrue(TEXT("A map of no area: Skyreach's"), AmbienceRules::BedFor(ELooterAudioArea::Unknown, DayName) == Sky);
	TestTrue(TEXT("Skyreach at a dusk keeps its breeze"), AmbienceRules::BedFor(ELooterAudioArea::Skyreach, DuskName).Air == Sky.Air);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAudioSweetenersTest, "Looter.Audio.Ambience.Sweeteners",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FAudioSweetenersTest::RunTest(const FString& Parameters)
{
	// Every area and light has calls, each an Ambience cue placed sensibly; only the thunder is heard flat.
	for (const ELooterAudioArea Area : { ELooterAudioArea::Skyreach, ELooterAudioArea::RansomsRest, ELooterAudioArea::Unknown })
	{
		for (const FName Light : { DayName, DuskName })
		{
			const TArray<FAmbienceSweetener>& Options = AmbienceRules::SweetenersFor(Area, Light);
			TestTrue(TEXT("It has sweeteners"), Options.Num() >= 3);
			for (const FAmbienceSweetener& S : Options)
			{
				const FString Name = S.Cue.ToString();
				TestTrue(FString::Printf(TEXT("%s is an ambience cue"), *Name), Name.StartsWith(TEXT("Ambience.")));
				TestTrue(FString::Printf(TEXT("%s waits a few seconds after"), *Name), S.MinGap >= 2.f && S.MaxGap >= S.MinGap);
				TestTrue(FString::Printf(TEXT("%s has a weight"), *Name), S.Weight > 0.f);
				TestTrue(FString::Printf(TEXT("%s: only far thunder is flat"), *Name), S.bFlat == (S.Cue == FName(LooterSoundCue::ThunderFar)));
				if (!S.bFlat)
				{
					TestTrue(FString::Printf(TEXT("%s stands off the listener, within the birds' reach"), *Name),
						S.MinDistance >= 100.f && S.MaxDistance >= S.MinDistance && S.MaxDistance <= 4000.f);
				}
			}
		}
	}
	TestTrue(TEXT("Owls at dusk, not by day"),
		AmbienceRules::SweetenersFor(ELooterAudioArea::RansomsRest, DuskName).ContainsByPredicate([](const FAmbienceSweetener& S) { return S.Cue == FName(LooterSoundCue::Owl); })
		&& !AmbienceRules::SweetenersFor(ELooterAudioArea::RansomsRest, DayName).ContainsByPredicate([](const FAmbienceSweetener& S) { return S.Cue == FName(LooterSoundCue::Owl); }));

	// Picking: never the same twice running while another is ready; cooldowns respected; weights followed.
	const TArray<FAmbienceSweetener>& Day = AmbienceRules::SweetenersFor(ELooterAudioArea::RansomsRest, DayName);
	TArray<double> Ready;
	Ready.Init(0.0, Day.Num());
	int32 Last = INDEX_NONE;
	FRandomStream Rolls(17);
	for (int32 Try = 0; Try < 200; ++Try)
	{
		const int32 Pick = AmbienceRules::PickSweetener(Day, Last, Ready, 100.0, Rolls.FRand());
		if (!TestTrue(TEXT("Something is ready"), Day.IsValidIndex(Pick)))
		{
			break;
		}
		TestNotEqual(TEXT("Never the same call twice running"), Pick, Last);
		Last = Pick;
	}
	TArray<double> Cooling;
	Cooling.Init(1000.0, Day.Num());
	TestEqual(TEXT("All cooling down: none"), AmbienceRules::PickSweetener(Day, INDEX_NONE, Cooling, 100.0, 0.5f), static_cast<int32>(INDEX_NONE));
	Cooling[2] = 0.0;
	TestEqual(TEXT("Only one ready: that one"), AmbienceRules::PickSweetener(Day, INDEX_NONE, Cooling, 100.0, 0.9f), 2);
	TestEqual(TEXT("...even when it was the last (better than silence)"), AmbienceRules::PickSweetener(Day, 2, Cooling, 100.0, 0.9f), 2);
	TestEqual(TEXT("The lowest roll takes the first ready"), AmbienceRules::PickSweetener(Day, INDEX_NONE, Ready, 100.0, 0.f), 0);

	// Gaps and places within their ranges.
	const FAmbienceSweetener& Bird = Day[0];
	TestEqual(TEXT("The shortest gap"), AmbienceRules::GapAfter(Bird, 0.f), Bird.MinGap);
	TestEqual(TEXT("The longest"), AmbienceRules::GapAfter(Bird, 1.f), Bird.MaxGap);
	const FVector Listener(100.f, -200.f, 50.f);
	for (int32 Try = 0; Try < 20; ++Try)
	{
		const FVector Spot = AmbienceRules::SpotAround(Listener, Bird, Rolls.FRand(), Rolls.FRand(), Rolls.FRand());
		const float Across = FVector::Dist2D(Spot, Listener);
		const float Up = Spot.Z - Listener.Z;
		TestTrue(TEXT("At its distance"), Across >= Bird.MinDistance - 0.5f && Across <= Bird.MaxDistance + 0.5f);
		TestTrue(TEXT("At its height"), Up >= Bird.MinHeight - 0.5f && Up <= Bird.MaxHeight + 0.5f);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAudioEmitterPathTest, "Looter.Audio.Ambience.EmitterPath",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FAudioEmitterPathTest::RunTest(const FString& Parameters)
{
	// A creek's sound comes from the point on it nearest the listener; a pond's from the nearest shore.
	const TArray<FVector> Creek = { FVector(0.f, 0.f, 0.f), FVector(1000.f, 0.f, 0.f), FVector(1000.f, 1000.f, 0.f) };
	TestTrue(TEXT("Beside the first stretch"), AmbienceRules::NearestOnPath(Creek, false, FVector(400.f, 300.f, 0.f)).Equals(FVector(400.f, 0.f, 0.f), 0.01f));
	TestTrue(TEXT("Beside the second"), AmbienceRules::NearestOnPath(Creek, false, FVector(1300.f, 600.f, 0.f)).Equals(FVector(1000.f, 600.f, 0.f), 0.01f));
	TestTrue(TEXT("Past its end: its end"), AmbienceRules::NearestOnPath(Creek, false, FVector(-500.f, -50.f, 0.f)).Equals(FVector::ZeroVector, 0.01f));
	TestTrue(TEXT("An open line doesn't close"), AmbienceRules::NearestOnPath(Creek, false, FVector(400.f, 700.f, 0.f)).Equals(FVector(1000.f, 700.f, 0.f), 0.01f));
	TestTrue(TEXT("A closed one does"), AmbienceRules::NearestOnPath(Creek, true, FVector(400.f, 700.f, 0.f)).Equals(FVector(550.f, 550.f, 0.f), 0.01f));
	TestTrue(TEXT("One point: that point"), AmbienceRules::NearestOnPath(TArray<FVector>{ FVector(5.f, 6.f, 7.f) }, false, FVector::ZeroVector).Equals(FVector(5.f, 6.f, 7.f)));
	TestTrue(TEXT("No path: the listener's own place"), AmbienceRules::NearestOnPath(TArray<FVector>(), false, FVector(1.f, 2.f, 3.f)).Equals(FVector(1.f, 2.f, 3.f)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAudioDirectorCuesTest, "Looter.Audio.Director.SliderClasses",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FAudioDirectorCuesTest::RunTest(const FString& Parameters)
{
	// The sliders: Ambience follows Effects, Music has its own.
	FLooterVolumes Volumes;
	Volumes.Effects = 0.5f;
	Volumes.Music = 0.2f;
	const FLooterClassGains Gains = LooterSoundRules::ClassGains(Volumes);
	TestTrue(TEXT("Ambience follows the Effects slider"), FMath::IsNearlyEqual(Gains.Ambience, Gains.Effects));
	TestTrue(TEXT("Music answers to its own"), FMath::IsNearlyEqual(Gains.Music, LooterSoundRules::SliderToGain(0.2f)));
	Volumes.Music = 0.f;
	TestEqual(TEXT("Music off doesn't touch the air"), LooterSoundRules::ClassGains(Volumes).Ambience, Gains.Ambience);

	// cues.json puts every ambience and place cue in the Ambience class and every music cue in Music, flat; the beds, the
	// places' loops and the music pieces loop.
	const FString Path = FPaths::Combine(FPaths::ProjectDir(), TEXT("Art/Sounds/cues.json"));
	FString Text;
	TSharedPtr<FJsonObject> Cues;
	if (!FFileHelper::LoadFileToString(Text, *Path) || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Cues) || !Cues)
	{
		AddWarning(TEXT("Art/Sounds/cues.json isn't readable here: the cues' classes weren't checked."));
		return true;
	}
	auto Check = [this, &Cues](const FString& Cue, const TCHAR* Class, bool bLoop, bool bFlat)
	{
		const TSharedPtr<FJsonObject>* Entry = nullptr;
		if (!Cues->TryGetObjectField(Cue, Entry) || !Entry)
		{
			AddWarning(FString::Printf(TEXT("%s isn't in cues.json yet (render it with Tools/sounds.ps1)."), *Cue));
			return;
		}
		TestEqual(FString::Printf(TEXT("%s's class"), *Cue), (*Entry)->GetStringField(TEXT("class")), FString(Class));
		if (bLoop)
		{
			TestTrue(FString::Printf(TEXT("%s loops"), *Cue), (*Entry)->GetBoolField(TEXT("loop")));
		}
		if (bFlat)
		{
			TestEqual(FString::Printf(TEXT("%s plays flat"), *Cue), (*Entry)->GetStringField(TEXT("space")), FString(TEXT("2D")));
		}
	};
	for (const ELooterAudioArea Area : { ELooterAudioArea::Skyreach, ELooterAudioArea::RansomsRest })
	{
		for (const FName Light : { DayName, DuskName })
		{
			const FAmbienceBed Bed = AmbienceRules::BedFor(Area, Light);
			Check(Bed.Air.ToString(), TEXT("Ambience"), true, true);
			Check(Bed.Life.ToString(), TEXT("Ambience"), true, true);
			for (const FAmbienceSweetener& S : AmbienceRules::SweetenersFor(Area, Light))
			{
				Check(S.Cue.ToString(), TEXT("Ambience"), false, S.bFlat);
			}
		}
	}
	for (const TCHAR* Cue : PlaceCues)
	{
		Check(Cue, TEXT("Ambience"), false, false);
	}
	for (const TCHAR* Cue : PlaceLoops)
	{
		Check(Cue, TEXT("Ambience"), true, false);
	}
	for (const FMusicPiece& Piece : MusicRules::AllPieces())
	{
		Check(Piece.Cue.ToString(), TEXT("Music"), true, true);
	}
	for (const TCHAR* Sting : { LooterSoundCue::StingElite, LooterSoundCue::StingPhase, LooterSoundCue::StingVictory })
	{
		Check(Sting, TEXT("Music"), false, true);
	}
	return true;
}

#endif

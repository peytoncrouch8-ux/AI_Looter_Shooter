#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Audio/LooterSound.h"
#include "Audio/LooterSoundBank.h"
#include "Audio/LooterSoundRules.h"
#include "Audio/LooterSoundSubsystem.h"
#include "Audio/PlayerSoundComponent.h"
#include "Audio/SoundSurface.h"
#include "Settings/AudioSettingsSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Internationalization/Regex.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Sound/SoundConcurrency.h"
#include "Sound/SoundWave.h"
#include "Tests/AutomationCommon.h"
#include <limits>

namespace
{
	const FName RealCue(TEXT("Test.Real"));
	const FName PairCue(TEXT("Test.Pair"));
	const FName EmptyCue(TEXT("Test.Empty"));
	const FName LoopCue(TEXT("Test.Loop"));
	const FName MissingCue(TEXT("Test.Missing"));

	FLooterSoundCueEntry TestEntry(FName Cue, int32 SoundCount, int32 MaxConcurrent)
	{
		FLooterSoundCueEntry Entry;
		Entry.Cue = Cue;
		for (int32 Index = 0; Index < SoundCount; ++Index)
		{
			Entry.Sounds.Add(NewObject<USoundWave>(GetTransientPackage()));
		}
		Entry.MaxConcurrent = MaxConcurrent;
		return Entry;
	}

	/**
	 * A sound system of the tests' own (the game's needs a game instance to belong to) on a bank of test cues: three
	 * sounds at -6 dB with a little jitter, two sounds, none, a loop. No world: it picks and refuses, it never plays.
	 */
	ULooterSoundSubsystem* MakeTestSounds()
	{
		UGameInstance* Game = NewObject<UGameInstance>(GetTransientPackage());
		ULooterSoundSubsystem* Sounds = NewObject<ULooterSoundSubsystem>(Game);
		ULooterSoundBank* Bank = NewObject<ULooterSoundBank>(GetTransientPackage());
		FLooterSoundCueEntry Real = TestEntry(RealCue, 3, 3);
		Real.VolumeDb = -6.f;
		Real.PitchJitter = 0.05f;
		FLooterSoundCueEntry Loop = TestEntry(LoopCue, 1, 1);
		Loop.bLoop = true;
		// A limit of 0 (a slip in the JSON) still lets one play.
		Bank->Cues = { Real, TestEntry(PairCue, 2, 0), TestEntry(EmptyCue, 0, 4), Loop };
		Sounds->SetBank(Bank);
		return Sounds;
	}

	/** The cue names LooterSoundCues.h gives, read from the source (the header is the list: nothing to keep in step). */
	bool ReadHeaderCues(TArray<FName>& OutCues)
	{
		const FString Header = FPaths::Combine(FPaths::GameSourceDir(), TEXT("AI_Looter_Shooter/Audio/LooterSoundCues.h"));
		FString Text;
		if (!FFileHelper::LoadFileToString(Text, *Header))
		{
			return false;
		}
		FRegexMatcher Matcher(FRegexPattern(TEXT("TEXT\\(\"([^\"]+)\"\\)")), Text);
		while (Matcher.FindNext())
		{
			OutCues.AddUnique(FName(*Matcher.GetCaptureGroup(1)));
		}
		return true;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAudioMissingCueTest, "Looter.Audio.MissingCueIsSilent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FAudioMissingCueTest::RunTest(const FString& Parameters)
{
	// Code plays any cue before its sound exists: with nothing to play into, nothing happens and nothing breaks.
	LooterSound::PlayAt(nullptr, MissingCue, FVector::ZeroVector);
	LooterSound::PlayAttached(MissingCue, nullptr);
	LooterSound::Play2D(nullptr, MissingCue);
	TestNull(TEXT("Nothing to start in no world"), LooterSound::Start(nullptr, LoopCue));
	LooterSound::Stop(nullptr);

	// The tests' worlds (and the editor's) never play: only a game's own do.
	FTestWorldWrapper WorldWrapper;
	if (TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		UWorld* World = WorldWrapper.GetTestWorld();
		TestFalse(TEXT("A preview world can't play sounds"), ULooterSoundSubsystem::CanPlayIn(World));
		TestNull(TEXT("...so there's no sound system for it"), ULooterSoundSubsystem::Find(World));
		LooterSound::Play2D(World, LooterSoundCue::Click);
		LooterSound::PlayAt(World, LooterSoundCue::RifleFire, FVector::ZeroVector);
		TestNull(TEXT("...and nothing starts in it"), LooterSound::Start(World, LooterSoundCue::LowHealth));
	}
	TestFalse(TEXT("No world can't play"), ULooterSoundSubsystem::CanPlayIn(nullptr));

	// A cue the bank doesn't have, or has with no sounds, gives nothing to play, and says so once (not on every play).
	ULooterSoundSubsystem* Sounds = MakeTestSounds();
	AddExpectedMessagePlain(MissingCue.ToString(), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);
	AddExpectedMessagePlain(EmptyCue.ToString(), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);
	for (int32 Try = 0; Try < 3; ++Try)
	{
		TestFalse(TEXT("A missing cue has nothing to play"), Sounds->Prepare(MissingCue).IsValid());
		TestFalse(TEXT("A cue with no sounds has nothing to play"), Sounds->Prepare(EmptyCue).IsValid());
	}
	TestFalse(TEXT("A missing cue has no sounds"), Sounds->HasSounds(MissingCue));
	TestFalse(TEXT("An empty one neither"), Sounds->HasSounds(EmptyCue));
	TestTrue(TEXT("A cue with sounds has them"), Sounds->HasSounds(RealCue));
	TestFalse(TEXT("Silent at no volume"), Sounds->Prepare(RealCue, 0.f).IsValid());

	// A loop through a one-shot would never stop: refused, said once.
	AddExpectedMessagePlain(TEXT("is a loop"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);
	Sounds->PlayAt(nullptr, LoopCue, FVector::ZeroVector, 1.f, 1.f);
	Sounds->PlayAt(nullptr, LoopCue, FVector::ZeroVector, 1.f, 1.f);

	// No bank at all (before the script has made it): nothing plays, and the one warning was the game's as it began.
	Sounds->SetBank(nullptr);
	TestFalse(TEXT("No bank: nothing to play"), Sounds->Prepare(RealCue).IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAudioVariationTest, "Looter.Audio.VariationNeverRepeats",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FAudioVariationTest::RunTest(const FString& Parameters)
{
	// The rule: any of them the first time, never the last one again while there's another, each of the others reachable.
	TestEqual(TEXT("None to pick from"), LooterSoundRules::PickVariation(0, INDEX_NONE, 0.5f), static_cast<int32>(INDEX_NONE));
	TestEqual(TEXT("One sound always plays"), LooterSoundRules::PickVariation(1, 0, 0.9f), 0);
	TestEqual(TEXT("The first pick: low roll"), LooterSoundRules::PickVariation(3, INDEX_NONE, 0.f), 0);
	TestEqual(TEXT("The first pick: top roll"), LooterSoundRules::PickVariation(3, INDEX_NONE, 1.f), 2);
	TestEqual(TEXT("Two alternate"), LooterSoundRules::PickVariation(2, 0, 0.3f), 1);
	TestEqual(TEXT("...both ways"), LooterSoundRules::PickVariation(2, 1, 0.99f), 0);
	for (int32 Last = 0; Last < 4; ++Last)
	{
		TSet<int32> Seen;
		for (int32 Step = 0; Step <= 100; ++Step)
		{
			const int32 Pick = LooterSoundRules::PickVariation(4, Last, Step / 100.f);
			Seen.Add(Pick);
			if (Pick == Last || Pick < 0 || Pick >= 4)
			{
				AddError(FString::Printf(TEXT("After %d, a roll of %.2f picked %d"), Last, Step / 100.f, Pick));
			}
		}
		TestEqual(FString::Printf(TEXT("After %d every other one comes up"), Last), Seen.Num(), 3);
	}

	// Played through the sound system: hundreds of plays, never the same sound twice in a row, all of them heard.
	ULooterSoundSubsystem* Sounds = MakeTestSounds();
	const USoundWave* Previous = nullptr;
	TSet<const USoundWave*> Heard;
	int32 Repeats = 0;
	for (int32 Play = 0; Play < 300; ++Play)
	{
		const ULooterSoundSubsystem::FPlay Next = Sounds->Prepare(RealCue, 1.f, 1.f);
		if (!TestTrue(TEXT("The cue plays"), Next.IsValid()))
		{
			return false;
		}
		Repeats += Next.Sound == Previous ? 1 : 0;
		Previous = Next.Sound;
		Heard.Add(Next.Sound);
		// Its gain (-6 dB, about a half) and its pitch within its jitter.
		if (!FMath::IsNearlyEqual(Next.Volume, LooterSoundRules::DbToGain(-6.f), 1.e-4f) || Next.Pitch < 0.95f - 1.e-4f || Next.Pitch > 1.05f + 1.e-4f)
		{
			AddError(FString::Printf(TEXT("Play %d: volume %.3f, pitch %.3f"), Play, Next.Volume, Next.Pitch));
		}
	}
	TestEqual(TEXT("Never the same sound twice in a row"), Repeats, 0);
	TestEqual(TEXT("Every variation is heard"), Heard.Num(), 3);
	TestTrue(TEXT("-6 dB is about half"), FMath::IsNearlyEqual(LooterSoundRules::DbToGain(-6.f), 0.501f, 0.001f));
	TestEqual(TEXT("0 dB is as made"), LooterSoundRules::DbToGain(0.f), 1.f);

	// Pitch: a scale passes through, the jitter spreads it either way, and a bigger body sounds lower.
	TestTrue(TEXT("No jitter: the scale"), FMath::IsNearlyEqual(LooterSoundRules::JitteredPitch(0.8f, 0.f, 0.9f), 0.8f, 1.e-4f));
	TestTrue(TEXT("Lowest roll: jitter below"), FMath::IsNearlyEqual(LooterSoundRules::JitteredPitch(1.f, 0.1f, 0.f), 0.9f, 1.e-4f));
	TestTrue(TEXT("Top roll: jitter above"), FMath::IsNearlyEqual(LooterSoundRules::JitteredPitch(1.f, 0.1f, 1.f), 1.1f, 1.e-4f));
	TestEqual(TEXT("Full size speaks as made"), LooterSoundRules::PitchForSize(1.f), 1.f);
	TestTrue(TEXT("The Gravemother (1.8x) is lower"), LooterSoundRules::PitchForSize(1.8f) < 0.8f);
	TestTrue(TEXT("A spiderling (0.45x) is higher"), LooterSoundRules::PitchForSize(0.45f) > 1.4f);
	TestEqual(TEXT("A broken size speaks as made"), LooterSoundRules::PitchForSize(0.f), 1.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAudioConcurrencyTest, "Looter.Audio.ConcurrencyLimit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FAudioConcurrencyTest::RunTest(const FString& Parameters)
{
	// Each cue plays under a concurrency of its own: its limit, a new play stopping the quietest, and no start within the
	// retrigger time (a shotgun's pellets heard as one hit).
	ULooterSoundSubsystem* Sounds = MakeTestSounds();
	const USoundConcurrency* Real = Sounds->GetConcurrency(RealCue);
	const USoundConcurrency* Pair = Sounds->GetConcurrency(PairCue);
	if (!TestNotNull(TEXT("A cue has its concurrency"), Real) || !TestNotNull(TEXT("So does another"), Pair))
	{
		return false;
	}
	TestTrue(TEXT("...each its own"), Real != Pair);
	TestEqual(TEXT("At most the bank's limit at once"), Real->Concurrency.GetMaxCount(), 3);
	TestEqual(TEXT("A limit of 0 still lets one play"), Pair->Concurrency.GetMaxCount(), 1);
	TestTrue(TEXT("One more stops the quietest"), Real->Concurrency.ResolutionRule == EMaxConcurrentResolutionRule::StopQuietest);
	TestEqual(TEXT("Not again within the retrigger time"), Real->Concurrency.RetriggerTime, LooterSoundRules::RetriggerSeconds);
	TestFalse(TEXT("Counted across the world, not per actor"), static_cast<bool>(Real->Concurrency.bLimitToOwner));
	TestNull(TEXT("A missing cue has none"), Sounds->GetConcurrency(MissingCue));
	TestTrue(TEXT("Every play of a cue goes under its concurrency"), Sounds->Prepare(RealCue).Concurrency == Real);

	// A new bank replaces the old one's cues.
	ULooterSoundBank* Other = NewObject<ULooterSoundBank>(GetTransientPackage());
	Other->Cues = { TestEntry(PairCue, 1, 6) };
	Sounds->SetBank(Other);
	TestNull(TEXT("The old bank's cues are gone"), Sounds->GetConcurrency(RealCue));
	const USoundConcurrency* Replaced = Sounds->GetConcurrency(PairCue);
	TestTrue(TEXT("...and the new one's limits hold"), Replaced && Replaced->Concurrency.GetMaxCount() == 6);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAudioVolumeSettingsTest, "Looter.Audio.VolumeSettings",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FAudioVolumeSettingsTest::RunTest(const FString& Parameters)
{
	using Audio = UAudioSettingsSubsystem;

	// A fresh install, and a save from before the setting, play everything as made.
	const ULooterAudioSave* Fresh = GetDefault<ULooterAudioSave>();
	TestTrue(TEXT("A fresh save is full volume"), Fresh->Master == 1.f && Fresh->Effects == 1.f && Fresh->Interface == 1.f && Fresh->Music == 1.f);
	TestEqual(TEXT("No save: Master full"), Audio::VolumesOf(nullptr).Master, 1.f);

	// The sliders run from 0 to 100% in 5% steps; anything else (a hand-edited save) is pulled back in.
	TestEqual(TEXT("Below silent clamps to 0"), Audio::ClampVolume(-0.4f), 0.f);
	TestEqual(TEXT("Past full clamps to 1"), Audio::ClampVolume(3.f), 1.f);
	TestEqual(TEXT("Rounded to the slider's steps"), Audio::ClampVolume(0.537f), 0.55f);
	TestEqual(TEXT("Not a number: the default"), Audio::ClampVolume(std::numeric_limits<float>::quiet_NaN()), Audio::DefaultVolume);

	// Saved and read back as the subsystem does, through a slot of the test's own so the player's setting is never touched.
	const FString Slot = TEXT("AudioSettings_AutomationTest");
	UGameplayStatics::DeleteGameInSlot(Slot, 0);
	TestEqual(TEXT("No save yet: the defaults"), Audio::VolumesOf(Audio::LoadAudio(Slot, nullptr)).Effects, 1.f);
	ULooterAudioSave* Written = NewObject<ULooterAudioSave>();
	Audio::SetVolumeOf(*Written, EAudioVolume::Master, 0.8f);
	Audio::SetVolumeOf(*Written, EAudioVolume::Effects, 0.5f);
	Audio::SetVolumeOf(*Written, EAudioVolume::Interface, 0.25f);
	Audio::SetVolumeOf(*Written, EAudioVolume::Music, 0.f);
	if (!TestTrue(TEXT("Saved"), UGameplayStatics::SaveGameToSlot(Written, Slot, 0)))
	{
		return false;
	}
	const ULooterAudioSave* ReadBack = Audio::LoadAudio(Slot, nullptr);
	TestTrue(TEXT("Read back a new copy"), ReadBack && ReadBack != Written);
	const FLooterVolumes Volumes = Audio::VolumesOf(ReadBack);
	TestEqual(TEXT("...Master"), Volumes.Master, 0.8f);
	TestEqual(TEXT("...Effects"), Volumes.Effects, 0.5f);
	TestEqual(TEXT("...Interface"), Volumes.Interface, 0.25f);
	TestEqual(TEXT("...Music"), Volumes.Music, 0.f);
	Written->Effects = 7.f;
	UGameplayStatics::SaveGameToSlot(Written, Slot, 0);
	TestEqual(TEXT("A save past full reads as full"), Audio::VolumesOf(Audio::LoadAudio(Slot, nullptr)).Effects, 1.f);
	UGameplayStatics::DeleteGameInSlot(Slot, 0);

	// Applied: each class's gain, squared as hearing works (half way is a quarter of the gain), the world's ambience with
	// the effects, Master on the class above them all.
	const FLooterClassGains Gains = LooterSoundRules::ClassGains(Volumes);
	TestTrue(TEXT("Master's gain"), FMath::IsNearlyEqual(Gains.Master, 0.64f, 1.e-4f));
	TestTrue(TEXT("Effects' gain"), FMath::IsNearlyEqual(Gains.Effects, 0.25f, 1.e-4f));
	TestTrue(TEXT("Ambience follows Effects"), FMath::IsNearlyEqual(Gains.Ambience, Gains.Effects, 1.e-4f));
	TestTrue(TEXT("Interface's gain"), FMath::IsNearlyEqual(Gains.Interface, 0.0625f, 1.e-4f));
	TestEqual(TEXT("Music off"), Gains.Music, 0.f);
	TestEqual(TEXT("Full is as made"), LooterSoundRules::SliderToGain(1.f), 1.f);

	// The sound system keeps them for every world of the game as it starts (with no world now, it only keeps them).
	ULooterSoundSubsystem* Sounds = MakeTestSounds();
	Sounds->SetVolumes(Volumes);
	TestEqual(TEXT("The sound system keeps Master"), Sounds->GetVolumes().Master, 0.8f);
	TestEqual(TEXT("...and Music"), Sounds->GetVolumes().Music, 0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAudioSurfacesTest, "Looter.Audio.Surfaces",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FAudioSurfacesTest::RunTest(const FString& Parameters)
{
	// The project's materials by name.
	const TPair<const TCHAR*, ESoundSurface> Names[] = {
		{ TEXT("MI_WoodPlanks"), ESoundSurface::Wood },
		{ TEXT("MI_RockGranite_Ransom"), ESoundSurface::Stone },
		{ TEXT("MI_RockCliffDen"), ESoundSurface::Stone },
		{ TEXT("MI_StoneWall"), ESoundSurface::Stone },
		{ TEXT("MI_SkyIslandGrass"), ESoundSurface::Grass },
		{ TEXT("MI_Hay"), ESoundSurface::Grass },
		{ TEXT("MI_GroundDirt"), ESoundSurface::Dirt },
		{ TEXT("MI_DenFloor"), ESoundSurface::Dirt },
		{ TEXT("MI_RansomsRestMacro_Steep"), ESoundSurface::Dirt },
		{ TEXT("MI_MetalRust"), ESoundSurface::Metal },
		{ TEXT("MI_Water"), ESoundSurface::Water },
	};
	for (const TPair<const TCHAR*, ESoundSurface>& Name : Names)
	{
		ESoundSurface Surface = ESoundSurface::Dirt;
		TestTrue(FString::Printf(TEXT("%s says a surface"), Name.Key), SoundSurface::FromName(Name.Key, Surface));
		TestTrue(FString::Printf(TEXT("%s is the right one"), Name.Key), Surface == Name.Value);
	}
	ESoundSurface Surface = ESoundSurface::Dirt;
	TestFalse(TEXT("A terrain's macro material says nothing (its slope decides)"), SoundSurface::FromName(TEXT("MI_RansomsRestMacro"), Surface));
	TestTrue(TEXT("A tag says it outright"), SoundSurface::FromTag(TEXT("Surface.Wood"), Surface) && Surface == ESoundSurface::Wood);
	TestFalse(TEXT("Any other tag says nothing"), SoundSurface::FromTag(TEXT("Ground"), Surface));

	// Every surface has a footstep; metal rings like stone, water is dirt until it has a splash.
	TestEqual(TEXT("Grass"), SoundSurface::FootstepCue(ESoundSurface::Grass), FName(LooterSoundCue::FootstepGrass));
	TestEqual(TEXT("Wood"), SoundSurface::FootstepCue(ESoundSurface::Wood), FName(LooterSoundCue::FootstepWood));
	TestEqual(TEXT("Metal"), SoundSurface::FootstepCue(ESoundSurface::Metal), FName(LooterSoundCue::FootstepStone));
	TestEqual(TEXT("Water"), SoundSurface::FootstepCue(ESoundSurface::Water), FName(LooterSoundCue::FootstepDirt));

	// The steps' pace and weight: longer strides and louder steps the faster the player goes; a hop lands as a step.
	using Player = UPlayerSoundComponent;
	TestTrue(TEXT("Sprint strides are longer than the jog's"), Player::StrideLength(790.f) > Player::StrideLength(510.f));
	TestTrue(TEXT("A jog steps about three times a second"), FMath::IsWithin(510.f / Player::StrideLength(510.f), 2.8f, 3.6f));
	TestTrue(TEXT("Crept steps are quieter"), Player::StepVolume(150.f) < Player::StepVolume(790.f));
	TestEqual(TEXT("A hop's landing is only a step"), Player::LandVolume(200.f), 0.f);
	TestTrue(TEXT("A hard landing is loud"), Player::LandVolume(1200.f) > 0.9f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAudioBankCoversCuesTest, "Looter.Audio.BankCoversCues",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FAudioBankCoversCuesTest::RunTest(const FString& Parameters)
{
	TArray<FName> Cues;
	if (!ReadHeaderCues(Cues))
	{
		AddWarning(TEXT("Audio/LooterSoundCues.h isn't readable here (no source): skipped."));
		return true;
	}
	TestTrue(TEXT("LooterSoundCues.h names the game's cues"), Cues.Num() >= 40);
	const ULooterSoundBank* Bank = ULooterSoundBank::Load();
	if (!Bank)
	{
		AddWarning(TEXT("No sound bank yet (/Game/Audio/DA_SoundBank): run Tools/Unreal/build_sound_bank.py. Skipped."));
		return true;
	}

	// Every cue the code plays has its sounds, set to play as the code expects.
	for (const FName& Cue : Cues)
	{
		const FLooterSoundCueEntry* Entry = Bank->FindCue(Cue);
		if (!TestNotNull(FString::Printf(TEXT("%s is in the bank"), *Cue.ToString()), Entry))
		{
			continue;
		}
		const bool bAllLoaded = !Entry->Sounds.IsEmpty() && !Entry->Sounds.Contains(nullptr);
		TestTrue(FString::Printf(TEXT("%s has its sounds"), *Cue.ToString()), bAllLoaded);
		TestTrue(FString::Printf(TEXT("%s plays at least one at a time"), *Cue.ToString()), Entry->MaxConcurrent >= 1);
		TestTrue(FString::Printf(TEXT("%s has a sound class (a volume slider)"), *Cue.ToString()), Entry->SoundClass != nullptr);
		if (Entry->bSpatial)
		{
			TestNotNull(FString::Printf(TEXT("%s, placed in the world, fades with distance"), *Cue.ToString()), Entry->Attenuation.Get());
		}
	}
	// The loops the code starts and stops.
	for (const TCHAR* Loop : { LooterSoundCue::LowHealth, LooterSoundCue::SlideLoop })
	{
		const FLooterSoundCueEntry* Entry = Bank->FindCue(Loop);
		TestTrue(FString::Printf(TEXT("%s loops"), Loop), Entry && Entry->bLoop);
	}
	// A cue the code never plays is likely a typo in cues.json (or a cue still to add to LooterSoundCues.h).
	for (const FLooterSoundCueEntry& Entry : Bank->Cues)
	{
		if (!Cues.Contains(Entry.Cue))
		{
			AddWarning(FString::Printf(TEXT("The bank's %s isn't in LooterSoundCues.h: nothing plays it."), *Entry.Cue.ToString()));
		}
	}
	return true;
}

#endif

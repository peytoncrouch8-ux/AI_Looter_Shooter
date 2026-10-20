#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Bestiary/BestiaryTiers.h"
#include "Combat/TargetDummy.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/CreatureRankSettings.h"
#include "Creatures/SpiderCreature.h"
#include "Progression/LooterProgressSave.h"
#include "Progression/PlayerProgressionSubsystem.h"
#include "Session/SessionSave.h"
#include "UI/Style/LooterUIStyle.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Tests/AutomationCommon.h"
#include "UObject/StrongObjectPtr.h"

// The bestiary counts defeats by enemy tier (2026-10-20; Docs/Pipeline.md, "Enemy tiers"): the count kept per tier as the
// creature stood when it died, what a save keeps (and an older save, which has no split), and the line under a page's
// defeat count. Looter.Tiers.* checks the tiers themselves.

namespace
{
	const FString SpiderKind = TEXT("/Script/AI_Looter_Shooter.SpiderCreature");

	ASpiderCreature* PlaceSpider(UWorld* World, ECreatureRank Rank, int32 Index)
	{
		FActorSpawnParameters Params;
		Params.Name = FName(*FString::Printf(TEXT("BestiarySpider_%d"), Index));
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		ASpiderCreature* Spider = World->SpawnActor<ASpiderCreature>(FVector(2000.0 * Index, 0.0, 0.0), FRotator::ZeroRotator, Params);
		if (Spider)
		{
			Spider->StartingRank = Rank;
			Spider->DispatchBeginPlay();
		}
		return Spider;
	}

	/** A progression of a local player of the test's own that never joins the game: nothing here saves. */
	TStrongObjectPtr<UPlayerProgressionSubsystem> MakeProgression(TStrongObjectPtr<ULocalPlayer>& OutPlayer)
	{
		OutPlayer = TStrongObjectPtr<ULocalPlayer>(GEngine ? NewObject<ULocalPlayer>(GEngine) : nullptr);
		return OutPlayer.IsValid() ? TStrongObjectPtr<UPlayerProgressionSubsystem>(NewObject<UPlayerProgressionSubsystem>(OutPlayer.Get()))
			: TStrongObjectPtr<UPlayerProgressionSubsystem>();
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBestiaryTierCountsTest, "Looter.Bestiary.TierCounts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBestiaryTierCountsTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper WorldWrapper;
	TStrongObjectPtr<ULocalPlayer> Player;
	const TStrongObjectPtr<UPlayerProgressionSubsystem> Progression = MakeProgression(Player);
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview))
		|| !TestTrue(TEXT("A player's progression to count on"), Progression.IsValid()))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	const UClass* Spiders = ASpiderCreature::StaticClass();

	// One spider of each tier; each defeat counts in the total and at the tier the creature held as it died.
	struct FKill
	{
		ECreatureRank Rank;
		int32 Defeats;
	};
	const FKill Kills[] = { { ECreatureRank::Basic, 2 }, { ECreatureRank::Rare, 3 }, { ECreatureRank::Epic, 1 }, { ECreatureRank::Hallowed, 1 } };
	int32 Index = 0;
	int32 Total = 0;
	for (const FKill& Kill : Kills)
	{
		const ASpiderCreature* Spider = PlaceSpider(World, Kill.Rank, Index++);
		if (!TestNotNull(TEXT("A spider placed at a tier"), Spider) || !TestTrue(TEXT("...holding it"), Spider->GetRank() == Kill.Rank))
		{
			return false;
		}
		for (int32 Count = 0; Count < Kill.Defeats; ++Count)
		{
			Progression->RecordDefeat(Spider);
		}
		Total += Kill.Defeats;
	}
	FTierDefeats Tiers = Progression->GetDefeatedByTier(Spiders);
	TestEqual(TEXT("The total counts every defeat"), Progression->GetDefeated(Spiders), Total);
	TestEqual(TEXT("Regular"), Tiers.Count(ECreatureRank::Basic), 2);
	TestEqual(TEXT("Restless"), Tiers.Count(ECreatureRank::Rare), 3);
	TestEqual(TEXT("Gravebound"), Tiers.Count(ECreatureRank::Epic), 1);
	TestEqual(TEXT("Hallowed"), Tiers.Count(ECreatureRank::Hallowed), 1);
	TestEqual(TEXT("Ethereal: none yet"), Tiers.Count(ECreatureRank::Ethereal), 0);

	// The tier is the one it held when it died: a promotion lasts until the death, so a Regular spider promoted in play counts
	// where it stood.
	ASpiderCreature* Promoted = PlaceSpider(World, ECreatureRank::Basic, Index++);
	if (!TestNotNull(TEXT("A Regular spider"), Promoted))
	{
		return false;
	}
	Promoted->SetRank(ECreatureRank::Ethereal);
	Progression->RecordDefeat(Promoted);
	++Total;
	Tiers = Progression->GetDefeatedByTier(Spiders);
	TestEqual(TEXT("Promoted to Ethereal before it died: an Ethereal defeat"), Tiers.Count(ECreatureRank::Ethereal), 1);
	TestEqual(TEXT("...and not a Regular one"), Tiers.Count(ECreatureRank::Basic), 2);

	// A Legendary monster is no tier (it keeps its page and its lair): the total has it, the tiers don't.
	ASpiderCreature* Soulfed = PlaceSpider(World, ECreatureRank::Basic, Index++);
	if (!TestNotNull(TEXT("A spider to make Legendary"), Soulfed))
	{
		return false;
	}
	Soulfed->SetRank(ECreatureRank::Legendary);
	Progression->RecordDefeat(Soulfed);
	++Total;
	Tiers = Progression->GetDefeatedByTier(Spiders);
	TestEqual(TEXT("The total counts the Legendary monster too"), Progression->GetDefeated(Spiders), Total);
	int32 InTiers = 0;
	for (const ECreatureRank Tier : LooterRanks::Tiers)
	{
		InTiers += Tiers.Count(Tier);
	}
	TestEqual(TEXT("...the tiers don't: they hold the other eight"), InTiers, Total - 1);
	TestFalse(TEXT("A boss is no tier either"), LooterRanks::IsTier(ECreatureRank::Boss));

	// Anything that isn't a creature (a target dummy) counts in the total only.
	const ATargetDummy* Dummy = GetDefault<ATargetDummy>();
	Progression->RecordDefeat(Dummy);
	TestEqual(TEXT("A dummy counts as one defeat"), Progression->GetDefeated(ATargetDummy::StaticClass()), 1);
	TestEqual(TEXT("...at no tier"), Progression->GetDefeatedByTier(ATargetDummy::StaticClass()).ByTier.Num(), 0);
	TestEqual(TEXT("A kind never defeated has none"), Progression->GetDefeatedByTier(AActor::StaticClass()).ByTier.Num(), 0);

	// Forgetting the bestiary clears the split with the counts.
	Progression->ForgetBestiary();
	TestEqual(TEXT("Forgotten: no defeats"), Progression->GetDefeated(Spiders), 0);
	TestEqual(TEXT("...no tiers"), Progression->GetDefeatedByTier(Spiders).ByTier.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBestiaryTierSaveTest, "Looter.Bestiary.TierSave",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBestiaryTierSaveTest::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<ULocalPlayer> Player;
	const TStrongObjectPtr<UPlayerProgressionSubsystem> Progression = MakeProgression(Player);
	if (!TestTrue(TEXT("A player's progression to count on"), Progression.IsValid()))
	{
		return false;
	}

	// A session keeps the split with the counts, and the version stays 3 (the field is new within it).
	ULooterSessionSave* Save = NewObject<ULooterSessionSave>();
	Save->Version = ULooterSessionSave::CurrentVersion;
	Save->Progress.Defeated.Add(SpiderKind, 11);
	FTierDefeats& Split = Save->Progress.DefeatedByTier.Add(SpiderKind);
	Split.ByTier.Add(ECreatureRank::Basic, 6);
	Split.ByTier.Add(ECreatureRank::Rare, 3);
	Split.ByTier.Add(ECreatureRank::Epic, 1);
	Split.ByTier.Add(ECreatureRank::Hallowed, 1);
	TArray<uint8> Bytes;
	if (!TestTrue(TEXT("The save writes"), UGameplayStatics::SaveGameToMemory(Save, Bytes)))
	{
		return false;
	}
	ULooterSessionSave* Read = Cast<ULooterSessionSave>(UGameplayStatics::LoadGameFromMemory(Bytes));
	if (!TestNotNull(TEXT("...and reads back"), Read))
	{
		return false;
	}
	TestEqual(TEXT("Still version 3: the split adds no version"), Read->Upgrade(), ULooterSessionSave::CurrentVersion);
	const FTierDefeats* Back = Read->Progress.DefeatedByTier.Find(SpiderKind);
	if (!TestNotNull(TEXT("The split is kept"), Back))
	{
		return false;
	}
	TestEqual(TEXT("Total kept"), Read->Progress.Defeated.FindRef(SpiderKind), 11);
	TestTrue(TEXT("Every tier's count kept"), Back->Count(ECreatureRank::Basic) == 6 && Back->Count(ECreatureRank::Rare) == 3
		&& Back->Count(ECreatureRank::Epic) == 1 && Back->Count(ECreatureRank::Hallowed) == 1 && Back->Count(ECreatureRank::Ethereal) == 0);

	// A save from before the tiers has totals and no split (what a save without the field reads as): the total stays right, no
	// tier is guessed (some of those defeats were Restless or Gravebound, which nobody recorded), and counting goes on from
	// there.
	ULooterSessionSave* Old = NewObject<ULooterSessionSave>();
	Old->Version = ULooterSessionSave::CurrentVersion;
	Old->Progress.Defeated.Add(SpiderKind, 9);
	Old->Progress.Encountered.Add(SpiderKind);
	Bytes.Reset();
	if (!TestTrue(TEXT("An older save writes"), UGameplayStatics::SaveGameToMemory(Old, Bytes)))
	{
		return false;
	}
	ULooterSessionSave* OldRead = Cast<ULooterSessionSave>(UGameplayStatics::LoadGameFromMemory(Bytes));
	if (!TestNotNull(TEXT("...and reads back"), OldRead))
	{
		return false;
	}
	OldRead->Upgrade();
	Progression->SetProgress(OldRead->Progress);
	const UClass* Spiders = ASpiderCreature::StaticClass();
	TestEqual(TEXT("An older save: its total is as it was"), Progression->GetDefeated(Spiders), 9);
	TestEqual(TEXT("...with tiers unknown: no split"), Progression->GetDefeatedByTier(Spiders).ByTier.Num(), 0);
	TestTrue(TEXT("...so its page shows no tier line"), BestiaryTiers::Pieces(Progression->GetDefeatedByTier(Spiders)).IsEmpty());

	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	const ASpiderCreature* Restless = PlaceSpider(WorldWrapper.GetTestWorld(), ECreatureRank::Rare, 0);
	if (!TestNotNull(TEXT("A Restless spider"), Restless))
	{
		return false;
	}
	Progression->RecordDefeat(Restless);
	TestEqual(TEXT("A defeat after the older save: the total goes on from its count"), Progression->GetDefeated(Spiders), 10);
	TestEqual(TEXT("...and the split starts"), Progression->GetDefeatedByTier(Spiders).Count(ECreatureRank::Rare), 1);

	// The save from before sessions has no split either, and becomes a session's progress with tiers unknown.
	ULooterProgressSave* Legacy = NewObject<ULooterProgressSave>();
	Legacy->Version = 2;
	Legacy->Defeated.Add(SpiderKind, 3);
	const FPlayerProgressData LegacyProgress = Legacy->ToProgress();
	TestEqual(TEXT("The pre-session save's total"), LegacyProgress.Defeated.FindRef(SpiderKind), 3);
	TestTrue(TEXT("...with no split"), LegacyProgress.DefeatedByTier.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBestiaryTierLineTest, "Looter.Bestiary.TierLine",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBestiaryTierLineTest::RunTest(const FString& Parameters)
{
	// The line under the defeat count: the tiers met above Regular, lowest first, in the tag's words and colours.
	FTierDefeats Defeats;
	Defeats.ByTier.Add(ECreatureRank::Hallowed, 1);
	Defeats.ByTier.Add(ECreatureRank::Basic, 12);
	Defeats.ByTier.Add(ECreatureRank::Epic, 1);
	Defeats.ByTier.Add(ECreatureRank::Rare, 3);
	const TArray<BestiaryTiers::FTierCount> Pieces = BestiaryTiers::Pieces(Defeats);
	TestEqual(TEXT("The words"), BestiaryTiers::Words(Pieces), FString(TEXT("Restless 3 · Gravebound 1 · Hallowed 1")));
	TestEqual(TEXT("Regular is left to the count above; Ethereal wasn't met: three pieces"), Pieces.Num(), 3);
	if (Pieces.Num() != 3)
	{
		return false;
	}
	TestTrue(TEXT("Lowest tier first, whatever the order they were counted"),
		Pieces[0].Tier == ECreatureRank::Rare && Pieces[1].Tier == ECreatureRank::Epic && Pieces[2].Tier == ECreatureRank::Hallowed);
	TestEqual(TEXT("Its counts"), Pieces[0].Count + Pieces[1].Count + Pieces[2].Count, 5);

	// Each piece is the tag's word in the tag's colour.
	TestTrue(TEXT("Restless in blue"), Pieces[0].Color.Equals(LooterUI::Color::TierRestless(), 0.001f));
	TestTrue(TEXT("Gravebound in purple"), Pieces[1].Color.Equals(LooterUI::Color::TierGravebound(), 0.001f));
	TestTrue(TEXT("Hallowed in saint-gold"), Pieces[2].Color.Equals(LooterUI::Color::TierHallowed(), 0.001f));
	for (const BestiaryTiers::FTierCount& Piece : Pieces)
	{
		TestEqual(FString::Printf(TEXT("%s: the word its tag shows"), *Piece.Word), Piece.Word, UCreatureRankSettings::Get(Piece.Tier).Word.ToString());
	}

	// The small mark is for the two top tiers, whose first defeat is an event.
	TestTrue(TEXT("No mark on Restless or Gravebound, one on Hallowed"), !Pieces[0].bMark && !Pieces[1].bMark && Pieces[2].bMark);
	Defeats.ByTier.Add(ECreatureRank::Ethereal, 2);
	const TArray<BestiaryTiers::FTierCount> WithEthereal = BestiaryTiers::Pieces(Defeats);
	TestEqual(TEXT("With an Ethereal one too"), BestiaryTiers::Words(WithEthereal), FString(TEXT("Restless 3 · Gravebound 1 · Hallowed 1 · Ethereal 2")));
	TestTrue(TEXT("Ethereal in red, marked"), WithEthereal.Num() == 4 && WithEthereal[3].Color.Equals(LooterUI::Color::TierEthereal(), 0.001f)
		&& WithEthereal[3].bMark);

	// Nothing met above Regular, or nothing at all: no line.
	FTierDefeats OnlyRegular;
	OnlyRegular.ByTier.Add(ECreatureRank::Basic, 40);
	TestTrue(TEXT("Only Regular defeats: no line"), BestiaryTiers::Pieces(OnlyRegular).IsEmpty() && BestiaryTiers::Words(BestiaryTiers::Pieces(OnlyRegular)).IsEmpty());
	TestTrue(TEXT("Nothing defeated: no line"), BestiaryTiers::Words(BestiaryTiers::Pieces(FTierDefeats())).IsEmpty());
	return true;
}

#endif

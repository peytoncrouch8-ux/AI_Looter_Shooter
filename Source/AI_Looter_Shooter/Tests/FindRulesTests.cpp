#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Session/CampaignRecord.h"
#include "World/FindRules.h"
#include "Math/RandomStream.h"

// The finds' plain rules (Docs/Work/2026-10-28-Core/FINDS.md §3): the saved flag, the experience table, the tally (Lore kept
// apart, regions in the map's order), the banner's words and the glint's timing. The world's side: FindWorldTests.cpp.

namespace
{
	FFindEntry Entry(const TCHAR* Id, const TCHAR* Region, EFindKind Kind)
	{
		FFindEntry Made;
		Made.Id = FName(Id);
		Made.Name = FText::FromString(Id);
		Made.Region = FText::FromString(Region);
		Made.Kind = Kind;
		return Made;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFindRulesTest, "Looter.Finds.Rules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFindRulesTest::RunTest(const FString& Parameters)
{
	// Flags: Find.<Id>, saved in the story flags; none for none.
	TestEqual(TEXT("Flag"), FindRules::FlagFor(TEXT("WidowsBox")), FName(TEXT("Find.WidowsBox")));
	TestTrue(TEXT("No flag for no id"), FindRules::FlagFor(NAME_None).IsNone());

	// Experience: the base table at zone 1, then x (1 + 0.35 x (zone - 1)); a zone under 1 reads as 1.
	TestEqual(TEXT("Cache 100"), FindRules::XpFor(EFindKind::Cache, 1), int64(100));
	TestEqual(TEXT("Grave 160"), FindRules::XpFor(EFindKind::Grave, 1), int64(160));
	TestEqual(TEXT("Hidden 220"), FindRules::XpFor(EFindKind::Hidden, 1), int64(220));
	TestEqual(TEXT("Elite 300"), FindRules::XpFor(EFindKind::Elite, 1), int64(300));
	TestEqual(TEXT("Lore 40"), FindRules::XpFor(EFindKind::Lore, 1), int64(40));
	TestEqual(TEXT("Cache at zone 5: 100 x 2.4"), FindRules::XpFor(EFindKind::Cache, 5), int64(240));
	TestEqual(TEXT("Hidden at zone 6: 220 x 2.75"), FindRules::XpFor(EFindKind::Hidden, 6), int64(605));
	TestEqual(TEXT("Zone 0 reads as 1"), FindRules::XpFor(EFindKind::Grave, 0), int64(160));
	TestEqual(TEXT("Completion 1,000"), FindRules::CompletionXP, int64(1000));
	TestFalse(TEXT("Lore isn't one of the finds"), FindRules::CountsInFinds(EFindKind::Lore));
	TestTrue(TEXT("An elite is"), FindRules::CountsInFinds(EFindKind::Elite));

	// The tally: the finds (Lore apart), per region in the map's order whatever order the spots came in, unknown regions last.
	const TArray<FFindEntry> Entries = {
		Entry(TEXT("Tarn"), TEXT("The Mountain"), EFindKind::Hidden),
		Entry(TEXT("Mooring"), TEXT("Farm & Point"), EFindKind::Cache),
		Entry(TEXT("Odd"), TEXT("Somewhere Else"), EFindKind::Cache),
		Entry(TEXT("WidowsBox"), TEXT("Farm & Point"), EFindKind::Cache),
		Entry(TEXT("Chimney"), TEXT("West Road"), EFindKind::Cache),
		Entry(TEXT("PageOne"), TEXT("Town & Boot Hill"), EFindKind::Lore),
		Entry(TEXT("PageTwo"), TEXT("West Road"), EFindKind::Lore),
	};
	FCampaignRecord Record;
	Record.SetFlag(FindRules::FlagFor(TEXT("WidowsBox")));
	Record.SetFlag(FindRules::FlagFor(TEXT("Mooring")));
	Record.SetFlag(FindRules::FlagFor(TEXT("PageTwo")));
	const FFindTally Tally = FindRules::Tally(Entries, Record);
	TestEqual(TEXT("Five finds, Lore apart"), Tally.Total, 5);
	TestEqual(TEXT("Two found"), Tally.Found, 2);
	TestFalse(TEXT("Not complete"), Tally.IsComplete());
	if (TestEqual(TEXT("Four regions"), Tally.ByRegion.Num(), 4))
	{
		TestEqual(TEXT("Farm & Point first"), Tally.ByRegion[0].Region.ToString(), FString(TEXT("Farm & Point")));
		TestEqual(TEXT("Farm & Point 2 / 2"), Tally.ByRegion[0].Found, 2);
		TestTrue(TEXT("Farm & Point done"), Tally.ByRegion[0].IsDone());
		TestEqual(TEXT("West Road second"), Tally.ByRegion[1].Region.ToString(), FString(TEXT("West Road")));
		TestEqual(TEXT("West Road counts its find, not its page"), Tally.ByRegion[1].Total, 1);
		TestEqual(TEXT("The Mountain third"), Tally.ByRegion[2].Region.ToString(), FString(TEXT("The Mountain")));
		TestEqual(TEXT("An unknown region last"), Tally.ByRegion[3].Region.ToString(), FString(TEXT("Somewhere Else")));
	}
	const FFindTally Lore = FindRules::Tally(Entries, Record, /*bLore*/ true);
	TestEqual(TEXT("Two pages"), Lore.Total, 2);
	TestEqual(TEXT("One page read"), Lore.Found, 1);
	TestEqual(TEXT("Five region rows on the map"), FindRules::RegionOrder().Num(), 5);
	for (const FFindEntry& Each : Entries)
	{
		Record.SetFlag(FindRules::FlagFor(Each.Id));
	}
	TestTrue(TEXT("All found: complete"), FindRules::Tally(Entries, Record).IsComplete());

	// The banner's words.
	const FText Area = FText::FromString(TEXT("Ransom's Rest"));
	TestEqual(TEXT("Count"), FindRules::BannerCount(4, 15).ToString(), FString(TEXT("4 / 15")));
	TestEqual(TEXT("Count line"), FindRules::CountLine(4, 15, Area).ToString(), FString(TEXT("4 / 15 · RANSOM'S REST")));
	TestEqual(TEXT("Count line without an area"), FindRules::CountLine(4, 15, FText::GetEmpty()).ToString(), FString(TEXT("4 / 15")));
	TestEqual(TEXT("Lore line"), FindRules::LoreLine(2, 6).ToString(), FString(TEXT("LEDGER PAGE · 2 / 6")));
	TestEqual(TEXT("Completion"), FindRules::CompleteTitle(Area).ToString(), FString(TEXT("EVERY FIND IN RANSOM'S REST")));
	TestEqual(TEXT("Collapsed"), FindRules::CollapsedTitle(3).ToString(), FString(TEXT("3 FOUND")));
	TestFalse(TEXT("Hob's last word"), FindRules::CompletionWords().IsEmpty());

	// Silver grows with the zone as kill silver does; none stays none.
	TestEqual(TEXT("Silver at zone 1"), FindRules::SilverAt(45, 1), 45);
	TestTrue(TEXT("Silver grows"), FindRules::SilverAt(45, 6) > 45);
	TestEqual(TEXT("No silver"), FindRules::SilverAt(0, 6), 0);

	// The glint: 8-45 m, a clear line and the sun's side; a seeded 3.5-5 s rhythm; a quick flare and a fade inside 0.3 s.
	TestTrue(TEXT("Glints at 20 m"), FindRules::CanGlint(2000.0, true, true));
	TestFalse(TEXT("Not at 5 m"), FindRules::CanGlint(500.0, true, true));
	TestFalse(TEXT("Not at 60 m"), FindRules::CanGlint(6000.0, true, true));
	TestFalse(TEXT("Not through a rock"), FindRules::CanGlint(2000.0, false, true));
	TestFalse(TEXT("Not on the shaded side"), FindRules::CanGlint(2000.0, true, false));
	const FVector SunEast(0.0, 1.0, 0.5);
	TestTrue(TEXT("Player east, sun east: lit"), FindRules::IsSunSide(FVector(0.0, 1000.0, 0.0), SunEast));
	TestFalse(TEXT("Player west, sun east: shaded"), FindRules::IsSunSide(FVector(0.0, -1000.0, 0.0), SunEast));
	TestFalse(TEXT("Sun down: nothing"), FindRules::IsSunSide(FVector(0.0, 1000.0, 0.0), FVector(0.0, 1.0, -0.3)));
	TestTrue(TEXT("Sun overhead: every side"), FindRules::IsSunSide(FVector(0.0, -1000.0, 0.0), FVector::UpVector));
	FRandomStream Random(FindRules::SeedFor(TEXT("WidowsBox")));
	for (int32 Index = 0; Index < 50; ++Index)
	{
		const float Delay = FindRules::NextGlintDelay(Random);
		if (Delay < FindRules::GlintEveryMin || Delay > FindRules::GlintEveryMax)
		{
			AddError(FString::Printf(TEXT("A glint's wait %.2f s is outside 3.5-5 s."), Delay));
			break;
		}
	}
	TestEqual(TEXT("A find's seed is its own, every play"), FindRules::SeedFor(TEXT("WidowsBox")), FindRules::SeedFor(TEXT("WidowsBox")));
	TestNotEqual(TEXT("Two finds' seeds differ"), FindRules::SeedFor(TEXT("WidowsBox")), FindRules::SeedFor(TEXT("Tarn")));
	TestEqual(TEXT("Dark before"), FindRules::GlintEnvelope(-0.01f), 0.f);
	TestEqual(TEXT("Full at a third"), FindRules::GlintEnvelope(FindRules::GlintSeconds / 3.f), 1.f, 0.001f);
	TestTrue(TEXT("Fading after"), FindRules::GlintEnvelope(FindRules::GlintSeconds * 0.8f) < 0.5f);
	TestEqual(TEXT("Dark after"), FindRules::GlintEnvelope(FindRules::GlintSeconds + 0.01f), 0.f);
	TestEqual(TEXT("Two glints at most"), FindRules::MaxGlints, 2);

	// Under water (the tarn's box): a steady breath of light, never out, and smaller close up.
	float Least = 1.f;
	float Most = 0.f;
	for (float Time = 0.f; Time < 4.f; Time += 0.05f)
	{
		const float Pulse = FindRules::UnderGlowPulse(Time, 0.3f);
		Least = FMath::Min(Least, Pulse);
		Most = FMath::Max(Most, Pulse);
	}
	TestTrue(FString::Printf(TEXT("The glow breathes (%.2f..%.2f), never out"), Least, Most),
		Least >= FindRules::UnderGlowLeast - 0.001f && Least < FindRules::UnderGlowLeast + 0.05f && Most > 0.95f && Most <= 1.f);
	TestTrue(TEXT("Each find its own phase"), !FMath::IsNearlyEqual(FindRules::UnderGlowPulse(1.f, 0.f), FindRules::UnderGlowPulse(1.f, 0.25f), 0.01f));
	TestTrue(TEXT("Smaller up close"), FindRules::UnderGlowScale(150.0) < FindRules::UnderGlowScale(800.0));
	TestEqual(TEXT("A glint's size from 12 m"), FindRules::UnderGlowScale(1500.0), 1.f, 0.001f);
	TestTrue(TEXT("Seen from the bank and a dive"), FindRules::UnderGlowRange >= 1500.0);
	return true;
}

#endif

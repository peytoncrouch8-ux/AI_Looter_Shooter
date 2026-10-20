#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Embers/HeroDefinition.h"
#include "Missions/MissionEventObjectives.h"
#include "Story/CaptionSubsystem.h"
#include "Story/HeroLines.h"
#include "Story/HeroLineTable.h"
#include "Story/StoryLine.h"
#include "Tests/HeroLinesTestKit.h"
#include "UI/HUD/HudBossBarWidget.h"
#include "World/ReadableProp.h"
#include "Dom/JsonObject.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Internationalization/Regex.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"

namespace
{
	/** Every {...} in Words, case kept ({Hero} and {HERO} are two). */
	TArray<FString> BracesIn(const FRegexPattern& Pattern, const FString& Words)
	{
		TArray<FString> Found;
		FRegexMatcher Matcher(Pattern, Words);
		while (Matcher.FindNext())
		{
			Found.Add(Matcher.GetCaptureGroup(0));
		}
		return Found;
	}

	bool HasExactly(const TArray<FString>& List, const FString& Wanted)
	{
		return List.ContainsByPredicate([&Wanted](const FString& Each) { return Each.Equals(Wanted, ESearchCase::CaseSensitive); });
	}

	/** The table's row whose Source is exactly Source, or null (without the game's cached lookup). */
	const FHeroLineVariant* RowWithSource(const UHeroLineTable& Table, const FString& Source)
	{
		return Table.Lines.FindByPredicate([&Source](const FHeroLineVariant& Row) { return Row.Source.Equals(Source, ESearchCase::CaseSensitive); });
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHeroLinesTableTest, "Looter.Lines.Table",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FHeroLinesTableTest::RunTest(const FString& Parameters)
{
	// DA_HeroLines as Tools/Unreal/create_hero_lines.py writes it: each of Ellis's texts keyed once (ignoring case, as the
	// lookup does), heroes of their own only from the roster and never Ellis, every variant's tokens filled while the
	// source's own braces ({0}, {Interact}) stay, no family word left for any other hero, and the asset holding what
	// Tools/Unreal/hero_lines.json says now (else the script wasn't run after the last edit).
	HeroLines::ForgetTable();
	const UHeroLineTable* Table = HeroLines::GetTable();
	if (!TestNotNull(TEXT("DA_HeroLines loads (Tools/Unreal/create_hero_lines.py makes it)"), Table))
	{
		return false;
	}
	TestTrue(TEXT("It has rows"), Table->Lines.Num() > 0);

	const HeroLinesTestKit::FFamilyWords Family;
	const FRegexPattern Braces(TEXT("\\{[A-Za-z0-9_]+\\}"));
	const TArray<FName> Others = HeroLinesTestKit::OtherHeroes();
	const TArray<FString>& Tokens = HeroLines::TokenNames();
	TSet<FString> Seen;
	for (const FHeroLineVariant& Row : Table->Lines)
	{
		const FString Name = Row.Id.IsNone() ? Row.Source.Left(40) : Row.Id.ToString();
		if (!TestFalse(FString::Printf(TEXT("%s: has its source"), *Name), Row.Source.IsEmpty()))
		{
			continue;
		}
		TestFalse(FString::Printf(TEXT("%s: its source is no other row's (ignoring case)"), *Name), Seen.Contains(Row.Source));
		Seen.Add(Row.Source);
		for (const TPair<FName, FText>& Own : Row.Heroes)
		{
			TestTrue(FString::Printf(TEXT("%s: %s is a roster hero but Ellis"), *Name, *Own.Key.ToString()), Others.Contains(Own.Key));
		}
		const TArray<FString> SourceBraces = BracesIn(Braces, Row.Source);
		for (const FName Hero : Others)
		{
			FString Words;
			if (!HeroLines::PickWords(Row, Hero, Words) || Words.IsEmpty())
			{
				continue;
			}
			for (const FString& Left : BracesIn(Braces, Words))
			{
				TestTrue(FString::Printf(TEXT("%s for %s: %s is filled, or the source's own"), *Name, *Hero.ToString(), *Left),
					HasExactly(SourceBraces, Left));
			}
			for (const FString& Kept : SourceBraces)
			{
				if (!HasExactly(Tokens, Kept))
				{
					TestTrue(FString::Printf(TEXT("%s for %s: keeps the source's %s"), *Name, *Hero.ToString(), *Kept),
						Words.Contains(Kept, ESearchCase::CaseSensitive));
				}
			}
			const FString Word = Family.FindIn(Words);
			TestTrue(FString::Printf(TEXT("%s for %s: no \"%s\" (Ellis's family) in \"%s\""), *Name, *Hero.ToString(), *Word, *Words.Left(80)),
				Word.IsEmpty());
		}
	}

	// The json is the one place the variants are written: the asset holds every entry but the painted ones, as it says.
	FString Text;
	const FString JsonFile = FPaths::Combine(FPaths::ProjectDir(), TEXT("Tools/Unreal/hero_lines.json"));
	TSharedPtr<FJsonObject> Root;
	if (TestTrue(TEXT("hero_lines.json reads"), FFileHelper::LoadFileToString(Text, *JsonFile))
		&& TestTrue(TEXT("...as JSON"), FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root) && Root.IsValid()))
	{
		const TArray<TSharedPtr<FJsonValue>>* Entries = nullptr;
		if (TestTrue(TEXT("...with its entries"), Root->TryGetArrayField(TEXT("entries"), Entries) && Entries && Entries->Num() > 0))
		{
			int32 Written = 0;
			for (const TSharedPtr<FJsonValue>& Each : *Entries)
			{
				const TSharedPtr<FJsonObject> Entry = Each->AsObject();
				FString Kind;
				if (!Entry.IsValid() || (Entry->TryGetStringField(TEXT("kind"), Kind) && Kind == TEXT("painted")))
				{
					continue;
				}
				++Written;
				FString Id;
				FString Current;
				FString Neutral;
				Entry->TryGetStringField(TEXT("id"), Id);
				Entry->TryGetStringField(TEXT("current"), Current);
				Entry->TryGetStringField(TEXT("neutral"), Neutral);
				const FHeroLineVariant* Row = RowWithSource(*Table, Current);
				if (!TestNotNull(FString::Printf(TEXT("%s is in DA_HeroLines (run create_hero_lines.py)"), *Id), Row))
				{
					continue;
				}
				const FString Other = Neutral.Equals(Current, ESearchCase::CaseSensitive) ? FString() : Neutral;
				TestTrue(FString::Printf(TEXT("%s: its neutral as the json has it (run create_hero_lines.py)"), *Id),
					Row->Other.ToString().Equals(Other, ESearchCase::CaseSensitive));
				const int32 OwnCount = Entry->HasField(TEXT("heroes")) ? Entry->GetObjectField(TEXT("heroes"))->Values.Num() : 0;
				TestEqual(FString::Printf(TEXT("%s: its heroes' own words (run create_hero_lines.py)"), *Id), Row->Heroes.Num(), OwnCount);
			}
			TestEqual(TEXT("The asset has no rows the json no longer has (run create_hero_lines.py)"), Table->Lines.Num(), Written);
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHeroLinesPrecedenceTest, "Looter.Lines.Precedence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FHeroLinesPrecedenceTest::RunTest(const FString& Parameters)
{
	// The precedence on rows made here: Ellis reads the source; a hero their own words; anyone else the neutral; a row
	// Ellis's alone gives nothing (unless a hero has their own); a row only for voices leaves the source to the rest. Then
	// the tokens: {HERO} is the hero's name in capitals ("ODESSA LARK"), {Initialed} "O. Lark", other braces untouched.
	const FName Ellis = UHeroDefinition::DefaultId();
	const FName Odessa(TEXT("Odessa"));
	const FName Crane(TEXT("Crane"));
	const FName Gauge(TEXT("Gauge"));
	const FName Pike(TEXT("Pike"));

	FHeroLineVariant Lantern;
	Lantern.Source = TEXT("Pa's lantern. Where is it?");
	Lantern.Other = FText::FromString(TEXT("The Keeper's lantern, then. Where is it?"));
	Lantern.Heroes.Add(Odessa, FText::FromString(TEXT("The dead keeper's lantern, then. Where's it gone?")));
	TestEqual(TEXT("Ellis reads the source"), HeroLines::Pick(Lantern, Ellis).ToString(), Lantern.Source);
	TestEqual(TEXT("Odessa her own words"), HeroLines::Pick(Lantern, Odessa).ToString(), FString(TEXT("The dead keeper's lantern, then. Where's it gone?")));
	TestEqual(TEXT("Crane the neutral"), HeroLines::Pick(Lantern, Crane).ToString(), FString(TEXT("The Keeper's lantern, then. Where is it?")));

	FHeroLineVariant Alone;
	Alone.Source = TEXT("Pa.");
	Alone.bDropForOthers = true;
	Alone.Heroes.Add(Pike, FText::FromString(TEXT("Mr. Ransom.")));
	TestTrue(TEXT("Ellis's alone: nothing for Crane"), HeroLines::Pick(Alone, Crane).IsEmpty());
	TestEqual(TEXT("...but Pike's own words over it"), HeroLines::Pick(Alone, Pike).ToString(), FString(TEXT("Mr. Ransom.")));
	TestEqual(TEXT("...and Ellis's as written"), HeroLines::Pick(Alone, Ellis).ToString(), FString(TEXT("Pa.")));

	FHeroLineVariant Voices;
	Voices.Source = TEXT("Riding.");
	Voices.Heroes.Add(Gauge, FText::FromString(TEXT("Riding. I weigh nine hundredweight.")));
	FString Words;
	TestFalse(TEXT("A row only for voices: the source stands for Crane"), HeroLines::PickWords(Voices, Crane, Words));
	TestEqual(TEXT("...who reads it"), HeroLines::Pick(Voices, Crane).ToString(), FString(TEXT("Riding.")));
	TestEqual(TEXT("...and Gauge his own"), HeroLines::Pick(Voices, Gauge).ToString(), FString(TEXT("Riding. I weigh nine hundredweight.")));

	TestEqual(TEXT("Initialed: first initial and last word"), HeroLines::Initialed(TEXT("Odessa Lark")), FString(TEXT("O. Lark")));
	TestEqual(TEXT("...of three words too"), HeroLines::Initialed(TEXT("Wendell T. Pike")), FString(TEXT("W. Pike")));
	TestEqual(TEXT("...one word stays"), HeroLines::Initialed(TEXT("Gauge")), FString(TEXT("Gauge")));

	HeroLines::ForgetTable();
	const UHeroDefinition* OdessaHero = UHeroDefinition::Load(Odessa);
	if (!OdessaHero)
	{
		AddInfo(TEXT("No DA_Hero_Odessa (create_ember_assets.py): the tokens fall back to her id."));
	}
	const FString Full = OdessaHero && !OdessaHero->DisplayName.IsEmpty() ? OdessaHero->DisplayName.ToString() : FString(TEXT("Odessa"));
	TestEqual(TEXT("{HERO}: her name in capitals"), HeroLines::FillTokens(TEXT("{HERO}. STOOD UP AT THE LAST."), Odessa),
		Full.ToUpper() + TEXT(". STOOD UP AT THE LAST."));
	if (OdessaHero)
	{
		TestEqual(TEXT("...ODESSA LARK"), HeroLines::FillTokens(TEXT("{HERO}"), Odessa), FString(TEXT("ODESSA LARK")));
		TestEqual(TEXT("{Initialed}: O. Lark"), HeroLines::FillTokens(TEXT("Collector, thereafter: {Initialed}."), Odessa),
			FString(TEXT("Collector, thereafter: O. Lark.")));
		TestEqual(TEXT("{HeroFull}: Odessa Lark"), HeroLines::FillTokens(TEXT("{HeroFull}"), Odessa), FString(TEXT("Odessa Lark")));
	}
	TestEqual(TEXT("{Hero}: the short name"), HeroLines::FillTokens(TEXT("{Hero}"), Odessa), FString(TEXT("Odessa")));
	TestEqual(TEXT("Other braces are left for whoever reads them after"),
		HeroLines::FillTokens(TEXT("{0} isn't for sale. Press {Interact}."), Odessa), FString(TEXT("{0} isn't for sale. Press {Interact}.")));
	TestEqual(TEXT("A painted asset's hero version, by name"),
		HeroLines::HeroVariantPath(TEXT("/Game/Art/Materials/MI_PosterDecal.MI_PosterDecal"), Odessa),
		FString(TEXT("/Game/Art/Materials/MI_PosterDecal_Odessa.MI_PosterDecal_Odessa")));
	TestTrue(TEXT("...and none for Ellis, whose the asset is"), HeroLines::HeroVariantPath(TEXT("/Game/Art/Materials/MI_PosterDecal.MI_PosterDecal"), Ellis).IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHeroLinesWiringTest, "Looter.Lines.Wiring",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FHeroLinesWiringTest::RunTest(const FString& Parameters)
{
	// Through a table made here, where the words reach the screen: the played hero (scoped; Ellis while the tests run
	// otherwise), Resolve by a text's source string, a line's speaker and words (Ellis's "Who?" is hers only in Ellis's
	// mouth), the captions, an objective (its keys after), the boss bar's phase, a readable's painted mesh. No table at all:
	// everything as written.
	const FName Ellis = UHeroDefinition::DefaultId();
	const FName Odessa(TEXT("Odessa"));
	const FName Crane(TEXT("Crane"));
	TestEqual(TEXT("While the tests run: Ellis's words, whatever Looter.Hero set"), HeroLines::GetPlayedHeroId(), Ellis);

	TStrongObjectPtr<UHeroLineTable> Table(NewObject<UHeroLineTable>(GetTransientPackage()));
	auto AddRow = [&Table](const TCHAR* Source, const TCHAR* Other, const TCHAR* Speaker = TEXT("")) -> FHeroLineVariant&
	{
		FHeroLineVariant& Row = Table->Lines.AddDefaulted_GetRef();
		Row.Source = Source;
		Row.Other = FText::FromString(Other);
		Row.Speaker = Speaker;
		return Row;
	};
	AddRow(TEXT("Ellis"), TEXT("{Hero}"));
	AddRow(TEXT("Who?"), TEXT(""), TEXT("Ellis")).Heroes.Add(Odessa, FText::FromString(TEXT("Who's dealing?")));
	AddRow(TEXT("Pa's lantern bail"), TEXT("The Keeper's lantern bail"));
	AddRow(TEXT("Bring Pa's lantern bail ({Interact})."), TEXT("Bring the Keeper's lantern bail ({Interact})."));
	AddRow(TEXT("You brought them here"), TEXT("You rode in with them"));
	const HeroLines::FScopedTable UseTable(Table.Get());
	{
		const HeroLines::FScopedHero AsOdessa(Odessa);
		TestEqual(TEXT("Scoped: Odessa is played"), HeroLines::GetPlayedHeroId(), Odessa);
		TestEqual(TEXT("Resolve: the neutral"), HeroLines::Resolve(FText::FromString(TEXT("Pa's lantern bail"))).ToString(),
			FString(TEXT("The Keeper's lantern bail")));
		TestEqual(TEXT("...found by a LOCTEXT's source string"),
			HeroLines::Resolve(NSLOCTEXT("HeroLinesTest", "Bail", "Pa's lantern bail")).ToString(), FString(TEXT("The Keeper's lantern bail")));
		TestEqual(TEXT("...Ellis's words exactly (case matters)"), HeroLines::Resolve(FText::FromString(TEXT("pa's lantern bail"))).ToString(),
			FString(TEXT("pa's lantern bail")));
		TestEqual(TEXT("...a text with no row as written"), HeroLines::Resolve(FText::FromString(TEXT("Morning, sunshine."))).ToString(),
			FString(TEXT("Morning, sunshine.")));
		TestEqual(TEXT("ResolveString too (the barks)"), HeroLines::ResolveString(TEXT("Pa's lantern bail")), FString(TEXT("The Keeper's lantern bail")));

		const FStoryLine Hers = HeroLines::ResolveLine(FStoryLine::Make(FText::FromString(TEXT("Ellis")), FText::FromString(TEXT("Who?")), 2.f));
		TestEqual(TEXT("The player character's line names the played hero"), Hers.Speaker.ToString(), FString(TEXT("Odessa")));
		TestEqual(TEXT("...in her own words"), Hers.Text.ToString(), FString(TEXT("Who's dealing?")));
		TestEqual(TEXT("...its seconds kept"), Hers.Seconds, 2.f);
		const FStoryLine Hobs = HeroLines::ResolveLine(FStoryLine::Make(FText::FromString(TEXT("Hob")), FText::FromString(TEXT("Who?"))));
		TestEqual(TEXT("Someone else's \"Who?\" stays theirs"), Hobs.Text.ToString(), FString(TEXT("Who?")));

		FTestWorldWrapper TestLevel;
		if (TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
		{
			UCaptionSubsystem* Captions = UCaptionSubsystem::Get(TestLevel.GetTestWorld());
			if (TestNotNull(TEXT("The level has captions"), Captions))
			{
				Captions->Play({ FStoryLine::Make(FText::FromString(TEXT("Ellis")), FText::FromString(TEXT("Who?")), 2.f) });
				const FCaptionEntry* Shown = Captions->GetCurrent();
				TestTrue(TEXT("A caption plays in the played hero's words, her name over them"),
					Shown && Shown->Line.Speaker.ToString() == TEXT("Odessa") && Shown->Line.Text.ToString() == TEXT("Who's dealing?"));
				Captions->Clear();
			}
			AReadableProp* Board = TestLevel.GetTestWorld()->SpawnActor<AReadableProp>();
			UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
			UStaticMesh* Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
			if (TestNotNull(TEXT("A readable spawns"), Board) && TestTrue(TEXT("The engine's shapes load"), Cube && Sphere))
			{
				TestNull(TEXT("A readable with no hero meshes stays as placed"), Board->PickMeshFor(Odessa));
				Board->HeroMeshes.Add(Odessa, Cube);
				Board->OtherMesh = Sphere;
				TestTrue(TEXT("The hero's own carving"), Board->PickMeshFor(Odessa) == Cube);
				TestTrue(TEXT("...any other hero's the one for all but Ellis"), Board->PickMeshFor(Crane) == Sphere);
				TestNull(TEXT("...and Ellis's as placed"), Board->PickMeshFor(Ellis));
			}
		}

		UMissionEventObjective* Objective = NewObject<UMissionEventObjective>(GetTransientPackage());
		Objective->Text = FText::FromString(TEXT("Bring Pa's lantern bail ({Interact})."));
		TestEqual(TEXT("An objective in the hero's words, its keys after"), Objective->GetDisplayText(nullptr),
			FString(TEXT("Bring the Keeper's lantern bail ([Interact]).")));
		TestEqual(TEXT("The boss bar's phase, in the hero's words before the capitals"),
			UHudBossBarWidget::MakePhaseLine(FText::FromString(TEXT("You brought them here")), false, FText::GetEmpty()).ToString(),
			FString(TEXT("YOU RODE IN WITH THEM")));
		{
			const HeroLines::FScopedTable NoTable(nullptr);
			TestEqual(TEXT("No table made: everything as written"), HeroLines::Resolve(FText::FromString(TEXT("Pa's lantern bail"))).ToString(),
				FString(TEXT("Pa's lantern bail")));
		}
		{
			const HeroLines::FScopedHero AsEllis(Ellis);
			TestEqual(TEXT("Scoped again to Ellis: as written"), HeroLines::Resolve(FText::FromString(TEXT("Pa's lantern bail"))).ToString(),
				FString(TEXT("Pa's lantern bail")));
		}
		TestEqual(TEXT("...and back to Odessa"), HeroLines::GetPlayedHeroId(), Odessa);
	}
	TestEqual(TEXT("Out of the scope: Ellis again"), HeroLines::GetPlayedHeroId(), Ellis);
	TestEqual(TEXT("...and Ellis's words"), HeroLines::Resolve(FText::FromString(TEXT("Pa's lantern bail"))).ToString(), FString(TEXT("Pa's lantern bail")));
	return true;
}

#endif

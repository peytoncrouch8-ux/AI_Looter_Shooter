#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Economy/EconomyRules.h"
#include "Economy/SecretBroker.h"
#include "Economy/SecretCatalog.h"
#include "Economy/SecretRules.h"
#include "Economy/SecretSubsystem.h"
#include "Economy/WalletSubsystem.h"
#include "Interaction/InteractionComponent.h"
#include "Interaction/InteractionTypes.h"
#include "Missions/MissionRunner.h"
#include "Session/CampaignRecord.h"
#include "Story/MisterSexton.h"
#include "Tests/MissionTestWorld.h"
#include "UI/Inventory/MapPins.h"
#include "World/FindRules.h"
#include "World/FindSpot.h"
#include "Engine/World.h"
#include "Misc/PackageName.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Package.h"

// Mister Sexton's secrets for silver (Docs/Work/2026-10-31-Wardens/MAINS_2_7.md section 12; lanes/SECRETS.md): the rules
// (prices, the sold flag, a secret's state in a story), the level's catalog as create_secret_assets.py writes it, buying in a
// test level (the wallet, the flag, the rows, the marks), the map's pins, and Sexton's key (trade or talk).

namespace
{
	const FName SecretsTestMainTwo(TEXT("TestMain2"));
	const FName SecretsTestPaid(TEXT("TestPaid"));
	const FName SecretsTestSideThree(TEXT("TestSide3"));
	const FName SecretsTestLairTag(TEXT("Place_TestLair"));

	FSecretOffer SecretsTestOffer(const TCHAR* Id, ESecretKind Kind, std::initializer_list<FName> Finds, int32 Price = 0)
	{
		FSecretOffer Offer;
		Offer.Id = FName(Id);
		Offer.Title = FText::FromString(FString::Printf(TEXT("The %s"), Id));
		Offer.Teaser = FText::FromString(TEXT("Something out there."));
		Offer.Line = FText::FromString(FString::Printf(TEXT("Under the %s, friend."), Id));
		Offer.Region = FText::FromString(TEXT("Farm & Point"));
		Offer.Kind = Kind;
		Offer.Price = Price;
		Offer.FindIds = Finds;
		Offer.AvailableWhen.AfterMissions = { SecretsTestMainTwo };
		return Offer;
	}

	/** A test catalog: open after TestMain2, a quarter off after TestPaid, a box, two pages, a lair, one with no find here. */
	USecretCatalog* SecretsTestCatalog()
	{
		USecretCatalog* Catalog = NewObject<USecretCatalog>(GetTransientPackage(), NAME_None, RF_Transient);
		Catalog->Title = FText::FromString(TEXT("Test Secrets"));
		Catalog->Tab = FText::FromString(TEXT("Mister Sexton"));
		Catalog->OpenWhen.AfterMissions = { SecretsTestMainTwo };
		Catalog->DiscountMission = SecretsTestPaid;
		Catalog->Discount = 0.25f;
		FSecretGreeting Paid;
		Paid.When.AfterMissions = { SecretsTestPaid };
		Paid.Line = FText::FromString(TEXT("Paid in full."));
		FSecretGreeting Plain;
		Plain.Line = FText::FromString(TEXT("Bring the manifest."));
		Catalog->Greetings = { Paid, Plain };
		Catalog->SoldOut = FText::FromString(TEXT("That's all I know."));
		Catalog->Secrets.Add(SecretsTestOffer(TEXT("TestBox"), ESecretKind::Cache, { FName(TEXT("TestBox")) }));
		Catalog->Secrets.Add(SecretsTestOffer(TEXT("TestPages"), ESecretKind::Lore, { FName(TEXT("TestPageA")), FName(TEXT("TestPageB")) }, 60));
		FSecretOffer Lair = SecretsTestOffer(TEXT("TestLair"), ESecretKind::Story, {}, 60);
		Lair.MarkTag = SecretsTestLairTag;
		Lair.DoneWhen.AfterMissions = { SecretsTestSideThree };
		Catalog->Secrets.Add(Lair);
		Catalog->Secrets.Add(SecretsTestOffer(TEXT("TestNowhere"), ESecretKind::Cache, { FName(TEXT("NoSuchFind")) }));
		return Catalog;
	}

	AFindSpot* SecretsTestSpot(UWorld* World, const TCHAR* Id, const FVector& Where)
	{
		AFindSpot* Spot = World->SpawnActor<AFindSpot>(Where, FRotator::ZeroRotator);
		if (Spot)
		{
			Spot->FindId = FName(Id);
			Spot->DisplayName = FText::FromString(Id);
		}
		return Spot;
	}

	/** The row with Id in Rows (kept by the caller: the pointer is into it), or null. */
	const FSecretRow* SecretsTestRow(const TArray<FSecretRow>& Rows, const TCHAR* Id)
	{
		return Rows.FindByPredicate([Id](const FSecretRow& Row) { return Row.Id == FName(Id); });
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSecretsRulesTest, "Looter.Story.Secrets.Rules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSecretsRulesTest::RunTest(const FString& Parameters)
{
	// The prices: the kind's tier at level 1 (§12: tins 40, strongboxes and named guns 100, the Tollman 150), grown with the
	// player's level as the vendors' are, a quarter off once earned, never under 1.
	TestEqual(TEXT("Sold as SecretSold.<Id>"), SecretRules::SoldFlag(TEXT("WidowsBox")), FName(TEXT("SecretSold.WidowsBox")));
	TestTrue(TEXT("...none for none"), SecretRules::SoldFlag(NAME_None).IsNone());
	TestEqual(TEXT("A cache 40"), SecretRules::BasePrice(ESecretKind::Cache), 40);
	TestEqual(TEXT("A strongbox 100"), SecretRules::BasePrice(ESecretKind::Strongbox), 100);
	TestEqual(TEXT("A named gun's find 100"), SecretRules::BasePrice(ESecretKind::Named), 100);
	TestEqual(TEXT("The Tollman 150"), SecretRules::BasePrice(ESecretKind::Elite), 150);
	TestEqual(TEXT("Level 1: the base"), SecretRules::PriceAt(40, 1, 0.f), 40);
	TestEqual(TEXT("A quarter off: 30"), SecretRules::PriceAt(40, 1, 0.25f), 30);
	TestEqual(TEXT("Grown as the vendors' are"), SecretRules::PriceAt(40, 10, 0.f), LooterEconomy::AtLevel(40, 10));
	TestTrue(TEXT("...dearer at level 10"), SecretRules::PriceAt(40, 10, 0.f) > 40);
	TestEqual(TEXT("Never under 1"), SecretRules::PriceAt(1, 1, 0.9f), 1);
	TestEqual(TEXT("Nothing costs nothing"), SecretRules::PriceAt(0, 5, 0.f), 0);
	FSecretOffer Box = SecretsTestOffer(TEXT("TestBox"), ESecretKind::Cache, { FName(TEXT("TestBox")) });
	TestEqual(TEXT("No price of its own: its kind's"), SecretRules::PriceFor(Box, 1, 0.f), 40);
	Box.Price = 55;
	TestEqual(TEXT("...else its own"), SecretRules::PriceFor(Box, 1, 0.f), 55);

	// A secret through a story: not yet, for sale, sold (marked), done once found.
	FCampaignRecord Record;
	TestEqual(TEXT("Before its step: hidden"), SecretRules::StateOf(Box, Record), ESecretState::Hidden);
	TestFalse(TEXT("...not on the list"), SecretRules::IsListed(SecretRules::StateOf(Box, Record)));
	Record.Complete(SecretsTestMainTwo);
	TestEqual(TEXT("After it: for sale"), SecretRules::StateOf(Box, Record), ESecretState::ForSale);
	Record.SetFlag(SecretRules::SoldFlag(Box.Id));
	TestEqual(TEXT("Bought: sold"), SecretRules::StateOf(Box, Record), ESecretState::Sold);
	TestTrue(TEXT("...still on the list (MARKED)"), SecretRules::IsListed(ESecretState::Sold));
	Record.SetFlag(FindRules::FlagFor(TEXT("TestBox")));
	TestEqual(TEXT("Found: done"), SecretRules::StateOf(Box, Record), ESecretState::Done);
	TestFalse(TEXT("...off the list"), SecretRules::IsListed(ESecretState::Done));

	// Found before it's bought: no secret any more.
	FCampaignRecord Early;
	Early.Complete(SecretsTestMainTwo);
	Early.SetFlag(FindRules::FlagFor(TEXT("TestBox")));
	TestEqual(TEXT("Found unbought: done"), SecretRules::StateOf(Box, Early), ESecretState::Done);

	// Pages: done only once every one is found.
	const FSecretOffer Pages = SecretsTestOffer(TEXT("TestPages"), ESecretKind::Lore, { FName(TEXT("TestPageA")), FName(TEXT("TestPageB")) });
	FCampaignRecord Reading;
	Reading.Complete(SecretsTestMainTwo);
	Reading.SetFlag(FindRules::FlagFor(TEXT("TestPageA")));
	TestEqual(TEXT("One page of two found: still for sale"), SecretRules::StateOf(Pages, Reading), ESecretState::ForSale);
	Reading.SetFlag(FindRules::FlagFor(TEXT("TestPageB")));
	TestEqual(TEXT("Both: done"), SecretRules::StateOf(Pages, Reading), ESecretState::Done);

	// A story secret: no finds; its story ends it.
	FSecretOffer Lair = SecretsTestOffer(TEXT("TestLair"), ESecretKind::Story, {});
	Lair.MarkTag = SecretsTestLairTag;
	Lair.DoneWhen.AfterMissions = { SecretsTestSideThree };
	FCampaignRecord Hunting;
	Hunting.Complete(SecretsTestMainTwo);
	TestFalse(TEXT("No finds: never 'found'"), SecretRules::IsFound(Lair, Hunting));
	TestEqual(TEXT("A story secret for sale"), SecretRules::StateOf(Lair, Hunting), ESecretState::ForSale);
	Hunting.Complete(SecretsTestSideThree);
	TestEqual(TEXT("...done with its story"), SecretRules::StateOf(Lair, Hunting), ESecretState::Done);

	// The catalog by the level's name.
	const FString Expected(TEXT("/Game/Data/Secrets/DA_Secrets_RansomsRest.DA_Secrets_RansomsRest"));
	TestEqual(TEXT("The level's catalog"), SecretRules::CatalogPathFor(TEXT("Lvl_RansomsRest")), Expected);
	TestEqual(TEXT("...in play in the editor too"), SecretRules::CatalogPathFor(TEXT("UEDPIE_0_Lvl_RansomsRest")), Expected);
	TestEqual(TEXT("...from a package path"), SecretRules::CatalogPathFor(TEXT("/Game/Maps/Lvl_RansomsRest")), Expected);
	TestFalse(TEXT("Every kind has a word"), SecretRules::KindWord(ESecretKind::Story).IsEmpty() || SecretRules::KindWord(ESecretKind::Named).IsEmpty());
	TestTrue(TEXT("The words name what's marked"), SecretRules::MarkedWords(FText::FromString(TEXT("The Box")), 1).ToString().Contains(TEXT("The Box"))
		&& SecretRules::MarkedWords(FText::FromString(TEXT("The Pages")), 6).ToString().Contains(TEXT("6")));
	TestTrue(TEXT("...and what's short"), SecretRules::ShortWords(40, 12).ToString().Contains(TEXT("40")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSecretsCatalogTest, "Looter.Story.Secrets.Catalog",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSecretsCatalogTest::RunTest(const FString& Parameters)
{
	// The catalog's own rules on a test one: open after its step, his line for the story, the discount after its mission.
	const USecretCatalog* Test = SecretsTestCatalog();
	FCampaignRecord Record;
	TestFalse(TEXT("Shut before Main 2"), Test->IsOpen(Record));
	TestEqual(TEXT("...his plain line"), Test->GreetingFor(Record).ToString(), FString(TEXT("Bring the manifest.")));
	TestEqual(TEXT("...full price"), Test->DiscountFor(Record), 0.f);
	Record.Complete(SecretsTestMainTwo);
	TestTrue(TEXT("Open after"), Test->IsOpen(Record));
	Record.Complete(SecretsTestPaid);
	TestEqual(TEXT("Paid: his paid line first"), Test->GreetingFor(Record).ToString(), FString(TEXT("Paid in full.")));
	TestEqual(TEXT("...a quarter off"), Test->DiscountFor(Record), 0.25f);
	TestNotNull(TEXT("Found by id"), Test->FindOffer(TEXT("TestPages")));
	TestNull(TEXT("...not by another"), Test->FindOffer(TEXT("Nope")));

	// Ransom's Rest's, as create_secret_assets.py writes it from story_texts.SEXTON_SECRETS.
	const FString Path = SecretRules::CatalogPathFor(TEXT("Lvl_RansomsRest"));
	if (!FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(Path)))
	{
		AddWarning(TEXT("DA_Secrets_RansomsRest isn't made yet: run Tools/Unreal/create_secret_assets.py."));
		return true;
	}
	const USecretCatalog* Rest = LoadObject<USecretCatalog>(nullptr, *Path);
	if (!TestNotNull(TEXT("DA_Secrets_RansomsRest loads"), Rest))
	{
		return false;
	}
	TestTrue(TEXT("He trades after Main 2"), Rest->OpenWhen.AfterMissions.Contains(FName(TEXT("Main2"))));
	TestEqual(TEXT("A quarter off after Paid on Delivery"), Rest->DiscountMission, FName(TEXT("PaidOnDelivery")));
	TestEqual(TEXT("...a quarter"), Rest->Discount, 0.25f);
	TestFalse(TEXT("His lines over the list"), Rest->Greetings.IsEmpty() || Rest->SoldOut.IsEmpty());
	TSet<FName> Ids;
	TSet<FName> Marked;
	for (const FSecretOffer& Offer : Rest->Secrets)
	{
		const FString Id = Offer.Id.ToString();
		TestFalse(FString::Printf(TEXT("%s: its own id"), *Id), Ids.Contains(Offer.Id));
		Ids.Add(Offer.Id);
		TestTrue(FString::Printf(TEXT("%s: marks something"), *Id), !Offer.FindIds.IsEmpty() || !Offer.MarkTag.IsNone());
		TestTrue(FString::Printf(TEXT("%s: a story secret ends"), *Id), !Offer.FindIds.IsEmpty() || !Offer.DoneWhen.IsEmpty());
		TestTrue(FString::Printf(TEXT("%s: costs something"), *Id), SecretRules::PriceFor(Offer, 1, 0.f) > 0);
		TestFalse(FString::Printf(TEXT("%s: its words"), *Id), Offer.Title.IsEmpty() || Offer.Teaser.IsEmpty() || Offer.Line.IsEmpty());
		TestTrue(FString::Printf(TEXT("%s: his line fits a caption"), *Id), Offer.Line.ToString().Len() <= 90);
		TestFalse(FString::Printf(TEXT("%s: not on sale from the start"), *Id), Offer.AvailableWhen.IsEmpty());
		Marked.Append(Offer.FindIds);
	}
	// Every one of the Rest's fifteen finds can be bought (story_texts.FINDS), and the Ledger pages.
	const TCHAR* const Finds[] = { TEXT("WidowsBox"), TEXT("Mooring"), TEXT("RangerPost"), TEXT("BoltHole"), TEXT("Chimney"), TEXT("LanternHouse"),
		TEXT("Tollman"), TEXT("Hoard"), TEXT("MiserGrave"), TEXT("Bottle"), TEXT("CoffinRock"), TEXT("PikesCache"), TEXT("Powder"), TEXT("Crate"),
		TEXT("Tarn"), TEXT("LedgerPage_RansomsRest") };
	for (const TCHAR* Find : Finds)
	{
		TestTrue(FString::Printf(TEXT("%s is for sale"), Find), Marked.Contains(FName(Find)));
	}
	if (const FSecretOffer* Tollman = Rest->FindOffer(TEXT("Tollman")))
	{
		TestEqual(TEXT("The Tollman's whereabouts: 150 at level 1"), SecretRules::PriceFor(*Tollman, 1, 0.f), 150);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSecretsBuyTest, "Looter.Story.Secrets.Buy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSecretsBuyTest::RunTest(const FString& Parameters)
{
	// Buying in a test level, as his screen does: the list (for sale, then marked; one with nowhere to mark isn't sold),
	// the purse (short, then paid), the flag that keeps it bought, the marks until found, the discount, a story secret.
	FCampaignRecord Campaign;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	USecretSubsystem* Secrets = USecretSubsystem::Get(World);
	UWalletSubsystem* Wallet = UWalletSubsystem::Get(World);
	AActor* Player = MissionTestWorld::SpawnMarker(World, FVector::ZeroVector);
	AFindSpot* BoxSpot = SecretsTestSpot(World, TEXT("TestBox"), FVector(1000.0, 0.0, 0.0));
	AFindSpot* PageA = SecretsTestSpot(World, TEXT("TestPageA"), FVector(0.0, 1000.0, 0.0));
	AFindSpot* PageB = SecretsTestSpot(World, TEXT("TestPageB"), FVector(0.0, -1000.0, 0.0));
	AActor* Lair = MissionTestWorld::SpawnMarker(World, FVector(-2000.0, 0.0, -500.0), SecretsTestLairTag);
	if (!Runner || !Secrets || !Wallet || !Player || !BoxSpot || !PageA || !PageB || !Lair)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	Runner->BeginForTesting({}, Campaign, Player);
	Secrets->SetCatalogForTesting(SecretsTestCatalog());
	int32 Sold = 0;
	Secrets->OnSold.AddLambda([&Sold](FName) { ++Sold; });

	TestFalse(TEXT("Shut before Main 2"), Secrets->IsOpen());
	TestEqual(TEXT("...nothing listed"), Secrets->GetRows().Num(), 0);
	Campaign.Complete(SecretsTestMainTwo);
	TestTrue(TEXT("Open after"), Secrets->IsOpen());
	TArray<FSecretRow> Rows = Secrets->GetRows();
	TestEqual(TEXT("Three for sale: the one with no find here left out"), Rows.Num(), 3);
	TestNull(TEXT("...not the nowhere one"), SecretsTestRow(Rows, TEXT("TestNowhere")));
	TestTrue(TEXT("The box first, at 40"), Rows.Num() > 0 && Rows[0].Id == FName(TEXT("TestBox")) && Rows[0].Price == 40);
	const FSecretRow* PagesRow = SecretsTestRow(Rows, TEXT("TestPages"));
	TestTrue(TEXT("The pages mark two places"), PagesRow && PagesRow->Places == 2);
	TestEqual(TEXT("His plain line"), Secrets->GetGreeting().ToString(), FString(TEXT("Bring the manifest.")));

	// Short, then paid.
	TestEqual(TEXT("Empty purse: not enough"), Secrets->Buy(TEXT("TestBox")), ESecretBuy::NotEnoughSilver);
	TestFalse(TEXT("...not sold"), Campaign.HasFlag(SecretRules::SoldFlag(TEXT("TestBox"))));
	Wallet->Add(100);
	TestEqual(TEXT("Paid"), Secrets->Buy(TEXT("TestBox")), ESecretBuy::Bought);
	TestEqual(TEXT("...40 out of the purse"), Wallet->GetSilver(), 60);
	TestTrue(TEXT("...kept as its flag (saved with the story)"), Campaign.HasFlag(SecretRules::SoldFlag(TEXT("TestBox"))));
	TestEqual(TEXT("...listeners told"), Sold, 1);
	TestEqual(TEXT("Not twice"), Secrets->Buy(TEXT("TestBox")), ESecretBuy::AlreadySold);
	TestEqual(TEXT("...nothing more paid"), Wallet->GetSilver(), 60);
	TestEqual(TEXT("Not one that isn't sold here"), Secrets->Buy(TEXT("TestNowhere")), ESecretBuy::NotForSale);
	TestEqual(TEXT("...nor one there's none of"), Secrets->Buy(TEXT("Nope")), ESecretBuy::NotForSale);
	Rows = Secrets->GetRows();
	TestTrue(TEXT("Bought: under MARKED, after those for sale"), Rows.Num() == 3 && Rows.Last().Id == FName(TEXT("TestBox"))
		&& Rows.Last().State == ESecretState::Sold);

	// The marks: the box where its spot stands.
	const TArray<FSecretMark> Marks = Secrets->GetMarks();
	TestTrue(TEXT("One mark, at the box"), Marks.Num() == 1 && Marks[0].Location.Equals(BoxSpot->GetActorLocation()));
	TestEqual(TEXT("Free (the console): nothing paid"), Secrets->Buy(TEXT("TestPages"), /*bFree*/ true), ESecretBuy::Bought);
	TestEqual(TEXT("...the purse as it was"), Wallet->GetSilver(), 60);
	TestEqual(TEXT("Three marks: the box and both pages"), Secrets->GetMarks().Num(), 3);
	Campaign.SetFlag(FindRules::FlagFor(TEXT("TestPageA")));
	TestEqual(TEXT("A page found: its mark goes"), Secrets->GetMarks().Num(), 2);
	Rows = Secrets->GetRows();
	PagesRow = SecretsTestRow(Rows, TEXT("TestPages"));
	TestTrue(TEXT("...one place left"), PagesRow && PagesRow->Places == 1);
	Campaign.SetFlag(FindRules::FlagFor(TEXT("TestBox")));
	Rows = Secrets->GetRows();
	TestNull(TEXT("The box found: off the list"), SecretsTestRow(Rows, TEXT("TestBox")));
	TestEqual(TEXT("...and off the map"), Secrets->GetMarks().Num(), 1);

	// Paid on Delivery done: a quarter off, his paid line.
	Campaign.Complete(SecretsTestPaid);
	Rows = Secrets->GetRows();
	const FSecretRow* LairRow = SecretsTestRow(Rows, TEXT("TestLair"));
	TestTrue(TEXT("A quarter off: 60 is 45"), LairRow && LairRow->Price == 45);
	TestEqual(TEXT("...and he says so"), Secrets->GetGreeting().ToString(), FString(TEXT("Paid in full.")));

	// A story secret: the tagged place, until its story ends.
	TestEqual(TEXT("The lair bought"), Secrets->Buy(TEXT("TestLair")), ESecretBuy::Bought);
	TestEqual(TEXT("...at 45"), Wallet->GetSilver(), 15);
	TestTrue(TEXT("...its place marked"), Secrets->GetMarks().ContainsByPredicate([Lair](const FSecretMark& Mark)
	{
		return Mark.Id == FName(TEXT("TestLair")) && Mark.Location.Equals(Lair->GetActorLocation());
	}));
	Campaign.Complete(SecretsTestSideThree);
	TestFalse(TEXT("Its story done: no mark"), Secrets->GetMarks().ContainsByPredicate([](const FSecretMark& Mark) { return Mark.Id == FName(TEXT("TestLair")); }));

	// Unsold (the console).
	TestTrue(TEXT("Forgotten"), Secrets->Forget(TEXT("TestPages")));
	Rows = Secrets->GetRows();
	PagesRow = SecretsTestRow(Rows, TEXT("TestPages"));
	TestTrue(TEXT("...for sale again"), PagesRow && PagesRow->State == ESecretState::ForSale);
	TestFalse(TEXT("...not twice"), Secrets->Forget(TEXT("TestPages")));
	TestEqual(TEXT("Something for sale: his line for the story"), Secrets->GetGreeting().ToString(), FString(TEXT("Paid in full.")));
	TestEqual(TEXT("The pages again"), Secrets->Buy(TEXT("TestPages"), /*bFree*/ true), ESecretBuy::Bought);
	TestEqual(TEXT("Nothing left for sale: his sold-out line"), Secrets->GetGreeting().ToString(), FString(TEXT("That's all I know.")));
	Secrets->SetCatalogForTesting(nullptr);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSecretsMarksTest, "Looter.Story.Secrets.Marks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSecretsMarksTest::RunTest(const FString& Parameters)
{
	// The map page's pins from a story's record: a bought, unfound secret is pinned with its name and his line; unbought or
	// found, none (the map still shows no chests of its own).
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	USecretSubsystem* Secrets = USecretSubsystem::Get(World);
	AFindSpot* BoxSpot = SecretsTestSpot(World, TEXT("TestBox"), FVector(1200.0, 300.0, 0.0));
	if (!Secrets || !BoxSpot)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	Secrets->SetCatalogForTesting(SecretsTestCatalog());
	FCampaignRecord Campaign;
	Campaign.Complete(SecretsTestMainTwo);
	FMapPinSources Sources;
	Sources.World = World;
	Sources.Campaign = &Campaign;
	TestEqual(TEXT("Unbought: no pin"), MapPins::Count(MapPins::Gather(Sources), EMapPinKind::Secret), 0);
	Campaign.SetFlag(SecretRules::SoldFlag(TEXT("TestBox")));
	const TArray<FMapPin> Pins = MapPins::Gather(Sources);
	const FMapPin* Pin = Pins.FindByPredicate([](const FMapPin& Each) { return Each.Kind == EMapPinKind::Secret; });
	TestTrue(TEXT("Bought: pinned at the box"), Pin && Pin->Location.Equals(BoxSpot->GetActorLocation()));
	TestTrue(TEXT("...its name and his line"), Pin && Pin->Name.ToString() == TEXT("The TestBox") && Pin->Detail.ToString().Contains(TEXT("friend")));
	TestEqual(TEXT("...one"), MapPins::Count(Pins, EMapPinKind::Secret), 1);
	TestFalse(TEXT("The kind has a legend name"), MapPins::KindName(EMapPinKind::Secret).IsEmpty());
	Campaign.SetFlag(FindRules::FlagFor(TEXT("TestBox")));
	TestEqual(TEXT("Found: the pin goes"), MapPins::Count(MapPins::Gather(Sources), EMapPinKind::Secret), 0);
	FMapPinSources Bare;
	Bare.World = World;
	TestEqual(TEXT("No story: no secrets pinned"), MapPins::Count(MapPins::Gather(Bare), EMapPinKind::Secret), 0);
	Secrets->SetCatalogForTesting(nullptr);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSecretsBrokerTest, "Looter.Story.Secrets.Broker",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSecretsBrokerTest::RunTest(const FString& Parameters)
{
	// Sexton's key: his talk until he trades (Main 2's turn-in says "Come back when you've silver"), then "Buy a secret".
	FCampaignRecord Campaign;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	USecretSubsystem* Secrets = USecretSubsystem::Get(World);
	AActor* Player = MissionTestWorld::SpawnMarker(World, FVector(200.0, 0.0, 0.0));
	AMisterSexton* Sexton = World->SpawnActor<AMisterSexton>(FVector::ZeroVector, FRotator::ZeroRotator);
	UInteractionComponent* Hands = Player ? NewObject<UInteractionComponent>(Player) : nullptr;
	if (!Runner || !Secrets || !Player || !Sexton || !Hands)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	Runner->BeginForTesting({}, Campaign, Player);
	Secrets->SetCatalogForTesting(SecretsTestCatalog());
	if (!TestNotNull(TEXT("He sells secrets (his broker)"), Sexton->Secrets.Get()))
	{
		return false;
	}
	TestFalse(TEXT("Before Main 2: he doesn't trade"), Sexton->Secrets->ShouldTrade());
	TestNotEqual(TEXT("...his key talks"), Sexton->GetInteractionOptions(*Hands).TapPrompt.ToString(), Sexton->Secrets->Prompt.ToString());
	Campaign.Complete(SecretsTestMainTwo);
	TestTrue(TEXT("After: he trades"), Sexton->Secrets->ShouldTrade());
	const FInteractionOptions Options = Sexton->GetInteractionOptions(*Hands);
	TestTrue(TEXT("...a tap: Buy a secret"), Options.CanUse() && Options.bTap && Options.TapPrompt.ToString() == Sexton->Secrets->Prompt.ToString());
	TestFalse(TEXT("No mission at him: no story business"), Sexton->Secrets->HasStoryBusiness());
	Sexton->SetActorHiddenInGame(true);
	TestFalse(TEXT("Hidden (the story's not showing him): no trade"), Sexton->Secrets->ShouldTrade());
	Secrets->SetCatalogForTesting(nullptr);
	return true;
}

#endif

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Creatures/EncounterRules.h"
#include "Interaction/InteractableProp.h"
#include "Interaction/InteractionComponent.h"
#include "Missions/MissionText.h"
#include "Story/SpeakerPointComponent.h"
#include "Tests/InteractionTestWorld.h"
#include "Tutorial/ControlHintRules.h"
#include "UI/HUD/HudMinimapWidget.h"
#include "UI/HUD/HudMissionTrackerWidget.h"
#include "UI/HUD/LooterHUD.h"
#include "UI/HUD/PlayerHUDWidget.h"
#include "UI/Inventory/LoadoutParts.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "InputCoreTypes.h"
#include "Tests/AutomationCommon.h"

// The rules from the Ransom's Rest play-check of 2026-10-23: a page's number key named from the tabs' own order (Main 2
// said "then 2" for the Ledger, tab 3), the HUD out from under the inventory and the loot card clear of the minimap, a
// press made while the prompt shows always used, a turn-in never refused mid-sentence, and a fight's stragglers recalled.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlayCheckPageKeysTest, "Looter.Missions.PageKeys",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPlayCheckPageKeysTest::RunTest(const FString& Parameters)
{
	// The tabs' table: every page's key turns to that page, and every tab's words name it.
	for (int32 Page = 0; Page < LoadoutParts::NumPages; ++Page)
	{
		const FKey Key = LoadoutParts::PageNumberKey(Page);
		TestTrue(*FString::Printf(TEXT("Page %d has a key"), Page), Key.IsValid());
		TestEqual(*FString::Printf(TEXT("...that turns to it (%d)"), Page), LoadoutParts::PageForNumberKey(Key), Page);
		TestEqual(*FString::Printf(TEXT("...and its tab names it (%d)"), Page), LoadoutParts::PageNamed(LoadoutParts::PageTabName(Page, false)), Page);
	}
	TestFalse(TEXT("No key past the last page"), LoadoutParts::PageNumberKey(LoadoutParts::NumPages).IsValid());
	TestTrue(TEXT("The bestiary's key is 3"), LoadoutParts::PageNumberKey(static_cast<int32>(EInventoryPage::Bestiary)) == EKeys::Three);
	TestEqual(TEXT("The Ledger is the bestiary"), LoadoutParts::PageNamed(TEXT("ledger")), static_cast<int32>(EInventoryPage::Bestiary));
	TestEqual(TEXT("Nothing else is a page"), LoadoutParts::PageNamed(TEXT("Nowhere")), static_cast<int32>(INDEX_NONE));

	// In the missions' words: {Page.X} is that tab's number key, needing no player to ask.
	TestEqual(TEXT("Main 2's Ledger step reads tab 3"), MissionText::ResolveKeys(nullptr, TEXT("Open the Ledger: {Inventory}, then {Page.Bestiary}.")),
		FString(TEXT("Open the Ledger: [Inventory], then [3].")));
	TestEqual(TEXT("{Page.Ledger} too"), MissionText::ResolveKeys(nullptr, TEXT("{Page.Ledger}")), FString(TEXT("[3]")));
	TestEqual(TEXT("The embers, second"), MissionText::ResolveKeys(nullptr, TEXT("{Page.Embers}")), FString(TEXT("[2]")));
	TestEqual(TEXT("The missions, fourth"), MissionText::ResolveKeys(nullptr, TEXT("{Page.Missions}")), FString(TEXT("[4]")));
	TestEqual(TEXT("The map, fifth"), MissionText::ResolveKeys(nullptr, TEXT("{Page.Map}")), FString(TEXT("[5]")));
	TestEqual(TEXT("An unknown page keeps its name"), MissionText::ResolveKeys(nullptr, TEXT("{Page.Nowhere}")), FString(TEXT("[Page.Nowhere]")));

	// The control hints say the same numbers, from the same table.
	TestTrue(TEXT("The Ledger's hint: then 3"), FControlHintRules::Words(EControlHint::Ledger).ToString().StartsWith(TEXT("Then 3:")));
	TestTrue(TEXT("The missions' hint: then 4"), FControlHintRules::Words(EControlHint::Missions).ToString().StartsWith(TEXT("Then 4:")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlayCheckLootCardTest, "Looter.UI.LootCardClear",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPlayCheckLootCardTest::RunTest(const FString& Parameters)
{
	// The loot card stands on the right edge's middle; a tall one drops just clear of the minimap's frame (the tracker's
	// top), never past the screen's foot.
	const float Under100 = UHudMissionTrackerWidget::TopFor(1.f, true);
	const float Under150 = UHudMissionTrackerWidget::TopFor(1.5f, true);
	TestTrue(TEXT("A bigger minimap reaches lower"), Under150 > Under100);
	TestEqual(TEXT("A card that fits stays in the middle"), UPlayerHUDWidget::LootCardDrop(1080.f, 400.f, Under100), 0.f);
	const float Tall = 1080.f - 2.f * (Under150 - 40.f);
	const float Drop = UPlayerHUDWidget::LootCardDrop(1080.f, Tall, Under150);
	TestTrue(TEXT("A tall card under a big minimap drops"), FMath::IsNearlyEqual(Drop, 40.f, 0.01f));
	TestTrue(TEXT("...its top just clear of the frame"), FMath::IsNearlyEqual((1080.f - Tall) * 0.5f + Drop, Under150, 0.01f));
	TestEqual(TEXT("A card taller than the room stops at the foot"), UPlayerHUDWidget::LootCardDrop(1080.f, 900.f, 350.f), 54.f);
	TestEqual(TEXT("Minimap off: nothing above to clear"), UPlayerHUDWidget::LootCardDrop(1080.f, 900.f, UHudMissionTrackerWidget::TopFor(1.5f, false)), 0.f);
	TestEqual(TEXT("Not laid out yet: no drop"), UPlayerHUDWidget::LootCardDrop(0.f, 400.f, Under150), 0.f);
	TestEqual(TEXT("No card height yet: no drop"), UPlayerHUDWidget::LootCardDrop(1080.f, 0.f, Under150), 0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlayCheckTrackerMenusTest, "Looter.UI.TrackerUnderMenus",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPlayCheckTrackerMenusTest::RunTest(const FString& Parameters)
{
	// Every menu covers the tracker, the inventory too (it sat over the Ledger's step on the loadout's card), and it stands
	// under the menus' layers so even its fade-out is behind them.
	TestTrue(TEXT("A menu covers it"), UHudMissionTrackerWidget::CoveredBy(true, false, false, false));
	TestTrue(TEXT("A scene covers it"), UHudMissionTrackerWidget::CoveredBy(false, true, false, false));
	TestTrue(TEXT("The pause covers it"), UHudMissionTrackerWidget::CoveredBy(false, false, true, false));
	TestTrue(TEXT("The loot card has its corner"), UHudMissionTrackerWidget::CoveredBy(false, false, false, true));
	TestFalse(TEXT("In play: shown"), UHudMissionTrackerWidget::CoveredBy(false, false, false, false));
	TestTrue(TEXT("Under the inventory's pages (20)"), UHudMissionTrackerWidget::ViewportZOrder < 20);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlayCheckFocusGraceTest, "Looter.Interaction.FocusGrace",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPlayCheckFocusGraceTest::RunTest(const FString& Parameters)
{
	using namespace InteractionTestWorld;
	// A press while the prompt is still fading out (the focus lost a moment ago) taps what it showed; later, nothing; a
	// hold is never started that way.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UInteractionComponent* Interaction = nullptr;
	APawn* Player = SpawnPlayer(World, Interaction);
	AInteractableProp* Door = SpawnDoor(World, Ahead(150.0));
	AInteractableProp* Bell = SpawnBell(World, FVector(0.0, 150.0, EyeHeight));
	if (!TestTrue(TEXT("Stand-in, door and bell placed"), Player && Interaction && Door && Bell))
	{
		return false;
	}
	int32 Rings = 0;
	Bell->OnUsedNative.AddLambda([&Rings](AInteractableProp&, AActor*, bool) { ++Rings; });

	Interaction->UpdateInteraction(0.f);
	TestTrue(TEXT("Looking at the door"), Interaction->GetFocusedActor() == Door);
	Face(Player, -90.0);
	Interaction->UpdateInteraction(UInteractionComponent::FocusGraceSeconds * 0.5f);
	TestNull(TEXT("Looked away: nothing focused"), Interaction->GetFocusedActor());
	Interaction->PressInteract();
	Interaction->ReleaseInteract();
	TestTrue(TEXT("Pressed while its prompt fades: the door opens"), Door->IsOn());

	// Once it's done swinging, looked at and away again, past the grace: the press does nothing.
	Door->Advance(0.5f);
	Face(Player, 0.0);
	Interaction->UpdateInteraction(0.f);
	TestTrue(TEXT("The door again"), Interaction->GetFocusedActor() == Door);
	Face(Player, -90.0);
	Interaction->UpdateInteraction(0.f);
	Interaction->UpdateInteraction(UInteractionComponent::FocusGraceSeconds + 0.05f);
	Interaction->PressInteract();
	Interaction->ReleaseInteract();
	TestTrue(TEXT("Past the grace: the door stays open"), Door->IsOn());

	// The bell takes only a hold: a press just after looking away starts nothing.
	Face(Player, 90.0);
	Interaction->UpdateInteraction(0.f);
	TestTrue(TEXT("Looking at the bell"), Interaction->GetFocusedActor() == Bell);
	Face(Player, -90.0);
	Interaction->UpdateInteraction(0.f);
	Interaction->PressInteract();
	TestFalse(TEXT("No hold on what isn't looked at"), Interaction->IsHolding());
	Interaction->UpdateInteraction(1.5f);
	Interaction->ReleaseInteract();
	TestEqual(TEXT("Never rung"), Rings, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlayCheckTurnInCutsInTest, "Looter.Missions.TurnInCutsIn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPlayCheckTurnInCutsInTest::RunTest(const FString& Parameters)
{
	// Sexton still saying his words (held under the inventory) with Main 2 waiting to be turned in to him: the key works.
	TestTrue(TEXT("Not talking: talk"), USpeakerPointComponent::MayTalkOver(false, false, false));
	TestFalse(TEXT("Talking, nothing to turn in: wait"), USpeakerPointComponent::MayTalkOver(true, false, false));
	TestTrue(TEXT("Talking, a turn-in waiting: turn in"), USpeakerPointComponent::MayTalkOver(true, true, false));
	TestFalse(TEXT("...but never over the turn-in's own words"), USpeakerPointComponent::MayTalkOver(true, true, true));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlayCheckStragglersTest, "Looter.Encounters.Stragglers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPlayCheckStragglersTest::RunTest(const FString& Parameters)
{
	using EncounterRules::EStragglerCall;
	using EncounterRules::StragglerCall;
	const float Every = EncounterRules::StragglerRecallEverySeconds;
	TestTrue(TEXT("Just out of the fight: nothing yet"), StragglerCall(0.5f, Every, false) == EStragglerCall::None);
	TestTrue(TEXT("Out a while: recalled"), StragglerCall(EncounterRules::StragglerRecallSeconds, Every, false) == EStragglerCall::Recall);
	TestTrue(TEXT("...not again straight after"), StragglerCall(EncounterRules::StragglerRecallSeconds + 0.5f, 0.5f, false) == EStragglerCall::None);
	TestTrue(TEXT("...again a while later"), StragglerCall(EncounterRules::StragglerRecallSeconds + Every, Every, false) == EStragglerCall::Recall);
	TestTrue(TEXT("Still out, out of sight: brought back"), StragglerCall(EncounterRules::StragglerBringBackSeconds, 0.f, false) == EStragglerCall::BringBack);
	TestTrue(TEXT("...never under the player's eyes"), StragglerCall(EncounterRules::StragglerBringBackSeconds + 30.f, 0.f, true) == EStragglerCall::None);
	TestTrue(TEXT("...only called there"), StragglerCall(EncounterRules::StragglerBringBackSeconds + 30.f, Every, true) == EStragglerCall::Recall);
	TestTrue(TEXT("Recalled before it's brought back"), EncounterRules::StragglerRecallSeconds < EncounterRules::StragglerBringBackSeconds);
	return true;
}

#endif

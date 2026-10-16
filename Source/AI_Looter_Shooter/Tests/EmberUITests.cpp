#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Bestiary/Ledger.h"
#include "UI/Embers/EmbersWidget.h"
#include "UI/HUD/HudEmberSocketWidget.h"
#include "UI/HUD/HudWeaponSlotsWidget.h"
#include "UI/HUD/LooterHUD.h"
#include "UI/HUD/PlayerHUDWidget.h"
#include "UI/Inventory/LoadoutParts.h"
#include "UI/Style/LooterButton.h"
#include "UI/Style/LooterUIStyle.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "InputCoreTypes.h"
#include "Tests/AutomationCommon.h"

// The ember powers' UI (Looter.Embers.Pages, Looter.Embers.Socket): the powers page as the inventory's second tab (the
// user's call, 2026-10-15: Loadout, Embers, then the Ledger), its number key and its tab's words; the HUD socket's rules,
// its place left of the weapon slots, and the powers' icons and colours (the contract's).

namespace
{
	/** The words on the inventory tab for page Page in Tree (the first text in its button), or empty. */
	FString EmberTestTabWords(UWidgetTree& Tree, int32 Page)
	{
		FString Words;
		Tree.ForEachWidget([&Words, Page](UWidget* Widget)
		{
			const ULooterButton* Tab = Cast<ULooterButton>(Widget);
			if (!Tab || Tab->Action != LoadoutParts::ActionPage || Tab->Index != Page || !Words.IsEmpty())
			{
				return;
			}
			UWidgetTree::ForWidgetAndChildren(const_cast<ULooterButton*>(Tab), [&Words](UWidget* Child)
			{
				if (const UTextBlock* Text = Cast<UTextBlock>(Child); Text && Words.IsEmpty())
				{
					Words = Text->GetText().ToString();
				}
			});
		});
		return Words;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEmberPagesTest, "Looter.Embers.Pages",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FEmberPagesTest::RunTest(const FString& Parameters)
{
	// The order: Loadout, Embers, Bestiary (the Ledger), Missions, Map.
	TestEqual(TEXT("The loadout first"), static_cast<int32>(EInventoryPage::Loadout), 0);
	TestEqual(TEXT("The ember powers second"), static_cast<int32>(EInventoryPage::Embers), 1);
	TestEqual(TEXT("The bestiary third"), static_cast<int32>(EInventoryPage::Bestiary), 2);
	TestEqual(TEXT("The missions fourth"), static_cast<int32>(EInventoryPage::Missions), 3);
	TestEqual(TEXT("The map last"), static_cast<int32>(EInventoryPage::Map), 4);
	TestEqual(TEXT("A tab per page"), LoadoutParts::NumPages, static_cast<int32>(EInventoryPage::Map) + 1);

	TestEqual(TEXT("The second tab reads Embers"), LoadoutParts::PageTabName(static_cast<int32>(EInventoryPage::Embers), false), FString(TEXT("Embers")));
	TestEqual(TEXT("The third, Bestiary before Sexton hands it over"), LoadoutParts::PageTabName(static_cast<int32>(EInventoryPage::Bestiary), false),
		FString(Ledger::BookName(false)));
	TestEqual(TEXT("...and the Ledger after"), LoadoutParts::PageTabName(static_cast<int32>(EInventoryPage::Bestiary), true), FString(Ledger::BookName(true)));
	TestEqual(TEXT("2 turns to the ember powers"), LoadoutParts::PageForNumberKey(EKeys::Two), static_cast<int32>(EInventoryPage::Embers));
	TestEqual(TEXT("3 to the bestiary"), LoadoutParts::PageForNumberKey(EKeys::Three), static_cast<int32>(EInventoryPage::Bestiary));
	TestEqual(TEXT("5 to the map"), LoadoutParts::PageForNumberKey(EKeys::Five), static_cast<int32>(EInventoryPage::Map));
	TestEqual(TEXT("Other keys turn no page"), LoadoutParts::PageForNumberKey(EKeys::Q), static_cast<int32>(INDEX_NONE));

	TestEqual(TEXT("A cooldown in whole seconds"), UEmbersWidget::SecondsWords(12.f, false), FString(TEXT("12 s")));
	TestEqual(TEXT("A short one with its tenth"), UEmbersWidget::SecondsWords(0.3f, true), FString(TEXT("0.3 s")));
	TestEqual(TEXT("No duration is instant"), UEmbersWidget::SecondsWords(0.f, true), FString(TEXT("Instant")));

	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UEmbersWidget* Page = CreateWidget<UEmbersWidget>(TestLevel.GetTestWorld(), UEmbersWidget::StaticClass());
	if (!TestNotNull(TEXT("The page"), Page))
	{
		return false;
	}
	const TSharedRef<SWidget> PageSlate = Page->TakeWidget();
	if (!TestNotNull(TEXT("Its tree"), Page->WidgetTree.Get()))
	{
		return false;
	}
	UWidgetTree& Tree = *Page->WidgetTree;
	int32 Tabs = 0;
	Tree.ForEachWidget([&Tabs](UWidget* Widget)
	{
		const ULooterButton* Tab = Cast<ULooterButton>(Widget);
		Tabs += Tab && Tab->Action == LoadoutParts::ActionPage ? 1 : 0;
	});
	TestEqual(TEXT("Five title tabs"), Tabs, LoadoutParts::NumPages);
	LoadoutParts::TitlePageTabs(Tree, true);
	TestEqual(TEXT("Its own tab reads EMBERS"), EmberTestTabWords(Tree, static_cast<int32>(EInventoryPage::Embers)), FString(TEXT("EMBERS")));
	TestEqual(TEXT("...and the Ledger's tab is renamed, not it"), EmberTestTabWords(Tree, static_cast<int32>(EInventoryPage::Bestiary)),
		FString(Ledger::BookName(true)).ToUpper());
	TestEqual(TEXT("No pawn, no powers: no cards"), Page->NumCards(), 0);
	TestFalse(TEXT("...and nothing to equip"), Page->EquipSelected());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEmberSocketTest, "Looter.Embers.Socket",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FEmberSocketTest::RunTest(const FString& Parameters)
{
	using Socket = UHudEmberSocketWidget;
	TestEqual(TEXT("Ready: no seconds"), Socket::SecondsText(0.f), FString());
	TestEqual(TEXT("Under a second reads 1, never 0"), Socket::SecondsText(0.2f), FString(TEXT("1")));
	TestEqual(TEXT("Whole seconds, rounded up"), Socket::SecondsText(2.01f), FString(TEXT("3")));
	TestEqual(TEXT("...exactly two is 2"), Socket::SecondsText(2.f), FString(TEXT("2")));
	TestEqual(TEXT("One letter's tab is the slots' 22 px"), Socket::TabWidthFor(TEXT("Q")), 22.f);
	TestTrue(TEXT("A longer key's tab is wider"), Socket::TabWidthFor(TEXT("TAB")) > 22.f);
	TestEqual(TEXT("The shake dies away"), Socket::ShakeOffset(Socket::ShakeSeconds), 0.f);
	TestTrue(TEXT("The pop swells and settles"), Socket::PopScale(Socket::PopSeconds * 0.5f) > 1.1f && Socket::PopScale(Socket::PopSeconds) == 1.f);

	// The powers' icons, found by the names their data gives (UEmberDefinition::IconName), and their colours.
	for (const TCHAR* Name : { TEXT("DustDevil"), TEXT("Sundown"), TEXT("BorrowedTime") })
	{
		const LooterUI::FInkedIcon* Icon = LooterUI::FindInkedIcon(Name);
		TestTrue(FString::Printf(TEXT("The %s icon (Art/Icons/InkedIcons.py)"), Name), Icon && Icon->Shapes.Num() > 0);
	}
	TestNull(TEXT("No icon under a name never made"), LooterUI::FindInkedIcon(TEXT("NoSuchIcon")));
	TestNull(TEXT("...or none"), LooterUI::FindInkedIcon(NAME_None));
	TestTrue(TEXT("Dust Devil #e8f6ff"), LooterUI::Color::EmberDustDevil().Equals(LooterUI::Hex(0xe8, 0xf6, 0xff)));
	TestTrue(TEXT("Sundown #ffb24d"), LooterUI::Color::EmberSundown().Equals(LooterUI::Hex(0xff, 0xb2, 0x4d)));
	TestTrue(TEXT("Borrowed Time #c9b4ff"), LooterUI::Color::EmberBorrowedTime().Equals(LooterUI::Hex(0xc9, 0xb4, 0xff)));

	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	Socket* TestSocket = CreateWidget<Socket>(World, Socket::StaticClass());
	if (!TestNotNull(TEXT("The socket"), TestSocket))
	{
		return false;
	}
	const TSharedRef<SWidget> SocketSlate = TestSocket->TakeWidget();
	TestSocket->Update(nullptr, TEXT("Q"), 0.f);
	TestFalse(TEXT("No ember component: nothing shown"), TestSocket->IsShowing());
	TestSocket->Flash(EEmberDenied::Cooldown);
	TestTrue(TEXT("A press on cooldown shakes it"), TestSocket->IsShaking());
	TestSocket->NotifyState(EEmberState::Ready);
	TestFalse(TEXT("Nothing shown: no pop"), TestSocket->IsPopping());

	// In the HUD: apart from the weapon cluster (which hides with no gun in hand: Q works unarmed), in a row of its own
	// with a spacer standing in for the slots and the cartridge on its right.
	UPlayerHUDWidget* TestHud = CreateWidget<UPlayerHUDWidget>(World, UPlayerHUDWidget::StaticClass());
	if (!TestNotNull(TEXT("The HUD"), TestHud))
	{
		return false;
	}
	const TSharedRef<SWidget> HudSlate = TestHud->TakeWidget();
	const UHudEmberSocketWidget* HudSocket = TestHud->GetEmberSocket();
	const UHudWeaponSlotsWidget* Slots = TestHud->GetWeaponSlots();
	const UPanelWidget* Arms = Slots ? Slots->GetParent() : nullptr;
	const UPanelWidget* Cluster = Arms ? Arms->GetParent() : nullptr;
	if (!TestTrue(TEXT("The socket and the slots made"), HudSocket && Slots && Arms && Cluster))
	{
		return false;
	}
	bool bInCluster = false;
	for (const UWidget* Up = HudSocket; Up; Up = Up->GetParent())
	{
		bInCluster |= Up == Cluster;
	}
	TestFalse(TEXT("The socket isn't in the weapon cluster (it shows with no gun in hand)"), bInCluster);
	const UPanelWidget* EmberRow = HudSocket->GetParent();
	TestTrue(TEXT("...its row has the socket first, the slots' stand-in on its right"),
		EmberRow && EmberRow->GetChildIndex(HudSocket) == 0 && EmberRow->GetChildrenCount() == 2);
	TestTrue(TEXT("...and that row stands on the HUD's canvas, as the cluster does"), EmberRow && EmberRow->GetParent() == Cluster->GetParent());
	return true;
}

#endif

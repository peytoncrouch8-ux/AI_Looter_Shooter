// UPauseMenuWidget: the pause menu's list over the paused game (the user, 2026-10-29 night: Esc opens a menu of menus, not
// the settings; Resume Game at the top, Save & Quit at the bottom). This file puts it together in the settings menu's
// round 3 look (its pieces: PauseMenuPieces.cpp), writes its words and says what was picked. PauseMenuNav.cpp drives it by
// keys; PauseMenuMotion.cpp moves it.

#include "UI/Menus/PauseMenuWidget.h"
#include "UI/Menus/SettingsMenuArt.h"
#include "UI/Menus/SettingsMenuParts.h"
#include "UI/Style/LooterButton.h"
#include "UI/Style/LooterUIStyle.h"
#include "Areas/AreaDefinition.h"
#include "Areas/AreaRulesSubsystem.h"
#include "Audio/LooterSound.h"
#include "Bestiary/Ledger.h"
#include "Progression/PlayerProgressionSubsystem.h"
#include "Session/SessionSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/HorizontalBox.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"

// The settings menu's sizes (the plate's padding, the ledger's margin), so the two read as one.
using namespace SettingsMenu;

namespace
{
	/** Every row's button carries this action, its Index the row (EPauseMenuItem). */
	const FName ActionPick(TEXT("PausePick"));
	const FName ActionSaveQuit(TEXT("SaveQuit"));

	const EPauseMenuItem RowOrder[] = {
		EPauseMenuItem::Resume,
		EPauseMenuItem::Inventory,
		EPauseMenuItem::Missions,
		EPauseMenuItem::Map,
		EPauseMenuItem::Bestiary,
		EPauseMenuItem::Embers,
		EPauseMenuItem::Settings,
		EPauseMenuItem::SaveQuit,
	};

	/** The few words at a row's right: what is behind it. */
	const TCHAR* DetailOf(EPauseMenuItem Item)
	{
		switch (Item)
		{
		case EPauseMenuItem::Inventory: return TEXT("Your guns and backpack");
		case EPauseMenuItem::Missions:  return TEXT("Active, open and done");
		case EPauseMenuItem::Map:       return TEXT("The area, pins, fast travel");
		case EPauseMenuItem::Bestiary:  return TEXT("Everything you have met");
		case EPauseMenuItem::Embers:    return TEXT("Your ember powers");
		case EPauseMenuItem::Settings:  return TEXT("Graphics, audio, interface, controls");
		default:                        return TEXT("");
		}
	}
}

TConstArrayView<EPauseMenuItem> UPauseMenuWidget::ItemOrder()
{
	return MakeArrayView(RowOrder);
}

FString UPauseMenuWidget::ItemWord(EPauseMenuItem Item, bool bLedger)
{
	switch (Item)
	{
	case EPauseMenuItem::Resume:    return TEXT("RESUME GAME");
	case EPauseMenuItem::Inventory: return TEXT("INVENTORY");
	case EPauseMenuItem::Missions:  return TEXT("MISSIONS");
	case EPauseMenuItem::Map:       return TEXT("MAP");
	case EPauseMenuItem::Bestiary:  return FString(Ledger::BookName(bLedger)).ToUpper();
	case EPauseMenuItem::Embers:    return TEXT("EMBERS");
	case EPauseMenuItem::Settings:  return TEXT("SETTINGS");
	case EPauseMenuItem::SaveQuit:  return TEXT("SAVE & QUIT");
	default:                        return FString();
	}
}

void UPauseMenuWidget::Open(EPauseMenuItem Focus)
{
	SetIsFocusable(true);
	bLeaving = false;
	OpenFocus = Focus;
	StopNavHold();
	NavStick = FVector2D::ZeroVector;
	NavStickMove = MenuNav::EMove::None;
	// Opened by the controller's Start (still down as the menu opens): the controller drives it from its first press.
	if (const APlayerController* PC = GetOwningPlayer(); PC && PC->IsInputKeyDown(EKeys::Gamepad_Special_Right))
	{
		LooterUI::SetMenuDevice(LooterUI::EMenuDevice::Gamepad);
	}
	if (!ItemButtons.IsEmpty())
	{
		ApplyOpen();
	}
}

void UPauseMenuWidget::ApplyOpen()
{
	RefreshWords();
	RebuildNavigation(GetItemButton(OpenFocus));
	RefreshNavLook();
	BeginOpenMotion();
}

ULooterButton* UPauseMenuWidget::GetItemButton(EPauseMenuItem Item) const
{
	const FPauseMenuLine* Found = Lines.FindByPredicate([Item](const FPauseMenuLine& Line) { return Line.Item == Item; });
	return Found ? Found->Button.Get() : nullptr;
}

FString UPauseMenuWidget::GetShownWord(EPauseMenuItem Item) const
{
	const FPauseMenuLine* Found = Lines.FindByPredicate([Item](const FPauseMenuLine& Line) { return Line.Item == Item; });
	return Found && Found->Word.IsValid() ? Found->Word->GetText().ToString() : ItemWord(Item, false);
}

void UPauseMenuWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	AdvanceNav(InDeltaTime);
	AdvanceLook(InDeltaTime);
}

// ---------------------------------------------------------------------------
// Building
// ---------------------------------------------------------------------------

TSharedRef<SWidget> UPauseMenuWidget::RebuildWidget()
{
	using namespace LooterUI;
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
		WidgetTree->RootWidget = Root;

		// The dimming over the paused game; it takes the clicks off the game.
		UBorder* Dimmer = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Dimmer"));
		Dimmer->SetBrush(RectBrush(Color::Backdrop()));
		Dimmer->SetVisibility(ESlateVisibility::Visible);
		MarkBackground(Dimmer);
		UOverlaySlot* DimmerSlot = Root->AddChildToOverlay(Dimmer);
		DimmerSlot->SetHorizontalAlignment(HAlign_Fill);
		DimmerSlot->SetVerticalAlignment(VAlign_Fill);

		UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		auto Add = [Column](UWidget* Child, float Top, bool bFill = false)
		{
			UVerticalBoxSlot* ChildSlot = Column->AddChildToVerticalBox(Child);
			ChildSlot->SetPadding(FMargin(0.f, Top, 0.f, 0.f));
			if (bFill)
			{
				ChildSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			}
		};

		Lines.Reset();
		ItemButtons.Reset();
		// GAME PAUSED beside the halo, RESUME GAME in its reticle under it: the top of the list, as the user asked.
		Add(MakeHeader(), 0.f);
		Add(MakeResumeRow(), 14.f);

		// The ledger: the screens to go to, numbered in a margin ruled off by the gold double line (c1). Far enough down that
		// the first row's number clears the halo's lowest ticks.
		UOverlay* LedgerBox = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		USettingsMenuArt* Margin = USettingsMenuArt::Make(WidgetTree, ESettingsArt::MarginRule);
		Margin->SetMarginX(LedgerMargin);
		FillOverlaySlot(LedgerBox->AddChildToOverlay(Margin));
		UVerticalBox* Rows = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		int32 Number = 1;
		for (const EPauseMenuItem Item : RowOrder)
		{
			if (Item != EPauseMenuItem::Resume && Item != EPauseMenuItem::SaveQuit)
			{
				Rows->AddChildToVerticalBox(MakeLedgerRow(Item, Number++, DetailOf(Item)));
			}
		}
		FillOverlaySlot(LedgerBox->AddChildToOverlay(Rows));
		Add(LedgerBox, 30.f);

		// Save & Quit's stamp at the bottom, the serial beside it (the room left over sits above them).
		Add(MakeSized(WidgetTree, nullptr, 0.f), 0.f, true);
		Add(MakeFooter(), 8.f);

		// The plate: its glass (a background), its double trim and rivets, its contents, and the free art over them.
		Plate = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		USettingsMenuArt* Glass = USettingsMenuArt::Make(WidgetTree, ESettingsArt::PlateFill);
		MarkBackground(Glass);
		FillOverlaySlot(Plate->AddChildToOverlay(Glass));
		FillOverlaySlot(Plate->AddChildToOverlay(USettingsMenuArt::Make(WidgetTree, ESettingsArt::PlateLines)));
		FillOverlaySlot(Plate->AddChildToOverlay(Column), FMargin(PlatePadX, PlatePadTop, PlatePadX, PlatePadBottom));
		PlateArt = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
		PlateArt->SetVisibility(ESlateVisibility::HitTestInvisible);
		FillOverlaySlot(Plate->AddChildToOverlay(PlateArt));
		MakePlateArt();

		// The plate and the hollow keys under it, in the middle of the screen; it drops in as it opens.
		PromptBar = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		UVerticalBox* Stack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		Stack->AddChildToVerticalBox(MakeSized(WidgetTree, Plate, PauseMenu::PlateWidth, PauseMenu::PlateHeight));
		UVerticalBoxSlot* PromptSlot = Stack->AddChildToVerticalBox(PromptBar);
		PromptSlot->SetHorizontalAlignment(HAlign_Center);
		PromptSlot->SetPadding(FMargin(0.f, 22.f, 0.f, 0.f));
		Panel = MakeSized(WidgetTree, Stack, 0.f);
		UOverlaySlot* PanelSlot = Root->AddChildToOverlay(Panel);
		PanelSlot->SetHorizontalAlignment(HAlign_Center);
		PanelSlot->SetVerticalAlignment(VAlign_Center);

		ApplyOpen();
	}
	return Super::RebuildWidget();
}

ULooterButton* UPauseMenuWidget::MakeItemButton(UWidget* Content, EPauseMenuItem Item)
{
	ULooterButton* Button = WidgetTree->ConstructWidget<ULooterButton>(ULooterButton::StaticClass());
	const bool bQuit = Item == EPauseMenuItem::SaveQuit;
	Button->SetupContent(Content, bQuit ? ActionSaveQuit : ActionPick, static_cast<int32>(Item), LooterUI::EButtonKind::Bare);
	// Resume is the page's close (the HUD plays the same cue in the same moment, heard as one, as the settings' Resume did);
	// Save & Quit sounds as the settings' stamp did; the rest click, and the HUD opens their screen with its own sound.
	if (Item == EPauseMenuItem::Resume)
	{
		Button->ClickCue = LooterSoundCue::Close;
	}
	else
	{
		Button->ClickCue = ULooterButton::ClickCueFor(bQuit ? ActionSaveQuit : ActionPick,
			bQuit ? LooterUI::EButtonKind::Danger : LooterUI::EButtonKind::Normal);
	}
	Button->OnButtonClicked.BindUObject(this, &UPauseMenuWidget::HandleButton);
	Button->OnButtonHovered.BindUObject(this, &UPauseMenuWidget::HandleButtonHovered);
	ItemButtons.Add(Button);
	return Button;
}

void UPauseMenuWidget::RefreshWords()
{
	// The session played (none from Looter.PlayLevel: 0) and the area's name, as the map page names it; the player's level.
	const UWorld* World = GetWorld();
	const USessionSubsystem* Sessions = USessionSubsystem::Get(this);
	const int32 Session = Sessions && Sessions->IsPlayingSession() ? Sessions->GetActiveSession() + 1 : 0;
	const UAreaRulesSubsystem* Rules = World ? World->GetSubsystem<UAreaRulesSubsystem>() : nullptr;
	const UAreaDefinition* Area = Rules ? Rules->GetArea() : nullptr;
	const FString AreaName = Area && !Area->DisplayName.IsEmpty() ? Area->DisplayName.ToString()
		: (World ? USessionSubsystem::AreaName(USessionSubsystem::MapOf(World)) : FString());
	const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	const UPlayerProgressionSubsystem* Progression = LocalPlayer ? LocalPlayer->GetSubsystem<UPlayerProgressionSubsystem>() : nullptr;
	const int32 Level = Progression ? Progression->GetLevel() : 1;

	if (AreaPlate)
	{
		const FString Words = SettingsMenuArt::PlateText(Session, AreaName);
		AreaPlate->SetLabel(Words, 13, 160);
		AreaPlate->SetVisibility(Words.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
	if (SerialText)
	{
		SerialText->SetText(FText::FromString(SettingsMenuArt::SerialText(Session, Level)));
	}
	if (Stamp)
	{
		Stamp->SetLabel(ItemWord(EPauseMenuItem::SaveQuit, false), 26, 230);
	}
	// The bestiary's row reads LEDGER once Sexton has handed his book over, as its tab does.
	const bool bLedger = Ledger::IsOpenIn(this);
	for (const FPauseMenuLine& Line : Lines)
	{
		if (UTextBlock* Word = Line.Word.Get())
		{
			Word->SetText(FText::FromString(ItemWord(Line.Item, bLedger)));
		}
	}
}

// ---------------------------------------------------------------------------
// Picking
// ---------------------------------------------------------------------------

void UPauseMenuWidget::HandleButton(ULooterButton* Button)
{
	if (Button && Button->Index >= 0 && Button->Index < static_cast<int32>(EPauseMenuItem::Count))
	{
		Choose(static_cast<EPauseMenuItem>(Button->Index));
	}
}

void UPauseMenuWidget::Choose(EPauseMenuItem Item)
{
	if (bLeaving)
	{
		return;
	}
	StopNavHold();
	if (Item == EPauseMenuItem::SaveQuit)
	{
		// The level is on its way out: the stamp says so, and nothing more is taken meanwhile.
		bLeaving = true;
		if (Stamp)
		{
			Stamp->SetLabel(TEXT("SAVING..."), 26, 230);
		}
	}
	OnChoice.ExecuteIfBound(Item);
}

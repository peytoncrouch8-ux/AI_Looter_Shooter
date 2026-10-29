#include "UI/Menus/MainMenuWidget.h"
#include "UI/Menus/HeroSelectWidget.h"
#include "UI/Menus/MainMenuArt.h"
#include "UI/Menus/MainMenuHUD.h"
#include "UI/Style/LooterButton.h"
#include "UI/Style/LooterUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Kismet/KismetSystemLibrary.h"

using namespace LooterUI;

namespace
{
	const FName ActionSinglePlayer(TEXT("SinglePlayer"));
	const FName ActionMultiplayer(TEXT("Multiplayer"));
	const FName ActionSettings(TEXT("Settings"));
	const FName ActionQuit(TEXT("Quit"));

	/**
	 * The shade under the menu runs past its side and thins out by here, so its edge is never seen as a line (the planet
	 * the menu's camera frames starts at the column's edge; the shade over it there is faint).
	 */
	constexpr float ShadeWidth = 1000.f;
	/** The menu's margins inside its side of the screen: room to breathe off the screen's edge, the title high. The right
	 *  one is slim: REVENANT, spaced wide, runs most of the way across. */
	const FMargin ColumnMargin(96.f, 104.f, 24.f, 44.f);

	/** The main buttons' list starts right under the title's box (its range arc's top is the mockup's gap); the picker
	 *  (three tall cards) a little further down. */
	constexpr float PickerTop = 28.f;
	/** The key hints arrive last (MainMenuLook.cpp times the rest). */
	constexpr float FooterDelay = 0.9f;

	/** A border showing one of the menu's painted pictures stretched over it, as a background the transparency fades. */
	UBorder* MakeBackdropPicture(UWidgetTree* Tree, FName Name, const FPaintedIcon& Picture, float PixelsPerUnit)
	{
		UBorder* Box = Tree->ConstructWidget<UBorder>(UBorder::StaticClass());
		Box->SetBrush(PaintedIconBrush(Name, Picture, PixelsPerUnit, Picture.ViewBox));
		Box->SetPadding(FMargin(0.f));
		Box->SetVisibility(ESlateVisibility::HitTestInvisible);
		MarkBackground(Box);
		return Box;
	}
}

// ---------------------------------------------------------------------------
// Layout
// ---------------------------------------------------------------------------

void UMainMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	// The menu holds the keyboard (Escape steps back); its buttons never take it.
	SetIsFocusable(true);
}

ULooterButton* UMainMenuWidget::MakeButton(FName Action, int32 Index, const FString& Label, int32 FontSize, EButtonKind Kind)
{
	ULooterButton* Button = WidgetTree->ConstructWidget<ULooterButton>(ULooterButton::StaticClass());
	Button->Setup(WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass()), Action, Index, FText::FromString(Label), FontSize, Kind);
	Button->OnButtonClicked.BindUObject(this, &UMainMenuWidget::HandleButton);
	Button->OnButtonHovered.BindUObject(this, &UMainMenuWidget::HandleButtonHovered);
	return Button;
}

TSharedRef<SWidget> UMainMenuWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
		// The whole screen answers the mouse, so a click beside the menu lands on it (NativeOnMouseButtonDown) and the
		// keyboard stays with the menu instead of going to the game viewport.
		Root->SetVisibility(ESlateVisibility::Visible);
		WidgetTree->RootWidget = Root;

		ColumnBox = MakeColumn();
		UOverlaySlot* ColumnSlot = Root->AddChildToOverlay(ColumnBox);
		ColumnSlot->SetHorizontalAlignment(HAlign_Left);
		ColumnSlot->SetVerticalAlignment(VAlign_Fill);

		// Over the whole screen while an empty slot's New Game chooses its hero (MainMenuSessions.cpp).
		HeroSelect = WidgetTree->ConstructWidget<UHeroSelectWidget>(UHeroSelectWidget::StaticClass(), TEXT("HeroSelect"));
		HeroSelect->OnConfirmed.BindUObject(this, &UMainMenuWidget::HandleHeroConfirmed);
		HeroSelect->OnBack.BindUObject(this, &UMainMenuWidget::CloseHeroSelect);
		HeroSelect->SetVisibility(ESlateVisibility::Collapsed);
		FillOverlaySlot(Root->AddChildToOverlay(HeroSelect));

		// On top of everything: the delete confirmation, while it's open.
		PopupLayer = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Popup"));
		FillOverlaySlot(Root->AddChildToOverlay(PopupLayer));

		ShowMainButtons();
		// The first page needs no fade of its own: the arrival brings it in.
		PageAge = 1000.f;
		RefreshNavLook();
		// Unseen until the arrival begins (BeginReveal, or the first frame).
		ApplyReveal();
		ApplyRows(0.f);
	}
	return Super::RebuildWidget();
}

UWidget* UMainMenuWidget::MakeColumn()
{
	// The menu's side: the shade (wider, thinning out) under the menu itself (ColumnWidth, 40% of a 16:9 screen).
	UOverlay* Side = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Column"));
	Side->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	UBorder* ShadePicture = MakeBackdropPicture(WidgetTree, TEXT("MenuShade"), MainMenuArt::Shade(), 8.f);
	Shade = ShadePicture;
	UOverlaySlot* ShadeSlot = Side->AddChildToOverlay(MakeSized(WidgetTree, ShadePicture, ShadeWidth));
	ShadeSlot->SetHorizontalAlignment(HAlign_Left);
	ShadeSlot->SetVerticalAlignment(VAlign_Fill);

	// Top to bottom: the title, the menu (the main buttons, or the session picker in their place), the key hints.
	UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	// The title's parts and the buttons each arrive in their own turn (MainMenuLook.cpp adds them).
	Content->AddChildToVerticalBox(MakeTitle());
	MainButtons = MakeMainButtons();
	UVerticalBoxSlot* ButtonsSlot = Content->AddChildToVerticalBox(MainButtons);
	ButtonsSlot->SetHorizontalAlignment(HAlign_Left);
	// The picker in their place, a little under the title's box.
	Picker = MakePicker();
	Content->AddChildToVerticalBox(Picker)->SetPadding(FMargin(0.f, PickerTop, 0.f, 0.f));
	Content->AddChildToVerticalBox(MakeSized(WidgetTree, nullptr, 0.f))->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	FooterPrompts = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Content->AddChildToVerticalBox(FooterPrompts);
	AddRevealPart(FooterPrompts, FooterDelay, 0.f);

	UOverlay* Menu = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	FillOverlaySlot(Menu->AddChildToOverlay(Content), ColumnMargin);
	UOverlaySlot* MenuSlot = Side->AddChildToOverlay(MakeSized(WidgetTree, Menu, ColumnWidth));
	MenuSlot->SetHorizontalAlignment(HAlign_Left);
	MenuSlot->SetVerticalAlignment(VAlign_Fill);
	return Side;
}

// The title lockup and the main buttons (MakeTitle, MakeHalo, MakeMainButtons, MakeMainButton) are in MainMenuLook.cpp.

ULooterButton* UMainMenuWidget::GetMainButton(FName Action) const
{
	for (ULooterButton* Button : MainNavButtons)
	{
		if (Button && Button->Action == Action)
		{
			return Button;
		}
	}
	return nullptr;
}

// ---------------------------------------------------------------------------
// What's on screen
// ---------------------------------------------------------------------------

void UMainMenuWidget::ShowMainButtons()
{
	CloseDeleteConfirm();
	bPickerShown = false;
	bHeroSelectShown = false;
	if (HeroSelect)
	{
		HeroSelect->Close();
		HeroSelect->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (ColumnBox)
	{
		ColumnBox->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	if (MainButtons)
	{
		MainButtons->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	if (Picker)
	{
		Picker->SetVisibility(ESlateVisibility::Collapsed);
	}
	SetStatus(FString(), Color::TextDim());
	RestartPage();
	// Back on the main buttons, the focus on Single Player (where the picker was opened from).
	RebuildNavigation(GetMainButton(TEXT("SinglePlayer")));
	RefreshFooter();
}

void UMainMenuWidget::ShowPicker()
{
	bPickerShown = true;
	// Read every time it opens: a session may have been played, saved or deleted since it last showed.
	RefreshSessions();
	SetStatus(FString(), Color::TextDim());
	if (MainButtons)
	{
		MainButtons->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (Picker)
	{
		Picker->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	RestartPage();
	// The focus on the first session's Continue or New Game.
	RebuildNavigation(PlayButtons.IsEmpty() ? nullptr : PlayButtons[0].Get());
	RefreshFooter();
}

void UMainMenuWidget::SetStatus(const FString& Message, const FLinearColor& TextColor)
{
	if (StatusText)
	{
		StatusText->SetText(FText::FromString(Message));
		StatusText->SetColorAndOpacity(FSlateColor(TextColor));
	}
}

// The key hints at the bottom (RefreshFooter) are in MainMenuNav.cpp, with the keyboard and controller focus.

// ---------------------------------------------------------------------------
// Buttons and keys
// ---------------------------------------------------------------------------

AMainMenuHUD* UMainMenuWidget::GetMenuHUD() const
{
	const APlayerController* PC = GetOwningPlayer();
	return PC ? Cast<AMainMenuHUD>(PC->GetHUD()) : nullptr;
}

void UMainMenuWidget::HandleButton(ULooterButton* Button)
{
	// While a session's level opens the menu is on its way out; while the settings menu is open it's in charge.
	AMainMenuHUD* HUD = GetMenuHUD();
	if (!Button || bLoading || (HUD && HUD->IsSettingsOpen()))
	{
		return;
	}

	if (Button->Action == ActionSettings)
	{
		if (HUD)
		{
			// The settings menu takes the keyboard while it's open, and the HUD hands it back here when it closes.
			HUD->OpenSettings();
			return;
		}
	}
	else if (Button->Action == ActionQuit)
	{
		UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
		return;
	}
	else if (Button->Action == ActionSinglePlayer)
	{
		ShowPicker();
	}
	else if (Button->Action != ActionMultiplayer)
	{
		// Multiplayer does nothing yet (its button is disabled anyway); the rest are the picker's and the popup's.
		HandleSessionButton(Button);
	}
	// Clicking handed the keyboard to the game viewport (LooterButton); take it back so Escape keeps reaching the menu.
	SetKeyboardFocus();
}

FReply UMainMenuWidget::NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	// Esc, B and the rest of the keyboard and controller: MainMenuNav.cpp.
	return HandleNavKey(InKeyEvent.GetKey(), InKeyEvent.IsRepeat()) ? FReply::Handled() : Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
}

FReply UMainMenuWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// A click on no button ends here rather than in the game viewport, which would take the keyboard from the menu.
	return FReply::Handled();
}

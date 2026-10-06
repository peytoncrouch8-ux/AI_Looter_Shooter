#include "UI/Menus/MainMenuWidget.h"
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
#include "Components/ScaleBox.h"
#include "Components/ScaleBoxSlot.h"
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

	/** The glass column on the left: under a third of a 16:9 screen, so the island behind the menu stays in view. */
	constexpr float ColumnWidth = 600.f;
	constexpr float MainButtonWidth = 360.f;
	constexpr float MainButtonHeight = 52.f;
	constexpr float MainButtonGap = 12.f;

	/** The column's glass: translucent like the kit's plates, so the sky shows faintly through it. */
	FLinearColor ColumnGlass() { return Hex(7, 26, 40, 215); }

	/** A small outlined word beside a button ("SOON"). Only an outline and text, so there's no background to fade. */
	UWidget* MakeTag(UWidgetTree* Tree, const FString& Word)
	{
		UBorder* Tag = Tree->ConstructWidget<UBorder>(UBorder::StaticClass());
		Tag->SetBrush(RectBrush(FLinearColor::Transparent, Color::Accent(), 1.f));
		Tag->SetPadding(FMargin(8.f, 2.f, 8.f, 3.f));
		Tag->SetContent(MakeText(Tree, Word, 9, Color::Accent(), true, 250));
		Tag->SetVisibility(ESlateVisibility::HitTestInvisible);
		return Tag;
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

		UOverlaySlot* ColumnSlot = Root->AddChildToOverlay(MakeSized(WidgetTree, MakeColumn(), ColumnWidth));
		ColumnSlot->SetHorizontalAlignment(HAlign_Left);
		ColumnSlot->SetVerticalAlignment(VAlign_Fill);

		// On top of everything: the delete confirmation, while it's open.
		PopupLayer = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Popup"));
		FillOverlaySlot(Root->AddChildToOverlay(PopupLayer));

		ShowMainButtons();
	}
	return Super::RebuildWidget();
}

UWidget* UMainMenuWidget::MakeColumn()
{
	UOverlay* Column = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Column"));

	// Dark glass, which follows the UI transparency setting; its cyan edge and the orange marks on it always show.
	UImage* Glass = MakeImage(WidgetTree, RectBrush(ColumnGlass()));
	MarkBackground(Glass);
	FillOverlaySlot(Column->AddChildToOverlay(Glass));

	UOverlaySlot* EdgeSlot = Column->AddChildToOverlay(MakeSized(WidgetTree, MakeImage(WidgetTree, RectBrush(Color::ScreenLine())), 1.5f));
	EdgeSlot->SetHorizontalAlignment(HAlign_Right);
	EdgeSlot->SetVerticalAlignment(VAlign_Fill);

	// Orange clamp ticks on the edge, as on the kit's panels, straddling it: half on the glass, half over the island.
	UVerticalBox* Clamp = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	for (int32 Notch = 0; Notch < 3; ++Notch)
	{
		UVerticalBoxSlot* NotchSlot = Clamp->AddChildToVerticalBox(MakeSized(WidgetTree,
			MakeImage(WidgetTree, RectBrush(Color::Accent(), Hex(122, 74, 0), 1.f)), 8.f, 6.f));
		NotchSlot->SetPadding(FMargin(0.f, Notch > 0 ? 3.f : 0.f, 0.f, 0.f));
	}
	Clamp->SetRenderTranslation(FVector2D(4.f, 0.f));
	UOverlaySlot* ClampSlot = Column->AddChildToOverlay(Clamp);
	ClampSlot->SetHorizontalAlignment(HAlign_Right);
	ClampSlot->SetVerticalAlignment(VAlign_Center);

	// A corner bracket over the title.
	UOverlaySlot* BracketSlot = Column->AddChildToOverlay(MakeImage(WidgetTree, ShapeBrush(EShape::Bracket, false, Color::Accent())));
	BracketSlot->SetHorizontalAlignment(HAlign_Left);
	BracketSlot->SetVerticalAlignment(VAlign_Top);
	BracketSlot->SetPadding(FMargin(28.f, 40.f, 0.f, 0.f));

	// Top to bottom: the title, the menu (the main buttons, or the session picker in their place), the key hint.
	UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Content->AddChildToVerticalBox(MakeTitle());
	MainButtons = MakeMainButtons();
	Content->AddChildToVerticalBox(MainButtons)->SetPadding(FMargin(0.f, 96.f, 0.f, 0.f));
	// The picker is tall (three cards), so it starts right under the title.
	Picker = MakePicker();
	Content->AddChildToVerticalBox(Picker)->SetPadding(FMargin(0.f, 48.f, 0.f, 0.f));
	Content->AddChildToVerticalBox(MakeSized(WidgetTree, nullptr, 0.f))->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	FooterText = MakeText(WidgetTree, TEXT(""), 10, Color::TextDim(), true, 200);
	Content->AddChildToVerticalBox(FooterText);
	FillOverlaySlot(Column->AddChildToOverlay(Content), FMargin(56.f, 88.f, 44.f, 36.f));
	return Column;
}

UWidget* UMainMenuWidget::MakeTitle()
{
	UVerticalBox* Title = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());

	// A short orange bar over the name.
	Title->AddChildToVerticalBox(MakeSized(WidgetTree, MakeImage(WidgetTree, RectBrush(Color::Accent())), 44.f, 4.f))
		->SetHorizontalAlignment(HAlign_Left);

	// The game's name, large. It shrinks rather than run past the column's edge (with a wider fallback font, say).
	UScaleBox* NameFit = WidgetTree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass());
	NameFit->SetStretch(EStretch::ScaleToFitX);
	NameFit->SetStretchDirection(EStretchDirection::DownOnly);
	if (UScaleBoxSlot* NameSlot = Cast<UScaleBoxSlot>(NameFit->SetContent(MakeText(WidgetTree, TEXT("AI Looter Shooter"), 34, Color::Title(), true, 60))))
	{
		NameSlot->SetHorizontalAlignment(HAlign_Left);
	}
	Title->AddChildToVerticalBox(NameFit)->SetPadding(FMargin(0.f, 14.f, 0.f, 0.f));

	// A thin cyan rule, then the world the game is set in, small and dim.
	Title->AddChildToVerticalBox(MakeSized(WidgetTree, MakeImage(WidgetTree, RectBrush(Color::ScreenLine())), 0.f, 1.f))
		->SetPadding(FMargin(0.f, 10.f, 0.f, 0.f));
	Title->AddChildToVerticalBox(MakeText(WidgetTree, TEXT("Skyreach"), 12, Color::TextDim(), true, 800))
		->SetPadding(FMargin(0.f, 10.f, 0.f, 0.f));
	return Title;
}

UWidget* UMainMenuWidget::MakeMainButtons()
{
	UVerticalBox* List = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	auto Add = [List](UWidget* Row, float Top)
	{
		UVerticalBoxSlot* RowSlot = List->AddChildToVerticalBox(Row);
		RowSlot->SetHorizontalAlignment(HAlign_Left);
		RowSlot->SetPadding(FMargin(0.f, Top, 0.f, 0.f));
	};
	auto Sized = [this](ULooterButton* Button)
	{
		return MakeSized(WidgetTree, Button, MainButtonWidth, MainButtonHeight);
	};

	Add(Sized(MakeButton(ActionSinglePlayer, 0, TEXT("Single Player"), 16, EButtonKind::Primary)), 0.f);

	// Multiplayer doesn't exist yet. Disabled, the kit's button draws dimmed, doesn't light up under the mouse and can't
	// be clicked; the tag beside it says why.
	ULooterButton* Multiplayer = MakeButton(ActionMultiplayer, 0, TEXT("Multiplayer"), 16, EButtonKind::Normal);
	Multiplayer->SetIsEnabled(false);
	UHorizontalBox* MultiplayerRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	MultiplayerRow->AddChildToHorizontalBox(Sized(Multiplayer));
	UHorizontalBoxSlot* TagSlot = MultiplayerRow->AddChildToHorizontalBox(MakeTag(WidgetTree, TEXT("Soon")));
	TagSlot->SetVerticalAlignment(VAlign_Center);
	TagSlot->SetPadding(FMargin(12.f, 0.f, 0.f, 0.f));
	Add(MultiplayerRow, MainButtonGap);

	Add(Sized(MakeButton(ActionSettings, 0, TEXT("Settings"), 16, EButtonKind::Normal)), MainButtonGap);
	// A little apart from the others: it ends the game.
	Add(Sized(MakeButton(ActionQuit, 0, TEXT("Quit Game"), 16, EButtonKind::Danger)), MainButtonGap * 2.f);
	return List;
}

// ---------------------------------------------------------------------------
// What's on screen
// ---------------------------------------------------------------------------

void UMainMenuWidget::ShowMainButtons()
{
	CloseDeleteConfirm();
	CloseNewGame();
	bPickerShown = false;
	if (MainButtons)
	{
		MainButtons->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	if (Picker)
	{
		Picker->SetVisibility(ESlateVisibility::Collapsed);
	}
	SetStatus(FString(), Color::TextDim());
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

void UMainMenuWidget::RefreshFooter()
{
	if (!FooterText)
	{
		return;
	}
	// Only what Escape does right now; on the main buttons it does nothing, so there's nothing to say.
	FString Hint;
	if (!bLoading && (DeleteIndex != INDEX_NONE || NewGameIndex != INDEX_NONE))
	{
		Hint = TEXT("Esc: cancel");
	}
	else if (!bLoading && bPickerShown)
	{
		Hint = TEXT("Esc: back");
	}
	FooterText->SetText(FText::FromString(Hint.ToUpper()));
}

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
	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		// One step back at a time: the popup, then the picker. On the main buttons it does nothing (the game is left
		// with its own button, never by a stray key), and nothing while a session's level opens.
		if (!bLoading)
		{
			if (DeleteIndex != INDEX_NONE)
			{
				CloseDeleteConfirm();
			}
			else if (NewGameIndex != INDEX_NONE)
			{
				CloseNewGame();
			}
			else if (bPickerShown)
			{
				ShowMainButtons();
			}
		}
		return FReply::Handled();
	}
	return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
}

FReply UMainMenuWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// A click on no button ends here rather than in the game viewport, which would take the keyboard from the menu.
	return FReply::Handled();
}

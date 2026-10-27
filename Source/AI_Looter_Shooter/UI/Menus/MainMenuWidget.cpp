#include "UI/Menus/MainMenuWidget.h"
#include "UI/Menus/HeroSelectWidget.h"
#include "UI/Menus/MainMenuArt.h"
#include "UI/Menus/MainMenuHUD.h"
#include "UI/Style/LooterButton.h"
#include "UI/Style/LooterUIStyle.h"
#include "Scenes/TransitionScreenSubsystem.h"
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

	/** A main button's press (Art/Sounds/recipes/opening.py): heavier than the kit's click, as the menu is pressed rarely. */
	const FName CueSelect(TEXT("UI.Menu.Select"));

	/**
	 * The shade under the menu runs past its side and thins out by here, so its edge is never seen as a line (the planet
	 * the menu's camera frames starts at the column's edge; the shade over it there is faint).
	 */
	constexpr float ShadeWidth = 1000.f;
	/** The menu's margins inside its side of the screen: room to breathe off the screen's edge, the title high. */
	const FMargin ColumnMargin(96.f, 104.f, 48.f, 44.f);

	/** The main buttons: words at 22 with room round them, so they read as a list, not a stack of boxes. */
	constexpr float MainButtonWidth = 440.f;
	constexpr float MainButtonHeight = 54.f;
	constexpr float MainButtonGap = 4.f;
	/** Quit Game stands a little apart: it ends the game. */
	constexpr float QuitGap = 22.f;
	constexpr int32 MainButtonFont = 22;
	/** The space between the title and the menu under it. */
	constexpr float ButtonsTop = 84.f;
	constexpr float PickerTop = 40.f;

	/** The title lockup: the mark, the title in the display type, the world's name under an orange rule. */
	constexpr float TitleMarkSize = 84.f;
	constexpr float TitleGlowSize = 190.f;
	constexpr int32 TitleFont = 60;
	constexpr int32 TitleSpacing = 360;
	constexpr float TitleRuleWidth = 64.f;

	/** When the parts arrive (seconds after the start, before Looter.Menu.Reveal.Speed) and how far they slide in. */
	constexpr float TitleDelay = 0.15f;
	constexpr float TitleSlide = 28.f;
	constexpr float FirstButtonDelay = 0.45f;
	/** One after another, quick enough to read as one movement. */
	constexpr float ButtonStagger = 0.07f;
	constexpr float ButtonSlide = 18.f;
	constexpr float FooterDelay = 0.85f;

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
	UWidget* Title = MakeTitle();
	Content->AddChildToVerticalBox(Title);
	AddRevealPart(Title, TitleDelay, TitleSlide);
	MainButtons = MakeMainButtons();
	Content->AddChildToVerticalBox(MainButtons)->SetPadding(FMargin(0.f, ButtonsTop, 0.f, 0.f));
	// The picker is tall (three cards), so it starts closer under the title.
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

UWidget* UMainMenuWidget::MakeTitle()
{
	UHorizontalBox* Lockup = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

	// The mark over its ember's warm glow (it breathes). The glow is laid out at the mark's size and drawn larger, so it
	// spreads past the mark without pushing the title away.
	UOverlay* Mark = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	TitleGlow = MakeImage(WidgetTree, GlowBrush(FVector2D(TitleMarkSize, TitleMarkSize), Color::Accent()));
	TitleGlow->SetRenderScale(FVector2D(TitleGlowSize / TitleMarkSize, TitleGlowSize / TitleMarkSize));
	FillOverlaySlot(Mark->AddChildToOverlay(TitleGlow));
	UOverlaySlot* MarkSlot = Mark->AddChildToOverlay(MakeImage(WidgetTree, PaintedIconBrush(TEXT("TitleMark"), MainMenuArt::TitleMark(), 3.f,
		FVector2D(TitleMarkSize, TitleMarkSize))));
	MarkSlot->SetHorizontalAlignment(HAlign_Center);
	MarkSlot->SetVerticalAlignment(VAlign_Center);
	Lockup->AddChildToHorizontalBox(MakeSized(WidgetTree, Mark, TitleMarkSize, TitleMarkSize))->SetVerticalAlignment(VAlign_Center);

	UVerticalBox* Words = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	// The game's title (the same words the cloud bank's title card rises with), in the kit's display type. It shrinks
	// rather than run past the menu's side (with a wider fallback font, say).
	TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	TitleText->SetText(UTransitionScreenSubsystem::GameTitle());
	TitleText->SetFont(DisplayFont(TitleFont, TitleSpacing));
	TitleText->SetColorAndOpacity(FSlateColor(Color::Title()));
	UScaleBox* TitleFit = WidgetTree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass());
	TitleFit->SetStretch(EStretch::ScaleToFitX);
	TitleFit->SetStretchDirection(EStretchDirection::DownOnly);
	if (UScaleBoxSlot* TitleSlot = Cast<UScaleBoxSlot>(TitleFit->SetContent(TitleText)))
	{
		TitleSlot->SetHorizontalAlignment(HAlign_Left);
	}
	Words->AddChildToVerticalBox(TitleFit);

	// Under it, an orange rule drawn out from the left (it grows as the menu arrives), then the world, Teropa (the
	// user's call, 2026-10-13), in the kit's cyan.
	UHorizontalBox* PlanetLine = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	TitleRule = MakeSized(WidgetTree, MakeImage(WidgetTree, RectBrush(Color::Accent())), TitleRuleWidth, 3.f);
	TitleRule->SetRenderTransformPivot(FVector2D(0.f, 0.5f));
	PlanetLine->AddChildToHorizontalBox(TitleRule)->SetVerticalAlignment(VAlign_Center);
	UHorizontalBoxSlot* PlanetSlot = PlanetLine->AddChildToHorizontalBox(MakeText(WidgetTree, TEXT("Teropa"), 13, Color::CyanText(), true, 900));
	PlanetSlot->SetVerticalAlignment(VAlign_Center);
	PlanetSlot->SetPadding(FMargin(14.f, 0.f, 0.f, 0.f));
	Words->AddChildToVerticalBox(PlanetLine)->SetPadding(FMargin(4.f, 4.f, 0.f, 0.f));

	UHorizontalBoxSlot* WordsSlot = Lockup->AddChildToHorizontalBox(Words);
	WordsSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	WordsSlot->SetVerticalAlignment(VAlign_Center);
	WordsSlot->SetPadding(FMargin(20.f, 0.f, 0.f, 0.f));
	return Lockup;
}

UWidget* UMainMenuWidget::MakeMainButton(FName Action, int32 Number, const FString& Label, bool bDanger, const FString& Tag)
{
	UOverlay* Body = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	// Under the words while lit: a faint orange wash from the left (a background, so the transparency fades it).
	UBorder* Wash = MakeBackdropPicture(WidgetTree, TEXT("MenuRowWash"), MainMenuArt::RowWash(), 4.f);
	FillOverlaySlot(Body->AddChildToOverlay(Wash));

	// The tick at the left (it rises and lights), the button's number, its words and any tag.
	UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UWidget* TickBar = MakeSized(WidgetTree, MakeImage(WidgetTree, RectBrush(Color::Accent())), 3.f, 28.f);
	TickBar->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	Line->AddChildToHorizontalBox(TickBar)->SetVerticalAlignment(VAlign_Center);
	UTextBlock* NumberText = MakeText(WidgetTree, FString::Printf(TEXT("%02d"), Number), 11, Color::TextDim(), true, 200);
	UHorizontalBoxSlot* NumberSlot = Line->AddChildToHorizontalBox(NumberText);
	NumberSlot->SetVerticalAlignment(VAlign_Center);
	NumberSlot->SetPadding(FMargin(16.f, 2.f, 0.f, 0.f));
	UTextBlock* Words = MakeText(WidgetTree, Label, MainButtonFont, Color::Text(), true, 260);
	UHorizontalBoxSlot* WordsSlot = Line->AddChildToHorizontalBox(Words);
	WordsSlot->SetVerticalAlignment(VAlign_Center);
	WordsSlot->SetPadding(FMargin(14.f, 0.f, 0.f, 0.f));
	if (!Tag.IsEmpty())
	{
		UHorizontalBoxSlot* TagSlot = Line->AddChildToHorizontalBox(MakeTag(WidgetTree, Tag));
		TagSlot->SetVerticalAlignment(VAlign_Center);
		TagSlot->SetPadding(FMargin(16.f, 0.f, 0.f, 0.f));
	}
	UOverlaySlot* LineSlot = Body->AddChildToOverlay(Line);
	LineSlot->SetHorizontalAlignment(HAlign_Fill);
	LineSlot->SetVerticalAlignment(VAlign_Center);

	// The kit's button, bare: its look is all in the content above (MainMenuMotion.cpp lights it).
	ULooterButton* Button = WidgetTree->ConstructWidget<ULooterButton>(ULooterButton::StaticClass());
	Button->SetupContent(Body, Action, 0, EButtonKind::Bare);
	Button->ClickCue = CueSelect;
	Button->OnButtonClicked.BindUObject(this, &UMainMenuWidget::HandleButton);
	Button->OnButtonHovered.BindUObject(this, &UMainMenuWidget::HandleButtonHovered);
	// Each made button is a stop for the keyboard and a controller too, top to bottom.
	MainNavButtons.Add(Button);

	FMainRow Row;
	Row.Button = Button;
	Row.Label = Words;
	Row.Number = NumberText;
	Row.Tick = TickBar;
	Row.Wash = Wash;
	Row.LitColor = bDanger ? Color::Worse() : Color::Accent();
	MainRows.Add(Row);
	return MakeSized(WidgetTree, Button, MainButtonWidth, MainButtonHeight);
}

UWidget* UMainMenuWidget::MakeMainButtons()
{
	UVerticalBox* List = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	MainNavButtons.Reset();
	MainRows.Reset();
	int32 Count = 0;
	auto Add = [this, List, &Count](UWidget* Row, float Top)
	{
		UVerticalBoxSlot* RowSlot = List->AddChildToVerticalBox(Row);
		RowSlot->SetHorizontalAlignment(HAlign_Left);
		RowSlot->SetPadding(FMargin(0.f, Top, 0.f, 0.f));
		AddRevealPart(Row, FirstButtonDelay + ButtonStagger * Count++, ButtonSlide);
	};

	Add(MakeMainButton(ActionSinglePlayer, 1, TEXT("Single Player"), false), 0.f);
	// Multiplayer doesn't exist yet. Disabled, the button can't be clicked or lit (the focus passes over it); its words
	// show dim, and the tag beside them says why.
	Add(MakeMainButton(ActionMultiplayer, 2, TEXT("Multiplayer"), false, TEXT("Soon")), MainButtonGap);
	if (ULooterButton* Multiplayer = GetMainButton(ActionMultiplayer))
	{
		Multiplayer->SetIsEnabled(false);
	}
	Add(MakeMainButton(ActionSettings, 3, TEXT("Settings"), false), MainButtonGap);
	Add(MakeMainButton(ActionQuit, 4, TEXT("Quit Game"), true), QuitGap);
	return List;
}

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

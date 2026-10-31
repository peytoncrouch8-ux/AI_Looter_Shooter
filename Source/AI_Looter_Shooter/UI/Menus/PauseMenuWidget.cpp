// UPauseMenuWidget: the full-screen pause hub (E2 "Big Dial"). This file holds the options' words, puts the hub together
// on its stage (the design space of PauseMenuLayout, fitted to the screen), opens it and says what was picked. Its pieces
// are built in PauseMenuPieces.cpp and PauseMenuCards.cpp, driven by keys in PauseMenuNav.cpp, moved in
// PauseMenuMotion.cpp.

#include "UI/Menus/PauseMenuWidget.h"
#include "UI/Menus/PauseMenuArt.h"
#include "UI/Menus/PauseMenuLayout.h"
#include "UI/Style/LooterButton.h"
#include "UI/Style/LooterReticleArt.h"
#include "UI/Style/LooterUIStyle.h"
#include "Areas/AreaDefinition.h"
#include "Areas/AreaRulesSubsystem.h"
#include "Session/SessionSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/SizeBoxSlot.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Widgets/Layout/Anchors.h"

using namespace PauseMenuLayout;

namespace
{
	const EPauseMenuItem ItemRows[] = {
		EPauseMenuItem::Resume,
		EPauseMenuItem::Settings,
		EPauseMenuItem::SaveQuit,
		EPauseMenuItem::QuitGame,
	};
	static_assert(UE_ARRAY_COUNT(ItemRows) == NumRows, "One row of the layout for each option");

	/** The backdrop's field and its texture (PauseMenuArt::Backdrop: 96 x 54 at 1 px a unit, stretched over the screen). */
	const FVector2D BackdropField(96.f, 54.f);
}

// ---------------------------------------------------------------------------
// The options' words
// ---------------------------------------------------------------------------

TConstArrayView<EPauseMenuItem> UPauseMenuWidget::ItemOrder()
{
	return MakeArrayView(ItemRows);
}

FString UPauseMenuWidget::ItemWord(EPauseMenuItem Item)
{
	switch (Item)
	{
	case EPauseMenuItem::Resume:   return TEXT("BACK TO GAME");
	case EPauseMenuItem::Settings: return TEXT("SETTINGS");
	case EPauseMenuItem::SaveQuit: return TEXT("SAVE & QUIT");
	case EPauseMenuItem::QuitGame: return TEXT("QUIT GAME");
	default:                       return FString();
	}
}

FString UPauseMenuWidget::ItemHelp(EPauseMenuItem Item)
{
	// The mockup's words (the contract).
	switch (Item)
	{
	case EPauseMenuItem::Resume:   return TEXT("Close the menu and pick up where you stopped.");
	case EPauseMenuItem::Settings: return TEXT("Graphics, audio, interface and controls.");
	case EPauseMenuItem::SaveQuit: return TEXT("Save and return to the main menu.");
	case EPauseMenuItem::QuitGame: return TEXT("Save and close the game.");
	default:                       return FString();
	}
}

FString UPauseMenuWidget::ItemFlavor(EPauseMenuItem Item, bool bSession)
{
	switch (Item)
	{
	case EPauseMenuItem::Resume:   return TEXT("The world waits while you're here.");
	case EPauseMenuItem::Settings: return TEXT("Changes apply as you make them.");
	case EPauseMenuItem::SaveQuit:
	case EPauseMenuItem::QuitGame:
		// Played without a session (Looter.PlayLevel, a level from the editor) nothing is written: it mustn't say it is.
		return bSession ? TEXT("Your session is saved first.") : TEXT("No session is played: nothing is saved.");
	default:
		return FString();
	}
}

bool UPauseMenuWidget::SavesFirst(EPauseMenuItem Item)
{
	return Item == EPauseMenuItem::SaveQuit || Item == EPauseMenuItem::QuitGame;
}

// ---------------------------------------------------------------------------
// Opening
// ---------------------------------------------------------------------------

void UPauseMenuWidget::Open(EPauseMenuItem Focus, bool bReturning)
{
	SetIsFocusable(true);
	bLeaving = false;
	OpenFocus = Focus;
	bOpenReturning = bReturning;
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
	RefreshCards();
	RebuildNavigation(GetItemButton(OpenFocus));
	RefreshNavLook();
	BeginOpenMotion();
}

ULooterButton* UPauseMenuWidget::GetItemButton(EPauseMenuItem Item) const
{
	const FPauseMenuOption* Option = FindOption(Item);
	return Option ? Option->Button.Get() : nullptr;
}

FString UPauseMenuWidget::GetShownWord(EPauseMenuItem Item) const
{
	const FPauseMenuOption* Option = FindOption(Item);
	return Option && Option->Word.IsValid() ? Option->Word->GetText().ToString() : ItemWord(Item);
}

FPauseMenuOption* UPauseMenuWidget::FindOption(EPauseMenuItem Item)
{
	return Options.FindByPredicate([Item](const FPauseMenuOption& Option) { return Option.Item == Item; });
}

const FPauseMenuOption* UPauseMenuWidget::FindOption(EPauseMenuItem Item) const
{
	return Options.FindByPredicate([Item](const FPauseMenuOption& Option) { return Option.Item == Item; });
}

const FPauseMenuOption* UPauseMenuWidget::FindOption(const ULooterButton* Button) const
{
	return Button ? Options.FindByPredicate([Button](const FPauseMenuOption& Option) { return Option.Button.Get() == Button; }) : nullptr;
}

void UPauseMenuWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	FitStage();
	AdvanceNav(InDeltaTime);
	AdvanceLook(InDeltaTime);
}

void UPauseMenuWidget::FitStage()
{
	// 1080 tall; as wide as the screen's shape asks, never under 1920 (a narrower screen scales the whole down instead, in
	// the scale box). A wider one keeps the dial on the left edge and the cards on the right.
	const FVector2D Size(GetCachedGeometry().GetLocalSize());
	if (!StageBox || Size.X < 1.0 || Size.Y < 1.0)
	{
		return;
	}
	const float Wanted = FMath::Max(DesignWidth, static_cast<float>(DesignHeight * Size.X / Size.Y));
	if (FMath::Abs(Wanted - StageWidth) > 0.5f)
	{
		StageWidth = Wanted;
		StageBox->SetWidthOverride(Wanted);
	}
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

		// The paused game dimmed, darker to the left, its edges in a vignette (a background). It takes the clicks off the game.
		Backdrop = MakeImage(WidgetTree, PaintedIconBrush(TEXT("PauseHubBackdrop"), PauseMenuArt::Backdrop(), 1.f, BackdropField));
		Backdrop->SetVisibility(ESlateVisibility::Visible);
		MarkBackground(Backdrop);
		UOverlaySlot* BackdropSlot = Root->AddChildToOverlay(Backdrop);
		BackdropSlot->SetHorizontalAlignment(HAlign_Fill);
		BackdropSlot->SetVerticalAlignment(VAlign_Fill);

		// The stage: the design space, scaled down to fit a screen narrower than 16:9 (FitStage widens it for a wider one).
		Stage = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Stage"));
		Stage->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		StageWidth = DesignWidth;
		StageBox = MakeSized(WidgetTree, Stage, DesignWidth, DesignHeight);
		UScaleBox* Fit = WidgetTree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass());
		Fit->SetStretch(EStretch::ScaleToFit);
		Fit->SetStretchDirection(EStretchDirection::DownOnly);
		Fit->SetContent(StageBox);
		Fit->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		UOverlaySlot* FitSlot = Root->AddChildToOverlay(Fit);
		FitSlot->SetHorizontalAlignment(HAlign_Fill);
		FitSlot->SetVerticalAlignment(VAlign_Fill);

		// Back to front: the title and the dial, the options over the arc, their callouts, the cards, the keys.
		Options.Reset();
		ItemButtons.Reset();
		MakeTitle();
		MakeDial();
		for (int32 Row = 0; Row < NumRows; ++Row)
		{
			MakeOption(ItemRows[Row], Row);
		}
		for (FPauseMenuOption& Option : Options)
		{
			MakeCallout(Option);
		}
		MakeHeroCard();
		MakeMissionCard();
		MakeSessionCard();
		MakeKeys();

		ApplyOpen();
	}
	return Super::RebuildWidget();
}

FSlateBrush UPauseMenuWidget::PictureBrush(const TCHAR* Base, const LooterUI::FPaintedIcon& Picture, float Resolution) const
{
	const FName Name(*FString::Printf(TEXT("%s_%dx%d"), Base, FMath::RoundToInt32(Picture.ViewBox.X * 10.0), FMath::RoundToInt32(Picture.ViewBox.Y * 10.0)));
	return LooterUI::PaintedIconBrush(Name, Picture, Resolution > 0.f ? Resolution : PictureResolution, Picture.ViewBox);
}

UImage* UPauseMenuWidget::MakePicture(const TCHAR* Base, const LooterUI::FPaintedIcon& Picture, float Resolution)
{
	return LooterUI::MakeImage(WidgetTree, PictureBrush(Base, Picture, Resolution));
}

USizeBox* UPauseMenuWidget::MakeBand(UWidget* Content, float Width, float Height, EHorizontalAlignment Across)
{
	USizeBox* Box = LooterUI::MakeSized(WidgetTree, Content, Width, Height);
	if (USizeBoxSlot* BoxSlot = Cast<USizeBoxSlot>(Box->GetContentSlot()))
	{
		BoxSlot->SetHorizontalAlignment(Across);
		BoxSlot->SetVerticalAlignment(VAlign_Center);
	}
	Box->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	return Box;
}

UCanvasPanelSlot* UPauseMenuWidget::Put(UCanvasPanel* Canvas, UWidget* Widget, const FVector2D& Position, const FVector2D& Size, bool bRight)
{
	UCanvasPanelSlot* PieceSlot = Canvas->AddChildToCanvas(Widget);
	const bool bOwnSize = Size.IsNearlyZero();
	PieceSlot->SetAutoSize(bOwnSize);
	if (!bOwnSize)
	{
		PieceSlot->SetSize(Size);
	}
	if (bRight)
	{
		// Kept to the right edge: a wider stage moves it with the edge.
		PieceSlot->SetAnchors(FAnchors(1.f, 0.f));
		PieceSlot->SetPosition(FVector2D(Position.X - DesignWidth, Position.Y));
	}
	else
	{
		PieceSlot->SetPosition(Position);
	}
	return PieceSlot;
}

UCanvasPanel* UPauseMenuWidget::MakeGroup(const FVector2D& Position, const FVector2D& Size, bool bRight)
{
	UCanvasPanel* Group = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
	Group->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	Put(Stage, Group, Position, Size, bRight);
	return Group;
}

UWidget* UPauseMenuWidget::MakeShape(const TCHAR* Base, const LooterUI::FPaintedIcon& Glass, const LooterUI::FPaintedIcon& Lines, const FVector2D& Size)
{
	// The glass under (a background: the UI transparency fades it), the lines over; both reach Pad past the box.
	UCanvasPanel* Shape = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
	Shape->SetVisibility(ESlateVisibility::HitTestInvisible);
	const FVector2D Corner(-LooterReticle::Pad, -LooterReticle::Pad);
	UImage* GlassImage = MakePicture(*FString::Printf(TEXT("%sGlass"), Base), Glass);
	LooterUI::MarkBackground(GlassImage);
	Put(Shape, GlassImage, Corner, FVector2D::ZeroVector);
	Put(Shape, MakePicture(*FString::Printf(TEXT("%sLines"), Base), Lines), Corner, FVector2D::ZeroVector);
	return LooterUI::MakeSized(WidgetTree, Shape, Size.X, Size.Y);
}

// ---------------------------------------------------------------------------
// Words
// ---------------------------------------------------------------------------

void UPauseMenuWidget::RefreshWords()
{
	// The area's name, as the map page names it.
	const UWorld* World = GetWorld();
	const UAreaRulesSubsystem* Rules = World ? World->GetSubsystem<UAreaRulesSubsystem>() : nullptr;
	const UAreaDefinition* Area = Rules ? Rules->GetArea() : nullptr;
	const FString AreaName = Area && !Area->DisplayName.IsEmpty() ? Area->DisplayName.ToString()
		: (World ? USessionSubsystem::AreaName(USessionSubsystem::MapOf(World)) : FString());
	if (AreaText)
	{
		AreaText->SetText(FText::FromString(AreaName.ToUpper()));
	}

	// The saves' flavor lines say whether anything is written (no session: nothing is).
	const USessionSubsystem* Sessions = USessionSubsystem::Get(this);
	const bool bSession = Sessions && Sessions->IsPlayingSession();
	for (FPauseMenuOption& Option : Options)
	{
		if (UTextBlock* Word = Option.Word.Get())
		{
			Word->SetText(FText::FromString(ItemWord(Option.Item)));
		}
		if (UTextBlock* Flavor = Option.CalloutFlavor.Get())
		{
			Flavor->SetText(FText::FromString(ItemFlavor(Option.Item, bSession)));
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
	if (SavesFirst(Item))
	{
		// The level is on its way out: its tag says what's happening, and nothing more is taken meanwhile.
		bLeaving = true;
		const USessionSubsystem* Sessions = USessionSubsystem::Get(this);
		const bool bSession = Sessions && Sessions->IsPlayingSession();
		if (FPauseMenuOption* Option = FindOption(Item); Option && Option->Word.IsValid())
		{
			Option->Word->SetText(FText::FromString(bSession ? TEXT("SAVING...") : Item == EPauseMenuItem::QuitGame ? TEXT("CLOSING...") : TEXT("LEAVING...")));
		}
	}
	OnChoice.ExecuteIfBound(Item);
}

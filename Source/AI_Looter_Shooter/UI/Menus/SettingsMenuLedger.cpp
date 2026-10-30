// USettingsMenuWidget's round 3 look (the user's mix of concept 1 "Ledger & Brass" and concept 3 "Halo Reticle",
// 2026-10-29; Docs/Work/2026-10-29-UI3Build/CONTRACT.md, lane P): the pieces that make it (the ledger's rows and margin,
// the header and its stamped plate, Back in its reticle, the footer's serial, the halo breaking out over the
// plate's corner, the ribbon) and the plate's words. Their motion is SettingsMenuLedgerMotion.cpp's; SettingsMenuArt.h
// paints the pieces; LooterReticleArt.h draws c3's pictures.

#include "UI/Menus/SettingsMenuWidget.h"
#include "UI/Menus/SettingsMenuArt.h"
#include "UI/Menus/SettingsMenuHelp.h"
#include "UI/Menus/SettingsMenuParts.h"
#include "UI/Style/LooterButton.h"
#include "UI/Style/LooterReticleArt.h"
#include "UI/Style/LooterUIStyle.h"
#include "Areas/AreaDefinition.h"
#include "Areas/AreaRulesSubsystem.h"
#include "Audio/LooterSound.h"
#include "Progression/PlayerProgressionSubsystem.h"
#include "Session/SessionSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"

using namespace SettingsMenu;

namespace
{
	/** The range arc in the plate's bottom-left corner (c3), its ticks every 6 degrees. */
	constexpr float CornerArcRadius = 90.f;
	/** Resume's small reticle: its radius and where its middle sits from the row's left (c3: 56 px in). */
	constexpr float ResumeReticleRadius = 17.f;
	constexpr float ResumeReticleX = 56.f;
	/** The serial's room from the column's left, clear of the corner's range arc. */
	constexpr float SerialIndent = 112.f;
	/** The pause bars in the halo (c3: two 13 x 60 bars, 14 px apart). */
	constexpr float PauseBarWidth = 13.f;
	constexpr float PauseBarHeight = 60.f;
	constexpr float PauseBarGap = 14.f;

	/** A picture of the kit's painter as an image at its own size (twice the resolution, so it stays crisp). */
	UImage* MakePicture(UWidgetTree* Tree, FName Name, const LooterUI::FPaintedIcon& Picture)
	{
		return LooterUI::MakeImage(Tree, LooterUI::PaintedIconBrush(Name, Picture, 2.f, Picture.ViewBox));
	}
}

// ---------------------------------------------------------------------------
// The ledger's pieces
// ---------------------------------------------------------------------------

ULooterButton* USettingsMenuWidget::MakeArtButton(FName Action, int32 Index, const FString& Label, ESettingsArt Look, int32 FontSize,
	int32 LetterSpacing, LooterUI::EButtonKind SoundKind)
{
	ULooterButton* Button = WidgetTree->ConstructWidget<ULooterButton>(ULooterButton::StaticClass());
	USettingsMenuArt* Art = USettingsMenuArt::Make(WidgetTree, Look);
	Art->SetLabel(Label.ToUpper(), FontSize, LetterSpacing);
	Art->SetButton(Button);
	Button->SetupContent(Art, Action, Index, LooterUI::EButtonKind::Bare);
	// The look is the painting's; the sound stays what the kit's button of that kind made (a switch's or a tab's click).
	Button->ClickCue = Action == ActionClose ? FName(LooterSoundCue::Close) : ULooterButton::ClickCueFor(Action, SoundKind);
	Button->OnButtonClicked.BindUObject(this, &USettingsMenuWidget::HandleButton);
	Button->OnButtonHovered.BindUObject(this, &USettingsMenuWidget::HandleButtonHovered);
	return Button;
}

UWidget* USettingsMenuWidget::MakeLedgerRow(UWidget* Content, float Height, bool bNumbered, bool bKeyList)
{
	using namespace LooterUI;
	UOverlay* Row = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	// Under everything, its focus look, hidden until lit: a gold wash out from the margin (a background) and the gold
	// double rule of a ledger's total along its foot (c1's Resume).
	UOverlay* Light = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	Light->SetVisibility(ESlateVisibility::Collapsed);
	USettingsMenuArt* Wash = USettingsMenuArt::Make(WidgetTree, ESettingsArt::RowWash);
	Wash->SetMarginX(LedgerMargin);
	MarkBackground(Wash);
	FillOverlaySlot(Light->AddChildToOverlay(Wash));
	USettingsMenuArt* Rule = USettingsMenuArt::Make(WidgetTree, ESettingsArt::RowRule);
	Rule->SetMarginX(LedgerMargin);
	FillOverlaySlot(Light->AddChildToOverlay(Rule));
	FillOverlaySlot(Row->AddChildToOverlay(Light));
	// The ruled hairline along its foot; it takes the pointer for the whole row, so pointing anywhere on it marks it.
	FillOverlaySlot(Row->AddChildToOverlay(USettingsMenuArt::Make(WidgetTree, ESettingsArt::LedgerLine)));

	// Its number stamped in the margin, then the row itself past the margin's rule.
	UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UTextBlock* Number = nullptr;
	if (bNumbered && PageRowNumbers.IsValidIndex(BuildPage))
	{
		Number = MakeText(WidgetTree, FString::Printf(TEXT("%02d"), PageRowNumbers[BuildPage]++), 17, Color::SlateIcon(), false, 118);
		Number->SetJustification(ETextJustify::Center);
	}
	UHorizontalBoxSlot* NumberSlot = Line->AddChildToHorizontalBox(MakeSized(WidgetTree, Number, LedgerMargin));
	NumberSlot->SetVerticalAlignment(VAlign_Center);
	UHorizontalBoxSlot* ContentSlot = Line->AddChildToHorizontalBox(Content);
	ContentSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	ContentSlot->SetVerticalAlignment(VAlign_Center);
	ContentSlot->SetPadding(FMargin(LedgerGap, 0.f, 6.f, 0.f));
	USizeBox* Sized = MakeSized(WidgetTree, Line, 0.f);
	Sized->SetMinDesiredHeight(Height);
	FillOverlaySlot(Row->AddChildToOverlay(Sized));

	FSettingsLedgerLine& Entry = LedgerLines.AddDefaulted_GetRef();
	Entry.Row = Row;
	Entry.Light = Light;
	Entry.Number = Number;
	Entry.Page = BuildPage;
	Entry.bKeyList = bKeyList;
	return Row;
}

UWidget* USettingsMenuWidget::LedgerLightOf(const UWidget* Row) const
{
	const FSettingsLedgerLine* Found = LedgerLines.FindByPredicate([Row](const FSettingsLedgerLine& Line) { return Line.Row.Get() == Row; });
	return Found ? Found->Light.Get() : nullptr;
}

UWidget* USettingsMenuWidget::MakeLedgerSection(const FString& Label)
{
	using namespace LooterUI;
	// A heading written in from the margin, gold and spaced like the description column's title.
	UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Line->AddChildToHorizontalBox(MakeSized(WidgetTree, nullptr, LedgerMargin + LedgerGap));
	UHorizontalBoxSlot* TitleSlot = Line->AddChildToHorizontalBox(MakeText(WidgetTree, Label, 15, Color::SoulGold(), true, 235));
	TitleSlot->SetVerticalAlignment(VAlign_Center);
	TitleSlot->SetPadding(FMargin(0.f, 12.f, 0.f, 8.f));
	return Line;
}

// ---------------------------------------------------------------------------
// The header, Resume, the footer
// ---------------------------------------------------------------------------

UWidget* USettingsMenuWidget::MakeHeader()
{
	using namespace LooterUI;
	// The title (ApplyMode's) at c3's size and spacing, the stamped plate of c1 beside it, c3's mil-dot ruler under both.
	UHorizontalBox* TitleLine = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	HeaderText = MakeText(WidgetTree, TEXT(""), 38, Color::SlateText(), true, 200);
	TitleLine->AddChildToHorizontalBox(HeaderText)->SetVerticalAlignment(VAlign_Center);
	TitleLine->AddChildToHorizontalBox(MakeSized(WidgetTree, nullptr, 0.f))->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	AreaPlate = USettingsMenuArt::Make(WidgetTree, ESettingsArt::AreaPlate);
	AreaPlate->SetLabel(TEXT(""), 13, 160);
	UHorizontalBoxSlot* PlateSlot = TitleLine->AddChildToHorizontalBox(AreaPlate);
	PlateSlot->SetVerticalAlignment(VAlign_Center);
	PlateSlot->SetPadding(FMargin(16.f, 0.f, 0.f, 0.f));

	UVerticalBox* Titles = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Titles->AddChildToVerticalBox(TitleLine);
	const float RulerLength = PanelWidth - PlatePadX * 2.f - TitleIndent - LooterReticle::Pad * 2.f;
	UVerticalBoxSlot* RulerSlot = Titles->AddChildToVerticalBox(MakePicture(WidgetTree, TEXT("SettingsTitleRuler"),
		LooterReticle::Ruler(RulerLength, 42.f, 3, true)));
	// Its line starts under the title's first letter (the picture keeps a little room round its lines).
	RulerSlot->SetHorizontalAlignment(HAlign_Left);
	RulerSlot->SetPadding(FMargin(-LooterReticle::Pad, 4.f, 0.f, 0.f));

	UHorizontalBox* Header = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Header->AddChildToHorizontalBox(MakeSized(WidgetTree, nullptr, TitleIndent));
	Header->AddChildToHorizontalBox(Titles)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	return Header;
}

UWidget* USettingsMenuWidget::MakeResumeRow()
{
	using namespace LooterUI;
	// c3's choice held in a reticle: the brackets ( ) round the row, the small reticle at its left (it turns while Resume is
	// marked), the word gold in the middle, a soft gold wash between the brackets that strengthens pointed at or focused.
	UOverlay* Look = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	USettingsMenuArt* Wash = USettingsMenuArt::Make(WidgetTree, ESettingsArt::ResumeWash);
	MarkBackground(Wash);
	FillOverlaySlot(Look->AddChildToOverlay(Wash));
	const float Edge = LooterReticle::BracketEdgeX(ResumeHeight);
	UOverlaySlot* LeftSlot = Look->AddChildToOverlay(MakePicture(WidgetTree, TEXT("SettingsResumeBracketL"), LooterReticle::Bracket(ResumeHeight, false)));
	LeftSlot->SetHorizontalAlignment(HAlign_Left);
	LeftSlot->SetVerticalAlignment(VAlign_Center);
	LeftSlot->SetPadding(FMargin(-Edge, 0.f, 0.f, 0.f));
	UOverlaySlot* RightSlot = Look->AddChildToOverlay(MakePicture(WidgetTree, TEXT("SettingsResumeBracketR"), LooterReticle::Bracket(ResumeHeight, true)));
	RightSlot->SetHorizontalAlignment(HAlign_Right);
	RightSlot->SetVerticalAlignment(VAlign_Center);
	RightSlot->SetPadding(FMargin(0.f, 0.f, -Edge, 0.f));
	ResumeReticle = MakePicture(WidgetTree, TEXT("SettingsResumeReticle"), LooterReticle::SmallReticle(ResumeReticleRadius));
	UOverlaySlot* ReticleSlot = Look->AddChildToOverlay(ResumeReticle);
	ReticleSlot->SetHorizontalAlignment(HAlign_Left);
	ReticleSlot->SetVerticalAlignment(VAlign_Center);
	ReticleSlot->SetPadding(FMargin(ResumeReticleX - LooterReticle::SmallReticleExtent(ResumeReticleRadius), 0.f, 0.f, 0.f));
	// The word is ApplyMode's (BACK: to the pause menu's list over the game, to the main menu from it).
	ResumeWord = MakeText(WidgetTree, TEXT("Back"), 32, Color::SoulGold(), true, 340);
	ResumeWord->SetVisibility(ESlateVisibility::HitTestInvisible);
	UOverlaySlot* WordSlot = Look->AddChildToOverlay(ResumeWord);
	WordSlot->SetHorizontalAlignment(HAlign_Center);
	WordSlot->SetVerticalAlignment(VAlign_Center);
	WordSlot->SetPadding(FMargin(40.f, 0.f, 0.f, 0.f));

	ResumeButton = WidgetTree->ConstructWidget<ULooterButton>(ULooterButton::StaticClass());
	ResumeButton->SetupContent(Look, ActionClose, 0, EButtonKind::Bare);
	Wash->SetButton(ResumeButton);
	// Closing is the page's close (over the game the HUD plays the same cue in the same moment, heard as one).
	ResumeButton->ClickCue = LooterSoundCue::Close;
	ResumeButton->OnButtonClicked.BindUObject(this, &USettingsMenuWidget::HandleButton);
	ResumeButton->OnButtonHovered.BindUObject(this, &USettingsMenuWidget::HandleButtonHovered);

	UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Line->AddChildToHorizontalBox(MakeSized(WidgetTree, nullptr, ResumeIndent));
	UHorizontalBoxSlot* ButtonSlot = Line->AddChildToHorizontalBox(MakeSized(WidgetTree, ResumeButton, 0.f, ResumeHeight));
	ButtonSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	ButtonSlot->SetPadding(FMargin(0.f, 0.f, 30.f, 0.f));
	return MakeSized(WidgetTree, Line, 0.f, ResumeHeight);
}

UWidget* USettingsMenuWidget::MakeFooter()
{
	using namespace LooterUI;
	UHorizontalBox* Footer = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	// The plate's serial, small and spaced, bottom-left past the corner's range arc (over the game: RefreshPlate).
	SerialText = MakeText(WidgetTree, TEXT(""), 14, Color::SlateIcon().CopyWithNewOpacity(0.75f), true, 357);
	UHorizontalBoxSlot* SerialSlot = Footer->AddChildToHorizontalBox(SerialText);
	SerialSlot->SetVerticalAlignment(VAlign_Bottom);
	SerialSlot->SetPadding(FMargin(SerialIndent, 0.f, 0.f, 8.f));
	Footer->AddChildToHorizontalBox(MakeSized(WidgetTree, nullptr, 0.f))->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	// Save & Quit's stamp stood here until the user moved it to the pause menu's list (2026-10-29 night: "Remove save and
	// quit from the settings menu at the bottom"); UPauseMenuWidget stamps it now.

	// Back: a chosen chip, gold, kept hidden (ApplyMode): going back is the reticle row at the top.
	BackButton = MakeArtButton(ActionClose, 0, TEXT("Back"), ESettingsArt::Chip, 20, 230, EButtonKind::Primary);
	BackButton->SetHighlighted(true);
	Footer->AddChildToHorizontalBox(MakeSized(WidgetTree, BackButton, 230.f, 56.f))->SetVerticalAlignment(VAlign_Center);
	return Footer;
}

// ---------------------------------------------------------------------------
// The free art over the plate
// ---------------------------------------------------------------------------

void USettingsMenuWidget::MakePlateArt()
{
	using namespace LooterUI;
	if (!PlateArt)
	{
		return;
	}
	auto Place = [this](UWidget* Widget, const FVector2D& Position, const FVector2D& Alignment)
	{
		UCanvasPanelSlot* ArtSlot = PlateArt->AddChildToCanvas(Widget);
		ArtSlot->SetAutoSize(true);
		ArtSlot->SetAlignment(Alignment);
		ArtSlot->SetPosition(Position);
		return ArtSlot;
	};

	// c3's range arc in the bottom-left corner, its circle's centre on the corner inside the trim; quieter than the lines.
	const FPaintedIcon Arc = LooterReticle::ArcScale(CornerArcRadius, -90.f, 0.f, 6.f, 5);
	UImage* ArcImage = MakePicture(WidgetTree, TEXT("SettingsCornerArc"), Arc);
	ArcImage->SetColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, 0.55f));
	Place(ArcImage, FVector2D(PlatePadX, PanelHeight - PlatePadBottom) - LooterReticle::ArcScaleCentre(CornerArcRadius, -90.f, 0.f, 6.f, 5),
		FVector2D::ZeroVector);

	// The halo, breaking out over the top-left corner: its dark disc (a background), the ring of ticks with its cross hairs,
	// the gold arcs (both spin up as the menu opens), the pause bars over the game; its range marks round its upper left.
	const float Extent = LooterReticle::HaloExtent(HaloRadius);
	UOverlay* Halo = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	auto Centre = [Halo](UWidget* Widget)
	{
		UOverlaySlot* CentreSlot = Halo->AddChildToOverlay(Widget);
		CentreSlot->SetHorizontalAlignment(HAlign_Center);
		CentreSlot->SetVerticalAlignment(VAlign_Center);
	};
	UImage* Disc = MakeImage(WidgetTree, CircleBrush(Color::SlateGlassFoot()));
	MarkBackground(Disc);
	Centre(MakeSized(WidgetTree, Disc, HaloRadius * 2.f, HaloRadius * 2.f));
	HaloRing = MakePicture(WidgetTree, TEXT("SettingsHalo"), LooterReticle::Halo(HaloRadius, 6.f, 5, true, 0.f, 0.f));
	Centre(HaloRing);
	HaloGold = MakePicture(WidgetTree, TEXT("SettingsHaloGold"), SettingsMenuArt::HaloArcs(HaloRadius - 14.f));
	Centre(HaloGold);
	UHorizontalBox* Bars = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Bars->AddChildToHorizontalBox(MakeSized(WidgetTree, MakeImage(WidgetTree, RectBrush(Color::SlateText())), PauseBarWidth, PauseBarHeight));
	Bars->AddChildToHorizontalBox(MakeSized(WidgetTree, MakeImage(WidgetTree, RectBrush(Color::SlateText())), PauseBarWidth, PauseBarHeight))
		->SetPadding(FMargin(PauseBarGap, 0.f, 0.f, 0.f));
	PauseBars = Bars;
	Centre(Bars);
	Place(MakeSized(WidgetTree, Halo, Extent * 2.f, Extent * 2.f), FVector2D(HaloCentreX, HaloCentreY), FVector2D(0.5f, 0.5f));
	const FVector2D HaloCentre(HaloCentreX, HaloCentreY);
	struct FRangeMark
	{
		float Degrees;
		const TCHAR* Words;
	};
	const FRangeMark Marks[] = { { 200.f, TEXT("100") }, { 230.f, TEXT("200") }, { 260.f, TEXT("300") }, { 290.f, TEXT("400") } };
	for (const FRangeMark& Mark : Marks)
	{
		const float Angle = FMath::DegreesToRadians(Mark.Degrees);
		UTextBlock* Range = MakeFloatingText(WidgetTree, 13, Color::SlateLabel(), 77, ETextJustify::Center);
		Range->SetText(FText::FromString(Mark.Words));
		Place(Range, HaloCentre + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * (Extent + 14.f), FVector2D(0.5f, 0.5f));
	}

	// The ribbon, its tails out past the plate's left edge at the row marked (AdvanceLook slides it there).
	Ribbon = USettingsMenuArt::Make(WidgetTree, ESettingsArt::Ribbon);
	Ribbon->SetRenderOpacity(0.f);
	Place(Ribbon, FVector2D(SettingsMenuArt::RibbonLeft, 0.f), FVector2D::ZeroVector);
}

void USettingsMenuWidget::RefreshPlate()
{
	using namespace LooterUI;
	const bool bPause = Mode == ESettingsMenuMode::Pause;
	// The session played (none from Looter.PlayLevel: 0) and the area's name, as the map page names it.
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
		AreaPlate->SetVisibility(bPause && !Words.IsEmpty() ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (SerialText)
	{
		SerialText->SetText(FText::FromString(SettingsMenuArt::SerialText(Session, Level)));
		SerialText->SetVisibility(bPause ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	// The halo holds the pause bars over the paused game; from the main menu only its plain cross.
	if (PauseBars)
	{
		PauseBars->SetVisibility(bPause ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}


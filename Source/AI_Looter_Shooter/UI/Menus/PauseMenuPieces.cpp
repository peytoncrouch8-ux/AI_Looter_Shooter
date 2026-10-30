// UPauseMenuWidget's pieces in the settings menu's round 3 look (the user's mix of concepts 1 "Ledger & Brass" and 3 "Halo
// Reticle", 2026-10-29): GAME PAUSED with the area's stamped plate over c3's dotted ruler, RESUME GAME in c3's reticle, the
// ledger's numbered rows, Save & Quit's rubber stamp with the serial, and the free art over the plate (the halo with the
// pause bars on its top-left corner, the corner's range arc, the ribbon). SettingsMenuArt.h paints the plate's pieces;
// LooterReticleArt.h draws c3's pictures. PauseMenuWidget.cpp puts them together.

#include "UI/Menus/PauseMenuWidget.h"
#include "UI/Menus/SettingsMenuArt.h"
#include "UI/Menus/SettingsMenuParts.h"
#include "UI/Style/LooterButton.h"
#include "UI/Style/LooterReticleArt.h"
#include "UI/Style/LooterUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

// The settings menu's sizes (the halo's place, the title's indent, the ledger's margin, Resume's row), so the two read as one.
using namespace SettingsMenu;

namespace
{
	/** A row of the ledger, a little taller than a settings row: each is a whole screen to go to. */
	constexpr float HubRowHeight = 60.f;
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
	/** The stamp (c1), as on the settings menu before it moved here. */
	constexpr float StampWidth = 300.f;
	constexpr float StampHeight = 84.f;

	/** A picture of the kit's painter as an image at its own size (twice the resolution, so it stays crisp). */
	UImage* MakePicture(UWidgetTree* Tree, FName Name, const LooterUI::FPaintedIcon& Picture)
	{
		return LooterUI::MakeImage(Tree, LooterUI::PaintedIconBrush(Name, Picture, 2.f, Picture.ViewBox));
	}
}

UWidget* UPauseMenuWidget::MakeHeader()
{
	using namespace LooterUI;
	// GAME PAUSED at c3's size and spacing, the stamped plate of c1 beside it, c3's mil-dot ruler under both.
	UHorizontalBox* TitleLine = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	TitleLine->AddChildToHorizontalBox(MakeText(WidgetTree, TEXT("GAME PAUSED"), 38, Color::SlateText(), true, 200))->SetVerticalAlignment(VAlign_Center);
	TitleLine->AddChildToHorizontalBox(MakeSized(WidgetTree, nullptr, 0.f))->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	AreaPlate = USettingsMenuArt::Make(WidgetTree, ESettingsArt::AreaPlate);
	AreaPlate->SetLabel(TEXT(""), 13, 160);
	UHorizontalBoxSlot* PlateSlot = TitleLine->AddChildToHorizontalBox(AreaPlate);
	PlateSlot->SetVerticalAlignment(VAlign_Center);
	PlateSlot->SetPadding(FMargin(16.f, 0.f, 0.f, 0.f));

	UVerticalBox* Titles = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Titles->AddChildToVerticalBox(TitleLine);
	const float RulerLength = PauseMenu::PlateWidth - PlatePadX * 2.f - TitleIndent - LooterReticle::Pad * 2.f;
	UVerticalBoxSlot* RulerSlot = Titles->AddChildToVerticalBox(MakePicture(WidgetTree, TEXT("PauseHubTitleRuler"),
		LooterReticle::Ruler(RulerLength, 42.f, 3, true)));
	// Its line starts under the title's first letter (the picture keeps a little room round its lines).
	RulerSlot->SetHorizontalAlignment(HAlign_Left);
	RulerSlot->SetPadding(FMargin(-LooterReticle::Pad, 4.f, 0.f, 0.f));

	UHorizontalBox* Header = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Header->AddChildToHorizontalBox(MakeSized(WidgetTree, nullptr, TitleIndent));
	Header->AddChildToHorizontalBox(Titles)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	return Header;
}

UWidget* UPauseMenuWidget::MakeResumeRow()
{
	using namespace LooterUI;
	// c3's choice held in a reticle, as the settings menu's Resume was: the brackets round the row, the small reticle at its
	// left (it turns while the row is marked), the words gold in the middle over a soft gold wash.
	UOverlay* Look = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	USettingsMenuArt* Wash = USettingsMenuArt::Make(WidgetTree, ESettingsArt::ResumeWash);
	MarkBackground(Wash);
	FillOverlaySlot(Look->AddChildToOverlay(Wash));
	const float Edge = LooterReticle::BracketEdgeX(ResumeHeight);
	UOverlaySlot* LeftSlot = Look->AddChildToOverlay(MakePicture(WidgetTree, TEXT("PauseHubBracketL"), LooterReticle::Bracket(ResumeHeight, false)));
	LeftSlot->SetHorizontalAlignment(HAlign_Left);
	LeftSlot->SetVerticalAlignment(VAlign_Center);
	LeftSlot->SetPadding(FMargin(-Edge, 0.f, 0.f, 0.f));
	UOverlaySlot* RightSlot = Look->AddChildToOverlay(MakePicture(WidgetTree, TEXT("PauseHubBracketR"), LooterReticle::Bracket(ResumeHeight, true)));
	RightSlot->SetHorizontalAlignment(HAlign_Right);
	RightSlot->SetVerticalAlignment(VAlign_Center);
	RightSlot->SetPadding(FMargin(0.f, 0.f, -Edge, 0.f));
	ResumeReticle = MakePicture(WidgetTree, TEXT("PauseHubReticle"), LooterReticle::SmallReticle(ResumeReticleRadius));
	UOverlaySlot* ReticleSlot = Look->AddChildToOverlay(ResumeReticle);
	ReticleSlot->SetHorizontalAlignment(HAlign_Left);
	ReticleSlot->SetVerticalAlignment(VAlign_Center);
	ReticleSlot->SetPadding(FMargin(ResumeReticleX - LooterReticle::SmallReticleExtent(ResumeReticleRadius), 0.f, 0.f, 0.f));
	UTextBlock* Word = MakeText(WidgetTree, ItemWord(EPauseMenuItem::Resume, false), 32, Color::SoulGold(), true, 340);
	Word->SetVisibility(ESlateVisibility::HitTestInvisible);
	UOverlaySlot* WordSlot = Look->AddChildToOverlay(Word);
	WordSlot->SetHorizontalAlignment(HAlign_Center);
	WordSlot->SetVerticalAlignment(VAlign_Center);
	WordSlot->SetPadding(FMargin(40.f, 0.f, 0.f, 0.f));

	ULooterButton* Button = MakeItemButton(Look, EPauseMenuItem::Resume);
	Wash->SetButton(Button);
	FPauseMenuLine& Line = Lines.AddDefaulted_GetRef();
	Line.Item = EPauseMenuItem::Resume;
	Line.Button = Button;
	Line.Word = Word;

	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Row->AddChildToHorizontalBox(MakeSized(WidgetTree, nullptr, ResumeIndent));
	UHorizontalBoxSlot* ButtonSlot = Row->AddChildToHorizontalBox(MakeSized(WidgetTree, Button, 0.f, ResumeHeight));
	ButtonSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	ButtonSlot->SetPadding(FMargin(0.f, 0.f, 30.f, 0.f));
	return MakeSized(WidgetTree, Row, 0.f, ResumeHeight);
}

UWidget* UPauseMenuWidget::MakeLedgerRow(EPauseMenuItem Item, int32 Number, const FString& Detail)
{
	using namespace LooterUI;
	UOverlay* Row = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	// Under everything, its look while marked, hidden until then: a gold wash out from the margin (a background) and the gold
	// double rule of a ledger's total along its foot (c1).
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
	// The ruled hairline along its foot; the row's button takes the pointer, so the line needn't.
	USettingsMenuArt* Hairline = USettingsMenuArt::Make(WidgetTree, ESettingsArt::LedgerLine);
	Hairline->SetVisibility(ESlateVisibility::HitTestInvisible);
	FillOverlaySlot(Row->AddChildToOverlay(Hairline));

	// Its number stamped in the margin, its word past the margin's rule, its few words at the right.
	UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UTextBlock* NumberText = MakeText(WidgetTree, FString::Printf(TEXT("%02d"), Number), 17, Color::SlateIcon(), false, 118);
	NumberText->SetJustification(ETextJustify::Center);
	Line->AddChildToHorizontalBox(MakeSized(WidgetTree, NumberText, LedgerMargin))->SetVerticalAlignment(VAlign_Center);
	UTextBlock* Word = MakeText(WidgetTree, ItemWord(Item, false), 24, Color::SlateText(), true, 230);
	UHorizontalBoxSlot* WordSlot = Line->AddChildToHorizontalBox(Word);
	WordSlot->SetVerticalAlignment(VAlign_Center);
	WordSlot->SetPadding(FMargin(LedgerGap, 0.f, 0.f, 0.f));
	Line->AddChildToHorizontalBox(MakeSized(WidgetTree, nullptr, 0.f))->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	UTextBlock* DetailText = MakeText(WidgetTree, Detail, 15, Color::SlateLabel(), false, 40);
	UHorizontalBoxSlot* DetailSlot = Line->AddChildToHorizontalBox(DetailText);
	DetailSlot->SetVerticalAlignment(VAlign_Center);
	DetailSlot->SetPadding(FMargin(16.f, 0.f, 18.f, 0.f));
	USizeBox* Sized = MakeSized(WidgetTree, Line, 0.f);
	Sized->SetMinDesiredHeight(HubRowHeight);
	FillOverlaySlot(Row->AddChildToOverlay(Sized));

	ULooterButton* Button = MakeItemButton(Row, Item);
	FPauseMenuLine& Entry = Lines.AddDefaulted_GetRef();
	Entry.Item = Item;
	Entry.Button = Button;
	Entry.Light = Light;
	Entry.Number = NumberText;
	Entry.Word = Word;
	Entry.Detail = DetailText;
	return Button;
}

UWidget* UPauseMenuWidget::MakeFooter()
{
	using namespace LooterUI;
	UHorizontalBox* Footer = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	// The plate's serial, small and spaced, bottom-left past the corner's range arc (RefreshWords writes it).
	SerialText = MakeText(WidgetTree, TEXT(""), 14, Color::SlateIcon().CopyWithNewOpacity(0.75f), true, 357);
	UHorizontalBoxSlot* SerialSlot = Footer->AddChildToHorizontalBox(SerialText);
	SerialSlot->SetVerticalAlignment(VAlign_Bottom);
	SerialSlot->SetPadding(FMargin(SerialIndent, 0.f, 0.f, 8.f));
	Footer->AddChildToHorizontalBox(MakeSized(WidgetTree, nullptr, 0.f))->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	// Save & Quit, c1's rubber stamp, the list's last row: its light red wash (a background), its red double outline and
	// word, tilted.
	UOverlay* StampLook = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	USettingsMenuArt* StampWash = USettingsMenuArt::Make(WidgetTree, ESettingsArt::StampWash);
	MarkBackground(StampWash);
	FillOverlaySlot(StampLook->AddChildToOverlay(StampWash));
	Stamp = USettingsMenuArt::Make(WidgetTree, ESettingsArt::Stamp);
	Stamp->SetLabel(ItemWord(EPauseMenuItem::SaveQuit, false), 26, 230);
	FillOverlaySlot(StampLook->AddChildToOverlay(Stamp));
	ULooterButton* Button = MakeItemButton(StampLook, EPauseMenuItem::SaveQuit);
	StampWash->SetButton(Button);
	Stamp->SetButton(Button);
	Button->SetRenderTransformAngle(-3.f);
	FPauseMenuLine& Line = Lines.AddDefaulted_GetRef();
	Line.Item = EPauseMenuItem::SaveQuit;
	Line.Button = Button;
	UHorizontalBoxSlot* StampSlot = Footer->AddChildToHorizontalBox(MakeSized(WidgetTree, Button, StampWidth, StampHeight));
	StampSlot->SetVerticalAlignment(VAlign_Center);
	StampSlot->SetPadding(FMargin(0.f, 0.f, 10.f, 0.f));
	return Footer;
}

void UPauseMenuWidget::MakePlateArt()
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
	UImage* ArcImage = MakePicture(WidgetTree, TEXT("PauseHubCornerArc"), Arc);
	ArcImage->SetColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, 0.55f));
	Place(ArcImage, FVector2D(PlatePadX, PauseMenu::PlateHeight - PlatePadBottom) - LooterReticle::ArcScaleCentre(CornerArcRadius, -90.f, 0.f, 6.f, 5),
		FVector2D::ZeroVector);

	// The halo, breaking out over the top-left corner: its dark disc (a background), the ring of ticks with its cross hairs,
	// the gold arcs (both spin up as the menu opens), the pause bars; its range marks round its upper left.
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
	HaloRing = MakePicture(WidgetTree, TEXT("PauseHubHalo"), LooterReticle::Halo(HaloRadius, 6.f, 5, true, 0.f, 0.f));
	Centre(HaloRing);
	HaloGold = MakePicture(WidgetTree, TEXT("PauseHubHaloGold"), SettingsMenuArt::HaloArcs(HaloRadius - 14.f));
	Centre(HaloGold);
	UHorizontalBox* Bars = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Bars->AddChildToHorizontalBox(MakeSized(WidgetTree, MakeImage(WidgetTree, RectBrush(Color::SlateText())), PauseBarWidth, PauseBarHeight));
	Bars->AddChildToHorizontalBox(MakeSized(WidgetTree, MakeImage(WidgetTree, RectBrush(Color::SlateText())), PauseBarWidth, PauseBarHeight))
		->SetPadding(FMargin(PauseBarGap, 0.f, 0.f, 0.f));
	Centre(Bars);
	Place(MakeSized(WidgetTree, Halo, Extent * 2.f, Extent * 2.f), FVector2D(HaloCentreX, HaloCentreY), FVector2D(0.5f, 0.5f));
	const FVector2D HaloCentre(HaloCentreX, HaloCentreY);
	const TCHAR* const MarkWords[] = { TEXT("100"), TEXT("200"), TEXT("300"), TEXT("400") };
	for (int32 Index = 0; Index < static_cast<int32>(UE_ARRAY_COUNT(MarkWords)); ++Index)
	{
		const float Angle = FMath::DegreesToRadians(200.f + 30.f * Index);
		UTextBlock* Range = MakeFloatingText(WidgetTree, 13, Color::SlateLabel(), 77, ETextJustify::Center);
		Range->SetText(FText::FromString(MarkWords[Index]));
		Place(Range, HaloCentre + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * (Extent + 14.f), FVector2D(0.5f, 0.5f));
	}

	// The ribbon, its tails out past the plate's left edge at the row marked (AdvanceLook slides it there).
	Ribbon = USettingsMenuArt::Make(WidgetTree, ESettingsArt::Ribbon);
	Ribbon->SetRenderOpacity(0.f);
	Place(Ribbon, FVector2D(SettingsMenuArt::RibbonLeft, 0.f), FVector2D::ZeroVector);
}

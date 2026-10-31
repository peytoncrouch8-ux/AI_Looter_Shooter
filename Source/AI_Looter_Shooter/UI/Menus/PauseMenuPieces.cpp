// UPauseMenuWidget's pieces on the left of the hub (E2b, mockups/e.js "The hub"): GAME PAUSED over its ruler and the
// area's name; the big dial cut by the screen's edge with its range arc, the swinging needle and the small reticle; each
// option's row (the salt dash across the arc, its number, the pegs out to its tag, the tag with its gem and icon); the
// callouts; and the key hints' place. Positions are PauseMenuLayout's.

#include "UI/Menus/PauseMenuWidget.h"
#include "UI/Menus/PauseMenuArt.h"
#include "UI/Menus/PauseMenuIcons.h"
#include "UI/Menus/PauseMenuLayout.h"
#include "UI/Style/LooterButton.h"
#include "UI/Style/LooterReticleArt.h"
#include "UI/Style/LooterUIStyle.h"
#include "Audio/LooterSound.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

using namespace PauseMenuLayout;
using namespace LooterUI;

namespace
{
	/** Every option's button carries this action, its Index the option (EPauseMenuItem); the saving ones sound as a stamp. */
	const FName ActionPick(TEXT("PausePick"));
	const FName ActionSaveQuit(TEXT("SaveQuit"));

	/** A row's group: this far above and below its middle (the tag and its gem fit inside). */
	constexpr float RowHalf = 50.f;
	/** The pegs: 2 px lines (the long one 2.5 lit), each over the kit's dark edge 3 px wider. */
	constexpr float PegWidth = 2.f;
	constexpr float PegEdge = 3.f;
	/** The number's box, a single line centred in it. */
	constexpr float NumberBand = 32.f;

	const TCHAR* IconOf(EPauseMenuItem Item)
	{
		switch (Item)
		{
		case EPauseMenuItem::Resume:   return TEXT("Resume");
		case EPauseMenuItem::Settings: return TEXT("Settings");
		case EPauseMenuItem::SaveQuit: return TEXT("SaveQuit");
		default:                       return TEXT("Quit");
		}
	}

	/** A line's dark edge: the kit's outline colour, 3 px wider and longer than the line, at 0.7 of its strength. */
	UImage* MakeEdge(UWidgetTree* Tree)
	{
		UImage* Edge = MakeImage(Tree, RectBrush(Color::Outline()));
		Edge->SetRenderOpacity(0.6f);
		return Edge;
	}
}

// ---------------------------------------------------------------------------
// The title
// ---------------------------------------------------------------------------

void UPauseMenuWidget::MakeTitle()
{
	UCanvasPanel* Group = MakeGroup(FVector2D::ZeroVector, FVector2D(TitleX + 800.f, AreaY + AreaBand + 8.f));
	TitleGroup = Group;

	// GAME PAUSED floating over the dimmed game (outlined, as the HUD's type), the main menu's ruler under it (it draws out
	// from the left as the hub opens), the area's name small and widely spaced.
	TitleText = MakeFloatingText(WidgetTree, TitleFont, Color::SlateText(), TitleSpacing);
	TitleText->SetText(FText::FromString(TEXT("GAME PAUSED")));
	Put(Group, MakeBand(TitleText, 0.f, TitleBand), FVector2D(TitleX, TitleY), FVector2D::ZeroVector);

	const FPaintedIcon Ruler = LooterReticle::Ruler(RulerLength, RulerStep, RulerLongEvery, false);
	UImage* RulerImage = MakePicture(TEXT("PauseTitleRuler"), Ruler);
	RulerImage->SetRenderTransformPivot(FVector2D(LooterReticle::Pad / Ruler.ViewBox.X, 0.5f));
	TitleRuler = RulerImage;
	Put(Group, RulerImage, FVector2D(TitleX - LooterReticle::Pad, RulerY - LooterReticle::RulerLineY(false)), FVector2D::ZeroVector);

	AreaText = MakeFloatingText(WidgetTree, AreaFont, Color::SlateLabel(), AreaSpacing);
	Put(Group, MakeBand(AreaText, 0.f, AreaBand), FVector2D(TitleX, AreaY), FVector2D::ZeroVector);
}

// ---------------------------------------------------------------------------
// The dial
// ---------------------------------------------------------------------------

void UPauseMenuWidget::MakeDial()
{
	// One group, so the whole dial swings up round its centre as the hub opens (PauseMenuMotion.cpp).
	const FVector2D GroupSize(DialX + ArcRadius + 80.f, DesignHeight);
	UCanvasPanel* Group = MakeGroup(FVector2D::ZeroVector, GroupSize);
	Group->SetVisibility(ESlateVisibility::HitTestInvisible);
	Group->SetRenderTransformPivot(FVector2D(DialX / GroupSize.X, DialY / GroupSize.Y));
	DialGroup = Group;
	const FVector2D Centre(DialX, DialY);

	// The face: the main menu's ring and ticks, the faint scale, the fine inner ring. It's a background picture only in
	// part, so it stays solid (it's lines, not glass).
	const float Extent = PauseMenuArt::DialExtent(DialRadius);
	Put(Group, MakePicture(TEXT("PauseDialFace"), PauseMenuArt::DialFace(DialRadius)), Centre - FVector2D(Extent, Extent), FVector2D::ZeroVector);

	// The range arc the options hang on, its ticks pointing in, one every 10.7 px, every fifth long.
	const float Step = FMath::RadiansToDegrees(ArcTickPx / ArcRadius);
	const FPaintedIcon Arc = LooterReticle::ArcScale(ArcRadius, ArcFrom, ArcTo, Step, ArcLongEvery, true);
	const FVector2D ArcCentre = LooterReticle::ArcScaleCentre(ArcRadius, ArcFrom, ArcTo, Step, ArcLongEvery, true);
	Put(Group, MakePicture(TEXT("PauseDialArc"), Arc), Centre - ArcCentre, FVector2D::ZeroVector);

	// What swings: the needle, its gold arc, the gold lead out to the arc and the gold run on it, drawn aimed at three
	// o'clock and turned round the dial's centre to the lit option's tick.
	const FPaintedIcon NeedlePicture = PauseMenuArt::DialNeedle(DialRadius, ArcRadius);
	const FVector2D NeedleCentre = PauseMenuArt::DialNeedleCentre(DialRadius, ArcRadius);
	Needle = MakePicture(TEXT("PauseDialNeedle"), NeedlePicture);
	Needle->SetRenderTransformPivot(NeedleCentre / NeedlePicture.ViewBox);
	Put(Group, Needle, Centre - NeedleCentre, FVector2D::ZeroVector);

	// The small reticle on the lit tick (ApplyNeedle keeps it at the needle's aim on the arc).
	Reticle = MakePicture(TEXT("PauseDialReticle"), LooterReticle::SmallReticle(ReticleRadius));
	Reticle->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	Put(Group, Reticle, FVector2D::ZeroVector, FVector2D::ZeroVector);
}

// ---------------------------------------------------------------------------
// The options
// ---------------------------------------------------------------------------

void UPauseMenuWidget::MakeOption(EPauseMenuItem Item, int32 Row)
{
	FPauseMenuOption& Option = Options.AddDefaulted_GetRef();
	Option.Item = Item;
	Option.Row = Row;
	Option.bDanger = Item == EPauseMenuItem::QuitGame;
	const PauseMenuArt::ELook BaseLook = Option.bDanger ? PauseMenuArt::ELook::Danger : PauseMenuArt::ELook::Plain;
	const FLinearColor BaseLine = Option.bDanger ? Color::Worse() : Color::SaltLine();

	// The row's group: its middle at RowHalf, x as on the stage.
	UCanvasPanel* Group = MakeGroup(FVector2D(0.f, RowY[Row] - RowHalf), FVector2D(TagX + TagWidth + TagLitGrow + TagLitSlide + 40.f, RowHalf * 2.f));
	Option.RowGroup = Group;
	const float Tick = RowArcPoint(Row).X;

	// The salt dash across the arc (gone while lit: the reticle sits there then).
	UCanvasPanel* Dash = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
	Dash->SetVisibility(ESlateVisibility::HitTestInvisible);
	Put(Dash, MakeEdge(WidgetTree), FVector2D::ZeroVector, FVector2D(DashHalf * 2.f + PegEdge, PegWidth + PegEdge));
	UImage* DashLine = MakeImage(WidgetTree, RectBrush(Color::SaltLine()));
	DashLine->SetRenderOpacity(0.9f);
	Put(Dash, DashLine, FVector2D(PegEdge * 0.5f, PegEdge * 0.5f), FVector2D(DashHalf * 2.f, PegWidth));
	Put(Group, Dash, FVector2D(Tick - DashHalf - PegEdge * 0.5f, RowHalf - (PegWidth + PegEdge) * 0.5f), FVector2D(DashHalf * 2.f + PegEdge, PegWidth + PegEdge));
	Option.Dash = Dash;

	// The short peg past the arc, toward the number (ApplyLit moves its start out of the reticle's way while lit).
	UImage* ShortPeg = MakeImage(WidgetTree, RectBrush(FLinearColor::White));
	ShortPeg->SetColorAndOpacity(BaseLine.CopyWithNewOpacity(0.8f));
	Put(Group, ShortPeg, FVector2D(Tick + DashHalf, RowHalf - PegWidth * 0.5f), FVector2D(ShortPegTo - DashHalf, PegWidth));
	Option.ShortPeg = ShortPeg;

	// The long peg from past the number to the tag's gem, over its dark edge (ApplyLit stretches both as the tag slides).
	UImage* LongEdge = MakeEdge(WidgetTree);
	Put(Group, LongEdge, FVector2D(Tick + LongPegFrom - PegEdge * 0.5f, RowHalf - (PegWidth + PegEdge) * 0.5f), FVector2D(10.f, PegWidth + PegEdge));
	UImage* LongPeg = MakeImage(WidgetTree, RectBrush(FLinearColor::White));
	LongPeg->SetColorAndOpacity(BaseLine.CopyWithNewOpacity(0.7f));
	Put(Group, LongPeg, FVector2D(Tick + LongPegFrom, RowHalf - PegWidth * 0.5f), FVector2D(10.f, PegWidth));
	Option.LongPeg = LongPeg;
	Option.LongPegEdge = LongEdge;

	// Its number past the arc, as on the main menu's range arc.
	UTextBlock* Number = MakeFloatingText(WidgetTree, NumberFont, Option.bDanger ? Color::Worse() : Color::SlateLabel(), NumberSpacing);
	Number->SetText(FText::FromString(FString::Printf(TEXT("%02d"), Row + 1)));
	Put(Group, MakeBand(Number, 0.f, NumberBand), FVector2D(Tick + NumberGap, RowHalf - NumberBand * 0.5f), FVector2D::ZeroVector);
	Option.Number = Number;

	// The tag: C's tag on its side, its plain (or red) look and its lit gold look crossfaded, the gem on its notch with the
	// option's icon, its words. The kit's bare button takes the clicks and the hover over the tag.
	UCanvasPanel* TagCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
	const TCHAR* LookName = Option.bDanger ? TEXT("PauseTagDanger") : TEXT("PauseTagPlain");
	UWidget* PlainLook = MakeShape(LookName, PauseMenuArt::Tag(TagWidth, TagHeight, BaseLook, PauseMenuArt::EPart::Fill),
		PauseMenuArt::Tag(TagWidth, TagHeight, BaseLook, PauseMenuArt::EPart::Lines), FVector2D(TagWidth, TagHeight));
	Put(TagCanvas, PlainLook, FVector2D::ZeroVector, FVector2D(TagWidth, TagHeight));
	const float LitWidth = TagWidth + TagLitGrow;
	UWidget* LitLook = MakeShape(TEXT("PauseTagLit"), PauseMenuArt::Tag(LitWidth, TagHeight, PauseMenuArt::ELook::Lit, PauseMenuArt::EPart::Fill),
		PauseMenuArt::Tag(LitWidth, TagHeight, PauseMenuArt::ELook::Lit, PauseMenuArt::EPart::Lines), FVector2D(LitWidth, TagHeight));
	LitLook->SetRenderOpacity(0.f);
	Put(TagCanvas, LitLook, FVector2D::ZeroVector, FVector2D(LitWidth, TagHeight));
	Option.PlainLook = PlainLook;
	Option.LitLook = LitLook;

	const FVector2D GemAt(TagGemX, TagHeight * 0.5f - TagGem * 0.5f);
	UImage* PlainGem = MakePicture(Option.bDanger ? TEXT("PauseGemDanger") : TEXT("PauseGemPlain"), PauseMenuArt::GemLook(TagGem, BaseLook));
	Put(TagCanvas, PlainGem, GemAt, FVector2D::ZeroVector);
	UImage* LitGem = MakePicture(TEXT("PauseGemLit"), PauseMenuArt::GemLook(TagGem, PauseMenuArt::ELook::Lit));
	LitGem->SetRenderOpacity(0.f);
	Put(TagCanvas, LitGem, GemAt, FVector2D::ZeroVector);
	Option.PlainGem = PlainGem;
	Option.LitGem = LitGem;

	const TCHAR* IconId = IconOf(Item);
	UImage* Icon = MakeImage(WidgetTree, PaintedIconBrush(FName(*FString::Printf(TEXT("PauseIcon%s"), IconId)), PauseMenuIcons::Get(IconId),
		TagIcon * PictureResolution / 24.f, FVector2D(TagIcon, TagIcon)));
	Icon->SetColorAndOpacity(BaseLine);
	Put(TagCanvas, Icon, GemAt + FVector2D(TagGem * 0.5f - TagIcon * 0.5f, TagGem * 0.5f - TagIcon * 0.5f), FVector2D::ZeroVector);
	Option.Icon = Icon;

	UTextBlock* Word = MakeText(WidgetTree, ItemWord(Item), TagFont, Option.bDanger ? Color::Worse() : Color::SlateText(), false, TagSpacing);
	Word->SetVisibility(ESlateVisibility::HitTestInvisible);
	Put(TagCanvas, MakeBand(Word, LitWidth - TagTextX, TagHeight), FVector2D(TagTextX, 0.f), FVector2D::ZeroVector);
	Option.Word = Word;

	// The navigator's focus light: nothing to see (the lit tag is the focus), only a state to keep.
	USizeBox* Light = MakeSized(WidgetTree, nullptr, 1.f, 1.f);
	Light->SetVisibility(ESlateVisibility::Collapsed);
	Put(TagCanvas, Light, FVector2D::ZeroVector, FVector2D::ZeroVector);
	Option.FocusLight = Light;

	ULooterButton* Button = WidgetTree->ConstructWidget<ULooterButton>(ULooterButton::StaticClass());
	const bool bSaves = SavesFirst(Item);
	Button->SetupContent(TagCanvas, bSaves ? ActionSaveQuit : ActionPick, static_cast<int32>(Item), EButtonKind::Bare);
	// Back to Game is the menu's close (the HUD plays the same cue in the same moment, heard as one); the saving ones sound
	// as Save & Quit's stamp did; Settings clicks, and the page opens with its own sound.
	Button->ClickCue = Item == EPauseMenuItem::Resume ? FName(LooterSoundCue::Close)
		: ULooterButton::ClickCueFor(bSaves ? ActionSaveQuit : ActionPick, bSaves ? EButtonKind::Danger : EButtonKind::Normal);
	Button->OnButtonClicked.BindUObject(this, &UPauseMenuWidget::HandleButton);
	Button->OnButtonHovered.BindUObject(this, &UPauseMenuWidget::HandleButtonHovered);
	ItemButtons.Add(Button);
	Put(Group, Button, FVector2D(TagX, RowHalf - TagHeight * 0.5f), FVector2D(LitWidth, TagHeight));
	Option.Button = Button;
}

void UPauseMenuWidget::MakeCallout(FPauseMenuOption& Option)
{
	// C's callout: a small card pointing back at its lit tag, the option's name, a rule with a diamond, what it does, and a
	// gold flavor line leaning like italics. Tall enough for the longer of its flavor lines (with a session or without).
	const FString Help = ItemHelp(Option.Item);
	const float Height = FMath::Max(CalloutHeight(Help, ItemFlavor(Option.Item, true)), CalloutHeight(Help, ItemFlavor(Option.Item, false)));
	Option.CalloutHeight = Height;
	UCanvasPanel* Group = MakeGroup(FVector2D(CalloutX, RowY[Option.Row] - CalloutPointY), FVector2D(CalloutWidth, Height));
	Group->SetVisibility(ESlateVisibility::Collapsed);
	Group->SetRenderOpacity(0.f);
	Option.Callout = Group;

	Put(Group, MakeShape(TEXT("PauseCallout"), PauseMenuArt::Card(CalloutWidth, Height, false, true, CalloutPointY, PauseMenuArt::EPart::Fill),
		PauseMenuArt::Card(CalloutWidth, Height, false, true, CalloutPointY, PauseMenuArt::EPart::Lines), FVector2D(CalloutWidth, Height)),
		FVector2D::ZeroVector, FVector2D(CalloutWidth, Height));

	UVerticalBox* Words = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Words->AddChildToVerticalBox(MakeText(WidgetTree, ItemWord(Option.Item), CalloutTitleFont, Color::SoulGold(), false, CalloutTitleSpacing));
	UVerticalBoxSlot* RuleSlot = Words->AddChildToVerticalBox(MakePicture(TEXT("PauseOrnament"), PauseMenuArt::Ornament(CalloutTextWidth)));
	RuleSlot->SetPadding(FMargin(0.f, (OrnamentBlock - 14.f) * 0.5f));
	RuleSlot->SetHorizontalAlignment(HAlign_Left);
	UTextBlock* HelpText = MakeText(WidgetTree, Help, HelpFont, Color::SlateText());
	HelpText->SetFont(Font(HelpFont, false));
	HelpText->SetWrapTextAt(CalloutTextWidth);
	Words->AddChildToVerticalBox(HelpText);
	UTextBlock* Flavor = MakeText(WidgetTree, ItemFlavor(Option.Item, true), FlavorFont, Color::SlateFlavor());
	Flavor->SetFont(Font(FlavorFont, false));
	Flavor->SetWrapTextAt(CalloutTextWidth);
	Flavor->SetRenderTransformPivot(FVector2D(0.f, 0.5f));
	Flavor->SetRenderShear(FVector2D(ItalicShear, 0.f));
	Words->AddChildToVerticalBox(Flavor)->SetPadding(FMargin(0.f, FlavorGap, 0.f, 0.f));
	Option.CalloutFlavor = Flavor;
	Put(Group, Words, FVector2D(CalloutPadLeft, CalloutPadTop), FVector2D(CalloutTextWidth, Height - CalloutPadTop - CalloutPadBottom));
}

// ---------------------------------------------------------------------------
// The key hints
// ---------------------------------------------------------------------------

void UPauseMenuWidget::MakeKeys()
{
	// Bottom-right, kept to the right edge; RefreshPrompts fills the bar for the device in use, right-aligned.
	UCanvasPanel* Group = MakeGroup(FVector2D(RightEdge - KeysWidth, KeysY - KeysHeight * 0.5f), FVector2D(KeysWidth, KeysHeight), true);
	Group->SetVisibility(ESlateVisibility::HitTestInvisible);
	KeysGroup = Group;
	PromptBar = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UCanvasPanelSlot* BarSlot = Put(Group, PromptBar, FVector2D(KeysWidth, KeysHeight * 0.5f), FVector2D::ZeroVector);
	BarSlot->SetAlignment(FVector2D(1.f, 0.5f));
}

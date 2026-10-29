// UMainMenuWidget's title lockup and main buttons, concept 3 "Halo Reticle" (the user's pick, 2026-10-29: c3.png, its
// main menu; make_a.js c3Menu): the logo mark held in a ticked halo, REVENANT, a salt ruler and TEROPA; the four words
// right-aligned to a range arc with their numbers on it, the lit one gold with a small reticle on the arc beside it. The
// distances are MainMenuArt::Layout's; MainMenuMotion.cpp moves it all.

#include "UI/Menus/MainMenuWidget.h"
#include "UI/Menus/MainMenuArt.h"
#include "UI/Style/LooterButton.h"
#include "UI/Style/LooterReticleArt.h"
#include "UI/Style/LooterUIStyle.h"
#include "Scenes/TransitionScreenSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScaleBox.h"
#include "Components/ScaleBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/SizeBoxSlot.h"
#include "Components/TextBlock.h"

using namespace LooterUI;

namespace
{
	namespace Layout = MainMenuArt::Layout;

	const FName ActionSinglePlayer(TEXT("SinglePlayer"));
	const FName ActionMultiplayer(TEXT("Multiplayer"));
	const FName ActionSettings(TEXT("Settings"));
	const FName ActionQuit(TEXT("Quit"));

	/** A main button's press (Art/Sounds/recipes/opening.py): heavier than the kit's click, as the menu is pressed rarely. */
	const FName CueSelect(TEXT("UI.Menu.Select"));

	/** The title's words and the rows slide in this far as they fade in. */
	constexpr float WordsSlide = 28.f;
	constexpr float RowSlide = 18.f;
	/** The pictures' texture resolution: about twice their drawn size, as the kit asks. */
	constexpr float PictureResolution = 2.f;
	/** The boxes the single lines of type are centred in (taller than the letters, so nothing clips). */
	constexpr float TitleBand = 90.f;
	constexpr float SmallBand = 32.f;

	/** Puts an overlay child at X, Y from its top-left (the mockup's absolute layout, in an overlay). */
	void Place(UOverlaySlot* Slot, float X, float Y)
	{
		Slot->SetHorizontalAlignment(HAlign_Left);
		Slot->SetVerticalAlignment(VAlign_Top);
		Slot->SetPadding(FMargin(X, Y, 0.f, 0.f));
	}

	/** Content centred up and down in a box Height tall (Width 0: the content's own), aligned across as asked. */
	USizeBox* Band(UWidgetTree* Tree, UWidget* Content, float Width, float Height, EHorizontalAlignment Across = HAlign_Left)
	{
		USizeBox* Box = MakeSized(Tree, Content, Width, Height);
		if (USizeBoxSlot* BoxSlot = Cast<USizeBoxSlot>(Box->GetContentSlot()))
		{
			BoxSlot->SetHorizontalAlignment(Across);
			BoxSlot->SetVerticalAlignment(VAlign_Center);
		}
		return Box;
	}

	/** One of the menu's painted pictures at its own size. */
	UImage* MakePicture(UWidgetTree* Tree, FName Name, const FPaintedIcon& Picture)
	{
		return MakeImage(Tree, PaintedIconBrush(Name, Picture, PictureResolution, Picture.ViewBox));
	}

	UTextBlock* MakeWords(UWidgetTree* Tree, const FString& Words, int32 Size, const FLinearColor& Tint, int32 Spacing,
		ETextJustify::Type Justify = ETextJustify::Left)
	{
		UTextBlock* Text = MakeFloatingText(Tree, Size, Tint, Spacing, Justify);
		Text->SetText(FText::FromString(Words.ToUpper()));
		return Text;
	}
}

// ---------------------------------------------------------------------------
// The title lockup
// ---------------------------------------------------------------------------

UWidget* UMainMenuWidget::MakeTitle()
{
	UOverlay* Lockup = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	Lockup->SetVisibility(ESlateVisibility::HitTestInvisible);
	const float Centre = Layout::HaloCentre;

	// The halo first; it spins up and lights (MainMenuMotion.cpp), then the words come.
	UWidget* Halo = MakeHalo();
	const float Extent = LooterReticle::HaloExtent(Layout::HaloRadius);
	Place(Lockup->AddChildToOverlay(Halo), Centre - Extent, Centre - Extent);
	AddRevealPart(Halo, Layout::HaloDelay, 0.f);

	UOverlay* Words = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	FillOverlaySlot(Lockup->AddChildToOverlay(Words));
	AddRevealPart(Words, Layout::WordsDelay, WordsSlide);

	// The game's title (the same words the cloud bank's title card rises with), in the kit's display type. It shrinks
	// rather than run past the menu's side (with a wider fallback font, say).
	TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	TitleText->SetText(UTransitionScreenSubsystem::GameTitle());
	TitleText->SetFont(DisplayFont(Layout::TitleFont, Layout::TitleSpacing));
	TitleText->SetColorAndOpacity(FSlateColor(Color::SlateText()));
	UScaleBox* TitleFit = WidgetTree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass());
	TitleFit->SetStretch(EStretch::ScaleToFitX);
	TitleFit->SetStretchDirection(EStretchDirection::DownOnly);
	if (UScaleBoxSlot* FitSlot = Cast<UScaleBoxSlot>(TitleFit->SetContent(TitleText)))
	{
		FitSlot->SetHorizontalAlignment(HAlign_Left);
	}
	const float WordsX = Centre + Layout::TitleX;
	UOverlaySlot* TitleSlot = Words->AddChildToOverlay(Band(WidgetTree, TitleFit, 0.f, TitleBand));
	Place(TitleSlot, WordsX, Centre + Layout::TitleY - TitleBand * 0.5f);
	// The overlay fills the title's box, so the fit box keeps inside the menu's side.
	TitleSlot->SetHorizontalAlignment(HAlign_Fill);

	// Under it the salt ruler (it draws out from the left as the menu arrives), then the world, Teropa (the user's call,
	// 2026-10-13), small and widely spaced.
	const FPaintedIcon Ruler = LooterReticle::Ruler(Layout::RulerLength, Layout::RulerStep, Layout::RulerLongEvery, false);
	TitleRule = MakePicture(WidgetTree, TEXT("MenuTitleRuler"), Ruler);
	TitleRule->SetRenderTransformPivot(FVector2D(LooterReticle::Pad / Ruler.ViewBox.X, 0.5f));
	Place(Words->AddChildToOverlay(TitleRule), Centre + Layout::RulerX - LooterReticle::Pad,
		Centre + Layout::RulerY - LooterReticle::RulerLineY(false));
	UTextBlock* Planet = MakeWords(WidgetTree, TEXT("Teropa"), Layout::PlanetFont, Color::SaltLine(), Layout::PlanetSpacing);
	Place(Words->AddChildToOverlay(Band(WidgetTree, Planet, 0.f, SmallBand)), Centre + Layout::PlanetX, Centre + Layout::PlanetY - SmallBand * 0.5f);

	return MakeSized(WidgetTree, Lockup, 0.f, Layout::TitleBoxHeight);
}

UWidget* UMainMenuWidget::MakeHalo()
{
	using namespace LooterReticle;
	const float Radius = Layout::HaloRadius;
	const float Side = HaloExtent(Radius) * 2.f;

	// What spins: the dark disc, the ring and its ticks. The logo mark over it stays upright.
	UOverlay* Spin = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	Spin->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	HaloSpin = Spin;

	// The ring's dark inside (a background: the transparency fades it), so the mark reads over the planet's glow.
	UImage* Disc = MakeImage(WidgetTree, CircleBrush(Color::SlateGlassFoot()));
	MarkBackground(Disc);
	UOverlaySlot* DiscSlot = Spin->AddChildToOverlay(MakeSized(WidgetTree, Disc, Radius * 2.f, Radius * 2.f));
	DiscSlot->SetHorizontalAlignment(HAlign_Center);
	DiscSlot->SetVerticalAlignment(VAlign_Center);

	FillOverlaySlot(Spin->AddChildToOverlay(MakePicture(WidgetTree, TEXT("MenuHaloRing"), Halo(Radius, 0.f, 0, false, 0.f, 0.f))));
	// The ticks in sectors round the ring, each its own picture, so they can light one after another.
	HaloSectors.Reset();
	const float Sector = 360.f / Layout::HaloSectors;
	for (int32 Index = 0; Index < Layout::HaloSectors; ++Index)
	{
		const FPaintedIcon Ticks = HaloTicks(Radius, Layout::HaloTickStep, Layout::HaloLongEvery, Index * Sector, (Index + 1) * Sector);
		UImage* SectorImage = MakePicture(WidgetTree, FName(*FString::Printf(TEXT("MenuHaloTicks%d"), Index)), Ticks);
		FillOverlaySlot(Spin->AddChildToOverlay(SectorImage));
		HaloSectors.Add(SectorImage);
	}

	UOverlay* Group = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	FillOverlaySlot(Group->AddChildToOverlay(Spin));
	// The logo mark (MainMenuArt::LogoMark: the user's pick drops in there).
	UImage* Mark = MakeImage(WidgetTree, PaintedIconBrush(TEXT("MenuLogoMark"), MainMenuArt::LogoMark(), 3.f,
		FVector2D(Layout::LogoSize, Layout::LogoSize)));
	UOverlaySlot* MarkSlot = Group->AddChildToOverlay(Mark);
	MarkSlot->SetHorizontalAlignment(HAlign_Center);
	MarkSlot->SetVerticalAlignment(VAlign_Center);
	return MakeSized(WidgetTree, Group, Side, Side);
}

// ---------------------------------------------------------------------------
// The main buttons
// ---------------------------------------------------------------------------

UWidget* UMainMenuWidget::MakeMainButtons()
{
	using namespace LooterReticle;
	UOverlay* List = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	List->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	MainNavButtons.Reset();
	MainRows.Reset();

	// The range arc the list hangs on, its ticks pointing in toward the planet, one every 8 mockup px.
	const float From = Layout::ArcDegrees(Layout::ArcBottom);
	const float To = Layout::ArcDegrees(Layout::ArcTop);
	const float Step = FMath::RadiansToDegrees(Layout::ArcTickPx / Layout::ArcRadius);
	UImage* Arc = MakePicture(WidgetTree, TEXT("MenuRangeArc"), ArcScale(Layout::ArcRadius, From, To, Step, 0, true));
	const FVector2D ArcCentre = ArcScaleCentre(Layout::ArcRadius, From, To, Step, 0, true);
	Place(List->AddChildToOverlay(Arc), Layout::ArcCentreX - ArcCentre.X, Layout::ArcCentreY - ArcCentre.Y);
	AddRevealPart(Arc, Layout::FirstRowDelay, 0.f);

	auto Add = [this, List](UWidget* Row, int32 Index)
	{
		Place(List->AddChildToOverlay(Row), 0.f, Layout::RowTop + Layout::RowHeight * Index);
		AddRevealPart(Row, Layout::FirstRowDelay + Layout::RowStagger * Index, RowSlide);
	};
	Add(MakeMainButton(ActionSinglePlayer, 0, TEXT("Single Player")), 0);
	// Multiplayer doesn't exist yet. Disabled, the button can't be clicked or lit (the focus passes over it); its words
	// show dim, and the tag on the arc says why.
	Add(MakeMainButton(ActionMultiplayer, 1, TEXT("Multiplayer"), TEXT("Soon")), 1);
	if (ULooterButton* Multiplayer = GetMainButton(ActionMultiplayer))
	{
		Multiplayer->SetIsEnabled(false);
	}
	Add(MakeMainButton(ActionSettings, 2, TEXT("Settings")), 2);
	Add(MakeMainButton(ActionQuit, 3, TEXT("Quit Game")), 3);

	// Over the rows (never in the mouse's way): the gold run on the arc and the small reticle, moved onto the lit row.
	UOverlay* Marks = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	Marks->SetVisibility(ESlateVisibility::HitTestInvisible);
	FillOverlaySlot(List->AddChildToOverlay(Marks));
	AddRevealPart(Marks, Layout::FirstRowDelay, 0.f);

	// The run is drawn at the circle's middle and turned round the circle's centre onto the lit row, so it lies on the arc.
	const float RunHalf = FMath::RadiansToDegrees(FMath::Asin(Layout::LitRunHalf / Layout::ArcRadius));
	const FPaintedIcon Run = MainMenuArt::LitRun(Layout::ArcRadius, RunHalf);
	const FVector2D RunCentre = MainMenuArt::LitRunCentre(Layout::ArcRadius, RunHalf);
	LitRunMark = MakePicture(WidgetTree, TEXT("MenuLitRun"), Run);
	LitRunMark->SetRenderTransformPivot(RunCentre / Run.ViewBox);
	LitRunMark->SetRenderOpacity(0.f);
	Place(Marks->AddChildToOverlay(LitRunMark), Layout::ArcCentreX - RunCentre.X, Layout::ArcCentreY - RunCentre.Y);

	ReticleMark = MakePicture(WidgetTree, TEXT("MenuReticle"), SmallReticle(Layout::ReticleRadius));
	ReticleMark->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	ReticleMark->SetRenderOpacity(0.f);
	Place(Marks->AddChildToOverlay(ReticleMark), 0.f, 0.f);
	ReticleRow = INDEX_NONE;
	ReticleAlpha = 0.f;

	return MakeSized(WidgetTree, List, Layout::ListWidth, Layout::ListHeight);
}

UWidget* UMainMenuWidget::MakeMainButton(FName Action, int32 Index, const FString& Label, const FString& Tag)
{
	// Row-local: its middle, and where it crosses the arc.
	const float Middle = Layout::RowHeight * 0.5f;
	const float ArcX = Layout::ArcX(Layout::RowY(Index));
	UOverlay* Body = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());

	// The words, right-aligned to end short of the arc (the arc bows, so they follow its curve down the list).
	UTextBlock* Words = MakeWords(WidgetTree, Label, Layout::WordFont, Color::SlateText(), Layout::WordSpacing, ETextJustify::Right);
	Place(Body->AddChildToOverlay(Band(WidgetTree, Words, ArcX - Layout::WordsGap, Layout::RowHeight, HAlign_Right)), 0.f, 0.f);

	// At rest a salt dash across the arc; lit, a gold dash from the words to the reticle instead.
	UWidget* SaltDash = MakeSized(WidgetTree, MakeImage(WidgetTree, RectBrush(Color::SaltLine())), Layout::DashHalf * 2.f, 2.f);
	Place(Body->AddChildToOverlay(SaltDash), ArcX - Layout::DashHalf, Middle - 1.f);
	UWidget* GoldDash = MakeSized(WidgetTree, MakeImage(WidgetTree, RectBrush(Color::SoulGold())), Layout::GoldDashFrom - Layout::GoldDashTo, 3.f);
	GoldDash->SetRenderOpacity(0.f);
	Place(Body->AddChildToOverlay(GoldDash), ArcX - Layout::GoldDashFrom, Middle - 1.5f);

	// Its number on the arc's far side, like a range mark; or a tag there instead (Multiplayer: SOON, in a hollow outline).
	UTextBlock* NumberText = nullptr;
	if (Tag.IsEmpty())
	{
		NumberText = MakeWords(WidgetTree, FString::Printf(TEXT("%02d"), Index + 1), Layout::NumberFont, Color::SlateLabel(), Layout::NumberSpacing);
		Place(Body->AddChildToOverlay(Band(WidgetTree, NumberText, 0.f, SmallBand)), ArcX + Layout::NumberGap, Middle - SmallBand * 0.5f);
	}
	else
	{
		const float TagX = ArcX + Layout::TagGap;
		const float TagY = Middle - Layout::TagHeight * 0.5f;
		Place(Body->AddChildToOverlay(MakePicture(WidgetTree, TEXT("MenuTagOutline"), MainMenuArt::TagOutline(Layout::TagWidth, Layout::TagHeight))),
			TagX - LooterReticle::Pad, TagY - LooterReticle::Pad);
		UTextBlock* TagWords = MakeWords(WidgetTree, Tag, Layout::TagFont, Color::SlateLabel(), Layout::TagSpacing, ETextJustify::Center);
		TagWords->SetRenderOpacity(0.8f);
		Place(Body->AddChildToOverlay(Band(WidgetTree, TagWords, Layout::TagWidth, Layout::TagHeight, HAlign_Center)), TagX, TagY);
	}

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
	Row.SaltDash = SaltDash;
	Row.GoldDash = GoldDash;
	Row.Y = Layout::RowY(Index);
	MainRows.Add(Row);
	return MakeSized(WidgetTree, Button, Layout::ListWidth, Layout::RowHeight);
}

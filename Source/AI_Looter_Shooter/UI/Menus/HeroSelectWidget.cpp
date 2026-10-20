// UHeroSelectWidget: the layout (header, stage, the hero's panel), choosing a hero and confirming the pick. The roster's
// cards are in HeroSelectRoster.cpp; the keys, the mouse and the key hints in HeroSelectNav.cpp; the portraits and power
// rows in HeroSelectParts.

#include "UI/Menus/HeroSelectWidget.h"
#include "Audio/LooterSound.h"
#include "Audio/LooterSoundCues.h"
#include "Embers/EmberDefinition.h"
#include "Embers/HeroDefinition.h"
#include "UI/Inventory/LoadoutParts.h"
#include "UI/Menus/HeroSelectParts.h"
#include "UI/Menus/HeroSelectStage.h"
#include "UI/Style/LooterButton.h"
#include "UI/Style/LooterUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"

using namespace LooterUI;
using namespace HeroSelectParts;

namespace
{
	const FName ActionBack(TEXT("Back"));
	const FName ActionConfirm(TEXT("ConfirmHero"));

	// The page is laid out at 1600 x 900 (LoadoutParts::PageSize) and scaled to the screen.
	const FVector2D HeaderPosition(64.f, 30.f);
	/** The header's line under the title wraps here: left of the stand's middle, where the tallest hero's hat reaches. */
	constexpr float HeaderTextWidth = 360.f;
	/**
	 * The painted portrait standing in for a hero whose body isn't in the game (the live stand is at HeroSelectParts::
	 * StandCentre): its middle and size. Its top clears the header's last line by ~30 (the painted frame's fittings stand
	 * proud of the picture; at 520 from 152 they touched the line, 2026-10-19).
	 */
	const FVector2D StageCentre(560.f, 424.f);
	constexpr float PortraitSize = 500.f;
	constexpr float GlowSize = 1080.f;
	/** The glow's strength behind a greyed hero (a playable one's: HeroSelectParts::StageGlowAlpha). */
	constexpr float GreyedGlowAlpha = 0.14f;
	/** The roster's top, under the stage (its cards: HeroSelectRoster.cpp). */
	constexpr float RosterTop = 694.f;
	/** The hero's panel on the right, and the key hints along the bottom left. */
	const FVector2D PanelPosition(1048.f, 40.f);
	const FVector2D PanelSize(496.f, 820.f);
	const FVector2D FooterPosition(64.f, 854.f);
	constexpr float ButtonHeight = 46.f;
	constexpr float BackWidth = 130.f;
	/** The panel's painted face beside the name (the portraits stay on the cards and the panel; the stand shows the body). */
	constexpr float PanelFaceSize = 76.f;

	/** The panel's glass: the kit's screen glass a little clearer, as the main menu's column is, so the two read as one menu. */
	FLinearColor PanelGlass() { return WithAlpha(Color::ScreenBg(), 0.87f); }
}

// ---------------------------------------------------------------------------
// Opening
// ---------------------------------------------------------------------------

void UHeroSelectWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	// The main menu holds the keyboard and hands this screen its keys; nothing here takes the focus.
	SetIsFocusable(false);
}

void UHeroSelectWidget::Open(int32 InSlot)
{
	SessionSlot = InSlot;
	ConfirmLeft = -1.f;
	bConfirmSent = false;
	StageIn = 1.f;
	if (StagePicture)
	{
		// As it stands at rest: a confirm's lift or a choice's settling cut short last time is put back.
		StagePicture->SetRenderScale(FVector2D::UnitVector);
		StagePicture->SetRenderOpacity(1.f);
		StagePicture->SetRenderTranslation(FVector2D::ZeroVector);
	}
	if (StageGlow)
	{
		StageGlow->SetRenderScale(FVector2D::UnitVector);
	}
	if (StandImage)
	{
		StandImage->SetRenderScale(FVector2D::UnitVector);
		StandImage->SetRenderOpacity(1.f);
	}
	StopNavHold();
	Gather();
	// Ellis first, the story's own (the roster's first).
	Selected = Heroes.IsEmpty() ? INDEX_NONE : 0;
	if (Eyebrow)
	{
		Eyebrow->SetText(FText::FromString(FString::Printf(TEXT("SESSION %d  ·  NEW GAME"), SessionSlot + 1)));
	}
	RebuildRoster();
	// The live stand runs only while the screen is open (Close stops it).
	OpenStand();
	ShowSelected();
	SetStatus(FString(), Color::TextDim());
	RebuildNavigation(Cards.IsValidIndex(Selected) ? Cards[Selected].Button : nullptr);
	RefreshNavLook();
}

void UHeroSelectWidget::Gather()
{
	// Read every time it opens: a hero's model may have landed, or Looter.Hero.Unlock changed, since it last showed.
	Heroes.Reset();
	Portraits.Reset();
	Pickable.Reset();
	for (const FName Id : UHeroDefinition::RosterIds())
	{
		if (const UHeroDefinition* Hero = UHeroDefinition::Load(Id))
		{
			Heroes.Add(Hero);
			Portraits.Add(Hero->LoadPortrait());
			Pickable.Add(Hero->CanBePicked());
		}
	}
}

const UHeroDefinition* UHeroSelectWidget::GetHero(int32 Index) const
{
	return Heroes.IsValidIndex(Index) ? Heroes[Index].Get() : nullptr;
}

FName UHeroSelectWidget::GetSelectedId() const
{
	const UHeroDefinition* Hero = GetHero(Selected);
	return Hero ? Hero->Id : NAME_None;
}

bool UHeroSelectWidget::CanConfirm(int32 Index) const
{
	return Pickable.IsValidIndex(Index) && Pickable[Index];
}

ULooterButton* UHeroSelectWidget::GetCard(int32 Index) const
{
	return Cards.IsValidIndex(Index) ? Cards[Index].Button : nullptr;
}

// ---------------------------------------------------------------------------
// Layout
// ---------------------------------------------------------------------------

TSharedRef<SWidget> UHeroSelectWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
		// The whole screen answers the mouse, so a click beside the cards lands here, not on the island's viewport.
		Root->SetVisibility(ESlateVisibility::Visible);
		WidgetTree->RootWidget = Root;

		// The island stays in view behind, dimmed as behind the inventory's pages.
		UImage* Backdrop = MakeImage(WidgetTree, RectBrush(Color::Backdrop()));
		MarkBackground(Backdrop);
		FillOverlaySlot(Root->AddChildToOverlay(Backdrop));

		UScaleBox* Scale = WidgetTree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass());
		Scale->SetStretch(EStretch::ScaleToFit);
		FillOverlaySlot(Root->AddChildToOverlay(Scale));
		UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Page"));
		Scale->SetContent(MakeSized(WidgetTree, Canvas, LoadoutParts::PageSize.X, LoadoutParts::PageSize.Y));

		auto Place = [Canvas](UWidget* Widget, const FVector2D& Position, const FVector2D& Size, const FVector2D& Alignment = FVector2D::ZeroVector)
		{
			UCanvasPanelSlot* CanvasSlot = Canvas->AddChildToCanvas(Widget);
			CanvasSlot->SetPosition(Position);
			CanvasSlot->SetAlignment(Alignment);
			if (Size.IsZero())
			{
				CanvasSlot->SetAutoSize(true);
			}
			else
			{
				CanvasSlot->SetSize(Size);
			}
		};

		// Back to front: the glow behind the hero, the live stand (or, when the hero's body isn't in the game, their
		// portrait), the header over the stand's corner, the roster, the panel, the hints.
		StageGlow = MakeImage(WidgetTree, GlowBrush(FVector2D(GlowSize, GlowSize), FLinearColor::White));
		Place(StageGlow, StandCentre, FVector2D(GlowSize, GlowSize), FVector2D(0.5f, 0.5f));
		// A soft shadow under the plinth's foot, so the stand sits on something rather than floating over the island.
		const FVector2D FloorShadowSize(440.f, 70.f);
		Place(MakeImage(WidgetTree, GlowBrush(FloorShadowSize, WithAlpha(Color::Ink(), 0.75f))),
			FVector2D(StandCentre.X, StandCentre.Y + StandSize.Y * 0.47f), FloorShadowSize, FVector2D(0.5f, 0.5f));
		Place(MakeStand(), StandCentre, StandSize, FVector2D(0.5f, 0.5f));
		Place(MakeStage(), StageCentre, FVector2D(PortraitSize, PortraitSize), FVector2D(0.5f, 0.5f));
		Place(MakeHeader(), HeaderPosition, FVector2D::ZeroVector);

		RosterRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		Place(RosterRow, FVector2D(StageCentre.X, RosterTop), FVector2D::ZeroVector, FVector2D(0.5f, 0.f));

		Place(MakePanel(), PanelPosition, PanelSize);

		FooterPrompts = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		Place(FooterPrompts, FooterPosition, FVector2D::ZeroVector);

		// Opened before it was first shown: the contents go in now.
		RebuildRoster();
		ShowSelected();
		RebuildNavigation(Cards.IsValidIndex(Selected) ? Cards[Selected].Button : nullptr);
		RefreshNavLook();
	}
	return Super::RebuildWidget();
}

UWidget* UHeroSelectWidget::MakeHeader()
{
	UVerticalBox* Header = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	// The main menu's title marks: a short orange bar, then whose new game it is, then the call to choose.
	Header->AddChildToVerticalBox(MakeSized(WidgetTree, MakeImage(WidgetTree, RectBrush(Color::Accent())), 44.f, 4.f))
		->SetHorizontalAlignment(HAlign_Left);
	Eyebrow = MakeText(WidgetTree, FString::Printf(TEXT("Session %d  ·  New game"), SessionSlot + 1), 11, Color::CyanText(), true, 300);
	Header->AddChildToVerticalBox(Eyebrow)->SetPadding(FMargin(0.f, 12.f, 0.f, 0.f));
	Header->AddChildToVerticalBox(MakeText(WidgetTree, TEXT("Choose your hero"), 30, Color::Title(), true, 120))
		->SetPadding(FMargin(0.f, 2.f, 0.f, 0.f));
	UTextBlock* Sub = MakeText(WidgetTree, TEXT("Whoever you pick is the Revenant Sexton raised. A session keeps its hero."), 12, Color::TextDim());
	Sub->SetFont(Font(12, false));
	// Two short lines, clear of the hero's hat on the stand beside them.
	Sub->SetAutoWrapText(true);
	Header->AddChildToVerticalBox(MakeSized(WidgetTree, Sub, HeaderTextWidth))->SetPadding(FMargin(0.f, 6.f, 0.f, 0.f));
	return Header;
}

UWidget* UHeroSelectWidget::MakeStage()
{
	// The chosen hero's portrait, whole (its painted frame and name plate), greyed and stamped while not playable. The
	// room round it is where a live 3D stand can go later.
	UOverlay* Picture = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	StageImage = MakeImage(WidgetTree, RectBrush(Color::Track()));
	FillOverlaySlot(Picture->AddChildToOverlay(StageImage));
	StageWash = MakeImage(WidgetTree, RectBrush(GreyedWash()));
	FillOverlaySlot(Picture->AddChildToOverlay(StageWash));
	StageStamp = MakeWorksStamp(WidgetTree, 18);
	UOverlaySlot* StampSlot = Picture->AddChildToOverlay(StageStamp);
	StampSlot->SetHorizontalAlignment(HAlign_Center);
	StampSlot->SetVerticalAlignment(VAlign_Center);
	Picture->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	StagePicture = Picture;
	return MakeSized(WidgetTree, Picture, PortraitSize, PortraitSize);
}

UWidget* UHeroSelectWidget::MakePanel()
{
	UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());

	// Their painted face (the stand shows their body), then the role (with the stamp on a hero in the works), the name
	// and a rule in their colour.
	{
		UHorizontalBox* Head = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		PanelFace = MakeSized(WidgetTree, nullptr, PanelFaceSize, PanelFaceSize);
		UHorizontalBoxSlot* FaceSlot = Head->AddChildToHorizontalBox(PanelFace);
		FaceSlot->SetVerticalAlignment(VAlign_Center);
		FaceSlot->SetPadding(FMargin(0.f, 0.f, 16.f, 0.f));
		UVerticalBox* Words = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		UHorizontalBox* Top = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		RoleText = MakeText(WidgetTree, TEXT(""), 12, Color::CyanText(), true, 400);
		UHorizontalBoxSlot* RoleSlot = Top->AddChildToHorizontalBox(RoleText);
		RoleSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		RoleSlot->SetVerticalAlignment(VAlign_Center);
		RoleWorks = MakeWorksStamp(WidgetTree, 9);
		Top->AddChildToHorizontalBox(RoleWorks)->SetVerticalAlignment(VAlign_Center);
		Words->AddChildToVerticalBox(Top);
		NameText = MakeText(WidgetTree, TEXT(""), 30, Color::Accent(), true, 60);
		NameText->SetTextOverflowPolicy(ETextOverflowPolicy::Ellipsis);
		Words->AddChildToVerticalBox(NameText)->SetPadding(FMargin(0.f, 2.f, 0.f, 0.f));
		NameRule = MakeImage(WidgetTree, RectBrush(FLinearColor::White));
		Words->AddChildToVerticalBox(MakeSized(WidgetTree, NameRule, 72.f, 3.f))->SetHorizontalAlignment(HAlign_Left);
		UHorizontalBoxSlot* WordsSlot = Head->AddChildToHorizontalBox(Words);
		WordsSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		WordsSlot->SetVerticalAlignment(VAlign_Center);
		Content->AddChildToVerticalBox(Head);
	}

	StoryText = MakeText(WidgetTree, TEXT(""), 13, Color::Text());
	StoryText->SetFont(Font(13, false));
	StoryText->SetAutoWrapText(true);
	Content->AddChildToVerticalBox(StoryText)->SetPadding(FMargin(0.f, 14.f, 0.f, 0.f));

	// The three powers, all owned from the start.
	Content->AddChildToVerticalBox(MakeSection(WidgetTree, TEXT("Ember powers")))->SetPadding(FMargin(0.f, 22.f, 0.f, 0.f));
	UTextBlock* Note = MakeText(WidgetTree, TEXT("All three are theirs from the start. One is equipped at a time; the others wait on the inventory's Embers page."), 10, Color::TextDim());
	Note->SetFont(Font(10, false));
	Note->SetAutoWrapText(true);
	Content->AddChildToVerticalBox(Note)->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));
	PowerList = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Content->AddChildToVerticalBox(PowerList)->SetPadding(FMargin(0.f, 6.f, 0.f, 0.f));

	Content->AddChildToVerticalBox(MakeSized(WidgetTree, nullptr, 0.f))->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	// What just happened (a hero in the works refused, the session starting), keeping its room when empty.
	StatusText = MakeText(WidgetTree, TEXT(""), 11, Color::TextDim());
	StatusText->SetAutoWrapText(true);
	USizeBox* StatusBox = MakeSized(WidgetTree, StatusText, 0.f);
	StatusBox->SetMinDesiredHeight(32.f);
	Content->AddChildToVerticalBox(StatusBox)->SetPadding(FMargin(0.f, 8.f, 0.f, 6.f));

	// Back, and the call to play.
	{
		UHorizontalBox* Buttons = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		BackButton = WidgetTree->ConstructWidget<ULooterButton>(ULooterButton::StaticClass());
		BackButton->Setup(WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass()), ActionBack, 0, FText::FromString(TEXT("Back")), 14, EButtonKind::Normal);
		BackButton->OnButtonClicked.BindUObject(this, &UHeroSelectWidget::HandleButton);
		BackButton->OnButtonHovered.BindUObject(this, &UHeroSelectWidget::HandleButtonHovered);
		Buttons->AddChildToHorizontalBox(MakeSized(WidgetTree, BackButton, BackWidth, ButtonHeight));
		ConfirmButton = WidgetTree->ConstructWidget<ULooterButton>(ULooterButton::StaticClass());
		ConfirmButton->Setup(WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass()), ActionConfirm, 0, FText::FromString(TEXT("Play")), 15, EButtonKind::Primary);
		// Confirming plays its own sounds (the equip's clack and the ember's chime), never a plain click under them.
		ConfirmButton->bPlaysSounds = false;
		ConfirmButton->OnButtonClicked.BindUObject(this, &UHeroSelectWidget::HandleButton);
		ConfirmButton->OnButtonHovered.BindUObject(this, &UHeroSelectWidget::HandleButtonHovered);
		UHorizontalBoxSlot* ConfirmSlot = Buttons->AddChildToHorizontalBox(MakeSized(WidgetTree, ConfirmButton, 0.f, ButtonHeight));
		ConfirmSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		ConfirmSlot->SetPadding(FMargin(12.f, 0.f, 0.f, 0.f));
		Content->AddChildToVerticalBox(Buttons);
	}

	// Dark glass with a cyan edge, all four corners cut like the kit's big frames, an orange bar over the role.
	UOverlay* Panel = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	FillOverlaySlot(Panel->AddChildToOverlay(MakeShapeBox(WidgetTree, EShape::Panel, PanelGlass(), Color::ScreenLine(), Content,
		FMargin(28.f, 26.f, 28.f, 24.f))));
	UOverlaySlot* BarSlot = Panel->AddChildToOverlay(MakeSized(WidgetTree, MakeImage(WidgetTree, RectBrush(Color::Accent())), 120.f, 3.f));
	BarSlot->SetHorizontalAlignment(HAlign_Left);
	BarSlot->SetVerticalAlignment(VAlign_Top);
	BarSlot->SetPadding(FMargin(28.f, 0.f, 0.f, 0.f));
	return Panel;
}

// ---------------------------------------------------------------------------
// The choice
// ---------------------------------------------------------------------------

void UHeroSelectWidget::Select(int32 Index, bool bSound)
{
	if (Heroes.IsEmpty() || IsConfirming())
	{
		return;
	}
	const int32 Clamped = FMath::Clamp(Index, 0, Heroes.Num() - 1);
	if (Clamped == Selected)
	{
		return;
	}
	Selected = Clamped;
	if (bSound)
	{
		LooterSound::Play2D(this, LooterSoundCue::Hover);
	}
	// The new hero fades in as they turn in on the stand (or their portrait settles in), felt as well as seen.
	StageIn = 0.f;
	SetStatus(FString(), Color::TextDim());
	ShowSelected(true);
}

bool UHeroSelectWidget::SelectById(FName Id)
{
	const int32 Index = Heroes.IndexOfByPredicate([Id](const UHeroDefinition* Hero) { return Hero && Hero->Id == Id; });
	if (Index == INDEX_NONE)
	{
		return false;
	}
	Select(Index, false);
	return true;
}

void UHeroSelectWidget::ShowSelected(bool bSwitched)
{
	RestyleCards();
	const UHeroDefinition* Hero = GetHero(Selected);
	if (!Hero || !NameText)
	{
		return;
	}
	const bool bPickable = CanConfirm(Selected);

	// The stage: the hero live on the stand, or (their body not in the game) the whole painting, greyed and stamped for a
	// hero in the works; the glow in their colour behind either.
	ShowStandHero(bSwitched);
	UTexture2D* Portrait = Portraits.IsValidIndex(Selected) ? Portraits[Selected].Get() : nullptr;
	StageImage->SetBrush(Portrait ? PortraitBrush(Portrait, FVector2D(PortraitSize, PortraitSize), ECrop::Whole) : RectBrush(Color::Track(), WithAlpha(Hero->Color, 0.6f), 2.f));
	StageImage->SetColorAndOpacity(bPickable ? FLinearColor::White : GreyedTint());
	StageWash->SetVisibility(bPickable ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	StageStamp->SetVisibility(bPickable ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	StageGlow->SetColorAndOpacity(WithAlpha(Hero->Color, bPickable ? StageGlowAlpha : GreyedGlowAlpha));

	// The panel: who they are (their painted face beside their name), then their three powers.
	PanelFace->SetContent(MakePortrait(WidgetTree, Portrait, FVector2D(PanelFaceSize, PanelFaceSize), ECrop::Face, !bPickable, Hero->Color));
	RoleText->SetText(FText::FromString(Hero->Role.ToString().ToUpper()));
	RoleWorks->SetVisibility(bPickable ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	NameText->SetText(FText::FromString(Hero->DisplayName.ToString().ToUpper()));
	NameRule->SetColorAndOpacity(bPickable ? Hero->Color : Color::RowLine());
	StoryText->SetText(Hero->Story);
	PowerList->ClearChildren();
	for (int32 Index = 0; Index < Hero->Powers.Num(); ++Index)
	{
		const UEmberDefinition* Power = Hero->Powers[Index];
		if (!Power)
		{
			continue;
		}
		// A hairline between rows, from under the medallions' edge.
		const bool bFirst = PowerList->GetChildrenCount() == 0;
		if (!bFirst)
		{
			PowerList->AddChildToVerticalBox(MakeSized(WidgetTree, MakeImage(WidgetTree, RectBrush(Color::RowLine())), 0.f, 1.f))
				->SetPadding(FMargin(68.f, 10.f, 0.f, 10.f));
		}
		PowerList->AddChildToVerticalBox(MakePowerRow(WidgetTree, *Power))->SetPadding(FMargin(0.f, bFirst ? 8.f : 0.f, 0.f, 0.f));
	}

	ConfirmButton->SetLabel(FText::FromString(ConfirmWords(*Hero, bPickable)));
	ConfirmButton->SetIsEnabled(bPickable);
	RefreshFooter();
}

void UHeroSelectWidget::SetStatus(const FString& Words, const FLinearColor& Tone)
{
	if (StatusText)
	{
		StatusText->SetText(FText::FromString(Words));
		StatusText->SetColorAndOpacity(FSlateColor(Tone));
	}
}

FString UHeroSelectWidget::GetStatus() const
{
	return StatusText ? StatusText->GetText().ToString() : FString();
}


// UPauseMenuWidget's hero card (E2b, mockups/e.js "C's hero card"): top-right in the gun card's tag shape with the level gem
// on top, the HUD's portrait in its medallion, HERO · LEVEL n and the name, health, experience, soul flask and grenades,
// the experience bar. The tracked mission under it is PauseMenuMission.cpp, the session tag PauseMenuSession.cpp; each
// reads the game again as the hub opens (RefreshCards).

#include "UI/Menus/PauseMenuWidget.h"
#include "UI/Menus/PauseMenuArt.h"
#include "UI/Menus/PauseMenuCardKit.h"
#include "UI/Menus/PauseMenuIcons.h"
#include "UI/Menus/PauseMenuLayout.h"
#include "UI/HUD/HudPortraitWidget.h"
#include "UI/Style/LooterReticleArt.h"
#include "UI/Style/LooterUIStyle.h"
#include "Combat/HealthComponent.h"
#include "Embers/HeroDefinition.h"
#include "Player/PlayerFlaskComponent.h"
#include "Player/PlayerThrowComponent.h"
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
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Pawn.h"

using namespace PauseMenuLayout;
using namespace PauseMenuCardKit;
using namespace LooterUI;

namespace
{
	// --- The hero card (e.js cards(): its gem 48 over the top, its words 24 in and 40 down, rows 44 tall) ---

	constexpr float HeroGem = 48.f;
	constexpr float HeroGemAbove = 18.f;
	constexpr float HeroPadSide = 24.f;
	constexpr float HeroPadTop = 40.f;
	constexpr float HeroInner = CardWidth - HeroPadSide * 2.f;
	/** The portrait's medallion, and the HUD's portrait inside its bezel. */
	constexpr float PortraitSize = 96.f;
	constexpr float PortraitInset = 5.f;
	constexpr float PortraitGap = 18.f;
	constexpr float HeroRowHeight = 44.f;
	constexpr float HeroRowPadX = 16.f;
	constexpr float HeroRowIcon = 24.f;
	constexpr float HeroBlockGap = 12.f;
	constexpr float XPBarHeight = 8.f;
	constexpr float PipStep = 15.f;
	constexpr float PipCountGap = 10.f;

	FString Number(int64 Value)
	{
		return FText::AsNumber(Value).ToString();
	}
}

// ---------------------------------------------------------------------------
// The hero card
// ---------------------------------------------------------------------------

void UPauseMenuWidget::MakeHeroCard()
{
	UCanvasPanel* Card = MakeGroup(FVector2D(CardX, HeroY), FVector2D(CardWidth, HeroHeight), true);
	Card->SetVisibility(ESlateVisibility::HitTestInvisible);
	HeroGroup = Card;
	Put(Card, MakeShape(TEXT("PauseHeroCard"), PauseMenuArt::TagCard(CardWidth, HeroHeight, 70.f, 28.f, 4.f, 1.f, PauseMenuArt::EPart::Fill),
		PauseMenuArt::TagCard(CardWidth, HeroHeight, 70.f, 28.f, 4.f, 1.f, PauseMenuArt::EPart::Lines), FVector2D(CardWidth, HeroHeight)),
		FVector2D::ZeroVector, FVector2D(CardWidth, HeroHeight));

	// The level gem on the card's step, its number in gold.
	const FVector2D GemAt(CardWidth * 0.5f - HeroGem * 0.5f, -HeroGemAbove);
	Put(Card, MakePicture(TEXT("PauseHeroGem"), PauseMenuArt::GoldGem(HeroGem, 3.5f)), GemAt, FVector2D::ZeroVector);
	UTextBlock* Level = Words(WidgetTree, 20, true, Color::SoulGold());
	Level->SetJustification(ETextJustify::Center);
	Put(Card, MakeBand(Level, HeroGem, HeroGem, HAlign_Center), GemAt, FVector2D::ZeroVector);
	Hero.Level = Level;

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());

	// The portrait (the HUD's, the hero played) in its medallion, HERO · LEVEL n and the hero's name beside it.
	UHorizontalBox* Header = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UCanvasPanel* Medal = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
	UHudPortraitWidget* Portrait = WidgetTree->ConstructWidget<UHudPortraitWidget>(UHudPortraitWidget::StaticClass());
	UScaleBox* PortraitFit = WidgetTree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass());
	PortraitFit->SetStretch(EStretch::ScaleToFit);
	PortraitFit->SetContent(Portrait);
	const float Window = PortraitSize - PortraitInset * 2.f;
	Put(Medal, MakeSized(WidgetTree, PortraitFit, Window, Window), FVector2D(PortraitInset, PortraitInset), FVector2D(Window, Window));
	Put(Medal, MakePicture(TEXT("PauseHeroMedallion"), PauseMenuArt::Medallion(PortraitSize)), FVector2D(-LooterReticle::Pad, -LooterReticle::Pad),
		FVector2D::ZeroVector);
	AddTo(Header, MakeSized(WidgetTree, Medal, PortraitSize, PortraitSize));
	UVerticalBox* Names = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	UTextBlock* Label = Words(WidgetTree, 16, true, Color::SlateFlavor(), 260);
	Names->AddChildToVerticalBox(Label);
	UTextBlock* Name = Words(WidgetTree, 29, true, Color::SlateText(), 50);
	Names->AddChildToVerticalBox(ShrinkToFit(WidgetTree, Name))->SetHorizontalAlignment(HAlign_Left);
	AddTo(Header, Names, PortraitGap)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	Hero.Label = Label;
	Hero.Name = Name;
	Column->AddChildToVerticalBox(Header);

	// The rows, banded like the gun card's stat list: an icon, the label, the value at the right.
	UVerticalBox* Rows = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	auto AddRow = [this, Rows](const FSlateBrush& IconBrush, const FLinearColor& IconTint, const TCHAR* LabelText, UWidget* Value, bool bBand)
	{
		UOverlay* Row = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		if (bBand)
		{
			UImage* Band = MakeImage(WidgetTree, RectBrush(Color::SlateBand()));
			MarkBackground(Band);
			FillOverlaySlot(Row->AddChildToOverlay(Band));
		}
		UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		UImage* RowIcon = MakeImage(WidgetTree, IconBrush);
		RowIcon->SetColorAndOpacity(IconTint);
		AddTo(Line, MakeSized(WidgetTree, RowIcon, HeroRowIcon, HeroRowIcon));
		UTextBlock* RowLabel = Words(WidgetTree, 20, true, Color::SlateLabel());
		RowLabel->SetText(FText::FromString(LabelText));
		AddTo(Line, RowLabel, 14.f)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		AddTo(Line, Value, 12.f);
		FillOverlaySlot(Row->AddChildToOverlay(Line), FMargin(HeroRowPadX, 0.f));
		Rows->AddChildToVerticalBox(MakeSized(WidgetTree, Row, 0.f, HeroRowHeight));
	};
	auto MakeIcon = [](const TCHAR* Id)
	{
		return PaintedIconBrush(FName(*FString::Printf(TEXT("PauseIcon%s"), Id)), PauseMenuIcons::Get(Id), HeroRowIcon * PictureResolution / 24.f,
			FVector2D(HeroRowIcon, HeroRowIcon));
	};
	auto MakePair = [this](TWeakObjectPtr<UTextBlock>& Bold, TWeakObjectPtr<UTextBlock>& Dim)
	{
		UHorizontalBox* Pair = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		UTextBlock* First = Words(WidgetTree, 22, true, Color::SlateText());
		UTextBlock* Second = Words(WidgetTree, 22, false, Color::SlateLabel());
		AddTo(Pair, First);
		AddTo(Pair, Second);
		Bold = First;
		Dim = Second;
		return Pair;
	};
	auto MakePips = [this](int32 Count, TArray<TWeakObjectPtr<UImage>>& OutPips, TWeakObjectPtr<UTextBlock>& OutCount)
	{
		UHorizontalBox* Pips = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		OutPips.Reset();
		for (int32 Index = 0; Index < Count; ++Index)
		{
			UImage* Pip = MakePicture(TEXT("PausePipOff"), PauseMenuArt::Pip(Color::SaltLine(), false));
			// Each pip's picture keeps a pixel round its lean; set them a step apart.
			AddTo(Pips, Pip, Index > 0 ? PipStep - (PauseMenuArt::PipWidth + PauseMenuArt::PipLean + 2.f) : 0.f);
			OutPips.Add(Pip);
		}
		UTextBlock* Text = Words(WidgetTree, 22, true, Color::SlateText());
		AddTo(Pips, Text, PipCountGap);
		OutCount = Text;
		return Pips;
	};

	AddRow(MakeIcon(TEXT("Health")), Color::SlateIcon(), TEXT("Health"), MakePair(Hero.Health, Hero.HealthMax), true);
	AddRow(MakeIcon(TEXT("Experience")), Color::SlateIcon(), TEXT("Experience"), MakePair(Hero.XP, Hero.XPNext), false);
	const FInkedIcon* Flask = FindInkedIcon(TEXT("SoulFlask"));
	const FInkedIcon* Grenade = FindInkedIcon(TEXT("GraveSaltGrenade"));
	const FVector2D IconBox(HeroRowIcon, HeroRowIcon);
	AddRow(Flask ? InkedIconBrush(TEXT("SoulFlask"), *Flask, IconBox) : MakeIcon(TEXT("Health")), FLinearColor::White, TEXT("Soul flask"),
		MakePips(UPlayerFlaskComponent::GetMaxCharges(), Hero.FlaskPips, Hero.Flask), true);
	AddRow(Grenade ? InkedIconBrush(TEXT("GraveSaltGrenade"), *Grenade, IconBox) : MakeIcon(TEXT("Health")), FLinearColor::White, TEXT("Grenades"),
		MakePips(UPlayerThrowComponent::GetMaxGrenades(), Hero.GrenadePips, Hero.Grenades), false);
	Column->AddChildToVerticalBox(Rows)->SetPadding(FMargin(0.f, HeroBlockGap, 0.f, 0.f));

	// The experience bar at its foot: the dark track, the gold fill clipped to how far the level is.
	UOverlay* Bar = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	Bar->AddChildToOverlay(MakePicture(TEXT("PauseXPTrack"), PauseMenuArt::SlantBar(HeroInner, XPBarHeight, false, Color::SlateFlavor())));
	UImage* Fill = MakePicture(TEXT("PauseXPFill"), PauseMenuArt::SlantBar(HeroInner, XPBarHeight, true, Color::SlateFlavor()));
	UCanvasPanel* FillCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
	Put(FillCanvas, Fill, FVector2D::ZeroVector, FVector2D::ZeroVector);
	USizeBox* FillBox = MakeSized(WidgetTree, FillCanvas, 0.f, XPBarHeight);
	FillBox->SetClipping(EWidgetClipping::ClipToBounds);
	UOverlaySlot* FillSlot = Bar->AddChildToOverlay(FillBox);
	FillSlot->SetHorizontalAlignment(HAlign_Left);
	Hero.XPFill = FillBox;
	Column->AddChildToVerticalBox(MakeSized(WidgetTree, Bar, HeroInner, XPBarHeight))->SetPadding(FMargin(0.f, HeroBlockGap, 0.f, 0.f));

	Put(Card, Column, FVector2D(HeroPadSide, HeroPadTop), FVector2D(HeroInner, HeroHeight - HeroPadTop));
}

void UPauseMenuWidget::RefreshHero()
{
	const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	const UPlayerProgressionSubsystem* Progression = LocalPlayer ? LocalPlayer->GetSubsystem<UPlayerProgressionSubsystem>() : nullptr;
	const int32 Level = Progression ? Progression->GetLevel() : 1;
	SetWords(Hero.Level, FString::FromInt(Level));
	SetWords(Hero.Label, FString::Printf(TEXT("HERO · LEVEL %d"), Level));

	// The hero played (the session's pick, or Looter.Hero's with none); Ellis's in a test world.
	const UHeroDefinition* Played = USessionSubsystem::FindPlayedHero(this);
	if (!Played)
	{
		Played = UHeroDefinition::Load(UHeroDefinition::DefaultId());
	}
	SetWords(Hero.Name, Played ? Played->DisplayName.ToString().ToUpper() : FString());

	// Health as the HUD's player frame shows it (rounded up, so a sliver left never reads 0).
	const APawn* Pawn = GetOwningPlayerPawn();
	const UHealthComponent* Health = Pawn ? Pawn->FindComponentByClass<UHealthComponent>() : nullptr;
	if (Health)
	{
		const int32 MaxPoints = FMath::Max(1, FMath::RoundToInt32(Health->GetMaxHealth()));
		SetWords(Hero.Health, Number(FMath::Clamp(FMath::CeilToInt32(Health->GetHealth()), 0, MaxPoints)));
		SetWords(Hero.HealthMax, FString::Printf(TEXT(" / %s"), *Number(MaxPoints)));
	}
	else
	{
		SetWords(Hero.Health, TEXT("-"));
		SetWords(Hero.HealthMax, FString());
	}

	// Experience into this level, of what the level takes; the bar under the card.
	const bool bMax = Progression && Progression->IsMaxLevel();
	SetWords(Hero.XP, bMax ? FString(TEXT("MAX")) : Number(Progression ? Progression->GetXP() : 0));
	SetWords(Hero.XPNext, bMax ? FString() : FString::Printf(TEXT(" / %s"), *Number(Progression ? Progression->GetXPToNextLevel() : 0)));
	if (USizeBox* Fill = Hero.XPFill.Get())
	{
		const float Share = bMax ? 1.f : (Progression ? FMath::Clamp(Progression->GetLevelProgress(), 0.f, 1.f) : 0.f);
		Fill->SetWidthOverride(FMath::Max(HeroInner * Share, 0.f));
		Fill->SetVisibility(Share > 0.f ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	// The soul flask's charges (heal green) and the grenades carried (salt), as pips and a count.
	auto ShowPips = [this](TArray<TWeakObjectPtr<UImage>>& Pips, TWeakObjectPtr<UTextBlock>& Count, int32 Have, int32 Max, const FLinearColor& Tint,
		const TCHAR* LitName)
	{
		for (int32 Index = 0; Index < Pips.Num(); ++Index)
		{
			if (UImage* Pip = Pips[Index].Get())
			{
				const bool bLit = Index < Have;
				Pip->SetBrush(PictureBrush(bLit ? LitName : TEXT("PausePipOff"), PauseMenuArt::Pip(Tint, bLit)));
			}
		}
		SetWords(Count, Max > 0 ? FString::Printf(TEXT("%d / %d"), Have, Max) : FString(TEXT("-")));
	};
	const UPlayerFlaskComponent* Flask = UPlayerFlaskComponent::Find(Pawn);
	ShowPips(Hero.FlaskPips, Hero.Flask, Flask ? Flask->GetCharges() : 0, Flask ? UPlayerFlaskComponent::GetMaxCharges() : 0, Color::Heal(), TEXT("PausePipHeal"));
	const UPlayerThrowComponent* Throw = UPlayerThrowComponent::Find(Pawn);
	ShowPips(Hero.GrenadePips, Hero.Grenades, Throw ? Throw->GetGrenades() : 0, Throw ? UPlayerThrowComponent::GetMaxGrenades() : 0, Color::SaltLine(),
		TEXT("PausePipSalt"));
}

void UPauseMenuWidget::RefreshCards()
{
	RefreshHero();
	RefreshMission();
	RefreshSession();
}

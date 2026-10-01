#include "UI/World/CreatureHealthBarWidget.h"
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

using namespace LooterUI;

namespace
{
	constexpr float BarHeight = 6.f;
	/** The same lean as the HUD's bars. */
	constexpr float BarSlant = 16.f;
	/** How long the chip of lost health stays before it drains, and how fast it drains (bar lengths per second). */
	constexpr float ChipHold = 0.4f;
	constexpr float ChipDrainRate = 0.8f;

	/** The dividers: solid near-black cuts, clear against the red and the empty track alike. */
	constexpr float DividerWidth = 2.f;
	const FLinearColor DividerColor(0.01f, 0.01f, 0.015f, 1.f);

	/** A dark rim round the whole bar so it holds its shape over bright sky and grass, and a lit strip along the red's top. */
	const FLinearColor RimColor(0.f, 0.f, 0.f, 0.8f);
	constexpr float HighlightHeight = 1.5f;

	USizeBox* AddFill(UWidgetTree* Tree, UOverlay* Bar, UWidget* Content)
	{
		USizeBox* Fill = MakeSized(Tree, Content, 0.f, BarHeight);
		UOverlaySlot* FillSlot = Bar->AddChildToOverlay(Fill);
		FillSlot->SetHorizontalAlignment(HAlign_Left);
		FillSlot->SetVerticalAlignment(VAlign_Fill);
		return Fill;
	}
}

TSharedRef<SWidget> UCreatureHealthBarWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());

		// "LV 1  Brown Spider": a small dim level, then the name as it's written, centered over the bar.
		UHorizontalBox* Label = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		LevelText = MakeFloatingText(WidgetTree, 10, Color::TextDim(), 80);
		UHorizontalBoxSlot* LevelSlot = Label->AddChildToHorizontalBox(LevelText);
		LevelSlot->SetVerticalAlignment(VAlign_Bottom);
		LevelSlot->SetPadding(FMargin(0.f, 0.f, 6.f, 1.f));
		NameText = MakeFloatingText(WidgetTree, 13, Color::Text(), 20);
		Label->AddChildToHorizontalBox(NameText)->SetVerticalAlignment(VAlign_Bottom);
		Box->AddChildToVerticalBox(Label)->SetHorizontalAlignment(HAlign_Center);
		ApplyLabel();

		// The bar, back to front: a dark track (faded by the UI transparency setting), the chip, the health with its lit
		// top edge, the quarter lines, then the rim.
		UOverlay* Bar = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		UImage* Track = MakeImage(WidgetTree, RectBrush(FLinearColor(0.01f, 0.015f, 0.02f, 0.65f)));
		MarkBackground(Track);
		FillOverlaySlot(Bar->AddChildToOverlay(Track));
		ChipFill = AddFill(WidgetTree, Bar, MakeImage(WidgetTree, RectBrush(FLinearColor(1.f, 0.82f, 0.72f, 0.85f))));

		UOverlay* Health = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		FillOverlaySlot(Health->AddChildToOverlay(MakeImage(WidgetTree, RectBrush(Color::Health()))));
		UOverlaySlot* HighlightSlot = Health->AddChildToOverlay(MakeSized(WidgetTree,
			MakeImage(WidgetTree, RectBrush(FMath::Lerp(Color::Health(), FLinearColor::White, 0.45f))), 0.f, HighlightHeight));
		HighlightSlot->SetHorizontalAlignment(HAlign_Fill);
		HighlightSlot->SetVerticalAlignment(VAlign_Top);
		HealthFill = AddFill(WidgetTree, Bar, Health);

		UCanvasPanel* Dividers = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
		for (const float Position : DividerPositions())
		{
			UCanvasPanelSlot* LineSlot = Dividers->AddChildToCanvas(MakeImage(WidgetTree, RectBrush(DividerColor)));
			LineSlot->SetPosition(FVector2D(BarWidth * Position - DividerWidth * 0.5f, 0.f));
			LineSlot->SetSize(FVector2D(DividerWidth, BarHeight));
		}
		FillOverlaySlot(Bar->AddChildToOverlay(Dividers));
		FillOverlaySlot(Bar->AddChildToOverlay(MakeImage(WidgetTree, RectBrush(FLinearColor::Transparent, RimColor, 1.f))));

		USizeBox* BarSize = MakeSized(WidgetTree, Bar, BarWidth, BarHeight);
		BarSize->SetRenderShear(FVector2D(BarSlant, 0.f));
		UVerticalBoxSlot* BarSlot = Box->AddChildToVerticalBox(BarSize);
		BarSlot->SetHorizontalAlignment(HAlign_Center);
		BarSlot->SetPadding(FMargin(0.f, 3.f, 0.f, 0.f));

		Box->SetVisibility(ESlateVisibility::HitTestInvisible);
		WidgetTree->RootWidget = Box;
		ApplyBar();
	}
	return Super::RebuildWidget();
}

void UCreatureHealthBarWidget::SetCreature(const FText& InName, int32 InLevel)
{
	if (!CreatureName.EqualTo(InName) || CreatureLevel != InLevel)
	{
		CreatureName = InName;
		CreatureLevel = InLevel;
		ApplyLabel();
	}
}

void UCreatureHealthBarWidget::ApplyLabel()
{
	if (LevelText && NameText)
	{
		LevelText->SetText(FText::FromString(FString::Printf(TEXT("LV %d"), CreatureLevel)));
		NameText->SetText(CreatureName);
	}
}

TArray<float> UCreatureHealthBarWidget::DividerPositions()
{
	TArray<float> Positions;
	for (int32 Part = 1; Part < Parts; ++Part)
	{
		Positions.Add(static_cast<float>(Part) / Parts);
	}
	return Positions;
}

void UCreatureHealthBarWidget::SetHealth(float Health, float MaxHealth)
{
	const float NewFraction = MaxHealth > 0.f ? FMath::Clamp(Health / MaxHealth, 0.f, 1.f) : 0.f;
	if (FMath::IsNearlyEqual(NewFraction, Fraction))
	{
		return;
	}
	if (NewFraction < Fraction)
	{
		// A hit: what it took lingers as the chip for a moment.
		GhostHold = ChipHold;
	}
	Fraction = NewFraction;
	GhostFraction = FMath::Max(GhostFraction, Fraction);
	ApplyBar();
}

void UCreatureHealthBarWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (GhostFraction <= Fraction)
	{
		return;
	}
	if (GhostHold > 0.f)
	{
		GhostHold -= InDeltaTime;
		return;
	}
	GhostFraction = FMath::Max(Fraction, GhostFraction - ChipDrainRate * InDeltaTime);
	ApplyBar();
}

void UCreatureHealthBarWidget::ApplyBar()
{
	if (HealthFill && ChipFill)
	{
		HealthFill->SetWidthOverride(BarWidth * Fraction);
		ChipFill->SetWidthOverride(BarWidth * GhostFraction);
	}
}

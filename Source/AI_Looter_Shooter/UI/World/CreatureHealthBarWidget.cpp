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
	constexpr float BarHeight = 5.f;
	/** The same lean as the HUD's bars. */
	constexpr float BarSlant = 16.f;
	/** How long the chip of lost health stays before it drains, and how fast it drains (bar lengths per second). */
	constexpr float ChipHold = 0.4f;
	constexpr float ChipDrainRate = 0.8f;

	/**
	 * A divider: a thin dark cut through the bar, 2 px where they're far apart and down to 1 px where they crowd, so a
	 * tough creature's bar stays mostly red. Dividers never come closer than MinDividerSpacing pixels.
	 */
	constexpr float MaxDividerWidth = 2.f;
	constexpr float MinDividerWidth = 1.f;
	constexpr float MinDividerSpacing = 4.f;
	const FLinearColor DividerColor(0.f, 0.01f, 0.02f, 0.9f);

	USizeBox* MakeFill(UWidgetTree* Tree, UOverlay* Bar, const FLinearColor& FillColor)
	{
		USizeBox* Fill = MakeSized(Tree, MakeImage(Tree, RectBrush(FillColor)), 0.f, BarHeight);
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

		// The bar: a faint dark track (faded by the UI transparency setting), the chip, then the health on top.
		UOverlay* Bar = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		UImage* Track = MakeImage(WidgetTree, RectBrush(FLinearColor(0.f, 0.02f, 0.04f, 0.5f)));
		MarkBackground(Track);
		FillOverlaySlot(Bar->AddChildToOverlay(Track));
		ChipFill = MakeFill(WidgetTree, Bar, FLinearColor(1.f, 0.82f, 0.72f, 0.85f));
		HealthFill = MakeFill(WidgetTree, Bar, Color::Health());
		// The dividers lie over everything, leaning with the bar.
		Dividers = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
		FillOverlaySlot(Bar->AddChildToOverlay(Dividers));
		RebuildDividers();
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

TArray<float> UCreatureHealthBarWidget::DividerPositions(float MaxHealth)
{
	TArray<float> Positions;
	if (MaxHealth <= HealthPerDivider)
	{
		return Positions;
	}
	float Step = HealthPerDivider;
	while (BarWidth * Step / MaxHealth < MinDividerSpacing)
	{
		Step *= 10.f;
	}
	// No line on the bar's end: 300 health is three parts, two lines.
	for (float At = Step; At < MaxHealth - 0.5f; At += Step)
	{
		Positions.Add(At / MaxHealth);
	}
	return Positions;
}

void UCreatureHealthBarWidget::RebuildDividers()
{
	if (!Dividers)
	{
		return;
	}
	Dividers->ClearChildren();
	const TArray<float> Positions = DividerPositions(ShownMaxHealth);
	const float Spacing = Positions.IsEmpty() ? BarWidth : BarWidth * Positions[0];
	const float Width = FMath::Clamp(Spacing * 0.15f, MinDividerWidth, MaxDividerWidth);
	for (const float Position : Positions)
	{
		UCanvasPanelSlot* LineSlot = Dividers->AddChildToCanvas(MakeImage(WidgetTree, RectBrush(DividerColor)));
		LineSlot->SetPosition(FVector2D(BarWidth * Position - Width * 0.5f, 0.f));
		LineSlot->SetSize(FVector2D(Width, BarHeight));
	}
}

void UCreatureHealthBarWidget::SetHealth(float Health, float MaxHealth)
{
	if (!FMath::IsNearlyEqual(MaxHealth, ShownMaxHealth))
	{
		ShownMaxHealth = MaxHealth;
		RebuildDividers();
	}
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

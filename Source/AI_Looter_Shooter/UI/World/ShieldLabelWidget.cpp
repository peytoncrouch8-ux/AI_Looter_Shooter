#include "UI/World/ShieldLabelWidget.h"
#include "Shields/ShieldDefinition.h"
#include "Shields/ShieldRules.h"
#include "Shields/ShieldText.h"
#include "UI/Style/LooterUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "HAL/PlatformTime.h"

// The ward's label from afar (its icon, name and trait's name, floating as a gun's) and which of its looks shows. The
// looked-at card over the ward box is ShieldLabelWidgetCard.cpp.

using namespace LooterUI;

namespace
{
	/** The Inked icon beside the name from afar. */
	constexpr float IconSize = 22.f;
	/** A look back at the same ward this soon (seconds) after looking away plays no intro again (the gun label's rule). */
	constexpr double RefocusQuietSeconds = 0.3;
}

TSharedRef<SWidget> UShieldLabelWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
		WidgetTree->RootWidget = Root;

		// Two looks, one at a time: the lines from afar, the card when looked at (each collapses whole).
		UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Lines"));
		UOverlaySlot* LinesSlot = Root->AddChildToOverlay(Box);
		LinesSlot->SetHorizontalAlignment(HAlign_Fill);
		LinesSlot->SetVerticalAlignment(VAlign_Fill);
		LinesSlot->SetPadding(FMargin(12.f, 6.f));
		Lines = Box;

		// The card stands on its tether's diamond, at the label's foot where the name hung (the component's pivot is 0.5, 1).
		UOverlaySlot* StackSlot = Root->AddChildToOverlay(BuildFocusStack());
		StackSlot->SetHorizontalAlignment(HAlign_Center);
		StackSlot->SetVerticalAlignment(VAlign_Bottom);

		UHorizontalBox* NameLine = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		Icon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		NameLine->AddChildToHorizontalBox(Icon)->SetVerticalAlignment(VAlign_Center);
		NameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Name"));
		UHorizontalBoxSlot* NameSlot = NameLine->AddChildToHorizontalBox(NameText);
		NameSlot->SetVerticalAlignment(VAlign_Center);
		NameSlot->SetPadding(FMargin(6.f, 0.f, 0.f, 0.f));
		Box->AddChildToVerticalBox(NameLine)->SetHorizontalAlignment(HAlign_Center);
		TraitText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Trait"));
		Box->AddChildToVerticalBox(TraitText)->SetHorizontalAlignment(HAlign_Center);
		ApplyContent();
	}
	return Super::RebuildWidget();
}

void UShieldLabelWidget::SetWard(const FShieldInstanceData& InWard, const FShieldInstanceData* InWorn)
{
	// The ward worn is read live by the card (UpdateCard), so a swap shows at once; InWorn stays for the pickup's call.
	const uint32 Identity = ShieldRules::Identity(InWard);
	if (Identity == ShownIdentity)
	{
		return;
	}
	Ward = InWard;
	ShownIdentity = Identity;
	ApplyContent();
	if (bIsFocused)
	{
		UpdateFromViewer();
	}
}

void UShieldLabelWidget::SetSource(const AActor* InSource)
{
	Source = InSource;
}

void UShieldLabelWidget::SetFocused(bool bFocused)
{
	if (bIsFocused == bFocused)
	{
		return;
	}
	bIsFocused = bFocused;
	// A ward newly looked at unfolds its card with its sound; a look back within a blink just shows it again.
	const double Now = FPlatformTime::Seconds();
	if (bFocused)
	{
		bIntroPending = Now - LostFocusAt > RefocusQuietSeconds;
	}
	else
	{
		LostFocusAt = Now;
	}
	ApplyContent();
}

FText UShieldLabelWidget::GetNameText() const
{
	return NameText ? NameText->GetText() : FText::GetEmpty();
}

FText UShieldLabelWidget::GetTraitText() const
{
	return TraitText ? TraitText->GetText() : FText::GetEmpty();
}

void UShieldLabelWidget::ApplyContent()
{
	if (!NameText || !Ward.IsSet())
	{
		return;
	}
	// From afar: the ward box's Inked icon and its name in its rarity's colour, outlined, floating as a gun's name.
	const FName IconName = Ward.Definition ? Ward.Definition->IconName : FName(TEXT("WardBox"));
	if (const FInkedIcon* Inked = FindInkedIcon(IconName))
	{
		Icon->SetBrush(InkedIconBrush(IconName, *Inked, FVector2D(IconSize, IconSize)));
		Icon->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	else
	{
		Icon->SetVisibility(ESlateVisibility::Collapsed);
	}
	NameText->SetText(FText::FromString(ShieldText::Name(Ward).ToUpper()));
	StyleFloatingText(NameText, 14, ShieldText::Color(Ward), 80);

	// Its trait's name under it (the reason to walk over), in the trait's colour, as a gun's element line.
	const FString Trait = ShieldText::TraitName(Ward.Trait).ToUpper();
	TraitText->SetText(FText::FromString(Trait));
	StyleFloatingText(TraitText, 11, ShieldText::TraitColor(Ward), 120);
	TraitText->SetVisibility(Trait.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);

	// Looked at: the card over the box in place of the lines.
	FocusStack->SetVisibility(bIsFocused ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	Lines->SetVisibility(bIsFocused ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);
	TetherDiamond->SetColorAndOpacity(ShieldText::Color(Ward));
}

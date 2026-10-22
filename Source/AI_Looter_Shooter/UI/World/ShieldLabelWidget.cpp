#include "UI/World/ShieldLabelWidget.h"
#include "Settings/KeyBindingSubsystem.h"
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
#include "Engine/LocalPlayer.h"

using namespace LooterUI;

namespace
{
	/** The Inked icon beside the name: small from afar, a size up once looked at. */
	constexpr float IconFar = 22.f;
	constexpr float IconNear = 30.f;

	/** "+6", "-0.4": a number's change, signed (whole points, or tenths of a second). */
	FString SignedWhole(float Delta)
	{
		return FString::Printf(TEXT("%+d"), FMath::RoundToInt32(Delta));
	}

	FString SignedTenths(float Delta)
	{
		return FString::Printf(TEXT("%+.1f"), Delta);
	}
}

TSharedRef<SWidget> UShieldLabelWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
		WidgetTree->RootWidget = Root;
		auto AddFill = [Root](UWidget* Child, const FMargin& ChildPadding)
		{
			UOverlaySlot* ChildSlot = Root->AddChildToOverlay(Child);
			ChildSlot->SetHorizontalAlignment(HAlign_Fill);
			ChildSlot->SetVerticalAlignment(VAlign_Fill);
			ChildSlot->SetPadding(ChildPadding);
		};
		// The plate a gun's label opens when looked at: dark glass (a background) and its cyan line.
		PlateFill = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		PlateFill->SetBrush(ShapeBrush(EShape::Control, false, Color::ScreenBg()));
		MarkBackground(PlateFill);
		AddFill(PlateFill, FMargin(0.f));
		PlateLine = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		PlateLine->SetBrush(ShapeBrush(EShape::Control, true, Color::ScreenLine()));
		AddFill(PlateLine, FMargin(0.f));

		UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		UHorizontalBox* NameLine = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		Icon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		NameLine->AddChildToHorizontalBox(Icon)->SetVerticalAlignment(VAlign_Center);
		NameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Name"));
		UHorizontalBoxSlot* NameSlot = NameLine->AddChildToHorizontalBox(NameText);
		NameSlot->SetVerticalAlignment(VAlign_Center);
		NameSlot->SetPadding(FMargin(6.f, 0.f, 0.f, 0.f));
		Box->AddChildToVerticalBox(NameLine)->SetHorizontalAlignment(HAlign_Center);

		TraitText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Trait"));
		StatsText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Stats"));
		CompareText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Compare"));
		PromptText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Prompt"));
		for (UTextBlock* Line : { TraitText.Get(), StatsText.Get(), CompareText.Get(), PromptText.Get() })
		{
			Box->AddChildToVerticalBox(Line)->SetHorizontalAlignment(HAlign_Center);
		}
		AddFill(Box, FMargin(12.f, 6.f));
		ApplyContent();
	}
	return Super::RebuildWidget();
}

void UShieldLabelWidget::SetWard(const FShieldInstanceData& InWard, const FShieldInstanceData* InWorn)
{
	const uint32 Identity = ShieldRules::Identity(InWard);
	const uint32 WornIdentity = InWorn ? ShieldRules::Identity(*InWorn) : 0;
	if (Identity == ShownIdentity && WornIdentity == ShownWorn)
	{
		return;
	}
	Ward = InWard;
	bHasWorn = InWorn != nullptr;
	Worn = InWorn ? InWorn->Stats : FShieldStats();
	ShownIdentity = Identity;
	ShownWorn = WornIdentity;
	ApplyContent();
}

void UShieldLabelWidget::SetFocused(bool bFocused)
{
	if (bIsFocused != bFocused)
	{
		bIsFocused = bFocused;
		ApplyContent();
	}
}

FText UShieldLabelWidget::GetNameText() const
{
	return NameText ? NameText->GetText() : FText::GetEmpty();
}

FText UShieldLabelWidget::GetStatsText() const
{
	return StatsText ? StatsText->GetText() : FText::GetEmpty();
}

void UShieldLabelWidget::ApplyContent()
{
	if (!NameText || !Ward.IsSet())
	{
		return;
	}
	const ESlateVisibility Detail = bIsFocused ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;
	PlateFill->SetVisibility(bIsFocused ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	PlateLine->SetVisibility(bIsFocused ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);

	// The ward box's Inked icon, then its name in its rarity's colour, as a gun's label names it.
	const FName IconName = Ward.Definition ? Ward.Definition->IconName : FName(TEXT("WardBox"));
	if (const FInkedIcon* Inked = FindInkedIcon(IconName))
	{
		const float Size = bIsFocused ? IconNear : IconFar;
		Icon->SetBrush(InkedIconBrush(IconName, *Inked, FVector2D(Size, Size)));
		Icon->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	else
	{
		Icon->SetVisibility(ESlateVisibility::Collapsed);
	}
	NameText->SetText(FText::FromString(ShieldText::Name(Ward).ToUpper()));
	StyleFloatingText(NameText, bIsFocused ? 17 : 14, ShieldText::Color(Ward), 80);

	// The trait from afar (its name: the reason to walk over), its whole line up close.
	const FString Trait = bIsFocused ? ShieldText::TraitLine(Ward) : ShieldText::TraitName(Ward.Trait).ToUpper();
	TraitText->SetText(FText::FromString(Trait));
	StyleFloatingText(TraitText, bIsFocused ? 12 : 11, ShieldText::TraitColor(Ward), bIsFocused ? 20 : 120);
	TraitText->SetAutoWrapText(bIsFocused);
	TraitText->SetWrapTextAt(bIsFocused ? 300.f : 0.f);
	TraitText->SetVisibility(Trait.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);

	const FShieldStats& S = Ward.Stats;
	StatsText->SetText(FText::FromString(FString::Printf(TEXT("LV %d   %s WARD   %s WAIT   %s"), Ward.Level, *ShieldText::CapacityString(S),
		*ShieldText::DelayString(S).ToUpper(), *ShieldText::RateString(S).ToUpper())));
	StyleFloatingText(StatsText, 11, Color::TextDim(), 60);
	StatsText->SetVisibility(Detail);

	// Against the ward worn: what it would add or cost, in green or red; a first ward is all gain.
	FString Compare;
	FLinearColor CompareColor = Color::Better();
	if (bHasWorn)
	{
		const float Gain = S.Capacity - Worn.Capacity;
		const float Sooner = Worn.RechargeDelay - S.RechargeDelay;
		// The verdict as the loadout gives it (ShieldRules::Worth, within 3%: about the same).
		const float Mine = ShieldRules::Worth(S);
		const float Theirs = ShieldRules::Worth(Worn);
		const int32 Verdict = Mine > Theirs * 1.03f ? 1 : (Mine < Theirs * 0.97f ? -1 : 0);
		Compare = FString::Printf(TEXT("%s WARD   %s S SOONER"), *SignedWhole(Gain), *SignedTenths(Sooner));
		CompareColor = Verdict > 0 ? Color::Better() : (Verdict < 0 ? Color::Worse() : Color::TextDim());
	}
	else
	{
		Compare = TEXT("NO WARD WORN");
	}
	CompareText->SetText(FText::FromString(Compare));
	StyleFloatingText(CompareText, 11, CompareColor, 60);
	CompareText->SetVisibility(Detail);

	const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	const UKeyBindingSubsystem* Bindings = LocalPlayer ? LocalPlayer->GetSubsystem<UKeyBindingSubsystem>() : nullptr;
	const FString Key = Bindings ? Bindings->GetKey(TEXT("Interact")).GetDisplayName().ToString().ToUpper() : FString(TEXT("E"));
	PromptText->SetText(FText::FromString(bHasWorn ? FString::Printf(TEXT("[%s] PICK UP  |  HOLD WEAR"), *Key)
		: FString::Printf(TEXT("[%s] WEAR"), *Key)));
	StyleFloatingText(PromptText, 13, Color::Accent(), 150);
	PromptText->SetVisibility(Detail);
}

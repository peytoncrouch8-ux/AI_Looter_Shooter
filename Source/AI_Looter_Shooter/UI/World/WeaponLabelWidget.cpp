#include "UI/World/WeaponLabelWidget.h"
#include "UI/Style/LooterUIStyle.h"
#include "UI/Style/WeaponText.h"
#include "Settings/KeyBindingSubsystem.h"
#include "Weapons/WeaponBase.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/LocalPlayer.h"

using namespace LooterUI;

TSharedRef<SWidget> UWeaponLabelWidget::RebuildWidget()
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

		PlateFill = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		PlateFill->SetBrush(ShapeBrush(EShape::Control, false, FLinearColor::FromSRGBColor(FColor(7, 26, 40, 225))));
		MarkBackground(PlateFill);
		AddFill(PlateFill, FMargin(0.f));
		PlateLine = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		PlateLine->SetBrush(ShapeBrush(EShape::Control, true, Color::ScreenLine()));
		AddFill(PlateLine, FMargin(0.f));

		UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		auto AddLine = [Box](UTextBlock* Text)
		{
			Box->AddChildToVerticalBox(Text)->SetHorizontalAlignment(HAlign_Center);
		};

		NameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Name"));
		StatsText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Stats"));
		PromptText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Prompt"));
		AddLine(NameText);
		AddLine(StatsText);
		AddLine(PromptText);
		AddFill(Box, FMargin(12.f, 6.f));

		ApplyContent();
	}
	return Super::RebuildWidget();
}

void UWeaponLabelWidget::SetWeapon(const AWeaponBase* Weapon)
{
	if (!Weapon)
	{
		return;
	}

	const FWeaponInstanceData& Instance = Weapon->GetInstance();
	const FWeaponStats& S = Instance.Stats;
	Name = FText::FromString(LooterWeaponText::Name(Instance).ToUpper());
	Stats = FText::FromString(FString::Printf(TEXT("LV %d   %s DMG   %.0f RPM   %d MAG"),
		Instance.Level, *LooterWeaponText::DamageString(S), S.FireRate, S.MagazineSize));
	NameColor = LooterWeaponText::Color(Instance);
	ApplyContent();
}

void UWeaponLabelWidget::SetFocused(bool bFocused)
{
	if (bIsFocused != bFocused)
	{
		bIsFocused = bFocused;
		ApplyContent();
	}
}

void UWeaponLabelWidget::ApplyContent()
{
	if (!NameText)
	{
		return;
	}

	// Unfocused: just the outlined name floating over the loot. Focused: a full plate with stats and the prompt.
	const ESlateVisibility Detail = bIsFocused ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;
	PlateFill->SetVisibility(bIsFocused ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	PlateLine->SetVisibility(bIsFocused ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);

	NameText->SetText(Name);
	StyleText(NameText, bIsFocused ? 17 : 14, NameColor, true, 80);

	StatsText->SetText(Stats);
	StyleText(StatsText, 11, Color::TextDim(), true, 60);
	StatsText->SetVisibility(Detail);

	const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	const UKeyBindingSubsystem* Bindings = LocalPlayer ? LocalPlayer->GetSubsystem<UKeyBindingSubsystem>() : nullptr;
	const FString Key = Bindings ? Bindings->GetKey(TEXT("Interact")).GetDisplayName().ToString().ToUpper() : FString(TEXT("E"));
	PromptText->SetText(FText::FromString(FString::Printf(TEXT("[%s] PICK UP"), *Key)));
	StyleText(PromptText, 13, Color::Accent(), true, 150);
	PromptText->SetVisibility(Detail);
}

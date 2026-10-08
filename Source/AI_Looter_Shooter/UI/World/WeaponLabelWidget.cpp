#include "UI/World/WeaponLabelWidget.h"
#include "UI/Style/LooterUIStyle.h"
#include "UI/Style/WeaponText.h"
#include "Settings/KeyBindingSubsystem.h"
#include "Weapons/WeaponBase.h"
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
		auto AddLine = [Box](UWidget* Line)
		{
			Box->AddChildToVerticalBox(Line)->SetHorizontalAlignment(HAlign_Center);
		};

		NameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Name"));
		FlavorText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Flavor"));
		StatsText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Stats"));
		PromptText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Prompt"));

		// A cursed iron's line: the cracked coin and the curse's name, in the curse's own brass, so the name above keeps the
		// rarity's color. ApplyContent sizes the coin and shows the row only for a cursed gun.
		UHorizontalBox* CurseLine = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("CurseLine"));
		CurseGlyph = MakeImage(WidgetTree, CrackedCoinBrush(FVector2D(16.f, 16.f)));
		CurseLine->AddChildToHorizontalBox(CurseGlyph)->SetVerticalAlignment(VAlign_Center);
		CurseText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Curse"));
		UHorizontalBoxSlot* CurseTextSlot = CurseLine->AddChildToHorizontalBox(CurseText);
		CurseTextSlot->SetVerticalAlignment(VAlign_Center);
		CurseTextSlot->SetPadding(FMargin(5.f, 0.f, 0.f, 0.f));
		CurseRow = CurseLine;

		AddLine(NameText);
		AddLine(CurseRow);
		AddLine(FlavorText);
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
	// The line in its own words, as written: it's said, not a label.
	Flavor = FText::FromString(LooterWeaponText::FlavorLine(Instance));
	// A cursed iron's curse ("HUNGRY"), empty for any other gun.
	Curse = FText::FromString(LooterWeaponText::CurseString(Instance).ToUpper());
	Stats = FText::FromString(FString::Printf(TEXT("LV %d   %s DMG   %.0f RPM   %d MAG"),
		Instance.Level, *LooterWeaponText::DamageString(S), S.FireRate, S.MagazineSize));
	NameColor = LooterWeaponText::Color(Instance);
	ApplyContent();
}

FText UWeaponLabelWidget::GetNameText() const
{
	return NameText ? NameText->GetText() : FText::GetEmpty();
}

FText UWeaponLabelWidget::GetFlavorText() const
{
	return FlavorText ? FlavorText->GetText() : FText::GetEmpty();
}

bool UWeaponLabelWidget::IsFlavorShown() const
{
	const ESlateVisibility Shown = FlavorText ? FlavorText->GetVisibility() : ESlateVisibility::Collapsed;
	return Shown != ESlateVisibility::Collapsed && Shown != ESlateVisibility::Hidden && !FlavorText->GetText().IsEmpty();
}

FText UWeaponLabelWidget::GetCurseText() const
{
	return CurseText ? CurseText->GetText() : FText::GetEmpty();
}

bool UWeaponLabelWidget::IsCurseShown() const
{
	const ESlateVisibility Shown = CurseRow ? CurseRow->GetVisibility() : ESlateVisibility::Collapsed;
	return Shown != ESlateVisibility::Collapsed && Shown != ESlateVisibility::Hidden && !CurseText->GetText().IsEmpty();
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
	StyleFloatingText(NameText, bIsFocused ? 17 : 14, NameColor, 80);

	// A cursed iron: the coin and the curse's name under the name, from afar as up close (it is the warning), the coin
	// a size up when the label opens.
	CurseText->SetText(Curse);
	StyleFloatingText(CurseText, bIsFocused ? 13 : 12, Color::Curse(), 80);
	const float CoinSize = bIsFocused ? 19.f : 16.f;
	CurseGlyph->SetBrush(CrackedCoinBrush(FVector2D(CoinSize, CoinSize)));
	CurseRow->SetVisibility(Curse.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);

	// A named gun's line under its name, with the details: the name alone floats over the loot from afar.
	FlavorText->SetText(Flavor);
	StyleFloatingText(FlavorText, 12, LooterWeaponText::FlavorColor(), 20);
	FlavorText->SetVisibility(bIsFocused && !Flavor.IsEmpty() ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);

	StatsText->SetText(Stats);
	StyleFloatingText(StatsText, 11, Color::TextDim(), 60);
	StatsText->SetVisibility(Detail);

	const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	const UKeyBindingSubsystem* Bindings = LocalPlayer ? LocalPlayer->GetSubsystem<UKeyBindingSubsystem>() : nullptr;
	const FString Key = Bindings ? Bindings->GetKey(TEXT("Interact")).GetDisplayName().ToString().ToUpper() : FString(TEXT("E"));
	PromptText->SetText(FText::FromString(FString::Printf(TEXT("[%s] PICK UP  |  HOLD EQUIP"), *Key)));
	StyleFloatingText(PromptText, 13, Color::Accent(), 150);
	PromptText->SetVisibility(Detail);
}

#include "UI/World/CreatureBarkWidget.h"
#include "UI/Style/LooterUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

TSharedRef<SWidget> UCreatureBarkWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		// No plate: the words float outlined over the world, as the HUD's do, and wrap to a few words a line.
		UVerticalBox* Root = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Root"));
		WidgetTree->RootWidget = Root;
		LineText = LooterUI::MakeFloatingText(WidgetTree, SaidSize, LooterUI::Color::Text(), 0, ETextJustify::Center);
		LineText->SetAutoWrapText(true);
		USizeBox* Wrap = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("Wrap"));
		Wrap->SetMaxDesiredWidth(WrapWidth);
		Wrap->AddChild(LineText);
		UVerticalBoxSlot* WrapSlot = Root->AddChildToVerticalBox(Wrap);
		WrapSlot->SetHorizontalAlignment(HAlign_Center);
		// The tag (name, level, bar) floats at the same spot: the words sit just over it.
		WrapSlot->SetPadding(FMargin(0.f, 0.f, 0.f, TagClearance));
		Root->SetVisibility(ESlateVisibility::HitTestInvisible);
		ApplyLine();
	}
	return Super::RebuildWidget();
}

void UCreatureBarkWidget::SetLine(const FText& Line, bool bMuttered)
{
	Shown = Line;
	bShownMuttered = bMuttered;
	ApplyLine();
}

void UCreatureBarkWidget::ApplyLine()
{
	if (!LineText)
	{
		return;
	}
	LineText->SetText(Shown);
	// A mutter to itself reads as one: smaller and dimmer than words said at the player.
	LooterUI::StyleFloatingText(LineText, bShownMuttered ? MutteredSize : SaidSize,
		bShownMuttered ? LooterUI::Color::TextDim() : LooterUI::Color::Text(), 0, ETextJustify::Center);
}

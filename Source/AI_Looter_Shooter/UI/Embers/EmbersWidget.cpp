// UEmbersWidget: the ember powers page (the inventory's second): opening, the layout and the power cards. The keys, the
// mouse and equipping are in EmbersWidgetInput.cpp.

#include "UI/Embers/EmbersWidget.h"
#include "Embers/EmberComponent.h"
#include "Embers/EmberDefinition.h"
#include "Bestiary/Ledger.h"
#include "Embers/HeroDefinition.h"
#include "Settings/KeyBindingSubsystem.h"
#include "UI/HUD/LooterHUD.h"
#include "UI/Inventory/LoadoutParts.h"
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
#include "Engine/LocalPlayer.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

using namespace LooterUI;
using namespace LoadoutParts;

namespace
{
	const FName ActionPower(TEXT("Ember"));

	/** The three cards side by side, centred on the page under the header. */
	constexpr float CardWidth = 440.f;
	constexpr float CardHeight = 560.f;
	constexpr float CardGap = 40.f;
	constexpr float CardsTop = 200.f;
	/** The medallion on each card: the disc, the icon in it, the glow behind the equipped one. */
	constexpr float MedallionSize = 132.f;
	constexpr float MedallionIcon = 92.f;
	constexpr float MedallionGlow = 220.f;
	/** The line for what just happened, under the cards, and how long it stays (fading over the last half second). */
	constexpr float MessageY = 790.f;
	constexpr float MessageSeconds = 3.f;

	FLinearColor WithAlpha(FLinearColor Tone, float Alpha)
	{
		Tone.A = Alpha;
		return Tone;
	}
}

// ---------------------------------------------------------------------------
// Opening and closing
// ---------------------------------------------------------------------------

void UEmbersWidget::Open(ALooterHUD* InHUD)
{
	OwningHUD = InHUD;
	SetIsFocusable(true);
	MessageTime = 0.f;
	if (MessageText)
	{
		MessageText->SetRenderOpacity(0.f);
	}
	Gather();
	RebuildCards();
	Restyle();
	RefreshStates();
	RefreshPrompts();
}

void UEmbersWidget::Close()
{
	if (ALooterHUD* HUD = OwningHUD.Get())
	{
		HUD->CloseInventory();
	}
}

UEmberComponent* UEmbersWidget::GetEmbers() const
{
	if (UEmberComponent* Followed = Embers.Get())
	{
		return Followed;
	}
	const APlayerController* PC = GetOwningPlayer();
	return UEmberComponent::Find(PC ? PC->GetPawn() : nullptr);
}

void UEmbersWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	// A swap made elsewhere (the console) restyles the cards; the states and their seconds follow the clock.
	const UEmberComponent* Component = GetEmbers();
	if (Component && Component->GetEquippedIndex() != ShownEquipped)
	{
		Restyle();
	}
	RefreshStates();
	if (MessageTime > 0.f && MessageText)
	{
		MessageTime = FMath::Max(MessageTime - InDeltaTime, 0.f);
		MessageText->SetRenderOpacity(FMath::Clamp(MessageTime / 0.5f, 0.f, 1.f));
	}
}

// ---------------------------------------------------------------------------
// Layout
// ---------------------------------------------------------------------------

TSharedRef<SWidget> UEmbersWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
		WidgetTree->RootWidget = Root;

		// The world stays in view behind the page, dimmed, as behind the other pages.
		UImage* Backdrop = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Backdrop"));
		Backdrop->SetBrush(RectBrush(Colors::Dim()));
		MarkBackground(Backdrop);
		FillOverlaySlot(Root->AddChildToOverlay(Backdrop));

		UScaleBox* Scale = WidgetTree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass());
		Scale->SetStretch(EStretch::ScaleToFit);
		FillOverlaySlot(Root->AddChildToOverlay(Scale));
		UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Page"));
		Scale->SetContent(MakeSized(WidgetTree, Canvas, PageSize.X, PageSize.Y));

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

		// Title tabs: this page second.
		TArray<ULooterButton*> Tabs;
		Place(MakePageTabs(WidgetTree, static_cast<int32>(EInventoryPage::Embers), Tabs), PageTabsPosition, FVector2D::ZeroVector, FVector2D(0.5f, 0.f));
		for (ULooterButton* Tab : Tabs)
		{
			Tab->OnButtonClicked.BindUObject(this, &UEmbersWidget::HandleTabClicked);
		}

		// The header: whose powers, and how they work.
		{
			UHorizontalBox* Header = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
			HeroTitle = Label(WidgetTree, TEXT("Embers"), 10, Color::Accent(), 300);
			Header->AddChildToHorizontalBox(HeroTitle)->SetVerticalAlignment(VAlign_Center);
			HowTo = MakeText(WidgetTree, TEXT(""), 10, Color::TextDim(), false, 60);
			UHorizontalBoxSlot* HowSlot = Header->AddChildToHorizontalBox(HowTo);
			HowSlot->SetVerticalAlignment(VAlign_Center);
			HowSlot->SetPadding(FMargin(14.f, 0.f, 0.f, 0.f));
			const float RowWidth = CardWidth * 3.f + CardGap * 2.f;
			Place(Header, FVector2D((PageSize.X - RowWidth) * 0.5f, ColumnTop), FVector2D::ZeroVector);
		}

		// The cards, side by side (RebuildCards fills the row), and what to say with none.
		CardRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		Place(CardRow, FVector2D(PageSize.X * 0.5f, CardsTop), FVector2D::ZeroVector, FVector2D(0.5f, 0.f));
		EmptyText = Label(WidgetTree, TEXT("No embers yet"), 12, Color::TextDim(), 200);
		EmptyText->SetVisibility(ESlateVisibility::Collapsed);
		Place(EmptyText, FVector2D(PageSize.X * 0.5f, CardsTop + CardHeight * 0.4f), FVector2D::ZeroVector, FVector2D(0.5f, 0.f));

		// What just happened (equipped, or refused), under the cards.
		MessageText = MakeText(WidgetTree, TEXT(""), 12, Color::Text(), true, 150);
		MessageText->SetRenderOpacity(0.f);
		Place(MessageText, FVector2D(PageSize.X * 0.5f, MessageY), FVector2D::ZeroVector, FVector2D(0.5f, 0.f));

		// Bottom: what the keys do.
		PromptBar = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		Place(PromptBar, FVector2D(800.f, 852.f), FVector2D::ZeroVector, FVector2D(0.5f, 0.f));

		// Opened before it was first shown: the contents can go in now.
		RebuildCards();
		Restyle();
		RefreshStates();
		RefreshPrompts();
	}
	return Super::RebuildWidget();
}

// ---------------------------------------------------------------------------
// The cards
// ---------------------------------------------------------------------------

void UEmbersWidget::Gather()
{
	// Read every time the page opens: another pawn (a respawn) has its own component.
	UEmberComponent* Component = nullptr;
	const APlayerController* PC = GetOwningPlayer();
	Component = UEmberComponent::Find(PC ? PC->GetPawn() : nullptr);
	Embers = Component;
	Powers.Reset();
	if (Component)
	{
		// Kept index for index with the hero's list, as EquipIndex counts (a missing one gets no card).
		Powers = Component->GetOwned();
	}
	// The choice starts on the equipped power.
	Selected = Component ? FMath::Max(Component->GetEquippedIndex(), 0) : 0;
}

void UEmbersWidget::RebuildCards()
{
	if (!CardRow)
	{
		return;
	}
	CardRow->ClearChildren();
	Cards.Reset();
	for (int32 Index = 0; Index < Powers.Num(); ++Index)
	{
		if (!Powers[Index])
		{
			Cards.AddDefaulted();
			continue;
		}
		FCard Card = MakePowerCard(Index, *Powers[Index]);
		UHorizontalBoxSlot* CardSlot = CardRow->AddChildToHorizontalBox(Card.Button);
		CardSlot->SetPadding(FMargin(CardRow->GetChildrenCount() > 1 ? CardGap : 0.f, 0.f, 0.f, 0.f));
		Cards.Add(Card);
	}
	EmptyText->SetVisibility(Powers.IsEmpty() ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);

	// The header: the hero's name, and the key that fires the equipped power.
	const UEmberComponent* Component = GetEmbers();
	const UHeroDefinition* Hero = Component ? Component->GetHero() : nullptr;
	const FString HeroName = Hero && !Hero->DisplayName.IsEmpty() ? Hero->DisplayName.ToString() : FString();
	HeroTitle->SetText(FText::FromString((HeroName.IsEmpty() ? FString(TEXT("Embers")) : FString::Printf(TEXT("%s's embers"), *HeroName)).ToUpper()));
	const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	const UKeyBindingSubsystem* Bindings = LocalPlayer ? LocalPlayer->GetSubsystem<UKeyBindingSubsystem>() : nullptr;
	const FString Key = Bindings ? Bindings->GetKey(UKeyBindingSubsystem::EmberBindingId()).GetDisplayName().ToString() : FString(TEXT("Q"));
	HowTo->SetText(FText::FromString(FString::Printf(TEXT("All three are yours. One is equipped at a time: [%s] fires it in the field."), *Key)));
}

UEmbersWidget::FCard UEmbersWidget::MakePowerCard(int32 Index, const UEmberDefinition& Power)
{
	FCard Card;
	const FLinearColor Tone = Power.Color;
	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());

	// The top line: its nature as a chip in its colour, and the EQUIPPED tag on the right.
	{
		UHorizontalBox* Top = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		UBorder* Chip = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		Chip->SetBrush(RectBrush(Color::Plate(), WithAlpha(Tone, 0.7f), 1.f));
		MarkBackground(Chip);
		Chip->SetPadding(FMargin(8.f, 2.f));
		Chip->SetContent(Label(WidgetTree, Power.Nature.ToString(), 8, Tone, 200));
		Top->AddChildToHorizontalBox(Chip)->SetVerticalAlignment(VAlign_Center);
		UHorizontalBoxSlot* Spacer = Top->AddChildToHorizontalBox(MakeSized(WidgetTree, nullptr, 0.f, 1.f));
		Spacer->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		UBorder* Tag = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		Tag->SetBrush(RectBrush(Color::Accent()));
		Tag->SetPadding(FMargin(8.f, 2.f));
		Tag->SetContent(Label(WidgetTree, TEXT("Equipped"), 8, Color::AccentDark(), 200));
		Top->AddChildToHorizontalBox(Tag)->SetVerticalAlignment(VAlign_Center);
		Card.EquippedTag = Tag;
		Column->AddChildToVerticalBox(Top);
	}

	// The medallion: a glow in its colour behind the equipped one, a dark disc ringed in its colour, its Inked icon.
	{
		UOverlay* Medallion = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		auto AddCentred = [Medallion](UWidget* Child)
		{
			UOverlaySlot* ChildSlot = Medallion->AddChildToOverlay(Child);
			ChildSlot->SetHorizontalAlignment(HAlign_Center);
			ChildSlot->SetVerticalAlignment(VAlign_Center);
		};
		Card.Glow = MakeImage(WidgetTree, GlowBrush(FVector2D(MedallionSize, MedallionSize), WithAlpha(Tone, 0.55f)));
		Card.Glow->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		Card.Glow->SetRenderScale(FVector2D(MedallionGlow / MedallionSize));
		AddCentred(Card.Glow);
		UImage* Disc = MakeImage(WidgetTree, CircleBrush(WithAlpha(Color::Track(), 0.85f)));
		MarkBackground(Disc);
		AddCentred(MakeSized(WidgetTree, Disc, MedallionSize, MedallionSize));
		AddCentred(MakeSized(WidgetTree, MakeImage(WidgetTree, CircleBrush(FLinearColor::Transparent, WithAlpha(Tone, 0.9f), 2.5f)),
			MedallionSize, MedallionSize));
		if (const FInkedIcon* Inked = FindInkedIcon(Power.IconName))
		{
			AddCentred(MakeImage(WidgetTree, InkedIconBrush(Power.IconName, *Inked, FVector2D(MedallionIcon, MedallionIcon))));
		}
		UVerticalBoxSlot* MedallionSlot = Column->AddChildToVerticalBox(MakeSized(WidgetTree, Medallion, MedallionSize + 40.f, MedallionSize + 30.f));
		MedallionSlot->SetHorizontalAlignment(HAlign_Center);
		MedallionSlot->SetPadding(FMargin(0.f, 8.f, 0.f, 4.f));
	}

	// Its name, and what it does.
	UTextBlock* Name = Label(WidgetTree, Power.DisplayName.ToString(), 20, Color::Title(), 150);
	Column->AddChildToVerticalBox(Name)->SetHorizontalAlignment(HAlign_Center);
	UTextBlock* Words = MakeText(WidgetTree, Power.Description.ToString(), 11, Color::Text());
	Words->SetAutoWrapText(true);
	Words->SetJustification(ETextJustify::Center);
	UVerticalBoxSlot* WordsSlot = Column->AddChildToVerticalBox(Words);
	WordsSlot->SetPadding(FMargin(4.f, 12.f, 4.f, 0.f));
	WordsSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	// Its clock: the cooldown and the duration, on a hairline.
	Column->AddChildToVerticalBox(MakeSized(WidgetTree, MakeImage(WidgetTree, RectBrush(Color::RowLine())), 0.f, 1.f))
		->SetPadding(FMargin(0.f, 10.f, 0.f, 8.f));
	auto AddClock = [this, Column](const TCHAR* What, const FString& Value)
	{
		UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		UHorizontalBoxSlot* WhatSlot = Line->AddChildToHorizontalBox(Label(WidgetTree, What, 9, Color::TextDim(), 200));
		WhatSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		WhatSlot->SetVerticalAlignment(VAlign_Center);
		Line->AddChildToHorizontalBox(MakeText(WidgetTree, Value, 13, Color::Text(), false, 40))->SetVerticalAlignment(VAlign_Center);
		Column->AddChildToVerticalBox(Line)->SetPadding(FMargin(0.f, 2.f));
	};
	AddClock(TEXT("Cooldown"), SecondsWords(Power.CooldownSeconds, false));
	AddClock(TEXT("Duration"), SecondsWords(Power.DurationSeconds, true));

	// How it stands (the equipped one: ready, running, cooling).
	Card.State = Label(WidgetTree, TEXT(""), 9, Color::TextDim(), 200);
	UVerticalBoxSlot* StateSlot = Column->AddChildToVerticalBox(Card.State);
	StateSlot->SetHorizontalAlignment(HAlign_Center);
	StateSlot->SetPadding(FMargin(0.f, 10.f, 0.f, 0.f));

	UOverlay* Box = MakeCard(WidgetTree, Column, FMargin(22.f, 18.f), 2.f, Card.Fill, Card.Line);
	Card.Button = WidgetTree->ConstructWidget<ULooterButton>(ULooterButton::StaticClass());
	// The page plays its own sounds: a hover as the choice moves, the equip's clack or a refusal on a click.
	Card.Button->bPlaysSounds = false;
	Card.Button->SetupContent(MakeSized(WidgetTree, Box, CardWidth, CardHeight), ActionPower, Index);
	Card.Button->SetCursor(EMouseCursor::Hand);
	Card.Button->OnButtonClicked.BindUObject(this, &UEmbersWidget::HandleCardClicked);
	Card.Button->OnButtonHovered.BindUObject(this, &UEmbersWidget::HandleCardHovered);
	return Card;
}

void UEmbersWidget::Restyle()
{
	const UEmberComponent* Component = GetEmbers();
	const int32 Equipped = Component ? Component->GetEquippedIndex() : INDEX_NONE;
	ShownEquipped = Equipped;
	for (int32 Index = 0; Index < Cards.Num(); ++Index)
	{
		const FCard& Card = Cards[Index];
		if (!Card.Button)
		{
			continue;
		}
		const bool bSelected = Index == Selected;
		const bool bEquipped = Index == Equipped;
		const FLinearColor Tone = Powers.IsValidIndex(Index) && Powers[Index] ? Powers[Index]->Color : Color::Text();
		Card.Fill->SetColorAndOpacity(bSelected ? Color::Tile() : Colors::CardFill());
		// The chosen card's edge is orange; the equipped one's, chosen or not, is lit in its colour otherwise.
		Card.Line->SetColorAndOpacity(bSelected ? Color::Accent() : (bEquipped ? WithAlpha(Tone, 0.8f) : Colors::CardLine()));
		Card.EquippedTag->SetVisibility(bEquipped ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
		Card.Glow->SetRenderOpacity(bEquipped ? 1.f : 0.f);
	}
}

void UEmbersWidget::RefreshStates()
{
	const UEmberComponent* Component = GetEmbers();
	const int32 Equipped = Component ? Component->GetEquippedIndex() : INDEX_NONE;
	for (int32 Index = 0; Index < Cards.Num(); ++Index)
	{
		FCard& Card = Cards[Index];
		if (!Card.Button)
		{
			continue;
		}
		FString Words;
		FLinearColor Tone = Color::TextDim();
		if (Index == Equipped && Component)
		{
			switch (Component->GetState())
			{
			case EEmberState::Ready:
				Words = TEXT("Ready");
				Tone = Color::Better();
				break;
			case EEmberState::Active:
				Words = FString::Printf(TEXT("Running, %d s left"), FMath::Max(1, FMath::CeilToInt(Component->GetActiveLeft())));
				Tone = Powers.IsValidIndex(Index) && Powers[Index] ? Powers[Index]->Color : Color::Text();
				break;
			case EEmberState::Cooldown:
				Words = FString::Printf(TEXT("Cooling down, %d s"), FMath::Max(1, FMath::CeilToInt(Component->GetCooldownLeft())));
				break;
			}
		}
		else
		{
			Words = TEXT("Choose to equip");
		}
		if (Words != Card.StateWords)
		{
			Card.StateWords = Words;
			Card.State->SetText(FText::FromString(Words.ToUpper()));
			Card.State->SetColorAndOpacity(FSlateColor(Tone));
		}
	}
}

void UEmbersWidget::RefreshPrompts()
{
	if (!PromptBar)
	{
		return;
	}
	PromptBar->ClearChildren();
	const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	const UKeyBindingSubsystem* Bindings = LocalPlayer ? LocalPlayer->GetSubsystem<UKeyBindingSubsystem>() : nullptr;
	struct FPrompt
	{
		FString Key;
		FString Text;
	};
	TArray<FPrompt, TInlineAllocator<6>> Prompts;
	Prompts.Add({ TEXT("A / D"), TEXT("Browse") });
	if (!Powers.IsEmpty())
	{
		Prompts.Add({ TEXT("E"), TEXT("Equip") });
	}
	Prompts.Add({ TEXT("1"), TEXT("Loadout") });
	Prompts.Add({ TEXT("3"), PageTabName(static_cast<int32>(EInventoryPage::Bestiary), Ledger::IsOpenIn(this)) });
	Prompts.Add({ TEXT("4"), TEXT("Missions") });
	Prompts.Add({ Bindings ? Bindings->GetKey(TEXT("Inventory")).GetDisplayName().ToString() : FString(TEXT("Tab")), TEXT("Close") });
	for (int32 Index = 0; Index < Prompts.Num(); ++Index)
	{
		PromptBar->AddChildToHorizontalBox(MakeKeyHint(WidgetTree, Prompts[Index].Key, Prompts[Index].Text, Index == 0))
			->SetPadding(FMargin(Index > 0 ? 26.f : 0.f, 0.f, 0.f, 0.f));
	}
}

void UEmbersWidget::ShowMessage(const FString& Words, const FLinearColor& Tone)
{
	if (!MessageText)
	{
		return;
	}
	MessageText->SetText(FText::FromString(Words.ToUpper()));
	MessageText->SetColorAndOpacity(FSlateColor(Tone));
	MessageText->SetRenderOpacity(1.f);
	MessageTime = MessageSeconds;
}

FString UEmbersWidget::SecondsWords(float Seconds, bool bDuration)
{
	if (bDuration && Seconds <= 0.f)
	{
		return TEXT("Instant");
	}
	const float Rounded = FMath::RoundToFloat(Seconds * 10.f) / 10.f;
	return FMath::IsNearlyEqual(Rounded, FMath::RoundToFloat(Rounded))
		? FString::Printf(TEXT("%d s"), FMath::RoundToInt(Rounded))
		: FString::Printf(TEXT("%.1f s"), Rounded);
}

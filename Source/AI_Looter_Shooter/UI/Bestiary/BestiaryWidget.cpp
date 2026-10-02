#include "UI/Bestiary/BestiaryWidget.h"
#include "AI_Looter_Shooter.h"
#include "Bestiary/BestiaryEntry.h"
#include "Progression/PlayerProgressionSubsystem.h"
#include "Settings/KeyBindingSubsystem.h"
#include "UI/Bestiary/BestiaryStage.h"
#include "UI/HUD/LooterHUD.h"
#include "UI/Inventory/LoadoutPaintLayer.h"
#include "UI/Inventory/LoadoutParts.h"
#include "UI/Style/LooterButton.h"
#include "UI/Style/LooterUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/ScaleBox.h"
#include "Components/ScrollBox.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/LocalPlayer.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

using namespace LooterUI;
using namespace LoadoutParts;

namespace
{
	const FName ActionEntry(TEXT("Entry"));

	/** The stand's brightness for something not met yet: a near-black silhouette with just a hint of its shape. */
	constexpr float SilhouetteExposure = 0.02f;
	const TCHAR* const Unknown = TEXT("???");

	constexpr float ColumnHeight = 690.f;

	const EBestiaryCategory Sections[] = { EBestiaryCategory::Creature, EBestiaryCategory::Enemy, EBestiaryCategory::NPC, EBestiaryCategory::Friend };

	/** "Creature", "Enemy", "NPC", "Friend": the details header's word for one entry. */
	FString CategoryWord(EBestiaryCategory Category)
	{
		switch (Category)
		{
		case EBestiaryCategory::Creature: return TEXT("Creature");
		case EBestiaryCategory::Enemy:    return TEXT("Enemy");
		case EBestiaryCategory::NPC:      return TEXT("NPC");
		case EBestiaryCategory::Friend:   return TEXT("Friend");
		}
		return FString();
	}

	FString FormatWhole(float Value)
	{
		return FText::AsNumber(FMath::RoundToInt(Value)).ToString();
	}
}

// ---------------------------------------------------------------------------
// Opening and closing
// ---------------------------------------------------------------------------

void UBestiaryWidget::Open(ALooterHUD* InHUD)
{
	OwningHUD = InHUD;
	SetIsFocusable(true);

	// Read every time, so an entry added in the editor shows up the next time the page opens.
	const UBestiaryEntry* Previous = GetSelected();
	Entries.Reset();
	for (UBestiaryEntry* Entry : UBestiaryEntry::LoadAll())
	{
		Entries.Add(Entry);
	}
	Selected = FMath::Max(Entries.IndexOfByKey(Previous), 0);

	// The stand is spawned the first time, then kept, idle while the page is closed.
	UWorld* World = GetWorld();
	if (!Stage.IsValid() && World)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Params.ObjectFlags |= RF_Transient;
		Stage = World->SpawnActor<ABestiaryStage>(Params);
	}
	if (ABestiaryStage* StagePtr = Stage.Get())
	{
		if (!StageMaterial)
		{
			// The loadout's picture material: it cuts the model out of the capture and tone maps it.
			if (UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, StageMaterialPath))
			{
				StageMaterial = UMaterialInstanceDynamic::Create(Base, this);
			}
			else
			{
				UE_LOG(LogLooter, Warning, TEXT("Bestiary: %s is missing, so models can't be shown."), StageMaterialPath);
			}
		}
		if (StageMaterial)
		{
			StageMaterial->SetTextureParameterValue(TEXT("Capture"), StagePtr->GetRenderTarget());
		}
		ApplyStageBrush();
		StagePtr->ShowEntry(GetSelected());
		StagePtr->SetActive(true);
	}

	RebuildList();
	Restyle();
	RefreshDetails();
	RefreshPrompts();
}

void UBestiaryWidget::Close()
{
	if (ALooterHUD* HUD = OwningHUD.Get())
	{
		HUD->CloseInventory();
	}
}

void UBestiaryWidget::GoToLoadout()
{
	if (ALooterHUD* HUD = OwningHUD.Get())
	{
		HUD->ShowInventoryPage(EInventoryPage::Loadout);
	}
}

void UBestiaryWidget::NativeDestruct()
{
	// Off screen: the stand stops animating and rendering.
	bDragging = false;
	TurnInput = 0.f;
	if (ABestiaryStage* StagePtr = Stage.Get())
	{
		StagePtr->SetActive(false);
	}
	Super::NativeDestruct();
}

void UBestiaryWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	ABestiaryStage* StagePtr = Stage.Get();
	if (!StagePtr)
	{
		return;
	}
	if (!FMath::IsNearlyZero(TurnInput))
	{
		StagePtr->AddTurn(TurnInput * StickTurnRate * InDeltaTime);
	}
	if (StageMaterial)
	{
		// Something not met yet stands as a dark silhouette: the picture's brightness all but off, its outline kept.
		StageMaterial->SetScalarParameterValue(TEXT("Exposure"), bSelectedKnown ? StagePtr->GetExposure() : SilhouetteExposure);
	}
}

// ---------------------------------------------------------------------------
// Layout
// ---------------------------------------------------------------------------

TSharedRef<SWidget> UBestiaryWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
		WidgetTree->RootWidget = Root;

		// The world stays in view behind the page, dimmed, as behind the loadout.
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

		// The stand: the far half of its ring, the picture, then the near half.
		BackLayer = WidgetTree->ConstructWidget<ULoadoutPaintLayer>(ULoadoutPaintLayer::StaticClass());
		BackLayer->SetPainter([this](const FGeometry& Geometry, FSlateWindowElementList& Elements, int32 LayerId) { PaintBack(Geometry, Elements, LayerId); });
		BackLayer->SetVisibility(ESlateVisibility::HitTestInvisible);
		Place(BackLayer, FVector2D::ZeroVector, PageSize);

		StageImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Stage"));
		StageImage->SetBrush(RectBrush(FLinearColor::Transparent));
		StageImage->SetCursor(EMouseCursor::GrabHand);
		Place(StageImage, BestiaryLayout::StageTopLeft, BestiaryLayout::StageSize);

		FrontLayer = WidgetTree->ConstructWidget<ULoadoutPaintLayer>(ULoadoutPaintLayer::StaticClass());
		FrontLayer->SetPainter([this](const FGeometry& Geometry, FSlateWindowElementList& Elements, int32 LayerId) { PaintFront(Geometry, Elements, LayerId); });
		FrontLayer->SetVisibility(ESlateVisibility::HitTestInvisible);
		Place(FrontLayer, FVector2D::ZeroVector, PageSize);

		StageHint = Label(WidgetTree, TEXT("Drag to turn"), 8, Hex(143, 179, 204, 204), 220);
		Place(StageHint, FVector2D(800.f, BestiaryLayout::StageTopLeft.Y + BestiaryLayout::StageSize.Y + 14.f), FVector2D::ZeroVector, FVector2D(0.5f, 0.f));

		// Title tabs: the loadout and this page.
		TArray<ULooterButton*> Tabs;
		Place(MakePageTabs(WidgetTree, static_cast<int32>(EInventoryPage::Bestiary), Tabs), PageTabsPosition, FVector2D::ZeroVector, FVector2D(0.5f, 0.f));
		for (ULooterButton* Tab : Tabs)
		{
			Tab->OnButtonClicked.BindUObject(this, &UBestiaryWidget::HandleTabClicked);
		}

		// Left: every entry, by section.
		{
			UVerticalBox* Left = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
			UHorizontalBox* Header = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
			Header->AddChildToHorizontalBox(Label(WidgetTree, TEXT("Field guide"), 10, Color::Accent(), 300));
			ListCount = Label(WidgetTree, TEXT(""), 10, Color::TextDim(), 200);
			Header->AddChildToHorizontalBox(ListCount)->SetPadding(FMargin(10.f, 0.f, 0.f, 0.f));
			Left->AddChildToVerticalBox(Header);
			ListBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass());
			StyleScrollBox(ListBox);
			UVerticalBoxSlot* ListSlot = Left->AddChildToVerticalBox(ListBox);
			ListSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			ListSlot->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));
			Place(Left, FVector2D(LeftX, ColumnTop), FVector2D(LeftWidth, ColumnHeight));
		}

		// Right: the chosen entry, then its description and field notes.
		{
			UVerticalBox* Right = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
			DetailsHeader = Label(WidgetTree, TEXT(""), 10, Color::Accent(), 300);
			Right->AddChildToVerticalBox(DetailsHeader);
			DetailsBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
			UImage* DetailsFill = nullptr;
			UImage* DetailsLine = nullptr;
			UOverlay* DetailsCard = MakeCard(WidgetTree, DetailsBox, FMargin(16.f, 14.f), 1.7f, DetailsFill, DetailsLine);
			DetailsFill->SetColorAndOpacity(Colors::CardFill());
			DetailsLine->SetColorAndOpacity(Hex(90, 200, 255, 102));
			Right->AddChildToVerticalBox(DetailsCard)->SetPadding(FMargin(0.f, 8.f, 0.f, 0.f));

			UScrollBox* NotesScroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass());
			StyleScrollBox(NotesScroll);
			NotesBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
			NotesScroll->AddChild(NotesBox);
			UVerticalBoxSlot* NotesSlot = Right->AddChildToVerticalBox(NotesScroll);
			NotesSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			NotesSlot->SetPadding(FMargin(0.f, 18.f, 0.f, 0.f));
			Place(Right, FVector2D(RightX, ColumnTop), FVector2D(RightWidth, ColumnHeight));
		}

		// Bottom: what the keys do.
		PromptBar = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		Place(PromptBar, FVector2D(800.f, 852.f), FVector2D::ZeroVector, FVector2D(0.5f, 0.f));

		DiscBrush = IconBrush(TEXT("LoadoutDisc"), DiscIcon(), 2.f, FVector2D(64.f, 64.f), FLinearColor::White);

		// Opened before it was first shown: the picture and contents can go in now.
		ApplyStageBrush();
		RebuildList();
		Restyle();
		RefreshDetails();
		RefreshPrompts();
	}
	return Super::RebuildWidget();
}

void UBestiaryWidget::ApplyStageBrush()
{
	if (StageImage && StageMaterial)
	{
		FSlateBrush Brush;
		Brush.SetResourceObject(StageMaterial);
		Brush.ImageSize = BestiaryLayout::StageSize;
		StageImage->SetBrush(Brush);
	}
}

// ---------------------------------------------------------------------------
// Contents
// ---------------------------------------------------------------------------

void UBestiaryWidget::RebuildList()
{
	if (!ListBox)
	{
		return;
	}
	ListBox->ClearChildren();
	Cards.Reset();
	Cards.SetNum(Entries.Num());
	ListCount->SetText(FText::FromString(FString::Printf(TEXT("%d %s"), Entries.Num(), Entries.Num() == 1 ? TEXT("entry") : TEXT("entries")).ToUpper()));

	// Every section is listed, empty or not, so the guide shows what's still out there to meet.
	for (int32 SectionIndex = 0; SectionIndex < UE_ARRAY_COUNT(Sections); ++SectionIndex)
	{
		const EBestiaryCategory Section = Sections[SectionIndex];
		int32 InSection = 0;
		for (const UBestiaryEntry* Entry : Entries)
		{
			InSection += Entry->Category == Section ? 1 : 0;
		}

		UHorizontalBox* Head = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		Head->AddChildToHorizontalBox(Label(WidgetTree, UBestiaryEntry::CategoryName(Section).ToString(), 8, Color::TextDim(), 220))
			->SetVerticalAlignment(VAlign_Center);
		UHorizontalBoxSlot* RuleSlot = Head->AddChildToHorizontalBox(MakeSized(WidgetTree, MakeImage(WidgetTree, RectBrush(Hex(90, 200, 255, 61))), 0.f, 1.f));
		RuleSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		RuleSlot->SetVerticalAlignment(VAlign_Center);
		RuleSlot->SetPadding(FMargin(10.f, 0.f));
		Head->AddChildToHorizontalBox(Label(WidgetTree, FString::FromInt(InSection), 8, Color::TextDim(), 150))->SetVerticalAlignment(VAlign_Center);
		Cast<UScrollBoxSlot>(ListBox->AddChild(Head))->SetPadding(FMargin(0.f, SectionIndex > 0 ? 16.f : 4.f, 6.f, 2.f));

		if (InSection == 0)
		{
			Cast<UScrollBoxSlot>(ListBox->AddChild(Label(WidgetTree, TEXT("Nothing here yet"), 8, Hex(143, 179, 204, 140), 150)))
				->SetPadding(FMargin(2.f, 6.f, 6.f, 0.f));
			continue;
		}
		for (int32 Index = 0; Index < Entries.Num(); ++Index)
		{
			if (Entries[Index]->Category == Section)
			{
				Cards[Index] = MakeEntryCard(Index);
				Cast<UScrollBoxSlot>(ListBox->AddChild(Cards[Index].Button))->SetPadding(FMargin(0.f, 6.f, 6.f, 0.f));
			}
		}
	}
}

UBestiaryWidget::FCard UBestiaryWidget::MakeEntryCard(int32 Index)
{
	FCard Card;
	const UBestiaryEntry& Entry = *Entries[Index];
	const FBestiaryStats Stats = Entry.ReadStats();
	UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

	// The name over what it is and its level; "???" until the player has met one.
	const bool bKnown = IsKnown(Entry);
	UVerticalBox* Text = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Text->AddChildToVerticalBox(FittedLabel(WidgetTree, bKnown ? Entry.DisplayName.ToString() : FString(Unknown), 10,
		bKnown ? Color::Text() : Color::TextDim(), 50));
	FString Sub = bKnown ? Entry.Kind.ToString() : FString(TEXT("Not met yet"));
	if (bKnown && Stats.Level > 0)
	{
		Sub += FString::Printf(TEXT("%sLv %d"), Sub.IsEmpty() ? TEXT("") : TEXT(" · "), Stats.Level);
	}
	Text->AddChildToVerticalBox(FittedLabel(WidgetTree, Sub, 8, Color::TextDim(), 120))->SetPadding(FMargin(0.f, 2.f, 0.f, 0.f));
	UHorizontalBoxSlot* TextSlot = Line->AddChildToHorizontalBox(Text);
	TextSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	TextSlot->SetVerticalAlignment(VAlign_Center);
	TextSlot->SetPadding(FMargin(4.f, 0.f, 8.f, 0.f));

	// How many you've defeated, on the right.
	const int32 Defeated = GetDefeated(Entry);
	UVerticalBox* Count = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Count->AddChildToVerticalBox(Label(WidgetTree, FText::AsNumber(Defeated).ToString(), 12, Defeated > 0 ? Color::Text() : Color::TextDim(), 40))
		->SetHorizontalAlignment(HAlign_Right);
	Count->AddChildToVerticalBox(Label(WidgetTree, TEXT("Defeated"), 7, Color::TextDim(), 120))->SetHorizontalAlignment(HAlign_Right);
	Line->AddChildToHorizontalBox(Count)->SetVerticalAlignment(VAlign_Center);

	UOverlay* Box = MakeCard(WidgetTree, Line, FMargin(14.f, 7.f), 1.3f, Card.Fill, Card.Line);
	Card.Button = WidgetTree->ConstructWidget<ULooterButton>(ULooterButton::StaticClass());
	Card.Button->SetupContent(MakeSized(WidgetTree, Box, 0.f, ListCardHeight), ActionEntry, Index);
	Card.Button->OnButtonClicked.BindUObject(this, &UBestiaryWidget::HandleCardClicked);
	Card.Button->OnButtonHovered.BindUObject(this, &UBestiaryWidget::HandleCardHovered);
	return Card;
}

void UBestiaryWidget::Select(int32 Index, bool bScrollIntoView)
{
	if (!Entries.IsValidIndex(Index) || Index == Selected)
	{
		return;
	}
	Selected = Index;
	Restyle();
	RefreshDetails();
	if (ABestiaryStage* StagePtr = Stage.Get())
	{
		StagePtr->ShowEntry(GetSelected());
	}
	if (bScrollIntoView && Cards.IsValidIndex(Index) && Cards[Index].Button)
	{
		ListBox->ScrollWidgetIntoView(Cards[Index].Button, false, EDescendantScrollDestination::IntoView, 8.f);
	}
}

void UBestiaryWidget::Restyle()
{
	for (int32 Index = 0; Index < Cards.Num(); ++Index)
	{
		const FCard& Card = Cards[Index];
		if (Card.Button)
		{
			const bool bSelected = Index == Selected;
			Card.Fill->SetColorAndOpacity(bSelected ? Color::Tile() : Colors::CardFill());
			Card.Line->SetColorAndOpacity(bSelected ? Color::Accent() : Colors::CardLine());
		}
	}
}

void UBestiaryWidget::RefreshDetails()
{
	if (!DetailsBox)
	{
		return;
	}
	DetailsBox->ClearChildren();
	NotesBox->ClearChildren();
	const UBestiaryEntry* Entry = GetSelected();
	const ABestiaryStage* StagePtr = Stage.Get();
	StageHint->SetVisibility(StagePtr && StagePtr->HasModel() ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	if (!Entry)
	{
		DetailsHeader->SetText(FText::FromString(TEXT("BESTIARY")));
		DetailsBox->AddChildToVerticalBox(Label(WidgetTree, TEXT("No entries yet"), 17, Color::TextDim(), 40));
		UTextBlock* Help = MakeText(WidgetTree, TEXT("The creatures, enemies and people you meet are written up here."), 10, Color::TextDim());
		Help->SetAutoWrapText(true);
		DetailsBox->AddChildToVerticalBox(Help)->SetPadding(FMargin(0.f, 8.f, 0.f, 0.f));
		return;
	}

	// Not met yet: only its section is known, and the page says how to fill it in.
	const bool bKnown = IsKnown(*Entry);
	bSelectedKnown = bKnown;
	const FString Kind = bKnown ? Entry->Kind.ToString() : FString();
	DetailsHeader->SetText(FText::FromString((Kind.IsEmpty() ? CategoryWord(Entry->Category) : CategoryWord(Entry->Category) + TEXT(" · ") + Kind).ToUpper()));

	UTextBlock* NameText = Label(WidgetTree, bKnown ? Entry->DisplayName.ToString() : FString(Unknown), 17, bKnown ? Color::Text() : Color::TextDim(), 40);
	NameText->SetAutoWrapText(true);
	DetailsBox->AddChildToVerticalBox(NameText);
	const FString Habitat = bKnown ? Entry->Habitat.ToString() : FString(TEXT("Not met yet"));
	if (!Habitat.IsEmpty())
	{
		DetailsBox->AddChildToVerticalBox(Label(WidgetTree, Habitat, 9, Color::TextDim(), 140))->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));
	}

	// The numbers, read from the actor itself.
	const FBestiaryStats Stats = Entry->ReadStats();
	auto AddStat = [this](const TCHAR* StatName, const FString& Value, bool bLit)
	{
		UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		UHorizontalBoxSlot* NameSlot = Line->AddChildToHorizontalBox(Label(WidgetTree, StatName, 8, Color::TextDim(), 120));
		NameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		NameSlot->SetVerticalAlignment(VAlign_Center);
		UTextBlock* ValueText = MakeText(WidgetTree, Value, 11, bLit ? Color::Text() : Color::TextDim());
		ValueText->SetJustification(ETextJustify::Right);
		Line->AddChildToHorizontalBox(ValueText)->SetVerticalAlignment(VAlign_Center);
		DetailsBox->AddChildToVerticalBox(MakeSized(WidgetTree, Line, 0.f, 24.f))->SetPadding(FMargin(0.f, 2.f));
	};
	DetailsBox->AddChildToVerticalBox(MakeSized(WidgetTree, nullptr, 0.f, 8.f));
	if (!bKnown)
	{
		for (const TCHAR* StatName : { TEXT("Level"), TEXT("Health"), TEXT("Attack"), TEXT("Experience") })
		{
			AddStat(StatName, Unknown, false);
		}
		AddStat(TEXT("Defeated"), TEXT("0"), false);
		UTextBlock* Help = MakeText(WidgetTree, TEXT("You haven't met one yet. Find it out in the world, or let it find you, to fill in this page."),
			10, Color::TextDim());
		Help->SetAutoWrapText(true);
		NotesBox->AddChildToVerticalBox(Help);
		return;
	}
	if (Stats.Level > 0)
	{
		AddStat(TEXT("Level"), FString::FromInt(Stats.Level), true);
	}
	if (Stats.bHasHealth)
	{
		AddStat(TEXT("Health"), FormatWhole(Stats.Health), true);
	}
	AddStat(TEXT("Attack"), Stats.bAttacks ? FString::Printf(TEXT("%s dmg"), *FormatWhole(Stats.AttackDamage)) : FString(TEXT("Harmless")), Stats.bAttacks);
	AddStat(TEXT("Experience"), Stats.XPReward > 0 ? FString::Printf(TEXT("%d XP"), Stats.XPReward) : FString(TEXT("None")), Stats.XPReward > 0);
	const int32 Defeated = GetDefeated(*Entry);
	AddStat(TEXT("Defeated"), FText::AsNumber(Defeated).ToString(), Defeated > 0);

	// Under the card: the description, then the field notes.
	if (!Entry->Description.IsEmpty())
	{
		UTextBlock* Description = MakeText(WidgetTree, Entry->Description.ToString(), 10, Color::Text());
		Description->SetAutoWrapText(true);
		NotesBox->AddChildToVerticalBox(Description);
	}
	if (!Entry->Notes.IsEmpty())
	{
		NotesBox->AddChildToVerticalBox(Label(WidgetTree, TEXT("Field notes"), 10, Color::Accent(), 300))->SetPadding(FMargin(0.f, 18.f, 0.f, 4.f));
		for (const FText& Note : Entry->Notes)
		{
			UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
			UHorizontalBoxSlot* BulletSlot = Line->AddChildToHorizontalBox(MakeSized(WidgetTree, MakeImage(WidgetTree, RectBrush(Color::Accent())), 5.f, 5.f));
			BulletSlot->SetVerticalAlignment(VAlign_Top);
			BulletSlot->SetPadding(FMargin(2.f, 6.f, 10.f, 0.f));
			UTextBlock* NoteText = MakeText(WidgetTree, Note.ToString(), 10, Color::Text());
			NoteText->SetAutoWrapText(true);
			Line->AddChildToHorizontalBox(NoteText)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			NotesBox->AddChildToVerticalBox(Line)->SetPadding(FMargin(0.f, 4.f, 8.f, 0.f));
		}
	}
}

void UBestiaryWidget::RefreshPrompts()
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
		const TCHAR* Text;
	};
	const FPrompt Prompts[] = {
		{ TEXT("W / S"), TEXT("Browse") },
		{ TEXT("1"), TEXT("Loadout") },
		{ TEXT("3"), TEXT("Missions") },
		{ Bindings ? Bindings->GetKey(TEXT("Inventory")).GetDisplayName().ToString() : FString(TEXT("Tab")), TEXT("Close") },
	};
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Prompts); ++Index)
	{
		PromptBar->AddChildToHorizontalBox(MakeKeyHint(WidgetTree, Prompts[Index].Key, Prompts[Index].Text, Index == 0))
			->SetPadding(FMargin(Index > 0 ? 26.f : 0.f, 0.f, 0.f, 0.f));
	}
}

// ---------------------------------------------------------------------------
// Queries
// ---------------------------------------------------------------------------

const UBestiaryEntry* UBestiaryWidget::GetSelected() const
{
	return Entries.IsValidIndex(Selected) ? Entries[Selected].Get() : nullptr;
}

bool UBestiaryWidget::IsKnown(const UBestiaryEntry& Entry) const
{
	// A page about no actor in particular has nothing to meet, so it's always open.
	const UClass* ActorType = Entry.ActorClass.LoadSynchronous();
	const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	const UPlayerProgressionSubsystem* Progression = LocalPlayer ? LocalPlayer->GetSubsystem<UPlayerProgressionSubsystem>() : nullptr;
	return !ActorType || !Progression || Progression->HasEncountered(ActorType);
}

int32 UBestiaryWidget::GetDefeated(const UBestiaryEntry& Entry) const
{
	const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	const UPlayerProgressionSubsystem* Progression = LocalPlayer ? LocalPlayer->GetSubsystem<UPlayerProgressionSubsystem>() : nullptr;
	return Progression ? Progression->GetDefeated(Entry.ActorClass.LoadSynchronous()) : 0;
}

#include "BuildMode/BuildModeWidget.h"
#include "BuildMode/BuildModeComponent.h"
#include "Environment/EnvironmentPalette.h"
#include "UI/Style/LooterButton.h"
#include "UI/Style/LooterUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/UniformGridPanel.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

using namespace LooterUI;

namespace
{
	const FName ActionCategory(TEXT("Category"));
	const FName ActionEntry(TEXT("Entry"));
	const FName ActionRandomRotation(TEXT("RandomRotation"));
	const FName ActionRandomScale(TEXT("RandomScale"));
	const FName ActionAlign(TEXT("Align"));
	const FName ActionBrush(TEXT("Brush"));
	const FName ActionSave(TEXT("Save"));
	const FName ActionUndo(TEXT("Undo"));
	const FName ActionClear(TEXT("Clear"));

	FText OnOff(const TCHAR* Label, bool bOn)
	{
		return FText::FromString(FString::Printf(TEXT("%s: %s"), Label, bOn ? TEXT("ON") : TEXT("OFF")));
	}
}

void UBuildModeWidget::Init(UBuildModeComponent* InBuildMode)
{
	BuildMode = InBuildMode;
}

bool UBuildModeWidget::IsPointerOverPanel() const
{
	return Panel && Panel->IsHovered();
}

TArray<int32> UBuildModeWidget::GetVisibleEntries() const
{
	TArray<int32> Indices;
	for (const ULooterButton* Button : EntryButtons)
	{
		Indices.Add(Button->Index);
	}
	return Indices;
}

UTextBlock* UBuildModeWidget::MakeText(const FString& Text, int32 Size, const FLinearColor& TextColor)
{
	return LooterUI::MakeText(WidgetTree, Text, Size, TextColor);
}

ULooterButton* UBuildModeWidget::MakeButton(FName Action, int32 Index, const FText& Text, int32 FontSize, EButtonKind Kind)
{
	ULooterButton* Button = WidgetTree->ConstructWidget<ULooterButton>(ULooterButton::StaticClass());
	UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Button->Setup(Label, Action, Index, Text, FontSize, Kind);
	Button->OnButtonClicked.BindUObject(this, &UBuildModeWidget::HandleButton);
	return Button;
}

TSharedRef<SWidget> UBuildModeWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
		Root->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		WidgetTree->RootWidget = Root;

		// Invisible hit-test wrapper so clicks on the panel never place objects.
		Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Panel"));
		Panel->SetBrush(RectBrush(FLinearColor::Transparent));
		Panel->SetPadding(FMargin(0.f));
		Panel->SetVisibility(ESlateVisibility::Visible);
		if (UCanvasPanelSlot* PanelSlot = Root->AddChildToCanvas(Panel))
		{
			// Full-height strip down the left edge.
			PanelSlot->SetAnchors(FAnchors(0.f, 0.f, 0.f, 1.f));
			PanelSlot->SetOffsets(FMargin(16.f, 12.f, 400.f, 16.f));
		}

		// Crosshair: placement and selection happen where it points while the mouse is looking around.
		Crosshair = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		StyleText(Crosshair, 24, Color::Text());
		Crosshair->SetText(FText::FromString(TEXT("+")));
		Crosshair->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (UCanvasPanelSlot* CrosshairSlot = Root->AddChildToCanvas(Crosshair))
		{
			CrosshairSlot->SetAnchors(FAnchors(0.5f, 0.5f));
			CrosshairSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			CrosshairSlot->SetAutoSize(true);
		}

		ModeHint = LooterUI::MakeText(WidgetTree, TEXT(""), 12, Color::Text(), true, 150);
		ModeHintPlate = MakePlate(WidgetTree, ModeHint, FMargin(16.f, 7.f));
		ModeHintPlate->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (UCanvasPanelSlot* HintSlot = Root->AddChildToCanvas(ModeHintPlate))
		{
			HintSlot->SetAnchors(FAnchors(0.5f, 1.f));
			HintSlot->SetAlignment(FVector2D(0.5f, 1.f));
			HintSlot->SetPosition(FVector2D(0.f, -28.f));
			HintSlot->SetAutoSize(true);
		}

		UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		auto Add = [Column](UWidget* Child, float Top = 4.f, bool bFill = false)
		{
			UVerticalBoxSlot* ChildSlot = Column->AddChildToVerticalBox(Child);
			ChildSlot->SetPadding(FMargin(0.f, Top, 0.f, 0.f));
			if (bFill)
			{
				ChildSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			}
		};

		Add(LooterUI::MakeText(WidgetTree, TEXT("Build Mode key (F1): play   |   Esc: settings"), 10, Color::TextDim(), true, 100), 0.f);
		CountText = LooterUI::MakeText(WidgetTree, TEXT(""), 11, Color::TextDim(), false, 100);
		Add(CountText);

		Add(MakeSection(WidgetTree, TEXT("Palette")), 12.f);
		// Tabs wrap onto rows of three so every category name stays readable.
		CategoryRow = WidgetTree->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass());
		CategoryRow->SetSlotPadding(FMargin(2.f));
		Add(CategoryRow, 6.f);
		EntryList = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass());
		StyleScrollBox(EntryList);
		Add(EntryList, 8.f, true);

		Add(MakeSection(WidgetTree, TEXT("Tools")), 12.f);
		RandomRotationButton = MakeButton(ActionRandomRotation, 0, FText::GetEmpty(), 11, EButtonKind::Mini);
		RandomScaleButton = MakeButton(ActionRandomScale, 0, FText::GetEmpty(), 11, EButtonKind::Mini);
		AlignButton = MakeButton(ActionAlign, 0, FText::GetEmpty(), 11, EButtonKind::Mini);
		BrushButton = MakeButton(ActionBrush, 0, FText::GetEmpty(), 11, EButtonKind::Mini);
		for (ULooterButton* Toggle : { RandomRotationButton.Get(), RandomScaleButton.Get(), AlignButton.Get(), BrushButton.Get() })
		{
			Add(Toggle, 4.f);
		}

		UHorizontalBox* FileRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		const TPair<ULooterButton*, bool> FileButtons[] = {
			{ MakeButton(ActionSave, 0, FText::FromString(TEXT("Save")), 12), false },
			{ MakeButton(ActionUndo, 0, FText::FromString(TEXT("Undo")), 12), false },
			{ MakeButton(ActionClear, 0, FText::FromString(TEXT("Clear all")), 12, EButtonKind::Danger), true } };
		for (const TPair<ULooterButton*, bool>& Entry : FileButtons)
		{
			UHorizontalBoxSlot* ButtonSlot = FileRow->AddChildToHorizontalBox(Entry.Key);
			ButtonSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			ButtonSlot->SetPadding(FMargin(0.f, 0.f, Entry.Value ? 0.f : 6.f, 0.f));
		}
		Add(FileRow, 10.f);

		StatusText = LooterUI::MakeText(WidgetTree, TEXT(""), 12, Color::Accent());
		StatusText->SetAutoWrapText(true);
		Add(StatusText, 10.f);

		UTextBlock* Help = LooterUI::MakeText(WidgetTree, TEXT(
			"Mouse: look    WASD / Q E: fly    Shift: fast\n"
			"Hold Alt: cursor    1-9: pick item\n"
			"LMB: place / select    X: stop tool / deselect\n"
			"R or wheel: rotate    +/- or Shift+wheel: scale\n"
			"G: move    Del: delete    Ctrl+D: duplicate\n"
			"B: brush (hold Shift to erase)    [ ]: brush size\n"
			"RMB+wheel: fly speed    Ctrl+Z: undo stroke\n"
			"Ctrl+S: save (also autosaves)"), 10, Color::TextDim());
		Help->SetFont(Font(10, false));
		Help->SetAutoWrapText(true);
		Add(Help, 10.f);

		Panel->SetContent(MakePanel(WidgetTree, TEXT("Build Mode"), Column));

		RebuildCategories();
		RebuildEntries();
		RefreshToggles();
	}
	return Super::RebuildWidget();
}


void UBuildModeWidget::RebuildCategories()
{
	CategoryRow->ClearChildren();
	CategoryButtons.Reset();

	const UBuildModeComponent* Component = BuildMode.Get();
	const UEnvironmentPalette* Palette = Component ? Component->GetPalette() : nullptr;
	if (!Palette)
	{
		return;
	}

	Categories = Palette->GetCategories();
	if (!Categories.Contains(ActiveCategory) && Categories.Num() > 0)
	{
		ActiveCategory = Categories[0];
	}

	for (int32 Index = 0; Index < Categories.Num(); ++Index)
	{
		ULooterButton* Button = MakeButton(ActionCategory, Index, FText::FromName(Categories[Index]), 11, EButtonKind::Tab);
		Button->SetHighlighted(Categories[Index] == ActiveCategory);
		CategoryRow->AddChildToUniformGrid(Button, Index / 3, Index % 3);
		CategoryButtons.Add(Button);
	}
}

void UBuildModeWidget::RebuildEntries()
{
	EntryList->ClearChildren();
	EntryButtons.Reset();

	const UBuildModeComponent* Component = BuildMode.Get();
	const UEnvironmentPalette* Palette = Component ? Component->GetPalette() : nullptr;
	if (!Palette)
	{
		EntryList->AddChild(MakeText(TEXT("No palette found.\nCreate /Game/Environment/DA_EnvironmentPalette."), 12, Color::Worse()));
		return;
	}

	for (int32 Index = 0; Index < Palette->Entries.Num(); ++Index)
	{
		const FEnvironmentPaletteEntry& Entry = Palette->Entries[Index];
		if (Entry.Category != ActiveCategory)
		{
			continue;
		}
		// Number prefix matches the 1-9 hotkeys.
		const int32 Number = EntryButtons.Num() + 1;
		const FText Label = Number <= 9
			? FText::FromString(FString::Printf(TEXT("%d   %s"), Number, *Entry.DisplayName.ToString()))
			: Entry.DisplayName;
		ULooterButton* Button = MakeButton(ActionEntry, Index, Label, 13);
		Button->SetHighlighted(Index == Component->GetSelectedEntry());
		if (UScrollBoxSlot* EntrySlot = Cast<UScrollBoxSlot>(EntryList->AddChild(Button)))
		{
			EntrySlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
		}
		EntryButtons.Add(Button);
	}
	ShownSelection = Component->GetSelectedEntry();
}

void UBuildModeWidget::RefreshToggles()
{
	const UBuildModeComponent* Component = BuildMode.Get();
	if (!Component || !RandomRotationButton)
	{
		return;
	}

	RandomRotationButton->SetLabel(OnOff(TEXT("Random rotation"), Component->IsRandomRotation()));
	RandomScaleButton->SetLabel(OnOff(TEXT("Random scale"), Component->IsRandomScale()));
	AlignButton->SetLabel(OnOff(TEXT("Force align to slope"), Component->IsAlignToSurface()));
	BrushButton->SetLabel(FText::FromString(FString::Printf(TEXT("Scatter brush: %s  (%.0f m)"),
		Component->IsBrush() ? TEXT("ON") : TEXT("OFF"), Component->GetBrushRadius() / 100.f)));
	BrushButton->SetHighlighted(Component->IsBrush());
}

void UBuildModeWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	const UBuildModeComponent* Component = BuildMode.Get();
	if (!Component || !StatusText)
	{
		return;
	}

	StatusText->SetText(FText::FromString(Component->GetStatus()));
	CountText->SetText(FText::FromString(FString::Printf(TEXT("%d objects in layout%s"), Component->GetObjectCount(),
		Component->HasUnsavedChanges() ? TEXT("   (unsaved changes)") : TEXT("   (saved)"))));
	RefreshToggles();

	const bool bCursor = Component->IsCursorMode();
	Crosshair->SetVisibility(bCursor ? ESlateVisibility::Hidden : ESlateVisibility::HitTestInvisible);
	ModeHint->SetText(FText::FromString(bCursor ? TEXT("CURSOR MODE: RELEASE ALT TO LOOK AROUND") : TEXT("HOLD ALT TO USE THE PALETTE WITH THE MOUSE")));

	// Hotkeys (X) can change the selection too, so keep highlights in sync.
	if (ShownSelection != Component->GetSelectedEntry())
	{
		ShownSelection = Component->GetSelectedEntry();
		for (ULooterButton* Button : EntryButtons)
		{
			Button->SetHighlighted(Button->Index == ShownSelection);
		}
	}
}

void UBuildModeWidget::HandleButton(ULooterButton* Button)
{
	UBuildModeComponent* Component = BuildMode.Get();
	if (!Component || !Button)
	{
		return;
	}

	const FName Action = Button->Action;
	if (Action == ActionCategory && Categories.IsValidIndex(Button->Index))
	{
		ActiveCategory = Categories[Button->Index];
		RebuildCategories();
		RebuildEntries();
	}
	else if (Action == ActionEntry) { Component->SelectEntry(Button->Index); }
	else if (Action == ActionRandomRotation) { Component->ToggleRandomRotation(); }
	else if (Action == ActionRandomScale) { Component->ToggleRandomScale(); }
	else if (Action == ActionAlign) { Component->ToggleAlignOverride(); }
	else if (Action == ActionBrush) { Component->ToggleBrush(); }
	else if (Action == ActionSave) { Component->SaveLayout(); }
	else if (Action == ActionUndo) { Component->Undo(); }
	else if (Action == ActionClear) { Component->ClearAll(); }
}

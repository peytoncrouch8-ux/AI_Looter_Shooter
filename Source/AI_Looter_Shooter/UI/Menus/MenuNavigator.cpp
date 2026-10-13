#include "UI/Menus/MenuNavigator.h"
#include "UI/Style/LooterButton.h"
#include "UI/Style/LooterUIFocus.h"
#include "Components/Slider.h"

// ---------------------------------------------------------------------------
// A stop
// ---------------------------------------------------------------------------

FMenuNavItem FMenuNavItem::MakeButton(ULooterButton* InButton, int32 InSection, UWidget* InRow)
{
	FMenuNavItem Item;
	Item.Button = InButton;
	Item.Light = InButton;
	Item.Row = InRow;
	Item.Section = InSection;
	return Item;
}

FMenuNavItem FMenuNavItem::MakeSlider(USlider* InSlider, UWidget* InLight, UWidget* InRow, int32 InSection)
{
	FMenuNavItem Item;
	Item.Slider = InSlider;
	Item.Light = InLight;
	Item.Row = InRow;
	Item.Section = InSection;
	return Item;
}

FMenuNavItem FMenuNavItem::MakeChoices(const TArray<ULooterButton*>& InChoices, UWidget* InLight, UWidget* InRow, int32 InSection)
{
	FMenuNavItem Item;
	for (ULooterButton* Choice : InChoices)
	{
		Item.Choices.Add(Choice);
	}
	Item.Light = InLight;
	Item.Row = InRow;
	Item.Section = InSection;
	return Item;
}

UWidget* FMenuNavItem::GetWidget() const
{
	if (ULooterButton* ButtonPtr = Button.Get())
	{
		return ButtonPtr;
	}
	if (USlider* SliderPtr = Slider.Get())
	{
		return SliderPtr;
	}
	return Choices.IsEmpty() ? nullptr : Choices[0].Get();
}

bool FMenuNavItem::IsAvailable() const
{
	// The widget's own visibility setting, not UWidget::IsVisible: that one is false until the Slate widget is built, and a
	// menu notes its stops while it's still being built (the main menu takes its first focus inside RebuildWidget).
	const UWidget* Widget = GetWidget();
	if (!Widget || !Widget->GetIsEnabled())
	{
		return false;
	}
	const ESlateVisibility Visibility = Widget->GetVisibility();
	return Visibility != ESlateVisibility::Collapsed && Visibility != ESlateVisibility::Hidden;
}

int32 FMenuNavItem::GetChosen() const
{
	for (int32 Index = 0; Index < Choices.Num(); ++Index)
	{
		if (const ULooterButton* Choice = Choices[Index].Get(); Choice && Choice->IsHighlighted())
		{
			return Index;
		}
	}
	return INDEX_NONE;
}

// ---------------------------------------------------------------------------
// The stops
// ---------------------------------------------------------------------------

void FMenuNavigator::SetRows(TArray<FMenuNavRow> InRows, const UWidget* Prefer)
{
	const UWidget* Before = GetFocusedWidget();
	EndSliderHold();
	Rows = MoveTemp(InRows);
	Row = INDEX_NONE;
	Column = 0;

	int32 FoundRow = INDEX_NONE;
	int32 FoundColumn = INDEX_NONE;
	if ((Prefer && FindWidget(Prefer, FoundRow, FoundColumn)) || (Before && FindWidget(Before, FoundRow, FoundColumn)))
	{
		SetFocus(FoundRow, FoundColumn);
		WantedColumn = FoundColumn;
	}
	else
	{
		FocusFirst();
	}
}

void FMenuNavigator::Reset()
{
	EndSliderHold();
	Rows.Reset();
	Row = INDEX_NONE;
	Column = 0;
	WantedColumn = 0;
	Relight();
}

bool FMenuNavigator::IsRowAvailable(int32 RowIndex) const
{
	return Rows.IsValidIndex(RowIndex) && Rows[RowIndex].ContainsByPredicate([](const FMenuNavItem& Item) { return Item.IsAvailable(); });
}

int32 FMenuNavigator::NearestColumn(int32 RowIndex, int32 Wanted) const
{
	if (!Rows.IsValidIndex(RowIndex))
	{
		return INDEX_NONE;
	}
	const FMenuNavRow& Items = Rows[RowIndex];
	int32 Best = INDEX_NONE;
	for (int32 Index = 0; Index < Items.Num(); ++Index)
	{
		if (Items[Index].IsAvailable() && (Best == INDEX_NONE || FMath::Abs(Index - Wanted) < FMath::Abs(Best - Wanted)))
		{
			Best = Index;
		}
	}
	return Best;
}

bool FMenuNavigator::FindWidget(const UWidget* Widget, int32& OutRow, int32& OutColumn) const
{
	if (!Widget)
	{
		return false;
	}
	for (int32 RowIndex = 0; RowIndex < Rows.Num(); ++RowIndex)
	{
		for (int32 Index = 0; Index < Rows[RowIndex].Num(); ++Index)
		{
			const FMenuNavItem& Item = Rows[RowIndex][Index];
			const bool bIsIt = Item.GetWidget() == Widget || Item.Light.Get() == Widget
				|| Item.Choices.ContainsByPredicate([Widget](const TWeakObjectPtr<ULooterButton>& Choice) { return Choice.Get() == Widget; });
			if (bIsIt && Item.IsAvailable())
			{
				OutRow = RowIndex;
				OutColumn = Index;
				return true;
			}
		}
	}
	return false;
}

void FMenuNavigator::FocusFirst()
{
	for (int32 RowIndex = 0; RowIndex < Rows.Num(); ++RowIndex)
	{
		const int32 First = NearestColumn(RowIndex, 0);
		if (First != INDEX_NONE)
		{
			SetFocus(RowIndex, First);
			WantedColumn = First;
			return;
		}
	}
	SetFocus(INDEX_NONE, 0);
}

void FMenuNavigator::SetFocus(int32 NewRow, int32 NewColumn)
{
	if (NewRow != Row || NewColumn != Column)
	{
		EndSliderHold();
		Row = NewRow;
		Column = NewColumn;
		LitAge = 0.f;
	}
	Relight();
}

const FMenuNavItem* FMenuNavigator::GetFocused() const
{
	return Rows.IsValidIndex(Row) && Rows[Row].IsValidIndex(Column) ? &Rows[Row][Column] : nullptr;
}

UWidget* FMenuNavigator::GetFocusedWidget() const
{
	const FMenuNavItem* Item = GetFocused();
	return Item ? Item->GetWidget() : nullptr;
}

// ---------------------------------------------------------------------------
// Moving
// ---------------------------------------------------------------------------

EMenuNavResult FMenuNavigator::MoveVertical(int32 Step)
{
	if (Step == 0 || Rows.IsEmpty())
	{
		return EMenuNavResult::None;
	}
	// The focus was lost (its stop hidden since): any move finds the first one again.
	const FMenuNavItem* Here = GetFocused();
	if (!Here || !Here->IsAvailable())
	{
		FocusFirst();
		return GetFocused() ? EMenuNavResult::Moved : EMenuNavResult::None;
	}
	const int32 Direction = Step > 0 ? 1 : -1;
	int32 Next = Row;
	for (int32 Tries = 0; Tries < Rows.Num(); ++Tries)
	{
		Next += Direction;
		if (!Rows.IsValidIndex(Next))
		{
			if (!bWrap)
			{
				return EMenuNavResult::None;
			}
			Next = Direction > 0 ? 0 : Rows.Num() - 1;
		}
		if (Next == Row)
		{
			return EMenuNavResult::None;
		}
		if (IsRowAvailable(Next))
		{
			SetFocus(Next, NearestColumn(Next, WantedColumn));
			return EMenuNavResult::Moved;
		}
	}
	return EMenuNavResult::None;
}

EMenuNavResult FMenuNavigator::MoveHorizontal(int32 Step)
{
	const FMenuNavItem* Here = GetFocused();
	if (Step == 0 || !Here || !Here->IsAvailable())
	{
		return EMenuNavResult::None;
	}
	const int32 Direction = Step > 0 ? 1 : -1;
	if (USlider* Slider = Here->Slider.Get())
	{
		return StepSlider(*Slider, Direction);
	}
	if (Here->IsChoices())
	{
		// Left and right stop at the ends (a switch's Off stays Off); A goes round.
		return PickChoice(*Here, Direction, false) ? EMenuNavResult::Chose : EMenuNavResult::None;
	}
	const FMenuNavRow& Items = Rows[Row];
	for (int32 Index = Column + Direction; Items.IsValidIndex(Index); Index += Direction)
	{
		if (Items[Index].IsAvailable())
		{
			SetFocus(Row, Index);
			WantedColumn = Index;
			return EMenuNavResult::Moved;
		}
	}
	return EMenuNavResult::None;
}

EMenuNavResult FMenuNavigator::Activate()
{
	const FMenuNavItem* Here = GetFocused();
	if (!Here || !Here->IsAvailable())
	{
		return EMenuNavResult::None;
	}
	if (Here->IsChoices())
	{
		return PickChoice(*Here, 1, true) ? EMenuNavResult::Chose : EMenuNavResult::None;
	}
	if (ULooterButton* Button = Here->Button.Get())
	{
		EndSliderHold();
		// Pressing may rebuild the menu's stops (a page opens): nothing here is touched after it.
		return Button->Press() ? EMenuNavResult::Pressed : EMenuNavResult::None;
	}
	return EMenuNavResult::None;
}

EMenuNavResult FMenuNavigator::JumpSection(int32 Step, int32 NumSections)
{
	if (Step == 0 || NumSections <= 0 || Rows.IsEmpty())
	{
		return EMenuNavResult::None;
	}
	const FMenuNavItem* Here = GetFocused();
	const int32 Current = Here ? Here->Section : INDEX_NONE;
	int32 Target = Current + (Step > 0 ? 1 : -1);
	if (Target < 0 || Target >= NumSections)
	{
		Target = Step > 0 ? 0 : NumSections - 1;
	}
	// The first available stop of that section (or the next one on, if it has none to stop at).
	for (int32 Tries = 0; Tries < NumSections; ++Tries)
	{
		for (int32 RowIndex = 0; RowIndex < Rows.Num(); ++RowIndex)
		{
			const int32 First = NearestColumn(RowIndex, 0);
			if (First != INDEX_NONE && Rows[RowIndex][First].Section == Target)
			{
				const bool bMoved = RowIndex != Row || First != Column;
				SetFocus(RowIndex, First);
				WantedColumn = First;
				return bMoved ? EMenuNavResult::Moved : EMenuNavResult::None;
			}
		}
		Target = (Target + (Step > 0 ? 1 : NumSections - 1)) % NumSections;
	}
	return EMenuNavResult::None;
}

bool FMenuNavigator::FocusOn(const UWidget* Widget)
{
	int32 FoundRow = INDEX_NONE;
	int32 FoundColumn = INDEX_NONE;
	if (!FindWidget(Widget, FoundRow, FoundColumn))
	{
		return false;
	}
	SetFocus(FoundRow, FoundColumn);
	WantedColumn = FoundColumn;
	return true;
}

// ---------------------------------------------------------------------------
// Sliders and choices
// ---------------------------------------------------------------------------

float FMenuNavigator::SteppedValue(float Value, float Min, float Max, float Step, int32 Direction)
{
	if (Max <= Min)
	{
		return Min;
	}
	const float Size = Step > 0.f ? Step : (Max - Min) / 20.f;
	// From the step it's on (or just past): a value between steps lands on the step that way, never skips one.
	constexpr float Slack = 1e-3f;
	const float Steps = (FMath::Clamp(Value, Min, Max) - Min) / Size;
	const float Target = Direction > 0 ? FMath::FloorToFloat(Steps + 1.f + Slack) : FMath::CeilToFloat(Steps - 1.f - Slack);
	return FMath::Clamp(Min + Target * Size, Min, Max);
}

EMenuNavResult FMenuNavigator::StepSlider(USlider& Slider, int32 Direction)
{
	const float Current = Slider.GetValue();
	const float Next = SteppedValue(Current, Slider.GetMinValue(), Slider.GetMaxValue(), Slider.GetStepSize(), Direction);
	if (FMath::IsNearlyEqual(Next, FMath::Clamp(Current, Slider.GetMinValue(), Slider.GetMaxValue()), UE_KINDA_SMALL_NUMBER))
	{
		return EMenuNavResult::None;
	}
	// Taken as a controller takes a focused slider, at its first step: the menu's previews show and its cancel knows the
	// value to go back to; the let-go (EndSliderHold) saves it, as the mouse's does.
	if (HeldSlider.Get() != &Slider)
	{
		EndSliderHold();
		HeldSlider = &Slider;
		Slider.OnControllerCaptureBegin.Broadcast();
	}
	// Its change event moves the setting and the value beside it, as a drag does.
	Slider.SetValue(Next);
	return EMenuNavResult::Stepped;
}

bool FMenuNavigator::PickChoice(const FMenuNavItem& Item, int32 Direction, bool bWrapRound)
{
	const int32 Count = Item.Choices.Num();
	if (Count == 0)
	{
		return false;
	}
	const int32 Chosen = Item.GetChosen();
	int32 Target = Chosen == INDEX_NONE ? (Direction > 0 ? 0 : Count - 1) : Chosen + Direction;
	if (bWrapRound)
	{
		Target = (Target % Count + Count) % Count;
	}
	if (!Item.Choices.IsValidIndex(Target) || Target == Chosen)
	{
		return false;
	}
	ULooterButton* Choice = Item.Choices[Target].Get();
	// The choice's own click: its sound and the menu's handler, which sets the setting and lights it.
	return Choice && Choice->Press();
}

void FMenuNavigator::EndSliderHold()
{
	if (USlider* Slider = HeldSlider.Get())
	{
		HeldSlider.Reset();
		Slider->OnControllerCaptureEnd.Broadcast();
	}
	HeldSlider.Reset();
}

// ---------------------------------------------------------------------------
// The look
// ---------------------------------------------------------------------------

void FMenuNavigator::SetShown(bool bInShown)
{
	if (bShown != bInShown)
	{
		bShown = bInShown;
		LitAge = 0.f;
		Relight();
	}
}

void FMenuNavigator::Relight()
{
	const FMenuNavItem* Here = GetFocused();
	UWidget* Wanted = bShown && Here && Here->IsAvailable() ? Here->Light.Get() : nullptr;
	UWidget* Lit = LitWidget.Get();
	if (Lit && Lit != Wanted)
	{
		LooterUI::SetFocusLit(Lit, false);
	}
	if (Wanted && (Wanted != Lit || !LooterUI::IsFocusLit(Wanted)))
	{
		LooterUI::SetFocusLit(Wanted, true);
		LooterUI::SetFocusGlow(Wanted, LooterUI::FocusGlowAt(LitAge));
	}
	LitWidget = Wanted;
}

void FMenuNavigator::Tick(float DeltaSeconds)
{
	// A stop that went away under the focus (a page closed, a button hidden) puts its look out.
	const FMenuNavItem* Here = GetFocused();
	if (LitWidget.IsValid() && (!Here || !Here->IsAvailable()))
	{
		Relight();
	}
	if (UWidget* Lit = LitWidget.Get())
	{
		LitAge += DeltaSeconds;
		LooterUI::SetFocusGlow(Lit, LooterUI::FocusGlowAt(LitAge));
	}
}

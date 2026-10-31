// USettingsMenuWidget's rows' list scrolling by whole rows (the user's check of the full page, 2026-10-31: the Controls page's
// key list ran on under the key hints, a row cut in half). The list's height is set each frame to the whole rows that fit
// from where it stands, so it always ends on a row's foot above the hints; the keys scroll it to keep the focus wholly in
// view, the wheel moves it a row at a time, and a list the scroll bar left part-way snaps to the nearest row.

#include "UI/Menus/SettingsMenuWidget.h"
#include "UI/Menus/SettingsMenuParts.h"
#include "UI/Menus/SettingsPageArt.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ScrollBox.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

using namespace SettingsMenu;
using namespace SettingsMenu::Layout;

namespace
{
	/** A scroll eases to its row: 95% of the way in three of these (about 0.15 s). */
	constexpr float ScrollEase = 0.05f;
	/** The scroll bar let go this long: the list snaps to the nearest row. */
	constexpr float SnapAfterSeconds = 0.15f;
	/** Within this of a row's top counts as on it. */
	constexpr float OnRow = 0.5f;

	/** The first row top from which every row after it fits in Room: the last place a view may start. */
	float LastStart(const TArray<float>& Tops, float End, float Room)
	{
		for (const float Top : Tops)
		{
			if (End - Top <= Room + OnRow)
			{
				return Top;
			}
		}
		return Tops.Num() > 0 ? Tops.Last() : 0.f;
	}

	/** The foot of the last whole row in a view Room tall starting at From (From itself when none fits). */
	float WholeRowsFoot(const TArray<float>& Tops, float End, float From, float Room)
	{
		float Foot = From;
		for (int32 Index = 0; Index < Tops.Num(); ++Index)
		{
			const float RowFoot = Index + 1 < Tops.Num() ? Tops[Index + 1] : End;
			if (RowFoot > From + OnRow && RowFoot <= From + Room + OnRow)
			{
				Foot = RowFoot;
			}
		}
		return Foot;
	}
}

float USettingsMenuWidget::ListRoom() const
{
	// The canvas is the screen in Slate units (1080 tall at 16:9); before it's laid out, the mockup's.
	float ScreenHeight = 1080.f;
	if (PageCanvas)
	{
		const float Laid = static_cast<float>(PageCanvas->GetCachedGeometry().GetLocalSize().Y);
		if (Laid > 1.f)
		{
			ScreenHeight = Laid;
		}
	}
	return FMath::Max(SettingsPageArt::RowHeight, ScreenHeight - RowsTop - RowsBottomMargin);
}

bool USettingsMenuWidget::GetRowSpans(TArray<float>& OutTops, float& OutEnd, TArray<const UWidget*>* OutRows) const
{
	OutTops.Reset();
	OutEnd = 0.f;
	if (OutRows)
	{
		OutRows->Reset();
	}
	const UVerticalBox* Rows = Body && Body->GetChildrenCount() > 0 ? Cast<UVerticalBox>(Body->GetChildAt(0)) : nullptr;
	if (!Rows)
	{
		return false;
	}
	// Down the page's list in order, the key list's rows each a row of their own, from their desired heights (laid out
	// whether on screen or not) and their slots' padding.
	float Y = 0.f;
	auto Add = [&Y, &OutTops, OutRows](const UWidget* Row)
	{
		const UVerticalBoxSlot* RowSlot = Cast<UVerticalBoxSlot>(Row->Slot);
		const FMargin Padding = RowSlot ? RowSlot->GetPadding() : FMargin(0.f);
		Y += Padding.Top;
		OutTops.Add(Y);
		if (OutRows)
		{
			OutRows->Add(Row);
		}
		Y += static_cast<float>(Row->GetDesiredSize().Y) + Padding.Bottom;
	};
	for (int32 Index = 0; Index < Rows->GetChildrenCount(); ++Index)
	{
		const UWidget* Child = Rows->GetChildAt(Index);
		if (!Child || !Child->IsVisible())
		{
			continue;
		}
		if (KeyList && Child == KeyList.Get())
		{
			for (int32 Key = 0; Key < KeyList->GetChildrenCount(); ++Key)
			{
				const UWidget* KeyRow = KeyList->GetChildAt(Key);
				if (KeyRow && KeyRow->IsVisible())
				{
					Add(KeyRow);
				}
			}
			continue;
		}
		Add(Child);
	}
	OutEnd = Y;
	return OutEnd > 1.f;
}

void USettingsMenuWidget::ScrollToFocus()
{
	if (const FMenuNavItem* Here = Nav.GetFocused())
	{
		ScrollRowIntoView(Here->Row.Get());
	}
}

void USettingsMenuWidget::ScrollRowIntoView(const UWidget* Row)
{
	TArray<float> Tops;
	TArray<const UWidget*> Rows;
	float End = 0.f;
	if (!Row || !GetRowSpans(Tops, End, &Rows))
	{
		return;
	}
	const int32 Index = Rows.IndexOfByKey(Row);
	if (Index == INDEX_NONE)
	{
		return;
	}
	const float Room = ListRoom();
	const float Top = Tops[Index];
	const float Foot = Index + 1 < Tops.Num() ? Tops[Index + 1] : End;
	const float Current = ScrollTarget >= 0.f ? ScrollTarget : ScrollSetOffset;
	float Want = Current;
	if (Top < Current - OnRow)
	{
		// Above the view: the view starts on it.
		Want = Top;
	}
	else if (Foot > Current + Room + OnRow)
	{
		// Below: the view starts on the first row from which everything down to its foot fits.
		for (const float Start : Tops)
		{
			if (Start >= Current - OnRow && Foot - Start <= Room + OnRow)
			{
				Want = Start;
				break;
			}
		}
	}
	if (!FMath::IsNearlyEqual(Want, Current, OnRow))
	{
		ScrollTarget = FMath::Min(Want, LastStart(Tops, End, Room));
	}
}

void USettingsMenuWidget::ScrollByRows(int32 Rows)
{
	TArray<float> Tops;
	float End = 0.f;
	if (Rows == 0 || !GetRowSpans(Tops, End))
	{
		return;
	}
	const float Room = ListRoom();
	const float Current = ScrollTarget >= 0.f ? ScrollTarget : ScrollSetOffset;
	const float Last = LastStart(Tops, End, Room);
	int32 From = 0;
	while (From + 1 < Tops.Num() && Tops[From] < Current - OnRow)
	{
		++From;
	}
	int32 LastIndex = Tops.IndexOfByPredicate([Last](float Top) { return FMath::IsNearlyEqual(Top, Last, OnRow); });
	LastIndex = LastIndex == INDEX_NONE ? Tops.Num() - 1 : LastIndex;
	ScrollTarget = Tops[FMath::Clamp(From + Rows, 0, LastIndex)];
	ScrollIdle = 0.f;
}

void USettingsMenuWidget::AdvanceScroll(float DeltaSeconds)
{
	UCanvasPanelSlot* ListSlot = PagesSlot.Get();
	if (!Body || !ListSlot)
	{
		return;
	}
	const float Delta = FMath::Clamp(DeltaSeconds, 0.f, 0.1f);
	const float Room = ListRoom();
	float Height = Room;
	TArray<float> Tops;
	float End = 0.f;
	if (GetRowSpans(Tops, End))
	{
		const float Offset = Body->GetScrollOffset();
		if (!FMath::IsNearlyEqual(Offset, ScrollSetOffset, OnRow))
		{
			// Moved by its scroll bar (or held back by the list's end): no snap until it rests.
			ScrollTarget = -1.f;
			ScrollIdle = 0.f;
			ScrollSetOffset = Offset;
		}
		else
		{
			ScrollIdle += Delta;
		}
		const float Last = LastStart(Tops, End, Room);
		if (ScrollTarget < 0.f && ScrollIdle > SnapAfterSeconds)
		{
			float Nearest = 0.f;
			float Best = TNumericLimits<float>::Max();
			for (const float Top : Tops)
			{
				if (Top <= Last + OnRow && FMath::Abs(Top - Offset) < Best)
				{
					Best = FMath::Abs(Top - Offset);
					Nearest = Top;
				}
			}
			if (Best > OnRow)
			{
				ScrollTarget = Nearest;
			}
		}
		// The view is the whole rows from where the scroll is going, so its foot holds still while the rows move.
		const float From = ScrollTarget >= 0.f ? ScrollTarget : ScrollSetOffset;
		const float Foot = WholeRowsFoot(Tops, End, From, Room);
		Height = Foot - From > 1.f ? Foot - From : Room;
		if (ScrollTarget >= 0.f)
		{
			const float Follow = Delta > 0.f ? 1.f - FMath::Exp(-Delta / ScrollEase) : 1.f;
			float Next = FMath::Lerp(ScrollSetOffset, ScrollTarget, Follow);
			if (FMath::IsNearlyEqual(Next, ScrollTarget, OnRow))
			{
				Next = ScrollTarget;
				ScrollTarget = -1.f;
			}
			Body->SetScrollOffset(Next);
			ScrollSetOffset = Next;
		}
	}
	if (!FMath::IsNearlyEqual(Height, ShownListHeight, OnRow))
	{
		ShownListHeight = Height;
		FMargin Offsets = ListSlot->GetOffsets();
		Offsets.Bottom = Height;
		ListSlot->SetOffsets(Offsets);
	}
}

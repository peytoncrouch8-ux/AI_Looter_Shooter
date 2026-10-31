// USettingsMenuWidget's motion and the row marked (E2 "Big Dial"). The open: the dial swings up into place as the title
// slides in, the pennants drop onto their ticks one after another, the rows arrive from the left one after another and the
// help card from the right, the key hints last (all within about 0.4 s, none of it in the way of a key press). A tab turned:
// the dial clicks a tick that way with a slight overshoot (the hub's "one click per option"), the lit pennant drops in, the
// new page's rows arrive. The card's words rise in when they change. The row marked (the focus's under keys, the pointer's
// under the mouse, else the page's first) wears the gold light, its number and name gold.

#include "UI/Menus/SettingsMenuWidget.h"
#include "UI/Menus/SettingsMenuParts.h"
#include "UI/Style/LooterButton.h"
#include "UI/Style/LooterUIFocus.h"
#include "UI/Style/LooterUIStyle.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

using namespace SettingsMenu;

namespace
{
	/** The dial swings up this far as the page opens, and clicks one tick (10 degrees) as a tab turns. */
	constexpr float OpenSwingDegrees = 60.f;
	constexpr float OpenSwingSeconds = 0.32f;
	constexpr float ClickDegrees = 10.f;
	constexpr float ClickSeconds = 0.18f;
	/** Each piece arrives in this long, after its delay. */
	constexpr float ArriveSeconds = 0.22f;
	constexpr float TitleDelay = 0.04f;
	constexpr float TitleSlide = 16.f;
	constexpr float TabDelay = 0.06f;
	constexpr float TabStagger = 0.035f;
	constexpr float TabDrop = 12.f;
	/** The lit pennant drops this far onto its tick as its page turns up. */
	constexpr float LitDrop = 8.f;
	constexpr float LitDropSeconds = 0.16f;
	/** The rows arrive one after another, the first this many staggered (the rest with the last). */
	constexpr float RowStagger = 0.03f;
	constexpr float RowSlide = 24.f;
	constexpr int32 RowsStaggered = 10;
	constexpr float CardDelay = 0.1f;
	constexpr float CardSlide = 24.f;
	constexpr float HintDelay = 0.14f;
	/** New words on the card rise this far, from this faint. */
	constexpr float HelpRiseSeconds = 0.14f;
	constexpr float HelpRise = 6.f;
	constexpr float HelpFrom = 0.35f;

	float EaseOut(float T)
	{
		const float Left = 1.f - FMath::Clamp(T, 0.f, 1.f);
		return 1.f - Left * Left * Left;
	}

	/** Eased out with a slight overshoot past the end, then back (a click settling). */
	float EaseOutBack(float T)
	{
		constexpr float Overshoot = 1.2f;
		const float Back = FMath::Clamp(T, 0.f, 1.f) - 1.f;
		return 1.f + (Overshoot + 1.f) * Back * Back * Back + Overshoot * Back * Back;
	}

	/** Widget at Opacity, moved by Translation: set only when it differs (each set redraws it). */
	void SetMotion(UWidget* Widget, float Opacity, const FVector2D& Translation)
	{
		if (!Widget)
		{
			return;
		}
		if (!FMath::IsNearlyEqual(Widget->GetRenderOpacity(), Opacity, 0.002f))
		{
			Widget->SetRenderOpacity(Opacity);
		}
		if (!Widget->GetRenderTransform().Translation.Equals(Translation, 0.05))
		{
			Widget->SetRenderTranslation(Translation);
		}
	}
}

void USettingsMenuWidget::BeginOpenMotion()
{
	OpenAge = 0.f;
	PageAge = 0.f;
	HelpAge = 10.f;
	DialClickAge = 10.f;
	MarkRow(DefaultMarkedRow());
	RefreshTabLooks();
	// The first frame shows everything at its start, before any tick has run.
	AdvanceLook(0.f);
}

void USettingsMenuWidget::BeginPageMotion(int32 Direction)
{
	PageAge = 0.f;
	DialClickAge = 0.f;
	DialClickDirection = Direction >= 0 ? 1.f : -1.f;
}

UWidget* USettingsMenuWidget::DefaultMarkedRow() const
{
	const FSettingsLedgerLine* First = LedgerLines.FindByPredicate([this](const FSettingsLedgerLine& Line)
	{
		return Line.Page == CurrentPage && Line.Number.IsValid();
	});
	return First ? First->Row.Get() : nullptr;
}

UWidget* USettingsMenuWidget::FindMarkedRow(bool& bOutHide) const
{
	bOutHide = false;
	if (Nav.IsShown())
	{
		// Keys or a gamepad: the focus's row.
		const FMenuNavItem* Here = Nav.GetFocused();
		if (Here && Here->Row.IsValid())
		{
			return Here->Row.Get();
		}
		bOutHide = true;
		return nullptr;
	}
	// The mouse: the row under the pointer; between rows (or off them) the light stays where it was.
	for (const FSettingsLedgerLine& Line : LedgerLines)
	{
		if (Line.Page == CurrentPage && Line.Row.IsValid() && Line.Row->IsHovered())
		{
			return Line.Row.Get();
		}
	}
	return nullptr;
}

void USettingsMenuWidget::MarkRow(UWidget* Row)
{
	using namespace LooterUI;
	const UWidget* Before = MarkedRow.Get();
	MarkedRow = Row;
	for (const FSettingsLedgerLine& Line : LedgerLines)
	{
		const UWidget* LineRow = Line.Row.Get();
		if (!LineRow || (LineRow != Before && LineRow != Row))
		{
			continue;
		}
		const bool bMarked = LineRow == Row;
		if (UTextBlock* Number = Line.Number.Get())
		{
			Number->SetColorAndOpacity(FSlateColor(bMarked ? Color::SoulGold() : Color::SlateIcon()));
		}
		if (UTextBlock* Name = Line.Name.Get())
		{
			Name->SetColorAndOpacity(FSlateColor(bMarked ? Color::SoulGold() : Line.NameColor));
		}
	}
}

void USettingsMenuWidget::AdvanceLook(float DeltaSeconds)
{
	const float Delta = FMath::Clamp(DeltaSeconds, 0.f, 0.1f);
	OpenAge += Delta;
	PageAge += Delta;
	HelpAge += Delta;
	DialClickAge += Delta;

	// The dial: up into place as the page opens, a click a tick as a tab turns. Its ticks are 10 degrees apart, so a whole
	// click ends looking as it began but for the needle's swing.
	if (Dial)
	{
		const float Swing = -OpenSwingDegrees * (1.f - EaseOut(OpenAge / OpenSwingSeconds));
		const float Click = -DialClickDirection * ClickDegrees * (1.f - EaseOutBack(DialClickAge / ClickSeconds));
		const float Angle = Swing + Click;
		if (!FMath::IsNearlyEqual(Dial->GetRenderTransformAngle(), Angle, 0.01f))
		{
			Dial->SetRenderTransformAngle(Angle);
		}
	}

	// The title, the pennants, the card and the hints arriving.
	const float TitleIn = EaseOut((OpenAge - TitleDelay) / ArriveSeconds);
	SetMotion(TitleBlock, TitleIn, FVector2D(-TitleSlide * (1.f - TitleIn), 0.f));
	for (int32 Page = 0; Page < TabCells.Num(); ++Page)
	{
		const float In = EaseOut((OpenAge - TabDelay - TabStagger * Page) / ArriveSeconds);
		const float Drop = Page == CurrentPage ? -LitDrop * (1.f - EaseOutBack(PageAge / LitDropSeconds)) : 0.f;
		SetMotion(TabCells[Page], In, FVector2D(0.f, -TabDrop * (1.f - In)));
		// The pennant alone drops onto its tick; the tick stays on the ruler.
		if (TabButtons.IsValidIndex(Page))
		{
			SetMotion(TabButtons[Page], 1.f, FVector2D(0.f, Drop));
		}
	}
	const float CardIn = EaseOut((OpenAge - CardDelay) / ArriveSeconds);
	SetMotion(HelpCard, CardIn, FVector2D(CardSlide * (1.f - CardIn), 0.f));
	const float WordsIn = EaseOut(HelpAge / HelpRiseSeconds);
	SetMotion(HelpWords, FMath::Lerp(HelpFrom, 1.f, WordsIn), FVector2D(0.f, HelpRise * (1.f - WordsIn)));
	SetMotion(Hints, EaseOut((OpenAge - HintDelay) / ArriveSeconds), FVector2D::ZeroVector);

	// The page's rows arriving one after another, and the light on the row marked. A row marked on another page (a tab
	// clicked) hands the light to the new page's first row.
	bool bHide = false;
	UWidget* Wanted = FindMarkedRow(bHide);
	if (!Wanted && !bHide)
	{
		const FSettingsLedgerLine* Marked = LedgerLines.FindByPredicate([this](const FSettingsLedgerLine& Line) { return Line.Row.Get() == MarkedRow.Get(); });
		if (!MarkedRow.IsValid() || !Marked || Marked->Page != CurrentPage)
		{
			Wanted = DefaultMarkedRow();
		}
	}
	if (Wanted && Wanted != MarkedRow.Get())
	{
		MarkRow(Wanted);
	}
	const bool bKeysDrive = Nav.IsShown();
	int32 Order = 0;
	for (const FSettingsLedgerLine& Line : LedgerLines)
	{
		if (Line.Page != CurrentPage)
		{
			continue;
		}
		const float Start = RowStagger * FMath::Min(Order++, RowsStaggered);
		const float In = EaseOut((PageAge - Start) / ArriveSeconds);
		SetMotion(Line.Row.Get(), In, FVector2D(-RowSlide * (1.f - In), 0.f));
		UWidget* Light = Line.Light.Get();
		const bool bLit = !bHide && Line.Row.Get() == MarkedRow.Get();
		if (Light && LooterUI::IsFocusLit(Light) != bLit)
		{
			LooterUI::SetFocusLit(Light, bLit);
		}
		// Under keys the focus look breathes (the navigator's); under the mouse the light holds steady.
		if (Light && bLit && !bKeysDrive)
		{
			LooterUI::SetFocusGlow(Light, 1.f);
		}
	}

	// Words set before the card was first drawn are measured now (they can't be until it is on screen).
	if (!bHelpCardSized)
	{
		SizeHelpCard();
	}

	// The tabs' looks follow the pointer.
	RefreshTabLooks();
}

// USettingsMenuWidget's round 3 motion (the contract's lane P): the plate drops in and settles as it opens (c1) while the
// halo spins up a quarter turn (c3); the brass ribbon slides to the row marked (the focus's under keys, the pointer's under
// the mouse) and the gold mark slides under the lit tab, each in about 0.09 s; Resume's reticle turns slowly while it is
// the row marked. The pieces are SettingsMenuLedger.cpp's.

#include "UI/Menus/SettingsMenuWidget.h"
#include "UI/Menus/SettingsMenuArt.h"
#include "UI/Menus/SettingsMenuParts.h"
#include "UI/Style/LooterButton.h"
#include "UI/Style/LooterReticleArt.h"
#include "UI/Style/LooterUIStyle.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"

using namespace SettingsMenu;

namespace
{
	/** The open: the plate drops the last OpenDrop px in OpenDropSeconds (c1); the halo spins up its last quarter (c3). */
	constexpr float OpenDrop = 16.f;
	constexpr float OpenDropSeconds = 0.18f;
	constexpr float OpenSpinDegrees = 90.f;
	constexpr float OpenSpinSeconds = 0.25f;
	/** The ribbon and the tab mark ease to theirs: 95% of the way in three of these (0.09 s, the contract's slide). */
	constexpr float SlideEase = 0.03f;
	constexpr float RibbonFadeSeconds = 0.12f;
	/** Resume's reticle turns once in this long while Resume is the row marked (c3). */
	constexpr float ReticleTurnSeconds = 8.f;

	/** Child's box in Space's local space, from the geometry they were last drawn with; false before they're drawn. */
	bool LocalBox(const UWidget& Child, const UWidget& Space, FVector2D& OutTopLeft, FVector2D& OutSize)
	{
		const FGeometry& ChildGeometry = Child.GetCachedGeometry();
		const FGeometry& SpaceGeometry = Space.GetCachedGeometry();
		const FVector2D ChildSize(ChildGeometry.GetLocalSize());
		if (ChildSize.IsNearlyZero() || FVector2D(SpaceGeometry.GetLocalSize()).IsNearlyZero())
		{
			return false;
		}
		const FVector2D TopLeft(SpaceGeometry.AbsoluteToLocal(FVector2D(ChildGeometry.LocalToAbsolute(FVector2D::ZeroVector))));
		const FVector2D BottomRight(SpaceGeometry.AbsoluteToLocal(FVector2D(ChildGeometry.LocalToAbsolute(ChildSize))));
		OutTopLeft = TopLeft;
		OutSize = BottomRight - TopLeft;
		return true;
	}
}

void USettingsMenuWidget::BeginOpenMotion()
{
	OpenAge = 0.f;
	bRibbonPlaced = false;
	bTabMarkPlaced = false;
	RibbonOpacity = 0.f;
	if (Ribbon)
	{
		Ribbon->SetRenderOpacity(0.f);
	}
	MarkRow(DefaultMarkedRow());
	// The first frame shows the plate up and the halo turned back, before any tick has run.
	AdvanceLook(0.f);
}

UWidget* USettingsMenuWidget::DefaultMarkedRow() const
{
	if (Mode == ESettingsMenuMode::Pause && ResumeButton && ResumeButton->IsVisible())
	{
		return ResumeButton;
	}
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
		// Keys or a gamepad: the focus's row (a page's row, Resume); the footer and nothing have no ribbon.
		const FMenuNavItem* Here = Nav.GetFocused();
		if (Here && Here->Row.IsValid())
		{
			return Here->Row.Get();
		}
		if (Here && Here->GetWidget() == ResumeButton)
		{
			return ResumeButton;
		}
		bOutHide = true;
		return nullptr;
	}
	// The mouse: the row under the pointer; between rows (or off them) the ribbon stays where it was.
	for (const FSettingsLedgerLine& Line : LedgerLines)
	{
		if (Line.Page == CurrentPage && Line.Row.IsValid() && Line.Row->IsHovered())
		{
			return Line.Row.Get();
		}
	}
	if (ResumeButton && ResumeButton->IsVisible() && ResumeButton->IsHovered())
	{
		return ResumeButton;
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
		UTextBlock* Number = Line.Number.Get();
		const UWidget* LineRow = Line.Row.Get();
		if (Number && LineRow && (LineRow == Before || LineRow == Row))
		{
			Number->SetColorAndOpacity(FSlateColor(LineRow == Row ? Color::SoulGold() : Color::SlateIcon()));
		}
	}
}

void USettingsMenuWidget::AdvanceLook(float DeltaSeconds)
{
	const float Delta = FMath::Clamp(DeltaSeconds, 0.f, 0.1f);
	const float Before = OpenAge;
	OpenAge += Delta;

	// The open: the plate drops its last 16 px and settles (c1), the halo and its gold arcs spin up a quarter turn (c3).
	if (Before <= FMath::Max(OpenDropSeconds, OpenSpinSeconds) || Delta <= 0.f)
	{
		const float Drop = FMath::Clamp(OpenAge / OpenDropSeconds, 0.f, 1.f);
		const float Spin = FMath::Clamp(OpenAge / OpenSpinSeconds, 0.f, 1.f);
		if (Panel)
		{
			Panel->SetRenderTranslation(FVector2D(0.f, -OpenDrop * FMath::Pow(1.f - Drop, 3.f)));
		}
		const float Angle = -OpenSpinDegrees * FMath::Pow(1.f - Spin, 3.f);
		if (HaloRing)
		{
			HaloRing->SetRenderTransformAngle(Angle);
		}
		if (HaloGold)
		{
			HaloGold->SetRenderTransformAngle(Angle);
		}
	}
	const float Follow = Delta > 0.f ? 1.f - FMath::Exp(-Delta / SlideEase) : 0.f;

	// The ribbon: to the row marked. A row left on another page (a tab clicked) hands it to the new page's start.
	bool bHide = false;
	UWidget* Wanted = FindMarkedRow(bHide);
	if (!Wanted && !bHide)
	{
		const FSettingsLedgerLine* Marked = LedgerLines.FindByPredicate([this](const FSettingsLedgerLine& Line) { return Line.Row.Get() == MarkedRow.Get(); });
		if (!MarkedRow.IsValid() || (Marked && Marked->Page != CurrentPage))
		{
			Wanted = DefaultMarkedRow();
		}
	}
	if (Wanted && Wanted != MarkedRow.Get())
	{
		MarkRow(Wanted);
	}
	// Resume wears its own reticle, and the halo holds that corner of the plate: the ribbon is the ledger rows' alone.
	UWidget* Target = bHide || MarkedRow.Get() == ResumeButton ? nullptr : MarkedRow.Get();
	bool bShow = false;
	float TargetY = RibbonY;
	FVector2D TopLeft;
	FVector2D Size;
	if (Target && Plate && Target->IsVisible() && LocalBox(*Target, *Plate, TopLeft, Size))
	{
		TargetY = TopLeft.Y + Size.Y * 0.5f;
		bShow = true;
		// A page's row scrolled out of the list's view: the ribbon waits out of sight.
		FVector2D ListTopLeft;
		FVector2D ListSize;
		if (Body && LocalBox(*Body, *Plate, ListTopLeft, ListSize))
		{
			bShow = TargetY >= ListTopLeft.Y && TargetY <= ListTopLeft.Y + ListSize.Y;
		}
	}
	if (bShow && !bRibbonPlaced)
	{
		RibbonY = TargetY;
		bRibbonPlaced = true;
	}
	RibbonY = FMath::Lerp(RibbonY, TargetY, Follow);
	if (Delta > 0.f)
	{
		RibbonOpacity = FMath::FInterpConstantTo(RibbonOpacity, bShow ? 1.f : 0.f, Delta, 1.f / RibbonFadeSeconds);
	}
	if (Ribbon)
	{
		if (UCanvasPanelSlot* RibbonSlot = Cast<UCanvasPanelSlot>(Ribbon->Slot))
		{
			RibbonSlot->SetPosition(FVector2D(SettingsMenuArt::RibbonLeft, RibbonY - SettingsMenuArt::RibbonHeight * 0.5f));
		}
		Ribbon->SetRenderOpacity(RibbonOpacity);
	}

	// The tab mark: the ruler's gold run and its triangle slide under the lit tab.
	ULooterButton* LitTab = TabButtons.IsValidIndex(CurrentPage) ? TabButtons[CurrentPage].Get() : nullptr;
	FVector2D TabTopLeft;
	FVector2D TabSize;
	FVector2D RulerTopLeft;
	FVector2D RulerSize;
	if (TabMark && TabBar && TabRuler && LitTab && LocalBox(*LitTab, *TabBar, TabTopLeft, TabSize) && LocalBox(*TabRuler, *TabBar, RulerTopLeft, RulerSize))
	{
		const float X = TabTopLeft.X + TabSize.X * 0.5f;
		if (!bTabMarkPlaced)
		{
			TabMarkX = X;
			bTabMarkPlaced = true;
		}
		TabMarkX = FMath::Lerp(TabMarkX, X, Follow);
		if (UCanvasPanelSlot* MarkSlot = Cast<UCanvasPanelSlot>(TabMark->Slot))
		{
			MarkSlot->SetPosition(FVector2D(TabMarkX - SettingsMenuArt::TabMarkWidth * 0.5f, RulerTopLeft.Y + LooterReticle::RulerLineY(false) - 2.f));
		}
		if (TabMark->GetVisibility() != ESlateVisibility::HitTestInvisible)
		{
			TabMark->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
	}

	// Resume's reticle turns slowly while Resume is the row marked.
	if (ResumeReticle && Delta > 0.f && MarkedRow.Get() == ResumeButton)
	{
		ReticleAngle = FMath::Fmod(ReticleAngle + Delta * 360.f / ReticleTurnSeconds, 360.f);
		ResumeReticle->SetRenderTransformAngle(ReticleAngle);
	}
}

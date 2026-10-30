// UPauseMenuWidget's motion, the settings menu's round 3 motion (SettingsMenuLedgerMotion.cpp): the plate drops in and
// settles as it opens (c1) while the halo spins up a quarter turn (c3); the row marked (the focus's under keys, the
// pointer's under the mouse) lights gold and c1's brass ribbon slides to it in about 0.09 s; Resume's reticle turns slowly
// while Resume is the row marked.

#include "UI/Menus/PauseMenuWidget.h"
#include "UI/Menus/SettingsMenuArt.h"
#include "UI/Style/LooterButton.h"
#include "UI/Style/LooterUIStyle.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"

namespace
{
	/** The open: the plate drops the last OpenDrop px in OpenDropSeconds (c1); the halo spins up its last quarter (c3). */
	constexpr float OpenDrop = 16.f;
	constexpr float OpenDropSeconds = 0.18f;
	constexpr float OpenSpinDegrees = 90.f;
	constexpr float OpenSpinSeconds = 0.25f;
	/** The ribbon eases to its row: 95% of the way in three of these (0.09 s, the settings' slide). */
	constexpr float SlideEase = 0.03f;
	constexpr float RibbonFadeSeconds = 0.12f;
	/** Resume's reticle turns once in this long while Resume is the row marked (c3). */
	constexpr float ReticleTurnSeconds = 8.f;

	/** Child's box in Space's local space, from the geometry they were last drawn with; false before they're drawn. */
	bool HubLocalBox(const UWidget& Child, const UWidget& Space, FVector2D& OutTopLeft, FVector2D& OutSize)
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

void UPauseMenuWidget::BeginOpenMotion()
{
	OpenAge = 0.f;
	bRibbonPlaced = false;
	RibbonOpacity = 0.f;
	if (Ribbon)
	{
		Ribbon->SetRenderOpacity(0.f);
	}
	// Marked from the start: the row it opens on (Resume over the game; the row it came back from).
	MarkButton(GetItemButton(OpenFocus));
	// The first frame shows the plate up and the halo turned back, before any tick has run.
	AdvanceLook(0.f);
}

const FPauseMenuLine* UPauseMenuWidget::FindLine(const ULooterButton* Button) const
{
	return Button ? Lines.FindByPredicate([Button](const FPauseMenuLine& Line) { return Line.Button.Get() == Button; }) : nullptr;
}

EPauseMenuItem UPauseMenuWidget::GetMarkedItem() const
{
	const FPauseMenuLine* Line = FindLine(MarkedButton.Get());
	return Line ? Line->Item : EPauseMenuItem::Count;
}

ULooterButton* UPauseMenuWidget::FindMarkedButton() const
{
	if (Nav.IsShown())
	{
		// Keys or a gamepad: the focus's row.
		return Cast<ULooterButton>(Nav.GetFocusedWidget());
	}
	// The mouse: the row under the pointer; between rows (or off them) the mark stays where it was.
	for (const FPauseMenuLine& Line : Lines)
	{
		ULooterButton* Button = Line.Button.Get();
		if (Button && Button->IsHovered())
		{
			return Button;
		}
	}
	return nullptr;
}

void UPauseMenuWidget::MarkButton(ULooterButton* Button)
{
	using namespace LooterUI;
	MarkedButton = Button;
	// The ledger row marked wears its gold wash and double rule, its number and word gold (c1's selected row); the others
	// their ledger ink. Resume and the stamp show it in their own pieces (they read their button as they paint).
	for (const FPauseMenuLine& Line : Lines)
	{
		const bool bMarked = Button && Line.Button.Get() == Button;
		if (UWidget* Light = Line.Light.Get())
		{
			const ESlateVisibility Wanted = bMarked ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;
			if (Light->GetVisibility() != Wanted)
			{
				Light->SetVisibility(Wanted);
			}
		}
		if (UTextBlock* Number = Line.Number.Get())
		{
			Number->SetColorAndOpacity(FSlateColor(bMarked ? Color::SoulGold() : Color::SlateIcon()));
		}
		if (UTextBlock* Word = Line.Word.Get(); Word && Line.Light.IsValid())
		{
			Word->SetColorAndOpacity(FSlateColor(bMarked ? Color::SoulGold() : Color::SlateText()));
		}
		if (UTextBlock* Detail = Line.Detail.Get())
		{
			Detail->SetColorAndOpacity(FSlateColor(bMarked ? Color::SlateText() : Color::SlateLabel()));
		}
	}
}

void UPauseMenuWidget::AdvanceLook(float DeltaSeconds)
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

	// The row marked follows the focus or the pointer.
	ULooterButton* Wanted = FindMarkedButton();
	if (Wanted && Wanted != MarkedButton.Get())
	{
		MarkButton(Wanted);
	}

	// The ribbon is the ledger rows' alone: Resume wears its own reticle under the halo, the stamp its own red.
	const FPauseMenuLine* Marked = FindLine(MarkedButton.Get());
	ULooterButton* Target = Marked && Marked->Light.IsValid() ? Marked->Button.Get() : nullptr;
	bool bShow = false;
	float TargetY = RibbonY;
	FVector2D TopLeft;
	FVector2D Size;
	if (Target && Plate && HubLocalBox(*Target, *Plate, TopLeft, Size))
	{
		TargetY = TopLeft.Y + Size.Y * 0.5f;
		bShow = true;
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

	// Resume's reticle turns slowly while Resume is the row marked.
	if (ResumeReticle && Delta > 0.f && GetMarkedItem() == EPauseMenuItem::Resume)
	{
		ReticleAngle = FMath::Fmod(ReticleAngle + Delta * 360.f / ReticleTurnSeconds, 360.f);
		ResumeReticle->SetRenderTransformAngle(ReticleAngle);
	}
}

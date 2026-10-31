// UPauseMenuWidget's motion (the contract's "Motion"): as it opens the game dims, the dial swings up from below round its
// centre, the options' rows slide in one after another and the cards fade in; back from the settings page it only fades
// up quickly. The option marked (the focus's under keys, the pointer's under the mouse) lights gold and slides out, its
// callout comes up beside it, and the needle swings to its tick in about 0.18 s with a slight overshoot
// (PauseMenuLayout::NeedleSwing), the small reticle riding the arc with it and turning slowly.

#include "UI/Menus/PauseMenuWidget.h"
#include "UI/Menus/PauseMenuLayout.h"
#include "UI/Style/LooterButton.h"
#include "UI/Style/LooterReticleArt.h"
#include "UI/Style/LooterUIStyle.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

using namespace PauseMenuLayout;

namespace
{
	/** The opening's parts: when each starts and how long it takes (seconds), and how far it moves. */
	struct FPhase
	{
		float Delay;
		float Length;
	};
	constexpr FPhase BackdropPhase{ 0.f, 0.2f };
	constexpr FPhase TitlePhase{ 0.04f, 0.3f };
	constexpr FPhase RulerPhase{ 0.1f, 0.4f };
	constexpr FPhase DialPhase{ 0.f, 0.42f };
	constexpr FPhase DialFadePhase{ 0.f, 0.18f };
	constexpr FPhase HeroPhase{ 0.16f, 0.3f };
	constexpr FPhase SessionPhase{ 0.2f, 0.3f };
	constexpr FPhase MissionPhase{ 0.22f, 0.3f };
	constexpr FPhase KeysPhase{ 0.26f, 0.3f };
	/** The rows slide in one after another. */
	constexpr float RowFirstDelay = 0.1f;
	constexpr float RowStagger = 0.06f;
	constexpr float RowLength = 0.26f;
	constexpr float RowSlide = 36.f;
	/** The dial comes up from below: turned this far down round its centre, and this much lower, at its start. */
	constexpr float DialTurn = 24.f;
	constexpr float DialRise = 40.f;
	/** The cards settle this far as they fade in. */
	constexpr float CardRise = 12.f;
	/** Back from a page: everything at once, this quick. */
	constexpr float ReturnLength = 0.14f;
	/** The needle's first swing waits for the dial to come up a little. */
	constexpr float OpenSwingDelay = 0.12f;

	/** A tag lights or goes out in about 0.1 s (95% in three of these); its callout slides this far in as it comes. */
	constexpr float LitEase = 0.035f;
	constexpr float CalloutSlide = 14.f;
	/** The reticle on the lit tick turns once in this long (the main menu's). */
	constexpr float ReticleTurnSeconds = 8.f;

	float EaseOut(float Alpha)
	{
		const float Rest = 1.f - FMath::Clamp(Alpha, 0.f, 1.f);
		return 1.f - Rest * Rest * Rest;
	}

	void SetOpacity(UWidget* Widget, float Opacity)
	{
		if (Widget && Widget->GetRenderOpacity() != Opacity)
		{
			Widget->SetRenderOpacity(Opacity);
		}
	}

	/** A group fading in and settling Rise px up (Rise < 0: from above). */
	void Arrive(UWidget* Widget, float Alpha, float Rise)
	{
		if (Widget)
		{
			SetOpacity(Widget, Alpha);
			Widget->SetRenderTranslation(FVector2D(0.f, Rise * (1.f - Alpha)));
		}
	}

	UCanvasPanelSlot* CanvasSlotOf(const UWidget* Widget)
	{
		return Widget ? Cast<UCanvasPanelSlot>(Widget->Slot) : nullptr;
	}
}

void UPauseMenuWidget::BeginOpenMotion()
{
	OpenAge = 0.f;
	// Lit from the start: the option it opens on (Back to Game over the game; Settings back from the settings page).
	ULooterButton* First = GetItemButton(OpenFocus);
	MarkButton(First);
	for (FPauseMenuOption& Option : Options)
	{
		Option.Lit = Option.Button.Get() == First ? 1.f : 0.f;
	}
	const FPauseMenuOption* Lit = FindOption(First);
	NeedleTo = Lit ? RowDegrees(Lit->Row) : RowDegrees(0);
	if (bOpenReturning)
	{
		// Back from a page: the needle is where it was.
		NeedleFrom = NeedleTo;
		SwingLength = 0.f;
	}
	else
	{
		// Over the game: the needle swings up from pointing down past the last option, as the dial comes up.
		NeedleFrom = OpenSwingFrom;
		SwingLength = OpenSwingSeconds;
		SwingDelay = OpenSwingDelay;
	}
	SwingAge = 0.f;
	NeedleNow = NeedleFrom;
	// The first frame shows the opening's start before any tick has run.
	AdvanceLook(0.f);
}

EPauseMenuItem UPauseMenuWidget::GetMarkedItem() const
{
	const FPauseMenuOption* Option = FindOption(MarkedButton.Get());
	return Option ? Option->Item : EPauseMenuItem::Count;
}

ULooterButton* UPauseMenuWidget::FindMarkedButton() const
{
	if (Nav.IsShown())
	{
		// Keys or a gamepad: the focus's option.
		return Cast<ULooterButton>(Nav.GetFocusedWidget());
	}
	// The mouse: the option under the pointer; off them the mark stays where it was.
	for (const FPauseMenuOption& Option : Options)
	{
		ULooterButton* Button = Option.Button.Get();
		if (Button && Button->IsHovered())
		{
			return Button;
		}
	}
	return nullptr;
}

void UPauseMenuWidget::MarkButton(ULooterButton* Button)
{
	MarkedButton = Button;
	const FPauseMenuOption* Option = FindOption(Button);
	if (!Option)
	{
		return;
	}
	// The needle swings from where it is now to the option's tick: one click per option.
	const float Target = RowDegrees(Option->Row);
	if (!FMath::IsNearlyEqual(Target, NeedleTo))
	{
		NeedleFrom = NeedleNow;
		NeedleTo = Target;
		SwingAge = 0.f;
		SwingDelay = 0.f;
		SwingLength = SwingSeconds;
	}
}

void UPauseMenuWidget::AdvanceLook(float DeltaSeconds)
{
	using namespace LooterUI;
	const float Delta = FMath::Clamp(DeltaSeconds, 0.f, 0.1f);
	OpenAge += Delta;

	// Where each part of the opening is (0 to 1); back from a page, all of them at once and quick.
	auto Phase = [this](float Delay, float Length)
	{
		if (bOpenReturning)
		{
			return EaseOut(OpenAge / ReturnLength);
		}
		return EaseOut((OpenAge - Delay) / Length);
	};
	auto At = [&Phase](const FPhase& Part) { return Phase(Part.Delay, Part.Length); };

	// Back from a page the game stays dimmed (the page was over it): only the hub's pieces fade up.
	SetOpacity(Backdrop, bOpenReturning ? 1.f : At(BackdropPhase));
	Arrive(TitleGroup, At(TitlePhase), -CardRise);
	if (TitleRuler)
	{
		TitleRuler->SetRenderScale(FVector2D(FMath::Max(At(RulerPhase), 0.001f), 1.f));
	}
	if (DialGroup)
	{
		// The dial swings up round its own centre from below.
		const float Swing = At(DialPhase);
		SetOpacity(DialGroup, At(DialFadePhase));
		DialGroup->SetRenderTransformAngle(DialTurn * (1.f - Swing));
		DialGroup->SetRenderTranslation(FVector2D(0.f, DialRise * (1.f - Swing)));
	}
	Arrive(HeroGroup, At(HeroPhase), CardRise);
	Arrive(MissionGroup, At(MissionPhase), CardRise);
	Arrive(SessionGroup, At(SessionPhase), CardRise);
	Arrive(KeysGroup, At(KeysPhase), CardRise);

	// The option marked follows the focus or the pointer.
	ULooterButton* Wanted = FindMarkedButton();
	if (Wanted && Wanted != MarkedButton.Get())
	{
		MarkButton(Wanted);
	}

	// Each row slides in, then its tag eases toward lit or not.
	const float Follow = Delta > 0.f ? 1.f - FMath::Exp(-Delta / LitEase) : 0.f;
	for (FPauseMenuOption& Option : Options)
	{
		const float Arrived = Phase(RowFirstDelay + RowStagger * Option.Row, RowLength);
		if (UWidget* Row = Option.RowGroup.Get())
		{
			SetOpacity(Row, Arrived);
			Row->SetRenderTranslation(FVector2D(-RowSlide * (1.f - Arrived), 0.f));
		}
		const float Target = Option.Button.IsValid() && Option.Button.Get() == MarkedButton.Get() ? 1.f : 0.f;
		Option.Lit = FMath::Lerp(Option.Lit, Target, Follow);
		if (FMath::Abs(Option.Lit - Target) < 0.002f)
		{
			Option.Lit = Target;
		}
		ApplyLit(Option);
		// The callout comes with its row and its light.
		if (UWidget* Callout = Option.Callout.Get())
		{
			const float Shown = Option.Lit * Arrived;
			SetOpacity(Callout, Shown);
			Callout->SetRenderTranslation(FVector2D(-CalloutSlide * (1.f - Option.Lit), 0.f));
			const ESlateVisibility Visible = Shown > 0.001f ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;
			if (Callout->GetVisibility() != Visible)
			{
				Callout->SetVisibility(Visible);
			}
		}
	}

	// The needle's swing, and the reticle turning slowly on the lit tick.
	SwingAge += Delta;
	if (SwingLength <= 0.f)
	{
		NeedleNow = NeedleTo;
	}
	else
	{
		NeedleNow = FMath::Lerp(NeedleFrom, NeedleTo, NeedleSwing((SwingAge - SwingDelay) / SwingLength));
	}
	ReticleAngle = FMath::Fmod(ReticleAngle + Delta * 360.f / ReticleTurnSeconds, 360.f);
	ApplyNeedle();
}

void UPauseMenuWidget::ApplyLit(FPauseMenuOption& Option)
{
	using namespace LooterUI;
	const float Lit = Option.Lit;
	const FLinearColor BaseLine = Option.bDanger ? Color::Worse() : Color::SaltLine();
	const FLinearColor Gold = Color::SoulGold();

	// The tag: its plain look gives way to the gold one, it slides out a little, its gem, icon and words turn gold.
	SetOpacity(Option.PlainLook.Get(), 1.f - Lit);
	SetOpacity(Option.LitLook.Get(), Lit);
	SetOpacity(Option.PlainGem.Get(), 1.f - Lit);
	SetOpacity(Option.LitGem.Get(), Lit);
	if (ULooterButton* Button = Option.Button.Get())
	{
		Button->SetRenderTranslation(FVector2D(TagLitSlide * Lit, 0.f));
	}
	if (UImage* Icon = Option.Icon.Get())
	{
		Icon->SetColorAndOpacity(FMath::Lerp(BaseLine, Gold, Lit));
	}
	if (UTextBlock* Word = Option.Word.Get())
	{
		Word->SetColorAndOpacity(FSlateColor(FMath::Lerp(Option.bDanger ? Color::Worse() : Color::SlateText(), Gold, Lit)));
	}
	if (UTextBlock* Number = Option.Number.Get())
	{
		Number->SetColorAndOpacity(FSlateColor(FMath::Lerp(Option.bDanger ? Color::Worse() : Color::SlateLabel(), Gold, Lit)));
		Number->SetRenderTranslation(FVector2D((NumberLitGap - NumberGap) * Lit, 0.f));
	}

	// The pegs: the dash across the arc gives way to the reticle; the short peg starts clear of it; the long one follows the
	// tag out and turns gold.
	const float Tick = RowArcPoint(Option.Row).X;
	SetOpacity(Option.Dash.Get(), 1.f - Lit);
	if (UImage* ShortPeg = Option.ShortPeg.Get())
	{
		const float From = Tick + FMath::Lerp(DashHalf, NumberLitGap - 14.f, Lit);
		if (UCanvasPanelSlot* PegSlot = CanvasSlotOf(ShortPeg))
		{
			PegSlot->SetPosition(FVector2D(From, PegSlot->GetPosition().Y));
			PegSlot->SetSize(FVector2D(FMath::Max(Tick + ShortPegTo - From, 0.f), PegSlot->GetSize().Y));
		}
		ShortPeg->SetColorAndOpacity(FMath::Lerp(BaseLine.CopyWithNewOpacity(0.8f), Gold, Lit));
	}
	const float LongFrom = Tick + LongPegFrom;
	const float LongLength = FMath::Max(TagX + 2.f + TagLitSlide * Lit - LongFrom, 0.f);
	if (UImage* LongPeg = Option.LongPeg.Get())
	{
		if (UCanvasPanelSlot* PegSlot = CanvasSlotOf(LongPeg))
		{
			PegSlot->SetSize(FVector2D(LongLength, PegSlot->GetSize().Y));
		}
		LongPeg->SetColorAndOpacity(FMath::Lerp(BaseLine.CopyWithNewOpacity(0.7f), Gold, Lit));
	}
	if (UCanvasPanelSlot* EdgeSlot = CanvasSlotOf(Option.LongPegEdge.Get()))
	{
		EdgeSlot->SetSize(FVector2D(LongLength + 3.f, EdgeSlot->GetSize().Y));
	}
}

void UPauseMenuWidget::ApplyNeedle()
{
	if (Needle)
	{
		Needle->SetRenderTransformAngle(NeedleNow);
	}
	if (Reticle)
	{
		// On the range arc where the needle aims (it rides the arc as the needle swings), turning slowly.
		const float Angle = FMath::DegreesToRadians(NeedleNow);
		const FVector2D OnArc = FVector2D(DialX, DialY) + ArcRadius * FVector2D(FMath::Cos(Angle), FMath::Sin(Angle));
		const float Extent = LooterReticle::SmallReticleExtent(ReticleRadius);
		if (UCanvasPanelSlot* ReticleSlot = CanvasSlotOf(Reticle))
		{
			ReticleSlot->SetPosition(OnArc - FVector2D(Extent, Extent));
		}
		Reticle->SetRenderTransformAngle(ReticleAngle);
		SetOpacity(Reticle, MarkedButton.IsValid() ? 1.f : 0.f);
	}
}

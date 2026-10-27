// UMainMenuWidget's motion: the menu arriving (the shade, the title, the buttons one after another), a main button's lit
// look easing in (its tick rising, its wash, its words turning orange and stepping right), the mark's ember breathing and
// a new page fading in. All render opacity and transforms: nothing here changes the layout or what the focus can reach.

#include "UI/Menus/MainMenuWidget.h"
#include "UI/Style/LooterButton.h"
#include "UI/Style/LooterUIStyle.h"
#include "Audio/LooterSound.h"
#include "Components/TextBlock.h"
#include "HAL/IConsoleManager.h"

using namespace LooterUI;

namespace
{
	TAutoConsoleVariable<float> CVarRevealSpeed(TEXT("Looter.Menu.Reveal.Speed"), 1.f,
		TEXT("How fast the main menu arrives (2: twice as fast; 0: at once)."));
	TAutoConsoleVariable<float> CVarLitRate(TEXT("Looter.Menu.Lit.Rate"), 14.f,
		TEXT("How fast a main menu button's lit look (tick, wash, orange words) eases in and out: the share closed a second."));
	TAutoConsoleVariable<float> CVarPageFade(TEXT("Looter.Menu.Page.Fade"), 0.3f,
		TEXT("Seconds a new page of the main menu (the buttons, the session picker) takes to fade in."));
	TAutoConsoleVariable<float> CVarEmberPeriod(TEXT("Looter.Menu.Ember.Period"), 4.f,
		TEXT("Seconds of one breath of the ember's glow beside the main menu's title."));

	/** The menu arriving (Art/Sounds/recipes/opening.py): air drawn up, the bell struck softly as the title lands. */
	const FName CueReveal(TEXT("UI.Menu.Reveal"));

	/** Each part's own fade and slide once its turn comes. */
	constexpr float PartFade = 0.55f;
	constexpr float ShadeFade = 0.6f;
	/** The orange rule under the title draws out once the title has landed. */
	constexpr float RuleDelay = 0.5f;
	constexpr float RuleGrow = 0.5f;
	/** The ember's glow breathes between these (never out: it's lit). */
	constexpr float GlowLow = 0.3f;
	constexpr float GlowHigh = 0.62f;
	/** A lit button's words step this far right; its tick rises from this share of its height. */
	constexpr float LitStep = 8.f;
	constexpr float TickRest = 0.3f;
	/** A disabled button's words (Multiplayer, SOON): dim. */
	constexpr float DisabledOpacity = 0.5f;
	/** A new page slides in this far as it fades. */
	constexpr float PageSlide = 14.f;

	float EaseOut(float Alpha)
	{
		const float A = FMath::Clamp(Alpha, 0.f, 1.f);
		return 1.f - FMath::Cube(1.f - A);
	}

	float RevealLength(float LastDelay)
	{
		return FMath::Max3(LastDelay + PartFade, RuleDelay + RuleGrow, ShadeFade);
	}
}

void UMainMenuWidget::HoldReveal()
{
	bRevealHeld = true;
	bRevealStarted = false;
	RevealAge = 0.f;
	ApplyReveal();
}

void UMainMenuWidget::BeginReveal()
{
	bRevealHeld = false;
	bRevealStarted = true;
	RevealAge = 0.f;
	LooterSound::Play2D(this, CueReveal);
	ApplyReveal();
}

bool UMainMenuWidget::IsRevealed() const
{
	float LastDelay = 0.f;
	for (const FRevealPart& Part : RevealParts)
	{
		LastDelay = FMath::Max(LastDelay, Part.Delay);
	}
	return bRevealStarted && RevealAge >= RevealLength(LastDelay);
}

void UMainMenuWidget::AddRevealPart(UWidget* Widget, float Delay, float Slide)
{
	if (Widget)
	{
		RevealParts.Add(FRevealPart{ Widget, Delay, Slide });
	}
}

void UMainMenuWidget::AdvanceLook(float DeltaSeconds)
{
	// A menu nobody held (no startup cards over it) arrives on its first frame.
	if (!bRevealStarted && !bRevealHeld)
	{
		BeginReveal();
	}
	if (bRevealStarted)
	{
		const float Speed = CVarRevealSpeed.GetValueOnGameThread();
		RevealAge = Speed > 0.f ? RevealAge + DeltaSeconds * Speed : 1000.f;
	}
	LookClock += DeltaSeconds;
	PageAge += DeltaSeconds;
	ApplyReveal();
	ApplyRows(DeltaSeconds);
	ApplyPage();
}

void UMainMenuWidget::ApplyReveal()
{
	const float Age = bRevealStarted ? RevealAge : 0.f;
	if (Shade)
	{
		// Render opacity: the shade's own colour is the transparency setting's (MarkBackground on a border).
		Shade->SetRenderOpacity(EaseOut(Age / ShadeFade));
	}
	for (const FRevealPart& Part : RevealParts)
	{
		if (UWidget* Widget = Part.Widget.Get())
		{
			const float Alpha = EaseOut((Age - Part.Delay) / PartFade);
			Widget->SetRenderOpacity(Alpha);
			Widget->SetRenderTranslation(FVector2D(-Part.Slide * (1.f - Alpha), 0.f));
		}
	}
	if (TitleRule)
	{
		TitleRule->SetRenderScale(FVector2D(EaseOut((Age - RuleDelay) / RuleGrow), 1.f));
	}
	if (TitleGlow)
	{
		const float Period = FMath::Max(CVarEmberPeriod.GetValueOnGameThread(), 0.5f);
		const float Breath = 0.5f + 0.5f * FMath::Sin(UE_TWO_PI * LookClock / Period);
		TitleGlow->SetRenderOpacity(FMath::Lerp(GlowLow, GlowHigh, Breath));
	}
}

void UMainMenuWidget::ApplyRows(float DeltaSeconds)
{
	// Under keys or a controller the focus lights a button; under the mouse, the pointer over it.
	const bool bKeys = Nav.IsShown();
	const float Step = FMath::Clamp(CVarLitRate.GetValueOnGameThread() * DeltaSeconds, 0.f, 1.f);
	for (FMainRow& Row : MainRows)
	{
		const ULooterButton* Button = Row.Button.Get();
		if (!Button)
		{
			continue;
		}
		const bool bEnabled = Button->GetIsEnabled();
		const bool bLit = bEnabled && (bKeys ? Button->IsFocusLit() : Button->IsHovered());
		Row.Lit += ((bLit ? 1.f : 0.f) - Row.Lit) * Step;
		if (FMath::Abs(Row.Lit - (bLit ? 1.f : 0.f)) < 0.002f)
		{
			Row.Lit = bLit ? 1.f : 0.f;
		}
		if (UWidget* TickBar = Row.Tick.Get())
		{
			TickBar->SetRenderOpacity(Row.Lit);
			TickBar->SetRenderScale(FVector2D(1.f, FMath::Lerp(TickRest, 1.f, Row.Lit)));
		}
		if (UWidget* Wash = Row.Wash.Get())
		{
			Wash->SetRenderOpacity(Row.Lit);
		}
		if (UTextBlock* Label = Row.Label.Get())
		{
			const FLinearColor Words = bEnabled ? FMath::Lerp(Color::Text(), Row.LitColor, Row.Lit)
				: Color::TextDim().CopyWithNewOpacity(DisabledOpacity);
			Label->SetColorAndOpacity(FSlateColor(Words));
			Label->SetRenderTranslation(FVector2D(LitStep * EaseOut(Row.Lit), 0.f));
		}
		if (UTextBlock* Number = Row.Number.Get())
		{
			const FLinearColor Digits = bEnabled ? FMath::Lerp(Color::TextDim(), Row.LitColor, Row.Lit)
				: Color::TextDim().CopyWithNewOpacity(DisabledOpacity);
			Number->SetColorAndOpacity(FSlateColor(Digits));
		}
	}
}

void UMainMenuWidget::RestartPage()
{
	PageAge = 0.f;
}

void UMainMenuWidget::ApplyPage()
{
	const float Fade = CVarPageFade.GetValueOnGameThread();
	const float Alpha = Fade > 0.f ? EaseOut(PageAge / Fade) : 1.f;
	UWidget* Page = bPickerShown ? Picker.Get() : MainButtons.Get();
	if (Page)
	{
		Page->SetRenderOpacity(Alpha);
		Page->SetRenderTranslation(FVector2D(-PageSlide * (1.f - Alpha), 0.f));
	}
}

float UMainMenuWidget::GetButtonLit(FName Action) const
{
	for (const FMainRow& Row : MainRows)
	{
		if (Row.Button.IsValid() && Row.Button->Action == Action)
		{
			return Row.Lit;
		}
	}
	return 0.f;
}

// UMainMenuWidget's motion, concept 3 "Halo Reticle" (c3's notes): the menu arriving (the shade; the halo spinning up a
// quarter turn as its ticks light clockwise; the words; the rows one after another), a main button's lit look easing in
// (its words and number turning gold, its gold dash), the small reticle snapping along the range arc to the lit row and
// turning slowly there, and a new page fading in. All render opacity and transforms: nothing here changes the layout or
// what the focus can reach.

#include "UI/Menus/MainMenuWidget.h"
#include "UI/Menus/MainMenuArt.h"
#include "UI/Style/LooterButton.h"
#include "UI/Style/LooterReticleArt.h"
#include "UI/Style/LooterUIStyle.h"
#include "Audio/LooterSound.h"
#include "Components/TextBlock.h"
#include "HAL/IConsoleManager.h"

using namespace LooterUI;

namespace
{
	namespace Layout = MainMenuArt::Layout;

	TAutoConsoleVariable<float> CVarRevealSpeed(TEXT("Looter.Menu.Reveal.Speed"), 1.f,
		TEXT("How fast the main menu arrives (2: twice as fast; 0: at once)."));
	TAutoConsoleVariable<float> CVarLitRate(TEXT("Looter.Menu.Lit.Rate"), 14.f,
		TEXT("How fast a main menu button's lit look (gold words, the reticle showing) eases in and out: the share closed a second."));
	TAutoConsoleVariable<float> CVarPageFade(TEXT("Looter.Menu.Page.Fade"), 0.3f,
		TEXT("Seconds a new page of the main menu (the buttons, the session picker) takes to fade in."));
	TAutoConsoleVariable<float> CVarReticlePeriod(TEXT("Looter.Menu.Reticle.Period"), 8.f,
		TEXT("Seconds of one turn of the small reticle on the main menu's lit row."));
	TAutoConsoleVariable<float> CVarReticleSnap(TEXT("Looter.Menu.Reticle.Snap"), 0.09f,
		TEXT("Seconds the main menu's reticle takes to slide along the arc to a newly lit row."));

	/** The menu arriving (Art/Sounds/recipes/opening.py): air drawn up, the bell struck softly as the title lands. */
	const FName CueReveal(TEXT("UI.Menu.Reveal"));

	/** Each part's own fade and slide once its turn comes. */
	constexpr float PartFade = 0.55f;
	constexpr float ShadeFade = 0.6f;
	/** The ruler under the title draws out once the words are coming. */
	constexpr float RuleDelay = Layout::WordsDelay + 0.1f;
	constexpr float RuleGrow = 0.5f;
	/** The halo's ticks before their turn to light (just seen), and how long each sector takes to come up. */
	constexpr float SectorDim = 0.15f;
	constexpr float SectorLight = 0.08f;
	/** The first sector to light: the one just past twelve o'clock (sector 9 of 12 starts at 270 degrees). */
	constexpr int32 FirstSector = Layout::HaloSectors * 3 / 4;
	/** A disabled button's words (Multiplayer, SOON): dim (the mockup's 0.42), its dash fainter. */
	constexpr float DisabledOpacity = 0.42f;
	constexpr float DisabledDash = 0.4f;
	/** The salt dash across the arc at rest. */
	constexpr float DashOpacity = 0.9f;
	/** A new page slides in this far as it fades. */
	constexpr float PageSlide = 14.f;

	float EaseOut(float Alpha)
	{
		const float A = FMath::Clamp(Alpha, 0.f, 1.f);
		return 1.f - FMath::Cube(1.f - A);
	}

	float RevealLength(float LastDelay)
	{
		const float HaloDone = Layout::HaloDelay + Layout::HaloSpinTime + SectorLight;
		return FMath::Max(FMath::Max3(LastDelay + PartFade, RuleDelay + RuleGrow, ShadeFade), HaloDone);
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
	ApplyHalo(Age);
}

void UMainMenuWidget::ApplyHalo(float Age)
{
	// A quarter turn up to rest, quick then settling, as the sight is brought up.
	if (HaloSpin)
	{
		const float Spin = EaseOut((Age - Layout::HaloDelay) / Layout::HaloSpinTime);
		HaloSpin->SetRenderTransformAngle(-Layout::HaloTurn * (1.f - Spin));
	}
	// Its ticks light clockwise round the ring during the spin, a sector at a time.
	const int32 Count = HaloSectors.Num();
	for (int32 Index = 0; Index < Count; ++Index)
	{
		if (UWidget* Sector = HaloSectors[Index])
		{
			const int32 Turn = (Index - FirstSector + Count) % Count;
			const float LightAt = Layout::HaloDelay + Layout::HaloSpinTime * Turn / Count;
			Sector->SetRenderOpacity(FMath::Lerp(SectorDim, 1.f, EaseOut((Age - LightAt) / SectorLight)));
		}
	}
}

void UMainMenuWidget::ApplyRows(float DeltaSeconds)
{
	// Under keys or a controller the focus lights a button; under the mouse, the pointer over it.
	const bool bKeys = Nav.IsShown();
	const float Step = FMath::Clamp(CVarLitRate.GetValueOnGameThread() * DeltaSeconds, 0.f, 1.f);
	int32 LitIndex = INDEX_NONE;
	for (int32 Index = 0; Index < MainRows.Num(); ++Index)
	{
		FMainRow& Row = MainRows[Index];
		const ULooterButton* Button = Row.Button.Get();
		if (!Button)
		{
			continue;
		}
		const bool bEnabled = Button->GetIsEnabled();
		const bool bLit = bEnabled && (bKeys ? Button->IsFocusLit() : Button->IsHovered());
		if (bLit && LitIndex == INDEX_NONE)
		{
			LitIndex = Index;
		}
		Row.Lit += ((bLit ? 1.f : 0.f) - Row.Lit) * Step;
		if (FMath::Abs(Row.Lit - (bLit ? 1.f : 0.f)) < 0.002f)
		{
			Row.Lit = bLit ? 1.f : 0.f;
		}
		if (UTextBlock* Label = Row.Label.Get())
		{
			const FLinearColor Words = bEnabled ? FMath::Lerp(Color::SlateText(), Color::SoulGold(), Row.Lit)
				: Color::SlateText().CopyWithNewOpacity(DisabledOpacity);
			Label->SetColorAndOpacity(FSlateColor(Words));
		}
		if (UTextBlock* Number = Row.Number.Get())
		{
			Number->SetColorAndOpacity(FSlateColor(FMath::Lerp(Color::SlateLabel(), Color::SoulGold(), Row.Lit)));
		}
		// The salt dash gives way to the gold one (the reticle sits over the arc there).
		if (UWidget* SaltDash = Row.SaltDash.Get())
		{
			SaltDash->SetRenderOpacity(bEnabled ? DashOpacity * (1.f - Row.Lit) : DisabledDash);
		}
		if (UWidget* GoldDash = Row.GoldDash.Get())
		{
			GoldDash->SetRenderOpacity(Row.Lit);
		}
	}
	ApplyReticle(DeltaSeconds, LitIndex);
}

void UMainMenuWidget::ApplyReticle(float DeltaSeconds, int32 LitIndex)
{
	const float Snap = CVarReticleSnap.GetValueOnGameThread();
	auto CurrentY = [this, Snap]()
	{
		return Snap > 0.f ? FMath::Lerp(ReticleFromY, ReticleToY, EaseOut(ReticleAge / Snap)) : ReticleToY;
	};
	const bool bLit = MainRows.IsValidIndex(LitIndex);
	if (bLit && LitIndex != ReticleRow)
	{
		const float Target = MainRows[LitIndex].Y;
		// Shown, it slides from where it is to the new row; coming up from nothing, it starts there.
		ReticleFromY = (ReticleRow == INDEX_NONE || ReticleAlpha < 0.05f) ? Target : CurrentY();
		ReticleToY = Target;
		ReticleAge = 0.f;
		ReticleRow = LitIndex;
	}
	ReticleAge += DeltaSeconds;
	const float Step = FMath::Clamp(CVarLitRate.GetValueOnGameThread() * DeltaSeconds, 0.f, 1.f);
	ReticleAlpha += ((bLit ? 1.f : 0.f) - ReticleAlpha) * Step;
	if (FMath::Abs(ReticleAlpha - (bLit ? 1.f : 0.f)) < 0.002f)
	{
		ReticleAlpha = bLit ? 1.f : 0.f;
	}

	// On the arc at that height: the reticle's middle on the line, the gold run turned round the circle's centre to it.
	const float Y = CurrentY();
	if (ReticleMark)
	{
		const float Extent = LooterReticle::SmallReticleExtent(Layout::ReticleRadius);
		ReticleMark->SetRenderTranslation(FVector2D(Layout::ArcX(Y) - Extent, Y - Extent));
		const float Period = FMath::Max(CVarReticlePeriod.GetValueOnGameThread(), 0.5f);
		ReticleMark->SetRenderTransformAngle(360.f * FMath::Fmod(LookClock / Period, 1.f));
		ReticleMark->SetRenderOpacity(ReticleAlpha);
	}
	if (LitRunMark)
	{
		LitRunMark->SetRenderTransformAngle(Layout::ArcDegrees(Y) - 180.f);
		LitRunMark->SetRenderOpacity(ReticleAlpha);
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

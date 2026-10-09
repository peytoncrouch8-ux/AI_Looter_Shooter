// UHudPlayerFrameWidget's regeneration: the wounds that close (FWoundsClose, ticked by UPlayerVitalsSubsystem) heal a
// sliver every frame. Shown as a heal each time, that would pop a "+0.4" and restart the shine sixty times a second, so
// the frame tells them apart (the vitals say when a run is on): the fill just rises with the health, a softer shine
// sweeps along it every couple of seconds, and the run's total rises once as "+N" when it has filled the bar. A bigger
// jump in one frame (a soul-mote's heal) is not regeneration and takes the heal's own show in HudPlayerFrameWidget.cpp.

#include "UI/HUD/HudPlayerFrameWidget.h"
#include "Combat/PlayerVitalsSubsystem.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

namespace
{
	/** A rise of more than this share of the bar in one frame is a heal (a soul-mote's 15%), not regeneration (12% a second at most). */
	constexpr float RegenJumpShare = 0.05f;
	/** The gentle shine runs at this share of a heal's speed (a sweep of 1.5 s), with this long between sweeps, and fainter. */
	constexpr float RegenShineSpeed = 0.5f;
	constexpr float RegenShineGapSeconds = 0.6f;
	constexpr float RegenShineOpacity = 0.55f;
	/** A run shows its "+N" only if it healed at least this share of the bar (a few points of top-up say nothing), and filled it. */
	constexpr float RunFloatMinShare = 0.1f;
	constexpr float FullShare = 0.995f;
}

bool UHudPlayerFrameWidget::IsRegenerating() const
{
	const UWorld* World = GetWorld();
	const UPlayerVitalsSubsystem* Vitals = World ? World->GetSubsystem<UPlayerVitalsSubsystem>() : nullptr;
	return Vitals && Vitals->IsRegenerating(GetOwningPlayer());
}

bool UHudPlayerFrameWidget::ShowRegenGain(float Gained, float Fraction, float DeltaTime)
{
	if (!Shine || Fraction - LastFraction > RegenJumpShare)
	{
		return false;
	}
	RegenGained += Gained;

	if (RiseTime < HealRiseSeconds)
	{
		// A heal's rise (a soul-mote's) is still easing up: let it finish, aimed at where the health is now.
		FillTo = Fraction;
	}
	else
	{
		// The rise is the health's own, a sliver a frame: the fill simply follows it.
		FillShown = Fraction;
		FillFrom = Fraction;
		FillTo = Fraction;
	}
	Activity = ActivityHoldSeconds;

	// One soft sweep along the fill, a pause, again: the wounds closing, not a heal landing.
	RegenShineWait = FMath::Max(RegenShineWait - DeltaTime, 0.f);
	if (ShineTime >= ShineSeconds && RegenShineWait <= 0.f)
	{
		ShineTime = 0.f;
		ShineSpeed = RegenShineSpeed;
		RegenShineWait = ShineSeconds / RegenShineSpeed + RegenShineGapSeconds;
		Shine->SetRenderOpacity(RegenShineOpacity);
	}
	return true;
}

void UHudPlayerFrameWidget::EndRegenRun(float Fraction, float MaxHealth)
{
	const float Gained = RegenGained;
	RegenGained = 0.f;
	RegenShineWait = 0.f;
	// Only a run that got all the way up from a real wound earns its "+N", and not over a heal's own number still rising.
	if (!HealFloat || Fraction < FullShare || Gained < MaxHealth * RunFloatMinShare || HealFloatTime > 0.f)
	{
		return;
	}
	RaiseHealFloat(Gained);
}

void UHudPlayerFrameWidget::RaiseHealFloat(float Amount)
{
	HealAmount = Amount;
	HealFloat->SetText(FText::FromString(FString::Printf(TEXT("+%d"), FMath::RoundToInt32(Amount))));
	HealFloat->SetVisibility(ESlateVisibility::HitTestInvisible);
	HealFloatTime = HealFloatSeconds;
	PaintFloat(HealFloat, 0.f, HealFloatSeconds);
}

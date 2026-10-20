// UHeroSelectWidget: confirming the pick. The sounds, the beat (the stand's picture lifting into its flaring lights, the
// glow swelling in the hero's colour), then OnConfirmed once; or the refusal for a hero in the works; or, the start
// failing after all, the screen taking keys again.

#include "UI/Menus/HeroSelectWidget.h"
#include "Audio/LooterSound.h"
#include "Audio/LooterSoundCues.h"
#include "Embers/HeroDefinition.h"
#include "UI/Menus/HeroSelectParts.h"
#include "UI/Menus/HeroSelectStage.h"
#include "UI/Style/LooterUIStyle.h"
#include "Components/Image.h"

using namespace LooterUI;
using namespace HeroSelectParts;

namespace
{
	/** The cue that says the ember took (the powers' own "ready" chime), under the equip's clack. */
	const FName EmberReadyCue(TEXT("Ember.Ready"));

	float EaseOut(float Alpha)
	{
		const float Inverse = 1.f - FMath::Clamp(Alpha, 0.f, 1.f);
		return 1.f - Inverse * Inverse * Inverse;
	}
}

bool UHeroSelectWidget::Confirm()
{
	const UHeroDefinition* Hero = GetHero(Selected);
	if (!Hero || IsConfirming())
	{
		return false;
	}
	if (!CanConfirm(Selected))
	{
		LooterSound::Play2D(this, LooterSoundCue::Denied);
		SetStatus(FString::Printf(TEXT("%s is in the works: their model and powers aren't in the game yet."), *Hero->DisplayName.ToString()), Color::Worse());
		return false;
	}
	// The pick takes: the equip's clack and the ember's chime, the hero lifting into the light, then the session starts.
	LooterSound::Play2D(this, LooterSoundCue::Inventory::Equip);
	LooterSound::Play2D(this, EmberReadyCue);
	ConfirmLeft = ConfirmSeconds;
	bConfirmSent = false;
	StopNavHold();
	SetStatus(FString::Printf(TEXT("Session %d begins as %s..."), SessionSlot + 1, *Hero->DisplayName.ToString()), Color::Accent());
	RefreshFooter();
	return true;
}

void UHeroSelectWidget::CancelConfirm(const FString& Why)
{
	ConfirmLeft = -1.f;
	bConfirmSent = false;
	if (StagePicture)
	{
		StagePicture->SetRenderScale(FVector2D::UnitVector);
	}
	if (StageGlow)
	{
		StageGlow->SetRenderScale(FVector2D::UnitVector);
	}
	if (StandImage)
	{
		StandImage->SetRenderScale(FVector2D::UnitVector);
	}
	if (AHeroSelectStage* StandPtr = Stand.Get())
	{
		StandPtr->SetFlare(0.f);
	}
	ShowSelected();
	SetStatus(Why, Color::Worse());
}

void UHeroSelectWidget::TickConfirm(float DeltaSeconds)
{
	if (ConfirmLeft < 0.f)
	{
		return;
	}
	ConfirmLeft = FMath::Max(ConfirmLeft - DeltaSeconds, 0.f);
	const float Progress = EaseOut(1.f - ConfirmLeft / ConfirmSeconds);
	if (StagePicture)
	{
		StagePicture->SetRenderScale(FVector2D(1.f + 0.035f * Progress));
	}
	// On the stand the hero steps up a touch into a rim and key burning brighter.
	if (StandImage)
	{
		StandImage->SetRenderScale(FVector2D(1.f + 0.03f * Progress));
	}
	if (AHeroSelectStage* StandPtr = Stand.Get())
	{
		StandPtr->SetFlare(Progress);
	}
	// The glow swells and burns brighter in the hero's colour: their ember catching.
	const UHeroDefinition* Hero = GetHero(Selected);
	if (StageGlow && Hero)
	{
		StageGlow->SetColorAndOpacity(WithAlpha(Hero->Color, FMath::Lerp(StageGlowAlpha, 0.9f, Progress)));
		StageGlow->SetRenderScale(FVector2D(1.f + 0.12f * Progress));
		StageGlow->SetRenderOpacity(1.f);
	}
	if (ConfirmLeft <= 0.f && !bConfirmSent)
	{
		bConfirmSent = true;
		OnConfirmed.ExecuteIfBound(SessionSlot, GetSelectedId());
	}
}

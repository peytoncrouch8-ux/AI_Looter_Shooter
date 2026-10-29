#include "UI/Cards/GunCardWidget.h"
#include "Audio/LooterSound.h"
#include "Audio/LooterSoundCues.h"
#include "UI/Cards/GunCardRules.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"

// The gun card's intro (the user's ask, 2026-10-28: "motion and juice"): it snaps open, its rows come in one by one, the
// damage counts up, the level's gem pops as it lands, and it opens with a tick and its maker's own sound.

void UGunCardWidget::PlayIntro(bool bWithSound)
{
	IntroAge = 0.f;
	ApplyIntro();
	if (bWithSound)
	{
		LooterSound::Play2D(this, FName(LooterSoundCue::Card::Open));
		const FName MakerCue = GetMakerCue();
		if (!MakerCue.IsNone())
		{
			LooterSound::Play2D(this, MakerCue);
		}
	}
}

void UGunCardWidget::SkipIntro()
{
	IntroAge = GunCardRules::IntroSeconds;
	ApplyIntro();
}

void UGunCardWidget::AdvanceIntro(float DeltaSeconds)
{
	if (!IsIntroPlaying())
	{
		return;
	}
	IntroAge = FMath::Min(IntroAge + FMath::Max(0.f, DeltaSeconds), GunCardRules::IntroSeconds);
	ApplyIntro();
}

void UGunCardWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	AdvanceIntro(InDeltaTime);
}

void UGunCardWidget::ApplyIntro()
{
	if (!Root)
	{
		return;
	}
	const float Age = IntroAge;
	// The unfold, from the foot for a world card (it rises out of the gun), from the top elsewhere.
	// The in-hand card is drawn at four fifths: it's a reminder on the screen's edge, not the gun being looked at, and at full
	// size it covered the weapon slots (the 2026-10-28 play-check). Shrunk toward its right edge, where it hangs.
	const bool bInHand = Mode == EGunCardMode::InHand;
	const float Size = bInHand ? 0.8f : 1.f;
	Root->SetRenderTransformPivot(FVector2D(bInHand ? 1.f : 0.5f, UnfoldPivotY(Mode)));
	FWidgetTransform Transform;
	Transform.Scale = FVector2D(Size, Size * GunCardRules::UnfoldScale(Age));
	Root->SetRenderTransform(Transform);
	Root->SetRenderOpacity(GunCardRules::UnfoldOpacity(Age));

	// The rows slide in from the left, one after another.
	for (int32 Index = 0; Index < RowWidgets.Num(); ++Index)
	{
		const float Share = GunCardRules::RowShare(Age, Index);
		RowWidgets[Index]->SetRenderOpacity(Share);
		RowWidgets[Index]->SetRenderTranslation(FVector2D((1.f - Share) * -14.f, 0.f));
	}
	// The damage counts up to its value (pellets and all).
	if (bHasGun && RowValues.Num() > 0 && RowValues[0] && ShownStats.Num() > 0)
	{
		FWeaponStats Counting = Shown.Stats;
		Counting.Damage = Age >= GunCardRules::CountSeconds ? Shown.Stats.Damage : Shown.Stats.Damage * GunCardRules::CountShare(Age);
		const FString Number = GunCardRules::StatValueText(ShownStats[0], Counting);
		if (!RowValues[0]->GetText().ToString().Equals(Number))
		{
			RowValues[0]->SetText(FText::FromString(Number));
		}
	}
	if (Gem)
	{
		const float Pop = GunCardRules::GemPop(Age);
		FWidgetTransform GemTransform;
		GemTransform.Scale = FVector2D(Pop, Pop);
		Gem->SetRenderTransform(GemTransform);
	}
}

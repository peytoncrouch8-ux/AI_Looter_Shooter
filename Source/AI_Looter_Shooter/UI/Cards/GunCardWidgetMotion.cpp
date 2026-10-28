#include "UI/Cards/GunCardWidget.h"
#include "Audio/LooterSound.h"
#include "Audio/LooterSoundCues.h"
#include "UI/Cards/GunCardStyle.h"
#include "UI/Cards/LooterCardBackdrop.h"
#include "UI/Cards/LooterSegmentBar.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"

// The gun card's intro (the user's ask, 2026-10-28: "motion and juice"): it snaps open, its bars fill one by one, the damage
// counts up, an Epic's or a Legendary's sweep catches a shine, and it opens with a tick and its maker's own sound.

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
	Root->SetRenderTransformPivot(FVector2D(0.5f, UnfoldPivotY(Mode)));
	FWidgetTransform Transform;
	Transform.Scale = FVector2D(1.f, GunCardRules::UnfoldScale(Age));
	Root->SetRenderTransform(Transform);
	Root->SetRenderOpacity(GunCardRules::UnfoldOpacity(Age));

	for (int32 Index = 0; Index < Bars.Num(); ++Index)
	{
		Bars[Index]->SetShown(GunCardRules::BarShare(Age, Index));
	}
	if (DamageText)
	{
		const int32 Counted = FMath::RoundToInt(DamageValue * GunCardRules::CountShare(Age));
		const FString Number = FText::AsNumber(Age >= GunCardRules::CountSeconds ? FMath::RoundToInt(DamageValue) : Counted).ToString();
		if (!DamageText->GetText().ToString().Equals(Number))
		{
			DamageText->SetText(FText::FromString(Number));
		}
	}
	if (Lines)
	{
		Lines->SetShine(bHasGun && GunCardRules::HasShine(Shown) ? GunCardRules::ShinePosition(Age) : -1.f);
	}
}

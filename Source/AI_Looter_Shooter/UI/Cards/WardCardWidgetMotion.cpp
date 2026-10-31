#include "UI/Cards/WardCardWidget.h"
#include "Audio/LooterSound.h"
#include "Audio/LooterSoundCues.h"
#include "Shields/ShieldText.h"
#include "UI/Cards/GunCardRules.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"

// The ward card's intro, the gun card's (GunCardWidgetMotion.cpp): it snaps open, its rows come in one by one, the capacity
// counts up, the level's gem pops as it lands, and it opens with the card's tick. Wards have no maker, so no maker's sound.

void UWardCardWidget::PlayIntro(bool bWithSound)
{
	IntroAge = 0.f;
	ApplyIntro();
	if (bWithSound)
	{
		LooterSound::Play2D(this, FName(LooterSoundCue::Card::Open));
	}
}

void UWardCardWidget::SkipIntro()
{
	IntroAge = GunCardRules::IntroSeconds;
	ApplyIntro();
}

void UWardCardWidget::AdvanceIntro(float DeltaSeconds)
{
	if (!IsIntroPlaying())
	{
		return;
	}
	IntroAge = FMath::Min(IntroAge + FMath::Max(0.f, DeltaSeconds), GunCardRules::IntroSeconds);
	ApplyIntro();
}

void UWardCardWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	AdvanceIntro(InDeltaTime);
}

void UWardCardWidget::ApplyIntro()
{
	if (!Root)
	{
		return;
	}
	const float Age = IntroAge;
	// A world card unfolds from its foot (it rises out of the ward box), the inventory's from its top.
	Root->SetRenderTransformPivot(FVector2D(0.5f, Mode == EWardCardMode::World ? 1.f : 0.f));
	FWidgetTransform Transform;
	Transform.Scale = FVector2D(1.f, GunCardRules::UnfoldScale(Age));
	Root->SetRenderTransform(Transform);
	Root->SetRenderOpacity(GunCardRules::UnfoldOpacity(Age));

	for (int32 Index = 0; Index < RowWidgets.Num(); ++Index)
	{
		const float Share = GunCardRules::RowShare(Age, Index);
		RowWidgets[Index]->SetRenderOpacity(Share);
		RowWidgets[Index]->SetRenderTranslation(FVector2D((1.f - Share) * -14.f, 0.f));
	}
	// The capacity counts up to its value, as a gun card's damage does.
	if (bHasWard && RowValues.Num() > 0 && RowValues[0])
	{
		FShieldStats Counting = Shown.Stats;
		Counting.Capacity = Age >= GunCardRules::CountSeconds ? Shown.Stats.Capacity : Shown.Stats.Capacity * GunCardRules::CountShare(Age);
		const FString Number = ShieldText::CapacityString(Counting);
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

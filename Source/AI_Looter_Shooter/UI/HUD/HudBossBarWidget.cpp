#include "UI/HUD/HudBossBarWidget.h"
#include "UI/HUD/LooterHUD.h"
#include "UI/Style/LooterUIStyle.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

using namespace LooterUI;

namespace
{
	/** After a hit the lost part lingers this long, then drains at this share of the bar per second (as the player's bar). */
	constexpr float BossChipHoldSeconds = 0.45f;
	constexpr float BossChipDrainSpeed = 0.6f;
	/** Seconds to fade fully in or out, to grey or back, and how long a new phase's name flashes. */
	constexpr float BossFadeSeconds = 0.4f;
	constexpr float BossGreySeconds = 0.25f;
	constexpr float BossPhaseFlashSeconds = 1.6f;
	/** A new phase's name starts this much bigger and settles as its flash fades. */
	constexpr float BossPhasePop = 0.15f;
	/** The fill's lit top line: its leading edge's light, this strong over the top band. */
	constexpr float BossTopLineOpacity = 0.35f;

	/** Tone, Amount of the way to the grey of its own brightness: an untargetable fill keeps its bands' light and dark. */
	FLinearColor BossGreyed(const FLinearColor& Tone, float Amount)
	{
		const float Luminance = Tone.GetLuminance();
		return FMath::Lerp(Tone, FLinearColor(Luminance, Luminance, Luminance, Tone.A), Amount);
	}
}

// ---------------------------------------------------------------------------
// What it shows
// ---------------------------------------------------------------------------

TArray<float> UHudBossBarWidget::MakeTickShares(const TArray<float>& PhaseShares)
{
	// The first phase starts at full health (no cut at the bar's end); a share at the very ends would sit on the rim.
	TArray<float> Shares;
	for (int32 Index = 1; Index < PhaseShares.Num(); ++Index)
	{
		const float Share = PhaseShares[Index];
		if (Share > 0.001f && Share < 0.999f && !Shares.ContainsByPredicate([Share](float Other) { return FMath::IsNearlyEqual(Other, Share, 0.001f); }))
		{
			Shares.Add(Share);
		}
	}
	Shares.Sort([](float A, float B) { return A > B; });
	return Shares;
}

FText UHudBossBarWidget::MakeLevelText(int32 Level)
{
	return FText::FromString(FString::Printf(TEXT("LV %d"), FMath::Max(Level, 1)));
}

FText UHudBossBarWidget::MakePhaseLine(const FText& InPhaseName, bool bUntargetable, const FText& InHint)
{
	return (bUntargetable && !InHint.IsEmpty() ? InHint : InPhaseName).ToUpper();
}

FText UHudBossBarWidget::GetNameText() const
{
	return NameLabel ? NameLabel->GetText() : BossName.ToUpper();
}

FText UHudBossBarWidget::GetLevelText() const
{
	// The gem has room for the number only; its words are the level as people read the gem.
	return MakeLevelText(BossLevel);
}

FText UHudBossBarWidget::GetPhaseText() const
{
	return PhaseLabel ? PhaseLabel->GetText() : MakePhaseLine(PhaseName, bGreyed, Hint);
}

void UHudBossBarWidget::SetBoss(const FText& InName, int32 InLevel, const FLinearColor& InNameColor, const TArray<float>& PhaseShares)
{
	BossName = InName;
	BossLevel = FMath::Max(InLevel, 1);
	NameColor = InNameColor;
	TickShares = MakeTickShares(PhaseShares);
	// A new fight: full, in its first phase, hurtable.
	Fraction = 1.f;
	ChipFraction = 1.f;
	ChipHold = 0.f;
	PhaseIndex = INDEX_NONE;
	PhaseName = FText::GetEmpty();
	Hint = FText::GetEmpty();
	bGreyed = false;
	Grey = 0.f;
	PhaseFlash = 0.f;
	ShownFillWidth = -1.f;
	ShownChipWidth = -1.f;
	BuildTicks();
	ApplyTexts();
	ApplyFill();
	ApplyColors();
}

void UHudBossBarWidget::SetHealth(float Health, float MaxHealth)
{
	const float NewFraction = MaxHealth > 0.f ? FMath::Clamp(Health / MaxHealth, 0.f, 1.f) : 0.f;
	if (FMath::IsNearlyEqual(NewFraction, Fraction))
	{
		return;
	}
	if (NewFraction < Fraction)
	{
		// A hit: what it took lingers as the chip for a moment.
		ChipHold = BossChipHoldSeconds;
	}
	else
	{
		// Healed (a reset): no chip.
		ChipFraction = NewFraction;
	}
	Fraction = NewFraction;
	ChipFraction = FMath::Max(ChipFraction, Fraction);
	ApplyFill();
}

void UHudBossBarWidget::SetPhase(int32 Index, const FText& InPhaseName)
{
	if (Index == PhaseIndex && PhaseName.EqualTo(InPhaseName))
	{
		return;
	}
	// A later phase flashes its name; the first is only the fight starting.
	if (PhaseIndex != INDEX_NONE && Index > PhaseIndex)
	{
		PhaseFlash = BossPhaseFlashSeconds;
	}
	PhaseIndex = Index;
	PhaseName = InPhaseName;
	ApplyTexts();
	ApplyColors();
}

void UHudBossBarWidget::SetUntargetable(bool bInUntargetable, const FText& InHint)
{
	if (bGreyed == bInUntargetable && Hint.EqualTo(InHint))
	{
		return;
	}
	bGreyed = bInUntargetable;
	Hint = InHint;
	ApplyTexts();
	ApplyColors();
}

void UHudBossBarWidget::SetShown(bool bInShown)
{
	bWanted = bInShown;
}

// ---------------------------------------------------------------------------
// Painting
// ---------------------------------------------------------------------------

void UHudBossBarWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (!Cluster)
	{
		return;
	}

	// In while its fight is on and nothing covers the game; out otherwise.
	const float Target = bWanted && !IsMenuOpen() ? 1.f : 0.f;
	if (Opacity != Target)
	{
		Opacity = FMath::FInterpConstantTo(Opacity, Target, InDeltaTime, 1.f / BossFadeSeconds);
		Cluster->SetRenderOpacity(Opacity);
		Cluster->SetVisibility(Opacity > 0.f ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	bool bFillMoved = false;
	if (ChipFraction > Fraction)
	{
		if (ChipHold > 0.f)
		{
			ChipHold -= InDeltaTime;
		}
		else
		{
			ChipFraction = FMath::Max(Fraction, ChipFraction - BossChipDrainSpeed * InDeltaTime);
			bFillMoved = true;
		}
	}
	bool bColorsMoved = false;
	const float GreyTarget = bGreyed ? 1.f : 0.f;
	if (Grey != GreyTarget)
	{
		Grey = FMath::FInterpConstantTo(Grey, GreyTarget, InDeltaTime, 1.f / BossGreySeconds);
		bColorsMoved = true;
	}
	if (PhaseFlash > 0.f)
	{
		PhaseFlash = FMath::Max(0.f, PhaseFlash - InDeltaTime);
		bColorsMoved = true;
	}
	if (bFillMoved)
	{
		ApplyFill();
	}
	if (bColorsMoved)
	{
		ApplyColors();
	}
}

bool UHudBossBarWidget::IsMenuOpen() const
{
	const APlayerController* Player = GetOwningPlayer();
	const ALooterHUD* LooterHUD = Player ? Cast<ALooterHUD>(Player->GetHUD()) : nullptr;
	if (LooterHUD && LooterHUD->IsMenuOpen())
	{
		return true;
	}
	const UWorld* World = GetWorld();
	return World && World->IsPaused();
}

void UHudBossBarWidget::ApplyTexts()
{
	if (LevelLabel)
	{
		LevelLabel->SetText(FText::AsNumber(BossLevel));
	}
	if (NameLabel)
	{
		NameLabel->SetText(BossName.ToUpper());
	}
	if (PhaseLabel)
	{
		const FText Line = MakePhaseLine(PhaseName, bGreyed, Hint);
		PhaseLabel->SetText(Line);
		PhaseLabel->SetVisibility(Line.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
}

void UHudBossBarWidget::ApplyColors()
{
	if (FillBands.Num() < 3 || !FillTopLine || !FillEdge || !GemFace || !NameLabel || !PhaseLabel)
	{
		return;
	}
	// The fill in the health bar's three bands, greying while the boss can't be hurt; its top line and leading edge the
	// light of its edge.
	const FLinearColor Bands[3] = { Color::HealthHi(), Color::Health(), Color::HealthLow() };
	for (int32 Index = 0; Index < 3; ++Index)
	{
		FillBands[Index]->SetColorAndOpacity(BossGreyed(Bands[Index], Grey));
	}
	const FLinearColor Edge = BossGreyed(Color::HealthEdge(), Grey);
	FillTopLine->SetColorAndOpacity(Edge.CopyWithNewOpacity(BossTopLineOpacity));
	FillEdge->SetColorAndOpacity(Edge);
	GemFace->SetColorAndOpacity(NameColor);
	NameLabel->SetColorAndOpacity(FSlateColor(FMath::Lerp(NameColor, Color::TextDim(), Grey * 0.6f)));

	// A hint reads in the text color (it's what to do now); a new phase's name flashes orange a little bigger, then
	// settles dim, quickly at first (eased out, as the mockup's).
	const float Flash = FMath::Square(FMath::Clamp(PhaseFlash / BossPhaseFlashSeconds, 0.f, 1.f));
	const FLinearColor Line = bGreyed && !Hint.IsEmpty() ? Color::Text() : FMath::Lerp(Color::TextDim(), Color::Accent(), Flash);
	PhaseLabel->SetColorAndOpacity(FSlateColor(Line));
	PhaseLabel->SetRenderScale(FVector2D(1.f + BossPhasePop * Flash));
}

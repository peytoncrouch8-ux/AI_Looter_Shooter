// AGravemotherStone's target glow (GravemotherStone.h, Mark): while it stands nothing hurts her (the user's call,
// 2026-10-27), so when her hide shrugs a hit off, and as her fight starts, the stone lights up as the thing to shoot: the
// mission tracker's orange drawn over it (M_ObjectiveHighlight, the objective highlight's own material, as its overlay),
// pulsing, fading at the end.

#include "Creatures/GravemotherStone.h"
#include "AI_Looter_Shooter.h"
#include "Missions/ObjectiveHighlightSubsystem.h"
#include "UI/Style/LooterUIStyle.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

namespace
{
	// M_ObjectiveHighlight's parameters (Tools/Unreal/build_highlight_material.py): MaskRadius 0 lights the whole mesh, Figure
	// 0 is a thing's look (a soft fill and a rim).
	const FName StrengthParam(TEXT("Strength"));
	const FName ColorParam(TEXT("Color"));
	const FName MaskRadiusParam(TEXT("MaskRadius"));
	const FName FigureParam(TEXT("Figure"));

	/** The glow comes up over this long and fades over the last FadeOut (s); between, it pulses this often (Hz) between
	 *  PulseLow and full: a beat the eye catches in a busy fight, not a steady paint. */
	constexpr float FadeIn = 0.08f;
	constexpr float FadeOut = 0.5f;
	constexpr float PulseHz = 2.5f;
	constexpr float PulseLow = 0.55f;
}

void AGravemotherStone::Mark(float Seconds)
{
	if (!IsOn() || Seconds <= 0.f)
	{
		return;
	}
	if (!MarkLook && !bMarkTried)
	{
		// Once: the objective highlight's own material (none in a build without it: the stone just doesn't glow).
		bMarkTried = true;
		if (UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, UObjectiveHighlightSubsystem::MaterialPath, nullptr, LOAD_NoWarn))
		{
			MarkLook = UMaterialInstanceDynamic::Create(Base, this);
			MarkLook->SetVectorParameterValue(ColorParam, LooterUI::Color::Accent());
			MarkLook->SetScalarParameterValue(MaskRadiusParam, 0.f);
			MarkLook->SetScalarParameterValue(FigureParam, 0.f);
			MarkLook->SetScalarParameterValue(StrengthParam, 0.f);
		}
		else
		{
			UE_LOG(LogLooter, Log, TEXT("The Gravemother's headstone: no %s, so it won't glow as the target."), UObjectiveHighlightSubsystem::MaterialPath);
		}
	}
	// A fresh mark comes up from nothing; one already glowing keeps glowing for the longer of the two.
	const bool bWasMarked = IsMarked();
	MarkLeft = bWasMarked ? FMath::Max(MarkLeft, Seconds) : Seconds;
	MarkSeconds = bWasMarked ? FMath::Max(MarkSeconds, MarkLeft) : Seconds;
	if (MarkLook && Slab->GetOverlayMaterial() != MarkLook.Get())
	{
		Slab->SetOverlayMaterial(MarkLook.Get());
		// Nanite draws no overlay (High/Epic): the slab drawn the old way while it glows (Medium has Nanite off anyway).
		Slab->SetForceDisableNanite(true);
	}
	TickMark(0.f);
	UpdateTickNeed();
}

float AGravemotherStone::GetMarkStrength() const
{
	if (!IsMarked())
	{
		return 0.f;
	}
	const float Age = FMath::Max(MarkSeconds - MarkLeft, 0.f);
	const float Envelope = FMath::Min(FMath::Clamp(Age / FadeIn, 0.f, 1.f), FMath::Clamp(MarkLeft / FadeOut, 0.f, 1.f));
	const float Pulse = FMath::Lerp(PulseLow, 1.f, 0.5f + 0.5f * FMath::Cos(Age * UE_TWO_PI * PulseHz));
	return Envelope * Pulse;
}

void AGravemotherStone::TickMark(float DeltaSeconds)
{
	if (!IsMarked())
	{
		return;
	}
	MarkLeft = FMath::Max(MarkLeft - FMath::Max(DeltaSeconds, 0.f), 0.f);
	if (!IsMarked() || !IsOn())
	{
		ClearMark();
		return;
	}
	if (MarkLook)
	{
		MarkLook->SetScalarParameterValue(StrengthParam, GetMarkStrength());
	}
}

void AGravemotherStone::ClearMark()
{
	MarkLeft = 0.f;
	MarkSeconds = 0.f;
	if (Slab && MarkLook && Slab->GetOverlayMaterial() == MarkLook.Get())
	{
		MarkLook->SetScalarParameterValue(StrengthParam, 0.f);
		Slab->SetOverlayMaterial(nullptr);
		Slab->SetForceDisableNanite(false);
	}
	UpdateTickNeed();
}

void AGravemotherStone::UpdateTickNeed()
{
	SetActorTickEnabled(ShardTime >= 0.f || IsMarked());
}

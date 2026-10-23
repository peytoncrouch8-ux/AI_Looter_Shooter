#include "UI/HUD/HudHitMarker.h"
#include "Audio/LooterSoundCues.h"
#include "Creatures/CreatureTraitComponent.h"
#include "UI/Style/LooterUIStyle.h"
#include "Weapons/GunfeelSoundTuning.h"
#include "GameFramework/Actor.h"
#include "HAL/IConsoleManager.h"

// The hit marker's rules (HudHitMarker.h). The cue names are GF-SOUND-CODE's (GunfeelSound::HitConfirmCue, LooterSoundCue::
// Gunfeel::WardBreak), so the HUD and the creature's voice agree on them.

namespace
{
	const HudHitMarker::FTuning HitMarkerDefaults;

	TAutoConsoleVariable<float> CVarHitMarkerSizeGain(TEXT("Looter.UI.HitMarker.SizeGain"), HitMarkerDefaults.SizeGain,
		TEXT("The hit marker grows by up to this share of its size with the hit's share of the target's health."), ECVF_Default);
	TAutoConsoleVariable<float> CVarHitMarkerSizeShare(TEXT("Looter.UI.HitMarker.SizeShare"), HitMarkerDefaults.SizeShare,
		TEXT("...fully grown at a hit taking this share of the target's most health."), ECVF_Default);
	TAutoConsoleVariable<float> CVarHitMarkerBlastWindow(TEXT("Looter.UI.HitMarker.BlastWindow"), HitMarkerDefaults.BlastWindow,
		TEXT("A pellet gun's hits this soon (s) after its blast's first are one blast: one marker, one sound."), ECVF_Default);
	TAutoConsoleVariable<float> CVarHitMarkerSeconds(TEXT("Looter.UI.HitMarker.Seconds"), HitMarkerDefaults.Seconds,
		TEXT("How long a hit's marker shows (s)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarHitMarkerKillSeconds(TEXT("Looter.UI.HitMarker.KillSeconds"), HitMarkerDefaults.KillSeconds,
		TEXT("...a kill's (s)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarHitMarkerBreakSeconds(TEXT("Looter.UI.HitMarker.BreakSeconds"), HitMarkerDefaults.BreakSeconds,
		TEXT("...a ward break's (s)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarHitMarkerPop(TEXT("Looter.UI.HitMarker.Pop"), HitMarkerDefaults.Pop,
		TEXT("A hit's marker pops in this share bigger and settles."), ECVF_Default);
	TAutoConsoleVariable<float> CVarHitMarkerKillPop(TEXT("Looter.UI.HitMarker.KillPop"), HitMarkerDefaults.KillPop,
		TEXT("...a kill's."), ECVF_Default);
	TAutoConsoleVariable<float> CVarHitMarkerBreakPop(TEXT("Looter.UI.HitMarker.BreakPop"), HitMarkerDefaults.BreakPop,
		TEXT("...a ward break's."), ECVF_Default);

	float HitMarkerNotBelowZero(const TAutoConsoleVariable<float>& Variable)
	{
		return FMath::Max(Variable.GetValueOnGameThread(), 0.f);
	}
}

HudHitMarker::FTuning HudHitMarker::FTuning::Live()
{
	FTuning Tuning;
	Tuning.SizeGain = HitMarkerNotBelowZero(CVarHitMarkerSizeGain);
	Tuning.SizeShare = FMath::Max(CVarHitMarkerSizeShare.GetValueOnGameThread(), 0.01f);
	Tuning.BlastWindow = HitMarkerNotBelowZero(CVarHitMarkerBlastWindow);
	Tuning.Seconds = FMath::Max(CVarHitMarkerSeconds.GetValueOnGameThread(), 0.02f);
	Tuning.KillSeconds = FMath::Max(CVarHitMarkerKillSeconds.GetValueOnGameThread(), 0.02f);
	Tuning.BreakSeconds = FMath::Max(CVarHitMarkerBreakSeconds.GetValueOnGameThread(), 0.02f);
	Tuning.Pop = HitMarkerNotBelowZero(CVarHitMarkerPop);
	Tuning.KillPop = HitMarkerNotBelowZero(CVarHitMarkerKillPop);
	Tuning.BreakPop = HitMarkerNotBelowZero(CVarHitMarkerBreakPop);
	return Tuning;
}

EHudHitMark HudHitMarker::Decide(bool bKill, bool bCritical, bool bWarded, float WardBefore, float WardAfter)
{
	if (bKill)
	{
		return EHudHitMark::Kill;
	}
	if (bWarded)
	{
		// The ward takes every hit first (a crit counts double into it): while it stands, it was the ward that was hit.
		if (WardAfter > 0.f)
		{
			return EHudHitMark::Ward;
		}
		if (WardBefore > 0.f)
		{
			return EHudHitMark::WardBreak;
		}
	}
	return bCritical ? EHudHitMark::Crit : EHudHitMark::Body;
}

float HudHitMarker::Size(float DamageShare, const FTuning& Tuning)
{
	return 1.f + Tuning.SizeGain * FMath::Clamp(DamageShare / FMath::Max(Tuning.SizeShare, 0.01f), 0.f, 1.f);
}

float HudHitMarker::BlastSize(int32 Landed, int32 Pellets, const FTuning& Tuning)
{
	const float Share = Pellets > 0 ? static_cast<float>(Landed) / Pellets : 0.f;
	return 1.f + Tuning.SizeGain * FMath::Clamp(Share, 0.f, 1.f);
}

float HudHitMarker::SecondsOf(EHudHitMark Mark, const FTuning& Tuning)
{
	switch (Mark)
	{
	case EHudHitMark::Kill: return Tuning.KillSeconds;
	case EHudHitMark::WardBreak: return Tuning.BreakSeconds;
	default: return Tuning.Seconds;
	}
}

float HudHitMarker::PopOf(EHudHitMark Mark, const FTuning& Tuning)
{
	switch (Mark)
	{
	case EHudHitMark::Kill: return Tuning.KillPop;
	case EHudHitMark::WardBreak: return Tuning.BreakPop;
	default: return Tuning.Pop;
	}
}

FLinearColor HudHitMarker::Tint(EHudHitMark Mark)
{
	using namespace LooterUI;
	switch (Mark)
	{
	case EHudHitMark::Kill: return Color::Hurt();
	// The break a touch whiter than a ward's tick: the same colour, louder.
	case EHudHitMark::WardBreak: return FMath::Lerp(Color::CyanText(), FLinearColor::White, 0.35f);
	case EHudHitMark::Ward: return Color::CyanText();
	case EHudHitMark::Crit: return Color::Accent();
	case EHudHitMark::Body:
	default: return FLinearColor::White;
	}
}

FName HudHitMarker::Cue(EHudHitMark Mark)
{
	switch (Mark)
	{
	case EHudHitMark::Kill: return NAME_None;
	case EHudHitMark::WardBreak: return FName(LooterSoundCue::Gunfeel::WardBreak);
	case EHudHitMark::Ward: return GunfeelSound::HitConfirmCue(false, true);
	case EHudHitMark::Crit: return GunfeelSound::HitConfirmCue(true, false);
	case EHudHitMark::Body:
	default: return GunfeelSound::HitConfirmCue(false, false);
	}
}

bool HudHitMarker::SoundsAgain(EHudHitMark Mark, EHudHitMark Sounded)
{
	// A crit pellet after plain ones pings; a break after the ward's ticks cracks; nothing sounds twice.
	return Outranks(Mark, Sounded) && Mark != EHudHitMark::Kill;
}

float FHudWardMemory::Before(const AActor* Target) const
{
	for (const FEntry& Entry : Entries)
	{
		if (Entry.Actor.Get() == Target)
		{
			return Entry.Fraction;
		}
	}
	return 1.f;
}

void FHudWardMemory::Note(const AActor* Target, const UCreatureTraitComponent* Traits, float Fraction)
{
	if (!Target)
	{
		return;
	}
	const int32 Found = Entries.IndexOfByPredicate([Target](const FEntry& Entry) { return Entry.Actor.Get() == Target; });
	if (Found != INDEX_NONE)
	{
		Entries.RemoveAt(Found, 1, EAllowShrinking::No);
	}
	else if (Entries.Num() >= Kept)
	{
		// The one hit longest ago makes room.
		Entries.RemoveAt(0, 1, EAllowShrinking::No);
	}
	FEntry& Entry = Entries.AddDefaulted_GetRef();
	Entry.Actor = Target;
	Entry.Traits = Traits;
	Entry.Fraction = Fraction;
}

void FHudWardMemory::Refresh()
{
	for (int32 Index = Entries.Num() - 1; Index >= 0; --Index)
	{
		FEntry& Entry = Entries[Index];
		const UCreatureTraitComponent* Traits = Entry.Traits.Get();
		if (!Entry.Actor.IsValid() || !Traits)
		{
			Entries.RemoveAt(Index, 1, EAllowShrinking::No);
			continue;
		}
		Entry.Fraction = Traits->GetWardFraction();
	}
}

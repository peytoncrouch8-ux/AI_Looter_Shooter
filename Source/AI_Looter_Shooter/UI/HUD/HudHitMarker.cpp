#include "UI/HUD/HudHitMarker.h"
#include "Audio/LooterSoundCues.h"
#include "Creatures/CreatureTraitComponent.h"
#include "Elements/ElementRules.h"
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
		TEXT("How long a hit's marker shows (s), its hold and its fade."), ECVF_Default);
	TAutoConsoleVariable<float> CVarHitMarkerCritSeconds(TEXT("Looter.UI.HitMarker.CritSeconds"), HitMarkerDefaults.CritSeconds,
		TEXT("...a crit's (s)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarHitMarkerKillSeconds(TEXT("Looter.UI.HitMarker.KillSeconds"), HitMarkerDefaults.KillSeconds,
		TEXT("...a kill's (s)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarHitMarkerBreakSeconds(TEXT("Looter.UI.HitMarker.BreakSeconds"), HitMarkerDefaults.BreakSeconds,
		TEXT("...a ward break's (s)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarHitMarkerFadeShare(TEXT("Looter.UI.HitMarker.FadeShare"), HitMarkerDefaults.FadeShare,
		TEXT("The last share of a marker's time it fades over (0-1); it holds whole before. 1: fades from the start."), ECVF_Default);
	TAutoConsoleVariable<float> CVarHitMarkerPop(TEXT("Looter.UI.HitMarker.Pop"), HitMarkerDefaults.Pop,
		TEXT("A hit's marker holds this share bigger, settling to its size as it fades."), ECVF_Default);
	TAutoConsoleVariable<float> CVarHitMarkerCritPop(TEXT("Looter.UI.HitMarker.CritPop"), HitMarkerDefaults.CritPop,
		TEXT("...a crit's."), ECVF_Default);
	TAutoConsoleVariable<float> CVarHitMarkerKillPop(TEXT("Looter.UI.HitMarker.KillPop"), HitMarkerDefaults.KillPop,
		TEXT("...a kill's."), ECVF_Default);
	TAutoConsoleVariable<float> CVarHitMarkerBreakPop(TEXT("Looter.UI.HitMarker.BreakPop"), HitMarkerDefaults.BreakPop,
		TEXT("...a ward break's."), ECVF_Default);
	TAutoConsoleVariable<float> CVarHitMarkerCritScale(TEXT("Looter.UI.HitMarker.CritScale"), HitMarkerDefaults.CritScale,
		TEXT("A crit's marker is this many times a plain hit's size for the same hit."), ECVF_Default);
	TAutoConsoleVariable<float> CVarHitMarkerKillScale(TEXT("Looter.UI.HitMarker.KillScale"), HitMarkerDefaults.KillScale,
		TEXT("...a kill's."), ECVF_Default);
	TAutoConsoleVariable<float> CVarHitMarkerBreakScale(TEXT("Looter.UI.HitMarker.BreakScale"), HitMarkerDefaults.BreakScale,
		TEXT("...a ward break's."), ECVF_Default);
	TAutoConsoleVariable<float> CVarHitMarkerCritRim(TEXT("Looter.UI.HitMarker.CritRim"), HitMarkerDefaults.CritRim,
		TEXT("The red rim round a crit's white ticks (px; the ticks are 4 px thick)."), ECVF_Default);

	float HitMarkerNotBelowZero(const TAutoConsoleVariable<float>& Variable)
	{
		return FMath::Max(Variable.GetValueOnGameThread(), 0.f);
	}

	float HitMarkerSecondsOf(const TAutoConsoleVariable<float>& Variable)
	{
		return FMath::Max(Variable.GetValueOnGameThread(), 0.02f);
	}
}

HudHitMarker::FTuning HudHitMarker::FTuning::Live()
{
	FTuning Tuning;
	Tuning.SizeGain = HitMarkerNotBelowZero(CVarHitMarkerSizeGain);
	Tuning.SizeShare = FMath::Max(CVarHitMarkerSizeShare.GetValueOnGameThread(), 0.01f);
	Tuning.BlastWindow = HitMarkerNotBelowZero(CVarHitMarkerBlastWindow);
	Tuning.Seconds = HitMarkerSecondsOf(CVarHitMarkerSeconds);
	Tuning.CritSeconds = HitMarkerSecondsOf(CVarHitMarkerCritSeconds);
	Tuning.KillSeconds = HitMarkerSecondsOf(CVarHitMarkerKillSeconds);
	Tuning.BreakSeconds = HitMarkerSecondsOf(CVarHitMarkerBreakSeconds);
	Tuning.FadeShare = FMath::Clamp(CVarHitMarkerFadeShare.GetValueOnGameThread(), 0.05f, 1.f);
	Tuning.Pop = HitMarkerNotBelowZero(CVarHitMarkerPop);
	Tuning.CritPop = HitMarkerNotBelowZero(CVarHitMarkerCritPop);
	Tuning.KillPop = HitMarkerNotBelowZero(CVarHitMarkerKillPop);
	Tuning.BreakPop = HitMarkerNotBelowZero(CVarHitMarkerBreakPop);
	Tuning.CritScale = FMath::Max(CVarHitMarkerCritScale.GetValueOnGameThread(), 0.1f);
	Tuning.KillScale = FMath::Max(CVarHitMarkerKillScale.GetValueOnGameThread(), 0.1f);
	Tuning.BreakScale = FMath::Max(CVarHitMarkerBreakScale.GetValueOnGameThread(), 0.1f);
	// Under half the 4 px tick's thickness, or the white would be gone.
	Tuning.CritRim = FMath::Clamp(CVarHitMarkerCritRim.GetValueOnGameThread(), 0.f, 1.75f);
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
	case EHudHitMark::Crit: return Tuning.CritSeconds;
	default: return Tuning.Seconds;
	}
}

float HudHitMarker::FadeSecondsOf(EHudHitMark Mark, const FTuning& Tuning)
{
	// The HUD fades it by what's left of its time over this (clamped whole), so a fade shorter than its time holds it first.
	return SecondsOf(Mark, Tuning) * FMath::Clamp(Tuning.FadeShare, 0.05f, 1.f);
}

float HudHitMarker::PopOf(EHudHitMark Mark, const FTuning& Tuning)
{
	switch (Mark)
	{
	case EHudHitMark::Kill: return Tuning.KillPop;
	case EHudHitMark::WardBreak: return Tuning.BreakPop;
	case EHudHitMark::Crit: return Tuning.CritPop;
	default: return Tuning.Pop;
	}
}

float HudHitMarker::ScaleOf(EHudHitMark Mark, const FTuning& Tuning)
{
	switch (Mark)
	{
	case EHudHitMark::Kill: return Tuning.KillScale;
	case EHudHitMark::WardBreak: return Tuning.BreakScale;
	case EHudHitMark::Crit: return Tuning.CritScale;
	default: return 1.f;
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
	// White inside a red rim (Rim): the pale gold it was hid in Ransom's Rest's grass; a red fill would read as a kill.
	case EHudHitMark::Crit: return FLinearColor::White;
	case EHudHitMark::Body:
	default: return FLinearColor::White;
	}
}

FLinearColor HudHitMarker::Rim(EHudHitMark Mark)
{
	return Mark == EHudHitMark::Crit ? LooterUI::Color::Hurt() : LooterUI::Color::Outline();
}

HudHitMarker::FLook HudHitMarker::LookOf(EHudHitMark Mark, ELooterElement WardElement, const FTuning& Tuning)
{
	FLook Look;
	Look.Fill = Tint(Mark, WardElement);
	Look.Rim = Rim(Mark);
	// The kit's ticks have a 1 px dark outline (PlayerHUDWidgetLayout's MakeTicks); a crit's red rim is its own width.
	Look.RimWidth = Mark == EHudHitMark::Crit ? Tuning.CritRim : 1.f;
	return Look;
}

FLinearColor HudHitMarker::Tint(EHudHitMark Mark, ELooterElement WardElement)
{
	if ((Mark != EHudHitMark::Ward && Mark != EHudHitMark::WardBreak) || WardElement == ELooterElement::None)
	{
		return Tint(Mark);
	}
	// The ward's own colour, as its bar over the tag; the break a touch whiter, as the cyan's is.
	const FLinearColor Ward = ElementRules::UiColor(WardElement);
	return Mark == EHudHitMark::WardBreak ? FMath::Lerp(Ward, FLinearColor::White, 0.35f) : Ward;
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

ELooterElement FHudWardMemory::ElementOf(const AActor* Target) const
{
	for (const FEntry& Entry : Entries)
	{
		if (Entry.Actor.Get() == Target)
		{
			return Entry.Element;
		}
	}
	return ELooterElement::None;
}

void FHudWardMemory::Note(const AActor* Target, const UCreatureTraitComponent* Traits, float Fraction, ELooterElement Element)
{
	if (!Target)
	{
		return;
	}
	const int32 Found = Entries.IndexOfByPredicate([Target](const FEntry& Entry) { return Entry.Actor.Get() == Target; });
	if (Found != INDEX_NONE)
	{
		// A broken ward may not name its element any more: the one seen while it stood stays.
		if (Element == ELooterElement::None)
		{
			Element = Entries[Found].Element;
		}
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
	Entry.Element = Element;
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
		// A ward made again (a new life) may roll a new element; a broken one naming none keeps the last.
		const ELooterElement Element = Traits->GetWardElement();
		if (Element != ELooterElement::None)
		{
			Entry.Element = Element;
		}
	}
}

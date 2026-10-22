#include "UI/HUD/HudWardHit.h"
#include "Combat/HealthComponent.h"
#include "Shields/PlayerShieldComponent.h"
#include "GameFramework/Pawn.h"
#include "HAL/IConsoleManager.h"

namespace
{
	const HudWardHit::FTuning WardHitDefaults;

	TAutoConsoleVariable<float> CVarWardHitEdgeSeconds(TEXT("Looter.Shield.Hud.EdgeSeconds"), WardHitDefaults.EdgeSeconds,
		TEXT("A hit the ward soaked tints the screen's edges cyan, fading over this long (s)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarWardHitEdgePeak(TEXT("Looter.Shield.Hud.EdgePeak"), WardHitDefaults.EdgePeak,
		TEXT("The cyan edges' strength at the hit (0-1; the red's is 0.85)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarWardHitGlintSeconds(TEXT("Looter.Shield.Hud.GlintSeconds"), WardHitDefaults.GlintSeconds,
		TEXT("A hit the ward soaked glints the portrait's window cyan, fading over this long (s)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarWardHitGlintPeak(TEXT("Looter.Shield.Hud.GlintPeak"), WardHitDefaults.GlintPeak,
		TEXT("The portrait's cyan glint's strength at the hit (0-1)."), ECVF_Default);
}

HudWardHit::FReaction HudWardHit::Decide(const FSeen& Seen)
{
	FReaction Reaction;
	const bool bBroke = Seen.WardBreaks > 0;
	const bool bHealthFell = Seen.HealthLost > HealthStep;
	Reaction.bWound = bBroke || bHealthFell;
	Reaction.bWard = bBroke || (Seen.WardHits > 0 && (Seen.ChargeBefore > 0.f || !bHealthFell));
	return Reaction;
}

HudWardHit::FTuning HudWardHit::FTuning::Live()
{
	FTuning Tuning;
	Tuning.EdgeSeconds = FMath::Max(CVarWardHitEdgeSeconds.GetValueOnGameThread(), 0.01f);
	Tuning.EdgePeak = FMath::Clamp(CVarWardHitEdgePeak.GetValueOnGameThread(), 0.f, 1.f);
	Tuning.GlintSeconds = FMath::Max(CVarWardHitGlintSeconds.GetValueOnGameThread(), 0.01f);
	Tuning.GlintPeak = FMath::Clamp(CVarWardHitGlintPeak.GetValueOnGameThread(), 0.f, 1.f);
	return Tuning;
}

UHealthComponent* FHudWardHitWatch::GetHealth() const
{
	return Health.Get();
}

UPlayerShieldComponent* FHudWardHitWatch::GetShield() const
{
	return Shield.Get();
}

HudWardHit::FReaction FHudWardHitWatch::Look(APawn* Pawn)
{
	if (Pawn != Watched.Get())
	{
		// A new pawn (a respawn, or none): taken as it is, nothing to react to.
		Watched = Pawn;
		Health = Pawn ? Pawn->FindComponentByClass<UHealthComponent>() : nullptr;
		Shield = nullptr;
		bShieldSeen = false;
		LastHealth = -1.f;
	}
	if (!Pawn)
	{
		return {};
	}
	// The weapon manager adds the ward's component as play begins (or ALooterCharacter makes it): looked for until found.
	if (!Shield.IsValid())
	{
		Shield = UPlayerShieldComponent::Find(Pawn);
		bShieldSeen = false;
	}

	HudWardHit::FSeen Seen;
	bool bPrimed = true;
	if (const UHealthComponent* Watching = Health.Get())
	{
		const float Points = Watching->GetHealth();
		bPrimed = LastHealth >= 0.f;
		Seen.HealthLost = bPrimed ? FMath::Max(LastHealth - Points, 0.f) : 0.f;
		LastHealth = Points;
	}
	if (const UPlayerShieldComponent* Ward = Shield.Get())
	{
		const uint32 Hits = Ward->GetHitCount();
		const uint32 Breaks = Ward->GetBreakCount();
		if (bShieldSeen)
		{
			Seen.WardHits = Hits - LastHits;
			Seen.WardBreaks = Breaks - LastBreaks;
			Seen.ChargeBefore = LastCharge;
		}
		else
		{
			bPrimed = false;
		}
		LastHits = Hits;
		LastBreaks = Breaks;
		LastCharge = Ward->GetCharge();
		bShieldSeen = true;
	}
	return bPrimed ? HudWardHit::Decide(Seen) : HudWardHit::FReaction();
}

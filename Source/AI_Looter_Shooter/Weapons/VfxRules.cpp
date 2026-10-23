#include "Weapons/VfxRules.h"
#include "Weapons/VfxTuning.h"

// The VFX pass's plain rules (Saved/Vfx/PLAN.md): detail, weights, tiers, tracers, blasts and the shapes' curves (the
// muzzle's in VfxRulesMuzzle.cpp).

namespace VfxRules
{
	float DetailAt(float Distance, const FVfxTuning& Tuning)
	{
		if (Distance <= Tuning.DetailNear)
		{
			return 1.f;
		}
		const float Span = FMath::Max(Tuning.DetailFar - Tuning.DetailNear, 1.f);
		const float T = FMath::Clamp((Distance - Tuning.DetailNear) / Span, 0.f, 1.f);
		// Smooth, so a creature running away doesn't step through its looks' sizes.
		return FMath::Lerp(1.f, Tuning.DetailMin, T * T * (3.f - 2.f * T));
	}

	float StreamDetail(int32 RecentHits)
	{
		if (RecentHits <= 2)
		{
			return 1.f;
		}
		return FMath::Max(1.f / FMath::Sqrt(static_cast<float>(RecentHits - 1)), 0.25f);
	}

	int32 Count(int32 Full, float Amount)
	{
		if (Full <= 0 || Amount <= 0.f)
		{
			return 0;
		}
		return FMath::Max(1, FMath::RoundToInt32(static_cast<float>(Full) * Amount));
	}

	float HitWeight(float MarkScale)
	{
		// A pellet's pop is small (nine land together), a sniper's round the biggest; between, as the marks grow.
		return FMath::Clamp(0.25f + 0.85f * MarkScale, 0.6f, 1.7f);
	}

	float ImpactViewScale(float Distance)
	{
		const float Far = FMath::Clamp((Distance - 1000.f) / 2000.f, 0.f, 1.f);
		return 1.f + 0.8f * Far;
	}

	bool IsHeavyRound(float MarkScale)
	{
		return MarkScale >= 1.2f;
	}

	float KillWeight(EWeaponKind Gun, bool bCritical, float Share)
	{
		float Weight = 1.f;
		switch (Gun)
		{
		case EWeaponKind::Shotgun: Weight = 1.35f; break;
		case EWeaponKind::Revolver: Weight = 1.15f; break;
		case EWeaponKind::Sniper: Weight = 1.4f; break;
		case EWeaponKind::Launcher: Weight = 1.6f; break;
		case EWeaponKind::SMG: Weight = 0.9f; break;
		case EWeaponKind::LMG: Weight = 1.05f; break;
		case EWeaponKind::Rifle: Weight = 1.f; break;
		// A power's or a burn's kill: as a rifle's, a touch more (it's the player's big moment).
		case EWeaponKind::None:
		default: Weight = 1.1f; break;
		}
		if (bCritical)
		{
			Weight *= 1.25f;
		}
		// Overkill: a killing hit worth more than the health it took reads as the body giving way all at once.
		Weight += FMath::Clamp((Share - 1.f) * 0.15f, 0.f, 0.3f);
		return FMath::Clamp(Weight, 0.6f, 2.2f);
	}

	float KillCarry(EWeaponKind Gun)
	{
		switch (Gun)
		{
		case EWeaponKind::Shotgun: return 0.85f;
		case EWeaponKind::Launcher: return 0.7f;
		case EWeaponKind::Sniper: return 0.6f;
		case EWeaponKind::Revolver: return 0.5f;
		default: return 0.35f;
		}
	}

	FVfxTierLayers TierLayers(ECreatureRank Rank)
	{
		FVfxTierLayers Layers;
		switch (Rank)
		{
		case ECreatureRank::Rare:
			Layers.Rings = 1;
			Layers.RingReach = 1.6f;
			Layers.Star = 0.9f;
			Layers.Motes = 6;
			break;
		case ECreatureRank::Epic:
			Layers.Rings = 2;
			Layers.RingReach = 2.f;
			Layers.Star = 1.3f;
			Layers.Motes = 12;
			break;
		case ECreatureRank::Hallowed:
		case ECreatureRank::Ethereal:
			// Their own show (ring, pillar, motes) is the big moment: a star of their colour on the body, nothing that doubles it.
			Layers.Rings = 0;
			Layers.Star = 1.5f;
			Layers.Motes = 0;
			Layers.bOwnShow = true;
			break;
		case ECreatureRank::Legendary:
			Layers.Rings = 2;
			Layers.RingReach = 2.6f;
			Layers.Star = 1.8f;
			Layers.Motes = 16;
			break;
		case ECreatureRank::Basic:
		case ECreatureRank::Boss:
		default:
			break;
		}
		return Layers;
	}

	FVfxTracerLook TracerLook(EWeaponKind Kind, int32 RoundsLeft)
	{
		FVfxTracerLook Look;
		switch (Kind)
		{
		case EWeaponKind::SMG:
			Look.Width = 0.75f;
			Look.Length = 0.8f;
			break;
		case EWeaponKind::Sniper:
			Look.Width = 1.6f;
			Look.Length = 1.3f;
			Look.Glow = 1.4f;
			break;
		case EWeaponKind::Revolver:
			Look.Width = 1.2f;
			break;
		case EWeaponKind::Shotgun:
			Look.Length = 0.6f;
			break;
		case EWeaponKind::LMG:
			// A belt's tracer every third round; the rest dimmer, so a long burst reads as a stream with a beat.
			if (RoundsLeft % 3 != 0)
			{
				Look.Glow = 0.5f;
				Look.Width = 0.8f;
			}
			break;
		default:
			break;
		}
		return Look;
	}

	FVfxExplosionLook ExplosionLook(float Radius)
	{
		FVfxExplosionLook Look;
		// A rocket's 350 cm is the design's middle; Slag Bomb's 450 a little more, a Grave-burst's 300 a little less.
		const float Big = FMath::Clamp(Radius / 350.f, 0.5f, 1.6f);
		Look.Fireballs = FMath::Clamp(FMath::RoundToInt32(5.f * Big), 3, 8);
		Look.SmokeRing = FMath::Clamp(FMath::RoundToInt32(12.f * Big), 6, 16);
		Look.SmokeColumn = FMath::Clamp(FMath::RoundToInt32(6.f * Big), 3, 9);
		Look.Embers = FMath::Clamp(FMath::RoundToInt32(22.f * Big), 10, 32);
		Look.Debris = FMath::Clamp(FMath::RoundToInt32(10.f * Big), 5, 14);
		Look.FireballLife = 0.55f + 0.15f * Big;
		return Look;
	}

	float RingGrowth(float T)
	{
		const float U = 1.f - FMath::Clamp(T, 0.f, 1.f);
		return 1.f - U * U * U;
	}

	float RingFade(float T)
	{
		const float U = 1.f - FMath::Clamp(T, 0.f, 1.f);
		return U * U;
	}

	float FireballHeat(float T)
	{
		return FMath::Pow(1.f - FMath::Clamp(T, 0.f, 1.f), 0.7f);
	}

	float FireballGrowth(float T)
	{
		return FMath::Sqrt(FMath::Clamp(T, 0.f, 1.f));
	}

	FLinearColor HeatColor(const FLinearColor& Color, float Heat)
	{
		const FLinearColor White(1.f, 0.95f, 0.85f);
		const FLinearColor Ember(0.35f, 0.06f, 0.02f);
		const float H = FMath::Clamp(Heat, 0.f, 1.f);
		return H >= 0.5f ? FMath::Lerp(Color, White, (H - 0.5f) * 2.f) : FMath::Lerp(Ember, Color, H * 2.f);
	}

	int32 RingPieces(float Radius)
	{
		return FMath::Clamp(FMath::RoundToInt32(Radius / 12.f), 10, 28);
	}
}

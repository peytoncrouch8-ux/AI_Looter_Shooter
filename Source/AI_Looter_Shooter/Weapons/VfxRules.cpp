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

	float ImpactWeight(EWeaponKind Kind, const FVfxTuning& Tuning)
	{
		switch (Kind)
		{
		case EWeaponKind::Rifle: return Tuning.ImpactWeightRifle;
		case EWeaponKind::Shotgun: return Tuning.ImpactWeightShotgun;
		case EWeaponKind::Revolver: return Tuning.ImpactWeightRevolver;
		case EWeaponKind::SMG: return Tuning.ImpactWeightSMG;
		case EWeaponKind::Sniper: return Tuning.ImpactWeightSniper;
		case EWeaponKind::LMG: return Tuning.ImpactWeightLMG;
		// A rocket's burst is its own look (SpawnExplosion); a hit with no gun keeps the look every hit had.
		case EWeaponKind::Launcher:
		case EWeaponKind::None:
		default:
			return 1.f;
		}
	}

	float ImpactWeight(EWeaponKind Kind)
	{
		return ImpactWeight(Kind, FVfxTuning::Live());
	}

	FVfxImpactLook ImpactLook(EWeaponKind Kind, int32 RoundsLeft, EImpactMaterial Material, const FVfxTuning& Tuning)
	{
		FVfxImpactLook Look;
		if (Kind == EWeaponKind::None || Kind == EWeaponKind::Launcher)
		{
			return Look;
		}
		float Weight = ImpactWeight(Kind, Tuning);
		if (Kind == EWeaponKind::LMG)
		{
			// The belt's beat, as its tracers: a full round, then lighter ones, so a long burst reads as a saw with a rhythm,
			// not a solid wall of dirt over the target.
			const int32 Every = FMath::Max(Tuning.ImpactLMGFullEvery, 1);
			const bool bFull = Every == 1 || RoundsLeft % Every == 0;
			Look.bChunks = bFull;
			if (!bFull)
			{
				Weight *= Tuning.ImpactLMGLight;
			}
		}
		Look.Count = Weight;
		Look.bPellet = Kind == EWeaponKind::Shotgun;
		// A pellet's hit is sized by its mark already (nine land together; the sheet carries the blast); a round's by its weight.
		Look.Size = Look.bPellet ? 1.f : FMath::Pow(FMath::Max(Weight, 0.f), Tuning.ImpactSizePower);
		if (!Look.bPellet)
		{
			Look.bChunks |= IsHardSurface(Material) && Weight >= Tuning.ImpactChunkFrom;
			Look.bPunch = Weight >= Tuning.ImpactPunchFrom;
		}
		return Look;
	}

	bool IsHardSurface(EImpactMaterial Material)
	{
		switch (Material)
		{
		case EImpactMaterial::Gravel:
		case EImpactMaterial::Stone:
		case EImpactMaterial::Wood:
		case EImpactMaterial::Metal:
		case EImpactMaterial::SheetMetal:
		case EImpactMaterial::Glass:
		case EImpactMaterial::Bone:
			return true;
		default:
			return false;
		}
	}

	EVfxSheet SheetOf(EImpactMaterial Material)
	{
		switch (Material)
		{
		case EImpactMaterial::Gravel:
		case EImpactMaterial::Stone:
		case EImpactMaterial::Metal:
		case EImpactMaterial::SheetMetal:
			return EVfxSheet::Hard;
		case EImpactMaterial::Water:
			return EVfxSheet::Splash;
		case EImpactMaterial::Unknown:
		case EImpactMaterial::Count:
			return EVfxSheet::None;
		default:
			return EVfxSheet::Soft;
		}
	}

	EImpactMaterial GuessSurface(EImpactMaterial Material, const FVector& Normal)
	{
		if (Material != EImpactMaterial::Unknown)
		{
			return Material;
		}
		return Normal.GetSafeNormal().Z >= 0.6 ? EImpactMaterial::Dirt : EImpactMaterial::Stone;
	}

	float GlowShare(int32 RecentHits)
	{
		// Additive light piles up: two glows at one spot are twice as bright. Sharing it keeps a stream or a blast about as
		// bright as two hits, never a white disc.
		return 1.f / static_cast<float>(FMath::Max(RecentHits, 1));
	}

	float HitFlashSizeAt(float Size, float Distance, float MaxRadians)
	{
		return StreakWidthAt(Size, Distance, MaxRadians);
	}

	float SheetRadius(float Spread)
	{
		// A point-blank blast's pellets land in a fist; at 15 m they cover a door. The sheet spans a little past them.
		return FMath::Clamp(40.f + 1.2f * FMath::Max(Spread, 0.f), 40.f, 220.f);
	}

	int32 SheetPuffs(float Radius)
	{
		return FMath::Clamp(FMath::RoundToInt32(Radius / 45.f), 3, 5);
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
		// Overkill: a killing hit worth more than the health it took reads as the body giving way all at once (HITS,
		// 2026-10-28: up to 0.6 more, was 0.3, and the cap 2.4, was 2.2, so a sniper's overkill reads past a plain kill).
		Weight += FMath::Clamp((Share - 1.f) * 0.3f, 0.f, 0.6f);
		return FMath::Clamp(Weight, 0.6f, 2.4f);
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

	float SpriteRoll(float Roll, float Age)
	{
		// Its pace and way from its roll's digits (no new field per particle); it eases off as a billow slows.
		const float Pace = FMath::Lerp(0.25f, 0.7f, FMath::Frac(Roll * 3.17f));
		const float Way = FMath::Frac(Roll * 7.31f) < 0.5f ? -1.f : 1.f;
		const float Time = FMath::Max(Age, 0.f);
		return Roll + Way * Pace * Time / (1.f + 0.6f * Time);
	}

	float GlowStretch(float Speed, float Size)
	{
		return FMath::Clamp(1.f + FMath::Max(Speed, 0.f) * GlowShutter / FMath::Max(Size, 0.5f), 1.f, GlowMostStretch);
	}
}

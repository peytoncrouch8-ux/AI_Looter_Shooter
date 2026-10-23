#include "Weapons/VfxRules.h"
#include "Weapons/VfxTuning.h"

// The muzzle's plain rules (split from VfxRules.cpp by the muzzle rework, 2026-10-22). The user: the hip-fire flash showed
// flat yellow diamonds up close, covering what was shot at. The flash is now layered light on M_FX_Muzzle (a white-hot
// core, a soft orange bloom, thin tapered star spikes, short tongues), every layer soft-edged and faded as it turns edge-on;
// in first person it is sized per layer, keeps clear of the crosshair and lasts about two frames. These rules give each
// kind's look, the quads, their size per view, the fade and the crosshair's clearance; AWeaponBase (WeaponBaseMuzzle.cpp)
// draws them and FWeaponFX::SpawnMuzzleBurst throws the sparks and smoke.

namespace
{
	// Strengths at full, on M_FX_Muzzle (additive): the tiny core hottest, burning white; the star's spikes and tongues are thin
	// gaussian lines, so only their hearts saturate and their soft edges keep the colour; the bloom low and wide (the old
	// flat-filled quads at 14-26 washed to whole pale cards). Tuned on the previews (Saved/ArtPreviews/Weapons/Muzzle).
	constexpr float MuzzleCoreGlow = 16.f;
	constexpr float MuzzleBloomGlow = 4.f;
	constexpr float MuzzleStarGlow = 10.f;
	constexpr float MuzzleTongueGlow = 9.f;
	constexpr float MuzzleJetGlow = 9.f;
	constexpr float MuzzleSpitGlow = 8.f;
	// White-yellow core; orange bloom; the star and tongues between (the material whitens each toward its heart).
	const FLinearColor MuzzleCoreColor(1.f, 0.9f, 0.72f);
	const FLinearColor MuzzleBloomColor(1.f, 0.45f, 0.12f);
	const FLinearColor MuzzleStarColor(1.f, 0.66f, 0.3f);
	const FLinearColor MuzzleTongueColor(1.f, 0.55f, 0.18f);
	/** The round layers face back down the barrel, a little out past the muzzle so the gun's own face never cuts them. */
	const FRotator MuzzleFacingBack(-90.f, 0.f, 0.f);
	constexpr float MuzzleCoreOut = 1.5f;
	constexpr float MuzzleBloomOut = 2.5f;
	constexpr float MuzzleStarOut = 3.f;
	/** A brake's jets sit this far behind the crown (cm at scale 1), and are this wide; a revolver's cylinder gap this far back. */
	constexpr float MuzzleBrakeBack = 3.f;
	constexpr float MuzzleJetWidth = 8.f;
	constexpr float MuzzleGapBack = 16.f;
	constexpr float MuzzleGapSide = 2.f;
	/** A spit's softness (between a core's and a bloom's). */
	constexpr float MuzzleSpitSoftness = 0.3f;
	/** The least share of a streak's length a shot shows. */
	constexpr float MuzzleStreakShortest = 0.55f;
}

namespace VfxRules
{
	FVfxMuzzleLook MuzzleLook(EWeaponKind Kind)
	{
		FVfxMuzzleLook Look;
		switch (Kind)
		{
		case EWeaponKind::Shotgun:
			// Big, wide and angry: a broad bloom, a cone of tongues, a cloud of unburnt powder.
			Look.CoreSize = 16.f;
			Look.BloomSize = 40.f;
			Look.StarSize = 46.f;
			Look.StarSpikes = 9;
			Look.Tongues = 5;
			Look.TongueLength = 30.f;
			Look.TongueWidth = 14.f;
			Look.Duration = 0.05f;
			Look.Sparks = 9;
			Look.SmokePuffs = 3;
			Look.SmokeSize = 46.f;
			break;
		case EWeaponKind::Revolver:
			// A hard star, the cylinder gap's spit to both sides.
			Look.CoreSize = 13.f;
			Look.BloomSize = 30.f;
			Look.StarSize = 40.f;
			Look.StarSpikes = 8;
			Look.Tongues = 3;
			Look.TongueLength = 30.f;
			Look.Duration = 0.045f;
			Look.GapSpit = 9.f;
			Look.Sparks = 5;
			Look.SmokePuffs = 2;
			Look.SmokeSize = 36.f;
			break;
		case EWeaponKind::SMG:
			// Small and quick, flickering: a cross of four, every third shot's sparks and smoke.
			Look.CoreSize = 8.f;
			Look.BloomSize = 18.f;
			Look.StarSize = 24.f;
			Look.StarSpikes = 4;
			Look.Tongues = 2;
			Look.TongueLength = 18.f;
			Look.TongueWidth = 7.f;
			Look.Duration = 0.03f;
			Look.Sparks = 1;
			Look.SmokePuffs = 1;
			Look.SmokeSize = 20.f;
			Look.Every = 3;
			break;
		case EWeaponKind::Sniper:
			// (Toned down 2026-10-22 after the play-check: its hip starburst was large.)
			// A sharp flash: a tight bloom, a big star of many thin bright spikes, the brake's two side jets, a ring of dust and
			// air thrown out round the muzzle, hanging smoke.
			Look.CoreSize = 14.f;
			Look.BloomSize = 22.f;
			Look.StarSize = 36.f;
			Look.StarSpikes = 10;
			Look.StarGlow = 1.1f;
			Look.Tongues = 3;
			Look.TongueLength = 34.f;
			Look.TongueWidth = 10.f;
			Look.Duration = 0.04f;
			Look.SideJets = 34.f;
			Look.Sparks = 6;
			Look.SmokePuffs = 4;
			Look.SmokeSize = 52.f;
			Look.Ring = 150.f;
			break;
		case EWeaponKind::LMG:
			// Heavy and steady: a big uneven star, four tongues, its smoke building over a long burst.
			Look.CoreSize = 13.f;
			Look.BloomSize = 30.f;
			Look.StarSize = 36.f;
			Look.StarSpikes = 7;
			Look.Tongues = 4;
			Look.TongueLength = 30.f;
			Look.TongueWidth = 10.f;
			Look.Duration = 0.04f;
			Look.Sparks = 3;
			Look.SmokePuffs = 1;
			Look.SmokeSize = 34.f;
			Look.Every = 2;
			break;
		case EWeaponKind::Launcher:
			// A puff: the rocket's motor is the flash, a broad soft bloom and fat short tongues, the star faint under it; a ring of
			// dust, a lot of smoke (the back-blast is the rocket's own).
			Look.CoreSize = 15.f;
			Look.BloomSize = 52.f;
			Look.StarSize = 30.f;
			Look.StarSpikes = 12;
			Look.StarGlow = 0.35f;
			Look.Tongues = 4;
			Look.TongueLength = 30.f;
			Look.TongueWidth = 18.f;
			Look.Duration = 0.06f;
			Look.Sparks = 10;
			Look.SmokePuffs = 5;
			Look.SmokeSize = 70.f;
			Look.Ring = 200.f;
			break;
		case EWeaponKind::Rifle:
		case EWeaponKind::None:
		default:
			// The measure: core, bloom, a six-point star and three tongues; a spark or three and a breath of smoke.
			Look.Sparks = 3;
			Look.SmokePuffs = 1;
			Look.SmokeSize = 28.f;
			Look.Every = 2;
			break;
		}
		return Look;
	}

	void MuzzleQuads(const FVfxMuzzleLook& Look, TArray<FVfxFlashQuad>& Out)
	{
		auto Add = [&Out](EVfxFlashLayer Layer, EVfxFlashShape Shape, const FVector& Location, const FRotator& Rotation, float Length,
			float Width, const FLinearColor& Color, float Glow, float Param)
		{
			FVfxFlashQuad& Quad = Out.AddDefaulted_GetRef();
			Quad.Layer = Layer;
			Quad.Shape = Shape;
			Quad.Location = Location;
			Quad.Rotation = Rotation;
			Quad.Length = Length;
			Quad.Width = Width;
			Quad.Color = Color;
			Quad.Glow = Glow;
			Quad.Param = Param;
		};
		// A soft orange bloom, the white-hot core in it, the star of spikes: all facing back at the shooter (additive: they add up)...
		Add(EVfxFlashLayer::Bloom, EVfxFlashShape::Round, FVector(MuzzleBloomOut, 0.f, 0.f), MuzzleFacingBack, Look.BloomSize, Look.BloomSize,
			MuzzleBloomColor, MuzzleBloomGlow, 1.f);
		Add(EVfxFlashLayer::Core, EVfxFlashShape::Round, FVector(MuzzleCoreOut, 0.f, 0.f), MuzzleFacingBack, Look.CoreSize, Look.CoreSize,
			MuzzleCoreColor, MuzzleCoreGlow, 0.f);
		Add(EVfxFlashLayer::Star, EVfxFlashShape::Star, FVector(MuzzleStarOut, 0.f, 0.f), MuzzleFacingBack, Look.StarSize, Look.StarSize,
			MuzzleStarColor, MuzzleStarGlow * Look.StarGlow, static_cast<float>(FMath::Clamp(Look.StarSpikes, 3, 16)));
		// ...tongues along the barrel, spread evenly about it, for side views (turned so the streak's hot head is at the muzzle)...
		const int32 Tongues = FMath::Max(Look.Tongues, 0);
		for (int32 Tongue = 0; Tongue < Tongues; ++Tongue)
		{
			Add(EVfxFlashLayer::Tongue, EVfxFlashShape::Streak, FVector(Look.TongueLength * 0.5f, 0.f, 0.f),
				FRotator(0.f, 180.f, Tongue * 180.f / Tongues), Look.TongueLength, Look.TongueWidth, MuzzleTongueColor, MuzzleTongueGlow, 1.f);
		}
		// ...a muzzle brake's two jets blown out square to the barrel...
		if (Look.SideJets > 0.f)
		{
			for (const float Side : { -1.f, 1.f })
			{
				Add(EVfxFlashLayer::Jet, EVfxFlashShape::Streak, FVector(-MuzzleBrakeBack, Side * Look.SideJets * 0.5f, 0.f),
					FRotator(0.f, -90.f * Side, 0.f), Look.SideJets, MuzzleJetWidth, MuzzleTongueColor, MuzzleJetGlow, 1.f);
			}
		}
		// ...and a revolver's spit out of the cylinder gap, either side.
		if (Look.GapSpit > 0.f)
		{
			for (const float Side : { -1.f, 1.f })
			{
				Add(EVfxFlashLayer::Spit, EVfxFlashShape::Round, FVector(-MuzzleGapBack, Side * MuzzleGapSide, 0.f), MuzzleFacingBack,
					Look.GapSpit, Look.GapSpit, MuzzleCoreColor, MuzzleSpitGlow, MuzzleSpitSoftness);
			}
		}
	}

	FVfxMuzzleView MuzzleView(bool bFirstPerson, float AimAlpha, const FVfxTuning& Tuning)
	{
		FVfxMuzzleView View;
		View.Size = Tuning.MuzzleFlash;
		if (!bFirstPerson)
		{
			return View;
		}
		View.Star = Tuning.MuzzleStar1P;
		View.Tongue = Tuning.MuzzleTongue1P;
		View.Bloom = Tuning.MuzzleBloom1P;
		View.Jet = Tuning.MuzzleJets1P;
		const float Aim = FMath::Clamp(AimAlpha, 0.f, 1.f);
		View.Size *= FMath::Lerp(1.f, Tuning.MuzzleAim, Aim);
		View.Glow = FMath::Lerp(1.f, Tuning.MuzzleAimGlow, Aim);
		return View;
	}

	float MuzzleLayerScale(EVfxFlashLayer Layer, const FVfxMuzzleView& View)
	{
		switch (Layer)
		{
		case EVfxFlashLayer::Bloom:
			return View.Bloom;
		case EVfxFlashLayer::Star:
			return View.Star;
		case EVfxFlashLayer::Tongue:
			return View.Tongue;
		case EVfxFlashLayer::Jet:
		case EVfxFlashLayer::Spit:
			return View.Jet;
		case EVfxFlashLayer::Core:
		default:
			return View.Core;
		}
	}

	FTransform MuzzleQuadTransform(const FVfxFlashQuad& Quad, float LayerScale, float GunScale)
	{
		const float Scale = FMath::Max(LayerScale, 0.f) * FMath::Max(GunScale, 0.f);
		const bool bStreak = Quad.Shape == EVfxFlashShape::Streak;
		const float WidthScale = bStreak ? FMath::Sqrt(FMath::Max(LayerScale, 0.f)) * FMath::Max(GunScale, 0.f) : Scale;
		// Kept above nothing: a quad scaled to zero is a degenerate transform.
		return FTransform(Quad.Rotation, Quad.Location * Scale,
			FVector(FMath::Max(Quad.Length * Scale, 0.01f) / 100.f, FMath::Max(Quad.Width * WidthScale, 0.01f) / 100.f, 1.f));
	}

	float MuzzleReach(const TArray<FVfxFlashQuad>& Quads, const FVfxMuzzleView& View, bool bAlongBarrel)
	{
		float Reach = 0.f;
		for (const FVfxFlashQuad& Quad : Quads)
		{
			const float Scale = MuzzleLayerScale(Quad.Layer, View);
			const FTransform Placed = MuzzleQuadTransform(Quad, Scale, 1.f);
			if (Quad.Shape == EVfxFlashShape::Streak)
			{
				// A streak tapers to nothing: its reach is its middle line's two ends (the Plane is -50..50 in X).
				for (const float End : { -50.f, 50.f })
				{
					const FVector Point = Placed.TransformPosition(FVector(End, 0.f, 0.f));
					Reach = FMath::Max(Reach, bAlongBarrel ? static_cast<float>(Point.X) : static_cast<float>(FVector2D(Point.Y, Point.Z).Size()));
				}
				continue;
			}
			// A round glow or a star fills the circle in its quad (facing back down the barrel: the circle is square to it).
			const FVector Middle = Placed.GetLocation();
			const float Radius = Quad.Length * Scale * 0.5f;
			Reach = FMath::Max(Reach, bAlongBarrel ? static_cast<float>(Middle.X) : static_cast<float>(FVector2D(Middle.Y, Middle.Z).Size()) + Radius);
		}
		return Reach * View.Size;
	}

	float MuzzleFade(float Age, float Duration, float Power)
	{
		if (Duration <= 0.f || Age >= Duration)
		{
			return 0.f;
		}
		if (Age <= 0.f)
		{
			return 1.f;
		}
		return FMath::Pow(1.f - Age / Duration, FMath::Max(Power, 0.01f));
	}

	float MuzzleClearScale(float MuzzleDegrees, float NearestDegrees, float ClearDegrees, float MinScale)
	{
		const float Floor = FMath::Clamp(MinScale, 0.f, 1.f);
		// How far toward the middle the flash reaches past its muzzle at full size, and how far it may.
		const float Reach = MuzzleDegrees - NearestDegrees;
		if (Reach <= KINDA_SMALL_NUMBER)
		{
			return 1.f;
		}
		const float Room = MuzzleDegrees - FMath::Max(ClearDegrees, 0.f);
		if (Room >= Reach)
		{
			return 1.f;
		}
		return FMath::Clamp(Room / Reach, Floor, 1.f);
	}

	float MuzzleClearShare(float ClearDegrees, float HorizontalFovDegrees)
	{
		if (ClearDegrees <= 0.f)
		{
			return 0.f;
		}
		const float HalfFov = FMath::DegreesToRadians(FMath::Clamp(HorizontalFovDegrees, 10.f, 170.f) * 0.5f);
		const float Clear = FMath::DegreesToRadians(FMath::Min(ClearDegrees, 60.f));
		// Half the screen's width is tan(HalfFov) out at a unit's distance.
		return 0.5f * FMath::Tan(Clear) / FMath::Tan(HalfFov);
	}

	float MuzzleStreakShare(float Random)
	{
		return FMath::Lerp(MuzzleStreakShortest, 1.f, FMath::Clamp(Random, 0.f, 1.f));
	}

	float MuzzleSizeCap(float FlashDegrees, float MaxDegrees)
	{
		if (MaxDegrees <= 0.f || FlashDegrees <= MaxDegrees)
		{
			return 1.f;
		}
		return MaxDegrees / FlashDegrees;
	}

	float StreakWidthAt(float Width, float Distance, float MaxRadians)
	{
		if (MaxRadians <= 0.f)
		{
			return Width;
		}
		// Never under a hair (a spark right at the eye is still a spark, not nothing).
		return FMath::Min(Width, FMath::Max(Distance * MaxRadians, 0.05f));
	}
}

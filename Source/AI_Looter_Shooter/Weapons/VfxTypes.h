#pragma once

#include "CoreMinimal.h"
#include "Creatures/CreatureRank.h"
#include "Weapons/CreatureBlood.h"
#include "Weapons/ImpactSurface.h"
#include "Weapons/WeaponTypes.h"

/**
 * The plain types of the VFX pass (Saved/Vfx/PLAN.md, 2026-10-21 night): what a kill, a blast and a muzzle hand the
 * code-drawn effects (FWeaponFX::SpawnKillLayers, SpawnExplosion, SpawnMuzzleBurst), and the shapes M_FX_Shape draws.
 * No engine state here: WeaponFX.h includes it, and the rules' tests (Looter.Vfx.*) build these by hand.
 */

/**
 * What M_FX_Shape (Tools/Unreal/build_vfx_materials.py) draws on a quad, by its custom data's shape float. Without the
 * material each falls back to M_FX_Glow's own (a ring in streak pieces, a fireball as a round glow, a starburst as its
 * six-pointed star).
 */
enum class EVfxShape : uint8
{
	/** A ring: Param its thickness as a share of the radius. Lies square to its axis (a shock ring on the ground) or faces the eye. */
	Ring = 0,
	/** A ball of fire or light that cools: Param its heat (1 white-hot at the heart, 0 a dull ember edge eaten by noise). */
	Fireball = 1,
	/** A starburst: Param its spikes (3-16), uneven by its seed, round a hot core. */
	Starburst = 2
};

/**
 * One puff of fire rolling into smoke (FWeaponFX::SpawnFlame; M_FX_Fire, alpha-blended, VFX-2 2026-10-22): it burns for
 * Burn of its life (white-yellow at the heart, its fire's colour, a deep red edge, cooling from the outside in), then is
 * dark smoke for the rest, growing and eroding into wisps. Alpha-blended, its fire keeps its colour in full sun, where an
 * additive fireball washed out to a pale cloud. Burn 0: smoke alone (a blast's rising soot column).
 */
struct FVfxFlame
{
	FVector Location = FVector::ZeroVector;
	FVector Velocity = FVector::ZeroVector;
	float StartSize = 50.f;
	float EndSize = 150.f;
	/** The fire's colour (a hue near 1: the shader brightens it) and its smoke's (as the light shows it: unlit). */
	FLinearColor Fire = FLinearColor(1.f, 0.42f, 0.1f);
	FLinearColor Smoke = FLinearColor(0.07f, 0.06f, 0.055f);
	float Opacity = 0.95f;
	/** Its whole life (s), the share of it on fire (0..1), and how long after the spawn it shows (s). */
	float Life = 1.8f;
	float Burn = 0.3f;
	float Delay = 0.f;
	/** Times gravity (negative rises: hot air and smoke), and air drag per second. */
	float Gravity = -0.2f;
	float Drag = 3.f;
};

/** A creature's kill as the effects see it: on top of its kind's death burst (FWeaponFX::SpawnKillLayers). */
struct FVfxKill
{
	/** What it spills (spiders' ichor, the Gravemother's dark ichor, a slime's gel, the Unpaid's embers...). */
	ECreatureBlood Blood = ECreatureBlood::Ichor;
	/** Its colour: the chitin, the gel, the coal. */
	FLinearColor Tint = FLinearColor::White;
	/** The ichor or gel's own colour for its mist and drops (the spiders' chitin isn't their ichor). */
	FLinearColor Spill = FLinearColor::White;
	/** The body's middle as drawn, and the ground under it (bGround: found). */
	FVector Center = FVector::ZeroVector;
	FVector Ground = FVector::ZeroVector;
	FVector GroundNormal = FVector::UpVector;
	bool bGround = false;
	/** The killing shot's way (unit; zero: none known) and where it landed (zero: not known). */
	FVector Direction = FVector::ZeroVector;
	FVector HitAt = FVector::ZeroVector;
	/** The body's size (cm; 100 for a spider at size 1). */
	float Size = 100.f;
	ECreatureRank Rank = ECreatureRank::Basic;
	/** Its tier's glow colour (CreatureTierLook::GlowColor), for the tiers above Regular. */
	FLinearColor TierColor = FLinearColor::White;
	/** The gun that killed it (None: a power, a burn, the world). */
	EWeaponKind Gun = EWeaponKind::None;
	bool bCritical = false;
	/** The killing hit's share of its health (how much it was overkilled: a sniper's head shot on a spider is 2+). */
	float Share = 0.f;
	/** The light unlit dust and mist show in (UCreatureReactSubsystem::GetDustLight; white without one). */
	FLinearColor DustLight = FLinearColor::White;
};

/** A blast (a rocket's burst, Slag Bomb's, a Grave-burst): fire or soul-light, its ring over the ground, smoke, debris. */
struct FVfxExplosion
{
	FVector Center = FVector::ZeroVector;
	/** The surface's up at the burst (the ground's: its ring lies on it); world up in the air. */
	FVector Up = FVector::UpVector;
	/** Its reach (cm): sizes everything. */
	float Radius = 350.f;
	/** The fire's colour (a rocket's ember orange; a soul blast's power colour). */
	FLinearColor Fire = FLinearColor(1.f, 0.42f, 0.1f);
	/** Soul-light rather than fire: no soot, motes rising instead of embers, the ring brighter. */
	bool bSoul = false;
	/** What it burst on (its debris), Unknown in the air. */
	EImpactMaterial Ground = EImpactMaterial::Dirt;
	bool bOnGround = true;
	/** The light the smoke shows in (white without one). */
	FLinearColor DustLight = FLinearColor::White;
};

/** One shot's muzzle as the world sees it (FWeaponFX::SpawnMuzzleBurst): the smoke, sparks, side jets and ring round the flash. */
struct FVfxMuzzle
{
	/** Where the flash shows (AWeaponBase::GetVisibleMuzzleLocation) and the barrel's way. */
	FVector Location = FVector::ZeroVector;
	FVector Forward = FVector::ForwardVector;
	/** The barrel's up (a muzzle brake's ports blow out to its sides). */
	FVector Up = FVector::UpVector;
	EWeaponKind Kind = EWeaponKind::Rifle;
	/** The gun's flash size (its definition's MuzzleFlashScale x its model's scale). */
	float Scale = 1.f;
	/** Drawn in the player's own view: the smoke stays clear of the eye and the ring is left out. */
	bool bFirstPerson = false;
	/** The shot's number in its burst (0 the first): a fast gun's smoke and sparks thin out over a long burst. */
	int32 InBurst = 0;
	/** The light its smoke shows in (white without one). */
	FLinearColor DustLight = FLinearColor::White;
	/**
	 * In the player's own view: the way from the middle of the screen (the crosshair) out to the muzzle, square to the eye's
	 * line (zero: not known). Sparks and smoke go this way there, so nothing a shot throws crosses the crosshair (the
	 * muzzle rework, 2026-10-22: forward-flying sparks near the eye drew as big yellow diamonds over the target).
	 */
	FVector Away = FVector::ZeroVector;
};

/**
 * A gun kind's muzzle look (VfxRules::MuzzleLook): both what SpawnMuzzleBurst throws round the flash and the flash quads
 * AWeaponBase draws (VfxRules::MuzzleQuads).
 */
struct FVfxMuzzleLook
{
	/**
	 * The flash (cm across at scale 1): a small white-hot core, a soft orange bloom round it, a star of thin tapered spikes
	 * (StarGlow times the usual strength: the sniper's sharp, the launcher's faint under its puff), tongues along the barrel
	 * for side views.
	 */
	float CoreSize = 11.f;
	float BloomSize = 26.f;
	float StarSize = 34.f;
	int32 StarSpikes = 6;
	float StarGlow = 1.f;
	int32 Tongues = 3;
	float TongueLength = 26.f;
	float TongueWidth = 9.f;
	/** How long it shows (s; times Looter.Vfx.Muzzle.Time): about two frames at 60 fps, fading fast (VfxRules::MuzzleFade). */
	float Duration = 0.04f;
	/** A muzzle brake's two side jets (cm long; 0 none) and a revolver's cylinder-gap spit (cm; 0 none). */
	float SideJets = 0.f;
	float GapSpit = 0.f;
	/** Sparks of unburnt powder thrown out with the shot, and smoke puffs left hanging (each at its size, cm). */
	int32 Sparks = 3;
	int32 SmokePuffs = 1;
	float SmokeSize = 30.f;
	/** A shock ring of dust and air round the muzzle (cm at its widest; 0 none: the heavy guns'). */
	float Ring = 0.f;
	/** The first shot in this many of a burst throws its sparks and smoke (a fast gun's every round would be a cloud). */
	int32 Every = 1;
};

/** A muzzle flash's layers (AWeaponBase's quads), each sized for the view on its own (FVfxMuzzleView). */
enum class EVfxFlashLayer : uint8
{
	Core,
	Bloom,
	Star,
	Tongue,
	/** A muzzle brake's side jets. */
	Jet,
	/** A revolver's cylinder-gap spit. */
	Spit
};

/** What M_FX_Muzzle draws on a quad, by its custom data's shape float (the same numbers as M_FX_Glow's, its stand-in). */
enum class EVfxFlashShape : uint8
{
	/** A tapered tongue of flame, its hot head at the quad's +X; Param the share of its length this shot shows. */
	Streak = 0,
	/** A soft round glow; Param its softness (0 a tight hot core .. 1 a wide soft bloom). */
	Round = 1,
	/** Thin tapered spikes round a hot point, uneven by its seed; Param the spikes (3-16). */
	Star = 2
};

/** One quad of a muzzle flash (VfxRules::MuzzleQuads) on the engine Plane (100 x 100 cm in XY, facing +Z), at scale 1. */
struct FVfxFlashQuad
{
	EVfxFlashLayer Layer = EVfxFlashLayer::Core;
	EVfxFlashShape Shape = EVfxFlashShape::Round;
	/** Its middle (cm: +X out along the barrel, +Y its side, +Z up) and its turn. */
	FVector Location = FVector::ZeroVector;
	FRotator Rotation = FRotator::ZeroRotator;
	/** Its size, cm: Length along the quad's X, Width along its Y. */
	float Length = 10.f;
	float Width = 10.f;
	FLinearColor Color = FLinearColor::White;
	/** Its strength at full (before the shot's fade, its view and Looter.Vfx.Muzzle.Glow). */
	float Glow = 1.f;
	/** The shape's param (EVfxFlashShape); a streak's is drawn again each shot (VfxRules::MuzzleStreakShare). */
	float Param = 0.f;
};

/** How each layer of a flash is drawn for one view (VfxRules::MuzzleView): all 1 from behind (third person). */
struct FVfxMuzzleView
{
	float Core = 1.f;
	float Bloom = 1.f;
	float Star = 1.f;
	float Tongue = 1.f;
	/** A brake's jets and a cylinder's spit. */
	float Jet = 1.f;
	/** The whole flash's size and strength (aimed down the sights they shrink and dim). */
	float Size = 1.f;
	float Glow = 1.f;
};

/** The extra kill layers a tier earns over a Regular's (VfxRules::TierLayers). */
struct FVfxTierLayers
{
	/** Shock rings of the tier's colour over the ground (0 for a Regular). */
	int32 Rings = 0;
	/** How far the first reaches, as many times the body's size. */
	float RingReach = 1.5f;
	/** A starburst of its colour, this big (times the body; 0 none). */
	float Star = 0.f;
	/** Motes of its colour rising. */
	int32 Motes = 0;
	/** The top tiers have their own death show (UCreatureTierFXSubsystem): these layers stay small under it. */
	bool bOwnShow = false;
};

/** The size of a blast's layers (VfxRules::ExplosionLook), from its reach. */
struct FVfxExplosionLook
{
	int32 Fireballs = 5;
	float FireballSize = 0.55f;
	float FireballLife = 0.65f;
	int32 SmokeRing = 12;
	int32 SmokeColumn = 6;
	int32 Embers = 22;
	int32 Debris = 10;
	float RingReach = 1.6f;
	float RingLife = 0.35f;
};

/**
 * The recent hits of a stream of fire (an SMG's or LMG's rounds into one spot): what each next hit's look is thinned by
 * (VfxRules::StreamDetail), so a long burst stays readable and costs what a few hits do. Plain data, kept by FWeaponFX.
 */
struct FVfxHitStream
{
	static constexpr int32 Kept = 16;

	/** Notes a hit at Where, Now; returns how many hits (this one too) landed within Window seconds and Reach cm. */
	int32 Note(const FVector& Where, double Now, float Window, float Reach)
	{
		int32 Near = 1;
		const float ReachSquared = Reach * Reach;
		for (int32 Index = 0; Index < Kept; ++Index)
		{
			if (Times[Index] > -1.0 && Now - Times[Index] <= Window && FVector::DistSquared(Where, Spots[Index]) <= ReachSquared)
			{
				++Near;
			}
		}
		Spots[Next] = Where;
		Times[Next] = Now;
		Next = (Next + 1) % Kept;
		return Near;
	}

	void Reset()
	{
		for (double& Time : Times)
		{
			Time = -1.0e9;
		}
		Next = 0;
	}

	FVector Spots[Kept];
	double Times[Kept] = { -1.0e9, -1.0e9, -1.0e9, -1.0e9, -1.0e9, -1.0e9, -1.0e9, -1.0e9, -1.0e9, -1.0e9, -1.0e9, -1.0e9, -1.0e9, -1.0e9,
		-1.0e9, -1.0e9 };
	int32 Next = 0;
};

/** A gun kind's tracer, times its definition's (VfxRules::TracerLook): width, length and glow (its colour's brightness). */
struct FVfxTracerLook
{
	float Width = 1.f;
	float Length = 1.f;
	float Glow = 1.f;
};

#pragma once

#include "CoreMinimal.h"
#include "CreatureTraitTuning.generated.h"

// Each trait's numbers (UCreatureTraitSettings holds one of each; Project Settings > Game > Creature Traits). The defaults
// are the approved design's (Saved/Tiers/DESIGN.md "Traits"); the look's numbers are the code's first pass. Colours are
// world light (sRGB as authored), each trait its own so a player learns them: wind white, life green, ward cyan, grave
// orange, banner gold, rage red, the Ethereal's red.

USTRUCT(BlueprintType)
struct AI_LOOTER_SHOOTER_API FFleetTraitTuning
{
	GENERATED_BODY()

	/** Its walk and chase, times this (the design's 25% faster). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fleet", meta = (ClampMin = "1"))
	float SpeedScale = 1.25f;

	/** Each attack's wind-up, times this (the design's 15% quicker). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fleet", meta = (ClampMin = "0.3", ClampMax = "1"))
	float WindupScale = 0.85f;

	/** Its wind streaks show once it goes faster than this share of its chase. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fleet|Look", meta = (ClampMin = "0", ClampMax = "1"))
	float StreakSpeedShare = 0.55f;

	/** The glint as it winds up: its seconds (the quicker strike's tell). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fleet|Look", meta = (ClampMin = "0", Units = "s"))
	float GlintSeconds = 0.16f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fleet|Look")
	FColor Color = FColor(0xDC, 0xEA, 0xF4);
};

USTRUCT(BlueprintType)
struct AI_LOOTER_SHOOTER_API FMendingTraitTuning
{
	GENERATED_BODY()

	/** It starts mending once this long unhit (any hit, ward hits too, starts the wait again). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mending", meta = (ClampMin = "0", Units = "s"))
	float UnhitSeconds = 3.f;

	/** Share of its most health healed a second while it mends. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mending", meta = (ClampMin = "0", ClampMax = "1"))
	float HealSharePerSecond = 0.03f;

	/** A mote rises off it this often (its shimmer, Creature.Trait.Mend, loops while it mends). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mending|Look", meta = (ClampMin = "0.02", Units = "s"))
	float MoteEvery = 0.09f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mending|Look")
	FColor Color = FColor(0xA6, 0xEA, 0x8C);
};

USTRUCT(BlueprintType)
struct AI_LOOTER_SHOOTER_API FWardedTraitTuning
{
	GENERATED_BODY()

	/** The ward is worth this share of its most health (it keeps its share when the most changes). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warded", meta = (ClampMin = "0", ClampMax = "3"))
	float HealthShare = 0.3f;

	/** A critical hit counts this many times over against the ward (on top of its own x1.5): aim for the weak spot to pop it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warded", meta = (ClampMin = "1"))
	float CritBreakScale = 2.f;

	/** Broken, it's dazed this long (the knock component's daze): the reward for breaking it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warded", meta = (ClampMin = "0", Units = "s"))
	float BreakDazeSeconds = 0.8f;

	/** The cage's glow, full and at its weakest (it fades as the ward wears down, and flickers under FlickerShare). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warded|Look", meta = (ClampMin = "0"))
	float ShellGlow = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warded|Look", meta = (ClampMin = "0", ClampMax = "1"))
	float FlickerShare = 0.35f;

	/** A hit's flare on the cage, on top. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warded|Look", meta = (ClampMin = "0"))
	float HitGlow = 9.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warded|Look")
	FColor Color = FColor(0x8C, 0xE6, 0xFF);
};

USTRUCT(BlueprintType)
struct AI_LOOTER_SHOOTER_API FGraveBurstTraitTuning
{
	GENERATED_BODY()

	/** The swell from its death to the burst: the tell to step away. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grave-burst", meta = (ClampMin = "0.2", Units = "s"))
	float SwellSeconds = 1.f;

	/** The burst reaches this far, measured to the nearest side of a body (its ring on the ground shows it). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grave-burst", meta = (ClampMin = "50", Units = "cm"))
	float Radius = 300.f;

	/** Full damage this near; falling straight to EdgeShare at Radius. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grave-burst", meta = (ClampMin = "0", Units = "cm"))
	float FullRadius = 120.f;

	/** The burst's damage at its heart, as a share of the creature's attack (its level and tier counted). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grave-burst", meta = (ClampMin = "0"))
	float DamageShare = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grave-burst", meta = (ClampMin = "0", ClampMax = "1"))
	float EdgeShare = 0.6f;

	/** The body swells to this times its size. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grave-burst|Look", meta = (ClampMin = "1", ClampMax = "2"))
	float SwellScale = 1.3f;

	/** A player caught is thrown out at this (at the heart; less toward the edge) and up at KnockLift (cm/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grave-burst", meta = (ClampMin = "0"))
	float KnockSpeed = 650.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grave-burst", meta = (ClampMin = "0"))
	float KnockLift = 260.f;

	/** Players' views jolt within this far, harder nearer. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grave-burst|Look", meta = (ClampMin = "0", Units = "cm"))
	float KickRadius = 1500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grave-burst|Look")
	FColor Color = FColor(0xFF, 0x8A, 0x3D);
};

USTRUCT(BlueprintType)
struct AI_LOOTER_SHOOTER_API FRallyingTraitTuning
{
	GENERATED_BODY()

	/** Creatures this near (flat; and within MaxRise above or below) are rallied. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rallying", meta = (ClampMin = "0", Units = "cm"))
	float Radius = 800.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rallying", meta = (ClampMin = "0", Units = "cm"))
	float MaxRise = 300.f;

	/** A rallied creature takes this share of each hit (the design's 15% less). Several ralliers count once. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rallying", meta = (ClampMin = "0.05", ClampMax = "1"))
	float DamageTakenScale = 0.85f;

	/** A rallied creature's walk and chase, times this (10% faster). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rallying", meta = (ClampMin = "1"))
	float SpeedScale = 1.1f;

	/** It looks round for whom it rallies this often. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rallying", meta = (ClampMin = "0.05", Units = "s"))
	float ScanSeconds = 0.25f;

	/** In a fight, its rallying cry and the pulse through its ring come this often (and as the fight starts). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rallying|Look", meta = (ClampMin = "1", Units = "s"))
	float PulseSeconds = 6.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rallying|Look")
	FColor Color = FColor(0xFF, 0xE0, 0x9A);
};

USTRUCT(BlueprintType)
struct AI_LOOTER_SHOOTER_API FVengefulTraitTuning
{
	GENERATED_BODY()

	/** It enrages once its health falls under this share, and stays enraged until it dies (healing doesn't calm it). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vengeful", meta = (ClampMin = "0", ClampMax = "1"))
	float HealthShare = 0.3f;

	/** Enraged: its attacks' damage and its walk and chase, times these (25% more each). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vengeful", meta = (ClampMin = "1"))
	float DamageScale = 1.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vengeful", meta = (ClampMin = "1"))
	float SpeedScale = 1.25f;

	/** The flare as it enrages: its seconds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vengeful|Look", meta = (ClampMin = "0", Units = "s"))
	float FlareSeconds = 0.45f;

	/** Its tier glow (eyes, core, coal, lamp: the look's) times this while enraged (UCreatureTraitComponent::GetGlowScale). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vengeful|Look", meta = (ClampMin = "1"))
	float GlowScale = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vengeful|Look")
	FColor Color = FColor(0xFF, 0x4A, 0x36);
};

USTRUCT(BlueprintType)
struct AI_LOOTER_SHOOTER_API FPhasingTraitTuning
{
	GENERATED_BODY()

	/** "Hit hard": this share of its most health taken within HardHitWindow (a shotgun blast, a grenade, a crit or two, a second of focused fire). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Phasing", meta = (ClampMin = "0.01", ClampMax = "1"))
	float HardHitShare = 0.08f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Phasing", meta = (ClampMin = "0.1", Units = "s"))
	float HardHitWindow = 1.f;

	/** At least this long from one blink's start to the next's (the design's every 4 s at most). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Phasing", meta = (ClampMin = "0", Units = "s"))
	float Cooldown = 4.f;

	/** A blink takes it this far aside (flat; the design's 4-6 m), tried longest first. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Phasing", meta = (ClampMin = "50", Units = "cm"))
	float MinDistance = 400.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Phasing", meta = (ClampMin = "50", Units = "cm"))
	float MaxDistance = 600.f;

	/** It never comes back nearer whoever hit it than this. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Phasing", meta = (ClampMin = "0", Units = "cm"))
	float NearestToAttacker = 250.f;

	/** The ground it comes back on is at most this far above or below its own: a blink never changes floors. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Phasing", meta = (ClampMin = "0", Units = "cm"))
	float MaxRise = 120.f;

	/** It's gone this long between the two shimmers: the arrival's gathering light is the tell where it comes back. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Phasing|Look", meta = (ClampMin = "0", Units = "s"))
	float GoneSeconds = 0.25f;

	/** No spot to blink to: it looks again after this long. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Phasing", meta = (ClampMin = "0.05", Units = "s"))
	float RetrySeconds = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Phasing|Look")
	FColor Color = FColor(0xFF, 0x3B, 0x3B);
};

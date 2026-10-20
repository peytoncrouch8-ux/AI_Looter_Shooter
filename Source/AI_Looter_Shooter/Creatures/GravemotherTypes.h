#pragma once

#include "CoreMinimal.h"
#include "Player/PlayerSize.h"
#include "GravemotherTypes.generated.h"

// The Gravemother's Barrow fight (the user's pick, 2026-10-15; built 2026-10-20): her moves' states and their settings,
// kept out of GravemotherCreature.h so it stays readable. Distances marked "at a spider's size" grow with her size
// (GetSizeScale); "world" ones are the fight's earth, made at their own size (the berm, the rubble, the clod).

/** Where her Furrow is (her old charge, upgraded: the ground heaves along her line, then she ploughs it). */
UENUM(BlueprintType)
enum class EGravemotherFurrow : uint8
{
	/** Not ploughing: she walks and bites as any spider does. */
	None,
	/** The 2 s tell: head low, front legs in the floor, the ground heaving and splitting in a line toward her target. */
	Heave,
	/** Down the line head-low at full tilt, throwing up a berm each side. */
	Plough,
	/** Stopped at the line's end with her forelegs slammed down: slow to rise, slower when she ran nobody down. */
	Recover,
};

/** Which of her attacks she's at (each is the creature's one attack, wound up, struck and recovered, with its own timing). */
UENUM(BlueprintType)
enum class EGravemotherMove : uint8
{
	Bite,
	Furrow,
	/** Rearing and screaming (her fight's start, each later phase), her forelegs slamming down at its height: at 66% and 33%
	 *  the slam is the Unearthed's (mounds bulge round her). */
	Roar,
	/** The Gravequake: rearing high while a ring of cracks spreads out under her, then a slam that bursts it. */
	Quake,
	/** The Grave Clod: front legs dig in and tear a clod of grave earth overhead (the tell), then she lobs it. */
	Clod,
};

/** Her headstone's state, by its share of health (GravemotherFight::StoneStateFor). */
UENUM(BlueprintType)
enum class EGravemotherStone : uint8
{
	Intact,
	/** At 66% of its health and under. */
	Cracked,
	/** At 33% and under. */
	Shattered,
	/** Cracked off: her pale Bone abdomen is bare, and all of it is a critical spot. */
	Broken,
};

/** Her Furrow (GravemotherCreatureFurrow.cpp; the timings are the user's). */
USTRUCT(BlueprintType)
struct FGravemotherFurrowSettings
{
	GENERATED_BODY()

	/** She ploughs at a target this far off at most (cm, at a spider's size), and no nearer than MinDistance: closer, she bites. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Furrow", meta = (ClampMin = "100", Units = "cm"))
	float Range = 900.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Furrow", meta = (ClampMin = "0", Units = "cm"))
	float MinDistance = 300.f;
	/** The line runs this far past where her target stood (cm, at a spider's size). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Furrow", meta = (ClampMin = "0", Units = "cm"))
	float Overshoot = 150.f;
	/** How fast she ploughs (cm/s at a spider's size), tuned against the full-size player's speed and scaled with it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Furrow", meta = (ClampMin = "100", Units = "cm/s"))
	float Speed = 900.f * LooterPlayerSize::MoveScale;
	/** The tell (the user's 2 s): she tracks her target until AimAt, then the split crawls out along the line, all of it
	 *  open by CrawlEnd. Never shortened by her pace. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Furrow", meta = (ClampMin = "0.5", Units = "s"))
	float Tell = 2.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Furrow", meta = (ClampMin = "0", Units = "s"))
	float AimAt = 0.4f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Furrow", meta = (ClampMin = "0.1", Units = "s"))
	float CrawlEnd = 1.8f;
	/** Down after the slam (s): having run her target down, and having missed (the window to punish her). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Furrow", meta = (ClampMin = "0", Units = "s"))
	float Recover = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Furrow", meta = (ClampMin = "0", Units = "s"))
	float MissRecover = 2.f;
	/** From one furrow's end to the next one's readiness while she's after someone, and from her first sight of them (s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Furrow", meta = (ClampMin = "0", Units = "s"))
	float Cooldown = 8.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Furrow", meta = (ClampMin = "0", Units = "s"))
	float FirstDelay = 2.f;
	/** The plough hits this many times as hard as her bite (damage and shove), whoever comes within HitMargin of her body. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Furrow", meta = (ClampMin = "0"))
	float Strength = 2.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Furrow", meta = (ClampMin = "0", Units = "cm"))
	float HitMargin = 40.f;
	/** The ground's burst round her slam (cm, at a spider's size). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Furrow", meta = (ClampMin = "10", Units = "cm"))
	float BurstRadius = 170.f;
	/** The berms: their middles this far either side of the line (world cm), one every BermEvery (ART's SM_GraveBerm is
	 *  2.6 m long, laid one every 2 m so they overlap), standing BermSeconds (the user's 20 s) from the plough's end, then
	 *  crumbling over BermCrumbleSeconds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Furrow|Berms", meta = (ClampMin = "50", Units = "cm"))
	float BermOffset = 165.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Furrow|Berms", meta = (ClampMin = "50", Units = "cm"))
	float BermEvery = 200.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Furrow|Berms", meta = (ClampMin = "1", Units = "s"))
	float BermSeconds = 20.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Furrow|Berms", meta = (ClampMin = "0.05", Units = "s"))
	float BermRiseSeconds = 0.3f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Furrow|Berms", meta = (ClampMin = "0.1", Units = "s"))
	float BermCrumbleSeconds = 1.2f;
	/** A berm rises once her middle is this far past it (world cm): behind her abdomen's tip, never under her. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Furrow|Berms", meta = (ClampMin = "0", Units = "cm"))
	float RearLag = 260.f;
};

/** Her Grave Clod (GravemotherCreatureClod.cpp, AGravemotherClod; the timings are the user's). */
USTRUCT(BlueprintType)
struct FGravemotherClodSettings
{
	GENERATED_BODY()

	/** She lobs at a target between these (cm, at a spider's size: 7.2 to 21.6 m at hers), her fight on and the lob's way clear. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clod", meta = (ClampMin = "0", Units = "cm"))
	float MinDistance = 400.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clod", meta = (ClampMin = "100", Units = "cm"))
	float MaxDistance = 1200.f;
	/** The tell (the user's 1.2 s): her front legs dig in, tear the clod up at TearAt and have it overhead by LiftedAt. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clod", meta = (ClampMin = "0.3", Units = "s"))
	float Tell = 1.2f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clod", meta = (ClampMin = "0", Units = "s"))
	float TearAt = 0.4f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clod", meta = (ClampMin = "0", Units = "s"))
	float LiftedAt = 0.8f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clod", meta = (ClampMin = "0.1", Units = "s"))
	float Recovery = 0.7f;
	/** The lob: pulled down this hard (cm/s^2), flying FlightAt20m to a target 20 m off (the user's 1.5 s), quicker nearer. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clod", meta = (ClampMin = "100"))
	float Gravity = 1800.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clod", meta = (ClampMin = "0.3", Units = "s"))
	float FlightAt20m = 1.5f;
	/** It bursts on whoever is within BurstRadius (world cm) of where it lands: Strength times her bite at its middle, down
	 *  to EdgeShare of that at its edge, and a shove. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clod", meta = (ClampMin = "0"))
	float Strength = 1.5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clod", meta = (ClampMin = "10", Units = "cm"))
	float BurstRadius = 200.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clod", meta = (ClampMin = "0", ClampMax = "1"))
	float EdgeShare = 0.5f;
	/** Shot in her grip it bursts on her and stuns her this long (the user's 2 s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clod", meta = (ClampMin = "0.2", Units = "s"))
	float StunSeconds = 2.f;
	/** The rubble it leaves: cover this long (the user's 10 s), then it settles away over RubbleSettleSeconds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clod", meta = (ClampMin = "1", Units = "s"))
	float RubbleSeconds = 10.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clod", meta = (ClampMin = "0.1", Units = "s"))
	float RubbleSettleSeconds = 1.f;
	/** The held clod's shootable reach (world cm, round its middle): forgiving, the shot is the fight's reward. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clod", meta = (ClampMin = "20", Units = "cm"))
	float HitRadius = 75.f;
};

/** Her Unearthed (GravemotherCreatureUnearthed.cpp, AGravemotherMound): her brood's calls, through mounds. */
USTRUCT(BlueprintType)
struct FGravemotherUnearthSettings
{
	GENERATED_BODY()

	/** Mounds come up within Radius of her (cm, at a spider's size), on her level (MaxStep cm above or below her feet), at
	 *  least Spacing apart and ClearOfPlayer (cm) from the player. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unearthed", meta = (ClampMin = "100", Units = "cm"))
	float Radius = 450.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unearthed", meta = (ClampMin = "10", Units = "cm"))
	float MaxStep = 200.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unearthed", meta = (ClampMin = "50", Units = "cm"))
	float Spacing = 200.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unearthed", meta = (ClampMin = "0", Units = "cm"))
	float ClearOfPlayer = 300.f;
	/** A mound bulges this long before its spiderling digs out (the user's 1 s): shot meanwhile, it caves in on it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unearthed", meta = (ClampMin = "0.2", Units = "s"))
	float BulgeSeconds = 1.f;
	/** Cracks race from her feet to the mounds over this long (s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unearthed", meta = (ClampMin = "0.05", Units = "s"))
	float CrackRaceSeconds = 0.3f;
	/** A call her roar hasn't answered after this long (s; no one to roar at, reeling) raises its mounds anyway. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unearthed", meta = (ClampMin = "0.5", Units = "s"))
	float LateRaiseSeconds = 2.5f;
};

/** Her headstone (AGravemotherStone; Main's numbers, 2026-10-19). */
USTRUCT(BlueprintType)
struct FGravemotherStoneSettings
{
	GENERATED_BODY()

	/** Its health, as a share of her most health. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stone", meta = (ClampMin = "0.01", ClampMax = "1"))
	float HealthShare = 0.2f;
	/** It shows cracked at this share of its own health and under, shattered at ShatteredAt. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stone", meta = (ClampMin = "0", ClampMax = "1"))
	float CrackedAt = 0.66f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stone", meta = (ClampMin = "0", ClampMax = "1"))
	float ShatteredAt = 0.33f;
	/** A shot this near (cm, in the abdomen bone's space) the soft rear's hit zone counts as on it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stone", meta = (ClampMin = "0", Units = "cm"))
	float RearMargin = 4.f;
	/** With no soft-rear zone in her physics asset (SK_Spider): the abdomen's far end past this share of its length. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stone", meta = (ClampMin = "0", ClampMax = "1"))
	float RearShare = 0.68f;
	/** The pieces of a broken stone lie this long on the floor, then sink away over ShardSinkSeconds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stone", meta = (ClampMin = "0", Units = "s"))
	float ShardLieSeconds = 12.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stone", meta = (ClampMin = "0.1", Units = "s"))
	float ShardSinkSeconds = 1.f;
};

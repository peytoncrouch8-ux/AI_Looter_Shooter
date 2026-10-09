#pragma once

#include "CoreMinimal.h"
#include "BreakableKinds.generated.h"

struct FRandomStream;

/**
 * Which breakable prop it is (Docs/Polish/BorderlandsComparison.md, item 10, "a lootable world"): the dressing's wooden
 * crates and barrel (Art/Models/Props/Containers.py), each with its pieces broken in advance and the stump it leaves
 * standing (Art/Models/Props/Lootables.py). The steel drum (Barrel_B) isn't one: a shot rings off it.
 */
UENUM(BlueprintType)
enum class EBreakableKind : uint8
{
	/** Crate_A: the 0.9 m slatted crate. */
	SlattedCrate,
	/** Crate_B: the 1 m plank packing crate, its lid pushed back, straw inside. */
	PackingCrate,
	/** Barrel_A: the 0.92 m wooden barrel, its staves held by four iron hoops. */
	Barrel,
};

/** One kind's models and numbers (FBreakableKindInfo::Get). */
struct AI_LOOTER_SHOOTER_API FBreakableKindInfo
{
	/** The intact model (the dressing's own) and the stump it leaves standing. */
	const TCHAR* BodyPath = nullptr;
	const TCHAR* StumpPath = nullptr;

	/** Its pieces, each modeled where it sat in the whole prop (origin at the prop's pivot): <PiecePrefix><n>, n from 1. */
	const TCHAR* PiecePrefix = nullptr;
	int32 PieceCount = 0;

	/**
	 * Its health: under the least a melee strike does at level 1 (60 less 10%), so one strike always breaks it; two or
	 * three rounds from a level-1 rifle, one shotgun blast.
	 */
	float Health = 50.f;

	/** How tall it stands (cm): the break's middle is halfway up, the dust rises from its foot to its top. */
	float Height = 60.f;

	/** The chance it leaves an ammo pickup (a kill's 18-36 rounds, leaning toward the gun that broke it), and a soul-mote. */
	float AmmoChance = 0.35f;
	float MoteChance = 0.06f;

	/** The break's sound (LooterSoundCue::Lootables). */
	const TCHAR* BreakCue = nullptr;

	/** The dust it throws (wood dust, straw chaff), before the light: dim, since the smoke material is unlit. */
	FLinearColor DustColor = FLinearColor(0.3f, 0.25f, 0.18f);

	static FBreakableKindInfo Get(EBreakableKind Kind);

	/** The Index-th piece's object path (Index from 0). */
	FString PiecePath(int32 Index) const;
};

/** How a breakable breaks: the pieces' throw, the hit's shudder, the loot's rolls. Plain numbers the tests read. */
namespace LooterBreakables
{
	/** The tag every breakable carries (the console, the build script, the impacts' surface). */
	inline constexpr const TCHAR* Tag = TEXT("Breakable");

	/** How the pieces fly (cm/s): out from the middle, along the blow that broke it, and up; how fast they turn (rad/s). */
	inline constexpr float OutSpeedMin = 110.f;
	inline constexpr float OutSpeedMax = 290.f;
	inline constexpr float BlowSpeed = 170.f;
	inline constexpr float UpSpeedMin = 170.f;
	inline constexpr float UpSpeedMax = 400.f;
	inline constexpr float SpinMin = 3.f;
	inline constexpr float SpinMax = 11.f;

	/** How long a piece lies once it's down before it shrinks away, and how long that takes (s). */
	inline constexpr float PieceLifeMin = 2.8f;
	inline constexpr float PieceLifeMax = 4.2f;
	inline constexpr float ShrinkSeconds = 0.6f;

	/** A piece's bounce: how much of its fall it keeps going up, and of its slide along the ground; the bounces before it lies. */
	inline constexpr float Bounce = 0.32f;
	inline constexpr float BounceSlide = 0.45f;
	inline constexpr int32 Bounces = 2;

	/** A hit that doesn't break it: it rocks away from the blow by this much and settles back over this long. */
	inline constexpr float ShudderDegrees = 3.5f;
	inline constexpr float ShudderSeconds = 0.2f;

	/** The break's dust: puffs round its foot and middle, and splinters thrown with the pieces. */
	inline constexpr int32 DustPuffs = 7;
	inline constexpr int32 Splinters = 14;

	/** Whether a break leaves a soul-mote (Info's chance). */
	AI_LOOTER_SHOOTER_API bool RollMote(const FBreakableKindInfo& Info, FRandomStream& Random);

	/**
	 * One piece's throw: out from the break's middle through the piece's own middle (Out, any length), pushed along the
	 * blow (Blow, any length; zero: none), and up.
	 */
	AI_LOOTER_SHOOTER_API FVector PieceVelocity(const FVector& Out, const FVector& Blow, FRandomStream& Random);

	/** A piece's tumble: a random axis at a random rate. */
	AI_LOOTER_SHOOTER_API FVector PieceSpin(FRandomStream& Random);
}

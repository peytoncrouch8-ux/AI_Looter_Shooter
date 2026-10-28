#pragma once

#include "CoreMinimal.h"
#include "PlayerMeleeToolTypes.generated.h"

class UStaticMesh;

/**
 * How a hero's melee tool swings (MeleeMotion::ArcFor has each one's arc). The user's note 5 (2026-10-27): "each
 * character to have a specific tool that they use and hits with the right hand"; the swings are the concept sheets'
 * (Art/Concepts/MeleeTools/README.md, the user's picks the same day).
 */
UENUM(BlueprintType)
enum class EMeleeSwingStyle : uint8
{
	/** A flat backhand swat, high on the right down to low on the left (Ellis's grave spade). */
	Swat,
	/** A backhand flick the other way, low on the left up to high on the right (Odessa's razor fan). */
	Flick,
	/** Straight down from high on the right onto the crosshair. */
	Chop,
	/** A short overhand peck, down and to the left (Pike's rock pick). */
	Peck,
	/** An overhand hooking chop, then a yank back toward the body (Crane's ferry gaff). */
	Gaff,
	/** Drawn back by the shoulder and driven straight into the crosshair (Gauge's piston fist). */
	Jab
};

/** How the tool sits on the right arm. */
UENUM(BlueprintType)
enum class EMeleeGripKind : uint8
{
	/** Held in the fist by its handle (the mesh's Grip socket). */
	Held,
	/** Clamped on the forearm, the fist bare inside it (the mesh's Forearm socket at the wrist): Gauge's piston fist. */
	Forearm
};

/**
 * The tool's weight as it reads (the sheets' QUICK, STEADY, HEAVY): a bigger wind-up and a harder jolt for a heavy one.
 * The swing's clock is the same for all (FMeleeRules): a hero's tool is a look and a feel, never a better strike.
 */
UENUM(BlueprintType)
enum class EMeleeHeft : uint8
{
	Quick,
	Steady,
	Heavy
};

/**
 * A hero's melee tool (UHeroDefinition::MeleeTool, or the picks' table: UHeroDefinition::GetMeleeTool): the mesh on the
 * right arm, how it sits, how it swings.
 *
 * Held: the mesh's "Grip" socket (SOCKET_Grip in Blender) is where the right palm closes; its axes: +X up the handle to
 * the head, +Z the side that strikes, +Y the way the right palm faces onto the handle. Forearm: the "Forearm" socket
 * (SOCKET_Forearm) sits on the wrist; +X toward the knuckles (the punch's way), +Z out of the back of the hand. "Tip"
 * (SOCKET_Tip) marks the striking end. With no socket, the mesh's origin and axes stand in. The meshes come from
 * Art/Models/Weapons/Melee/ (/Game/Art/Melee/SM_Melee_<Tool>); until a hero's is in the game the placeholder is swung
 * (UHeroDefinition::PlaceholderMeleeToolPath), and until that one is, a stand-in from the engine's basic shapes
 * (PlayerMeleeTool.cpp).
 */
USTRUCT(BlueprintType)
struct AI_LOOTER_SHOOTER_API FHeroMeleeTool
{
	GENERATED_BODY()

	/** The tool's mesh; empty: the picks' table (UHeroDefinition::GetMeleeTool). */
	UPROPERTY(EditDefaultsOnly, Category = "Melee")
	TSoftObjectPtr<UStaticMesh> Mesh;

	UPROPERTY(EditDefaultsOnly, Category = "Melee")
	EMeleeGripKind Grip = EMeleeGripKind::Held;

	UPROPERTY(EditDefaultsOnly, Category = "Melee")
	EMeleeSwingStyle Swing = EMeleeSwingStyle::Swat;

	UPROPERTY(EditDefaultsOnly, Category = "Melee")
	EMeleeHeft Heft = EMeleeHeft::Steady;

	/**
	 * The tool's key for its sounds (Player.Melee.<Kind>.Swing, .Hit; MeleeFX::SwingCue, HitCue) and its impact effects
	 * (MeleeFX::Spawn): Spade, Fan, Gaff, Piston, Pick (the user's picks, 2026-10-27). None: the shared swing and blow.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Melee")
	FName Kind;

	/** The mesh drawn this much bigger than modeled (as a gun is in first person). */
	UPROPERTY(EditDefaultsOnly, Category = "Melee", meta = (ClampMin = "0.25", ClampMax = "3"))
	float Scale = 1.f;

	/** A held tool's handle radius where the hand closes on it (cm, as modeled): the fingers close till they touch it. */
	UPROPERTY(EditDefaultsOnly, Category = "Melee", meta = (ClampMin = "0.5", ClampMax = "4"))
	float HandleRadius = 1.6f;

	/**
	 * A held tool round all the way where the hand closes (a pole): the hand turns round it to meet the arm, the forearm
	 * back toward the shoulder, instead of keeping its knuckles on the striking side. The gaff's yank and thrust otherwise
	 * hung the hand under the pole with the wrist above it (playtest 2026-10-28: "fingers open beside the gaff").
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Melee")
	bool bRoundHandle = false;
};

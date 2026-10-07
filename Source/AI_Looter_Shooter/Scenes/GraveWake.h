#pragma once

#include "CoreMinimal.h"
#include "Scenes/SceneSubsystem.h"

class AActor;
class UWorld;

/**
 * Clawing out of the grave (Docs/Areas/RansomsRest.md, Main 1, step 2: "press Jump three times"), as rules apart from the
 * world so the tests press it. Each press that counts is a claw: the view jolts and eases a third of the way up through
 * the dirt, which lifts from near black toward the afternoon, and grave dirt bursts up. Presses come at least PressSpacing
 * apart, so each claw gets its moment (a key held down, or mashed, still claws one at a time); three, and Ellis is out,
 * and once the third claw has settled, the climb out of the grave begins.
 */
struct AI_LOOTER_SHOOTER_API FGraveClawOut
{
	/** Presses to claw out. */
	static constexpr int32 PressesNeeded = 3;

	/** A press sooner than this after the last one that counted doesn't count (seconds). */
	static constexpr float PressSpacing = 0.3f;

	/** How long a claw's rise takes to settle (seconds). */
	static constexpr float RiseSeconds = 0.45f;

	/** How dark it is under the dirt before the first claw (0 clear, 1 black). */
	static constexpr float BuriedDarkness = 0.94f;

	/** A press at Now (seconds since the wake-up began). True when it counted as a claw. */
	bool Press(float Now);

	int32 GetPresses() const { return Presses; }

	/** All three claws are in: Ellis is out. */
	bool IsOut() const { return Presses >= PressesNeeded; }

	/** Out, and the last claw has settled: the climb out can begin. */
	bool IsSettled(float Now) const;

	/** How far up through the dirt Ellis is at Now: 0 lying in the coffin, 1 through (each claw eases a third of the way). */
	float RiseAt(float Now) const;

	/** How dark it is at Now: nearly black under the dirt, lifting with each claw, gone once through. */
	float DarknessAt(float Now) const;

	/** How hard the last claw jolts the view at Now (1 as it lands, dying away to 0). */
	float JoltAt(float Now) const;

private:
	int32 Presses = 0;
	float LastPress = -1000.f;
};

/** Where the wake-up happens, from Ellis's grave in the level (Art/Models/Props/Graves.py: Grave_Ellis and its sockets). */
struct AI_LOOTER_SHOOTER_API FGraveWakeSpots
{
	/** The view lying in the coffin: the head at the headboard's end, looking up past the open grave's rim at the sky. */
	FTransform Coffin = FTransform::Identity;

	/** Where the dirt bursts from (the hole's middle on the ground), and which way is up there. */
	FVector Hole = FVector::ZeroVector;
	FVector Up = FVector::UpVector;

	/** Where Ellis stands once out, at the grave's foot, and the way they face: back at their own headboard. */
	FVector Stand = FVector::ZeroVector;
	float StandYaw = 0.f;

	/** What they look at standing: their headboard's face. */
	FVector LookAt = FVector::ZeroVector;

	/** The grave it was found from. */
	TWeakObjectPtr<AActor> Grave;
};

/** The grave wake-up as a scene for USceneSubsystem::Play (UColdOpenSubsystem plays it after the cold open). */
namespace GraveWake
{
	/** The scene ("GraveWake": Main 1's claw-out step waits for Scene.GraveWake). */
	AI_LOOTER_SHOOTER_API FName SceneName();

	/** The tag that marks Ellis's grave when its model can't say (a test's stand-in); the model is SM_Grave_Ellis. */
	AI_LOOTER_SHOOTER_API FName GraveTag();

	/** How far from the hole's middle Ellis stands once out, along the grave toward its foot (cm): past the heap. */
	inline constexpr float StandOut = 190.f;

	/** When the scene stops to wait for the claws, and how long the climb out takes after (seconds). */
	inline constexpr float ClawWaitAt = 1.2f;
	inline constexpr float ClimbSeconds = 2.4f;

	/** Ellis's grave in World (the actor showing SM_Grave_Ellis, or one tagged GraveTag), or null. */
	AI_LOOTER_SHOOTER_API AActor* FindGrave(const UWorld* World);

	/** The wake-up's spots around Grave: its sockets when its model has them, else the model's measurements. */
	AI_LOOTER_SHOOTER_API FGraveWakeSpots SpotsFor(AActor& Grave);

	/** The wake-up's state while it plays (its claws, its clock, its view, its prompt); its insides are GraveWake.cpp's. */
	struct FState;

	/**
	 * The wake-up as a scene: in the coffin, near black; the scene waits while the player claws (each press a burst of
	 * dirt, a jolt and a rise, the prompt's pips filling); then the climb out to the grave's foot, facing the headboard, and
	 * the view handed back to the player's eyes. The player stands there when it ends, played or skipped. OutState is the
	 * wake-up's state, to press from outside the keys (Press).
	 */
	AI_LOOTER_SHOOTER_API FScenePlay Make(USceneSubsystem& Scenes, const FGraveWakeSpots& Spots, TSharedPtr<FState>& OutState);

	/** A claw for the wake-up whose state this is (a console command, the tests); true when it counted. */
	AI_LOOTER_SHOOTER_API bool Press(FState& State);

	/** How many claws the wake-up whose state this is has taken. */
	AI_LOOTER_SHOOTER_API int32 GetPresses(const FState& State);
}

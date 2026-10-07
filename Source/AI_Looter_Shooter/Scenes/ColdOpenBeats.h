#pragma once

#include "CoreMinimal.h"
#include "Scenes/ColdOpenCourse.h"
#include "Story/StoryLine.h"

class AColdOpenCast;
class AColdOpenSet;
class USceneSubsystem;
class USoundBase;
struct FScenePlay;

/**
 * The cold open's insides, shared by its two halves: ColdOpen.cpp (its lines, its start, the gang's skiff, and the helpers
 * below) and ColdOpenPoint.cpp (dusk on Ransom's Point). Nothing else includes this; ColdOpen.h is the scene's face.
 */
namespace ColdOpenBeats
{
	/** The lines, beat by beat (Docs/Story.md: Cold open), each on screen for its seconds. */
	enum class ELine : uint8
	{
		Home,
		SevenDays,
		Bell,
		PutHerBack,
		El,
		Sorry,
		Count,
	};

	/** What the cold open's moves and moments share; it outlives the call that made them. */
	struct FColdOpenState
	{
		TWeakObjectPtr<USceneSubsystem> Scenes;
		TWeakObjectPtr<AColdOpenSet> Set;
		TWeakObjectPtr<AColdOpenCast> Props;
		FColdOpenCourse Course;
		FText Title;
		float SkiffSeconds = 26.f;
		double SkiffYaw = 0.0;
		bool bSkiffYawKnown = false;
		/** When the Deacon's shot kicked the view (scene seconds; negative: not yet). */
		float KickAt = -1.f;
	};
	using FStateRef = TSharedRef<FColdOpenState>;

	FStoryLine LineFor(ELine Which);

	/** The scene is jumping to its end: what only a played scene shows or says is left out. */
	bool IsSkipping(const FColdOpenState& State);

	float SceneTime(const FColdOpenState& State);

	/** The scene camera's place and look. */
	void SetView(const FColdOpenState& State, const FVector& Location, const FRotator& Rotation);

	FRotator LookAt(const FVector& From, const FVector& To);

	/** A line said now, cutting off what's on screen (nothing else talks over a scene). Never on a skip. */
	void Say(const FColdOpenState& State, ELine Which);

	/** A sound now, if the set has one: in the world at At, or everywhere at once without. Never on a skip. */
	void Sound(const FColdOpenState& State, USoundBase* Played, const FVector* At = nullptr);

	/** The camera's own fade to or from black. A skip goes straight to where it would end. */
	void Fade(const FColdOpenState& State, float From, float To, float Seconds, bool bHold);

	/** Dusk on Ransom's Point, from Cut seconds into the scene to its end (ColdOpenPoint.cpp). */
	void AddPointBeats(FScenePlay& Scene, const FStateRef& State, float Cut);
}

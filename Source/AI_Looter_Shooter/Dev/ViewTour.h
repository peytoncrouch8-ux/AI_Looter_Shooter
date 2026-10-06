#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ViewTour.generated.h"

class ACameraActor;

/**
 * Looter.Tour [views.json] [noshots] [quit]: looks at the level from each of its viewpoints in turn, as the game draws
 * it, and reports how long frames take there. For art reviews and the per-area performance budget: the frame at the
 * spawn alone says little about the forest or the view from the lookout.
 *
 * The viewpoints come from a JSON file (default Art/Levels/TutorialIsland/views.json): a name, x and y, a height above
 * the ground there, yaw, pitch and field of view. At each one the player's view switches to a camera, settles for a
 * moment (streaming, exposure), measures frame, game, render and GPU time and draw calls, and takes a screenshot to
 * Saved/Screenshots/Tour/<view>.png. The results go to the log and to Saved/Tour/<level>.csv. With "quit" the game
 * exits when the tour is done (Tools/tour.ps1 runs it that way).
 *
 * A view may also give console commands: "exec" runs as the tour arrives there (before it settles), "after" as it
 * leaves. So one place can be measured twice, once as it is and once with something hidden or switched
 * ("exec": ["Looter.Perf.HideTag Beyond 1"], "after": ["Looter.Perf.HideTag Beyond 0"]), and the difference between
 * the two rows is what that thing costs there.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UViewTourSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override;
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

	/** Starts a tour of the viewpoints in the file (relative to the project folder). False if it can't be read. */
	bool Start(const FString& ViewsFile, bool bShots, bool bQuit);

	/** A tour is going on (scenes stay off meanwhile: USceneSubsystem::AreScenesOn). */
	bool IsRunning() const { return Stops.IsValidIndex(Current); }

private:
	struct FStop
	{
		FString Name;
		FVector Location = FVector::ZeroVector;
		FRotator Rotation = FRotator::ZeroRotator;
		float FieldOfView = 80.f;
		/** Console commands run on arriving here, and on leaving. */
		TArray<FString> Exec;
		TArray<FString> After;
	};

	void Visit(int32 Index);
	void RunCommands(const TArray<FString>& Commands) const;
	void Report();

	TArray<FStop> Stops;
	int32 Current = INDEX_NONE;
	float Clock = 0.f;
	bool bShots = true;
	bool bQuit = false;
	bool bShotRequested = false;

	// Sums over the measured frames at the current stop.
	int32 Frames = 0;
	double FrameMs = 0.0;
	double GameMs = 0.0;
	double RenderMs = 0.0;
	double GpuMs = 0.0;
	double WorstFrameMs = 0.0;
	int64 DrawCalls = 0;
	int64 Primitives = 0;

	TArray<FString> Rows;

	UPROPERTY()
	TObjectPtr<ACameraActor> Camera;
};

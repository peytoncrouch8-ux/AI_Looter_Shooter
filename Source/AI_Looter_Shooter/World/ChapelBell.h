#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/Interactable.h"
#include "Story/StoryCondition.h"
#include "ChapelBell.generated.h"

class UBoxComponent;
class USceneComponent;
class USoundBase;
class UStaticMeshComponent;

/**
 * The bell of the Chapel of Saint Ada (Docs/Areas/RansomsRest.md, Main 4 "Hallowed Ground": "Ring the chapel bell"; the bell
 * that won't call the Unpaid to rest while the saint is dark). SM_ChapelBell (Art/Models/Buildings/Chapel.py) hangs in the
 * belfry on the chapel's SOCKET_Bell, its origin on the headstock's axis, which runs along its X; this actor stands at the
 * rope's woollen grip in the vestibule (the chapel's SOCKET_Interact), where the player rings it.
 *
 * Holding Interact on the rope for HoldSeconds rings it: the bell swings about its axis, a swing every SwingSeconds dying
 * away over RingSeconds, big enough to read through the belfry's louvres from the yard, and the clapper tolls at each end
 * of a swing while it still swings hard (TollSound, when there is one; none is made yet). It can't be rung again until
 * it's still, nor while RingWhen doesn't hold (empty: always). The player's interaction component tells the missions (a
 * held Interact on the actor tagged Bell_Chapel: Main 4's third step).
 *
 * Tools/Unreal/build_area_story.py (build_area_chapel.py) places it at the grip, with the Bell component on the belfry
 * socket; the level keeps that place. A small box round the grip is what the Interact key's line finds (it blocks nothing
 * else). The bell has no collision: it hangs out of everyone's reach. It never ticks while still.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API AChapelBell : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	/** The tag missions find it by (Main 4's "Ring the chapel bell"). */
	static const FName BellTag;

	AChapelBell();

	// --- IInteractable ---
	virtual FInteractionOptions GetInteractionOptions(const UInteractionComponent& User) const override;
	virtual bool Interact(UInteractionComponent& User, bool bHeld) override;
	virtual TOptional<FVector> GetInteractionLocation() const override;

	virtual void Tick(float DeltaSeconds) override;

	/**
	 * Rings it as a held Interact does (the console, a scene): the swing and its tolls. False when it can't be rung now.
	 * Missions hear of it from the player's interaction component, not from here.
	 */
	UFUNCTION(BlueprintCallable, Category = "Bell")
	bool Ring(AActor* ByWhom);

	/** It can be rung now: RingWhen holds, and it hangs still. */
	UFUNCTION(BlueprintPure, Category = "Bell")
	bool CanRing() const;

	/** It's swinging from a ring. */
	UFUNCTION(BlueprintPure, Category = "Bell")
	bool IsRinging() const { return RingTime >= 0.f; }

	/** How far it has swung now (degrees about its axis; 0 hanging still). */
	float GetSwing() const { return Swing; }

	/** The clapper's strokes since it was last rung (a toll each). */
	int32 GetStrokes() const { return Strokes; }

	/** Moves the swing on by DeltaSeconds (the tick does; a test level never ticks, so tests call it). */
	void Advance(float DeltaSeconds);

	/**
	 * The swing Time seconds after the pull: Degrees either way at first, a whole swing every Period seconds, the swings
	 * dying away until it hangs still at Seconds.
	 */
	static float SwingAt(float Time, float Degrees, float Period, float Seconds);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	/** Round the rope's grip at the actor's origin: what the Interact key's line finds; it blocks nothing else. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> Grip;

	/** The bell (SM_ChapelBell), its origin on its swing axis, which runs along its X; placed on the belfry's socket. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Bell;

	/** The prompt's words while the key is held. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bell")
	FText Prompt;

	/** How long Interact is held to ring it (s): a long pull on a heavy rope. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bell", meta = (ClampMin = "0.05", Units = "s"))
	float HoldSeconds = 1.2f;

	/** It can be rung only while this holds. Empty: any time. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bell")
	FStoryCondition RingWhen;

	/** How far it swings either way at first (degrees about its axis). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bell|Swing", meta = (ClampMin = "0", ClampMax = "80"))
	float SwingDegrees = 30.f;

	/** A whole swing, there and back (s): a heavy bell's slow one. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bell|Swing", meta = (ClampMin = "0.2", Units = "s"))
	float SwingSeconds = 2.4f;

	/** From the pull until it hangs still again (s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bell|Swing", meta = (ClampMin = "0.5", Units = "s"))
	float RingSeconds = 9.f;

	/** The clapper strikes while a swing still reaches this share of the first one's strength. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bell|Swing", meta = (ClampMin = "0", ClampMax = "1"))
	float TollWhileAbove = 0.25f;

	/** The toll at each stroke, quieter as the swing dies (none made yet: it plays when set). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bell")
	TObjectPtr<USoundBase> TollSound;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** Remembers the bell as placed, hanging still, the first time it's needed. */
	void CaptureRest();

	/** Turns the bell to Swing about its own axis, from where it hangs. */
	void ApplySwing();

	/** The clapper strikes, as hard as Strength (0 to 1). */
	void Toll(float Strength);

	/** The bell as placed: hanging still. */
	FTransform BellRest = FTransform::Identity;
	bool bRestCaptured = false;

	/** Seconds since the pull; negative while it hangs still. */
	float RingTime = -1.f;
	float Swing = 0.f;
	int32 Strokes = 0;
};

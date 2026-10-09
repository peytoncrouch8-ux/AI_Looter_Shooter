#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CreatureBarkActor.generated.h"

class USceneComponent;
class UWidgetComponent;

/**
 * The words of the bark being said, floating over its speaker (UCreatureVoiceDirector spawns one per level and reuses it,
 * so there is never more than one bark on screen): a screen-space UCreatureBarkWidget that follows the speaker's tag
 * (where its name and bar float; its head where it has none), fades in, holds and fades out. It ticks only while it shows.
 */
UCLASS(NotPlaceable, Transient)
class AI_LOOTER_SHOOTER_API ACreatureBarkActor : public AActor
{
	GENERATED_BODY()

public:
	ACreatureBarkActor();

	virtual void Tick(float DeltaSeconds) override;

	/** Shows Line over Speaker for Seconds (its fades included), said aloud or muttered, replacing whatever showed. */
	void Show(AActor* Speaker, const FText& Line, float Seconds, bool bMuttered);

	/** Gone at once. */
	void Hide();

	/** Moves its clock on (the tick does; a test level never ticks, so tests call it). */
	void Advance(float DeltaSeconds);

	bool IsShowing() const { return bShowing; }
	const FText& GetLine() const { return Line; }
	AActor* GetSpeaker() const { return Speaker.Get(); }
	bool IsMuttered() const { return bMuttered; }

	/** How visible the words are Age seconds into Seconds on screen (0-1): a quick fade in, a slower one out. */
	static float AlphaAt(float Age, float Seconds);

	static constexpr float FadeInSeconds = 0.15f;
	static constexpr float FadeOutSeconds = 0.45f;

	/** Over a speaker with no tag: this far above its middle (cm, before its size). */
	static constexpr float HeadRoom = 110.f;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UWidgetComponent> Widget;

private:
	/** Where the words hang now: on the speaker's tag, over its head, or where it last stood. */
	FVector AnchorNow() const;

	TWeakObjectPtr<AActor> Speaker;
	TWeakObjectPtr<USceneComponent> Anchor;
	FVector LastSpot = FVector::ZeroVector;
	FText Line;
	float Age = 0.f;
	float Seconds = 0.f;
	bool bShowing = false;
	bool bMuttered = false;
};

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/HitResult.h"
#include "PlayerSoundComponent.generated.h"

class AController;
class UAudioComponent;
class UHealthComponent;
class UPlayerLocomotionComponent;

/**
 * The player character's own sounds (ALooterCharacter makes it): footsteps by the surface underfoot (SoundSurface), at a
 * pace that follows the speed (walk, sprint, crouch); the jump (the character tells it, OnJumped) and the landing, louder
 * and lower the harder it comes down; a hurt grunt (now and then, not for every pellet); the death; and the low-health
 * heartbeat, a loop that runs while health is at the HUD's low share or under and stops with death or the character.
 * Steps stop while sliding: the slide has its own sounds (UPlayerLocomotionComponent).
 *
 * It ticks for the footsteps alone: a few numbers a frame, and one short trace against the floor's own component per
 * step for its material.
 */
UCLASS(ClassGroup = (Looter), meta = (BlueprintSpawnableComponent))
class AI_LOOTER_SHOOTER_API UPlayerSoundComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPlayerSoundComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** The character jumped (ACharacter::OnJumped). */
	void Jumped();

	/** Ground covered between steps (cm) at a speed (cm/s): a long stride running, short ones creeping. */
	static float StrideLength(float Speed);

	/** A step's loudness at a speed: crept steps are quiet, sprinted ones full. */
	static float StepVolume(float Speed);

	/** A landing's loudness for the speed it came down at (cm/s); 0 for a step's worth, which is only a footstep. */
	static float LandVolume(float FallSpeed);

	/** Slower than this (cm/s) on the ground isn't walking: no steps. */
	static constexpr float MinStepSpeed = 60.f;

	/** At least this long between hurt grunts (s): a burst of hits is one grunt. */
	static constexpr float HurtInterval = 0.35f;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	void HandleLanded(const FHitResult& Hit);

	UFUNCTION()
	void HandleDamaged(float Damage, bool bCritical, FVector HitLocation, AController* InstigatedBy, AActor* DamageCauser);

	UFUNCTION()
	void HandleHealthChanged(float NewHealth, float MaxHealth);

	UFUNCTION()
	void HandleDeath(AController* Killer);

	/** One footstep on the floor the movement stands on now. */
	void Step(float VolumeScale);

	/** One footstep on Floor (a floor the movement found, or the ground a landing hit): its material says the surface. */
	void StepOn(const FHitResult& Floor, float VolumeScale);

	/** Starts or stops the low-health heartbeat. */
	void SetHeartbeat(bool bOn);

	TWeakObjectPtr<UHealthComponent> Health;
	TWeakObjectPtr<UPlayerLocomotionComponent> Locomotion;
	TWeakObjectPtr<UAudioComponent> Heartbeat;

	/** Ground left to cover before the next step (cm). */
	float StrideLeft = 0.f;
	/** World time of the next hurt grunt allowed. */
	double NextHurtTime = 0.0;
};

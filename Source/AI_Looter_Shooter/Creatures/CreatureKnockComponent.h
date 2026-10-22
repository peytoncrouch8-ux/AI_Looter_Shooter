#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Creatures/CreatureRank.h"
#include "Engine/HitResult.h"
#include "CreatureKnockComponent.generated.h"

class ACreatureBase;
class AController;

/** Where a creature stands with a knock (UCreatureKnockComponent::GetState). */
UENUM()
enum class ECreatureKnock : uint8
{
	None,
	/** Flung through the air, tumbling. */
	Knocked,
	/** Landed (or dazed where it stood): reeling, sparks circling its head. */
	Dizzy,
};

/** How a knock looks (UCreatureKnockComponent::Knock). */
UENUM()
enum class ECreatureKnockStyle : uint8
{
	/** Head over heels along the throw, then reeling with sparks circling its head (the ember powers). */
	Fling,
	/** Rocked back along the throw and upright again as it lands, then a moment to gather itself: no stars (a shotgun blast). */
	Shove,
};

/**
 * A creature knocked off its feet and left reeling (the ember powers: Dust Devil's fling; a trip or a drag later). While
 * knocked or dizzy its brain waits, as for a stagger (ACreatureBase::TickBrain asks HoldsBrain): it starts no attack and
 * steers nowhere, its state's clock held.
 *  - Knocked: launched like a slime's hop (LaunchCharacter, its own speed replaced) along a flat way, to land about as far
 *    as asked, never over an edge: the throw is shortened to where there's ground near its feet's height, else it only
 *    reels where it stands. Its body tumbles over once along the throw (the mesh turned round the capsule's middle), and
 *    lands upright.
 *  - Dizzy: from the landing (ACharacter::LandedDelegate, or the movement finding the floor), for the seconds asked: a puff
 *    of dust where it came down, its body swaying, and three sparks circling over its head (FWeaponFX streaks).
 * A close shotgun blast knocks a small one back in the Shove style (UCreatureHitReactionComponent): rocked back, no stars.
 * A boss is never knocked or dazed (its fight's rules); a Legendary monster goes a shorter way (FlingShare); a bigger body
 * shorter still. Death ends it at once, the body put back. Ticks only while knocked or dizzy.
 */
UCLASS(ClassGroup = (Looter))
class AI_LOOTER_SHOOTER_API UCreatureKnockComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCreatureKnockComponent();

	/** Actor's knock component, if it has one (every ACreatureBase does). */
	static UCreatureKnockComponent* Find(const AActor* Actor);

	/**
	 * Flings it along Away (flattened) about Distance cm, the arc's top Height cm over where it leaves, then leaves it dizzy
	 * DizzySeconds once it lands. Returns the launch velocity; zero when it isn't thrown (a boss, the dead, one held back,
	 * no ground to land on: then it's only dazed where it stands, except a boss). A Shove knock only rocks back (ShoveTip)
	 * and gathers itself after, without the dizzy sway and sparks; with no ground for it, it only holds where it stands.
	 */
	FVector Knock(const FVector& Away, float Distance, float Height, float DizzySeconds, ECreatureKnockStyle InStyle = ECreatureKnockStyle::Fling);

	/** How a shove knock looks: rocked back this far (degrees) at its arc's top. */
	static constexpr float ShoveTip = 24.f;

	ECreatureKnockStyle GetStyle() const { return Style; }

	/** Dizzy where it stands for Seconds (a longer daze already on keeps its own). Nothing for a boss or the dead. */
	void Daze(float Seconds);

	ECreatureKnock GetState() const { return State; }
	bool IsKnocked() const { return State == ECreatureKnock::Knocked; }
	bool IsDizzy() const { return State == ECreatureKnock::Dizzy; }

	/** Knocked or dizzy: its brain waits (ACreatureBase::TickBrain). */
	bool HoldsBrain() const { return State != ECreatureKnock::None; }

	/** Seconds of dizziness left (0 when not dizzy; the whole daze while still in the air). */
	float GetDizzyLeft() const { return State == ECreatureKnock::None ? 0.f : DizzyLeft; }

	/** How far the body has tumbled round this throw (degrees, 0 to 360). */
	float GetTumbleDegrees() const { return TumbleDegrees; }

	/** It has come down from a knock (the movement's landing; public for the tests, which have no movement ticking). */
	void NoteLanded();

	/** Back on its feet: no knock, no daze, the body put back (death; tests). */
	void Recover();

	/** How far each rank is thrown, as a share of a Basic one's (Dust Devil's flings; Hallowed 60%, Ethereal 50%): a boss not at all. */
	static float FlingShare(ECreatureRank Rank);

	/** The launch that lands Distance cm along Away (flat, unit) on ground as high as it left, its arc's top Height over it. */
	static FVector LaunchVelocity(const FVector& Away, float Distance, float Height, float Gravity);

	/** A knock that hasn't landed after this long lands anyway (it fell somewhere the landing wasn't heard). */
	static constexpr float MaxAirSeconds = 2.5f;
	/** A landing spot may be this far under or over its feet (cm, at size 1); past it the throw is shortened. */
	static constexpr float LandingDrop = 150.f;
	static constexpr float LandingRise = 110.f;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	UFUNCTION()
	void HandleLanded(const FHitResult& Hit);

	ACreatureBase* GetCreature() const;
	/** Whether there's ground at Point near Feet's height (world-static: terrain and props). */
	bool HasLandingAt(const FVector& Point, float FeetZ, float Size) const;
	void StartDizzy();
	/** The body turned Degrees round the throw's axis and swayed by Sway (degrees of yaw), round the capsule's middle. */
	void PoseBody(float Degrees, float Sway);
	void CaptureRestPose();
	void RestoreRestPose();
	void ShowLanding();
	void DrawDizzySparks();

	ECreatureKnock State = ECreatureKnock::None;
	ECreatureKnockStyle Style = ECreatureKnockStyle::Fling;
	/** Seconds in the current state. */
	float StateTime = 0.f;
	float DizzyLeft = 0.f;
	float DizzyTotal = 0.f;
	/** The throw's flight time as planned, and its tumble axis (world, horizontal). */
	float PlannedAir = 1.f;
	FVector TumbleAxis = FVector::RightVector;
	float TumbleDegrees = 0.f;
	/** On landing, the tumble left over settles to upright from here. */
	float LandedTumble = 0.f;

	/** The mesh's own place in the capsule, kept while the body is turned. */
	bool bRestCaptured = false;
	FVector RestLocation = FVector::ZeroVector;
	FQuat RestRotation = FQuat::Identity;

	/** Where the dizzy sparks are round their circle (radians). */
	float SparkPhase = 0.f;
};

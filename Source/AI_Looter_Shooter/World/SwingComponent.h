#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "World/GustEvent.h"
#include "SwingComponent.generated.h"

/**
 * Something hung from a pivot that swings in the wind (a hanging sign on its SOCKET_Sign, a lantern on a hook, a bucket on
 * a rope): put this at the pivot and attach what hangs to it; it turns about HingeAxis (its own space) as a damped
 * pendulum PendulumLength long. While the player is within ActiveRadius it idles with a slight sway in the breeze; a gust
 * (UGustSubsystem::OnGust) pushes its bottom downwind as hard as the gust blows and as squarely as it faces it, and it
 * swings back and settles once the gust has passed.
 *
 * It ticks only while it moves: near the player (UGustSubsystem looks twice a second and wakes it) or still swinging
 * after a gust. Far off and at rest, it sleeps with its tick off. Its motion is set only while what hangs on it was
 * drawn lately, so nobody pays for a sign behind them. Never collides; what hangs on it brings its own collision, if any.
 */
UCLASS(ClassGroup = (World), meta = (BlueprintSpawnableComponent))
class AI_LOOTER_SHOOTER_API USwingComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	USwingComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** The axis it turns about, in its own space: the bar a sign hangs from (default: its Y, a sign facing along X). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Swing")
	FVector HingeAxis = FVector(0.0, 1.0, 0.0);

	/** From the pivot to the middle of what hangs (cm): sets how quickly it swings (a 60 cm sign about 0.65 a second). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Swing", meta = (ClampMin = "5", Units = "cm"))
	float PendulumLength = 60.f;

	/** How quickly a swing dies away (a share of critical damping: 0.06 swings on a good while, 1 never overshoots). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Swing", meta = (ClampMin = "0.01", ClampMax = "1"))
	float Damping = 0.07f;

	/** The furthest it swings either way (degrees): its hooks stop it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Swing", meta = (ClampMin = "0", ClampMax = "80", Units = "Degrees"))
	float MaxDegrees = 35.f;

	/** The breeze's slight sway while someone is near (degrees). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Swing", meta = (ClampMin = "0", Units = "Degrees"))
	float IdleDegrees = 1.5f;

	/** How far a full-strength gust on its face pushes it (degrees); edge-on, hardly at all. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Swing", meta = (ClampMin = "0", Units = "Degrees"))
	float GustDegrees = 18.f;

	/** It's awake while the view is this near (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Swing", meta = (ClampMin = "0", Units = "cm"))
	float ActiveRadius = 4000.f;

	/** A gust's push: its bottom swung downwind over the gust's life (UGustSubsystem calls it; tests may). */
	void Kick(const FGustEvent& InGust);

	/** Moves the swing on by DeltaSeconds (the tick does; tests call it). */
	void Step(float DeltaSeconds);

	/** The view came within ActiveRadius or left it (UGustSubsystem's look): near, it wakes; far, it sleeps once settled. */
	void SetNear(bool bInNear);
	bool IsNear() const { return bNear; }

	/** Swung from rest (degrees, about HingeAxis) and how fast (degrees a second). */
	float GetAngle() const { return Angle; }
	float GetSpeed() const { return Speed; }

	/** At rest with nothing pushing it. */
	bool IsSettled() const;

	/** Which way its bottom moves (world, level) as the angle grows: a gust along it swings it positive. */
	FVector GetSwingDirection() const;

	/** A pendulum's rate (radians a second) for a length (cm). */
	static float RateFor(float Length);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void HandleGust(const FGustEvent& InGust);
	void Wake();
	void Apply();
	/** What hangs on it was drawn in the last moment (or it has nothing that draws). */
	bool IsSeen() const;

	FQuat RestRotation = FQuat::Identity;
	float Angle = 0.f;
	float Speed = 0.f;
	/** The gust pushing it now (none once it's passed) and how far into it. */
	FGustEvent Gust;
	float GustAge = 0.f;
	bool bGust = false;
	bool bNear = false;
	float IdleTime = 0.f;
	float IdlePhase = 0.f;
	FDelegateHandle GustHandle;
};

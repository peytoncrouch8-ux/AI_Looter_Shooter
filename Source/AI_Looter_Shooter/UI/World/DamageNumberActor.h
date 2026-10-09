#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DamageNumberActor.generated.h"

class UWidgetComponent;

/**
 * Short-lived actor that floats a damage number up from the hit point and fades it out. A normal hit's number ticks in
 * with a small pop; a critical hit's is thrown: it slams in at twice its size, white-hot, settles with a little bounce
 * into the crit colour and arcs off to one side (a lob on the screen: up, over and a little back down), tilted the way
 * it flies, and stays a little longer (DamageNumberMotion has its numbers).
 */
UCLASS(Blueprintable)
class AI_LOOTER_SHOOTER_API ADamageNumberActor : public AActor
{
	GENERATED_BODY()

public:
	ADamageNumberActor();

	void Show(float Damage, bool bCritical);

	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage Number")
	float Lifetime = 0.9f;

	/** cm/s upward. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage Number")
	float RiseSpeed = 120.f;

	/** Random sideways drift so rapid hits don't stack on top of each other. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage Number")
	float MaxDriftSpeed = 60.f;

	/** A critical hit's number stays this much longer. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage Number")
	float CriticalExtraLife = 0.25f;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UWidgetComponent> Widget;

private:
	FVector Velocity = FVector::ZeroVector;
	float Age = 0.f;
	bool bCritical = false;
	/** A crit's lob goes this way across the screen (-1 left, 1 right). */
	float Side = 1.f;
};

/** A damage number's motion on the screen over its life, as plain math (the actor plays it; the tests check it). */
namespace DamageNumberMotion
{
	struct FFrame
	{
		/** On-screen offset (px, y down), scale, tilt (degrees) and how white-hot it is (0-1). */
		FVector2D Offset = FVector2D::ZeroVector;
		float Scale = 1.f;
		float Angle = 0.f;
		float Heat = 0.f;
	};

	/** Its frame Age seconds in; Side: a crit's lob's way (-1 or 1). */
	AI_LOOTER_SHOOTER_API FFrame At(float Age, bool bCritical, float Side);

	/** A crit slams in at this size, and its lob rises this fast against this pull (px/s, px/s^2) while drifting this far sideways (px). */
	inline constexpr float CritSlamScale = 2.2f;
	inline constexpr float CritLobSpeed = 220.f;
	inline constexpr float CritLobPull = 420.f;
	inline constexpr float CritLobSide = 36.f;
}

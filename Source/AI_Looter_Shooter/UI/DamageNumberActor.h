#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DamageNumberActor.generated.h"

class UWidgetComponent;

/** Short-lived actor that floats a damage number up from the hit point and fades it out. */
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

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UWidgetComponent> Widget;

private:
	FVector Velocity = FVector::ZeroVector;
	float Age = 0.f;
	bool bCritical = false;
};

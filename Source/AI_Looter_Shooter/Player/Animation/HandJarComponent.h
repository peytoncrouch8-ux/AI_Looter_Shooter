#pragma once

#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"
#include "HandJarComponent.generated.h"

class UPlayerThrowComponent;

/**
 * The grave-salt jar in the body's left hand through a throw's wind-up, for third person (F5): the tin
 * (AGraveSaltGrenade::FindJarMesh) in the left hand's grip socket, shown from the throw's start until it leaves the hand
 * (ThrowMotion::IsJarInHand), where the real grenade takes over. The stance layer (LooterStancePose::DescribeThrow) swings
 * the hand it sits in.
 *
 * Drawn as the body is: never to its owner in first person (UPlayerThrowComponent shows its own jar under the view
 * there), though its shadow falls with the body's; seen in third person. Full size whatever the player's size, like the
 * jar that flies off.
 */
UCLASS(ClassGroup = (Looter))
class AI_LOOTER_SHOOTER_API UHandJarComponent : public UStaticMeshComponent
{
	GENERATED_BODY()

public:
	UHandJarComponent();

	/** The body's socket it sits in: the mannequin's left-hand grip point (the middle of what the hand holds). */
	static FName HandSocket() { return TEXT("HandGrip_L"); }

	/** Whether a throw has the jar in the hand right now (it shows then). */
	bool IsInHand() const { return IsVisible(); }

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	/** Follows the throw (shown while the jar is in the hand) and the body's first-person drawing. */
	void Refresh();

	TWeakObjectPtr<const UPlayerThrowComponent> Throw;
};

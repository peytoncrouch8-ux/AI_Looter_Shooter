#pragma once

#include "CoreMinimal.h"
#include "Components/SkeletalMeshComponent.h"
#include "BodyFigureComponent.generated.h"

class UIKRetargeter;

/**
 * The player's own body, Ellis (SK_Ellis, the user's call of 2026-10-15), worn over the mannequin, which stays the
 * animated body: attached to it, Ellis copies its pose every frame through RTG_MannyToEllis (UBodyRetargetAnimInstance),
 * ticking after it. Once both assets are imported the mannequin is hidden but keeps animating (the guns, the hands' IK
 * and every socket still use it), and Ellis is drawn the way the mannequin would be: hidden to its owner in first person
 * but casting the player's shadow, seen in third (UPlayerViewComponent switches the mannequin; this follows it). Without
 * them it stays hidden and the mannequin shows as before.
 */
UCLASS(ClassGroup = (Looter), meta = (BlueprintSpawnableComponent))
class AI_LOOTER_SHOOTER_API UBodyFigureComponent : public USkeletalMeshComponent
{
	GENERATED_BODY()

public:
	UBodyFigureComponent();

	/** The retargeter from the mannequin to this body (UBodyRetargetAnimInstance reads it as it starts). */
	UIKRetargeter* GetRetargeter() const { return Retargeter; }

	/** Ellis is worn: his mesh and the retargeter are there and the mannequin is hidden under him. */
	bool IsWorn() const;

	/** The skeletal mesh this is attached to and copies (the mannequin), or null. */
	USkeletalMeshComponent* GetBody() const;

	/** Ellis's mesh and the retargeter, found by path once imported (null before). */
	static USkeletalMesh* FindFigureMesh();
	static UIKRetargeter* FindRetargeter();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** Hides the mannequin under Ellis, ticks after it, and draws as it is drawn (FollowBody). */
	void Wear();
	/** Drawn as the mannequin would be: hidden to its owner in first person (a world-space stand-in for the shadow). */
	void FollowBody();

	UPROPERTY(Transient)
	TObjectPtr<UIKRetargeter> Retargeter;

	/** The mannequin this hid, shown again if Ellis goes. */
	TWeakObjectPtr<USkeletalMeshComponent> HiddenBody;
};

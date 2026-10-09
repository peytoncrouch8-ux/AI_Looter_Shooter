#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraModifier.h"
#include "Camera/CameraTypes.h"
#include "Player/ViewKick.h"
#include "CameraShakeModifier.generated.h"

class APlayerController;

/**
 * Every shake and kick of the local player's view, as one camera modifier on their camera manager: no shake assets and
 * no camera plugin. It moves only what the player sees: the aim (the controller's rotation) stays where it was, and the
 * camera components and their offsets (the slide's tilt) are left alone; first and third person alike. Two kinds:
 *  - Shakes (AddShake; a boss fight's big moments, BossCameraShake::Kick): smooth noise (about 1.6 degrees and 4 cm at
 *    full strength), each dying away over its seconds as the square of the time left; together they stack to a little
 *    past one full shake.
 *  - Kicks (AddKick; ViewKicks: a shot by its gun, the jolt of being hurt, a kill's punch): a damped spring each, turning
 *    the view and widening or narrowing it (FViewKickStack).
 * The player's camera shake setting (UControlSettingsSubsystem::GetCameraShake, 0 turns all of it off; the
 * Looter.CameraShake command sets it) scales both, read each frame so a change in the menu shows at once.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UCameraShakeModifier : public UCameraModifier
{
	GENERATED_BODY()

public:
	/** The modifier on Player's camera manager, made if it isn't there yet (null without a camera manager). */
	static UCameraShakeModifier* FindOrAdd(APlayerController* Player);

	/** Adds Kick to a local Player's view. Nothing for a player who isn't local. */
	static void Kick(APlayerController* Player, const FViewKick& Kick);

	/** Adds a shake of Strength (0-1) dying away over Seconds. */
	void AddShake(float Strength, float Seconds);

	void AddKick(const FViewKick& Kick) { Kicks.Add(Kick); }

	/** How hard it shakes now (0 still; past 1 while shakes stack), before the setting. */
	float GetAmount() const;

	const FViewKickStack& GetKicks() const { return Kicks; }

	/** The camera modifier's work: the view turned, shifted and widened by this frame's shakes and kicks. Never stops the modifiers after it. */
	virtual bool ModifyCamera(float DeltaTime, FMinimalViewInfo& InOutPOV) override;

	/** A shake at full strength (1): the view's turn (degrees: pitch, yaw, roll) and its shift (cm). */
	static constexpr float MaxPitch = 1.6f;
	static constexpr float MaxYaw = 1.1f;
	static constexpr float MaxRoll = 0.9f;
	static constexpr float MaxShift = 4.f;

	/** Shakes stack to no more than this, and no more than this many at once. */
	static constexpr float MaxAmount = 1.25f;
	static constexpr int32 MaxShakes = 6;

private:
	/** The player's camera shake setting (1 without a local player, as in tests). */
	float GetShakeScale() const;

	struct FShake
	{
		float Strength = 0.f;
		float Age = 0.f;
		float Life = 0.5f;
	};
	TArray<FShake, TInlineAllocator<MaxShakes>> Shakes;
	float Clock = 0.f;

	FViewKickStack Kicks;
};

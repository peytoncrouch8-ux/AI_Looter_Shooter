#pragma once

#include "CoreMinimal.h"
#include "WeaponRecoil.generated.h"

/** How a weapon kicks when it fires. Set per weapon on its definition. */
USTRUCT(BlueprintType)
struct FWeaponRecoilProfile
{
	GENERATED_BODY()

	/** How far the gun jumps back toward you per shot (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recoil", meta = (ClampMin = "0"))
	float KickBack = 2.5f;

	/** How far the muzzle flips up per shot (degrees). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recoil", meta = (ClampMin = "0"))
	float MuzzleFlip = 3.f;

	/** Random sideways twist of the gun per shot (degrees, either way). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recoil", meta = (ClampMin = "0"))
	float MuzzleTwist = 0.8f;

	/** Random roll of the gun per shot (degrees, either way). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recoil", meta = (ClampMin = "0"))
	float Roll = 1.5f;

	/** How far each shot kicks your aim up (degrees). It settles back once you stop firing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recoil", meta = (ClampMin = "0"))
	float AimKick = 0.35f;

	/** Random sideways aim kick per shot (degrees, either way). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recoil", meta = (ClampMin = "0"))
	float AimKickSide = 0.12f;

	/** How fast the aim settles back after you stop firing (higher is faster). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recoil", meta = (ClampMin = "0.1"))
	float AimRecovery = 7.f;

	/** How fast the gun springs back into place (1 = normal; heavy guns are slower). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recoil", meta = (ClampMin = "0.2", ClampMax = "3"))
	float Snappiness = 1.f;
};

/**
 * Recoil for whoever holds a gun: each shot kicks the gun (back, muzzle up, a little twist and roll) on springs that
 * bounce it back into place, and kicks the aim up, which settles back after the shooting stops. Plain math, no engine
 * state: the owner feeds it shots and time, applies the aim change to the view and poses the gun with the kick.
 */
class AI_LOOTER_SHOOTER_API FWeaponRecoil
{
public:
	void AddShot(const FWeaponRecoilProfile& Profile, FRandomStream& Random);

	/**
	 * Advances the kick springs and the aim kick. PlayerPitchInput is how far the player moved the aim up (+) or down (-)
	 * since the last tick, not counting recoil: pulling down against the kick uses up the recovery, so the aim never ends
	 * up below where the player put it. Returns the aim change (pitch, yaw) to add to the view this tick.
	 */
	FRotator Tick(float DeltaSeconds, float PlayerPitchInput = 0.f);

	/** How far back the gun is kicked right now (cm, positive = toward the shooter). */
	float GetKickBack() const { return Back.Value; }

	/** The gun's kick rotation right now (pitch up = muzzle climbing), in the gun's own frame. */
	FRotator GetKickRotation() const { return FRotator(Pitch.Value, Yaw.Value, RollSpring.Value); }

	/** Aim kick currently added to the view (degrees), still to be recovered. */
	FRotator GetAimOffset() const { return AimApplied; }

	bool IsSettled() const;
	void Reset();

private:
	/** An underdamped spring toward zero: a shot adds velocity, the value overshoots a little and settles. */
	struct FSpring
	{
		float Value = 0.f;
		float Velocity = 0.f;
		void Step(float DeltaSeconds, float Frequency, float Damping);
	};

	FSpring Back;
	FSpring Pitch;
	FSpring Yaw;
	FSpring RollSpring;
	float Frequency = 9.f;

	/** Aim kick still to be added (it goes in over a few frames rather than snapping) and already added (to recover). */
	FRotator AimPending = FRotator::ZeroRotator;
	FRotator AimApplied = FRotator::ZeroRotator;
	float AimRecovery = 7.f;
	float SinceShot = 100.f;
};

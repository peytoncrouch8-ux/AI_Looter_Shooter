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

	/**
	 * The aim kick grows by this share of the first shot's for each shot of a burst (the rifle's climb you pull down against);
	 * 0 = every shot kicks the same. Not data: AWeaponBase::GetRecoilProfile sets it from the Looter.Gunfeel.Recoil.RifleClimb
	 * variables.
	 */
	float ClimbPerShot = 0.f;

	/** ...and the most it grows to, as a multiple of the first shot's (1 = no climb). Set with ClimbPerShot. */
	float ClimbMax = 1.f;

	/**
	 * The most a held burst may lift the aim above where the player has put it (degrees of aim kick not yet recovered or
	 * pulled down; 0 = no cap). A shot's kick past it keeps only ClimbCapFloor of its size, so a long belt climbs a few
	 * degrees and then only sways and nods instead of walking into the sky (the LMG, 2026-10-29 play-check). Not data:
	 * GetRecoilProfile sets them from Looter.Gunfeel.LMG.ClimbCap / .ClimbCapFloor.
	 */
	float ClimbCap = 0.f;
	float ClimbCapFloor = 0.f;

	/**
	 * The gun model's kick through the sight as a share of the hip's (GunfeelKick::AimedModelKick(1, ...)): the muzzle flip and
	 * twist, the roll, and the push back along the sight line. 1 = the same as the hip. Not data: GetRecoilProfile sets them
	 * from the Looter.Gunfeel.Recoil.Aimed* variables; AddShot blends them in by the shot's AimAlpha.
	 */
	float AimedBack = 1.f;
	float AimedFlip = 1.f;
	float AimedRoll = 1.f;
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
	 * The same shot taken with the sights up AimAlpha (0 hip, 1 raised): the gun model's back, flip, twist and roll are blended
	 * toward the profile's Aimed* shares, so a sighted shot doesn't throw the sights off the target. The aim kick (what the
	 * player pulls down against) is the profile's whole.
	 */
	void AddShot(const FWeaponRecoilProfile& Profile, FRandomStream& Random, float AimAlpha);

	/** Adds Degrees (+ right) to the aim kick still to come: the recoil pattern's sideways drift. */
	void AddAimYaw(float Degrees) { AimPending.Yaw += Degrees; }

	/**
	 * The last shot's aim kick holds, not recovering at all, for Seconds after it (0 recovers as ever): a bolt gun's kick
	 * held while its bolt is worked (gun identity). Call it after AddShot; the next shot clears it.
	 */
	void HoldRecovery(float Seconds) { HoldSeconds = FMath::Max(Seconds, 0.f); }

	/** Where the last shot added sits in its burst (0 the first), for the pattern and the first shot's reward. */
	int32 GetShotInBurst() const { return FMath::Max(BurstShots - 1, 0); }

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
	/** The aim kick holds until SinceShot reaches this (HoldRecovery). */
	float HoldSeconds = 0.f;
	/** Shots so far in this burst (a pause longer than BurstGap ends it), for the aim kick's climb. */
	int32 BurstShots = 0;

public:
	/** A pause between shots longer than this (s) ends a burst, and the climb starts over. */
	static constexpr float BurstGap = 0.35f;

	/** The climb the next shot would kick with, as a multiple of the profile's AimKick (1 on the first shot of a burst). */
	float GetClimb(const FWeaponRecoilProfile& Profile) const;

	/** How far the aim is held up by kicks not yet recovered or pulled down right now (degrees; what ClimbCap measures). */
	float GetHeldClimb() const { return AimApplied.Pitch + AimPending.Pitch; }

private:
	/** A shot's upward aim kick Kick (degrees) trimmed by the profile's ClimbCap: whole under it, ClimbCapFloor of it past. */
	float CappedPitchKick(const FWeaponRecoilProfile& Profile, float Kick) const;
};

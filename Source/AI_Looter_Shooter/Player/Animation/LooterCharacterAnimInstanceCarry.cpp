#include "Player/Animation/LooterCharacterAnimInstance.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Player/Animation/LooterClimbPose.h"
#include "Player/Animation/LooterStancePoseCarry.h"
#include "Player/PlayerLocomotionComponent.h"
#include "Player/PlayerMeleeComponent.h"
#include "Player/PlayerViewComponent.h"
#include "Player/WeaponSwapMotion.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponDefinition.h"
#include "GameFramework/Pawn.h"

// ULooterCharacterAnimInstance's carry: on the game thread it works out how the gun in the body's hands is carried
// (LooterStancePoseCarry.h) from what the player is doing: lowered once they've been on the move a moment at ease, back
// to the shoulder the instant they aim, shoot or strike (or draw a gun), across the chest in a sprint. The view turns the
// gun by it and the stance layer the hands. Only third person shows it: first person's gun is the camera's own.

void ULooterCharacterAnimInstance::RefreshCarry(float DeltaSeconds, const AWeaponBase* Weapon)
{
	const APawn* Pawn = TryGetPawnOwner();
	if (!Pawn || !bHoldsWeapon)
	{
		// No player (the loadout screen's stand-in) or a body that holds no gun: shouldered, and fresh for the next one.
		ReadyClock.Reset();
		Carry = FLooterCarry();
		return;
	}

	// Aiming, shooting, striking or a gun coming up after a swap brings it to the shoulder; on the move at ease it goes low.
	const UPlayerViewComponent* Viewer = View.Get();
	const UPlayerMeleeComponent* Striker = Melee.Get();
	const bool bAlert = (Viewer && (Viewer->WantsToAim() || Viewer->IsAiming()))
		|| (Weapon && (Weapon->WantsToFire() || Weapon->HasShotPending() || Weapon->GetReadyAlpha() < 1.f))
		|| (Striker && Striker->IsSwinging());
	const bool bMoving = Pawn->GetVelocity().Size2D() > LooterCarry::MovingSpeed;
	ReadyClock.Update(DeltaSeconds, bAlert, bMoving);

	// A reload (its own hands, the gun tipped for them) and a climb (the view carries the gun up out of the way) take over
	// from the carry as they come in, by the same measures the view uses for them.
	const UPlayerLocomotionComponent* Loco = Locomotion.Get();
	const float Reloading = Weapon ? FMath::Clamp(Weapon->GetReloadBlend(), 0.f, 1.f) : 0.f;
	const float Climbing = Loco ? FMath::Clamp(Loco->GetTraversalAlpha() / LooterClimbPose::VaultAlphaShare, 0.f, 1.f) : 0.f;
	const float Free = (1.f - Reloading) * (1.f - Climbing);
	const UWeaponDefinition* Definition = Weapon ? Weapon->GetInstance().Definition.Get() : nullptr;
	Carry.LowReady = ReadyClock.GetAlpha() * Free;
	Carry.Sprint = (Loco ? Loco->GetSprintAlpha() : 0.f) * Free;
	Carry.bShortGun = Definition && Definition->HoldReach > 0.f;
	// A swap: the arm lowers the gun in hand to the side out of the way and raises the next, on the swap's own clock (the
	// first-person view's, WeaponSwapMotion), so the change reads from behind as well.
	const UWeaponManagerComponent* Manager = WeaponManager.Get();
	Carry.Swap = Weapon && Manager ? WeaponSwapMotion::BodyShare(Manager->GetGunLowered()) * Free : 0.f;
}

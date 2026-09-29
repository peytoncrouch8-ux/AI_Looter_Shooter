#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Settings/PawnInputBinding.h"
#include "Weapons/WeaponRecoil.h"
#include "PlayerViewComponent.generated.h"

class ACharacter;
class AController;
class APawn;
class AWeaponBase;
class UAnimInstance;
class UCameraComponent;
class UPlayerLocomotionComponent;
class USkeletalMeshComponent;
class USpringArmComponent;
class UWeaponManagerComponent;

UENUM(BlueprintType)
enum class EPlayerViewMode : uint8
{
	FirstPerson,
	/** Behind the character, over the right shoulder, looking where you aim. */
	ThirdPerson,
	/** In front of the character looking back at them (Minecraft's second F5 press). Controls stay the character's. */
	ThirdPersonFront
};

/**
 * First/third-person camera for the player character, cycled like Minecraft's F5: first person -> behind -> front ->
 * first person. You keep full control (move, sprint, crouch, shoot, loot) in every view.
 *
 *  - Third person: a collision-tested spring arm over the right shoulder. It eases out of the head when you switch,
 *    follows crouching without popping, and pulls in instead of clipping through walls. The full body becomes
 *    visible and holds the gun in its hands; shots still go exactly where the crosshair points.
 *  - First person: the original camera and camera-held gun.
 *
 * Also owns the camera field of view (sprinting widens it in every view) and gives the body the armed animation
 * set whenever a weapon is in hand, so the third-person body and the first-person shadow match what you hold.
 *
 * Recoil lives here too: every shot of the gun in hand kicks the aim up (it settles back once you stop, and pulling down
 * against it counts) and kicks the gun on springs. The first-person view model and the third-person arms and chest
 * read the gun's kick from GetKickBack / GetKickRotation.
 */
UCLASS(ClassGroup = (Looter), meta = (BlueprintSpawnableComponent))
class AI_LOOTER_SHOOTER_API UPlayerViewComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPlayerViewComponent();

	UFUNCTION(BlueprintCallable, Category = "View")
	void SetViewMode(EPlayerViewMode NewMode);

	/** First person -> third person -> front view -> first person. */
	UFUNCTION(BlueprintCallable, Category = "View")
	void CycleViewMode();

	UFUNCTION(BlueprintPure, Category = "View")
	EPlayerViewMode GetViewMode() const { return Mode; }

	bool IsFirstPerson() const { return Mode == EPlayerViewMode::FirstPerson; }

	/**
	 * Where shots start and which way they go. First person and third person follow the crosshair (in third person
	 * the start is moved up to the character, so nothing between the camera and the character is hit); the front
	 * view shoots from the character's eyes the way they face.
	 */
	void GetAimViewPoint(FVector& OutLocation, FRotator& OutRotation) const;

	/** The character's own first-person camera (any camera that isn't on a spring arm). */
	static UCameraComponent* FindFirstPersonCamera(const AActor* Owner);

	/**
	 * Where the character looks and aims: the controller's rotation, which is always the way the character faces.
	 * Not the camera's rotation: the front-view camera looks back at the character, and anything following it (the held
	 * gun, the torso's aim) would turn around too.
	 */
	FRotator GetAimRotation() const;

	/** Which way a gun held in the character's hands points: along the player's aim (plus its recoil kick), or carried low while sprinting. */
	FQuat GetHeldWeaponRotation() const;

	/** How far the gun in hand has kicked back toward the shooter right now (cm). */
	float GetKickBack() const { return Recoil.GetKickBack(); }

	/** How the gun in hand is kicked right now, in its own frame (pitch up = muzzle climbing). */
	FRotator GetKickRotation() const { return Recoil.GetKickRotation(); }

	/** The body's animation with or without a gun in hand (the character switches between the two as weapons change). */
	TSubclassOf<UAnimInstance> GetBodyAnimClass(bool bArmed) const { return bArmed && LoadedArmedAnimClass ? LoadedArmedAnimClass : UnarmedAnimClass; }

	// --- Third-person camera ---

	/** Distance from the pivot to the camera, behind the character. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "View|Third Person", meta = (ClampMin = "50"))
	float ArmLength = 270.f;

	/**
	 * Camera offset at the end of the arm, in view space: Y to the right, Z up. Over the right shoulder by default, which
	 * keeps the character left of the crosshair (picked from side-by-side comparisons of closer, farther and centered).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "View|Third Person")
	FVector ShoulderOffset = FVector(0.f, 60.f, 25.f);

	/** Arm pivot height above the feet when standing / fully crouched (around the shoulders). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "View|Third Person", meta = (ClampMin = "0"))
	float PivotHeight = 150.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "View|Third Person", meta = (ClampMin = "0"))
	float CrouchedPivotHeight = 108.f;

	/** Distance of the front-view camera from the character's face. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "View|Third Person", meta = (ClampMin = "50"))
	float FrontArmLength = 210.f;

	/** Radius of the camera's collision probe; walls pull the camera in rather than letting it clip through. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "View|Third Person", meta = (ClampMin = "0"))
	float ProbeSize = 12.f;

	/** Seconds for the camera to ease out of the head into its third-person spot. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "View|Third Person", meta = (ClampMin = "0.01"))
	float PullOutTime = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "View|Third Person", meta = (ClampMin = "30", ClampMax = "140"))
	float ThirdPersonFieldOfView = 90.f;

	/** Animation Blueprint the body uses while a weapon is in hand (rifle idle, locomotion and jumps). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "View|Body")
	TSoftClassPtr<UAnimInstance> ArmedAnimClass;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	UFUNCTION()
	void HandleControllerChanged(APawn* Pawn, AController* OldController, AController* NewController);

	UFUNCTION()
	void HandleActiveWeaponChanged(AWeaponBase* NewWeapon, AWeaponBase* OldWeapon);

	UFUNCTION()
	void HandleWeaponFired();

	/** Listens to the gun in hand's shots (and stops listening to the last one). */
	void BindFiringWeapon(AWeaponBase* Weapon);
	/** Advances the recoil and adds its aim kick to the view. */
	void UpdateRecoil(float DeltaTime);

	void SetupInput(AController* Controller);
	void ApplyMode();
	void RefreshBodyAnimation();
	void UpdateBoom(float DeltaTime);
	void UpdateFieldOfView();
	/** Third person: grip in the right hand (which the animation moves), barrel along the aim. */
	void UpdateHeldWeapon();

	TWeakObjectPtr<ACharacter> Character;
	TWeakObjectPtr<UCameraComponent> FirstPersonCamera;
	/** First-person arms: they only carry the camera now (the gun is camera-held), so they stay hidden. */
	TWeakObjectPtr<USkeletalMeshComponent> Arms;
	TWeakObjectPtr<UPlayerLocomotionComponent> Locomotion;
	TWeakObjectPtr<UWeaponManagerComponent> WeaponManager;

	UPROPERTY(Transient)
	TObjectPtr<USpringArmComponent> Boom;

	UPROPERTY(Transient)
	TObjectPtr<UCameraComponent> ThirdPersonCamera;

	UPROPERTY(Transient)
	TSubclassOf<UAnimInstance> UnarmedAnimClass;

	UPROPERTY(Transient)
	TSubclassOf<UAnimInstance> LoadedArmedAnimClass;

	FPawnInputBinding InputBinding;
	EPlayerViewMode Mode = EPlayerViewMode::FirstPerson;
	/** 0 = camera at the head, 1 = fully out on the arm. */
	float PullOut = 1.f;
	float FirstPersonFieldOfView = 90.f;

	FWeaponRecoil Recoil;
	FRandomStream RecoilRandom{ 0x2ec011 };
	TWeakObjectPtr<AWeaponBase> FiringWeapon;
	/** The view's pitch right after our own recoil change last tick, to tell the player's mouse movement apart from ours. */
	float LastViewPitch = 0.f;
	bool bHaveLastViewPitch = false;
};

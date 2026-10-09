#include "Player/PlayerThrowComponent.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Combat/GraveSaltGrenade.h"
#include "Combat/GraveSaltRules.h"
#include "Player/CameraShakeModifier.h"
#include "Player/PlayerMeleeComponent.h"
#include "Player/PlayerThrowMotion.h"
#include "Player/PlayerViewComponent.h"
#include "Weapons/WeaponBase.h"
#include "Camera/CameraComponent.h"
#include "CollisionQueryParams.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/HitResult.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

// UPlayerThrowComponent's throw: its clock, the jar in the off hand and the gun dipping out of its way (ThrowMotion), and
// the jar leaving the hand as a real grenade (AGraveSaltGrenade).

namespace
{
	/** What a throw's first stretch checks for a wall between the eye and the hand: the bullets' channel, as the jar's flight. */
	constexpr ECollisionChannel ReleaseChannel = ECC_GameTraceChannel2;
	/** The jar in the off hand: its size in first person (the gun is drawn the same way, at its own field of view). */
	constexpr float JarModelScale = 1.f;
}

void UPlayerThrowComponent::StartThrow()
{
	const UWorld* World = GetWorld();
	LastThrowTime = World ? World->GetTimeSeconds() : 0.0;
	++ThrowCount;
	bThrowing = true;
	bReleased = false;
	ThrowTime = 0.f;

	// With a gun in hand it dips out of the way: the gun holds its fire, and a reload under way is cut short (it starts
	// over after), exactly as for a strike (the gun's melee swing state, reused).
	AWeaponBase* Gun = GetWeaponInHand();
	ThrowWeapon = Gun;
	const bool bCutReload = Gun && Gun->BeginMeleeSwing();

	// Heard and felt the moment the key goes down: the jar coming up with a rattle of salt, the view drawing back.
	LooterSound::Play2D(this, ThrowCue::Throw);
	if (APlayerController* Player = GetPlayer())
	{
		UCameraShakeModifier::Kick(Player, ThrowMotion::WindUp());
	}
	SetComponentTickEnabled(true);
	UpdatePoses();
	UE_LOG(LogLooter, Verbose, TEXT("Grenade: throw %d%s"), ThrowCount, bCutReload ? TEXT(", reload cut short") : TEXT(""));
}

void UPlayerThrowComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!bThrowing)
	{
		SetComponentTickEnabled(false);
		return;
	}
	if (IsOwnerDead())
	{
		EndThrow(false);
		return;
	}
	// A strike swung mid-throw: the gun's swing state and pose are the strike's now, and it ends them; the throw only lets
	// go of it. (The strike's own gate doesn't know of throws yet.)
	const UPlayerMeleeComponent* Melee = UPlayerMeleeComponent::Find(GetOwner());
	if (ThrowWeapon.IsValid() && Melee && Melee->IsSwinging())
	{
		ThrowWeapon.Reset();
	}
	// The gun left the hand mid-throw (a swap, a drop): back in its hold with nothing following on it from here; the
	// throw goes on without it.
	if (ThrowWeapon.IsValid() && ThrowWeapon.Get() != GetWeaponInHand())
	{
		ReleaseWeapon(false);
	}
	ThrowTime += FMath::Min(DeltaTime, 0.1f);
	if (!bReleased && ThrowTime >= FThrowRules::ReleaseSeconds)
	{
		Release();
	}
	UpdatePoses();
	if (bThrowing && ThrowTime >= FThrowRules::ThrowSeconds)
	{
		EndThrow(true);
	}
}

void UPlayerThrowComponent::Release()
{
	bReleased = true;
	HideJarModel();
	APawn* Owner = Cast<APawn>(GetOwner());
	UWorld* World = GetWorld();
	if (!Owner || !World || Grenades <= 0)
	{
		return;
	}
	FVector Eye;
	FVector Look;
	GetAim(Eye, Look);
	// From the off hand, low and left of the eye; never through a wall the player stands against: the jar starts where
	// the line from the eye to the hand meets it, and bounces straight back off it.
	FVector Start = FThrowRules::ReleasePoint(Eye, Look);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(GrenadeRelease), false, Owner);
	TArray<AActor*> Held;
	Owner->GetAttachedActors(Held);
	Params.AddIgnoredActors(Held);
	FHitResult Wall;
	if (World->SweepSingleByChannel(Wall, Eye, Start, FQuat::Identity, ReleaseChannel, FCollisionShape::MakeSphere(FGraveSaltRules::CollisionRadius), Params))
	{
		Start = Wall.bStartPenetrating ? Eye : Wall.Location;
	}
	const FVector Velocity = FThrowRules::LaunchVelocity(Look, Owner->GetVelocity());
	if (!AGraveSaltGrenade::Throw(World, Start, Velocity, Owner, GetLevelScale()))
	{
		return;
	}
	SetGrenades(Grenades - 1, EGrenadeChange::Thrown);
	if (APlayerController* Player = GetPlayer())
	{
		UCameraShakeModifier::Kick(Player, ThrowMotion::Release());
	}
	UE_LOG(LogLooter, Verbose, TEXT("Grenade: thrown at %.0f cm/s, %d left"), Velocity.Size(), Grenades);
}

void UPlayerThrowComponent::EndThrow(bool bResume)
{
	if (!bThrowing)
	{
		return;
	}
	bThrowing = false;
	HideJarModel();
	ReleaseWeapon(bResume);
	SetComponentTickEnabled(false);
}

void UPlayerThrowComponent::ReleaseWeapon(bool bResume)
{
	AWeaponBase* Gun = ThrowWeapon.Get();
	ThrowWeapon.Reset();
	if (Gun)
	{
		Gun->EndMeleeSwing(bResume);
	}
}

void UPlayerThrowComponent::UpdatePoses()
{
	if (AWeaponBase* Gun = ThrowWeapon.Get())
	{
		const FThrowPose Pose = ThrowMotion::GunPose(ThrowTime);
		Gun->SetMeleePose(Pose.Offset, Pose.Rotation);
	}

	// The jar is only seen in first person: a third-person body has no throw to carry it (the grenade flies all the same).
	const APawn* Owner = Cast<APawn>(GetOwner());
	const UPlayerViewComponent* View = Owner ? Owner->FindComponentByClass<UPlayerViewComponent>() : nullptr;
	bool bShown = false;
	const FThrowPose Pose = ThrowMotion::JarPose(ThrowTime, bShown);
	if (!bShown || bReleased || !GetPlayer() || (View && !View->IsFirstPerson()))
	{
		HideJarModel();
		return;
	}
	if (UStaticMeshComponent* Jar = GetOrMakeJarModel())
	{
		Jar->SetRelativeLocationAndRotation(Pose.Offset, Pose.Rotation);
		Jar->SetVisibility(true);
	}
}

UStaticMeshComponent* UPlayerThrowComponent::GetOrMakeJarModel()
{
	if (JarModel)
	{
		return JarModel;
	}
	AActor* Owner = GetOwner();
	UCameraComponent* Camera = UPlayerViewComponent::FindFirstPersonCamera(Owner);
	UStaticMesh* Mesh = AGraveSaltGrenade::FindJarMesh();
	if (!Owner || !Camera || !Mesh)
	{
		// No tin yet (before the model's import) or no first-person camera: the throw still happens, unseen in the hand.
		return nullptr;
	}
	JarModel = NewObject<UStaticMeshComponent>(Owner, TEXT("GrenadeInHand"));
	JarModel->SetStaticMesh(Mesh);
	JarModel->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	JarModel->SetCastShadow(false);
	JarModel->SetCanEverAffectNavigation(false);
	JarModel->SetupAttachment(Camera);
	JarModel->SetRelativeScale3D(FVector(JarModelScale));
	// Drawn like the gun in hand: first-person rendering, at the view model's own field of view, never clipping a wall.
	JarModel->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::FirstPerson);
	JarModel->SetOnlyOwnerSee(true);
	JarModel->SetVisibility(false);
	JarModel->RegisterComponent();
	return JarModel;
}

void UPlayerThrowComponent::HideJarModel()
{
	if (JarModel && JarModel->IsVisible())
	{
		JarModel->SetVisibility(false);
	}
}

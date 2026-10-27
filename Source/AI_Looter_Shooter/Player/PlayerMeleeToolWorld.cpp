#include "Player/PlayerMeleeComponent.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Combat/BulletSubsystem.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Player/PlayerViewComponent.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponFX.h"
#include "Weapons/WeaponFXMelee.h"
#include "Camera/CameraComponent.h"
#include "Camera/CameraTypes.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshSocket.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "HAL/IConsoleManager.h"
#include "Math/RotationMatrix.h"

// UPlayerMeleeComponent's tool seen from outside and where it lands (round 4, 2026-10-27, "melee polish"): its mesh in the
// body's right hand in third person (the gun's socket; the gun hidden for the swing), its own hit sound and effects
// (MeleeFX), and Gauge's piston venting steam from its cylinder.

namespace
{
	TAutoConsoleVariable<float> CVarThirdPersonPitch(TEXT("Looter.Melee.ThirdPerson.Pitch"), 0.f,
		TEXT("The third-person tool turned in the hand (degrees about the hand socket's Y), for tuning against a hero's grip."));
	TAutoConsoleVariable<float> CVarThirdPersonYaw(TEXT("Looter.Melee.ThirdPerson.Yaw"), 0.f,
		TEXT("The third-person tool turned in the hand (degrees about the hand socket's Z)."));
	TAutoConsoleVariable<float> CVarThirdPersonRoll(TEXT("Looter.Melee.ThirdPerson.Roll"), 0.f,
		TEXT("The third-person tool turned in the hand (degrees about the hand socket's X)."));
	TAutoConsoleVariable<float> CVarThirdPersonWristBack(TEXT("Looter.Melee.ThirdPerson.WristBack"), 15.f,
		TEXT("A forearm tool's clamp behind the hand socket in third person (cm, full-size body)."));

	const FName GripSocket(TEXT("Grip"));
	const FName ForearmSocket(TEXT("Forearm"));
	const FName VentSocket(TEXT("Vent"));

	/**
	 * The tool's own frame in the hand socket's (the gun's: X along the barrel, Z up, the grip point at its origin). Held,
	 * like a pistol grip: the handle up (the index-and-thumb way), the knuckles and so the striking side forward, the palm
	 * onto it from the right (toward -Y). On the forearm: the punch forward, the back of the hand out to the right.
	 */
	FTransform ToolInHand(bool bForearm)
	{
		const FRotator Tune(CVarThirdPersonPitch.GetValueOnGameThread(), CVarThirdPersonYaw.GetValueOnGameThread(),
			CVarThirdPersonRoll.GetValueOnGameThread());
		if (bForearm)
		{
			const FQuat Turn = FRotationMatrix::MakeFromXZ(FVector::ForwardVector, FVector::RightVector).ToQuat();
			return FTransform(Tune.Quaternion() * Turn, FVector(-CVarThirdPersonWristBack.GetValueOnGameThread(), 0.f, 0.f));
		}
		const FQuat Turn = FRotationMatrix::MakeFromXZ(FVector::UpVector, FVector::ForwardVector).ToQuat();
		return FTransform(Tune.Quaternion() * Turn);
	}
}

void UPlayerMeleeComponent::BuildThirdPersonTool(UStaticMesh* Mesh)
{
	if (ThirdPersonTool)
	{
		ThirdPersonTool->DestroyComponent();
		ThirdPersonTool = nullptr;
	}
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	USkeletalMeshComponent* Hand = Character ? Character->GetMesh() : nullptr;
	// The basic-shape stand-in stays a first-person thing: in third person the body's jab alone, as before.
	if (!Mesh || !Hand)
	{
		return;
	}
	const UWeaponManagerComponent* Manager = Character->FindComponentByClass<UWeaponManagerComponent>();
	const FName Socket = Manager ? Manager->ThirdPersonAttachSocket : FName(TEXT("HandGrip_R"));
	const UStaticMeshSocket* Mount = Mesh->FindSocket(bForearmMesh ? ForearmSocket : GripSocket);
	const FTransform OnMesh = Mount ? FTransform(Mount->RelativeRotation, Mount->RelativeLocation) : FTransform::Identity;

	UStaticMeshComponent* Part = NewObject<UStaticMeshComponent>(Character, TEXT("MeleeToolThirdPerson"), RF_Transient);
	Part->SetStaticMesh(Mesh);
	Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Part->SetGenerateOverlapEvents(false);
	Part->SetCanEverAffectNavigation(false);
	Part->bReceivesDecals = false;
	// The body's own, as the gun in its hand is: the mesh's socket on the hand's, turned into the hand.
	Part->SetupAttachment(Hand, Socket);
	Part->SetRelativeTransform(OnMesh.Inverse() * ToolInHand(bForearmMesh));
	Part->SetVisibility(false);
	Part->RegisterComponent();
	ThirdPersonTool = Part;
}

void UPlayerMeleeComponent::HideGunForSwing(bool bHide)
{
	if (bHide)
	{
		if (AWeaponBase* Gun = SwingWeapon.Get())
		{
			Gun->SetActorHiddenInGame(true);
			HiddenGun = Gun;
		}
		return;
	}
	// Shown again only while it's still the gun in hand (one put away mid-swing is the weapon manager's to show).
	AWeaponBase* Gun = HiddenGun.Get();
	HiddenGun.Reset();
	if (Gun && Gun == GetWeaponInHand())
	{
		Gun->SetActorHiddenInGame(false);
	}
}

bool UPlayerMeleeComponent::FindToolSocket(FName Socket, FVector& OutLocation, FVector& OutForward) const
{
	const bool bFirstPerson = IsFirstPersonView();
	const UStaticMeshComponent* Part = bFirstPerson ? (ToolParts.IsEmpty() ? nullptr : ToolParts[0].Get()) : ThirdPersonTool.Get();
	if (!Part || !Part->IsVisible() || !Part->DoesSocketExist(Socket))
	{
		return false;
	}
	const FTransform Place = Part->GetSocketTransform(Socket);
	OutLocation = Place.GetLocation();
	OutForward = Place.GetRotation().GetForwardVector();
	const UCameraComponent* Camera = bFirstPerson ? UPlayerViewComponent::FindFirstPersonCamera(GetOwner()) : nullptr;
	if (!Camera)
	{
		return true;
	}
	// Drawn with the view model's field of view and pulled in toward the eye: where it shows, as the renderer puts it
	// (AWeaponBase::GetVisibleLocation does the same for a muzzle's flash).
	FMinimalViewInfo View;
	View.Location = Camera->GetComponentLocation();
	View.Rotation = Camera->GetComponentRotation();
	View.FOV = Camera->FieldOfView;
	View.FirstPersonFOV = Camera->bEnableFirstPersonFieldOfView ? Camera->FirstPersonFieldOfView : Camera->FieldOfView;
	View.FirstPersonScale = Camera->bEnableFirstPersonScale ? Camera->FirstPersonScale : 1.f;
	View.bUseFirstPersonParameters = Camera->bEnableFirstPersonFieldOfView || Camera->bEnableFirstPersonScale;
	if (View.bUseFirstPersonParameters)
	{
		OutLocation = View.TransformWorldToFirstPerson(OutLocation, false);
	}
	return true;
}

void UPlayerMeleeComponent::ShowToolImpact(const FVector& At, const FVector& Normal, const FVector& Along, EMeleeStruck Struck,
	EImpactMaterial Material, float Volume)
{
	const FName Cue = MeleeFX::HitCue(Tool.Kind);
	if (!Cue.IsNone())
	{
		LooterSound::PlayAt(this, Cue, At, Volume);
	}
	UWorld* World = GetWorld();
	UBulletSubsystem* Bullets = World ? World->GetSubsystem<UBulletSubsystem>() : nullptr;
	if (!Bullets)
	{
		return;
	}
	FWeaponFX& Effects = Bullets->GetEffects();
	Effects.Initialize(World);
	MeleeFX::Spawn(Effects, Tool.Kind, At, Normal, Along, Struck, Material, ToolRandom);
	// Gauge's piston fires on a hit and vents its cylinder: a jet of steam out of the vent's mouth.
	FVector Vent;
	FVector Out;
	if (Tool.Kind == MeleeFX::Piston() && FindToolSocket(VentSocket, Vent, Out))
	{
		MeleeFX::SpawnSteam(Effects, Vent, Out, 1.f, ToolRandom);
	}
}

// ADeliaRansom's shotgun: the user's model once it's in (TRIPO_MODELS.md 7, "an old double-barrel shotgun, worn wood
// stock": /Game/Art/Props/SM_DeliaShotgun, its sockets Grip, Foregrip and Muzzle), the Ranchhand's heritage parts standing
// in until then (worn hard: the gun master's wear). The gun hangs by its grip from the right hand's HandGrip_R, its barrel
// along the socket's +X, as the player's guns hang in third person; the left hand finds its foregrip (WardenCrewPose).

#include "Story/DeliaRansom.h"
#include "Combat/BulletSubsystem.h"
#include "Story/WardenCrewPose.h"
#include "Weapons/VfxTypes.h"
#include "Weapons/WeaponFX.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshSocket.h"
#include "Engine/World.h"
#include "Misc/PackageName.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	/** The user's shotgun, once it's in the game. */
	const TCHAR* const DeliaShotgunPath = TEXT("/Game/Art/Props/SM_DeliaShotgun.SM_DeliaShotgun");

	/** One stand-in piece: its mesh, the piece it hangs from (-1: the gun's frame) and that piece's socket. */
	struct FDeliaGunPiece
	{
		const TCHAR* Name;
		const TCHAR* Path;
		int32 Parent;
		const TCHAR* Socket;
	};
	const FDeliaGunPiece DeliaGunPieces[] = {
		{ TEXT("GunReceiver"), TEXT("/Game/Art/Weapons/SM_RanchhandBody_Heritage.SM_RanchhandBody_Heritage"), -1, nullptr },
		{ TEXT("GunBarrel"), TEXT("/Game/Art/Weapons/SM_RanchhandBarrel_Heritage.SM_RanchhandBarrel_Heritage"), 0, TEXT("Barrel") },
		{ TEXT("GunMuzzle"), TEXT("/Game/Art/Weapons/SM_RanchhandMuzzle_Crown.SM_RanchhandMuzzle_Crown"), 1, TEXT("Muzzle") },
		{ TEXT("GunStock"), TEXT("/Game/Art/Weapons/SM_RanchhandStock_Field.SM_RanchhandStock_Field"), 0, TEXT("Stock") },
		{ TEXT("GunTube"), TEXT("/Game/Art/Weapons/SM_RanchhandMagazine_Tube6.SM_RanchhandMagazine_Tube6"), 0, TEXT("Magazine") },
		{ TEXT("GunPump"), TEXT("/Game/Art/Weapons/SM_RanchhandPump_Walnut.SM_RanchhandPump_Walnut"), 0, TEXT("Pump") },
	};
	/** The gun master's wear (custom primitive data 0, UWeaponModelComponent's): a gun carried forty years. */
	constexpr int32 DeliaWearIndex = 0;
	constexpr float DeliaGunWear = 0.85f;
	/** The flash's size against a plain shot's. */
	constexpr float DeliaFlashScale = 1.4f;

	UStaticMeshComponent* MakeGunPiece(AActor& Owner, const TCHAR* Name)
	{
		UStaticMeshComponent* Piece = Owner.CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Piece->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Piece->SetGenerateOverlapEvents(false);
		Piece->SetCanEverAffectNavigation(false);
		Piece->bReceivesDecals = false;
		return Piece;
	}

	bool DeliaSocket(const UStaticMesh* Mesh, const TCHAR* Name, FVector& OutAt)
	{
		const UStaticMeshSocket* Socket = Mesh ? Mesh->FindSocket(FName(Name)) : nullptr;
		if (!Socket)
		{
			return false;
		}
		OutAt = Socket->RelativeLocation;
		return true;
	}
}

void ADeliaRansom::MakeShotgun()
{
	Shotgun = CreateDefaultSubobject<USceneComponent>(TEXT("Shotgun"));
	Shotgun->SetupAttachment(GetMesh(), FName(WardenCrewPose::GunSocket));
	ShotgunModel = MakeGunPiece(*this, TEXT("ShotgunModel"));
	ShotgunModel->SetupAttachment(Shotgun);

	for (const FDeliaGunPiece& Each : DeliaGunPieces)
	{
		UStaticMeshComponent* Piece = MakeGunPiece(*this, Each.Name);
		UStaticMesh* PieceMesh = nullptr;
		if (FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(FString(Each.Path))))
		{
			ConstructorHelpers::FObjectFinder<UStaticMesh> Finder(Each.Path);
			PieceMesh = Finder.Object;
		}
		Piece->SetStaticMesh(PieceMesh);
		Piece->SetCustomPrimitiveDataFloat(DeliaWearIndex, DeliaGunWear);
		USceneComponent* Parent = Each.Parent >= 0 && ShotgunParts.IsValidIndex(Each.Parent) ? ShotgunParts[Each.Parent].Get() : Shotgun.Get();
		Piece->SetupAttachment(Parent, Each.Socket ? FName(Each.Socket) : FName());
		ShotgunParts.Add(Piece);
	}

	// The receiver hangs so its grip is the frame's origin (the hand's socket); the foregrip, from it, is where the left hand goes.
	const UStaticMesh* Receiver = ShotgunParts.Num() > 0 ? ShotgunParts[0]->GetStaticMesh().Get() : nullptr;
	FVector Grip = FVector::ZeroVector;
	FVector Foregrip = FVector::ZeroVector;
	if (DeliaSocket(Receiver, TEXT("Grip"), Grip))
	{
		ShotgunParts[0]->SetRelativeLocation(-Grip);
		if (DeliaSocket(Receiver, TEXT("Foregrip"), Foregrip))
		{
			Forend = Foregrip - Grip;
		}
	}
}

void ADeliaRansom::WearShotgun()
{
	// The user's shotgun when it's in: one mesh by its grip, the stand-in's pieces hidden.
	UStaticMesh* Own = nullptr;
	if (FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(FString(DeliaShotgunPath))))
	{
		Own = LoadObject<UStaticMesh>(nullptr, DeliaShotgunPath, nullptr, LOAD_NoWarn | LOAD_Quiet);
	}
	if (Own && ShotgunModel)
	{
		ShotgunModel->SetStaticMesh(Own);
		FVector Grip = FVector::ZeroVector;
		FVector Foregrip = FVector::ZeroVector;
		FVector Muzzle = FVector::ZeroVector;
		DeliaSocket(Own, TEXT("Grip"), Grip);
		ShotgunModel->SetRelativeLocation(-Grip);
		if (DeliaSocket(Own, TEXT("Foregrip"), Foregrip))
		{
			Forend = Foregrip - Grip;
		}
		if (DeliaSocket(Own, TEXT("Muzzle"), Muzzle))
		{
			MuzzleFromGrip = Muzzle - Grip;
		}
		for (UStaticMeshComponent* Piece : ShotgunParts)
		{
			Piece->SetVisibility(false);
		}
		return;
	}
	// The stand-in's muzzle: the crown's tip, else the barrel's, in the frame's space.
	for (const int32 Index : { 2, 1 })
	{
		const UStaticMeshComponent* Piece = ShotgunParts.IsValidIndex(Index) ? ShotgunParts[Index].Get() : nullptr;
		if (Piece && Shotgun && Piece->GetStaticMesh() && Piece->DoesSocketExist(TEXT("Muzzle")))
		{
			MuzzleFromGrip = Shotgun->GetComponentTransform().InverseTransformPosition(Piece->GetSocketLocation(TEXT("Muzzle")));
			break;
		}
	}
}

FVector ADeliaRansom::GetMuzzle() const
{
	return Shotgun ? Shotgun->GetComponentTransform().TransformPosition(MuzzleFromGrip) : GetActorLocation();
}

void ADeliaRansom::ShowMuzzle(const FVector& Muzzle, const FVector& Way)
{
	UBulletSubsystem* Bullets = GetWorld() ? GetWorld()->GetSubsystem<UBulletSubsystem>() : nullptr;
	if (!Bullets)
	{
		return;
	}
	// A shot in the open with no gun model of the game's to wear the flash: the scene gunfire's flash, and the shotgun's
	// smoke and sparks round it.
	FWeaponFX& Effects = Bullets->GetEffects();
	Effects.SpawnFlash(Muzzle, Way, DeliaFlashScale);
	FVfxMuzzle Burst;
	Burst.Location = Muzzle;
	Burst.Forward = Way;
	Burst.Up = FVector::UpVector;
	Burst.Kind = EWeaponKind::Shotgun;
	Burst.Scale = DeliaFlashScale;
	Effects.SpawnMuzzleBurst(Burst);
}

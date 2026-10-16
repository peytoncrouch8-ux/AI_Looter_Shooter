// AColdOpenCast: building the cold open's props and moving them beat by beat (the figures and Sexton are
// ColdOpenCastFigures.cpp's).

#include "Scenes/ColdOpenCast.h"
#include "AI_Looter_Shooter.h"
#include "Combat/BulletSubsystem.h"
#include "Scenes/ColdOpenSet.h"
#include "Weapons/WeaponFX.h"
#include "World/WorldQueries.h"
#include "Animation/AnimationAsset.h"
#include "CollisionQueryParams.h"
#include "Components/PointLightComponent.h"
#include "Components/PoseableMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/HitResult.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshSocket.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Misc/PackageName.h"
#include "Sound/SoundBase.h"

namespace
{
	/** A shot's flash lights the deck this brightly for this long (candelas, seconds). */
	constexpr float FlashCandelas = 90.f;
	constexpr float FlashSeconds = 0.12f;

	/** Abel's run: the jog played faster for a sprint up the path (its own pace is about 3.75 m/s). */
	constexpr float JogPace = 375.f;
	constexpr float RunSeconds = 3.f;

	/** His lantern rolls this far off the path, and drops this far down the slope, before it goes out (cm). */
	constexpr float LanternRoll = 480.f;
	constexpr float LanternDrop = 140.f;

	/** The mannequin faces its mesh's +Y: a figure facing a yaw turns its mesh this much less. */
	constexpr float MeshYaw = -90.f;

	/** Ned's gun hand, the Deacon's, and the hand the ember's in (Abel's lantern too). */
	const FName GunHand(TEXT("hand_r"));
	const FName LeftHand(TEXT("hand_l"));
	const FName HeadBone(TEXT("head"));
	const FName SpineBone(TEXT("spine_03"));

	/** The skiff's Deck socket (Art/Models/Vehicles/Skiff.py): every paint has it. */
	const FName DeckSocket(TEXT("Deck"));

	/** A model the set names, when its package is in this checkout (a model being imported may not be yet). */
	template <typename T>
	T* LoadIfMade(const TSoftObjectPtr<T>& Asset)
	{
		if (Asset.IsNull())
		{
			return nullptr;
		}
		if (T* Loaded = Asset.Get())
		{
			return Loaded;
		}
		return FPackageName::DoesPackageExist(Asset.ToSoftObjectPath().GetLongPackageName()) ? Asset.LoadSynchronous() : nullptr;
	}

	/** The ground under Where (the Point's top), or Where's own height when the trace finds none. */
	FVector OnGround(const UWorld* World, const FVector& Where)
	{
		FHitResult Hit;
		if (World && World->LineTraceSingleByObjectType(Hit, Where + FVector(0.0, 0.0, 400.0), Where - FVector(0.0, 0.0, 600.0),
			FCollisionObjectQueryParams(ECC_WorldStatic), LooterWorld::StaticGeometryParams(World, TEXT("ColdOpenGround"))))
		{
			return Hit.ImpactPoint;
		}
		return Where;
	}
}

AColdOpenCast::AColdOpenCast()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	// After the camera has its place for the frame, so the glows face where it is now.
	PrimaryActorTick.TickGroup = TG_PostUpdateWork;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;
}

AColdOpenCast* AColdOpenCast::Spawn(const AColdOpenSet& Set)
{
	UWorld* World = Set.GetWorld();
	if (!World)
	{
		return nullptr;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.ObjectFlags |= RF_Transient;
	AColdOpenCast* Props = World->SpawnActor<AColdOpenCast>(AColdOpenCast::StaticClass(), FTransform::Identity, Params);
	if (Props)
	{
		Props->Build(Set);
	}
	return Props;
}

void AColdOpenCast::Build(const AColdOpenSet& Set)
{
	SetActor = &Set;
	SilhouetteBase = LoadIfMade(Set.SilhouetteMaterial);
	SilhouetteColor = Set.SilhouetteColor;
	EyeHeight = Set.EyeHeight;
	if (!SilhouetteBase)
	{
		UE_LOG(LogLooter, Warning, TEXT("Cold open: no silhouette material (%s), so the gang shows in the mannequin's own colors."),
			*Set.SilhouetteMaterial.ToString());
	}

	BuildGlows(Set);
	BuildSkiff(Set);
	BuildPoint(Set);
	BuildSexton(Set);
	ShowSkiff(false);
	ShowPoint(false);
	ShowSexton(false);
	UpdateGlows();
}

void AColdOpenCast::BuildSkiff(const AColdOpenSet& Set)
{
	UStaticMesh* Model = LoadIfMade(Set.SkiffMesh);
	Skiff = NewObject<UStaticMeshComponent>(this, TEXT("Skiff"), RF_Transient);
	Skiff->SetupAttachment(Root);
	Skiff->SetMobility(EComponentMobility::Movable);
	Skiff->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Skiff->SetStaticMesh(Model);
	Skiff->RegisterComponent();
	if (!Model)
	{
		UE_LOG(LogLooter, Warning, TEXT("Cold open: no gang's skiff (%s); its crew stands on nothing."), *Set.SkiffMesh.ToString());
	}
	else if (const UStaticMeshSocket* Deck = Model->FindSocket(DeckSocket))
	{
		DeckLocal = Deck->RelativeLocation;
	}
	SkiffCrew.Reset();
	for (const FTransform& Spot : Set.SkiffCrew)
	{
		if (UPoseableMeshComponent* Figure = MakeFigure(Skiff, Spot))
		{
			Outfit(*Figure, SkiffCrew.Num());
			SkiffCrew.Add(Figure);
		}
	}
}

void AColdOpenCast::BuildPoint(const AColdOpenSet& Set)
{
	UWorld* World = GetWorld();
	Gang.Reset();
	GangGuns.Reset();
	for (const FTransform& Spot : Set.Gang)
	{
		if (UPoseableMeshComponent* Figure = MakeFigure(Root, Set.ToWorld(Spot)))
		{
			GangGuns.Add(Outfit(*Figure, Gang.Num()));
			Gang.Add(Figure);
		}
	}
	if (Gang.IsValidIndex(AColdOpenSet::DeaconIndex))
	{
		PoseHoldingEmber(*Gang[AColdOpenSet::DeaconIndex]);
	}

	// Abel, running up the bluff path with his lantern and shotgun: the mannequin's jog, faster, drawn black.
	AbelFrom = OnGround(World, Set.ToWorld(Set.AbelFrom));
	AbelTo = OnGround(World, Set.ToWorld(Set.AbelTo));
	const FVector Run = AbelTo - AbelFrom;
	AbelYaw = FVector(Run.X, Run.Y, 0.0).IsNearlyZero() ? 0.f : FVector(Run.X, Run.Y, 0.0).Rotation().Yaw;
	Abel = NewObject<USkeletalMeshComponent>(this, TEXT("Abel"), RF_Transient);
	Abel->SetupAttachment(Root);
	Abel->SetMobility(EComponentMobility::Movable);
	Abel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Abel->SetCastShadow(false);
	Abel->SetSkeletalMeshAsset(LoadIfMade(Set.FigureMesh));
	Abel->RegisterComponent();
	Abel->SetWorldLocationAndRotation(AbelFrom, FRotator(0.0, AbelYaw + MeshYaw, 0.0));
	PaintBlack(*Abel);
	if (UAnimationAsset* Running = LoadIfMade(Set.RunAnimation))
	{
		Abel->PlayAnimation(Running, /*bLooping*/ true);
		Abel->SetPlayRate(FMath::Clamp(static_cast<float>(Run.Size2D()) / RunSeconds / JogPace, 1.f, 2.2f));
	}
	Abel->SetVisibility(false);
}

// ---------------------------------------------------------------------------
// The skiff
// ---------------------------------------------------------------------------

void AColdOpenCast::ShowSkiff(bool bShow)
{
	bSkiffShown = bShow;
	if (Skiff)
	{
		// The crew rides on it: shown and hidden with it.
		Skiff->SetVisibility(bShow, /*bPropagateToChildren*/ true);
	}
	if (bShow)
	{
		ShowPoint(false);
	}
}

void AColdOpenCast::PlaceSkiff(const FTransform& Pose)
{
	if (Skiff)
	{
		Skiff->SetWorldTransform(Pose, /*bSweep*/ false, nullptr, ETeleportType::TeleportPhysics);
	}
}

FVector AColdOpenCast::GetDeckEye() const
{
	return Skiff ? Skiff->GetComponentTransform().TransformPosition(DeckLocal + FVector(0.0, 0.0, EyeHeight)) : GetActorLocation();
}

FVector AColdOpenCast::GetCrewHead(int32 Index) const
{
	return SkiffCrew.IsValidIndex(Index) ? BoneInWorld(SkiffCrew[Index], HeadBone) : GetDeckEye();
}

// ---------------------------------------------------------------------------
// The Point
// ---------------------------------------------------------------------------

void AColdOpenCast::ShowPoint(bool bShow)
{
	bPointShown = bShow;
	for (UPoseableMeshComponent* Figure : Gang)
	{
		if (Figure)
		{
			Figure->SetVisibility(bShow);
		}
	}
	if (!bShow)
	{
		bAbelShown = false;
		if (Abel)
		{
			Abel->SetVisibility(false);
		}
	}
	else if (bSkiffShown)
	{
		ShowSkiff(false);
	}
	UpdateGlows();
}

void AColdOpenCast::RunAbel(float Alpha)
{
	if (!Abel || bLanternLoose)
	{
		return;
	}
	bAbelShown = bPointShown;
	Abel->SetVisibility(bAbelShown);
	Abel->SetWorldLocation(FMath::Lerp(AbelFrom, AbelTo, FMath::Clamp(Alpha, 0.f, 1.f)));
	UpdateGlows();
}

void AColdOpenCast::DropAbel(float Alpha)
{
	if (!Abel)
	{
		return;
	}
	// The run stops where the shot found him, and he pitches forward onto the path, face down, from his feet.
	Abel->bPauseAnims = true;
	if (!bLanternLoose)
	{
		bLanternLoose = true;
		LanternFrom = BoneInWorld(Abel, LeftHand);
		LanternAt = LanternFrom;
	}
	const float Eased = FMath::Square(FMath::Clamp(Alpha, 0.f, 1.f));
	const FQuat Facing = FRotator(0.0, AbelYaw + MeshYaw, 0.0).Quaternion();
	// The mannequin's forward is its mesh's +Y: turning about its X tips its head toward +Y.
	const FQuat Pitched(FVector::XAxisVector, -UE_HALF_PI * Eased);
	Abel->SetWorldRotation(Facing * Pitched);
	UpdateGlows();
}

void AColdOpenCast::RollLantern(float Alpha)
{
	if (!bLanternLoose)
	{
		return;
	}
	// Off the path to Abel's right and down the slope, slowing as it goes, and its flame dying.
	const float Out = 1.f - FMath::Square(1.f - FMath::Clamp(Alpha, 0.f, 1.f));
	const FVector Side = FRotator(0.0, AbelYaw + 90.0, 0.0).Vector();
	LanternAt = LanternFrom + Side * (LanternRoll * Out) - FVector(0.0, 0.0, LanternDrop * Out + 60.f * FMath::Min(Alpha * 4.f, 1.f));
	LanternGlow = 1.f - FMath::SmoothStep(0.45f, 1.f, Alpha);
	UpdateGlows();
}

void AColdOpenCast::Aim(int32 GangIndex, const FVector& Target)
{
	if (Gang.IsValidIndex(GangIndex) && Gang[GangIndex])
	{
		PoseAim(*Gang[GangIndex], Target);
		HoldGunToAim(GangIndex);
	}
}

void AColdOpenCast::Fire(int32 GangIndex, const FVector& Target)
{
	if (!Gang.IsValidIndex(GangIndex) || !Gang[GangIndex])
	{
		return;
	}
	const FVector Hand = BoneInWorld(Gang[GangIndex], GunHand);
	const FVector Along = (Target - Hand).GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector);
	// The gun's own muzzle (its prop's Muzzle socket), else just past the hand, where a pistol's would be.
	const FVector Muzzle = GunMuzzle(GangIndex, Hand + Along * 22.f);
	UWorld* World = GetWorld();
	if (UBulletSubsystem* Bullets = World ? World->GetSubsystem<UBulletSubsystem>() : nullptr)
	{
		FWeaponFX& Effects = Bullets->GetEffects();
		Effects.Initialize(World);
		Effects.SpawnFlash(Muzzle, Along, 1.4f);
	}
	if (FlashLight)
	{
		FlashLight->SetWorldLocation(Muzzle);
		FlashLight->SetIntensity(FlashCandelas);
		FlashLight->SetVisibility(true);
		FlashLeft = FlashSeconds;
	}
	const AColdOpenSet* Set = SetActor.Get();
	if (Set && Set->ShotSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, Set->ShotSound, Muzzle);
	}
}

void AColdOpenCast::ShowSexton(bool bShow)
{
	bSextonShown = bShow;
	if (SextonSeat)
	{
		SextonSeat->SetVisibility(bShow, /*bPropagateToChildren*/ true);
	}
}

FVector AColdOpenCast::GetHead(int32 GangIndex) const
{
	return Gang.IsValidIndex(GangIndex) ? BoneInWorld(Gang[GangIndex], HeadBone) : GetActorLocation();
}

FVector AColdOpenCast::GetAbelChest() const
{
	return Abel ? BoneInWorld(Abel, SpineBone) : AbelTo;
}

FVector AColdOpenCast::GetSextonHead() const
{
	// His head is about 95 cm over the seat he sits on (the rail's top).
	return SextonSeat ? SextonSeat->GetComponentTransform().TransformPosition(FVector(8.0, 0.0, 95.0)) : GetActorLocation();
}

// ---------------------------------------------------------------------------
// Every frame
// ---------------------------------------------------------------------------

void AColdOpenCast::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	// The cast's own clock: the ember's spark runs on it, so it keeps the scene's pace whatever the world's time does.
	GlowClock += DeltaSeconds;
	if (FlashLeft > 0.f && FlashLight)
	{
		FlashLeft = FMath::Max(FlashLeft - DeltaSeconds, 0.f);
		FlashLight->SetIntensity(FlashCandelas * FlashLeft / FlashSeconds);
		FlashLight->SetVisibility(FlashLeft > 0.f);
	}
	UpdateGlows();
}

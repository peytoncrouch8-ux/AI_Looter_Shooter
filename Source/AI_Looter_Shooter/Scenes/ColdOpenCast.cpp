// AColdOpenCast: building the cold open's props and moving them beat by beat (the figures and Sexton are
// ColdOpenCastFigures.cpp's).

#include "Scenes/ColdOpenCast.h"
#include "AI_Looter_Shooter.h"
#include "Combat/BulletSubsystem.h"
#include "Scenes/ColdOpenSet.h"
#include "Weapons/WeaponFX.h"
#include "World/WorldQueries.h"
#include "Animation/AnimationAsset.h"
#include "Camera/PlayerCameraManager.h"
#include "CollisionQueryParams.h"
#include "Components/InstancedStaticMeshComponent.h"
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
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Math/RotationMatrix.h"
#include "Misc/PackageName.h"
#include "Sound/SoundBase.h"

namespace
{
	/** The glows: the game's additive glow on camera-facing quads (R, G, B, strength, shape 1 = round). */
	const TCHAR* GlowMaterialPath = TEXT("/Game/Weapons/FX/M_FX_Glow.M_FX_Glow");
	const TCHAR* QuadPath = TEXT("/Engine/BasicShapes/Plane.Plane");
	constexpr int32 GlowFloats = 5;
	constexpr int32 EmberGlow = 0;
	constexpr int32 LanternGlowIndex = 1;

	/** Saint Ada's ember: a burning orange light in a gloved hand. Abel's lantern: warm, and smaller in the dark. */
	const FLinearColor EmberColor(1.f, 0.42f, 0.08f);
	constexpr float EmberSize = 22.f;
	constexpr float EmberStrength = 32.f;
	constexpr float EmberCandelas = 14.f;
	const FLinearColor LanternColor(1.f, 0.72f, 0.38f);
	constexpr float LanternSize = 30.f;
	constexpr float LanternStrength = 26.f;
	constexpr float LanternCandelas = 22.f;

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

	/** Ned's gun hand, the Deacon's, and the hand the ember's in. */
	const FName GunHand(TEXT("hand_r"));
	const FName EmberHand(TEXT("hand_l"));
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

	/** Where the camera is, for the glows to face: the player's, or a point in front of Fallback. */
	FVector Viewer(const UWorld* World, const FVector& Fallback)
	{
		const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
		return Controller && Controller->PlayerCameraManager ? Controller->PlayerCameraManager->GetCameraLocation()
			: Fallback + FVector(500.0, 0.0, 0.0);
	}

	UPointLightComponent* MakeLight(AActor& Owner, USceneComponent* Parent, const FLinearColor& Color, float Radius)
	{
		UPointLightComponent* Light = NewObject<UPointLightComponent>(&Owner, NAME_None, RF_Transient);
		Light->SetupAttachment(Parent);
		Light->SetMobility(EComponentMobility::Movable);
		Light->SetIntensityUnits(ELightUnits::Candelas);
		Light->SetIntensity(0.f);
		Light->SetLightColor(Color);
		Light->SetAttenuationRadius(Radius);
		// Lanterns and flashes light what's near them; at dusk nothing they could shadow reads anyway.
		Light->SetCastShadows(false);
		Light->SetVisibility(false);
		Light->RegisterComponent();
		return Light;
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

	// The ember and the lantern: round glows facing the camera, each with a little light of its own.
	Glows = NewObject<UInstancedStaticMeshComponent>(this, TEXT("Glows"), RF_Transient);
	Glows->SetupAttachment(Root);
	Glows->SetMobility(EComponentMobility::Movable);
	Glows->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Glows->SetCastShadow(false);
	Glows->bReceivesDecals = false;
	Glows->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, QuadPath));
	Glows->SetMaterial(0, LoadObject<UMaterialInterface>(nullptr, GlowMaterialPath));
	Glows->SetNumCustomDataFloats(GlowFloats);
	Glows->RegisterComponent();
	for (int32 Index = 0; Index < 2; ++Index)
	{
		Glows->AddInstance(FTransform(FQuat::Identity, Set.GetActorLocation(), FVector(0.01)), /*bWorldSpace*/ true);
	}
	EmberLight = MakeLight(*this, Root, EmberColor, 380.f);
	LanternLight = MakeLight(*this, Root, LanternColor, 700.f);
	FlashLight = MakeLight(*this, Root, FLinearColor(1.f, 0.78f, 0.5f), 1600.f);

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
			SkiffCrew.Add(Figure);
		}
	}
}

void AColdOpenCast::BuildPoint(const AColdOpenSet& Set)
{
	UWorld* World = GetWorld();
	Gang.Reset();
	for (const FTransform& Spot : Set.Gang)
	{
		if (UPoseableMeshComponent* Figure = MakeFigure(Root, Set.ToWorld(Spot)))
		{
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
		LanternFrom = BoneInWorld(Abel, EmberHand);
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
	// Just past the hand, where a pistol's muzzle would be.
	const FVector Muzzle = Hand + Along * 22.f;
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
	if (FlashLeft > 0.f && FlashLight)
	{
		FlashLeft = FMath::Max(FlashLeft - DeltaSeconds, 0.f);
		FlashLight->SetIntensity(FlashCandelas * FlashLeft / FlashSeconds);
		FlashLight->SetVisibility(FlashLeft > 0.f);
	}
	UpdateGlows();
}

void AColdOpenCast::UpdateGlows()
{
	if (!Glows || Glows->GetInstanceCount() < 2)
	{
		return;
	}
	// The ember in the Deacon's hand while the Point shows; the lantern in Abel's, then rolling off with its flame dying.
	const bool bEmber = bPointShown && Gang.IsValidIndex(AColdOpenSet::DeaconIndex);
	const FVector Ember = bEmber ? BoneInWorld(Gang[AColdOpenSet::DeaconIndex], EmberHand) + FVector(0.0, 0.0, 6.0) : GetActorLocation();
	const bool bLantern = bPointShown && (bAbelShown || bLanternLoose) && (!bLanternLoose || LanternGlow > 0.f);
	const FVector Lantern = bLanternLoose ? LanternAt : (Abel ? BoneInWorld(Abel, EmberHand) - FVector(0.0, 0.0, 10.0) : GetActorLocation());
	const float LanternStrengthNow = bLantern ? (bLanternLoose ? LanternGlow : 1.f) : 0.f;

	const FVector Camera = Viewer(GetWorld(), Ember);
	auto Facing = [&Camera](const FVector& Where, float Size)
	{
		FVector ToCamera = Camera - Where;
		if (!ToCamera.Normalize())
		{
			ToCamera = FVector::UpVector;
		}
		// The engine's quad lies in XY facing +Z, 100 across.
		return FTransform(FRotationMatrix::MakeFromZ(ToCamera).ToQuat(), Where, FVector(Size / 100.0, Size / 100.0, 1.0));
	};
	Glows->UpdateInstanceTransform(EmberGlow, Facing(Ember, bEmber ? EmberSize : 0.01f), /*bWorldSpace*/ true, false, /*bTeleport*/ true);
	Glows->UpdateInstanceTransform(LanternGlowIndex, Facing(Lantern, LanternStrengthNow > 0.f ? LanternSize : 0.01f), true, false, true);
	const float EmberData[] = { EmberColor.R, EmberColor.G, EmberColor.B, bEmber ? EmberStrength : 0.f, 1.f };
	const float LanternData[] = { LanternColor.R, LanternColor.G, LanternColor.B, LanternStrength * LanternStrengthNow, 1.f };
	Glows->SetCustomData(EmberGlow, MakeArrayView(EmberData), false);
	Glows->SetCustomData(LanternGlowIndex, MakeArrayView(LanternData), false);
	Glows->MarkRenderStateDirty();

	if (EmberLight)
	{
		EmberLight->SetWorldLocation(Ember);
		EmberLight->SetIntensity(bEmber ? EmberCandelas : 0.f);
		EmberLight->SetVisibility(bEmber);
	}
	if (LanternLight)
	{
		LanternLight->SetWorldLocation(Lantern);
		LanternLight->SetIntensity(LanternCandelas * LanternStrengthNow);
		LanternLight->SetVisibility(LanternStrengthNow > 0.f);
	}
}

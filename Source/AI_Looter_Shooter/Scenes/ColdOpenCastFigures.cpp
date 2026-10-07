// AColdOpenCast: the gang as black mannequins posed by code, and Mister Sexton on the far rail.

#include "Scenes/ColdOpenCast.h"
#include "AI_Looter_Shooter.h"
#include "Scenes/ColdOpenSet.h"
#include "Components/PoseableMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SkinnedMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/PackageName.h"

namespace
{
	/** The mannequin faces its mesh's +Y: a figure facing along its spot's +X turns its mesh this much. */
	constexpr float MeshYaw = -90.f;

	/** The silhouette master's parameters (M_Backdrop: Tint times Brightness, unlit). */
	const FName TintParameter(TEXT("Tint"));
	const FName BrightnessParameter(TEXT("Brightness"));

	/** The lookout's seat for Sexton, his model's ledger socket (Art/Models/Buildings/Lookout.py, Characters/MisterSexton.py). */
	const FName SitSocket(TEXT("Sit"));
	const FName LedgerSocket(TEXT("Ledger"));

	/** How far from the set the lookout's Sit socket may be and still be this lookout's (cm). */
	constexpr double SeatSearchRadius = 3000.0;

	// The mannequin's bones (UE5 Manny), left then right.
	const FName UpperArm[2] = { FName(TEXT("upperarm_l")), FName(TEXT("upperarm_r")) };
	const FName LowerArm[2] = { FName(TEXT("lowerarm_l")), FName(TEXT("lowerarm_r")) };
	const FName Hand[2] = { FName(TEXT("hand_l")), FName(TEXT("hand_r")) };
	const FName Foot[2] = { FName(TEXT("foot_l")), FName(TEXT("foot_r")) };
	const FName Ball[2] = { FName(TEXT("ball_l")), FName(TEXT("ball_r")) };
	constexpr int32 Left = 0;
	constexpr int32 Right = 1;

	/** A model the set names, when its package is in this checkout (Sexton's was being imported as this was written). */
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

	bool HasBone(const USkinnedMeshComponent& Figure, FName Bone)
	{
		return Figure.GetBoneIndex(Bone) != INDEX_NONE;
	}

	FVector BoneCS(UPoseableMeshComponent& Figure, FName Bone)
	{
		return Figure.GetBoneLocationByName(Bone, EBoneSpaces::ComponentSpace);
	}

	/**
	 * Turns Bone (in the figure's own space) so the line from it to Child points along Direction, whichever way the bone's
	 * own axes happen to run: the figure's children follow it.
	 */
	void AimBone(UPoseableMeshComponent& Figure, FName Bone, FName Child, const FVector& Direction)
	{
		if (!HasBone(Figure, Bone) || !HasBone(Figure, Child))
		{
			return;
		}
		const FTransform BoneTransform = Figure.GetBoneTransformByName(Bone, EBoneSpaces::ComponentSpace);
		const FVector Now = (BoneCS(Figure, Child) - BoneTransform.GetLocation()).GetSafeNormal();
		const FVector Wanted = Direction.GetSafeNormal();
		if (Now.IsNearlyZero() || Wanted.IsNearlyZero())
		{
			return;
		}
		const FQuat Turn = FQuat::FindBetweenNormals(Now, Wanted);
		Figure.SetBoneRotationByName(Bone, (Turn * BoneTransform.GetRotation()).Rotator(), EBoneSpaces::ComponentSpace);
	}

	/** The figure's own directions, in its mesh's space: forward (heels to toes), its left, and up. */
	struct FBodyAxes
	{
		FVector Forward = FVector(0.0, 1.0, 0.0);
		FVector Leftward = FVector(1.0, 0.0, 0.0);
		FVector Up = FVector::UpVector;
	};

	FBodyAxes AxesOf(UPoseableMeshComponent& Figure)
	{
		FBodyAxes Axes;
		if (HasBone(Figure, UpperArm[Left]) && HasBone(Figure, UpperArm[Right]))
		{
			FVector Across = BoneCS(Figure, UpperArm[Left]) - BoneCS(Figure, UpperArm[Right]);
			Across.Z = 0.0;
			Axes.Leftward = Across.GetSafeNormal(UE_SMALL_NUMBER, Axes.Leftward);
		}
		FVector Toes = FVector::ZeroVector;
		for (int32 Side = 0; Side < 2; ++Side)
		{
			if (HasBone(Figure, Foot[Side]) && HasBone(Figure, Ball[Side]))
			{
				Toes += BoneCS(Figure, Ball[Side]) - BoneCS(Figure, Foot[Side]);
			}
		}
		Toes.Z = 0.0;
		Toes -= Axes.Leftward * FVector::DotProduct(Toes, Axes.Leftward);
		Axes.Forward = Toes.GetSafeNormal(UE_SMALL_NUMBER, Axes.Forward);
		return Axes;
	}

	/** A plain shape of the engine's for Sexton's stand-in, in his seat's frame (the engine's shapes are 100 cm across). */
	UStaticMeshComponent* AddShape(AActor& Owner, USceneComponent* Seat, const TCHAR* Shape, const FVector& Where, const FRotator& Turn,
		const FVector& Size)
	{
		UStaticMeshComponent* Part = NewObject<UStaticMeshComponent>(&Owner, NAME_None, RF_Transient);
		Part->SetupAttachment(Seat);
		Part->SetMobility(EComponentMobility::Movable);
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->SetCastShadow(false);
		Part->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, Shape));
		Part->SetRelativeLocationAndRotation(Where, Turn);
		Part->SetRelativeScale3D(Size / 100.0);
		Part->RegisterComponent();
		return Part;
	}
}

UMaterialInterface* AColdOpenCast::GetSilhouette()
{
	if (!Silhouette && SilhouetteBase)
	{
		// Unlit, so it's as black at dusk against the sunset as anywhere: a silhouette, not a dark figure lit by the sun.
		Silhouette = UMaterialInstanceDynamic::Create(SilhouetteBase, this);
		Silhouette->SetVectorParameterValue(TintParameter, SilhouetteColor);
		Silhouette->SetScalarParameterValue(BrightnessParameter, 1.f);
	}
	return Silhouette;
}

void AColdOpenCast::PaintBlack(UMeshComponent& Mesh)
{
	UMaterialInterface* Black = GetSilhouette();
	if (!Black)
	{
		return;
	}
	for (int32 Slot = 0; Slot < Mesh.GetNumMaterials(); ++Slot)
	{
		Mesh.SetMaterial(Slot, Black);
	}
}

UPoseableMeshComponent* AColdOpenCast::MakeFigure(USceneComponent* Parent, const FTransform& Feet)
{
	const AColdOpenSet* Set = SetActor.Get();
	USkeletalMesh* Model = Set ? LoadIfMade(Set->FigureMesh) : nullptr;
	if (!Model || !Parent)
	{
		return nullptr;
	}
	UPoseableMeshComponent* Figure = NewObject<UPoseableMeshComponent>(this, NAME_None, RF_Transient);
	Figure->SetupAttachment(Parent);
	Figure->SetMobility(EComponentMobility::Movable);
	Figure->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	// Black on black: a shadow would add nothing the silhouette doesn't, and fourteen of them cost.
	Figure->SetCastShadow(false);
	Figure->RegisterComponent();
	Figure->SetSkinnedAssetAndUpdate(Model);
	// Standing on the spot, facing along its +X: the mesh turned in its own frame first, then as the spot faces.
	const FTransform Placed(Feet.GetRotation() * FRotator(0.0, MeshYaw, 0.0).Quaternion(), Feet.GetLocation());
	if (Parent == Root)
	{
		Figure->SetWorldTransform(Placed);
	}
	else
	{
		Figure->SetRelativeTransform(Placed);
	}
	PaintBlack(*Figure);
	PoseAtEase(*Figure);
	return Figure;
}

void AColdOpenCast::PoseAtEase(UPoseableMeshComponent& Figure)
{
	// From the mannequin's A pose: the arms hang a little out from the sides, the forearms a little forward.
	const FBodyAxes Axes = AxesOf(Figure);
	for (int32 Side = 0; Side < 2; ++Side)
	{
		const double Out = Side == Left ? 1.0 : -1.0;
		AimBone(Figure, UpperArm[Side], LowerArm[Side], -Axes.Up + Axes.Leftward * (0.16 * Out) + Axes.Forward * 0.05);
		AimBone(Figure, LowerArm[Side], Hand[Side], -Axes.Up + Axes.Forward * 0.22 + Axes.Leftward * (0.04 * Out));
	}
	Figure.RefreshBoneTransforms();
}

void AColdOpenCast::PoseAim(UPoseableMeshComponent& Figure, const FVector& Target)
{
	// The gun arm straight out from the shoulder at the target, as a pistol's held.
	if (!HasBone(Figure, UpperArm[Right]))
	{
		return;
	}
	const FVector TargetHere = Figure.GetComponentTransform().InverseTransformPosition(Target);
	const FVector Along = TargetHere - BoneCS(Figure, UpperArm[Right]);
	AimBone(Figure, UpperArm[Right], LowerArm[Right], Along);
	AimBone(Figure, LowerArm[Right], Hand[Right], Along);
	Figure.RefreshBoneTransforms();
}

void AColdOpenCast::PoseHoldingEmber(UPoseableMeshComponent& Figure)
{
	// The left hand held up before the chest, the forearm across toward the middle: the ember in the gloved hand.
	const FBodyAxes Axes = AxesOf(Figure);
	AimBone(Figure, UpperArm[Left], LowerArm[Left], -Axes.Up * 0.75 + Axes.Forward * 0.55 + Axes.Leftward * 0.12);
	AimBone(Figure, LowerArm[Left], Hand[Left], Axes.Forward * 0.85 + Axes.Up * 0.35 - Axes.Leftward * 0.3);
	Figure.RefreshBoneTransforms();
}

FVector AColdOpenCast::BoneInWorld(USceneComponent* Figure, FName Bone)
{
	if (UPoseableMeshComponent* Posed = Cast<UPoseableMeshComponent>(Figure))
	{
		if (HasBone(*Posed, Bone))
		{
			return Posed->GetBoneTransformByName(Bone, EBoneSpaces::WorldSpace).GetLocation();
		}
	}
	else if (const USkinnedMeshComponent* Animated = Cast<USkinnedMeshComponent>(Figure))
	{
		if (HasBone(*Animated, Bone))
		{
			return Animated->GetSocketLocation(Bone);
		}
	}
	// No bone to ask: about chest height over the feet.
	return Figure ? Figure->GetComponentLocation() + FVector(0.0, 0.0, 130.0) : FVector::ZeroVector;
}

// ---------------------------------------------------------------------------
// Mister Sexton
// ---------------------------------------------------------------------------

void AColdOpenCast::BuildSexton(const AColdOpenSet& Set)
{
	// His seat: the lookout's Sit socket, on its front rail's top facing into the deck, as his model attaches "snapped to
	// target"; without it (a lookout from before the socket), the set's own seat.
	FTransform Seat = Set.ToWorld(Set.SextonSeat);
	double Nearest = SeatSearchRadius * SeatSearchRadius;
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		TInlineComponentArray<UStaticMeshComponent*> Meshes(*It);
		for (const UStaticMeshComponent* Mesh : Meshes)
		{
			if (!Mesh || Mesh->GetOwner() == this || !Mesh->DoesSocketExist(SitSocket))
			{
				continue;
			}
			const FTransform Socket = Mesh->GetSocketTransform(SitSocket);
			const double Distance = FVector::DistSquared(Socket.GetLocation(), Set.GetActorLocation());
			if (Distance < Nearest)
			{
				Nearest = Distance;
				Seat = FTransform(Socket.GetRotation(), Socket.GetLocation());
			}
		}
	}

	SextonSeat = NewObject<USceneComponent>(this, TEXT("SextonSeat"), RF_Transient);
	SextonSeat->SetupAttachment(Root);
	SextonSeat->SetMobility(EComponentMobility::Movable);
	SextonSeat->RegisterComponent();
	SextonSeat->SetWorldTransform(Seat);

	UStaticMesh* Model = LoadIfMade(Set.SextonMesh);
	if (!Model)
	{
		UE_LOG(LogLooter, Log, TEXT("Cold open: Sexton's model isn't in this checkout (%s): a stand-in sits on the rail."), *Set.SextonMesh.ToString());
		BuildSextonStandIn(SextonSeat);
		return;
	}
	// His pivot is the seat point and his front faces into the deck: on the seat as it is.
	SextonModel = NewObject<UStaticMeshComponent>(this, TEXT("Sexton"), RF_Transient);
	SextonModel->SetupAttachment(SextonSeat);
	SextonModel->SetMobility(EComponentMobility::Movable);
	SextonModel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SextonModel->SetStaticMesh(Model);
	SextonModel->RegisterComponent();
	// Drawn black: every one of his slots on the unlit black (the art session's note for the cold open).
	PaintBlack(*SextonModel);
	if (UStaticMesh* Ledger = LoadIfMade(Set.SextonLedgerMesh))
	{
		UStaticMeshComponent* Book = NewObject<UStaticMeshComponent>(this, TEXT("SextonLedger"), RF_Transient);
		Book->SetupAttachment(SextonModel, SextonModel->DoesSocketExist(LedgerSocket) ? LedgerSocket : NAME_None);
		Book->SetMobility(EComponentMobility::Movable);
		Book->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Book->SetStaticMesh(Ledger);
		Book->RegisterComponent();
		PaintBlack(*Book);
	}
}

void AColdOpenCast::BuildSextonStandIn(USceneComponent* Seat)
{
	// Tall and gaunt, sitting on the rail with his legs crossed, the stovepipe tall: plain shapes, only ever a black
	// shape against the sky. Seat's frame: +X into the deck, the rail's top at its origin.
	const TCHAR* Cylinder = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
	const TCHAR* Sphere = TEXT("/Engine/BasicShapes/Sphere.Sphere");
	const TCHAR* Cube = TEXT("/Engine/BasicShapes/Cube.Cube");
	const FRotator Level = FRotator::ZeroRotator;
	TArray<UStaticMeshComponent*> Parts;
	Parts.Add(AddShape(*this, Seat, Cylinder, FVector(4.0, 0.0, 40.0), FRotator(-6.0, 0.0, 0.0), FVector(30.0, 26.0, 76.0)));   // coat
	Parts.Add(AddShape(*this, Seat, Sphere, FVector(9.0, 0.0, 93.0), Level, FVector(21.0, 19.0, 24.0)));                        // head
	Parts.Add(AddShape(*this, Seat, Cylinder, FVector(9.0, 0.0, 104.0), Level, FVector(38.0, 38.0, 2.0)));                      // brim
	Parts.Add(AddShape(*this, Seat, Cylinder, FVector(9.0, 0.0, 126.0), Level, FVector(24.0, 24.0, 42.0)));                     // stovepipe
	Parts.Add(AddShape(*this, Seat, Cylinder, FVector(24.0, -9.0, 3.0), FRotator(90.0, 0.0, 0.0), FVector(15.0, 15.0, 48.0)));  // thigh
	Parts.Add(AddShape(*this, Seat, Cylinder, FVector(24.0, 5.0, 13.0), FRotator(80.0, -14.0, 0.0), FVector(14.0, 14.0, 48.0))); // crossed
	Parts.Add(AddShape(*this, Seat, Cylinder, FVector(47.0, -9.0, -24.0), Level, FVector(12.0, 12.0, 52.0)));                    // shin
	Parts.Add(AddShape(*this, Seat, Cylinder, FVector(51.0, 12.0, -3.0), FRotator(35.0, 0.0, 0.0), FVector(11.0, 11.0, 46.0)));  // crossed shin
	Parts.Add(AddShape(*this, Seat, Cube, FVector(-9.0, 0.0, -20.0), Level, FVector(6.0, 34.0, 42.0)));                         // coat tails
	Parts.Add(AddShape(*this, Seat, Cube, FVector(36.0, 0.0, 22.0), FRotator(0.0, 0.0, 0.0), FVector(24.0, 18.0, 3.0)));        // ledger
	for (UStaticMeshComponent* Part : Parts)
	{
		if (Part)
		{
			PaintBlack(*Part);
		}
	}
}

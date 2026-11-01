// ADredgerCreature: its look and its stand-in kit. The mannequin in black rubber (every slot), a gas mask (a face plate, an
// iron snout and filter, two round glass lenses that glow and flare) and a flat company cap, all engine shapes on the head
// bone, each placed where it should sit at rest in the mesh's frame and riding its bone from there (PlacePart), whatever
// that bone's own axes are. Stand-ins until the user's Tripo Dredgers (TRIPO_MODELS.md 5-6) bring their own: then
// Looter.Dredger.Body.Kit 0 hides the kit, and DredgerLook::BodyMeshPath names the new body.

#include "Creatures/DredgerCreature.h"
#include "Creatures/DredgerLook.h"
#include "Creatures/DredgerTuning.h"
#include "AnimationRuntime.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

namespace
{
	/** A turn that lays an engine cylinder (its axis up) along the mesh's ahead (+Y). */
	FQuat KitAlongAhead()
	{
		return FQuat(FVector::XAxisVector, -HALF_PI);
	}
}

UMaterialInstanceDynamic* ADredgerCreature::MakeLook(const FLinearColor& Color, float Glow, float Variation)
{
	if (!SurfaceMaster)
	{
		return nullptr;
	}
	UMaterialInstanceDynamic* Look = UMaterialInstanceDynamic::Create(SurfaceMaster, this);
	Look->SetVectorParameterValue(TEXT("Color"), Color);
	Look->SetScalarParameterValue(TEXT("Glow"), Glow);
	Look->SetScalarParameterValue(TEXT("Variation"), Variation);
	return Look;
}

void ADredgerCreature::ApplyBodyLook()
{
	RubberLook = MakeLook(DredgerLook::Rubber, 0.f, 0.18f);
	LensLook = MakeLook(DredgerLook::Lens, DredgerLook::LensGlow, 0.f);
	USkeletalMeshComponent* Body = GetMesh();
	// Stand-in: the mannequin in black rubber on every slot. The Tripo Dredger brings its own materials: with the kit off
	// (Looter.Dredger.Body.Kit 0, its default once that model is in) the body keeps them.
	const bool bStandIn = FDredgerTuning::Live().Kit != 0;
	for (int32 Slot = 0; bStandIn && RubberLook && Slot < Body->GetNumMaterials(); ++Slot)
	{
		Body->SetMaterial(Slot, RubberLook);
	}
	UMaterialInstanceDynamic* Mask = MakeLook(DredgerLook::MaskRubber, 0.f, 0.25f);
	UMaterialInstanceDynamic* IronLook = MakeLook(DredgerLook::Iron, 0.f, 0.3f);
	UMaterialInstanceDynamic* Cap = MakeLook(DredgerLook::CapCloth, 0.f, 0.3f);
	MaskFace->SetMaterial(0, Mask);
	MaskSnout->SetMaterial(0, IronLook);
	MaskFilter->SetMaterial(0, IronLook);
	LensLeft->SetMaterial(0, LensLook);
	LensRight->SetMaterial(0, LensLook);
	CapCrown->SetMaterial(0, Cap);
	CapBrim->SetMaterial(0, Cap);
}

FTransform ADredgerCreature::BoneAtRest(FName Bone) const
{
	const USkeletalMesh* Model = GetMesh()->GetSkeletalMeshAsset();
	const int32 Index = Model ? Model->GetRefSkeleton().FindBoneIndex(Bone) : INDEX_NONE;
	return Index != INDEX_NONE ? FAnimationRuntime::GetComponentSpaceTransformRefPose(Model->GetRefSkeleton(), Index) : FTransform::Identity;
}

FTransform ADredgerCreature::FitInBox(const UStaticMesh* PartMesh, const FVector& Centre, const FVector& Size, const FQuat& Turn)
{
	// (PartMesh, not Mesh: ACharacter's Mesh member would be hidden, a shadowing error under UE's warnings.)
	if (!PartMesh)
	{
		return FTransform(Turn, Centre);
	}
	// The mesh's own box, each of its axes scaled to the box side it is turned onto (turns here are quarter turns).
	const FBox Local = PartMesh->GetBoundingBox();
	const FVector LocalSize = Local.GetSize();
	FVector Scale = FVector::OneVector;
	for (int32 Axis = 0; Axis < 3; ++Axis)
	{
		FVector Unit = FVector::ZeroVector;
		Unit[Axis] = 1.0;
		const FVector Turned = Turn.RotateVector(Unit).GetAbs();
		const int32 Onto = Turned.X >= Turned.Y && Turned.X >= Turned.Z ? 0 : (Turned.Y >= Turned.Z ? 1 : 2);
		Scale[Axis] = Size[Onto] / FMath::Max(LocalSize[Axis], 0.001);
	}
	const FVector Middle = Turn.RotateVector(Local.GetCenter() * Scale);
	return FTransform(Turn, Centre - Middle, Scale);
}

void ADredgerCreature::PlacePart(UStaticMeshComponent* Part, FName Bone, const FTransform& InMesh) const
{
	if (Part)
	{
		// The part rides its bone: where it should sit at rest, taken into the bone's own frame.
		Part->SetRelativeTransform(InMesh.GetRelativeTransform(BoneAtRest(Bone)));
	}
}

void ADredgerCreature::PlaceKit()
{
	if (bKitPlaced)
	{
		return;
	}
	bKitPlaced = true;
	// The mesh's frame: +Y ahead, +X its left, +Z up; numbers in cm from the head bone at rest (the mannequin's skull's top
	// about 25 cm over it, the eyes about 11 up and 11 ahead). Stand-ins for the Tripo mask and cap: tune here.
	const FVector Head = BoneAtRest(TEXT("head")).GetLocation();
	const FQuat Ahead = KitAlongAhead();
	PlacePart(MaskFace, TEXT("head"), FitInBox(SphereMesh, Head + FVector(0.f, 8.5f, 7.5f), FVector(17.f, 12.f, 18.f)));
	PlacePart(MaskSnout, TEXT("head"), FitInBox(CylinderMesh, Head + FVector(0.f, 14.5f, 3.f), FVector(6.f, 7.f, 6.f), Ahead));
	PlacePart(MaskFilter, TEXT("head"), FitInBox(CylinderMesh, Head + FVector(0.f, 19.f, 2.5f), FVector(8.5f, 4.f, 8.5f), Ahead));
	PlacePart(LensLeft, TEXT("head"), FitInBox(CylinderMesh, Head + FVector(4.4f, 13.6f, 11.f), FVector(4.8f, 1.6f, 4.8f), Ahead));
	PlacePart(LensRight, TEXT("head"), FitInBox(CylinderMesh, Head + FVector(-4.4f, 13.6f, 11.f), FVector(4.8f, 1.6f, 4.8f), Ahead));
	PlacePart(CapCrown, TEXT("head"), FitInBox(CylinderMesh, Head + FVector(0.f, 0.5f, 21.f), FVector(21.f, 22.f, 8.f)));
	PlacePart(CapBrim, TEXT("head"), FitInBox(CylinderMesh, Head + FVector(0.f, 4.5f, 17.5f), FVector(25.f, 30.f, 1.4f)));
	if (FDredgerTuning::Live().Kit == 0)
	{
		for (UStaticMeshComponent* Part : { MaskFace.Get(), MaskSnout.Get(), MaskFilter.Get(), LensLeft.Get(), LensRight.Get(),
			CapCrown.Get(), CapBrim.Get() })
		{
			Part->SetVisibility(false);
		}
	}
}

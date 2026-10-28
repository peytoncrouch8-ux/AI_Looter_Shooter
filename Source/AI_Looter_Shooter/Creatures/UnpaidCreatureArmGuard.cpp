// AUnpaidCreature's arm guard: the points of the arms' hit hulls (PA_Unpaid) that keep out of the torso, read once from
// the model's physics asset, and how deep an arm sits in the torso past its rest (UnpaidCreatureArms.cpp moves the arm out).
// Split from UnpaidCreatureArms.cpp (2026-10-28, the ~500-line rule).

#include "Creatures/UnpaidCreature.h"
#include "AI_Looter_Shooter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "ReferenceSkeleton.h"

namespace
{
	/**
	 * Points spread evenly all round each arm bone's hull (upper arm, forearm, hand); an upper arm's within this share of its
	 * length of the shoulder sit in the chest it hangs from by design (the armpit's sleeve folds into it), so only the chest
	 * lets them in. (The keep-out's own numbers are UnpaidCreatureArms.cpp's.)
	 */
	// The upper arm's hull has the most corners near the torso (its armpit against the waist in the shriek): with 28 points a
	// corner between them sat 2-3 cm deeper than any the guard watched, so it watches nearly all of them.
	constexpr int32 GuardPoints[3] = { 48, 16, 12 };
	constexpr float ArmpitShare = 0.45f;
}

// ---------------------------------------------------------------------------
// The guard: what keeps out of what, from the model's hit hulls
// ---------------------------------------------------------------------------

void AUnpaidCreature::SetupArmGuard()
{
	TorsoHulls.Shapes.Reset();
	TorsoGuards.Reset();
	for (int32 Side = 0; Side < 2; ++Side)
	{
		ArmGuards[Side].Reset();
		ArmSway[Side] = FArmSway();
	}
	const USkeletalMeshComponent* Body = GetMesh();
	const USkeletalMesh* Model = Body ? Body->GetSkeletalMeshAsset() : nullptr;
	const UPhysicsAsset* Hulls = Body ? Body->GetPhysicsAsset() : nullptr;
	if (!Model || !Hulls)
	{
		UE_LOG(LogLooter, Verbose, TEXT("%s: no hit hulls, so its arms aren't kept out of its torso."), *GetName());
		return;
	}
	const FReferenceSkeleton& Skeleton = Model->GetRefSkeleton();

	// The torso: pelvis, waist, chest, and the shroud's top two links (the hands hang beside them).
	TArray<FName> TorsoNames = { Rig.Pelvis, Rig.Spine, Rig.Chest };
	TorsoNames.Append(TConstArrayView<FName>(Rig.Shroud).Left(2));
	for (const FName& Name : TorsoNames)
	{
		const int32 Posed = FindPosedBone(Name);
		if (Posed == INDEX_NONE)
		{
			continue;
		}
		const int32 First = TorsoHulls.Shapes.Num();
		TorsoHulls.AddFromPhysicsAsset(Hulls, &Skeleton, MakeArrayView(&Name, 1));
		for (int32 Shape = First; Shape < TorsoHulls.Shapes.Num(); ++Shape)
		{
			TorsoGuards.Add({ Posed, Shape });
		}
	}
	if (TorsoGuards.IsEmpty())
	{
		return;
	}

	for (int32 Side = 0; Side < 2; ++Side)
	{
		const FArm& Arm = Arms[Side];
		// Each arm bone points at the next: the upper arm at the elbow, the forearm at the wrist, the hand at its fingers.
		const int32 Chain[3] = { Arm.UpperArm, Arm.LowerArm, Arm.Hand };
		for (int32 Link = 0; Link < 3; ++Link)
		{
			const int32 Posed = Chain[Link];
			if (Posed == INDEX_NONE)
			{
				continue;
			}
			FCreatureBodyHulls ArmHulls;
			ArmHulls.AddFromPhysicsAsset(Hulls, &Skeleton, MakeArrayView(&Bones[Posed].Name, 1));
			const FTransform& Rest = Bones[Posed].Rest;
			const FVector Joint = Rest.GetLocation();
			FVector Toward = FVector::ZeroVector;
			if (Link < 2 && Chain[Link + 1] != INDEX_NONE)
			{
				Toward = Bones[Chain[Link + 1]].Rest.GetLocation() - Joint;
			}
			else if (!Arm.Fingers.IsEmpty())
			{
				Toward = Bones[Arm.Fingers[Arm.Fingers.Num() / 2]].Rest.GetLocation() - Joint;
			}
			const float Length = static_cast<float>(Toward.Size());
			const FVector Along = Toward.GetSafeNormal(UE_SMALL_NUMBER, FVector::DownVector);

			// Points all round its hull, spread evenly: the first the farthest from the joint, then each the farthest from those
			// already taken, so every side (the back of the armpit too) has one near wherever the hull could touch the torso.
			TArray<FVector> Corners;
			for (const FCreatureBodyHulls::FShape& Shape : ArmHulls.Shapes)
			{
				Corners.Append(Shape.Points);
			}
			TArray<double> Spread;
			Spread.Init(TNumericLimits<double>::Max(), Corners.Num());
			int32 Pick = INDEX_NONE;
			double Farthest = -1.0;
			for (int32 Index = 0; Index < Corners.Num(); ++Index)
			{
				const double FromJoint = FVector::DistSquared(Rest.TransformPosition(Corners[Index]), Joint);
				if (FromJoint > Farthest)
				{
					Farthest = FromJoint;
					Pick = Index;
				}
			}
			for (int32 Taken = 0; Taken < GuardPoints[Link] && Pick != INDEX_NONE; ++Taken)
			{
				FArmGuardPoint& Guard = ArmGuards[Side].AddDefaulted_GetRef();
				Guard.Bone = Posed;
				Guard.InBone = Corners[Pick];
				Guard.bNearShoulder = Link == 0
					&& FVector::DotProduct(Rest.TransformPosition(Corners[Pick]) - Joint, Along) < ArmpitShare * Length;
				const FVector Chosen = Corners[Pick];
				Pick = INDEX_NONE;
				double Widest = 1.0;
				for (int32 Index = 0; Index < Corners.Num(); ++Index)
				{
					Spread[Index] = FMath::Min(Spread[Index], FVector::DistSquared(Corners[Index], Chosen));
					if (Spread[Index] > Widest)
					{
						Widest = Spread[Index];
						Pick = Index;
					}
				}
			}
		}
		// How deep each point sits in each torso shape at rest: only going deeper than that counts.
		for (FArmGuardPoint& Guard : ArmGuards[Side])
		{
			const FVector Here = Bones[Guard.Bone].Rest.TransformPosition(Guard.InBone);
			for (const FTorsoGuard& Torso : TorsoGuards)
			{
				const FVector InTorso = Bones[Torso.Bone].Rest.InverseTransformPosition(Here);
				Guard.RestDepth.Add(FCreatureBodyHulls::Depth(TorsoHulls.Shapes[Torso.Shape], InTorso));
			}
		}
	}
	UE_LOG(LogLooter, Verbose, TEXT("%s: its arms keep out of %d torso hulls by %d and %d points."), *GetName(), TorsoGuards.Num(),
		ArmGuards[0].Num(), ArmGuards[1].Num());
}

float AUnpaidCreature::ArmIntrusion(int32 Side, float* OutLever) const
{
	float Deepest = 0.f;
	const FArm& Arm = Arms[Side];
	const FVector Shoulder = Arm.UpperArm != INDEX_NONE ? Bones[Arm.UpperArm].Posed.GetLocation() : FVector::ZeroVector;
	// The chest the upper arm hangs from: its armpit's points may sit in it.
	const int32 HungFrom = Arm.UpperArm != INDEX_NONE ? Bones[Arm.UpperArm].Parent : INDEX_NONE;
	for (const FArmGuardPoint& Guard : ArmGuards[Side])
	{
		const FVector Here = Bones[Guard.Bone].Posed.TransformPosition(Guard.InBone);
		for (int32 Index = 0; Index < TorsoGuards.Num(); ++Index)
		{
			const FTorsoGuard& Torso = TorsoGuards[Index];
			if (Guard.bNearShoulder && Torso.Bone == HungFrom)
			{
				continue;
			}
			const FVector InTorso = Bones[Torso.Bone].Posed.InverseTransformPosition(Here);
			const float Past = FCreatureBodyHulls::DepthWithin(TorsoHulls.Shapes[Torso.Shape], InTorso, 0.f)
				- FMath::Max(Guard.RestDepth.IsValidIndex(Index) ? Guard.RestDepth[Index] : 0.f, 0.f);
			if (Past > Deepest)
			{
				Deepest = Past;
				if (OutLever)
				{
					// Turning the arm out moves this point across the body by about its distance from the shoulder there.
					*OutLever = static_cast<float>(FVector::Dist(FVector(0.0, Shoulder.Y, Shoulder.Z), FVector(0.0, Here.Y, Here.Z)));
				}
			}
		}
	}
	return Deepest;
}

float AUnpaidCreature::GetArmIntrusion() const
{
	if (!bRigReady || TorsoGuards.IsEmpty())
	{
		return 0.f;
	}
	return FMath::Max(ArmIntrusion(0), ArmIntrusion(1));
}

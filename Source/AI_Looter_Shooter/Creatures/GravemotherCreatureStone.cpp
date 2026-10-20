// AGravemotherCreature's side of her headstone (AGravemotherStone; the user's pick over plain armour, 2026-10-19): it's made
// and set whole for each life (its health Main's 20% of hers), placed on her abdomen bone, and tells her of its hits, cracks
// and breaking (broken early, her boss's stagger; at her 33% it cracks off by itself, no stagger). And her crits: her head
// always; her abdomen only on its soft rear while the stone stands (her physics asset's round zone there, ART's
// USP_AbdomenRear: one body per bone keeps no names, so the shape's kind tells it; on SK_Spider, the abdomen's far end),
// all of it once the stone is off.

#include "Creatures/GravemotherCreature.h"
#include "AI_Looter_Shooter.h"
#include "AnimationRuntime.h"
#include "Bosses/BossComponent.h"
#include "Combat/HealthComponent.h"
#include "Creatures/GravemotherAssets.h"
#include "Creatures/GravemotherStone.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/HitResult.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/Controller.h"
#include "Materials/MaterialInterface.h"
#include "PhysicsEngine/PhysicsAsset.h"

namespace
{
	/**
	 * The stone on SK_Gravemother: ART's stone_attach.json (Intermediate/ArtExport_Gravemother), its pivot (the middle of
	 * its foot) in the mesh's component space at rest, at the rig's size 1 (her 1.8 is her mesh's), +X out of its carved
	 * face toward her head. The json's bone-space numbers assume the abdomen's rest pose imports as (-40, 0, 66), P 0 /
	 * Y -90 / R 13.496; these are exact whatever it imports as, so the bone-space place is worked out from them.
	 */
	const FVector StoneOnHerAt(-78.179, 0.0, 89.017);
	const FRotator StoneOnHerTurn(26.0, -4.0, 5.0);
	const FVector AbdomenRestExpected(-40.0, 0.0, 66.0);

	/**
	 * On another body (SK_Spider, before her own is imported): worked out from the abdomen's hull at her model's size: on
	 * top of the hull, standing upright on her back and leaning back a little, wide across her, thin front to back.
	 */
	constexpr float StoneSeatShare = 0.8f;
	constexpr float StoneLeanBack = 10.f;
	/** Without a hull to work from: about where the spider's abdomen top is, in its bone's space. */
	const FVector FallbackStoneSeat(-50.0, 0.0, 40.0);
}

void AGravemotherCreature::ReadAbdomenZones()
{
	bZonesRead = true;
	AbdomenZones = FCreatureBodyHulls();
	RearZones.Reset();
	AbdomenHull = INDEX_NONE;
	const USkeletalMesh* Model = GetMesh()->GetSkeletalMeshAsset();
	const UPhysicsAsset* Physics = GetMesh()->GetPhysicsAsset();
	if (!Model || !Physics)
	{
		return;
	}
	const FName Bones[] = { AbdomenBone };
	AbdomenZones.AddFromPhysicsAsset(Physics, &Model->GetRefSkeleton(), Bones);
	for (int32 Index = 0; Index < AbdomenZones.Shapes.Num(); ++Index)
	{
		if (AbdomenZones.Shapes[Index].IsRound())
		{
			RearZones.Add(Index);
		}
		else if (AbdomenHull == INDEX_NONE)
		{
			AbdomenHull = Index;
		}
	}
	UE_LOG(LogLooter, Verbose, TEXT("%s: abdomen zones: %d round (its soft rear), hull %s."), *GetName(), RearZones.Num(),
		AbdomenHull != INDEX_NONE ? TEXT("found") : TEXT("none"));
}

FTransform AGravemotherCreature::StonePlacement() const
{
	const USkeletalMesh* Model = GetMesh()->GetSkeletalMeshAsset();
	const int32 Bone = Model ? Model->GetRefSkeleton().FindBoneIndex(AbdomenBone) : INDEX_NONE;
	if (Bone != INDEX_NONE && Model->GetPathName() == GravemotherAssets::Body)
	{
		// Her own body: ART's place, from the mesh's frame into the bone's as the model rests.
		const FTransform Rest = FAnimationRuntime::GetComponentSpaceTransformRefPose(Model->GetRefSkeleton(), Bone);
		if (!Rest.GetLocation().Equals(AbdomenRestExpected, 1.0))
		{
			UE_LOG(LogLooter, Log, TEXT("%s: the abdomen rests at %s, not ART's %s: the stone is placed from the mesh's frame."), *GetName(),
				*Rest.GetLocation().ToString(), *AbdomenRestExpected.ToString());
		}
		return FTransform(StoneOnHerTurn, StoneOnHerAt).GetRelativeTransform(Rest);
	}
	if (Bone == INDEX_NONE || !AbdomenZones.Shapes.IsValidIndex(AbdomenHull))
	{
		return FTransform(FallbackStoneSeat);
	}
	// The bone as the model rests: the stone stands up in the model's frame (wide across her, leaning back), on the hull's top.
	const FTransform Rest = FAnimationRuntime::GetComponentSpaceTransformRefPose(Model->GetRefSkeleton(), Bone);
	const FCreatureBodyHulls::FShape& Hull = AbdomenZones.Shapes[AbdomenHull];
	const FVector UpInBone = Rest.InverseTransformVectorNoScale(FVector::UpVector);
	// The hull's top over its middle: the stone's foot sits a little into it.
	double Top = 0.0;
	for (const FVector& Corner : Hull.Points)
	{
		Top = FMath::Max(Top, FVector::DotProduct(Corner - Hull.BoundCenter, UpInBone));
	}
	const FVector Seat = Hull.BoundCenter + UpInBone * (Top * StoneSeatShare);
	const FQuat Standing = Rest.GetRotation().Inverse() * FRotator(StoneLeanBack, 0.f, 0.f).Quaternion();
	return FTransform(Standing, Seat);
}

void AGravemotherCreature::ResetStone()
{
	if (!bZonesRead)
	{
		ReadAbdomenZones();
	}
	const float StoneHealth = (Health ? Health->GetMaxHealth() : 1000.f) * StoneSettings.HealthShare;
	AGravemotherStone* Slab = Stone.Get();
	if (!Slab)
	{
		Slab = AGravemotherStone::Spawn(*this, StonePlacement(), StoneSettings);
		Stone = Slab;
	}
	if (Slab)
	{
		Slab->ResetStone(StoneHealth);
		Slab->SetActorHiddenInGame(IsHidden());
	}
	// Her abdomen as it is under a standing stone (its own slot's look again, the bare one cleared).
	if (GetMesh()->GetMaterialIndex(GravemotherAssets::AbdomenSlot) != INDEX_NONE)
	{
		GetMesh()->SetMaterialByName(GravemotherAssets::AbdomenSlot, nullptr);
	}
}

bool AGravemotherCreature::IsStoneOn() const
{
	const AGravemotherStone* Slab = Stone.Get();
	return Slab && Slab->IsOn();
}

bool AGravemotherCreature::IsSoftRear(const FVector& WorldPoint) const
{
	if (!bZonesRead || AbdomenZones.Shapes.IsEmpty())
	{
		return false;
	}
	// In the abdomen bone's own space (its pose now; the physics asset's shapes are there at the model's size).
	const FTransform Bone = GetMesh()->GetSocketTransform(AbdomenBone, RTS_World);
	const FVector Point = Bone.InverseTransformPosition(WorldPoint);
	if (!RearZones.IsEmpty())
	{
		for (const int32 Index : RearZones)
		{
			if (FCreatureBodyHulls::Depth(AbdomenZones.Shapes[Index], Point) >= -StoneSettings.RearMargin)
			{
				return true;
			}
		}
		return false;
	}
	// SK_Spider: one hull. The rear is its far end from the pedicel.
	if (!AbdomenZones.Shapes.IsValidIndex(AbdomenHull))
	{
		return false;
	}
	const FCreatureBodyHulls::FShape& Hull = AbdomenZones.Shapes[AbdomenHull];
	const FVector Axis = Hull.BoundCenter.GetSafeNormal();
	if (Axis.IsNearlyZero())
	{
		return false;
	}
	double Farthest = 0.0;
	for (const FVector& Corner : Hull.Points)
	{
		Farthest = FMath::Max(Farthest, FVector::DotProduct(Corner, Axis));
	}
	return Farthest > 0.0 && FVector::DotProduct(Point, Axis) >= StoneSettings.RearShare * Farthest;
}

bool AGravemotherCreature::IsCriticalSpot(const FHitResult& Hit) const
{
	if (Hit.GetComponent() == GetMesh() && Hit.BoneName == AbdomenBone)
	{
		if (!IsStoneOn())
		{
			return true;
		}
		const FVector Where = !FVector(Hit.ImpactPoint).IsZero() ? FVector(Hit.ImpactPoint) : FVector(Hit.Location);
		return IsSoftRear(Where);
	}
	return Super::IsCriticalSpot(Hit);
}

void AGravemotherCreature::NoteStoneHit(AController* By)
{
	// A shot on her stone turns her on the shooter as a shot on her would.
	APawn* Shooter = By ? By->GetPawn() : nullptr;
	if (Shooter && !IsDead())
	{
		AlertTo(Shooter);
	}
}

void AGravemotherCreature::OnStoneCracked(EGravemotherStone NewState)
{
	// She feels it crack: the abdomen jolts under it.
	if (const AGravemotherStone* Slab = Stone.Get(); Slab && !IsDead())
	{
		ASpiderCreature::OnHurt(false, Slab->GetActorLocation() + FVector(0.0, 0.0, 200.0));
	}
}

void AGravemotherCreature::OnStoneBroken(bool bByHerThird)
{
	if (IsDead())
	{
		return;
	}
	if (const AGravemotherStone* Slab = Stone.Get())
	{
		ASpiderCreature::OnHurt(true, Slab->GetActorLocation() + FVector(0.0, 0.0, 200.0));
	}
	// Bare where it stood: her abdomen all pale bone, the stone's print showing (ART's MI_GravemotherBare on her own body).
	if (GetMesh()->GetMaterialIndex(GravemotherAssets::AbdomenSlot) != INDEX_NONE)
	{
		if (UMaterialInterface* Bare = GravemotherAssets::Load<UMaterialInterface>(GravemotherAssets::AbdomenBare))
		{
			GetMesh()->SetMaterialByName(GravemotherAssets::AbdomenSlot, Bare);
		}
	}
	// Shot off before her last third: her long stagger (her boss's), but not her last third (Main's call, 2026-10-19). At
	// her third it comes off by itself as she screams into her fury: no stagger.
	if (!bByHerThird && Boss && Boss->IsFighting())
	{
		Boss->BeginStagger();
	}
	UE_LOG(LogLooter, Log, TEXT("%s: her headstone is off%s: her whole abdomen is a crit now."), *GetName(),
		bByHerThird ? TEXT(" (her third)") : TEXT(" (shot off)"));
}

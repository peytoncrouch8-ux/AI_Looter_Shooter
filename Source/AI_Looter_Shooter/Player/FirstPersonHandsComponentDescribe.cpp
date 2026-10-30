#include "Player/FirstPersonHandsComponent.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponDefinition.h"
#include "Weapons/WeaponModelComponent.h"
#include "Weapons/WeaponTypes.h"
#include "Components/StaticMeshComponent.h"

// UFirstPersonHandsComponent::DescribeGun: the gun read off as it is this frame (where it sits to the camera, its grip and
// foregrip, its reload and bolt state, the grip spots its parts mark, the magazine seat or cylinder) for the hands to be placed on.

namespace
{
	const FName MagazineSocket(TEXT("Magazine"));
	const FName RocketSocket(TEXT("Rocket"));

	/** A socket on any of the model's parts (the latest that has it), in the model's own space. */
	bool FindPartSocket(const UWeaponModelComponent& Model, FName Socket, FVector& OutLocation)
	{
		const TArray<TObjectPtr<UStaticMeshComponent>>& Parts = Model.GetParts();
		for (int32 Index = Parts.Num() - 1; Index >= 0; --Index)
		{
			const UStaticMeshComponent* Part = Parts[Index];
			if (Part && Part->DoesSocketExist(Socket))
			{
				OutLocation = Model.GetComponentTransform().InverseTransformPosition(Part->GetSocketLocation(Socket));
				return true;
			}
		}
		return false;
	}
}

void UFirstPersonHandsComponent::DescribeGun(const AWeaponBase& Weapon, const FTransform& CameraWorld, FHandsGun& OutGun) const
{
	// The visible model, not the gun's root: a strike or a throw turns the model off its hold, and the hands go with it.
	const UWeaponModelComponent* Model = Weapon.FindComponentByClass<UWeaponModelComponent>();
	const bool bModel = Model && Model->IsAssembled();
	const FTransform GunWorld = bModel ? Model->GetComponentTransform() : Weapon.GetActorTransform();
	OutGun.ToCamera = GunWorld.GetRelativeTransform(CameraWorld);
	// The first-person size (FPHandsFraming::GunScale, on the gun's actor) taken out of the transform and into the gun's
	// own numbers below: its sockets and Scale, which the grip tables, the handle shapes and the reload paths all use.
	const float Drawn = FMath::Max(static_cast<float>(OutGun.ToCamera.GetScale3D().X), UE_KINDA_SMALL_NUMBER);
	OutGun.ToCamera.SetScale3D(FVector::OneVector);
	const UWeaponDefinition* Definition = Weapon.GetInstance().Definition.Get();
	OutGun.Kind = Definition ? Definition->Kind : EWeaponKind::None;
	// A gun not built from parts has no grip socket: the weapon manager lines such a gun's hold up on AttachGrip.
	const UWeaponManagerComponent* Manager = Weapons.Get();
	OutGun.Grip = (bModel && !Model->GetGrip().IsZero() ? Model->GetGrip() : (Manager ? Manager->AttachGrip : FVector::ZeroVector)) * Drawn;
	OutGun.Foregrip = (bModel ? Model->GetForegrip() : Weapon.GetForegripPoint()) * Drawn;
	// Drawn bigger than modeled (UWeaponModelComponent::ScaleFor, and first person's own size): the sockets already are,
	// the hands' own numbers off the gun follow it.
	OutGun.Scale = Weapon.GetModelScale() * Drawn;
	OutGun.ReloadPart = Weapon.GetReloadPart();
	OutGun.ReloadProgress = Weapon.GetReloadProgress();
	OutGun.ReloadPlan = Weapon.GetReloadPlan();
	OutGun.PumpCycle = Weapon.GetPumpCycle();
	// The sniper's bolt, worked by the trigger hand between shots and in its reload (FirstPersonHandsGrips::OnGrip).
	OutGun.BoltOpen = Weapon.GetBoltOpen();
	OutGun.BoltHand = Weapon.GetBoltHand();
	// The hands' own size in the camera's space, and the grip spots the parts mark, drawn to scale like the sockets above
	// (FirstPersonHandsMarks: the hands go to them where a gun has them).
	OutGun.HandScale = GetWantedScaleFor(OutGun.Kind);
	if (bModel)
	{
		OutGun.Cup = CupFitFor(*Model, OutGun.Kind, OutGun.ReloadProgress < 0.f && OutGun.PumpCycle < 0.f && OutGun.BoltOpen <= 0.f);
		if (Model->GetHandMarks(OutGun.Marks))
		{
			OutGun.Marks.ScalePoints(Drawn);
		}
	}
	if (OutGun.ReloadProgress < 0.f || !bModel)
	{
		return;
	}
	if (IsMagazineLike(OutGun.ReloadPart))
	{
		OutGun.bHasMagazineSeat = FindPartSocket(*Model, MagazineSocket, OutGun.MagazineSeat);
		OutGun.MagazineSeat *= Drawn;
	}
	else if (OutGun.ReloadPart == EWeaponReloadPart::Rocket)
	{
		// The launcher's rocket seats in its barrel's mouth: the reload's hand works there.
		OutGun.bHasMagazineSeat = FindPartSocket(*Model, RocketSocket, OutGun.MagazineSeat);
		OutGun.MagazineSeat *= Drawn;
	}
	else if (OutGun.ReloadPart == EWeaponReloadPart::Cylinder)
	{
		// The cylinder as the gun's model has swung it this frame (the gun ticks before the hands): its chambers' backs
		// ring its axis at its rear face.
		TArray<FVector> Rears;
		FVector Back;
		if (Model->GetChamberRears(Rears, Back) && !Rears.IsEmpty())
		{
			FVector Middle = FVector::ZeroVector;
			for (const FVector& Rear : Rears)
			{
				Middle += Rear;
			}
			OutGun.CylinderRear = GunWorld.InverseTransformPosition(Middle / Rears.Num()) * Drawn;
			OutGun.CylinderAxis = -GunWorld.InverseTransformVectorNoScale(Back).GetSafeNormal(UE_SMALL_NUMBER, -FVector::ForwardVector);
			OutGun.bHasCylinder = true;
		}
	}
}

#include "Weapons/WeaponModelComponent.h"
#include "AI_Looter_Shooter.h"
#include "Affixes/WeaponRollLibrary.h"
#include "Weapons/WeaponDefinition.h"
#include "Weapons/WeaponParts.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"

bool UWeaponModelComponent::Assemble(const FWeaponInstanceData& Instance)
{
	Clear();
	const UWeaponDefinition* Definition = Instance.Definition;
	AActor* Owner = GetOwner();
	if (!Definition || Definition->Parts.IsEmpty() || !Owner)
	{
		return false;
	}
	const FWeaponLook Look = WeaponParts::Pick(Instance);
	const float Wear = WeaponParts::Wear(Instance);

	// The parts share their materials; the ones this gun colors become one dynamic instance each, shared by every part.
	TMap<FName, UMaterialInterface*> Painted;
	auto Paint = [&](FName Slot, UMaterialInterface* Material) -> UMaterialInterface*
	{
		const FLinearColor* Color = Look.Colors.Find(Slot);
		const bool bGlow = Slot == Definition->RarityGlowSlot;
		if (!Material || (!Color && !bGlow))
		{
			return Material;
		}
		if (UMaterialInterface** Existing = Painted.Find(Slot))
		{
			return *Existing;
		}
		UMaterialInstanceDynamic* Instanced = UMaterialInstanceDynamic::Create(Material, this);
		if (bGlow)
		{
			Instanced->SetVectorParameterValue(TEXT("Color"), UWeaponRollLibrary::GetRarityColor(Definition, Instance.Rarity));
			Instanced->SetScalarParameterValue(TEXT("Glow"), WeaponParts::RarityGlow(Instance.Rarity));
		}
		else
		{
			// The flat stylized materials take a Color, the textured world materials tint their texture with Tint.
			Instanced->SetVectorParameterValue(TEXT("Color"), *Color);
			Instanced->SetVectorParameterValue(TEXT("Tint"), *Color);
		}
		Paints.Add(Instanced);
		Painted.Add(Slot, Instanced);
		return Instanced;
	};

	// Where the tally can go: the stock, or the body (the first part, when no slot is named for it).
	UStaticMeshComponent* StockPart = nullptr;
	UStaticMeshComponent* BodyPart = nullptr;
	for (int32 Index = 0; Index < Definition->Parts.Num(); ++Index)
	{
		const FWeaponPartSlot& Slot = Definition->Parts[Index];
		const FWeaponPartOption* Option = Look.Parts[Index];
		if (!Option)
		{
			continue;
		}

		// Hang it from its socket on an earlier part (the latest that has it: a muzzle device hangs from the barrel's
		// Muzzle, not the body's), or at the gun's origin.
		USceneComponent* Parent = this;
		if (!Slot.Socket.IsNone())
		{
			const int32 Holder = Parts.FindLastByPredicate([&Slot](const UStaticMeshComponent* Placed)
			{
				return Placed->DoesSocketExist(Slot.Socket);
			});
			if (Holder != INDEX_NONE)
			{
				Parent = Parts[Holder];
			}
			else
			{
				UE_LOG(LogLooter, Warning, TEXT("%s: no part before %s has the socket %s."), *Definition->GetName(), *Slot.Name.ToString(), *Slot.Socket.ToString());
			}
		}

		UStaticMeshComponent* Part = NewObject<UStaticMeshComponent>(Owner, NAME_None, RF_Transient);
		Part->SetStaticMesh(Option->Mesh);
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->SetGenerateOverlapEvents(false);
		Part->SetCanEverAffectNavigation(false);
		Part->SetFirstPersonPrimitiveType(FirstPersonType);
		Part->SetupAttachment(Parent, Parent == this ? NAME_None : Slot.Socket);
		// How battered this gun is, for the gun master's scuffs and grime (custom primitive data: no material copies).
		Part->SetCustomPrimitiveDataFloat(WeaponParts::WearDataIndex, Wear);
		const TArray<FName> MaterialSlots = Part->GetMaterialSlotNames();
		for (int32 Material = 0; Material < MaterialSlots.Num(); ++Material)
		{
			Part->SetMaterial(Material, Paint(MaterialSlots[Material], Part->GetMaterial(Material)));
		}
		Part->RegisterComponent();
		Parts.Add(Part);

		if (Slot.Name == Definition->ReloadSlot && Definition->ReloadPart != EWeaponReloadPart::None)
		{
			ReloadPartMesh = Part;
			ReloadPart = Definition->ReloadPart;
			if (ReloadPart == EWeaponReloadPart::Cylinder)
			{
				// A cylinder holds as many rounds as its part says, and swings out on the crane its model marks (else on a
				// hinge low on its left, where a swing-out cylinder's is).
				CylinderChambers = FMath::Max(Option->Stats.Magazine, 1);
				const FBox PartBounds = Option->Mesh ? Option->Mesh->GetBoundingBox() : FBox(FVector(-2.0), FVector(2.0));
				CranePivot = Part->DoesSocketExist(CraneSocket) ? Part->GetSocketTransform(CraneSocket, RTS_Component).GetLocation()
					: FVector(0.0, PartBounds.Min.Y * 0.65, PartBounds.Min.Z * 1.1);
			}
		}
		if (Slot.Socket == SightSocket || Slot.Name == SightSocket)
		{
			SightPart = Part;
		}
		if (Slot.Name == StockSlot || Slot.Name == GripSlot)
		{
			StockPart = Part;
		}
		if (Slot.Name == BodySlot)
		{
			BodyPart = Part;
		}
	}
	if (!BodyPart && !Parts.IsEmpty())
	{
		BodyPart = Parts[0];
	}

	FindSocket(TEXT("Muzzle"), Muzzle);
	FindSocket(TEXT("Grip"), Grip);
	FindSocket(TEXT("Foregrip"), Foregrip);
	FindAimPoint();
	ChooseNotchPart(StockPart, BodyPart);
	ShowNotches(Instance);
	return !Parts.IsEmpty();
}

void UWeaponModelComponent::FindAimPoint()
{
	// The sight's own line of sight (its SOCKET_Aim: the dot, the optic's center, the notch of the irons) when it has one.
	if (SightPart && SightPart->DoesSocketExist(AimSocket))
	{
		AimPoint = SightPart->GetSocketTransform(AimSocket, RTS_World).GetRelativeTransform(GetComponentTransform()).GetLocation();
		return;
	}
	// Otherwise just over the top of the sight, at its middle, on the gun's center line; without a sight, over the whole gun.
	FBox Box(ForceInit);
	for (const UStaticMeshComponent* Part : Parts)
	{
		const UStaticMesh* Mesh = Part->GetStaticMesh();
		if (Mesh && (!SightPart || Part == SightPart))
		{
			Box += Mesh->GetBoundingBox().TransformBy(Part->GetComponentTransform().GetRelativeTransform(GetComponentTransform()));
		}
	}
	AimPoint = Box.IsValid ? FVector(Box.GetCenter().X, 0.0, Box.Max.Z) : FVector::ZeroVector;
}

void UWeaponModelComponent::Clear()
{
	for (UStaticMeshComponent* Part : Parts)
	{
		if (Part)
		{
			Part->DestroyComponent();
		}
	}
	Parts.Reset();
	Paints.Reset();
	ReloadPartMesh = nullptr;
	SightPart = nullptr;
	NotchPart = nullptr;
	AimPoint = FVector::ZeroVector;
	ReloadPart = EWeaponReloadPart::None;
	CylinderChambers = 0;
	CranePivot = FVector::ZeroVector;
	CylinderSwing = 0.f;
	CylinderTurnShown = CylinderTurnFrom = CylinderTurnTo = 0.f;
	CylinderTurnAge = 1000.f;
	Muzzle = FVector::ZeroVector;
	Grip = FVector::ZeroVector;
	Foregrip = FVector::ZeroVector;
}

void UWeaponModelComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	Clear();
	Super::OnComponentDestroyed(bDestroyingHierarchy);
}

bool UWeaponModelComponent::FindSocket(FName Socket, FVector& OutLocation) const
{
	// The latest part that has it: the muzzle device's tip, not the barrel's it hangs from.
	for (int32 Index = Parts.Num() - 1; Index >= 0; --Index)
	{
		const UStaticMeshComponent* Part = Parts[Index];
		if (Part->DoesSocketExist(Socket))
		{
			OutLocation = GetComponentTransform().InverseTransformPosition(Part->GetSocketLocation(Socket));
			return true;
		}
	}
	return false;
}

FVector UWeaponModelComponent::GetCenter() const
{
	FBox Box(ForceInit);
	for (const UStaticMeshComponent* Part : Parts)
	{
		if (const UStaticMesh* Mesh = Part->GetStaticMesh())
		{
			Box += Mesh->GetBoundingBox().TransformBy(Part->GetComponentTransform().GetRelativeTransform(GetComponentTransform()));
		}
	}
	return Box.IsValid ? Box.GetCenter() : FVector::ZeroVector;
}

void UWeaponModelComponent::SetReloadTravel(float Travel, bool bShow)
{
	if (!ReloadPartMesh)
	{
		return;
	}
	if (ReloadPart == EWeaponReloadPart::Cylinder)
	{
		CylinderSwing = Travel;
		PoseCylinder();
		ReloadPartMesh->SetVisibility(bShow);
		return;
	}
	// It rests on its socket: a magazine slides out along its -Z, a pump back along its -X.
	const FVector Way = ReloadPart == EWeaponReloadPart::Pump ? FVector(-1.f, 0.f, 0.f) : FVector(0.f, 0.f, -1.f);
	ReloadPartMesh->SetRelativeLocation(Way * Travel);
	ReloadPartMesh->SetVisibility(bShow);
}

void UWeaponModelComponent::TurnCylinder(int32 Chambers)
{
	if (GetCylinderChambers() <= 0 || Chambers <= 0)
	{
		return;
	}
	// A shot fired mid-turn finishes the last turn at once and starts the next from there.
	CylinderTurnFrom = CylinderTurnTo;
	CylinderTurnTo += static_cast<float>(Chambers);
	CylinderTurnShown = CylinderTurnFrom;
	CylinderTurnAge = 0.f;
	PoseCylinder();
}

bool UWeaponModelComponent::UpdateCylinder(float DeltaSeconds)
{
	if (!IsCylinderTurning())
	{
		return false;
	}
	CylinderTurnAge += FMath::Max(DeltaSeconds, 0.f);
	// It waits for the kick, then turns quickly and eases onto the next chamber, where the hand's stop catches it.
	const float T = FMath::Clamp((CylinderTurnAge - CylinderTurnDelay) / CylinderTurnSeconds, 0.f, 1.f);
	CylinderTurnShown = FMath::Lerp(CylinderTurnFrom, CylinderTurnTo, FMath::InterpEaseOut(0.f, 1.f, T, 2.5f));
	// The count only matters within a turn of the cylinder: keep it small so the angle stays exact.
	if (!IsCylinderTurning() && CylinderChambers > 0)
	{
		const float Whole = FMath::Fmod(CylinderTurnTo, static_cast<float>(CylinderChambers));
		CylinderTurnShown = CylinderTurnFrom = CylinderTurnTo = Whole;
	}
	PoseCylinder();
	return IsCylinderTurning();
}

void UWeaponModelComponent::PoseCylinder()
{
	if (!ReloadPartMesh || ReloadPart != EWeaponReloadPart::Cylinder)
	{
		return;
	}
	// Both turns are about the gun's long axis: the cylinder's own turn about its center (its origin, on the socket), the
	// swing about the crane's hinge. About +X, a positive angle carries the top over to the left (-Y), out of the frame's
	// left side, and turns the chambers the same way a swing-out six-gun's do.
	const float TurnDegrees = CylinderChambers > 0 ? CylinderTurnShown * 360.f / static_cast<float>(CylinderChambers) : 0.f;
	const FQuat Swing(FVector::ForwardVector, FMath::DegreesToRadians(CylinderSwing));
	const FQuat Turn(FVector::ForwardVector, FMath::DegreesToRadians(TurnDegrees));
	ReloadPartMesh->SetRelativeLocationAndRotation(CranePivot - Swing.RotateVector(CranePivot), Swing * Turn);
}

void UWeaponModelComponent::SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType Type)
{
	FirstPersonType = Type;
	for (UStaticMeshComponent* Part : Parts)
	{
		Part->SetFirstPersonPrimitiveType(Type);
	}
}

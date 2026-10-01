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
		}
		if (Slot.Socket == SightSocket || Slot.Name == SightSocket)
		{
			SightPart = Part;
		}
	}

	FindSocket(TEXT("Muzzle"), Muzzle);
	FindSocket(TEXT("Grip"), Grip);
	FindSocket(TEXT("Foregrip"), Foregrip);
	FindAimPoint();
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
	AimPoint = FVector::ZeroVector;
	ReloadPart = EWeaponReloadPart::None;
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
	// It rests on its socket: a magazine slides out along its -Z, a pump back along its -X.
	const FVector Way = ReloadPart == EWeaponReloadPart::Pump ? FVector(-1.f, 0.f, 0.f) : FVector(0.f, 0.f, -1.f);
	ReloadPartMesh->SetRelativeLocation(Way * Travel);
	ReloadPartMesh->SetVisibility(bShow);
}

void UWeaponModelComponent::SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType Type)
{
	FirstPersonType = Type;
	for (UStaticMeshComponent* Part : Parts)
	{
		Part->SetFirstPersonPrimitiveType(Type);
	}
}

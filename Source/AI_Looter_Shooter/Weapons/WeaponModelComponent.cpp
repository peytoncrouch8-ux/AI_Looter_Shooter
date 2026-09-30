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
	const FWeaponLook Look = WeaponParts::Pick(*Definition, Instance.Seed, Instance.Rarity);

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
			Instanced->SetVectorParameterValue(TEXT("Color"), *Color);
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

		// Hang it from its socket on an earlier part, or at the gun's origin.
		USceneComponent* Parent = this;
		if (!Slot.Socket.IsNone())
		{
			const TObjectPtr<UStaticMeshComponent>* Holder = Parts.FindByPredicate([&Slot](const UStaticMeshComponent* Placed)
			{
				return Placed->DoesSocketExist(Slot.Socket);
			});
			if (Holder)
			{
				Parent = *Holder;
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
	}

	FindSocket(TEXT("Muzzle"), Muzzle);
	FindSocket(TEXT("Grip"), Grip);
	FindSocket(TEXT("Foregrip"), Foregrip);
	return !Parts.IsEmpty();
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
	for (const UStaticMeshComponent* Part : Parts)
	{
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

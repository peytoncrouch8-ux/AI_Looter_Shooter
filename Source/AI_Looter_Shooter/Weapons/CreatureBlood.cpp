#include "Weapons/CreatureBlood.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/DeadMinerCreature.h"
#include "Creatures/GravemotherCreature.h"
#include "Creatures/SlimeCreature.h"
#include "Creatures/SpiderCreature.h"
#include "Creatures/UnpaidCreature.h"
#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialInterface.h"

namespace
{
	// The kinds' own colors (linear). Ichor is the old Flesh splash's; gel is M_Gel's default Tint.
	const FLinearColor BloodIchor(0.55f, 0.66f, 0.1f);
	const FLinearColor BloodDarkIchor(0.13f, 0.17f, 0.025f);
	const FLinearColor BloodGel(0.48f, 0.87f, 0.25f);
	/** A Basic coal's dull red, for an Unpaid that can't say its own. */
	const FLinearColor BloodEmber(1.f, 0.32f, 0.08f);

	/** The slime's gel as its body's gel material tints it (the slot named for the gel, M_Gel's Tint), if it says. */
	bool GelTintOf(const ACreatureBase& Creature, FLinearColor& OutTint)
	{
		const USkeletalMeshComponent* Body = Creature.GetMesh();
		if (!Body)
		{
			return false;
		}
		const TArray<FName> Slots = Body->GetMaterialSlotNames();
		for (int32 Index = 0; Index < Body->GetNumMaterials(); ++Index)
		{
			// Only the gel's slot: the core and the pebbles caught in it wear a tinted world material with a Tint of its own.
			const bool bGelSlot = Slots.IsValidIndex(Index) && Slots[Index].ToString().Contains(TEXT("Gel"));
			const UMaterialInterface* Material = Body->GetMaterial(Index);
			if (bGelSlot && Material && Material->GetVectorParameterValue(FHashedMaterialParameterInfo(TEXT("Tint")), OutTint))
			{
				return true;
			}
		}
		return false;
	}
}

ECreatureBlood CreatureBlood::OfClass(const UClass* Class)
{
	if (!Class || !Class->IsChildOf(ACreatureBase::StaticClass()))
	{
		return ECreatureBlood::None;
	}
	// Most specific first: the Gravemother is a spider and the Foreman a dead miner, but each spills their own.
	if (Class->IsChildOf(AGravemotherCreature::StaticClass()))
	{
		return ECreatureBlood::DarkIchor;
	}
	if (Class->IsChildOf(ASpiderCreature::StaticClass()))
	{
		return ECreatureBlood::Ichor;
	}
	if (Class->IsChildOf(ASlimeCreature::StaticClass()))
	{
		return ECreatureBlood::Gel;
	}
	if (Class->IsChildOf(ADeadMinerCreature::StaticClass()))
	{
		return ECreatureBlood::MinerEmbers;
	}
	if (Class->IsChildOf(AUnpaidCreature::StaticClass()))
	{
		return ECreatureBlood::Embers;
	}
	// A creature with no look of its own yet: the old splash.
	return ECreatureBlood::Ichor;
}

ECreatureBlood CreatureBlood::Of(const AActor* Actor)
{
	return Actor ? OfClass(Actor->GetClass()) : ECreatureBlood::None;
}

FLinearColor CreatureBlood::DefaultTint(ECreatureBlood Blood)
{
	switch (Blood)
	{
	case ECreatureBlood::DarkIchor:
		return BloodDarkIchor;
	case ECreatureBlood::Gel:
		return BloodGel;
	case ECreatureBlood::Embers:
	case ECreatureBlood::MinerEmbers:
		return BloodEmber;
	case ECreatureBlood::Ichor:
	case ECreatureBlood::None:
	default:
		return BloodIchor;
	}
}

FLinearColor CreatureBlood::TintOf(const AActor* Actor)
{
	const ECreatureBlood Blood = Of(Actor);
	// The coal at its rank (dull red Basic, the rank's tag color above): the embers are its.
	if (const AUnpaidCreature* Unpaid = Cast<AUnpaidCreature>(Actor))
	{
		return Unpaid->GetCoalColor();
	}
	FLinearColor Gel;
	if (Blood == ECreatureBlood::Gel && GelTintOf(*CastChecked<ACreatureBase>(Actor), Gel))
	{
		return Gel;
	}
	return DefaultTint(Blood);
}

bool CreatureBlood::IsWet(ECreatureBlood Blood)
{
	return Blood == ECreatureBlood::Ichor || Blood == ECreatureBlood::DarkIchor || Blood == ECreatureBlood::Gel;
}

const TCHAR* CreatureBlood::Name(ECreatureBlood Blood)
{
	switch (Blood)
	{
	case ECreatureBlood::Ichor: return TEXT("Ichor");
	case ECreatureBlood::DarkIchor: return TEXT("DarkIchor");
	case ECreatureBlood::Gel: return TEXT("Gel");
	case ECreatureBlood::Embers: return TEXT("Embers");
	case ECreatureBlood::MinerEmbers: return TEXT("MinerEmbers");
	case ECreatureBlood::None:
	default: return TEXT("None");
	}
}

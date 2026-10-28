// AUnpaidCreature's look: its clothes, its coal's color, the rank's look and the dissolve through custom primitive data
// (UnpaidLook, M_Ghost), and its hit zones while it fades. Split from UnpaidCreature.cpp (2026-10-28, the ~500-line rule).

#include "Creatures/UnpaidCreature.h"
#include "Creatures/CreatureRankSettings.h"
#include "Creatures/CreatureTierLook.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"

namespace
{
	/** Its hit zones turn off once it's this far faded: a shot passes through what's mostly gone. */
	constexpr float GoneEnoughToMiss = 0.6f;
}

void AUnpaidCreature::PutOnClothes()
{
	const int32 Choice = Clothes >= 0 ? Clothes : FMath::RandRange(0, OtherClothes.Num());
	UMaterialInterface* Worn = Choice > 0 && OtherClothes.IsValidIndex(Choice - 1) ? OtherClothes[Choice - 1].Get() : nullptr;
	const int32 Slot = GetMesh()->GetMaterialIndex(BodySlot);
	if (Worn && Slot != INDEX_NONE)
	{
		GetMesh()->SetMaterial(Slot, Worn);
		// The hat goes with the clothes.
		const int32 HatSlot = Hat->GetMaterialIndex(BodySlot);
		if (HatSlot != INDEX_NONE)
		{
			Hat->SetMaterial(HatSlot, Worn);
		}
	}
}

FLinearColor AUnpaidCreature::GetCoalColor() const
{
	// Its tier's glow (FCreatureRankInfo::GlowColor, the tag's colour unless set apart); a Regular one's dull red.
	const ECreatureRank Rank = GetRank();
	return Rank == ECreatureRank::Basic ? BasicCoalColor : CreatureTierLook::GlowColor(Rank);
}

void AUnpaidCreature::ApplyCoalColor()
{
	// The coal and the ember edge both read it (M_Ghost), on every slot of the body and the hat: no material instance per
	// creature. The top tiers burn harder (CreatureTierLook::CoalGlow): Ethereal's red is a dark colour.
	const FLinearColor Color = GetCoalColor();
	const FVector4 RankColor(Color.R, Color.G, Color.B, CreatureTierLook::CoalGlow(GetRank(), Traits.CoalGlow));
	GetMesh()->SetCustomPrimitiveDataVector4(UnpaidLook::RankColorIndex, RankColor);
	Hat->SetCustomPrimitiveDataVector4(UnpaidLook::RankColorIndex, RankColor);
	for (UPrimitiveComponent* Part : LookParts)
	{
		if (Part)
		{
			Part->SetCustomPrimitiveDataVector4(UnpaidLook::RankColorIndex, RankColor);
		}
	}
}

void AUnpaidCreature::ApplyLook(bool bForce)
{
	// The hat dissolves with the body.
	USkeletalMeshComponent* Body = GetMesh();
	if (bForce || !FMath::IsNearlyEqual(PhaseAmount, ShownPhase, 0.002f))
	{
		Body->SetCustomPrimitiveDataFloat(UnpaidLook::PhaseIndex, PhaseAmount);
		Hat->SetCustomPrimitiveDataFloat(UnpaidLook::PhaseIndex, PhaseAmount);
		for (UPrimitiveComponent* Part : LookParts)
		{
			if (Part)
			{
				Part->SetCustomPrimitiveDataFloat(UnpaidLook::PhaseIndex, PhaseAmount);
			}
		}
		ShownPhase = PhaseAmount;
	}
	if (bForce || !FMath::IsNearlyEqual(Heat, ShownHeat, 0.01f))
	{
		Body->SetCustomPrimitiveDataFloat(UnpaidLook::HeatIndex, Heat);
		Hat->SetCustomPrimitiveDataFloat(UnpaidLook::HeatIndex, Heat);
		for (UPrimitiveComponent* Part : LookParts)
		{
			if (Part)
			{
				Part->SetCustomPrimitiveDataFloat(UnpaidLook::HeatIndex, Heat);
			}
		}
		ShownHeat = Heat;
	}
	// A shot passes through what has mostly faded away.
	const bool bOn = bHitVolumesWanted && PhaseAmount < GoneEnoughToMiss;
	if (bForce || bOn != bHitVolumesOn)
	{
		Body->SetCollisionEnabled(bOn ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
		bHitVolumesOn = bOn;
	}
}

void AUnpaidCreature::SetHitVolumesEnabled(bool bEnabled)
{
	bHitVolumesWanted = bEnabled;
	ApplyLook(true);
}

void AUnpaidCreature::OnRankChanged()
{
	// A Restless one's quicker lunge starts with a shorter shriek: its wind-up is a share of the class's (or the level's).
	Traits = UnpaidRules::RankTraits(GetRank());
	if (!bTimingCaptured)
	{
		bTimingCaptured = true;
		BaseWindup = AttackWindup;
		// The lunge's recovery, which its kit's rake replaces for a rake and puts back for the next lunge.
		Kit.BaseRecovery = AttackRecovery;
	}
	AttackWindup = BaseWindup * Traits.WindupScale;
	ApplyCoalColor();
}

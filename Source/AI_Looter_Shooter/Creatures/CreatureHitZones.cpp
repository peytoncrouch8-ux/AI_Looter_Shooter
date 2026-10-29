// CreatureHitZones (CreatureHitZones.h): which part of each kind a shot's bone is (HITS layer 2).

#include "Creatures/CreatureHitZones.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/GravemotherCreature.h"
#include "Creatures/SlimeCreature.h"
#include "Creatures/SpiderCreature.h"
#include "Creatures/SpitterSpider.h"
#include "Creatures/UnpaidCreature.h"
#include "Components/CapsuleComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"

namespace
{
	/** The spider's bones by name, as Art/Models/Creatures/Spider.py makes them (SpiderCreatureRig.cpp poses them). */
	const FName SpiderHead(TEXT("head"));
	const FName SpiderAbdomen(TEXT("abdomen"));
	/** The slime's core (SlimeCreature.h's CoreBone; Slime.py). */
	const FName SlimeCore(TEXT("core"));
	/** Any other kind's head, if its model names one so. */
	const FName AnyHead(TEXT("head"));

	bool StartsWith(FName Bone, const TCHAR* Prefix)
	{
		return Bone.ToString().StartsWith(Prefix, ESearchCase::IgnoreCase);
	}

	ECreatureHitZone SpiderZone(const ASpiderCreature& Spider, FName Bone)
	{
		if (Bone == SpiderHead || StartsWith(Bone, TEXT("fang_")) || StartsWith(Bone, TEXT("palp_")))
		{
			return ECreatureHitZone::Head;
		}
		if (Bone == SpiderAbdomen || Bone == AGravemotherCreature::AbdomenBone)
		{
			return ECreatureHitZone::Abdomen;
		}
		// The Spitter's venom sac: a weak spot of its own (crit or not, it wobbles and drips).
		if (Spider.IsA<ASpitterSpider>() && Bone == SpitterLook::SacBone)
		{
			return ECreatureHitZone::Weak;
		}
		if (CreatureHitZones::SpiderLegOf(Bone) != INDEX_NONE)
		{
			return ECreatureHitZone::Leg;
		}
		return ECreatureHitZone::Body;
	}

	bool InChain(const TArray<FName>& Chain, FName Bone)
	{
		return Chain.Contains(Bone);
	}

	bool IsArmBone(const FUnpaidArmBones& Arm, FName Bone)
	{
		return Bone == Arm.UpperArm || Bone == Arm.LowerArm || Bone == Arm.Hand || Arm.Fingers.Contains(Bone);
	}

	ECreatureHitZone UnpaidZone(const AUnpaidCreature& Unpaid, FName Bone, bool bCritical)
	{
		const FUnpaidRigBones& Rig = Unpaid.Rig;
		if (Bone == Rig.Head || Bone == Rig.Neck || Bone == Rig.Jaw || Bone == Rig.Hat)
		{
			return ECreatureHitZone::Head;
		}
		// The coal shows through the chest (AUnpaidCreature::IsCoalShotBone's bones): a crit there is the coal shot.
		if (Bone == Rig.Chest || Bone == Rig.Coal || Bone == Rig.Spine)
		{
			return bCritical ? ECreatureHitZone::Core : ECreatureHitZone::Body;
		}
		if (IsArmBone(Rig.LeftArm, Bone) || IsArmBone(Rig.RightArm, Bone))
		{
			return ECreatureHitZone::Arm;
		}
		if (InChain(Rig.Shroud, Bone) || InChain(Rig.LeftStrip, Bone) || InChain(Rig.RightStrip, Bone))
		{
			return ECreatureHitZone::Shroud;
		}
		return ECreatureHitZone::Body;
	}
}

ECreatureHitZone CreatureHitZones::ZoneOf(const ACreatureBase& Creature, FName Bone, bool bCritical)
{
	// The slime's crit is a shot through the gel onto the core (ASlimeCreature::IsCriticalSpot): its bone is the gel's.
	if (Creature.IsA<ASlimeCreature>())
	{
		return bCritical || Bone == SlimeCore ? ECreatureHitZone::Core : ECreatureHitZone::Body;
	}
	if (Bone.IsNone())
	{
		return ECreatureHitZone::Body;
	}
	if (const AUnpaidCreature* Unpaid = Cast<AUnpaidCreature>(&Creature))
	{
		return UnpaidZone(*Unpaid, Bone, bCritical);
	}
	if (const ASpiderCreature* Spider = Cast<ASpiderCreature>(&Creature))
	{
		return SpiderZone(*Spider, Bone);
	}
	return Bone == AnyHead ? ECreatureHitZone::Head : ECreatureHitZone::Body;
}

ECreatureHitZone CreatureHitZones::ZoneOfHit(const ACreatureBase& Creature, const UPrimitiveComponent* Component, FName Bone, bool bCritical)
{
	// The Unpaid's kin carry their weak spots as hit zones of their own (the dead miner's lamp, the Bell-Ringer's bell):
	// anything of theirs a shot meets besides the body and the capsule. Other kinds' extra parts (a placeholder's blocks)
	// are their body.
	const bool bOwnPart = Component && Component->GetOwner() == &Creature && Component != Creature.GetMesh()
		&& Component != Creature.GetCapsuleComponent();
	if (bOwnPart && Creature.IsA<AUnpaidCreature>())
	{
		return ECreatureHitZone::Weak;
	}
	return ZoneOf(Creature, Bone, bCritical);
}

int32 CreatureHitZones::SpiderLegOf(FName Bone)
{
	// femur_<pair>_<side>, tibia_..., foot_...: pair 0 (front) to 3 (back), side l or r.
	const FString Name = Bone.ToString();
	FString Rest;
	for (const TCHAR* Prefix : { TEXT("femur_"), TEXT("tibia_"), TEXT("foot_") })
	{
		if (Name.StartsWith(Prefix, ESearchCase::IgnoreCase))
		{
			Rest = Name.RightChop(FCString::Strlen(Prefix));
			break;
		}
	}
	if (Rest.Len() != 3 || Rest[1] != TCHAR('_') || !FChar::IsDigit(Rest[0]))
	{
		return INDEX_NONE;
	}
	const int32 Pair = Rest[0] - TCHAR('0');
	const TCHAR Side = FChar::ToLower(Rest[2]);
	if (Pair > 3 || (Side != TCHAR('l') && Side != TCHAR('r')))
	{
		return INDEX_NONE;
	}
	// As SpiderCreatureRig.cpp adds them: each pair's left, then its right.
	return Pair * 2 + (Side == TCHAR('r') ? 1 : 0);
}

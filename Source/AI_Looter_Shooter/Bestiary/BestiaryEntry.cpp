#include "Bestiary/BestiaryEntry.h"
#include "Combat/HealthComponent.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/CreatureRankSettings.h"
#include "Session/CampaignRecord.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Actor.h"
#include "Modules/ModuleManager.h"

FBestiaryStats UBestiaryEntry::ReadStats() const
{
	FBestiaryStats Stats;
	const UClass* Class = ActorClass.LoadSynchronous();
	const AActor* Defaults = Class ? Class->GetDefaultObject<AActor>() : nullptr;
	if (!Defaults)
	{
		return Stats;
	}
	if (const UHealthComponent* Health = Defaults->FindComponentByClass<UHealthComponent>())
	{
		Stats.bHasHealth = true;
		Stats.Health = Health->MaxHealth;
	}
	if (const ACreatureBase* Creature = Cast<ACreatureBase>(Defaults))
	{
		// A creature that is always of one rank (the Gravemother, Legendary) shows what that rank makes of it, as it's
		// met; a Basic one, its own numbers (a promotion is luck, not the creature).
		const FCreatureRankInfo& Rank = UCreatureRankSettings::Get(Creature->StartingRank);
		Stats.Level = Creature->Level;
		Stats.Health *= Rank.HealthMultiplier;
		Stats.AttackDamage = Creature->AttackDamage * Rank.DamageMultiplier;
		Stats.XPReward = FMath::Max(0, FMath::RoundToInt32(Creature->XPReward * Rank.XPMultiplier));
		Stats.bAttacks = Creature->AttackDamage > 0.f;
	}
	return Stats;
}

namespace
{
	/** The actor's body: the first skeletal mesh component on its defaults that has a mesh (not always the main one). */
	const USkeletalMeshComponent* FindBody(const TSoftClassPtr<AActor>& ActorClass)
	{
		const UClass* Class = ActorClass.LoadSynchronous();
		const AActor* Defaults = Class ? Class->GetDefaultObject<AActor>() : nullptr;
		if (!Defaults)
		{
			return nullptr;
		}
		TInlineComponentArray<USkeletalMeshComponent*> Meshes(Defaults);
		for (const USkeletalMeshComponent* Component : Meshes)
		{
			if (Component->GetSkeletalMeshAsset())
			{
				return Component;
			}
		}
		return nullptr;
	}
}

USkeletalMesh* UBestiaryEntry::LoadPreviewMesh() const
{
	if (USkeletalMesh* Mesh = PreviewMesh.LoadSynchronous())
	{
		return Mesh;
	}
	const USkeletalMeshComponent* Body = FindBody(ActorClass);
	return Body ? Body->GetSkeletalMeshAsset() : nullptr;
}

UStaticMesh* UBestiaryEntry::LoadPreviewStaticMesh() const
{
	return PreviewStaticMesh.LoadSynchronous();
}

TArray<UMaterialInterface*> UBestiaryEntry::GetPreviewMaterials(const USkeletalMesh* Mesh) const
{
	TArray<UMaterialInterface*> Materials;
	const USkeletalMeshComponent* Body = FindBody(ActorClass);
	if (Body && Mesh && Body->GetSkeletalMeshAsset() == Mesh)
	{
		for (int32 Slot = 0; Slot < Body->GetNumMaterials(); ++Slot)
		{
			Materials.Add(Body->GetMaterial(Slot));
		}
	}
	return Materials;
}

TArray<FBestiaryStandPart> UBestiaryEntry::GetPreviewParts(const USkeletalMesh* Mesh) const
{
	TArray<FBestiaryStandPart> Parts;
	const USkeletalMeshComponent* Body = FindBody(ActorClass);
	if (!Body || !Mesh || Body->GetSkeletalMeshAsset() != Mesh)
	{
		return Parts;
	}
	// Every static mesh the defaults wear on one of the body's bones.
	TInlineComponentArray<UStaticMeshComponent*> Worn(Body->GetOwner());
	for (const UStaticMeshComponent* Part : Worn)
	{
		UStaticMesh* PartMesh = Part->GetStaticMesh();
		const FName Bone = Part->GetAttachSocketName();
		if (!PartMesh || !Part->GetVisibleFlag() || Part->GetAttachParent() != Body || Bone.IsNone()
			|| Mesh->GetRefSkeleton().FindBoneIndex(Bone) == INDEX_NONE)
		{
			continue;
		}
		FBestiaryStandPart& Stand = Parts.AddDefaulted_GetRef();
		Stand.Mesh = PartMesh;
		Stand.Bone = Bone;
		Stand.Relative = Part->GetRelativeTransform();
		for (int32 Slot = 0; Slot < Part->GetNumMaterials(); ++Slot)
		{
			Stand.Materials.Add(Part->GetMaterial(Slot));
		}
	}
	return Parts;
}

bool UBestiaryEntry::IsListed(bool bLedgerOpen) const
{
	// The Ledger's own pages wait for Sexton to hand it over; the rest were the bestiary's before it was his.
	return bLedgerOpen || (!bLedgerOnly && Page != EBestiaryPage::LedgerName);
}

bool UBestiaryEntry::IsKnown(bool bMet, const FCampaignRecord& Campaign, const UMissionRunner* Runner) const
{
	switch (Page)
	{
	case EBestiaryPage::LedgerName:
		// The name is written in from the first: that's the debt. Only where they are waits.
		return true;
	case EBestiaryPage::StoryCharacter:
		return KnownWhen.IsMet(Campaign, Runner);
	case EBestiaryPage::Actor:
		break;
	}
	return bMet || (!KnownWhen.IsEmpty() && KnownWhen.IsMet(Campaign, Runner));
}

bool UBestiaryEntry::IsFound(const FCampaignRecord& Campaign, const UMissionRunner* Runner) const
{
	return Page == EBestiaryPage::LedgerName && !Habitat.IsEmpty() && !FoundWhen.IsEmpty() && FoundWhen.IsMet(Campaign, Runner);
}

bool UBestiaryEntry::Describes(const UClass* ActorType) const
{
	const UClass* Class = ActorClass.Get();
	return Class && ActorType && ActorType->IsChildOf(Class);
}

TArray<UBestiaryEntry*> UBestiaryEntry::LoadAll()
{
	// Every entry asset wherever it lives, so a new page is just a new data asset.
	IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
	TArray<FAssetData> Assets;
	Registry.GetAssetsByClass(StaticClass()->GetClassPathName(), Assets, true);

	TArray<UBestiaryEntry*> Entries;
	for (const FAssetData& Asset : Assets)
	{
		if (UBestiaryEntry* Entry = Cast<UBestiaryEntry>(Asset.GetAsset()))
		{
			Entries.Add(Entry);
		}
	}
	Entries.Sort([](const UBestiaryEntry& A, const UBestiaryEntry& B)
	{
		if (A.Category != B.Category)
		{
			return A.Category < B.Category;
		}
		if (A.SortOrder != B.SortOrder)
		{
			return A.SortOrder < B.SortOrder;
		}
		return A.DisplayName.CompareTo(B.DisplayName) < 0;
	});
	return Entries;
}

FText UBestiaryEntry::CategoryName(EBestiaryCategory Category)
{
	switch (Category)
	{
	case EBestiaryCategory::Creature: return NSLOCTEXT("Bestiary", "Creatures", "Creatures");
	case EBestiaryCategory::Enemy:    return NSLOCTEXT("Bestiary", "Enemies", "Enemies");
	case EBestiaryCategory::NPC:      return NSLOCTEXT("Bestiary", "NPCs", "NPCs");
	case EBestiaryCategory::Friend:   return NSLOCTEXT("Bestiary", "Friends", "Friends");
	}
	return FText::GetEmpty();
}

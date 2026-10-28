#include "Bestiary/BestiaryEntry.h"
#include "Areas/AreaDefinition.h"
#include "Combat/HealthComponent.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/CreatureRankSettings.h"
#include "Creatures/FightRules.h"
#include "Creatures/FightTuning.h"
#include "Progression/PlayerProgressionSubsystem.h"
#include "Session/CampaignRecord.h"
#include "AnimationRuntime.h"
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
		// met; a Basic one, its own numbers (a promotion is luck, not the creature). A kind at a set level in its area (a
		// boss or a Legendary monster: UAreaDefinition::KindLevels, Abel 6 and the Gravemother 4 on Ransom's Rest) shows
		// that level with its rank's on top and its numbers grown for it, as it's met (Abel 7); any other kind's level is
		// where it's met (its zone's), so its page keeps its class's.
		const FCreatureRankInfo& Rank = UCreatureRankSettings::Get(Creature->StartingRank);
		const FLevelRules Rules = UPlayerProgressionSubsystem::GetLevelRules();
		const int32 SetLevel = UAreaDefinition::FindKindLevelInAnyArea(Class);
		Stats.Level = SetLevel > 0 ? SetLevel + Rank.LevelOffset : Creature->Level;
		const float Growth = SetLevel > 0 ? Rules.EnemyScale(Stats.Level) : 1.f;
		Stats.Health *= Rank.HealthMultiplier * Growth;
		// The bite as it lands: a regular kind hits Looter.Fight.DamageScale harder (1.8), so the page said 11 where a spider
		// took 20 off the player (playtest 2026-10-28). A boss or Legendary monster keeps its own.
		const float FightScale = FightRules::DamageScaleFor(FightRules::KeepsOwnFight(Creature->StartingRank, Creature->IsBossCreature()),
			FFightTuning::LiveDamageScale());
		Stats.AttackDamage = Creature->AttackDamage * Rank.DamageMultiplier * Growth * FightScale;
		// What a kill really gives a player of its level: the kill rules grow the creature's XPReward with its level (Abel's
		// 200 is 2,066 at his 7), so the page shows what the kill pays.
		const int32 BaseXP = FMath::Max(0, FMath::RoundToInt32(Creature->XPReward * Rank.XPMultiplier));
		Stats.XPReward = static_cast<int32>(Rules.KillXP(BaseXP, Stats.Level, Stats.Level));
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
	if (!Mesh)
	{
		return Parts;
	}
	// The page's own, for a figure with no actor: each as the model has it at rest, at its bone and turned back by the
	// bone's rest turn (as the Unpaid wear their hats).
	for (const FBestiaryBonePart& Worn : PreviewBoneParts)
	{
		UStaticMesh* PartMesh = Worn.Mesh.LoadSynchronous();
		const int32 Bone = Mesh->GetRefSkeleton().FindBoneIndex(Worn.Bone);
		if (!PartMesh || Bone == INDEX_NONE)
		{
			continue;
		}
		FBestiaryStandPart& Stand = Parts.AddDefaulted_GetRef();
		Stand.Mesh = PartMesh;
		Stand.Bone = Worn.Bone;
		Stand.Relative = FTransform(FAnimationRuntime::GetComponentSpaceTransformRefPose(Mesh->GetRefSkeleton(), Bone).GetRotation().Inverse());
	}
	const USkeletalMeshComponent* Body = FindBody(ActorClass);
	if (!Body || Body->GetSkeletalMeshAsset() != Mesh)
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

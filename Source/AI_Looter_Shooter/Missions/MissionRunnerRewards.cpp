// UMissionRunner: what a mission's end gives (its rewards, or a turn-in choice's in their place, and the optional objectives'
// bonus), turning in with a choice, the story's flags, and forgetting what the campaign kept for a mission under way.

#include "Missions/MissionRunner.h"
#include "AI_Looter_Shooter.h"
#include "Affixes/WeaponRollLibrary.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionRewards.h"
#include "Progression/PlayerProgressionSubsystem.h"
#include "Weapons/WeaponBase.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"

// ---------------------------------------------------------------------------
// Rewards
// ---------------------------------------------------------------------------

int32 UMissionRunner::GetPlayerLevel() const
{
	const FMissionContext Context = MakeContext();
	const ULocalPlayer* LocalPlayer = Context.Controller ? Context.Controller->GetLocalPlayer() : nullptr;
	const UPlayerProgressionSubsystem* Progression = LocalPlayer ? LocalPlayer->GetSubsystem<UPlayerProgressionSubsystem>() : nullptr;
	return Progression ? FMath::Max(Progression->GetLevel(), 1) : 1;
}

FMissionRewardsGiven UMissionRunner::GrantRewards(const UMissionDefinition& Mission, int32 Choice)
{
	// A choice's rewards stand in for the mission's own (its experience the mission's unless it names its own).
	const FMissionRewards Rewards = Mission.RewardsFor(Choice);
	const FMissionTurnInChoice* Chosen = Mission.GetTurnInChoice(Choice);
	const FMissionContext Context = MakeContext();
	FMissionRewardsGiven Given;
	Given.Choice = Chosen ? Chosen->Title : FText::GetEmpty();

	// Experience: the mission's fixed amount (or its share of the level the player is on). This and a kill are the only
	// ways experience is earned: never for an objective or a step on the way, optional ones included. Without a player (a
	// test level) it's still worked out, at level 1, for whoever hears what the mission gave.
	const ULocalPlayer* LocalPlayer = Context.Controller ? Context.Controller->GetLocalPlayer() : nullptr;
	UPlayerProgressionSubsystem* Progression = LocalPlayer ? LocalPlayer->GetSubsystem<UPlayerProgressionSubsystem>() : nullptr;
	const int32 PlayerLevel = GetPlayerLevel();
	Given.Experience = MissionRewards::ExperienceOf(Rewards, PlayerLevel, UPlayerProgressionSubsystem::GetCurve());
	if (Progression && Given.Experience > 0)
	{
		Progression->AddXP(Given.Experience, EXPSource::Mission);
	}

	// Silver: the purse (the rewards' own, else the giver's kept by mission id; a choice's never falls back to that), and the
	// optional objectives' bonus with it, Borderlands' way: each done adds its share, grown with the player's level.
	const FName PurseId = Chosen ? FName(NAME_None) : Mission.GetMissionId();
	const int32 Purse = MissionRewards::PurseOf(Rewards, PurseId);
	Given.OptionalDone = GetCampaign().CountOptionalDone(Mission.GetMissionId());
	const int32 Bonus = MissionRewards::OptionalBonus(Rewards, Given.OptionalDone, PlayerLevel);
	Given.Silver = MissionRewards::GiveAmount(Context.World, Purse + Bonus, Mission.GetMissionId());
	Given.OptionalSilver = FMath::Min(Bonus, Given.Silver);
	if (Given.OptionalDone > 0)
	{
		UE_LOG(LogLooter, Log, TEXT("Missions: %s: %d optional objective%s done, +%d silver."), *Mission.GetMissionId().ToString(),
			Given.OptionalDone, Given.OptionalDone == 1 ? TEXT("") : TEXT("s"), Bonus);
	}

	// A gun in front of the player, at their level, and a named gun the same way. One the story hands over itself (Heirloom
	// through Delia's door) is made sure of rather than dropped again: brought to them if it was never taken, given if it was
	// never handed out, nothing if they carry it.
	if (Rewards.bGun)
	{
		if (const AWeaponBase* Gun = MissionRewards::DropGun(Context.World, Rewards, PlayerLevel, Context.Player))
		{
			const FWeaponInstanceData& Instance = Gun->GetInstance();
			Given.bGun = true;
			Given.GunRarity = Instance.Rarity;
			Given.GunColor = UWeaponRollLibrary::GetRarityColor(Instance.Definition, Instance.Rarity);
		}
	}
	if (!Rewards.NamedGun.IsNone())
	{
		if (Rewards.bNamedGunByHand)
		{
			MissionRewards::EnsureNamedGun(Context.World, Rewards, PlayerLevel, Context.Player);
		}
		else
		{
			MissionRewards::DropNamedGun(Context.World, Rewards, PlayerLevel, Context.Player);
		}
		Given.NamedGun = MissionRewards::NamedGunName(Rewards);
	}

	// Areas opened to travel.
	FCampaignRecord& Campaign = GetCampaign();
	for (const FName AreaId : Rewards.UnlockAreas)
	{
		if (Campaign.OpenArea(AreaId))
		{
			Given.AreasOpened.Add(MissionRewards::AreaName(AreaId));
			UE_LOG(LogLooter, Log, TEXT("Missions: %s opens %s"), *Mission.GetMissionId().ToString(), *AreaId.ToString());
		}
	}
	return Given;
}

void UMissionRunner::ForgetMissionThings(const UMissionDefinition& Mission)
{
	FCampaignRecord& Campaign = GetCampaign();
	for (const FMissionItemInfo& Item : Mission.Items)
	{
		if (Campaign.RemoveItem(Item.Id))
		{
			UE_LOG(LogLooter, Log, TEXT("Missions: %s handed over with %s."), *Item.Id.ToString(), *Mission.GetMissionId().ToString());
		}
		Campaign.DroppedItems.Remove(Item.Id);
	}
	Campaign.ClearOptionals(Mission.GetMissionId());
}

// ---------------------------------------------------------------------------
// Turning in with a choice
// ---------------------------------------------------------------------------

bool UMissionRunner::TurnInWithChoice(FName MissionId, int32 Choice)
{
	const FRun* Run = FindRun(MissionId);
	const UMissionDefinition* Mission = Run ? Run->Mission.Get() : nullptr;
	const FMissionTurnInChoice* Chosen = Mission ? Mission->GetTurnInChoice(Choice) : nullptr;
	if (!bActive || !Run || !Run->bReady || !Chosen)
	{
		return false;
	}
	UE_LOG(LogLooter, Log, TEXT("Mission %s turned in: %s (%s)"), *MissionId.ToString(), *Chosen->Title.ToString(),
		Chosen->Flag.IsNone() ? TEXT("no flag") : *Chosen->Flag.ToString());
	FinishRun(MissionId, Choice);
	// What it was the last prerequisite of starts now, and whatever reads the flag (an encounter, a speaker's topic) hears.
	StartDue(NAME_None);
	bChanged = true;
	AfterChange();
	return true;
}

int32 UMissionRunner::GetTurnInChoice(FName MissionId) const
{
	return GetCampaign().GetTurnInChoice(MissionId);
}

// ---------------------------------------------------------------------------
// The story's flags
// ---------------------------------------------------------------------------

bool UMissionRunner::HasFlag(FName Flag) const
{
	return GetCampaign().HasFlag(Flag);
}

bool UMissionRunner::SetFlag(FName Flag)
{
	if (!GetCampaign().SetFlag(Flag))
	{
		return false;
	}
	UE_LOG(LogLooter, Log, TEXT("Story flag %s set."), *Flag.ToString());
	AskToSave();
	// Listeners that read flags (a lantern's look, an encounter's condition) look again.
	bChanged = true;
	if (bActive)
	{
		AfterChange();
	}
	return true;
}

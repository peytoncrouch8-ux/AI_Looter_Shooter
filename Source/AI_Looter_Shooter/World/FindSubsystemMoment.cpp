#include "World/FindSubsystem.h"
#include "AI_Looter_Shooter.h"
#include "Affixes/WeaponRollLibrary.h"
#include "Areas/AreaDefinition.h"
#include "Areas/AreaRulesSubsystem.h"
#include "Audio/LooterSound.h"
#include "Economy/SilverPickup.h"
#include "Loot/LootFanfareSubsystem.h"
#include "Missions/MissionRewards.h"
#include "Missions/MissionRunner.h"
#include "Progression/PlayerProgressionSubsystem.h"
#include "Session/CampaignRecord.h"
#include "Story/CaptionSubsystem.h"
#include "Story/StoryLine.h"
#include "UI/HUD/FindBannerShow.h"
#include "UI/HUD/LooterHUD.h"
#include "Weapons/NamedWeaponDefinition.h"
#include "Weapons/WeaponBase.h"
#include "World/FindSpot.h"
#include "World/RoostCrows.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

// UFindSubsystem's moment of a find: the flag, the banner and its cue, the silver and the named gun at the source, and the
// experience and Hob's lines once the banner is done (so LEVEL UP follows the find, never cuts it).

#define LOCTEXT_NAMESPACE "LooterFinds"

namespace
{
	/** Silver comes up a little over the source and is tossed out and up, to land by it (cm, cm/s). */
	constexpr double SilverRise = 60.0;
	constexpr double SilverTossOut = 120.0;
	constexpr double SilverTossUp = 380.0;
	/** A named gun dropped where no actor stands to drop it by (a body already gone): up from the spot and tossed up (cm, cm/s). */
	constexpr double GunRise = 80.0;
	constexpr double GunTossUp = 420.0;
	/** Several finds at once (the console) ring the Discover cue once: this long apart at least (s). */
	constexpr float CueGap = 0.5f;
}

bool UFindSubsystem::Found(AFindSpot& Spot, bool bAnnounce, const AActor* At)
{
	FCampaignRecord* Campaign = GetCampaign();
	if (!Campaign || IsFound(Spot.FindId))
	{
		return false;
	}
	// Kept by the story, so it's saved with the session (the runner asks the session to save soon) and an old save reads it.
	const FName Flag = FindRules::FlagFor(Spot.FindId);
	if (UMissionRunner* Runner = GetRunner())
	{
		Runner->SetFlag(Flag);
	}
	else
	{
		Campaign->SetFlag(Flag);
	}
	Spot.SetTellsShown(false);
	if (FSpotState* State = FindState(Spot.FindId))
	{
		State->GlintAge = -1.f;
		if (bAnnounce)
		{
			SendCrowsOff(*State);
		}
		else
		{
			UpdateTellsFor(*State);
		}
	}
	const bool bLore = !FindRules::CountsInFinds(Spot.Kind);
	const FFindTally Tally = bLore ? GetLoreTally() : GetTally();
	UE_LOG(LogLooter, Log, TEXT("Finds: %s found (%s), %d / %d%s."), *Spot.FindId.ToString(), *Spot.ToEntry().Name.ToString(), Tally.Found,
		Tally.Total, bAnnounce ? TEXT("") : TEXT(", silently"));
	if (bAnnounce)
	{
		Announce(Spot, At);
	}
	OnFound.Broadcast(Spot.FindId, Tally.Found, Tally.Total);
	return true;
}

int32 UFindSubsystem::ZoneLevelOf(const AFindSpot& Spot) const
{
	if (Spot.ZoneLevel > 0)
	{
		return Spot.ZoneLevel;
	}
	// The area's zone at the spot, at its top: a find pays what the ground round it is worth, never what the player is.
	const UAreaRulesSubsystem* Rules = GetWorld() ? GetWorld()->GetSubsystem<UAreaRulesSubsystem>() : nullptr;
	const UAreaDefinition* Area = Rules ? Rules->GetArea() : nullptr;
	if (!Area)
	{
		return 1;
	}
	int32 Lowest = 1;
	int32 Highest = 1;
	Area->GetLevelsAt(Spot.GetActorLocation(), Lowest, Highest);
	return FMath::Max(Highest, 1);
}

FText UFindSubsystem::AreaName() const
{
	const UAreaRulesSubsystem* Rules = GetWorld() ? GetWorld()->GetSubsystem<UAreaRulesSubsystem>() : nullptr;
	const UAreaDefinition* Area = Rules ? Rules->GetArea() : nullptr;
	return Area ? Area->DisplayName : FText::GetEmpty();
}

void UFindSubsystem::Announce(AFindSpot& Spot, const AActor* At)
{
	const FVector Where = At ? At->GetActorLocation() : Spot.GetRewardLocation();
	GiveSilver(Spot, Where);
	GiveNamedGun(Spot, At, Where);

	const bool bLore = !FindRules::CountsInFinds(Spot.Kind);
	const FFindTally Tally = bLore ? GetLoreTally() : GetTally();
	const int64 Xp = FindRules::XpFor(Spot.Kind, ZoneLevelOf(Spot));
	FFindBannerShow Words;
	Words.Name = Spot.ToEntry().Name;
	Words.Count = bLore ? FindRules::LoreLine(Tally.Found, Tally.Total) : FindRules::CountLine(Tally.Found, Tally.Total, AreaName());
	Words.Xp = static_cast<int32>(Xp);
	// The sound the moment it counts (the banner may wait behind a menu); several at once ring once.
	if (Clock - LastCueAt >= CueGap)
	{
		LooterSound::Play2D(this, FindCue::Discover);
		LastCueAt = Clock;
	}
	Show(Words, Spot.FindId, Xp, /*bComplete*/ false);

	// The area's every find: its own banner after this one's, its cue as it comes up, a thousand on top and Hob's word.
	if (!bLore && Tally.IsComplete())
	{
		FFindBannerShow All;
		All.Title = FindRules::CompleteTitle(AreaName());
		All.Count = FindRules::CountLine(Tally.Found, Tally.Total, AreaName());
		All.Xp = static_cast<int32>(FindRules::CompletionXP);
		Show(All, NAME_None, FindRules::CompletionXP, /*bComplete*/ true);
		bCompleteWaiting = true;
		UE_LOG(LogLooter, Log, TEXT("Finds: every find in the area made (%d)."), Tally.Total);
	}
}

void UFindSubsystem::GiveSilver(const AFindSpot& Spot, const FVector& Where)
{
	UWorld* World = GetWorld();
	const int32 Amount = FindRules::SilverAt(Spot.Silver, ZoneLevelOf(Spot));
	if (!World || Amount <= 0)
	{
		return;
	}
	ASilverPickup* Silver = ASilverPickup::SpawnSilver(World, Amount, Where + FVector(0.0, 0.0, SilverRise));
	if (Silver)
	{
		const FVector Out = FVector(FMath::FRandRange(-1.f, 1.f), FMath::FRandRange(-1.f, 1.f), 0.f).GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector);
		Silver->Toss(Out * SilverTossOut + FVector(0.0, 0.0, SilverTossUp));
	}
}

void UFindSubsystem::GiveNamedGun(const AFindSpot& Spot, const AActor* At, const FVector& Where)
{
	UWorld* World = GetWorld();
	if (!World || Spot.NamedGun.IsNone())
	{
		return;
	}
	UNamedWeaponDefinition* Named = UNamedWeaponDefinition::FindByName(Spot.NamedGun.ToString());
	if (!Named || !Named->Weapon)
	{
		UE_LOG(LogLooter, Warning, TEXT("Finds: no named gun %s (%s%s, made by Tools/Unreal/create_named_weapons.py), so %s gives none."),
			*Spot.NamedGun.ToString(), UNamedWeaponDefinition::AssetPrefix, *Spot.NamedGun.ToString(), *Spot.FindId.ToString());
		return;
	}
	// One of a kind: never a second while the first is carried, lying here or kept at Lost Property.
	if (MissionRewards::FindNamedGun(World, Named, GetPlayer()).IsAnywhere())
	{
		UE_LOG(LogLooter, Log, TEXT("Finds: %s is had already: %s gives no second."), *Named->GetNamedId().ToString(), *Spot.FindId.ToString());
		return;
	}
	// Its own parts, rarity and fixed stats; only its level is the player's, as every reward gun's is.
	const UPlayerProgressionSubsystem* Progression = GetProgression();
	const FWeaponInstanceData Instance = Named->MakeInstance(Progression ? FMath::Max(Progression->GetLevel(), 1) : 1);
	// By the source as a reward is handed over: out of a chest's front, by the body, kept on this side of any wall.
	AWeaponBase* Gun = At ? MissionRewards::DropInFront(World, Instance, *At) : nullptr;
	if (!Gun)
	{
		Gun = UWeaponRollLibrary::SpawnWeapon(World, Instance, FTransform(FRotator(0.f, FMath::FRandRange(0.f, 360.f), 0.f), Where + FVector(0.0, 0.0, GunRise)));
		if (Gun)
		{
			Gun->Toss(FVector(0.0, 0.0, GunTossUp));
			ULootFanfareSubsystem::ExpectLanding(Gun);
			MissionRewards::LayFlatOnLanding(*Gun);
		}
	}
	UE_LOG(LogLooter, Log, TEXT("Finds: %s gives the named gun %s%s."), *Spot.FindId.ToString(), *Named->GetNamedId().ToString(),
		Gun ? TEXT("") : TEXT(" (couldn't be spawned)"));
}

void UFindSubsystem::Show(const FFindBannerShow& Words, FName FindId, int64 Xp, bool bComplete)
{
	FPendingShow& Waiting = Pending.AddDefaulted_GetRef();
	Waiting.FindId = FindId;
	Waiting.Xp = Xp;
	Waiting.bComplete = bComplete;
	if (bTesting)
	{
		return;
	}
	// The HUD queues it behind banners showing and holds it under menus and scenes; it tells BannerDone as it fades.
	const UWorld* World = GetWorld();
	const AActor* Player = GetPlayer();
	const AActor* Who = Player ? Player : (World ? World->GetFirstPlayerController() : nullptr);
	ALooterHUD* HUD = ALooterHUD::FindFor(Who);
	if (HUD && HUD->GetHUDWidget())
	{
		HUD->ShowFind(Words);
		Waiting.bOnHud = true;
	}
}

void UFindSubsystem::UpdatePending(float DeltaSeconds)
{
	if (Pending.IsEmpty())
	{
		return;
	}
	// Banners show one after another, so only the first waits: without a HUD its length, with one until it says it's done
	// (or, should it never say, a long while: experience is never lost).
	FPendingShow& Head = Pending[0];
	Head.Age += DeltaSeconds;
	if ((!Head.bOnHud && Head.Age >= FindRules::BannerSeconds) || Head.Age >= FindRules::BannerWaitLimit)
	{
		FinishShows(1);
	}
}

void UFindSubsystem::FinishShows(int32 Count)
{
	UCaptionSubsystem* Captions = UCaptionSubsystem::Get(this);
	for (int32 Done = 0; Done < Count && !Pending.IsEmpty(); ++Done)
	{
		const FPendingShow Show = Pending[0];
		Pending.RemoveAt(0);
		PayXp(Show.Xp);
		if (Show.bComplete)
		{
			if (bCompleteWaiting)
			{
				// Merged into one banner with the last find's: its cue still rings.
				LooterSound::Play2D(this, FindCue::Complete);
				bCompleteWaiting = false;
			}
			if (Captions)
			{
				Captions->Play({ FStoryLine::Make(LOCTEXT("Hob", "Hob"), FindRules::CompletionWords()) }, ECaptionPlay::Queue);
			}
			continue;
		}
		const AFindSpot* Spot = FindSpot(Show.FindId);
		if (Captions && Spot && !Spot->Lines.IsEmpty())
		{
			Captions->Play(Spot->Lines, ECaptionPlay::Queue);
		}
	}
	// The completion's banner comes up next: its cue with it.
	if (bCompleteWaiting && !Pending.IsEmpty() && Pending[0].bComplete)
	{
		LooterSound::Play2D(this, FindCue::Complete);
		bCompleteWaiting = false;
	}
}

void UFindSubsystem::PayXp(int64 Xp)
{
	if (Xp <= 0)
	{
		return;
	}
	XpPaid += Xp;
	if (UPlayerProgressionSubsystem* Progression = GetProgression())
	{
		Progression->AddXP(Xp, EXPSource::Find);
	}
}

#undef LOCTEXT_NAMESPACE

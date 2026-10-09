#include "Creatures/CreaturePackComponent.h"
#include "AI_Looter_Shooter.h"
#include "Audio/CreatureVoiceComponent.h"
#include "Audio/LooterSound.h"
#include "Combat/HealthComponent.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/EncounterSettings.h"
#include "Creatures/EncounterSubsystem.h"
#include "Creatures/PackRules.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"

namespace
{
	/** How often (s) a chaser looks at its pack again: often enough to follow a fight, rarely enough to cost nothing. */
	constexpr float PackLookInterval = 0.4f;

	/** "A high rank appears": a ranked creature's first sight of the player. A cue without sounds plays nothing (list it in LooterSoundCues.h). */
	const FName RankStingCue(TEXT("Creature.RankSting"));

	/** A roaming creature lets its anchor walk this far off (cm, at size 1) before it sets off after it. */
	constexpr float RoamSlack = 150.f;

	/** Keeping pace: it walks at this many times its gap to the anchor (per second), between a stroll and its walking pace. */
	constexpr float RoamCatchUpRate = 1.5f;
	constexpr float RoamSlowestShare = 0.35f;

	/** Its break-off cry, a little quieter than a hurt cry: a whimper, not a scream. */
	constexpr float RetreatCryVolume = 0.8f;

	bool IsHunting(ECreatureState State)
	{
		return State == ECreatureState::Chase || State == ECreatureState::Attack;
	}

	/** A boss or a Legendary monster holds the middle of its fight: it's no one's flank, and flanks nobody. */
	bool CanFlank(const ACreatureBase& Creature)
	{
		const ECreatureRank Rank = Creature.GetRank();
		return Rank != ECreatureRank::Boss && Rank != ECreatureRank::Legendary && !Creature.IsPassive();
	}
}

UCreaturePackComponent::UCreaturePackComponent()
{
	// The brain calls it (ACreatureBase::TickBrain, SetState): it never ticks.
	PrimaryComponentTick.bCanEverTick = false;
}

void UCreaturePackComponent::BeginPlay()
{
	Super::BeginPlay();
	// Each creature rolls on its own stream, the same each time it lives (the tests seed it themselves).
	Rolls.Initialize(static_cast<int32>(GetUniqueID()));
	NextLook = FMath::FRandRange(0.f, PackLookInterval);
}

ACreatureBase* UCreaturePackComponent::GetCreature() const
{
	return Cast<ACreatureBase>(GetOwner());
}

void UCreaturePackComponent::SetSeed(int32 Seed)
{
	Rolls.Initialize(Seed);
}

// ---------------------------------------------------------------------------
// Its brain's changes
// ---------------------------------------------------------------------------

void UCreaturePackComponent::HandleStateChanged(ECreatureState OldState, ECreatureState NewState)
{
	if (NewState == ECreatureState::Chase)
	{
		if (!IsHunting(OldState))
		{
			// A new hunt: look at the pack at once, so the flank is there from its first stride.
			NextLook = 0.f;
		}
		TryRankSting();
	}
	else if (!IsHunting(NewState))
	{
		// Let go (home, calm, or dead): no flank and no running until the next hunt.
		FlankDegrees = 0.f;
		RetreatLeft = 0.f;
	}
}

void UCreaturePackComponent::ResetLife()
{
	FlankDegrees = 0.f;
	NextLook = 0.f;
	RetreatLeft = 0.f;
	bHadPack = false;
	bRetreatRolled = false;
	bRankStung = false;
}

// ---------------------------------------------------------------------------
// While it chases
// ---------------------------------------------------------------------------

void UCreaturePackComponent::TickChase(float DeltaSeconds)
{
	RetreatLeft = FMath::Max(RetreatLeft - DeltaSeconds, 0.f);
	NextLook -= DeltaSeconds;
	if (NextLook <= 0.f)
	{
		NextLook = PackLookInterval;
		LookAtPack();
	}
}

void UCreaturePackComponent::LookAtPackNow()
{
	NextLook = PackLookInterval;
	LookAtPack();
}

void UCreaturePackComponent::LookAtPack()
{
	ACreatureBase* Me = GetCreature();
	UWorld* World = GetWorld();
	const APawn* Victim = Me ? Me->GetTarget() : nullptr;
	if (!Me || !World || !Victim || Me->IsDead())
	{
		FlankDegrees = 0.f;
		return;
	}
	const UEncounterSettings& Settings = UEncounterSettings::Get();
	const FVector Here = Me->GetActorLocation();
	const FVector Prey = Victim->GetActorLocation();
	const double ReachSquared = FMath::Square(static_cast<double>(Settings.PackRadius));
	const bool bFlanks = CanFlank(*Me) && Settings.FlankMaxDegrees > 0.f;

	// Its packmates near it, alive; and of them, those chasing the same player beside it (itself first).
	int32 Packmates = 0;
	TArray<float> Bearings = { PackRules::BearingAround(Prey, Here) };
	TArray<uint32> TieBreaks = { Me->GetUniqueID() };
	auto Consider = [&](const ACreatureBase* Other)
	{
		if (Other == Me || Other->IsDead() || Other->IsHidden() || Other->IsActorBeingDestroyed() || !Other->SharesPackWith(*Me)
			|| FVector::DistSquared(Other->GetActorLocation(), Here) > ReachSquared)
		{
			return;
		}
		++Packmates;
		if (bFlanks && CanFlank(*Other) && Other->GetTarget() == Victim && IsHunting(Other->GetCreatureState()))
		{
			Bearings.Add(PackRules::BearingAround(Prey, Other->GetActorLocation()));
			TieBreaks.Add(Other->GetUniqueID());
		}
	};
	// Every chaser looks a few times a second, so it reads the encounters' list of the level's creatures instead of walking all
	// the level's actors. (The flank ranks by bearing, so the order the creatures come in doesn't matter.) A level without
	// that list walks them.
	if (const UEncounterSubsystem* Encounters = World->GetSubsystem<UEncounterSubsystem>())
	{
		for (const TWeakObjectPtr<ACreatureBase>& Each : Encounters->GetTrackedCreatures())
		{
			if (const ACreatureBase* Other = Each.Get())
			{
				Consider(Other);
			}
		}
	}
	else
	{
		for (TActorIterator<ACreatureBase> It(World); It; ++It)
		{
			Consider(*It);
		}
	}
	FlankDegrees = bFlanks ? PackRules::FlankOffsets(Bearings, TieBreaks, Settings.FlankStepDegrees, Settings.FlankMaxDegrees)[0] : 0.f;

	// Its pack gone and itself hurt: perhaps it runs for a moment (one roll a life).
	bHadPack |= Packmates > 0;
	if (IsRetreating())
	{
		return;
	}
	const UHealthComponent* Life = Me->FindComponentByClass<UHealthComponent>();
	PackRules::FRetreatAsk Ask;
	Ask.Rank = Me->GetRank();
	Ask.bAllowedKind = Settings.MayRetreat(Me->GetClass());
	Ask.HealthShare = Life ? Life->GetHealthPercent() : 1.f;
	Ask.bHadPack = bHadPack;
	Ask.PackmatesLeft = Packmates;
	Ask.bAlreadyRolled = bRetreatRolled;
	Ask.HealthBelow = Settings.RetreatHealthShare;
	Ask.Chance = Settings.RetreatChance;
	if (!PackRules::WantsRetreatRoll(Ask))
	{
		return;
	}
	bRetreatRolled = true;
	const float Roll = Rolls.FRand();
	if (PackRules::ShouldRetreat(Ask, Roll))
	{
		StartRetreat();
	}
	else
	{
		UE_LOG(LogLooter, Verbose, TEXT("%s: its pack is gone, and it stands its ground (rolled %.2f)."), *Me->GetName(), Roll);
	}
}

void UCreaturePackComponent::StartRetreat()
{
	ACreatureBase* Me = GetCreature();
	const APawn* Victim = Me ? Me->GetTarget() : nullptr;
	const UEncounterSettings& Settings = UEncounterSettings::Get();
	if (!Victim || Settings.RetreatSeconds <= 0.f)
	{
		return;
	}
	const FVector Home = Me->GetHome().GetLocation();
	const FHuntingGround& Ground = Me->HuntingGround;
	RetreatTo = PackRules::RetreatGoal(Me->GetActorLocation(), Victim->GetActorLocation(), Home, Settings.RetreatDistance * Me->GetSizeScale(),
		[&Ground, &Home](const FVector& Point) { return Ground.Contains(Point, Home); });
	RetreatLeft = Settings.RetreatSeconds;
	FlankDegrees = 0.f;
	// A cry as it breaks: the player hears it give way.
	if (const UCreatureVoiceComponent* Voice = Me->FindComponentByClass<UCreatureVoiceComponent>())
	{
		Voice->Play(UCreatureVoiceComponent::CriesFor(*Me).Hurt, RetreatCryVolume);
	}
	UE_LOG(LogLooter, Log, TEXT("%s: its pack is gone; hurt, it breaks off for %.1f s."), *Me->GetName(), RetreatLeft);
}

FVector UCreaturePackComponent::GetChaseGoal(const APawn& Victim) const
{
	const ACreatureBase* Me = GetCreature();
	const FVector Prey = Victim.GetActorLocation();
	if (!Me)
	{
		return Prey;
	}
	if (IsRetreating())
	{
		return RetreatTo;
	}
	if (FMath::IsNearlyZero(FlankDegrees))
	{
		return Prey;
	}
	// A point as far off as the target, along its flank: the steering heads that way and bends round what's in it.
	const FVector Here = Me->GetActorLocation();
	const float Release = UEncounterSettings::Get().FlankReleaseDistance * Me->GetSizeScale();
	const FVector Way = PackRules::FlankDirection(Here, Prey, FlankDegrees, Release);
	return Way.IsNearlyZero() ? Prey : Here + Way * FVector::Dist2D(Here, Prey);
}

float UCreaturePackComponent::GetRetreatSpeedShare() const
{
	return UEncounterSettings::Get().RetreatSpeedShare;
}

// ---------------------------------------------------------------------------
// The rank sting
// ---------------------------------------------------------------------------

void UCreaturePackComponent::TryRankSting()
{
	ACreatureBase* Me = GetCreature();
	UWorld* World = GetWorld();
	if (!Me || !World || !Me->GetTarget() || !PackRules::ShouldRankSting(Me->GetRank(), Me->bShowsHealthTag, bRankStung))
	{
		return;
	}
	// Once a life, whether it sounds or not: a Restless one losing the player and finding them again doesn't sting twice.
	bRankStung = true;
	const double Now = World->GetTimeSeconds();
	const float Rest = UEncounterSettings::Get().RankStingRest;
	for (TActorIterator<ACreatureBase> It(World); It; ++It)
	{
		const UCreaturePackComponent* Other = *It != Me ? It->GetPack() : nullptr;
		if (Other && Other->RankStings > 0 && Now - Other->LastRankStingTime < Rest)
		{
			// Another ranked creature just sounded it (two Restless turning together): one sting for both.
			return;
		}
	}
	++RankStings;
	LastRankStingTime = Now;
	// Heard by the player wherever the creature stands, as music would be: it says what just turned on them.
	LooterSound::Play2D(Me, RankStingCue);
	UE_LOG(LogLooter, Log, TEXT("%s (%s) turns on the player: the rank sting."), *Me->GetName(), *UEnum::GetValueAsString(Me->GetRank()));
}

float UCreaturePackComponent::GetRankStingAge() const
{
	const UWorld* World = GetWorld();
	if (!World || RankStings <= 0)
	{
		return TNumericLimits<float>::Max();
	}
	return static_cast<float>(World->GetTimeSeconds() - LastRankStingTime);
}

// ---------------------------------------------------------------------------
// A patrol's anchor
// ---------------------------------------------------------------------------

void UCreaturePackComponent::SetRoamAnchor(const FVector& Anchor, bool bMoving)
{
	bHasRoamAnchor = true;
	RoamAnchor = Anchor;
	bRoamAnchorMoving = bMoving;
}

void UCreaturePackComponent::ClearRoamAnchor()
{
	bHasRoamAnchor = false;
	bRoamAnchorMoving = false;
}

bool UCreaturePackComponent::WantsToRoam(const FVector& Here) const
{
	const ACreatureBase* Me = GetCreature();
	const float Slack = RoamSlack * (Me ? Me->GetSizeScale() : 1.f);
	return bHasRoamAnchor && FVector::DistSquared2D(Here, RoamAnchor) > FMath::Square(Slack);
}

bool UCreaturePackComponent::GetRoamMove(const FVector& Here, float WalkSpeed, FVector& OutGoal, float& OutSpeed) const
{
	if (!bHasRoamAnchor)
	{
		return false;
	}
	OutGoal = RoamAnchor;
	const float Gap = static_cast<float>(FVector::Dist2D(Here, RoamAnchor));
	OutSpeed = FMath::Clamp(Gap * RoamCatchUpRate, WalkSpeed * RoamSlowestShare, WalkSpeed);
	return true;
}

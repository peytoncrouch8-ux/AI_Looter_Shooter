// ABountyFight: a taken bounty's set piece, made from its posting and its arena: the pushes and their beats, the named
// mark with its traits, a swift posse, and its two moments (the mark down, the fight over).

#include "Bounties/BountyFight.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Bounties/BountyArena.h"
#include "Bounties/BountyRules.h"
#include "Bounties/BountyTuning.h"
#include "Combat/HealthComponent.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/CreatureTraitComponent.h"
#include "Creatures/CreatureTraitRules.h"
#include "Engine/World.h"

const FName ABountyFight::MarkTag(TEXT("BountyMark"));

namespace
{
	/** Every bounty creature's tag (a mission or the console finds them by it). */
	const FName BountyTag(TEXT("Bounty"));

	/** Its beats' shakes: the first push a nudge, the mark's push a jolt (0-1 of the boss shakes, s). */
	constexpr float FirstShake = 0.15f;
	constexpr float FirstShakeSeconds = 0.5f;
	constexpr float MarkShake = 0.25f;
	constexpr float MarkShakeSeconds = 0.7f;

	/** The longest a push waits on the last one's stragglers (s): as the set pieces'. */
	constexpr float PushAtLatest = 30.f;
}

ABountyFight::ABountyFight()
{
	// The strongbox is the reward (the board drops it); a set piece's shower would pay twice.
	bReward = false;
	bHuntOnSpawn = true;
	WaveInterval = PushAtLatest;
}

ABountyFight* ABountyFight::SpawnFor(UWorld* World, const ABountyArena& Arena, const FBountyPosting& InPosting)
{
	if (!World || !BountyRules::FindKind(InPosting.Kind))
	{
		return nullptr;
	}
	const FTransform At(FRotator(0.f, Arena.GetActorRotation().Yaw, 0.f), Arena.GetActorLocation());
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.ObjectFlags |= RF_Transient;
	Params.bDeferConstruction = true;
	ABountyFight* Fight = World->SpawnActor<ABountyFight>(ABountyFight::StaticClass(), At, Params);
	if (!Fight)
	{
		return nullptr;
	}
	// Set up before its play begins, as a level's spawners are (its story read, its timer started, as it begins).
	Fight->Setup(Arena, InPosting);
	Fight->FinishSpawning(At);
	// In a level that isn't playing yet (a test level), it begins as play would begin it.
	if (!Fight->HasActorBegunPlay())
	{
		Fight->DispatchBeginPlay();
	}
	UE_LOG(LogLooter, Log, TEXT("Bounty fight %s at %s: %d creatures in %d pushes, waiting for the player."),
		*Fight->GetSpawnerId().ToString(), *Arena.GetArenaId().ToString(), Fight->PlannedTotal, Fight->NumWaves);
	return Fight;
}

void ABountyFight::Setup(const ABountyArena& Arena, const FBountyPosting& InPosting)
{
	Posting = InPosting;
	const FBountyTuning Tuning = FBountyTuning::Current();
	const FBountyKindInfo* Kind = BountyRules::FindKind(Posting.Kind);
	const FBountyFightPlan Plan = BountyRules::MakeGroups(Posting);
	Groups = Plan.Groups;
	NumWaves = Plan.Pushes;
	MarkClass = Plan.Mark;
	MarkRank = Plan.MarkRank;
	PlannedTotal = Plan.Total;

	SpawnerId = FName(*FString::Printf(TEXT("Bounty_%d"), Posting.Serial));
	CreatureTags = { BountyTag, SpawnerId };
	PushWhenLeft = Tuning.PushWhenLeft;

	// Its ground: the arena's, as an encounter's.
	GiveUpRadius = Arena.GiveUpRadius;
	GroundCorners = Arena.GroundCorners;
	GroundMargin = Arena.GroundMargin;
	GroundMaxRise = Arena.GroundMaxRise;
	MaxGroundStep = Arena.MaxGroundStep;
	WaitForSpawners = Arena.WaitForSpawners;
	bAroundPlayer = Arena.bAroundPlayer;

	// It springs on walking onto its ground (a band of trees), or coming near its middle: nobody wakes into it.
	if (Arena.bSpringOnGround && Arena.GroundCorners.Num() >= 3)
	{
		AmbushCorners = Arena.GroundCorners;
		AmbushMaxRise = Arena.GroundMaxRise;
		SpringRadius = 0.f;
	}
	else
	{
		AmbushCorners.Reset();
		SpringRadius = Arena.SpringRadius > 0.f ? Arena.SpringRadius : Tuning.SpringRadius;
	}
	ActivationRadius = FMath::Max(SpringRadius, 1000.f);

	// The dead rise; spiders drop from the Webwood's trees, else come their own way.
	Entrance = Kind && Kind->bRises ? EAmbushEntrance::Rise : (Arena.bDropFromAbove ? EAmbushEntrance::Drop : EAmbushEntrance::Appear);

	// The pushes: the first from ahead, the middle ones from the flank, the mark's with the mark on its own spot.
	const TArray<FVector>& Ahead = Arena.PushSpots;
	const TArray<FVector>& Flank = Arena.FlankSpots.Num() > 0 ? Arena.FlankSpots : Arena.PushSpots;
	const TArray<FVector>& MarkSide = Arena.MarkSpots.Num() > 0 ? Arena.MarkSpots : Arena.PushSpots;
	SpawnPoints = bAroundPlayer ? TArray<FVector>() : Ahead;
	Beats.Reset();
	for (int32 Wave = 1; Wave <= NumWaves; ++Wave)
	{
		FSetPieceBeat& Beat = Beats.AddDefaulted_GetRef();
		Beat.Wave = Wave;
		if (!bAroundPlayer)
		{
			Beat.Spots = Wave == 1 ? Ahead : (Wave == NumWaves ? MarkSide : Flank);
		}
		if (Wave == NumWaves)
		{
			Beat.Speaker = BountyRules::HobName();
			Beat.Line = BountyRules::MarkArriveLine(Posting);
			Beat.StingCue = LooterSoundCue::Bounty::MarkArrive;
			Beat.Shake = MarkShake;
			Beat.ShakeSeconds = MarkShakeSeconds;
		}
		else if (Wave == 1)
		{
			Beat.Speaker = BountyRules::HobName();
			Beat.Line = BountyRules::PushLine(Posting);
			Beat.StingCue = LooterSoundCue::RankSting;
			Beat.Shake = FirstShake;
			Beat.ShakeSeconds = FirstShakeSeconds;
		}
	}
	ClearedBeat = FSetPieceBeat();
	ClearedBeat.Speaker = BountyRules::HobName();
	ClearedBeat.Line = BountyRules::ClearedLine(Posting);
	RewardSpot = Arena.RewardSpot;
}

bool ABountyFight::IsMarkCandidate(const ACreatureBase& Creature) const
{
	// The posse is Basic and Restless only (Fixed ranks), so the mark's rank and class tell it apart.
	return !bMarkDown && !Mark.IsValid() && MarkClass && Creature.IsA(MarkClass) && Creature.GetRank() == MarkRank;
}

void ABountyFight::OnCreatureSpawned(ACreatureBase& Creature)
{
	Super::OnCreatureSpawned(Creature);
	UCreatureTraitComponent* Traits = UCreatureTraitComponent::FindOrAdd(Creature);
	if (IsMarkCandidate(Creature))
	{
		Mark = &Creature;
		// Its name on its tag (after its rank's word) and exactly the posting's traits: the card told the player what comes.
		Creature.DisplayName = BountyRules::MarkName(Posting.Kind, Posting.NameIndex);
		Creature.Tags.AddUnique(MarkTag);
		if (Traits)
		{
			Traits->SetTraits(Posting.MarkTraits);
		}
		if (UHealthComponent* Life = Creature.FindComponentByClass<UHealthComponent>())
		{
			Life->OnDeath.AddUniqueDynamic(this, &ABountyFight::HandleMarkDeath);
		}
		UE_LOG(LogLooter, Log, TEXT("Bounty fight %s: its mark %s is out (level %d)."), *GetSpawnerId().ToString(),
			*Creature.DisplayName.ToString(), Creature.Level);
		return;
	}
	// A swift posse runs Fleet, where Fleet fits it (never its support: the Spitter and the Bell-Ringer keep back).
	if (Posting.HasTwist(EBountyTwist::SwiftPosse) && Traits
		&& CreatureTraitRules::Fits(ECreatureTrait::Fleet, CreatureTraitRules::KindOf(Creature)) && !Traits->HasTrait(ECreatureTrait::Fleet))
	{
		TArray<ECreatureTrait> With(Traits->GetTraits());
		With.Add(ECreatureTrait::Fleet);
		Traits->SetTraits(With);
	}
}

void ABountyFight::HandleMarkDeath(AController* Killer)
{
	if (bMarkDown)
	{
		return;
	}
	bMarkDown = true;
	UE_LOG(LogLooter, Log, TEXT("Bounty fight %s: its mark is down: the bounty is won."), *GetSpawnerId().ToString());
	OnMarkDown.Broadcast(*this);
}

void ABountyFight::OnStateChanged(EEncounterState From, EEncounterState To)
{
	Super::OnStateChanged(From, To);
	// The set piece finishes as its last creature falls (no placed ones: it's done at once).
	if (To == EEncounterState::Cleared && IsDone() && !bDoneTold)
	{
		bDoneTold = true;
		OnDone.Broadcast(*this);
	}
}

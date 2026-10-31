// ASpiderCreature's cunning in a fight (the user, 2026-10-31: their AI is "lackluster"; rules SpiderHuntRules): given its turn
// it often circles the player first, a few metres out, facing them and working round toward their side or back, then rushes
// in and bites (the base's chase and attack do the rush and the bite); a burst of damage sends it skittering back, still
// facing them, before it comes again, and now and then after a bite it skips back out of reach; given its turn just as a
// packmate's bite at them began, it circles first, so they don't all bite at once. Only a plain brown spider fighting on the
// ground (not the Spitter, whose role is ranged; not a boss or a Legendary monster; not a spiderling: the Gravemother's
// brood keeps its pacing). Its health and bite are untouched. Looter.Spider.Cunning 0: as before.
//
// Why it's built this way: the base's chase asks its kind for a move each frame it holds a turn (StartsRingMove), so the
// circle is a move that holds the turn a moment and gives the frame back when it's done; a skitter holds its brain (HoldBrain)
// and moves it itself, as an entrance does. Both face the player by turning the actor after the movement has turned it
// toward where it walks (the movement turns 320 degrees a second, this 540), so no movement setting is left changed.

#include "Creatures/SpiderCreature.h"
#include "Audio/CreatureVoiceComponent.h"
#include "Creatures/CreatureReactComponent.h"
#include "Creatures/CreatureReactRules.h"
#include "Creatures/FightDirectorSubsystem.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"

namespace
{
	/** It turns to face its target this fast while it circles or skitters (degrees a second). */
	constexpr float FaceRate = 540.f;
	/** A turn held again within this (s) is the same turn; a longer gap starts a fresh one (its plan made again). */
	constexpr float TurnGap = 0.25f;
	/** Its way is looked along this far (cm at size 1) for ground no more than GroundDrop under it (cm at size 1): never off an edge. */
	constexpr float LookAhead = 120.f;
	constexpr float GroundDrop = 180.f;
	/** Circling into something (moving under this share of its pace this long, s), it turns the other way once, then comes in. */
	constexpr float StuckShare = 0.3f;
	constexpr float StuckSeconds = 0.3f;
	/** Spiders smaller than this (spiderlings) keep the plain fight. */
	constexpr float SmallestCunning = 0.6f;
}

bool ASpiderCreature::UsesCunning() const
{
	return bWeightyGait && Lurk.Phase == ESpiderLurkPhase::None && !IsDead() && GetFightRole() == EFightRole::Melee && !KeepsOwnFight()
		&& GetSizeScale() >= SmallestCunning && FSpiderHuntTuning::Live().Enabled != 0;
}

bool ASpiderCreature::IsMateBiting(const APawn& Victim, float Within) const
{
	// Once a turn (its plan), so a walk of the level's spiders is cheap.
	UWorld* World = GetWorld();
	if (!World || Within <= 0.f)
	{
		return false;
	}
	for (TActorIterator<ASpiderCreature> It(World); It; ++It)
	{
		const ASpiderCreature* Mate = *It;
		if (Mate != this && !Mate->IsDead() && Mate->GetTarget() == &Victim && Mate->GetCreatureState() == ECreatureState::Attack
			&& Mate->GetStateTime() < Within)
		{
			return true;
		}
	}
	return false;
}

bool ASpiderCreature::StartsRingMove(const APawn& Victim, float Distance)
{
	if (!UsesCunning() || IsHeldByHit())
	{
		return false;
	}
	UWorld* World = GetWorld();
	UCreatureReactComponent* Reacts = GetReact();
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (!World || !Reacts || !Movement)
	{
		return false;
	}
	const FSpiderHuntTuning Tuning = FSpiderHuntTuning::Live();
	const float Scale = GetSizeScale();
	const float DeltaSeconds = World->GetDeltaSeconds();
	const double Now = World->GetTimeSeconds();
	// A fresh turn: its plan is made again (a circle first, or straight in).
	if (Now - Hunt.LastTurnTime > TurnGap)
	{
		Hunt.bTurnPlanned = false;
		Hunt.CircleLeft = 0.f;
	}
	Hunt.LastTurnTime = Now;
	const FVector Here = GetActorLocation();
	const FVector Prey = Victim.GetActorLocation();
	FRandomStream& Dice = Reacts->GetRandom();
	if (!Hunt.bTurnPlanned)
	{
		Hunt.bTurnPlanned = true;
		// A packmate's bite at them just began: this one circles a moment first, so they don't all bite at once.
		const bool bCrowded = IsMateBiting(Victim, Tuning.BiteSpacing);
		if (SpiderHuntRules::WantsCircle(Distance, Scale, Dice.FRand(), bCrowded, Tuning))
		{
			Hunt.CircleLeft = SpiderHuntRules::CircleSeconds(Dice.FRand(), bCrowded, Tuning);
			Hunt.CircleSide = SpiderHuntRules::CircleSide(Here, Prey, Victim.GetControlRotation().Vector(), Dice.FRand());
			Hunt.Blocked = 0.f;
			Hunt.bFlipped = false;
		}
	}
	if (Hunt.CircleLeft <= 0.f)
	{
		return false;
	}
	Hunt.CircleLeft -= DeltaSeconds;
	const float Speed = GetChaseSpeedNow() * Tuning.CircleSpeedShare;
	FVector Way = SpiderHuntRules::CircleWay(Here, Prey, Hunt.CircleSide, Tuning.CircleRadius * Scale, Tuning.CircleRadialGain);
	// Never off an edge, nor grinding into a rock: the other way round once, then it comes straight in.
	auto Clear = [this, &Here, Scale](const FVector& Along)
	{
		FVector Ground;
		return FindGround(Here + Along * (LookAhead * Scale), 60.f * Scale, (GroundDrop + 62.f) * Scale, Ground);
	};
	const bool bStuck = GetVelocity().Size2D() < Speed * StuckShare;
	Hunt.Blocked = bStuck ? Hunt.Blocked + DeltaSeconds : 0.f;
	if (!Clear(Way) || Hunt.Blocked > StuckSeconds)
	{
		if (Hunt.bFlipped)
		{
			Hunt.CircleLeft = 0.f;
			return false;
		}
		Hunt.bFlipped = true;
		Hunt.Blocked = 0.f;
		Hunt.CircleSide = -Hunt.CircleSide;
		Way = SpiderHuntRules::CircleWay(Here, Prey, Hunt.CircleSide, Tuning.CircleRadius * Scale, Tuning.CircleRadialGain);
		if (!Clear(Way))
		{
			Hunt.CircleLeft = 0.f;
			return false;
		}
	}
	Movement->MaxWalkSpeed = Speed;
	AddMovementInput(Way);
	bCirclingNow = true;
	return true;
}

void ASpiderCreature::OnTurnPlayedOut()
{
	Super::OnTurnPlayedOut();
	Hunt.bTurnPlanned = false;
	Hunt.CircleLeft = 0.f;
	// Now and then, its bite in, it skips back out of reach (hit and run) before it comes again.
	UCreatureReactComponent* Reacts = GetReact();
	const FSpiderHuntTuning Tuning = FSpiderHuntTuning::Live();
	if (Reacts && UsesCunning() && SpiderHuntRules::WantsHitAndRun(Hunt.SinceSkitter, Reacts->GetRandom().FRand(), Tuning))
	{
		StartSkitter(Tuning.HitAndRunSeconds);
	}
}

void ASpiderCreature::NoteHurtForHunt(const FCreatureHit& Hit)
{
	if (!UsesCunning())
	{
		return;
	}
	const FSpiderHuntTuning Tuning = FSpiderHuntTuning::Live();
	Hunt.RecentHurt += Hit.Share;
	// Mid-bite it carries on; staggered, the stagger is its answer; already skittering, it goes on.
	UCreatureReactComponent* Reacts = GetReact();
	if (GetCreatureState() != ECreatureState::Chase || IsHeldByHit() || Hunt.SkitterLeft > 0.f || !Reacts || !GetTarget())
	{
		return;
	}
	if (SpiderHuntRules::WantsBackOff(Hunt.RecentHurt, Hunt.SinceSkitter, Reacts->GetRandom().FRand(), Tuning))
	{
		StartSkitter(Tuning.BackOffSeconds);
	}
}

void ASpiderCreature::StartSkitter(float Seconds)
{
	const FSpiderHuntTuning Tuning = FSpiderHuntTuning::Live();
	Hunt.SkitterLeft = Seconds;
	Hunt.SinceSkitter = 0.f;
	Hunt.SkitterSpeed = Tuning.BackOffSpeed * GetSizeScale();
	Hunt.RecentHurt = 0.f;
	Hunt.CircleLeft = 0.f;
	// Out of the fight a moment: its turn goes to the next one.
	if (UFightDirectorSubsystem* Director = UFightDirectorSubsystem::Get(this))
	{
		if (Director->HoldsToken(*this))
		{
			Director->ReleaseToken(*this);
		}
	}
	// A hiss as it goes, its abdomen tucked.
	Voice->Play(FName(CreatureReactCue::SpiderNotice), 0.6f);
	AbdomenKick = -5.f;
}

void ASpiderCreature::HoldForHunt(float DeltaSeconds)
{
	if (Hunt.SkitterLeft > 0.f && !IsDead())
	{
		HoldBrain(DeltaSeconds + 0.05f);
	}
}

void ASpiderCreature::TickHunt(float DeltaSeconds)
{
	const FSpiderHuntTuning Tuning = FSpiderHuntTuning::Live();
	Hunt.SinceSkitter += DeltaSeconds;
	Hunt.RecentHurt = SpiderHuntRules::FadeHurt(Hunt.RecentHurt, DeltaSeconds, Tuning);
	const APawn* Victim = GetTarget();
	const FVector Here = GetActorLocation();
	auto FaceVictim = [this, Victim, &Here, DeltaSeconds]()
	{
		const FVector To = Victim->GetActorLocation() - Here;
		if (To.SizeSquared2D() > 1.0)
		{
			SetActorRotation(FMath::RInterpConstantTo(GetActorRotation(), FRotator(0.f, static_cast<float>(To.Rotation().Yaw), 0.f), DeltaSeconds,
				FaceRate));
		}
	};
	// Circling: its eyes stay on its prey as it sidles round.
	if (bCirclingNow)
	{
		bCirclingNow = false;
		if (Victim)
		{
			FaceVictim();
		}
	}
	if (Hunt.SkitterLeft <= 0.f)
	{
		return;
	}
	Hunt.SkitterLeft -= DeltaSeconds;
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (!Victim || IsDead() || !Movement || GetCreatureState() == ECreatureState::Attack || !Movement->IsMovingOnGround())
	{
		Hunt.SkitterLeft = 0.f;
		return;
	}
	// Backing off, facing it, a little to one side; never off an edge.
	const float Scale = GetSizeScale();
	const FVector Way = SpiderHuntRules::BackOffWay(Here, Victim->GetActorLocation(), GetUniqueID() % 2 == 0 ? 1.f : -1.f);
	FVector Ground;
	if (Way.IsNearlyZero() || !FindGround(Here + Way * (LookAhead * Scale), 60.f * Scale, (GroundDrop + 62.f) * Scale, Ground))
	{
		Hunt.SkitterLeft = 0.f;
		return;
	}
	Movement->MaxWalkSpeed = Hunt.SkitterSpeed;
	AddMovementInput(Way);
	FaceVictim();
}

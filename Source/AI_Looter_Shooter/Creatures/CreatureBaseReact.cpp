// ACreatureBase: its notice beat and its entrance (the 2026-10-14 polish, "creatures that react"). The beat: when its own
// eyes first find its prey (from calm, or a search that never came to a chase) it holds where it is for half a second,
// turning to face them while its body shows it (a spider rears and flares its fangs, an Unpaid's coal flares and its head
// snaps round, a miner lowers his pick, a slime shudders), then comes. FCreatureNotice has the rules: once a hunt, never
// for one already hurt, a boss or one held back; a hit during it ends it. A pack's call and a shot start a hunt with none.
// Its entrance: brought into the world where a player can see it (a spawner's wave, a respawn), it comes its kind's way
// rather than popping in (PlayEntrance: the dead rise through the dirt, a spider climbs out of it, a slime swells up).

#include "Creatures/CreatureBase.h"
#include "AI_Looter_Shooter.h"
#include "Combat/HealthComponent.h"
#include "Creatures/CreatureHitReactionComponent.h"
#include "Creatures/CreatureReactComponent.h"
#include "Creatures/CreatureReactSubsystem.h"
#include "Creatures/EncounterSubsystem.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Pawn.h"

namespace
{
	/** Prey this near (a share of its attack range) is point blank: a short beat only. */
	constexpr float NoticePointBlankShare = 1.5f;

	/** An entrance is played for a player this near (cm) who has it in view; farther, nobody sees it pop in. */
	constexpr float EntranceSeenWithin = 6000.f;
}

bool ACreatureBase::IsNoticing() const
{
	return React && React->GetNotice().IsHolding();
}

float ACreatureBase::GetNoticeStrength() const
{
	return React ? React->GetNotice().GetStrength() : 0.f;
}

/** Its own eyes found its prey: the beat, if this hunt may have one (it was From before). */
void ACreatureBase::BeginNotice(ECreatureState From)
{
	const APawn* Prey = Target.Get();
	if (!React || !Prey || State != ECreatureState::Chase || bPassive || HitReaction->IsStaggered())
	{
		return;
	}
	FCreatureNotice& Notice = React->GetNotice();
	const bool bHurt = Health->GetHealth() < Health->GetMaxHealth() - KINDA_SMALL_NUMBER;
	if (Notice.IsHolding() || !FCreatureNotice::MayNotice(From, bHurt, KeepsBossRules(), Notice.HasChasedThisHunt()))
	{
		return;
	}
	const float PointBlank = GetAttackRange() * NoticePointBlankShare;
	const bool bPointBlank = FVector::DistSquared2D(Prey->GetActorLocation(), GetActorLocation()) <= FMath::Square(PointBlank);
	Notice.Begin(FCreatureNotice::LengthFor(React->GetSeed(), bPointBlank));
	UE_LOG(LogLooter, Verbose, TEXT("%s notices %s (%.2f s)."), *GetName(), *GetNameSafe(Prey),
		FCreatureNotice::LengthFor(React->GetSeed(), bPointBlank));
	OnNoticed();
}

/** In its chase: while its beat holds, it only turns to face its prey (true); after, it has chased this hunt. */
bool ACreatureBase::HoldForNotice(const APawn& Prey, float DeltaSeconds)
{
	if (!React)
	{
		return false;
	}
	FCreatureNotice& Notice = React->GetNotice();
	if (Notice.IsHolding())
	{
		FaceToward(Prey.GetActorLocation(), DeltaSeconds);
		return true;
	}
	Notice.NoteChase();
	return false;
}

bool ACreatureBase::PlayEntranceIfSeen()
{
	if (IsDead())
	{
		return false;
	}
	// In a local player's view (the encounters' own measure, a margin past the screen's edges) and near enough to see.
	const UEncounterSubsystem* Encounters = UEncounterSubsystem::Get(this);
	UCreatureReactSubsystem* Reacts = UCreatureReactSubsystem::Get(this);
	const float Radius = GetCapsuleComponent()->GetScaledCapsuleRadius() * 3.f;
	if (!Encounters || !Reacts || !Encounters->IsInPlayersView(GetActorLocation(), Radius)
		|| !Reacts->IsNearAViewer(GetActorLocation(), EntranceSeenWithin))
	{
		return false;
	}
	PlayEntrance();
	return true;
}

// AAbelKeeper under the bosses' shared attack rules (round 5; Bosses/BossAttackRules.h). His attacks are the Unpaid's
// stock lunge (his creature's attack) and the buckshot after his lantern's flare (StartFlare, from his phases' schedule);
// his other moments (the grief, the drift into the fog and its shots, the walk into the wind, the pull) are his phases'
// own beats and stay as they were. Here: when a lunge may start (a breath after his last attack, the player in front of
// him, never a third lunge in a row), his log's clocks, his flare out of its schedule to break a run of lunges or a long
// idle stretch, and his hurt zones as they stand (what Looter.Boss.HurtZones 1 draws).

#include "Bosses/AbelKeeper.h"
#include "AI_Looter_Shooter.h"
#include "Bosses/BossAttackRules.h"
#include "Bosses/BossComponent.h"
#include "Bosses/AbelRules.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Pawn.h"

namespace
{
	/** His hurt zones reach this high over his feet (cm, at size 1 and up), as a lunge's strike reaches up or down. */
	constexpr float ReachUpDown = 220.f;

	/** The buckshot's cone is drawn this far past the player (cm), so it reads as passing them. */
	constexpr float ConePastPlayer = 300.f;
}

bool AAbelKeeper::CanStartAttack() const
{
	// None of his own moments under way: they hold him.
	if (!Super::CanStartAttack() || Move != EAbelMove::None)
	{
		return false;
	}
	// The shared rules: a breath after his last attack (a buckshot's too), the player in front of him, and never a third
	// lunge in a row (his flare comes instead: TickAttackRules).
	const APawn* Victim = GetTarget();
	if (!Boss || !Victim)
	{
		return true;
	}
	const FBossAttackSpec Lunge(AbelRules::LungeAttack(), 0.f, 0.f, BossAttackRules::FrontDegrees());
	return BossAttackRules::MayStart(Lunge, Boss->AttackLog, GetActorForwardVector(), Victim->GetActorLocation() - GetActorLocation());
}

void AAbelKeeper::OnAttackStarted()
{
	// The Unpaid's shriek and lunge, as ever; noted in his log.
	Super::OnAttackStarted();
	if (Boss)
	{
		Boss->AttackLog.NoteStarted(AbelRules::LungeAttack());
	}
}

void AAbelKeeper::TickAttackRules(float DeltaSeconds)
{
	if (!Boss)
	{
		return;
	}
	FBossAttackLog& Log = Boss->AttackLog;
	const bool bBusy = GetCreatureState() == ECreatureState::Attack || IsLunging() || Move == EAbelMove::Flare || Move == EAbelMove::Fire;
	if (Log.IsAttacking() && !bBusy)
	{
		Log.NoteEnded();
	}
	const APawn* Player = Boss->GetFightPlayer();
	const bool bFree = Player && GetTarget() && Move == EAbelMove::None && IsFreeForMoment();
	Log.Advance(DeltaSeconds, bFree);
	if (!BossAttackRules::IsOn() || !bFree)
	{
		return;
	}
	// Two lunges in a row with the player still in his reach: his lantern flares and the buckshot follows, out of its turn.
	const float Distance = static_cast<float>(FVector::Dist2D(Player->GetActorLocation(), GetActorLocation()));
	if (BossAttackRules::IsBlocked(AbelRules::LungeAttack(), Log) && Distance <= GetAttackStartRange())
	{
		if (StartFlare())
		{
			UE_LOG(LogLooter, Verbose, TEXT("%s: two lunges in a row; he flares instead."), *GetActorNameOrLabel());
			return;
		}
	}
	// Too long free without an attack (the player keeping away between his buckshot's turns): his flare, now.
	if (BossAttackRules::IsOverdue(Log))
	{
		StartFlare();
		Log.NoteNudged();
	}
}

void AAbelKeeper::GatherHurtZones(TArray<FBossHurtZone>& Out) const
{
	if (IsDead() || !Boss)
	{
		return;
	}
	const float Scale = GetSizeScale();
	const FVector Feet = GetActorLocation() - FVector(0.0, 0.0, GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
	const float Tall = ReachUpDown * FMath::Max(Scale, 1.f);
	const FName LungeId = AbelRules::LungeAttack();

	// The lunge: its path grey through the shriek, his body red while he flies (it lands within LungeHitRadius of him).
	if (IsLunging())
	{
		Out.Add(FBossHurtZone::Disc(LungeId, EBossAttackWindow::Active, Feet, LungeHitRadius * Scale, Tall));
	}
	else if (GetCreatureState() == ECreatureState::Attack && GetStateTime() < AttackWindup && Move == EAbelMove::None)
	{
		const FVector End = Feet + GetActorForwardVector().GetSafeNormal2D() * (LungeDistance * Scale);
		Out.Add(FBossHurtZone::Lane(LungeId, EBossAttackWindow::Windup, Feet, End, LungeHitRadius * Scale, Tall));
	}

	// The buckshot: its cone at the player grey through the flare, red as it fires (a barrage: while its shots go).
	const APawn* Player = Boss->GetFightPlayer();
	if (Player && (Move == EAbelMove::Flare || Move == EAbelMove::Fire))
	{
		const FVector ToPlayer = Player->GetActorLocation() - GetActorLocation();
		const float Reach = FMath::Min(Buckshot.Range, static_cast<float>(ToPlayer.Size2D()) + ConePastPlayer);
		const EBossAttackWindow Window = Move == EAbelMove::Flare ? EBossAttackWindow::Windup : EBossAttackWindow::Active;
		Out.Add(FBossHurtZone::Wedge(AbelRules::BuckshotAttack(), Window, Feet, ToPlayer, Reach, Buckshot.SpreadDegrees * 0.5f, Tall));
	}
}

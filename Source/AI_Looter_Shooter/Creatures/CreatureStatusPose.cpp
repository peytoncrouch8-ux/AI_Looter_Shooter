#include "Creatures/CreatureStatusPose.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/CreatureStatusReactComponent.h"
#include "Elements/ElementRules.h"
#include "Elements/ElementStatus.h"
#include "Elements/ElementTuning.h"
#include "GameFramework/Actor.h"
#include "HAL/IConsoleManager.h"

// The status reactions' plain rules: what each status asks of the pose, its easing, the jolt's jerk and the rot's cough,
// and the burn's panic rule. The kinds lay the pose on their bodies in their own files; the panic runs in
// CreatureBaseStatus.cpp.

namespace
{
	TAutoConsoleVariable<float> CVarBurnPanic(TEXT("Looter.Element.BurnPanic"), 0.3f,
		TEXT("The chance a burn taking a Basic or Restless creature (not a spider, a boss or a Soulfed) sends it running 1.5 s from the player."),
		ECVF_Default);
	TAutoConsoleVariable<float> CVarBurnPanicCooldown(TEXT("Looter.Element.BurnPanicCooldown"), 6.f,
		TEXT("Seconds after a burn's panic before the same creature may panic again."), ECVF_Default);
	TAutoConsoleVariable<float> CVarStatusPose(TEXT("Looter.Element.StatusPose"), 1.f,
		TEXT("The element statuses' poses on creatures (burn flail, jolt lock-up, rot hunch and cough, salt stiffness, smite flinch), times (0 hides them)."),
		ECVF_Default);

	/** An alpha toward its goal: up at EaseIn a second, down at EaseOut. */
	float EaseToward(float Now, float Goal, float DeltaSeconds)
	{
		const float Rate = Goal > Now ? CreatureStatusPose::EaseIn : CreatureStatusPose::EaseOut;
		return FMath::FInterpConstantTo(Now, Goal, DeltaSeconds, Rate);
	}
}

// ---------------------------------------------------------------------------
// The pose
// ---------------------------------------------------------------------------

bool FCreatureStatusPose::IsAny() const
{
	return Burn > 0.f || Jolt > 0.f || Rot > 0.f || Salt > 0.f || Smite > 0.f || JoltJerk > 0.f || Cough > 0.f;
}

float FCreatureStatusPose::Wave(float Hz, float Phase) const
{
	return FMath::Sin(UE_TWO_PI * Hz * Time + Phase);
}

float FCreatureStatusPose::Flail(float Phase) const
{
	// Two waves out of step, so the flail never settles into a plain swing.
	return 0.7f * Wave(CreatureStatusPose::FlailHz, Phase) + 0.3f * Wave(CreatureStatusPose::FlailHz * 2.7f, 1.3f * Phase + 0.6f);
}

float FCreatureStatusPose::Tremor(float Phase) const
{
	return 0.65f * Wave(CreatureStatusPose::TremorHz, Phase) + 0.35f * Wave(CreatureStatusPose::TremorHz * 1.618f, 2.1f * Phase + 1.f);
}

// ---------------------------------------------------------------------------
// The tracker
// ---------------------------------------------------------------------------

void FCreatureStatusPoseTracker::Advance(const FElementStatus& Status, const FElementTuning& Tuning, bool bAlive, float DeltaSeconds)
{
	const float Dt = FMath::Max(DeltaSeconds, 0.f);
	Pose.Time += Dt;
	FCreatureStatusPose Goal = bAlive ? CreatureStatusPose::GoalOf(Status, Tuning) : FCreatureStatusPose();

	// The jolt's burst: one fewer waiting than a step ago, from one charged to go (a jolt cleared by a death or a new life
	// isn't a burst).
	const int32 Jolts = Status.NumJolts();
	if (bAlive && Jolts < LastJolts && LastJoltCharge >= 0.5f)
	{
		JerkAge = 0.f;
	}
	else if (JerkAge >= 0.f)
	{
		JerkAge += Dt;
	}
	LastJolts = Jolts;
	LastJoltCharge = Status.GetJoltCharge(Tuning);
	const bool bJerking = bAlive && JerkAge >= 0.f && JerkAge < CreatureStatusPose::JoltJerkSeconds;
	if (bAlive && JerkAge >= 0.f && JerkAge < CreatureStatusPose::JoltLockSeconds)
	{
		// Locked up through the burst's stagger, then let go.
		Goal.Jolt = 1.f;
	}

	Pose.Burn = EaseToward(Pose.Burn, Goal.Burn, Dt);
	Pose.Jolt = EaseToward(Pose.Jolt, Goal.Jolt, Dt);
	Pose.Rot = EaseToward(Pose.Rot, Goal.Rot, Dt);
	Pose.Salt = EaseToward(Pose.Salt, Goal.Salt, Dt);
	Pose.Smite = EaseToward(Pose.Smite, Goal.Smite, Dt);
	Pose.ClockScale = FMath::Lerp(1.f, CreatureStatusPose::SaltClock, Pose.Salt);
	// The jerk snaps in at once (the burst is a hit) and fades fast.
	Pose.JoltJerk = bJerking ? FMath::Square(1.f - JerkAge / CreatureStatusPose::JoltJerkSeconds) : 0.f;

	// The rot's cough keeps its beat while its hunch shows; the first comes a little into the rot.
	if (Pose.Rot >= CreatureStatusPose::CoughFrom)
	{
		CoughClock += Dt;
		const float Into = FMath::Fmod(CoughClock, CreatureStatusPose::CoughEvery);
		Pose.Cough = CreatureStatusPose::CoughShape(Into) * Pose.Rot;
	}
	else
	{
		CoughClock = CreatureStatusPose::CoughEvery - CreatureStatusPose::CoughLead;
		Pose.Cough = 0.f;
	}
}

bool FCreatureStatusPoseTracker::IsAtRest() const
{
	return !Pose.IsAny() && (JerkAge < 0.f || JerkAge >= CreatureStatusPose::JoltLockSeconds);
}

void FCreatureStatusPoseTracker::Reset(float TimeOffset)
{
	Pose = FCreatureStatusPose();
	Pose.Time = TimeOffset;
	LastJolts = 0;
	LastJoltCharge = 0.f;
	JerkAge = -1.f;
	CoughClock = CreatureStatusPose::CoughEvery - CreatureStatusPose::CoughLead;
}

// ---------------------------------------------------------------------------
// The rules
// ---------------------------------------------------------------------------

FCreatureStatusPose CreatureStatusPose::Of(const AActor* Creature)
{
	const UCreatureStatusReactComponent* Reacts = UCreatureStatusReactComponent::Find(Creature);
	if (!Reacts)
	{
		return FCreatureStatusPose();
	}
	// A boss keeps its own show: no status pose, as no stagger pose.
	const ACreatureBase* Base = Cast<ACreatureBase>(Creature);
	if (Base && Base->IsBossCreature())
	{
		return FCreatureStatusPose();
	}
	return Scaled(Reacts->GetPose(), LiveStatusPoseScale());
}

FCreatureStatusPose CreatureStatusPose::GoalOf(const FElementStatus& Status, const FElementTuning& Tuning)
{
	FCreatureStatusPose Goal;
	// A burn flails, harder for each burn on it.
	const int32 Burns = Status.NumBurns();
	Goal.Burn = Burns > 0 ? FMath::Min(0.75f + 0.125f * static_cast<float>(Burns - 1), 1.f) : 0.f;
	// A jolt waiting trembles, more as it charges (its burst holds the lock-up: the tracker's).
	Goal.Jolt = Status.NumJolts() > 0 ? 0.6f + 0.4f * Status.GetJoltCharge(Tuning) : 0.f;
	// The rot hunches it further as it grows.
	Goal.Rot = Status.IsRotting() ? 0.55f + 0.45f * Status.GetRotGrowth() : 0.f;
	Goal.Salt = Status.IsSalted() ? 1.f : 0.f;
	// The light gathering: a shy flinch at first, a cower as the burst nears.
	Goal.Smite = Status.NumSmites() > 0 ? 0.3f + 0.7f * Status.GetSmiteCharge(Tuning) : 0.f;
	return Goal;
}

FCreatureStatusPose CreatureStatusPose::Scaled(const FCreatureStatusPose& Pose, float Scale)
{
	const float Amount = FMath::Clamp(Scale, 0.f, 2.f);
	FCreatureStatusPose Out = Pose;
	Out.Burn = FMath::Min(Pose.Burn * Amount, 2.f);
	Out.Jolt = FMath::Min(Pose.Jolt * Amount, 2.f);
	Out.Rot = FMath::Min(Pose.Rot * Amount, 2.f);
	Out.Salt = FMath::Min(Pose.Salt * Amount, 2.f);
	Out.Smite = FMath::Min(Pose.Smite * Amount, 2.f);
	Out.JoltJerk = Pose.JoltJerk * Amount;
	Out.Cough = Pose.Cough * Amount;
	// The clock slows with the salt shown, never stops or runs backwards.
	Out.ClockScale = FMath::Clamp(1.f + (Pose.ClockScale - 1.f) * FMath::Min(Amount, 1.f), 0.2f, 1.f);
	return Out;
}

float CreatureStatusPose::CoughShape(float Into)
{
	if (Into <= 0.f || Into >= CoughSeconds)
	{
		return 0.f;
	}
	// The heave over the whole cough, a retch on it: the body jerks twice, the second the harder.
	const float X = Into / CoughSeconds;
	const float Heave = FMath::Sin(UE_PI * X);
	const float Retch = FMath::Max(0.f, FMath::Sin(UE_TWO_PI * 2.f * X - 0.4f));
	return FMath::Clamp(Heave * (0.6f + 0.4f * Retch), 0.f, 1.f);
}

bool CreatureStatusPose::MayPanic(const FPanicAsk& Ask)
{
	const bool bLowRank = Ask.Rank == ECreatureRank::Basic || Ask.Rank == ECreatureRank::Rare;
	return bLowRank && !Ask.bBoss && !Ask.bOwnFight && !Ask.bSpider && !Ask.bFireShrugged && Ask.bAlive && Ask.bHunting
		&& !Ask.bAttacking && !Ask.bPassive && !Ask.bRunning && Ask.CooldownLeft <= 0.f;
}

bool CreatureStatusPose::Panics(const FPanicAsk& Ask, float Roll, float Chance)
{
	return MayPanic(Ask) && Roll < FMath::Clamp(Chance, 0.f, 1.f);
}

bool CreatureStatusPose::ShrugsFire(EElementAffinity Affinity)
{
	return Affinity == EElementAffinity::Resist || Affinity == EElementAffinity::Immune;
}

float CreatureStatusPose::LiveBurnPanicChance()
{
	return FMath::Clamp(CVarBurnPanic.GetValueOnGameThread(), 0.f, 1.f);
}

float CreatureStatusPose::LiveBurnPanicCooldown()
{
	return FMath::Max(CVarBurnPanicCooldown.GetValueOnGameThread(), 0.f);
}

float CreatureStatusPose::LiveStatusPoseScale()
{
	return FMath::Clamp(CVarStatusPose.GetValueOnGameThread(), 0.f, 2.f);
}

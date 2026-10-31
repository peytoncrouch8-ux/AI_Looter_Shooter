// AUnpaidCreature's rise out of the ground (2026-10-31; the user's pick for the Unpaid's AI: "Unpaid rise from the ground near
// the player"). UnpaidRiseRules has the rules (phases, depths, the surround's slots); this is where they meet the creature: its
// burst slot and its spot round the player, its stagger buried unseen (its capsule letting pawns through, its tag hidden), the
// moments of its climb (the rumble's sound, the burst, its hands planted, the heaves, its first breath, the howl of the first
// to stand) and its turn to its prey. Its pose (the channels, the trunk, the arms, its depth) is UnpaidCreatureRisePose.cpp,
// its look in the world UnpaidCreatureRiseLook.cpp. The quick rise (RiseIn: a boss's adds, a toll's dead, the miners) plays
// the same climb, shorter and silent (its caller has its own cue), with no wait.

#include "Creatures/UnpaidLifeWork.h"
#include "Creatures/UnpaidCreature.h"
#include "AI_Looter_Shooter.h"
#include "Audio/CreatureVoiceComponent.h"
#include "Audio/LooterSound.h"
#include "Creatures/CreatureReactComponent.h"
#include "Creatures/CreatureReactRules.h"
#include "Creatures/FightDirectorSubsystem.h"
#include "Creatures/UnpaidFightTuning.h"
#include "Creatures/UnpaidLifeTuning.h"
#include "Creatures/UnpaidPackSubsystem.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"

namespace
{
	/** Through the haul and the stand it turns to its prey at most this fast (degrees a second). */
	constexpr float RiseTurnRate = 220.f;
	/** While the ground rumbles over it, it stirs every this long (s). */
	constexpr float StirSeconds = 0.13f;
	/** A quick rise's burst against an ambush's 1. */
	constexpr float QuickBurst = 0.75f;
	/** Its first breath flares its coal this bright. */
	constexpr float BreathFlare = 0.9f;
	/** Killed this far under its hang (cm) or more, it sinks back into its grave as it dies (UnpaidCreatureRisePose.cpp). */
	constexpr float DiedDeep = 20.f;
}

// ---------------------------------------------------------------------------
// Starting a rise
// ---------------------------------------------------------------------------

void AUnpaidCreature::RiseIn()
{
	FUnpaidRiseWork::Begin(*this, nullptr, EUnpaidRiseStyle::Quick, false, 0.f);
}

void AUnpaidCreature::RiseAmbush(const APawn* Around, bool bSurround, float Radius)
{
	FUnpaidRiseWork::Begin(*this, Around, Around ? EUnpaidRiseStyle::Ambush : EUnpaidRiseStyle::Quick, bSurround, Radius);
}

void FUnpaidRiseWork::Begin(AUnpaidCreature& Unpaid, const APawn* Around, EUnpaidRiseStyle Style, bool bSurround, float Radius)
{
	if (Unpaid.IsDead() || Style == EUnpaidRiseStyle::None)
	{
		return;
	}
	const FUnpaidLifeTuning Tuning = FUnpaidLifeTuning::Live();
	FUnpaidRiseState& Rise = Unpaid.Kit.Rise;
	// A rise already under way starts over cleanly.
	if (Rise.bPawnsThrough)
	{
		LetPawnsThrough(Unpaid, false);
	}
	Rise = FUnpaidRiseState();
	Rise.Style = Style;
	Rise.For = Around;
	Rise.LeadArm = FMath::RandBool() ? 1 : 0;

	float Delay = 0.f;
	if (Style == EUnpaidRiseStyle::Ambush)
	{
		// Its place in its pack's burst at the player: its stagger, and (round them) its bearing.
		UUnpaidPackSubsystem* Packs = UUnpaidPackSubsystem::Get(&Unpaid);
		FUnpaidRiseBurst Alone;
		FUnpaidRiseBurst& Round = Packs ? Packs->BurstFor(Around) : Alone;
		const UWorld* World = Unpaid.GetWorld();
		const double Now = World ? World->GetTimeSeconds() : 0.0;
		const float ViewYaw = static_cast<float>(Around ? Around->GetViewRotation().Yaw : Unpaid.GetActorRotation().Yaw);
		Rise.Slot = UnpaidRiseRules::ClaimSlot(Round, Now, ViewYaw, Tuning.RiseWindow, FMath::FRand(), Tuning.RiseTurn);
		// Round the player who sprang it: a melee one only (a support kind, the Bell-Ringer, keeps its place behind its pack).
		const bool bAround = bSurround && Around && Unpaid.GetFightRole() == EFightRole::Melee;
		FVector Feet = FeetOf(Unpaid);
		FString Why;
		if (bAround && FindSurroundSpot(Unpaid, *Around, Round, Rise.Slot, Radius > 0.f ? Radius : Tuning.RiseRadius, Feet, &Why))
		{
			// Unseen (it's whole only once its hand breaks the ground): there now, facing them.
			const float HalfHeight = Unpaid.GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
			const float Facing = static_cast<float>((Around->GetActorLocation() - Feet).Rotation().Yaw);
			Unpaid.SetActorLocationAndRotation(Feet + FVector(0.f, 0.f, HalfHeight + 2.f), FRotator(0.f, Facing, 0.f), false, nullptr,
				ETeleportType::TeleportPhysics);
			for (FShroudChain& Chain : Unpaid.Chains)
			{
				Chain.Settle();
			}
			Unpaid.LastYaw = Facing;
		}
		else if (bAround)
		{
			UE_LOG(LogLooter, Verbose, TEXT("%s (rise slot %d): nowhere round %s to rise; it rises where it was put. %s"), *Unpaid.GetName(),
				Rise.Slot, *Around->GetName(), *Why);
		}
		Round.Spots.Add(Feet);
		Delay = UnpaidRiseRules::StaggerDelay(Rise.Slot, Tuning.RiseFirst, Tuning.RiseGap, FMath::FRand());
		UUnpaidPackSubsystem::Note(&Unpaid, EUnpaidTally::Rise);
	}
	Rise.Timing = UnpaidRiseRules::Timing(Style, Delay, Tuning.RiseRumble);
	Rise.Phase = UnpaidRiseRules::PhaseAt(0.f, Rise.Timing, Rise.Into);
	Rise.StirIn = 0.f;

	// Gone, fading back in as a phase-step's end does once its hand breaks the ground (buried, TickPhaseStep holds it whole).
	Unpaid.Kit.PhaseState = EUnpaidPhaseState::In;
	Unpaid.Kit.PhaseTime = 0.f;
	Unpaid.PhaseAmount = 1.f;
	Unpaid.Kit.LandedGap = TNumericLimits<float>::Max();
	Unpaid.ApplyLook(true);
	if (UCharacterMovementComponent* Movement = Unpaid.GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
	}
	// Its brain waits until it straightens: it stands still unseen, and comes for them once it's out.
	Unpaid.HoldBrain(Rise.Timing.BrainFree());
	if (Style == EUnpaidRiseStyle::Ambush)
	{
		// Under the ground nothing walks into it.
		LetPawnsThrough(Unpaid, true);
	}
	else
	{
		// A quick rise breaks the ground at once.
		Rise.bBurst = true;
		Burst(Unpaid, QuickBurst);
	}
	UE_LOG(LogLooter, Verbose, TEXT("%s rises (%s, slot %d, %.2f s buried, %.2f s in all)."), *Unpaid.GetName(),
		Style == EUnpaidRiseStyle::Ambush ? TEXT("ambush") : TEXT("quick"), Rise.Slot, Rise.Timing.Buried, Rise.Timing.Length());
}

bool FUnpaidRiseWork::FindSurroundSpot(AUnpaidCreature& Unpaid, const APawn& Around, FUnpaidRiseBurst& Round, int32 Slot, float Radius,
	FVector& OutFeet, FString* OutWhy)
{
	// Each candidate's first refusal, counted for the log: too near the player or another of the burst, off its hunting ground,
	// CanStandAt's (1 no ground, 2 steep or an obstacle's top, 3 no room, 4 no sight of the player, 5 a safe zone or past the
	// playable area), or another level than theirs.
	int32 Crowded = 0;
	int32 OffGround = 0;
	int32 Refused[6] = {};
	int32 OtherLevel = 0;
	int32 Candidates = 0;
	const FUnpaidLifeTuning Tuning = FUnpaidLifeTuning::Live();
	const float Scale = Unpaid.GetSizeScale();
	const FVector Center = Around.GetActorLocation();
	const float TheirFeet = static_cast<float>(Center.Z) - Around.GetSimpleCollisionHalfHeight();
	const float Bearing = UnpaidRiseRules::SurroundBearing(Slot, Round.ViewYaw, Round.Turn);
	const float Out = UnpaidRiseRules::SurroundRadius(Slot, Radius) * Scale;
	for (const FVector& Candidate : UnpaidRiseRules::SurroundCandidates(FVector(Center.X, Center.Y, TheirFeet), Bearing, Out))
	{
		++Candidates;
		// Clear of the player and of the burst's others, on its own ground, and where it could stand after a phase-step (ground,
		// room, the player in sight, no safe zone), on their level.
		if (!UnpaidRiseRules::IsClear(Candidate, Round.Spots, Tuning.RiseSpacing * Scale, Center, Tuning.RiseClear * Scale))
		{
			++Crowded;
			continue;
		}
		if (!Unpaid.HuntingGround.ContainsSpot(Candidate, Unpaid.GetHome().GetLocation(), Unpaid.GetGroundReach()))
		{
			++OffGround;
			continue;
		}
		FVector Ground;
		int32 Refusal = 0;
		if (!Unpaid.CanStandAt(Candidate, Ground, &Refusal, false, &Around))
		{
			++Refused[FMath::Clamp(Refusal, 0, 5)];
			continue;
		}
		if (FMath::Abs(static_cast<float>(Ground.Z) - TheirFeet) > Unpaid.PhaseStep.MaxRise)
		{
			++OtherLevel;
			continue;
		}
		OutFeet = Ground;
		return true;
	}
	if (OutWhy)
	{
		*OutWhy = FString::Printf(TEXT("%d spots %.0f cm out: %d crowded (player or burst), %d off its ground, %d no ground, %d steep or an ")
			TEXT("obstacle's top, %d no room, %d no sight of the player, %d in a safe zone or past the playable area, %d other level, %d other."),
			Candidates, Out, Crowded, OffGround, Refused[1], Refused[2], Refused[3], Refused[4], Refused[5], OtherLevel, Refused[0]);
	}
	return false;
}

void FUnpaidRiseWork::LetPawnsThrough(AUnpaidCreature& Unpaid, bool bThrough)
{
	FUnpaidRiseState& Rise = Unpaid.Kit.Rise;
	UCapsuleComponent* Capsule = Unpaid.GetCapsuleComponent();
	if (!Capsule || bThrough == Rise.bPawnsThrough)
	{
		return;
	}
	if (bThrough)
	{
		Rise.PawnResponse = static_cast<uint8>(Capsule->GetCollisionResponseToChannel(ECC_Pawn));
		Capsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	}
	else
	{
		Capsule->SetCollisionResponseToChannel(ECC_Pawn, static_cast<ECollisionResponse>(Rise.PawnResponse));
	}
	Rise.bPawnsThrough = bThrough;
}

// ---------------------------------------------------------------------------
// Each frame of it
// ---------------------------------------------------------------------------

void FUnpaidRiseWork::Tick(AUnpaidCreature& Unpaid, float DeltaSeconds)
{
	FUnpaidRiseState& Rise = Unpaid.Kit.Rise;
	if (!Rise.IsRising() || Unpaid.IsDead())
	{
		return;
	}
	Rise.Time += FMath::Max(DeltaSeconds, 0.f);
	const EUnpaidRisePhase Was = Rise.Phase;
	Rise.Phase = UnpaidRiseRules::PhaseAt(Rise.Time, Rise.Timing, Rise.Into);
	const bool bAmbush = Rise.Style == EUnpaidRiseStyle::Ambush;
	const float VoicePitch = Unpaid.Voice ? Unpaid.Voice->GetPitch() : 1.f;
	if (Rise.Phase == EUnpaidRisePhase::None)
	{
		// Up and whole: it's one of the fight now.
		Stop(Unpaid, /*bDied*/ false);
		return;
	}

	// Nothing of it shows over the ground until its head is out: no tag floating over its grave.
	if (Rise.Phase == EUnpaidRisePhase::Buried || Rise.Phase == EUnpaidRisePhase::Breach)
	{
		if (Unpaid.HealthBar)
		{
			Unpaid.HealthBar->SetVisibility(false);
		}
	}
	if (Rise.Phase == EUnpaidRisePhase::Buried)
	{
		// The tell: the ground stirs over it a moment before its hand breaks it, and a low rumble.
		if (UnpaidRiseRules::IsRumbling(Rise.Time, Rise.Timing))
		{
			if (!Rise.bRumbled)
			{
				Rise.bRumbled = true;
				LooterSound::PlayAt(&Unpaid, FName(UnpaidLifeCue::RiseRumble), FeetOf(Unpaid), 1.f, VoicePitch);
			}
			Rise.StirIn -= DeltaSeconds;
			if (Rise.StirIn <= 0.f)
			{
				Rise.StirIn = StirSeconds;
				Stir(Unpaid);
			}
		}
		return;
	}

	// Its hand breaks the ground: the dirt bursts, and it starts to come whole (the phase-step's fade, from now).
	if (!Rise.bBurst)
	{
		Rise.bBurst = true;
		Unpaid.Kit.PhaseTime = 0.f;
		Burst(Unpaid, 1.f);
		LooterSound::PlayAt(&Unpaid, FName(UnpaidLifeCue::RiseBurst), FeetOf(Unpaid), 1.f, VoicePitch);
	}

	if (Rise.Phase == EUnpaidRisePhase::Haul || Rise.Phase == EUnpaidRisePhase::Stand)
	{
		// Out of the ground enough to stand in the way again.
		if (Rise.bPawnsThrough)
		{
			LetPawnsThrough(Unpaid, false);
		}
		// It turns to its prey as it comes out.
		const APawn* Prey = Rise.For.IsValid() ? Rise.For.Get() : Unpaid.GetTarget();
		if (Prey)
		{
			const float Want = static_cast<float>((Prey->GetActorLocation() - Unpaid.GetActorLocation()).Rotation().Yaw);
			const float Now = static_cast<float>(Unpaid.GetActorRotation().Yaw);
			Unpaid.SetActorRotation(FRotator(0.f, FMath::FixedTurn(Now, Want, RiseTurnRate * DeltaSeconds), 0.f));
		}
	}

	if (Rise.Phase == EUnpaidRisePhase::Haul)
	{
		// The ambush's climb has its own heave and breath (the old rise's cue: the earth heaving, the long inhale).
		if (bAmbush && Was != EUnpaidRisePhase::Haul)
		{
			LooterSound::PlayAt(&Unpaid, FName(CreatureReactCue::UnpaidRise), FeetOf(Unpaid), 1.f, VoicePitch);
		}
		const int32 Lead = FMath::Clamp(Rise.LeadArm, 0, 1);
		if (!Rise.bPlanted[Lead] && Rise.Into >= UnpaidRiseRules::LeadPlantAt)
		{
			Rise.bPlanted[Lead] = true;
			Plant(Unpaid, Lead);
		}
		if (!Rise.bPlanted[1 - Lead] && Rise.Into >= UnpaidRiseRules::OtherPlantAt)
		{
			Rise.bPlanted[1 - Lead] = true;
			Plant(Unpaid, 1 - Lead);
		}
		if (!Rise.bHeaved && Rise.Into >= UnpaidRiseRules::SecondHeaveAt)
		{
			Rise.bHeaved = true;
			if (UCreatureReactComponent* Reacts = Unpaid.GetReact())
			{
				Reacts->ThrowGraveDirt(FeetOf(Unpaid), Unpaid.GetSizeScale(), bAmbush ? 0.45f : 0.3f);
			}
		}
	}

	if (Rise.Phase == EUnpaidRisePhase::Stand && !Rise.bStood)
	{
		// Its first breath: its coal catches, it draws a breath it shouldn't have.
		Rise.bStood = true;
		Unpaid.Heat = FMath::Max(Unpaid.Heat, BreathFlare);
		if (bAmbush)
		{
			Unpaid.Voice->Play(Unpaid.NoticeCue.IsNone() ? FName(CreatureReactCue::UnpaidNotice) : Unpaid.NoticeCue);
			// The first of a burst to stand howls (once a player per the howl's cooldown): the dead know you're here.
			UFightDirectorSubsystem* Director = UFightDirectorSubsystem::Get(&Unpaid);
			const APawn* Prey = Rise.For.Get();
			if (Rise.Slot == 0 && Prey && Director && Director->TryHowl(*Prey, FUnpaidFightTuning::Live().HowlCooldown))
			{
				LooterSound::PlayAt(&Unpaid, FName(UnpaidKitCue::Howl), Unpaid.GetActorLocation(), 1.f, VoicePitch);
			}
		}
	}
}

bool FUnpaidRiseWork::HoldsPhase(const AUnpaidCreature& Unpaid)
{
	const FUnpaidRiseState& Rise = Unpaid.Kit.Rise;
	return Rise.IsRising() && Rise.Phase == EUnpaidRisePhase::Buried;
}

void FUnpaidRiseWork::Stop(AUnpaidCreature& Unpaid, bool bDied)
{
	FUnpaidRiseState& Rise = Unpaid.Kit.Rise;
	if (Rise.bPawnsThrough)
	{
		LetPawnsThrough(Unpaid, false);
	}
	if (bDied && Rise.IsRising())
	{
		// Shot down as it climbs: it goes back into its grave as it breaks up, never popping up to crumple.
		const float Sink = UnpaidRiseRules::Sink(Rise.Style, Rise.Phase, Rise.Into);
		Rise = FUnpaidRiseState();
		Rise.bDiedRising = Sink < -DiedDeep;
		Rise.DiedAt = Sink;
		return;
	}
	Rise = FUnpaidRiseState();
}


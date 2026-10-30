// AUnpaidCreature's failed-lunge shriek (2026-10-29; the user's brief: "a AAA failed-lunge shriek"). A lunge that misses (the
// player dodged, slid, or it hit cover) sometimes (Looter.Unpaid.Shriek.Chance, its own and its pack's cooldowns) turns into a
// shriek instead of another lunge: it stops, turns on the player and screams.
//  - Tell (0.5 s): it rears back, head up, jaw dropping wide, arms spread, claws curling, shroud flaring, its coal brightening
//    and pulsing fast; cold soul motes are drawn into its mouth. Shooting its coal now chokes it off: a stagger, its coal
//    exposed a moment (a bigger crit spot), a choked cry.
//  - Scream (1.2 s): its head thrusts forward shaking, jaw at its widest, body and shroud shuddering, its hat lifting off its
//    head. A shockwave ring rolls out along the ground lifting dust, grass and leaves (UUnpaidPackSubsystem's wave), a cone
//    of spectral wisps and soul streaks out of its mouth, frost on its breath, embers and soot
//    flung from its coal, its coal's light flaring round it. Players near it are rattled (UPlayerRattleSubsystem: a short aim
//    sway and a slight slow within RattleRadius, never a stun; the shake falling off, frost at the screen's edges, a colour
//    wobble and an arc on the HUD to where it came from), its pack within RallyRadius rallied (the frenzy's pace a while).
//  - Recovery (0.6 s): hunched, panting, then back into the fight.
// Its pose is UnpaidShriekPose.cpp. One light each from a pool of three; particles at fixed rates, thinned by distance.

// The flow is here (start, tick, end, the interrupt); its look (cry, rings, rattle, rally, FX, light) is UnpaidCreatureShriekLook.cpp.

#include "Creatures/UnpaidKitWork.h"
#include "Creatures/UnpaidShriekWork.h"
#include "Creatures/UnpaidCreature.h"
#include "AI_Looter_Shooter.h"
#include "Audio/CreatureVoiceComponent.h"
#include "Audio/LooterSound.h"
#include "Creatures/CreatureHitReactionComponent.h"
#include "Creatures/CreatureKnockComponent.h"
#include "Creatures/FightDirectorSubsystem.h"
#include "Creatures/UnpaidMoveRules.h"
#include "Creatures/UnpaidMoveTuning.h"
#include "Creatures/UnpaidPackSubsystem.h"
#include "Weapons/VfxTuning.h"
#include "Weapons/WeaponFX.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"

using namespace UnpaidShriekWork;

namespace
{
	/** It turns on its target this fast (1/s) through the tell, the scream (it's committed) and the recovery. */
	constexpr float TellTurnRate = 9.f;
	constexpr float ScreamTurnRate = 2.5f;
	constexpr float RecoverTurnRate = 4.f;
}

// ---------------------------------------------------------------------------
// Start, frame, end
// ---------------------------------------------------------------------------

bool FUnpaidShriekWork::Start(AUnpaidCreature& Unpaid)
{
	const APawn* Victim = Unpaid.GetTarget();
	if (!Victim || Unpaid.IsDead() || !FUnpaidMoveWork::IsOn(Unpaid) || Unpaid.Kit.Action != EUnpaidAction::None || Unpaid.IsPhasing()
		|| Unpaid.Kit.bLunging || Unpaid.IsFadingBack() || Unpaid.GetCreatureState() != ECreatureState::Chase)
	{
		return false;
	}
	const FUnpaidMoveTuning Tuning = FUnpaidMoveTuning::Live();
	UUnpaidPackSubsystem* Packs = UUnpaidPackSubsystem::Get(&Unpaid);
	// One a pack at one player a gap (two of a pack that missed together don't both scream).
	if (Packs && Packs->SincePackShriek(*Victim, Unpaid.PackTag) < Tuning.ShriekPackGap)
	{
		return false;
	}
	if (Packs)
	{
		Packs->NotePackShriek(*Victim, Unpaid.PackTag);
		Packs->Count(EUnpaidTally::Shriek);
	}
	FUnpaidMoveState& State = Unpaid.Kit.Moves;
	Unpaid.Kit.Action = EUnpaidAction::Shriek;
	Unpaid.Kit.ActionTime = 0.f;
	Unpaid.Kit.bThrown = false;
	State.Shriek = EUnpaidShriekPhase::Tell;
	State.ShriekTime = 0.f;
	State.SinceShriek = 0.f;
	State.bScreamed = false;
	State.MotesOwed = State.WispsOwed = State.EmbersOwed = State.FrostOwed = 0.f;
	State.HangLeft = 0.f;
	State.Planned = EUnpaidMove::None;
	// Its voice's own pitch, and a little of its own on top (from its pose's seed): a pack's shrieks never sound copied.
	State.ShriekPitch = Unpaid.Voice->GetPitch() * (0.94f + 0.12f * FMath::Frac(Unpaid.PoseSeed * 0.618f));
	Unpaid.GetCharacterMovement()->StopMovementImmediately();
	// Out of the turns' line while it shrieks: a turn now would only wait on it.
	if (UFightDirectorSubsystem* Director = UFightDirectorSubsystem::Get(&Unpaid))
	{
		Director->RestFromTurns(Unpaid, Tuning.ShriekLength());
	}
	State.Light = Packs ? Packs->TakeLight(&Unpaid) : INDEX_NONE;
	// Its hat as it sits now (on its head, not knocked off): the scream lifts it from there and settles it back.
	State.bHatLifted = false;
	if (UStaticMeshComponent* HatMesh = Unpaid.GetHat(); HatMesh && HatMesh->IsVisible() && !Unpaid.Hurt.Hat.IsOff())
	{
		State.HatRestLocation = HatMesh->GetRelativeLocation();
		State.HatRestTurn = HatMesh->GetRelativeRotation().Quaternion();
		State.bHatLifted = true;
	}
	LooterSound::PlayAttached(FName(Cue::Draw), Unpaid.GetMesh(), Unpaid.Rig.Head, 1.f, State.ShriekPitch);
	UE_LOG(LogLooter, Verbose, TEXT("%s shrieks at %s."), *Unpaid.GetName(), *Victim->GetName());
	return true;
}

void FUnpaidShriekWork::Tick(AUnpaidCreature& Unpaid, float DeltaSeconds, const APawn& Victim)
{
	FUnpaidMoveState& State = Unpaid.Kit.Moves;
	const FUnpaidMoveTuning Tuning = FUnpaidMoveTuning::Live();
	State.ShriekTime += DeltaSeconds;
	Unpaid.Kit.ActionTime = State.ShriekTime;
	// Its fight moved on (a search, a knock that throws it, a blink): called off.
	if (Unpaid.GetCreatureState() != ECreatureState::Chase || Unpaid.GetKnock()->HoldsBrain() || Unpaid.IsPhasing())
	{
		Unpaid.EndKitAction(true);
		return;
	}
	const EUnpaidShriekPhase Phase = UnpaidMoveRules::ShriekPhaseAt(State.ShriekTime, Tuning.ShriekTell, Tuning.ShriekScream, Tuning.ShriekRecover);
	if (Phase == EUnpaidShriekPhase::None)
	{
		Unpaid.EndKitAction(false);
		return;
	}
	// Staggered in its tell it chokes on it (its coal shot outright interrupts it: OnHurt); once it screams it's committed.
	if (Phase == EUnpaidShriekPhase::Tell && Unpaid.IsHeldByHit())
	{
		Unpaid.EndKitAction(true);
		return;
	}
	const EUnpaidShriekPhase Was = State.Shriek;
	State.Shriek = Phase;
	if (Phase == EUnpaidShriekPhase::Scream && !State.bScreamed)
	{
		Scream(Unpaid, Tuning);
	}
	else if (Phase == EUnpaidShriekPhase::Recover && Was != EUnpaidShriekPhase::Recover)
	{
		LooterSound::PlayAttached(FName(Cue::Pant), Unpaid.GetMesh(), Unpaid.Rig.Head, 1.f, State.ShriekPitch);
	}
	// It turns on them: quickly as it rears up, barely once it screams (it's committed), slowly as it recovers.
	const float Rate = Phase == EUnpaidShriekPhase::Tell ? TellTurnRate : (Phase == EUnpaidShriekPhase::Scream ? ScreamTurnRate : RecoverTurnRate);
	const FVector To = Victim.GetActorLocation() - Unpaid.GetActorLocation();
	if (!To.IsNearlyZero())
	{
		const FRotator Facing(0.f, static_cast<float>(To.Rotation().Yaw), 0.f);
		Unpaid.SetActorRotation(FMath::RInterpTo(Unpaid.GetActorRotation(), Facing, DeltaSeconds, Rate));
	}
	Show(Unpaid, DeltaSeconds, Phase, Tuning);
	TickBody(Unpaid, DeltaSeconds);
}

void FUnpaidShriekWork::CleanUp(AUnpaidCreature& Unpaid, bool bCalledOff)
{
	FUnpaidMoveState& State = Unpaid.Kit.Moves;
	State.Shriek = EUnpaidShriekPhase::None;
	State.ShriekTime = 0.f;
	if (State.Light != INDEX_NONE)
	{
		if (UUnpaidPackSubsystem* Packs = UUnpaidPackSubsystem::Get(&Unpaid))
		{
			Packs->GiveLight(State.Light);
		}
		State.Light = INDEX_NONE;
	}
	SettleHat(Unpaid);
	if (bCalledOff)
	{
		UE_LOG(LogLooter, Verbose, TEXT("%s's shriek is called off."), *Unpaid.GetName());
	}
}

// ---------------------------------------------------------------------------
// The interrupt: its coal shot in the tell
// ---------------------------------------------------------------------------

bool FUnpaidShriekWork::OnHurt(AUnpaidCreature& Unpaid, bool bCritical, const FVector& HitLocation)
{
	FUnpaidMoveState& State = Unpaid.Kit.Moves;
	if (Unpaid.Kit.Action != EUnpaidAction::Shriek || !UnpaidMoveRules::InterruptsShriek(State.Shriek, bCritical))
	{
		return false;
	}
	const FUnpaidMoveTuning Tuning = FUnpaidMoveTuning::Live();
	const float Scale = Unpaid.GetSizeScale();
	State.ExposedLeft = Tuning.ShriekExposedSeconds;
	// Choked on its own scream: staggered (at its rank, past any cooldown), thrown back from the shot.
	FVector Away = Unpaid.GetActorLocation() - HitLocation;
	Away.Z = 0.f;
	Unpaid.HitReaction->WardBreak(Away.GetSafeNormal());
	LooterSound::PlayAttached(FName(Cue::Choke), Unpaid.GetMesh(), Unpaid.Rig.Head, 1.f, State.ShriekPitch);
	// Its coal cracks and sputters: embers torn out of its back along the shot, a hot pop, a puff of soot.
	if (FWeaponFX* Effects = ShriekEffects(Unpaid.GetWorld()))
	{
		const FVector Coal = Unpaid.GetCoalLocation();
		const FLinearColor Tint = Unpaid.GetCoalColor();
		const FVector Shot = Unpaid.GetLastHit().Direction.IsNearlyZero() ? -Away.GetSafeNormal() : Unpaid.GetLastHit().Direction.GetSafeNormal();
		const float Glow = FVfxTuning::Live().Glow;
		Effects->SpawnEmberTear(Coal, Shot, 160.f * Scale, Tint, 10);
		Effects->SpawnStarburst(Coal, 42.f * Scale, FMath::Lerp(Tint, FLinearColor::White, 0.5f), 5.f * Glow, 0.12f, 7);
		Effects->SpawnCoalFling(Coal, FVector::UpVector, Tint, 8, true);
		Effects->SpawnEmberPuff(Coal, Tint, 26.f * Scale);
	}
	Unpaid.Heat = 1.2f;
	UUnpaidPackSubsystem::Note(&Unpaid, EUnpaidTally::ShriekInterrupted);
	UE_LOG(LogLooter, Verbose, TEXT("%s chokes on its shriek: its coal shot."), *Unpaid.GetName());
	Unpaid.EndKitAction(true);
	return true;
}

float FUnpaidShriekWork::CoalExposure(const AUnpaidCreature& Unpaid)
{
	return Unpaid.Kit.Moves.ExposedLeft > 0.f ? FUnpaidMoveTuning::Live().ShriekExposedCoal : 1.f;
}

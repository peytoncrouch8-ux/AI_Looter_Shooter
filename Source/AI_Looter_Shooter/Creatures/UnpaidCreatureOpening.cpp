// The Unpaid pack's punish of an opening (2026-10-31; the brief's "punish a reload"; the rules: UnpaidOpeningRules). Each frame
// an Unpaid fights with its kit it asks the pack's look at its target (read once a frame, whoever asks: the gun in their hands
// reloading, being swapped or empty, the soul flask at their lips). As an opening begins one of the pack may claim its punish:
// one behind the player or at their side (or right on them), near, seeing them and free to act. It hisses (the tell, heard
// from where it is), its coal flares, its pack's frenzy opens for a moment (one more turn at the player at once, the frenzy's
// pace and glow; never over a frenzy already on, nor in the lull after its leader fell), and while the player is open every
// turn picked favours a quick commit (UnpaidMoveRules::Weights, through FUnpaidMoveContext::bOpening and bPunisher). One
// punish per player per Looter.Unpaid.Opening.Gap: reloading behind cover or away from the pack draws nothing. Plain Unpaid
// with their kit and a fight director only (TickFightKit's gate); Looter.Unpaid.Opening 0 turns it off.

#include "Creatures/UnpaidLifeWork.h"
#include "Creatures/UnpaidCreature.h"
#include "AI_Looter_Shooter.h"
#include "Audio/CreatureVoiceComponent.h"
#include "Audio/LooterSound.h"
#include "Creatures/FightDirectorSubsystem.h"
#include "Creatures/FightRules.h"
#include "Creatures/PackRules.h"
#include "Creatures/UnpaidFightTuning.h"
#include "Creatures/UnpaidLifeTuning.h"
#include "Creatures/UnpaidPackSubsystem.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Player/PlayerFlaskComponent.h"
#include "Weapons/WeaponBase.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"

FUnpaidOpeningRead FUnpaidOpeningWork::Read(const APawn& Victim)
{
	FUnpaidOpeningRead Hands;
	const UWeaponManagerComponent* Weapons = Victim.FindComponentByClass<UWeaponManagerComponent>();
	const AWeaponBase* Gun = Weapons ? Weapons->GetActiveWeapon() : nullptr;
	Hands.bSwapping = Weapons && Weapons->IsSwapping();
	if (Gun)
	{
		Hands.bReloading = Gun->IsReloading();
		Hands.bEmpty = !Hands.bReloading && Gun->GetCurrentMagazine() <= 0;
	}
	Hands.bDrinking = UPlayerFlaskComponent::IsDrinkingActor(&Victim);
	return Hands;
}

bool FUnpaidOpeningWork::IsOpen(const AUnpaidCreature& Unpaid, const APawn& Victim)
{
	UUnpaidPackSubsystem* Packs = UUnpaidPackSubsystem::Get(&Unpaid);
	return Packs && FUnpaidLifeTuning::Live().Opening != 0 && Packs->OpeningFor(Victim).bOpen;
}

bool FUnpaidOpeningWork::IsPunisher(const AUnpaidCreature& Unpaid, const APawn& Victim)
{
	UUnpaidPackSubsystem* Packs = UUnpaidPackSubsystem::Get(&Unpaid);
	const UWorld* World = Unpaid.GetWorld();
	if (!Packs || !World)
	{
		return false;
	}
	return UnpaidOpeningRules::IsPunisher(Packs->OpeningFor(Victim), Unpaid.GetUniqueID(), World->GetTimeSeconds(),
		FUnpaidLifeTuning::Live().OpeningPunish);
}

bool FUnpaidOpeningWork::Tick(AUnpaidCreature& Unpaid, const APawn& Victim)
{
	const FUnpaidLifeTuning Tuning = FUnpaidLifeTuning::Live();
	UUnpaidPackSubsystem* Packs = UUnpaidPackSubsystem::Get(&Unpaid);
	UFightDirectorSubsystem* Director = UFightDirectorSubsystem::Get(&Unpaid);
	const UWorld* World = Unpaid.GetWorld();
	if (Tuning.Opening == 0 || !Packs || !Director || !World)
	{
		return false;
	}
	// The pack's look at the player, once a frame whoever asks first.
	const double Now = World->GetTimeSeconds();
	FUnpaidOpeningGate& Gate = Packs->OpeningFor(Victim);
	if (Gate.ReadAt != Now)
	{
		UnpaidOpeningRules::Step(Gate, Read(Victim).IsOpen(), Now, Tuning.OpeningGap, Tuning.OpeningClaim);
	}
	if (!Gate.bDue)
	{
		return false;
	}
	// Free to act on it: chasing (on the ring or closing), not mid-move, not staggered, not blind to them.
	if (Unpaid.GetCreatureState() != ECreatureState::Chase || Unpaid.IsPhasing() || Unpaid.IsRising() || Unpaid.Kit.bLunging
		|| Unpaid.Kit.Action != EUnpaidAction::None || Unpaid.IsFadingBack() || Unpaid.IsHeldByHit() || Unpaid.IsBlindToTurns())
	{
		return false;
	}
	// Behind them or at a side (where they aren't looking while their hands are busy), or right on them; near, and seeing them.
	const FVector Here = Unpaid.GetActorLocation();
	const FVector There = Victim.GetActorLocation();
	const float Distance = static_cast<float>(FVector::Dist2D(Here, There));
	const float Front = FightRules::Frontness(PackRules::BearingAround(There, Here), static_cast<float>(Victim.GetViewRotation().Yaw));
	const float Scale = Unpaid.GetSizeScale();
	if (!UnpaidOpeningRules::MayClaim(Front, Distance, Tuning.OpeningRange * Scale, Tuning.OpeningNear * Scale, Tuning.OpeningFront)
		|| !Unpaid.SeesTarget(Victim) || !UnpaidOpeningRules::Claim(Gate, Unpaid.GetUniqueID(), Now))
	{
		return false;
	}
	// The tell: a hungry hiss from where it is, its coal flaring; a hang-back is given up (it presses now).
	const FUnpaidFightTuning Kit = FUnpaidFightTuning::Live();
	LooterSound::PlayAt(&Unpaid, FName(UnpaidLifeCue::Opening), Here, 1.f, Unpaid.Voice->GetPitch());
	Unpaid.Heat = FMath::Max(Unpaid.Heat, Kit.TellFlare);
	Unpaid.Kit.Moves.HangLeft = 0.f;
	// Its pack comes on together a moment: one more turn at once, the frenzy's pace and glow.
	if (Tuning.OpeningFrenzy > 0.f && Director->GetFrenzyLeft(Victim, Unpaid.PackTag) <= 0.f && !Director->IsInLeaderLull(Unpaid))
	{
		Director->OpenFrenzy(Victim, Unpaid.PackTag, Tuning.OpeningFrenzy, Kit.FrenzyTokens);
	}
	UUnpaidPackSubsystem::Note(&Unpaid, EUnpaidTally::Opening);
	UE_LOG(LogLooter, Verbose, TEXT("%s punishes %s's opening (%.1f m off, frontness %.2f)."), *Unpaid.GetName(), *Victim.GetName(),
		Distance / 100.f, Front);
	return true;
}

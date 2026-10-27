// AGravemotherCreature's boss fight (GravemotherFight has its phases and pace): which of her attacks she makes (her one
// attack, wound up, struck and recovered, each move with its own timing: the roar, the Furrow, the Gravequake, the Grave
// Clod, the bite), her roar (its slam raises her brood's mounds at 66% and 33%), the Gravequake, her reel when her boss is
// staggered (and the clod's stun), her pace, her bar's stone bar, and her boss's events.

#include "Creatures/GravemotherCreature.h"
#include "AI_Looter_Shooter.h"
#include "Audio/CreatureVoiceComponent.h"
#include "Audio/LooterSound.h"
#include "Bosses/BossCameraShake.h"
#include "Bosses/BossComponent.h"
#include "Creatures/CreatureRankSettings.h"
#include "Creatures/GravemotherFX.h"
#include "Creatures/GravemotherStone.h"
#include "Creatures/GravemotherTuning.h"
#include "Creatures/GroundCrack.h"
#include "Scenes/BossIntro.h"
#include "UI/HUD/HudBossBarWidget.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"

namespace
{
	/** A roar starts wherever its target is: it strikes nobody. */
	constexpr float RoarFromAnywhere = 1.0e6f;
	/** The quake's ring of cracks, this wide (cm at a spider's size), and its glow's share of her rank's colour. */
	constexpr float QuakeCrackWidth = 23.f;
	/** After the quake's slam its ring closes over this long (s); a broken-off quake's at once. */
	constexpr float QuakeCrackLingers = 1.2f;
	/** A quake reaches a player no more than this far above or below her middle, past her half height (cm). */
	constexpr float QuakeLevel = 150.f;
	/** A later phase's quake comes no sooner than this after the phase starts (s): her roar first. */
	constexpr float QuakeAfterPhase = 3.f;
	/**
	 * Her sink as she reels: her body pushed down as a hit from this high over her pushes it, the point swaying this far
	 * side to side this fast (radians a second), so she trembles as she sinks.
	 */
	constexpr float SinkAbove = 1000.f;
	constexpr float SinkSway = 140.f;
	constexpr float SinkSwaySpeed = 18.f;
}

// ---------------------------------------------------------------------------
// Which attack
// ---------------------------------------------------------------------------

float AGravemotherCreature::GetAttackStartRange() const
{
	if (bReeling)
	{
		return 0.f;
	}
	// A roar from anywhere; a furrow from well out; the quake at whoever hugs her; a clod at whoever keeps away; else she
	// closes in to bite.
	if (bRoarWanted)
	{
		return RoarFromAnywhere;
	}
	const float Scale = GetSizeScale();
	if (WantsToFurrow())
	{
		return Furrows.Range * Scale;
	}
	if (WantsToQuake())
	{
		return QuakeRange * Scale;
	}
	if (WantsToClod())
	{
		return Clods.MaxDistance * Scale;
	}
	return Super::GetAttackStartRange();
}

bool AGravemotherCreature::CanStartAttack() const
{
	return !bReeling && FurrowState == EGravemotherFurrow::None && !HeldClod.IsValid() && Super::CanStartAttack();
}

void AGravemotherCreature::OnAttackStarted()
{
	// The same attack, several ways, each with its own timing.
	if (bRoarWanted)
	{
		BeginRoar();
	}
	else if (WantsToFurrow())
	{
		BeginFurrow();
	}
	else if (WantsToQuake())
	{
		BeginQuake();
	}
	else if (WantsToClod())
	{
		BeginClod();
	}
	else
	{
		Move = EGravemotherMove::Bite;
		AttackWindup = BiteWindup;
		AttackRecovery = BiteRecovery;
	}
	Super::OnAttackStarted();
}

void AGravemotherCreature::Strike()
{
	// Broken off by a stagger: nothing lands.
	if (bReeling)
	{
		return;
	}
	switch (Move)
	{
	case EGravemotherMove::Roar:
		RoarPeak();
		return;
	case EGravemotherMove::Quake:
		QuakeSlam();
		return;
	case EGravemotherMove::Clod:
		ThrowClod();
		return;
	case EGravemotherMove::Furrow:
		if (FurrowState == EGravemotherFurrow::Heave)
		{
			BeginPlough();
		}
		else
		{
			UE_LOG(LogLooter, Verbose, TEXT("%s: a furrow's strike with no heave under way (%s)."), *GetName(), *UEnum::GetValueAsString(FurrowState));
		}
		return;
	default:
		break;
	}
	// A bite.
	Super::Strike();
}

bool AGravemotherCreature::IsInQuake(const FVector& Where) const
{
	const float Scale = GetSizeScale();
	const UCapsuleComponent* Capsule = GetCapsuleComponent();
	const float Reach = QuakeRadius * Scale + 34.f;
	return FVector::Dist2D(Where, GetActorLocation()) <= Reach
		&& FMath::Abs(Where.Z - GetActorLocation().Z) <= Capsule->GetScaledCapsuleHalfHeight() + QuakeLevel;
}

bool AGravemotherCreature::WantsToQuake() const
{
	const APawn* Victim = GetTarget();
	if (!Pace.bQuake || QuakeCooldownLeft > 0.f || !Victim || IsDead())
	{
		return false;
	}
	return FVector::Dist2D(GetActorLocation(), Victim->GetActorLocation()) <= QuakeRange * GetSizeScale()
		&& IsInQuake(Victim->GetActorLocation());
}

// ---------------------------------------------------------------------------
// The roar (and at her brood's calls, the Unearthed's slam)
// ---------------------------------------------------------------------------

void AGravemotherCreature::BeginRoar()
{
	// She rears (the spider's attack pose through its wind-up) and screams, then slams her forelegs down.
	Move = EGravemotherMove::Roar;
	bRoarWanted = false;
	AttackWindup = RoarWindup;
	AttackRecovery = RoarRecovery;
	LooterSound::PlayAttached(LooterSoundCue::GravemotherRoar, GetRootComponent());
	KickDirt(GetFeet() - GetActorForwardVector() * GetCapsuleComponent()->GetScaledCapsuleRadius(), 0.5f);
	UE_LOG(LogLooter, Verbose, TEXT("%s rears and screams."), *GetName());
}

void AGravemotherCreature::RoarPeak()
{
	const float Scale = GetSizeScale();
	const FVector Slam = GetFeet() + GetActorForwardVector() * (GetCapsuleComponent()->GetScaledCapsuleRadius() * 1.2f);
	KickDirtRing(Slam, 90.f * Scale, 5, 0.7f);
	LooterSound::PlayAt(this, LooterSoundCue::GravemotherSlam, Slam);
	ShakeFrom(Slam, 0.55f, 0.7f);
	// Her brood's calls: the slam raises their mounds round her (Unearthed).
	if (PendingUnearths > 0)
	{
		const int32 Calls = PendingUnearths;
		PendingUnearths = 0;
		RaiseMounds(Calls);
	}
}

// ---------------------------------------------------------------------------
// The Gravequake
// ---------------------------------------------------------------------------

void AGravemotherCreature::BeginQuake()
{
	// She rears high while a ring of cracks spreads out under her to the quake's reach (the time to get out of it).
	Move = EGravemotherMove::Quake;
	AttackWindup = QuakeWindup;
	AttackRecovery = QuakeRecovery;
	const float Scale = GetSizeScale();
	const FVector Feet = GetFeet();
	if (AGroundCrack* Old = QuakeCrack.Get())
	{
		Old->Close(0.f);
	}
	AGroundCrack* Ring = AGroundCrack::Spawn(GetWorld(), Feet, Feet + GetActorForwardVector(), QuakeCrackWidth * Scale,
		UCreatureRankSettings::Get(GetRank()).Color, FMath::Rand());
	if (Ring)
	{
		Ring->BurstSeconds = QuakeWindup;
		Ring->Burst(Feet, QuakeRadius * Scale);
	}
	QuakeCrack = Ring;
	LooterSound::PlayAttached(LooterSoundCue::GravemotherQuake, GetRootComponent());
	KickDirtRing(Feet, QuakeRadius * Scale * 0.5f, 4, 0.3f);
	UE_LOG(LogLooter, Verbose, TEXT("%s rears for the Gravequake."), *GetName());
}

void AGravemotherCreature::QuakeSlam()
{
	const float Scale = GetSizeScale();
	const FVector Feet = GetFeet();
	QuakeCooldownLeft = Pace.QuakeCooldown;
	// Whoever is still inside the ring is thrown out of it.
	APawn* Victim = GetTarget();
	bool bHit = false;
	if (IsValidTarget(Victim) && IsInQuake(Victim->GetActorLocation()))
	{
		FVector Away = Victim->GetActorLocation() - GetActorLocation();
		Away.Z = 0.0;
		HitWithAttack(Victim, Away.IsNearlyZero() ? GetActorForwardVector() : Away.GetSafeNormal(), QuakeStrength);
		bHit = true;
		UE_LOG(LogLooter, Log, TEXT("%s's Gravequake caught %s."), *GetName(), *GetNameSafe(Victim));
	}
	if (AGroundCrack* Ring = QuakeCrack.Get())
	{
		Ring->Close(QuakeCrackLingers);
	}
	QuakeCrack = nullptr;
	KickDirt(Feet, 1.f);
	KickDirtRing(Feet, QuakeRadius * Scale * 0.85f, 8, 0.8f);
	LooterSound::PlayAt(this, LooterSoundCue::GravemotherSlam, Feet, 1.2f, 0.85f);
	ShakeFrom(Feet, bHit ? 0.9f : 0.65f, 0.8f);
}

// ---------------------------------------------------------------------------
// Her moves' clocks, the reel
// ---------------------------------------------------------------------------

void AGravemotherCreature::TickMoves(float DeltaSeconds)
{
	// Their cooldowns run while she's after someone, not while she dozes in her den.
	if (GetTarget())
	{
		QuakeCooldownLeft = FMath::Max(0.f, QuakeCooldownLeft - DeltaSeconds);
		ClodCooldownLeft = FMath::Max(0.f, ClodCooldownLeft - DeltaSeconds);
	}
	// A quake broken off (a stagger, her death): its ring closes.
	if (Move == EGravemotherMove::Quake && GetCreatureState() != ECreatureState::Attack)
	{
		if (AGroundCrack* Ring = QuakeCrack.Get())
		{
			Ring->Close(0.f);
		}
		QuakeCrack = nullptr;
	}
	// Her own reel (a clod's stun when her boss couldn't stagger her) runs its clock.
	if (OwnReelLeft > 0.f)
	{
		OwnReelLeft -= DeltaSeconds;
		if (OwnReelLeft <= 0.f)
		{
			OwnReelLeft = 0.f;
			EndReel();
		}
	}
	// Reeling, or her forelegs in the dirt after a plough that missed: sunk and trembling, open to punishment.
	SinkClock += DeltaSeconds;
	if (!IsDead() && (bReeling || (FurrowState == EGravemotherFurrow::Recover && !bFurrowHit)))
	{
		Sink();
	}
	if (bStunned)
	{
		StunClock += DeltaSeconds;
		DrawStunStars();
	}
	UpdateStoneBar();
}

void AGravemotherCreature::Sink()
{
	// The spider's own flinch, from a point high over her (swaying side to side): her body pushed down and rocked on her
	// planted legs, every frame while it lasts. Her brood calls are her own OnHurt's, so the spider's is called straight.
	const FVector Above = GetActorLocation() + GetActorRightVector() * (FMath::Sin(SinkClock * SinkSwaySpeed) * SinkSway)
		+ FVector(0.0, 0.0, SinkAbove);
	ASpiderCreature::OnHurt(true, Above);
}

void AGravemotherCreature::BeginReel()
{
	bReeling = true;
	SinkClock = 0.f;
	// Whatever she was at is broken off: its strike never lands (Strike does nothing while she reels) and her attack ends at
	// its next frame (its wind-up and recovery cut short): a clod in her grip falls, a furrow's tell fades, a plough stops.
	if (GetCreatureState() == ECreatureState::Attack)
	{
		AttackWindup = FMath::Min(AttackWindup, 0.05f);
		AttackRecovery = 0.05f;
	}
	DropClod();
	if (AGroundCrack* Ring = QuakeCrack.Get())
	{
		Ring->Close(0.f);
	}
	QuakeCrack = nullptr;
	// Held still while she reels (her pace's chase is nothing).
	ApplyPace(PacePhase);
	Voice->Play(LooterSoundCue::SpiderHurt, 1.3f);
	KickDirtRing(GetFeet(), GetCapsuleComponent()->GetScaledCapsuleRadius(), 4, 0.5f);
	UE_LOG(LogLooter, Log, TEXT("%s reels%s."), *GetName(), bStunned ? TEXT(", stunned") : TEXT(", staggered"));
}

void AGravemotherCreature::EndReel()
{
	bReeling = false;
	bStunned = false;
	ApplyPace(PacePhase);
}

void AGravemotherCreature::ApplyPace(int32 Phase)
{
	PacePhase = FMath::Clamp(Phase, 0, 2);
	Pace = GravemotherFight::PaceFor(PacePhase);
	if (!bOwnPaceCaptured)
	{
		return;
	}
	Furrows.Cooldown = OwnFurrowCooldown * Pace.FurrowCooldownScale;
	// Held still while she reels; a quarter faster in her last third (the plough reads the pace itself).
	ChaseSpeed = bReeling ? 0.f : OwnChaseSpeed * Pace.SpeedScale;
}

void AGravemotherCreature::UpdateStoneBar()
{
	// The thin stone bar under her boss bar while the stone stands (Main's call, 2026-10-20).
	UHudBossBarWidget* Bar = Boss ? Boss->GetBar() : nullptr;
	if (!Bar)
	{
		return;
	}
	const AGravemotherStone* Slab = Stone.Get();
	Bar->SetArmour(Slab && Slab->IsOn() && !IsDead(), Slab ? Slab->GetHealthShare() : 0.f);
}

// ---------------------------------------------------------------------------
// Her boss's events
// ---------------------------------------------------------------------------

void AGravemotherCreature::HandleFightStarted()
{
	// Her bar sweeps in as she rears and screams (her next attack).
	bRoarWanted = true;
	// Her headstone lights up as the target from the start: nothing hurts her until it's off (the user, 2026-10-27).
	if (AGravemotherStone* Slab = Stone.Get(); Slab && GravemotherTuning::StoneGuards())
	{
		Slab->Mark(GravemotherTuning::MarkAtFightSeconds());
	}
	// Her first fight in this session: the burrow and burst before it (Scenes/BossIntro; once per session, never a rematch).
	BossIntro::PlayIfDue(*this);
}

void AGravemotherCreature::HandleFightReset()
{
	// The player fell: she's home and healed (her boss's reset; her brood went with it, her stone is whole: OnRespawned).
	bRoarWanted = false;
	bReeling = false;
	bStunned = false;
	ApplyPace(0);
}

void AGravemotherCreature::HandlePhaseChanged(int32 NewPhase, int32 OldPhase)
{
	ApplyPace(NewPhase);
	if (OldPhase != INDEX_NONE && NewPhase > OldPhase)
	{
		// The phase's tell: she screams and slams the floor (her brood's mounds); the quake comes after it.
		bRoarWanted = true;
		QuakeCooldownLeft = FMath::Max(QuakeCooldownLeft, QuakeAfterPhase);
	}
}

void AGravemotherCreature::HandleStaggered(bool bStaggered)
{
	if (bStaggered)
	{
		BeginReel();
	}
	else
	{
		EndReel();
	}
}

void AGravemotherCreature::ShakeFrom(const FVector& Where, float Strength, float Seconds) const
{
	BossCameraShake::Kick(this, Where, Strength, Seconds);
}

void AGravemotherCreature::KickDirtRing(const FVector& Center, float Radius, int32 Count, float Strength)
{
	KickDirt(Center, Strength);
	const float Start = FMath::FRandRange(0.f, 360.f);
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const FVector Out = FVector::ForwardVector.RotateAngleAxis(Start + 360.f * static_cast<float>(Index) / static_cast<float>(FMath::Max(Count, 1)),
			FVector::UpVector);
		KickDirt(Center + Out * Radius, Strength * 0.7f);
	}
}

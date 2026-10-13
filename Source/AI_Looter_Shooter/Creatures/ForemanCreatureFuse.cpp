// AForemanCreature's fuse sticks ("Fire in the hole", all fight long at a player who keeps off): the stick lit in his raised
// left fist for its tell (sparks, the hiss, his coal open: AForemanFuseStick), then lobbed in a high arc at where the player
// stands, leading them a little; it lands, burns a moment over a ring of cracks, and bursts. Staggered or killed with it lit,
// it falls at his own feet; his fight starting over puts it out.

#include "Creatures/ForemanCreature.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Bosses/BossComponent.h"
#include "Creatures/ForemanFuseStick.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"

namespace
{
	/** He leads a moving player by this share of a second of their run. */
	constexpr float LeadSeconds = 0.25f;

	/** A longer lob flies a little longer (against FuseFlightSeconds at 10 m), within these shares. */
	constexpr float FlightPerMeter = 0.1f;
	constexpr float ShortestFlight = 0.8f;
	constexpr float LongestFlight = 1.3f;

	/** "Fire in the hole!" with his first stick, then now and then, never twice within this long (s). */
	constexpr float FuseLineRest = 14.f;
	constexpr float FuseLineChance = 0.5f;
}

bool AForemanCreature::WantsFuse() const
{
	const APawn* Victim = GetTarget();
	if (!Victim || IsDead() || !Boss || !Boss->IsFighting() || FuseCooldownLeft > 0.f || HeldStick.IsValid() || bStaggeredNow)
	{
		return false;
	}
	const float Scale = GetSizeScale();
	const float Distance = static_cast<float>(FVector::Dist2D(GetActorLocation(), Victim->GetActorLocation()));
	return Distance >= FuseMinDistance * Scale && Distance <= FuseMaxDistance * Scale;
}

bool AForemanCreature::IsLightingFuse() const
{
	return Move == EForemanMove::Fuse && HeldStick.IsValid() && GetCreatureState() == ECreatureState::Attack && GetStateTime() < AttackWindup;
}

void AForemanCreature::BeginFuse()
{
	// The stick out of his satchel and lit in his raised fist: the tell, its sparks lighting his face and his open coal.
	Move = EForemanMove::Fuse;
	AttackWindup = FuseLightSeconds;
	AttackRecovery = FuseRecovery;
	if (AForemanFuseStick* Stick = AForemanFuseStick::SpawnLit(GetWorld(), this, GetMesh(), Rig.LeftArm.Hand, FVector::ZeroVector))
	{
		Stick->Damage = AttackDamage * FuseStrength;
		Stick->Radius = FuseRadius;
		Stick->GroundSeconds = FuseGroundSeconds;
		HeldStick = Stick;
	}
	LooterSound::PlayAttached(LooterSoundCue::Foreman::CoalOpen, GetRootComponent(), NAME_None, 0.8f);
	UE_LOG(LogLooter, Verbose, TEXT("%s lights a fuse."), *GetName());
}

void AForemanCreature::LobFuse()
{
	AForemanFuseStick* Stick = HeldStick.Get();
	HeldStick.Reset();
	FuseCooldownLeft = Pace.FuseCooldown * FMath::FRandRange(0.9f, 1.1f);
	if (!Stick)
	{
		return;
	}
	// At the player's feet, leading their run a little; on the floor there (or level with them over nothing).
	const APawn* Victim = GetTarget();
	FVector Aim = GetActorLocation() + GetActorForwardVector() * 800.f;
	if (Victim)
	{
		const ACharacter* Body = Cast<ACharacter>(Victim);
		const float Half = Body ? Body->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 88.f;
		FVector Run = Victim->GetVelocity();
		Run.Z = 0.0;
		Aim = Victim->GetActorLocation() + Run * LeadSeconds - FVector(0.0, 0.0, Half);
	}
	Aim = GroundUnder(Aim);
	// Let go over his shoulder as his arm comes through (where the throw's pose has his fist), well clear of the floor.
	const float Scale = GetSizeScale();
	const FVector Release = GetActorLocation() + GetActorForwardVector() * (40.f * Scale) + FVector(0.0, 0.0, 70.f * Scale);
	const float Meters = static_cast<float>(FVector::Dist2D(Release, Aim)) / 100.f;
	const float Flight = FuseFlightSeconds * FMath::Clamp(Meters * FlightPerMeter, ShortestFlight, LongestFlight);
	Stick->Lob(Release, Aim, Flight);
	++FusesThrown;
	if (FusesThrown == 1 || (SinceFuseLine > FuseLineRest && FMath::FRand() < FuseLineChance))
	{
		Say(ForemanFight::FuseLine());
		SinceFuseLine = 0.f;
	}
	UE_LOG(LogLooter, Verbose, TEXT("%s lobs a fuse stick %.1f m (%.2f s)."), *GetName(), Meters, Flight);
}

void AForemanCreature::DropHeldStick(bool bPutOut)
{
	AForemanFuseStick* Stick = HeldStick.Get();
	HeldStick.Reset();
	if (!Stick)
	{
		return;
	}
	if (bPutOut)
	{
		Stick->Defuse();
		return;
	}
	// Lit at his own feet: it bursts there, on him and his shift too, credited to whoever knocked it from his fist.
	const APawn* Victim = GetTarget();
	Stick->Drop(Victim ? Victim->GetController() : nullptr);
	FuseCooldownLeft = FMath::Max(FuseCooldownLeft, Pace.FuseCooldown);
	UE_LOG(LogLooter, Log, TEXT("%s drops his lit fuse stick at his feet."), *GetName());
}

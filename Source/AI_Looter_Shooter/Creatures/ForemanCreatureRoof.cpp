// AForemanCreature's roof strike (below 66%, every 18 s or so): he turns to the nearest timber prop in his reach, heaves the
// pick handle up (the tell) and hammers it into the prop (it cracks, shedding dust), or into the floor with none near; the
// roof answers. Dust sifts on four marked spots for 2 s (AForemanRockFall: the prop he struck, the player's spot, the prop
// the player is hiding behind, one more near them), then rock falls there, hurting whoever is still under it and splintering
// the props it lands on, so the cover changes. His coal is open for 3 s from the blow.

#include "Creatures/ForemanCreature.h"
#include "AI_Looter_Shooter.h"
#include "Audio/CreatureVoiceComponent.h"
#include "Audio/LooterSound.h"
#include "Bosses/BossCameraShake.h"
#include "Bosses/BossComponent.h"
#include "Creatures/ForemanRockFall.h"
#include "Creatures/ForemanTimberProp.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Math/RandomStream.h"

namespace
{
	/** He turns to the prop he means to strike this fast (degrees a second). */
	constexpr float PropTurnRate = 300.f;

	/** A spot's floor is his own level: no more than this above or below his feet (cm). */
	constexpr float SpotMaxRise = 300.f;

	/** The blow on a prop lands this high on it (cm): chest height. */
	constexpr float PropBlowHeight = 120.f;
}

bool AForemanCreature::WantsRoof() const
{
	return Boss && Boss->IsFighting() && Pace.bRoof && RoofCooldownLeft <= 0.f && GetTarget() && !IsDead() && !bStaggeredNow;
}

TArray<AForemanTimberProp*> AForemanCreature::GetProps(bool bStandingOnly) const
{
	const FVector Center = Boss && Boss->IsFighting() ? Boss->GetSpot() : GetHome().GetLocation();
	return AForemanTimberProp::FindNear(GetWorld(), Center, ArenaRadius, bStandingOnly);
}

TArray<AForemanRockFall*> AForemanCreature::GetRockFalls() const
{
	TArray<AForemanRockFall*> Falls;
	for (const TWeakObjectPtr<AForemanRockFall>& Each : RockFalls)
	{
		if (AForemanRockFall* Fall = Each.Get())
		{
			Falls.Add(Fall);
		}
	}
	return Falls;
}

void AForemanCreature::BeginRoof()
{
	// The nearest standing prop he can reach, or the floor before him with none near.
	Move = EForemanMove::Roof;
	AttackWindup = RoofWindup;
	AttackRecovery = RoofRecovery;
	const TArray<AForemanTimberProp*> Near = AForemanTimberProp::FindNear(GetWorld(), GetActorLocation(), RoofPropReach * GetSizeScale(), true);
	StruckProp = Near.Num() > 0 ? Near[0] : nullptr;
	LooterSound::PlayAttached(LooterSoundCue::DeadMiner::Heave, GetRootComponent(), NAME_None, 1.2f, 0.8f * Voice->GetPitch());
	UE_LOG(LogLooter, Verbose, TEXT("%s heaves up his pick handle at %s."), *GetName(), StruckProp.IsValid() ? *StruckProp->GetName() : TEXT("the floor"));
}

void AForemanCreature::FaceStruckProp(float DeltaSeconds)
{
	const AForemanTimberProp* Prop = StruckProp.Get();
	if (!Prop)
	{
		return;
	}
	const FVector To = Prop->GetFoot() - GetActorLocation();
	if (To.SizeSquared2D() < 1.0)
	{
		return;
	}
	const FRotator Wanted(0.f, static_cast<float>(To.Rotation().Yaw), 0.f);
	SetActorRotation(FMath::RInterpConstantTo(GetActorRotation(), Wanted, DeltaSeconds, PropTurnRate));
}

void AForemanCreature::RoofStrike()
{
	UWorld* World = GetWorld();
	++RoofStrikes;
	RoofCooldownLeft = Pace.RoofEvery * FMath::FRandRange(0.85f, 1.15f);
	// His coal is open from the blow (ForemanFight::IsCoalOpen): he's spent, his chest heaving.
	SinceRoofStrike = 0.f;
	const float Scale = GetSizeScale();
	const FVector Feet = GetFeet();

	// The blow: into the prop (it cracks and sheds dust) or the floor; the roof groans over the whole stope.
	AForemanTimberProp* Struck = StruckProp.Get();
	if (Struck && Struck->IsStanding())
	{
		Struck->Crack();
		LooterSound::PlayAt(this, LooterSoundCue::Foreman::RoofStrike, Struck->GetFoot() + FVector(0.0, 0.0, PropBlowHeight));
	}
	else
	{
		Struck = nullptr;
		const FVector Floor = GroundUnder(GetActorLocation() + GetActorForwardVector() * (110.f * Scale));
		KickDirt(Floor, 0.9f);
		LooterSound::PlayAt(this, LooterSoundCue::Foreman::Slam, Floor);
	}
	LooterSound::PlayAt(this, LooterSoundCue::Foreman::RoofGroan, Feet + FVector(0.0, 0.0, 500.0));
	LooterSound::PlayAttached(LooterSoundCue::Foreman::CoalOpen, GetRootComponent(), NAME_None, 0.8f);
	BossCameraShake::Kick(this, Feet, 0.45f, 0.7f);
	ShakeRoofDust(3);

	// The marked spots: on the prop he struck, the player's spot, the prop they hide behind, and round them.
	const APawn* Victim = GetTarget();
	FVector PlayerFeet = Feet + GetActorForwardVector() * 500.f;
	if (Victim)
	{
		const ACharacter* Body = Cast<ACharacter>(Victim);
		PlayerFeet = Victim->GetActorLocation() - FVector(0.0, 0.0, Body ? Body->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 88.0);
	}
	TOptional<FVector> StruckFoot;
	if (Struck)
	{
		StruckFoot = Struck->GetFoot();
	}
	TOptional<FVector> CoverFoot;
	for (AForemanTimberProp* Prop : AForemanTimberProp::FindNear(World, PlayerFeet, CoverReach, true))
	{
		if (Prop != Struck)
		{
			CoverFoot = Prop->GetFoot();
			break;
		}
	}
	FRandomStream Random(FMath::Rand());
	const FVector Arena = Boss && Boss->IsFighting() ? Boss->GetSpot() : GetHome().GetLocation();
	const TArray<FVector> Spots = ForemanFight::RoofSpots(PlayerFeet, StruckFoot, CoverFoot, Arena, ArenaRadius, RoofSpots, RoofSpotSpacing,
		RoofFillDistance, Random);
	for (const FVector& Spot : Spots)
	{
		// On his own level of the stope's floor; a spot over nothing at all (a test level) lies level with the player.
		FVector Floor = Spot;
		FVector Ground;
		if (FindGround(Spot, SpotMaxRise, SpotMaxRise * 2.f, Ground))
		{
			if (FMath::Abs(Ground.Z - Feet.Z) > SpotMaxRise)
			{
				continue;
			}
			Floor = Ground;
		}
		const TArray<AForemanTimberProp*> Under = AForemanTimberProp::FindNear(World, Floor, RoofRockRadius + 60.f, true);
		if (AForemanRockFall* Fall = AForemanRockFall::Spawn(World, Floor, RoofRockRadius, AttackDamage * RoofStrength, this, RoofDustSeconds, Under))
		{
			RockFalls.Add(Fall);
		}
	}
	UE_LOG(LogLooter, Log, TEXT("%s strikes the roof (%d): rock coming down on %d spots."), *GetName(), RoofStrikes, Spots.Num());
}

#include "Embers/Powers/EmberLuckyStreak.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Core/LooterCharacter.h"
#include "Combat/BulletSubsystem.h"
#include "Creatures/CreatureBase.h"
#include "Embers/EmberDefinition.h"
#include "Player/ViewKick.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponFX.h"
#include "CoreGlobals.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"

namespace
{
	const FName CritEveryName(TEXT("CritEvery"));

	/** The gold of a lucky round and a ricochet (warmer than the power's colour, so it reads against the tracers). */
	const FLinearColor LuckyGold(1.f, 0.72f, 0.18f);
	/** A ricochet's streak: how long, how wide, how bright. */
	constexpr float RicochetLength = 140.f;
	constexpr float RicochetWidth = 2.6f;
	constexpr float RicochetGlow = 26.f;
	/** The cards fanned out on use. */
	constexpr int32 StartCards = 14;
}

// ---------------------------------------------------------------------------
// The streak
// ---------------------------------------------------------------------------

void UEmberLuckyStreak::Activate(const FEmberContext& Context)
{
	if (!BeginUse(Context))
	{
		return;
	}
	const UEmberDefinition* Def = Context.Definition;
	Damage = ScaledDamage(Context, DefaultDamage);
	Range = Def && Def->Range > 0.f ? Def->Range : DefaultRange;
	CritEvery = FMath::Max(FMath::RoundToInt(Def ? Def->Extra(CritEveryName, DefaultCritEvery) : DefaultCritEvery), 1);
	Pulls = 0;
	bLuckyPull = false;
	LastPullFrame = TNumericLimits<uint64>::Max();
	LastRicochetFrame = TNumericLimits<uint64>::Max();
	LastPullWeapon.Reset();
	bStreaking = true;
	Bind(true);

	LooterSound::Play2D(Body.Get(), LooterSoundCue::Ember::LuckyStreakStart);
	ShowStart();
	StartLinger();
	UE_LOG(LogLooter, Log, TEXT("Lucky Streak: every %d pull(s) lucky, ricochets of %.1f within %.0f cm"), CritEvery, Damage, Range);
}

void UEmberLuckyStreak::Tick(const FEmberContext& Context, float DeltaSeconds)
{
	// The flight runs on its own linger (the player's clock), so ricochets keep flying past the end.
}

void UEmberLuckyStreak::End(const FEmberContext& Context, bool bInterrupted)
{
	if (!bStreaking)
	{
		return;
	}
	bStreaking = false;
	Bind(false);
	if (bInterrupted)
	{
		// Death, a scene or the level ending: nothing lands after it.
		Ricochets.Reset();
	}
	UE_LOG(LogLooter, Log, TEXT("Lucky Streak: %s after %d pull(s)"), bInterrupted ? TEXT("cut short") : TEXT("over"), Pulls);
}

void UEmberLuckyStreak::Bind(bool bOn)
{
	if (UBulletSubsystem* Old = Bullets.Get())
	{
		Old->OnBulletFiring.Remove(FiringHandle);
		Old->OnBulletLanding.Remove(LandingHandle);
	}
	FiringHandle.Reset();
	LandingHandle.Reset();
	Bullets.Reset();
	UWorld* World = GetBodyWorld();
	UBulletSubsystem* Subsystem = World ? World->GetSubsystem<UBulletSubsystem>() : nullptr;
	if (bOn && Subsystem)
	{
		Bullets = Subsystem;
		FiringHandle = Subsystem->OnBulletFiring.AddUObject(this, &UEmberLuckyStreak::NoteFiring);
		LandingHandle = Subsystem->OnBulletLanding.AddUObject(this, &UEmberLuckyStreak::NoteLanding);
	}
}

// ---------------------------------------------------------------------------
// The shots
// ---------------------------------------------------------------------------

void UEmberLuckyStreak::NoteFiring(FBulletShot& Shot)
{
	if (!bStreaking || !Body.IsValid() || Shot.Shooter.Get() != Body.Get())
	{
		return;
	}
	// One pull's pellets share a frame and a gun: one round, lucky or not.
	if (GFrameCounter != LastPullFrame || Shot.Weapon.Get() != LastPullWeapon.Get())
	{
		LastPullFrame = GFrameCounter;
		LastPullWeapon = Shot.Weapon.Get();
		++Pulls;
		bLuckyPull = Pulls % CritEvery == 0;
		if (bLuckyPull)
		{
			LooterSound::Play2D(Body.Get(), LooterSoundCue::Ember::LuckyStreakLucky, 0.8f);
			// A glint of gold at the muzzle as the lucky round leaves it.
			for (int32 Index = 0; Index < 6; ++Index)
			{
				const FVector Out = (Shot.Direction + Random.GetUnitVector() * 0.7).GetSafeNormal();
				Sparks.Spawn(Shot.VisualStart, Out * Random.FRandRange(150.f, 380.f), Random.FRandRange(0.15f, 0.3f), 1.2f, LuckyGold, 20.f, 300.f, 4.f);
			}
		}
	}
	if (bLuckyPull)
	{
		Shot.bSureCritical = true;
		Shot.TracerColor = LuckyGold;
		Shot.TracerWidth *= 1.45f;
	}
}

void UEmberLuckyStreak::NoteLanding(const FBulletShot& Shot, const FHitResult& Hit, FBulletLanding& Landing)
{
	if (!bStreaking || !Body.IsValid() || Shot.Shooter.Get() != Body.Get() || Shot.bBlank)
	{
		return;
	}
	ACreatureBase* Struck = Cast<ACreatureBase>(Hit.GetActor());
	if (!Struck || Struck->IsDead() || GFrameCounter == LastRicochetFrame)
	{
		return;
	}
	LastRicochetFrame = GFrameCounter;
	SendRicochet(*Struck, Hit.ImpactPoint);
}

void UEmberLuckyStreak::SendRicochet(ACreatureBase& From, const FVector& At)
{
	UWorld* World = From.GetWorld();
	if (!World)
	{
		return;
	}
	// The nearest other living creature in reach of the hit, in sight of it.
	ACreatureBase* Best = nullptr;
	float BestDistance = Range;
	for (TActorIterator<ACreatureBase> It(World); It; ++It)
	{
		ACreatureBase* Other = *It;
		if (!Other || Other == &From || Other->IsDead() || Other->IsHidden())
		{
			continue;
		}
		const float Distance = static_cast<float>(FVector::Dist(At, Other->GetActorLocation()));
		if (Distance >= BestDistance || !CanSee(*World, At, Other->GetActorLocation(), &From))
		{
			continue;
		}
		Best = Other;
		BestDistance = Distance;
	}
	// A card flicked off the hit either way: luck showing.
	FlickCard(At, (At - From.GetActorLocation()).GetSafeNormal() + FVector(0.0, 0.0, 0.8), 380.f);
	if (!Best)
	{
		return;
	}
	FRicochet& Ricochet = Ricochets.AddDefaulted_GetRef();
	Ricochet.Target = Best;
	Ricochet.From = At;
	Ricochet.Flight = FMath::Max(BestDistance / RicochetSpeed, 0.03f);
	LastTarget = Best;
	LooterSound::PlayAt(&From, LooterSoundCue::Ember::LuckyStreakRicochet, At, 0.9f, Random.FRandRange(0.94f, 1.08f));
	StartLinger();
}

void UEmberLuckyStreak::LandRicochet(ACreatureBase& Target, const FVector& At, const FVector& Along)
{
	Hurt(Target, Damage * Random.FRandRange(0.9f, 1.1f), At, Along);
	FlickCard(At, -Along + FVector(0.0, 0.0, 0.9), 300.f);
	for (int32 Index = 0; Index < 10; ++Index)
	{
		const FVector Out = (Along * 0.4 + Random.GetUnitVector()).GetSafeNormal();
		Sparks.Spawn(At, Out * Random.FRandRange(200.f, 520.f), Random.FRandRange(0.2f, 0.4f), 1.4f, LuckyGold, 22.f, 700.f, 2.5f);
	}
	if (FWeaponFX* FX = Effects())
	{
		FX->SpawnRicochet(At, Along, 0.8f);
	}
}

// ---------------------------------------------------------------------------
// What flies
// ---------------------------------------------------------------------------

void UEmberLuckyStreak::StepFlight(float DeltaSeconds)
{
	const float Dt = FMath::Max(DeltaSeconds, 0.f);
	FWeaponFX* FX = Effects();
	for (int32 Index = Ricochets.Num() - 1; Index >= 0; --Index)
	{
		FRicochet& Ricochet = Ricochets[Index];
		ACreatureBase* Target = Ricochet.Target.Get();
		if (!Target || Target->IsDead())
		{
			Ricochets.RemoveAtSwap(Index);
			continue;
		}
		Ricochet.Age += Dt;
		// It homes on the body's middle as it moves (a lucky bounce never misses).
		const FVector To = Target->GetActorLocation();
		const float U = FMath::Clamp(Ricochet.Age / Ricochet.Flight, 0.f, 1.f);
		const FVector Head = FMath::Lerp(Ricochet.From, To, U);
		const FVector Along = (To - Ricochet.From).GetSafeNormal();
		if (FX)
		{
			const FVector Tail = Head - Along * FMath::Min(RicochetLength, static_cast<float>(FVector::Dist(Head, Ricochet.From)));
			FX->AddStreak(Tail, Head, RicochetWidth, LuckyGold, RicochetGlow);
		}
		if (U >= 1.f)
		{
			Ricochets.RemoveAtSwap(Index);
			LandRicochet(*Target, To, Along);
		}
	}
	for (int32 Index = Cards.Num() - 1; Index >= 0; --Index)
	{
		FCard& Card = Cards[Index];
		Card.Age += Dt;
		if (Card.Age >= Card.Life)
		{
			Cards.RemoveAtSwap(Index);
			continue;
		}
		// Paper on the air: it slows fast and drifts down, spinning.
		Card.Velocity *= FMath::Max(1.f - 3.f * Dt, 0.f);
		Card.Velocity.Z -= 260.f * Dt;
		Card.Location += Card.Velocity * Dt;
		Card.Spin += Card.SpinRate * Dt;
		if (FX)
		{
			const float Fade = 1.f - Card.Age / Card.Life;
			EmberFX::DrawCard(*FX, Card.Location, Card.Velocity.IsNearlyZero() ? FVector::UpVector : Card.Velocity, Card.Spin, Color, 14.f * Fade);
		}
	}
	Sparks.Tick(Dt, FX);
}

bool UEmberLuckyStreak::Linger(float DeltaSeconds)
{
	StepFlight(DeltaSeconds);
	return bStreaking || !Ricochets.IsEmpty() || !Cards.IsEmpty() || !Sparks.IsEmpty();
}

void UEmberLuckyStreak::FlickCard(const FVector& At, const FVector& Away, float Speed)
{
	FCard& Card = Cards.AddDefaulted_GetRef();
	Card.Location = At;
	Card.Velocity = Away.GetSafeNormal() * Speed + Random.GetUnitVector() * 80.f;
	Card.Spin = Random.FRandRange(0.f, UE_TWO_PI);
	Card.SpinRate = Random.FRandRange(14.f, 24.f) * (Random.FRand() < 0.5f ? -1.f : 1.f);
	Card.Life = Random.FRandRange(0.45f, 0.65f);
}

void UEmberLuckyStreak::ShowStart()
{
	ACharacter* Character = Body.Get();
	if (!Character)
	{
		return;
	}
	// A fan of cards bursting from the hand, up and out in front, slowing and drifting down.
	FVector Eye;
	FVector Aim;
	GetAim(*Character, Eye, Aim);
	const FVector Hand = HandPoint(*Character, 14.f, 10.f, 55.f);
	const FVector Side = FVector::CrossProduct(FVector::UpVector, Aim).GetSafeNormal();
	for (int32 Index = 0; Index < StartCards; ++Index)
	{
		const float Spread = (Index / static_cast<float>(StartCards - 1) - 0.5f) * 1.6f;
		FCard& Card = Cards.AddDefaulted_GetRef();
		Card.Location = Hand;
		Card.Velocity = (Aim + Side * Spread + FVector(0.0, 0.0, 0.55 + 0.25 * Random.FRand())).GetSafeNormal() * Random.FRandRange(260.f, 420.f);
		Card.Spin = Random.FRandRange(0.f, UE_TWO_PI);
		Card.SpinRate = Random.FRandRange(10.f, 20.f) * (Index % 2 ? 1.f : -1.f);
		Card.Life = Random.FRandRange(0.55f, 0.8f);
	}
	for (int32 Index = 0; Index < 16; ++Index)
	{
		const FVector Out = (Aim * 0.6 + Random.GetUnitVector()).GetSafeNormal();
		Sparks.Spawn(Hand, Out * Random.FRandRange(120.f, 320.f), Random.FRandRange(0.3f, 0.6f), 1.3f, LuckyGold, 18.f, 120.f, 2.f);
	}
	FViewKick Deal;
	Deal.Pitch = 0.35f;
	Deal.Roll = -0.6f;
	Deal.Frequency = 6.f;
	Deal.Damping = 0.65f;
	Kick(Deal);
}

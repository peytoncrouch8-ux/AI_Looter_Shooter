// ASinkIntake: what it does to those near it: the draw on the player (the pull, the frost, the drain, the smog sucked in),
// the swinging hose's knocks, and the smog pool draining with the growl after it. Its story, the charges and the choke are
// SinkIntake.cpp's; the hoses SinkIntakeHose.cpp's.

#include "World/SinkIntake.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Bosses/BossCameraShake.h"
#include "Combat/HealthComponent.h"
#include "Player/PlayerRattleSubsystem.h"
#include "World/LamplighterEffects.h"
#include "Components/AudioComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	/** The draw's frost renews this often (s); its drain ticks this often (s); smog is sucked in this often (s). */
	constexpr float FrostEvery = 0.9f;
	constexpr float DrainEvery = 0.35f;
	constexpr float SmogEvery = 0.18f;
	/** A player more than this above or below the mouth isn't in its draw (cm): the ramp over it. */
	constexpr double DrawHeight = 450.0;
	/** The knock: thrown back this hard (cm/s) and up. */
	constexpr float KnockAway = 900.f;
	constexpr float KnockUp = 420.f;
	/** The pool drains this long after the choke, and the growl comes this long after it's gone (s). */
	constexpr float PoolWait = 2.f;
	constexpr float GrowlWait = 1.f;
}

APawn* ASinkIntake::GetPlayer() const
{
	const UWorld* World = GetWorld();
	const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
	return Controller ? Controller->GetPawn() : nullptr;
}

void ASinkIntake::TickDraw(float DeltaSeconds)
{
	const FVector Mouth = GetActorLocation();
	// Smog sucked in toward the mouth round its edge: the draw seen before it's felt.
	SmogClock += DeltaSeconds;
	if (SmogClock >= SmogEvery)
	{
		SmogClock = 0.f;
		const float Angle = FMath::FRandRange(0.f, 2.f * static_cast<float>(UE_PI));
		const FVector Edge = Mouth + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.0) * (DrawRadius * 0.7) + FVector(0.0, 0.0, 60.0);
		LamplighterEffects::SmokePuff(this, Edge, (Mouth - Edge).GetSafeNormal() * (State == EIntakeState::Fused ? 520.0 : 300.0));
	}
	APawn* Player = GetPlayer();
	if (!Player)
	{
		return;
	}
	const FVector At = Player->GetActorLocation();
	const float Distance = static_cast<float>(FVector::Dist2D(At, Mouth));
	const float Pull = FMath::Abs(At.Z - Mouth.Z) <= DrawHeight ? GetDrawOn(Distance) : 0.f;
	if (Pull <= 0.f)
	{
		LooterSound::Stop(DrawSound.Get(), 0.4f);
		DrawSound.Reset();
		return;
	}
	if (!DrawSound.IsValid())
	{
		DrawSound = LooterSound::Start(this, LooterSoundCue::DredgerHoseDrain, Root, NAME_None, 1.f, 0.55f);
	}
	// The pull: a share of their own walk toward the mouth (they lean out of it slower; standing still, they drift in).
	Player->AddMovementInput((Mouth - At).GetSafeNormal2D(), Pull);
	FrostClock += DeltaSeconds;
	if (FrostClock >= FrostEvery)
	{
		FrostClock = 0.f;
		if (UPlayerRattleSubsystem* Rattles = UPlayerRattleSubsystem::Get(this))
		{
			FPlayerRattle How;
			How.Strength = FMath::Clamp(Pull / SinkIntakeRules::MouthDraw, 0.3f, 1.f);
			How.bSway = false;
			How.bSlow = false;
			How.Shake = 0.06f;
			How.ShakeSeconds = 0.4f;
			How.FrostSeconds = 1.2f;
			Rattles->Rattle(Player, Mouth, How);
		}
	}
	// Inside the drain: it drinks them too.
	if (Distance <= DrainRadius)
	{
		DrainClock += DeltaSeconds;
		if (DrainClock >= DrainEvery)
		{
			const UHealthComponent* Life = Player->FindComponentByClass<UHealthComponent>();
			const float Amount = (Life ? Life->GetMaxHealth() : 100.f) * DrainShare * DrainClock;
			DrainClock = 0.f;
			UGameplayStatics::ApplyDamage(Player, Amount, nullptr, this, UDamageType::StaticClass());
		}
	}
	else
	{
		DrainClock = 0.f;
	}
}

void ASinkIntake::TickKnocks()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	// The hose's lower length as it swings: whatever stands near it is thrown down.
	const FIntakeHoseShape Shape = GetHoseShape();
	TArray<FVector> Points;
	for (int32 Index = 0; Index <= 6; ++Index)
	{
		Points.Add(Shape.At(0.55f + 0.45f * Index / 6.f));
	}
	const double Reach = FMath::Square(static_cast<double>(KnockRadius));
	for (TActorIterator<APawn> It(World); It; ++It)
	{
		APawn* Pawn = *It;
		if (!Pawn || Knocked.Contains(Pawn))
		{
			continue;
		}
		const FVector At = Pawn->GetActorLocation();
		const FVector* Hit = Points.FindByPredicate([&At, Reach](const FVector& Point) { return FVector::DistSquared(Point, At) <= Reach; });
		if (!Hit)
		{
			continue;
		}
		UHealthComponent* Life = Pawn->FindComponentByClass<UHealthComponent>();
		if (Life && Life->IsDead())
		{
			continue;
		}
		Knocked.Add(Pawn);
		const bool bPlayer = Pawn->IsPlayerControlled();
		const float Share = bPlayer ? KnockPlayerShare : KnockCreatureShare;
		if (Life)
		{
			UGameplayStatics::ApplyDamage(Pawn, Life->GetMaxHealth() * Share, nullptr, this, UDamageType::StaticClass());
		}
		const FVector Away = (At - *Hit).GetSafeNormal2D();
		if (ACharacter* Body = Cast<ACharacter>(Pawn))
		{
			Body->LaunchCharacter(Away * KnockAway + FVector(0.0, 0.0, KnockUp), true, true);
		}
		LooterSound::PlayAt(this, LooterSoundCue::ImpactMetal, At, 1.f, 0.6f);
		if (bPlayer)
		{
			LamplighterEffects::Shake(this, 0.6f, 0.6f);
		}
		UE_LOG(LogLooter, Log, TEXT("%s: the swinging hose knocks %s down."), *GetActorNameOrLabel(), *Pawn->GetName());
	}
}

void ASinkIntake::TickPool(float DeltaSeconds)
{
	SinceChoke += DeltaSeconds;
	if (PoolAt.IsNearlyZero())
	{
		bGrowled = true;
		return;
	}
	if (SinceChoke >= PoolWait && PoolLeft > 0.f)
	{
		// It drains away into the floor as the intake stops drinking from it.
		PoolLeft = FMath::Max(PoolLeft - DeltaSeconds / FMath::Max(PoolDrainSeconds, 0.1f), 0.f);
		const float Size = PoolRadius / PoolMeshRadius * (0.35f + 0.65f * PoolLeft);
		Pool->SetWorldScale3D(FVector(Size, Size, 1.0));
		Pool->SetWorldLocation(PoolAt - FVector(0.0, 0.0, 4.0 * (1.0 - PoolLeft)));
		if (PoolLeft <= 0.f)
		{
			Pool->SetVisibility(false);
		}
	}
	if (PoolLeft <= 0.f && SinceChoke >= PoolWait + PoolDrainSeconds + GrowlWait)
	{
		// Something huge growls in the dark of the den: it was drinking that runoff (Side 3's hook).
		bGrowled = true;
		const FVector From = DenAt.IsNearlyZero() ? PoolAt : DenAt;
		LooterSound::PlayAt(this, LooterSoundCue::GravemotherRoar, From, 0.7f, 0.75f);
		BossCameraShake::Kick(this, From, 0.25f, 1.f, 3500.f);
		Say(GrowlLines);
	}
}

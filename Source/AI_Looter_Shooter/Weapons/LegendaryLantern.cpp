#include "Weapons/LegendaryLantern.h"
#include "Audio/LooterSound.h"
#include "Audio/LooterSoundCues.h"
#include "Combat/BulletSubsystem.h"
#include "Combat/HealthComponent.h"
#include "Weapons/LegendaryRules.h"
#include "Weapons/LegendarySubsystem.h"
#include "Weapons/LegendaryTuning.h"
#include "Weapons/WeaponFX.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"

namespace
{
	/** The flame's heart, and the warm light round it (Abel's lanterns' amber). */
	const FLinearColor FlameCore(1.f, 0.82f, 0.5f);
	const FLinearColor FlameWarm(1.f, 0.55f, 0.18f);
	/** Where the flame sits over the ground (cm): a lantern's height set down. */
	constexpr float FlameHeight = 30.f;
	/** Motes a second off the flame, and off the player it heals. */
	constexpr float FlameMoteRate = 5.f;
	constexpr float HealMoteRate = 4.f;
	/** How long past its burn it may linger if its tick never ends it (s). */
	constexpr float LifeSpanSlack = 1.f;
}

ALegendaryLantern::ALegendaryLantern()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetCanBeDamaged(false);
}

ALegendaryLantern* ALegendaryLantern::Light(UWorld* World, const FVector& Ground, APawn* InHealer, float Seconds, float InRadius)
{
	if (!World)
	{
		return nullptr;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ALegendaryLantern* Lantern = World->SpawnActor<ALegendaryLantern>(Ground, FRotator::ZeroRotator, Params);
	if (!Lantern)
	{
		return nullptr;
	}
	Lantern->Healer = InHealer;
	Lantern->Life = FMath::Max(Seconds, 0.1f);
	Lantern->Radius = FMath::Max(InRadius, 0.f);
	Lantern->SetLifeSpan(Lantern->Life + LifeSpanSlack);
	// Lit: a soft rising chime where it's set down.
	LooterSound::PlayAt(Lantern, LooterSoundCue::SoulMote::Drop, Ground, 0.8f, 0.9f);
	return Lantern;
}

void ALegendaryLantern::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Advance(DeltaSeconds);
}

void ALegendaryLantern::Advance(float DeltaSeconds)
{
	Age += FMath::Max(DeltaSeconds, 0.f);
	if (Age >= Life)
	{
		Destroy();
		return;
	}
	// The one who made the kill, while they stand in its light (alive), and no other lantern heals them this frame.
	APawn* Pawn = Healer.Get();
	UHealthComponent* Health = Pawn ? Pawn->FindComponentByClass<UHealthComponent>() : nullptr;
	const APawn* Healing = nullptr;
	if (Health && !Health->IsDead() && LegendaryRules::InLanternReach(GetActorLocation(), Pawn->GetActorLocation(), Radius))
	{
		ULegendarySubsystem* Legendaries = ULegendarySubsystem::Get(this);
		if (!Legendaries || Legendaries->ClaimHeal(Pawn, this))
		{
			Healed += Health->Heal(LegendaryRules::LanternHeal(Health->GetMaxHealth(), DeltaSeconds, FLegendaryTuning::Live()));
			Healing = Pawn;
		}
	}
	Draw(LegendaryRules::LanternGlow(Age, Life), Healing, DeltaSeconds);
}

void ALegendaryLantern::Draw(float Glow, const APawn* Healing, float DeltaSeconds)
{
	UWorld* World = GetWorld();
	UBulletSubsystem* Bullets = World ? World->GetSubsystem<UBulletSubsystem>() : nullptr;
	if (!Bullets || Glow <= 0.f)
	{
		return;
	}
	FWeaponFX& FX = Bullets->GetEffects();
	FX.Initialize(World);
	const FVector Ground = GetActorLocation();
	const FVector Flame = Ground + FVector(0.0, 0.0, FlameHeight);
	// A flame's flicker: two waves, never in step.
	const float Flicker = 0.85f + 0.1f * FMath::Sin(Age * 23.f) + 0.05f * FMath::Sin(Age * 7.3f + 1.1f);
	FX.AddGlow(Flame, 10.f * Glow * Flicker, FlameCore, 8.f * Glow);
	FX.AddGlow(Flame, 70.f * Glow, FlameWarm, 0.9f * Glow * Flicker);
	// Its reach on the ground: step inside the ring to be healed.
	FX.AddGroundRing(Ground + FVector(0.0, 0.0, 4.0), Radius, 4.f, FlameWarm, 0.8f * Glow, 48);
	MotesOwed += FlameMoteRate * DeltaSeconds;
	while (MotesOwed >= 1.f)
	{
		MotesOwed -= 1.f;
		FX.SpawnLootMotes(Flame, 25.f, FlameWarm, 1);
	}
	if (Healing)
	{
		// The light gathering on the one it heals.
		HealMotesOwed += HealMoteRate * DeltaSeconds;
		while (HealMotesOwed >= 1.f)
		{
			HealMotesOwed -= 1.f;
			FX.SpawnLootMotes(Healing->GetActorLocation() - FVector(0.0, 0.0, 50.0), 40.f, FlameWarm, 1);
		}
	}
}

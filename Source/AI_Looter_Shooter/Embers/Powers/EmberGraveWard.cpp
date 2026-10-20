#include "Embers/Powers/EmberGraveWard.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Core/LooterCharacter.h"
#include "Combat/HealthComponent.h"
#include "Creatures/CreatureBase.h"
#include "Embers/EmberDefinition.h"
#include "Embers/Powers/EmberPropActor.h"
#include "Player/ViewKick.h"
#include "Weapons/WeaponFX.h"
#include "Components/AudioComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"

namespace
{
	const FName HealName(TEXT("HealPerSecond"));

	/** The lantern's size against the mine's own, its light (candelas, and how far past the ring it reaches). */
	constexpr float LanternScale = 1.3f;
	constexpr float LanternCandela = 14.f;
	constexpr float LightReach = 1.35f;
	/** Its ring: points round it, laid on the ground, lifted a little. */
	constexpr int32 RingPoints = 48;
	/** How often motes rise in the light, off a slowed creature, and off the player while they mend (seconds). */
	constexpr float MoteEvery = 0.1f;
	constexpr float SlowedMoteEvery = 0.35f;
	constexpr float HealMoteEvery = 0.2f;

	bool IsClock(float Dilation, float Value)
	{
		return FMath::IsNearlyEqual(Dilation, Value, 0.001f);
	}
}

bool UEmberGraveWard::FindPlantSpot(const ACharacter& Character, FVector& OutGround)
{
	const UWorld* World = Character.GetWorld();
	if (!World)
	{
		return false;
	}
	// A step in front of him, on the ground there; else at his own feet.
	const FVector Feet = FeetOf(Character);
	const FVector Forward = FVector(Character.GetActorForwardVector().X, Character.GetActorForwardVector().Y, 0.0).GetSafeNormal();
	return FindGround(*World, Feet + Forward * 70.f, 80.f, 160.f, OutGround, nullptr, &Character)
		|| FindGround(*World, Feet, 40.f, 120.f, OutGround, nullptr, &Character);
}

EEmberDenied UEmberGraveWard::CanActivate(const FEmberContext& Context) const
{
	if (!Context.Character)
	{
		return EEmberDenied::Refused;
	}
	if (bBurning)
	{
		return EEmberDenied::Active;
	}
	// In the air with no ground under him: nowhere to plant it.
	FVector Ground;
	return FindPlantSpot(*Context.Character, Ground) ? EEmberDenied::None : EEmberDenied::Refused;
}

void UEmberGraveWard::Activate(const FEmberContext& Context)
{
	ACharacter* Character = Context.Character;
	UWorld* World = Character ? Character->GetWorld() : nullptr;
	if (!World || !BeginUse(Context) || !FindPlantSpot(*Character, Center))
	{
		return;
	}
	const UEmberDefinition* Def = Context.Definition;
	Radius = Def && Def->Radius > 0.f ? Def->Radius : DefaultRadius;
	Strength = Def && Def->Strength > 0.f ? FMath::Clamp(Def->Strength, 0.05f, 1.f) : DefaultStrength;
	HealPerSecond = FMath::Max(Def ? Def->Extra(HealName, DefaultHealPerSecond) : DefaultHealPerSecond, 0.f);
	Time = 0.f;
	Mended = 0.f;
	MoteTimer = HealMoteTimer = 0.f;
	Slowed.Reset();
	bBurning = true;

	// The lantern set down facing him, lit in the ward's colour; its ring laid on the ground once (it stays put).
	const float Yaw = (Character->GetActorLocation() - Center).GetSafeNormal2D().Rotation().Yaw;
	Lantern = AEmberPropActor::Place(World, LanternPath, Center, Yaw, FVector(LanternScale));
	if (AEmberPropActor* Prop = Lantern.Get())
	{
		const FVector Glass = Prop->GetTop() - Prop->GetActorLocation();
		Prop->SetLight(Color, LanternCandela, Radius * LightReach, Glass * 0.6f);
		Hum = LooterSound::Start(Prop, LooterSoundCue::Ember::GraveWardHum, Prop->GetRootComponent());
	}
	Ring = EmberFX::GroundRing(*World, Center, Radius, RingPoints, 200.f, 6.f);
	LooterSound::PlayAt(Character, LooterSoundCue::Ember::GraveWardPlant, Center);

	// Set down: dust round its foot, a breath of light rising, the view dipping as he kneels to it.
	if (FWeaponFX* FX = Effects())
	{
		const FLinearColor Dust = EmberFX::DustColor(Character);
		for (int32 Index = 0; Index < 8; ++Index)
		{
			const float Angle = 2.f * UE_PI * Index / 8.f;
			const FVector Out(FMath::Cos(Angle), FMath::Sin(Angle), 0.f);
			FX->SpawnDustPuff(Center + Out * 15.f + FVector(0.0, 0.0, 6.0), Out * Random.FRandRange(120.f, 200.f) + FVector(0.0, 0.0, 30.0), Dust, 0.35f, 12.f,
				Random.FRandRange(50.f, 70.f), Random.FRandRange(0.6f, 0.9f));
		}
		FX->SpawnDeathBurst(EDeathBurst::SoulLight, Center + FVector(0.0, 0.0, 40.0), FVector::UpVector, 70.f, Color);
	}
	FViewKick Kneel;
	Kneel.Pitch = -0.5f;
	Kneel.Frequency = 4.f;
	Kneel.Damping = 0.75f;
	Kick(Kneel);
	StartLinger();
	UE_LOG(LogLooter, Log, TEXT("Grave Ward: planted at %s, %.0f cm of light, creatures at %.0f%%, mending %.1f a second"), *Center.ToCompactString(),
		Radius, Strength * 100.f, HealPerSecond);
}

void UEmberGraveWard::Tick(const FEmberContext& Context, float DeltaSeconds)
{
	StepWard(DeltaSeconds);
}

void UEmberGraveWard::End(const FEmberContext& Context, bool bInterrupted)
{
	if (!bBurning)
	{
		return;
	}
	bBurning = false;
	ReleaseAll();
	const UWorld* World = GetBodyWorld();
	if (World && !World->bIsTearingDown)
	{
		ShowOut();
	}
	LooterSound::Stop(Hum.Get(), 0.3f);
	Hum.Reset();
	if (AEmberPropActor* Prop = Lantern.Get())
	{
		Prop->Remove();
	}
	Lantern.Reset();
	UE_LOG(LogLooter, Log, TEXT("Grave Ward: %s, %.0f mended"), bInterrupted ? TEXT("put out early") : TEXT("burned out"), Mended);
}

bool UEmberGraveWard::IsInLight(const AActor& Actor) const
{
	const FVector Where = Actor.GetActorLocation();
	return FVector::Dist2D(Where, Center) <= Radius && FMath::Abs(Where.Z - Center.Z) <= LightHeight;
}

// ---------------------------------------------------------------------------
// The ward, frame by frame
// ---------------------------------------------------------------------------

void UEmberGraveWard::StepWard(float DeltaSeconds)
{
	ACharacter* Character = Body.Get();
	UWorld* World = Character ? Character->GetWorld() : nullptr;
	if (!bBurning || !World)
	{
		return;
	}
	const float Dt = FMath::Max(DeltaSeconds, 0.f);
	Time += Dt;

	// Who's in the light: slowed (their clock held at the ward's, again each frame after a hit-stop gave it back).
	for (TActorIterator<ACreatureBase> It(World); It; ++It)
	{
		ACreatureBase* Creature = *It;
		if (!Creature || Creature->IsDead() || IsBoss(*Creature) || !IsInLight(*Creature))
		{
			continue;
		}
		Slow(*Creature);
	}
	for (int32 Index = Slowed.Num() - 1; Index >= 0; --Index)
	{
		ACreatureBase* Creature = Slowed[Index].Get();
		if (!Creature)
		{
			Slowed.RemoveAtSwap(Index);
			continue;
		}
		if (Creature->IsDead() || !IsInLight(*Creature))
		{
			Unslow(*Creature);
			Slowed.RemoveAtSwap(Index);
		}
	}

	// The player standing in it mends.
	UHealthComponent* Life = Character->FindComponentByClass<UHealthComponent>();
	const bool bMending = Life && !Life->IsDead() && IsInLight(*Character) && Life->GetHealth() < Life->GetMaxHealth();
	if (bMending)
	{
		Mended += Life->Heal(HealPerSecond * Dt);
	}

	// The light breathes; motes rise in it, off the slowed, off the player while they mend.
	if (AEmberPropActor* Prop = Lantern.Get())
	{
		Prop->SetLightIntensity(LanternCandela * (0.86f + 0.1f * FMath::Sin(Time * 9.f) + 0.04f * FMath::Sin(Time * 23.f)));
	}
	MoteTimer -= Dt;
	while (MoteTimer <= 0.f)
	{
		MoteTimer += MoteEvery;
		const float Angle = Random.FRandRange(0.f, UE_TWO_PI);
		const float Out = Radius * FMath::Sqrt(Random.FRand());
		const FVector Spot = Center + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * Out + FVector(0.0, 0.0, Random.FRandRange(5.f, 40.f));
		Motes.Spawn(Spot, FVector(0.0, 0.0, Random.FRandRange(25.f, 55.f)), Random.FRandRange(1.f, 1.6f), 1.5f, Color, 9.f, -15.f, 0.4f);
	}
	if (FMath::Fmod(Time, SlowedMoteEvery) < Dt)
	{
		for (const TWeakObjectPtr<ACreatureBase>& Each : Slowed)
		{
			if (const ACreatureBase* Creature = Each.Get())
			{
				Motes.Spawn(Creature->GetActorLocation() + Random.GetUnitVector() * 30.f, FVector(0.0, 0.0, 40.f), 0.9f, 1.8f, Color, 12.f, -10.f, 0.5f);
			}
		}
	}
	HealMoteTimer -= Dt;
	if (bMending && HealMoteTimer <= 0.f)
	{
		HealMoteTimer = HealMoteEvery;
		const FVector Feet = FeetOf(*Character);
		for (int32 Index = 0; Index < 2; ++Index)
		{
			const float Angle = Random.FRandRange(0.f, UE_TWO_PI);
			Motes.Spawn(Feet + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * Random.FRandRange(25.f, 45.f) + FVector(0.0, 0.0, 10.0),
				FVector(0.0, 0.0, Random.FRandRange(60.f, 90.f)), Random.FRandRange(0.7f, 1.f), 1.6f, Color, 14.f, -20.f, 0.4f);
		}
	}
}

void UEmberGraveWard::Slow(ACreatureBase& Creature)
{
	// Only a clock still its own is taken (a hit-stop's stop runs out, then the ward's comes back).
	if (IsClock(Creature.CustomTimeDilation, 1.f))
	{
		Creature.CustomTimeDilation = Strength;
	}
	if (!Slowed.Contains(&Creature))
	{
		Slowed.Add(&Creature);
	}
}

void UEmberGraveWard::Unslow(ACreatureBase& Creature)
{
	if (IsClock(Creature.CustomTimeDilation, Strength))
	{
		Creature.CustomTimeDilation = 1.f;
	}
}

void UEmberGraveWard::ReleaseAll()
{
	for (const TWeakObjectPtr<ACreatureBase>& Each : Slowed)
	{
		if (ACreatureBase* Creature = Each.Get())
		{
			Unslow(*Creature);
		}
	}
	Slowed.Reset();
}

bool UEmberGraveWard::Linger(float DeltaSeconds)
{
	FWeaponFX* FX = Effects();
	if (bBurning && FX && Ring.Num() > 2)
	{
		// Its ring on the ground: steady, breathing a little, a fainter one just inside it.
		const float Breath = 0.75f + 0.25f * FMath::Sin(Time * 3.f);
		EmberFX::DrawLine(*FX, Ring, true, 2.6f, Color, 7.f * Breath);
	}
	Motes.Tick(DeltaSeconds, FX);
	return bBurning || !Motes.IsEmpty();
}

void UEmberGraveWard::ShowOut()
{
	const AEmberPropActor* Prop = Lantern.Get();
	const FVector Glass = Prop ? Prop->GetTop() : Center + FVector(0.0, 0.0, 40.0);
	LooterSound::PlayAt(GetBodyWorld(), LooterSoundCue::Ember::GraveWardOut, Glass);
	if (FWeaponFX* FX = Effects())
	{
		// Snuffed: a curl of smoke off the glass and the last of its light going up.
		for (int32 Index = 0; Index < 4; ++Index)
		{
			FX->SpawnWisp(Glass + Random.GetUnitVector() * 4.f, FVector(0.0, 0.0, Random.FRandRange(30.f, 55.f)), 0.25f, 6.f, 26.f, Random.FRandRange(0.9f, 1.4f));
		}
	}
	for (int32 Index = 0; Index < 10; ++Index)
	{
		Motes.Spawn(Glass, Random.GetUnitVector() * 60.f + FVector(0.0, 0.0, 70.0), Random.FRandRange(0.5f, 0.9f), 1.5f, Color, 14.f, -30.f, 1.f);
	}
}

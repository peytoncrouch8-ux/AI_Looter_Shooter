#include "Embers/Powers/EmberDustDevil.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Combat/BulletSubsystem.h"
#include "Combat/HealthComponent.h"
#include "Core/LooterCharacter.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/CreatureKnockComponent.h"
#include "Creatures/CreatureReactSubsystem.h"
#include "Embers/EmberDamageType.h"
#include "Embers/EmberDefinition.h"
#include "Embers/Powers/EmberVfx.h"
#include "Player/CameraShakeModifier.h"
#include "Player/PlayerViewComponent.h"
#include "Player/SlideDust.h"
#include "Player/ViewKick.h"
#include "Weapons/WeaponFX.h"
#include "World/PlayableArea.h"
#include "World/WorldQueries.h"
#include "Camera/PlayerCameraManager.h"
#include "CollisionQueryParams.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

// UEmberDustDevil (Ellis's Dust Devil): the creatures in its way (struck, knocked, the blow's look) and the show (speed lines, view widening,
// trail dust, dust rings).

namespace
{
	/** How high over or under the path a creature may stand and still be caught in it (cm). */
	constexpr float ReachHeight = 220.f;
	/** Speed lines a second while it dashes, how bright, and how far round the eye they rush past (cm). */
	constexpr float LinesPerSecond = 80.f;
	constexpr float LineGlow = 7.f;
	constexpr float LineNear = 70.f;
	constexpr float LineFar = 190.f;
	/** A puff of the trail every so far along the ground (cm). */
	constexpr float TrailEvery = 110.f;

	FWeaponFX* EffectsIn(UWorld* World)
	{
		UBulletSubsystem* Bullets = World ? World->GetSubsystem<UBulletSubsystem>() : nullptr;
		if (!Bullets)
		{
			return nullptr;
		}
		FWeaponFX& Effects = Bullets->GetEffects();
		Effects.Initialize(World);
		return &Effects;
	}

	/** Dust in the light where it's thrown (the puffs are unlit), as the creatures' and the slide's is. */
	FLinearColor DustColor(const UObject* WorldContext)
	{
		UCreatureReactSubsystem* Reacts = WorldContext ? UCreatureReactSubsystem::Get(WorldContext) : nullptr;
		return FSlideDust::DustAlbedo(ESlideGround::Dirt) * (Reacts ? Reacts->GetDustLight() : FLinearColor(0.8f, 0.8f, 0.8f));
	}

	APlayerController* PlayerOf(const ACharacter* Character)
	{
		return Character ? Cast<APlayerController>(Character->GetController()) : nullptr;
	}
}

// ---------------------------------------------------------------------------
// The creatures in its way
// ---------------------------------------------------------------------------

void UEmberDustDevil::StrikeAlong(float From, float To)
{
	ACharacter* Character = Body.Get();
	UWorld* World = Character ? Character->GetWorld() : nullptr;
	if (!World || To < From)
	{
		return;
	}
	const FVector A = Path.At(From);
	const FVector B = Path.At(To);
	const FCollisionQueryParams SightParams = LooterWorld::StaticGeometryParams(World, TEXT("DustDevilSight"), Character);
	for (const TWeakObjectPtr<ACreatureBase>& Each : Candidates)
	{
		ACreatureBase* Creature = Each.Get();
		if (!Creature || Creature->IsDead() || Struck.Contains(Each))
		{
			continue;
		}
		const FVector Where = Creature->GetActorLocation();
		const FVector Nearest = FMath::ClosestPointOnSegment(Where, A, B);
		if (FVector::Dist2D(Where, Nearest) > Radius || FMath::Abs(Where.Z - Nearest.Z) > ReachHeight)
		{
			continue;
		}
		// Not through a wall: the wind reaches only what the path can see.
		FHitResult Wall;
		if (World->LineTraceSingleByObjectType(Wall, Nearest, Where, FCollisionObjectQueryParams(ECC_WorldStatic), SightParams))
		{
			continue;
		}
		Strike(*Creature, Nearest);
	}
}

void UEmberDustDevil::Strike(ACreatureBase& Creature, const FVector& Nearest)
{
	Struck.Add(&Creature);
	ACharacter* Character = Body.Get();
	UWorld* World = Creature.GetWorld();

	// Flung out to the side of the path it was on (straight in the way: a side of its own), carried a little along.
	FVector Side = Creature.GetActorLocation() - Nearest;
	Side.Z = 0.0;
	// Square to the dash: the nearest path point lies behind a creature struck as the dash passes it, and measured from
	// there the way out was a diagonal (the test saw a fling more along the path than off it).
	Side -= Path.Direction * FVector::DotProduct(Side, Path.Direction);
	if (Side.SizeSquared() < 100.0)
	{
		const float Sign = (GetTypeHash(Creature.GetFName()) & 1u) ? 1.f : -1.f;
		Side = FVector::CrossProduct(FVector::UpVector, Path.Direction) * Sign;
	}
	const FVector Away = (Side.GetSafeNormal() + Path.Direction * 0.2).GetSafeNormal();

	// The damage, the player's (the hit marker, the number, the kill's experience), no gun's.
	UGameplayStatics::ApplyDamage(&Creature, Damage, Instigator.Get(), nullptr, UEmberDamageType::StaticClass());
	if (!Creature.IsDead())
	{
		if (UCreatureKnockComponent* Knock = Creature.GetKnock())
		{
			Knock->Knock(Away, Random.FRandRange(FlingMin, FlingMax), FlingHeight, Dizzy);
		}
	}

	// A burst of dust off it the way it goes, grit, and the blow's weight.
	const FVector Middle = Creature.GetActorLocation();
	LooterSound::PlayAt(&Creature, LooterSoundCue::Melee::Hit, Middle, 0.8f);
	if (FWeaponFX* Effects = EffectsIn(World))
	{
		const FLinearColor Dust = DustColor(&Creature);
		const float Size = FMath::Max(Creature.GetSizeScale(), 0.4f);
		for (int32 Index = 0; Index < 7; ++Index)
		{
			const FVector Out = (Away + Random.GetUnitVector() * 0.6).GetSafeNormal();
			Effects->SpawnDustPuff(Middle + Out * (25.f * Size), Out * Random.FRandRange(220.f, 420.f) + FVector(0.0, 0.0, 60.0), Dust, 0.45f,
				22.f * Size, Random.FRandRange(80.f, 120.f) * Size, Random.FRandRange(0.7f, 1.1f));
		}
		for (int32 Index = 0; Index < 6; ++Index)
		{
			const FVector Out = (Away + FVector(0.0, 0.0, 0.8) + Random.GetUnitVector() * 0.5).GetSafeNormal();
			Effects->SpawnGrit(Middle, Out * Random.FRandRange(300.f, 600.f), Random.FRandRange(0.8f, 2.f), Random.FRandRange(0.5f, 0.9f));
		}
		// (The VFX pass) the blow on the body: what it spills, and the gust's strike snapping out round it.
		EmberVfx::BodyHit(*Effects, Creature, Middle - Away * 20.f, Away, Damage, Color);
		EmberVfx::Strike(*Effects, Middle, Color, 0.7f);
	}
	// Each one struck is felt: a small jolt toward it.
	if (APlayerController* Player = PlayerOf(Character))
	{
		const FVector ViewRight = FRotator(0.f, Player->GetControlRotation().Yaw, 0.f).RotateVector(FVector::RightVector);
		FViewKick Jolt;
		Jolt.Pitch = 0.3f;
		Jolt.Yaw = 0.4f * FMath::Sign(static_cast<float>(FVector::DotProduct(Side, ViewRight)));
		Jolt.Frequency = 12.f;
		Jolt.Damping = 0.6f;
		UCameraShakeModifier::Kick(Player, Jolt);
	}
	UE_LOG(LogLooter, Verbose, TEXT("Dust Devil: struck %s for %.1f"), *Creature.GetName(), Damage);
}

// ---------------------------------------------------------------------------
// The show
// ---------------------------------------------------------------------------

void UEmberDustDevil::SetViewWidening(float Degrees)
{
	Widening = Degrees;
	if (UPlayerViewComponent* View = Body.IsValid() ? Body->FindComponentByClass<UPlayerViewComponent>() : nullptr)
	{
		View->SetPowerFieldOfView(Degrees);
	}
}

FVector UEmberDustDevil::EyeLocation() const
{
	const ACharacter* Character = Body.Get();
	if (!Character)
	{
		return FVector::ZeroVector;
	}
	const APlayerController* Player = PlayerOf(Character);
	if (Player && Player->PlayerCameraManager)
	{
		return Player->PlayerCameraManager->GetCameraLocation();
	}
	return Character->GetActorLocation() + FVector(0.0, 0.0, Character->BaseEyeHeight);
}

FVector UEmberDustDevil::FeetAt(const FVector& Middle) const
{
	const ACharacter* Character = Body.Get();
	const float HalfHeight = Character && Character->GetCapsuleComponent() ? Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 90.f;
	return Middle - FVector(0.0, 0.0, HalfHeight);
}

void UEmberDustDevil::SpawnSpeedLine()
{
	// A line of rushing air somewhere ahead and round the eye (never across the crosshair, never into the ground), lying
	// along the dash: the body runs through them, so they streak past.
	const FVector Along = Path.Direction;
	const FVector Right = FVector::CrossProduct(FVector::UpVector, Along).GetSafeNormal();
	float Angle = Random.FRandRange(0.f, 2.f * UE_PI);
	if (FMath::Sin(Angle) < -0.55f)
	{
		Angle = -Angle;
	}
	const float Out = Random.FRandRange(LineNear, LineFar);
	const FVector Offset = (Right * FMath::Cos(Angle) + FVector::UpVector * FMath::Sin(Angle)) * Out;
	const float Length = Random.FRandRange(140.f, 300.f);
	const FVector Center = EyeLocation() + Along * Random.FRandRange(200.f, 650.f) + Offset;
	FSpeedLine& Line = SpeedLines.AddDefaulted_GetRef();
	Line.Tail = Center - Along * (Length * 0.5f);
	Line.Head = Center + Along * (Length * 0.5f);
	Line.Life = Random.FRandRange(0.16f, 0.24f);
	Line.Width = Random.FRandRange(1.1f, 1.8f);
}

void UEmberDustDevil::TickSpeedLines(float DeltaSeconds)
{
	if (bDashing)
	{
		LinesOwed += DeltaSeconds * LinesPerSecond;
		while (LinesOwed >= 1.f)
		{
			LinesOwed -= 1.f;
			SpawnSpeedLine();
		}
	}
	for (int32 Index = SpeedLines.Num() - 1; Index >= 0; --Index)
	{
		SpeedLines[Index].Age += DeltaSeconds;
		if (SpeedLines[Index].Age >= SpeedLines[Index].Life)
		{
			SpeedLines.RemoveAtSwap(Index);
		}
	}
	FWeaponFX* Effects = SpeedLines.IsEmpty() ? nullptr : EffectsIn(Body.IsValid() ? Body->GetWorld() : nullptr);
	if (!Effects)
	{
		return;
	}
	for (const FSpeedLine& Line : SpeedLines)
	{
		const float Fade = FMath::Clamp(Line.Age / 0.04f, 0.f, 1.f) * (1.f - Line.Age / Line.Life);
		Effects->AddStreak(Line.Tail, Line.Head, Line.Width, Color, LineGlow * Fade);
	}
}

void UEmberDustDevil::TrailDust(float From, float To)
{
	if (!Path.bOnGround || To <= TrailLaid)
	{
		return;
	}
	FWeaponFX* Effects = EffectsIn(Body.IsValid() ? Body->GetWorld() : nullptr);
	if (!Effects)
	{
		TrailLaid = To;
		return;
	}
	const FLinearColor Dust = DustColor(Body.Get());
	// A low, thin trail kicked up where the feet skimmed, curling up behind.
	while (TrailLaid + TrailEvery <= To)
	{
		TrailLaid += TrailEvery;
		const FVector Feet = FeetAt(Path.At(TrailLaid));
		const FVector Drift = -Path.Direction * Random.FRandRange(40.f, 120.f) + FVector(0.0, 0.0, Random.FRandRange(30.f, 70.f));
		Effects->SpawnDustPuff(Feet + FVector(0.0, 0.0, 10.0), Drift, Dust, 0.22f, 20.f, Random.FRandRange(70.f, 95.f), Random.FRandRange(0.7f, 1.0f));
	}
}

void UEmberDustDevil::SpawnDustRing(const FVector& Feet, float Scale, int32 Puffs)
{
	FWeaponFX* Effects = EffectsIn(Body.IsValid() ? Body->GetWorld() : nullptr);
	if (!Effects || Puffs <= 0)
	{
		return;
	}
	const FLinearColor Dust = DustColor(Body.Get());
	// The ring rolling out over the ground...
	for (int32 Index = 0; Index < Puffs; ++Index)
	{
		const float Angle = 2.f * UE_PI * (Index + Random.FRandRange(-0.3f, 0.3f)) / Puffs;
		const FVector Out(FMath::Cos(Angle), FMath::Sin(Angle), 0.f);
		const float Start = Random.FRandRange(22.f, 32.f) * Scale;
		Effects->SpawnDustPuff(Feet + Out * (30.f * Scale) + FVector(0.0, 0.0, 10.0 * Scale),
			Out * Random.FRandRange(280.f, 460.f) * Scale + FVector(0.0, 0.0, Random.FRandRange(30.f, 80.f)), Dust, 0.5f, Start,
			Start * Random.FRandRange(3.2f, 4.2f), Random.FRandRange(0.9f, 1.4f));
	}
	// ...grit flicked up off it...
	for (int32 Index = 0; Index < Puffs / 2; ++Index)
	{
		const FVector Out = (Random.GetUnitVector() * FVector(1.0, 1.0, 0.0) + FVector(0.0, 0.0, 1.0)).GetSafeNormal();
		Effects->SpawnGrit(Feet + FVector(0.0, 0.0, 5.0), Out * Random.FRandRange(250.f, 480.f) * Scale, Random.FRandRange(0.8f, 1.8f),
			Random.FRandRange(0.45f, 0.8f));
	}
	// ...and a little whirl of air spinning up off the middle: the dust devil itself.
	for (int32 Index = 0; Index < 5; ++Index)
	{
		const float Angle = 2.f * UE_PI * Index / 5.f;
		const FVector Round(FMath::Cos(Angle), FMath::Sin(Angle), 0.f);
		const FVector Spin(-FMath::Sin(Angle), FMath::Cos(Angle), 0.f);
		Effects->SpawnWisp(Feet + Round * (35.f * Scale) + FVector(0.0, 0.0, 20.0 * Scale),
			Spin * (160.f * Scale) + FVector(0.0, 0.0, Random.FRandRange(90.f, 150.f) * Scale), 0.18f, 18.f * Scale, 70.f * Scale,
			Random.FRandRange(0.6f, 0.9f));
	}
}

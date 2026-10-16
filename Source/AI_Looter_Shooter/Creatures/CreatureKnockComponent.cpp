#include "Creatures/CreatureKnockComponent.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Audio/LooterSoundRules.h"
#include "Bosses/BossComponent.h"
#include "Combat/BulletSubsystem.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/CreatureReactSubsystem.h"
#include "Player/SlideDust.h"
#include "Weapons/WeaponFX.h"
#include "World/PlayableArea.h"
#include "World/WorldQueries.h"
#include "CollisionQueryParams.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"

namespace
{
	/** A boss: Boss rank, or a creature a boss fight is built round (its UBossComponent). Its fight's rules hold. */
	bool IsBoss(const ACreatureBase& Creature)
	{
		return Creature.GetRank() == ECreatureRank::Boss || Creature.FindComponentByClass<UBossComponent>() != nullptr;
	}

	/** On landing, what's left of the tumble settles upright over this long. */
	constexpr float SettleSeconds = 0.15f;
	/** The dizzy body's sway: degrees of yaw each way, and how often a second. */
	constexpr float SwayDegrees = 9.f;
	constexpr float SwayHz = 1.6f;
	/** The sparks over its head: how many, how often round a second, how far out (cm at size 1). */
	constexpr int32 SparkCount = 3;
	constexpr float SparkTurnsPerSecond = 1.4f;
	constexpr float SparkRadius = 24.f;
	const FLinearColor SparkColor(1.f, 0.86f, 0.45f);
	constexpr float SparkGlow = 22.f;
	/** Dust and sparks are only drawn this near a player's view (cm). */
	constexpr float ShowWithin = 5000.f;

	/** 0 to 1 eased in and out. */
	float Ease(float U)
	{
		const float X = FMath::Clamp(U, 0.f, 1.f);
		return X * X * (3.f - 2.f * X);
	}
}

UCreatureKnockComponent::UCreatureKnockComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	// After the creature's own frame (it ticks after physics), so the body is turned over this frame's pose and place.
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
}

UCreatureKnockComponent* UCreatureKnockComponent::Find(const AActor* Actor)
{
	return Actor ? Actor->FindComponentByClass<UCreatureKnockComponent>() : nullptr;
}

ACreatureBase* UCreatureKnockComponent::GetCreature() const
{
	return Cast<ACreatureBase>(GetOwner());
}

void UCreatureKnockComponent::BeginPlay()
{
	Super::BeginPlay();
	CaptureRestPose();
	if (ACreatureBase* Creature = GetCreature())
	{
		Creature->LandedDelegate.AddDynamic(this, &UCreatureKnockComponent::HandleLanded);
	}
}

void UCreatureKnockComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ACreatureBase* Creature = GetCreature())
	{
		Creature->LandedDelegate.RemoveDynamic(this, &UCreatureKnockComponent::HandleLanded);
	}
	Recover();
	Super::EndPlay(EndPlayReason);
}

// ---------------------------------------------------------------------------
// The rules
// ---------------------------------------------------------------------------

float UCreatureKnockComponent::FlingShare(ECreatureRank Rank)
{
	switch (Rank)
	{
	case ECreatureRank::Rare: return 0.85f;
	case ECreatureRank::Epic: return 0.7f;
	case ECreatureRank::Legendary: return 0.45f;
	case ECreatureRank::Boss: return 0.f;
	default: return 1.f;
	}
}

FVector UCreatureKnockComponent::LaunchVelocity(const FVector& Away, float Distance, float Height, float Gravity)
{
	// Up fast enough to top out Height over the start, and across fast enough to cover Distance in the time it takes to
	// come back down to the start's height.
	const float G = FMath::Max(Gravity, 1.f);
	const float Up = FMath::Sqrt(2.f * G * FMath::Max(Height, 1.f));
	const float Air = 2.f * Up / G;
	const FVector Flat = FVector(Away.X, Away.Y, 0.f).GetSafeNormal();
	return Flat * (FMath::Max(Distance, 0.f) / Air) + FVector(0.f, 0.f, Up);
}

// ---------------------------------------------------------------------------
// Knocked and dazed
// ---------------------------------------------------------------------------

FVector UCreatureKnockComponent::Knock(const FVector& Away, float Distance, float Height, float DizzySeconds)
{
	ACreatureBase* Creature = GetCreature();
	UCharacterMovementComponent* Movement = Creature ? Creature->GetCharacterMovement() : nullptr;
	// A boss's fight keeps its own rules; a corpse goes with its death's knock; one held back by a spell walks home.
	if (!Creature || !Movement || !GetWorld() || Creature->IsDead() || Creature->IsPassive() || IsBoss(*Creature))
	{
		return FVector::ZeroVector;
	}
	const FVector Way = FVector(Away.X, Away.Y, 0.f).GetSafeNormal();
	const float Size = FMath::Max(Creature->GetSizeScale(), 0.3f);
	// A tougher rank and a bigger body give less ground.
	float Length = Distance * FlingShare(Creature->GetRank()) / FMath::Sqrt(Size);
	if (Way.IsNearlyZero() || Length < 1.f)
	{
		Daze(DizzySeconds);
		return FVector::ZeroVector;
	}

	// Where it comes down: ground near its feet's height, else a shorter throw, else it only reels where it stands. Never
	// off a ledge, into a pit, or out of the playable area.
	const float FeetZ = static_cast<float>(Creature->GetActorLocation().Z) - Creature->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	float Share = 0.f;
	for (const float Try : { 1.f, 0.75f, 0.5f, 0.3f })
	{
		if (HasLandingAt(Creature->GetActorLocation() + Way * (Length * Try), FeetZ, Size))
		{
			Share = Try;
			break;
		}
	}
	if (Share <= 0.f)
	{
		Daze(DizzySeconds);
		return FVector::ZeroVector;
	}
	Length *= Share;

	const float Gravity = FMath::Max(-Movement->GetGravityZ(), 1.f);
	const FVector Launch = LaunchVelocity(Way, Length, Height, Gravity);
	// Its own speed is replaced, not added to: a charge at the player is turned aside, which is the point.
	Creature->LaunchCharacter(Launch, true, true);

	CaptureRestPose();
	State = ECreatureKnock::Knocked;
	StateTime = 0.f;
	DizzyLeft = FMath::Max(DizzySeconds, 0.f);
	DizzyTotal = DizzyLeft;
	PlannedAir = FMath::Max(2.f * static_cast<float>(Launch.Z) / Gravity, 0.2f);
	// It turns head over heels the way it's thrown.
	TumbleAxis = FVector::CrossProduct(FVector::UpVector, Way).GetSafeNormal();
	TumbleDegrees = 0.f;
	LandedTumble = 0.f;
	SetComponentTickEnabled(true);
	UE_LOG(LogLooter, Verbose, TEXT("%s: knocked %.0f cm (%.2f s in the air), dizzy %.1f s"), *Creature->GetName(), Length, PlannedAir, DizzyLeft);
	return Launch;
}

void UCreatureKnockComponent::Daze(float Seconds)
{
	ACreatureBase* Creature = GetCreature();
	if (!Creature || Creature->IsDead() || IsBoss(*Creature) || Seconds <= 0.f)
	{
		return;
	}
	if (State != ECreatureKnock::None)
	{
		// In the air it reels once it lands; already reeling, the longer of the two.
		DizzyLeft = FMath::Max(DizzyLeft, Seconds);
		DizzyTotal = FMath::Max(DizzyTotal, DizzyLeft);
		return;
	}
	CaptureRestPose();
	TumbleDegrees = 0.f;
	LandedTumble = 0.f;
	DizzyLeft = Seconds;
	StartDizzy();
	SetComponentTickEnabled(true);
}

void UCreatureKnockComponent::HandleLanded(const FHitResult& Hit)
{
	NoteLanded();
}

void UCreatureKnockComponent::NoteLanded()
{
	if (State != ECreatureKnock::Knocked)
	{
		return;
	}
	LandedTumble = TumbleDegrees;
	ShowLanding();
	StartDizzy();
}

void UCreatureKnockComponent::StartDizzy()
{
	State = ECreatureKnock::Dizzy;
	StateTime = 0.f;
	// Even a knock asked for no daze reels a moment, so the body settles before the brain takes it up again.
	DizzyLeft = FMath::Max(DizzyLeft, SettleSeconds + 0.05f);
	DizzyTotal = DizzyLeft;
}

void UCreatureKnockComponent::Recover()
{
	State = ECreatureKnock::None;
	StateTime = 0.f;
	DizzyLeft = 0.f;
	DizzyTotal = 0.f;
	TumbleDegrees = 0.f;
	LandedTumble = 0.f;
	RestoreRestPose();
	SetComponentTickEnabled(false);
}

void UCreatureKnockComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	ACreatureBase* Creature = GetCreature();
	if (!Creature || Creature->IsDead() || State == ECreatureKnock::None)
	{
		Recover();
		return;
	}
	StateTime += DeltaTime;

	if (State == ECreatureKnock::Knocked)
	{
		// Once head over heels across the planned flight, upright again as it comes down.
		TumbleDegrees = 360.f * Ease(StateTime / PlannedAir);
		PoseBody(TumbleDegrees, 0.f);
		// Its landing is heard through LandedDelegate; the movement finding the floor (or too long aloft) lands it too.
		const UCharacterMovementComponent* Movement = Creature->GetCharacterMovement();
		if ((StateTime > 0.12f && Movement && Movement->IsMovingOnGround()) || StateTime > MaxAirSeconds)
		{
			NoteLanded();
		}
		return;
	}

	// Dizzy: what's left of the tumble settles upright, the body sways, sparks circle its head.
	DizzyLeft -= DeltaTime;
	const float Upright = LandedTumble >= 180.f ? 360.f : 0.f;
	TumbleDegrees = FMath::Lerp(LandedTumble, Upright, Ease(StateTime / SettleSeconds));
	const float SwayIn = FMath::Clamp(StateTime / 0.12f, 0.f, 1.f);
	const float SwayOut = FMath::Clamp(DizzyLeft / 0.25f, 0.f, 1.f);
	const float Sway = SwayDegrees * SwayIn * SwayOut * FMath::Sin(2.f * UE_PI * SwayHz * StateTime);
	PoseBody(TumbleDegrees, Sway);
	SparkPhase = FMath::Fmod(SparkPhase + DeltaTime * 2.f * UE_PI * SparkTurnsPerSecond, 2.f * UE_PI);
	DrawDizzySparks();
	if (DizzyLeft <= 0.f)
	{
		Recover();
	}
}

// ---------------------------------------------------------------------------
// The body and the show
// ---------------------------------------------------------------------------

bool UCreatureKnockComponent::HasLandingAt(const FVector& Point, float FeetZ, float Size) const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}
	if (const APlayableArea* Area = APlayableArea::Find(World))
	{
		if (!Area->Contains(Point))
		{
			return false;
		}
	}
	const FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(World, TEXT("CreatureKnockLanding"), GetOwner());
	FHitResult Ground;
	const FVector Top(Point.X, Point.Y, FeetZ + LandingRise * Size);
	const FVector Bottom(Point.X, Point.Y, FeetZ - LandingDrop * Size);
	return World->LineTraceSingleByObjectType(Ground, Top, Bottom, FCollisionObjectQueryParams(ECC_WorldStatic), Params)
		&& !Ground.bStartPenetrating && Ground.ImpactNormal.Z > 0.6;
}

void UCreatureKnockComponent::CaptureRestPose()
{
	const ACreatureBase* Creature = GetCreature();
	const USkeletalMeshComponent* Mesh = Creature ? Creature->GetMesh() : nullptr;
	if (!Mesh || bRestCaptured)
	{
		return;
	}
	// The mesh's authored place in the capsule (nothing else moves it in play): every turn starts from it and ends on it.
	RestLocation = Mesh->GetRelativeLocation();
	RestRotation = Mesh->GetRelativeRotation().Quaternion();
	bRestCaptured = true;
}

void UCreatureKnockComponent::RestoreRestPose()
{
	const ACreatureBase* Creature = GetCreature();
	USkeletalMeshComponent* Mesh = Creature ? Creature->GetMesh() : nullptr;
	if (Mesh && bRestCaptured)
	{
		Mesh->SetRelativeLocationAndRotation(RestLocation, RestRotation);
	}
}

void UCreatureKnockComponent::PoseBody(float Degrees, float Sway)
{
	const ACreatureBase* Creature = GetCreature();
	USkeletalMeshComponent* Mesh = Creature ? Creature->GetMesh() : nullptr;
	if (!Mesh || !bRestCaptured)
	{
		return;
	}
	// Turned round the capsule's middle (the mesh hangs under it), so the body cartwheels about its own centre.
	const FVector LocalAxis = Creature->GetActorQuat().UnrotateVector(TumbleAxis).GetSafeNormal();
	const FQuat Tumble = LocalAxis.IsNearlyZero() ? FQuat::Identity : FQuat(LocalAxis, FMath::DegreesToRadians(Degrees));
	const FQuat Turn = Tumble * FQuat(FVector::UpVector, FMath::DegreesToRadians(Sway));
	Mesh->SetRelativeLocationAndRotation(Turn.RotateVector(RestLocation), Turn * RestRotation);
}

void UCreatureKnockComponent::ShowLanding()
{
	ACreatureBase* Creature = GetCreature();
	UWorld* World = GetWorld();
	UBulletSubsystem* Bullets = World ? World->GetSubsystem<UBulletSubsystem>() : nullptr;
	if (!Creature || !Bullets)
	{
		return;
	}
	const float Size = FMath::Max(Creature->GetSizeScale(), 0.3f);
	const FVector Feet = Creature->GetActorLocation() - FVector(0.0, 0.0, Creature->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
	// A heavy body coming down: its weight heard, and dust thrown out round it.
	LooterSound::PlayAt(this, LooterSoundCue::Melee::Hit, Feet, 0.45f, LooterSoundRules::PitchForSize(Size) * 0.85f);
	UCreatureReactSubsystem* Reacts = UCreatureReactSubsystem::Get(this);
	if (!Reacts || !Reacts->IsNearAViewer(Feet, ShowWithin))
	{
		return;
	}
	FWeaponFX& Effects = Bullets->GetEffects();
	Effects.Initialize(World);
	const FLinearColor Color = FSlideDust::DustAlbedo(ESlideGround::Dirt) * Reacts->GetDustLight();
	FRandomStream Random(static_cast<int32>(GetTypeHash(Feet)));
	constexpr int32 Ring = 7;
	for (int32 Index = 0; Index < Ring; ++Index)
	{
		const float Angle = 2.f * UE_PI * (Index + Random.FRandRange(-0.3f, 0.3f)) / Ring;
		const FVector Out(FMath::Cos(Angle), FMath::Sin(Angle), 0.f);
		const float Start = Random.FRandRange(16.f, 24.f) * Size;
		Effects.SpawnDustPuff(Feet + Out * (30.f * Size) + FVector(0.0, 0.0, 8.0 * Size),
			Out * Random.FRandRange(140.f, 240.f) * FMath::Sqrt(Size) + FVector(0.0, 0.0, Random.FRandRange(40.f, 100.f)), Color,
			0.45f, Start, Start * Random.FRandRange(3.f, 4.f), Random.FRandRange(0.9f, 1.4f));
	}
	for (int32 Index = 0; Index < 6; ++Index)
	{
		const FVector Out = (Random.GetUnitVector() * FVector(1.0, 1.0, 0.0) + FVector(0.0, 0.0, 1.2)).GetSafeNormal();
		Effects.SpawnGrit(Feet + FVector(0.0, 0.0, 4.0), Out * Random.FRandRange(220.f, 420.f), Random.FRandRange(0.8f, 1.8f),
			Random.FRandRange(0.4f, 0.7f));
	}
}

void UCreatureKnockComponent::DrawDizzySparks()
{
	ACreatureBase* Creature = GetCreature();
	UWorld* World = GetWorld();
	UBulletSubsystem* Bullets = World ? World->GetSubsystem<UBulletSubsystem>() : nullptr;
	if (!Creature || !Bullets)
	{
		return;
	}
	const float Size = FMath::Max(Creature->GetSizeScale(), 0.3f);
	const float HalfHeight = Creature->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const FVector Center = Creature->GetActorLocation() + FVector(0.0, 0.0, HalfHeight + 14.f * Size);
	UCreatureReactSubsystem* Reacts = UCreatureReactSubsystem::Get(this);
	if (Reacts && !Reacts->IsNearAViewer(Center, ShowWithin))
	{
		return;
	}
	FWeaponFX& Effects = Bullets->GetEffects();
	Effects.Initialize(World);
	// They fade in as it starts reeling and out as it comes to.
	const float Fade = FMath::Clamp(StateTime / 0.1f, 0.f, 1.f) * FMath::Clamp(DizzyLeft / 0.2f, 0.f, 1.f);
	const float Radius = SparkRadius * Size;
	for (int32 Index = 0; Index < SparkCount; ++Index)
	{
		const float Angle = SparkPhase + 2.f * UE_PI * Index / SparkCount;
		const FVector Out(FMath::Cos(Angle), FMath::Sin(Angle), 0.f);
		const FVector Along(-FMath::Sin(Angle), FMath::Cos(Angle), 0.f);
		// Each bobs a little as it goes round, so the ring reads as stars circling rather than a hoop.
		const FVector Spark = Center + Out * Radius + FVector(0.0, 0.0, FMath::Sin(2.f * Angle) * 4.f * Size);
		Effects.AddStreak(Spark - Along * (7.f * Size), Spark, 2.4f * Size, SparkColor, SparkGlow * Fade);
		// A short dim tail behind each.
		Effects.AddStreak(Spark - Along * (16.f * Size), Spark - Along * (7.f * Size), 1.4f * Size, SparkColor, SparkGlow * 0.35f * Fade);
	}
}

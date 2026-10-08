#include "Audio/PlayerSoundComponent.h"
#include "Audio/LooterSound.h"
#include "Audio/SoundSurface.h"
#include "Combat/HealthComponent.h"
#include "Player/PlayerLocomotionComponent.h"
#include "UI/HUD/HudPlayerFrameWidget.h"
#include "Components/AudioComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

namespace
{
	/** The first step after setting off comes this share of a stride in: at once it would sound like a stomp. */
	constexpr float FirstStepShare = 0.35f;

	/** Creeping on the stick, and the sprint (the player's speeds, Player/PlayerSize.h: the jog is 510). */
	constexpr float CreepSpeed = 150.f;
	constexpr float SprintSpeed = 790.f;

	/** Landings slower than this are a step's worth (a hop, a curb); from it to HardLandSpeed they grow to a full thud. */
	constexpr float SoftLandSpeed = 300.f;
	constexpr float HardLandSpeed = 1100.f;

	/** How far down a step looks for the floor's face under the foot (cm). */
	constexpr float StepProbeAbove = 30.f;
	constexpr float StepProbeBelow = 50.f;
}

UPlayerSoundComponent::UPlayerSoundComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	// After the movement, so a step goes by this frame's speed and floor.
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
}

float UPlayerSoundComponent::StrideLength(float Speed)
{
	// About 1.1 m creeping, 1.6 m at the jog, 2.2 m sprinting: a step every 0.6, 0.32 and 0.28 seconds.
	return 60.f + 0.2f * FMath::Max(Speed, 0.f);
}

float UPlayerSoundComponent::StepVolume(float Speed)
{
	return FMath::GetMappedRangeValueClamped(FVector2f(CreepSpeed, SprintSpeed), FVector2f(0.45f, 1.f), Speed);
}

float UPlayerSoundComponent::LandVolume(float FallSpeed)
{
	if (FallSpeed < SoftLandSpeed)
	{
		return 0.f;
	}
	return FMath::GetMappedRangeValueClamped(FVector2f(SoftLandSpeed, HardLandSpeed), FVector2f(0.4f, 1.f), FallSpeed);
}

void UPlayerSoundComponent::BeginPlay()
{
	Super::BeginPlay();
	AActor* Owner = GetOwner();
	if (ACharacter* Character = Cast<ACharacter>(Owner))
	{
		Character->LandedDelegate.AddDynamic(this, &UPlayerSoundComponent::HandleLanded);
	}
	Locomotion = Owner ? Owner->FindComponentByClass<UPlayerLocomotionComponent>() : nullptr;
	if (UHealthComponent* Found = Owner ? Owner->FindComponentByClass<UHealthComponent>() : nullptr)
	{
		Health = Found;
		Found->OnDamaged.AddDynamic(this, &UPlayerSoundComponent::HandleDamaged);
		Found->OnHealthChanged.AddDynamic(this, &UPlayerSoundComponent::HandleHealthChanged);
		Found->OnDeath.AddDynamic(this, &UPlayerSoundComponent::HandleDeath);
		// A session can begin hurt: the heartbeat picks it up from the start.
		HandleHealthChanged(Found->GetHealth(), Found->GetMaxHealth());
	}
	StrideLeft = StrideLength(0.f) * FirstStepShare;
}

void UPlayerSoundComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// The heartbeat is heard flat, not from the body: it would go on into the next body after a respawn.
	SetHeartbeat(false);
	Super::EndPlay(EndPlayReason);
}

void UPlayerSoundComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	const UCharacterMovementComponent* Movement = Character ? Character->GetCharacterMovement() : nullptr;
	const float Speed = Movement && Movement->IsMovingOnGround() ? static_cast<float>(Movement->Velocity.Size2D()) : 0.f;
	const bool bSliding = Locomotion.IsValid() && Locomotion->IsSliding();
	if (Speed < MinStepSpeed || bSliding)
	{
		// Standing, in the air or sliding: the next walk's first step comes soon after setting off.
		StrideLeft = FMath::Min(StrideLeft, StrideLength(Speed) * FirstStepShare);
		return;
	}
	StrideLeft -= Speed * DeltaTime;
	if (StrideLeft <= 0.f)
	{
		StrideLeft = FMath::Max(StrideLeft + StrideLength(Speed), 0.f);
		// Crouched steps are placed softly.
		Step(StepVolume(Speed) * (Movement->IsCrouching() ? 0.7f : 1.f));
	}
}

void UPlayerSoundComponent::Step(float VolumeScale)
{
	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (const UCharacterMovementComponent* Movement = Character ? Character->GetCharacterMovement() : nullptr)
	{
		StepOn(Movement->CurrentFloor.HitResult, VolumeScale);
	}
}

void UPlayerSoundComponent::StepOn(const FHitResult& Floor, float VolumeScale)
{
	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!Character)
	{
		return;
	}
	const FVector Feet = Character->GetActorLocation() - FVector(0.f, 0.f, Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
	// The movement found the floor with a sweep of its capsule, which says what it stands on but not which face: a short
	// line against that component alone finds the face under the foot, and its material (the terrain's tiles have
	// several).
	FHitResult Under = Floor;
	if (UPrimitiveComponent* FloorComponent = Floor.GetComponent())
	{
		FCollisionQueryParams Params(SCENE_QUERY_STAT(FootstepSurface), /*bTraceComplex*/ true);
		Params.bReturnFaceIndex = true;
		FHitResult Face;
		if (FloorComponent->LineTraceComponent(Face, Feet + FVector(0.f, 0.f, StepProbeAbove), Feet - FVector(0.f, 0.f, StepProbeBelow), Params))
		{
			Under = Face;
		}
	}
	const ESoundSurface Surface = Under.GetComponent() ? SoundSurface::Of(Under) : ESoundSurface::Dirt;
	LooterSound::PlayAt(this, SoundSurface::FootstepCue(Surface), Feet, VolumeScale);
}

void UPlayerSoundComponent::Jumped()
{
	AActor* Owner = GetOwner();
	LooterSound::PlayAttached(LooterSoundCue::Jump, Owner ? Owner->GetRootComponent() : nullptr);
	// The push-off: the feet leave whatever they stood on.
	Step(0.5f);
}

void UPlayerSoundComponent::HandleLanded(const FHitResult& Hit)
{
	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	const UCharacterMovementComponent* Movement = Character ? Character->GetCharacterMovement() : nullptr;
	// Still the speed it came down at: the movement tells the character before it takes the fall's speed away.
	const float FallSpeed = Movement ? FMath::Max(0.f, static_cast<float>(-Movement->Velocity.Z)) : 0.f;
	const float Volume = LandVolume(FallSpeed);
	if (Volume > 0.f)
	{
		// The harder the landing, the louder and lower the thud.
		const float Hardness = FMath::GetMappedRangeValueClamped(FVector2f(SoftLandSpeed, HardLandSpeed), FVector2f(0.f, 1.f), FallSpeed);
		LooterSound::PlayAt(this, LooterSoundCue::Land, Hit.ImpactPoint, Volume, FMath::Lerp(1.04f, 0.86f, Hardness));
	}
	// The feet meet the ground under the thud, on its own surface; the next step comes a stride on.
	StepOn(Hit, FMath::Max(Volume, 0.6f));
	StrideLeft = StrideLength(Movement ? static_cast<float>(Movement->Velocity.Size2D()) : 0.f);
}

void UPlayerSoundComponent::HandleDamaged(float Damage, bool bCritical, FVector HitLocation, AController* InstigatedBy, AActor* DamageCauser)
{
	const UHealthComponent* Body = Health.Get();
	const UWorld* World = GetWorld();
	// The killing blow has the death's sound instead.
	if (!Body || !World || Body->GetHealth() <= 0.f)
	{
		return;
	}
	const double Now = World->GetTimeSeconds();
	if (Now < NextHurtTime)
	{
		return;
	}
	NextHurtTime = Now + HurtInterval;
	// A bigger bite is a louder grunt: a fifth of the health or more is a full one.
	const float Share = Body->GetMaxHealth() > 0.f ? Damage / Body->GetMaxHealth() : 0.f;
	LooterSound::Play2D(this, LooterSoundCue::PlayerHurt, FMath::Lerp(0.7f, 1.f, FMath::Clamp(Share * 5.f, 0.f, 1.f)));
}

void UPlayerSoundComponent::HandleHealthChanged(float NewHealth, float MaxHealth)
{
	// The HUD's low share, so the heartbeat starts as the frame and the screen's edges start to beat.
	const bool bDead = Health.IsValid() && Health->IsDead();
	SetHeartbeat(!bDead && NewHealth > 0.f && MaxHealth > 0.f && NewHealth / MaxHealth <= UHudPlayerFrameWidget::LowFraction);
}

void UPlayerSoundComponent::HandleDeath(AController* Killer)
{
	SetHeartbeat(false);
	LooterSound::Play2D(this, LooterSoundCue::PlayerDeath);
}

void UPlayerSoundComponent::SetHeartbeat(bool bOn)
{
	UAudioComponent* Playing = Heartbeat.Get();
	if (bOn == (Playing != nullptr))
	{
		return;
	}
	if (bOn)
	{
		Heartbeat = LooterSound::Start(this, LooterSoundCue::LowHealth);
	}
	else
	{
		// It fades as health comes back, rather than cutting off mid-beat.
		LooterSound::Stop(Playing, 0.6f);
		Heartbeat.Reset();
	}
}

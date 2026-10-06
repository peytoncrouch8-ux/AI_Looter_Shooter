#include "Combat/MovementSlowComponent.h"
#include "AI_Looter_Shooter.h"
#include "Player/PlayerLocomotionComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

namespace
{
	UCharacterMovementComponent* MovementOf(const UActorComponent* Component)
	{
		const ACharacter* Character = Cast<ACharacter>(Component->GetOwner());
		return Character ? Character->GetCharacterMovement() : nullptr;
	}
}

UMovementSlowComponent::UMovementSlowComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

UMovementSlowComponent* UMovementSlowComponent::Apply(ACharacter* Victim, float SlowShare, float Seconds)
{
	if (!Victim || Seconds <= 0.f)
	{
		return nullptr;
	}
	UMovementSlowComponent* Slow = Victim->FindComponentByClass<UMovementSlowComponent>();
	if (!Slow)
	{
		Slow = NewObject<UMovementSlowComponent>(Victim);
		Victim->AddInstanceComponent(Slow);
		Slow->RegisterComponent();
	}
	Slow->AddSlow(SlowShare, Seconds);
	return Slow;
}

void UMovementSlowComponent::OnRegister()
{
	Super::OnRegister();
	// After everything that sets the walking speeds each frame (the movement's own frame, then the player's sprint and
	// aim), so the slow is the last word before the next frame's move.
	if (UCharacterMovementComponent* Movement = MovementOf(this))
	{
		AddTickPrerequisiteComponent(Movement);
	}
	const AActor* Owner = GetOwner();
	if (UPlayerLocomotionComponent* Locomotion = Owner ? Owner->FindComponentByClass<UPlayerLocomotionComponent>() : nullptr)
	{
		AddTickPrerequisiteComponent(Locomotion);
	}
}

void UMovementSlowComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Release();
	TimeLeft = 0.f;
	Super::EndPlay(EndPlayReason);
}

void UMovementSlowComponent::AddSlow(float SlowShare, float Seconds)
{
	const float Share = FMath::Clamp(SlowShare, 0.05f, 1.f);
	Multiplier = IsSlowed() ? FMath::Min(Multiplier, Share) : Share;
	TimeLeft = FMath::Max(TimeLeft, Seconds);
	UE_LOG(LogLooter, Verbose, TEXT("%s slowed to %.0f%% for %.1f s."), *GetNameSafe(GetOwner()), Multiplier * 100.f, TimeLeft);
	// At once: the next move is already slowed.
	HoldSpeeds();
	SetComponentTickEnabled(true);
}

void UMovementSlowComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	Advance(DeltaTime);
}

void UMovementSlowComponent::Advance(float DeltaSeconds)
{
	if (!IsSlowed())
	{
		return;
	}
	TimeLeft = FMath::Max(TimeLeft - DeltaSeconds, 0.f);
	if (IsSlowed())
	{
		HoldSpeeds();
		return;
	}
	Release();
	Multiplier = 1.f;
	SetComponentTickEnabled(false);
}

void UMovementSlowComponent::HoldSpeeds()
{
	UCharacterMovementComponent* Movement = MovementOf(this);
	if (!Movement)
	{
		return;
	}
	// A speed that isn't the one it last wrote was set by something else (a sprint starting, the sight coming up): that's
	// the character's own speed now.
	if (!bHolding || !FMath::IsNearlyEqual(Movement->MaxWalkSpeed, WrittenWalkSpeed))
	{
		OwnWalkSpeed = Movement->MaxWalkSpeed;
	}
	if (!bHolding || !FMath::IsNearlyEqual(Movement->MaxWalkSpeedCrouched, WrittenCrouchedSpeed))
	{
		OwnCrouchedSpeed = Movement->MaxWalkSpeedCrouched;
	}
	WrittenWalkSpeed = OwnWalkSpeed * Multiplier;
	WrittenCrouchedSpeed = OwnCrouchedSpeed * Multiplier;
	Movement->MaxWalkSpeed = WrittenWalkSpeed;
	Movement->MaxWalkSpeedCrouched = WrittenCrouchedSpeed;
	bHolding = true;
}

void UMovementSlowComponent::Release()
{
	UCharacterMovementComponent* Movement = MovementOf(this);
	if (!bHolding || !Movement)
	{
		bHolding = false;
		return;
	}
	if (FMath::IsNearlyEqual(Movement->MaxWalkSpeed, WrittenWalkSpeed))
	{
		Movement->MaxWalkSpeed = OwnWalkSpeed;
	}
	if (FMath::IsNearlyEqual(Movement->MaxWalkSpeedCrouched, WrittenCrouchedSpeed))
	{
		Movement->MaxWalkSpeedCrouched = OwnCrouchedSpeed;
	}
	bHolding = false;
}

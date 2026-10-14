#include "Loot/PickupPull.h"
#include "Combat/BulletSubsystem.h"
#include "Loot/SoulMotePickup.h"
#include "Weapons/WeaponFX.h"
#include "Camera/PlayerCameraManager.h"
#include "CollisionQueryParams.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

namespace
{
	/** The line is looked along from this far over the pickup, so a pile lying in a dip still sees past its own ground. */
	constexpr double PullLineLift = 20.0;
	/** The glint's brightest (the effects' glow strength: a tracer's is 30), and the trail's. */
	constexpr float PullGlintGlow = 12.f;
	constexpr float PullTrailGlow = 2.5f;
	constexpr float PullTrailWidth = 1.2f;
	/** The glint flares up over this long, then fades out to GlintSeconds. */
	constexpr float PullGlintRise = 0.05f;
}

// ---------------------------------------------------------------------------
// Reach and line
// ---------------------------------------------------------------------------

FVector PickupPull::TargetOf(const APawn& Player)
{
	if (const ACharacter* Character = Cast<ACharacter>(&Player))
	{
		if (const UCapsuleComponent* Capsule = Character->GetCapsuleComponent())
		{
			return Capsule->GetComponentLocation();
		}
	}
	return Player.GetActorLocation();
}

bool PickupPull::HasClearLine(const UWorld& World, const FVector& From, const APawn& Player, const AActor* Pickup)
{
	FCollisionQueryParams Params(SCENE_QUERY_STAT(PickupPullLine), /*bTraceComplex*/ false);
	Params.AddIgnoredActor(&Player);
	if (Pickup)
	{
		Params.AddIgnoredActor(Pickup);
	}
	// What the player carries (the gun in hand, the guns on the back) is part of them.
	TArray<AActor*> Carried;
	Player.GetAttachedActors(Carried, /*bResetArray*/ true, /*bRecursivelyIncludeAttachedActors*/ true);
	Params.AddIgnoredActors(Carried);
	return !World.LineTraceTestByChannel(From + FVector(0.0, 0.0, PullLineLift), TargetOf(Player), ECC_Visibility, Params);
}

bool PickupPull::CanPull(const UWorld& World, const FVector& From, const APawn& Player, float Range, const AActor* Pickup)
{
	return ASoulMotePickup::DistanceToBody(From, Player) <= Range && HasClearLine(World, From, Player, Pickup);
}

// ---------------------------------------------------------------------------
// The glint and the trail
// ---------------------------------------------------------------------------

float PickupPull::GlintAt(float Age)
{
	if (Age < 0.f || Age >= GlintSeconds)
	{
		return 0.f;
	}
	if (Age < PullGlintRise)
	{
		return Age / PullGlintRise;
	}
	return FMath::Square(1.f - (Age - PullGlintRise) / (GlintSeconds - PullGlintRise));
}

void PickupPull::DrawGlint(UWorld& World, const FVector& Where, float Strength, float Size, const FLinearColor& Color)
{
	UBulletSubsystem* Bullets = World.GetSubsystem<UBulletSubsystem>();
	if (!Bullets || Strength <= 0.01f || Size <= 0.f)
	{
		return;
	}
	// The star's arms along the screen's across and up, so it reads as a star from wherever it's seen.
	FVector Across = FVector::RightVector;
	FVector Up = FVector::UpVector;
	const APlayerController* Controller = World.GetFirstPlayerController();
	if (Controller && Controller->PlayerCameraManager)
	{
		const FRotationMatrix View(Controller->PlayerCameraManager->GetCameraRotation());
		Across = View.GetUnitAxis(EAxis::Y);
		Up = View.GetUnitAxis(EAxis::Z);
	}
	FWeaponFX& Effects = Bullets->GetEffects();
	const float Half = Size * 0.5f;
	const float Glow = PullGlintGlow * FMath::Clamp(Strength, 0.f, 1.f);
	Effects.AddStreak(Where - Across * Half, Where + Across * Half, Size * 0.07f, Color, Glow);
	Effects.AddStreak(Where - Up * (Half * 0.8f), Where + Up * (Half * 0.8f), Size * 0.07f, Color, Glow);
	// Two short diagonals between the arms, and a soft heart.
	const FVector Rising = (Across + Up).GetSafeNormal() * (Half * 0.32f);
	const FVector Falling = (Across - Up).GetSafeNormal() * (Half * 0.32f);
	Effects.AddStreak(Where - Rising, Where + Rising, Size * 0.05f, Color, Glow * 0.55f);
	Effects.AddStreak(Where - Falling, Where + Falling, Size * 0.05f, Color, Glow * 0.55f);
	Effects.AddStreak(Where - Across * (Size * 0.08f), Where + Across * (Size * 0.08f), Size * 0.22f, Color, Glow * 0.3f);
}

void PickupPull::DrawTrail(UWorld& World, const FVector& From, const FVector& To, float Strength, const FLinearColor& Color)
{
	UBulletSubsystem* Bullets = World.GetSubsystem<UBulletSubsystem>();
	if (!Bullets || Strength <= 0.01f || FVector::DistSquared(From, To) < 1.0)
	{
		return;
	}
	Bullets->GetEffects().AddStreak(From, To, PullTrailWidth, Color, PullTrailGlow * FMath::Clamp(Strength, 0.f, 1.f));
}

// ---------------------------------------------------------------------------
// The flight
// ---------------------------------------------------------------------------

void FPickupFlight::Start(const FVector& From, const FVector& Target, float StartSpeed)
{
	Base = From;
	StartDistance = FMath::Max(static_cast<float>(FVector::Dist(From, Target)), 1.f);
	Speed = FMath::Max(StartSpeed, 0.f);
	Lift = FMath::Clamp(StartDistance * LiftShare, MinLift, MaxLift);
	Progress = 0.f;
	Age = 0.f;
	bStarted = true;
}

bool FPickupFlight::Step(float DeltaSeconds, const FVector& Target, float Acceleration, float MaxSpeed)
{
	const float Dt = FMath::Max(DeltaSeconds, 0.f);
	Age += Dt;
	Speed = FMath::Min(Speed + Acceleration * Dt, FMath::Max(MaxSpeed, Speed));
	const FVector ToTarget = Target - Base;
	const double Distance = ToTarget.Size();
	const double Travel = static_cast<double>(Speed) * Dt;
	if (Distance <= UE_KINDA_SMALL_NUMBER || Travel >= Distance)
	{
		Base = Target;
		Progress = 1.f;
		return true;
	}
	Base += ToTarget / Distance * Travel;
	// Never back: the player stepping away holds the arc where it is rather than lifting it again.
	const float Left = static_cast<float>(Distance - Travel);
	Progress = FMath::Clamp(FMath::Max(Progress, 1.f - Left / StartDistance), 0.f, 1.f);
	return false;
}

FVector FPickupFlight::GetLocation() const
{
	return Base + FVector(0.0, 0.0, Lift * FMath::Sin(UE_PI * Progress));
}

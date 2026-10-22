#include "Embers/Powers/EmberPowerBase.h"
#include "AI_Looter_Shooter.h"
#include "Combat/BulletSubsystem.h"
#include "Combat/GraveSaltBurst.h"
#include "Core/LooterCharacter.h"
#include "Creatures/CreatureBase.h"
#include "Embers/EmberDamageType.h"
#include "Embers/EmberDefinition.h"
#include "Embers/Powers/EmberVfx.h"
#include "Player/CameraShakeModifier.h"
#include "Player/PlayerViewComponent.h"
#include "Player/ViewKick.h"
#include "Weapons/WeaponFX.h"
#include "World/WorldQueries.h"
#include "CollisionQueryParams.h"
#include "Components/CapsuleComponent.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

// UEmberPowerBase: the twelve new powers' shared footing (the use's body and numbers, damage the contract's way, the aim and
// the creature in the sights, the lingering frame chain).

namespace
{
	/** A creature this far off the aim line still counts as in the sights however far it is (cm, on top of its body's radius). */
	constexpr float SightsSlack = 45.f;
}

// ---------------------------------------------------------------------------
// The aim and the world
// ---------------------------------------------------------------------------

void UEmberPowerBase::GetAim(const ACharacter& Character, FVector& OutStart, FVector& OutDirection)
{
	FRotator Rotation = FRotator::ZeroRotator;
	OutStart = FVector::ZeroVector;
	if (const UPlayerViewComponent* View = Character.FindComponentByClass<UPlayerViewComponent>())
	{
		View->GetAimViewPoint(OutStart, Rotation);
	}
	// No view (a bare body), or one not set up yet (it reads zero): the eyes, the way the body faces or the controller looks.
	if (OutStart.IsZero())
	{
		Character.GetActorEyesViewPoint(OutStart, Rotation);
	}
	OutDirection = Rotation.Vector().GetSafeNormal();
	if (OutDirection.IsNearlyZero())
	{
		OutDirection = Character.GetActorForwardVector();
	}
}

FVector UEmberPowerBase::HandPoint(const ACharacter& Character, float Right, float Down, float Forward)
{
	FVector Start;
	FVector Direction;
	GetAim(Character, Start, Direction);
	const UPlayerViewComponent* View = Character.FindComponentByClass<UPlayerViewComponent>();
	const FVector Flat = FVector(Direction.X, Direction.Y, 0.0).GetSafeNormal();
	const FVector Side = FVector::CrossProduct(FVector::UpVector, Flat.IsNearlyZero() ? Character.GetActorForwardVector() : Flat).GetSafeNormal();
	if (View && View->IsFirstPerson())
	{
		// Under the eye, a little to the right, out in front: where a thrown hand would be seen to let go.
		const FVector Up = FVector::CrossProduct(Direction, Side).GetSafeNormal();
		return Start + Direction * Forward + Side * Right - Up * Down;
	}
	// Seen from outside: the chest, out in front the way the player aims.
	const float HalfHeight = Character.GetCapsuleComponent() ? Character.GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 90.f;
	return Character.GetActorLocation() + FVector(0.0, 0.0, HalfHeight * 0.45f) + Direction * Forward + Side * Right;
}

FVector UEmberPowerBase::FeetOf(const ACharacter& Character)
{
	const float HalfHeight = Character.GetCapsuleComponent() ? Character.GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 90.f;
	return Character.GetActorLocation() - FVector(0.0, 0.0, HalfHeight);
}

bool UEmberPowerBase::CanSee(const UWorld& World, const FVector& From, const FVector& To, const AActor* Ignored)
{
	const FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(&World, TEXT("EmberSight"), Ignored);
	FHitResult Wall;
	return !World.LineTraceSingleByObjectType(Wall, From, To, FCollisionObjectQueryParams(ECC_WorldStatic), Params);
}

bool UEmberPowerBase::FindGround(const UWorld& World, const FVector& Point, float Above, float Below, FVector& OutGround, FVector* OutNormal,
	const AActor* Ignored)
{
	const FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(&World, TEXT("EmberGround"), Ignored);
	FHitResult Ground;
	if (!World.LineTraceSingleByObjectType(Ground, Point + FVector(0.0, 0.0, Above), Point - FVector(0.0, 0.0, Below),
			FCollisionObjectQueryParams(ECC_WorldStatic), Params)
		|| Ground.bStartPenetrating)
	{
		return false;
	}
	OutGround = Ground.ImpactPoint;
	if (OutNormal)
	{
		*OutNormal = Ground.ImpactNormal;
	}
	return true;
}

ACreatureBase* UEmberPowerBase::FindInSights(const ACharacter& Character, float Range, float ConeDegrees)
{
	UWorld* World = Character.GetWorld();
	if (!World || Range <= 0.f)
	{
		return nullptr;
	}
	FVector Eye;
	FVector Aim;
	GetAim(Character, Eye, Aim);
	const float Tan = FMath::Tan(FMath::DegreesToRadians(FMath::Clamp(ConeDegrees, 0.5f, 60.f)));
	ACreatureBase* Best = nullptr;
	float BestScore = TNumericLimits<float>::Max();
	for (TActorIterator<ACreatureBase> It(World); It; ++It)
	{
		ACreatureBase* Creature = *It;
		if (!Creature || Creature->IsDead() || Creature->IsHidden())
		{
			continue;
		}
		const FVector Middle = Creature->GetActorLocation();
		const FVector To = Middle - Eye;
		const float Along = static_cast<float>(FVector::DotProduct(To, Aim));
		if (Along <= 0.f || Along > Range)
		{
			continue;
		}
		const float Off = static_cast<float>((To - Aim * Along).Size());
		const float Radius = Creature->GetCapsuleComponent() ? Creature->GetCapsuleComponent()->GetScaledCapsuleRadius() : 40.f;
		// Inside the cone, or within its body's width (plus a little) of the line: a near one counts by its size, a far one by the angle.
		const float Allowed = FMath::Max(Along * Tan, Radius + SightsSlack);
		if (Off > Allowed)
		{
			continue;
		}
		// Its middle or its top in sight.
		const float HalfHeight = Creature->GetCapsuleComponent() ? Creature->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 50.f;
		if (!CanSee(*World, Eye, Middle, Creature) && !CanSee(*World, Eye, Middle + FVector(0.0, 0.0, HalfHeight * 0.8f), Creature))
		{
			continue;
		}
		// Nearest the line first (as a share of what's allowed), a little for the distance.
		const float Score = Off / Allowed + 0.15f * Along / Range;
		if (Score < BestScore)
		{
			BestScore = Score;
			Best = Creature;
		}
	}
	return Best;
}

bool UEmberPowerBase::IsBoss(const ACreatureBase& Creature)
{
	return GraveSaltBurst::IsBoss(&Creature);
}

void UEmberPowerBase::BodyOf(const ACreatureBase& Creature, FVector& OutMiddle, float& OutRadius, float& OutHalfHeight)
{
	OutMiddle = Creature.GetActorLocation();
	const UCapsuleComponent* Capsule = Creature.GetCapsuleComponent();
	OutRadius = Capsule ? Capsule->GetScaledCapsuleRadius() : 0.f;
	OutHalfHeight = Capsule ? Capsule->GetScaledCapsuleHalfHeight() : 0.f;
}

float UEmberPowerBase::DistanceToBody(const ACreatureBase& Creature, const FVector& A, const FVector& B, FVector& OutOnLine, FVector& OutOnBody)
{
	FVector Middle;
	float Radius = 0.f;
	float HalfHeight = 0.f;
	BodyOf(Creature, Middle, Radius, HalfHeight);
	// The capsule as its axis (between the centres of its two end spheres) and its radius.
	const FVector Reach(0.0, 0.0, FMath::Max(HalfHeight - Radius, 0.f));
	FMath::SegmentDistToSegmentSafe(A, B, Middle - Reach, Middle + Reach, OutOnLine, OutOnBody);
	return static_cast<float>(FVector::Dist(OutOnLine, OutOnBody)) - Radius;
}

float UEmberPowerBase::ScaledDamage(const FEmberContext& Context, float Fallback)
{
	const UEmberDefinition* Def = Context.Definition;
	return (Def && Def->Damage > 0.f ? Def->Damage : Fallback) * FMath::Max(Context.LevelScale, 0.f);
}

FWeaponFX* UEmberPowerBase::EffectsIn(UWorld* World)
{
	UBulletSubsystem* Bullets = World ? World->GetSubsystem<UBulletSubsystem>() : nullptr;
	if (!Bullets)
	{
		return nullptr;
	}
	FWeaponFX& FX = Bullets->GetEffects();
	FX.Initialize(World);
	return &FX;
}

// ---------------------------------------------------------------------------
// The use
// ---------------------------------------------------------------------------

bool UEmberPowerBase::BeginUse(const FEmberContext& Context)
{
	ACharacter* Character = Context.Character;
	if (!Character)
	{
		return false;
	}
	Body = Character;
	Instigator = Character->GetController();
	if (Context.Definition)
	{
		Color = Context.Definition->Color;
	}
	LevelScale = FMath::Max(Context.LevelScale, 0.f);
	Dealt.Reset();
	return true;
}

float UEmberPowerBase::Hurt(AActor& Target, float Damage, const FVector& Where, const FVector& Along)
{
	if (Damage <= 0.f)
	{
		return 0.f;
	}
	FDealt& Hit = Dealt.AddDefaulted_GetRef();
	Hit.Target = &Target;
	Hit.Damage = Damage;
	// The player's (the number, the hit's experience) and no gun's: no causer, so no notch (UEmberDamageType's contract).
	const FVector Way = Along.GetSafeNormal();
	UPrimitiveComponent* Root = Cast<UPrimitiveComponent>(Target.GetRootComponent());
	FHitResult Struck(&Target, Root, Where, -Way);
	UGameplayStatics::ApplyPointDamage(&Target, Damage, Way, Struck, Instigator.Get(), nullptr, UEmberDamageType::StaticClass());
	// The blow shows on the body it lands on: what it spills, as a gun's hit of its weight, and a pop of the power's colour
	// (the VFX pass: before it, a power's damage left nothing on the body but its number).
	if (FWeaponFX* FX = EffectsIn(Target.GetWorld()))
	{
		EmberVfx::BodyHit(*FX, Target, Where, Way, Damage, Color);
	}
	return Damage;
}

UWorld* UEmberPowerBase::GetBodyWorld() const
{
	const ACharacter* Character = Body.Get();
	return Character ? Character->GetWorld() : nullptr;
}

APlayerController* UEmberPowerBase::GetPlayer() const
{
	const ACharacter* Character = Body.Get();
	return Character ? Cast<APlayerController>(Character->GetController()) : nullptr;
}

void UEmberPowerBase::Kick(const FViewKick& ViewKick) const
{
	if (APlayerController* Player = GetPlayer())
	{
		UCameraShakeModifier::Kick(Player, ViewKick);
	}
}

float UEmberPowerBase::PlayerDeltaSeconds() const
{
	const ACharacter* Character = Body.Get();
	const UWorld* World = Character ? Character->GetWorld() : nullptr;
	return World ? World->GetDeltaSeconds() * FMath::Max(Character->CustomTimeDilation, 0.f) : 0.f;
}

// ---------------------------------------------------------------------------
// What lingers
// ---------------------------------------------------------------------------

void UEmberPowerBase::StartLinger()
{
	UWorld* World = GetBodyWorld();
	if (bPumping || !World || World->bIsTearingDown)
	{
		return;
	}
	bPumping = true;
	World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &UEmberPowerBase::Pump));
}

void UEmberPowerBase::Pump()
{
	bPumping = false;
	UWorld* World = GetBodyWorld();
	if (!World || World->bIsTearingDown)
	{
		return;
	}
	if (Linger(PlayerDeltaSeconds()))
	{
		StartLinger();
	}
}

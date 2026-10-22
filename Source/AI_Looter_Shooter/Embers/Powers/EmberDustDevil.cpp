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

// UEmberDustDevil (Ellis's Dust Devil): the use and the dash frame by frame. The path is EmberDustDevilPath.cpp, the creatures in its way and
// the show (speed lines, dust, the view) EmberDustDevilLook.cpp (split 2026-10-22 to keep each under the size guide).

namespace
{
	const FName FlingMinName(TEXT("FlingMin"));
	const FName FlingMaxName(TEXT("FlingMax"));
	const FName DizzyName(TEXT("Dizzy"));
	const FName FovKickName(TEXT("FovKick"));

	float Ease(float U)
	{
		const float X = FMath::Clamp(U, 0.f, 1.f);
		return X * X * (3.f - 2.f * X);
	}

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

	APlayerController* PlayerOf(const ACharacter* Character)
	{
		return Character ? Cast<APlayerController>(Character->GetController()) : nullptr;
	}
}

// ---------------------------------------------------------------------------
// Using it
// ---------------------------------------------------------------------------

EEmberDenied UEmberDustDevil::CanActivate(const FEmberContext& Context) const
{
	const ACharacter* Character = Context.Character;
	if (!Character)
	{
		return EEmberDenied::Refused;
	}
	if (bDashing)
	{
		return EEmberDenied::Active;
	}
	const UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
	// A mantle or vault has the movement in its own mode: the body is spoken for.
	if (!Movement || Movement->MovementMode == MOVE_None || Movement->MovementMode == MOVE_Custom)
	{
		return EEmberDenied::Busy;
	}
	const float Range = Context.Definition && Context.Definition->Range > 0.f ? Context.Definition->Range : 900.f;
	// Facing a wall or the edge of a drop: no room to dash, so the press does nothing (the socket shakes).
	return PlanDash(*Character, DashDirection(*Character), Range).Length >= MinDash ? EEmberDenied::None : EEmberDenied::Refused;
}

void UEmberDustDevil::Activate(const FEmberContext& Context)
{
	ACharacter* Character = Context.Character;
	UWorld* World = Character ? Character->GetWorld() : nullptr;
	if (!Character || !World)
	{
		return;
	}
	// The definition's numbers (the asset holds them), with the design's as a fallback.
	const UEmberDefinition* Def = Context.Definition;
	Damage = (Def && Def->Damage > 0.f ? Def->Damage : 15.f) * FMath::Max(Context.LevelScale, 0.f);
	const float Range = Def && Def->Range > 0.f ? Def->Range : 900.f;
	Radius = Def && Def->Radius > 0.f ? Def->Radius : 260.f;
	FlingMin = Def ? Def->Extra(FlingMinName, 400.f) : 400.f;
	FlingMax = FMath::Max(FlingMin, Def ? Def->Extra(FlingMaxName, 500.f) : 500.f);
	Dizzy = Def ? Def->Extra(DizzyName, 1.f) : 1.f;
	FovKick = Def ? Def->Extra(FovKickName, 14.f) : 14.f;
	bComponentTicks = Def && Def->DurationSeconds > 0.f;
	DashSeconds = bComponentTicks ? Def->DurationSeconds : DefaultSeconds;
	if (Def)
	{
		Color = Def->Color;
	}

	Body = Character;
	Instigator = Character->GetController();
	Path = PlanDash(*Character, DashDirection(*Character), Range);
	Struck.Reset();
	SpeedLines.Reset();
	LinesOwed = 0.f;
	DashTime = 0.f;
	Travelled = 0.f;
	TrailLaid = 0.f;
	SettleTime = -1.f;
	bDashing = true;

	// Who may be in its way: every living creature near enough to the path to be reached.
	Candidates.Reset();
	const FVector Start = Character->GetActorLocation();
	for (TActorIterator<ACreatureBase> It(World); It; ++It)
	{
		if (!It->IsDead() && FVector::Dist(It->GetActorLocation(), Start) <= Range + Radius + 400.f)
		{
			Candidates.Add(*It);
		}
	}

	// Nothing hurts the player through it (unless something else already made them so: theirs to undo).
	UHealthComponent* Life = Character->FindComponentByClass<UHealthComponent>();
	Health = Life;
	bMadeInvulnerable = Life && !Life->bInvulnerable;
	if (bMadeInvulnerable)
	{
		Life->bInvulnerable = true;
	}
	// The movement stands aside while the body rides the path.
	if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
		Movement->SetMovementMode(MOVE_None);
	}

	// Felt at once: the whoosh, the dust thrown up where it leaves, a lean into the way it goes.
	LooterSound::Play2D(Character, LooterSoundCue::Ember::DustDevilUse);
	if (Path.bOnGround)
	{
		SpawnDustRing(FeetAt(Start), 1.f, 16);
		// (The VFX pass) the gust's ring of wind racing out over the ground from where it leaves.
		if (FWeaponFX* Effects = EffectsIn(World))
		{
			EmberVfx::Activation(*Effects, FeetAt(Start), Color, 0.8f);
		}
	}
	if (APlayerController* Player = PlayerOf(Character))
	{
		const FVector ViewRight = FRotator(0.f, Player->GetControlRotation().Yaw, 0.f).RotateVector(FVector::RightVector);
		FViewKick Lean;
		Lean.Pitch = -0.5f;
		Lean.Roll = 1.4f * static_cast<float>(FVector::DotProduct(Path.Direction, ViewRight));
		Lean.Frequency = 5.f;
		Lean.Damping = 0.7f;
		UCameraShakeModifier::Kick(Player, Lean);
	}
	UE_LOG(LogLooter, Log, TEXT("Dust Devil: %.0f cm dash (%s), %d creatures near"), Path.Length, Path.bOnGround ? TEXT("on the ground") : TEXT("in the air"),
		Candidates.Num());

	// An instant definition isn't ticked by the component: the dash runs on its own timer.
	if (!bComponentTicks)
	{
		SchedulePump();
	}
}

void UEmberDustDevil::Tick(const FEmberContext& Context, float DeltaSeconds)
{
	if (bComponentTicks)
	{
		StepDash(DeltaSeconds);
	}
}

void UEmberDustDevil::End(const FEmberContext& Context, bool bInterrupted)
{
	if (!bDashing)
	{
		return;
	}
	if (bInterrupted)
	{
		// Death, a scene or the level ending: it stops where it is, and the world is left as it was.
		FinishDash(true);
	}
	else if (bComponentTicks)
	{
		// Its time is up: whatever's left of the path, now.
		StepDash(DashSeconds);
	}
	// An instant definition's End comes the moment it's used: its timer carries the dash on.
}

// ---------------------------------------------------------------------------
// The dash, frame by frame
// ---------------------------------------------------------------------------

void UEmberDustDevil::StepDash(float DeltaSeconds)
{
	if (!bDashing)
	{
		return;
	}
	ACharacter* Character = Body.Get();
	if (!Character)
	{
		FinishDash(true);
		return;
	}
	DashTime += FMath::Max(DeltaSeconds, 0.f);
	const float U = DashSeconds > 0.f ? FMath::Clamp(DashTime / DashSeconds, 0.f, 1.f) : 1.f;
	// Fastest as it leaves (a burst), still quick as it arrives: an even pace blended with an ease-out.
	const float Share = 0.6f * U + 0.4f * (1.f - FMath::Square(1.f - U));
	const float Distance = Path.Length * Share;
	const FVector Was = Character->GetActorLocation();
	const FVector Middle = Path.At(Distance);
	Character->SetActorLocation(Middle, false, nullptr, ETeleportType::TeleportPhysics);
	if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
	{
		// For what reads the body's speed (its animation, its footfalls): the dash's own.
		Movement->Velocity = DeltaSeconds > 0.f ? (Middle - Was) / DeltaSeconds : FVector::ZeroVector;
	}
	StrikeAlong(Travelled, Distance);
	TrailDust(Travelled, Distance);
	Travelled = Distance;
	SetViewWidening(FovKick * Ease(DashTime / WidenSeconds));
	TickSpeedLines(DeltaSeconds);
	if (U >= 1.f)
	{
		FinishDash(false);
	}
}

void UEmberDustDevil::FinishDash(bool bAborted)
{
	if (!bDashing)
	{
		return;
	}
	bDashing = false;
	ACharacter* Character = Body.Get();
	if (UHealthComponent* Life = Health.Get(); Life && bMadeInvulnerable)
	{
		Life->bInvulnerable = false;
	}
	bMadeInvulnerable = false;
	if (Character)
	{
		if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
		{
			// Walking on with a little of its speed (or falling, from a dash in the air).
			Movement->SetMovementMode(Path.bOnGround ? MOVE_Walking : MOVE_Falling);
			Movement->Velocity = bAborted ? FVector::ZeroVector : Path.Direction * ExitSpeed;
		}
		if (!bAborted)
		{
			if (Path.bOnGround)
			{
				SpawnDustRing(FeetAt(Character->GetActorLocation()), 0.65f, 9);
				if (FWeaponFX* Effects = EffectsIn(Character->GetWorld()))
				{
					EmberVfx::Activation(*Effects, FeetAt(Character->GetActorLocation()), Color, 0.45f);
				}
			}
			if (APlayerController* Player = PlayerOf(Character))
			{
				FViewKick Arrive;
				Arrive.Pitch = -0.35f;
				Arrive.FieldOfView = -0.01f;
				Arrive.Frequency = 9.f;
				Arrive.Damping = 0.6f;
				UCameraShakeModifier::Kick(Player, Arrive);
			}
		}
	}
	// The view settles back and the lines fly on, on the dash's own timer.
	SettleTime = 0.f;
	SchedulePump();
	UE_LOG(LogLooter, Verbose, TEXT("Dust Devil: dash %s after %.0f cm, %d struck"), bAborted ? TEXT("cut short") : TEXT("done"), Travelled, Struck.Num());
}

void UEmberDustDevil::SchedulePump()
{
	UWorld* World = Body.IsValid() ? Body->GetWorld() : nullptr;
	if (bPumping || !World)
	{
		return;
	}
	bPumping = true;
	World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &UEmberDustDevil::Pump));
}

void UEmberDustDevil::Pump()
{
	bPumping = false;
	ACharacter* Character = Body.Get();
	UWorld* World = Character ? Character->GetWorld() : nullptr;
	if (!World)
	{
		return;
	}
	// On the player's own clock (Sundown's world crawl doesn't slow the player's dash).
	const float DeltaSeconds = World->GetDeltaSeconds() * FMath::Max(Character->CustomTimeDilation, 0.f);
	if (bDashing)
	{
		if (!bComponentTicks)
		{
			StepDash(DeltaSeconds);
		}
	}
	else
	{
		if (SettleTime >= 0.f)
		{
			SettleTime += DeltaSeconds;
			SetViewWidening(FovKick * (1.f - Ease(SettleTime / SettleSeconds)));
			if (SettleTime >= SettleSeconds)
			{
				SetViewWidening(0.f);
				SettleTime = -1.f;
			}
		}
		TickSpeedLines(DeltaSeconds);
	}
	if (bDashing || SettleTime >= 0.f || !SpeedLines.IsEmpty())
	{
		SchedulePump();
	}
}

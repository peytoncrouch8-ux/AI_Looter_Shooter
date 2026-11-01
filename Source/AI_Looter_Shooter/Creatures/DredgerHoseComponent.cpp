// UDredgerHoseComponent: the soul-hose's life (thrown, latched, broken, whipped back) and what it does to its victim (the
// pull, the drain, the feedback). Its look: DredgerHoseComponentDraw.cpp.

#include "Creatures/DredgerHoseComponent.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Audio/LooterSoundCues.h"
#include "Combat/BulletSubsystem.h"
#include "Combat/EnemyProjectileSubsystem.h"
#include "Combat/HealthComponent.h"
#include "Combat/LooterDamageTypes.h"
#include "Combat/MovementSlowComponent.h"
#include "Creatures/DredgerHoseMan.h"
#include "Creatures/DredgerLook.h"
#include "Creatures/DredgerRules.h"
#include "Creatures/FightDirectorSubsystem.h"
#include "Creatures/FightEvent.h"
#include "Player/PlayerRattleSubsystem.h"
#include "Weapons/WeaponFX.h"
#include "World/WorldQueries.h"
#include "Components/AudioComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	/** Every hose in play, for GetDrainOn (the HUD asks a few times a second). */
	TArray<TWeakObjectPtr<UDredgerHoseComponent>> LiveHoses;

	/** The thrown tip: how near it must pass the victim's capsule (cm), how far past the latch range it may fly (share). */
	constexpr float TipRadius = 25.f;
	constexpr float ReachShare = 1.35f;
	/** It seeks its victim a little in flight (the aim's pull toward their chest, a second). */
	constexpr float SeekRate = 3.f;
	/** The drain's damage ticks this often (s); the feedback renews this often (s); the slow while latched (share kept, s). */
	constexpr float DrainTick = 0.35f;
	constexpr float TouchEvery = 0.9f;
	constexpr float LatchedSlow = 0.82f;
	constexpr float LatchedSlowSeconds = 1.f;
	/** The latch's yank toward him (cm/s along, up). */
	constexpr float YankAlong = 220.f;
	constexpr float YankUp = 60.f;
	/** The snap's whip back (s). */
	constexpr float SnapSeconds = 0.25f;
	/** The drain's shown strength eases in and out this fast (a second). */
	constexpr float DrainShowIn = 3.f;
	constexpr float DrainShowOut = 2.f;

	FWeaponFX* HoseEffects(UWorld* World)
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
}

UDredgerHoseComponent::UDredgerHoseComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
}

void UDredgerHoseComponent::BeginPlay()
{
	Super::BeginPlay();
	LiveHoses.Add(this);
	// After the hose-man's own frame, so the nozzle is where his hands put it this frame.
	if (AActor* Owner = GetOwner())
	{
		PrimaryComponentTick.AddPrerequisite(Owner, Owner->PrimaryActorTick);
	}
	EnsureLine();
}

void UDredgerHoseComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopDrainSound();
	LiveHoses.RemoveAll([this](const TWeakObjectPtr<UDredgerHoseComponent>& Each) { return !Each.IsValid() || Each.Get() == this; });
	Super::EndPlay(EndPlayReason);
}

void UDredgerHoseComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	Advance(DeltaTime);
}

ADredgerHoseMan* UDredgerHoseComponent::GetHoseMan() const
{
	return Cast<ADredgerHoseMan>(GetOwner());
}

float UDredgerHoseComponent::GetDrainOn(const APawn* Pawn)
{
	float Strongest = 0.f;
	for (const TWeakObjectPtr<UDredgerHoseComponent>& Each : LiveHoses)
	{
		const UDredgerHoseComponent* Hose = Each.Get();
		if (Hose && Pawn && Hose->Victim.Get() == Pawn)
		{
			Strongest = FMath::Max(Strongest, Hose->DrainShown);
		}
	}
	return Strongest;
}

// ---------------------------------------------------------------------------
// Its life
// ---------------------------------------------------------------------------

void UDredgerHoseComponent::Advance(float DeltaSeconds)
{
	Clock += DeltaSeconds;
	switch (State)
	{
	case EDredgerHoseState::Flying:
		StepFlying(DeltaSeconds);
		break;
	case EDredgerHoseState::Latched:
		StepLatched(DeltaSeconds);
		break;
	case EDredgerHoseState::Snapping:
		StepSnapping(DeltaSeconds);
		break;
	case EDredgerHoseState::Stowed:
	default:
		Tug = 0.f;
		break;
	}
	const float Wanted = IsLatched() ? 1.f : 0.f;
	DrainShown = FMath::FInterpConstantTo(DrainShown, Wanted, DeltaSeconds, Wanted > DrainShown ? DrainShowIn : DrainShowOut);
	if (State != EDredgerHoseState::Latched && DrainShown <= 0.f && State == EDredgerHoseState::Stowed)
	{
		Victim.Reset();
	}
	Draw(DeltaSeconds);
}

bool UDredgerHoseComponent::Fire(APawn* InVictim)
{
	const ADredgerHoseMan* HoseMan = GetHoseMan();
	if (!HoseMan || HoseMan->IsDead() || HoseMan->IsTankBurst() || !InVictim || IsOut() || bTorn)
	{
		return false;
	}
	const FDredgerTuning Tuning = FDredgerTuning::Live();
	Victim = InVictim;
	Tip = HoseMan->GetNozzleMouth();
	// At their chest, led a little: it seeks them in flight, but a step aside at the throw still makes it miss.
	Aim = VictimChest() + InVictim->GetVelocity() * 0.15f;
	Flown = 0.f;
	Reach = Tuning.LatchRange * ReachShare;
	LastBreak = EDredgerHoseBreak::None;
	State = EDredgerHoseState::Flying;
	return true;
}

FVector UDredgerHoseComponent::VictimChest() const
{
	const APawn* Pawn = Victim.Get();
	return Pawn ? Pawn->GetActorLocation() + FVector(0.f, 0.f, DredgerRules::ChestRise) : Tip;
}

bool UDredgerHoseComponent::IsCut(const FVector& Point) const
{
	const ADredgerHoseMan* HoseMan = GetHoseMan();
	const UWorld* World = GetWorld();
	if (!HoseMan || !World)
	{
		return false;
	}
	FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(World, TEXT("DredgerHose"), HoseMan, false);
	if (const APawn* Pawn = Victim.Get())
	{
		Params.AddIgnoredActor(Pawn);
	}
	return World->LineTraceTestByObjectType(HoseMan->GetNozzleMouth(), Point, FCollisionObjectQueryParams(ECC_WorldStatic), Params);
}

void UDredgerHoseComponent::StepFlying(float DeltaSeconds)
{
	const ADredgerHoseMan* HoseMan = GetHoseMan();
	APawn* Pawn = Victim.Get();
	if (!HoseMan || HoseMan->IsDead() || !Pawn)
	{
		Break(EDredgerHoseBreak::OwnerDown);
		return;
	}
	const FDredgerTuning Tuning = FDredgerTuning::Live();
	const FVector From = Tip;
	Aim = FMath::VInterpTo(Aim, VictimChest(), DeltaSeconds, SeekRate);
	const FVector Way = (Aim - Tip).GetSafeNormal();
	const float Step = Tuning.LineSpeed * DeltaSeconds;
	Tip += Way * Step;
	Flown += Step;

	float Radius = 0.f;
	float HalfHeight = 0.f;
	Pawn->GetSimpleCollisionCylinder(Radius, HalfHeight);
	float When = 0.f;
	if (UEnemyProjectileSubsystem::SweepHitsCapsule(From, Tip, TipRadius, Pawn->GetActorLocation(), Radius, HalfHeight, When))
	{
		if (IsCut(VictimChest()))
		{
			Break(EDredgerHoseBreak::Blocked);
		}
		else
		{
			Latch();
		}
		return;
	}
	// The world in its way, its reach flown, or past where it was thrown (a step aside at the throw): a miss.
	if (IsCut(Tip) || Flown >= Reach || FVector::DotProduct(Aim - Tip, Way) < 0.f)
	{
		Break(EDredgerHoseBreak::Missed);
	}
}

void UDredgerHoseComponent::Latch()
{
	ADredgerHoseMan* HoseMan = GetHoseMan();
	APawn* Pawn = Victim.Get();
	State = EDredgerHoseState::Latched;
	LatchedFor = 0.f;
	BlockedFor = 0.f;
	DrainOwed = 0.f;
	DrainClock = 0.f;
	TouchClock = 0.f;
	Tip = VictimChest();
	if (!HoseMan || !Pawn)
	{
		return;
	}
	// The hook going in: its clamp's bite on them, the drain's hiss on them from now on, a yank toward him, the soul tearing.
	LooterSound::PlayAt(this, LooterSoundCue::DredgerHoseLatch, Tip);
	StopDrainSound();
	DrainSound = LooterSound::Start(this, LooterSoundCue::DredgerHoseDrain, Pawn->GetRootComponent());
	const FVector ToHim = (HoseMan->GetActorLocation() - Pawn->GetActorLocation()).GetSafeNormal2D();
	if (ACharacter* Character = Cast<ACharacter>(Pawn))
	{
		Character->LaunchCharacter(ToHim * YankAlong + FVector(0.f, 0.f, YankUp), false, false);
	}
	if (FWeaponFX* Effects = HoseEffects(GetWorld()))
	{
		Effects->SpawnSoulTatters(Tip, ToHim, DredgerLook::SoulCyan, 40.f, 5);
	}
	TouchVictim(true);
	HoseMan->Bark(LooterSoundCue::DredgerBark, 0.7f);
}

void UDredgerHoseComponent::StepLatched(float DeltaSeconds)
{
	ADredgerHoseMan* HoseMan = GetHoseMan();
	APawn* Pawn = Victim.Get();
	const FDredgerTuning Tuning = FDredgerTuning::Live();
	LatchedFor += DeltaSeconds;
	Tip = VictimChest();

	FDredgerHoseFacts Facts;
	Facts.LatchedFor = LatchedFor;
	Facts.bOwnerDown = !HoseMan || HoseMan->IsDead() || HoseMan->IsPassive();
	Facts.bTankBurst = HoseMan && HoseMan->IsTankBurst();
	Facts.bOwnerHeld = HoseMan && HoseMan->IsHeldByHit();
	const UHealthComponent* Life = Pawn ? Pawn->FindComponentByClass<UHealthComponent>() : nullptr;
	Facts.bVictimGone = !Pawn || (Life && Life->IsDead()) || (HoseMan && !HoseMan->CanDrain(Pawn));
	if (HoseMan && Pawn)
	{
		Facts.Distance = static_cast<float>(FVector::Dist2D(HoseMan->GetActorLocation(), Pawn->GetActorLocation()));
		BlockedFor = IsCut(Tip) ? BlockedFor + DeltaSeconds : 0.f;
		Facts.BlockedFor = BlockedFor;
	}
	const EDredgerHoseBreak Why = DredgerRules::HoseBreak(Facts, Tuning);
	if (Why != EDredgerHoseBreak::None)
	{
		Break(Why);
		return;
	}

	// The drain: his rate, dealt in small ticks as damage (the ward takes it first; it can kill), counted either way.
	DrainOwed += HoseMan->GetDrainPerSecond() * DeltaSeconds;
	DrainClock += DeltaSeconds;
	if (DrainClock >= DrainTick)
	{
		DrainClock -= DrainTick;
		const float Amount = DrainOwed;
		DrainOwed = 0.f;
		DrainDealt += Amount;
		UGameplayStatics::ApplyDamage(Pawn, Amount, HoseMan->GetController(), HoseMan, UCreatureAttackDamageType::StaticClass());
		UFightDirectorSubsystem::Note(*HoseMan, EFightEvent::AttackLanded, Amount);
	}

	// The pull: a share of their own walking input toward him (they walk away slower, and drift in standing still).
	// Only a player is pulled: a story person latched (Delia on her porch, who stands where she's put) is drained, never moved,
	// and an AI pawn's input would pile up unspent.
	const FVector ToHim = (HoseMan->GetActorLocation() - Pawn->GetActorLocation()).GetSafeNormal2D();
	if (Pawn->IsPlayerControlled())
	{
		Pawn->AddMovementInput(ToHim, DredgerRules::PullInput(Facts.Distance, Tuning));
	}
	// His body feels it: the line tugs him as they strain at it, harder the farther they get.
	Tug = FMath::Sin(Clock * 7.f) * 2.5f + Facts.Distance / FMath::Max(Tuning.BreakRange, 1.f) * 4.f;

	TouchClock += DeltaSeconds;
	if (TouchClock >= TouchEvery)
	{
		TouchClock = 0.f;
		TouchVictim(false);
	}
}

void UDredgerHoseComponent::TouchVictim(bool bFirst)
{
	APawn* Pawn = Victim.Get();
	const ADredgerHoseMan* HoseMan = GetHoseMan();
	if (!Pawn || !HoseMan)
	{
		return;
	}
	const FDredgerTuning Tuning = FDredgerTuning::Live();
	// The latch rattles them (a sway, a shake, the frost, the arc to him); while it holds, the frost stays on the edges.
	if (UPlayerRattleSubsystem* Rattles = UPlayerRattleSubsystem::Get(this))
	{
		if (bFirst || Tuning.FrostEdge != 0)
		{
			FPlayerRattle How;
			How.Strength = bFirst ? 0.7f : 0.4f;
			How.bSway = bFirst;
			How.SwayDegrees = 0.8f;
			How.bSlow = false;
			How.Shake = bFirst ? 0.35f : 0.08f;
			How.ShakeSeconds = 0.4f;
			How.FrostSeconds = bFirst ? 1.f : 1.25f;
			Rattles->Rattle(Pawn, HoseMan->GetNozzleMouth(), How);
		}
	}
	// The slow is a player's (a story person latched stands where she's put).
	ACharacter* Character = Pawn->IsPlayerControlled() ? Cast<ACharacter>(Pawn) : nullptr;
	if (Character)
	{
		UMovementSlowComponent::Apply(Character, LatchedSlow, LatchedSlowSeconds);
	}
}

void UDredgerHoseComponent::Break(EDredgerHoseBreak Why, bool bQuiet)
{
	if (State == EDredgerHoseState::Stowed || State == EDredgerHoseState::Snapping)
	{
		if (bQuiet)
		{
			State = EDredgerHoseState::Stowed;
		}
		return;
	}
	const bool bWasLatched = State == EDredgerHoseState::Latched;
	LastBreak = Why;
	StopDrainSound();
	SnapFrom = bWasLatched ? VictimChest() : Tip;
	SnapShare = 0.f;
	State = bQuiet ? EDredgerHoseState::Stowed : EDredgerHoseState::Snapping;
	if (!bQuiet && bWasLatched)
	{
		// Broken off them: the snap, a spit of soul-light where it tore free.
		LooterSound::PlayAt(this, LooterSoundCue::DredgerHoseSnap, SnapFrom);
		if (FWeaponFX* Effects = HoseEffects(GetWorld()))
		{
			Effects->SpawnSoulTatters(SnapFrom, FVector::UpVector, DredgerLook::SoulCyan, 30.f, 4);
			Effects->SpawnFlash(SnapFrom, FVector::UpVector, 0.4f);
		}
	}
	UE_LOG(LogLooter, Verbose, TEXT("%s's hose let go (%d) after %.1f s, %.1f drained."), *GetNameSafe(GetOwner()), static_cast<int32>(Why),
		LatchedFor, DrainDealt);
	if (ADredgerHoseMan* HoseMan = GetHoseMan())
	{
		HoseMan->NoteHoseLetGo();
	}
}

void UDredgerHoseComponent::StepSnapping(float DeltaSeconds)
{
	const ADredgerHoseMan* HoseMan = GetHoseMan();
	SnapShare = FMath::Min(1.f, SnapShare + DeltaSeconds / SnapSeconds);
	const FVector Home = HoseMan ? HoseMan->GetNozzleMouth() : SnapFrom;
	Tip = FMath::Lerp(SnapFrom, Home, FMath::InterpEaseIn(0.f, 1.f, SnapShare, 2.f));
	Tug = 0.f;
	if (SnapShare >= 1.f)
	{
		State = EDredgerHoseState::Stowed;
	}
}

void UDredgerHoseComponent::StopDrainSound()
{
	LooterSound::Stop(DrainSound.Get(), 0.25f);
	DrainSound.Reset();
}

void UDredgerHoseComponent::SetTorn(bool bInTorn)
{
	bTorn = bInTorn;
	if (bTorn && IsOut())
	{
		Break(EDredgerHoseBreak::TankBurst);
	}
}

// UAbelCorruptionComponent: the smog tendrils (thrown from the black lantern after its flare, latched like a hose-man's
// line: a drain and a slow pull; broken at 14 m, by a staggering hit, or by a shot on the black lantern as it flares).
// Their look is drawn in AbelCorruptionLines.cpp.

#include "Bosses/AbelCorruption.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Audio/LooterSoundCues.h"
#include "Bosses/AbelKeeper.h"
#include "Bosses/BossComponent.h"
#include "Combat/BulletSubsystem.h"
#include "Combat/EnemyProjectileSubsystem.h"
#include "Combat/HealthComponent.h"
#include "Combat/LooterDamageTypes.h"
#include "Combat/MovementSlowComponent.h"
#include "Creatures/DredgerLook.h"
#include "Creatures/DredgerRules.h"
#include "Player/PlayerRattleSubsystem.h"
#include "Weapons/WeaponFX.h"
#include "HAL/IConsoleManager.h"
#include "Components/AudioComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	TAutoConsoleVariable<int32> CVarAbelTendrils(TEXT("Looter.Abel.Tendrils"), 1,
		TEXT("Abel's smog tendrils: 1 he throws them (the fight), 0 never (a test about something else; a capture)."));

	/** The tendril's tip must pass this near the player's capsule to latch (cm); it seeks them a little in flight (a second). */
	constexpr float TipRadius = 30.f;
	constexpr float SeekRate = 3.5f;
	/** The drain's damage ticks this often (s), the feedback renews this often (s), the slow while latched (share kept, s). */
	constexpr float DrainTick = 0.35f;
	constexpr float TouchEvery = 0.9f;
	constexpr float LatchedSlow = 0.8f;
	constexpr float LatchedSlowSeconds = 1.f;
	/** The last call's tendril drains at this share of an ordinary one's: it's to be held through, not to kill. */
	constexpr float FinalDrainShare = 0.45f;
	/** The latch's yank toward him (cm/s along, up), the snap's whip back (s). */
	constexpr float YankAlong = 180.f;
	constexpr float YankUp = 50.f;
	constexpr float SnapSeconds = 0.28f;
	/** The black lantern's flare: its glow at the globe (cm across), its colour, how big a ring it shows (cm). */
	constexpr float TellGlow = 22.f;
	const FLinearColor TellColor(0.3f, 0.85f, 1.f);

	FVector ChestOf(const APawn* Pawn)
	{
		return Pawn ? Pawn->GetActorLocation() + FVector(0.0, 0.0, DredgerRules::ChestRise) : FVector::ZeroVector;
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
}

// ---------------------------------------------------------------------------
// Throwing, latching, letting go
// ---------------------------------------------------------------------------

bool UAbelCorruptionComponent::ThrowTendrils(bool bTwo)
{
	AAbelKeeper* Abel = GetAbel();
	const UBossComponent* Boss = GetBoss();
	APawn* Player = GetPlayer();
	if (CVarAbelTendrils.GetValueOnGameThread() == 0 || !Abel || !Boss || !Player || Abel->IsDead() || !Boss->IsFighting() || bLocked
		|| Abel->IsOutInFog() || NumTendrilsOut() > 0)
	{
		return false;
	}
	if (FVector::Dist(Abel->GetLanternGlobe(), ChestOf(Player)) > AbelCorruptionRules::TendrilThrowRange)
	{
		return false;
	}
	const int32 Count = bTwo ? 2 : 1;
	for (int32 Index = 0; Index < Count && Index < Tendrils.Num(); ++Index)
	{
		FTendril& Tendril = Tendrils[Index];
		Tendril = FTendril();
		Tendril.State = FTendril::EState::Tell;
		Tendril.Victim = Player;
		Tendril.Delay = Index == 0 ? 0.f : AbelCorruptionRules::SecondTendrilAfter;
		Tendril.Tip = Abel->GetLanternGlobe();
	}
	// The tell: the black lantern flares (a cold cyan glow swelling in its soot-black glass) a second before the line flies.
	// Shot then, it never leaves.
	if (UEnemyProjectileSubsystem* Shots = GetWorld() ? GetWorld()->GetSubsystem<UEnemyProjectileSubsystem>() : nullptr)
	{
		Shots->ShowCharge(Abel, Abel->GetActorTransform().InverseTransformPosition(Abel->GetLanternGlobe()), AbelCorruptionRules::TendrilTell,
			TellGlow, TellColor);
	}
	LooterSound::PlayAt(Abel, LooterSoundCue::AbelFogFlare, Abel->GetLanternGlobe(), 0.8f, 1.15f);
	return true;
}

void UAbelCorruptionComponent::ThrowFinalTendril()
{
	AAbelKeeper* Abel = GetAbel();
	APawn* Player = GetPlayer();
	if (CVarAbelTendrils.GetValueOnGameThread() == 0 || !Abel || !Player || Tendrils.IsEmpty())
	{
		return;
	}
	FTendril& Tendril = Tendrils[0];
	if (Tendril.State != FTendril::EState::Off)
	{
		LetGo(Tendril, AbelCorruptionRules::EBreak::OwnerDown, true);
	}
	Tendril = FTendril();
	Tendril.State = FTendril::EState::Flying;
	Tendril.bFinal = true;
	Tendril.Victim = Player;
	Tendril.Tip = Abel->GetLanternGlobe();
	Tendril.Aim = ChestOf(Player);
	LooterSound::PlayAt(Abel, LooterSoundCue::AbelFogFlare, Abel->GetLanternGlobe());
}

int32 UAbelCorruptionComponent::NumTendrilsOut() const
{
	int32 Out = 0;
	for (const FTendril& Tendril : Tendrils)
	{
		Out += Tendril.State != FTendril::EState::Off && Tendril.State != FTendril::EState::Snapping ? 1 : 0;
	}
	return Out;
}

int32 UAbelCorruptionComponent::NumTendrilsLatched() const
{
	int32 Latched = 0;
	for (const FTendril& Tendril : Tendrils)
	{
		Latched += Tendril.State == FTendril::EState::Latched ? 1 : 0;
	}
	return Latched;
}

bool UAbelCorruptionComponent::IsLanternFlaring() const
{
	for (const FTendril& Tendril : Tendrils)
	{
		if (Tendril.State == FTendril::EState::Tell && Tendril.Clock >= Tendril.Delay)
		{
			return true;
		}
		if (Tendril.State == FTendril::EState::Latched && !Tendril.bFinal && AbelCorruptionRules::IsLanternFlaring(Tendril.LatchedFor))
		{
			return true;
		}
	}
	return false;
}

bool UAbelCorruptionComponent::ShootLantern()
{
	if (!IsLanternFlaring())
	{
		return false;
	}
	AAbelKeeper* Abel = GetAbel();
	bool bAny = false;
	for (FTendril& Tendril : Tendrils)
	{
		const bool bTelling = Tendril.State == FTendril::EState::Tell;
		const bool bFlaring = Tendril.State == FTendril::EState::Latched && !Tendril.bFinal && AbelCorruptionRules::IsLanternFlaring(Tendril.LatchedFor);
		if (bTelling || bFlaring)
		{
			LetGo(Tendril, AbelCorruptionRules::EBreak::LanternShot, false);
			bAny = true;
		}
	}
	// The shot on the black lantern: a crack of glass and a spit of cyan from it, his smog flinching.
	if (bAny && Abel)
	{
		const FVector Globe = Abel->GetLanternGlobe();
		LooterSound::PlayAt(Abel, LooterSoundCue::Impact::Glass, Globe);
		if (FWeaponFX* Effects = EffectsIn(GetWorld()))
		{
			Effects->SpawnSoulTatters(Globe, FVector::UpVector, DredgerLook::SoulCyan, 30.f, 6);
			Effects->SpawnFlash(Globe, FVector::UpVector, 0.5f);
		}
		UE_LOG(LogLooter, Log, TEXT("%s: the black lantern shot as it flares; its tendril lets go."), *Abel->GetActorNameOrLabel());
	}
	return bAny;
}

void UAbelCorruptionComponent::BreakTendrils(AbelCorruptionRules::EBreak Why, bool bQuiet)
{
	for (FTendril& Tendril : Tendrils)
	{
		if (Tendril.State != FTendril::EState::Off)
		{
			LetGo(Tendril, Why, bQuiet);
		}
	}
}

void UAbelCorruptionComponent::LatchTendril(FTendril& Tendril)
{
	APawn* Pawn = Tendril.Victim.Get();
	AAbelKeeper* Abel = GetAbel();
	Tendril.State = FTendril::EState::Latched;
	Tendril.LatchedFor = 0.f;
	Tendril.DrainOwed = 0.f;
	Tendril.DrainClock = 0.f;
	Tendril.TouchClock = 0.f;
	if (!Pawn || !Abel)
	{
		return;
	}
	Tendril.Tip = ChestOf(Pawn);
	// The smog's bite: the hose-man's clamp and hiss (the same soul-hose's sounds, his furnace's), a yank toward him.
	LooterSound::PlayAt(Abel, LooterSoundCue::DredgerHoseLatch, Tendril.Tip);
	Tendril.DrainSound = LooterSound::Start(Abel, LooterSoundCue::DredgerHoseDrain, Pawn->GetRootComponent());
	const FVector ToHim = (Abel->GetActorLocation() - Pawn->GetActorLocation()).GetSafeNormal2D();
	if (ACharacter* Character = Cast<ACharacter>(Pawn); Character && !Tendril.bFinal)
	{
		Character->LaunchCharacter(ToHim * YankAlong + FVector(0.0, 0.0, YankUp), false, false);
	}
	if (FWeaponFX* Effects = EffectsIn(GetWorld()))
	{
		Effects->SpawnSoulTatters(Tendril.Tip, ToHim, DredgerLook::SoulCyan, 40.f, 5);
	}
	if (UPlayerRattleSubsystem* Rattles = UPlayerRattleSubsystem::Get(this))
	{
		FPlayerRattle How;
		How.Strength = 0.75f;
		How.bSway = true;
		How.SwayDegrees = 0.9f;
		How.bSlow = false;
		How.Shake = 0.35f;
		How.ShakeSeconds = 0.4f;
		How.FrostSeconds = 1.f;
		Rattles->Rattle(Pawn, Abel->GetLanternGlobe(), How);
	}
}

void UAbelCorruptionComponent::LetGo(FTendril& Tendril, AbelCorruptionRules::EBreak Why, bool bQuiet)
{
	const bool bWasLatched = Tendril.State == FTendril::EState::Latched;
	LooterSound::Stop(Tendril.DrainSound.Get(), 0.25f);
	Tendril.DrainSound.Reset();
	if (bQuiet || Tendril.State == FTendril::EState::Off || Tendril.State == FTendril::EState::Tell)
	{
		Tendril = FTendril();
		return;
	}
	Tendril.SnapFrom = bWasLatched ? ChestOf(Tendril.Victim.Get()) : Tendril.Tip;
	Tendril.SnapShare = 0.f;
	Tendril.State = FTendril::EState::Snapping;
	if (bWasLatched)
	{
		// Torn off them: the snap, a spit of soul-light where it let go.
		LooterSound::PlayAt(GetOwner(), LooterSoundCue::DredgerHoseSnap, Tendril.SnapFrom);
		if (FWeaponFX* Effects = EffectsIn(GetWorld()))
		{
			Effects->SpawnSoulTatters(Tendril.SnapFrom, FVector::UpVector, DredgerLook::SoulCyan, 30.f, 4);
			Effects->SpawnFlash(Tendril.SnapFrom, FVector::UpVector, 0.4f);
		}
	}
	UE_LOG(LogLooter, Verbose, TEXT("%s: a smog tendril lets go (%d) after %.1f s."), *GetNameSafe(GetOwner()), static_cast<int32>(Why), Tendril.LatchedFor);
}

// ---------------------------------------------------------------------------
// Frame by frame
// ---------------------------------------------------------------------------

void UAbelCorruptionComponent::TickTendrils(float DeltaSeconds)
{
	bLanternShotThisFrame = false;
	for (FTendril& Tendril : Tendrils)
	{
		StepTendril(Tendril, DeltaSeconds);
	}
}

void UAbelCorruptionComponent::StepTendril(FTendril& Tendril, float DeltaSeconds)
{
	AAbelKeeper* Abel = GetAbel();
	APawn* Pawn = Tendril.Victim.Get();
	if (Tendril.State == FTendril::EState::Off)
	{
		return;
	}
	Tendril.Clock += DeltaSeconds;
	const FVector Globe = Abel ? Abel->GetLanternGlobe() : Tendril.Tip;
	switch (Tendril.State)
	{
	case FTendril::EState::Tell:
		Tendril.Tip = Globe;
		if (!Abel || !Pawn || (Abel->IsDead() && !Tendril.bFinal) || bLocked)
		{
			LetGo(Tendril, AbelCorruptionRules::EBreak::OwnerDown, true);
		}
		else if (Tendril.Clock >= Tendril.Delay + AbelCorruptionRules::TendrilTell)
		{
			Tendril.State = FTendril::EState::Flying;
			Tendril.Flown = 0.f;
			// At their chest, led a little: it seeks them in flight, but a step aside as it leaves still makes it miss.
			Tendril.Aim = ChestOf(Pawn) + Pawn->GetVelocity() * 0.15f;
		}
		break;

	case FTendril::EState::Flying:
	{
		if (!Abel || !Pawn || (Abel->IsDead() && !Tendril.bFinal))
		{
			LetGo(Tendril, AbelCorruptionRules::EBreak::OwnerDown, false);
			break;
		}
		const FVector From = Tendril.Tip;
		Tendril.Aim = FMath::VInterpTo(Tendril.Aim, ChestOf(Pawn), DeltaSeconds, Tendril.bFinal ? SeekRate * 3.f : SeekRate);
		const FVector Way = (Tendril.Aim - Tendril.Tip).GetSafeNormal();
		const float Step = AbelCorruptionRules::TendrilSpeed * DeltaSeconds;
		Tendril.Tip += Way * Step;
		Tendril.Flown += Step;
		float Radius = 0.f;
		float HalfHeight = 0.f;
		Pawn->GetSimpleCollisionCylinder(Radius, HalfHeight);
		float When = 0.f;
		if (UEnemyProjectileSubsystem::SweepHitsCapsule(From, Tendril.Tip, TipRadius, Pawn->GetActorLocation(), Radius, HalfHeight, When))
		{
			LatchTendril(Tendril);
		}
		else if (!Tendril.bFinal && (Tendril.Flown >= AbelCorruptionRules::TendrilReach || FVector::DotProduct(Tendril.Aim - Tendril.Tip, Way) < 0.f))
		{
			// A miss: it whips back to the lantern.
			Tendril.LatchedFor = 0.f;
			LetGo(Tendril, AbelCorruptionRules::EBreak::Distance, false);
		}
		break;
	}

	case FTendril::EState::Latched:
	{
		Tendril.LatchedFor += DeltaSeconds;
		const UHealthComponent* Life = Pawn ? Pawn->FindComponentByClass<UHealthComponent>() : nullptr;
		AbelCorruptionRules::FTendrilFacts Facts;
		Facts.LatchedFor = Tendril.LatchedFor;
		Facts.bFinal = Tendril.bFinal;
		Facts.bOwnerDown = !Abel || (Abel->IsDead() && !Tendril.bFinal) || !Abel->IsPresent();
		Facts.bVictimGone = !Pawn || (Life && Life->IsDead());
		const UBossComponent* Boss = GetBoss();
		Facts.bStaggered = (Boss && Boss->IsStaggered()) || bKneeling;
		Facts.Distance = Abel && Pawn ? static_cast<float>(FVector::Dist2D(Abel->GetActorLocation(), Pawn->GetActorLocation())) : 0.f;
		const AbelCorruptionRules::EBreak Why = AbelCorruptionRules::TendrilBreaks(Facts);
		if (Why != AbelCorruptionRules::EBreak::None)
		{
			LetGo(Tendril, Why, false);
			break;
		}
		Tendril.Tip = ChestOf(Pawn);
		// The drain: a share of his bite a second, in small ticks as damage (the ward takes it first).
		const float Rate = Abel->AttackDamage * AbelCorruptionRules::TendrilDrainShare * (Tendril.bFinal ? FinalDrainShare : 1.f);
		Tendril.DrainOwed += Rate * DeltaSeconds;
		Tendril.DrainClock += DeltaSeconds;
		if (Tendril.DrainClock >= DrainTick)
		{
			Tendril.DrainClock -= DrainTick;
			const float Amount = Tendril.DrainOwed;
			Tendril.DrainOwed = 0.f;
			TendrilDrainDealt += Amount;
			UGameplayStatics::ApplyDamage(Pawn, Amount, Abel->GetController(), Abel, UCreatureAttackDamageType::StaticClass());
		}
		// The pull, as a hose-man's: a share of their own walk toward him (none on the last call: they're holding the post).
		if (!Tendril.bFinal && Pawn->IsPlayerControlled())
		{
			const FVector ToHim = (Abel->GetActorLocation() - Pawn->GetActorLocation()).GetSafeNormal2D();
			const float Ease = FMath::SmoothStep(250.f, 500.f, Facts.Distance);
			Pawn->AddMovementInput(ToHim, AbelCorruptionRules::TendrilPull * Ease);
		}
		Tendril.TouchClock += DeltaSeconds;
		if (Tendril.TouchClock >= TouchEvery)
		{
			Tendril.TouchClock = 0.f;
			if (UPlayerRattleSubsystem* Rattles = UPlayerRattleSubsystem::Get(this))
			{
				FPlayerRattle How;
				How.Strength = 0.4f;
				How.bSway = false;
				How.bSlow = false;
				How.Shake = 0.08f;
				How.ShakeSeconds = 0.4f;
				How.FrostSeconds = 1.25f;
				Rattles->Rattle(Pawn, Globe, How);
			}
			if (ACharacter* Character = Pawn->IsPlayerControlled() ? Cast<ACharacter>(Pawn) : nullptr; Character && !Tendril.bFinal)
			{
				UMovementSlowComponent::Apply(Character, LatchedSlow, LatchedSlowSeconds);
			}
		}
		break;
	}

	case FTendril::EState::Snapping:
		Tendril.SnapShare = FMath::Min(1.f, Tendril.SnapShare + DeltaSeconds / SnapSeconds);
		Tendril.Tip = FMath::Lerp(Tendril.SnapFrom, Globe, FMath::InterpEaseIn(0.f, 1.f, Tendril.SnapShare, 2.f));
		if (Tendril.SnapShare >= 1.f)
		{
			Tendril = FTendril();
		}
		break;

	default:
		break;
	}
}


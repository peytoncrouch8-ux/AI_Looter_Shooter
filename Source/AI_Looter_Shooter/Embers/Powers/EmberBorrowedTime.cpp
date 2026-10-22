#include "Embers/Powers/EmberBorrowedTime.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Combat/BulletSubsystem.h"
#include "Combat/HealthComponent.h"
#include "Core/LooterCharacter.h"
#include "Embers/EmberDefinition.h"
#include "Embers/Powers/EmberVfx.h"
#include "Player/CameraShakeModifier.h"
#include "Player/ViewKick.h"
#include "Progression/PlayerProgressionSubsystem.h"
#include "Weapons/WeaponFX.h"
#include "Components/CapsuleComponent.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"

namespace
{
	const FName PayBackName(TEXT("PayBackSeconds"));

	/** A debt smaller than this isn't worth calling in (health points). */
	constexpr float SmallestDebt = 0.5f;
	/** While it runs, a few violet motes rise round the feet this often (seconds). */
	constexpr float MoteEvery = 0.2f;

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

	FVector FeetOf(const ACharacter& Character)
	{
		const float HalfHeight = Character.GetCapsuleComponent() ? Character.GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 90.f;
		return Character.GetActorLocation() - FVector(0.0, 0.0, HalfHeight);
	}
}

void UEmberBorrowedTime::Activate(const FEmberContext& Context)
{
	ACharacter* Character = Context.Character;
	if (!Character)
	{
		return;
	}
	const UEmberDefinition* Def = Context.Definition;
	Strength = Def && Def->Strength > 0.f ? FMath::Clamp(Def->Strength, 0.f, 1.f) : DefaultStrength;
	PayBackSeconds = FMath::Max(Def ? Def->Extra(PayBackName, DefaultPayBackSeconds) : DefaultPayBackSeconds, 0.1f);
	if (Def)
	{
		Color = Def->Color;
	}
	Body = Character;
	Health = Character->FindComponentByClass<UHealthComponent>();
	const APlayerController* Player = Cast<APlayerController>(Character->GetController());
	const ULocalPlayer* Local = Player ? Player->GetLocalPlayer() : nullptr;
	Progression = Local ? Local->GetSubsystem<UPlayerProgressionSubsystem>() : nullptr;

	// A debt still being paid back (a console use straight after) is carried into the new ledger, not forgiven.
	if (!bSettling)
	{
		Debt = 0.f;
	}
	bSettling = false;
	KnownHealth = Health.IsValid() ? Health->GetHealth() : 0.f;
	bBorrowing = true;
	MoteTimer = 0.f;
	Bind(true);

	LooterSound::Play2D(Character, LooterSoundCue::Ember::BorrowedTimeStart);
	if (FWeaponFX* Effects = EffectsIn(Character->GetWorld()))
	{
		// The ledger opens: soul-light in its violet rising round the feet (and, the VFX pass, its ring racing out).
		Effects->SpawnDeathBurst(EDeathBurst::SoulLight, FeetOf(*Character) + FVector(0.0, 0.0, 40.0), FVector::UpVector, 80.f, Color);
		EmberVfx::Activation(*Effects, FeetOf(*Character), Color, 0.9f);
	}
	if (APlayerController* Owner = Cast<APlayerController>(Character->GetController()))
	{
		FViewKick Open;
		Open.Pitch = 0.25f;
		Open.FieldOfView = 0.015f;
		Open.Frequency = 5.f;
		Open.Damping = 0.7f;
		UCameraShakeModifier::Kick(Owner, Open);
	}
	UE_LOG(LogLooter, Log, TEXT("Borrowed Time: the ledger is open (%.0f%% struck off a kill, paid back over %.1f s)"), Strength * 100.f,
		PayBackSeconds);
}

void UEmberBorrowedTime::Tick(const FEmberContext& Context, float DeltaSeconds)
{
	MoteTimer -= DeltaSeconds;
	if (bBorrowing && MoteTimer <= 0.f)
	{
		MoteTimer = MoteEvery;
		ShowMotes();
	}
}

void UEmberBorrowedTime::End(const FEmberContext& Context, bool bInterrupted)
{
	if (!bBorrowing)
	{
		return;
	}
	Bind(false);
	bBorrowing = false;
	UHealthComponent* Life = Health.Get();
	// Death forgives the debt; anything else (the time running out, a scene) calls it in.
	if (!Life || Life->IsDead() || Debt < SmallestDebt)
	{
		Debt = 0.f;
		UE_LOG(LogLooter, Log, TEXT("Borrowed Time: closed with nothing owed"));
		return;
	}
	bSettling = true;
	PayRate = Debt / PayBackSeconds;
	ACharacter* Character = Body.Get();
	UWorld* World = Character ? Character->GetWorld() : nullptr;
	if (World && !World->bIsTearingDown)
	{
		LooterSound::Play2D(World, LooterSoundCue::Ember::BorrowedTimeSettle);
		// (The VFX pass) the debt called in: the ledger's ring closing in a darker violet, short and heavy.
		if (FWeaponFX* Effects = EffectsIn(World))
		{
			EmberVfx::Activation(*Effects, FeetOf(*Character), Color * 0.6f, 0.6f);
		}
		if (APlayerController* Player = Cast<APlayerController>(Character->GetController()))
		{
			// The debt called in: a heavy dip of the view.
			FViewKick Called;
			Called.Pitch = -0.6f;
			Called.Roll = 0.5f;
			Called.FieldOfView = -0.02f;
			Called.Frequency = 4.f;
			Called.Damping = 0.7f;
			UCameraShakeModifier::Kick(Player, Called);
			if (UCameraShakeModifier* Shakes = UCameraShakeModifier::FindOrAdd(Player))
			{
				Shakes->AddShake(0.25f, PayBackSeconds);
			}
		}
	}
	UE_LOG(LogLooter, Log, TEXT("Borrowed Time: %.1f owed, paid back over %.1f s%s"), Debt, PayBackSeconds, bInterrupted ? TEXT(" (cut short)") : TEXT(""));
	SchedulePump();
}

float UEmberBorrowedTime::GetDebtFraction() const
{
	const UHealthComponent* Life = Health.Get();
	const float Most = Life ? FMath::Max(Life->GetMaxHealth(), 1.f) : 100.f;
	return FMath::Clamp(Debt / Most, 0.f, 1.f);
}

// ---------------------------------------------------------------------------
// The ledger
// ---------------------------------------------------------------------------

void UEmberBorrowedTime::Bind(bool bOn)
{
	if (UHealthComponent* Life = Health.Get())
	{
		if (bOn)
		{
			Life->OnDamaged.AddUniqueDynamic(this, &UEmberBorrowedTime::HandleDamaged);
			Life->OnHealthChanged.AddUniqueDynamic(this, &UEmberBorrowedTime::HandleHealthChanged);
		}
		else
		{
			Life->OnDamaged.RemoveDynamic(this, &UEmberBorrowedTime::HandleDamaged);
			Life->OnHealthChanged.RemoveDynamic(this, &UEmberBorrowedTime::HandleHealthChanged);
		}
	}
	if (UPlayerProgressionSubsystem* Kills = Progression.Get())
	{
		Kills->OnKillXP.Remove(KillHandle);
		KillHandle.Reset();
		if (bOn)
		{
			KillHandle = Kills->OnKillXP.AddUObject(this, &UEmberBorrowedTime::HandleKillXP);
		}
	}
}

void UEmberBorrowedTime::HandleHealthChanged(float NewHealth, float MaxHealth)
{
	KnownHealth = NewHealth;
}

void UEmberBorrowedTime::HandleDamaged(float Damage, bool bCritical, FVector HitLocation, AController* InstigatedBy, AActor* DamageCauser)
{
	UHealthComponent* Life = Health.Get();
	if (!bBorrowing || !Life)
	{
		return;
	}
	// What the hurt really took (none if something already made the player untouchable). Healed back now, before the
	// health component counts a death, so a killing blow is taken back too.
	const float Lost = KnownHealth - Life->GetHealth();
	if (Lost <= 0.f)
	{
		return;
	}
	Life->Heal(Lost);
	// The debt is what reached health: a part the ward soaked was never a wound (GetLastHealthLoss: after the shield).
	const float Owed = FMath::Min(Damage, Life->GetLastHealthLoss());
	Debt += Owed;
	UE_LOG(LogLooter, Verbose, TEXT("Borrowed Time: %.1f taken back, %.1f owed"), Owed, Debt);
	if (FWeaponFX* Effects = EffectsIn(Life->GetWorld()))
	{
		// Where it landed, a puff of violet: the ledger took it.
		for (int32 Index = 0; Index < 4; ++Index)
		{
			Effects->SpawnDustPuff(HitLocation + Random.GetUnitVector() * 12.f, Random.GetUnitVector() * 60.f + FVector(0.0, 0.0, 50.0),
				Color * 0.6f, 0.35f, 10.f, Random.FRandRange(45.f, 70.f), Random.FRandRange(0.5f, 0.8f));
		}
	}
}

void UEmberBorrowedTime::HandleKillXP(int64 XP, const AActor* Victim)
{
	NoteKill(Victim);
}

float UEmberBorrowedTime::NoteKill(const AActor* Victim)
{
	if (!bBorrowing || Debt <= 0.f)
	{
		return 0.f;
	}
	const float Struck = Debt * Strength;
	Debt -= Struck;
	UE_LOG(LogLooter, Verbose, TEXT("Borrowed Time: a kill strikes %.1f off, %.1f owed"), Struck, Debt);
	if (FWeaponFX* Effects = Victim ? EffectsIn(Victim->GetWorld()) : nullptr)
	{
		// A line struck through in the ledger: the soul-light of the one killed flares violet (the pass: and strikes out).
		Effects->SpawnDeathBurst(EDeathBurst::SoulLight, Victim->GetActorLocation(), FVector::UpVector, 50.f, Color);
		EmberVfx::Strike(*Effects, Victim->GetActorLocation(), Color, 0.9f);
	}
	return Struck;
}

// ---------------------------------------------------------------------------
// Paying it back
// ---------------------------------------------------------------------------

void UEmberBorrowedTime::StepSettle(float DeltaSeconds)
{
	if (!bSettling)
	{
		return;
	}
	UHealthComponent* Life = Health.Get();
	if (!Life || Life->IsDead())
	{
		// Death forgives what's left.
		Debt = 0.f;
		bSettling = false;
		return;
	}
	// Evenly over the pay-back time: what Drain can't take (the last point of health) is forgiven, never fatal.
	const float Pay = FMath::Min(Debt, PayRate * FMath::Max(DeltaSeconds, 0.f));
	Life->Drain(Pay);
	Debt -= Pay;
	if (Debt <= 0.001f)
	{
		Debt = 0.f;
		bSettling = false;
	}
}

void UEmberBorrowedTime::SchedulePump()
{
	UWorld* World = Body.IsValid() ? Body->GetWorld() : nullptr;
	if (bPumping || !World)
	{
		return;
	}
	bPumping = true;
	World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &UEmberBorrowedTime::Pump));
}

void UEmberBorrowedTime::Pump()
{
	bPumping = false;
	ACharacter* Character = Body.Get();
	UWorld* World = Character ? Character->GetWorld() : nullptr;
	if (!World)
	{
		return;
	}
	// The player's own seconds (the same under any slowed world).
	StepSettle(World->GetDeltaSeconds() * FMath::Max(Character->CustomTimeDilation, 0.f));
	if (bSettling)
	{
		SchedulePump();
	}
}

void UEmberBorrowedTime::ShowMotes()
{
	ACharacter* Character = Body.Get();
	FWeaponFX* Effects = Character ? EffectsIn(Character->GetWorld()) : nullptr;
	if (!Effects)
	{
		return;
	}
	// The ledger's light round the feet while it's open: a few violet motes curling up.
	const FVector Feet = FeetOf(*Character);
	for (int32 Index = 0; Index < 2; ++Index)
	{
		const float Angle = Random.FRandRange(0.f, 2.f * UE_PI);
		const FVector Round(FMath::Cos(Angle), FMath::Sin(Angle), 0.f);
		Effects->SpawnDustPuff(Feet + Round * Random.FRandRange(35.f, 60.f) + FVector(0.0, 0.0, 10.0),
			FVector(0.0, 0.0, Random.FRandRange(50.f, 90.f)) + Round * 15.f, Color * 0.5f, 0.25f, 8.f, Random.FRandRange(30.f, 45.f),
			Random.FRandRange(0.8f, 1.1f));
	}
}

// UPlayerShieldComponent: the ward worn as it soaks, breaks and fills, and what it's bound to (the health component's
// hook, the weapon manager for the starting ward, the progression for kills). The traits are PlayerShieldTraits.cpp, the
// wards carried and saved PlayerShieldWards.cpp.

#include "Shields/PlayerShieldComponent.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Combat/HealthComponent.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Progression/PlayerProgressionSubsystem.h"
#include "Shields/ShieldDefinition.h"
#include "Shields/ShieldSounds.h"
#include "Shields/ShieldTuning.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

namespace
{
	/** A refill that starts or ends with the ward fuller than this is a top-up: no sound for it. */
	constexpr float TopUpShare = 0.85f;
	/** Shield.Hit rises in pitch as the ward runs low: this much higher at empty. */
	constexpr float HitPitchRise = 0.25f;
}

UPlayerShieldComponent::UPlayerShieldComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

UPlayerShieldComponent* UPlayerShieldComponent::Find(const AActor* Owner)
{
	return Owner ? Owner->FindComponentByClass<UPlayerShieldComponent>() : nullptr;
}

UPlayerShieldComponent* UPlayerShieldComponent::FindOrAdd(AActor& Owner)
{
	if (UPlayerShieldComponent* Found = Find(&Owner))
	{
		return Found;
	}
	UPlayerShieldComponent* Made = NewObject<UPlayerShieldComponent>(&Owner);
	Owner.AddInstanceComponent(Made);
	// Registered on an actor whose play has begun, it begins at once (binds the health, the manager); a test level's actor
	// started by hand (DispatchBeginPlay) may not have it begun by the registration, so it's begun here then.
	Made->RegisterComponent();
	if (Owner.HasActorBegunPlay() && !Made->HasBegunPlay())
	{
		Made->BeginPlay();
	}
	return Made;
}

void UPlayerShieldComponent::BeginPlay()
{
	Super::BeginPlay();
	BindHealth();
	BindWeaponManager();
	BindProgression();
}

void UPlayerShieldComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UHealthComponent* Life = Health.Get())
	{
		// Only its own binding goes: a creature's ward never shares a body with the player's.
		if (Life->Shield.IsBoundToObject(this))
		{
			Life->Shield.Unbind();
		}
	}
	if (UWeaponManagerComponent* Manager = BoundManager.Get())
	{
		Manager->OnInventoryChanged.RemoveDynamic(this, &UPlayerShieldComponent::HandleInventoryChanged);
	}
	if (UPlayerProgressionSubsystem* Kills = Progression.Get())
	{
		Kills->OnKillXP.Remove(KillHandle);
	}
	KillHandle.Reset();
	Super::EndPlay(EndPlayReason);
}

void UPlayerShieldComponent::BindHealth()
{
	UHealthComponent* Life = GetOwner() ? GetOwner()->FindComponentByClass<UHealthComponent>() : nullptr;
	if (!Life || Life == Health.Get())
	{
		return;
	}
	if (Life->Shield.IsBound() && !Life->Shield.IsBoundToObject(this))
	{
		UE_LOG(LogLooter, Warning, TEXT("%s: its health already has a shield bound; the player's ward stays off it."), *GetNameSafe(GetOwner()));
		return;
	}
	Health = Life;
	Life->Shield.BindUObject(this, &UPlayerShieldComponent::SoakHit);
}

void UPlayerShieldComponent::BindWeaponManager()
{
	UWeaponManagerComponent* Manager = GetOwner() ? GetOwner()->FindComponentByClass<UWeaponManagerComponent>() : nullptr;
	if (!Manager || Manager == BoundManager.Get())
	{
		return;
	}
	BoundManager = Manager;
	Manager->OnInventoryChanged.AddUniqueDynamic(this, &UPlayerShieldComponent::HandleInventoryChanged);
	// Guns given before play began (a character's starting weapons) count too.
	HandleInventoryChanged();
}

void UPlayerShieldComponent::BindProgression()
{
	if (Progression.IsValid())
	{
		return;
	}
	const APawn* Pawn = Cast<APawn>(GetOwner());
	const APlayerController* PC = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	const ULocalPlayer* Local = PC ? PC->GetLocalPlayer() : nullptr;
	UPlayerProgressionSubsystem* Found = Local ? Local->GetSubsystem<UPlayerProgressionSubsystem>() : nullptr;
	if (!Found)
	{
		return;
	}
	Progression = Found;
	KillHandle = Found->OnKillXP.AddUObject(this, &UPlayerShieldComponent::HandleKillXP);
}

void UPlayerShieldComponent::HandleInventoryChanged()
{
	// The first gun in the player's hands brings the starting ward, as it brings the starting grenades
	// (UPlayerThrowComponent): a new game's bullpup, Skyreach's rack rifle, or the guns of a session saved before wards.
	const UWeaponManagerComponent* Manager = BoundManager.Get();
	const APawn* Pawn = Cast<APawn>(GetOwner());
	if (!bStarterGiven && Manager && Manager->GetWeapons().Num() > 0 && Pawn && Pawn->IsPlayerControlled())
	{
		GiveStarter();
	}
}

bool UPlayerShieldComponent::GiveStarter()
{
	if (bStarterGiven || !FShieldTuning::Live().Starter)
	{
		return false;
	}
	bStarterGiven = true;
	if (HasWard())
	{
		// A ward found before the first gun stays on.
		return false;
	}
	// At the player's level (a session saved before wards, loaded late in the game, gets a ward that keeps up).
	const UPlayerProgressionSubsystem* Kills = Progression.Get();
	const int32 Level = Kills ? FMath::Max(Kills->GetLevel(), 1) : 1;
	Ward = ShieldRules::Starter(UShieldDefinition::GetWardBox(), Level);
	ResetWardState(true);
	UE_LOG(LogLooter, Log, TEXT("Shield: given the starting %s (level %d, %.0f)"), *GetNameSafe(Ward.Definition), Level, GetCapacity());
	NotifyWardsChanged();
	return true;
}

void UPlayerShieldComponent::HandleKillXP(int64 XP, const AActor* Victim)
{
	if (!HasWard() || Ward.Trait != EShieldTrait::Harvest)
	{
		return;
	}
	// Harvest: a kill puts back a share and starts the ward filling at once (its wait skipped).
	const FShieldTuning Tuning = FShieldTuning::Live();
	const float Capacity = GetCapacity();
	Charge.Charge = FMath::Min(Charge.Charge + ShieldRules::HarvestRestore(Capacity, Strength(), Tuning), Capacity);
	Charge.Quiet = ShieldRules::LiveDelay(Ward.Stats, Tuning);
	bBroken = Charge.Charge <= 0.f;
}

// ---------------------------------------------------------------------------
// The ward as it stands
// ---------------------------------------------------------------------------

float UPlayerShieldComponent::GetCapacity() const
{
	return HasWard() ? ShieldRules::LiveCapacity(Ward.Stats, FShieldTuning::Live()) : 0.f;
}

float UPlayerShieldComponent::GetChargeShare() const
{
	const float Capacity = GetCapacity();
	return Capacity > 0.f ? FMath::Clamp(Charge.Charge / Capacity, 0.f, 1.f) : 0.f;
}

bool UPlayerShieldComponent::IsFull() const
{
	return HasWard() && Charge.IsFull(GetCapacity());
}

float UPlayerShieldComponent::GetWaitLeft() const
{
	if (!HasWard() || Charge.bFilling || IsFull())
	{
		return 0.f;
	}
	return FMath::Max(ShieldRules::LiveDelay(Ward.Stats, FShieldTuning::Live()) - Charge.Quiet, 0.f);
}

bool UPlayerShieldComponent::HoldsHealthBack() const
{
	const FShieldTuning Tuning = FShieldTuning::Live();
	return Tuning.Enable && Tuning.HealthWaits && HasWard() && !IsFull();
}

void UPlayerShieldComponent::SetChargeForTest(float NewCharge)
{
	Charge.Charge = FMath::Clamp(NewCharge, 0.f, GetCapacity());
	bBroken = HasWard() && Charge.Charge <= 0.f;
}

// ---------------------------------------------------------------------------
// Soaking and filling
// ---------------------------------------------------------------------------

float UPlayerShieldComponent::SoakHit(float Damage, bool bCritical, const FVector& HitLocation)
{
	const FShieldTuning Tuning = FShieldTuning::Live();
	if (!HasWard() || !Tuning.Enable)
	{
		return Damage;
	}
	++HitCount;
	const float ChargeBefore = Charge.Charge;
	// Any hit, soaked or not, starts the wait over: a fight still going never refills.
	Charge.Hurt();
	const ShieldRules::FShieldSoak Result = ShieldRules::Soak(ChargeBefore, Damage, Tuning.BreakSpill);
	Charge.Charge = Result.ChargeAfter;
	if (ChargeBefore > 0.f)
	{
		TraitsOnHit(ChargeBefore);
	}
	if (Result.bBroke)
	{
		bBroken = true;
		++BreakCount;
		PlayCue(ShieldCue::Break);
		TraitsOnBreak();
	}
	else if (Result.Absorbed > 0.f)
	{
		const UWorld* World = GetWorld();
		const double Now = World ? World->GetTimeSeconds() : 0.0;
		if (Now - LastHitSound >= Tuning.HitSoundGap)
		{
			LastHitSound = Now;
			// Thinner as it runs low: the ward's own warning before the break.
			PlayCue(ShieldCue::Hit, 1.f, 1.f + HitPitchRise * (1.f - GetChargeShare()));
		}
	}
	return Result.ToHealth;
}

void UPlayerShieldComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	// A health component or a controller that came after play began (a possession) is bound as soon as it's there.
	if (!Health.IsValid())
	{
		BindHealth();
	}
	if (!Progression.IsValid())
	{
		BindProgression();
	}
	// Starting guns are given as play begins, before the controller possesses the pawn: the starting ward waits for it.
	if (!bStarterGiven)
	{
		HandleInventoryChanged();
	}
	Advance(DeltaTime);
}

void UPlayerShieldComponent::Advance(float DeltaSeconds)
{
	if (!HasWard())
	{
		return;
	}
	const UHealthComponent* Life = Health.Get();
	if (Life && Life->IsDead())
	{
		// Dead: the ward is out. A respawn (alive again) wakes with it full, as the health comes back full.
		Charge.Charge = 0.f;
		Charge.Hurt();
		bBroken = true;
		bWasDead = true;
		return;
	}
	if (bWasDead)
	{
		bWasDead = false;
		ResetWardState(true);
		return;
	}

	const FShieldTuning Tuning = FShieldTuning::Live();
	const float Capacity = GetCapacity();
	const float ShareBefore = GetChargeShare();
	switch (Charge.Step(DeltaSeconds, Capacity, ShieldRules::LiveDelay(Ward.Stats, Tuning), ShieldRules::LiveRate(Ward.Stats, Tuning)))
	{
	case ShieldRules::ERefill::Started:
		++RefillStartCount;
		bBroken = false;
		RefillFrom = ShareBefore;
		if (ShareBefore < TopUpShare)
		{
			PlayCue(ShieldCue::RechargeStart);
		}
		break;
	case ShieldRules::ERefill::Filled:
		++FillCount;
		bBroken = false;
		bNovaReady = true;
		bGraceReady = true;
		if (FMath::Min(RefillFrom, ShareBefore) < TopUpShare)
		{
			PlayCue(ShieldCue::Full);
		}
		RefillFrom = 1.f;
		break;
	default:
		break;
	}
	TickTraits();
}

void UPlayerShieldComponent::ResetWardState(bool bFull)
{
	Charge = ShieldRules::FShieldCharge();
	if (bFull)
	{
		Charge.Fill(GetCapacity());
		// The wait already met: a full ward that is chipped refills after its delay, as any other.
		Charge.Quiet = ShieldRules::LiveDelay(Ward.Stats, FShieldTuning::Live());
	}
	bBroken = HasWard() && !bFull;
	bNovaReady = true;
	bGraceReady = true;
	bNovaPending = false;
	PendingBarbs.Reset();
	RefillFrom = 1.f;
}

void UPlayerShieldComponent::PlayCue(const TCHAR* Cue, float Volume, float Pitch) const
{
	// The player's own ward: heard in their head (2D), as the HUD shows it.
	const APawn* Pawn = Cast<APawn>(GetOwner());
	if (Pawn && Pawn->IsLocallyControlled())
	{
		LooterSound::Play2D(this, FName(Cue), Volume, Pitch);
	}
}

void UPlayerShieldComponent::NotifyWardsChanged()
{
	OnWardsChanged.Broadcast();
}

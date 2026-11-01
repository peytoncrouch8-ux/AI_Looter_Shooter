// ADredgerRifleman: his rifle, his turns and his attacks (his distance and cover: DredgerRiflemanCover.cpp; his rounds,
// tracers, reload and butt: DredgerRiflemanFire.cpp).

#include "Creatures/DredgerRifleman.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSoundCues.h"
#include "Combat/EnemyProjectileSubsystem.h"
#include "Combat/HealthComponent.h"
#include "Creatures/DredgerLook.h"
#include "Creatures/DredgerTuning.h"
#include "Weapons/WeaponDefinition.h"
#include "Weapons/WeaponModelComponent.h"
#include "Weapons/WeaponTypes.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"

ADredgerRifleman::ADredgerRifleman()
{
	DredgerRole = EDredgerRole::Rifleman;
	DisplayName = NSLOCTEXT("Dredger", "RiflemanName", "Dredger Rifleman");
	const FDredgerTuning Defaults;
	Health->MaxHealth = 140.f;
	AttackDamage = 7.f;
	XPReward = 14;
	WalkSpeed = 170.f;
	ChaseSpeed = 360.f;
	// His brain closes in only from past his band's far edge; within it his own tactics move him (the placeholder's rule:
	// the brain holds at three quarters of the attack range).
	AttackRange = Defaults.BandFar / 0.75f;
	AttackWindup = Defaults.RifleWindup;
	AttackRecovery = DredgerRules::BurstRecovery(Defaults);
	AttackCooldown = DredgerRules::BurstCooldown(Defaults);

	Rifle = CreateDefaultSubobject<UWeaponModelComponent>(TEXT("Rifle"));
	Rifle->SetupAttachment(Held);
}

void ADredgerRifleman::BeginPlay()
{
	Super::BeginPlay();
	// The company rifle: the project's AR from its parts at one seed (every rifleman carries the same issue gun) until the
	// Tripo rifle lands (DredgerLook::RifleDefinitionPath).
	UWeaponDefinition* Definition = LoadObject<UWeaponDefinition>(nullptr, DredgerLook::RifleDefinitionPath);
	FWeaponInstanceData Gun;
	Gun.Definition = Definition;
	Gun.Rarity = EWeaponRarity::Common;
	Gun.Level = 1;
	Gun.Seed = DredgerLook::RifleSeed;
	bRifleBuilt = Definition && Rifle->Assemble(Gun);
	UE_CLOG(!bRifleBuilt, LogLooter, Warning, TEXT("%s: its rifle (%s) could not be built: it holds nothing."), *GetName(),
		DredgerLook::RifleDefinitionPath);
	Rounds = FDredgerTuning::Live().Magazine;
	Dice.Initialize(static_cast<int32>(GetTypeHash(GetFName().ToString())) ^ static_cast<int32>(GetUniqueID()));
	RecheckIn = Dice.FRandRange(0.f, 0.6f);
}

// ---------------------------------------------------------------------------
// What he holds
// ---------------------------------------------------------------------------

FVector ADredgerRifleman::GetHeldGrip() const
{
	return bRifleBuilt ? Rifle->GetGrip() : FVector::ZeroVector;
}

FVector ADredgerRifleman::GetHeldForegrip() const
{
	return bRifleBuilt ? Rifle->GetForegrip() : FVector(30.f, 0.f, -3.f);
}

FVector ADredgerRifleman::GetHeldMuzzle() const
{
	return bRifleBuilt ? Rifle->GetMuzzle() : FVector(70.f, 0.f, 3.f);
}

float ADredgerRifleman::GetHeldScale() const
{
	return bRifleBuilt ? Rifle->GetModelScale() : 1.f;
}

// ---------------------------------------------------------------------------
// His turns
// ---------------------------------------------------------------------------

EFightRole ADredgerRifleman::GetAttackTokenRole() const
{
	return WantsButt() ? EFightRole::Melee : EFightRole::Ranged;
}

bool ADredgerRifleman::GetFightBand(float& OutNear, float& OutFar) const
{
	const FDredgerTuning Tuning = FDredgerTuning::Live();
	OutNear = Tuning.BandNear;
	OutFar = Tuning.BandFar;
	return true;
}

float ADredgerRifleman::GetAttackStartRange() const
{
	const APawn* Victim = GetTarget();
	if (!Victim)
	{
		return 0.f;
	}
	const FDredgerTuning Tuning = FDredgerTuning::Live();
	if (WantsButt())
	{
		return Tuning.ButtReach * GetSizeScale();
	}
	// He fires from where he means to stand (his cover, his spot, his ground) or backing away (on a deck he soon meets the
	// rail), never on the run to a spot, and only with a clear line.
	const bool bPlaced = Tactic == EDredgerTactic::Covered || Tactic == EDredgerTactic::Exposed || Tactic == EDredgerTactic::Hold
		|| Tactic == EDredgerTactic::BackOff;
	return bPlaced && !IsReloading() && Rounds > 0 && IsShotClear() ? Tuning.FarRange : 0.f;
}

bool ADredgerRifleman::CanStartAttack() const
{
	return !IsReloading() && (Rounds > 0 || WantsButt());
}

void ADredgerRifleman::OnAttackStarted()
{
	const FDredgerTuning Tuning = FDredgerTuning::Live();
	bButtAttack = WantsButt();
	bShotAttack = !bButtAttack;
	Wants.Crouch = 0.f;
	Wants.bShouldered = true;
	if (bButtAttack)
	{
		// Caught close: the butt, quick and short.
		AttackWindup = 0.4f;
		AttackRecovery = 0.5f;
		AttackCooldown = 0.9f;
		Bark(LooterSoundCue::DredgerBark, 0.6f);
		return;
	}
	// The burst's tell: he rises and shoulders through the wind-up, his lenses flare to its end, the muzzle glints
	// (TickBurst draws it); now and then a muffled call.
	AttackWindup = Tuning.RifleWindup;
	AttackRecovery = DredgerRules::BurstRecovery(Tuning);
	AttackCooldown = DredgerRules::BurstCooldown(Tuning);
	ShotsLeft = FMath::Min(Tuning.BurstCount, Rounds);
	BurstClock = 0.f;
	FlareLenses(Tuning.RifleWindup);
	Bark(LooterSoundCue::DredgerBark, 0.2f);
}

void ADredgerRifleman::Strike()
{
	if (bButtAttack)
	{
		ButtStrike();
		return;
	}
	if (bShotAttack && ShotsLeft > 0)
	{
		FireRound();
		BurstClock = FDredgerTuning::Live().BurstGap;
	}
}

void ADredgerRifleman::OnAttackCalledOff()
{
	ShotsLeft = 0;
	bShotAttack = false;
	bButtAttack = false;
}

void ADredgerRifleman::OnTurnPlayedOut()
{
	if (bShotAttack)
	{
		++BurstsHere;
	}
	bShotAttack = false;
	bButtAttack = false;
	ShotsLeft = 0;
	// Empty: he ducks and reloads (his call is the player's moment to push).
	if (Rounds <= 0)
	{
		StartReload();
	}
}

void ADredgerRifleman::DevStartReload()
{
	Rounds = 0;
	if (GetCreatureState() != ECreatureState::Attack)
	{
		StartReload();
	}
}

// ---------------------------------------------------------------------------
// Each frame, hits, death
// ---------------------------------------------------------------------------

void ADredgerRifleman::TickJob(float DeltaSeconds)
{
	TickBurst(DeltaSeconds);
	TickTracers(DeltaSeconds);
	TickReload(DeltaSeconds);
	TickTactics(DeltaSeconds);

	// The body: up to fire, down behind cover (and while reloading there), half down reloading in the open.
	const bool bAttacking = GetCreatureState() == ECreatureState::Attack;
	const bool bInCover = Tactic == EDredgerTactic::Covered && bSpotCovered;
	Wants.bShouldered = bAttacking || Tactic != EDredgerTactic::None;
	if (bAttacking)
	{
		Wants.Crouch = 0.f;
	}
	else if (bInCover)
	{
		Wants.Crouch = 1.f;
	}
	else
	{
		Wants.Crouch = IsReloading() && Tactic != EDredgerTactic::Advance ? 0.45f : 0.f;
	}
	Wants.Lean = 0.f;
}

void ADredgerRifleman::OnHurt(bool bCritical, const FVector& HitLocation)
{
	Super::OnHurt(bCritical, HitLocation);
	// Hit while ducked in his cover: it doesn't cover him (he's flanked): he moves on soon, and quickly.
	if (Tactic == EDredgerTactic::Covered && Wants.Crouch > 0.5f && GetCreatureState() != ECreatureState::Attack)
	{
		FlankedFor = 1.5f;
	}
}

void ADredgerRifleman::OnDied()
{
	Super::OnDied();
	Tracers.Reset();
	ReloadLeft = 0.f;
	ShotsLeft = 0;
	SetKeepingDistance(false);
}

void ADredgerRifleman::OnRespawned()
{
	Super::OnRespawned();
	Rounds = FDredgerTuning::Live().Magazine;
	Tactic = EDredgerTactic::None;
	bHasSpot = false;
	bSearching = false;
	BurstsHere = 0;
	FlankedFor = 0.f;
	ShotsLeft = 0;
	bShotAttack = false;
	bButtAttack = false;
	if (bRifleBuilt)
	{
		Rifle->SetReloadTravel(0.f, true);
	}
	SetKeepingDistance(false);
}

// UCreatureTraitComponent: its life (traits set, a death, a new life), the health events it hears, and the measures every
// trait shares (its body, the effects, who's near to see). Each trait's own play is in its topic file (the header says which).

#include "Creatures/CreatureTraitComponent.h"
#include "AI_Looter_Shooter.h"
#include "Combat/BulletSubsystem.h"
#include "Combat/HealthComponent.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/CreatureTraitRules.h"
#include "Creatures/CreatureTraitSettings.h"
#include "Weapons/WeaponFX.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Misc/Crc.h"

UCreatureTraitComponent::UCreatureTraitComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	// Before the creature's own tick (TG_PostPhysics): an attack's wind-up is quickened on the frame after it starts, before
	// the brain counts any of it.
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

UCreatureTraitComponent* UCreatureTraitComponent::Find(const AActor* Actor)
{
	return Actor ? Actor->FindComponentByClass<UCreatureTraitComponent>() : nullptr;
}

UCreatureTraitComponent* UCreatureTraitComponent::FindOrAdd(ACreatureBase& Creature)
{
	if (UCreatureTraitComponent* Found = Find(&Creature))
	{
		return Found;
	}
	// Every creature makes one (ACreatureBase's "Traits"); this is for a body that somehow has none.
	UCreatureTraitComponent* Made = NewObject<UCreatureTraitComponent>(&Creature, NAME_None, RF_Transient);
	Creature.AddInstanceComponent(Made);
	Made->RegisterComponent();
	// Registered on a creature already in play, it begins with it (the engine does that only for some worlds).
	if (Creature.HasActorBegunPlay() && !Made->HasBegunPlay())
	{
		Made->BeginPlay();
	}
	return Made;
}

TArray<ECreatureTrait> UCreatureTraitComponent::RollTraits(int32 Count, bool bEthereal, const ACreatureBase& For, int32 Seed)
{
	return CreatureTraitRules::Roll(Count, bEthereal, CreatureTraitRules::KindOf(For), Seed);
}

// ---------------------------------------------------------------------------
// Its life
// ---------------------------------------------------------------------------

void UCreatureTraitComponent::BeginPlay()
{
	Super::BeginPlay();
	// Its looks' dice and the side its first blink tries, from its name (the same each run).
	Random.Initialize(static_cast<int32>(FCrc::StrCrc32(GetOwner() ? *GetOwner()->GetName() : TEXT("Traits"))));
	Bind();
	if (!Traits.IsEmpty())
	{
		BeginTraits();
	}
	UpdateTickEnabled();
}

void UCreatureTraitComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Whatever its traits did to it and to others (a rallied pack, its pace, a body hidden mid-blink, a swelling corpse) goes back.
	EndTraits(true);
	Unbind();
	Super::EndPlay(EndPlayReason);
}

void UCreatureTraitComponent::Bind()
{
	if (bBound)
	{
		return;
	}
	const ACreatureBase* Creature = GetCreature();
	UHealthComponent* Life = Creature ? Creature->FindComponentByClass<UHealthComponent>() : nullptr;
	if (!Life)
	{
		return;
	}
	Health = Life;
	Life->OnDamaged.AddDynamic(this, &UCreatureTraitComponent::HandleDamaged);
	Life->OnDeath.AddDynamic(this, &UCreatureTraitComponent::HandleDeath);
	Life->OnHealthChanged.AddDynamic(this, &UCreatureTraitComponent::HandleHealthChanged);
	bDeadSeen = Life->IsDead();
	bBound = true;
}

void UCreatureTraitComponent::Unbind()
{
	UHealthComponent* Life = Health.Get();
	if (bBound && Life)
	{
		Life->OnDamaged.RemoveDynamic(this, &UCreatureTraitComponent::HandleDamaged);
		Life->OnDeath.RemoveDynamic(this, &UCreatureTraitComponent::HandleDeath);
		Life->OnHealthChanged.RemoveDynamic(this, &UCreatureTraitComponent::HandleHealthChanged);
		if (Life->Shield.IsBoundToObject(this))
		{
			Life->Shield.Unbind();
		}
	}
	bBound = false;
}

void UCreatureTraitComponent::SetTraits(TConstArrayView<ECreatureTrait> NewTraits)
{
	TArray<ECreatureTrait> Wanted;
	for (const ECreatureTrait Trait : NewTraits)
	{
		Wanted.AddUnique(Trait);
	}
	if (Wanted == Traits && (Wanted.IsEmpty() || bBegun || !HasBegunPlay()))
	{
		return;
	}
	EndTraits();
	Traits = MoveTemp(Wanted);
	if (HasBegunPlay())
	{
		Bind();
		BeginTraits();
	}
	UpdateTickEnabled();
	if (const ACreatureBase* Creature = GetCreature())
	{
		TArray<FString> Names;
		for (const ECreatureTrait Trait : Traits)
		{
			Names.Add(CreatureTraitRules::Name(Trait));
		}
		UE_LOG(LogLooter, Verbose, TEXT("%s: traits %s"), *Creature->GetName(), Names.IsEmpty() ? TEXT("none") : *FString::Join(Names, TEXT(", ")));
	}
	OnTraitsChanged.Broadcast();
}

void UCreatureTraitComponent::BeginTraits()
{
	ACreatureBase* Creature = GetCreature();
	if (!Creature)
	{
		return;
	}
	// Twice in a row (new traits while it lay dead, then its coming back) is once.
	EndTraits();
	bBegun = true;
	SinceHit = 1000.f;
	RecentHits.Reset();
	SinceBlink = 1000.f;
	BlinkPendingLeft = 0.f;
	Mended = 0.f;
	LastBurst.Reset();
	bBurst = false;
	if (HasTrait(ECreatureTrait::Fleet))
	{
		BeginFleet();
	}
	if (HasTrait(ECreatureTrait::Warded))
	{
		RaiseWard();
	}
	// The rest start from nothing: calm, unmended, nobody rallied yet (its first look round is now).
	RallyScanIn = 0.f;
	RallyPulseIn = 0.f;
	bRallyFighting = false;
	RallyRing.Reset();
	bRightFirst = Random.FRand() < 0.5f;
}

void UCreatureTraitComponent::EndTraits(bool bEndingPlay)
{
	EndFleet();
	Calm();
	ReleaseRally(false);
	DropWard();
	StopMending(false);
	// A corpse already swelling bursts all the same, whatever its traits become as it lies dead (a rank set on its body);
	// a new life or its removal ends it.
	const UHealthComponent* Life = Health.Get();
	if (bEndingPlay || !Life || !Life->IsDead())
	{
		CancelSwell();
	}
	EndBlink();
	Ripples.Reset();
	bDazePending = false;
	bBegun = false;
}

void UCreatureTraitComponent::HandleDamaged(float Damage, bool bCritical, FVector HitLocation, AController* InstigatedBy, AActor* DamageCauser)
{
	if (Damage <= 0.f || Traits.IsEmpty())
	{
		return;
	}
	// Any hit at all: mending breaks off, and its wait starts again.
	SinceHit = 0.f;
	if (bMending)
	{
		StopMending(true);
	}
	if (APawn* Attacker = InstigatedBy ? InstigatedBy->GetPawn() : nullptr)
	{
		LastAttacker = Attacker;
	}
	if (HasTrait(ECreatureTrait::Phasing))
	{
		NoteHitForPhase(Damage);
	}
	if (HasTrait(ECreatureTrait::Vengeful))
	{
		CheckEnrage();
	}
}

void UCreatureTraitComponent::HandleDeath(AController* Killer)
{
	bDeadSeen = true;
	// What it did while it lived ends with it: its rally (the tethers snap), its rage and pace, a mend, a blink cut short
	// (shown where it is), a wind-up. The ward goes quietly: no pop over a corpse.
	ReleaseRally(true);
	Calm();
	EndWindup();
	StopMending(false);
	EndBlink();
	WardFraction = 0.f;
	Ripples.Reset();
	bDazePending = false;
	const ACreatureBase* Creature = GetCreature();
	if (HasTrait(ECreatureTrait::GraveBurst) && Creature && !Creature->IsBossCreature())
	{
		StartSwell();
	}
	UpdateTickEnabled();
}

void UCreatureTraitComponent::HandleHealthChanged(float NewHealth, float MaxHealth)
{
	// Back from the dead (its respawn resets its health): a new life, every trait afresh.
	const UHealthComponent* Life = Health.Get();
	if (bDeadSeen && Life && !Life->IsDead())
	{
		bDeadSeen = false;
		BeginTraits();
		UpdateTickEnabled();
	}
}

void UCreatureTraitComponent::UpdateTickEnabled()
{
	// Ticks while it lives with traits, or while something of theirs is still playing out (a swell, a blast, sparks in the
	// air); a corpse waiting to come back costs nothing.
	const bool bLiving = !Traits.IsEmpty() && !bDeadSeen;
	const bool bPlaying = bSwelling || bBlinking || !Sparks.IsEmpty() || BurstAge < 1.f || WardBreakAge < 1.f;
	SetComponentTickEnabled(bLiving || bPlaying);
}

// ---------------------------------------------------------------------------
// Frame by frame
// ---------------------------------------------------------------------------

void UCreatureTraitComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	Advance(DeltaTime);
}

void UCreatureTraitComponent::Advance(float DeltaSeconds)
{
	if (!GetCreature())
	{
		return;
	}
	const float Dt = FMath::Max(DeltaSeconds, 0.f);
	Clock += Dt;
	SinceHit = FMath::Min(SinceHit + Dt, 1000.f);
	FWeaponFX* Effects = GetEffects();

	TickFleet(Dt, Effects);
	TickVengeance(Dt, Effects);
	TickWard(Dt, Effects);
	TickMending(Dt, Effects);
	TickRally(Dt, Effects);
	TickPhasing(Dt, Effects);
	TickSwell(Dt, Effects);
	Sparks.Tick(Dt, Effects);
	if (Traits.IsEmpty() || bDeadSeen)
	{
		UpdateTickEnabled();
	}
}

// ---------------------------------------------------------------------------
// Shared measures
// ---------------------------------------------------------------------------

ACreatureBase* UCreatureTraitComponent::GetCreature() const
{
	return Cast<ACreatureBase>(GetOwner());
}

FWeaponFX* UCreatureTraitComponent::GetEffects() const
{
	const ACreatureBase* Creature = GetCreature();
	UWorld* World = GetWorld();
	if (!Creature || !World)
	{
		return nullptr;
	}
	// Only with a player near enough to read it (a test level has no player: nothing is drawn there).
	const float Range = UCreatureTraitSettings::Get().LookRange;
	const FVector Here = Creature->GetActorLocation();
	bool bNear = false;
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It && !bNear; ++It)
	{
		const APlayerController* Player = It->Get();
		const APawn* Pawn = Player ? Player->GetPawn() : nullptr;
		bNear = Pawn && FVector::DistSquared(Pawn->GetActorLocation(), Here) <= FMath::Square(Range);
	}
	if (!bNear)
	{
		return nullptr;
	}
	UBulletSubsystem* Bullets = World->GetSubsystem<UBulletSubsystem>();
	if (!Bullets)
	{
		return nullptr;
	}
	FWeaponFX& Effects = Bullets->GetEffects();
	Effects.Initialize(World);
	return &Effects;
}

bool UCreatureTraitComponent::ShowsLooks() const
{
	const ACreatureBase* Creature = GetCreature();
	return Creature && !Creature->IsPoseFrozen() && !Creature->IsHidden();
}

FVector UCreatureTraitComponent::BodyCenter() const
{
	const ACreatureBase* Creature = GetCreature();
	if (!Creature)
	{
		return FVector::ZeroVector;
	}
	// The middle of the body as drawn (a spider's legs spread wide, but its middle is its body), as the death burst finds it.
	const USkeletalMeshComponent* Body = Creature->GetMesh();
	return Body && Body->GetSkinnedAsset() ? FVector(Body->Bounds.Origin) : Creature->GetActorLocation();
}

float UCreatureTraitComponent::BodyRadius() const
{
	const ACreatureBase* Creature = GetCreature();
	if (!Creature)
	{
		return 50.f;
	}
	const float Size = FMath::Max(Creature->GetSizeScale(), 0.2f);
	const UCapsuleComponent* Capsule = Creature->GetCapsuleComponent();
	const float FromCapsule = Capsule ? Capsule->GetScaledCapsuleRadius() * 1.5f : 50.f * Size;
	const USkeletalMeshComponent* Body = Creature->GetMesh();
	const float FromBody = Body && Body->GetSkinnedAsset() ? static_cast<float>(FMath::Max(Body->Bounds.BoxExtent.X, Body->Bounds.BoxExtent.Y)) * 0.85f : 0.f;
	return FMath::Clamp(FMath::Max(FromCapsule, FromBody), 35.f * Size, 240.f * Size);
}

float UCreatureTraitComponent::BodyHalfHeight() const
{
	const ACreatureBase* Creature = GetCreature();
	const UCapsuleComponent* Capsule = Creature ? Creature->GetCapsuleComponent() : nullptr;
	return Capsule ? Capsule->GetScaledCapsuleHalfHeight() : 60.f;
}

FVector UCreatureTraitComponent::Feet() const
{
	const ACreatureBase* Creature = GetCreature();
	return Creature ? Creature->GetActorLocation() - FVector(0.0, 0.0, BodyHalfHeight()) : FVector::ZeroVector;
}

float UCreatureTraitComponent::MaxHealthNow() const
{
	const UHealthComponent* Life = Health.Get();
	return Life ? Life->GetMaxHealth() : 0.f;
}

void UCreatureTraitComponent::ScaleSpeed(ACreatureBase& Creature, float Scale)
{
	Creature.ChaseSpeed *= Scale;
	Creature.WalkSpeed *= Scale;
}

int32 UCreatureTraitComponent::GetRalliedCount() const
{
	int32 Count = 0;
	for (const TWeakObjectPtr<ACreatureBase>& Each : Rallied)
	{
		Count += Each.IsValid() ? 1 : 0;
	}
	return Count;
}

float UCreatureTraitComponent::GetGlowScale() const
{
	const UCreatureTraitSettings& Settings = UCreatureTraitSettings::Get();
	float Scale = 1.f;
	if (bEnraged)
	{
		const FVengefulTraitTuning& Rage = Settings.Vengeful;
		const float Flare = Rage.FlareSeconds > 0.f ? FMath::Clamp(FlareLeft / Rage.FlareSeconds, 0.f, 1.f) : 0.f;
		Scale *= Rage.GlowScale * (1.f + Flare);
	}
	if (bSwelling)
	{
		const float Swell = FMath::Clamp(SwellTime / FMath::Max(Settings.GraveBurst.SwellSeconds, 0.05f), 0.f, 1.f);
		Scale *= 1.f + 2.f * Swell;
	}
	if (bMending)
	{
		Scale *= 1.1f + 0.1f * FMath::Sin(Clock * 4.f);
	}
	return Scale;
}

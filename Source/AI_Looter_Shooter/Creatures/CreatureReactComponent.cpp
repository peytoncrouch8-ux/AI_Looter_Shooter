#include "Creatures/CreatureReactComponent.h"
#include "Combat/BulletSubsystem.h"
#include "Combat/HealthComponent.h"
#include "Creatures/CreatureReactSubsystem.h"
#include "Creatures/CreatureTierLookComponent.h"
#include "Elements/ElementStatusComponent.h"
#include "Player/SlideDust.h"
#include "Weapons/WeaponFX.h"
#include "Components/MeshComponent.h"
#include "Components/SkinnedMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/PackageName.h"

const TCHAR* const UCreatureReactComponent::FlashMaterialPath = TEXT("/Game/Weapons/FX/M_FX_HitFlash.M_FX_HitFlash");
int32 UCreatureReactComponent::QuietDepth = 0;

namespace
{
	/** M_FX_HitFlash's parameters (the dummies' and egg sacs' flash). */
	const FName CreatureFlashColorParam(TEXT("Color"));
	const FName CreatureFlashIntensityParam(TEXT("Intensity"));
	/** White with a little warmth; a crit's warmer, toward the coal. */
	const FLinearColor CreatureFlashColor(1.f, 0.94f, 0.86f);
	const FLinearColor CreatureCritFlashColor(1.f, 0.78f, 0.48f);

	/** The ground under a creature is looked at again this often (s): it changes as it runs, but never every step. */
	constexpr double CreatureGroundLookSeconds = 0.75;

	/** Ash as the light shows it: a pale grey with a little warmth. */
	const FLinearColor CreatureAshColor(0.47f, 0.45f, 0.43f);

	/** The flash's material, looked for once it's needed (a checkout without it flashes nothing). */
	UMaterialInterface* CreatureFlashMaterial()
	{
		static TWeakObjectPtr<UMaterialInterface> Found;
		if (!Found.IsValid()
			&& FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(FString(UCreatureReactComponent::FlashMaterialPath))))
		{
			Found = LoadObject<UMaterialInterface>(nullptr, UCreatureReactComponent::FlashMaterialPath);
		}
		return Found.Get();
	}

	/** The effects every creature's dust goes through (the bullets' FWeaponFX: pooled, instanced puffs), ready. */
	FWeaponFX* CreatureEffects(UWorld* World)
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

UCreatureReactComponent::UCreatureReactComponent()
{
	// The creature's frame moves it on (Advance): nothing of its own ticks.
	PrimaryComponentTick.bCanEverTick = false;
}

void UCreatureReactComponent::BeginPlay()
{
	Super::BeginPlay();
	// Its own dice: a crowd's taps, glances and bubbles never fall in step, nor do its notice beats.
	Random.Initialize(FMath::Rand());
	Seed = Random.FRand();
	if (UHealthComponent* Found = GetOwner() ? GetOwner()->FindComponentByClass<UHealthComponent>() : nullptr)
	{
		Found->OnDamaged.AddDynamic(this, &UCreatureReactComponent::HandleDamaged);
		Health = Found;
	}
}

void UCreatureReactComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UHealthComponent* Found = Health.Get())
	{
		Found->OnDamaged.RemoveDynamic(this, &UCreatureReactComponent::HandleDamaged);
	}
	WearFlash(false);
	Super::EndPlay(EndPlayReason);
}

void UCreatureReactComponent::Advance(float DeltaSeconds)
{
	Notice.Advance(DeltaSeconds);
	Flash.Advance(DeltaSeconds);
	if (!bFlashWorn)
	{
		return;
	}
	if (!Flash.IsWorn())
	{
		WearFlash(false);
		return;
	}
	if (FlashLook)
	{
		FlashLook->SetScalarParameterValue(CreatureFlashIntensityParam, Flash.GetIntensity());
	}
}

void UCreatureReactComponent::HandleStateChanged(ECreatureState OldState, ECreatureState NewState)
{
	if (NewState == ECreatureState::Idle || NewState == ECreatureState::Wander || NewState == ECreatureState::Return
		|| NewState == ECreatureState::Dead)
	{
		// Its hunt is over: the next one may have its beat.
		Notice.EndHunt();
	}
	if (NewState == ECreatureState::Dead)
	{
		// Dead: its body shows its kind's death (ichor, gel, ash), not the killing shot's white flash. The kill's hit-stop
		// held that flash at full on the whole body for a tenth of a second or more (all the slowed frames), a white ghost
		// in place of the death (vfx_shotgun_kill, 2026-10-22).
		Flash.Stop();
		WearFlash(false);
	}
	else if (NewState != ECreatureState::Chase)
	{
		// Lost them, or at them already: the beat is over, the hunt goes on.
		Notice.Stop();
	}
}

void UCreatureReactComponent::HandleDamaged(float Damage, bool bCritical, FVector HitLocation, AController* InstigatedBy, AActor* DamageCauser)
{
	// A shot gets it going at once: no beat for a creature already hurt.
	Notice.Stop();
	// A burn's or the salt's piece (twice a second), or a power's burn tick (FQuietHits), doesn't flash the body white: its
	// status or tint shows on it instead.
	if (!UElementStatusComponent::IsDealingNow() && !IsQuietNow())
	{
		FlashHit(bCritical);
	}
}

// ---------------------------------------------------------------------------
// The hit flash
// ---------------------------------------------------------------------------

void UCreatureReactComponent::FlashHit(bool bCritical)
{
	Flash.Start(bCritical);
	if (!FlashLook)
	{
		UMaterialInterface* Base = CreatureFlashMaterial();
		if (!Base)
		{
			return;
		}
		FlashLook = UMaterialInstanceDynamic::Create(Base, this);
	}
	FlashLook->SetVectorParameterValue(CreatureFlashColorParam, Flash.IsCritical() ? CreatureCritFlashColor : CreatureFlashColor);
	FlashLook->SetScalarParameterValue(CreatureFlashIntensityParam, Flash.GetIntensity());
	if (!bFlashWorn)
	{
		WearFlash(true);
	}
}

void UCreatureReactComponent::WearFlash(bool bWear)
{
	AActor* Owner = GetOwner();
	if (bWear && (!FlashLook || !Owner))
	{
		return;
	}
	if (bWear && FlashMeshes.IsEmpty())
	{
		// Its skinned meshes: the body (and any body built on it). Its worn props (a hat, a pick) are small, and may be
		// drawn by Nanite, which draws no overlay. Never its tier shell (the body drawn again in its tier's glow): the
		// body's own flash shows through it, and a second would double it.
		TInlineComponentArray<USkinnedMeshComponent*> Skinned(Owner);
		for (USkinnedMeshComponent* Mesh : Skinned)
		{
			if (!UCreatureTierLookComponent::IsTierShell(Mesh))
			{
				FlashMeshes.Add(Mesh);
			}
		}
	}
	for (const TWeakObjectPtr<UMeshComponent>& Each : FlashMeshes)
	{
		UMeshComponent* Mesh = Each.Get();
		if (!Mesh)
		{
			continue;
		}
		const UMaterialInterface* Worn = Mesh->OverlayMaterial.Get();
		UMaterialInterface* Own = FlashLook.Get();
		if (bWear && !Worn && Mesh->IsVisible())
		{
			// Never over another overlay (an objective's highlight): that one stays, and this flash isn't seen there.
			Mesh->SetOverlayMaterial(Own);
		}
		else if (!bWear && Worn && Worn == Own)
		{
			Mesh->SetOverlayMaterial(nullptr);
		}
	}
	bFlashWorn = bWear;
}

// ---------------------------------------------------------------------------
// Dust, ash and grave dirt
// ---------------------------------------------------------------------------

uint8 UCreatureReactComponent::GroundNow()
{
	const UWorld* World = GetWorld();
	const double Now = World ? World->GetTimeSeconds() : 0.0;
	if (Now >= GroundLookedAt && Now - GroundLookedAt < CreatureGroundLookSeconds)
	{
		return Ground;
	}
	GroundLookedAt = Now;
	// What its movement stands on (no trace of its own): its material's name, then its mesh's (FSlideDust's names).
	ESlideGround Found = ESlideGround::Dirt;
	const ACharacter* Body = Cast<ACharacter>(GetOwner());
	const UCharacterMovementComponent* Movement = Body ? Body->GetCharacterMovement() : nullptr;
	if (Movement && Movement->CurrentFloor.bBlockingHit)
	{
		Found = FSlideDust::GroundOf(Movement->CurrentFloor.HitResult);
	}
	Ground = static_cast<uint8>(Found);
	return Ground;
}

int32 UCreatureReactComponent::KickFootfall(const FVector& Foot, const FVector& Velocity, float Size, int32 Puffs)
{
	UWorld* World = GetWorld();
	UCreatureReactSubsystem* Reacts = UCreatureReactSubsystem::Get(this);
	if (!World || !Reacts || Puffs <= 0 || !Reacts->IsNearAViewer(Foot, DustSeenWithin))
	{
		return 0;
	}
	// None on water, a haze on wood, the most on dirt.
	const ESlideGround Under = static_cast<ESlideGround>(GroundNow());
	const float Amount = FSlideDust::DustAmount(Under);
	FWeaponFX* Effects = Amount > 0.f ? CreatureEffects(World) : nullptr;
	if (!Effects)
	{
		return 0;
	}
	const int32 Granted = Reacts->TakeFootfalls(Puffs);
	const FLinearColor Color = FSlideDust::DustAlbedo(Under) * Reacts->GetDustLight();
	const FVector Along(Velocity.X, Velocity.Y, 0.0);
	const float Scale = FMath::Max(Size, 0.2f);
	for (int32 Index = 0; Index < Granted; ++Index)
	{
		// A low puff off the foot, carried a little with the run, spreading as it hangs.
		const FVector Where = Foot + FVector(Random.FRandRange(-8.f, 8.f) * Scale, Random.FRandRange(-8.f, 8.f) * Scale, 3.f * Scale);
		const FVector Drift = Along * Random.FRandRange(0.05f, 0.15f)
			+ FVector(Random.FRandRange(-35.f, 35.f), Random.FRandRange(-35.f, 35.f), Random.FRandRange(25.f, 60.f)) * FMath::Sqrt(Scale);
		const float Start = Random.FRandRange(8.f, 12.f) * Scale;
		Effects->SpawnDustPuff(Where, Drift, Color, 0.3f * Amount * Random.FRandRange(0.8f, 1.1f), Start, Start * Random.FRandRange(2.8f, 3.6f),
			Random.FRandRange(0.6f, 0.95f));
	}
	return Granted;
}

int32 UCreatureReactComponent::ShedAsh(const FVector& Where, const FVector& Drift, float Size, int32 Count)
{
	UWorld* World = GetWorld();
	UCreatureReactSubsystem* Reacts = UCreatureReactSubsystem::Get(this);
	FWeaponFX* Effects = Reacts && Count > 0 && Reacts->IsNearAViewer(Where, AshSeenWithin) ? CreatureEffects(World) : nullptr;
	if (!Effects)
	{
		return 0;
	}
	const int32 Granted = Reacts->TakeAsh(Count);
	const FLinearColor Color = CreatureAshColor * Reacts->GetDustLight();
	const float Scale = FMath::Max(Size, 0.2f);
	for (int32 Index = 0; Index < Granted; ++Index)
	{
		// Small flakes that lift off and drift away on the air, thinning as they go.
		const FVector From = Where + FVector(Random.FRandRange(-10.f, 10.f), Random.FRandRange(-10.f, 10.f), Random.FRandRange(-8.f, 8.f)) * Scale;
		const FVector Velocity = Drift + FVector(Random.FRandRange(-25.f, 25.f), Random.FRandRange(-25.f, 25.f), Random.FRandRange(50.f, 110.f));
		const float Start = Random.FRandRange(2.5f, 4.5f) * Scale;
		Effects->SpawnDustPuff(From, Velocity, Color, Random.FRandRange(0.4f, 0.6f), Start, Start * Random.FRandRange(2.5f, 3.5f),
			Random.FRandRange(1.4f, 2.2f));
	}
	return Granted;
}

void UCreatureReactComponent::ThrowGraveDirt(const FVector& Feet, float Size, float Strength)
{
	UWorld* World = GetWorld();
	UCreatureReactSubsystem* Reacts = UCreatureReactSubsystem::Get(this);
	FWeaponFX* Effects = Reacts && Reacts->IsNearAViewer(Feet, AshSeenWithin) ? CreatureEffects(World) : nullptr;
	if (!Effects)
	{
		return;
	}
	// The grave dirt's clods and puffs (the grave wake-up's) where there's earth to throw (not through boards or water), and
	// a ring of dust rolling out over the ground round it.
	const ESlideGround Under = static_cast<ESlideGround>(GroundNow());
	if (Under == ESlideGround::Water)
	{
		return;
	}
	if (Under != ESlideGround::Wood)
	{
		Effects->SpawnDirt(Feet + FVector(0.0, 0.0, 5.0), FVector::UpVector, FMath::Clamp(Strength, 0.f, 1.f));
	}
	const FLinearColor Color = FSlideDust::DustAlbedo(Under == ESlideGround::None ? ESlideGround::Dirt : Under) * Reacts->GetDustLight();
	const float Scale = FMath::Max(Size, 0.2f);
	constexpr int32 Ring = 6;
	for (int32 Index = 0; Index < Ring; ++Index)
	{
		const float Angle = 2.f * UE_PI * (Index + Random.FRandRange(-0.3f, 0.3f)) / Ring;
		const FVector Out(FMath::Cos(Angle), FMath::Sin(Angle), 0.f);
		const float Start = Random.FRandRange(14.f, 20.f) * Scale;
		Effects->SpawnDustPuff(Feet + Out * (25.f * Scale) + FVector(0.0, 0.0, 6.0 * Scale),
			Out * Random.FRandRange(110.f, 190.f) * FMath::Sqrt(Scale) + FVector(0.0, 0.0, Random.FRandRange(40.f, 90.f)), Color,
			0.35f * Strength, Start, Start * Random.FRandRange(3.f, 4.f), Random.FRandRange(0.9f, 1.4f));
	}
}

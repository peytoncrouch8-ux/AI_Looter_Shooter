#include "Player/PlayerThrowComponent.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Combat/GraveSaltBurst.h"
#include "Combat/HealthComponent.h"
#include "Creatures/CreatureBase.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Player/PlayerLocomotionComponent.h"
#include "Player/PlayerMeleeComponent.h"
#include "Player/PlayerThrowSave.h"
#include "Player/PlayerViewComponent.h"
#include "Progression/PlayerProgressionSubsystem.h"
#include "Scenes/SceneSubsystem.h"
#include "Settings/KeyBindingSubsystem.h"
#include "UI/HUD/LooterHUD.h"
#include "Weapons/WeaponBase.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "Engine/LocalPlayer.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

// UPlayerThrowComponent's life, input, the gate, the count and what it reports. The throw itself (its clock, the jar and
// the gun's motion, the release) is in PlayerThrowRelease.cpp.

UPlayerThrowComponent::UPlayerThrowComponent()
{
	// Ticks only while a throw runs.
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

UPlayerThrowComponent* UPlayerThrowComponent::Find(const AActor* Pawn)
{
	return Pawn ? Pawn->FindComponentByClass<UPlayerThrowComponent>() : nullptr;
}

// ---------------------------------------------------------------------------
// Life and input
// ---------------------------------------------------------------------------

void UPlayerThrowComponent::BeginPlay()
{
	Super::BeginPlay();
	if (APawn* Owner = Cast<APawn>(GetOwner()))
	{
		Owner->ReceiveControllerChangedDelegate.AddDynamic(this, &UPlayerThrowComponent::HandleControllerChanged);
		SetupInput(Owner->GetController());
	}
	BindWeaponManager();
}

void UPlayerThrowComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// A gun mid-throw goes back to its hold and is free to fire again.
	EndThrow(false);
	InputBinding.Teardown();
	if (APawn* Owner = Cast<APawn>(GetOwner()))
	{
		Owner->ReceiveControllerChangedDelegate.RemoveDynamic(this, &UPlayerThrowComponent::HandleControllerChanged);
	}
	if (UWeaponManagerComponent* Manager = BoundManager.Get())
	{
		Manager->OnInventoryChanged.RemoveDynamic(this, &UPlayerThrowComponent::HandleInventoryChanged);
	}
	BoundManager.Reset();
	Super::EndPlay(EndPlayReason);
}

void UPlayerThrowComponent::HandleControllerChanged(APawn* Pawn, AController* OldController, AController* NewController)
{
	SetupInput(NewController);
}

void UPlayerThrowComponent::SetupInput(AController* Controller)
{
	InputBinding.Teardown();
	UKeyBindingSubsystem* Bindings = FPawnInputBinding::GetBindings(Controller);
	if (!Bindings || !Bindings->GetGrenadeAction())
	{
		return;
	}
	// The character's own controls (with sprint, crouch, aim and melee), so it only works while the player has their character.
	if (UEnhancedInputComponent* Input = InputBinding.Setup(GetOwner(), Controller, Bindings->GetCharacterContext(), UKeyBindingSubsystem::CharacterContextPriority))
	{
		Input->BindAction(Bindings->GetGrenadeAction(), ETriggerEvent::Started, this, &UPlayerThrowComponent::HandleGrenadePressed);
	}
}

void UPlayerThrowComponent::HandleGrenadePressed()
{
	TryThrow();
}

void UPlayerThrowComponent::BindWeaponManager()
{
	UWeaponManagerComponent* Manager = GetOwner() ? GetOwner()->FindComponentByClass<UWeaponManagerComponent>() : nullptr;
	if (!Manager || Manager == BoundManager.Get())
	{
		return;
	}
	BoundManager = Manager;
	Manager->OnInventoryChanged.AddUniqueDynamic(this, &UPlayerThrowComponent::HandleInventoryChanged);
	// Guns given before play began (a character's starting weapons) count too.
	HandleInventoryChanged();
}

void UPlayerThrowComponent::HandleInventoryChanged()
{
	// The first gun in the player's hands brings the starting two: the tutorial's rifle, the skip's bullpup, or the guns
	// of a session saved before grenades existed (a newer save puts its own count back afterwards, RestoreThrowables).
	const UWeaponManagerComponent* Manager = BoundManager.Get();
	if (!bUnlocked && Manager && Manager->GetWeapons().Num() > 0)
	{
		GiveStartingGrenades();
	}
}

// ---------------------------------------------------------------------------
// Starting a throw
// ---------------------------------------------------------------------------

bool UPlayerThrowComponent::TryThrow()
{
	const EThrowBlock Block = GetBlock();
	if (Block == EThrowBlock::Empty)
	{
		// Out: a soft refusal, and the counter flashes so the eye goes to it.
		LooterSound::Play2D(this, LooterSoundCue::Denied, 0.5f);
		OnGrenadesChanged.Broadcast(Grenades, 0, EGrenadeChange::Denied);
	}
	if (Block != EThrowBlock::None)
	{
		UE_LOG(LogLooter, Verbose, TEXT("Grenade: no throw (%d)"), static_cast<int32>(Block));
		return false;
	}
	StartThrow();
	return true;
}

EThrowBlock UPlayerThrowComponent::GetBlock() const
{
	return FThrowRules::WhyBlocked(GatherGate());
}

FThrowGateInput UPlayerThrowComponent::GatherGate() const
{
	FThrowGateInput In;
	const UWorld* World = GetWorld();
	const APawn* Owner = Cast<APawn>(GetOwner());
	const APlayerController* Player = GetPlayer();
	In.Now = World ? World->GetTimeSeconds() : 0.0;
	In.LastThrow = LastThrowTime;
	In.bThrowing = bThrowing;
	In.bAlive = Owner && Player && !IsOwnerDead();
	const UPlayerLocomotionComponent* Locomotion = Owner ? Owner->FindComponentByClass<UPlayerLocomotionComponent>() : nullptr;
	In.bTraversing = Locomotion && Locomotion->IsTraversing();
	const USceneSubsystem* Scenes = USceneSubsystem::Get(this);
	In.bInScene = Scenes && (Scenes->IsPlaying() || Scenes->IsHoldingPlayer());
	const ALooterHUD* Hud = Player ? Player->GetHUD<ALooterHUD>() : nullptr;
	In.bMenuOpen = (Hud && Hud->IsMenuOpen()) || (World && World->IsPaused());
	const UPlayerMeleeComponent* Melee = UPlayerMeleeComponent::Find(Owner);
	In.bMeleeing = Melee && Melee->IsSwinging();
	In.Grenades = Grenades;
	return In;
}

// ---------------------------------------------------------------------------
// The count
// ---------------------------------------------------------------------------

void UPlayerThrowComponent::SetGrenades(int32 Count, EGrenadeChange Why)
{
	const int32 Clamped = FMath::Clamp(Count, 0, FThrowRules::MaxGrenades);
	const int32 Delta = Clamped - Grenades;
	Grenades = Clamped;
	if (Delta != 0 || Why == EGrenadeChange::Restored)
	{
		OnGrenadesChanged.Broadcast(Grenades, Delta, Why);
	}
}

int32 UPlayerThrowComponent::AddGrenades(int32 Amount, EGrenadeChange Why)
{
	bUnlocked = true;
	int32 Taken = 0;
	const int32 Count = FThrowRules::Add(Grenades, FMath::Max(Amount, 0), &Taken);
	SetGrenades(Count, Why);
	return Taken;
}

bool UPlayerThrowComponent::GiveStartingGrenades()
{
	if (bUnlocked)
	{
		return false;
	}
	// Never fewer than the starting two (a pickup found before the first gun keeps what it gave on top, up to the most).
	bUnlocked = true;
	SetGrenades(FMath::Max(Grenades, FThrowRules::StartingGrenades), EGrenadeChange::Given);
	UE_LOG(LogLooter, Log, TEXT("Grenade: given the starting %d"), Grenades);
	return true;
}

void UPlayerThrowComponent::SaveThrowables(FThrowablesSave& OutSave) const
{
	OutSave.Grenades = Grenades;
	OutSave.bUnlocked = bUnlocked;
}

void UPlayerThrowComponent::RestoreThrowables(const FThrowablesSave& Save)
{
	// What the session had, whatever play began with (the first gun's starting two included).
	bUnlocked = Save.bUnlocked;
	SetGrenades(Save.bUnlocked ? Save.Grenades : 0, EGrenadeChange::Restored);
	// A save made before the first gun but loaded with guns carried (it can't, but an edited one could): the gun's gift.
	HandleInventoryChanged();
}

// ---------------------------------------------------------------------------
// What it reads and reports
// ---------------------------------------------------------------------------

void UPlayerThrowComponent::GetAim(FVector& OutEye, FVector& OutLook) const
{
	const APawn* Owner = Cast<APawn>(GetOwner());
	if (!Owner)
	{
		OutEye = FVector::ZeroVector;
		OutLook = FVector::ForwardVector;
		return;
	}
	// The body's own eye (the first-person camera rides it in every view), never a third-person camera behind the
	// shoulder; and the way the player looks.
	const UCameraComponent* Eye = UPlayerViewComponent::FindFirstPersonCamera(Owner);
	OutEye = Eye ? Eye->GetComponentLocation() : Owner->GetPawnViewLocation();
	OutLook = (Owner->GetController() ? Owner->GetControlRotation() : Owner->GetActorRotation()).Vector();
}

int32 UPlayerThrowComponent::CountTargetsAhead(float Range, float HalfConeDegrees) const
{
	const UWorld* World = GetWorld();
	if (!World || Range <= 0.f)
	{
		return 0;
	}
	FVector Eye;
	FVector Look;
	GetAim(Eye, Look);
	const FVector2D Forward = FVector2D(Look.X, Look.Y).GetSafeNormal();
	const float MinCos = FMath::Cos(FMath::DegreesToRadians(FMath::Clamp(HalfConeDegrees, 0.f, 180.f)));
	FCollisionObjectQueryParams Objects;
	Objects.AddObjectTypesToQuery(ECC_Pawn);
	TArray<FOverlapResult> Overlaps;
	World->OverlapMultiByObjectType(Overlaps, Eye, FQuat::Identity, Objects, FCollisionShape::MakeSphere(Range),
		FCollisionQueryParams(SCENE_QUERY_STAT(GrenadeCrowd), false, GetOwner()));
	TArray<const AActor*, TInlineAllocator<16>> Counted;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		const ACreatureBase* Creature = Cast<ACreatureBase>(Overlap.GetActor());
		if (!Creature || Counted.Contains(Creature) || !GraveSaltBurst::CanHurt(Creature, GetOwner()))
		{
			continue;
		}
		const FVector To = Creature->GetActorLocation() - Eye;
		const FVector2D Flat = FVector2D(To.X, To.Y).GetSafeNormal();
		if (!Forward.IsZero() && FVector2D::DotProduct(Forward, Flat) < MinCos)
		{
			continue;
		}
		if (GraveSaltBurst::CanSee(*World, Eye, GraveSaltBurst::BodyOf(*Creature), Creature, GetOwner()))
		{
			Counted.Add(Creature);
		}
	}
	return Counted.Num();
}

void UPlayerThrowComponent::NotifyBurst(const FGraveSaltBurstResult& Result)
{
	// The hit marker for each body, as a gun's OnHit tells of its bullets (the kill's red marker for the dead).
	for (int32 Index = 0; Index < Result.HitResults.Num(); ++Index)
	{
		OnGrenadeHit.Broadcast(Result.HitResults[Index], Result.Damages.IsValidIndex(Index) ? Result.Damages[Index] : 0.f, false);
	}
	OnGrenadeBurst.Broadcast(Result.Hits, Result.Kills);
}

APlayerController* UPlayerThrowComponent::GetPlayer() const
{
	const APawn* Owner = Cast<APawn>(GetOwner());
	APlayerController* Player = Owner ? Cast<APlayerController>(Owner->GetController()) : nullptr;
	return Player && Player->IsLocalController() ? Player : nullptr;
}

AWeaponBase* UPlayerThrowComponent::GetWeaponInHand() const
{
	const UWeaponManagerComponent* Manager = GetOwner() ? GetOwner()->FindComponentByClass<UWeaponManagerComponent>() : nullptr;
	return Manager ? Manager->GetActiveWeapon() : nullptr;
}

bool UPlayerThrowComponent::IsOwnerDead() const
{
	const UHealthComponent* Health = GetOwner() ? GetOwner()->FindComponentByClass<UHealthComponent>() : nullptr;
	return Health && Health->IsDead();
}

float UPlayerThrowComponent::GetLevelScale() const
{
	// The burst grows with the player as enemies and guns grow with theirs, so it takes the same share at equal levels.
	const APlayerController* Player = GetPlayer();
	const ULocalPlayer* Local = Player ? Player->GetLocalPlayer() : nullptr;
	const UPlayerProgressionSubsystem* Progression = Local ? Local->GetSubsystem<UPlayerProgressionSubsystem>() : nullptr;
	return UPlayerProgressionSubsystem::GetLevelRules().EnemyScale(Progression ? Progression->GetLevel() : 1);
}

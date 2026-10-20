#include "Embers/EmberComponent.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Combat/HealthComponent.h"
#include "Core/LooterCharacter.h"
#include "Embers/EmberDefinition.h"
#include "Embers/EmberPower.h"
#include "Embers/HeroDefinition.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Player/PlayerLocomotionComponent.h"
#include "Player/PlayerMeleeComponent.h"
#include "Player/PlayerThrowComponent.h"
#include "Progression/PlayerProgressionSubsystem.h"
#include "Scenes/SceneSubsystem.h"
#include "Settings/KeyBindingSubsystem.h"
#include "UI/HUD/LooterHUD.h"
#include "EnhancedInputComponent.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

// UEmberComponent: the hero's powers, the Ember key's gate, the two clocks, the save and the reports. What each power does
// is its UEmberPower (Embers/Powers/*).

namespace
{
	/** The sounds the component itself plays (by name, so the cue list can grow without this file: Audio/LooterSoundCues.h). */
	const FName ReadyCue(TEXT("Ember.Ready"));
	const FName DeniedCue(TEXT("Ember.Denied"));

	/** The local player controlling Owner, or null. */
	APlayerController* LocalPlayerOf(const AActor* Owner)
	{
		const APawn* Pawn = Cast<APawn>(Owner);
		APlayerController* Player = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
		return Player && Player->IsLocalController() ? Player : nullptr;
	}

	/** A scene plays or holds the player. */
	bool IsInScene(const UObject* Context)
	{
		const USceneSubsystem* Scenes = USceneSubsystem::Get(Context);
		return Scenes && (Scenes->IsPlaying() || Scenes->IsHoldingPlayer());
	}
}

UEmberComponent::UEmberComponent()
{
	// Ticks only while a power runs or cools down.
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

UEmberComponent* UEmberComponent::Find(const AActor* Pawn)
{
	return Pawn ? Pawn->FindComponentByClass<UEmberComponent>() : nullptr;
}

const TCHAR* UEmberComponent::DefaultHeroPath()
{
	return TEXT("/Game/Data/Embers/DA_Hero_Ellis.DA_Hero_Ellis");
}

// ---------------------------------------------------------------------------
// Life and input
// ---------------------------------------------------------------------------

void UEmberComponent::BeginPlay()
{
	Super::BeginPlay();
	BindHero();
	if (APawn* Owner = Cast<APawn>(GetOwner()))
	{
		Owner->ReceiveControllerChangedDelegate.AddDynamic(this, &UEmberComponent::HandleControllerChanged);
		SetupInput(Owner->GetController());
	}
}

void UEmberComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// A power running when the level ends leaves the world as it found it.
	Interrupt();
	InputBinding.Teardown();
	if (APawn* Owner = Cast<APawn>(GetOwner()))
	{
		Owner->ReceiveControllerChangedDelegate.RemoveDynamic(this, &UEmberComponent::HandleControllerChanged);
	}
	Super::EndPlay(EndPlayReason);
}

void UEmberComponent::HandleControllerChanged(APawn* Pawn, AController* OldController, AController* NewController)
{
	SetupInput(NewController);
}

void UEmberComponent::SetupInput(AController* Controller)
{
	InputBinding.Teardown();
	UKeyBindingSubsystem* Bindings = FPawnInputBinding::GetBindings(Controller);
	if (!Bindings || !Bindings->GetEmberAction())
	{
		return;
	}
	// The character's own controls (with the grenade, melee and aim), so it only works while the player has their character.
	if (UEnhancedInputComponent* Input = InputBinding.Setup(GetOwner(), Controller, Bindings->GetCharacterContext(), UKeyBindingSubsystem::CharacterContextPriority))
	{
		Input->BindAction(Bindings->GetEmberAction(), ETriggerEvent::Started, this, &UEmberComponent::HandleEmberPressed);
	}
}

void UEmberComponent::HandleEmberPressed()
{
	TryUse();
}

// Binding the hero (BindHero, RebindHero, FindHeroToBind) is in EmberComponentHero.cpp.

// ---------------------------------------------------------------------------
// The powers owned and the one equipped
// ---------------------------------------------------------------------------

const TArray<TObjectPtr<UEmberDefinition>>& UEmberComponent::GetOwned() const
{
	static const TArray<TObjectPtr<UEmberDefinition>> None;
	return Hero ? Hero->Powers : None;
}

const UEmberDefinition* UEmberComponent::GetEquipped() const
{
	const TArray<TObjectPtr<UEmberDefinition>>& Owned = GetOwned();
	return Owned.IsValidIndex(EquippedIndex) ? Owned[EquippedIndex].Get() : nullptr;
}

FName UEmberComponent::GetEquippedId() const
{
	const UEmberDefinition* Def = GetEquipped();
	return Def ? Def->Id : NAME_None;
}

bool UEmberComponent::Equip(FName Id)
{
	BindHero();
	const TArray<TObjectPtr<UEmberDefinition>>& Owned = GetOwned();
	const int32 Index = Owned.IndexOfByPredicate([Id](const UEmberDefinition* Def) { return Def && Def->Id == Id; });
	return Index != INDEX_NONE && EquipIndex(Index);
}

bool UEmberComponent::EquipIndex(int32 Index)
{
	BindHero();
	const TArray<TObjectPtr<UEmberDefinition>>& Owned = GetOwned();
	// A running power finishes first: swapping mid-power would leave its world half changed.
	if (!Owned.IsValidIndex(Index) || !Owned[Index] || Index == EquippedIndex || State == EEmberState::Active)
	{
		return false;
	}
	EquippedIndex = Index;
	// The wait carries over (held to the new power's own), so a swap is never a way round a cooldown.
	if (CooldownLeft > 0.f)
	{
		CooldownTotal = FMath::Max(Owned[Index]->CooldownSeconds, KINDA_SMALL_NUMBER);
		CooldownLeft = FMath::Min(CooldownLeft, CooldownTotal);
	}
	UE_LOG(LogLooter, Log, TEXT("Embers: %s equipped"), *Owned[Index]->Id.ToString());
	OnEquipped.Broadcast(Owned[Index]->Id);
	return true;
}

UEmberPower* UEmberComponent::GetActivePower() const
{
	return Powers.IsValidIndex(ActiveIndex) ? Powers[ActiveIndex].Get() : nullptr;
}

UEmberPower* UEmberComponent::GetPowerFor(FName Id) const
{
	const TArray<TObjectPtr<UEmberDefinition>>& Owned = GetOwned();
	const int32 Index = Owned.IndexOfByPredicate([Id](const UEmberDefinition* Def) { return Def && Def->Id == Id; });
	return Powers.IsValidIndex(Index) ? Powers[Index].Get() : nullptr;
}

// ---------------------------------------------------------------------------
// Using it
// ---------------------------------------------------------------------------

EEmberDenied UEmberComponent::TryUse()
{
	BindHero();
	EEmberDenied Why = GetBlock();
	if (Why == EEmberDenied::None)
	{
		// The power's own say last: nothing to dash into, no room, and so on.
		const UEmberPower* Power = Powers[EquippedIndex];
		Why = Power->CanActivate(MakeContext());
		if (Why == EEmberDenied::Cooldown || Why == EEmberDenied::Active)
		{
			// Those are the component's to say; a power that refuses is Refused.
			Why = EEmberDenied::Refused;
		}
	}
	if (Why != EEmberDenied::None)
	{
		// A soft click for a press the player can do something about; nothing while dead or the body is spoken for (a scene,
		// a menu, a mantle: the press was never really the game's).
		if (Why != EEmberDenied::Dead && Why != EEmberDenied::Busy)
		{
			LooterSound::Play2D(this, DeniedCue, 0.5f);
		}
		UE_LOG(LogLooter, Verbose, TEXT("Embers: no use (%s)"), *UEnum::GetValueAsString(Why));
		OnDenied.Broadcast(Why);
		return Why;
	}
	Fire(EquippedIndex);
	return EEmberDenied::None;
}

EEmberDenied UEmberComponent::GetBlock() const
{
	if (!GetEquipped() || !Powers.IsValidIndex(EquippedIndex) || !Powers[EquippedIndex])
	{
		return EEmberDenied::NoPower;
	}
	if (IsOwnerDead())
	{
		return EEmberDenied::Dead;
	}

	// The body spoken for, as the grenade's gate judges it: a scene, a menu or a paused game, a mantle or vault, a gun swap,
	// a melee strike or a grenade on its way. Without a player controlling the owner (a test's bare body) only the world's
	// own (a scene, a pause) count.
	const AActor* Owner = GetOwner();
	const UWorld* World = GetWorld();
	if (IsInScene(this) || (World && World->IsPaused()))
	{
		return EEmberDenied::Busy;
	}
	const APlayerController* Player = LocalPlayerOf(Owner);
	const ALooterHUD* Hud = Player ? Player->GetHUD<ALooterHUD>() : nullptr;
	if (Hud && Hud->IsMenuOpen())
	{
		return EEmberDenied::Busy;
	}
	const UPlayerLocomotionComponent* Locomotion = Owner ? Owner->FindComponentByClass<UPlayerLocomotionComponent>() : nullptr;
	const UWeaponManagerComponent* Manager = Owner ? Owner->FindComponentByClass<UWeaponManagerComponent>() : nullptr;
	const UPlayerMeleeComponent* Melee = UPlayerMeleeComponent::Find(Owner);
	const UPlayerThrowComponent* Throw = UPlayerThrowComponent::Find(Owner);
	if ((Locomotion && Locomotion->IsTraversing()) || (Manager && Manager->IsSwapping()) || (Melee && Melee->IsSwinging())
		|| (Throw && Throw->IsThrowing()))
	{
		return EEmberDenied::Busy;
	}

	if (State == EEmberState::Active)
	{
		return EEmberDenied::Active;
	}
	if (CooldownLeft > 0.f)
	{
		return EEmberDenied::Cooldown;
	}
	return EEmberDenied::None;
}

void UEmberComponent::Fire(int32 Index)
{
	const TArray<TObjectPtr<UEmberDefinition>>& Owned = GetOwned();
	UEmberPower* Power = Powers.IsValidIndex(Index) ? Powers[Index].Get() : nullptr;
	const UEmberDefinition* Def = Owned.IsValidIndex(Index) ? Owned[Index].Get() : nullptr;
	if (!Power || !Def)
	{
		return;
	}
	// One power at a time: a console use over a running one ends it first.
	if (ActiveIndex != INDEX_NONE)
	{
		EndActive(true);
	}

	// Both clocks start at the use: the cooldown counts from now, not from the end of the duration.
	ActiveIndex = Index;
	ActiveTotal = ActiveLeft = FMath::Max(Def->DurationSeconds, 0.f);
	CooldownTotal = CooldownLeft = FMath::Max(Def->CooldownSeconds, 0.f);
	const bool bLasts = ActiveLeft > 0.f;
	Power->MarkRunning(bLasts);
	const FEmberContext Context = MakeContext();
	Power->Activate(Context);
	UE_LOG(LogLooter, Log, TEXT("Embers: %s used (%.1f s, cooldown %.1f s, level scale %.2f)"), *Def->Id.ToString(), ActiveTotal, CooldownTotal,
		Context.LevelScale);
	OnUsed.Broadcast(Def->Id);

	if (bLasts)
	{
		SetState(EEmberState::Active);
	}
	else
	{
		// An instant power is over the moment it's used.
		Power->End(Context, false);
		ActiveIndex = INDEX_NONE;
		ActiveLeft = ActiveTotal = 0.f;
		SetState(CooldownLeft > 0.f ? EEmberState::Cooldown : EEmberState::Ready);
	}
	SetComponentTickEnabled(State != EEmberState::Ready);
}

void UEmberComponent::EndActive(bool bInterrupted)
{
	UEmberPower* Power = GetActivePower();
	if (!Power)
	{
		ActiveIndex = INDEX_NONE;
		return;
	}
	const FEmberContext Context = MakeContext();
	Power->End(Context, bInterrupted);
	Power->MarkRunning(false);
	ActiveIndex = INDEX_NONE;
	ActiveLeft = 0.f;
	if (CooldownLeft > 0.f)
	{
		SetState(EEmberState::Cooldown);
		return;
	}
	// Its cooldown already ran out while it lasted: ready again at once (and said so, unless death or a scene cut it short).
	SetState(EEmberState::Ready);
	if (!bInterrupted)
	{
		LooterSound::Play2D(this, ReadyCue);
	}
}

void UEmberComponent::Interrupt()
{
	if (ActiveIndex != INDEX_NONE)
	{
		UE_LOG(LogLooter, Log, TEXT("Embers: the running power was cut short"));
		EndActive(true);
	}
}

void UEmberComponent::Finish()
{
	if (ActiveIndex != INDEX_NONE)
	{
		UE_LOG(LogLooter, Log, TEXT("Embers: the running power finished early"));
		ActiveLeft = 0.f;
		EndActive(false);
		SetComponentTickEnabled(State != EEmberState::Ready);
	}
}

void UEmberComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// The cooldown counts from the use, through the power's own duration.
	CooldownLeft = FMath::Max(CooldownLeft - DeltaTime, 0.f);

	if (State == EEmberState::Active)
	{
		// Death or a scene ends a running power at once (it leaves the world as it found it).
		if (IsOwnerDead() || IsInScene(this))
		{
			Interrupt();
		}
		else if (UEmberPower* Power = GetActivePower())
		{
			Power->Tick(MakeContext(), DeltaTime);
			ActiveLeft = FMath::Max(ActiveLeft - DeltaTime, 0.f);
			// The power may have ended itself in its Tick (through Interrupt); otherwise its time decides.
			if (ActiveIndex != INDEX_NONE && ActiveLeft <= 0.f)
			{
				EndActive(false);
			}
		}
		else
		{
			// Its object went (it can't, but never stay Active with nothing running).
			EndActive(true);
		}
	}
	else if (State == EEmberState::Cooldown && CooldownLeft <= 0.f)
	{
		SetState(EEmberState::Ready);
		LooterSound::Play2D(this, ReadyCue);
	}

	if (State == EEmberState::Ready)
	{
		CooldownTotal = 0.f;
		SetComponentTickEnabled(false);
	}
}

// ---------------------------------------------------------------------------
// The clocks, the context and the level
// ---------------------------------------------------------------------------

float UEmberComponent::GetCooldownFraction() const
{
	return CooldownTotal > 0.f ? FMath::Clamp(CooldownLeft / CooldownTotal, 0.f, 1.f) : 0.f;
}

float UEmberComponent::GetActiveFraction() const
{
	return State == EEmberState::Active && ActiveTotal > 0.f ? FMath::Clamp(ActiveLeft / ActiveTotal, 0.f, 1.f) : 0.f;
}

void UEmberComponent::SetState(EEmberState NewState)
{
	if (State == NewState)
	{
		return;
	}
	State = NewState;
	OnStateChanged.Broadcast(State);
}

FEmberContext UEmberComponent::MakeContext() const
{
	FEmberContext Context;
	Context.Character = Cast<ALooterCharacter>(GetOwner());
	// Powers are given the component itself to work with (Within = EmberComponent); this function only reads.
	Context.Component = const_cast<UEmberComponent*>(this);
	const TArray<TObjectPtr<UEmberDefinition>>& Owned = GetOwned();
	const int32 Index = ActiveIndex != INDEX_NONE ? ActiveIndex : EquippedIndex;
	Context.Definition = Owned.IsValidIndex(Index) ? Owned[Index].Get() : nullptr;
	Context.LevelScale = GetLevelScale();
	return Context;
}

float UEmberComponent::GetLevelScale() const
{
	// A power grows with the player as enemies and guns grow with theirs, so it takes the same share at equal levels.
	const APlayerController* Player = LocalPlayerOf(GetOwner());
	const ULocalPlayer* Local = Player ? Player->GetLocalPlayer() : nullptr;
	const UPlayerProgressionSubsystem* Progression = Local ? Local->GetSubsystem<UPlayerProgressionSubsystem>() : nullptr;
	return UPlayerProgressionSubsystem::GetLevelRules().EnemyScale(Progression ? Progression->GetLevel() : 1);
}

bool UEmberComponent::IsOwnerDead() const
{
	const UHealthComponent* Health = GetOwner() ? GetOwner()->FindComponentByClass<UHealthComponent>() : nullptr;
	return Health && Health->IsDead();
}

// ---------------------------------------------------------------------------
// Saving
// ---------------------------------------------------------------------------

void UEmberComponent::SaveEmbers(FEmberSave& OutSave) const
{
	// A running power isn't saved (a load finds it ended), but its cooldown counts from the use, so it's right as it stands.
	OutSave.Equipped = GetEquippedId();
	OutSave.CooldownLeft = CooldownLeft;
}

void UEmberComponent::RestoreEmbers(const FEmberSave& Save)
{
	BindHero();
	Interrupt();
	const TArray<TObjectPtr<UEmberDefinition>>& Owned = GetOwned();
	if (Owned.IsEmpty())
	{
		return;
	}
	// The saved one by its id; one the hero no longer owns (or none saved) gives the hero's first.
	int32 Index = Owned.IndexOfByPredicate([&Save](const UEmberDefinition* Def) { return Def && Def->Id == Save.Equipped; });
	if (Index == INDEX_NONE)
	{
		Index = 0;
	}
	EquippedIndex = Index;
	const UEmberDefinition* Def = Owned[Index];
	const float Cooldown = Def ? FMath::Max(Def->CooldownSeconds, 0.f) : 0.f;
	CooldownLeft = FMath::Clamp(Save.CooldownLeft, 0.f, Cooldown);
	CooldownTotal = CooldownLeft > 0.f ? Cooldown : 0.f;
	ActiveLeft = ActiveTotal = 0.f;
	SetState(CooldownLeft > 0.f ? EEmberState::Cooldown : EEmberState::Ready);
	SetComponentTickEnabled(State != EEmberState::Ready);
	OnEquipped.Broadcast(GetEquippedId());
}

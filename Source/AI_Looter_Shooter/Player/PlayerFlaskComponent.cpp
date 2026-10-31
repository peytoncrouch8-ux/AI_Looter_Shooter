#include "Player/PlayerFlaskComponent.h"
#include "AI_Looter_Shooter.h"
#include "Bosses/BossComponent.h"
#include "Combat/FightState.h"
#include "Combat/HealthComponent.h"
#include "Creatures/CreatureBase.h"
#include "Embers/EmberComponent.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Player/FlaskFX.h"
#include "Player/PlayerFlaskSave.h"
#include "Player/PlayerLocomotionComponent.h"
#include "Player/PlayerMeleeComponent.h"
#include "Player/PlayerThrowComponent.h"
#include "Progression/PlayerProgressionSubsystem.h"
#include "Scenes/SceneSubsystem.h"
#include "Settings/KeyBindingSubsystem.h"
#include "UI/HUD/LooterHUD.h"
#include "Weapons/WeaponBase.h"
#include "EnhancedInputComponent.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

// UPlayerFlaskComponent's life, input, the gate, the charges and the soul that fills them, the save and the kills. The drink
// itself (its clock, the heal, the gun's dip) is in PlayerFlaskDrink.cpp, the flask seen in either hand in PlayerFlaskModels.cpp.

UPlayerFlaskComponent::UPlayerFlaskComponent()
{
	// Ticks only while a drink runs.
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

UPlayerFlaskComponent* UPlayerFlaskComponent::Find(const AActor* Pawn)
{
	return Pawn ? Pawn->FindComponentByClass<UPlayerFlaskComponent>() : nullptr;
}

bool UPlayerFlaskComponent::IsDrinkingActor(const AActor* Pawn)
{
	const UPlayerFlaskComponent* Flask = Find(Pawn);
	return Flask && Flask->IsDrinking();
}

// ---------------------------------------------------------------------------
// Life and input
// ---------------------------------------------------------------------------

void UPlayerFlaskComponent::BeginPlay()
{
	Super::BeginPlay();
	if (APawn* Owner = Cast<APawn>(GetOwner()))
	{
		Owner->ReceiveControllerChangedDelegate.AddDynamic(this, &UPlayerFlaskComponent::HandleControllerChanged);
		SetupInput(Owner->GetController());
	}
	BindProgression();
}

void UPlayerFlaskComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// A gun mid-drink goes back to its hold and is free to fire again; a heal still pouring stops.
	EndDrink(false);
	bPouring = false;
	InputBinding.Teardown();
	if (APawn* Owner = Cast<APawn>(GetOwner()))
	{
		Owner->ReceiveControllerChangedDelegate.RemoveDynamic(this, &UPlayerFlaskComponent::HandleControllerChanged);
	}
	UnbindProgression();
	Super::EndPlay(EndPlayReason);
}

void UPlayerFlaskComponent::HandleControllerChanged(APawn* Pawn, AController* OldController, AController* NewController)
{
	SetupInput(NewController);
	// Possessed after play began (the usual way for the player): its kills are the new controller's.
	UnbindProgression();
	BindProgression();
}

void UPlayerFlaskComponent::SetupInput(AController* Controller)
{
	InputBinding.Teardown();
	UKeyBindingSubsystem* Bindings = FPawnInputBinding::GetBindings(Controller);
	if (!Bindings || !Bindings->GetFlaskAction())
	{
		return;
	}
	// The character's own controls (with sprint, crouch, aim, melee and the grenade), so it only works while the player has
	// their character.
	if (UEnhancedInputComponent* Input = InputBinding.Setup(GetOwner(), Controller, Bindings->GetCharacterContext(), UKeyBindingSubsystem::CharacterContextPriority))
	{
		Input->BindAction(Bindings->GetFlaskAction(), ETriggerEvent::Started, this, &UPlayerFlaskComponent::HandleFlaskPressed);
	}
}

void UPlayerFlaskComponent::HandleFlaskPressed()
{
	TryDrink();
}

void UPlayerFlaskComponent::BindProgression()
{
	if (Progression.IsValid())
	{
		return;
	}
	const APawn* Owner = Cast<APawn>(GetOwner());
	const APlayerController* Player = Owner ? Cast<APlayerController>(Owner->GetController()) : nullptr;
	const ULocalPlayer* Local = Player ? Player->GetLocalPlayer() : nullptr;
	UPlayerProgressionSubsystem* Found = Local ? Local->GetSubsystem<UPlayerProgressionSubsystem>() : nullptr;
	if (!Found)
	{
		return;
	}
	Progression = Found;
	// Every kill, experience or not (OnKillXP stays quiet at the top level and for kills worth none).
	KillHandle = Found->OnKill.AddUObject(this, &UPlayerFlaskComponent::HandleKill);
}

void UPlayerFlaskComponent::UnbindProgression()
{
	if (UPlayerProgressionSubsystem* Kills = Progression.Get())
	{
		Kills->OnKill.Remove(KillHandle);
	}
	KillHandle.Reset();
	Progression.Reset();
}

void UPlayerFlaskComponent::HandleKill(const AActor* Victim)
{
	GiveKillSoul(Victim);
}

// ---------------------------------------------------------------------------
// The gate
// ---------------------------------------------------------------------------

bool UPlayerFlaskComponent::TryDrink()
{
	const EFlaskBlock Block = GetBlock();
	APawn* Owner = Cast<APawn>(GetOwner());
	// Said only when it's news the player can act on: dry (go and kill something), or nothing to heal. The rest (busy, a menu,
	// a scene) are quiet: the press simply waits for the hands.
	if (Block == EFlaskBlock::Empty || Block == EFlaskBlock::FullHealth)
	{
		const bool bFull = Block == EFlaskBlock::FullHealth;
		if (Owner)
		{
			FlaskFX::Denied(*Owner, bFull);
		}
		SetCharges(Charges, bFull ? EFlaskChange::DeniedFull : EFlaskChange::DeniedEmpty);
	}
	if (Block != EFlaskBlock::None)
	{
		UE_LOG(LogLooter, Verbose, TEXT("Flask: no drink (%d)"), static_cast<int32>(Block));
		return false;
	}
	StartDrink();
	return true;
}

EFlaskBlock UPlayerFlaskComponent::GetBlock() const
{
	return FFlaskRules::WhyBlocked(GatherGate());
}

FFlaskGateInput UPlayerFlaskComponent::GatherGate() const
{
	FFlaskGateInput In;
	const UWorld* World = GetWorld();
	const APawn* Owner = Cast<APawn>(GetOwner());
	const UHealthComponent* Health = GetHealth();
	In.Now = World ? World->GetTimeSeconds() : 0.0;
	In.LastDrinkEnd = LastDrinkEnd;
	In.bDrinking = bDrinking;
	// No local player is needed (the key only comes from one anyway): a test's holder drinks the same way.
	In.bAlive = Owner && Health && !Health->IsDead();
	const USceneSubsystem* Scenes = USceneSubsystem::Get(this);
	In.bInScene = Scenes && (Scenes->IsPlaying() || Scenes->IsHoldingPlayer());
	const APlayerController* Player = GetPlayer();
	const ALooterHUD* Hud = Player ? Player->GetHUD<ALooterHUD>() : nullptr;
	In.bMenuOpen = (Hud && Hud->IsMenuOpen()) || (World && World->IsPaused());
	const UPlayerLocomotionComponent* Locomotion = Owner ? Owner->FindComponentByClass<UPlayerLocomotionComponent>() : nullptr;
	In.bTraversing = Locomotion && Locomotion->IsTraversing();
	// One thing in the hands at a time: a jar mid-throw, the tool mid-strike, a power's cast, a gun on its way between
	// hands, or both hands stroking through the water (the gun lowered away: nothing to tip the flask with).
	const UPlayerThrowComponent* Throw = UPlayerThrowComponent::Find(Owner);
	const UPlayerMeleeComponent* Melee = UPlayerMeleeComponent::Find(Owner);
	const UEmberComponent* Embers = UEmberComponent::Find(Owner);
	const UWeaponManagerComponent* Manager = Owner ? Owner->FindComponentByClass<UWeaponManagerComponent>() : nullptr;
	In.bHandsBusy = (Throw && Throw->IsThrowing()) || (Melee && Melee->IsSwinging()) || (Embers && Embers->IsCasting())
		|| (Manager && Manager->IsSwapping()) || UPlayerLocomotionComponent::IsSwimmingActor(Owner);
	In.Charges = Charges;
	// Half a point short counts as full, as the soul-motes count it: no charge spent on a scratch.
	In.bFullHealth = Health && Health->GetHealth() >= Health->GetMaxHealth() - 0.5f;
	return In;
}

// ---------------------------------------------------------------------------
// The charges and soul
// ---------------------------------------------------------------------------

float UPlayerFlaskComponent::GetDrinkAlpha() const
{
	return bDrinking ? FMath::Clamp(Clock / FFlaskRules::DrinkSeconds, 0.f, 1.f) : 0.f;
}

bool UPlayerFlaskComponent::IsHealing() const
{
	return bPouring && Clock >= FFlaskRules::SipAt && Clock < FFlaskRules::HealEndSeconds;
}

float UPlayerFlaskComponent::GetSoulShare() const
{
	return IsFull() ? 0.f : FMath::Clamp(Soul / FFlaskRules::SoulPerCharge, 0.f, 1.f);
}

void UPlayerFlaskComponent::SetCharges(int32 Count, EFlaskChange Why)
{
	const int32 Clamped = FMath::Clamp(Count, 0, FFlaskRules::MaxCharges);
	const int32 Delta = Clamped - Charges;
	Charges = Clamped;
	if (IsFull())
	{
		Soul = 0.f;
	}
	const bool bAlwaysTold = Why == EFlaskChange::Restored || Why == EFlaskChange::Refilled || Why == EFlaskChange::DeniedEmpty
		|| Why == EFlaskChange::DeniedFull || Why == EFlaskChange::Soul;
	if (Delta != 0 || bAlwaysTold)
	{
		OnFlaskChanged.Broadcast(Charges, Delta, Why);
	}
	UpdateGlow();
}

void UPlayerFlaskComponent::AddSoul(float Added)
{
	if (Added <= 0.f || IsFull())
	{
		return;
	}
	int32 After = Charges;
	float Left = Soul;
	const int32 Filled = FFlaskRules::Pour(After, Left, Added);
	// The soul first, so whoever hears of a filled charge already reads the next one's level.
	Soul = Left;
	if (Filled <= 0)
	{
		SetCharges(Charges, EFlaskChange::Soul);
		return;
	}
	// One change a charge, so the HUD lights each pip in turn; one chime for the lot (a boss's three at once ring once).
	const int32 Before = Charges;
	for (int32 Index = 1; Index <= Filled; ++Index)
	{
		SetCharges(Before + Index, EFlaskChange::Filled);
	}
	if (APawn* Owner = Cast<APawn>(GetOwner()))
	{
		FlaskFX::ChargeFilled(*Owner);
	}
	UE_LOG(LogLooter, Log, TEXT("Flask: %d charge(s) filled by %.0f soul, %d of %d, %.0f toward the next"), Filled, Added, Charges,
		FFlaskRules::MaxCharges, GetSoul());
}

void UPlayerFlaskComponent::RefillAll(EFlaskChange Why)
{
	const bool bChanged = !IsFull() || Soul > 0.f;
	Soul = 0.f;
	SetCharges(FFlaskRules::MaxCharges, Why);
	// The respawn's soft three notes, only when there was something to fill.
	APawn* Owner = Cast<APawn>(GetOwner());
	if (Owner && bChanged && Why == EFlaskChange::Refilled)
	{
		FlaskFX::RefilledAll(*Owner);
	}
}

void UPlayerFlaskComponent::SaveFlask(FFlaskSave& OutSave) const
{
	OutSave.Charges = Charges;
	OutSave.Soul = GetSoul();
	OutSave.bSaved = true;
}

void UPlayerFlaskComponent::RestoreFlask(const FFlaskSave& Save)
{
	// A save from before the flask (bSaved false) has a full one, as a new game does.
	if (!Save.bSaved)
	{
		Soul = 0.f;
		SetCharges(FFlaskRules::MaxCharges, EFlaskChange::Restored);
		return;
	}
	const int32 Count = FMath::Clamp(Save.Charges, 0, FFlaskRules::MaxCharges);
	// Never a whole charge's worth of loose soul (an edited save): it would have filled one.
	Soul = Count >= FFlaskRules::MaxCharges ? 0.f : FMath::Clamp(Save.Soul, 0.f, FFlaskRules::SoulPerCharge - 0.01f);
	SetCharges(Count, EFlaskChange::Restored);
}

float UPlayerFlaskComponent::GiveKillSoul(const AActor* Victim)
{
	if (!Victim)
	{
		return 0.f;
	}
	// Anything that isn't a creature yet gave kill experience (the Gravemother's mound counting its spiderlings) is a
	// regular kill. A boss is one with a UBossComponent (or a Boss rank); a boss fight running lifts every kill to its floor.
	const ACreatureBase* Creature = Cast<ACreatureBase>(Victim);
	const ECreatureRank Rank = Creature ? Creature->GetRank() : ECreatureRank::Basic;
	const bool bBoss = Victim->FindComponentByClass<UBossComponent>() != nullptr;
	const bool bBossFight = FightState::IsBossFightOn(GetWorld(), Cast<APawn>(GetOwner()));
	const float Given = FFlaskRules::KillSoul(Rank, bBoss, bBossFight);
	UE_LOG(LogLooter, Verbose, TEXT("Flask: %s's kill gives %.0f soul%s"), *GetNameSafe(Victim), Given, bBossFight ? TEXT(" (boss fight)") : TEXT(""));
	AddSoul(Given);
	return Given;
}

// ---------------------------------------------------------------------------
// What it reads
// ---------------------------------------------------------------------------

APlayerController* UPlayerFlaskComponent::GetPlayer() const
{
	const APawn* Owner = Cast<APawn>(GetOwner());
	APlayerController* Player = Owner ? Cast<APlayerController>(Owner->GetController()) : nullptr;
	return Player && Player->IsLocalController() ? Player : nullptr;
}

AWeaponBase* UPlayerFlaskComponent::GetWeaponInHand() const
{
	const UWeaponManagerComponent* Manager = GetOwner() ? GetOwner()->FindComponentByClass<UWeaponManagerComponent>() : nullptr;
	return Manager ? Manager->GetActiveWeapon() : nullptr;
}

UHealthComponent* UPlayerFlaskComponent::GetHealth() const
{
	return GetOwner() ? GetOwner()->FindComponentByClass<UHealthComponent>() : nullptr;
}

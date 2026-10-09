#include "Player/PlayerMeleeComponent.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Combat/HealthComponent.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Player/CameraShakeModifier.h"
#include "Player/PlayerLocomotionComponent.h"
#include "Player/PlayerMeleeMotion.h"
#include "Player/PlayerThrowComponent.h"
#include "Player/PlayerViewComponent.h"
#include "Progression/PlayerProgressionSubsystem.h"
#include "Scenes/SceneSubsystem.h"
#include "Settings/KeyBindingSubsystem.h"
#include "UI/HUD/LooterHUD.h"
#include "Weapons/WeaponBase.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "EnhancedInputComponent.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

// UPlayerMeleeComponent's life, input and the swing's clock. The blow itself (finding the body, the damage, the knock and
// the feedback) is in PlayerMeleeStrike.cpp.

UPlayerMeleeComponent::UPlayerMeleeComponent()
{
	// Ticks only while a swing runs.
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

UPlayerMeleeComponent* UPlayerMeleeComponent::Find(const AActor* Pawn)
{
	return Pawn ? Pawn->FindComponentByClass<UPlayerMeleeComponent>() : nullptr;
}

// ---------------------------------------------------------------------------
// Life and input
// ---------------------------------------------------------------------------

void UPlayerMeleeComponent::BeginPlay()
{
	Super::BeginPlay();
	if (APawn* Owner = Cast<APawn>(GetOwner()))
	{
		Owner->ReceiveControllerChangedDelegate.AddDynamic(this, &UPlayerMeleeComponent::HandleControllerChanged);
		SetupInput(Owner->GetController());
	}
}

void UPlayerMeleeComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// A gun mid-swing goes back to its hold and is free to fire again.
	EndSwing(false);
	InputBinding.Teardown();
	if (APawn* Owner = Cast<APawn>(GetOwner()))
	{
		Owner->ReceiveControllerChangedDelegate.RemoveDynamic(this, &UPlayerMeleeComponent::HandleControllerChanged);
	}
	Super::EndPlay(EndPlayReason);
}

void UPlayerMeleeComponent::HandleControllerChanged(APawn* Pawn, AController* OldController, AController* NewController)
{
	SetupInput(NewController);
}

void UPlayerMeleeComponent::SetupInput(AController* Controller)
{
	InputBinding.Teardown();
	UKeyBindingSubsystem* Bindings = FPawnInputBinding::GetBindings(Controller);
	if (!Bindings || !Bindings->GetMeleeAction())
	{
		return;
	}
	// The character's own controls (with sprint, crouch and aim), so it only works while the player has their character.
	if (UEnhancedInputComponent* Input = InputBinding.Setup(GetOwner(), Controller, Bindings->GetCharacterContext(), UKeyBindingSubsystem::CharacterContextPriority))
	{
		Input->BindAction(Bindings->GetMeleeAction(), ETriggerEvent::Started, this, &UPlayerMeleeComponent::HandleMeleePressed);
	}
}

void UPlayerMeleeComponent::HandleMeleePressed()
{
	TryMelee();
}

// ---------------------------------------------------------------------------
// Starting a strike
// ---------------------------------------------------------------------------

bool UPlayerMeleeComponent::TryMelee()
{
	const EMeleeBlock Block = GetBlock();
	if (Block != EMeleeBlock::None)
	{
		UE_LOG(LogLooter, Verbose, TEXT("Melee: no strike (%d)"), static_cast<int32>(Block));
		return false;
	}
	StartSwing();
	return true;
}

EMeleeBlock UPlayerMeleeComponent::GetBlock() const
{
	return FMeleeRules::WhyBlocked(GatherGate());
}

FMeleeGateInput UPlayerMeleeComponent::GatherGate() const
{
	FMeleeGateInput In;
	const UWorld* World = GetWorld();
	const APawn* Owner = Cast<APawn>(GetOwner());
	const APlayerController* Player = GetPlayer();
	In.Now = World ? World->GetTimeSeconds() : 0.0;
	In.LastStrike = LastStrikeTime;
	In.bSwinging = bSwinging;
	// A grenade throw has the hands, as a strike keeps the grenade's (EThrowBlock::Busy): one at a time.
	const UPlayerThrowComponent* Throw = UPlayerThrowComponent::Find(Owner);
	In.bSwinging |= Throw && Throw->IsThrowing();
	In.bAlive = Owner && Player && !IsOwnerDead();
	const UPlayerLocomotionComponent* Locomotion = Owner ? Owner->FindComponentByClass<UPlayerLocomotionComponent>() : nullptr;
	In.bTraversing = Locomotion && Locomotion->IsTraversing();
	const USceneSubsystem* Scenes = USceneSubsystem::Get(this);
	In.bInScene = Scenes && (Scenes->IsPlaying() || Scenes->IsHoldingPlayer());
	const ALooterHUD* Hud = Player ? Player->GetHUD<ALooterHUD>() : nullptr;
	In.bMenuOpen = (Hud && Hud->IsMenuOpen()) || (World && World->IsPaused());
	return In;
}

void UPlayerMeleeComponent::StartSwing()
{
	const UWorld* World = GetWorld();
	LastStrikeTime = World ? World->GetTimeSeconds() : 0.0;
	++MeleeCount;
	bSwinging = true;
	bStruck = false;
	bLanded = false;
	SwingTime = 0.f;
	HoldTime = 0.f;

	// With a gun in hand it's the stock: the gun holds its fire, and a reload under way is cut short (it starts over after).
	AWeaponBase* Gun = GetWeaponInHand();
	bArmedSwing = Gun != nullptr;
	SwingWeapon = Gun;
	const bool bCutReload = Gun && Gun->BeginMeleeSwing();

	// Heard and felt the moment the key goes down: the whoosh, and the view drawn back with the coil.
	LooterSound::Play2D(this, MeleeCue::Swing, 1.f, bArmedSwing ? 1.f : MeleeMotion::FistSoundPitch);
	if (APlayerController* Player = GetPlayer())
	{
		UCameraShakeModifier::Kick(Player, MeleeMotion::WindUp(bArmedSwing));
	}
	SetComponentTickEnabled(true);
	UE_LOG(LogLooter, Verbose, TEXT("Melee: %s strike %d%s"), bArmedSwing ? TEXT("stock") : TEXT("fist"), MeleeCount,
		bCutReload ? TEXT(", reload cut short") : TEXT(""));
}

// ---------------------------------------------------------------------------
// The swing's clock
// ---------------------------------------------------------------------------

void UPlayerMeleeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!bSwinging)
	{
		SetComponentTickEnabled(false);
		return;
	}
	if (IsOwnerDead())
	{
		EndSwing(false);
		return;
	}
	// The gun left the hand mid-swing (a swap, a drop): back in its hold with nothing following on it; the swing goes on
	// bare (its blow still comes).
	if (SwingWeapon.IsValid() && SwingWeapon.Get() != GetWeaponInHand())
	{
		ReleaseWeapon(false);
	}

	float Step = FMath::Min(DeltaTime, 0.1f);
	// The blow's hit-stop: the swing's clock holds, the gun stopped dead where it struck.
	if (HoldTime > 0.f)
	{
		const float Held = FMath::Min(HoldTime, Step);
		HoldTime -= Held;
		Step -= Held;
	}
	SwingTime += Step;
	if (!bStruck && SwingTime >= FMeleeRules::ContactSeconds)
	{
		// The blow lands at its moment exactly, however long the frame was, so the pose holds there through any hit-stop.
		SwingTime = FMeleeRules::ContactSeconds;
		Strike();
	}
	UpdateGunPose();
	if (bSwinging && SwingTime >= FMeleeRules::SwingSeconds)
	{
		EndSwing(true);
	}
}

void UPlayerMeleeComponent::EndSwing(bool bResume)
{
	if (!bSwinging)
	{
		return;
	}
	bSwinging = false;
	HoldTime = 0.f;
	ReleaseWeapon(bResume);
	SetComponentTickEnabled(false);
}

void UPlayerMeleeComponent::ReleaseWeapon(bool bResume)
{
	if (AWeaponBase* Gun = SwingWeapon.Get())
	{
		Gun->EndMeleeSwing(bResume);
	}
	SwingWeapon.Reset();
}

void UPlayerMeleeComponent::UpdateGunPose()
{
	if (AWeaponBase* Gun = SwingWeapon.Get())
	{
		const FMeleeSwingPose Pose = MeleeMotion::GunPose(SwingTime, bLanded);
		Gun->SetMeleePose(Pose.Offset, Pose.Rotation);
	}
}

// ---------------------------------------------------------------------------
// What it reads
// ---------------------------------------------------------------------------

FMeleeAim UPlayerMeleeComponent::GetAim() const
{
	FMeleeAim Aim;
	const APawn* Owner = Cast<APawn>(GetOwner());
	if (!Owner)
	{
		return Aim;
	}
	// The body's own eye (the first-person camera rides it in every view), never a third-person camera behind the
	// shoulder; and the way the player looks, which is the way the body faces even in the front view.
	const UCameraComponent* Eye = UPlayerViewComponent::FindFirstPersonCamera(Owner);
	Aim.Eye = Eye ? Eye->GetComponentLocation() : Owner->GetPawnViewLocation();
	Aim.Forward = (Owner->GetController() ? Owner->GetControlRotation() : Owner->GetActorRotation()).Vector();
	const ACharacter* Character = Cast<ACharacter>(Owner);
	const UCapsuleComponent* Capsule = Character ? Character->GetCapsuleComponent() : nullptr;
	Aim.FeetZ = Owner->GetActorLocation().Z - (Capsule ? Capsule->GetScaledCapsuleHalfHeight() : 0.f);
	return Aim;
}

AActor* UPlayerMeleeComponent::FindTargetInReach(float ReachScale, FMeleeContact* OutContact) const
{
	FMeleeContact Contact;
	AActor* Target = FindStrikeTarget(GetWorld(), GetAim(), FMeleeRules::Reach() * FMath::Max(ReachScale, 0.f), GetOwner(), Contact);
	if (Target && OutContact)
	{
		*OutContact = Contact;
	}
	return Target;
}

APlayerController* UPlayerMeleeComponent::GetPlayer() const
{
	const APawn* Owner = Cast<APawn>(GetOwner());
	APlayerController* Player = Owner ? Cast<APlayerController>(Owner->GetController()) : nullptr;
	return Player && Player->IsLocalController() ? Player : nullptr;
}

AWeaponBase* UPlayerMeleeComponent::GetWeaponInHand() const
{
	const UWeaponManagerComponent* Manager = GetOwner() ? GetOwner()->FindComponentByClass<UWeaponManagerComponent>() : nullptr;
	return Manager ? Manager->GetActiveWeapon() : nullptr;
}

bool UPlayerMeleeComponent::IsOwnerDead() const
{
	const UHealthComponent* Health = GetOwner() ? GetOwner()->FindComponentByClass<UHealthComponent>() : nullptr;
	return Health && Health->IsDead();
}

float UPlayerMeleeComponent::GetLevelScale() const
{
	// The strike grows with the player as enemies and guns grow with theirs, so it takes the same share at equal levels.
	const APlayerController* Player = GetPlayer();
	const ULocalPlayer* Local = Player ? Player->GetLocalPlayer() : nullptr;
	const UPlayerProgressionSubsystem* Progression = Local ? Local->GetSubsystem<UPlayerProgressionSubsystem>() : nullptr;
	return UPlayerProgressionSubsystem::GetLevelRules().EnemyScale(Progression ? Progression->GetLevel() : 1);
}

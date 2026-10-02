#include "Interaction/InteractableProp.h"
#include "AI_Looter_Shooter.h"
#include "Interaction/InteractionComponent.h"
#include "Interaction/InteractionSubsystem.h"
#include "World/MinimapSubsystem.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Materials/MaterialInterface.h"

AInteractableProp::AInteractableProp()
{
	// Ticks only while a swing or a cooldown is under way (RefreshTick).
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;

	// Solid like any prop: it blocks the player, and the crosshair's line finds it (it blocks Visibility).
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Root);
	Mesh->SetMobility(EComponentMobility::Movable);
	Mesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);

	Prompt = FText::FromString(TEXT("Use"));
	Tags.Add(MinimapTags::Obstacle);
}

void AInteractableProp::BeginPlay()
{
	Super::BeginPlay();
	CapturePlacedLook();
	// As it was placed: shut and dark, unless it starts on.
	SetOn(bStartOn, /*bInstant*/ true);
	if (UInteractionSubsystem* Registry = UInteractionSubsystem::Get(this))
	{
		Registry->Register(this);
	}
}

void AInteractableProp::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UInteractionSubsystem* Registry = UInteractionSubsystem::Get(this))
	{
		Registry->Unregister(this);
	}
	Super::EndPlay(EndPlayReason);
}

void AInteractableProp::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Advance(DeltaSeconds);
}

// ---------------------------------------------------------------------------
// Being used
// ---------------------------------------------------------------------------

FInteractionOptions AInteractableProp::GetInteractionOptions(const UInteractionComponent& User) const
{
	FInteractionOptions Options;
	Options.bUsable = IsUsable();
	Options.bTap = !bHold;
	Options.bHold = bHold;
	Options.HoldSeconds = HoldSeconds;
	(bHold ? Options.HoldPrompt : Options.TapPrompt) = GetCurrentPrompt();
	Options.Reach = Reach;
	return Options;
}

bool AInteractableProp::Interact(UInteractionComponent& User, bool bHeld)
{
	return Use(User.GetOwner(), bHeld);
}

TOptional<FVector> AInteractableProp::GetInteractionLocation() const
{
	// The middle of what's seen: a door's leaf wherever it has swung to, the bell.
	return Mesh ? TOptional<FVector>(Mesh->Bounds.Origin) : TOptional<FVector>();
}

const FText& AInteractableProp::GetCurrentPrompt() const
{
	return bOn && !PromptWhenOn.IsEmpty() ? PromptWhenOn : Prompt;
}

bool AInteractableProp::IsUsable() const
{
	return bEnabled && CooldownLeft <= 0.f && !(UseMode == EInteractablePropUse::TurnOn && bOn);
}

bool AInteractableProp::Use(AActor* User, bool bHeld)
{
	if (!IsUsable())
	{
		return false;
	}
	CapturePlacedLook();
	// The words of what was done, before the use changes them ("Open the door", not "Close the door").
	const FString Done = GetCurrentPrompt().ToString();
	switch (UseMode)
	{
	case EInteractablePropUse::Toggle:
		bOn = !bOn;
		break;
	case EInteractablePropUse::TurnOn:
		bOn = true;
		break;
	case EInteractablePropUse::Trigger:
		break;
	}
	CooldownLeft = CooldownSeconds;
	ApplyGlow();
	RefreshTick();

	UE_LOG(LogLooter, Log, TEXT("%s: %s (%s%s)."), *GetActorNameOrLabel(), *Done, bHeld ? TEXT("held") : TEXT("tapped"),
		User ? *FString::Printf(TEXT(" by %s"), *User->GetName()) : TEXT(""));
	OnUsed.Broadcast(this, User, bHeld);
	OnUsedNative.Broadcast(*this, User, bHeld);
	return true;
}

void AInteractableProp::SetOn(bool bInOn, bool bInstant)
{
	CapturePlacedLook();
	bOn = bInOn;
	if (bInstant)
	{
		SwingAlpha = bOn ? 1.f : 0.f;
	}
	ApplySwing();
	ApplyGlow();
	RefreshTick();
}

// ---------------------------------------------------------------------------
// Looks: the swing and the glow
// ---------------------------------------------------------------------------

void AInteractableProp::Advance(float DeltaSeconds)
{
	CooldownLeft = FMath::Max(CooldownLeft - DeltaSeconds, 0.f);
	if (Effect == EInteractablePropEffect::Swing)
	{
		// A whole swing takes SwingSeconds, either way.
		const float Step = SwingSeconds > 0.f ? DeltaSeconds / SwingSeconds : 1.f;
		SwingAlpha = bOn ? FMath::Min(SwingAlpha + Step, 1.f) : FMath::Max(SwingAlpha - Step, 0.f);
		ApplySwing();
	}
	RefreshTick();
}

void AInteractableProp::CapturePlacedLook()
{
	if (bPlacedLookCaptured || !Mesh)
	{
		return;
	}
	bPlacedLookCaptured = true;
	PlacedTransform = Mesh->GetRelativeTransform();
	PlacedGlowMaterial = Mesh->GetMaterial(GetGlowIndex());
}

void AInteractableProp::ApplySwing()
{
	if (Effect != EInteractablePropEffect::Swing || !Mesh)
	{
		return;
	}
	// Eased, so it starts and stops softly; the mesh turns about the hinge's upright line.
	const float Angle = OpenAngle * FMath::SmoothStep(0.f, 1.f, SwingAlpha);
	const FQuat Turn(FVector::UpVector, FMath::DegreesToRadians(Angle));
	const FVector Swung = HingeOffset + Turn.RotateVector(PlacedTransform.GetLocation() - HingeOffset);
	Mesh->SetRelativeLocationAndRotation(Swung, Turn * PlacedTransform.GetRotation());
}

void AInteractableProp::ApplyGlow()
{
	if (Effect != EInteractablePropEffect::Glow || !Mesh)
	{
		return;
	}
	UMaterialInterface* Wanted = bOn ? GlowMaterial.Get() : DarkMaterial.Get();
	Mesh->SetMaterial(GetGlowIndex(), Wanted ? Wanted : PlacedGlowMaterial.Get());
}

int32 AInteractableProp::GetGlowIndex() const
{
	const int32 Named = Mesh && !GlowSlot.IsNone() ? Mesh->GetMaterialIndex(GlowSlot) : INDEX_NONE;
	return Named != INDEX_NONE ? Named : 0;
}

void AInteractableProp::RefreshTick()
{
	const bool bSwinging = Effect == EInteractablePropEffect::Swing && SwingAlpha != (bOn ? 1.f : 0.f);
	SetActorTickEnabled(CooldownLeft > 0.f || bSwinging);
}

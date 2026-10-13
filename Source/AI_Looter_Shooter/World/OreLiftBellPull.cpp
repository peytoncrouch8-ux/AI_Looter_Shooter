// AOreLiftBellPull: a landing's bell cord on the ore lift: ride, call, or the closed lift's clunk and its reason.

#include "World/OreLiftBellPull.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Interaction/InteractionComponent.h"
#include "Interaction/InteractionSubsystem.h"
#include "Components/SphereComponent.h"

#define LOCTEXT_NAMESPACE "OreLiftBellPull"

namespace
{
	/** The handle's size (cm): a hand's grip on the cord. */
	constexpr float HandleRadius = 18.f;
}

AOreLiftBellPull::AOreLiftBellPull()
{
	PrimaryActorTick.bCanEverTick = false;

	// Only the crosshair's line finds it: it stops nobody and nothing else.
	Handle = CreateDefaultSubobject<USphereComponent>(TEXT("Handle"));
	Handle->InitSphereRadius(HandleRadius);
	Handle->SetMobility(EComponentMobility::Movable);
	Handle->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Handle->SetCollisionObjectType(ECC_WorldDynamic);
	Handle->SetCollisionResponseToAllChannels(ECR_Ignore);
	Handle->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	Handle->SetGenerateOverlapEvents(false);
	Handle->SetCanEverAffectNavigation(false);
	RootComponent = Handle;

	RideUpPrompt = LOCTEXT("RideUp", "Ride the lift up");
	RideDownPrompt = LOCTEXT("RideDown", "Ride the lift down");
	CallPrompt = LOCTEXT("Call", "Ring for the cage");
	ClosedPrompt = LOCTEXT("Closed", "Pull the bell cord");
}

void AOreLiftBellPull::BeginPlay()
{
	Super::BeginPlay();
	if (UInteractionSubsystem* Registry = UInteractionSubsystem::Get(this))
	{
		Registry->Register(this);
	}
	if (Lift)
	{
		Lift->RegisterBellPull(*this, End);
	}
	else
	{
		UE_LOG(LogLooter, Warning, TEXT("%s: a bell pull with no lift; it does nothing."), *GetActorNameOrLabel());
	}
}

void AOreLiftBellPull::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UInteractionSubsystem* Registry = UInteractionSubsystem::Get(this))
	{
		Registry->Unregister(this);
	}
	Super::EndPlay(EndPlayReason);
}

FInteractionOptions AOreLiftBellPull::GetInteractionOptions(const UInteractionComponent& User) const
{
	if (!Lift)
	{
		return FInteractionOptions::None();
	}
	FInteractionOptions Options;
	Options.Reach = Reach;
	switch (Lift->GetBellState(End))
	{
	case EOreLiftBell::Closed:
		// A tap, so the player learns why at once; nothing to hold for.
		Options.bTap = true;
		Options.bHold = false;
		Options.TapPrompt = ClosedPrompt;
		break;
	case EOreLiftBell::Ride:
		Options.bTap = false;
		Options.bHold = true;
		Options.HoldSeconds = HoldSeconds;
		Options.HoldPrompt = End == EOreLiftEnd::Bottom ? RideUpPrompt : RideDownPrompt;
		break;
	case EOreLiftBell::Call:
		Options.bTap = false;
		Options.bHold = true;
		Options.HoldSeconds = HoldSeconds;
		Options.HoldPrompt = CallPrompt;
		break;
	default:
		// The cage is on its way, or ridden: the cord hangs still.
		return FInteractionOptions::None();
	}
	return Options;
}

bool AOreLiftBellPull::Interact(UInteractionComponent& User, bool bHeld)
{
	return Pull(User.GetOwner(), bHeld);
}

TOptional<FVector> AOreLiftBellPull::GetInteractionLocation() const
{
	return TOptional<FVector>(GetActorLocation());
}

bool AOreLiftBellPull::Pull(AActor* User, bool bHeld)
{
	if (!Lift)
	{
		return false;
	}
	switch (Lift->GetBellState(End))
	{
	case EOreLiftBell::Closed:
		if (bHeld)
		{
			return false;
		}
		Lift->RingClosed(End);
		return true;
	case EOreLiftBell::Ride:
		if (!bHeld || !Lift->Ride(End, User))
		{
			return false;
		}
		// Two strokes: the winchman's signal to haul away.
		Ring(LooterSoundCue::OreLift::BellSignal);
		LooterSound::PlayAt(this, LooterSoundCue::OreLift::GateLatch, Lift->GetRiderSpot(End));
		return true;
	case EOreLiftBell::Call:
		if (!bHeld || !Lift->CallTo(End))
		{
			return false;
		}
		Ring(LooterSoundCue::OreLift::BellSignal);
		return true;
	default:
		return false;
	}
}

FText AOreLiftBellPull::GetPromptNow() const
{
	if (!Lift)
	{
		return FText::GetEmpty();
	}
	switch (Lift->GetBellState(End))
	{
	case EOreLiftBell::Closed:
		return ClosedPrompt;
	case EOreLiftBell::Ride:
		return End == EOreLiftEnd::Bottom ? RideUpPrompt : RideDownPrompt;
	case EOreLiftBell::Call:
		return CallPrompt;
	default:
		return FText::GetEmpty();
	}
}

void AOreLiftBellPull::Ring(FName Cue)
{
	LooterSound::PlayAt(this, Cue, GetActorLocation());
}

#undef LOCTEXT_NAMESPACE

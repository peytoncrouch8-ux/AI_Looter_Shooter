// UInteractionComponent: finding what the player means to use (the view, the candidates, the crosshair's line, sight).

#include "Interaction/InteractionComponent.h"
#include "Interaction/Interactable.h"
#include "Interaction/InteractionFocus.h"
#include "Interaction/InteractionSource.h"
#include "Interaction/InteractionSubsystem.h"
#include "Player/PlayerViewComponent.h"
#include "CollisionQueryParams.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

namespace
{
	/** Nothing is used from farther than this (cm): interactables beyond it aren't even asked how they're used. */
	constexpr float MaxReach = 1000.f;

	/** A sight line blocked this close to the point looked at still sees it (a poster's own wall, a door's frame). */
	constexpr float SightTolerance = 15.f;

	/** The interactable a hit belongs to: the actor hit, or the one it's fixed to (a door's parts). */
	AActor* FindInteractableOwner(AActor* Hit)
	{
		for (AActor* Candidate = Hit; Candidate; Candidate = Candidate->GetAttachParentActor())
		{
			if (Candidate->Implements<UInteractable>())
			{
				return Candidate;
			}
		}
		return nullptr;
	}
}

bool UInteractionComponent::GetView(FInteractionView& OutView) const
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	if (!Pawn)
	{
		return false;
	}

	// Aim like the gun does: the crosshair in first and third person, the eyes in the front view. Reach is measured from
	// the character, since a third-person camera sits a couple of meters behind them.
	FVector Location;
	FRotator Rotation;
	if (const UPlayerViewComponent* PlayerView = Pawn->FindComponentByClass<UPlayerViewComponent>())
	{
		PlayerView->GetAimViewPoint(Location, Rotation);
	}
	else if (const APlayerController* PC = Cast<APlayerController>(Pawn->GetController()))
	{
		PC->GetPlayerViewPoint(Location, Rotation);
	}
	else
	{
		Pawn->GetActorEyesViewPoint(Location, Rotation);
	}
	OutView.Location = Location;
	OutView.Direction = Rotation.Vector();
	OutView.ReachOrigin = Pawn->GetPawnViewLocation();
	return true;
}

void UInteractionComponent::RefreshFocus()
{
	FInteractionView View;
	TArray<FInteractionCandidate> Candidates;
	if (GetView(View))
	{
		GatherCandidates(View, Candidates);
	}

	// While the key is held on something, it stays the focus for as long as the player still looks at it.
	const AActor* Keep = bPressed ? PressedActor.Get() : nullptr;
	const int32 Best = InteractionFocus::Select(View, Candidates, AimThreshold,
		[this, &View](const FInteractionCandidate& Candidate) { return !Candidate.bCheckSight || IsInSight(View, Candidate); }, Keep);
	SetFocus(Candidates.IsValidIndex(Best) ? &Candidates[Best] : nullptr);
}

void UInteractionComponent::GatherCandidates(const FInteractionView& View, TArray<FInteractionCandidate>& OutCandidates)
{
	// The level's interactables, each of which joined the registry as its play began.
	if (const UInteractionSubsystem* Registry = UInteractionSubsystem::Get(this))
	{
		for (const TWeakObjectPtr<AActor>& Entry : Registry->GetInteractables())
		{
			AddInteractable(View, Entry.Get(), nullptr, OutCandidates);
		}
	}

	// What the player's own components offer: the weapon manager's loot.
	if (!bSourcesFound)
	{
		FindSources();
	}
	for (const TWeakObjectPtr<UObject>& Entry : Sources)
	{
		UObject* SourceObject = Entry.Get();
		const IInteractionSource* Source = Cast<IInteractionSource>(SourceObject);
		if (!Source)
		{
			continue;
		}
		const int32 First = OutCandidates.Num();
		Source->GatherInteractions(View, OutCandidates);
		for (int32 Index = First; Index < OutCandidates.Num(); ++Index)
		{
			OutCandidates[Index].Source = SourceObject;
		}
	}

	AddLookedAt(View, OutCandidates);
}

FInteractionCandidate* UInteractionComponent::AddInteractable(const FInteractionView& View, AActor* Actor, const FVector* AtPoint,
	TArray<FInteractionCandidate>& OutCandidates) const
{
	const IInteractable* Interactable = Cast<IInteractable>(Actor);
	if (!Interactable || Actor == GetOwner() || Actor->IsActorBeingDestroyed())
	{
		return nullptr;
	}
	FVector Point = Actor->GetActorLocation();
	if (AtPoint)
	{
		Point = *AtPoint;
	}
	else if (const TOptional<FVector> OwnPoint = Interactable->GetInteractionLocation(); OwnPoint.IsSet())
	{
		Point = OwnPoint.GetValue();
	}
	if (FVector::DistSquared(Point, View.ReachOrigin) > FMath::Square(static_cast<double>(MaxReach)))
	{
		return nullptr;
	}

	FInteractionCandidate& Candidate = OutCandidates.AddDefaulted_GetRef();
	Candidate.Actor = Actor;
	Candidate.Location = Point;
	Candidate.Options = Interactable->GetInteractionOptions(*this);
	Candidate.Reach = FMath::Min(Candidate.Options.Reach > 0.f ? Candidate.Options.Reach : Reach, MaxReach);
	// Found by the crosshair's line, it's in sight already.
	Candidate.bCheckSight = AtPoint == nullptr;
	return &Candidate;
}

void UInteractionComponent::AddLookedAt(const FInteractionView& View, TArray<FInteractionCandidate>& OutCandidates) const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	// From the line's start out to the farthest anything reaches past the eyes (a third-person view starts behind them).
	const double Length = FVector::Dist(View.Location, View.ReachOrigin) + MaxReach;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(InteractionLook), /*bTraceComplex*/ false);
	IgnorePlayer(Params);
	FHitResult Hit;
	if (!World->LineTraceSingleByChannel(Hit, View.Location, View.Location + View.Direction * Length, ECC_Visibility, Params))
	{
		return;
	}
	AActor* LookedAt = FindInteractableOwner(Hit.GetActor());
	if (!LookedAt)
	{
		return;
	}

	// Looked straight at, wherever its middle is (the far edge of a wide gate): it's judged by where the line meets it.
	const FVector Point = Hit.ImpactPoint;
	if (FInteractionCandidate* Existing = OutCandidates.FindByPredicate([LookedAt](const FInteractionCandidate& Candidate) { return Candidate.Actor == LookedAt; }))
	{
		Existing->Location = Point;
		Existing->bCheckSight = false;
		return;
	}
	// One that never joined the registry is still found this way.
	AddInteractable(View, LookedAt, &Point, OutCandidates);
}

bool UInteractionComponent::IsInSight(const FInteractionView& View, const FInteractionCandidate& Candidate) const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return true;
	}
	FCollisionQueryParams Params(SCENE_QUERY_STAT(InteractionSight), /*bTraceComplex*/ false);
	IgnorePlayer(Params);
	FHitResult Hit;
	if (!World->LineTraceSingleByChannel(Hit, View.ReachOrigin, Candidate.Location, ECC_Visibility, Params))
	{
		return true;
	}
	// Its own parts don't hide it, nor does what it stands against right at the point (a poster's wall).
	const AActor* HitActor = Hit.GetActor();
	if (HitActor && Candidate.Actor && (HitActor == Candidate.Actor || HitActor->IsAttachedTo(Candidate.Actor)))
	{
		return true;
	}
	return FVector::Dist(Hit.ImpactPoint, Candidate.Location) <= SightTolerance;
}

void UInteractionComponent::IgnorePlayer(FCollisionQueryParams& Params) const
{
	if (AActor* Player = GetOwner())
	{
		Params.AddIgnoredActor(Player);
		// The guns they carry (hidden ones too).
		TArray<AActor*> Carried;
		Player->GetAttachedActors(Carried, /*bResetArray*/ true, /*bRecursivelyIncludeAttachedActors*/ true);
		Params.AddIgnoredActors(Carried);
	}
}

void UInteractionComponent::SetFocus(const FInteractionCandidate* Candidate)
{
	AActor* NewFocus = Candidate ? Candidate->Actor : nullptr;
	FocusedSource = Candidate ? Candidate->Source : nullptr;
	FocusedOptions = Candidate ? Candidate->Options : FInteractionOptions::None();
	if (FocusedActor.Get() != NewFocus)
	{
		FocusedActor = NewFocus;
		OnFocusChanged.Broadcast(NewFocus);
	}
}

void UInteractionComponent::FindSources()
{
	bSourcesFound = true;
	Sources.Reset();
	TInlineComponentArray<UActorComponent*> Components(GetOwner());
	for (UActorComponent* Component : Components)
	{
		if (Component && Component != this && Component->Implements<UInteractionSource>())
		{
			Sources.Add(Component);
		}
	}
}

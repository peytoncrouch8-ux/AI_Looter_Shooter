// AInteriorVisibility: an interior and the outside never draw each other: hidden by tag as the player's view goes in or out.

#include "World/InteriorVisibility.h"
#include "AI_Looter_Shooter.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"

namespace
{
	const TCHAR* ViewName(EInteriorView View)
	{
		switch (View)
		{
		case EInteriorView::Outside: return TEXT("outside");
		case EInteriorView::Inside: return TEXT("inside");
		default: return TEXT("at the portal");
		}
	}

	bool HasActor(const TArray<TWeakObjectPtr<AActor>>& Actors, const AActor* Wanted)
	{
		return Actors.ContainsByPredicate([Wanted](const TWeakObjectPtr<AActor>& Each) { return Each.Get() == Wanted; });
	}

	UBoxComponent* MakeBox(AActor& Owner, const TCHAR* Name, USceneComponent* Parent, const FVector& Extent, const FColor& Colour)
	{
		UBoxComponent* Box = Owner.CreateDefaultSubobject<UBoxComponent>(Name);
		Box->SetupAttachment(Parent);
		Box->SetMobility(EComponentMobility::Movable);
		// Only a place to measure against: it touches nothing.
		Box->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
		Box->SetGenerateOverlapEvents(false);
		Box->SetCanEverAffectNavigation(false);
		Box->InitBoxExtent(Extent);
		Box->ShapeColor = Colour;
		return Box;
	}
}

AInteriorVisibility::AInteriorVisibility()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* Anchor = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Anchor->SetMobility(EComponentMobility::Movable);
	RootComponent = Anchor;

	// The Providence's rooms, about 30 x 20 m and 12 m high, and its mouth at their south face (the build script sizes them).
	Interior = MakeBox(*this, TEXT("Interior"), Anchor, FVector(1500.0, 1000.0, 600.0), FColor(90, 160, 255));
	Portal = MakeBox(*this, TEXT("Portal"), Anchor, FVector(400.0, 400.0, 400.0), FColor(255, 170, 60));
	Portal->SetRelativeLocation(FVector(-1700.0, 0.0, 0.0));
}

void AInteriorVisibility::BeginPlay()
{
	Super::BeginPlay();
	GatherActors();
	// Where the player is now (usually outside: the interior hidden from the start), then a few looks a second.
	if (const TOptional<FVector> Eyes = FindViewPoint())
	{
		UpdateFor(Eyes.GetValue());
	}
	else
	{
		ApplyView(EInteriorView::Outside);
	}
	GetWorldTimerManager().SetTimer(PollTimer, this, &AInteriorVisibility::Poll, PollSeconds, true, FMath::FRandRange(0.f, PollSeconds));
}

void AInteriorVisibility::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (const UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PollTimer);
	}
	// Gone while the level goes on (a rebuild): what it hid shows again.
	for (const TWeakObjectPtr<AActor>& Each : HiddenByMe)
	{
		if (AActor* Hidden = Each.Get())
		{
			Hidden->SetActorHiddenInGame(false);
		}
	}
	HiddenByMe.Reset();
	Super::EndPlay(EndPlayReason);
}

// ---------------------------------------------------------------------------
// Where the view is
// ---------------------------------------------------------------------------

bool AInteriorVisibility::IsIn(const UBoxComponent* Box, const FVector& Point, float Grow)
{
	if (!Box)
	{
		return false;
	}
	const FTransform& Frame = Box->GetComponentTransform();
	const FVector Local = Frame.InverseTransformPosition(Point);
	const FVector Scale = Frame.GetScale3D().GetAbs();
	const FVector Extent = Box->GetUnscaledBoxExtent();
	const FVector Reach(Grow / FMath::Max(Scale.X, UE_KINDA_SMALL_NUMBER), Grow / FMath::Max(Scale.Y, UE_KINDA_SMALL_NUMBER),
		Grow / FMath::Max(Scale.Z, UE_KINDA_SMALL_NUMBER));
	return FMath::Abs(Local.X) <= Extent.X + Reach.X && FMath::Abs(Local.Y) <= Extent.Y + Reach.Y && FMath::Abs(Local.Z) <= Extent.Z + Reach.Z;
}

bool AInteriorVisibility::IsInAdditional(const FVector& Point, float Grow) const
{
	const FVector Local = GetActorTransform().InverseTransformPosition(Point);
	return AdditionalInside.ContainsByPredicate([&Local, Grow](const FBox& Room) { return Room.IsValid && Room.ExpandBy(Grow).IsInsideOrOn(Local); });
}

EInteriorView AInteriorVisibility::Classify(const FVector& Point) const
{
	// The mouth first: both sides show while the view is in it. Each place keeps the view a margin past its face.
	if (IsIn(Portal, Point, View == EInteriorView::Portal ? Margin : 0.f))
	{
		return EInteriorView::Portal;
	}
	const float Grow = View == EInteriorView::Inside ? Margin : 0.f;
	if (IsIn(Interior, Point, Grow) || IsInAdditional(Point, Grow))
	{
		return EInteriorView::Inside;
	}
	return EInteriorView::Outside;
}

TOptional<FVector> AInteriorVisibility::FindViewPoint() const
{
	const UWorld* World = GetWorld();
	const APlayerController* Player = World ? World->GetFirstPlayerController() : nullptr;
	if (!Player)
	{
		return TOptional<FVector>();
	}
	FVector Eyes;
	FRotator Looking;
	Player->GetPlayerViewPoint(Eyes, Looking);
	return TOptional<FVector>(Eyes);
}

void AInteriorVisibility::Poll()
{
	if (const TOptional<FVector> Eyes = FindViewPoint())
	{
		UpdateFor(Eyes.GetValue());
	}
}

void AInteriorVisibility::UpdateFor(const FVector& ViewPoint)
{
	const EInteriorView Wanted = Classify(ViewPoint);
	if (bApplied && Wanted == View)
	{
		return;
	}
	// The pieces may have changed since the last time (a rebuilt or streamed piece): looked for again.
	GatherActors();
	ApplyView(Wanted);
}

// ---------------------------------------------------------------------------
// What shows
// ---------------------------------------------------------------------------

void AInteriorVisibility::GatherActors()
{
	InsideActors.Reset();
	OutsideActors.Reset();
	VeilActors.Reset();
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Each = *It;
		if (!Each || Each == this)
		{
			continue;
		}
		if (!InsideTag.IsNone() && Each->ActorHasTag(InsideTag))
		{
			InsideActors.Add(Each);
		}
		if (!OutsideTag.IsNone() && Each->ActorHasTag(OutsideTag))
		{
			OutsideActors.Add(Each);
		}
		if (!VeilTag.IsNone() && Each->ActorHasTag(VeilTag))
		{
			VeilActors.Add(Each);
		}
	}
}

void AInteriorVisibility::ApplyView(EInteriorView NewView)
{
	View = NewView;
	bApplied = true;
	TArray<TWeakObjectPtr<AActor>> Hide;
	if (NewView == EInteriorView::Outside && bHideInsideWhenOutside)
	{
		Hide.Append(InsideActors);
	}
	else if (NewView == EInteriorView::Inside && bHideOutsideWhenInside)
	{
		Hide.Append(OutsideActors);
	}
	// The veil only stands while the rooms past it are hidden.
	if (NewView != EInteriorView::Outside)
	{
		Hide.Append(VeilActors);
	}
	// What it hid that should show now shows again; nothing else is touched.
	for (int32 Index = HiddenByMe.Num() - 1; Index >= 0; --Index)
	{
		AActor* Hidden = HiddenByMe[Index].Get();
		if (!Hidden || !HasActor(Hide, Hidden))
		{
			if (Hidden)
			{
				Hidden->SetActorHiddenInGame(false);
			}
			HiddenByMe.RemoveAt(Index);
		}
	}
	// What should hide and shows now hides, and is remembered as its own to show again.
	for (const TWeakObjectPtr<AActor>& Each : Hide)
	{
		AActor* Shown = Each.Get();
		if (Shown && !Shown->IsHidden() && !HasActor(HiddenByMe, Shown))
		{
			Shown->SetActorHiddenInGame(true);
			HiddenByMe.Add(Shown);
		}
	}
	UE_LOG(LogLooter, Verbose, TEXT("%s: the view is %s; %d hidden."), *GetActorNameOrLabel(), ViewName(View), CountHidden());
}

int32 AInteriorVisibility::CountHidden() const
{
	int32 Count = 0;
	for (const TWeakObjectPtr<AActor>& Each : HiddenByMe)
	{
		Count += Each.IsValid() ? 1 : 0;
	}
	return Count;
}

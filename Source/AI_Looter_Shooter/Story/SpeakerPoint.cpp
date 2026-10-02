#include "Story/SpeakerPoint.h"
#include "Story/SpeakerPointComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"

ASpeakerPoint::ASpeakerPoint()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;

	// Empty unless a level gives the door a leaf of its own: solid like any prop, so the player bumps it and the
	// crosshair's line finds it (it blocks Visibility).
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Root);
	Mesh->SetMobility(EComponentMobility::Movable);
	Mesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);

	// About the middle of a door, where the player looks to talk through it.
	SpeakerPoint = CreateDefaultSubobject<USpeakerPointComponent>(TEXT("SpeakerPoint"));
	SpeakerPoint->SetupAttachment(Root);
	SpeakerPoint->SetRelativeLocation(FVector(0.0, 0.0, 130.0));

#if WITH_EDITORONLY_DATA
	Arrow = CreateEditorOnlyDefaultSubobject<UArrowComponent>(TEXT("Arrow"));
	if (Arrow)
	{
		Arrow->SetupAttachment(Root);
		Arrow->SetRelativeLocation(FVector(0.0, 0.0, 130.0));
		Arrow->ArrowColor = FColor(255, 170, 60);
		Arrow->bTreatAsASprite = true;
		Arrow->bIsScreenSizeScaled = true;
	}
#endif
}

FInteractionOptions ASpeakerPoint::GetInteractionOptions(const UInteractionComponent& User) const
{
	return SpeakerPoint ? SpeakerPoint->GetInteractionOptions(User) : FInteractionOptions::None();
}

bool ASpeakerPoint::Interact(UInteractionComponent& User, bool bHeld)
{
	return SpeakerPoint && SpeakerPoint->Interact(User, bHeld);
}

TOptional<FVector> ASpeakerPoint::GetInteractionLocation() const
{
	return SpeakerPoint ? SpeakerPoint->GetInteractionLocation() : TOptional<FVector>();
}

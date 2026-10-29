#include "World/FindSpot.h"
#include "World/FindSubsystem.h"
#include "Components/SceneComponent.h"

const FName AFindSpot::FindTag(TEXT("Find"));

AFindSpot::AFindSpot()
{
	// Data only: nothing ticks, collides or draws (the tells are the subsystem's).
	PrimaryActorTick.bCanEverTick = false;
	SetCanBeDamaged(false);
	SetActorEnableCollision(false);
	SetHidden(true);
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	Root->SetMobility(EComponentMobility::Static);
	Tags.Add(FindTag);
}

void AFindSpot::BeginPlay()
{
	Super::BeginPlay();
	if (UFindSubsystem* Finds = UFindSubsystem::Get(this))
	{
		Finds->RegisterSpot(this);
	}
}

void AFindSpot::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UFindSubsystem* Finds = UFindSubsystem::Get(this))
	{
		Finds->UnregisterSpot(this);
	}
	Super::EndPlay(EndPlayReason);
}

AActor* AFindSpot::GetSourceActor() const
{
	return Source.Get();
}

FVector AFindSpot::GetRewardLocation() const
{
	const AActor* From = GetSourceActor();
	return From ? From->GetActorLocation() : GetActorLocation();
}

FFindEntry AFindSpot::ToEntry() const
{
	FFindEntry Entry;
	Entry.Id = FindId;
	Entry.Name = DisplayName.IsEmpty() ? FText::FromString(FName::NameToDisplayString(FindId.ToString(), false)) : DisplayName;
	Entry.Region = Region;
	Entry.Kind = Kind;
	return Entry;
}

void AFindSpot::SetTellsShown(bool bShown)
{
	for (const TSoftObjectPtr<AActor>& Each : Tells)
	{
		// A lantern's glow or the coins on a mound: gone with the find, and solid things stop blocking once hidden.
		if (AActor* Prop = Each.Get())
		{
			Prop->SetActorHiddenInGame(!bShown);
			Prop->SetActorEnableCollision(bShown);
		}
	}
}

#include "World/TownLifeDoor.h"
#include "Story/SpeakerPointComponent.h"
#include "World/TownLifeRules.h"
#include "World/TownLifeSubsystem.h"
#include "Components/SceneComponent.h"

ATownLifeDoor::ATownLifeDoor()
{
	PrimaryActorTick.bCanEverTick = false;
	SetCanBeDamaged(false);

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;

	// Townsfolk mutter; they don't talk to the dead. A caption with no name of its own says where the voice is.
	SpeakerPoint = CreateDefaultSubobject<USpeakerPointComponent>(TEXT("SpeakerPoint"));
	SpeakerPoint->SetupAttachment(Root);
	SpeakerPoint->bTalkable = false;
	SpeakerPoint->SpeakerName = NSLOCTEXT("LooterTownLife", "BehindTheDoor", "Behind a door");
}

void ATownLifeDoor::BeginPlay()
{
	// Before the components begin (Super does that): the speaker point joins the town's life only with mutters to say.
	if (SpeakerPoint && SpeakerPoint->Mutters.IsEmpty())
	{
		SpeakerPoint->Mutters = TownLifeRules::MutterTopicsFor(Household);
	}
	Super::BeginPlay();
	if (UTownLifeSubsystem* TownLife = UTownLifeSubsystem::Get(this))
	{
		TownLife->AddSource(this, Household);
	}
}

void ATownLifeDoor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UTownLifeSubsystem* TownLife = UTownLifeSubsystem::Get(this))
	{
		TownLife->RemoveSource(this);
	}
	Super::EndPlay(EndPlayReason);
}

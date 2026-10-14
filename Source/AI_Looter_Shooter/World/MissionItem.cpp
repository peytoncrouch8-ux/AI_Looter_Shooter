// AMissionItem: a thing that belongs to a mission, lying in the world until it's taken: shown by its mission's state, taken
// with Interact or a touch, and its fall when a creature drops it.

#include "World/MissionItem.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Interaction/InteractionComponent.h"
#include "Interaction/InteractionSubsystem.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionRunner.h"
#include "Missions/MissionText.h"
#include "UI/Style/MissionMarkIcons.h"
#include "World/LightBeam.h"
#include "World/WorldQueries.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/HitResult.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "MissionItem"

const FName AMissionItem::ItemTag(TEXT("MissionItem"));

namespace
{
	/** How often a touch pickup looks for the player (s): a walk past at a run still finds it. */
	constexpr float TouchCheckSeconds = 0.1f;
	/** A drop falls from this far over the body to the ground under it, in FallSeconds, with a little hop at the end. */
	constexpr float FallRise = 60.f;
	constexpr float FallSeconds = 0.45f;
	constexpr float FallHop = 12.f;
	/** Its gold pillar: tall enough to see over a fight's dust, thin enough not to be loot's. */
	constexpr float BeamHeight = 260.f;
	constexpr float BeamRadius = 6.f;
	constexpr float BeamGlow = 2.2f;
	/** The stand-in's size: a small knot of something, until the item's model is made. */
	constexpr float StandInScale = 0.14f;

	APawn* PlayerPawnIn(const UWorld* World)
	{
		const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
		return Controller ? Controller->GetPawn() : nullptr;
	}
}

AMissionItem::AMissionItem()
{
	// Still while it lies; it ticks only while it falls.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;

	// A small thing lying about: the Interact key's line and its sight check find it (Visibility), nothing else does. The
	// player walks over a pin; shots and the camera pass it by.
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Root);
	Mesh->SetMobility(EComponentMobility::Movable);
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Mesh->SetCollisionObjectType(ECC_WorldDynamic);
	Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	Mesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	Mesh->SetGenerateOverlapEvents(false);
	Mesh->SetCanEverAffectNavigation(false);

	Beam = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Beam"));
	Beam->SetupAttachment(Root);
	Beam->SetMobility(EComponentMobility::Movable);
	Beam->SetVisibility(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	StandIn = Sphere.Object;

	Tags.Add(ItemTag);
}

FName AMissionItem::TagFor(FName ItemId)
{
	return FName(*(TEXT("Item_") + ItemId.ToString()));
}

void AMissionItem::BeginPlay()
{
	Super::BeginPlay();
	// The build script sets its tags whole; these two it always carries.
	Tags.AddUnique(ItemTag);
	if (!ItemId.IsNone())
	{
		Tags.AddUnique(TagFor(ItemId));
	}
	ApplyLook();
	bShownNow = true;
	if (UInteractionSubsystem* Registry = UInteractionSubsystem::Get(this))
	{
		Registry->Register(this);
	}
	if (UMissionRunner* Runner = UMissionRunner::Get(this))
	{
		BoundRunner = Runner;
		ChangedHandle = Runner->OnChanged.AddUObject(this, &AMissionItem::HandleRunnerChanged);
	}
	if (!GetMission())
	{
		UE_LOG(LogLooter, Warning, TEXT("%s: no mission has an item %s, so it's never there to take."), *GetActorNameOrLabel(), *ItemId.ToString());
	}
	Refresh();
	UpdateTouchTimer();
}

void AMissionItem::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UMissionRunner* Runner = BoundRunner.Get())
	{
		Runner->OnChanged.Remove(ChangedHandle);
	}
	BoundRunner.Reset();
	ChangedHandle.Reset();
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TouchTimer);
	}
	if (UInteractionSubsystem* Registry = UInteractionSubsystem::Get(this))
	{
		Registry->Unregister(this);
	}
	Super::EndPlay(EndPlayReason);
}

// ---------------------------------------------------------------------------
// Its item and whether it's there
// ---------------------------------------------------------------------------

const UMissionDefinition* AMissionItem::GetMission() const
{
	const UMissionRunner* Runner = UMissionRunner::Get(this);
	return Runner ? Runner->FindItemMission(ItemId) : nullptr;
}

const FMissionItemInfo* AMissionItem::GetInfo() const
{
	const UMissionDefinition* Mission = GetMission();
	return Mission ? Mission->FindItem(ItemId) : nullptr;
}

FText AMissionItem::GetItemName() const
{
	if (!DisplayName.IsEmpty())
	{
		return DisplayName;
	}
	const FMissionItemInfo* Info = GetInfo();
	return Info && !Info->Name.IsEmpty() ? Info->Name : FText::FromString(FName::NameToDisplayString(ItemId.ToString(), /*bIsBool*/ false));
}

bool AMissionItem::ShouldShow() const
{
	const UMissionRunner* Runner = UMissionRunner::Get(this);
	const UMissionDefinition* Mission = GetMission();
	if (!Runner || !Mission)
	{
		return false;
	}
	// Carried, or its mission over: gone for good.
	if (Runner->IsCarrying(ItemId) || Runner->IsCompleted(Mission->GetMissionId()))
	{
		return false;
	}
	// Fallen from its carrier: it lies there whatever the mission's state (a kill before the mission was taken counts).
	if (bDropped)
	{
		return true;
	}
	if (!ShownWhen.IsEmpty())
	{
		return ShownWhen.IsMet(Runner->GetCampaign(), Runner);
	}
	// A mission under way (taken from its giver, running here, or waiting in its area).
	return Runner->GetStatus(*Mission) == EMissionStatus::Active;
}

void AMissionItem::Refresh()
{
	SetShown(ShouldShow());
}

void AMissionItem::HandleRunnerChanged()
{
	Refresh();
}

void AMissionItem::SetShown(bool bShown)
{
	if (bShown == bShownNow)
	{
		return;
	}
	bShownNow = bShown;
	SetActorHiddenInGame(!bShown);
	SetActorEnableCollision(bShown);
	if (UInteractionSubsystem* Registry = UInteractionSubsystem::Get(this))
	{
		if (bShown)
		{
			Registry->Register(this);
		}
		else
		{
			Registry->Unregister(this);
		}
	}
	UpdateTouchTimer();
}

void AMissionItem::UpdateTouchTimer()
{
	// A touch pickup looks for the player only while it's there to take.
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	FTimerManager& Timers = World->GetTimerManager();
	if (bShownNow && bPickUpOnTouch)
	{
		if (!Timers.IsTimerActive(TouchTimer))
		{
			Timers.SetTimer(TouchTimer, FTimerDelegate::CreateUObject(this, &AMissionItem::PollTouch), TouchCheckSeconds, /*bLoop*/ true);
		}
	}
	else
	{
		Timers.ClearTimer(TouchTimer);
	}
}

void AMissionItem::ApplyLook()
{
	if (Mesh && !Mesh->GetStaticMesh())
	{
		// Its item's model when it names one; else a small stand-in, so a drop is never invisible before its art exists.
		const FMissionItemInfo* Info = GetInfo();
		UStaticMesh* Model = Info && !Info->Mesh.IsNull() ? Info->Mesh.LoadSynchronous() : nullptr;
		Mesh->SetStaticMesh(Model ? Model : StandIn.Get());
		if (!Model)
		{
			Mesh->SetRelativeScale3D(FVector(StandInScale));
		}
	}
	if (Beam && (bDropped || bShowBeam))
	{
		// The mission's own gold or white, as its giver's mark is.
		const UMissionDefinition* Mission = GetMission();
		LightBeams::Setup(Beam, MissionMarkIcons::Color(!Mission || Mission->Kind == EMissionKind::Main), BeamGlow, BeamHeight, BeamRadius);
		Beam->SetVisibility(true);
	}
}

// ---------------------------------------------------------------------------
// Taking it
// ---------------------------------------------------------------------------

FInteractionOptions AMissionItem::GetInteractionOptions(const UInteractionComponent& User) const
{
	FInteractionOptions Options;
	Options.bUsable = bShownNow && !IsFalling();
	Options.bTap = true;
	Options.bHold = false;
	// Its name after the verb as in a sentence: "Take the clamp pin", though the data writes it as a title ("The clamp pin").
	Options.TapPrompt = Prompt.IsEmpty() ? FText::Format(LOCTEXT("TakePrompt", "Take {0}"), MissionText::InSentence(GetItemName())) : Prompt;
	Options.Reach = Reach;
	return Options;
}

bool AMissionItem::Interact(UInteractionComponent& User, bool bHeld)
{
	return !bHeld && Take(User.GetOwner());
}

TOptional<FVector> AMissionItem::GetInteractionLocation() const
{
	return Mesh && Mesh->GetStaticMesh() ? TOptional<FVector>(Mesh->Bounds.Origin) : TOptional<FVector>(GetActorLocation());
}

bool AMissionItem::Take(AActor* Player)
{
	UMissionRunner* Runner = UMissionRunner::Get(this);
	if (!Runner || !bShownNow)
	{
		return false;
	}
	const FVector Where = GetInteractionLocation().Get(GetActorLocation());
	const FText Name = GetItemName();
	// Into the pocket: the campaign keeps it, the missions hear it (the runner's change hides this one).
	if (!Runner->GiveItem(ItemId, this))
	{
		return false;
	}
	LooterSound::PlayAt(this, LooterSoundCue::MissionFeatures::ItemTaken, Where);
	// A word on the HUD, in the line the weapons' notes use: what was taken, and that it's the mission's.
	const APawn* Pawn = Cast<APawn>(Player);
	const AController* Controller = Pawn ? Pawn->GetController() : Cast<AController>(Player);
	const APawn* Holder = Controller ? Controller->GetPawn() : Pawn;
	if (UWeaponManagerComponent* Weapons = Holder ? Holder->FindComponentByClass<UWeaponManagerComponent>() : nullptr)
	{
		Weapons->OnMessage.Broadcast(FText::Format(LOCTEXT("Taken", "Mission item: {0}"), Name));
	}
	UE_LOG(LogLooter, Log, TEXT("%s: %s taken."), *GetActorNameOrLabel(), *Name.ToString());
	// One that fell from a creature was never part of the level: it goes.
	if (bDropped)
	{
		Destroy();
	}
	return true;
}

void AMissionItem::PollTouch()
{
	APawn* Pawn = PlayerPawnIn(GetWorld());
	if (bShownNow && !IsFalling() && Pawn && FVector::Dist(Pawn->GetActorLocation(), GetActorLocation()) <= TouchRadius)
	{
		Take(Pawn);
	}
}

// ---------------------------------------------------------------------------
// Dropped by a creature
// ---------------------------------------------------------------------------

AMissionItem* AMissionItem::FindIn(const UWorld* World, FName ItemId)
{
	if (!World || ItemId.IsNone())
	{
		return nullptr;
	}
	for (TActorIterator<AMissionItem> It(World); It; ++It)
	{
		if (IsValid(*It) && !It->IsActorBeingDestroyed() && It->ItemId == ItemId)
		{
			return *It;
		}
	}
	return nullptr;
}

AMissionItem* AMissionItem::SpawnDrop(UWorld* World, FName ItemId, const FVector& Where)
{
	if (!World || ItemId.IsNone())
	{
		return nullptr;
	}
	// The ground under the body: it lands there, not in the air where the creature's middle was.
	FVector Ground = Where;
	FHitResult Hit;
	const FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(World, TEXT("MissionItemDrop"));
	if (World->LineTraceSingleByObjectType(Hit, Where + FVector(0.0, 0.0, 100.0), Where - FVector(0.0, 0.0, 600.0),
		FCollisionObjectQueryParams(ECC_WorldStatic), Params))
	{
		Ground = Hit.ImpactPoint;
	}
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.bDeferConstruction = true;
	AMissionItem* Item = World->SpawnActor<AMissionItem>(Ground, FRotator(0.0, FMath::FRandRange(0.0, 360.0), 0.0), SpawnParams);
	if (!Item)
	{
		return nullptr;
	}
	Item->ItemId = ItemId;
	Item->bDropped = true;
	Item->bPickUpOnTouch = true;
	Item->FinishSpawning(FTransform(FRotator(0.0, FMath::FRandRange(0.0, 360.0), 0.0), Ground));
	// It falls from the body to the ground, then lies there under its beam.
	Item->FallFrom = Where + FVector(0.0, 0.0, FallRise);
	Item->FallTo = Ground;
	Item->FallAge = 0.f;
	Item->SetActorLocation(Item->FallFrom);
	Item->SetActorTickEnabled(true);
	UE_LOG(LogLooter, Log, TEXT("Mission item %s dropped at (%.0f, %.0f, %.0f)."), *ItemId.ToString(), Ground.X, Ground.Y, Ground.Z);
	return Item;
}

void AMissionItem::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	AdvanceFall(DeltaSeconds);
}

void AMissionItem::AdvanceFall(float DeltaSeconds)
{
	if (FallAge < 0.f)
	{
		SetActorTickEnabled(false);
		return;
	}
	FallAge += DeltaSeconds;
	const float Share = FMath::Clamp(FallAge / FallSeconds, 0.f, 1.f);
	// Falling faster as it goes (a drop, not a glide), then a little hop as it lands.
	const float Down = Share * Share;
	const float Hop = Share > 0.8f ? FMath::Sin((Share - 0.8f) / 0.2f * UE_PI) * FallHop : 0.f;
	SetActorLocation(FMath::Lerp(FallFrom, FallTo, Down) + FVector(0.0, 0.0, Hop));
	if (Share >= 1.f)
	{
		FallAge = -1.f;
		SetActorLocation(FallTo);
		SetActorTickEnabled(false);
		LooterSound::PlayAt(this, LooterSoundCue::MissionFeatures::ItemDrop, FallTo);
	}
}

#undef LOCTEXT_NAMESPACE

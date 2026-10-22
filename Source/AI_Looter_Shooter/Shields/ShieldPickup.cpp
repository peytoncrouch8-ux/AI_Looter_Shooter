#include "Shields/ShieldPickup.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Audio/LooterSoundCues.h"
#include "Interaction/InteractionComponent.h"
#include "Interaction/InteractionSubsystem.h"
#include "Loot/LootDropMoments.h"
#include "Loot/LootFanfareSubsystem.h"
#include "Loot/LootTossComponent.h"
#include "Shields/PlayerShieldComponent.h"
#include "Shields/ShieldDefinition.h"
#include "Shields/ShieldRules.h"
#include "Shields/ShieldText.h"
#include "UI/World/ShieldLabelWidget.h"
#include "World/LightBeam.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/RotatingMovementComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
	/** The box's top at the pickup's size, where the beam starts (the model is about 13 cm tall). */
	constexpr float BoxTop = 13.f * AShieldPickup::ModelScale;
	/** The loot's reach (UWeaponManagerComponent::PickupRange) and hold (PickupHoldSeconds): a ward is taken as a gun is. */
	constexpr float PickupReach = 250.f;
	constexpr float WearHoldSeconds = 0.4f;
	/** How often it looks round for the player once landed (its label, its life). */
	constexpr float WatchSeconds = 0.2f;
}

AShieldPickup::AShieldPickup()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = WatchSeconds;

	// Root: a small sphere that only bounces off world geometry while tossed (as the ammo and grenade pickups do).
	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	Collision->InitSphereRadius(12.f);
	Collision->SetCollisionObjectType(ECC_WorldDynamic);
	Collision->SetCollisionResponseToAllChannels(ECR_Ignore);
	Collision->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SetRootComponent(Collision);

	Model = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Model"));
	Model->SetupAttachment(Collision);
	// Its own collision answers the crosshair (the interaction's line), nothing else.
	Model->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	Model->SetRelativeLocation(FVector(0.f, 0.f, -10.f));
	Model->SetRelativeScale3D(FVector(ModelScale));
	Model->SetCastShadow(true);

	Beam = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Beam"));
	Beam->SetupAttachment(Collision);
	Beam->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	Beam->SetCastShadow(false);
	Beam->SetCanEverAffectNavigation(false);

	Label = CreateDefaultSubobject<UWidgetComponent>(TEXT("Label"));
	Label->SetupAttachment(Collision);
	Label->SetUsingAbsoluteRotation(true);
	Label->SetRelativeLocation(FVector(0.f, 0.f, 55.f));
	Label->SetWidgetSpace(EWidgetSpace::Screen);
	Label->SetDrawAtDesiredSize(true);
	Label->SetPivot(FVector2D(0.5f, 1.f));
	Label->SetWidgetClass(UShieldLabelWidget::StaticClass());
	Label->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Label->SetVisibility(false);

	TossMovement = CreateDefaultSubobject<ULootTossComponent>(TEXT("TossMovement"));
	TossMovement->SetUpdatedComponent(Collision);
	TossMovement->Bounciness = 0.25f;

	Spin = CreateDefaultSubobject<URotatingMovementComponent>(TEXT("Spin"));
	Spin->RotationRate = FRotator(0.f, 70.f, 0.f);
	Spin->SetUpdatedComponent(Model);
}

AShieldPickup* AShieldPickup::SpawnWard(UWorld* World, const FShieldInstanceData& Ward, const FVector& Location)
{
	if (!World || !Ward.IsSet())
	{
		return nullptr;
	}
	const FTransform Transform(FRotator(0.f, FMath::FRandRange(0.f, 360.f), 0.f), Location);
	AShieldPickup* Pickup = World->SpawnActorDeferred<AShieldPickup>(AShieldPickup::StaticClass(), Transform, nullptr, nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (Pickup)
	{
		Pickup->Ward = Ward;
		Pickup->Ward.Stats = ShieldRules::ComputeStats(Ward);
		Pickup->FinishSpawning(Transform);
	}
	return Pickup;
}

AShieldPickup* AShieldPickup::DropWard(UWorld* World, const FShieldInstanceData& Ward, const FVector& Location, const FVector& Velocity)
{
	AShieldPickup* Pickup = SpawnWard(World, Ward, Location);
	if (Pickup && !Velocity.IsNearlyZero())
	{
		Pickup->Toss(Velocity);
		// Put down by the player: no landing sting (it isn't news).
		Pickup->bTossed = false;
	}
	return Pickup;
}

void AShieldPickup::BeginPlay()
{
	Super::BeginPlay();
	LifeLeft = LifeSeconds;
	ApplyLook();
	TossMovement->OnProjectileStop.AddDynamic(this, &AShieldPickup::HandleTossStopped);
	if (UInteractionSubsystem* Registry = UInteractionSubsystem::Get(this))
	{
		Registry->Register(this);
	}
	if (UShieldLabelWidget* Widget = Cast<UShieldLabelWidget>(Label->GetUserWidgetObject()))
	{
		Widget->SetWard(Ward, nullptr);
	}
}

void AShieldPickup::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UInteractionSubsystem* Registry = UInteractionSubsystem::Get(this))
	{
		Registry->Unregister(this);
	}
	Super::EndPlay(EndPlayReason);
}

void AShieldPickup::ApplyLook()
{
	const UShieldDefinition* Definition = Ward.Definition;
	UStaticMesh* Mesh = Definition ? Definition->LoadModel(Ward.Rarity) : nullptr;
	if (Mesh)
	{
		Model->SetStaticMesh(Mesh);
		// The coal window glows in the rarity's colour (the flat glow material's Color and Glow, as a gun's accent strip).
		const int32 GlowIndex = Model->GetMaterialIndex(Definition->GlowSlot);
		if (GlowIndex != INDEX_NONE)
		{
			GlowMaterial = Model->CreateDynamicMaterialInstance(GlowIndex);
			if (GlowMaterial)
			{
				const FShieldRarityInfo& Tier = Definition->GetRarityInfo(Ward.Rarity);
				GlowMaterial->SetVectorParameterValue(TEXT("Color"), Tier.Color);
				GlowMaterial->SetScalarParameterValue(TEXT("Glow"), Tier.Glow);
			}
		}
	}
	else
	{
		// Before the model's import: the engine's cube, box-sized.
		Model->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
		Model->SetRelativeScale3D(FVector(0.14f, 0.07f, 0.11f) * ModelScale);
	}

	// Its rarity's beam, the guns' own numbers (Looter.Loot.Drop.Beam.*), so a ward reads like a gun of its rarity.
	const FDropMoment Moment = LootDropMoments::For(Ward.Rarity);
	BeamGlow = Moment.BeamGlow;
	BeamHeight = Moment.BeamHeight;
	BeamRadius = Moment.BeamRadius;
	Beam->SetVisibility(BeamHeight > 0.f);
	if (BeamHeight > 0.f)
	{
		LightBeams::Setup(Beam, ShieldText::Color(Ward), BeamGlow, BeamHeight, BeamRadius);
		Beam->SetRelativeLocation(FVector(0.0, 0.0, Model->GetRelativeLocation().Z + BoxTop + BeamHeight * 0.5));
	}
}

void AShieldPickup::Toss(const FVector& Velocity)
{
	bTossed = true;
	TossMovement->Throw(Velocity);
}

void AShieldPickup::HandleTossStopped(const FHitResult& ImpactResult)
{
	SetActorRotation(FRotator(0.f, GetActorRotation().Yaw, 0.f));
	LooterSound::PlayAt(this, FName(LooterSoundCue::LootLand), GetActorLocation());
	if (!bTossed)
	{
		return;
	}
	bTossed = false;
	// Rare and up: heard and seen as it lands, as a gun of its rarity is (ULootFanfareSubsystem's cues and flare).
	const FName Sting = ULootFanfareSubsystem::DropCue(Ward.Rarity);
	if (!Sting.IsNone())
	{
		LooterSound::PlayAt(this, Sting, GetActorLocation());
	}
	if (ULootFanfareSubsystem::FlaresBeam(Ward.Rarity) && BeamHeight > 0.f)
	{
		FlareAge = 0.f;
		SetActorTickInterval(0.f);
	}
}

void AShieldPickup::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (FlareAge < LightBeams::FlareSeconds)
	{
		FlareAge += DeltaSeconds;
		LightBeams::Flare(Beam, FMath::Min(FlareAge, LightBeams::FlareSeconds), Ward.Rarity == EWeaponRarity::Legendary, BeamGlow, BeamHeight, BeamRadius);
		if (FlareAge >= LightBeams::FlareSeconds)
		{
			SetActorTickInterval(WatchSeconds);
		}
	}
	WatchClock += DeltaSeconds;
	if (WatchClock >= WatchSeconds)
	{
		const float Elapsed = WatchClock;
		WatchClock = 0.f;
		LifeLeft -= Elapsed;
		Watch();
	}
}

void AShieldPickup::Watch()
{
	const UWorld* World = GetWorld();
	const APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	const APawn* Player = PC ? PC->GetPawn() : nullptr;
	const bool bNear = Player && FVector::DistSquared(Player->GetActorLocation(), GetActorLocation()) <= FMath::Square(NearDistance);
	if (bNear)
	{
		// Never gone from under the player's nose (the guns' Lost Loot rule).
		LifeLeft = FMath::Max(LifeLeft, 10.f);
	}
	else if (LifeLeft <= 0.f)
	{
		UE_LOG(LogLooter, Verbose, TEXT("%s: a ward left lying, gone"), *GetName());
		Destroy();
		return;
	}
	const UInteractionComponent* Interaction = Player ? Player->FindComponentByClass<UInteractionComponent>() : nullptr;
	const bool bFocused = Interaction && Interaction->GetFocusedActor() == this;
	Label->SetVisibility(bNear);
	if (UShieldLabelWidget* Widget = Cast<UShieldLabelWidget>(Label->GetUserWidgetObject()))
	{
		const UPlayerShieldComponent* Shield = UPlayerShieldComponent::Find(Player);
		Widget->SetWard(Ward, Shield && Shield->HasWard() ? &Shield->GetWard() : nullptr);
		Widget->SetFocused(bFocused);
	}
}

FInteractionOptions AShieldPickup::GetInteractionOptions(const UInteractionComponent& User) const
{
	const UPlayerShieldComponent* Shield = UPlayerShieldComponent::Find(User.GetOwner());
	FInteractionOptions Options;
	Options.bUsable = Shield != nullptr && Ward.IsSet();
	Options.Reach = PickupReach;
	Options.bHold = true;
	Options.HoldSeconds = WearHoldSeconds;
	Options.HoldPrompt = FText::FromString(TEXT("Wear"));
	// A tap takes it: worn when none is, else a spare; with the spares full only the hold wears it (the worn one drops).
	const bool bWears = Shield && !Shield->HasWard();
	Options.bTap = Shield && (bWears || Shield->HasSpareRoom());
	Options.TapPrompt = FText::FromString(bWears ? TEXT("Wear") : TEXT("Pick up"));
	if (bWears)
	{
		Options.bHold = false;
	}
	return Options;
}

bool AShieldPickup::Interact(UInteractionComponent& User, bool bHeld)
{
	UPlayerShieldComponent* Shield = UPlayerShieldComponent::Find(User.GetOwner());
	if (!Shield || !Ward.IsSet())
	{
		return false;
	}
	const bool bTaken = bHeld ? Shield->Equip(Ward, false) : Shield->Take(Ward);
	if (!bTaken)
	{
		LooterSound::Play2D(this, FName(LooterSoundCue::Denied));
		return false;
	}
	UE_LOG(LogLooter, Log, TEXT("Shield: %s %s %s (lv %d)"), bHeld ? TEXT("wore") : TEXT("took"), *UEnum::GetDisplayValueAsText(Ward.Rarity).ToString(),
		*ShieldText::Name(Ward), Ward.Level);
	Ward = FShieldInstanceData();
	Destroy();
	return true;
}

// ARappelSpawner: an encounter whose creatures slide down ropes off a tender or down the barge's hose.

#include "Creatures/RappelSpawner.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/DredgerCreature.h"
#include "Creatures/EncounterSubsystem.h"
#include "World/CompanyTenderDrop.h"
#include "World/LamplighterEffects.h"
#include "Components/CapsuleComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "Materials/MaterialInterface.h"
#include "Math/RotationMatrix.h"
#include "Misc/PackageName.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const TCHAR* RopeCylinderPath = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
	const TCHAR* RopeHempPath = TEXT("/Game/Art/Materials/MI_RopeHemp.MI_RopeHemp");
	/** The rope: this thick (cm); it hangs from the top to the man's hands, this far over his middle. */
	constexpr float RopeWidth = 3.5f;
	constexpr float HandsOverMiddle = 70.f;
	/** He hangs under the rope's top this far (cm, from his middle) before he lets himself go. */
	constexpr float HangUnder = 120.f;
	/** A drop's men go down one rope after another, each this share of a slide after the one before on it. */
	constexpr float NextOnRopeShare = 0.7f;
	/** Spawns this close together (s) are one drop. */
	constexpr double SameDrop = 1.0;
	/** He sways on the rope this far (cm), this fast. */
	constexpr float SwayWidth = 22.f;
	constexpr float SwaySpeed = 2.4f;

	template <typename T>
	T* RopeAssetIfMade(const TCHAR* ObjectPath)
	{
		if (!FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(FString(ObjectPath))))
		{
			return nullptr;
		}
		ConstructorHelpers::FObjectFinder<T> Finder(ObjectPath);
		return Finder.Object;
	}
}

ARappelSpawner::ARappelSpawner()
{
	// It ticks only while someone is on a rope or a rope hangs (its waves keep the encounter's own timer); after the
	// creatures' own movement, so a man on a rope is where his slide puts him.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	PrimaryActorTick.TickGroup = TG_PostUpdateWork;

	static UStaticMesh* const Cylinder = RopeAssetIfMade<UStaticMesh>(RopeCylinderPath);
	static UMaterialInterface* const Hemp = RopeAssetIfMade<UMaterialInterface>(RopeHempPath);
	Ropes = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Ropes"));
	Ropes->SetupAttachment(RootComponent);
	Ropes->SetMobility(EComponentMobility::Movable);
	Ropes->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Ropes->SetCanEverAffectNavigation(false);
	Ropes->SetCastShadow(false);
	Ropes->SetStaticMesh(Cylinder);
	if (Hemp)
	{
		Ropes->SetMaterial(0, Hemp);
	}
}

void ARappelSpawner::BeginPlay()
{
	Super::BeginPlay();
	Tender = FindTender();
	Sliders.Reset();
	RopeFeet.Reset();
	bRopesDown = false;
	if (Ropes)
	{
		Ropes->ClearInstances();
	}
}

ACompanyTenderDrop* ARappelSpawner::FindTender() const
{
	UWorld* World = GetWorld();
	if (!World || AnchorTag.IsNone())
	{
		return nullptr;
	}
	for (TActorIterator<ACompanyTenderDrop> It(World); It; ++It)
	{
		if (It->ActorHasTag(AnchorTag))
		{
			return *It;
		}
	}
	return nullptr;
}

bool ARappelSpawner::CheckApproach()
{
	// With a tender, the first drop waits for it to hang over the yard: nobody slides down a rope that isn't there.
	if (const ACompanyTenderDrop* Found = Tender.Get(); Found && !Found->IsHovering())
	{
		return false;
	}
	return Super::CheckApproach();
}

int32 ARappelSpawner::NumRopes() const
{
	if (const ACompanyTenderDrop* Found = Tender.Get())
	{
		return Found->NumRopeAnchors();
	}
	return RopeTops.Num();
}

FVector ARappelSpawner::GetRopeTop(int32 Index) const
{
	if (const ACompanyTenderDrop* Found = Tender.Get())
	{
		return Found->GetRopeAnchor(Index);
	}
	return RopeTops.IsValidIndex(Index) ? RopeTops[Index] : GetActorLocation();
}

float ARappelSpawner::SlideAlpha(float Time, float Seconds)
{
	const float T = FMath::Clamp(Time / FMath::Max(Seconds, 0.05f), 0.f, 1.f);
	// Let go and dropping (most of the way in the first two thirds), then the brake: the gloves bite and he slows to his feet.
	return T < 0.7f ? 0.85f * FMath::Square(T / 0.7f) : 0.85f + 0.15f * FMath::Sin((T - 0.7f) / 0.3f * static_cast<float>(UE_HALF_PI));
}

void ARappelSpawner::OnCreatureSpawned(ACreatureBase& Creature)
{
	const int32 Count = NumRopes();
	if (Count <= 0)
	{
		// No ropes (no tender, no tops): it appears where it was spawned, as any encounter's.
		Creature.PlayEntranceIfSeen();
		return;
	}
	const UWorld* World = GetWorld();
	const double Now = World ? World->GetTimeSeconds() : 0.0;
	Turn = Now - LastSpawn <= SameDrop ? Turn + 1 : 0;
	LastSpawn = Now;

	FSlider& Slider = Sliders.AddDefaulted_GetRef();
	Slider.Creature = &Creature;
	Slider.Feet = Creature.GetActorLocation();
	TArray<FVector> Tops;
	for (int32 Index = 0; Index < Count; ++Index)
	{
		Tops.Add(GetRopeTop(Index));
	}
	Slider.Rope = PickRope(Tops, Slider.Feet, Turn, bRopeNearestFeet);
	// A drop's men go down one rope after another: the second on a rope waits at its top.
	if (Turn == 0 || DropOnRope.Num() != Count)
	{
		DropOnRope.Init(0, Count);
	}
	const int32 Ahead = bRopeNearestFeet ? DropOnRope[Slider.Rope] : Turn / Count;
	++DropOnRope[Slider.Rope];
	Slider.Clock = -Ahead * SlideSeconds * NextOnRopeShare;
	Slider.Sway = FMath::FRandRange(0.f, 6.f);
	// Held, passive, out of the fight until he's down (the spawner's call to hunt is refused while he's passive).
	Creature.SetPassive(true);
	if (UCharacterMovementComponent* Movement = Creature.GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
		Movement->SetMovementMode(MOVE_Flying);
	}
	const FVector Top = GetRopeTop(Slider.Rope);
	Creature.SetActorLocation(Top - FVector(0.0, 0.0, HangUnder), false, nullptr, ETeleportType::TeleportPhysics);
	// One waiting his turn isn't seen yet (he's still aboard).
	Creature.SetActorHiddenInGame(Slider.Clock < 0.f);
	if (RopeFeet.Num() < Count)
	{
		RopeFeet.SetNum(Count);
	}
	RopeFeet[Slider.Rope] = Slider.Feet;
	if (!bRopesDown || Slider.Clock >= 0.f)
	{
		// The rope thrown down: a pop of air and the line whipping out.
		LooterSound::PlayAt(this, LooterSoundCue::GrappleFire, Top, 0.6f, 1.3f);
	}
	bRopesDown = true;
	RopesHung = 0.f;
	SetActorTickEnabled(true);
}

int32 ARappelSpawner::PickRope(TConstArrayView<FVector> Tops, const FVector& Feet, int32 Turn, bool bNearest)
{
	if (Tops.IsEmpty())
	{
		return INDEX_NONE;
	}
	if (!bNearest)
	{
		return FMath::Max(Turn, 0) % Tops.Num();
	}
	int32 Best = 0;
	for (int32 Index = 1; Index < Tops.Num(); ++Index)
	{
		if (FVector::DistSquared2D(Tops[Index], Feet) < FVector::DistSquared2D(Tops[Best], Feet))
		{
			Best = Index;
		}
	}
	return Best;
}

void ARappelSpawner::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	AdvanceRopes(DeltaSeconds);
}

void ARappelSpawner::AdvanceRopes(float DeltaSeconds)
{
	for (int32 Index = Sliders.Num() - 1; Index >= 0; --Index)
	{
		if (StepSlider(Sliders[Index], DeltaSeconds))
		{
			Sliders.RemoveAtSwap(Index);
		}
	}
	// Without a tender the ropes hang on a while after the last man is down; with one, while it hovers.
	const ACompanyTenderDrop* Found = Tender.Get();
	if (Sliders.IsEmpty() && bRopesDown)
	{
		RopesHung += DeltaSeconds;
		const bool bHold = Found ? Found->IsHovering() : RopesHung < RopeLingerSeconds;
		if (!bHold)
		{
			bRopesDown = false;
		}
	}
	PlaceRopes();
	if (Sliders.IsEmpty() && !bRopesDown)
	{
		SetActorTickEnabled(false);
	}
}

bool ARappelSpawner::StepSlider(FSlider& Slider, float DeltaSeconds)
{
	ACreatureBase* Creature = Slider.Creature.Get();
	if (!Creature)
	{
		return true;
	}
	if (Creature->IsDead())
	{
		// Shot off the rope: he falls.
		Creature->SetActorHiddenInGame(false);
		if (UCharacterMovementComponent* Movement = Creature->GetCharacterMovement())
		{
			Movement->SetMovementMode(MOVE_Falling);
		}
		return true;
	}
	const bool bWaited = Slider.Clock < 0.f;
	Slider.Clock += DeltaSeconds;
	const FVector Top = GetRopeTop(Slider.Rope);
	if (Slider.Clock < 0.f)
	{
		// Still aboard, his turn to come: kept at the rope's top, unseen.
		Creature->SetActorLocation(Top - FVector(0.0, 0.0, HangUnder), false, nullptr, ETeleportType::TeleportPhysics);
		return false;
	}
	if (bWaited)
	{
		Creature->SetActorHiddenInGame(false);
	}
	const float Alpha = SlideAlpha(Slider.Clock, SlideSeconds);
	const FVector Start = Top - FVector(0.0, 0.0, HangUnder);
	FVector At = FMath::Lerp(Start, Slider.Feet, Alpha);
	// A sway on the rope that dies as he brakes.
	const float SwayNow = SwayWidth * (1.f - Alpha) * FMath::Sin(Slider.Clock * SwaySpeed + Slider.Sway);
	At += FVector(SwayNow, SwayNow * 0.6f, 0.0);
	Creature->SetActorLocation(At, false, nullptr, ETeleportType::TeleportPhysics);
	if (UCharacterMovementComponent* Movement = Creature->GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
	}
	if (Slider.Clock >= SlideSeconds)
	{
		Land(*Creature, Slider.Feet);
		return true;
	}
	return false;
}

void ARappelSpawner::Land(ACreatureBase& Creature, const FVector& Feet)
{
	Creature.SetActorLocation(Feet, false, nullptr, ETeleportType::TeleportPhysics);
	if (UCharacterMovementComponent* Movement = Creature.GetCharacterMovement())
	{
		Movement->SetMovementMode(MOVE_Walking);
	}
	Creature.SetPassive(false);
	const float HalfHeight = Creature.GetCapsuleComponent() ? Creature.GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 90.f;
	const FVector Ground = Feet - FVector(0.0, 0.0, HalfHeight);
	LamplighterEffects::DustSkirt(this, Ground, 90.f, 3);
	LooterSound::PlayAt(this, LooterSoundCue::ImpactWood, Ground, 0.5f, 0.7f);
	if (ADredgerCreature* Dredger = Cast<ADredgerCreature>(&Creature))
	{
		Dredger->Bark(LooterSoundCue::DredgerBark, 0.6f);
	}
	const UEncounterSubsystem* Encounters = GetEncounters();
	if (APawn* Player = Encounters ? Encounters->GetPlayer() : nullptr; Player && bHuntOnSpawn)
	{
		Creature.AlertTo(Player);
	}
}

void ARappelSpawner::PlaceRopes()
{
	if (!Ropes)
	{
		return;
	}
	TArray<FTransform> Transforms;
	if (bRopesDown)
	{
		const int32 Count = NumRopes();
		// Each rope from its top to whoever is on it (his hands), else down to its foot.
		TArray<FVector> Ends;
		Ends.SetNum(Count);
		for (int32 Rope = 0; Rope < Count; ++Rope)
		{
			const FVector Foot = RopeFeet.IsValidIndex(Rope) && !RopeFeet[Rope].IsNearlyZero() ? RopeFeet[Rope] : GetRopeTop(Rope) - FVector(0.0, 0.0, 1200.0);
			Ends[Rope] = Foot - FVector(0.0, 0.0, 80.0);
		}
		for (const FSlider& Slider : Sliders)
		{
			const ACreatureBase* Creature = Slider.Creature.Get();
			if (Creature && Slider.Clock >= 0.f && Ends.IsValidIndex(Slider.Rope))
			{
				Ends[Slider.Rope] = Creature->GetActorLocation() + FVector(0.0, 0.0, HandsOverMiddle);
			}
		}
		for (int32 Rope = 0; Rope < Count; ++Rope)
		{
			const FVector Top = GetRopeTop(Rope);
			const FVector Span = Ends[Rope] - Top;
			const double Length = Span.Size();
			if (Length < 10.0)
			{
				continue;
			}
			Transforms.Add(FTransform(FRotationMatrix::MakeFromZ(Span / Length).ToQuat(), (Top + Ends[Rope]) * 0.5,
				FVector(RopeWidth / 100.0, RopeWidth / 100.0, Length / 100.0)));
		}
	}
	if (Ropes->GetInstanceCount() != Transforms.Num())
	{
		Ropes->ClearInstances();
		if (!Transforms.IsEmpty())
		{
			Ropes->AddInstances(Transforms, /*bShouldReturnIndices*/ false, /*bWorldSpace*/ true);
		}
	}
	else if (!Transforms.IsEmpty())
	{
		Ropes->BatchUpdateInstancesTransforms(0, Transforms, /*bWorldSpace*/ true, /*bMarkRenderStateDirty*/ true, /*bTeleport*/ true);
	}
}

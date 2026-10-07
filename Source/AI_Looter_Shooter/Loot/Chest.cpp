// AChest: a loot chest opened once with Interact (the Ranger caches and the gang's Strongbox, step 26): its models by
// kind, being used, the wheel and the lid, and staying open with the session. ChestLoot.cpp is what it gives.

#include "Loot/Chest.h"
#include "AI_Looter_Shooter.h"
#include "Interaction/InteractionComponent.h"
#include "Interaction/InteractionSubsystem.h"
#include "Missions/MissionRunner.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Misc/PackageName.h"
#include "UObject/UObjectGlobals.h"

const FName AChest::ChestTag(TEXT("Chest"));
const FName AChest::LidSocket(TEXT("Lid"));
const FName AChest::LootSocket(TEXT("Loot"));
const FName AChest::WheelSocket(TEXT("Wheel"));

namespace
{
	/** The tag the minimap and the scatter read on solid things standing on the ground. */
	const FName ObstacleTag(TEXT("Obstacle"));

	/** Every kind, for telling a kind's own model from one the build script chose. */
	constexpr EChestKind AllKinds[] = { EChestKind::SupplyCrate, EChestKind::Strongbox };

	/** A model by its object path, quietly null while it isn't imported. */
	UStaticMesh* LoadModel(const TCHAR* Path)
	{
		if (!Path || !FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(FString(Path))))
		{
			return nullptr;
		}
		return LoadObject<UStaticMesh>(nullptr, Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
	}

	/** Mesh is some kind's own model (or none), so it follows the kind; anything else was chosen for this chest. */
	bool IsKindModel(const UStaticMesh* Mesh)
	{
		if (!Mesh)
		{
			return true;
		}
		const FString Path = Mesh->GetPathName();
		for (const EChestKind Each : AllKinds)
		{
			const FChestKindInfo Info = FChestKindInfo::Get(Each);
			for (const TCHAR* Own : { Info.BodyPath, Info.LidPath, Info.WheelPath })
			{
				if (Own && Path == Own)
				{
					return true;
				}
			}
		}
		return false;
	}

	/** Gives Part its kind's model, unless the build script chose another for it. */
	void FollowKind(UStaticMeshComponent* Part, const TCHAR* Path)
	{
		if (Part && IsKindModel(Part->GetStaticMesh()))
		{
			Part->SetStaticMesh(LoadModel(Path));
		}
	}

	UStaticMeshComponent* MakePart(AActor* Owner, const TCHAR* Name)
	{
		UStaticMeshComponent* Made = Owner->CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Made->SetMobility(EComponentMobility::Movable);
		Made->SetGenerateOverlapEvents(false);
		Made->SetCanEverAffectNavigation(false);
		return Made;
	}
}

FChestKindInfo FChestKindInfo::Get(EChestKind Kind)
{
	FChestKindInfo Info;
	switch (Kind)
	{
	case EChestKind::Strongbox:
		// Chests.py strongbox(): 0.82 x 0.6 m on 4.5 cm feet, 0.445 m to the lid; the hinge 3 cm behind its back.
		Info.BodyPath = TEXT("/Game/Art/Loot/SM_Strongbox.SM_Strongbox");
		Info.LidPath = TEXT("/Game/Art/Loot/SM_Strongbox_Lid.SM_Strongbox_Lid");
		Info.WheelPath = TEXT("/Game/Art/Loot/SM_Strongbox_Wheel.SM_Strongbox_Wheel");
		Info.OpenAngle = 102.f;
		Info.WheelTurns = 1.5f;
		Info.Guns = 2;
		Info.Luck = 1.f;
		Info.AmmoPickups = 2;
		Info.Hinge = FVector(-33.0, 0.0, 44.5);
		Info.LootPoint = FVector(0.0, 0.0, 13.5);
		Info.WheelHub = FVector(31.2, 0.0, 24.5);
		Info.Height = 44.5f;
		break;
	case EChestKind::SupplyCrate:
	default:
		// Chests.py supply_crate(): 1.15 m across its front, 0.52 m deep, 0.40 m to the lid; the hinge just behind its back.
		Info.BodyPath = TEXT("/Game/Art/Loot/SM_SupplyCrate.SM_SupplyCrate");
		Info.LidPath = TEXT("/Game/Art/Loot/SM_SupplyCrate_Lid.SM_SupplyCrate_Lid");
		Info.OpenAngle = 112.f;
		Info.Guns = 1;
		Info.Luck = 0.5f;
		Info.AmmoPickups = 2;
		Info.Hinge = FVector(-28.2, 0.0, 40.0);
		Info.LootPoint = FVector(0.0, 0.0, 24.0);
		Info.Height = 40.f;
		break;
	}
	return Info;
}

AChest::AChest()
{
	// Only while the wheel or the lid moves.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;

	// Solid, so it's stood on and found by the Interact key's line; world dynamic, so ground traces (a creature's footing,
	// the scatter, settling props) never take it for the ground and loot, which lands on world static only, flies out of
	// it rather than catching on its walls.
	Body = MakePart(this, TEXT("Body"));
	Body->SetupAttachment(Root);
	Body->SetCollisionProfileName(UCollisionProfile::BlockAllDynamic_ProfileName);

	Lid = MakePart(this, TEXT("Lid"));
	Lid->SetupAttachment(Body, LidSocket);
	Lid->SetCollisionProfileName(UCollisionProfile::BlockAllDynamic_ProfileName);

	// The wheel is a hand's size on the front: nothing to stand on or shoot at (Chests.py gives it no collision).
	Wheel = MakePart(this, TEXT("Wheel"));
	Wheel->SetupAttachment(Body, WheelSocket);
	Wheel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Wheel->SetCastShadow(false);

	Tags.Add(ChestTag);
	Tags.Add(ObstacleTag);
}

void AChest::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyKind();
}

void AChest::BeginPlay()
{
	Super::BeginPlay();
	// The build script sets its tags whole; these it always carries.
	Tags.AddUnique(ChestTag);
	Tags.AddUnique(ObstacleTag);
	if (UInteractionSubsystem* Registry = UInteractionSubsystem::Get(this))
	{
		Registry->Register(this);
	}
}

void AChest::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UInteractionSubsystem* Registry = UInteractionSubsystem::Get(this))
	{
		Registry->Unregister(this);
	}
	Super::EndPlay(EndPlayReason);
}

void AChest::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Advance(DeltaSeconds);
}

// ---------------------------------------------------------------------------
// Its kind
// ---------------------------------------------------------------------------

void AChest::ApplyKind()
{
	const FChestKindInfo Info = GetKindInfo();
	FollowKind(Body, Info.BodyPath);
	FollowKind(Lid, Info.LidPath);
	FollowKind(Wheel, Info.WheelPath);
	// On the model's sockets; without the model, where they would be on it.
	Lid->SetRelativeLocation(Body->DoesSocketExist(LidSocket) ? FVector::ZeroVector : Info.Hinge);
	Wheel->SetRelativeLocation(Body->DoesSocketExist(WheelSocket) ? FVector::ZeroVector : Info.WheelHub);
	Wheel->SetVisibility(Info.WheelTurns > 0.f);
	// A socket that appeared with the model moves its part even when the relative place didn't change.
	Lid->UpdateComponentToWorld();
	Wheel->UpdateComponentToWorld();
	const bool bOpen = State == EChestState::Open;
	SetLidAngle(bOpen ? Info.OpenAngle : LidAngle);
	SetWheelAngle(bOpen ? Info.WheelTurns * 360.f : WheelAngle);
}

FText AChest::DefaultPrompt(EChestKind ForKind)
{
	return ForKind == EChestKind::Strongbox ? NSLOCTEXT("LooterChest", "OpenStrongbox", "Open the strongbox")
		: NSLOCTEXT("LooterChest", "OpenCrate", "Open the crate");
}

FName AChest::GetSaveKey() const
{
	return ChestId.IsNone() ? GetFName() : ChestId;
}

// ---------------------------------------------------------------------------
// Being used
// ---------------------------------------------------------------------------

FInteractionOptions AChest::GetInteractionOptions(const UInteractionComponent& User) const
{
	FInteractionOptions Options;
	// A tap, once: open, it's never focused again, and the loot it threw out takes the key.
	Options.bUsable = CanOpen();
	Options.bTap = true;
	Options.TapPrompt = Prompt.IsEmpty() ? DefaultPrompt(Kind) : Prompt;
	Options.Reach = Reach;
	return Options;
}

bool AChest::Interact(UInteractionComponent& User, bool bHeld)
{
	return Open(User.GetOwner());
}

TOptional<FVector> AChest::GetInteractionLocation() const
{
	// The middle of the chest, where hands go to the lid; without its model, halfway up where it would be.
	if (Body && Body->GetStaticMesh())
	{
		return Body->Bounds.Origin;
	}
	return GetActorTransform().TransformPosition(FVector(0.0, 0.0, GetKindInfo().Height * 0.5));
}

bool AChest::CanOpen() const
{
	if (State != EChestState::Closed)
	{
		return false;
	}
	if (OpenWhen.IsEmpty())
	{
		return true;
	}
	const UMissionRunner* Runner = UMissionRunner::Get(this);
	return Runner && OpenWhen.IsMet(Runner->GetCampaign(), Runner);
}

bool AChest::Open(AActor* ByWhom, bool bForce)
{
	if (State != EChestState::Closed || (!bForce && !CanOpen()))
	{
		return false;
	}
	// The Strongbox's wheel first, then the lid.
	State = GetKindInfo().WheelTurns > 0.f ? EChestState::Unlocking : EChestState::Opening;
	Clock = 0.f;
	SetActorTickEnabled(true);
	UE_LOG(LogLooter, Log, TEXT("%s (%s): opened%s."), *GetActorNameOrLabel(), *GetSaveKey().ToString(),
		ByWhom ? *FString::Printf(TEXT(" by %s"), *ByWhom->GetName()) : TEXT(""));
	return true;
}

void AChest::Advance(float DeltaSeconds)
{
	const FChestKindInfo Info = GetKindInfo();
	float Left = DeltaSeconds;
	if (State == EChestState::Unlocking)
	{
		Clock += Left;
		Left = 0.f;
		const float Spin = FMath::Max(WheelSeconds, KINDA_SMALL_NUMBER);
		if (Clock < Spin)
		{
			SetWheelAngle(Info.WheelTurns * 360.f * FMath::SmoothStep(0.f, Spin, Clock));
		}
		else
		{
			// Unlocked: what's left of the frame goes to the lid.
			SetWheelAngle(Info.WheelTurns * 360.f);
			Left = Clock - Spin;
			Clock = 0.f;
			State = EChestState::Opening;
		}
	}
	if (State == EChestState::Opening)
	{
		Clock += Left;
		const float Share = FMath::Clamp(Clock / FMath::Max(LidSeconds, KINDA_SMALL_NUMBER), 0.f, 1.f);
		// Easing up off the box and settling at the top.
		SetLidAngle(Info.OpenAngle * FMath::SmoothStep(0.f, 1.f, Share));
		if (Share >= LootShare)
		{
			DropLoot();
		}
		if (Share >= 1.f)
		{
			State = EChestState::Open;
		}
	}
	if (State == EChestState::Closed || State == EChestState::Open)
	{
		SetActorTickEnabled(false);
	}
}

void AChest::RestoreOpened()
{
	const FChestKindInfo Info = GetKindInfo();
	State = EChestState::Open;
	bLootGiven = true;
	Clock = 0.f;
	SetWheelAngle(Info.WheelTurns * 360.f);
	SetLidAngle(Info.OpenAngle);
	SetActorTickEnabled(false);
}

void AChest::CloseAgain()
{
	State = EChestState::Closed;
	bLootGiven = false;
	Clock = 0.f;
	SetWheelAngle(0.f);
	SetLidAngle(0.f);
	DroppedLoot.Reset();
	SetActorTickEnabled(false);
}

void AChest::SetLidAngle(float Degrees)
{
	LidAngle = Degrees;
	// Pitched about the hinge: its front (+X) rises toward +Z and goes on over the back past upright. A quaternion, so the
	// turn past 90 degrees isn't folded into another rotator.
	if (Lid)
	{
		Lid->SetRelativeRotation(FRotator(Degrees, 0.f, 0.f).Quaternion());
	}
}

void AChest::SetWheelAngle(float Degrees)
{
	WheelAngle = Degrees;
	// About its front axis, turning the way a hand on its right spoke pulls it down (clockwise to the player in front).
	if (Wheel)
	{
		Wheel->SetRelativeRotation(FQuat(FVector::ForwardVector, FMath::DegreesToRadians(Degrees)));
	}
}


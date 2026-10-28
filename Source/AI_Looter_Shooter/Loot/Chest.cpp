// AChest: a loot chest opened once with Interact (the Ranger caches and the gang's Strongbox, step 26; the lootable
// world's coffins, graves, mailboxes and footlockers): its models by kind, being used, and staying open with the session.
// ChestKinds.cpp is each kind's numbers, ChestOpening.cpp the opening's motions, ChestLoot.cpp what it gives.

#include "Loot/Chest.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
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

	/** A tier's show pieces' sockets: the latches' and the lock's on the body, the crest's on the lid. */
	const FName LatchLeftSocket(TEXT("Latch_L"));
	const FName LatchRightSocket(TEXT("Latch_R"));
	const FName LockSocket(TEXT("Lock"));
	const FName CrestSocket(TEXT("Crest"));

	/** Whether a model's package is there (imported). */
	bool ModelExists(const TCHAR* Path)
	{
		return Path && FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(FString(Path)));
	}

	/** A model by its object path, quietly null while it isn't imported. */
	UStaticMesh* LoadModel(const TCHAR* Path)
	{
		if (!ModelExists(Path))
		{
			return nullptr;
		}
		return LoadObject<UStaticMesh>(nullptr, Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
	}

	/** Says once per kind that it wears its stand-in's models (its own aren't imported yet). */
	void LogStandInOnce(EChestKind Kind, const FChestKindInfo& Own, const FChestKindInfo& StandIn)
	{
		static uint64 Logged = 0;
		const uint64 Bit = uint64(1) << (static_cast<uint8>(Kind) & 63);
		if ((Logged & Bit) != 0)
		{
			return;
		}
		Logged |= Bit;
		UE_LOG(LogLooter, Log, TEXT("Chests: %s has no %s yet; it wears %s until its own model is imported."),
			*AChest::KindName(Kind).ToString(), Own.BodyPath, StandIn.BodyPath);
	}

	/** Mesh is some kind's own model (or none), so it follows the kind; anything else was chosen for this chest. */
	bool IsKindModel(const UStaticMesh* Mesh)
	{
		if (!Mesh)
		{
			return true;
		}
		const FString Path = Mesh->GetPathName();
		for (const EChestKind Each : FChestKindInfo::All())
		{
			const FChestKindInfo Info = FChestKindInfo::Get(Each);
			for (const TCHAR* Own : { Info.BodyPath, Info.LidPath, Info.WheelPath, Info.CoverPath, Info.ShovelPath, Info.LatchLeftPath,
				Info.LatchRightPath, Info.LockPath, Info.CrestPath })
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

	/** A part that's only looked at (a grave's mound and spade, the wheel): nothing collides with it. */
	UStaticMeshComponent* MakeLookPart(AActor* Owner, USceneComponent* Parent, const TCHAR* Name)
	{
		UStaticMeshComponent* Made = MakePart(Owner, Name);
		Made->SetupAttachment(Parent);
		Made->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		return Made;
	}
}

AChest::AChest()
{
	// Only while something on it moves.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;

	// Solid, so it's stood on and found by the Interact key's line; world dynamic, so ground traces (a creature's footing,
	// the scatter, settling props) never take it for the ground and loot, which lands on world static only, flies out of
	// it rather than catching on its walls. A kind that isn't solid (a grave) turns it off (ApplyKind).
	Body = MakePart(this, TEXT("Body"));
	Body->SetupAttachment(Root);
	Body->SetCollisionProfileName(UCollisionProfile::BlockAllDynamic_ProfileName);

	Lid = MakePart(this, TEXT("Lid"));
	Lid->SetupAttachment(Body, LidSocket);
	Lid->SetCollisionProfileName(UCollisionProfile::BlockAllDynamic_ProfileName);

	// The wheel is a hand's size on the front: nothing to stand on or shoot at (Chests.py gives it no collision).
	Wheel = MakeLookPart(this, Body, TEXT("Wheel"));
	Wheel->SetupAttachment(Body, WheelSocket);
	Wheel->SetCastShadow(false);

	// A grave's mound and spade: on the ground, not on the body (which starts underground and heaves up). Drawn as far as
	// the dressing's graves round it (build_area_dressing.py SMALL_CULL).
	Cover = MakeLookPart(this, Root, TEXT("Cover"));
	Shovel = MakeLookPart(this, Root, TEXT("Shovel"));
	Cover->LDMaxDrawDistance = 12000.f;
	Shovel->LDMaxDrawDistance = 12000.f;

	// A tier's show pieces, on their sockets (ChestTiers.py): small, nothing to stand on, and no shadow of their own worth
	// the cost (the body's covers them).
	LatchLeft = MakeLookPart(this, Body, TEXT("LatchLeft"));
	LatchLeft->SetupAttachment(Body, LatchLeftSocket);
	LatchRight = MakeLookPart(this, Body, TEXT("LatchRight"));
	LatchRight->SetupAttachment(Body, LatchRightSocket);
	Lock = MakeLookPart(this, Body, TEXT("Lock"));
	Lock->SetupAttachment(Body, LockSocket);
	Crest = MakeLookPart(this, Lid, TEXT("Crest"));
	Crest->SetupAttachment(Lid, CrestSocket);
	for (UStaticMeshComponent* Piece : { LatchLeft.Get(), LatchRight.Get(), Lock.Get() })
	{
		Piece->SetCastShadow(false);
	}

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
	if (GetKindInfo().bSolid)
	{
		Tags.AddUnique(ObstacleTag);
	}
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

bool FChestKindInfo::WearsStandIn(EChestKind Kind)
{
	const FChestKindInfo Info = Get(Kind);
	return Info.StandIn.IsSet() && !ModelExists(Info.BodyPath);
}

FChestKindInfo FChestKindInfo::GetWorn(EChestKind Kind)
{
	FChestKindInfo Info = Get(Kind);
	if (Info.StandIn.IsSet() && !ModelExists(Info.BodyPath))
	{
		// The stand-in's body, lid and wheel (its wheel only for the look: a tier's lid just swings).
		const FChestKindInfo StandIn = Get(Info.StandIn.GetValue());
		Info.BodyPath = StandIn.BodyPath;
		Info.LidPath = StandIn.LidPath;
		Info.WheelPath = StandIn.WheelPath;
		// The stand-ins have no show pieces: the show plays on with its glows and sounds only.
		Info.LatchLeftPath = StandIn.LatchLeftPath;
		Info.LatchRightPath = StandIn.LatchRightPath;
		Info.LockPath = StandIn.LockPath;
		Info.CrestPath = StandIn.CrestPath;
	}
	return Info;
}

void AChest::ApplyKind()
{
	const FChestKindInfo Info = FChestKindInfo::GetWorn(Kind);
	const FChestKindInfo Own = GetKindInfo();
	if (Own.StandIn.IsSet() && !ModelExists(Own.BodyPath))
	{
		LogStandInOnce(Kind, Own, Info);
	}
	FollowKind(Body, Info.BodyPath);
	FollowKind(Lid, Info.LidPath);
	FollowKind(Wheel, Info.WheelPath);
	FollowKind(Cover, Info.CoverPath);
	FollowKind(Shovel, Info.ShovelPath);
	// A tier's show pieces, shown where it has them (its own models imported). Their looks are made again for new models.
	if (Show.bLooksMade)
	{
		for (UStaticMeshComponent* Part : { Body.Get(), Lid.Get(), LatchLeft.Get(), LatchRight.Get(), Lock.Get(), Crest.Get() })
		{
			Part->EmptyOverrideMaterials();
		}
		Show.Looks.Reset();
		Show.bLooksMade = false;
	}
	auto FollowPiece = [](UStaticMeshComponent* Piece, const TCHAR* Path)
	{
		FollowKind(Piece, Path);
		Piece->SetVisibility(Piece->GetStaticMesh() != nullptr);
	};
	FollowPiece(LatchLeft, Info.LatchLeftPath);
	FollowPiece(LatchRight, Info.LatchRightPath);
	FollowPiece(Lock, Info.LockPath);
	FollowPiece(Crest, Info.CrestPath);
	// On the model's sockets; without the model, where they would be on it.
	LidRest = Body->DoesSocketExist(LidSocket) ? FVector::ZeroVector : Info.Hinge;
	Wheel->SetRelativeLocation(Body->DoesSocketExist(WheelSocket) ? FVector::ZeroVector : Info.WheelHub);
	// The wheel shows where the kind has one (the Strongbox's, and a tier wearing the Strongbox's model, which never turns it).
	Wheel->SetVisibility(Info.WheelPath != nullptr);
	// A grave's heaps and its coffin's lid are nothing to stand on or snag on; a shoved lid never sweeps a player aside.
	const bool bLidSolid = Info.bSolid && Info.LidMotion == EChestLidMotion::Hinge;
	Body->SetCollisionProfileName(Info.bSolid ? UCollisionProfile::BlockAllDynamic_ProfileName : UCollisionProfile::NoCollision_ProfileName);
	Lid->SetCollisionProfileName(bLidSolid ? UCollisionProfile::BlockAllDynamic_ProfileName : UCollisionProfile::NoCollision_ProfileName);
	if (!Info.bSolid)
	{
		Tags.Remove(ObstacleTag);
	}
	// A socket that appeared with the model moves its part even when the relative place didn't change.
	if (State == EChestState::Open)
	{
		PoseOpen();
	}
	else
	{
		PoseClosed();
	}
	Lid->UpdateComponentToWorld();
	Wheel->UpdateComponentToWorld();
}

FName AChest::GetSaveKey() const
{
	return ChestId.IsNone() ? GetFName() : ChestId;
}

float AChest::GetPreSeconds() const
{
	const FChestKindInfo Info = GetKindInfo();
	switch (Info.PreMotion)
	{
	case EChestPreMotion::Wheel: return FMath::Max(WheelSeconds, KINDA_SMALL_NUMBER);
	case EChestPreMotion::Pry:
	case EChestPreMotion::Dig:
	case EChestPreMotion::Show: return FMath::Max(Info.PreSeconds, KINDA_SMALL_NUMBER);
	default: return 0.f;
	}
}

float AChest::GetLidSecondsFor() const
{
	const float KindSeconds = GetKindInfo().LidSeconds;
	return FMath::Max(KindSeconds > 0.f ? KindSeconds : LidSeconds, KINDA_SMALL_NUMBER);
}

// ---------------------------------------------------------------------------
// Being used
// ---------------------------------------------------------------------------

FInteractionOptions AChest::GetInteractionOptions(const UInteractionComponent& User) const
{
	FInteractionOptions Options;
	// Once: open, it's never focused again, and the loot it threw out takes the key. A coffin or a grave takes a hold
	// (prying and digging are work); everything else a tap.
	const float Hold = GetHoldSeconds();
	const FText Words = Prompt.IsEmpty() ? DefaultPrompt(Kind) : Prompt;
	Options.bUsable = CanOpen();
	Options.bTap = Hold <= 0.f;
	Options.bHold = Hold > 0.f;
	Options.HoldSeconds = Hold;
	if (Options.bHold)
	{
		Options.HoldPrompt = Words;
	}
	else
	{
		Options.TapPrompt = Words;
	}
	Options.Reach = Reach;
	return Options;
}

bool AChest::Interact(UInteractionComponent& User, bool bHeld)
{
	return Open(User.GetOwner());
}

TOptional<FVector> AChest::GetInteractionLocation() const
{
	// A kind's own point (a grave's, over its mound: its body starts underground); else the model's SOCKET_Interact on its
	// front (where the hasp is), so focus is judged toward the side a player opens it from; else the middle of the chest;
	// without its model, halfway up where it would be.
	const FChestKindInfo Info = GetKindInfo();
	if (Info.InteractPoint.IsSet())
	{
		return GetActorTransform().TransformPosition(Info.InteractPoint.GetValue());
	}
	if (Body && Body->GetStaticMesh())
	{
		static const FName InteractSocket(TEXT("Interact"));
		return Body->DoesSocketExist(InteractSocket) ? Body->GetSocketLocation(InteractSocket) : Body->Bounds.Origin;
	}
	return GetActorTransform().TransformPosition(FVector(0.0, 0.0, Info.Height * 0.5));
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
	// What comes first (the Strongbox's wheel, a pry, a dig), then the lid; each is heard as it starts.
	const FChestKindInfo Info = GetKindInfo();
	State = Info.PreMotion != EChestPreMotion::None && GetPreSeconds() > 0.f ? EChestState::Unlocking : EChestState::Opening;
	Clock = 0.f;
	DirtThrown = 0;
	SetActorTickEnabled(true);
	// A tier's show starts now: its looks made, its first moments (the first latch, the lock's spin, the orb, the crest) heard.
	Show.LastPreShare = -1.f;
	Show.MomentsPlayed = 0;
	Show.Fountain.Reset();
	Show.FountainThrown = 0;
	Show.FountainClock = 0.f;
	if (Info.PreMotion == EChestPreMotion::Show && State == EChestState::Unlocking)
	{
		MakeShowLooks();
		SetPreShare(0.f);
	}
	else if (State == EChestState::Unlocking)
	{
		const FVector Where = Info.PreMotion == EChestPreMotion::Wheel && Wheel ? Wheel->GetComponentLocation()
			: GetActorTransform().TransformPosition(FVector(0.0, 0.0, Info.Height * 0.5));
		if (Info.PreCue)
		{
			LooterSound::PlayAt(this, Info.PreCue, Where);
		}
	}
	else if (Info.OpenCue)
	{
		// A better chest's lid is heard louder and deeper (its tier's OpenVolume, OpenPitch).
		LooterSound::PlayAt(this, Info.OpenCue, Lid ? Lid->GetComponentLocation() : GetActorLocation(), Info.OpenVolume, Info.OpenPitch);
	}
	UE_LOG(LogLooter, Log, TEXT("%s (%s): opened%s."), *GetActorNameOrLabel(), *GetSaveKey().ToString(),
		ByWhom ? *FString::Printf(TEXT(" by %s"), *ByWhom->GetName()) : TEXT(""));
	return true;
}

void AChest::RestoreOpened()
{
	State = EChestState::Open;
	bLootGiven = true;
	Clock = 0.f;
	SetActorTickEnabled(false);
	// Snapped to its show's end: the latches out, the lock green, the crest unfolded, nothing heard or flashed.
	Show.Fountain.Reset();
	Show.FountainThrown = 0;
	MakeShowLooks();
	PoseOpen();
}

void AChest::CloseAgain()
{
	State = EChestState::Closed;
	bLootGiven = false;
	Clock = 0.f;
	DirtThrown = 0;
	DroppedLoot.Reset();
	Show.Fountain.Reset();
	Show.FountainThrown = 0;
	Show.LastPreShare = -1.f;
	Show.MomentsPlayed = 0;
	SetActorTickEnabled(false);
	PoseClosed();
}

// ASoulFollowers: the souls freed from the barge's coal, drifting after Abel's lantern, waiting at the Keeper's Gate, and
// carried off west on the Gravewind as Saint Ada wakes.

#include "World/SoulFollowers.h"
#include "AI_Looter_Shooter.h"
#include "Missions/MissionRunner.h"
#include "Session/CampaignRecord.h"
#include "World/CarriedLantern.h"
#include "World/CarriedLanternRules.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInterface.h"
#include "Math/RotationMatrix.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	/** The game's additive glow on the engine's quad (R, G, B, strength, shape 1 = round), as Saint Ada's ember is drawn. */
	const TCHAR* GlowMaterialPath = TEXT("/Game/Weapons/FX/M_FX_Glow.M_FX_Glow");
	const TCHAR* QuadPath = TEXT("/Engine/BasicShapes/Plane.Plane");
	constexpr int32 GlowFloats = 5;

	/** A soul's light: pale soul-cyan warming to gold as it nears the lantern (the keeper's light takes it in). */
	const FLinearColor SoulCyan(0.55f, 0.92f, 1.f);
	const FLinearColor SoulGold(1.f, 0.86f, 0.5f);
	/** Its size (cm across) and strength, and the gentle flicker over them. */
	constexpr float SoulSize = 22.f;
	constexpr float SoulStrength = 9.f;
	/** Rising out of its lump: up this far, over this long, then off to its place. */
	constexpr float RiseUp = 110.f;
	constexpr float RiseSeconds = 1.2f;
	/** The spring toward its place (stiffness, a second): loose, so the trail drifts and lags round corners. */
	constexpr float Stiffness = 9.f;
	constexpr float Damping = 5.5f;
	/** A soul farther than this from its place (a fast travel, a fall) is put there. */
	constexpr float SnapDistance = 4000.f;
	/** The story's wait read this often (s). */
	constexpr float WaitCheckEvery = 0.3f;
	/** Released extras beyond the followers: at most this many (the six dead and a few more). */
	constexpr int32 MaxExtras = 12;
}

ASoulFollowers::ASoulFollowers()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	SetCanBeDamaged(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Quad(QuadPath);
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Glow(GlowMaterialPath);
	Glows = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Glows"));
	RootComponent = Glows;
	Glows->SetMobility(EComponentMobility::Movable);
	Glows->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Glows->SetCastShadow(false);
	Glows->bReceivesDecals = false;
	Glows->SetStaticMesh(Quad.Object);
	if (Glow.Object)
	{
		Glows->SetMaterial(0, Glow.Object);
	}
	Glows->SetNumCustomDataFloats(GlowFloats);

	// Waiting at the gate while Abel's lantern hangs on the keeper's post and his fight and scene play (Main 7 steps 1-3).
	WaitWhen.DuringMission = GivenToTheWind::Mission();
	WaitWhen.FromStep = 1;
	WaitWhen.BeforeStep = GivenToTheWind::CarryStep;
	WaitTag = GivenToTheWind::WaitPlaceTag();
}

void ASoulFollowers::BeginPlay()
{
	Super::BeginPlay();
	Glows->ClearInstances();
	Glows->SetNumCustomDataFloats(GlowFloats);
}

int32 ASoulFollowers::NumFollowing() const
{
	int32 Count = 0;
	for (const FSoul& Soul : Souls)
	{
		Count += Soul.Mode != ESoulFollow::Released ? 1 : 0;
	}
	return Count;
}

int32 ASoulFollowers::AddSoul(const FVector& From, bool bAtOnce)
{
	if (bReleased || NumFollowing() >= CarriedLanternRules::MaxFollowers)
	{
		return NumFollowing();
	}
	FSoul& Soul = Souls.AddDefaulted_GetRef();
	Soul.From = From;
	Soul.At = From;
	Soul.Slot = NumFollowing() - 1;
	Soul.Mode = bAtOnce ? ESoulFollow::Following : ESoulFollow::Rising;
	Soul.Glow = bAtOnce ? 1.f : 0.f;
	Soul.Clock = FMath::FRandRange(0.f, 10.f);
	// A loaded trail is put straight in its place (behind the lantern, or by the gate), not flown in from its lump.
	Soul.bSnap = bAtOnce;
	SetActorTickEnabled(true);
	UE_LOG(LogLooter, Verbose, TEXT("A soul from the coal follows Abel's lantern (%d now)."), NumFollowing());
	return NumFollowing();
}

void ASoulFollowers::Release(const TArray<FVector>& Extra)
{
	if (bReleased)
	{
		return;
	}
	bReleased = true;
	for (FSoul& Soul : Souls)
	{
		Soul.Mode = ESoulFollow::Released;
		Soul.From = Soul.At;
		Soul.Clock = FMath::FRandRange(0.f, 0.8f);
	}
	int32 Added = 0;
	for (const FVector& Point : Extra)
	{
		if (Added++ >= MaxExtras)
		{
			break;
		}
		FSoul& Soul = Souls.AddDefaulted_GetRef();
		Soul.Mode = ESoulFollow::Released;
		Soul.From = Point;
		Soul.At = Point;
		Soul.Slot = Souls.Num() - 1;
		// Each body lets go a beat after the one before: a line of lights lifting off the boards.
		Soul.Clock = -0.35f * static_cast<float>(Added);
	}
	SetActorTickEnabled(!Souls.IsEmpty());
	UE_LOG(LogLooter, Log, TEXT("The Gravewind takes the dead west: %d lights."), Souls.Num());
}

bool ASoulFollowers::ShouldWait() const
{
	return bWaitCached;
}

FVector ASoulFollowers::WaitSpot()
{
	if (!bWaitFound)
	{
		bWaitFound = true;
		for (TActorIterator<AActor> It(GetWorld()); It; ++It)
		{
			if (It->ActorHasTag(WaitTag))
			{
				FoundWait = It->GetActorLocation();
				break;
			}
		}
	}
	return FoundWait;
}

void ASoulFollowers::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Advance(DeltaSeconds);
}

void ASoulFollowers::Advance(float DeltaSeconds)
{
	Clock += DeltaSeconds;
	WaitCheckLeft -= DeltaSeconds;
	if (WaitCheckLeft <= 0.f)
	{
		WaitCheckLeft = WaitCheckEvery;
		const UMissionRunner* Runner = UMissionRunner::Get(this);
		bWaitCached = Runner && WaitWhen.IsMet(Runner->GetCampaign(), Runner);
	}
	const UWorld* World = GetWorld();
	AActor* Following = Leader.Get();
	if (!Following && World && World->GetFirstPlayerController())
	{
		Following = World->GetFirstPlayerController()->GetPawn();
	}
	const APawn* LeaderPawn = Cast<APawn>(Following);
	const FVector Lantern = LeaderPawn ? UCarriedLanternSubsystem::LanternPoint(LeaderPawn) : (Following ? Following->GetActorLocation() : GetActorLocation());
	const FVector Ahead = Following ? Following->GetActorForwardVector().GetSafeNormal2D() : FVector::ForwardVector;
	const FMatrix Frame = FRotationMatrix::MakeFromXZ(Ahead.IsNearlyZero() ? FVector::ForwardVector : Ahead, FVector::UpVector);
	const bool bWait = ShouldWait();
	const FVector Gate = bWait ? WaitSpot() : FVector::ZeroVector;
	const int32 Count = NumFollowing();

	for (int32 Index = Souls.Num() - 1; Index >= 0; --Index)
	{
		FSoul& Soul = Souls[Index];
		Soul.Clock += DeltaSeconds;
		if (Soul.Mode == ESoulFollow::Released)
		{
			if (Soul.Clock < 0.f)
			{
				continue;
			}
			Soul.At = CarriedLanternRules::ReleasedAt(Soul.From, Soul.Clock, Index);
			Soul.Glow = CarriedLanternRules::ReleasedGlow(Soul.Clock);
			if (Soul.Clock >= CarriedLanternRules::ReleaseSeconds)
			{
				Souls.RemoveAt(Index);
			}
			continue;
		}
		FVector Target;
		if (Soul.Mode == ESoulFollow::Rising && Soul.Clock < RiseSeconds)
		{
			// Straight up out of the cracked coal first, brightening.
			const float A = FMath::SmoothStep(0.f, RiseSeconds, Soul.Clock);
			Soul.At = Soul.From + FVector(0.0, 0.0, RiseUp * A);
			Soul.Glow = A;
			continue;
		}
		Soul.Mode = bWait && !Gate.IsNearlyZero() ? ESoulFollow::Waiting : ESoulFollow::Following;
		Target = Soul.Mode == ESoulFollow::Waiting ? Gate + CarriedLanternRules::WaitSlot(Soul.Slot, Count, Clock)
			: Lantern + Frame.TransformVector(CarriedLanternRules::FollowerSlot(Soul.Slot, Clock));
		if (Soul.bSnap || FVector::DistSquared(Soul.At, Target) > SnapDistance * SnapDistance)
		{
			Soul.bSnap = false;
			Soul.At = Target;
			Soul.Velocity = FVector::ZeroVector;
		}
		// A loose spring: they lag round turns and bunch when Sully stops, as lights drifting after a lantern would.
		const FVector Pull = (Target - Soul.At) * Stiffness - Soul.Velocity * Damping;
		Soul.Velocity += Pull * DeltaSeconds;
		Soul.At += Soul.Velocity * DeltaSeconds;
		Soul.Glow = FMath::Min(1.f, Soul.Glow + DeltaSeconds);
	}
	if (Souls.IsEmpty())
	{
		Glows->ClearInstances();
		SetActorTickEnabled(false);
		return;
	}
	Draw();
}

void ASoulFollowers::Draw()
{
	const UWorld* World = GetWorld();
	const APlayerController* Viewer = World ? World->GetFirstPlayerController() : nullptr;
	const bool bHasCamera = Viewer && Viewer->PlayerCameraManager;
	const FVector Camera = bHasCamera ? Viewer->PlayerCameraManager->GetCameraLocation() : FVector::ZeroVector;
	const bool bNear = !bHasCamera || Souls.ContainsByPredicate([&Camera](const FSoul& Soul)
	{
		return FVector::DistSquared(Soul.At, Camera) < DrawReach * DrawReach * (Soul.Mode == ESoulFollow::Released ? 9.0 : 1.0);
	});
	if (!bNear)
	{
		if (Glows->GetInstanceCount() > 0)
		{
			Glows->ClearInstances();
		}
		return;
	}
	while (Glows->GetInstanceCount() < Souls.Num())
	{
		Glows->AddInstance(FTransform::Identity, /*bWorldSpace*/ true);
	}
	while (Glows->GetInstanceCount() > Souls.Num())
	{
		Glows->RemoveInstance(Glows->GetInstanceCount() - 1);
	}
	for (int32 Index = 0; Index < Souls.Num(); ++Index)
	{
		const FSoul& Soul = Souls[Index];
		FVector ToCamera = bHasCamera ? Camera - Soul.At : FVector::UpVector;
		if (!ToCamera.Normalize())
		{
			ToCamera = FVector::UpVector;
		}
		const float Flicker = 0.85f + 0.15f * FMath::Sin(Soul.Clock * 7.3f + static_cast<float>(Index));
		// Released far off they're seen across the valley: bigger so a river of them reads at a few hundred metres.
		const float Size = SoulSize * (Soul.Mode == ESoulFollow::Released ? 2.4f : 1.f) * FMath::Max(Soul.Glow, 0.05f);
		Glows->UpdateInstanceTransform(Index, FTransform(FRotationMatrix::MakeFromZ(ToCamera).ToQuat(), Soul.At,
			FVector(Size / 100.0, Size / 100.0, 1.0)), /*bWorldSpace*/ true, /*bMarkRenderStateDirty*/ false, /*bTeleport*/ true);
		const float Near = Soul.Mode == ESoulFollow::Following ? 0.6f : Soul.Mode == ESoulFollow::Released ? 0.8f : 0.2f;
		const FLinearColor Tint = FMath::Lerp(SoulCyan, SoulGold, Near);
		const float Data[] = { Tint.R, Tint.G, Tint.B, SoulStrength * Soul.Glow * Flicker, 1.f };
		Glows->SetCustomData(Index, MakeArrayView(Data), /*bMarkRenderStateDirty*/ false);
	}
	Glows->MarkRenderStateDirty();
}

// ASoulCoalLump: a lump of the barge's fallen soul coal with a soul in it; cracked, the soul follows Abel's lantern.

#include "World/SoulCoal.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Audio/LooterSoundCues.h"
#include "Combat/BulletSubsystem.h"
#include "Interaction/InteractionComponent.h"
#include "Interaction/InteractionFocus.h"
#include "Interaction/InteractionSubsystem.h"
#include "Missions/MissionRunner.h"
#include "Session/CampaignRecord.h"
#include "Weapons/WeaponFX.h"
#include "World/CarriedLantern.h"
#include "World/CarriedLanternRules.h"
#include "World/LamplighterEffects.h"
#include "World/LamplighterRules.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInterface.h"
#include "Math/RotationMatrix.h"
#include "Misc/PackageName.h"
#include "UObject/ConstructorHelpers.h"

const TCHAR* const ASoulCoalLump::ModelPath = TEXT("/Game/Art/Tripo/SM_SoulCoal.SM_SoulCoal");

namespace
{
	/** The stand-in: a small faceted boulder in charcoal (the Tripo coal replaces both). */
	const TCHAR* StandInPath = TEXT("/Game/Art/Rocks/SM_Boulder_A.SM_Boulder_A");
	const TCHAR* CharcoalPath = TEXT("/Game/Art/Materials/MI_Charcoal.MI_Charcoal");
	const TCHAR* GlowMaterialPath = TEXT("/Game/Weapons/FX/M_FX_Glow.M_FX_Glow");
	const TCHAR* QuadPath = TEXT("/Engine/BasicShapes/Plane.Plane");
	constexpr int32 GlowFloats = 5;

	/** The soul's cyan in the cracks: a slow breath, and the quad's size (cm) and strength. */
	const FLinearColor VeinCyan(0.45f, 0.9f, 1.f);
	constexpr float GlowSize = 30.f;
	constexpr float GlowStrength = 6.f;
	/** Drawn, and its smoke puffed, only with the player this near (cm); a slow tick farther off. */
	constexpr double NearReach = 3500.0;
	constexpr float FarTickSeconds = 0.5f;
	/** A thread of smoke off it now and then (s). */
	constexpr float SmokeEvery = 2.6f;
	/** Cracked: the lump dulled (its colour's scale), and the burst's size. */
	constexpr float CrackBurstSize = 45.f;

	template <typename T>
	T* FindIfMade(const TCHAR* ObjectPath)
	{
		if (!FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(FString(ObjectPath))))
		{
			return nullptr;
		}
		ConstructorHelpers::FObjectFinder<T> Finder(ObjectPath);
		return Finder.Object;
	}

	const APawn* FirstPlayer(const UWorld* World)
	{
		const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
		return Controller ? Controller->GetPawn() : nullptr;
	}
}

ASoulCoalLump::ASoulCoalLump()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = FarTickSeconds;
	SetCanBeDamaged(false);

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	static UStaticMesh* const Tripo = FindIfMade<UStaticMesh>(ModelPath);
	static UStaticMesh* const StandIn = FindIfMade<UStaticMesh>(StandInPath);
	static UMaterialInterface* const Charcoal = FindIfMade<UMaterialInterface>(CharcoalPath);
	Lump = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Lump"));
	Lump->SetupAttachment(Root);
	Lump->SetStaticMesh(Tripo ? Tripo : StandIn);
	if (!Tripo && Charcoal)
	{
		Lump->SetMaterial(0, Charcoal);
	}
	// The Interact line finds it (Visibility); the player steps over it; shots pass (a lump isn't cover).
	Lump->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	Lump->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Lump->SetCollisionResponseToAllChannels(ECR_Ignore);
	Lump->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	Lump->SetCanEverAffectNavigation(false);
	Lump->SetCastShadow(false);

	static UStaticMesh* const Quad = FindIfMade<UStaticMesh>(QuadPath);
	static UMaterialInterface* const GlowMaterial = FindIfMade<UMaterialInterface>(GlowMaterialPath);
	Glow = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Glow"));
	Glow->SetupAttachment(Root);
	Glow->SetMobility(EComponentMobility::Movable);
	Glow->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Glow->SetCastShadow(false);
	Glow->bReceivesDecals = false;
	Glow->SetStaticMesh(Quad);
	if (GlowMaterial)
	{
		Glow->SetMaterial(0, GlowMaterial);
	}
	Glow->SetVisibility(false);

	// From Main 6's end (BargeFall rained it) until Saint Ada wakes; cracked while Main 7 runs.
	ShownWhen.AfterMissions = { FName(TEXT("Main6")) };
	ShownWhen.WithoutFlags = { GivenToTheWind::SaintAdaAwakeFlag() };
	FreeWhen.DuringMission = GivenToTheWind::Mission();
	Tags.Add(GivenToTheWind::SoulCoalTag());
}

void ASoulCoalLump::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	FitLump();
}

void ASoulCoalLump::FitLump()
{
	const UStaticMesh* Mesh = Lump ? Lump->GetStaticMesh() : nullptr;
	if (!Mesh)
	{
		return;
	}
	// Whatever the mesh's own size, Size across, sitting on the ground (its bounds' foot at the actor's origin), half buried.
	const FBox Box = Mesh->GetBoundingBox();
	const float Across = FMath::Max3(static_cast<float>(Box.GetSize().X), static_cast<float>(Box.GetSize().Y), 1.f);
	const float Scale = Size / Across;
	Lump->SetRelativeScale3D(FVector(Scale, Scale, Scale * 0.8f));
	Lump->SetRelativeLocation(FVector(0.0, 0.0, -Box.Min.Z * Scale * 0.8 - Size * 0.12));
}

void ASoulCoalLump::BeginPlay()
{
	Super::BeginPlay();
	FitLump();
	Glow->ClearInstances();
	Glow->SetNumCustomDataFloats(GlowFloats);
	Glow->AddInstance(FTransform(FQuat::Identity, GlowPoint(), FVector(0.01)), /*bWorldSpace*/ true);
	Clock = FMath::FRandRange(0.f, 10.f);
	SmokeWait = FMath::FRandRange(0.f, SmokeEvery);
	if (UInteractionSubsystem* Registry = UInteractionSubsystem::Get(this))
	{
		Registry->Register(this);
	}
	if (UMissionRunner* Runner = UMissionRunner::Get(this))
	{
		BoundRunner = Runner;
		MissionsChangedHandle = Runner->OnChanged.AddUObject(this, &ASoulCoalLump::HandleMissionsChanged);
	}
	RefreshStory();
}

void ASoulCoalLump::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UInteractionSubsystem* Registry = UInteractionSubsystem::Get(this))
	{
		Registry->Unregister(this);
	}
	if (UMissionRunner* Runner = BoundRunner.Get())
	{
		Runner->OnChanged.Remove(MissionsChangedHandle);
	}
	Super::EndPlay(EndPlayReason);
}

bool ASoulCoalLump::StoryHolds(const FStoryCondition& Condition) const
{
	if (Condition.IsEmpty())
	{
		return true;
	}
	const UMissionRunner* Runner = UMissionRunner::Get(this);
	return Runner && Condition.IsMet(Runner->GetCampaign(), Runner);
}

void ASoulCoalLump::HandleMissionsChanged()
{
	RefreshStory();
}

void ASoulCoalLump::RefreshStory()
{
	const UMissionRunner* Runner = UMissionRunner::Get(this);
	const bool bWasCracked = bCracked;
	bCracked = bCracked || (Runner && !LumpId.IsNone() && Runner->HasFlag(GivenToTheWind::SoulCoalFlag(LumpId)));
	SetShown(StoryHolds(ShownWhen));
	if (bCracked && !bWasCracked)
	{
		ApplyCrackedLook();
	}
	// Cracked before this visit (a load): its soul is in the trail already, put there at once, while Main 7 runs.
	if (bCracked && !bSoulAdded && bShown && StoryHolds(FreeWhen))
	{
		bSoulAdded = true;
		if (UCarriedLanternSubsystem* Lantern = UCarriedLanternSubsystem::Get(this))
		{
			Lantern->AddFollower(GlowPoint(), /*bAtOnce*/ true);
		}
	}
}

void ASoulCoalLump::SetShown(bool bInShown)
{
	bShown = bInShown;
	SetActorHiddenInGame(!bShown);
	SetActorEnableCollision(bShown);
	SetActorTickEnabled(bShown && !bCracked);
}

FVector ASoulCoalLump::GlowPoint() const
{
	return GetActorLocation() + FVector(0.0, 0.0, Size * 0.45);
}

bool ASoulCoalLump::CanFree() const
{
	return bShown && !bCracked && StoryHolds(FreeWhen);
}

FInteractionOptions ASoulCoalLump::GetInteractionOptions(const UInteractionComponent& User) const
{
	if (!CanFree())
	{
		return FInteractionOptions::None();
	}
	FInteractionOptions Options;
	Options.bTap = false;
	Options.bHold = true;
	Options.HoldSeconds = HoldSeconds;
	Options.HoldPrompt = NSLOCTEXT("LooterCoal", "Free", "Free the soul");
	Options.Reach = Reach;
	return Options;
}

bool ASoulCoalLump::Interact(UInteractionComponent& User, bool bHeld)
{
	return bHeld && Crack(User.GetOwner());
}

TOptional<FVector> ASoulCoalLump::GetInteractionLocation() const
{
	return TOptional<FVector>(GlowPoint());
}

TOptional<FInteractionBox> ASoulCoalLump::GetInteractionBox() const
{
	// A little bigger than the lump, so a look at it from a step away finds it.
	return InteractionFocus::BoxAround(GlowPoint() - FVector(0.0, 0.0, Size * 0.2), FVector(Size * 0.9), GetActorQuat());
}

bool ASoulCoalLump::Crack(AActor* ByWhom, bool bForce)
{
	if (bCracked || (!bForce && !CanFree()))
	{
		return false;
	}
	bCracked = true;
	bSoulAdded = true;
	const FVector At = GlowPoint();
	UMissionRunner* Runner = UMissionRunner::Get(this);
	if (Runner && !LumpId.IsNone())
	{
		Runner->SetFlag(GivenToTheWind::SoulCoalFlag(LumpId));
	}
	// The crack: the coal splitting (stone's sharp knock), the soul's chime as it comes free, cyan tatters and white sparks.
	LooterSound::PlayAt(this, LooterSoundCue::Impact::Stone, At, 1.f, 1.15f);
	LooterSound::PlayAt(this, LooterSoundCue::Element::SoulLightApply, At, 0.9f, 1.1f);
	if (UBulletSubsystem* Bullets = GetWorld() ? GetWorld()->GetSubsystem<UBulletSubsystem>() : nullptr)
	{
		FWeaponFX& Effects = Bullets->GetEffects();
		Effects.Initialize(GetWorld());
		Effects.SpawnSoulTatters(At, FVector::UpVector, VeinCyan, CrackBurstSize, 6);
		Effects.SpawnStarburst(At, CrackBurstSize, VeinCyan, 8.f, 0.35f, 6);
	}
	LamplighterEffects::Sparks(this, At, FVector::UpVector, 8, 0.6f);
	ApplyCrackedLook();
	if (UCarriedLanternSubsystem* Lantern = UCarriedLanternSubsystem::Get(this))
	{
		Lantern->AddFollower(At);
	}
	// Hob's word, the first ever: what they are, and why they follow.
	if (Runner && !Runner->HasFlag(GivenToTheWind::CoalWordFlag()))
	{
		Runner->SetFlag(GivenToTheWind::CoalWordFlag());
		LandfallRules::Say(this, GivenToTheWind::CoalLines());
	}
	UE_LOG(LogLooter, Log, TEXT("%s: the soul coal cracks (%s); its soul follows Abel's lantern."), *GetActorNameOrLabel(), *LumpId.ToString());
	return true;
}

void ASoulCoalLump::ApplyCrackedLook()
{
	// The soul's gone out of it: no glow, the coal dull and split (a little lower, flatter: what's left of a lump).
	Glow->SetVisibility(false);
	// The Tripo coal's vein slot is an emissive surface: it goes dull charcoal too (the stand-in has no such slot).
	const int32 Vein = Lump->GetMaterialIndex(TEXT("SoulCoalVein"));
	if (Vein != INDEX_NONE)
	{
		if (UMaterialInterface* const Dull = LoadObject<UMaterialInterface>(nullptr, CharcoalPath))
		{
			Lump->SetMaterial(Vein, Dull);
		}
	}
	Lump->SetRelativeScale3D(Lump->GetRelativeScale3D() * FVector(1.1, 1.1, 0.6));
	SetActorTickEnabled(false);
}

void ASoulCoalLump::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Advance(DeltaSeconds);
}

void ASoulCoalLump::Advance(float DeltaSeconds)
{
	Clock += DeltaSeconds;
	const APawn* Player = FirstPlayer(GetWorld());
	const bool bNear = !bCracked && bShown && Player && FVector::DistSquared(Player->GetActorLocation(), GetActorLocation()) <= NearReach * NearReach;
	SetActorTickInterval(bNear ? 0.f : FarTickSeconds);
	Glow->SetVisibility(bNear);
	if (!bNear)
	{
		return;
	}
	// The soul breathing in the cracks: slow, uneven, the lump's own pulse, so a field of them doesn't throb in step.
	const float Breath = 0.6f + 0.25f * FMath::Sin(Clock * 1.4f) + 0.15f * FMath::Sin(Clock * 3.7f + 1.f);
	const APlayerController* Viewer = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	const FVector At = GlowPoint();
	FVector ToCamera = Viewer && Viewer->PlayerCameraManager ? Viewer->PlayerCameraManager->GetCameraLocation() - At : FVector::UpVector;
	if (!ToCamera.Normalize())
	{
		ToCamera = FVector::UpVector;
	}
	if (Glow->GetInstanceCount() > 0)
	{
		Glow->UpdateInstanceTransform(0, FTransform(FRotationMatrix::MakeFromZ(ToCamera).ToQuat(), At + ToCamera * 8.0,
			FVector(GlowSize / 100.0 * Breath, GlowSize / 100.0 * Breath, 1.0)), /*bWorldSpace*/ true, false, true);
		const float Data[] = { VeinCyan.R, VeinCyan.G, VeinCyan.B, GlowStrength * Breath, 1.f };
		Glow->SetCustomData(0, MakeArrayView(Data), /*bMarkRenderStateDirty*/ false);
		Glow->MarkRenderStateDirty();
	}
	SmokeWait -= DeltaSeconds;
	if (SmokeWait <= 0.f)
	{
		SmokeWait = SmokeEvery * FMath::FRandRange(0.7f, 1.3f);
		LamplighterEffects::SmokePuff(this, At, FVector(FMath::FRandRange(-10.f, 10.f), FMath::FRandRange(-10.f, 10.f), 45.f));
	}
}

// AAbelKeeper: construction, his props and light, his place in the story (in the world during Main 6 only), his frame and
// his coal. AbelKeeperFight.cpp has his fight's moments, AbelKeeperMoves.cpp each moment frame by frame and his ending,
// AbelKeeperWind.cpp the Gravewind, AbelKeeperPose.cpp his pose table laid over the Unpaid's body.

#include "Bosses/AbelKeeper.h"
#include "AI_Looter_Shooter.h"
#include "Bosses/BossComponent.h"
#include "Combat/HealthComponent.h"
#include "Missions/MissionRunner.h"
#include "Story/AbelOnBoard.h"
#include "World/KeeperLanternPost.h"
#include "AnimationRuntime.h"
#include "Algo/Sort.h"
#include "Components/CapsuleComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Materials/MaterialInterface.h"
#include "Misc/PackageName.h"
#include "UObject/ConstructorHelpers.h"

const FName AAbelKeeper::BossTag(TEXT("Boss_Abel"));
const FName AAbelKeeper::BossId(TEXT("Abel"));
const FName AAbelKeeper::Mission(TEXT("Main6"));
const TCHAR* const AAbelKeeper::ModelPath = TEXT("/Game/Art/Creatures/SK_Abel.SK_Abel");
const TCHAR* const AAbelKeeper::HatPath = TEXT("/Game/Art/Creatures/SM_AbelHat.SM_AbelHat");
const TCHAR* const AAbelKeeper::LanternPath = TEXT("/Game/Art/Creatures/SM_AbelLantern.SM_AbelLantern");
const TCHAR* const AAbelKeeper::PumpPath = TEXT("/Game/Art/Creatures/SM_AbelPump.SM_AbelPump");

namespace
{
	/** An Unpaid's health and bite at level 1 (AUnpaidCreature), which his are counted against. */
	constexpr float UnpaidHealth = 160.f;

	/** His props' bones (Abel.py), and the lantern's light socket. */
	const FName LanternBone(TEXT("lantern"));
	const FName GunBone(TEXT("gun"));
	const FName LightSocket(TEXT("Light"));

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

	/** A bone's rest turn taken back off, so a prop modeled where he holds it at rest sits as modeled. False without the bone. */
	bool RestTurnOff(const USkeletalMesh* Model, FName Bone, FQuat& OutTurn)
	{
		const int32 Index = Model && !Bone.IsNone() ? Model->GetRefSkeleton().FindBoneIndex(Bone) : INDEX_NONE;
		if (Index == INDEX_NONE)
		{
			return false;
		}
		OutTurn = FAnimationRuntime::GetComponentSpaceTransformRefPose(Model->GetRefSkeleton(), Index).GetRotation().Inverse();
		return true;
	}

	UStaticMeshComponent* MakeProp(AActor* Owner, const TCHAR* Name, USceneComponent* Body, const USkeletalMesh* Model, FName Bone,
		UStaticMesh* Mesh)
	{
		UStaticMeshComponent* Part = Owner->CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Part->SetupAttachment(Body, Bone);
		FQuat Turn;
		if (RestTurnOff(Model, Bone, Turn))
		{
			Part->SetRelativeRotation(Turn);
		}
		Part->SetStaticMesh(Mesh);
		// A shot meets the arm and the hand round it (his hit zones: the lantern's own hull guards his coal); the props
		// have no collision of their own, and a ghost casts no shadow.
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->SetGenerateOverlapEvents(false);
		Part->SetCanEverAffectNavigation(false);
		Part->SetCastShadow(false);
		return Part;
	}
}

AAbelKeeper::AAbelKeeper()
{
	DisplayName = NSLOCTEXT("LooterAbel", "AbelDisplay", "Abel Ransom");
	StartingRank = ECreatureRank::Boss;
	BodyScale = AbelRules::BodyScale;
	// 40 Unpaid of his level; his rank (Boss) adds no health, so it's all here.
	Health->MaxHealth = UnpaidHealth * AbelRules::HealthScale;
	AttackDamage = 12.f;
	// The bar at the top of the screen names him; no tag over him.
	bShowsHealthTag = false;
	bRespawns = false;
	// He never sinks away: he kneels, and the scene sits him on his board.
	CorpseTime = 1.0e5f;
	// He walks the boards slowly, and comes at the player a little slower than an Unpaid.
	WalkSpeed = 110.f;
	ChaseSpeed = 430.f;
	WanderRadius = 450.f;
	AttackRange = 330.f;
	LungeSpeed = 1300.f;
	LungeDistance = 300.f;
	// The deck's open end is the fight's: his own steps never take him off it (his drift and his walk are scripted).
	GetCharacterMovement()->bCanWalkOffLedges = false;

	// His own clothes, always; his coal's crit spot is his small coal (6.2 cm on the bone): the Unpaid's 12 is wider than his
	// forearm, and the lantern arm couldn't guard it.
	Clothes = 0;
	OtherClothes.Reset();
	BodySlot = TEXT("GhostAbel");
	CoalCritRadius = 6.f;
	CoalPointWithoutRig = FVector(12.1f, -5.9f, 133.f);

	static USkeletalMesh* const Model = FindIfMade<USkeletalMesh>(ModelPath);
	static UStaticMesh* const HatModel = FindIfMade<UStaticMesh>(HatPath);
	static UStaticMesh* const LanternModel = FindIfMade<UStaticMesh>(LanternPath);
	static UStaticMesh* const PumpModel = FindIfMade<UStaticMesh>(PumpPath);
	USkeletalMeshComponent* Figure = GetMesh();
	Figure->SetSkeletalMeshAsset(Model);
	FQuat HatTurn;
	if (RestTurnOff(Model, Rig.Hat, HatTurn))
	{
		GetHat()->SetRelativeRotation(HatTurn);
	}
	GetHat()->SetStaticMesh(HatModel);

	Lantern = MakeProp(this, TEXT("Lantern"), Figure, Model, LanternBone, LanternModel);
	Pump = MakeProp(this, TEXT("Pump"), Figure, Model, GunBone, PumpModel);
	// They wear his look as his hat does: his coal's gold, his flare, his dissolve.
	LookParts.Reset();
	LookParts.Add(Lantern);
	LookParts.Add(Pump);

	LanternLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("LanternLight"));
	LanternLight->SetupAttachment(Lantern, LightSocket);
	// His one light: shadowless, pale (Abel.py's flame), small.
	LanternLight->SetCastShadows(false);
	LanternLight->SetIntensityUnits(ELightUnits::Candelas);
	LanternLight->SetIntensity(LanternCandela);
	LanternLight->SetAttenuationRadius(LanternRadius);
	LanternLight->SetLightColor(FLinearColor::FromSRGBColor(FColor(0xFF, 0xF0, 0xD6)));
	// It lights the deck round him, never him: so close to his chest it blew his dark coat out white. His body and props
	// keep channel 0 (the sun and sky) and never take the ghost light's.
	AbelRules::ShineOnGhostChannel(*LanternLight);

	// The Gravewind's wisps: in the world's own frame wherever he goes, seen only while it blows.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	Wisps = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Wisps"));
	Wisps->SetupAttachment(GetCapsuleComponent());
	Wisps->SetUsingAbsoluteLocation(true);
	Wisps->SetUsingAbsoluteRotation(true);
	Wisps->SetUsingAbsoluteScale(true);
	Wisps->SetMobility(EComponentMobility::Movable);
	Wisps->SetStaticMesh(Cube.Object);
	Wisps->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Wisps->SetCastShadow(false);
	Wisps->SetCanEverAffectNavigation(false);
	Wisps->bReceivesDecals = false;
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Surface(TEXT("/Game/Environment/Materials/M_StylizedSurface.M_StylizedSurface"));
	WispBase = Surface.Object;

	// The fight: his phases, and nothing starts it but a player on the deck once the lantern hangs (FightWhen); his wall is
	// the Keeper's Gate's, which the build script points his boss at.
	Boss = CreateDefaultSubobject<UBossComponent>(TEXT("Boss"));
	Boss->BossId = BossId;
	Boss->BossName = NSLOCTEXT("LooterAbel", "AbelBar", "Abel Ransom, the Keeper");
	Boss->Phases = AbelRules::MakePhases();
	Boss->EngageRadius = 0.f;
	Boss->bStartWhenHurt = false;
	Boss->MaxAliveAdds = 10;
	Boss->SealRadius = 0.f;
	Buckshot = AbelRules::Buckshot();

	// Main 6: there all through it; his fight once the lantern hangs; the ending at its last step.
	PresentWhen.DuringMission = Mission;
	FightWhen.DuringMission = Mission;
	FightWhen.FromStep = FightStep;
	EndingWhen.DuringMission = Mission;
	EndingWhen.FromStep = SceneStep;
	Tags.Add(BossTag);
}

void AAbelKeeper::BeginPlay()
{
	LookParts.Reset();
	LookParts.Add(Lantern);
	LookParts.Add(Pump);
	// His rank, level, size and rig first (the Unpaid's start, which lays his pose over it once).
	Super::BeginPlay();
	WearProps();
	// Waiting for his fight he can't be hurt, and shows no numbers to say so.
	Health->bInvulnerable = true;
	Health->bShowDamageNumbers = false;
	LanternLight->SetIntensity(LanternCandela);
	LanternLight->SetAttenuationRadius(LanternRadius);

	Boss->OnFightStarted.AddUObject(this, &AAbelKeeper::HandleFightStarted);
	Boss->OnFightReset.AddUObject(this, &AAbelKeeper::HandleFightReset);
	Boss->OnFightWon.AddUObject(this, &AAbelKeeper::HandleFightWon);
	Boss->OnPhaseChanged.AddUObject(this, &AAbelKeeper::HandlePhaseChanged);
	Boss->OnCustomEvent.AddUObject(this, &AAbelKeeper::HandleCustomEvent);

	if (!bPaceCaptured)
	{
		bPaceCaptured = true;
		OwnCooldown = AttackCooldown;
		OwnWindup = AttackWindup;
		OwnChase = ChaseSpeed;
		OwnLunge = LungeSpeed;
	}
	FindLanternPosts();
	if (!OnBoard)
	{
		for (TActorIterator<AAbelOnBoard> It(GetWorld()); It; ++It)
		{
			OnBoard = *It;
			break;
		}
	}
	if (UMissionRunner* Runner = UMissionRunner::Get(this))
	{
		BoundRunner = Runner;
		MissionsChangedHandle = Runner->OnChanged.AddUObject(this, &AAbelKeeper::HandleMissionsChanged);
	}
	RefreshStory();
}

void AAbelKeeper::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UMissionRunner* Runner = BoundRunner.Get())
	{
		Runner->OnChanged.Remove(MissionsChangedHandle);
	}
	BoundRunner.Reset();
	for (AKeeperLanternPost* Post : LanternPosts)
	{
		if (Post)
		{
			Post->OnRelit.RemoveAll(this);
		}
	}
	StopWind();
	Super::EndPlay(EndPlayReason);
}

void AAbelKeeper::WearProps()
{
	// As the Unpaid wears its hat: at the bone, turned back by its rest turn, so each sits as it was modeled.
	const USkeletalMesh* Model = GetMesh()->GetSkeletalMeshAsset();
	for (const TPair<UStaticMeshComponent*, FName>& Prop : { TPair<UStaticMeshComponent*, FName>(Lantern, LanternBone),
		TPair<UStaticMeshComponent*, FName>(Pump, GunBone) })
	{
		FQuat Turn;
		if (!RestTurnOff(Model, Prop.Value, Turn) || !Prop.Key->GetStaticMesh())
		{
			Prop.Key->SetVisibility(false);
			continue;
		}
		Prop.Key->AttachToComponent(GetMesh(), FAttachmentTransformRules::KeepRelativeTransform, Prop.Value);
		Prop.Key->SetRelativeLocationAndRotation(FVector::ZeroVector, Turn);
		Prop.Key->SetVisibility(true);
	}
}

void AAbelKeeper::FindLanternPosts()
{
	if (LanternPosts.IsEmpty())
	{
		// The deck's posts nearest his spot.
		const FVector Here = GetHome().GetLocation();
		TArray<AKeeperLanternPost*> Found;
		for (TActorIterator<AKeeperLanternPost> It(GetWorld()); It; ++It)
		{
			if (It->ActorHasTag(AKeeperLanternPost::DeckTag))
			{
				Found.Add(*It);
			}
		}
		Algo::SortBy(Found, [Here](const AKeeperLanternPost* Post) { return FVector::DistSquared(Post->GetActorLocation(), Here); });
		for (int32 Index = 0; Index < Found.Num() && Index < AbelRules::Lanterns; ++Index)
		{
			LanternPosts.Add(Found[Index]);
		}
	}
	for (AKeeperLanternPost* Post : LanternPosts)
	{
		if (Post)
		{
			Post->OnRelit.RemoveAll(this);
			Post->OnRelit.AddUObject(this, &AAbelKeeper::HandleLanternRelit);
		}
	}
}

AKeeperLanternPost* AAbelKeeper::GetKeepersPost() const
{
	for (AKeeperLanternPost* Post : LanternPosts)
	{
		if (Post && Post->bKeepersPost)
		{
			return Post;
		}
	}
	return nullptr;
}

// ---------------------------------------------------------------------------
// In the story
// ---------------------------------------------------------------------------

bool AAbelKeeper::StoryHolds(const FStoryCondition& Condition) const
{
	if (Condition.IsEmpty())
	{
		return true;
	}
	const UMissionRunner* Runner = UMissionRunner::Get(this);
	return Runner && Condition.IsMet(Runner->GetCampaign(), Runner);
}

void AAbelKeeper::RefreshStory()
{
	if (bEndingDone)
	{
		SetPresent(false);
		return;
	}
	// Knelt at zero, the scene has him until it sits him on his board, whatever the missions say meanwhile.
	if (IsDead())
	{
		SetPresent(true);
		return;
	}
	// A session loaded at the ending's step: the scene played before; the world as it left it.
	if (!bForcedPresent && !EndingWhen.IsEmpty() && StoryHolds(EndingWhen))
	{
		FinishEndingAtOnce();
		return;
	}
	const bool bWanted = bForcedPresent || StoryHolds(PresentWhen);
	SetPresent(bWanted);
	// Once the lantern hangs, a player on the deck starts his fight (and starts it again after a death).
	Boss->EngageRadius = bWanted && StoryHolds(FightWhen) ? DeckRadius : 0.f;
}

void AAbelKeeper::ForcePresent(bool bForce)
{
	bForcedPresent = bForce;
	if (bForce)
	{
		bEndingDone = false;
	}
	RefreshStory();
}

void AAbelKeeper::HandleMissionsChanged()
{
	RefreshStory();
}

void AAbelKeeper::SetPresent(bool bInPresent)
{
	const bool bChanged = bPresent != bInPresent;
	bPresent = bInPresent;
	// Out of the story he's out of the world: unseen, nothing to meet, nothing ticking, his light out.
	SetActorHiddenInGame(!bPresent);
	SetActorEnableCollision(bPresent);
	SetActorTickEnabled(bPresent);
	GetMesh()->SetComponentTickEnabled(bPresent);
	Boss->SetComponentTickEnabled(bPresent && !bEndingDone);
	LanternLight->SetVisibility(bPresent);
	if (!bPresent)
	{
		StopWind();
		ClearWisps();
	}
	if (bChanged)
	{
		UE_LOG(LogLooter, Log, TEXT("%s: %s."), *GetActorNameOrLabel(), bPresent ? TEXT("walks the boards") : TEXT("isn't on the boards"));
	}
}

// ---------------------------------------------------------------------------
// His frame, his coal, his death's look
// ---------------------------------------------------------------------------

void AAbelKeeper::Tick(float DeltaSeconds)
{
	// The brain and the Unpaid's body, his pose laid over it (LayerPose).
	Super::Tick(DeltaSeconds);
	if (!bPresent)
	{
		return;
	}
	RiseNewAdds();
	TickMove(DeltaSeconds);
	if (bWindBlowing)
	{
		TickWind(DeltaSeconds);
	}
}

bool AAbelKeeper::IsCriticalSpot(const FHitResult& Hit) const
{
	// "His lantern arm covers it while he fights; it is open only when he grieves" (and while a pull stuns him).
	return IsCoalOpen() && Super::IsCriticalSpot(Hit);
}

void AAbelKeeper::UpdateDeathLook(float DeltaSeconds)
{
	// He doesn't fade: he kneels, and his coal sinks to an ember.
	PhaseAmount = 0.f;
	Heat = FMath::FInterpTo(Heat, EmberHeat, DeltaSeconds, 1.5f);
}

void AAbelKeeper::GetExtraRigBones(TArray<FName>& OutBones) const
{
	OutBones.Append(AbelPoses::ExtraBones());
}

bool AAbelKeeper::CanStartAttack() const
{
	// None of his own moments under way: they hold him.
	return Super::CanStartAttack() && Move == EAbelMove::None;
}

void AAbelKeeper::SetLanternFlare(float Share)
{
	LanternShare = FMath::Clamp(Share, -1.f, 1.f);
	const float Candela = LanternShare >= 0.f ? FMath::Lerp(LanternCandela, FlareCandela, LanternShare)
		: FMath::Lerp(LanternCandela, EmberCandela, -LanternShare);
	if (!FMath::IsNearlyEqual(LanternLight->Intensity, Candela, 0.05f))
	{
		LanternLight->SetIntensity(Candela);
	}
}

FVector AAbelKeeper::GetLanternGlobe() const
{
	return LanternLight->GetComponentLocation();
}

FVector AAbelKeeper::GetSunsetDirection() const
{
	// His spot faces the sunset: the deck's open end.
	const FVector Facing = GetHome().GetRotation().GetForwardVector().GetSafeNormal2D();
	return Facing.IsNearlyZero() ? FVector::ForwardVector : Facing;
}

FVector AAbelKeeper::GetFogSpot() const
{
	if (!FogSpot.IsNearlyZero())
	{
		return FogSpot;
	}
	return GetHome().GetLocation() + GetSunsetDirection() * FogDistance + FVector(0.0, 0.0, FogRise);
}

bool AAbelKeeper::GetBoardPlace(FVector& OutLocation, float& OutYaw) const
{
	if (!OnBoard)
	{
		return false;
	}
	// His feet on the board, his middle a scaled half height over them, facing as the board's seat faces.
	OutLocation = OnBoard->GetActorLocation() + FVector(0.0, 0.0, GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
	OutYaw = static_cast<float>(OnBoard->GetActorRotation().Yaw);
	return true;
}

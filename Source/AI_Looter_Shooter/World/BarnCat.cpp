// ABarnCat: the porch cat's senses (who it watches, what sends it under the porch) and what it does: loafing, sitting
// up, a stretch, a few steps, greeting a player close by, walking off from one who lingers, hiding and coming back.

#include "World/BarnCat.h"
#include "World/AnimalCues.h"
#include "World/AnimalPoseTables.h"
#include "World/FaunaRules.h"
#include "Audio/LooterSound.h"
#include "Components/PoseableMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/SkeletalMesh.h"
#include "Misc/PackageName.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	/** A player running faster than this (cm/s) is sprinting (between FaunaRules' walk and sprint speeds). */
	constexpr float CatSprintSpeed = 600.f;

	/** How fast it turns to face its spot's way, settled there (degrees a second). */
	constexpr float SettleTurnRate = 60.f;

	/** A player within this share of WatchRadius keeps it under the porch. */
	constexpr float HiddenWatchShare = 0.6f;

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
}

ABarnCat::ABarnCat()
{
	CullDistance = 4000.f;
	MeowCue = FName(AnimalCue::CatMeow);
	HissCue = FName(AnimalCue::CatHiss);
	static USkeletalMesh* const Model = FindIfMade<USkeletalMesh>(TEXT("/Game/Art/Creatures/SK_BarnCat.SK_BarnCat"));
	Figure = CreateDefaultSubobject<UPoseableMeshComponent>(TEXT("Figure"));
	Figure->SetupAttachment(Root);
	Figure->SetSkinnedAssetAndUpdate(Model, false);
	Figure->SetMobility(EComponentMobility::Movable);
	Figure->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	Figure->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Figure->SetGenerateOverlapEvents(false);
	Figure->SetCanEverAffectNavigation(false);
	Figure->bReceivesDecals = false;
	// A stretch and a walk reach past its loafing bounds.
	Figure->SetBoundsScale(1.5f);
	// Its pose comes from here: the component's own tick only looks after its level of detail.
	Figure->SetComponentTickInterval(0.25f);
}

void ABarnCat::BeginPlay()
{
	Super::BeginPlay();
	SetupCat();
}

void ABarnCat::SetupCat()
{
	if (bCatReady)
	{
		return;
	}
	bCatReady = true;
	Random.Initialize(Seed);
	// Without its porch from the build script (a test, a hand-placed cat): a short porch about where it stands.
	if (LoafSpot.IsZero())
	{
		LoafSpot = GetActorLocation();
		LoafYaw = static_cast<float>(GetActorRotation().Yaw);
	}
	const FVector Forward = FRotator(0.f, LoafYaw, 0.f).Vector();
	const FVector Right = FRotator(0.f, LoafYaw + 90.f, 0.f).Vector();
	if (PorchA.IsZero() || PorchB.IsZero())
	{
		PorchA = LoafSpot - Right * 100.0;
		PorchB = LoafSpot + Right * 100.0;
	}
	if (PorchExit.IsZero())
	{
		PorchExit = LoafSpot + Forward * 60.0;
	}
	if (HidePoint.IsZero())
	{
		HidePoint = PorchExit - FVector(0.0, 0.0, 50.0);
	}
	Position = LoafSpot;
	Yaw = LoafYaw;
	Doing = PosedDoing = ECatDoing::Loaf;
	IdleTimer = Random.FRandRange(15.f, 45.f);
	MeowTimer = Random.FRandRange(60.f, 180.f);
	TwitchTimer = Random.FRandRange(2.f, 8.f);
	FlickTimer = Random.FRandRange(1.f, 5.f);
	if (Figure)
	{
		Figure->SetCastShadow(bCastShadows);
		Figure->SetCachedMaxDrawDistance(CullDistance);
		Rig.Bind(Cast<USkeletalMesh>(Figure->GetSkinnedAsset()), AnimalPoseTables::Cat());
	}
	// How far the loaf_watch pose turns the head by itself: the look adds only the rest.
	const AnimalPoseTables::FCatBones& Bones = AnimalPoseTables::CatBones();
	FAnimalPoseSolve Watching;
	FAnimalPoseSolve Loafing;
	AnimalPose::Solve(AnimalPoseTables::Cat(), AnimalPoseTables::CatPose(ECatPose::LoafWatch), Watching);
	AnimalPose::Solve(AnimalPoseTables::Cat(), AnimalPoseTables::CatPose(ECatPose::Loaf), Loafing);
	WatchPoseYaw = AnimalPose::YawOf(Watching, Bones.Head) - AnimalPose::YawOf(Loafing, Bones.Head);
	PoseFigure(0.f);
}

FVector ABarnCat::GetFaunaCenter() const
{
	return LoafSpot.IsZero() ? GetActorLocation() : LoafSpot;
}

float ABarnCat::GetFaunaRadius() const
{
	const FVector Middle = GetFaunaCenter();
	float Reach = 60.f;
	for (const FVector& Point : { PorchA, PorchB, PorchExit, HidePoint })
	{
		Reach = FMath::Max(Reach, static_cast<float>(FVector::Dist(Middle, Point)) + 60.f);
	}
	return Reach;
}

bool ABarnCat::IsBusy() const
{
	return Doing == ECatDoing::Walk || Doing == ECatDoing::Slink || Doing == ECatDoing::Emerge || Doing == ECatDoing::Stretch;
}

void ABarnCat::UpdateFauna(const FFaunaTick& Tick, const FFaunaContext& Context)
{
	if (!bCatReady)
	{
		SetupCat();
	}
	const float DeltaSeconds = Tick.DeltaSeconds;
	CheckSenses(Tick, Context, DeltaSeconds);
	UpdateDoing(DeltaSeconds);
	UpdateLook(DeltaSeconds);

	// The small things: an ear's twitch, the tail tip's flick, a breath.
	Breath += DeltaSeconds;
	TwitchClock += DeltaSeconds;
	TwitchTimer -= DeltaSeconds;
	if (TwitchTimer <= 0.f)
	{
		TwitchTimer = Random.FRandRange(4.f, 10.f);
		TwitchClock = 0.f;
		bTwitchLeft = Random.FRand() < 0.5f;
	}
	FlickClock += DeltaSeconds;
	FlickTimer -= DeltaSeconds;
	if (FlickTimer <= 0.f)
	{
		FlickTimer = Random.FRandRange(3.f, 8.f);
		FlickClock = 0.f;
		FlickSide = Random.FRand() < 0.5f ? -1.f : 1.f;
	}
	// A meow now and then for someone sitting a while near it.
	MeowTimer -= DeltaSeconds;
	if (MeowTimer <= 0.f)
	{
		MeowTimer = Random.FRandRange(60.f, 180.f);
		if (bWatching && (Doing == ECatDoing::Loaf || Doing == ECatDoing::Sit || Doing == ECatDoing::Greet)
			&& FVector::Dist(WatchAt, Position) < 500.0)
		{
			PlayCue(MeowCue);
		}
	}
	if (Tick.bOnScreen && IsFaunaShown())
	{
		PoseFigure(DeltaSeconds);
	}
}

void ABarnCat::CheckSenses(const FFaunaTick& Tick, const FFaunaContext& Context, float DeltaSeconds)
{
	const bool bHeard = FaunaRules::FirstHeard(Position, Context.Noises, Tick.Since, GunfireRadius, ImpactRadius) != INDEX_NONE;
	bool bCreature = false;
	bool bSprint = false;
	bool bNearHide = false;
	bWatching = false;
	bClose = false;
	float Nearest = TNumericLimits<float>::Max();
	for (const FFaunaThreat& Threat : Context.Threats)
	{
		if (!Threat.bPlayer)
		{
			bCreature |= FaunaRules::IsThreatened(Position, Threat, CreatureRadius);
			continue;
		}
		const float Flat = static_cast<float>(FVector::Dist2D(Threat.Location, Position));
		const bool bLevel = FMath::Abs(Threat.Location.Z - Position.Z) < 250.0;
		bSprint |= bLevel && Flat < SprintRadius && Threat.Velocity.Size2D() > CatSprintSpeed;
		bClose |= bLevel && Flat < GreetRadius;
		bNearHide |= FaunaRules::IsThreatened(HidePoint, Threat, WatchRadius * HiddenWatchShare);
		if (FaunaRules::IsThreatened(Position, Threat, WatchRadius) && Flat < Nearest)
		{
			Nearest = Flat;
			bWatching = true;
			// Its eyes on their face, not their feet.
			WatchAt = Threat.Location + FVector(0.0, 0.0, 60.0);
		}
	}
	CloseTime = bClose ? CloseTime + DeltaSeconds : 0.f;
	AwayTime = bClose ? 0.f : AwayTime + DeltaSeconds;

	if (Doing == ECatDoing::Hidden)
	{
		// Out again once nothing has troubled it for a while, and nobody stands over its hole.
		CalmTime = (bHeard || bCreature || bNearHide) ? 0.f : CalmTime + DeltaSeconds;
		if (CalmTime >= ReturnAfter)
		{
			Position = HidePoint;
			Pitch = 0.f;
			if (Figure)
			{
				Figure->SetVisibility(true);
			}
			const FVector Back[] = { StepPoint(), PorchExit, LoafSpot };
			StartPath(ECatDoing::Emerge, Back, WalkSpeed);
			if (Random.FRand() < 0.3f)
			{
				PlayCue(MeowCue);
			}
		}
		return;
	}
	if (bSprint && !(Doing == ECatDoing::Slink && bRunning))
	{
		PlayCue(HissCue);
	}
	if (bHeard || bCreature || bSprint)
	{
		Spook(true);
	}
}

void ABarnCat::Spook(bool bRun)
{
	if (!bCatReady)
	{
		SetupCat();
	}
	if (Doing == ECatDoing::Hidden)
	{
		CalmTime = 0.f;
		return;
	}
	if (Doing == ECatDoing::Slink)
	{
		// Already going: a gunshot hurries it.
		if (bRun && !bRunning)
		{
			bRunning = true;
			PathSpeed = SlinkSpeed;
		}
		return;
	}
	bRunning = bRun;
	bWalkingOff = false;
	TArray<FVector, TInlineAllocator<4>> Way;
	if (IsOnDeck(Position))
	{
		Way = { PorchExit, StepPoint(), HidePoint };
	}
	else
	{
		Way = { HidePoint };
	}
	StartPath(ECatDoing::Slink, Way, bRun ? SlinkSpeed : WalkSpeed * 1.4f);
}

void ABarnCat::UpdateDoing(float DeltaSeconds)
{
	Clock += DeltaSeconds;
	switch (Doing)
	{
	case ECatDoing::Loaf:
	case ECatDoing::Sit:
		if (Position.Equals(LoafSpot, 10.0))
		{
			Yaw = FaunaRules::StepAngle(Yaw, LoafYaw, SettleTurnRate * DeltaSeconds);
		}
		if (bClose && AwayTime <= 0.f)
		{
			StartDoing(ECatDoing::Greet, 0.f);
			break;
		}
		if (Doing == ECatDoing::Sit && Clock >= Length)
		{
			StartDoing(ECatDoing::Loaf, 0.f);
			break;
		}
		IdleTimer -= bWatching ? 0.f : DeltaSeconds;
		if (Doing == ECatDoing::Loaf && IdleTimer <= 0.f)
		{
			ChooseIdle();
		}
		break;
	case ECatDoing::Stretch:
		if (Clock >= CatTimes::Stretch)
		{
			// Off a few steps once it's stretched.
			StartFewSteps();
		}
		break;
	case ECatDoing::Walk:
		if (FollowPath(DeltaSeconds))
		{
			if (bWalkingOff)
			{
				// At the far end it lies down again, keeping an eye on them.
				StartDoing(ECatDoing::Loaf, 0.f);
				CloseTime = 0.f;
			}
			else if (Random.FRand() < 0.3f)
			{
				StartDoing(ECatDoing::Sit, Random.FRandRange(6.f, 14.f));
			}
			else
			{
				StartDoing(ECatDoing::Loaf, 0.f);
			}
		}
		else if (bClose && !bWalkingOff)
		{
			StartDoing(ECatDoing::Greet, 0.f);
		}
		break;
	case ECatDoing::Greet:
		if (CloseTime >= LingerSeconds)
		{
			// Enough of them: off along the porch, or (at its end already) off under it.
			const FVector End = FarEnd(WatchAt);
			if (FVector::Dist2D(End, Position) > 60.0)
			{
				bWalkingOff = true;
				const FVector Target[] = { End };
				StartPath(ECatDoing::Walk, Target, WalkSpeed * 1.2f);
			}
			else
			{
				Spook(false);
			}
		}
		else if (AwayTime > 1.5f)
		{
			StartDoing(ECatDoing::Sit, Random.FRandRange(4.f, 10.f));
		}
		break;
	case ECatDoing::Slink:
		if (FollowPath(DeltaSeconds))
		{
			StartDoing(ECatDoing::Hidden, 0.f);
			CalmTime = 0.f;
			ReturnAfter = Random.FRandRange(ReturnSecondsMin, FMath::Max(ReturnSecondsMin, ReturnSecondsMax));
			if (Figure)
			{
				Figure->SetVisibility(false);
			}
		}
		break;
	case ECatDoing::Emerge:
		if (FollowPath(DeltaSeconds))
		{
			StartDoing(ECatDoing::Sit, Random.FRandRange(3.f, 6.f));
		}
		break;
	case ECatDoing::Hidden:
		break;
	}
}

void ABarnCat::StartDoing(ECatDoing Next, float InLength)
{
	Doing = Next;
	Clock = 0.f;
	Length = InLength;
	Pitch = 0.f;
	if (Next != ECatDoing::Walk)
	{
		bWalkingOff = Next == ECatDoing::Loaf && bWalkingOff;
	}
	if (Next == ECatDoing::Loaf)
	{
		IdleTimer = Random.FRandRange(20.f, 60.f);
	}
}

void ABarnCat::ChooseIdle()
{
	const float Roll = Random.FRand();
	if (Roll < 0.35f)
	{
		StartDoing(ECatDoing::Sit, Random.FRandRange(8.f, 20.f));
	}
	else if (Roll < 0.6f)
	{
		StartDoing(ECatDoing::Stretch, CatTimes::Stretch);
	}
	else
	{
		StartFewSteps();
	}
}

void ABarnCat::PlayCue(FName Cue) const
{
	if (!Cue.IsNone() && IsFaunaShown() && Doing != ECatDoing::Hidden)
	{
		LooterSound::PlayAt(this, Cue, Position + FVector(0.0, 0.0, 22.0));
	}
}

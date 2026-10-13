// ASpitterSpider: construction (its numbers, its model), its life (rank traits, hurt, death, coming back) and its venom sac
// (found on its model, swelling, throbbing and glowing with its tell, bursting when a shot chokes a spit or it dies).
// SpitterSpiderMoves.cpp has its spit and its bite, SpitterSpiderKeepAway.cpp how it keeps its distance.

#include "Creatures/SpitterSpider.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Combat/BulletSubsystem.h"
#include "Combat/EnemyProjectileSubsystem.h"
#include "Combat/HealthComponent.h"
#include "Creatures/SpitterGlob.h"
#include "Weapons/WeaponFX.h"
#include "AnimationRuntime.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/PackageName.h"
#include "Player/PlayerSize.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	/** Its model (Art/Models/Creatures/Spitter.py). Until it's imported the Spitter wears the brown spider's, with no sac. */
	const TCHAR* SpitterModelPath = TEXT("/Game/Art/Creatures/SK_Spitter.SK_Spitter");

	/** An asset made after this code, found only once it exists (as the Gravemother finds her pale hide). */
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

	/**
	 * The sac: at rest it breathes this much (share of its size) this fast (radians a second); a tell swells it by up to
	 * TellSwell more, throbbing from ThrobSlow to ThrobFast times a second as it nears the spit; a spit empties it by
	 * SpentShrink, refilling over RefillSeconds; dead, it hangs deflated at DeadSize.
	 */
	constexpr float SacBreath = 0.03f;
	constexpr float SacBreathSpeed = 1.3f;
	constexpr float TellSwell = 0.24f;
	constexpr float ThrobSlow = 4.f;
	constexpr float ThrobFast = 10.f;
	constexpr float SpentShrink = 0.2f;
	constexpr float RefillSeconds = 1.6f;
	constexpr float DeadSize = 0.6f;
	/** A crit on the sac flashes it this much brighter (InnerGlow), fading in a fifth of a second. */
	constexpr float FlashGlow = 3.f;
	constexpr float FlashFade = 5.f;
}

ASpitterSpider::ASpitterSpider()
{
	DisplayName = FText::FromString(TEXT("Venom Spitter"));
	// Frailer than the brown spider: it fights from afar, and rushing it should pay.
	Health->MaxHealth = 200.f;
	// The head as on every spider, and the glowing sac: a soft, bright target on its back.
	CriticalSpotBones = { TEXT("head"), SpitterLook::SacBone };

	WalkSpeed = 160.f;
	// A little slower than the brown spider (540 at the full-size player's scale): a walking player gains on it as it
	// backs away, so closing in on it works. Scaled with the player's speed and pace, as the spider's is.
	ChaseSpeed = 480.f * LooterPlayerSize::MoveScale;
	// Its bite, for when it's caught; its glob, splash and puddle are shares of it, so they grow with its level and rank.
	AttackDamage = 10.f;
	AttackWindup = 0.5f;
	AttackRecovery = 0.45f;
	// Its own clocks pace its spit and bite (SpitEvery, BiteCooldown): the brain's cooldown only keeps attacks a beat apart.
	AttackCooldown = 0.25f;
	// The brain's chase stops at three quarters of AttackRange (ACreatureBase's Chase): there it holds and turns to its target,
	// and keeping its distance takes over (BeginPlay sets it again from CloseInTo).
	AttackRange = CloseInTo / 0.75f;
	// Its tag over the sac, which stands above the spider's back.
	HealthBarHeight = 140.f;
	// A kill pays as a spider's does.
	XPReward = 10;

	static USkeletalMesh* const Model = FindIfMade<USkeletalMesh>(SpitterModelPath);
	if (Model)
	{
		GetMesh()->SetSkeletalMeshAsset(Model);
	}
}

void ASpitterSpider::BeginPlay()
{
	Super::BeginPlay();
	// Its bite's timing as its class (or the level) gave it: a spit borrows the attack's and gives it back.
	OwnBiteWindup = AttackWindup;
	OwnBiteRecovery = AttackRecovery;
	AttackRange = CloseInTo / 0.75f;
	SpitReadyIn = FirstSpitDelay;
	StrafeSign = GetUniqueID() % 2 == 0 ? 1.f : -1.f;
	SetupSac();
	TickSac(0.f);
}

void ASpitterSpider::Tick(float DeltaSeconds)
{
	// The brain first (its attacks, its chase, its hold) and the spider's body; then its own clocks, its keeping its
	// distance (input the walking movement takes next frame, as the brain's own is), and its sac laid on the pose.
	Super::Tick(DeltaSeconds);
	Clock += DeltaSeconds;
	TickMoves(DeltaSeconds);
	TickKeepAway(DeltaSeconds);
	TickSac(DeltaSeconds);
}

// ---------------------------------------------------------------------------
// Its life
// ---------------------------------------------------------------------------

void ASpitterSpider::OnRankChanged()
{
	Super::OnRankChanged();
	GlobsPerSpit = SpitterRules::GlobsFor(GetRank());
	IntervalShare = SpitterRules::IntervalShareFor(GetRank());
}

void ASpitterSpider::OnHurt(bool bCritical, const FVector& HitLocation)
{
	Super::OnHurt(bCritical, HitLocation);
	if (!bCritical || !IsSacHit(HitLocation))
	{
		return;
	}
	// A crit on the sac flashes it, and in a tell it chokes the spit: the reward for the shot.
	SacFlash = 1.f;
	if (IsTelling() && !IsDead() && Health && Health->GetHealth() > 0.f)
	{
		ChokeSpit(HitLocation);
	}
}

void ASpitterSpider::OnDied()
{
	Super::OnDied();
	// Its sac bursts as it falls (no puddle: the kill shouldn't burn the player who made it), and the glow at its fangs goes out.
	if (UEnemyProjectileSubsystem* Shots = GetWorld() ? GetWorld()->GetSubsystem<UEnemyProjectileSubsystem>() : nullptr)
	{
		Shots->ClearShotsFrom(this);
	}
	if (SacPoseSlot != INDEX_NONE)
	{
		BurstSac(GetMesh()->GetBoneLocation(SpitterLook::SacBone), 45.f);
	}
	bSpitChoked = false;
	Keep = ESpitterKeep::Close;
	CorneredFor = 0.f;
	SetKeepingDistance(false);
}

void ASpitterSpider::OnRespawned()
{
	Super::OnRespawned();
	// A new life: a full sac, nothing under way.
	Move = ESpitterMove::Spit;
	SpitReadyIn = FirstSpitDelay;
	BiteReadyIn = 0.f;
	bSpitChoked = false;
	bHadTarget = false;
	Keep = ESpitterKeep::Close;
	CorneredFor = 0.f;
	SacSpent = 0.f;
	SacFlash = 0.f;
	SacScale = 1.f;
	AttackWindup = OwnBiteWindup;
	AttackRecovery = OwnBiteRecovery;
	SetKeepingDistance(false);
	TickSac(0.f);
}

// ---------------------------------------------------------------------------
// The sac
// ---------------------------------------------------------------------------

void ASpitterSpider::SetupSac()
{
	SacPoseSlot = INDEX_NONE;
	AbdomenPoseSlot = INDEX_NONE;
	SacLook = nullptr;
	USkeletalMeshComponent* Body = GetMesh();
	const USkeletalMesh* Model = Body ? Body->GetSkeletalMeshAsset() : nullptr;
	if (!Model)
	{
		return;
	}
	// Its glow: the sac's material slot (M_Gel's InnerGlow), one instance per Spitter.
	const int32 Slot = Body->GetMaterialIndex(SpitterLook::SacSlot);
	if (Slot != INDEX_NONE)
	{
		SacLook = Body->CreateDynamicMaterialInstance(Slot);
	}
	// Its swell: the sac's bone, laid on the abdomen's pose every frame (the spider's code moves the abdomen, not the sac).
	const FReferenceSkeleton& Skeleton = Model->GetRefSkeleton();
	const int32 SacIndex = Skeleton.FindBoneIndex(SpitterLook::SacBone);
	const FName AbdomenBone(TEXT("abdomen"));
	const int32 AbdomenIndex = Skeleton.FindBoneIndex(AbdomenBone);
	AbdomenPoseSlot = BonePose.IndexOfByPredicate([&AbdomenBone](const FCreatureBonePose& Pose) { return Pose.Bone == AbdomenBone; });
	if (SacIndex == INDEX_NONE || AbdomenIndex == INDEX_NONE || AbdomenPoseSlot == INDEX_NONE)
	{
		if (SacIndex == INDEX_NONE)
		{
			UE_LOG(LogLooter, Verbose, TEXT("%s: %s has no sac; import SK_Spitter (Art/Models/Creatures/Spitter.py)."), *GetName(), *Model->GetName());
		}
		AbdomenPoseSlot = INDEX_NONE;
		return;
	}
	const FTransform SacRest = FAnimationRuntime::GetComponentSpaceTransformRefPose(Skeleton, SacIndex);
	const FTransform AbdomenRest = FAnimationRuntime::GetComponentSpaceTransformRefPose(Skeleton, AbdomenIndex);
	SacInAbdomen = SacRest.GetRelativeTransform(AbdomenRest);
	SacPoseSlot = BonePose.Add({ SpitterLook::SacBone, FTransform::Identity });
}

bool ASpitterSpider::IsTelling() const
{
	return GetCreatureState() == ECreatureState::Attack && Move == ESpitterMove::Spit && !bSpitChoked && GetStateTime() < AttackWindup;
}

float ASpitterSpider::GetTellStrength() const
{
	if (!IsTelling())
	{
		return 0.f;
	}
	return FMath::SmoothStep(0.f, 1.f, FMath::Clamp(GetStateTime() / FMath::Max(AttackWindup, 0.05f), 0.f, 1.f));
}

bool ASpitterSpider::IsSacHit(const FVector& HitLocation) const
{
	const USkeletalMeshComponent* Body = GetMesh();
	if (!Body || Body->GetBoneIndex(SpitterLook::SacBone) == INDEX_NONE)
	{
		return false;
	}
	const FVector Sac = Body->GetBoneLocation(SpitterLook::SacBone);
	const FVector Head = Body->GetBoneLocation(TEXT("head"));
	return FVector::DistSquared(HitLocation, Sac) < FVector::DistSquared(HitLocation, Head);
}

void ASpitterSpider::TickSac(float DeltaSeconds)
{
	SacSpent = FMath::Max(0.f, SacSpent - DeltaSeconds / RefillSeconds);
	SacFlash = FMath::Max(0.f, SacFlash - DeltaSeconds * FlashFade);
	const float Tell = GetTellStrength();
	if (IsDead())
	{
		// Burst and dark.
		SacScale = DeltaSeconds > 0.f ? FMath::FInterpTo(SacScale, DeadSize, DeltaSeconds, 8.f) : DeadSize;
		SacGlow = DeltaSeconds > 0.f ? FMath::FInterpTo(SacGlow, 0.05f, DeltaSeconds, 4.f) : 0.05f;
	}
	else
	{
		// The throb quickens as the spit nears (its phase kept running, so a changing rate never jumps it).
		SacClock += DeltaSeconds * FMath::Lerp(ThrobSlow, ThrobFast, Tell) * 2.f * UE_PI;
		const float Throb = 0.5f + 0.5f * FMath::Sin(SacClock);
		SacScale = 1.f + SacBreath * FMath::Sin(Clock * SacBreathSpeed) + Tell * TellSwell * (0.65f + 0.35f * Throb) - SpentShrink * SacSpent;
		SacGlow = SacRestGlow * (1.f - 0.75f * SacSpent) + FMath::Max(SacTellGlow - SacRestGlow, 0.f) * Tell * (0.55f + 0.45f * Throb)
			+ FlashGlow * SacFlash;
	}
	if (SacLook)
	{
		SacLook->SetScalarParameterValue(TEXT("InnerGlow"), SacGlow);
	}
	// The sac's bone swells from its root on the abdomen, wherever the spider's code put the abdomen this frame.
	if (SacPoseSlot != INDEX_NONE && !IsPoseFrozen() && BonePose.IsValidIndex(SacPoseSlot) && BonePose.IsValidIndex(AbdomenPoseSlot))
	{
		BonePose[SacPoseSlot].Transform = FTransform(FQuat::Identity, FVector::ZeroVector, FVector(SacScale)) * SacInAbdomen
			* BonePose[AbdomenPoseSlot].Transform;
	}
}

void ASpitterSpider::BurstSac(const FVector& Where, float Size)
{
	UWorld* World = GetWorld();
	if (World && World->IsGameWorld())
	{
		if (UBulletSubsystem* Bullets = World->GetSubsystem<UBulletSubsystem>())
		{
			FWeaponFX& Effects = Bullets->GetEffects();
			Effects.Initialize(World);
			Effects.SpawnDeathBurst(EDeathBurst::Gel, Where, FVector::UpVector, Size * GetSizeScale(), SpitterLook::Venom);
		}
	}
	LooterSound::PlayAt(this, LooterSoundCue::Spitter::Splat, Where, 1.f, 1.15f);
}

FVector ASpitterSpider::GetMuzzle() const
{
	// Just ahead of its fangs: from its head's bone (where its rear has put it) along its facing, at its size.
	const USkeletalMeshComponent* Body = GetMesh();
	const float Scale = GetSizeScale();
	if (Body && Body->GetBoneIndex(TEXT("head")) != INDEX_NONE)
	{
		return Body->GetBoneLocation(TEXT("head")) + GetActorForwardVector() * (38.f * Scale) - FVector(0.0, 0.0, 4.0 * Scale);
	}
	return GetActorTransform().TransformPosition(FVector(95.0, 0.0, 15.0));
}

TArray<ASpitterGlob*> ASpitterSpider::GetLastGlobs() const
{
	TArray<ASpitterGlob*> Globs;
	for (const TWeakObjectPtr<ASpitterGlob>& Each : LastGlobs)
	{
		if (ASpitterGlob* Glob = Each.Get())
		{
			Globs.Add(Glob);
		}
	}
	return Globs;
}

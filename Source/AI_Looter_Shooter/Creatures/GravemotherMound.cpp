// AGravemotherMound (GravemotherMound.h): bulging and stirring, bursting as its spiderling claws out, caved in by a shot or
// by her end, and sinking away.

#include "Creatures/GravemotherMound.h"
#include "AI_Looter_Shooter.h"
#include "Areas/AreaRulesSubsystem.h"
#include "Audio/LooterSound.h"
#include "Combat/BulletSubsystem.h"
#include "Combat/HealthComponent.h"
#include "Creatures/CreatureRankSettings.h"
#include "Creatures/GravemotherAssets.h"
#include "Creatures/GravemotherCreature.h"
#include "Creatures/GravemotherFX.h"
#include "Creatures/SpiderCreature.h"
#include "Progression/PlayerProgressionSubsystem.h"
#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Components/DecalComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/LocalPlayer.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

namespace
{
	/** The mound at full swell (the sheet's 170 x 160 x 47 cm), and its shootable box over it (half extents, cm). */
	const FVector StandInMoundScale(1.7, 1.6, 0.94);
	const FVector HitBoxExtent(90.0, 85.0, 40.0);
	/** It starts this small, and stirs this much this fast as it swells. */
	constexpr float StartShare = 0.15f;
	constexpr float StirShare = 0.04f;
	constexpr float StirHz = 7.f;
	/** The stand-in crater: the sphere flattened (the sheet's 190 x 184 x 29); a caved one smaller and sunk. */
	const FVector StandInCraterScale(1.9, 1.84, 0.3);
	constexpr float CavedShare = 0.7f;
	constexpr float CavedSink = 10.f;
	/** Its cracks' and its crater's decals (cm across). */
	constexpr float CracksSize = 230.f;
	/** Earth sifts off it every this often while it swells (s). */
	constexpr float SiftEvery = 0.12f;
}

AGravemotherMound::AGravemotherMound()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;

	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	Body->SetupAttachment(Root);
	Body->SetMobility(EComponentMobility::Movable);
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Body->SetCanEverAffectNavigation(false);

	// The shot that counts: a generous box over the swelling earth, seen only by bullets, and only while it swells.
	HitBox = CreateDefaultSubobject<UBoxComponent>(TEXT("HitBox"));
	HitBox->SetupAttachment(Root);
	HitBox->SetMobility(EComponentMobility::Movable);
	HitBox->InitBoxExtent(HitBoxExtent);
	HitBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	HitBox->SetCollisionObjectType(ECC_WorldDynamic);
	HitBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	HitBox->SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECR_Block);
	HitBox->SetGenerateOverlapEvents(false);
	HitBox->SetCanEverAffectNavigation(false);
	HitBox->ComponentTags.Add(FName(UBulletSubsystem::OwnLookTag));
	HitBox->ComponentTags.Add(GravemotherAssets::SurfaceDirtTag);

	Health = CreateDefaultSubobject<UHealthComponent>(TEXT("Health"));
	Health->MaxHealth = 1.f;
	Health->bShowDamageNumbers = false;
}

AGravemotherMound* AGravemotherMound::Spawn(AGravemotherCreature& InMother, const FVector& InFeet, float InYaw, float InBulgeSeconds,
	int32 InSpiderlingLevel)
{
	UWorld* World = InMother.GetWorld();
	if (!World)
	{
		return nullptr;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.ObjectFlags |= RF_Transient;
	AGravemotherMound* Mound = World->SpawnActor<AGravemotherMound>(AGravemotherMound::StaticClass(),
		FTransform(FRotator(0.f, FMath::FRandRange(0.f, 360.f), 0.f), InFeet), Params);
	if (!Mound)
	{
		return nullptr;
	}
	Mound->Mother = &InMother;
	Mound->Feet = InFeet;
	Mound->Yaw = InYaw;
	Mound->BulgeSeconds = FMath::Max(InBulgeSeconds, 0.1f);
	Mound->SpiderlingLevel = FMath::Max(InSpiderlingLevel, 1);
	if (UStaticMesh* Mesh = GravemotherAssets::Load<UStaticMesh>(GravemotherAssets::Mound))
	{
		Mound->Body->SetStaticMesh(Mesh);
	}
	else
	{
		Mound->bStandIn = true;
		Mound->Body->SetStaticMesh(GravemotherAssets::StandInMesh(GravemotherAssets::EStandIn::Sphere));
		Mound->Body->SetMaterial(0, GravemotherAssets::StandInLook(Mound, GravemotherAssets::EarthColor()));
	}
	Mound->Cracks = GravemotherAssets::AddGroundDecal(*Mound, GravemotherAssets::DecalMoundCracks, InFeet, FMath::FRandRange(0.f, 360.f), CracksSize,
		CracksSize, 2, 160.f);
	if (!Mound->HasActorBegunPlay())
	{
		Mound->DispatchBeginPlay();
	}
	Mound->Health->OnDeath.AddUniqueDynamic(Mound, &AGravemotherMound::HandleDeath);
	Mound->StirSound = LooterSound::Start(Mound, LooterSoundCue::GravemotherMoundBulge, Mound->Root);
	Mound->Shape();
	return Mound;
}

void AGravemotherMound::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Advance(DeltaSeconds);
}

void AGravemotherMound::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	LooterSound::Stop(StirSound.Get());
	StirSound = nullptr;
	Super::EndPlay(EndPlayReason);
}

float AGravemotherMound::GetBulge() const
{
	return Phase == EPhase::Bulging ? FMath::InterpEaseOut(StartShare, 1.f, FMath::Clamp(Age / BulgeSeconds, 0.f, 1.f), 2.f) : 1.f;
}

ASpiderCreature* AGravemotherMound::GetSpiderling() const
{
	return Spiderling.Get();
}

void AGravemotherMound::Shape()
{
	const float Share = GetBulge();
	const float Stir = 1.f + StirShare * FMath::Sin(Age * 2.f * UE_PI * StirHz);
	const FVector Base = bStandIn ? StandInMoundScale : FVector::OneVector;
	Body->SetRelativeScale3D(FVector(Base.X * Share * Stir, Base.Y * Share * Stir, Base.Z * Share / Stir));
	const float Box = FMath::Max(Share, 0.5f);
	HitBox->SetBoxExtent(HitBoxExtent * Box, false);
	HitBox->SetRelativeLocation(FVector(0.0, 0.0, HitBoxExtent.Z * Box * 0.75));
	GravemotherAssets::SetDecalOpacity(Cracks, FMath::Clamp(Age / (BulgeSeconds * 0.5f), 0.f, 1.f));
}

void AGravemotherMound::Advance(float DeltaSeconds)
{
	if (IsActorBeingDestroyed())
	{
		return;
	}
	const float Dt = FMath::Max(DeltaSeconds, 0.f);
	if (Phase == EPhase::Bulging)
	{
		Age += Dt;
		Shape();
		// Earth sifting off it, stirring from under.
		SiftIn -= Dt;
		if (SiftIn <= 0.f)
		{
			SiftIn += SiftEvery;
			if (UGravemotherFX* Effects = UGravemotherFX::Get(this))
			{
				FGraveDustBurst Sift;
				Sift.Count = 1;
				Sift.Spread = 50.f;
				Sift.SpreadUp = 20.f;
				Sift.StartSize = 35.f;
				Sift.EndSize = 90.f;
				Sift.Life = 0.9f;
				Sift.Opacity = 0.22f;
				Sift.Outward = 40.f;
				Effects->Puffs(Feet + FVector(0.0, 0.0, 30.0), Sift);
				Effects->Chunks(EGraveChunk::Debris, Feet + FVector(0.0, 0.0, 35.0), FVector::UpVector, 1, 180.f, 0.5f);
			}
		}
		if (Age >= BulgeSeconds)
		{
			Burst();
		}
		return;
	}
	// A crater or a caved hole: it lies a while, then sinks away.
	EndAge += Dt;
	const float Lies = Phase == EPhase::Burst ? CraterSeconds : CavedSeconds;
	const float Sink = FMath::Clamp((EndAge - Lies) / SinkSeconds, 0.f, 1.f);
	if (Sink > 0.f)
	{
		const float Depth = Phase == EPhase::Popped || Phase == EPhase::Collapsed ? CavedSink : 0.f;
		Body->SetRelativeLocation(FVector(0.0, 0.0, -Depth - Sink * 40.f));
		GravemotherAssets::SetDecalOpacity(Cracks, 1.f - Sink);
		GravemotherAssets::SetDecalOpacity(Scar, 1.f - Sink);
	}
	if (Sink >= 1.f)
	{
		Destroy();
	}
}

void AGravemotherMound::Burst()
{
	Phase = EPhase::Burst;
	EndAge = 0.f;
	LooterSound::Stop(StirSound.Get());
	StirSound = nullptr;
	HitBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Health->bInvulnerable = true;
	if (UStaticMesh* Crater = GravemotherAssets::Load<UStaticMesh>(GravemotherAssets::MoundBurst))
	{
		Body->SetStaticMesh(Crater);
		Body->SetRelativeScale3D(FVector::OneVector);
	}
	else
	{
		bCraterStandIn = true;
		Body->SetStaticMesh(GravemotherAssets::StandInMesh(GravemotherAssets::EStandIn::Sphere));
		Body->SetRelativeScale3D(StandInCraterScale);
	}
	Scar = GravemotherAssets::AddGroundDecal(*this, GravemotherAssets::DecalImpactScar, Feet, FMath::FRandRange(0.f, 360.f), CracksSize, CracksSize, 1, 160.f);
	GravemotherAssets::SetDecalOpacity(Scar, 1.f);
	if (UGravemotherFX* Effects = UGravemotherFX::Get(this))
	{
		FGraveDustBurst Out;
		Out.Count = 12;
		Out.Spread = 50.f;
		Out.SpreadUp = 30.f;
		Out.StartSize = 60.f;
		Out.EndSize = 160.f;
		Out.Life = 1.4f;
		Out.Opacity = 0.34f;
		Out.Drift = FVector(0.0, 0.0, 60.0);
		Out.Outward = 170.f;
		Effects->Puffs(Feet + FVector(0.0, 0.0, 30.0), Out);
		Effects->Chunks(EGraveChunk::Debris, Feet + FVector(0.0, 0.0, 30.0), FVector::UpVector, 5, 450.f);
	}
	LooterSound::PlayAt(this, LooterSoundCue::GravemotherMoundBurst, Feet);
	// Its spiderling claws out (her brood, her boss's add), unless she's gone.
	AGravemotherCreature* Her = Mother.Get();
	if (Her && !Her->IsDead())
	{
		Spiderling = Her->RaiseFromMound(*this);
	}
}

void AGravemotherMound::CaveIn(EPhase Next)
{
	Phase = Next;
	EndAge = 0.f;
	LooterSound::Stop(StirSound.Get());
	StirSound = nullptr;
	HitBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Health->bInvulnerable = true;
	// A hole where it fell in on itself.
	if (UStaticMesh* Crater = GravemotherAssets::Load<UStaticMesh>(GravemotherAssets::MoundBurst))
	{
		Body->SetStaticMesh(Crater);
		Body->SetRelativeScale3D(FVector(CavedShare));
	}
	else
	{
		Body->SetStaticMesh(GravemotherAssets::StandInMesh(GravemotherAssets::EStandIn::Sphere));
		Body->SetRelativeScale3D(StandInCraterScale * CavedShare);
	}
	Body->SetRelativeLocation(FVector(0.0, 0.0, -CavedSink));
}

bool AGravemotherMound::Pop(AController* By)
{
	if (Phase != EPhase::Bulging)
	{
		return false;
	}
	CaveIn(EPhase::Popped);
	// A muffled burst, the earth caving in on what was under it.
	if (UGravemotherFX* Effects = UGravemotherFX::Get(this))
	{
		FGraveDustBurst Caved;
		Caved.Count = 8;
		Caved.Spread = 70.f;
		Caved.SpreadUp = 40.f;
		Caved.StartSize = 55.f;
		Caved.EndSize = 140.f;
		Caved.Life = 1.2f;
		Caved.Opacity = 0.33f;
		Caved.Drift = FVector(0.0, 0.0, 70.0);
		Caved.Outward = -60.f;
		Effects->Puffs(Feet + FVector(0.0, 0.0, 25.0), Caved);
		Effects->Chunks(EGraveChunk::Debris, Feet + FVector(0.0, 0.0, 30.0), FVector::UpVector, 3, 260.f);
	}
	LooterSound::PlayAt(this, LooterSoundCue::GravemotherMoundPop, Feet);
	CreditKill(By);
	if (AGravemotherCreature* Her = Mother.Get())
	{
		Her->NoteMoundPopped(*this, By);
	}
	return true;
}

void AGravemotherMound::Collapse()
{
	if (Phase == EPhase::Bulging)
	{
		CaveIn(EPhase::Collapsed);
	}
}

void AGravemotherMound::HandleDeath(AController* Killer)
{
	Pop(Killer);
}

void AGravemotherMound::CreditKill(AController* By) const
{
	// The skill's reward (Main's call, 2026-10-20): the spiderling's kill, as if it had dug out and been shot.
	const APlayerController* Player = Cast<APlayerController>(By);
	const ULocalPlayer* Local = Player ? Player->GetLocalPlayer() : nullptr;
	UPlayerProgressionSubsystem* Progression = Local ? Local->GetSubsystem<UPlayerProgressionSubsystem>() : nullptr;
	if (!Progression)
	{
		return;
	}
	const ASpiderCreature* Kind = GetDefault<ASpiderCreature>();
	Progression->RecordDefeat(Kind);
	if (UAreaRulesSubsystem::GivesKillExperienceAt(this))
	{
		const int32 Reward = FMath::Max(0, FMath::RoundToInt32(Kind->XPReward * UCreatureRankSettings::Get(ECreatureRank::Basic).XPMultiplier));
		const int64 XP = UPlayerProgressionSubsystem::GetLevelRules().KillXP(Reward, SpiderlingLevel, Progression->GetLevel());
		if (XP > 0)
		{
			const bool bAdds = !Progression->IsMaxLevel();
			Progression->AddXP(XP, EXPSource::Kill);
			if (bAdds)
			{
				Progression->OnKillXP.Broadcast(XP, this);
			}
		}
	}
	LooterSound::Play2D(Player, LooterSoundCue::Kill);
}

// AGravemotherStone (GravemotherStone.h): its making and placing on her abdomen, its health and three states, a shot's look
// on it. Its crack-off and pieces are GravemotherStoneBreak.cpp's.

#include "Creatures/GravemotherStone.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Combat/BulletSubsystem.h"
#include "Combat/HealthComponent.h"
#include "Creatures/GravemotherAssets.h"
#include "Creatures/GravemotherCreature.h"
#include "Creatures/GravemotherFX.h"
#include "Creatures/GravemotherFight.h"
#include "World/BreakableKinds.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DamageEvents.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
	/** The stand-in slab: the engine cube (100 cm) made 22 cm thick, 70 wide and 85 tall at her model's size, on its foot (the pivot). */
	const FVector StandInSlabScale(0.22, 0.7, 0.85);
	const FVector StandInSlabOffset(0.0, 0.0, 42.5);
	/** Its granite darkens a little with each crack (the stand-in's only way to show it). */
	constexpr float StandInCrackDarken = 0.18f;

	/** A shot's crack on it isn't played twice within this long (s) and this near (cm): a volley is one crack. */
	constexpr double HitSoundGap = 0.05;
	constexpr float HitSoundMerge = 250.f;
	/** The flash of a shot on it, this big (cm). */
	constexpr float HitFlashSize = 55.f;
}

AGravemotherStone::AGravemotherStone()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	// Its break is no kill (the HUD's white marker), and bullets leave no marks on it.
	Tags.Add(FName(LooterBreakables::Tag));

	Pivot = CreateDefaultSubobject<USceneComponent>(TEXT("Pivot"));
	Pivot->SetMobility(EComponentMobility::Movable);
	RootComponent = Pivot;

	Slab = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Slab"));
	Slab->SetupAttachment(Pivot);
	Slab->SetMobility(EComponentMobility::Movable);
	// Only bullets see it: never her movement, her lane's sweeps, her eyes or the camera.
	Slab->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Slab->SetCollisionObjectType(ECC_WorldDynamic);
	Slab->SetCollisionResponseToAllChannels(ECR_Ignore);
	Slab->SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECR_Block);
	Slab->SetGenerateOverlapEvents(false);
	Slab->SetCanEverAffectNavigation(false);
	Slab->bReceivesDecals = false;
	// It shows and sounds its own hits (sparks off granite), and it's stone to anything reading surfaces.
	Slab->ComponentTags.Add(FName(UBulletSubsystem::OwnLookTag));
	Slab->ComponentTags.Add(GravemotherAssets::SurfaceStoneTag);

	Health = CreateDefaultSubobject<UHealthComponent>(TEXT("Health"));
	Health->MaxHealth = 1000.f;
}

AGravemotherStone* AGravemotherStone::Spawn(AGravemotherCreature& InMother, const FTransform& Placement, const FGravemotherStoneSettings& InSettings)
{
	UWorld* World = InMother.GetWorld();
	if (!World)
	{
		return nullptr;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.ObjectFlags |= RF_Transient;
	Params.Owner = &InMother;
	AGravemotherStone* Stone = World->SpawnActor<AGravemotherStone>(AGravemotherStone::StaticClass(), InMother.GetActorTransform(), Params);
	if (!Stone)
	{
		return nullptr;
	}
	Stone->Mother = &InMother;
	Stone->Settings = InSettings;
	// On her abdomen bone: it moves with her breathing, her rears and her death, and her size carries its.
	Stone->AttachToComponent(InMother.GetMesh(), FAttachmentTransformRules::KeepRelativeTransform, AGravemotherCreature::AbdomenBone);
	Stone->SetActorRelativeTransform(Placement);
	Stone->MakeShards();
	if (!Stone->HasActorBegunPlay())
	{
		Stone->DispatchBeginPlay();
	}
	Stone->ShowState();
	return Stone;
}

void AGravemotherStone::BeginPlay()
{
	Super::BeginPlay();
	Health->OnDamaged.AddDynamic(this, &AGravemotherStone::HandleDamaged);
	Health->OnDeath.AddDynamic(this, &AGravemotherStone::HandleDeath);
}

void AGravemotherStone::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Advance(DeltaSeconds);
}

// ---------------------------------------------------------------------------
// Health and states
// ---------------------------------------------------------------------------

void AGravemotherStone::ResetStone(float InMaxHealth)
{
	State = EGravemotherStone::Intact;
	Health->SetMaxHealth(FMath::Max(InMaxHealth, 1.f));
	Health->ResetHealth();
	Slab->SetHiddenInGame(false);
	SetShootable(true);
	ClearMark();
	HideShards();
	ShowState();
}

float AGravemotherStone::GetHealthShare() const
{
	return IsOn() ? Health->GetHealthPercent() : 0.f;
}

void AGravemotherStone::SetShootable(bool bShootable)
{
	Slab->SetCollisionEnabled(bShootable && IsOn() ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
}

void AGravemotherStone::HandleDamaged(float Damage, bool bCritical, FVector HitLocation, AController* InstigatedBy, AActor* DamageCauser)
{
	if (!IsOn())
	{
		return;
	}
	// A hit on the stone turns her on the shooter as any hit on her would.
	AGravemotherCreature* Her = Mother.Get();
	if (Her)
	{
		Her->NoteStoneHit(InstigatedBy);
	}
	const EGravemotherStone Now = GravemotherFight::StoneStateFor(Health->GetHealthPercent(), Settings.CrackedAt, Settings.ShatteredAt);
	if (Now == EGravemotherStone::Broken || static_cast<uint8>(Now) <= static_cast<uint8>(State))
	{
		return;
	}
	// A new crack: its sound, chips and dust off it, and she feels it.
	State = Now;
	ShowState();
	LooterSound::PlayAttached(LooterSoundCue::GravemotherStoneCrack, Slab);
	Shower(6, 10, 420.f);
	if (Her)
	{
		Her->OnStoneCracked(State);
	}
	UE_LOG(LogLooter, Log, TEXT("%s: the headstone is %s (%.0f / %.0f)."), *GetNameSafe(Her),
		State == EGravemotherStone::Cracked ? TEXT("cracked") : TEXT("shattered"), Health->GetHealth(), Health->GetMaxHealth());
}

void AGravemotherStone::HandleDeath(AController* Killer)
{
	Break(false);
}

void AGravemotherStone::ShowState()
{
	ShownMesh = IsOn() ? static_cast<int32>(State) : -1;
	if (!IsOn())
	{
		return;
	}
	UStaticMesh* Mesh = GravemotherAssets::Load<UStaticMesh>(GravemotherAssets::StonePath(ShownMesh));
	bStandIn = Mesh == nullptr;
	if (Mesh)
	{
		Slab->SetStaticMesh(Mesh);
		Slab->SetRelativeTransform(FTransform::Identity);
		Slab->EmptyOverrideMaterials();
		return;
	}
	// The stand-in: a granite slab of the engine cube, a little darker with each crack.
	Slab->SetStaticMesh(GravemotherAssets::StandInMesh(GravemotherAssets::EStandIn::Cube));
	Slab->SetRelativeTransform(FTransform(FQuat::Identity, StandInSlabOffset, StandInSlabScale));
	const float Darken = 1.f - StandInCrackDarken * static_cast<float>(ShownMesh);
	Slab->SetMaterial(0, GravemotherAssets::StandInLook(this, GravemotherAssets::GraniteColor() * Darken));
}

// ---------------------------------------------------------------------------
// Shots on it
// ---------------------------------------------------------------------------

float AGravemotherStone::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (!IsOn())
	{
		return 0.f;
	}
	// A bullet's hit glances off with its own look (it carries the bullets' own-look tag); the damage goes to its health.
	FVector Where = GetActorLocation();
	if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
	{
		const FPointDamageEvent& Point = static_cast<const FPointDamageEvent&>(DamageEvent);
		PlayHitLook(Point.HitInfo.ImpactPoint, Point.HitInfo.ImpactNormal, Point.ShotDirection);
		Where = Point.HitInfo.ImpactPoint;
	}
	// From off her ground (the Sink's rim) it does nothing, as a shot on her wouldn't (the user, 2026-10-27): she's fought
	// down in the Sink. The granite's look, her glancing whine, her bar and Hob's hint all the same.
	AGravemotherCreature* Her = Mother.Get();
	if (Her && DamageAmount > 0.f && Her->ShrugFor(EventInstigator) == EGravemotherShrug::OffGround)
	{
		Her->PlayShrugSound(Where, EGravemotherShrug::OffGround);
		Her->AnswerRimShot(EventInstigator);
		Her->NoteShrug(EGravemotherShrug::OffGround, EventInstigator);
		return 0.f;
	}
	return Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
}

void AGravemotherStone::PlayHitLook(const FVector& Where, const FVector& Normal, const FVector& ShotDirection)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	const FVector Out = Normal.IsNearlyZero() ? -ShotDirection.GetSafeNormal() : Normal.GetSafeNormal();
	if (UGravemotherFX* Effects = UGravemotherFX::Get(this))
	{
		Effects->Sparks(Where, Out, ShotDirection, FMath::RandRange(3, 6));
		// One flash a frame: a shotgun's pellets light up once.
		if (LastFlashFrame != GFrameCounter)
		{
			LastFlashFrame = GFrameCounter;
			Effects->Flash(Where + Out * 3.f, HitFlashSize);
		}
		FGraveDustBurst Dust;
		Dust.Kind = EGraveDust::Stone;
		Dust.Count = 4;
		Dust.Spread = 8.f;
		Dust.SpreadUp = 4.f;
		Dust.StartSize = 18.f;
		Dust.EndSize = 42.f;
		Dust.Life = 0.7f;
		Dust.Opacity = 0.4f;
		Dust.Drift = Out * 60.f + FVector(0.0, 0.0, 20.0);
		Dust.Outward = 40.f;
		Effects->Puffs(Where + Out * 4.f, Dust);
		Effects->Chunks(EGraveChunk::StoneChip, Where + Out * 3.f, Out, FMath::RandRange(2, 3), 380.f);
	}
	const double Now = World->GetTimeSeconds();
	if (Now - LastHitSound >= HitSoundGap || FVector::DistSquared(Where, LastHitSoundAt) > FMath::Square(HitSoundMerge))
	{
		LastHitSound = Now;
		LastHitSoundAt = Where;
		LooterSound::PlayAt(this, LooterSoundCue::GravemotherStoneHit, Where);
	}
}

void AGravemotherStone::Shower(int32 Chips, int32 Dust, float Speed)
{
	UGravemotherFX* Effects = UGravemotherFX::Get(this);
	if (!Effects)
	{
		return;
	}
	const FBoxSphereBounds Bounds = Slab->Bounds;
	const FVector Middle = Bounds.Origin;
	FGraveDustBurst Burst;
	Burst.Kind = EGraveDust::Stone;
	Burst.Count = Dust;
	Burst.Spread = static_cast<float>(Bounds.SphereRadius) * 0.6f;
	Burst.SpreadUp = static_cast<float>(Bounds.SphereRadius) * 0.5f;
	Burst.StartSize = 35.f;
	Burst.EndSize = 110.f;
	Burst.Life = 1.4f;
	Burst.Opacity = 0.35f;
	Burst.Outward = 140.f;
	Effects->Puffs(Middle, Burst);
	Effects->Chunks(EGraveChunk::StoneChip, Middle, FVector::UpVector, Chips, Speed, 1.2f);
}

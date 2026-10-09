// ABreakableProp: a crate or barrel that breaks: its kind's models, being hit (the rock), the break (the pieces, the dust,
// the sound, the stump), what it held, and staying broken with the session.

#include "World/BreakableProp.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Combat/BulletSubsystem.h"
#include "Combat/HealthComponent.h"
#include "Loot/LootLibrary.h"
#include "Loot/LootTable.h"
#include "Loot/SoulMotePickup.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponFX.h"
#include "World/BreakableDebris.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Math/RandomStream.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	/** The tag the minimap and the scatter read on solid things standing on the ground. */
	const FName ObstacleTag(TEXT("Obstacle"));

	/** Every kind, for telling a kind's own body from one the build script chose. */
	constexpr EBreakableKind AllKinds[] = { EBreakableKind::SlattedCrate, EBreakableKind::PackingCrate, EBreakableKind::Barrel };

	/** A model by its object path, quietly null while it isn't imported. */
	UStaticMesh* LoadModel(const FString& Path)
	{
		if (Path.IsEmpty() || !FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(Path)))
		{
			return nullptr;
		}
		return LoadObject<UStaticMesh>(nullptr, *Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
	}

	/** Mesh is some kind's own body (or none), so it follows the kind; anything else was chosen for this prop. */
	bool IsKindBody(const UStaticMesh* Mesh)
	{
		if (!Mesh)
		{
			return true;
		}
		const FString Path = Mesh->GetPathName();
		for (const EBreakableKind Each : AllKinds)
		{
			if (Path == FBreakableKindInfo::Get(Each).BodyPath)
			{
				return true;
			}
		}
		return false;
	}

	/** The break's dust and splinters: the light at the spot is unknown, so the dust is a dim colour that never glows. */
	constexpr float DustOpacity = 0.55f;

	/** Drawn as far as the dressing's yard props it stands among (build_area_dressing.py SMALL_CULL: about 70 m on Medium). */
	constexpr float CullDistance = 12000.f;
}

ABreakableProp::ABreakableProp()
{
	// Only for the instant a hit rocks it; its pieces fly in the debris subsystem.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	// Solid, so it's stood on and the shots and the strike find it; world dynamic, so ground traces (a creature's footing,
	// the scatter, loot landing) never take it for the ground. The body is the root: a strike's hit names it, so the
	// impact reads its wood.
	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	Body->SetMobility(EComponentMobility::Movable);
	Body->SetCollisionProfileName(UCollisionProfile::BlockAllDynamic_ProfileName);
	Body->SetGenerateOverlapEvents(false);
	Body->SetCanEverAffectNavigation(false);
	Body->LDMaxDrawDistance = CullDistance;
	RootComponent = Body;

	// The stump: nothing to stand on or shoot (the pieces are gone, a kerb of splinters is left), shown once broken.
	Stump = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Stump"));
	Stump->SetupAttachment(Body);
	Stump->SetMobility(EComponentMobility::Movable);
	Stump->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Stump->SetGenerateOverlapEvents(false);
	Stump->SetCanEverAffectNavigation(false);
	Stump->LDMaxDrawDistance = CullDistance;
	Stump->SetVisibility(false);

	// A prop, not an enemy: no numbers over it (the hit marker still says it was hit).
	Health = CreateDefaultSubobject<UHealthComponent>(TEXT("Health"));
	Health->MaxHealth = FBreakableKindInfo::Get(Kind).Health;
	Health->bShowDamageNumbers = false;

	Tags.Add(FName(LooterBreakables::Tag));
	Tags.Add(ObstacleTag);
}

void ABreakableProp::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyKind();
	bRestCaptured = false;
}

void ABreakableProp::BeginPlay()
{
	// Its kind's health before the health component starts full with it (its BeginPlay comes from ours).
	if (PieceMeshes.IsEmpty() || !StumpMesh)
	{
		// Built before its pieces were imported: they're looked for once more.
		ApplyKind();
	}
	Health->MaxHealth = GetKindInfo().Health;
	Super::BeginPlay();
	Tags.AddUnique(FName(LooterBreakables::Tag));
	if (!bBroken)
	{
		Tags.AddUnique(ObstacleTag);
	}
	Health->OnDamaged.AddDynamic(this, &ABreakableProp::HandleDamaged);
	Health->OnDeath.AddDynamic(this, &ABreakableProp::HandleDeath);
	RestTransform = GetActorTransform();
	bRestCaptured = true;
}

void ABreakableProp::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Advance(DeltaSeconds);
}

// ---------------------------------------------------------------------------
// Its kind
// ---------------------------------------------------------------------------

void ABreakableProp::ApplyKind()
{
	const FBreakableKindInfo Info = GetKindInfo();
	if (Body && IsKindBody(Body->GetStaticMesh()))
	{
		Body->SetStaticMesh(LoadModel(Info.BodyPath));
	}
	PieceMeshes.Reset();
	for (int32 Index = 0; Index < Info.PieceCount; ++Index)
	{
		if (UStaticMesh* Piece = LoadModel(Info.PiecePath(Index)))
		{
			PieceMeshes.Add(Piece);
		}
	}
	StumpMesh = LoadModel(Info.StumpPath);
	if (Health)
	{
		Health->MaxHealth = Info.Health;
	}
	if (bBroken && Stump)
	{
		Stump->SetStaticMesh(StumpMesh);
	}
}

FName ABreakableProp::GetSaveKey() const
{
	return BreakableId.IsNone() ? GetFName() : BreakableId;
}

FVector ABreakableProp::GetMiddle() const
{
	return GetActorTransform().TransformPosition(FVector(0.0, 0.0, GetKindInfo().Height * 0.5));
}

// ---------------------------------------------------------------------------
// Being hit
// ---------------------------------------------------------------------------

void ABreakableProp::HandleDamaged(float Damage, bool bCritical, FVector HitLocation, AController* InstigatedBy, AActor* DamageCauser)
{
	if (bBroken)
	{
		return;
	}
	// The blow's way: from whoever dealt it (their pawn, else what dealt it) through the prop; else from the hit inward.
	const APawn* Pawn = InstigatedBy ? InstigatedBy->GetPawn() : nullptr;
	const FVector From = Pawn ? Pawn->GetActorLocation() : (DamageCauser ? DamageCauser->GetActorLocation() : HitLocation);
	FVector Blow = (GetMiddle() - From).GetSafeNormal2D();
	if (Blow.IsNearlyZero())
	{
		Blow = (GetMiddle() - HitLocation).GetSafeNormal2D();
	}
	LastBlow = Blow;
	if (Health->IsDead() || Health->GetHealth() <= 0.f)
	{
		// It breaks on this one (its death comes next).
		return;
	}
	// Rocked away from the blow: its top tips along it, about a level axis across it.
	if (!bRestCaptured)
	{
		RestTransform = GetActorTransform();
		bRestCaptured = true;
	}
	const FVector Axis = FVector::CrossProduct(FVector::UpVector, Blow.IsNearlyZero() ? GetActorForwardVector() : Blow);
	ShudderAxis = Axis.IsNearlyZero() ? FVector::RightVector : Axis.GetSafeNormal();
	ShudderClock = 0.f;
	SetActorTickEnabled(true);
}

void ABreakableProp::HandleDeath(AController* Killer)
{
	Break(Killer, LastBlow);
}

void ABreakableProp::Advance(float DeltaSeconds)
{
	if (!bRestCaptured || ShudderClock >= LooterBreakables::ShudderSeconds)
	{
		SetActorTickEnabled(false);
		return;
	}
	ShudderClock = FMath::Min(ShudderClock + DeltaSeconds, LooterBreakables::ShudderSeconds);
	const float Share = ShudderClock / LooterBreakables::ShudderSeconds;
	// Out quickly, back, a little past, and still: a damped half and a bit of a swing.
	ShudderAngle = LooterBreakables::ShudderDegrees * FMath::Sin(UE_PI * 1.6f * Share) * FMath::Pow(1.f - Share, 1.5f);
	const FQuat Rock(ShudderAxis, FMath::DegreesToRadians(ShudderAngle));
	SetActorTransform(FTransform(Rock * RestTransform.GetRotation(), RestTransform.GetLocation(), RestTransform.GetScale3D()));
	if (Share >= 1.f)
	{
		ShudderAngle = 0.f;
		SetActorTransform(RestTransform);
		SetActorTickEnabled(false);
	}
}

// ---------------------------------------------------------------------------
// Breaking
// ---------------------------------------------------------------------------

bool ABreakableProp::Break(AController* By, FVector Blow)
{
	if (bBroken)
	{
		return false;
	}
	bBroken = true;
	// Whatever the rock was doing, it breaks from where it stands.
	if (bRestCaptured)
	{
		SetActorTransform(RestTransform);
	}
	ShudderAngle = 0.f;
	ShudderClock = LooterBreakables::ShudderSeconds;
	SetActorTickEnabled(false);

	ShowBroken();
	Burst(Blow.IsNearlyZero() ? LastBlow : Blow);
	DropLoot(By);
	UE_LOG(LogLooter, Verbose, TEXT("%s (%s): broken%s."), *GetActorNameOrLabel(), *GetSaveKey().ToString(),
		By ? *FString::Printf(TEXT(" by %s"), *By->GetName()) : TEXT(""));
	return true;
}

void ABreakableProp::ShowBroken()
{
	// The body goes (its stump stays: it hangs from the body, so it isn't hidden with it).
	Body->SetVisibility(false, /*bPropagateToChildren*/ false);
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Stump->SetStaticMesh(StumpMesh);
	Stump->SetVisibility(StumpMesh != nullptr);
	Tags.Remove(ObstacleTag);
}

void ABreakableProp::Burst(const FVector& Blow)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	const FBreakableKindInfo Info = GetKindInfo();
	const FTransform From = GetActorTransform();
	const FVector Middle = GetMiddle();
	FRandomStream Random(FMath::Rand());

	// Each piece out of where it stood, away from the middle through its own, carried along by the blow.
	if (UBreakableDebrisSubsystem* Debris = UBreakableDebrisSubsystem::Get(this))
	{
		for (UStaticMesh* Piece : PieceMeshes)
		{
			if (!Piece)
			{
				continue;
			}
			const FVector PieceMiddle = From.TransformPosition(Piece->GetBounds().Origin);
			const FVector Velocity = LooterBreakables::PieceVelocity(PieceMiddle - Middle, Blow, Random);
			Debris->Throw(Piece, From, Velocity, LooterBreakables::PieceSpin(Random),
				Random.FRandRange(LooterBreakables::PieceLifeMin, LooterBreakables::PieceLifeMax), this);
		}
	}

	// Dust rolling out from its foot and middle, and splinters flung with the pieces (the bullets' pooled effects).
	if (UBulletSubsystem* Bullets = World->GetSubsystem<UBulletSubsystem>())
	{
		FWeaponFX& Effects = Bullets->GetEffects();
		Effects.Initialize(World);
		const FVector Foot = From.GetLocation();
		const float Radius = FMath::Max(static_cast<float>(Body->GetStaticMesh() ? Body->GetStaticMesh()->GetBounds().BoxExtent.Size2D() : 40.0), 25.f);
		for (int32 Index = 0; Index < LooterBreakables::DustPuffs; ++Index)
		{
			const FVector Out = FRotator(0.f, Random.FRandRange(0.f, 360.f), 0.f).Vector();
			const FVector Spot = Foot + Out * Radius * Random.FRandRange(0.2f, 0.8f)
				+ FVector::UpVector * Info.Height * Random.FRandRange(0.1f, 0.8f);
			const FVector Drift = Out * Random.FRandRange(40.f, 130.f) + Blow * 40.f + FVector::UpVector * Random.FRandRange(15.f, 70.f);
			Effects.SpawnDustPuff(Spot, Drift, Info.DustColor, DustOpacity, Random.FRandRange(18.f, 30.f), Random.FRandRange(90.f, 160.f),
				Random.FRandRange(1.1f, 1.9f));
		}
		for (int32 Index = 0; Index < LooterBreakables::Splinters; ++Index)
		{
			const FVector Out = (Random.GetUnitVector() + Blow * 0.6f).GetSafeNormal2D();
			const FVector Spot = Middle + Random.GetUnitVector() * Radius * 0.4f;
			const FVector Throw = Out * Random.FRandRange(220.f, 520.f) + FVector::UpVector * Random.FRandRange(140.f, 420.f);
			Effects.SpawnGrit(Spot, Throw, Random.FRandRange(1.5f, 3.6f), Random.FRandRange(0.6f, 1.1f));
		}
	}

	if (Info.BreakCue)
	{
		LooterSound::PlayAt(this, Info.BreakCue, Middle);
	}
}

void ABreakableProp::DropLoot(AController* By)
{
	if (!bDropsLoot || bLootGiven)
	{
		return;
	}
	bLootGiven = true;
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	// The kill rules: an ammo pickup now and then, its class leaning toward the gun that broke it (the one in hand for a
	// strike), a kill's rounds; never a gun, in a practice area or anywhere.
	const AWeaponBase* Gun = AWeaponBase::FindKillWeapon(this);
	const TOptional<EAmmoType> GunAmmo = Gun ? TOptional<EAmmoType>(Gun->GetAmmoType()) : TOptional<EAmmoType>();
	for (AActor* Loot : ULootLibrary::SpawnKillLoot(this, MakeLootTable(), GetActorLocation(), 1, 0.f, GunAmmo, /*bWeapons*/ false))
	{
		DroppedLoot.Add(Loot);
	}
	FRandomStream MoteRoll(FMath::Rand());
	if (LooterBreakables::RollMote(GetKindInfo(), MoteRoll))
	{
		for (ASoulMotePickup* Mote : ASoulMotePickup::SpawnMotes(World, GetMiddle(), 1))
		{
			DroppedLoot.Add(Mote);
		}
	}
}

ULootTable* ABreakableProp::MakeLootTable() const
{
	const FBreakableKindInfo Info = GetKindInfo();
	ULootTable* Table = NewObject<ULootTable>(GetTransientPackage(), NAME_None, RF_Transient);
	// The game's ammo classes; no guns. The amounts and the lean toward the breaking gun stay a kill's (the table's own).
	if (const ULootTable* Default = ULootLibrary::GetDefaultLootTable())
	{
		Table->AmmoTypes = Default->AmmoTypes;
	}
	Table->Entries.Reset();
	Table->WeaponDropChance = 0.f;
	Table->MinWeaponDrops = 0;
	Table->MaxWeaponDrops = 0;
	Table->AmmoDropChance = Info.AmmoChance;
	Table->MinAmmoDrops = 1;
	Table->MaxAmmoDrops = 1;
	return Table;
}

TArray<AActor*> ABreakableProp::GetDroppedLoot() const
{
	TArray<AActor*> Loot;
	for (const TWeakObjectPtr<AActor>& Each : DroppedLoot)
	{
		if (AActor* Actor = Each.Get(); Actor && !Actor->IsActorBeingDestroyed())
		{
			Loot.Add(Actor);
		}
	}
	return Loot;
}

// ---------------------------------------------------------------------------
// The session, the console
// ---------------------------------------------------------------------------

void ABreakableProp::RestoreBroken()
{
	bBroken = true;
	bLootGiven = true;
	ShudderAngle = 0.f;
	ShudderClock = LooterBreakables::ShudderSeconds;
	if (bRestCaptured)
	{
		SetActorTransform(RestTransform);
	}
	SetActorTickEnabled(false);
	ShowBroken();
}

void ABreakableProp::Mend()
{
	bBroken = false;
	bLootGiven = false;
	DroppedLoot.Reset();
	Body->SetVisibility(true, /*bPropagateToChildren*/ false);
	Body->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Stump->SetVisibility(false);
	Health->ResetHealth();
	Tags.AddUnique(ObstacleTag);
}

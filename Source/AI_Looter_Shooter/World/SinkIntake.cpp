// ASinkIntake: the barge's intake in the Sink: its story, its draw on the player, the charges, the fuse, the choke, the
// swing's knocks, the smog pool and the growl. The hoses' shapes and sections are SinkIntakeHose.cpp's.

#include "World/SinkIntake.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Bosses/BossCameraShake.h"
#include "Combat/BulletSubsystem.h"
#include "Combat/HealthComponent.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/EncounterSpawner.h"
#include "Creatures/EncounterSubsystem.h"
#include "Missions/MissionObjective.h"
#include "Missions/MissionRunner.h"
#include "Player/PlayerRattleSubsystem.h"
#include "Session/CampaignRecord.h"
#include "Story/CaptionSubsystem.h"
#include "Story/StoryLineSet.h"
#include "Weapons/ImpactSurface.h"
#include "Weapons/VfxTypes.h"
#include "Weapons/WeaponFX.h"
#include "World/CompanyLoudspeaker.h"
#include "World/IntakeCharge.h"
#include "World/LamplighterEffects.h"
#include "Components/AudioComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Misc/PackageName.h"
#include "UObject/ConstructorHelpers.h"

const FName ASinkIntake::IntakeTag(TEXT("Intake_Sink"));
const FName ASinkIntake::ChokedEvent(TEXT("Intake.Choked"));

namespace
{
	const TCHAR* IntakeCylinderPath = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
	const TCHAR* IntakePoolPath = TEXT("/Game/Art/Props/SM_SlagSplatA.SM_SlagSplatA");
	const TCHAR* IntakePaintPath = TEXT("/Game/Art/Materials/MI_PaintBlack.MI_PaintBlack");

	/** The blast: its radius, its colour (salt-white with a cyan edge), its shake. (The draw's, the knock's and the pool's
	 * numbers are SinkIntakeDraw.cpp's.) */
	constexpr float BlastRadius = 700.f;
	const FLinearColor BlastColor(0.85f, 0.95f, 1.f);
	constexpr float BlastShake = 0.7f;
	constexpr float BlastShakeSeconds = 1.3f;
	constexpr float BlastShakeReach = 6000.f;

	template <typename T>
	T* IntakeAssetIfMade(const TCHAR* ObjectPath)
	{
		if (!FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(FString(ObjectPath))))
		{
			return nullptr;
		}
		ConstructorHelpers::FObjectFinder<T> Finder(ObjectPath);
		return Finder.Object;
	}

	UInstancedStaticMeshComponent* MakeHoseLine(AActor& Owner, USceneComponent* Parent, const TCHAR* Name, UStaticMesh* Mesh)
	{
		UInstancedStaticMeshComponent* Line = Owner.CreateDefaultSubobject<UInstancedStaticMeshComponent>(Name);
		Line->SetupAttachment(Parent);
		Line->SetMobility(EComponentMobility::Movable);
		Line->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Line->SetCanEverAffectNavigation(false);
		Line->SetStaticMesh(Mesh);
		return Line;
	}
}

ASinkIntake::ASinkIntake()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	Tags.Add(IntakeTag);

	static UStaticMesh* const Cylinder = IntakeAssetIfMade<UStaticMesh>(IntakeCylinderPath);
	static UStaticMesh* const Splat = IntakeAssetIfMade<UStaticMesh>(IntakePoolPath);
	static UMaterialInterface* const Paint = IntakeAssetIfMade<UMaterialInterface>(IntakePaintPath);

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;

	Sections = MakeHoseLine(*this, Root, TEXT("Sections"), Cylinder);
	SecondSections = MakeHoseLine(*this, Root, TEXT("SecondSections"), Cylinder);

	// The pool: the slag splat in black paint on every slot (its glowing slots would read as lava), glossy-dark.
	Pool = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Pool"));
	Pool->SetupAttachment(Root);
	Pool->SetMobility(EComponentMobility::Movable);
	Pool->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Pool->SetCanEverAffectNavigation(false);
	Pool->SetCastShadow(false);
	Pool->SetStaticMesh(Splat);
	for (int32 Slot = 0; Paint && Splat && Slot < Splat->GetStaticMaterials().Num(); ++Slot)
	{
		Pool->SetMaterial(Slot, Paint);
	}
	Pool->SetVisibility(false);
}

void ASinkIntake::BeginPlay()
{
	Super::BeginPlay();
	if (HoseMesh)
	{
		Sections->SetStaticMesh(HoseMesh);
		SecondSections->SetStaticMesh(HoseMesh);
	}
	if (HoseMaterial)
	{
		Sections->SetMaterial(0, HoseMaterial);
		SecondSections->SetMaterial(0, HoseMaterial);
	}
	if (!PoolAt.IsNearlyZero())
	{
		Pool->SetWorldLocation(PoolAt);
		Pool->SetWorldScale3D(FVector(PoolRadius / PoolMeshRadius, PoolRadius / PoolMeshRadius, 1.0));
	}
	if (UMissionRunner* Runner = UMissionRunner::Get(this))
	{
		BoundRunner = Runner;
		ChangedHandle = Runner->OnChanged.AddUObject(this, &ASinkIntake::HandleRunnerChanged);
	}
	ChargesSet = 0;
	State = EIntakeState::Drinking;
	RefreshStory();
}

void ASinkIntake::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UMissionRunner* Runner = BoundRunner.Get())
	{
		Runner->OnChanged.Remove(ChangedHandle);
	}
	BoundRunner.Reset();
	ChangedHandle.Reset();
	LooterSound::Stop(DrawSound.Get(), 0.f);
	DrawSound.Reset();
	Super::EndPlay(EndPlayReason);
}

void ASinkIntake::HandleRunnerChanged()
{
	RefreshStory();
}

bool ASinkIntake::AnyHolds(const TArray<FStoryCondition>& Conditions) const
{
	const UMissionRunner* Runner = UMissionRunner::Get(this);
	if (!Runner)
	{
		return false;
	}
	const FCampaignRecord& Campaign = Runner->GetCampaign();
	return Conditions.ContainsByPredicate([&Campaign, Runner](const FStoryCondition& When) { return When.IsMet(Campaign, Runner); });
}

void ASinkIntake::RefreshStory()
{
	if (AnyHolds(GoneWhen))
	{
		SetState(EIntakeState::Gone);
	}
	else if (AnyHolds(ChokedWhen) && State != EIntakeState::Swinging)
	{
		// Its story past the choke (a load after it): limp from the start, the pool long drained.
		if (State != EIntakeState::Limp)
		{
			PoolLeft = 0.f;
			bGrowled = true;
			SetState(EIntakeState::Limp);
		}
	}
	RefreshSecondHose();
	SetActorTickEnabled(IsDrawing() || State == EIntakeState::Fused || State == EIntakeState::Swinging || bSecondDropping
		|| (SinceChoke >= 0.f && !bGrowled));
	if (!IsDrawing())
	{
		LooterSound::Stop(DrawSound.Get(), 0.4f);
		DrawSound.Reset();
	}
	PlaceHoses();
}

void ASinkIntake::SetState(EIntakeState NewState)
{
	State = NewState;
	Clock = 0.f;
	const bool bGone = State == EIntakeState::Gone;
	SetActorHiddenInGame(bGone);
	// Landfall's hose shows while the intake drinks; this one's from the choke on.
	ShowRestHose(!bGone && !DrawsHose());
	Pool->SetVisibility(!bGone && !PoolAt.IsNearlyZero() && PoolLeft > 0.f);
	PlaceHoses();
}

bool ASinkIntake::IsDrawing() const
{
	if (State == EIntakeState::Fused)
	{
		return true;
	}
	if (State != EIntakeState::Drinking || DrawWhen.IsEmpty())
	{
		return false;
	}
	const UMissionRunner* Runner = UMissionRunner::Get(this);
	return Runner && DrawWhen.IsMet(Runner->GetCampaign(), Runner);
}

float ASinkIntake::GetDrawOn(float Distance) const
{
	return IsDrawing() ? SinkIntakeRules::DrawInput(Distance, DrawRadius, DrainRadius, State == EIntakeState::Fused ? FuseDrawScale : 1.f) : 0.f;
}

// ---------------------------------------------------------------------------
// The charges and the fuse
// ---------------------------------------------------------------------------

void ASinkIntake::NoteChargeSet(AIntakeCharge& Charge)
{
	if (State != EIntakeState::Drinking)
	{
		return;
	}
	++ChargesSet;
	UE_LOG(LogLooter, Log, TEXT("%s: charge %d of 3 on the collar."), *GetActorNameOrLabel(), ChargesSet);
	if (ChargesSet == 1)
	{
		// The first clang wakes the ramp's spiders; Vane hears it on the barge.
		TriggerSpawner(SpiderSpawnerId);
		Broadcast(ChargeLines);
	}
	else if (ChargesSet == 2)
	{
		TriggerSpawner(DropSpawnerId);
	}
	else if (ChargesSet >= 3)
	{
		// The fuse: the charges hiss, the draw doubles, Hob shouts to get clear.
		for (AIntakeCharge* Each : FindCharges())
		{
			Each->LightFuse();
		}
		LooterSound::PlayAt(this, LooterSoundCue::Foreman::FuseLight, Charge.GetActorLocation(), 1.f, 1.f);
		Say(FuseLines);
		SetState(EIntakeState::Fused);
		SetActorTickEnabled(true);
	}
}

bool ASinkIntake::Choke()
{
	if (State != EIntakeState::Drinking && State != EIntakeState::Fused)
	{
		return false;
	}
	for (AIntakeCharge* Each : FindCharges())
	{
		Each->Blow();
	}
	Blast();
	Knocked.Reset();
	SinceChoke = 0.f;
	bGrowled = false;
	PoolLeft = 1.f;
	// Landfall's hose goes, and this one, in its very shape, bucks and swings.
	SetState(EIntakeState::Swinging);
	LooterSound::Stop(DrawSound.Get(), 0.1f);
	DrawSound.Reset();
	Broadcast(ChokedLines);
	if (UMissionRunner* Runner = UMissionRunner::Get(this))
	{
		Runner->NotifyEvent(FMissionEvent::Named(ChokedEvent, this));
	}
	SetActorTickEnabled(true);
	UE_LOG(LogLooter, Log, TEXT("%s: choked; the hose swings loose."), *GetActorNameOrLabel());
	return true;
}

void ASinkIntake::Blast()
{
	const FVector Mouth = GetActorLocation() + FVector(0.0, 0.0, 120.0);
	LooterSound::PlayAt(this, LooterSoundCue::Foreman::FuseBurst, Mouth, 1.3f, 0.85f);
	LooterSound::PlayAt(this, LooterSoundCue::DredgerTankBurst, Mouth, 1.f, 0.7f);
	BossCameraShake::Kick(this, Mouth, BlastShake, BlastShakeSeconds, BlastShakeReach);
	if (UBulletSubsystem* Bullets = GetWorld() ? GetWorld()->GetSubsystem<UBulletSubsystem>() : nullptr)
	{
		FWeaponFX& Effects = Bullets->GetEffects();
		Effects.Initialize(GetWorld());
		FVfxExplosion Burst;
		Burst.Center = Mouth;
		Burst.Up = FVector::UpVector;
		Burst.Radius = BlastRadius;
		Burst.Fire = BlastColor;
		Burst.bSoul = true;
		Burst.Ground = EImpactMaterial::Unknown;
		Burst.bOnGround = true;
		Effects.SpawnExplosion(Burst);
		Effects.SpawnShockRing(Mouth, FVector::UpVector, 40.f, BlastRadius * 1.4f, 24.f, BlastColor, 6.f, 0.5f);
	}
	// What it was drinking coughs up out of its mouth: a black cloud; the barge's dying stacks cough their last.
	for (int32 Puff = 0; Puff < 12; ++Puff)
	{
		const FVector Drift(FMath::FRandRange(-160.f, 160.f), FMath::FRandRange(-160.f, 160.f), FMath::FRandRange(140.f, 320.f));
		LamplighterEffects::SmokePuff(this, Mouth + FVector(FMath::FRandRange(-120.f, 120.f), FMath::FRandRange(-120.f, 120.f), 0.0), Drift);
	}
	for (const FVector& Stack : DyingStacks)
	{
		for (int32 Puff = 0; Puff < 8; ++Puff)
		{
			LamplighterEffects::SmokePuff(this, Stack + FVector(0.0, 0.0, Puff * 60.0),
				FVector(FMath::FRandRange(-80.f, 80.f), FMath::FRandRange(-80.f, 80.f), FMath::FRandRange(200.f, 420.f)));
		}
	}
	// The barge groans overhead as it loses its draw.
	if (!HoseTop.IsNearlyZero())
	{
		LooterSound::PlayAt(this, LooterSoundCue::ShipLurch, HoseTop, 1.f, 0.6f);
	}
}

// ---------------------------------------------------------------------------
// Moving on
// ---------------------------------------------------------------------------

void ASinkIntake::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Advance(DeltaSeconds);
}

void ASinkIntake::Advance(float DeltaSeconds)
{
	Clock += DeltaSeconds;
	if (IsDrawing())
	{
		TickDraw(DeltaSeconds);
	}
	switch (State)
	{
	case EIntakeState::Fused:
		if (Clock >= FuseSeconds)
		{
			Choke();
		}
		break;
	case EIntakeState::Swinging:
		PlaceHoses();
		if (Clock >= BuckSeconds)
		{
			TickKnocks();
		}
		if (Clock >= BuckSeconds + SwingSeconds)
		{
			// Hanging limp: the hose lies still on the floor.
			LooterSound::PlayAt(this, LooterSoundCue::ImpactMetal, LimpRest, 1.f, 0.5f);
			LamplighterEffects::DustSkirt(this, LimpRest, 300.f, 6);
			SetState(EIntakeState::Limp);
		}
		break;
	default:
		break;
	}
	if (SinceChoke >= 0.f && !bGrowled)
	{
		TickPool(DeltaSeconds);
	}
	if (bSecondDropping)
	{
		SecondShare = FMath::Min(SecondShare + DeltaSeconds / FMath::Max(SecondDropSeconds, 0.1f), 1.f);
		PlaceHoses();
		if (SecondShare >= 1.f)
		{
			bSecondDropping = false;
			LooterSound::PlayAt(this, LooterSoundCue::GrappleHit, SecondHoseTo, 1.f, 0.6f);
		}
	}
	if (!IsDrawing() && State != EIntakeState::Fused && State != EIntakeState::Swinging && !bSecondDropping
		&& (SinceChoke < 0.f || bGrowled))
	{
		SetActorTickEnabled(false);
	}
}

// (GetPlayer, the draw, the swing's knocks and the pool: SinkIntakeDraw.cpp.)

// ---------------------------------------------------------------------------
// Talk, spawners, things it finds
// ---------------------------------------------------------------------------

void ASinkIntake::Broadcast(const UStoryLineSet* Lines) const
{
	if (!Lines || Lines->Lines.IsEmpty() || LoudspeakerTag.IsNone())
	{
		return;
	}
	if (ACompanyLoudspeaker* Horn = ACompanyLoudspeaker::FindTagged(this, LoudspeakerTag))
	{
		Horn->Broadcast(Lines->Lines);
	}
}

void ASinkIntake::Say(const UStoryLineSet* Lines) const
{
	if (!Lines || Lines->Lines.IsEmpty())
	{
		return;
	}
	if (UCaptionSubsystem* Captions = UCaptionSubsystem::Get(this))
	{
		Captions->Play(Lines->Lines, ECaptionPlay::Queue);
	}
}

void ASinkIntake::TriggerSpawner(FName SpawnerId) const
{
	if (SpawnerId.IsNone())
	{
		return;
	}
	UEncounterSubsystem* Encounters = UEncounterSubsystem::Get(this);
	AEncounterSpawner* Spawner = Encounters ? Encounters->FindSpawner(SpawnerId) : nullptr;
	if (!Spawner)
	{
		UE_LOG(LogLooter, Warning, TEXT("%s: no encounter %s to bring."), *GetActorNameOrLabel(), *SpawnerId.ToString());
		return;
	}
	// The second drop: a spawner the player never came near still brings its first first.
	if (!Spawner->TriggerWave())
	{
		UE_LOG(LogLooter, Log, TEXT("%s: %s brings no wave (%s)."), *GetActorNameOrLabel(), *SpawnerId.ToString(), *Spawner->Describe());
	}
}

TArray<AIntakeCharge*> ASinkIntake::FindCharges() const
{
	TArray<AIntakeCharge*> Found;
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<AIntakeCharge> It(World); It; ++It)
		{
			if (It->IntakeTag.IsNone() || Tags.Contains(It->IntakeTag))
			{
				Found.Add(*It);
			}
		}
	}
	return Found;
}

void ASinkIntake::ShowRestHose(bool bShown) const
{
	UWorld* World = GetWorld();
	if (!World || RestHoseTag.IsNone())
	{
		return;
	}
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (It->ActorHasTag(RestHoseTag))
		{
			// For this visit: the props' own story hides them from the next step on, and shows them again on a later load.
			if (!bShown)
			{
				It->SetActorHiddenInGame(true);
			}
		}
	}
}

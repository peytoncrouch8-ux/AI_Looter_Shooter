// UAbelCorruptionComponent: its life on Abel, the hits that meet its parts (the drink line's collar, the black lantern, the
// locked bar's sparks) and his words. His look and smog are AbelCorruptionLook.cpp's, the call-back
// AbelCorruptionCallBack.cpp's, the tendrils AbelCorruptionTendril.cpp's (their look AbelCorruptionLines.cpp's), the six dead
// AbelCorruptionDrink.cpp's and the fog surge AbelCorruptionSurge.cpp's.

#include "Bosses/AbelCorruption.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Bosses/AbelKeeper.h"
#include "Bosses/BossComponent.h"
#include "Combat/BulletSubsystem.h"
#include "Combat/HealthComponent.h"
#include "Creatures/DredgerLook.h"
#include "Creatures/UnpaidCreature.h"
#include "Story/CaptionSubsystem.h"
#include "Weapons/WeaponFX.h"
#include "World/KeeperLanternPost.h"
#include "World/LamplighterRules.h"
#include "Components/CapsuleComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

namespace
{
	/** Bullets' trace channel (DefaultEngine.ini's Weapon): the collar and the lantern's sphere block it and nothing else. */
	constexpr ECollisionChannel CorruptionWeaponChannel = ECC_GameTraceChannel2;

	/** The smog's soot-violet, for his drink line's colour (the puffs off him are AbelCorruptionLook.cpp's). */
	const FLinearColor SmogColor(0.035f, 0.025f, 0.05f);

	/** The locked bar's sparks on a hit: a ricochet's size, and its tink's volume. */
	constexpr float LockedSparkScale = 0.7f;

	/** The black lantern's hit sphere (cm), and the collar's capsule round where the line meets the body. */
	constexpr float LanternHitRadius = 26.f;
	constexpr float CollarRadius = 46.f;
	constexpr float CollarHalfHeight = 70.f;
}

UAbelCorruptionComponent::UAbelCorruptionComponent()
{
	// His frame drives it (AAbelKeeper::Tick): the tests tick him by hand, and it must move with his moments, not after them.
	PrimaryComponentTick.bCanEverTick = false;
	Tendrils.SetNum(2);
}

AAbelKeeper* UAbelCorruptionComponent::GetAbel() const
{
	return Cast<AAbelKeeper>(GetOwner());
}

UBossComponent* UAbelCorruptionComponent::GetBoss() const
{
	const AAbelKeeper* Abel = GetAbel();
	return Abel ? Abel->GetBoss() : nullptr;
}

APawn* UAbelCorruptionComponent::GetPlayer() const
{
	if (const UBossComponent* Boss = GetBoss())
	{
		if (APawn* Fighting = Boss->GetFightPlayer())
		{
			return Fighting;
		}
	}
	// At zero his boss's fight is won and has let go of its player, but the last call (its tendril) is still against them.
	if (APawn* Fought = FoughtPlayer.Get())
	{
		return Fought;
	}
	const UWorld* World = GetWorld();
	const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
	return Controller ? Controller->GetPawn() : nullptr;
}

bool UAbelCorruptionComponent::IsRematch() const
{
	const UBossComponent* Boss = GetBoss();
	return Boss && Boss->IsRematch();
}

void UAbelCorruptionComponent::BeginPlay()
{
	Super::BeginPlay();
	if (UHealthComponent* Health = GetOwner() ? GetOwner()->FindComponentByClass<UHealthComponent>() : nullptr)
	{
		Health->OnDamaged.AddDynamic(this, &UAbelCorruptionComponent::HandleDamaged);
	}
	MakeParts();
	Drunk.Init(false, BoardsDead.Num());
}

void UAbelCorruptionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	BreakTendrils(AbelCorruptionRules::EBreak::OwnerDown, /*bQuiet*/ true);
	StopDrink(false);
	// The level's fog given back at once: a level that goes on (the console, a build) never keeps the surge.
	bSurgeWanted = false;
	SurgeShare = 0.f;
	TickSurge(0.f);
	if (AKeeperLanternPost* Post = BoundPost.Get())
	{
		Post->OnCalledBack.RemoveAll(this);
	}
	if (UStaticMeshComponent* Falling = FallingLantern.Get())
	{
		Falling->DestroyComponent();
	}
	Super::EndPlay(EndPlayReason);
}

void UAbelCorruptionComponent::MakeParts()
{
	AActor* Owner = GetOwner();
	USceneComponent* Root = Owner ? Owner->GetRootComponent() : nullptr;
	if (!Root || Lines)
	{
		return;
	}
	// The lines: engine cylinders in the world's frame on one instanced mesh, soot-black rubber of smog, no shadow.
	Lines = NewObject<UInstancedStaticMeshComponent>(Owner, TEXT("SmogLines"), RF_Transient);
	Lines->SetupAttachment(Root);
	Lines->SetUsingAbsoluteLocation(true);
	Lines->SetUsingAbsoluteRotation(true);
	Lines->SetUsingAbsoluteScale(true);
	Lines->SetWorldTransform(FTransform::Identity);
	Lines->SetMobility(EComponentMobility::Movable);
	Lines->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, DredgerLook::CylinderPath));
	if (UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, DredgerLook::SurfacePath))
	{
		LineLook = UMaterialInstanceDynamic::Create(Base, this);
		LineLook->SetVectorParameterValue(TEXT("Color"), SmogColor);
		LineLook->SetScalarParameterValue(TEXT("Glow"), 0.f);
		LineLook->SetScalarParameterValue(TEXT("Variation"), 0.1f);
		Lines->SetMaterial(0, LineLook);
	}
	Lines->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Lines->SetCastShadow(false);
	Lines->SetCanEverAffectNavigation(false);
	Lines->bReceivesDecals = false;
	Lines->RegisterComponent();

	// What shots meet: on the Weapon channel only (the player walks through, nothing stands on them). Off until wanted.
	auto ForShots = [](UPrimitiveComponent* Part)
	{
		Part->SetCollisionObjectType(ECC_WorldDynamic);
		Part->SetCollisionResponseToAllChannels(ECR_Ignore);
		Part->SetCollisionResponseToChannel(CorruptionWeaponChannel, ECR_Block);
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->SetGenerateOverlapEvents(false);
		Part->SetCanEverAffectNavigation(false);
		Part->SetHiddenInGame(true);
	};
	CollarHit = NewObject<UCapsuleComponent>(Owner, TEXT("SmogCollar"), RF_Transient);
	CollarHit->SetupAttachment(Root);
	CollarHit->SetUsingAbsoluteLocation(true);
	CollarHit->SetUsingAbsoluteRotation(true);
	CollarHit->SetUsingAbsoluteScale(true);
	CollarHit->InitCapsuleSize(CollarRadius, CollarHalfHeight);
	ForShots(CollarHit);
	CollarHit->RegisterComponent();

	AAbelKeeper* Abel = GetAbel();
	LanternHit = NewObject<USphereComponent>(Owner, TEXT("BlackLanternHit"), RF_Transient);
	if (Abel && Abel->Lantern)
	{
		LanternHit->SetupAttachment(Abel->Lantern.Get());
	}
	else
	{
		LanternHit->SetupAttachment(Root);
	}
	LanternHit->InitSphereRadius(LanternHitRadius);
	ForShots(LanternHit);
	LanternHit->RegisterComponent();
}

// ---------------------------------------------------------------------------
// His frame
// ---------------------------------------------------------------------------

void UAbelCorruptionComponent::Advance(float DeltaSeconds)
{
	if (!bLookMade)
	{
		// On his first frame, not at BeginPlay: the Unpaid puts his clothes on after the components begin, which would
		// replace dynamic instances made earlier.
		bLookMade = true;
		MakeLook();
		ApplyLook(true);
	}
	LineClock += DeltaSeconds;
	TickCallBack(DeltaSeconds);
	TickFinalCall(DeltaSeconds);
	TickTendrils(DeltaSeconds);
	TickDrink(DeltaSeconds);
	TickSurge(DeltaSeconds);
	TickFallingLantern(DeltaSeconds);
	// The black lantern's hit sphere is there only while it flares: the rest of the time his lantern arm guards his coal.
	if (LanternHit)
	{
		const bool bWanted = IsLanternFlaring();
		const ECollisionEnabled::Type Want = bWanted ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision;
		if (LanternHit->GetCollisionEnabled() != Want)
		{
			LanternHit->SetCollisionEnabled(Want);
		}
		if (AAbelKeeper* Abel = GetAbel())
		{
			LanternHit->SetWorldLocation(Abel->GetLanternGlobe());
		}
	}
	if (!bSceneHoldsLook)
	{
		// The smog's echo (a rematch) is all smog, whatever the real Abel's freeing left.
		const bool bClean = bFreed && !IsRematch();
		CorruptionShown = FMath::FInterpConstantTo(CorruptionShown, bClean ? 0.f : AbelCorruptionRules::CorruptionFor(Pips), DeltaSeconds, 0.8f);
	}
	ApplyLook(false);
	EmitSmog(DeltaSeconds);
	DrawLines(DeltaSeconds);
	UpdatePips();
}

void UAbelCorruptionComponent::HandleFightStarted()
{
	// A fight from the first phase: the smog all on him again (a rematch, or one started over), nothing owed.
	const UBossComponent* Boss = GetBoss();
	FoughtPlayer = Boss ? Boss->GetFightPlayer() : nullptr;
	bLocked = false;
	bKneeling = false;
	LocksOwed = 0;
	RetryIn = 0.f;
	bHintSaid = false;
	bSixSent = false;
	bFinalCall = false;
	if (!bFreed || IsRematch())
	{
		Pips = 0;
	}
	if (IsRematch())
	{
		// The echo comes up smog-black at once, its black lantern in its fist.
		CorruptionShown = 1.f;
		bSceneHoldsLook = false;
		ApplyLook(true);
	}
	Drunk.Init(false, BoardsDead.Num());
	Drinking = INDEX_NONE;
	LastDrunkTried = INDEX_NONE;
	DrinkRestLeft = 0.f;
}

void UAbelCorruptionComponent::HandleFightReset()
{
	// The player fell: everything of this fight goes, the six dead back on their biers, the smog back on him, the fog thin.
	// (Whoever comes back is the first controller's pawn again, maybe a new one: the next fight's start names them.)
	FoughtPlayer.Reset();
	BreakTendrils(AbelCorruptionRules::EBreak::OwnerDown, /*bQuiet*/ true);
	StopDrink(false);
	RestoreDead();
	if (bKneeling)
	{
		EndKneel(false);
	}
	bLocked = false;
	LocksOwed = 0;
	RetryIn = 0.f;
	bFinalCall = false;
	Pips = 0;
	CorruptionShown = 1.f;
	SetSurge(false);
	if (AKeeperLanternPost* Post = BindKeepersPost())
	{
		Post->CloseCallBack();
		Post->SetCallGlow(0.f);
	}
	ApplyLook(true);
}

void UAbelCorruptionComponent::HandleFightWon()
{
	// At zero: the lock is over (his boss's spells went with the fight); the tendrils and the line die with the fight. The
	// last call follows his kneel (AAbelKeeper::PlayEnding).
	BreakTendrils(AbelCorruptionRules::EBreak::OwnerDown, /*bQuiet*/ false);
	StopDrink(false);
	if (bKneeling)
	{
		EndKneel(false);
	}
	bLocked = false;
	LocksOwed = 0;
	SendSixSaved();
	SetSurge(false);
}

void UAbelCorruptionComponent::HandleStaggered()
{
	for (FTendril& Tendril : Tendrils)
	{
		if (Tendril.State == FTendril::EState::Latched && !Tendril.bFinal)
		{
			LetGo(Tendril, AbelCorruptionRules::EBreak::Staggered, false);
		}
	}
}

void UAbelCorruptionComponent::HandleHidden()
{
	BreakTendrils(AbelCorruptionRules::EBreak::OwnerDown, /*bQuiet*/ true);
	StopDrink(false);
	if (Lines && Lines->GetInstanceCount() > 0)
	{
		Lines->ClearInstances();
	}
	if (CollarHit)
	{
		CollarHit->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

// ---------------------------------------------------------------------------
// Hits on him
// ---------------------------------------------------------------------------

void UAbelCorruptionComponent::HandleDamaged(float Damage, bool bCritical, FVector HitLocation, AController* InstigatedBy, AActor* DamageCauser)
{
	const AAbelKeeper* Abel = GetAbel();
	const UHealthComponent* Health = Abel ? Abel->FindComponentByClass<UHealthComponent>() : nullptr;
	const UPrimitiveComponent* Part = Health ? Health->GetLastHitComponent() : nullptr;
	// The drink line's collar, out on the boards while he hangs in the fog: three hits break it.
	if (Part && Part == CollarHit && Drinking != INDEX_NONE)
	{
		HitCollar();
		return;
	}
	// The black lantern during its flare: the tendril it feeds lets go (or never leaves it).
	if (Part && Part == LanternHit && IsLanternFlaring())
	{
		ShootLantern();
		return;
	}
	// Locked for a call-back: the shots only spark off the smog, a hard tink. (The fog's spell is the grey bar's to say.)
	if (bLocked && Abel)
	{
		if (UBulletSubsystem* Bullets = Abel->GetWorld() ? Abel->GetWorld()->GetSubsystem<UBulletSubsystem>() : nullptr)
		{
			FWeaponFX& Effects = Bullets->GetEffects();
			Effects.Initialize(Abel->GetWorld());
			Effects.SpawnRicochet(HitLocation, (HitLocation - Abel->GetActorLocation()).GetSafeNormal(), LockedSparkScale);
		}
		LooterSound::PlayAt(Abel, LooterSoundCue::ImpactMetal, HitLocation, 0.6f, 1.3f);
	}
}

// ---------------------------------------------------------------------------
// His words
// ---------------------------------------------------------------------------

void UAbelCorruptionComponent::Say(FName SetName, const TArray<FStoryLine>& Fallback, int32 LineIndex, bool bInterrupt) const
{
	// The writer's set when it's made (create_story_lines.py), the code's draft until then.
	TArray<FStoryLine> Said = LandfallRules::LoadLines(SetName);
	if (Said.IsEmpty())
	{
		Said = Fallback;
	}
	if (LineIndex != INDEX_NONE)
	{
		if (Said.IsEmpty())
		{
			return;
		}
		Said = { Said[LineIndex % Said.Num()] };
	}
	if (UCaptionSubsystem* Captions = Said.IsEmpty() ? nullptr : UCaptionSubsystem::Get(GetOwner()))
	{
		Captions->Play(Said, bInterrupt ? ECaptionPlay::Interrupt : ECaptionPlay::Queue);
	}
}

void UAbelCorruptionComponent::Mutter()
{
	// The keeper in him, turned to the six dead: one fragment a time, in turn (the furnace's words between them).
	static const FText Abel = NSLOCTEXT("LooterAbel", "Abel", "Abel");
	const TArray<FStoryLine> Fallback = {
		FStoryLine::Make(Abel, NSLOCTEXT("LooterAbel", "Mutter0", "Dusk. It's dusk. Who's sitting with them?"), 2.8f),
		FStoryLine::Make(Abel, NSLOCTEXT("LooterAbel", "Mutter1", "Six on the boards. Somebody walk them."), 2.6f),
		FStoryLine::Make(Abel, NSLOCTEXT("LooterAbel", "Mutter2", "Saint Ada's dark. Why is she dark?"), 2.6f),
	};
	Say(TEXT("DA_Lines_AbelMutter"), Fallback, MutterIndex++);
}

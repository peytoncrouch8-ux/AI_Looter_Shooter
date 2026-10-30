// AForemanCreature: construction (his size, rank, numbers and parts, his boss fight), his life in the stope, his engagement,
// his entrance and his coal's windows. ForemanCreatureRules.cpp has which move he makes (under the bosses' shared rules),
// his stamp and his hurt zones, ForemanCreatureEvents.cpp his boss's events, his death and his coming back,
// ForemanCreatureFuse.cpp the fuse sticks, ForemanCreatureRoof.cpp the roof strikes, ForemanCreatureShift.cpp the shift and
// the dark, ForemanCreaturePose.cpp his arms, his kneel and his death's look.

#include "Creatures/ForemanCreature.h"
#include "AI_Looter_Shooter.h"
#include "Audio/CreatureVoiceComponent.h"
#include "Audio/LooterSound.h"
#include "Bosses/BossCameraShake.h"
#include "Bosses/BossComponent.h"
#include "Combat/BulletSubsystem.h"
#include "Combat/HealthComponent.h"
#include "Creatures/EncounterSubsystem.h"
#include "Creatures/ForemanFuseStick.h"
#include "Creatures/ForemanRockFall.h"
#include "Creatures/ForemanTimberProp.h"
#include "Player/PlayerSize.h"
#include "Story/CaptionSubsystem.h"
#include "Weapons/WeaponFX.h"
#include "World/WorldQueries.h"
#include "AnimationRuntime.h"
#include "ReferenceSkeleton.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/HitResult.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"
#include "UObject/ConstructorHelpers.h"

const TCHAR* const AForemanCreature::ModelPath = TEXT("/Game/Art/Creatures/SK_Foreman.SK_Foreman");
const TCHAR* const AForemanCreature::BowlerPath = TEXT("/Game/Art/Creatures/SM_ForemanBowler.SM_ForemanBowler");
const TCHAR* const AForemanCreature::LampPath = TEXT("/Game/Art/Creatures/SM_ForemanLamp.SM_ForemanLamp");
const TCHAR* const AForemanCreature::PickHandlePath = TEXT("/Game/Art/Creatures/SM_ForemanPickHandle.SM_ForemanPickHandle");
const TCHAR* const AForemanCreature::SatchelPath = TEXT("/Game/Art/Creatures/SM_ForemanSatchel.SM_ForemanSatchel");
const FName AForemanCreature::BarkKind(TEXT("Foreman"));

namespace
{
	/** His coal: banked under his coat (M_Ghost's flare: -1 out), and open, throbbing. */
	constexpr float CoalClosedHeat = -0.55f;
	constexpr float CoalOpenHeat = 0.6f;
	constexpr float CoalThrob = 0.25f;

	/** Dust shaken off the roof by his entrance's slam: this many spots round him. */
	constexpr int32 IntroDustSpots = 7;

	/** He looks for a player come into his stope this often (s). */
	constexpr float EngageEvery = 0.25f;

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

AForemanCreature::AForemanCreature()
{
	// His name stands for his rank's word on his tag: "The Foreman" in orange.
	DisplayName = NSLOCTEXT("LooterForeman", "ForemanName", "The Foreman");
	bNameIsRankWord = true;
	StartingRank = ECreatureRank::Legendary;
	// 1.5 times an Unpaid as seen, his rank's own growth (x1.4) included.
	BodyScale = Foreman::Size / Foreman::RankSize;
	// Soulfed's twelve times this: about 7,000 at level 6, about 30 s on target (380 since 2026-10-30: at 190 the story's
	// boss had 3,500, under half the optional Gravemother's; he missed the 10-27 x1.6 that made the others last).
	Health->MaxHealth = 380.f;
	// The pick handle (Soulfed's 1.6 on top: about 24 a blow at level 7); his fuse and the roof hit harder.
	AttackDamage = 10.f;
	XPReward = 11;
	// A heavy man who doesn't hurry: his fuses reach a player who keeps away.
	WalkSpeed = 120.f;
	ChaseSpeed = 340.f * LooterPlayerSize::MoveScale;
	AttackRange = 240.f;
	AttackWindup = 0.75f;
	AttackRecovery = 0.8f;
	AttackCooldown = 0.6f;
	Tuning.SwingReach = 260.f;
	// He kneels, says his last word and fades (UpdateDeathLook) before his body goes.
	CorpseTime = 3.5f;
	// He walks his stope: he steps up only when stuck.
	PhaseStep.FarBehind = 1.0e6f;
	PhaseStep.Cooldown = 6.f;
	Voice->BarkKind = BarkKind;
	// His carbide lamp lights him and round him whatever the miners' budget, brighter and wider; it's no weak spot.
	bLampIsWeakSpot = false;
	LampBrightness = Foreman::OwnLampBrightness;
	Tuning.LampCandela = 8.f;
	Tuning.FlareCandela = 16.f;
	Tuning.LampRadius = 650.f;
	Tuning.LampGlowSize = 11.f;

	// His own body and things once made (the dead miners' meanwhile).
	static USkeletalMesh* const ForemanModel = FindIfMade<USkeletalMesh>(ModelPath);
	static UStaticMesh* const BowlerModel = FindIfMade<UStaticMesh>(BowlerPath);
	static UStaticMesh* const LampModel = FindIfMade<UStaticMesh>(LampPath);
	static UStaticMesh* const PickHandleModel = FindIfMade<UStaticMesh>(PickHandlePath);
	static UStaticMesh* const SatchelModel = FindIfMade<UStaticMesh>(SatchelPath);
	USkeletalMeshComponent* Body = GetMesh();
	if (ForemanModel)
	{
		Body->SetSkeletalMeshAsset(ForemanModel);
		// His coat is his model's own: no tints.
		OtherClothes.Reset();
		Clothes = 0;
	}
	if (BowlerModel)
	{
		GetHat()->SetStaticMesh(BowlerModel);
	}
	if (LampModel)
	{
		Lamp->SetStaticMesh(LampModel);
	}
	if (PickHandleModel)
	{
		Pick->SetStaticMesh(PickHandleModel);
	}
	// His things where SK_Foreman holds them (Intermediate/ForemanModel/Foreman_parts.json): the handle on his pick bone (or
	// off his right hand), the lamp on the bowler (its SOCKET_Lamp in play; on the hat bone without it).
	const USkeletalMesh* Model = Body->GetSkeletalMeshAsset();
	PickFromHand = FVector(4.16f, -3.11f, -8.06f);
	PickTurn = FQuat(0.04972f, 0.49584f, -0.08651f, 0.86266f);
	LampOnHat = FVector(10.04f, -0.16f, 5.23f);
	LampTurnOnHat = FQuat(0.01744f, -0.00873f, 0.00076f, 0.99981f);
	FName PickOn;
	FTransform Held;
	HoldPick(Model, PickOn, Held);
	Pick->SetupAttachment(Body, PickOn);
	Pick->SetRelativeTransform(Held);
	if (Model && Model->GetRefSkeleton().FindBoneIndex(Rig.Hat) != INDEX_NONE)
	{
		Lamp->SetRelativeTransform(PartOnBone(Model, Rig.Hat, LampOnHat, LampTurnOnHat));
	}
	// His satchel on his chest bone, modelled in place with its pivot at the bone's head.
	Satchel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Satchel"));
	Satchel->SetupAttachment(Body, Rig.Chest);
	Satchel->SetRelativeTransform(PartOnBone(Model, Rig.Chest, FVector::ZeroVector, FQuat::Identity));
	Satchel->SetStaticMesh(SatchelModel);
	Satchel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Satchel->SetGenerateOverlapEvents(false);
	Satchel->SetCanEverAffectNavigation(false);
	Satchel->SetCastShadow(false);

	// His boss fight. He digs at the fall, waiting, until a player comes into his stope or shoots him; then his bar sweeps
	// in as he turns and slams his pick handle down. No fog wall (a ring would cut through the rock; the level may place one
	// at the barricade: Seal); gone from his stope a while, the fight stands down and he goes back to the fall, healed. His
	// shift are his adds and crumble with him; his loot bursts out of him after his last word.
	Boss = CreateDefaultSubobject<UBossComponent>(TEXT("Boss"));
	Boss->BossName = NSLOCTEXT("LooterForeman", "ForemanBossName", "Cyrus Teague, the Foreman");
	Boss->Phases = ForemanFight::MakePhases();
	Boss->bWaitsPassive = true;
	Boss->bStartWhenHurt = true;
	Boss->bStartWhenHunting = false;
	// His own engagement (TickEngage: flat and on his level), not his boss's flat one: the bench lies over his stope.
	Boss->EngageRadius = 0.f;
	Boss->LeashRadius = 0.f;
	Boss->SealRadius = 0.f;
	Boss->StandDownSeconds = ForemanFight::StandDownSeconds;
	Boss->bAddsDieWithBoss = true;
	Boss->MaxAliveAdds = Foreman::ShiftSize * Foreman::NumShifts;
	Boss->Show = ForemanFight::MakeShow();
	Boss->Stagger = ForemanFight::MakeStagger();
	Boss->LootShower = ForemanFight::MakeLootShower();
}

void AForemanCreature::BeginPlay()
{
	// His satchel wears his look (his dissolve, his rank's ember), as the miner's things do.
	LookParts.AddUnique(Satchel);
	Super::BeginPlay();

	Satchel->AttachToComponent(GetMesh(), FAttachmentTransformRules::KeepRelativeTransform, Rig.Chest);
	// His own chase and swing as his class (and rank) gave them, which each phase's pace scales.
	if (!bOwnPaceCaptured)
	{
		bOwnPaceCaptured = true;
		OwnChase = ChaseSpeed;
		OwnSwingWindup = SwingWindup;
	}
	ApplyPace(0);
	Boss->OnFightStarted.AddUObject(this, &AForemanCreature::HandleFightStarted);
	Boss->OnFightReset.AddUObject(this, &AForemanCreature::HandleFightReset);
	Boss->OnPhaseChanged.AddUObject(this, &AForemanCreature::HandlePhaseChanged);
	Boss->OnStaggered.AddUObject(this, &AForemanCreature::HandleStaggered);
	Boss->CanStagger.BindWeakLambda(this, [this]() { return !IsDead(); });
	// His hurt zones for Looter.Boss.HurtZones (ForemanCreatureRules.cpp).
	Boss->GatherHurtZones.BindWeakLambda(this, [this](TArray<FBossHurtZone>& Out) { GatherHurtZones(Out); });
	// A new arrival, or his vigil stone's call: his stope's props stand whole again.
	for (AForemanTimberProp* Prop : GetProps(false))
	{
		Prop->Mend();
	}
}

void AForemanCreature::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearFightState(/*bMendProps*/ false);
	if (Boss)
	{
		Boss->CanStagger.Unbind();
		Boss->GatherHurtZones.Unbind();
		Boss->OnFightStarted.RemoveAll(this);
		Boss->OnFightReset.RemoveAll(this);
		Boss->OnPhaseChanged.RemoveAll(this);
		Boss->OnStaggered.RemoveAll(this);
	}
	Super::EndPlay(EndPlayReason);
}

void AForemanCreature::Tick(float DeltaSeconds)
{
	// The dead miner's frame (his brain, his work at the fall, his body and lamp), then his own moves' clocks.
	Super::Tick(DeltaSeconds);
	TickMoves(DeltaSeconds);
}

// ---------------------------------------------------------------------------
// His coal
// ---------------------------------------------------------------------------

bool AForemanCreature::IsCoalOpen() const
{
	return !IsDead() && ForemanFight::IsCoalOpen(IsLightingFuse(), SinceRoofStrike, RoofCoalSeconds, bStaggeredNow);
}

bool AForemanCreature::IsCriticalSpot(const FHitResult& Hit) const
{
	// His coat covers his coal but in its windows: then a shot through it is critical, as through any Unpaid's.
	return IsCoalOpen() && Super::IsCriticalSpot(Hit);
}

// ---------------------------------------------------------------------------
// His entrance
// ---------------------------------------------------------------------------

void AForemanCreature::BeginIntro()
{
	// He turns from the fall, the pick handle coming up over his head, and calls the shift to order.
	Move = EForemanMove::Intro;
	bIntroWanted = false;
	AttackWindup = IntroWindup;
	AttackRecovery = IntroRecovery;
	SetAct(EAct::None);
	Say(ForemanFight::IntroLine());
	LooterSound::PlayAttached(LooterSoundCue::DeadMiner::Heave, GetRootComponent(), NAME_None, 1.2f, 0.8f * Voice->GetPitch());
	UE_LOG(LogLooter, Log, TEXT("%s turns from the fall: his fight starts."), *GetName());
}

void AForemanCreature::IntroSlam()
{
	// The pick handle slammed on the stope floor: dirt thrown up, dust shaken off the roof all round, the view jolted.
	const float Scale = GetSizeScale();
	const FVector Slam = GroundUnder(GetActorLocation() + GetActorForwardVector() * (110.f * Scale));
	KickDirt(Slam, 1.f);
	for (int32 Index = 0; Index < 5; ++Index)
	{
		const FVector Out = FVector::ForwardVector.RotateAngleAxis(72.f * Index, FVector::UpVector);
		KickDirt(Slam + Out * (90.f * Scale), 0.6f);
	}
	ShakeRoofDust(IntroDustSpots);
	LooterSound::PlayAt(this, LooterSoundCue::Foreman::Slam, Slam);
	BossCameraShake::Kick(this, Slam, 0.6f, 0.8f);
}

// ---------------------------------------------------------------------------
// His moves' clocks and his coal's look
// ---------------------------------------------------------------------------

void AForemanCreature::TickEngage(float DeltaSeconds)
{
	if (!Boss || Boss->IsFighting() || Boss->IsWon() || IsDead() || EngageRadius <= 0.f)
	{
		return;
	}
	EngageCheck -= DeltaSeconds;
	if (EngageCheck > 0.f)
	{
		return;
	}
	EngageCheck = EngageEvery;
	// The player in his stope: near his spot seen from above, and on his level (not on the bench over the mine).
	const UEncounterSubsystem* Encounters = UEncounterSubsystem::Get(this);
	APawn* Player = Encounters ? Encounters->GetPlayer() : nullptr;
	if (!Player)
	{
		Player = UGameplayStatics::GetPlayerPawn(this, 0);
	}
	const FVector Spot = GetHome().GetLocation();
	if (Player && FVector::Dist2D(Player->GetActorLocation(), Spot) <= EngageRadius
		&& FMath::Abs(Player->GetActorLocation().Z - Spot.Z) <= EngageRise)
	{
		Boss->StartFight(Player);
	}
}

void AForemanCreature::TickMoves(float DeltaSeconds)
{
	TickEngage(DeltaSeconds);
	// His attacks under the bosses' shared rules: his log's clocks, a run of swings broken, a long idle stretch ended.
	TickAttackRules(DeltaSeconds);
	// Their cooldowns run while his fight is on.
	if (Boss && Boss->IsFighting() && !IsDead())
	{
		FuseCooldownLeft = FMath::Max(0.f, FuseCooldownLeft - DeltaSeconds);
		RoofCooldownLeft = FMath::Max(0.f, RoofCooldownLeft - DeltaSeconds);
	}
	SinceFuseLine += DeltaSeconds;
	if (SinceRoofStrike >= 0.f)
	{
		SinceRoofStrike += DeltaSeconds;
	}
	if (Move == EForemanMove::Roof && GetCreatureState() == ECreatureState::Attack && GetStateTime() < AttackWindup)
	{
		FaceStruckProp(DeltaSeconds);
	}
	// A fuse still in his fist when its move is over (broken off): it falls at his feet.
	if (HeldStick.IsValid() && (GetCreatureState() != ECreatureState::Attack || Move != EForemanMove::Fuse))
	{
		DropHeldStick(/*bPutOut*/ false);
	}
	// His coal: banked under the coat, or open and throbbing (the window to crit him), its own flare on a crit on top.
	CoalPulse += DeltaSeconds;
	if (!IsDead())
	{
		const float Wanted = IsCoalOpen() ? CoalOpenHeat + CoalThrob * FMath::Sin(CoalPulse * 9.f) : CoalClosedHeat;
		Heat = DeltaSeconds > 0.f ? FMath::FInterpTo(Heat, Wanted, DeltaSeconds, 10.f) : Wanted;
	}
	PruneShift();
	RockFalls.RemoveAll([](const TWeakObjectPtr<AForemanRockFall>& Each) { return !Each.IsValid(); });
	TickDark(DeltaSeconds);
}

void AForemanCreature::ApplyPace(int32 Phase)
{
	PacePhase = FMath::Clamp(Phase, 0, Foreman::LastShiftPhase);
	Pace = ForemanFight::PaceFor(PacePhase);
	if (!bOwnPaceCaptured)
	{
		return;
	}
	// Held still while he's staggered.
	ChaseSpeed = bStaggeredNow ? 0.f : OwnChase * Pace.ChaseScale;
	SwingWindup = OwnSwingWindup * Pace.SwingWindupScale;
}

void AForemanCreature::Say(const FStoryLine& Line) const
{
	if (Line.Text.IsEmpty())
	{
		return;
	}
	if (UCaptionSubsystem* Captions = UCaptionSubsystem::Get(this))
	{
		// His shouts land when he shouts them: they cut in over whatever else is being said.
		Captions->Play({ Line }, ECaptionPlay::Interrupt);
	}
}

void AForemanCreature::ShakeRoofDust(int32 Count) const
{
	UWorld* World = GetWorld();
	UBulletSubsystem* Bullets = World ? World->GetSubsystem<UBulletSubsystem>() : nullptr;
	if (!Bullets)
	{
		return;
	}
	FWeaponFX& Effects = Bullets->GetEffects();
	Effects.Initialize(World);
	const FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(World, TEXT("ForemanRoofDust"), this);
	const FVector Feet = GetFeet();
	for (int32 Index = 0; Index < Count; ++Index)
	{
		// Somewhere over the stope's floor round him: dust sifting off the roof there (or 6 m up without one).
		const FVector2D Offset = FMath::RandPointInCircle(ArenaRadius * 0.6f);
		const FVector Floor = Feet + FVector(Offset.X, Offset.Y, 0.0);
		FVector Roof = Floor + FVector(0.0, 0.0, AForemanRockFall::RoofFallback);
		FHitResult Hit;
		if (World->LineTraceSingleByObjectType(Hit, Floor + FVector(0.0, 0.0, 100.0), Floor + FVector(0.0, 0.0, 1200.0),
			FCollisionObjectQueryParams(ECC_WorldStatic), Params))
		{
			Roof = Hit.ImpactPoint - FVector(0.0, 0.0, 15.0);
		}
		Effects.SpawnDustPuff(Roof, FVector(0.0, 0.0, -260.0), FLinearColor(0.3f, 0.27f, 0.23f), 0.5f, 15.f, 90.f, 2.2f);
		Effects.SpawnGrit(Roof, FVector(0.0, 0.0, -150.0), 3.f, 2.f);
	}
}

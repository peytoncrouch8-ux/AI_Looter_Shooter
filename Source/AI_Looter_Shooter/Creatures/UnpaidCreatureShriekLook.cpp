// AUnpaidCreature's failed-lunge shriek, its look: the cry, rings, cone, rattle, rally, effects and light (flow: UnpaidCreatureShriek.cpp).

#include "Creatures/UnpaidKitWork.h"
#include "Creatures/UnpaidShriekWork.h"
#include "Creatures/UnpaidCreature.h"
#include "Audio/LooterSound.h"
#include "Combat/BulletSubsystem.h"
#include "Creatures/FightDirectorSubsystem.h"
#include "Creatures/UnpaidMoveRules.h"
#include "Creatures/UnpaidMoveTuning.h"
#include "Creatures/UnpaidPackSubsystem.h"
#include "Player/PlayerRattleSubsystem.h"
#include "Weapons/VfxTuning.h"
#include "Weapons/WeaponFX.h"
#include "World/WorldQueries.h"
#include "CollisionQueryParams.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

using namespace UnpaidShriekWork;

namespace
{
	/** The soul's cold light: its rings, its breath, its motes (the coal's colour runs through them). */
	const FLinearColor SoulCold(0.7f, 0.85f, 1.f);

	/** Its coal's light: reach (cm at size 1), candelas through the tell and the scream. 70/150 cd 35 cm off its pale chest
	 *  washed the body flat white in daylight (PIE 2026-10-30; lanterns are 6-14 cd). */
	constexpr float LightReach = 700.f;
	constexpr float TellCandelas = 25.f;
	constexpr float ScreamCandelas = 50.f;
	/** It sits this far out of the coal (cm at size 1) and this far up: on its chest, face and arms, and the ground before it. */
	constexpr float LightOut = 60.f;
	constexpr float LightUp = 15.f;
	/** The light's share of white through the tell / the scream (the rest the coal's colour). */
	constexpr float TellWhiten = 0.15f, ScreamWhiten = 0.25f;

	/** Particles a second (at Looter.Unpaid.Shriek.Fx 1): motes drawn in, the scream's cone at its start and its tail, embers, frost. */
	constexpr float MoteRate = 50.f;
	constexpr float ConeRate = 45.f;
	constexpr float ConeTail = 8.f;
	constexpr float EmberRate = 14.f;
	constexpr float FrostRate = 7.f;
	constexpr float PantRate = 3.f;
	/** Never more than this many of a kind in one frame (a long frame doesn't dump a burst). */
	constexpr int32 PerFrame = 4;

	/** Its mouth sits this far in front of its head's pivot and this far under it (cm at size 1). */
	constexpr float MouthForward = 14.f;
	constexpr float MouthDrop = 5.f;
	/** The motes come from a shell this far round its head (cm at size 1); the cone carries this far. */
	constexpr float InhaleShell = 110.f;
	constexpr float ConeReach = 380.f;
	/** The cone stops this far short of its target's eyes (wisps grow to ~90 cm; carried through the camera from ~1.4 m off
	 *  it washed half the screen white, PIE 2026-10-30), never shorter than ConeMin. */
	constexpr float ConeClear = 140.f, ConeMin = 60.f;

	/** Felt beyond the rattle's reach: this share of a full rattle at the rattle's edge, falling to nothing at FeltRadius. */
	constexpr float FeltShare = 0.35f;
	/** Behind a wall from it: this share. */
	constexpr float CoveredShare = 0.6f;
	/** The ringing in their ears plays for a rattle this strong or more. */
	constexpr float RingingAt = 0.15f;

	float Ease(float Alpha)
	{
		return FMath::InterpEaseInOut(0.f, 1.f, FMath::Clamp(Alpha, 0.f, 1.f), 2.f);
	}

	/** Whole pieces owed this frame at Rate a second (the rest carried to the next). */
	int32 Owe(float& Owed, float Rate, float DeltaSeconds)
	{
		Owed += FMath::Max(Rate, 0.f) * DeltaSeconds;
		const int32 Count = FMath::Min(FMath::FloorToInt32(Owed), PerFrame);
		Owed = FMath::Min(Owed - Count, static_cast<float>(PerFrame));
		return Count;
	}

	/** How far its cone carries (cm): its full reach, cut to stop short of its target's eyes when they stand inside it. */
	float ConeReachTo(const AUnpaidCreature& Unpaid, const FVector& Mouth, float Full)
	{
		const APawn* Victim = Unpaid.GetTarget();
		if (!Victim)
		{
			return Full;
		}
		const float ToEyes = static_cast<float>(FVector::Dist(Mouth, Victim->GetPawnViewLocation()));
		return FMath::Clamp(ToEyes - ConeClear, FMath::Min(ConeMin, Full), Full);
	}
}

FWeaponFX* UnpaidShriekWork::ShriekEffects(UWorld* World)
{
	UBulletSubsystem* Bullets = World ? World->GetSubsystem<UBulletSubsystem>() : nullptr;
	if (!Bullets)
	{
		return nullptr;
	}
	FWeaponFX& Effects = Bullets->GetEffects();
	Effects.Initialize(World);
	return &Effects;
}

// ---------------------------------------------------------------------------
// The scream's moment, and each frame's show
// ---------------------------------------------------------------------------

FVector FUnpaidShriekWork::MouthOf(const AUnpaidCreature& Unpaid, FVector& OutWay)
{
	const float Scale = Unpaid.GetSizeScale();
	const FVector Forward = Unpaid.GetActorForwardVector();
	// Out along its facing, a little up while it rears (the tell), level as it thrusts its head at them (the scream).
	const float Up = Unpaid.Kit.Moves.Shriek == EUnpaidShriekPhase::Tell ? 0.45f : 0.08f;
	OutWay = (Forward + FVector(0.f, 0.f, Up)).GetSafeNormal();
	FVector Head;
	if (Unpaid.bRigReady && Unpaid.HeadBone != INDEX_NONE && Unpaid.Bones.IsValidIndex(Unpaid.HeadBone))
	{
		Head = Unpaid.GetMesh()->GetComponentTransform().TransformPosition(Unpaid.Bones[Unpaid.HeadBone].Posed.GetLocation());
	}
	else
	{
		Head = Unpaid.GetActorLocation() + FVector(0.f, 0.f, 0.75f * Unpaid.GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
	}
	return Head + Forward * (MouthForward * Scale) - FVector(0.f, 0.f, MouthDrop * Scale);
}

void FUnpaidShriekWork::Scream(AUnpaidCreature& Unpaid, const FUnpaidMoveTuning& Tuning)
{
	FUnpaidMoveState& State = Unpaid.Kit.Moves;
	State.bScreamed = true;
	UWorld* World = Unpaid.GetWorld();
	const float Scale = Unpaid.GetSizeScale();
	FVector Way;
	const FVector Mouth = MouthOf(Unpaid, Way);
	const FVector Feet = Unpaid.GetActorLocation() - FVector(0.f, 0.f, Unpaid.GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
	const FVector Coal = Unpaid.GetCoalLocation();
	const FLinearColor Tint = Unpaid.GetCoalColor();
	const FLinearColor RingColor = FMath::Lerp(SoulCold, Tint, 0.3f);
	LooterSound::PlayAttached(FName(Cue::Scream), Unpaid.GetMesh(), Unpaid.Rig.Head, 1.f, State.ShriekPitch);

	// The look: a shockwave ring rolling out along the ground (its wave lifts the dust, grass and leaves), a ring torn off its
	// mouth, a flash, the first blast of the cone, embers and frost. No ring at chest height: born round its chest, it read as a
	// flat grey collar (play-check 2026-10-30).
	FWeaponFX* Effects = Tuning.ShriekFx > 0.f ? ShriekEffects(World) : nullptr;
	const float Reach = Tuning.ShriekRattleRadius * 1.15f * Scale;
	if (Effects)
	{
		const float Glow = FVfxTuning::Live().Glow * Tuning.ShriekFx;
		Effects->SpawnShockRing(Feet + FVector(0.f, 0.f, 4.f), FVector::UpVector, 50.f * Scale, Reach, 48.f * Scale, RingColor, 12.f * Glow, 0.6f);
		Effects->SpawnShockRing(Mouth + Way * (10.f * Scale), Way, 8.f * Scale, 130.f * Scale, 9.f * Scale, SoulCold, 10.f * Glow, 0.22f);
		Effects->SpawnStarburst(Mouth + Way * (8.f * Scale), 50.f * Scale, FMath::Lerp(SoulCold, FLinearColor::White, 0.5f), 4.f * Glow, 0.12f, 9);
		Effects->SpawnShriekCone(Mouth, Way, ConeReachTo(Unpaid, Mouth, ConeReach * 1.1f * Scale), Tint, 12, 1.f);
		Effects->SpawnCoalFling(Coal, Unpaid.GetCoalFacing(), Tint, 10, true);
		Effects->SpawnFrostBreath(Mouth, Way, 55.f * Scale);
	}
	if (UUnpaidPackSubsystem* Packs = UUnpaidPackSubsystem::Get(&Unpaid))
	{
		Packs->StartWave(Feet, Reach, 0.6f, 1.f, &Unpaid);
	}

	// The players it rattles: near and in its sight, a sway and a slight slow; farther (or behind a wall) only felt.
	UPlayerRattleSubsystem* Rattles = UPlayerRattleSubsystem::Get(&Unpaid);
	TArray<APawn*, TInlineAllocator<2>> Players;
	if (World && Rattles)
	{
		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			APlayerController* Player = It->Get();
			if (APawn* Pawn = Player && Player->IsLocalController() ? Player->GetPawn() : nullptr)
			{
				Players.Add(Pawn);
			}
		}
	}
	for (APawn* Pawn : Players)
	{
		const float Distance = static_cast<float>(FVector::Dist(Pawn->GetActorLocation(), Mouth));
		const float Felt = Tuning.ShriekFeltRadius * Scale;
		const float Near = Tuning.ShriekRattleRadius * Scale;
		if (Distance > Felt)
		{
			continue;
		}
		FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(World, TEXT("UnpaidShriekRattle"), &Unpaid);
		Params.AddIgnoredActor(Pawn);
		const bool bClear = !World->LineTraceTestByObjectType(Mouth, Pawn->GetActorLocation() + FVector(0.f, 0.f, 50.f),
			FCollisionObjectQueryParams(ECC_WorldStatic), Params);
		FPlayerRattle How;
		How.Shake = Tuning.ShriekShake;
		How.Gap = Tuning.ShriekRattleGap;
		const float Share = UnpaidMoveRules::RattleShare(Distance, Near);
		if (Share > 0.f && bClear)
		{
			How.Strength = Share;
			How.SwayDegrees = Tuning.ShriekSway;
			How.SwaySeconds = Tuning.ShriekSwaySeconds;
			How.SlowShare = Tuning.ShriekSlow;
			How.SlowSeconds = Tuning.ShriekSlowSeconds;
		}
		else
		{
			// Felt from farther off, or through a wall: the shake (falling off), a breath of frost, the arc. No sway, no slow.
			const float Beyond = FMath::Clamp((Distance - Near) / FMath::Max(Felt - Near, 1.f), 0.f, 1.f);
			How.Strength = FeltShare * (Distance <= Near ? 1.f : 1.f - Beyond) * (bClear ? 1.f : CoveredShare);
			How.bSway = false;
			How.bSlow = false;
		}
		Rattles->Rattle(Pawn, Mouth, How);
		if (How.Strength >= RingingAt)
		{
			LooterSound::Play2D(&Unpaid, FName(Cue::Rattled), FMath::Clamp(How.Strength, 0.f, 1.f));
		}
	}

	// Its pack rallied: others of its pack near it come on at the frenzy's pace a while (never in the lull after its leader fell).
	const UFightDirectorSubsystem* Director = UFightDirectorSubsystem::Get(&Unpaid);
	if (Director && Director->IsInLeaderLull(Unpaid))
	{
		return;
	}
	const float RallyReach = Tuning.ShriekRallyRadius * Scale;
	auto Rally = [&Unpaid, &Tuning, RallyReach](ACreatureBase* Each)
	{
		AUnpaidCreature* Other = Cast<AUnpaidCreature>(Each);
		if (!Other || Other == &Unpaid || Other->IsDead() || !Other->SharesPackWith(Unpaid) || !FUnpaidMoveWork::IsOn(*Other)
			|| FVector::DistSquared(Other->GetActorLocation(), Unpaid.GetActorLocation()) > FMath::Square(RallyReach))
		{
			return;
		}
		Other->Kit.Moves.RalliedLeft = FMath::Max(Other->Kit.Moves.RalliedLeft, Tuning.ShriekRallySeconds);
	};
	// Every Unpaid in the world (placed ones aren't all the encounters' tracked spawns); once a shriek, a few dozen at most.
	if (World)
	{
		for (TActorIterator<AUnpaidCreature> It(World); It; ++It)
		{
			Rally(*It);
		}
	}
}

void FUnpaidShriekWork::Show(AUnpaidCreature& Unpaid, float DeltaSeconds, EUnpaidShriekPhase Phase, const FUnpaidMoveTuning& Tuning)
{
	FUnpaidMoveState& State = Unpaid.Kit.Moves;
	const float Scale = Unpaid.GetSizeScale();
	const float Time = State.ShriekTime;
	const float Fx = Tuning.ShriekFx;
	FWeaponFX* Effects = Fx > 0.f ? ShriekEffects(Unpaid.GetWorld()) : nullptr;
	FVector Way;
	const FVector Mouth = MouthOf(Unpaid, Way);
	const FVector Coal = Unpaid.GetCoalLocation();
	const FVector CoalOut = Unpaid.GetCoalFacing();
	const FLinearColor Tint = Unpaid.GetCoalColor();
	float Candelas = 0.f;
	float Whiten = TellWhiten;
	float EmberShare = 1.f;
	switch (Phase)
	{
	case EUnpaidShriekPhase::Tell:
	{
		// Breath drawn in: cold motes sucked into its mouth, its coal brightening and pulsing fast.
		const float Rise = Ease(Time / FMath::Max(Tuning.ShriekTell, 0.01f));
		const float Pulse = FMath::Sin(2.f * UE_PI * 9.f * Time);
		Unpaid.Heat = FMath::Max(Unpaid.Heat, 0.55f + 0.4f * Rise + 0.25f * Pulse);
		Candelas = TellCandelas * (0.3f + 0.7f * Rise) * (1.f + 0.3f * Pulse);
		if (Effects)
		{
			Effects->SpawnSoulInhale(Mouth, InhaleShell * Scale, Tint, Owe(State.MotesOwed, MoteRate * Fx * (0.5f + Rise), DeltaSeconds));
		}
		break;
	}
	case EUnpaidShriekPhase::Scream:
	{
		// The cone pours out of its mouth, strongest as it starts; frost on its breath; its coal at its hottest, flickering.
		const float Into = FMath::Clamp((Time - Tuning.ShriekTell) / FMath::Max(Tuning.ShriekScream, 0.01f), 0.f, 1.f);
		const float Flicker = FMath::Sin(2.f * UE_PI * 13.f * Time);
		Unpaid.Heat = FMath::Max(Unpaid.Heat, 1.1f + 0.25f * Flicker);
		Candelas = ScreamCandelas * (1.f - 0.4f * Into) * (1.f + 0.3f * Flicker);
		Whiten = ScreamWhiten;
		EmberShare = 1.4f;
		if (Effects)
		{
			const float Cone = ConeRate * FMath::Square(1.f - Into) + ConeTail;
			Effects->SpawnShriekCone(Mouth, Way, ConeReachTo(Unpaid, Mouth, ConeReach * Scale), Tint, Owe(State.WispsOwed, Cone * Fx, DeltaSeconds), 1.f - 0.5f * Into);
			for (int32 Count = Owe(State.FrostOwed, FrostRate * Fx, DeltaSeconds); Count > 0; --Count)
			{
				Effects->SpawnFrostBreath(Mouth, Way, 40.f * Scale);
			}
		}
		break;
	}
	case EUnpaidShriekPhase::Recover:
	{
		// Hunched, panting: small cold breaths, its coal settling, its light dying away.
		const float Into = FMath::Clamp((Time - Tuning.ShriekTell - Tuning.ShriekScream) / FMath::Max(Tuning.ShriekRecover, 0.01f), 0.f, 1.f);
		Unpaid.Heat = FMath::Max(Unpaid.Heat, FMath::Lerp(0.7f, 0.15f, Into));
		Candelas = TellCandelas * 0.6f * (1.f - Into);
		EmberShare = 0.35f;
		if (Effects)
		{
			for (int32 Count = Owe(State.FrostOwed, PantRate * Fx, DeltaSeconds); Count > 0; --Count)
			{
				Effects->SpawnFrostBreath(Mouth, (Way - FVector(0.f, 0.f, 0.4f)).GetSafeNormal(), 24.f * Scale);
			}
		}
		break;
	}
	default:
		break;
	}
	// Embers and soot flung from its coal all through it.
	if (Effects)
	{
		const int32 Embers = Owe(State.EmbersOwed, EmberRate * EmberShare * Fx, DeltaSeconds);
		if (Embers > 0)
		{
			Effects->SpawnCoalFling(Coal, CoalOut, Tint, Embers, FMath::FRand() < 0.25f);
		}
	}
	// Its coal's light on its own chest, face and arms and the ground round it.
	if (State.Light != INDEX_NONE)
	{
		if (UUnpaidPackSubsystem* Packs = UUnpaidPackSubsystem::Get(&Unpaid))
		{
			const FVector Where = Coal + CoalOut * (LightOut * Scale) + FVector(0.f, 0.f, LightUp * Scale);
			Packs->SetLight(State.Light, Where, FMath::Lerp(Tint, FLinearColor::White, Whiten), Candelas, LightReach * Scale);
		}
	}
}

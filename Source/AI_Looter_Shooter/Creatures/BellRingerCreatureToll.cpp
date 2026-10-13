// ABellRingerCreature's toll: the wait between tolls, the tell (the bell raised high and shaken, a halo of light gathering
// round it, the clapper rattling), the toll itself (heard across the fight, a pale ring running out over the ground), what
// it does (raise one of the Unpaid fallen lately near it, or steady its pack), the lowering after it, and the bell shot in
// the tell: the toll lost, the ringer reeling.

#include "Creatures/BellRingerCreature.h"
#include "AI_Looter_Shooter.h"
#include "Audio/CreatureVoiceComponent.h"
#include "Audio/LooterSound.h"
#include "Creatures/BellRingerGraves.h"
#include "Creatures/BellRingerRing.h"
#include "Creatures/BellRingerSteady.h"
#include "Creatures/EncounterSettings.h"
#include "Creatures/EncounterSubsystem.h"
#include "Creatures/ShriekRing.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"

namespace
{
	/** The halo circles this far round the bell's body (cm at size 1), under the fist like the bell. */
	constexpr float HaloRadius = 24.f;
	constexpr float HaloDrop = 15.f;

	/** A gathering ring closes on a grave from this far (cm). */
	constexpr float GatherRadius = 260.f;

	/** A tell broken off without a toll (its target gone) lets it try again this soon (s). */
	constexpr float RetryAfterAbandon = 1.f;

	/** The toll throws the bell out on its bail (radians a second: out to its right, and back). */
	constexpr float TollKickOut = 7.f;
	constexpr float TollKickBack = 2.f;
	/** A bullet on the bell knocks it swinging away from the shot this hard (radians a second). */
	constexpr float StruckKick = 6.f;

	/** A broken toll's burst: dull, the light knocked out of it. */
	const FLinearColor BrokenColor(0.32f, 0.36f, 0.38f);
}

bool ABellRingerCreature::WantsToll() const
{
	const APawn* Victim = GetTarget();
	if (!Victim || IsDead() || IsPassive() || Act != EAct::None || TollWait > 0.f || bBackingOff || IsPhasing()
		|| GetCreatureState() != ECreatureState::Chase)
	{
		return false;
	}
	return FVector::Dist2D(GetActorLocation(), Victim->GetActorLocation()) <= Tuning.TollRange;
}

bool ABellRingerCreature::BeginToll()
{
	if (!GetTarget() || IsDead() || Act != EAct::None || IsPhasing() || GetCreatureState() == ECreatureState::Attack)
	{
		return false;
	}
	StopBackingOff();
	SetAct(EAct::Tell);
	HoldStill();
	SetTurnsWithMovement(false);
	// The tell, heard and seen: the clapper rattling as the bell comes up, and the halo gathering round it (in the cracked
	// bell's rank color for a Gravebound one).
	LooterSound::PlayAttached(LooterSoundCue::BellRinger::Lift, Bell, NAME_None, 1.f, Voice->GetPitch());
	const FLinearColor Color = BellRingerRules::IsCrackedBell(GetRank()) ? GetCoalColor() : TollColor();
	Halo = ABellRingerRing::SpawnHalo(Bell, FVector(0.f, 0.f, -HaloDrop), HaloRadius * GetSizeScale(), Color);
	UE_LOG(LogLooter, Verbose, TEXT("%s raises its bell."), *GetName());
	return true;
}

void ABellRingerCreature::SetAct(EAct NewAct)
{
	Act = NewAct;
	ActTime = 0.f;
	if (NewAct == EAct::None)
	{
		// Done at the bell: its phase-steps and its turning with its walk come back.
		if (HeldPhaseCooldown >= 0.f)
		{
			PhaseStep.Cooldown = HeldPhaseCooldown;
			HeldPhaseCooldown = -1.f;
		}
		SetTurnsWithMovement(!bBackingOff);
	}
}

void ABellRingerCreature::TickToll(float DeltaSeconds)
{
	// A fresh hunt: its first toll waits a moment, so the pack's rush comes first and the bell after it.
	const ECreatureState BrainState = GetCreatureState();
	const bool bHunting = GetTarget() && (BrainState == ECreatureState::Chase || BrainState == ECreatureState::Attack);
	if (bHunting && !bWasHunting)
	{
		TollWait = FMath::Max(TollWait, Tuning.FirstTollDelay);
	}
	bWasHunting = bHunting;

	if (Act == EAct::None)
	{
		if (bHunting)
		{
			TollWait = FMath::Max(TollWait - DeltaSeconds, 0.f);
		}
		if (WantsToll())
		{
			BeginToll();
		}
		return;
	}

	ActTime += DeltaSeconds;
	switch (Act)
	{
	case EAct::Tell:
		// Its target gone, out of sight or off its ground (it searches, or watches from its edge): the tell breaks off quietly.
		if (!GetTarget() || BrainState == ECreatureState::Return || BrainState == ECreatureState::Idle || BrainState == ECreatureState::Search)
		{
			AbandonToll();
			return;
		}
		HoldStill();
		FaceTarget(DeltaSeconds);
		if (ABellRingerRing* Ring = Halo.Get())
		{
			Ring->SetStrength(ActTime / FMath::Max(Tuning.TellSeconds, 0.01f));
		}
		if (ActTime >= Tuning.TellSeconds)
		{
			LandToll();
		}
		break;

	case EAct::Lower:
		HoldStill();
		FaceTarget(DeltaSeconds);
		if (ActTime >= Tuning.LowerSeconds)
		{
			SetAct(EAct::None);
		}
		break;

	case EAct::Reel:
		HoldStill();
		if (ActTime >= Tuning.ReelSeconds)
		{
			SetAct(EAct::None);
		}
		break;

	default:
		break;
	}
}

EBellToll ABellRingerCreature::ChooseTollNow(int32* OutFallen) const
{
	const UBellRingerGraves* Graves = UBellRingerGraves::Get(this);
	const UWorld* World = GetWorld();
	int32 Index = INDEX_NONE;
	if (Graves && World && RaisedCount < Tuning.MaxRaises)
	{
		// Without a target (a test, a console toll) the player is nowhere near any grave.
		const APawn* Victim = GetTarget();
		const FVector Player = Victim ? Victim->GetActorLocation() : FVector(1.0e9, 1.0e9, 0.0);
		Index = Graves->FindRaisable(GetActorLocation(), World->GetTimeSeconds(), Player, Tuning);
	}
	const bool bRoom = Index != INDEX_NONE && HasRoomFor(Graves->GetFallen()[Index].Class.Get());
	const EBellToll Toll = BellRingerRules::ChooseToll(Index != INDEX_NONE, RaisedCount, Tuning.MaxRaises, bRoom);
	if (OutFallen)
	{
		*OutFallen = Toll == EBellToll::Raise ? Index : INDEX_NONE;
	}
	return Toll;
}

bool ABellRingerCreature::HasRoomFor(const UClass* Class) const
{
	if (!Class)
	{
		return false;
	}
	const UEncounterSubsystem* Encounters = UEncounterSubsystem::Get(this);
	if (!Encounters)
	{
		return true;
	}
	// The same caps a spawner keeps to: the Unpaid's 12 in the level, and 16 creatures of any kind near the player.
	const UEncounterSettings& Settings = UEncounterSettings::Get();
	const UClass* Counted = nullptr;
	const int32 Cap = Settings.FindClassCap(Class, &Counted);
	if (Cap > 0 && Encounters->CountAlive(Counted) >= Cap)
	{
		return false;
	}
	const APawn* Player = Encounters->GetPlayer();
	return !Player || Encounters->CountAliveNear(Player->GetActorLocation(), Settings.NearPlayerRadius) < Settings.MaxCreaturesNearPlayer;
}

void ABellRingerCreature::LandToll()
{
	UWorld* World = GetWorld();
	if (UBellRingerGraves* Graves = UBellRingerGraves::Get(this))
	{
		// A last look first: one that fell during the tell is still called.
		Graves->Watch(World ? World->GetTimeSeconds() : 0.0, /*bNow*/ true);
	}
	int32 FallenIndex = INDEX_NONE;
	const EBellToll Wanted = ChooseTollNow(&FallenIndex);
	const bool bRaised = Wanted == EBellToll::Raise && RaiseFallen(FallenIndex);
	if (!bRaised)
	{
		SteadyPack();
	}
	LastToll = bRaised ? EBellToll::Raise : EBellToll::Steady;
	++TollCount;
	TollWait = BellRingerRules::TollInterval(Tuning, GetRank());

	// Heard across the fight, and seen: a pale ring running out over the ground as far as the toll reaches, a burst round
	// the bell. A Gravebound ringer's cracked bell rings sour, in its rank's color, and its ring slows the player.
	const bool bCracked = BellRingerRules::IsCrackedBell(GetRank());
	const FLinearColor Color = bCracked ? GetCoalColor() : TollColor();
	LooterSound::PlayAttached(bCracked ? LooterSoundCue::BellRinger::TollCracked : LooterSoundCue::BellRinger::Toll, Bell, NAME_None, 1.f,
		Voice->GetPitch());
	const float Scale = GetSizeScale();
	const FVector Feet = GetActorLocation() - FVector(0.f, 0.f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
	ABellRingerRing::SpawnWave(World, Feet, bRaised ? Tuning.RaiseRadius : Tuning.SteadyRadius, Color);
	ABellRingerRing::SpawnBurst(World, GetBellCenter(), Color);
	if (bCracked)
	{
		ACharacter* Victim = Cast<ACharacter>(GetTarget());
		if (Victim && IsValidTarget(Victim))
		{
			AShriekRing::Spawn(World, Feet, Tuning.CrackedRingRadius * Scale, Color, Victim, ShriekSlowMultiplier, ShriekSlowSeconds);
		}
	}
	if (ABellRingerRing* Ring = Halo.Get())
	{
		Ring->Finish();
	}
	Halo.Reset();
	// The stroke flings the bell out on its bail (the arm's snap swings it too).
	BellSwing.Speed.Y += TollKickOut;
	BellSwing.Speed.X -= TollKickBack;
	UE_LOG(LogLooter, Verbose, TEXT("%s tolls: %s."), *GetName(), bRaised ? TEXT("a fallen one rises") : TEXT("its pack is steadied"));
	SetAct(EAct::Lower);
}

bool ABellRingerCreature::RaiseFallen(int32 Index)
{
	UBellRingerGraves* Graves = UBellRingerGraves::Get(this);
	UWorld* World = GetWorld();
	if (!Graves || !World || !Graves->GetFallen().IsValidIndex(Index))
	{
		return false;
	}
	const FBellRingerFallen Fallen = Graves->GetFallen()[Index];
	UClass* Class = Fallen.Class.Get();
	if (!Class || !Class->IsChildOf(ACreatureBase::StaticClass()) || !Graves->Claim(Index))
	{
		return false;
	}
	ACreatureBase::FRuntimeSpawn Spawn;
	Spawn.Rank = Fallen.Rank;
	Spawn.Level = Fallen.OwnLevel;
	Spawn.BodyScale = Fallen.BodyScale;
	Spawn.HealthScale = Tuning.RaisedHealthShare;
	ACreatureBase* Risen = ACreatureBase::SpawnAtRuntime(World, Class, Fallen.Feet, Fallen.Yaw, Spawn);
	if (!Risen)
	{
		return false;
	}
	// The game's level has begun its play already; a test level never begins it by itself.
	if (!Risen->HasActorBegunPlay())
	{
		Risen->DispatchBeginPlay();
	}
	// Raised once, it falls for good the second time, and is worth half as much (its XPReward carries its rank already).
	Risen->Tags.AddUnique(UBellRingerGraves::RaisedTag);
	Risen->HuntingGround = Fallen.Ground;
	Risen->XPReward = FMath::Max(0, FMath::RoundToInt32(static_cast<float>(Risen->XPReward) * Tuning.RaisedXPShare));
	if (AUnpaidCreature* Unpaid = Cast<AUnpaidCreature>(Risen))
	{
		Unpaid->RiseIn();
	}
	if (APawn* Victim = GetTarget())
	{
		Risen->AlertTo(Victim);
	}
	Raised.Add(Risen);
	++RaisedCount;
	// At the grave: the whisper of the dead called up, the earth giving it back, a ring of soul-light closing on it.
	LooterSound::PlayAt(this, LooterSoundCue::BellRinger::Raise, Fallen.Feet + FVector(0.f, 0.f, 90.f));
	LooterSound::PlayAt(this, LooterSoundCue::UnpaidRise, Fallen.Feet);
	ABellRingerRing::SpawnGather(World, Fallen.Feet, GatherRadius, RaiseColor());
	UE_LOG(LogLooter, Log, TEXT("%s's toll raises %s where it fell (%d of %d)."), *GetName(), *Risen->GetName(), RaisedCount, Tuning.MaxRaises);
	return true;
}

int32 ABellRingerCreature::SteadyPack()
{
	int32 Steadied = 0;
	const FVector Here = GetActorLocation();
	const double Reach = FMath::Square(static_cast<double>(Tuning.SteadyRadius));
	auto Steady = [&](ACreatureBase& Other)
	{
		// The Unpaid near it, itself too; never a boss (Abel's own fight paces him).
		if (Other.IsDead() || Other.IsActorBeingDestroyed() || !Other.IsA<AUnpaidCreature>() || Other.GetRank() == ECreatureRank::Boss
			|| FVector::DistSquared(Other.GetActorLocation(), Here) > Reach)
		{
			return;
		}
		if (UBellRingerSteady::Apply(Other, Tuning.SteadySeconds, Tuning.SteadySpeedScale, Tuning.SteadyCooldownScale))
		{
			++Steadied;
		}
	};
	Steady(*this);
	if (const UEncounterSubsystem* Encounters = UEncounterSubsystem::Get(this))
	{
		for (const TWeakObjectPtr<ACreatureBase>& Each : Encounters->GetTrackedCreatures())
		{
			if (ACreatureBase* Other = Each.Get(); Other && Other != this)
			{
				Steady(*Other);
			}
		}
	}
	else if (UWorld* World = GetWorld())
	{
		for (TActorIterator<AUnpaidCreature> It(World); It; ++It)
		{
			if (*It != this)
			{
				Steady(**It);
			}
		}
	}
	return Steadied;
}

void ABellRingerCreature::BellStruck(const FVector& HitLocation)
{
	// The clang of it, and the bell knocked swinging away from the shot.
	LooterSound::PlayAttached(LooterSoundCue::BellRinger::Struck, Bell, NAME_None, 1.f, Voice->GetPitch());
	const FVector Away = GetActorRotation().UnrotateVector(GetBellCenter() - HitLocation).GetSafeNormal2D();
	BellSwing.Speed += FVector2D(Away.X, Away.Y) * StruckKick;
	if (Act == EAct::Tell && !IsDead())
	{
		BreakToll();
	}
}

void ABellRingerCreature::BreakToll()
{
	// The toll is lost: the halo knocked out in a dull burst, the ringer reeling, and a whole interval before it tries again.
	if (ABellRingerRing* Ring = Halo.Get())
	{
		Ring->Finish();
	}
	Halo.Reset();
	ABellRingerRing::SpawnBurst(GetWorld(), GetBellCenter(), BrokenColor);
	TollWait = BellRingerRules::TollInterval(Tuning, GetRank());
	SetAct(EAct::Reel);
	UE_LOG(LogLooter, Verbose, TEXT("%s's bell is shot: its toll is lost."), *GetName());
}

void ABellRingerCreature::AbandonToll()
{
	if (ABellRingerRing* Ring = Halo.Get())
	{
		Ring->Finish();
	}
	Halo.Reset();
	if (Act == EAct::Tell)
	{
		TollWait = FMath::Max(TollWait, RetryAfterAbandon);
	}
	if (Act != EAct::None)
	{
		SetAct(EAct::None);
	}
}

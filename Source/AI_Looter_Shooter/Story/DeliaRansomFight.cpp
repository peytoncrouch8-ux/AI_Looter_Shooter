// ADeliaRansom's fight on her porch: who she turns on, her blasts through the game's own bullets, the reload, the floor she
// leaves for the player, her flinch when hit, her barks. The numbers and rules are DeliaFightRules (DeliaRansomRules.h).

#include "Story/DeliaRansom.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Combat/BulletSubsystem.h"
#include "Combat/HealthComponent.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/EncounterSubsystem.h"
#include "Story/CaptionSubsystem.h"
#include "Story/DeliaRansomRules.h"
#include "Story/StoryLineSet.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	/** How often she looks round for creatures (s). */
	constexpr float DeliaScanEvery = 0.2f;
	/** The gun comes up this fast and goes down this fast (s for the whole way). */
	constexpr float DeliaRaiseSeconds = 0.35f;
	constexpr float DeliaLowerSeconds = 0.6f;
	/** A shot's kick and a blow's flinch ease off this fast (shares a second). */
	constexpr float DeliaKickFade = 4.f;
	constexpr float DeliaFlinchFade = 2.5f;
	/** Her pellets: their speed (cm/s), reach as a share past her fight's edge, shove, and look (the Ranchhand's warm tracer). */
	constexpr float DeliaPelletSpeed = 22000.f;
	constexpr float DeliaPelletReach = 2.2f;
	constexpr float DeliaPelletShove = 400.f;
	const FLinearColor DeliaTracer(1.f, 0.62f, 0.25f);
	/** How likely each moment is to draw a bark (her first sight of a fight always does). */
	constexpr float BarkOnHurt = 0.6f;
	constexpr float BarkOnReload = 0.35f;
	constexpr float BarkOnDown = 0.5f;
	constexpr float BarkOnFinish = 0.6f;
	constexpr float BarkOnClear = 0.6f;
	constexpr float BarkOnHoseMan = 0.9f;

	/** Nothing but creatures between From and Creature (the porch rail, a post or a wall hides it). */
	bool DeliaCanSee(const AActor& Delia, const FVector& From, const ACreatureBase& Creature)
	{
		UWorld* World = Delia.GetWorld();
		if (!World)
		{
			return false;
		}
		FCollisionQueryParams Params(SCENE_QUERY_STAT(DeliaSight), false, &Delia);
		if (const APawn* Player = UGameplayStatics::GetPlayerPawn(&Delia, 0))
		{
			Params.AddIgnoredActor(Player);
		}
		FHitResult Hit;
		if (!World->LineTraceSingleByChannel(Hit, From, Creature.GetActorLocation(), ECC_Visibility, Params))
		{
			return true;
		}
		return Hit.GetActor() == &Creature || Cast<ACreatureBase>(Hit.GetActor()) != nullptr;
	}
}

float ADeliaRansom::PelletDamageFor(const ACreatureBase& Creature) const
{
	const UHealthComponent* Vitals = Creature.FindComponentByClass<UHealthComponent>();
	return DeliaFightRules::PelletDamage(Vitals ? Vitals->GetMaxHealth() : 100.f);
}

void ADeliaRansom::ScanForTargets()
{
	// Every living creature in reach of her porch, as she sees it from her eyes (the gun comes up to them).
	TArray<FDeliaTarget> Seen;
	TArray<ACreatureBase*, TInlineAllocator<16>> Creatures;
	const FVector Eyes = (GetMesh() ? GetMesh()->GetComponentLocation() : GetActorLocation()) + FVector(0.0, 0.0, 160.0 * BodyScale());
	auto Consider = [&](ACreatureBase* Creature)
	{
		if (!Creature || Creature->IsDead() || Creature->IsHidden() || Creature->IsActorBeingDestroyed()
			|| !DeliaFightRules::InReach(Porch, Creature->GetActorLocation()))
		{
			return;
		}
		const UHealthComponent* Vitals = Creature->FindComponentByClass<UHealthComponent>();
		if (!Vitals || Vitals->IsDead())
		{
			return;
		}
		FDeliaTarget& Each = Seen.AddDefaulted_GetRef();
		Each.Where = Creature->GetActorLocation();
		Each.Health = Vitals->GetHealth();
		Each.MaxHealth = Vitals->GetMaxHealth();
		Each.bSeen = DeliaCanSee(*this, Eyes, *Creature);
		Creatures.Add(Creature);
	};
	if (const UEncounterSubsystem* Encounters = UEncounterSubsystem::Get(this))
	{
		for (const TWeakObjectPtr<ACreatureBase>& Each : Encounters->GetTrackedCreatures())
		{
			Consider(Each.Get());
		}
	}
	else if (UWorld* World = GetWorld())
	{
		for (TActorIterator<ACreatureBase> It(World); It; ++It)
		{
			Consider(*It);
		}
	}

	const bool bHadTarget = Target.IsValid();
	bool bWorth = false;
	const int32 Chosen = DeliaFightRules::Choose(GetActorLocation(), Seen, bWorth);
	Target = Chosen != INDEX_NONE ? Creatures[Chosen] : nullptr;
	bTargetWorth = Chosen != INDEX_NONE && bWorth;
	bTargetSeen = Chosen != INDEX_NONE && Seen[Chosen].bSeen;
	// Her first sight of a fight: the opening bark.
	if (!bHadTarget && Target.IsValid())
	{
		Bark(EDeliaBark::Opening, 1.f);
	}
	// A hose-man coming at the porch (the Dredgers' DredgerHoseMan, known by its class's name so she needs none of theirs):
	// a warning, once each.
	ACreatureBase* const* HoseMan = Creatures.FindByPredicate([](const ACreatureBase* Each) { return Each->GetClass()->GetName().Contains(TEXT("HoseMan")); });
	if (HoseMan && HoseManCalledFor.Get() != *HoseMan)
	{
		HoseManCalledFor = *HoseMan;
		Bark(EDeliaBark::HoseMan, BarkOnHoseMan);
	}
	// The one she shot at went down (hers or the player's doing): a word for it.
	if (const ACreatureBase* Shot = LastShotAt.Get(); Shot && Shot->IsDead())
	{
		LastShotAt.Reset();
		Bark(EDeliaBark::Down, BarkOnDown);
	}
	else if (LastShotAt.IsStale())
	{
		LastShotAt.Reset();
	}
}

void ADeliaRansom::StepFight(float DeltaSeconds)
{
	const float Dt = FMath::Max(DeltaSeconds, 0.f);
	ScanLeft -= Dt;
	if (ScanLeft <= 0.f)
	{
		ScanLeft = DeliaScanEvery;
		ScanForTargets();
	}
	ACreatureBase* Aim = Target.Get();
	if (Aim && Aim->IsDead())
	{
		Target.Reset();
		Aim = nullptr;
	}
	// A script's word overrides what's near; told not to fight she aims at nothing.
	if (FightOverride.IsSet() && !FightOverride.GetValue())
	{
		Target.Reset();
		Aim = nullptr;
	}
	ReadyLeft = Aim ? DeliaFightRules::StayReadySeconds : FMath::Max(ReadyLeft - Dt, 0.f);
	const bool bWantsFight = FightOverride.IsSet() ? FightOverride.GetValue() : (Aim != nullptr || ReadyLeft > 0.f);
	Kick = FMath::Max(Kick - DeliaKickFade * Dt, 0.f);
	Flinch = FMath::Max(Flinch - DeliaFlinchFade * Dt, 0.f);
	Cooldown -= Dt;

	switch (Fight)
	{
	case EDeliaFight::Reloading:
		ReloadClock += Dt;
		if (ReloadClock >= DeliaFightRules::ReloadSeconds)
		{
			ReloadClock = 0.f;
			Shells = DeliaFightRules::ShellsPerLoad;
			Fight = bWantsFight ? EDeliaFight::Ready : EDeliaFight::Calm;
		}
		break;
	case EDeliaFight::Calm:
		if (bWantsFight)
		{
			Fight = EDeliaFight::Ready;
		}
		else if (Shells < DeliaFightRules::ShellsPerLoad)
		{
			// The yard's quiet: she tops her gun off.
			StartReload();
		}
		break;
	case EDeliaFight::Ready:
		if (!bWantsFight)
		{
			Fight = EDeliaFight::Calm;
			Bark(EDeliaBark::Clear, BarkOnClear);
		}
		else if (Shells <= 0)
		{
			StartReload();
		}
		break;
	}

	// The gun up while she has something to aim at (or a script says fight), else carried low.
	const bool bUp = Fight == EDeliaFight::Ready && (Aim || (FightOverride.IsSet() && FightOverride.GetValue()));
	Shoulder = FMath::FInterpConstantTo(Shoulder, bUp ? 1.f : 0.f, Dt, 1.f / (bUp ? DeliaRaiseSeconds : DeliaLowerSeconds));
	if (Fight != EDeliaFight::Ready || !Aim || Shoulder < 0.95f)
	{
		return;
	}
	if (!bTargetWorth)
	{
		// Down to the floor she leaves: hers to call, the player's to finish.
		if (FinishCalledFor.Get() != Aim)
		{
			FinishCalledFor = Aim;
			Bark(EDeliaBark::Finish, BarkOnFinish);
		}
		return;
	}
	if (!bTargetSeen || Cooldown > 0.f)
	{
		return;
	}
	// On it (the chest leads the feet) and nobody of ours in the way.
	const FVector Muzzle = GetMuzzle();
	const float AimYaw = static_cast<float>((Aim->GetActorLocation() - GetActorLocation()).Rotation().Yaw);
	const float Facing = static_cast<float>(GetActorRotation().Yaw) + PoseInput.ChestYaw;
	if (FMath::Abs(FRotator::NormalizeAxis(AimYaw - Facing)) > DeliaFightRules::AimTolerance)
	{
		return;
	}
	if (const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
		Player && DeliaFightRules::PlayerInLine(Muzzle, Aim->GetActorLocation(), Player->GetActorLocation()))
	{
		return;
	}
	FireBlast();
}

void ADeliaRansom::FireBlast()
{
	ACreatureBase* Aim = Target.Get();
	if (!Aim)
	{
		return;
	}
	const FVector Muzzle = GetMuzzle();
	const FVector AimAt = Aim->GetActorLocation();
	const FVector Way = (AimAt - Muzzle).GetSafeNormal();
	if (UBulletSubsystem* Bullets = GetWorld() ? GetWorld()->GetSubsystem<UBulletSubsystem>() : nullptr)
	{
		// The game's own pellets: they fly, strike, mark the world and hurt as the player's do (blamed on her, so the ones
		// she hurts may turn on her), against bodies on the guns' channel; never into the player.
		FBulletShot Shot;
		Shot.Start = Muzzle;
		Shot.VisualStart = Muzzle;
		Shot.Speed = DeliaPelletSpeed;
		Shot.Range = DeliaFightRules::FightRadius * DeliaPelletReach;
		Shot.Damage = PelletDamageFor(*Aim);
		Shot.HitImpulse = DeliaPelletShove;
		Shot.Channel = ECC_GameTraceChannel2;
		Shot.TracerColor = DeliaTracer;
		Shot.TracerWidth = 2.2f;
		Shot.TracerLength = 260.f;
		Shot.Shooter = this;
		Shot.Instigator = GetController();
		Shot.IgnoreActor = UGameplayStatics::GetPlayerPawn(this, 0);
		const float Spread = FMath::DegreesToRadians(DeliaFightRules::SpreadFor(static_cast<float>(FVector::Dist(Muzzle, AimAt))));
		for (int32 Pellet = 0; Pellet < DeliaFightRules::Pellets; ++Pellet)
		{
			Shot.Direction = Random.VRandCone(Way, Spread);
			Bullets->Fire(Shot);
		}
	}
	ShowMuzzle(Muzzle, Way);
	LooterSound::PlayAt(this, LooterSoundCue::DeliaShotgun, Muzzle);
	Kick = 1.f;
	--Shells;
	++ShotsFired;
	Cooldown = DeliaFightRules::ShotInterval + Random.FRandRange(-DeliaFightRules::ShotJitter, DeliaFightRules::ShotJitter);
	LastShotAt = Aim;
	UE_LOG(LogLooter, Verbose, TEXT("%s: Delia fires at %s (%d shell%s left)."), *GetActorNameOrLabel(), *Aim->GetName(), Shells,
		Shells == 1 ? TEXT("") : TEXT("s"));
}

void ADeliaRansom::StartReload()
{
	Fight = EDeliaFight::Reloading;
	ReloadClock = 0.f;
	if (Shotgun)
	{
		LooterSound::PlayAttached(LooterSoundCue::DeliaReload, Shotgun);
	}
	Bark(EDeliaBark::Reload, BarkOnReload);
}

void ADeliaRansom::HandleBulletLanding(const FBulletShot& Shot, const FHitResult& Hit, FBulletLanding& Landing)
{
	if (Shot.Shooter.Get() != this)
	{
		return;
	}
	// Hers never land critical (the crit spots are the player's to find), and never take anything below her floor.
	Landing.bCritical = false;
	const AActor* Struck = Hit.GetActor();
	if (const UHealthComponent* Vitals = Struck ? Struck->FindComponentByClass<UHealthComponent>() : nullptr)
	{
		Landing.DamageScale *= DeliaFightRules::LandingScale(Vitals->GetHealth(), Vitals->GetMaxHealth(), Shot.Damage);
	}
}

void ADeliaRansom::HandleDamaged(float Damage, bool bCritical, FVector HitLocation, AController* InstigatedBy, AActor* DamageCauser)
{
	// She takes it (her health never drops): a flinch away from the blow, and she lets them hear about it.
	Flinch = 1.f;
	const FVector From = DamageCauser ? DamageCauser->GetActorLocation() : HitLocation;
	FlinchSide = ((GetActorLocation() - From) | GetActorRightVector()) >= 0.0 ? 1.f : -1.f;
	Bark(EDeliaBark::Hurt, BarkOnHurt);
}

void ADeliaRansom::Bark(EDeliaBark Moment, float Chance)
{
	if (!Barks || Barks->Lines.IsEmpty() || Random.FRand() > Chance)
	{
		return;
	}
	// The lines that suit the moment and are in the set, not the one she said last.
	TArray<int32, TInlineAllocator<4>> Suits;
	for (const int32 Index : DeliaFightRules::BarkLines(Moment))
	{
		if (Barks->Lines.IsValidIndex(Index) && Index != LastBarkLine)
		{
			Suits.Add(Index);
		}
	}
	if (Suits.IsEmpty())
	{
		return;
	}
	const UWorld* World = GetWorld();
	const double Now = World ? World->GetTimeSeconds() : 0.0;
	if (Now - LastBarkAt < BarkGap)
	{
		return;
	}
	// Never over someone else's words (the story's lines, the crew on the tube).
	UCaptionSubsystem* Captions = UCaptionSubsystem::Get(this);
	if (!Captions || Captions->GetCurrent())
	{
		return;
	}
	const int32 Pick = Suits[Random.RandRange(0, Suits.Num() - 1)];
	FStoryLine Line = Barks->Lines[Pick];
	if (Line.Speaker.IsEmpty())
	{
		Line.Speaker = SpeakerNameText();
	}
	Captions->Play({ Line }, ECaptionPlay::Queue);
	LastBarkAt = Now;
	LastBarkLine = Pick;
	++BarksSaid;
}

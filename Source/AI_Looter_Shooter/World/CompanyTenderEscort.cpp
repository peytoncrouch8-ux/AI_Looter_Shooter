// ACompanyTender as one of Main 6's escorts ("The Pilot Light", 2026-11-01): spawned by the sortie, it keeps a station of
// its own on the Lamplighter's calm pose (standing off, or alongside to board her), and its gas-bag is its weak spot: shots
// on it leak it, then tear it, and the tender falls burning into the fields below, crashes with a blast and smokes there a
// while. Landfall's tender never comes here.

#include "World/CompanyTender.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Creatures/CreatureBase.h"
#include "Player/ViewKick.h"
#include "World/LamplighterEffects.h"
#include "World/LamplighterShip.h"
#include "Components/AudioComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

namespace
{
	/** Bullets' trace channel (DefaultEngine.ini's Weapon). */
	constexpr ECollisionChannel EscortWeaponChannel = ECC_GameTraceChannel2;
	/** Its heave as it keeps station (CompanyTender.cpp's: a lighter bag than hers, a quicker bob). */
	constexpr float EscortHeaveCm = 10.f;
	constexpr float EscortHeaveHz = 0.31f;
	/** Its run in angles toward her this much at its start (degrees), straightening as it comes alongside. */
	constexpr float EscortRunYaw = 18.f;
	/** Its bag's middle over its keel (the Tripo tender's: CompanyTender.cpp's BagHeight). */
	constexpr float EscortBagHeight = 388.f;
	/** Past this share of its damage the bag leaks: a jet of gas every LeakEvery seconds and a list toward the hole. */
	constexpr float LeakShare = 0.5f;
	constexpr float LeakEvery = 0.12f;
	constexpr float LeakListDegrees = 7.f;
	/** The wreck: it smokes this long in the field (a column the player can see from her deck), then it's gone. */
	constexpr float WreckSeconds = 40.f;
	constexpr float WreckSmokeEvery = 0.2f;
	/** How it lies there: on its side a little, nose down. */
	constexpr float WreckRoll = 24.f;
	constexpr float WreckPitch = 9.f;
	/** The crash is felt on her deck within this far (cm), at most this hard. */
	constexpr float CrashFeltWithin = 9000.f;
	constexpr float CrashShake = 0.45f;

	APawn* EscortPlayer(const UWorld* World)
	{
		const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
		return Controller ? Controller->GetPawn() : nullptr;
	}
}

ACompanyTender* ACompanyTender::SpawnEscort(ALamplighterShip& ForShip, const FVector& FromLocal, const FVector& StationLocal, float InSide,
	float RunSeconds, float BagHealth)
{
	UWorld* World = ForShip.GetWorld();
	if (!World)
	{
		return nullptr;
	}
	const FTransform Start = FTransform(FromLocal) * ForShip.GetCalmTransform();
	ACompanyTender* Tender = World->SpawnActorDeferred<ACompanyTender>(ACompanyTender::StaticClass(), Start, nullptr, nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Tender)
	{
		return nullptr;
	}
	Tender->Ship = &ForShip;
	Tender->Side = InSide >= 0.f ? 1.f : -1.f;
	Tender->bEscort = true;
	Tender->EscortFrom = FromLocal;
	Tender->EscortStation = StationLocal;
	Tender->EscortNow = FromLocal;
	Tender->EscortRunSeconds = FMath::Max(RunSeconds, 0.5f);
	Tender->BagHealthMax = FMath::Max(BagHealth, 1.f);
	Tender->FinishSpawning(Start);
	// An escort grapples nothing: its launchers are Landfall's (they'd sit unturned on its root).
	for (UStaticMeshComponent* Launcher : Tender->Launchers)
	{
		if (Launcher)
		{
			Launcher->SetVisibility(false);
		}
	}
	// Its bag takes shots now (the Weapon channel only, so the creatures' ground traces and the camera pass it).
	Tender->SetCanBeDamaged(true);
	Tender->BagHit->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Tender->BagHit->SetCollisionResponseToChannel(EscortWeaponChannel, ECR_Block);
	Tender->OnTakePointDamage.AddUniqueDynamic(Tender, &ACompanyTender::HandleBagDamage);
	Tender->PlankAlpha = 0.f;
	Tender->Advance(0.f);
	UE_LOG(LogLooter, Log, TEXT("The Pilot Light: a Company tender comes up on her %s (its bag good for %.0f)."),
		Tender->Side > 0.f ? TEXT("starboard") : TEXT("port"), Tender->BagHealthMax);
	return Tender;
}

void ACompanyTender::MoveToStation(const FVector& StationLocal, float Seconds)
{
	MoveFrom = EscortNow;
	EscortStation = StationLocal;
	MoveSeconds = FMath::Max(Seconds, 0.2f);
	MoveClock = 0.f;
}

void ACompanyTender::RaiseGangplank()
{
	bPlankLowering = false;
}

float ACompanyTender::GetBagShare() const
{
	return bBagBurst ? 1.f : FMath::Clamp(BagDamage / FMath::Max(BagHealthMax, 1.f), 0.f, 1.f);
}

void ACompanyTender::AdvanceEscort(float DeltaSeconds)
{
	const ALamplighterShip* Boat = Ship.Get();
	if (!Boat)
	{
		return;
	}
	Clock += DeltaSeconds;
	float Yaw = 0.f;
	if (MoveClock >= 0.f)
	{
		// Sliding to a new station (the boarder pulling off her rail): eased, its nose swinging out a little as it goes.
		MoveClock += DeltaSeconds;
		const float A = FMath::SmoothStep(0.f, 1.f, FMath::Clamp(MoveClock / MoveSeconds, 0.f, 1.f));
		EscortNow = FMath::Lerp(MoveFrom, EscortStation, static_cast<double>(A));
		Yaw = Side * 10.f * FMath::Sin(UE_PI * A);
		if (MoveClock >= MoveSeconds)
		{
			MoveClock = -1.f;
		}
	}
	else
	{
		// The run in, as Landfall's: catching her up astern, sliding in to its station late, dropping as it comes.
		const float A = FMath::Clamp(Clock / EscortRunSeconds, 0.f, 1.f);
		const float Ahead = 1.f - FMath::Pow(1.f - A, 3.f);
		const float In = FMath::SmoothStep(0.3f, 1.f, A);
		const float Down = FMath::SmoothStep(0.f, 0.85f, A);
		EscortNow = FVector(FMath::Lerp(EscortFrom.X, EscortStation.X, static_cast<double>(Ahead)),
			FMath::Lerp(EscortFrom.Y, EscortStation.Y, static_cast<double>(In)),
			FMath::Lerp(EscortFrom.Z, EscortStation.Z, static_cast<double>(Down)));
		Yaw = Side * EscortRunYaw * (1.f - In);
		if (!bAlongside && A >= 1.f)
		{
			bAlongside = true;
			UE_LOG(LogLooter, Log, TEXT("The Pilot Light: a tender is on station off her %s."), Side > 0.f ? TEXT("starboard") : TEXT("port"));
		}
	}
	// A leaking bag lists it toward its hole, more the more it's lost.
	const float List = bLeaking ? LeakListDegrees * GetBagShare() * (LeakLocal.Y >= 0.0 ? 1.f : -1.f) : 0.f;
	const float Heave = EscortHeaveCm * FMath::Sin(UE_TWO_PI * EscortHeaveHz * Clock);
	const FTransform Station(FRotator(0.0, Yaw, List), EscortNow + FVector(0.0, 0.0, Heave), FVector::OneVector);
	SetActorTransform(Station * Boat->GetCalmTransform());
	UpdatePlank(DeltaSeconds);
	HoldRiflemen();
	if (bLeaking)
	{
		LeakWait -= DeltaSeconds;
		if (LeakWait <= 0.f)
		{
			LeakWait = LeakEvery;
			const FTransform Now = GetActorTransform();
			LamplighterEffects::GasPuff(this, Now.TransformPosition(LeakLocal), Now.TransformVector(LeakLocal.GetSafeNormal() * 100.0));
		}
	}
}

void ACompanyTender::HoldRiflemen()
{
	// They hold their ground on its deck and fire across; it flies under them, so they're put back where they stand.
	const FTransform Now = GetActorTransform();
	for (int32 Index = 0; Index < Riflemen.Num(); ++Index)
	{
		ACreatureBase* Rifleman = Riflemen[Index].Get();
		if (Rifleman && !Rifleman->IsDead() && RiflemanSpots.IsValidIndex(Index))
		{
			const float Half = Rifleman->GetCapsuleComponent() ? Rifleman->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 90.f;
			Rifleman->SetActorLocation(Now.TransformPosition(RiflemanSpots[Index]) + Now.GetRotation().GetUpVector() * Half,
				/*bSweep*/ false, nullptr, ETeleportType::TeleportPhysics);
		}
	}
}

void ACompanyTender::HandleBagDamage(AActor* DamagedActor, float Damage, AController* InstigatedBy, FVector HitLocation,
	UPrimitiveComponent* HitComponent, FName BoneName, FVector ShotFromDirection, const UDamageType* DamageType, AActor* DamageCauser)
{
	if (!bEscort || bBagBurst || bFalling || HitComponent != BagHit || Damage <= 0.f)
	{
		return;
	}
	BagDamage += Damage;
	// Every hole spits gas: a puff where the shot went in, and the canvas's thwip.
	const FVector Out = -ShotFromDirection.GetSafeNormal();
	LamplighterEffects::GasPuff(this, HitLocation, Out * 90.0);
	LooterSound::PlayAt(this, LooterSoundCue::Impact::Cloth, HitLocation, 0.8f, 0.85f);
	if (!bLeaking && GetBagShare() >= LeakShare)
	{
		// Half gone: the first big hole hisses and jets, and it lists toward it (the tell that a few more will drop it).
		bLeaking = true;
		LeakLocal = GetActorTransform().InverseTransformPosition(HitLocation);
		LeakSound = LooterSound::Start(this, LooterSoundCue::Foreman::FuseHiss, BagHit, NAME_None, 0.7f, 0.7f);
		UE_LOG(LogLooter, Log, TEXT("The Pilot Light: a tender's bag is leaking (%.0f of %.0f)."), BagDamage, BagHealthMax);
	}
	if (BagDamage >= BagHealthMax)
	{
		BurstBag(InstigatedBy);
	}
}

bool ACompanyTender::BurstBag(AController* By)
{
	if (!bEscort || bBagBurst || bFalling)
	{
		return false;
	}
	bBagBurst = true;
	LooterSound::Stop(LeakSound.Get(), 0.1f);
	LeakSound.Reset();
	BagHit->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetCanBeDamaged(false);
	RaiseGangplank();
	// The bag tears with a ripping roar and goes up: a ball of fire, the blast felt on her deck, then the burning fall.
	const FVector Bag = GetActorTransform().TransformPosition(FVector(0.0, 0.0, EscortBagHeight));
	LooterSound::PlayAt(this, LooterSoundCue::ShipBagTear, Bag);
	LooterSound::PlayAt(this, LooterSoundCue::Launcher::Explosion, Bag, 0.8f, 0.8f);
	LamplighterEffects::Fireball(this, Bag, FVector(0.0, 0.0, 200.0));
	LamplighterEffects::Sparks(this, Bag, FVector::UpVector, 14, 1.4f);
	if (const APawn* Player = EscortPlayer(GetWorld()))
	{
		const float Near = 1.f - FMath::Clamp(FVector::Dist(Player->GetActorLocation(), Bag) / CrashFeltWithin, 0.f, 1.f);
		LamplighterEffects::Shake(this, 0.25f + 0.45f * Near, 0.8f);
	}
	FallAway();
	UE_LOG(LogLooter, Log, TEXT("The Pilot Light: a tender's bag tears (%s); it falls burning."), By ? *By->GetName() : TEXT("nobody"));
	return true;
}

void ACompanyTender::Crash(const FVector& Ground)
{
	bFalling = false;
	bCrashed = true;
	WreckClock = 0.f;
	WreckSmokeWait = 0.f;
	// Down in the field: a blast heard across the valley, dust thrown up, the hull lying broken on its side, its riflemen
	// burned with it (no bodies to loot: the fall had them).
	const FRotator Lying(-WreckPitch, GetActorRotation().Yaw, Side * WreckRoll);
	SetActorLocationAndRotation(Ground + FVector(0.0, 0.0, 60.0), Lying, /*bSweep*/ false, nullptr, ETeleportType::TeleportPhysics);
	SetActorEnableCollision(false);
	LooterSound::PlayAt(this, LooterSoundCue::Launcher::ExplosionDistant, Ground);
	LooterSound::PlayAt(this, LooterSoundCue::Gunfeel::ImpactHeavy, Ground, 1.f, 0.7f);
	LamplighterEffects::Fireball(this, Ground + FVector(0.0, 0.0, 200.0), FVector(0.0, 0.0, 300.0));
	LamplighterEffects::DustSkirt(this, Ground, 900.f, 18);
	if (const APawn* Player = EscortPlayer(GetWorld()))
	{
		const float Near = 1.f - FMath::Clamp(FVector::Dist(Player->GetActorLocation(), Ground) / CrashFeltWithin, 0.f, 1.f);
		if (Near > 0.f)
		{
			LamplighterEffects::Shake(this, CrashShake * Near, 0.7f);
		}
	}
	for (const TWeakObjectPtr<ACreatureBase>& Rifleman : Riflemen)
	{
		if (ACreatureBase* Gone = Rifleman.Get())
		{
			Gone->Destroy();
		}
	}
	Riflemen.Reset();
	RiflemanSpots.Reset();
	WreckSound = LooterSound::Start(this, LooterSoundCue::WatchFire::Burn, Root, NAME_None, 0.8f, 0.9f);
	UE_LOG(LogLooter, Log, TEXT("The Pilot Light: a tender crashed in the fields at %s; it smokes a while."), *Ground.ToCompactString());
}

void ACompanyTender::AdvanceWreck(float DeltaSeconds)
{
	WreckClock += DeltaSeconds;
	WreckSmokeWait -= DeltaSeconds;
	// A column of black smoke from the wreck, thinning toward its end: seen from her deck as she flies on.
	const float Thinning = 1.f - FMath::SmoothStep(WreckSeconds * 0.6f, WreckSeconds, WreckClock);
	if (WreckSmokeWait <= 0.f && Thinning > 0.f)
	{
		WreckSmokeWait = WreckSmokeEvery / FMath::Max(Thinning, 0.25f);
		const FVector At = GetActorTransform().TransformPosition(FVector(FMath::FRandRange(-250.f, 250.f), FMath::FRandRange(-80.f, 80.f), 150.0));
		LamplighterEffects::SmokePuff(this, At, FVector(FMath::FRandRange(-40.f, 40.f), FMath::FRandRange(-40.f, 40.f), 380.0 * Thinning + 80.0));
	}
	if (WreckClock >= WreckSeconds)
	{
		Destroy();
	}
}

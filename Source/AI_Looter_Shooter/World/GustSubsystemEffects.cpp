// What a gust stirs in the world while it blows (UGustSubsystem): the wind heard swelling past, a swirl of dust rolling
// along the open ground past the player, a shutter or sign knocking near them, the washing flapping harder, the windmill
// spinning up, now and then a tumbleweed sent rolling. Each goes through what the thing already offers (FWeaponFX's dust
// puffs, the washing's and windmill's own settings, put back after; a tumbleweed lane's Launch), so nothing else changes.

#include "World/GustSubsystem.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Combat/BulletSubsystem.h"
#include "Core/LooterMenuGameMode.h"
#include "Weapons/ImpactSurface.h"
#include "Weapons/WeaponFX.h"
#include "World/AmbientAirSubsystem.h"
#include "World/FaunaActor.h"
#include "World/FaunaCloth.h"
#include "World/FaunaSubsystem.h"
#include "World/FaunaTumbleweeds.h"
#include "World/SwingComponent.h"
#include "World/Windmill.h"
#include "World/WindowShutter.h"
#include "World/WorldQueries.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

namespace
{
	/** The washing and windmills within these of the player feel it (cm); the rest are too far to see. */
	constexpr float ClothRange = 6000.f;
	constexpr float WindmillRange = 15000.f;
	/** At full push: the washing swings out this many times further and flutters this many times harder (capped as its own settings are). */
	constexpr float ClothSwingBoost = 1.8f;
	constexpr float ClothFlutterBoost = 2.5f;
	constexpr float ClothSwingMax = 60.f;
	constexpr float ClothFlutterMax = 20.f;
	/** The windmill turns this many times faster at full push. */
	constexpr float WindmillBoost = 1.6f;

	/** The swirl: only gusts this strong raise one; it starts this far upwind of the player (cm) and this far to a side. */
	constexpr float SwirlStrength = 0.45f;
	constexpr float SwirlUpwindMin = 700.f;
	constexpr float SwirlUpwindMax = 1300.f;
	constexpr float SwirlSideMax = 450.f;
	/** How fast it rolls (cm/s: this, plus SwirlSpeedPerStrength times the gust's strength) and puffs a second at full push. */
	constexpr float SwirlSpeed = 350.f;
	constexpr float SwirlSpeedPerStrength = 450.f;
	constexpr float SwirlPuffsPerSecond = 16.f;
	/** Over grass it's chaff and a little dust: fewer, fainter puffs. */
	constexpr float GrassShare = 0.35f;
	/** Its ground is looked at again this often (s) as it rolls: it follows the ground, and stops where open ground ends. */
	constexpr float SwirlGroundSeconds = 0.25f;
	/** Dust's own color (FSlideDust's on dirt), before the light. */
	const FLinearColor DustAlbedo(0.40f, 0.33f, 0.25f);

	/** A shutter or sign this near the player (cm) may knock; the knock comes this far into the gust (a share of it). */
	constexpr float RattleRange = 2500.f;
	constexpr float RattleShare = 0.35f;

	/** Gusts this strong may send a tumbleweed rolling off a lane whose start is between these from the player (cm). */
	constexpr float TumbleStrength = 0.65f;
	constexpr float TumbleRangeMin = 1500.f;
	constexpr float TumbleRangeMax = 5000.f;

	const FName GroundTag(TEXT("Ground"));

	/** The local player's pawn (or else the camera): where the gust is felt. */
	bool PlayerSpot(const UWorld* World, FVector& OutLocation)
	{
		const APlayerController* Player = World ? World->GetFirstPlayerController() : nullptr;
		if (const APawn* Pawn = Player ? Player->GetPawn() : nullptr)
		{
			OutLocation = Pawn->GetActorLocation();
			return true;
		}
		if (Player && Player->PlayerCameraManager)
		{
			OutLocation = Player->PlayerCameraManager->GetCameraLocation();
			return true;
		}
		return false;
	}

	/** The open ground under Where (walkable terrain, nothing over it), and what it is; false over anything else. */
	bool OpenGroundUnder(const UWorld& World, const FVector& Where, FHitResult& OutHit)
	{
		FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(&World, TEXT("GustSwirlGround"), nullptr, true);
		Params.bReturnFaceIndex = true;
		if (!World.LineTraceSingleByObjectType(OutHit, Where + FVector(0.0, 0.0, 600.0), Where - FVector(0.0, 0.0, 1200.0),
			FCollisionObjectQueryParams(ECC_WorldStatic), Params))
		{
			return false;
		}
		const AActor* Actor = OutHit.GetActor();
		return Actor && Actor->ActorHasTag(GroundTag);
	}

	/** Dust rises off bare ground: roads, yards, sand, gravel (and ground the game can't name). Grass gives a little. */
	float DustOf(EImpactMaterial Surface)
	{
		switch (Surface)
		{
		case EImpactMaterial::Dirt:
		case EImpactMaterial::Sand:
		case EImpactMaterial::Gravel:
		case EImpactMaterial::Unknown:
			return 1.f;
		case EImpactMaterial::Grass:
		case EImpactMaterial::Hay:
			return GrassShare;
		default:
			return 0.f;
		}
	}

	/** The angle from A to B (degrees, -180 to 180). */
	float YawDelta(float From, float To)
	{
		return static_cast<float>(FRotator::NormalizeAxis(static_cast<double>(To - From)));
	}
}

void UGustSubsystem::StirWorld()
{
	UWorld* World = GetWorld();
	const FGustEvent& Gust = Clock.GetCurrent();
	FVector Player;
	if (!World || !PlayerSpot(World, Player))
	{
		return;
	}
	const UAmbientAirSubsystem* Air = UAmbientAirSubsystem::Get(this);
	const bool bSheltered = Air && Air->IsSheltered();
	// The main menu's backdrop bows in the gusts but stays quiet under the menu's music.
	const bool bHeard = !bSheltered && !ALooterMenuGameMode::IsMenuWorld(World);

	// The wind swelling past: as loud as it's strong. Not down the mine.
	if (bHeard)
	{
		LooterSound::Play2D(this, GustCue::Pass, 0.45f + 0.55f * Gust.Strength, StirRandom.FRandRange(0.94f, 1.06f));
	}

	// The washing and the windmills near enough to see: their own settings kept, to push from and put back.
	StirredCloth.Reset();
	StirredWindmills.Reset();
	if (const UFaunaSubsystem* Fauna = UFaunaSubsystem::Get(this))
	{
		for (const TWeakObjectPtr<AFaunaActor>& Weak : Fauna->GetActors())
		{
			AFaunaCloth* Cloth = Cast<AFaunaCloth>(Weak.Get());
			if (Cloth && FVector::DistSquared(Cloth->GetFaunaCenter(), Player) < FMath::Square(static_cast<double>(ClothRange)))
			{
				StirredCloth.Add({ Cloth, Cloth->SwingDegrees, Cloth->FlutterDegrees, Cloth->WindYaw });
			}
		}
	}
	for (TActorIterator<AWindmill> It(World); It; ++It)
	{
		if (FVector::DistSquared(It->GetActorLocation(), Player) < FMath::Square(static_cast<double>(WindmillRange)))
		{
			StirredWindmills.Add({ *It, It->TurnsPerMinute });
		}
	}

	// The dust swirl: upwind of the player, so it rolls toward them and past, on open dusty ground.
	bSwirl = false;
	if (!bSheltered && Gust.Strength >= SwirlStrength)
	{
		const FVector Side = FVector::CrossProduct(FVector::UpVector, Gust.Direction).GetSafeNormal();
		const FVector Start = Player - Gust.Direction * StirRandom.FRandRange(SwirlUpwindMin, SwirlUpwindMax)
			+ Side * StirRandom.FRandRange(-SwirlSideMax, SwirlSideMax);
		FHitResult Hit;
		if (OpenGroundUnder(*World, Start, Hit))
		{
			const float Dust = DustOf(ImpactSurface::Of(Hit));
			if (Dust > 0.f)
			{
				bSwirl = true;
				bSwirlOnDust = Dust >= 1.f;
				SwirlAt = Hit.ImpactPoint;
				SwirlPuffsOwed = 0.f;
				SwirlGroundIn = SwirlGroundSeconds;
				SwirlSpin = StirRandom.FRandRange(0.f, UE_TWO_PI);
				// Dust is lit as the light where it starts shows it (the smoke material is unlit): dim in shade and at dusk.
				const bool bSun = Air && Air->IsInSun(SwirlAt + FVector(0.0, 0.0, 60.0));
				SwirlColor = DustAlbedo * (Air ? Air->LightFor(bSun ? 1.f : 0.f) : FLinearColor::White);
			}
		}
	}

	// A loose shutter or a hanging sign near the player knocks at the gust's height.
	bRattlePending = false;
	if (bHeard && (Gust.Strength >= 0.55f || StirRandom.FRand() < 0.5f))
	{
		double Best = FMath::Square(static_cast<double>(RattleRange));
		for (TActorIterator<AWindowShutter> It(World); It; ++It)
		{
			const double Distance = FVector::DistSquared(It->GetActorLocation(), Player);
			if (Distance < Best)
			{
				Best = Distance;
				RattleAt = It->GetActorLocation();
				bRattlePending = true;
			}
		}
		for (const TWeakObjectPtr<USwingComponent>& Weak : Swings)
		{
			const USwingComponent* Swing = Weak.Get();
			const double Distance = Swing ? FVector::DistSquared(Swing->GetComponentLocation(), Player) : TNumericLimits<double>::Max();
			if (Distance < Best)
			{
				Best = Distance;
				RattleAt = Swing->GetComponentLocation();
				bRattlePending = true;
			}
		}
	}

	if (bHeard && Gust.Strength >= TumbleStrength)
	{
		LaunchTumbleweed(Player);
	}
}

void UGustSubsystem::StirWorldUpdate(float DeltaSeconds)
{
	UWorld* World = GetWorld();
	const FGustEvent& Gust = Clock.GetCurrent();
	const float Push = Clock.GetStrengthNow();
	if (!World)
	{
		return;
	}

	// The washing swings out further and flutters harder, turned toward the gust's way as it comes on.
	const float GustYaw = static_cast<float>(Gust.Direction.Rotation().Yaw);
	for (const FGustStirredCloth& Each : StirredCloth)
	{
		if (AFaunaCloth* Cloth = Each.Cloth.Get())
		{
			Cloth->SwingDegrees = FMath::Min(Each.Swing * (1.f + ClothSwingBoost * Push), FMath::Max(Each.Swing, ClothSwingMax));
			Cloth->FlutterDegrees = FMath::Min(Each.Flutter * (1.f + ClothFlutterBoost * Push), FMath::Max(Each.Flutter, ClothFlutterMax));
			Cloth->WindYaw = Each.Yaw + YawDelta(Each.Yaw, GustYaw) * FMath::Min(Push * 2.f, 1.f);
		}
	}
	for (const FGustStirredWindmill& Each : StirredWindmills)
	{
		if (AWindmill* Windmill = Each.Windmill.Get())
		{
			Windmill->TurnsPerMinute = Each.TurnsPerMinute * (1.f + WindmillBoost * Push);
		}
	}

	// The knock, at the gust's height.
	if (bRattlePending && Clock.GetAge() >= RattleShare * Gust.Duration)
	{
		bRattlePending = false;
		LooterSound::PlayAt(this, GustCue::Rattle, RattleAt, 0.6f + 0.4f * Gust.Strength, StirRandom.FRandRange(0.92f, 1.08f));
	}

	// The swirl rolls on downwind, puffing dust as it goes, the ground under it looked at a few times a second.
	if (!bSwirl || DeltaSeconds <= 0.f)
	{
		return;
	}
	const float Shape = Gust.Strength > 0.f ? Push / Gust.Strength : 0.f;
	const float Speed = (SwirlSpeed + SwirlSpeedPerStrength * Gust.Strength) * (0.3f + 0.7f * Shape);
	SwirlAt += Gust.Direction * Speed * DeltaSeconds;
	SwirlSpin += DeltaSeconds * 5.f;
	SwirlGroundIn -= DeltaSeconds;
	if (SwirlGroundIn <= 0.f)
	{
		SwirlGroundIn = SwirlGroundSeconds;
		FHitResult Hit;
		const float Dust = OpenGroundUnder(*World, SwirlAt, Hit) ? DustOf(ImpactSurface::Of(Hit)) : 0.f;
		if (Dust <= 0.f)
		{
			// Off the open ground (a wall, a porch, the creek): it's spent.
			bSwirl = false;
			return;
		}
		SwirlAt = Hit.ImpactPoint;
		bSwirlOnDust = Dust >= 1.f;
	}
	UBulletSubsystem* Bullets = World->GetSubsystem<UBulletSubsystem>();
	if (!Bullets)
	{
		return;
	}
	FWeaponFX& Effects = Bullets->GetEffects();
	Effects.Initialize(World);
	const float Share = bSwirlOnDust ? 1.f : GrassShare;
	SwirlPuffsOwed += SwirlPuffsPerSecond * Push * Share * DeltaSeconds;
	const float Opacity = (bSwirlOnDust ? 0.3f : 0.18f) * FMath::Clamp(0.4f + Push, 0.f, 1.f);
	while (SwirlPuffsOwed >= 1.f)
	{
		SwirlPuffsOwed -= 1.f;
		// Thrown out round its middle and turned about it, low over the ground, carried on with the wind.
		const float Angle = SwirlSpin + StirRandom.FRandRange(0.f, UE_TWO_PI);
		const FVector Around(FMath::Cos(Angle), FMath::Sin(Angle), 0.f);
		const FVector Where = SwirlAt + Around * StirRandom.FRandRange(40.f, 130.f) + FVector(0.0, 0.0, StirRandom.FRandRange(10.f, 60.f));
		const FVector Curl = FVector::CrossProduct(FVector::UpVector, Around) * StirRandom.FRandRange(120.f, 260.f);
		const FVector Velocity = Gust.Direction * Speed * StirRandom.FRandRange(0.8f, 1.3f) + Curl
			+ FVector(0.0, 0.0, StirRandom.FRandRange(25.f, 90.f));
		const float Size = StirRandom.FRandRange(25.f, 45.f);
		Effects.SpawnDustPuff(Where, Velocity, SwirlColor, Opacity * StirRandom.FRandRange(0.7f, 1.1f), Size, Size * StirRandom.FRandRange(3.f, 4.5f),
			StirRandom.FRandRange(1.f, 1.6f));
	}
}

void UGustSubsystem::StillWorld()
{
	for (const FGustStirredCloth& Each : StirredCloth)
	{
		if (AFaunaCloth* Cloth = Each.Cloth.Get())
		{
			Cloth->SwingDegrees = Each.Swing;
			Cloth->FlutterDegrees = Each.Flutter;
			Cloth->WindYaw = Each.Yaw;
		}
	}
	for (const FGustStirredWindmill& Each : StirredWindmills)
	{
		if (AWindmill* Windmill = Each.Windmill.Get())
		{
			Windmill->TurnsPerMinute = Each.TurnsPerMinute;
		}
	}
	StirredCloth.Reset();
	StirredWindmills.Reset();
	bSwirl = false;
	bRattlePending = false;
}

void UGustSubsystem::LaunchTumbleweed(const FVector& Player)
{
	const UFaunaSubsystem* Fauna = UFaunaSubsystem::Get(this);
	if (!Fauna || !UFaunaSubsystem::IsEnabled())
	{
		return;
	}
	for (const TWeakObjectPtr<AFaunaActor>& Weak : Fauna->GetActors())
	{
		AFaunaTumbleweeds* Tumbleweeds = Cast<AFaunaTumbleweeds>(Weak.Get());
		if (!Tumbleweeds || !Tumbleweeds->IsFaunaShown())
		{
			continue;
		}
		// The lane whose upwind end is in range and nearest: it rolls toward the player's part of the world, with the gust.
		int32 Chosen = INDEX_NONE;
		double Best = TNumericLimits<double>::Max();
		for (int32 Lane = 0; Lane < Tumbleweeds->Lanes.Num(); ++Lane)
		{
			const double Distance = FVector::Dist2D(Tumbleweeds->Lanes[Lane].Start, Player);
			if (Distance >= TumbleRangeMin && Distance <= TumbleRangeMax && Distance < Best)
			{
				Best = Distance;
				Chosen = Lane;
			}
		}
		if (Chosen != INDEX_NONE && Tumbleweeds->Launch(Chosen))
		{
			UE_LOG(LogLooter, Verbose, TEXT("Gust: a tumbleweed off lane %d of %s."), Chosen, *Tumbleweeds->GetActorNameOrLabel());
			return;
		}
	}
}

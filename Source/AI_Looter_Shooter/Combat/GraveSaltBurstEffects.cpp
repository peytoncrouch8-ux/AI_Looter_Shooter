#include "Combat/GraveSaltBurst.h"
#include "Audio/LooterSound.h"
#include "Combat/BulletSubsystem.h"
#include "Combat/GraveSaltTuning.h"
#include "Combat/GrenadeKinds.h"
#include "Creatures/CreatureReactSubsystem.h"
#include "Elements/LooterElement.h"
#include "Player/CameraShakeModifier.h"
#include "Player/PlayerThrowMotion.h"
#include "Player/PlayerThrowRules.h"
#include "Player/PlayerViewComponent.h"
#include "Weapons/DeathSplats.h"
#include "Weapons/ImpactSurface.h"
#include "Weapons/NiagaraFX.h"
#include "Weapons/VfxSubsystem.h"
#include "Weapons/VfxTuning.h"
#include "Weapons/VfxTypes.h"
#include "Weapons/WeaponFX.h"
#include "World/WorldQueries.h"
#include "Camera/CameraComponent.h"
#include "CollisionQueryParams.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

// The grave-salt burst as it's seen, heard and felt. Since the user's playtest (2026-10-27, note 7: "lacks explosion
// visuals") it's a real explosion: the launcher's blast (FWeaponFX::SpawnExplosion: fire rolling into soot, a smoke
// column, a dust skirt, embers, debris) sized to the burst's 4 m, a shock ring racing out over the ground to that reach
// (so the player reads what it covered), a scorch left behind, and the salt's own white look on top. All drawn in code
// through FWeaponFX (no particle assets); the flash of light is the jar's (AGraveSaltGrenade). Each grenade kind draws
// it in its own look (GrenadeKinds::LookOf: its fire, its tar or embers on top, a small jar's quick pop). The one
// exception (round 5): the cinder's embers on top are NS_CinderBurst when Looter.Fx.Niagara is on and it's built.

namespace
{
	// The fire's colour is the kind's (GrenadeKinds::LookOf): the salt's hotter, paler than black powder's (the rocket's
	// is 1, 0.42, 0.1).
	/** The shock ring: a hot white with a warm cast, the crack of the burst running over the ground. */
	const FLinearColor RingTint(1.f, 0.86f, 0.62f);
	/** The salt: bone white with a warm cast, a little short of pure white so it never blooms out. */
	const FLinearColor SaltTint(0.92f, 0.9f, 0.84f);
	/** The salt cloud (unlit smoke: the colour it shows in afternoon light). */
	const FLinearColor SaltCloud(0.86f, 0.84f, 0.79f);

	/** A surface facing up at least this much is ground the burst lies on (its ring, its scorch); a wall isn't. */
	constexpr float GroundUp = 0.6f;
	/** The ring starts this share of the radius out (the fire covers the heart) and lies this far over the ground (cm). */
	constexpr float RingStartShare = 0.08f;
	constexpr float RingLift = 8.f;
	/**
	 * The salt on top, as FWeaponFX's sizes go: the gel burst's white clumps flung out (a body's width, cm), the crack of
	 * light as the tin gives, a few salt puffs rolling out with the dust skirt, a haze hanging over the spot after the
	 * smoke, and shards of the tin. Counts kept low: the blast under it is most of the pieces, and three bursts in two
	 * seconds must stay inside Looter.Vfx.MaxParticles.
	 */
	constexpr float SaltSize = 170.f;
	constexpr float FlashScale = 3.f;
	constexpr int32 CloudPuffs = 5;
	constexpr int32 HazePuffs = 4;
	constexpr int32 ShardPieces = 6;
	/** A burst sears at most this many of the dead at once: past it the sizzles only smear together. */
	constexpr int32 MaxSears = 3;

	FVector AcrossFrom(const FVector& Up, FRandomStream& Random)
	{
		const FVector Any = Random.GetUnitVector();
		const FVector Flat = (Any - Up * FVector::DotProduct(Any, Up)).GetSafeNormal();
		return Flat.IsNearlyZero() ? FVector::ForwardVector : Flat;
	}

	/** A kind's element burst on top of its blast (the cinder's embers): this share of the radius across. */
	constexpr float KindBurstShare = 0.5f;
	/** The pitch's tar pool left on the ground: this share of the radius across (the scorch round it is wider). */
	constexpr float TarPoolShare = 0.35f;
	/**
	 * A small jar's pop (the scatter's brood, four or more in a second): an Explosive round's pop sized to it, one
	 * fireball, a small ring and a few puffs of salt; about 30 pieces, so a whole brood costs less than one jar's blast.
	 */
	constexpr float PopFireballLife = 0.35f;
	constexpr float PopFireballGlow = 5.f;
	constexpr int32 PopPuffs = 3;
	constexpr float PopRingGlow = 0.6f;

	void DrawPop(FWeaponFX& Effects, const FGraveSaltBlast& Blast, const FVector& Base, const FVector& Up, float Radius,
		const FGraveSaltTuning& Tuning, const FGrenadeBurstLook& Look, const FVfxTuning& Vfx, FRandomStream& Random)
	{
		const FVector Heart = Base + Up * (Blast.bOnGround ? FGraveSaltRules::JarMiddle * 0.5f : 0.f);
		Effects.SpawnSplashPop(Heart, Up, Radius * 0.5f);
		Effects.SpawnFireball(Heart, Up * 60.f, Radius * 0.1f, Radius * 0.3f, Look.Fire, PopFireballGlow * Vfx.Glow, PopFireballLife);
		if (Tuning.RingGlow > 0.f && Blast.bOnGround)
		{
			Effects.SpawnShockRing(Base + Up * RingLift, Up, Radius * RingStartShare, Radius, Radius * Tuning.RingWidth, RingTint,
				Tuning.RingGlow * PopRingGlow * Vfx.Glow, Tuning.RingLife * 0.7f);
		}
		const float Salt = Tuning.SaltLayer * Look.Salt;
		for (int32 Index = 0; Salt > 0.f && Index < PopPuffs; ++Index)
		{
			const FVector Out = AcrossFrom(Up, Random);
			Effects.SpawnDustPuff(Heart + Out * 10.f, Out * Random.FRandRange(150.f, 260.f) + Up * Random.FRandRange(30.f, 80.f), SaltCloud,
				0.45f * Salt, 20.f, Random.FRandRange(80.f, 120.f) * Radius / 200.f, Random.FRandRange(0.9f, 1.3f));
		}
	}
}

FGraveSaltBlast GraveSaltBurst::FindGround(const UWorld& World, const FVector& Center, float Reach)
{
	FGraveSaltBlast Blast;
	Blast.Center = Center;
	Blast.Ground = Center;
	if (Reach <= 0.f)
	{
		return Blast;
	}
	// The world's solid shapes only (terrain, rocks, floors; never grass, volumes or bodies), traced as a bullet is so the
	// ground's own surface comes back (its debris: dirt clods, stone chips, splinters).
	FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(&World, TEXT("GraveSaltGround"));
	Params.bReturnPhysicalMaterial = true;
	Params.bReturnFaceIndex = true;
	// From a little over the heart: a jar lying on the ground has its heart only its ball's radius up.
	const FVector From = Center + FVector(0.f, 0.f, FGraveSaltRules::CollisionRadius);
	const FVector To = Center - FVector(0.f, 0.f, Reach);
	FHitResult Hit;
	if (!World.LineTraceSingleByObjectType(Hit, From, To, FCollisionObjectQueryParams(ECC_WorldStatic), Params) || Hit.bStartPenetrating)
	{
		return Blast;
	}
	const FVector Normal = Hit.ImpactNormal.GetSafeNormal();
	if (Normal.Z < GroundUp)
	{
		// A steep face: drawn in the air, nothing laid on a slope it would slide off.
		return Blast;
	}
	Blast.Ground = Hit.ImpactPoint;
	Blast.Up = Normal;
	Blast.bOnGround = true;
	const EImpactMaterial Surface = ImpactSurface::Of(Hit);
	// Ground that doesn't say what it is still throws up dirt (SpawnExplosion throws no debris for Unknown).
	Blast.Surface = Surface == EImpactMaterial::Unknown ? EImpactMaterial::Dirt : Surface;
	return Blast;
}

void GraveSaltBurst::DrawBlast(FWeaponFX& Effects, const FGraveSaltBlast& Blast, const FGraveSaltTuning& Tuning)
{
	DrawBlast(Effects, Blast, Tuning, GrenadeKinds::LookOf(EGrenadeKind::Salt));
}

void GraveSaltBurst::DrawBlast(FWeaponFX& Effects, const FGraveSaltBlast& Blast, const FGraveSaltTuning& Tuning, const FGrenadeBurstLook& Look)
{
	const float Radius = Tuning.Radius * Tuning.LookScale;
	const FVfxTuning Vfx = FVfxTuning::Live();
	if (Radius <= 0.f || Vfx.BlastScale <= 0.f)
	{
		return;
	}
	const FVector Up = Blast.Up.IsNearlyZero() ? FVector::UpVector : Blast.Up.GetSafeNormal();
	// On the ground the blast stands on it (its fire rises off it, its debris and ring lie on it); in the air it opens
	// where the jar was.
	const FVector Base = Blast.bOnGround ? Blast.Ground : Blast.Center;
	FRandomStream Random(static_cast<int32>(GetTypeHash(Blast.Center)));
	if (Look.bSmall)
	{
		DrawPop(Effects, Blast, Base, Up, Radius, Tuning, Look, Vfx, Random);
		return;
	}

	// The explosion: the launcher's (the white crack, a hot core and fire puffs rolling into soot, a smoke column after
	// the fire, a dust skirt over the ground, embers arcing out, the ground thrown up in its own look and debris raining
	// back), in the salt's hotter, paler fire (or the kind's own).
	FVfxExplosion Explosion;
	Explosion.Center = Base;
	Explosion.Up = Up;
	Explosion.Radius = Radius;
	Explosion.Fire = Look.Fire;
	Explosion.Ground = Blast.bOnGround ? Blast.Surface : EImpactMaterial::Unknown;
	Explosion.bOnGround = Blast.bOnGround;
	Explosion.DustLight = Blast.DustLight;
	Effects.SpawnExplosion(Explosion);

	// The shock ring, racing out to the burst's reach and fading as it gets there: what the player reads as "this far it
	// hurt". Flat on the ground; facing the eye in the air.
	if (Tuning.RingGlow > 0.f)
	{
		const float Glow = Tuning.RingGlow * Vfx.Glow;
		const float Width = Radius * Tuning.RingWidth;
		if (Blast.bOnGround)
		{
			Effects.SpawnShockRing(Base + Up * RingLift, Up, Radius * RingStartShare, Radius, Width, RingTint, Glow, Tuning.RingLife);
		}
		else
		{
			Effects.SpawnShockRing(Base, FVector::ZeroVector, Radius * RingStartShare, Radius, Width, RingTint, Glow, Tuning.RingLife, true);
		}
	}

	const FVector Heart = Base + Up * (Blast.bOnGround ? FGraveSaltRules::JarMiddle : 0.f);
	// The kind's own on top: the pitch's black globs of tar flung out, the cinder's burst of embers.
	if (Look.GlobSize > 0.f)
	{
		Effects.SpawnDeathBurst(EDeathBurst::Gel, Heart, Up, Look.GlobSize * Radius / 400.f, Look.GlobTint);
	}
	if (Look.Element != ELooterElement::None)
	{
		Effects.SpawnElementBurst(Look.Element, Heart, Radius * KindBurstShare, true);
	}

	// The salt on top: the crack of light as the tin gives, white clumps flung out every way with a glow on them...
	const float Salt = Tuning.SaltLayer * Look.Salt;
	if (Salt <= 0.f)
	{
		return;
	}
	Effects.SpawnFlash(Heart, Up, FlashScale * Salt);
	Effects.SpawnDeathBurst(EDeathBurst::Gel, Heart, Up, SaltSize * Salt, SaltTint);
	// ...a few puffs of salt rolling out with the dust...
	const float Reach = Radius / 400.f;
	for (int32 Index = 0; Index < CloudPuffs; ++Index)
	{
		const FVector Out = AcrossFrom(Up, Random);
		const FVector Velocity = (Out * Random.FRandRange(300.f, 480.f) + Up * Random.FRandRange(20.f, 70.f)) * Reach;
		Effects.SpawnDustPuff(Heart + Out * 20.f, Velocity, SaltCloud, 0.5f * Salt, 30.f, Random.FRandRange(140.f, 200.f) * Reach,
			Random.FRandRange(1.1f, 1.7f));
	}
	// ...a haze of it left hanging over the spot once the smoke's gone up, slow to clear, where the dead won't walk...
	for (int32 Index = 0; Index < HazePuffs; ++Index)
	{
		const FVector Drift = AcrossFrom(Up, Random) * Random.FRandRange(10.f, 40.f) + Up * Random.FRandRange(8.f, 24.f);
		Effects.SpawnDustPuff(Heart + AcrossFrom(Up, Random) * Random.FRandRange(40.f, 120.f) * Reach, Drift, SaltCloud, 0.3f * Salt, 80.f,
			Random.FRandRange(240.f, 320.f) * Reach, Random.FRandRange(3.f, 3.8f));
	}
	// ...and shards of the tin.
	for (int32 Index = 0; Index < ShardPieces; ++Index)
	{
		const FVector Out = (Up * Random.FRandRange(0.5f, 1.2f) + Random.GetUnitVector() * 0.8f).GetSafeNormal();
		Effects.SpawnGrit(Heart, Out * Random.FRandRange(400.f, 800.f), Random.FRandRange(1.5f, 3.f), Random.FRandRange(0.6f, 1.1f));
	}
}

void GraveSaltBurst::SpawnEffects(UWorld& World, const FVector& Center, const FVector& Up)
{
	SpawnEffects(World, Center, Up, FGraveSaltTuning::Current(), GrenadeKinds::LookOf(EGrenadeKind::Salt));
}

void GraveSaltBurst::SpawnEffects(UWorld& World, const FVector& Center, const FVector& Up, const FGraveSaltTuning& Tuning,
	const FGrenadeBurstLook& Look)
{
	UBulletSubsystem* Bullets = World.GetSubsystem<UBulletSubsystem>();
	if (!Bullets)
	{
		return;
	}
	FWeaponFX& Effects = Bullets->GetEffects();
	Effects.Initialize(&World);
	FGraveSaltBlast Blast = FindGround(World, Center, Tuning.GroundReach);
	if (!Blast.bOnGround && !Up.IsNearlyZero())
	{
		Blast.Up = Up.GetSafeNormal();
	}
	if (UCreatureReactSubsystem* Reacts = UCreatureReactSubsystem::Get(&World))
	{
		Blast.DustLight = Reacts->GetDustLight();
	}
	// The cinder jar's own layer (its burst of embers) in Niagara when that's the look (Looter.Fx.Niagara, NS_CinderBurst
	// built): a fountain of embers, a fire flare, a plume and a flash of heat on the ground, in place of the code's ember
	// burst; the salt jar's blast under it stays the code's (every kind shares it).
	FGrenadeBurstLook Drawn = Look;
	if (Look.Element == ELooterElement::Ember && !Look.bSmall && NiagaraFX::UseFor(ENiagaraFX::CinderBurst))
	{
		const FVector Up = Blast.Up.IsNearlyZero() ? FVector::UpVector : Blast.Up.GetSafeNormal();
		const FVector Base = Blast.bOnGround ? Blast.Ground : Blast.Center;
		const float Radius = Tuning.Radius * Tuning.LookScale;
		if (NiagaraFX::SpawnAt(&World, ENiagaraFX::CinderBurst, Base, Up, Radius / NiagaraFX::BurstDesignRadius))
		{
			Drawn.Element = ELooterElement::None;
		}
	}
	DrawBlast(Effects, Blast, Tuning, Drawn);
	// The scorch: a dark sooty star burned into the ground where it went off (the rocket's, sized to this burst); the
	// pitch's tar left in a glossy black pool in it.
	const float Scorch = Tuning.ScorchShare * Look.Scorch;
	if (Blast.bOnGround && Scorch > 0.f)
	{
		if (UVfxSubsystem* Vfx = UVfxSubsystem::Get(&World))
		{
			Vfx->AddSplat(Blast.Ground, Blast.Up, EVfxSplat::Scorch, FLinearColor::Black, Tuning.Radius * Tuning.LookScale * Scorch);
			if (Look.GlobSize > 0.f)
			{
				Vfx->AddSplat(Blast.Ground, Blast.Up, EVfxSplat::Gel, Look.GlobTint, Tuning.Radius * Tuning.LookScale * TarPoolShare);
			}
		}
	}
}

void GraveSaltBurst::PlaySounds(UWorld& World, const FVector& Center, const TArray<FGraveSaltTarget>& Targets)
{
	PlaySounds(World, Center, Targets, GrenadeKinds::LookOf(EGrenadeKind::Salt));
}

void GraveSaltBurst::PlaySounds(UWorld& World, const FVector& Center, const TArray<FGraveSaltTarget>& Targets, const FGrenadeBurstLook& Look)
{
	// The thump and the salt's crystalline crackle are one cue, carried far like a gunshot; a kind lays its own over it
	// (the pitch's splatter, the scatter's split, the coals' whoomp), and a small jar's pop is its own alone.
	if (Look.SaltVolume > 0.f)
	{
		LooterSound::PlayAt(&World, ThrowCue::Burst, Center, Look.SaltVolume);
	}
	if (Look.LayerCue)
	{
		LooterSound::PlayAt(&World, Look.LayerCue, Center);
	}
	int32 Sears = 0;
	for (const FGraveSaltTarget& Target : Targets)
	{
		if (Target.bUnpaid && Sears < MaxSears)
		{
			++Sears;
			LooterSound::PlayAt(&World, ThrowCue::Sear, Target.Point, 1.f - 0.15f * (Sears - 1));
		}
	}
}

void GraveSaltBurst::KickViews(UWorld& World, const FVector& Center, FRandomStream& Random)
{
	KickViews(World, Center, Random, 1.f);
}

void GraveSaltBurst::KickViews(UWorld& World, const FVector& Center, FRandomStream& Random, float Scale)
{
	if (Scale <= 0.f)
	{
		return;
	}
	for (FConstPlayerControllerIterator It = World.GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* Player = It->Get();
		const APawn* Pawn = Player && Player->IsLocalController() ? Player->GetPawn() : nullptr;
		if (!Pawn)
		{
			continue;
		}
		const UCameraComponent* Eye = UPlayerViewComponent::FindFirstPersonCamera(Pawn);
		const FVector From = Eye ? Eye->GetComponentLocation() : Pawn->GetPawnViewLocation();
		const float Share = FGraveSaltRules::KickShare(static_cast<float>(FVector::Dist(From, Center))) * Scale;
		if (Share <= 0.f)
		{
			continue;
		}
		UCameraShakeModifier::Kick(Player, ThrowMotion::Burst(Share, Random.FRandRange(-1.f, 1.f)));
		if (UCameraShakeModifier* Modifier = UCameraShakeModifier::FindOrAdd(Player))
		{
			Modifier->AddShake(ThrowMotion::BurstShake * Share, ThrowMotion::BurstShakeSeconds);
		}
	}
}

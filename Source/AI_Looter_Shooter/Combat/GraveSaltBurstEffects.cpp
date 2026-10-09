#include "Combat/GraveSaltBurst.h"
#include "Audio/LooterSound.h"
#include "Combat/BulletSubsystem.h"
#include "Player/CameraShakeModifier.h"
#include "Player/PlayerThrowMotion.h"
#include "Player/PlayerThrowRules.h"
#include "Player/PlayerViewComponent.h"
#include "Weapons/WeaponFX.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

// The grave-salt burst as it's seen, heard and felt: white salt and embers drawn through FWeaponFX (no particle assets),
// the burst's sound and the salt searing the dead, and the jolt on the players' views by distance.

namespace
{
	/** The embers: hot orange going white at the heart, the colour of salt catching fire on the dead. */
	const FLinearColor EmberTint(1.f, 0.45f, 0.14f);
	/** The salt: bone white with a warm cast, a little short of pure white so it never blooms out. */
	const FLinearColor SaltTint(0.92f, 0.9f, 0.84f);
	/** The salt cloud (unlit smoke: the colour it shows in afternoon light). */
	const FLinearColor SaltCloud(0.86f, 0.84f, 0.79f);

	/** How big each part of the burst reads, as FWeaponFX's sizes go (a body's width in cm for the death bursts). */
	constexpr float FlashScale = 3.f;
	constexpr float EmberSize = 170.f;
	constexpr float SaltSize = 150.f;
	/** The ring of salt cloud rolling out along the ground, then the haze left hanging over the spot. */
	constexpr int32 CloudPuffs = 10;
	constexpr int32 HazePuffs = 4;
	constexpr int32 GritPieces = 10;
	/** A burst sears at most this many of the dead at once: past it the sizzles only smear together. */
	constexpr int32 MaxSears = 3;

	FVector AcrossFrom(const FVector& Up, FRandomStream& Random)
	{
		const FVector Any = Random.GetUnitVector();
		const FVector Flat = (Any - Up * FVector::DotProduct(Any, Up)).GetSafeNormal();
		return Flat.IsNearlyZero() ? FVector::ForwardVector : Flat;
	}
}

void GraveSaltBurst::SpawnEffects(UWorld& World, const FVector& Center, const FVector& Up)
{
	UBulletSubsystem* Bullets = World.GetSubsystem<UBulletSubsystem>();
	if (!Bullets)
	{
		return;
	}
	FWeaponFX& Effects = Bullets->GetEffects();
	Effects.Initialize(&World);
	FRandomStream Random(static_cast<int32>(GetTypeHash(Center)));

	// The crack of light as the tin gives, then the embers: a white-hot heart in an orange halo, sparks flung and motes
	// of fire rising off it (the death burst's soul-light, in salt-fire colours).
	Effects.SpawnFlash(Center, Up, FlashScale);
	Effects.SpawnDeathBurst(EDeathBurst::SoulLight, Center, Up, EmberSize, EmberTint);
	// The salt itself: white clumps flung out every way with a glow on them, and a low white spray (the gel burst's
	// shapes, in salt).
	Effects.SpawnDeathBurst(EDeathBurst::Gel, Center, Up, SaltSize, SaltTint);

	// A ring of salt cloud rolling out along the ground...
	for (int32 Index = 0; Index < CloudPuffs; ++Index)
	{
		const FVector Out = AcrossFrom(Up, Random);
		const FVector Velocity = Out * Random.FRandRange(260.f, 440.f) + Up * Random.FRandRange(20.f, 70.f);
		Effects.SpawnDustPuff(Center + Out * 20.f, Velocity, SaltCloud, 0.55f, 30.f, Random.FRandRange(130.f, 190.f), Random.FRandRange(1.1f, 1.7f));
	}
	// ...a haze of it left hanging over the spot, slow to clear, where the dead won't want to walk...
	for (int32 Index = 0; Index < HazePuffs; ++Index)
	{
		const FVector Drift = AcrossFrom(Up, Random) * Random.FRandRange(10.f, 40.f) + Up * Random.FRandRange(8.f, 24.f);
		Effects.SpawnDustPuff(Center + AcrossFrom(Up, Random) * Random.FRandRange(30.f, 90.f), Drift, SaltCloud, 0.32f, 80.f,
			Random.FRandRange(220.f, 300.f), Random.FRandRange(2.8f, 3.6f));
	}
	// ...and grit off the ground and shards of the tin.
	for (int32 Index = 0; Index < GritPieces; ++Index)
	{
		const FVector Out = (Up * Random.FRandRange(0.5f, 1.2f) + Random.GetUnitVector() * 0.8f).GetSafeNormal();
		Effects.SpawnGrit(Center, Out * Random.FRandRange(300.f, 700.f), Random.FRandRange(1.2f, 3.f), Random.FRandRange(0.6f, 1.1f));
	}
}

void GraveSaltBurst::PlaySounds(UWorld& World, const FVector& Center, const TArray<FGraveSaltTarget>& Targets)
{
	// The thump and the salt's crystalline crackle are one cue, carried far like a gunshot.
	LooterSound::PlayAt(&World, ThrowCue::Burst, Center);
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
		const float Share = FGraveSaltRules::KickShare(static_cast<float>(FVector::Dist(From, Center)));
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

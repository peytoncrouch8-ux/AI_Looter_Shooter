// AAbelKeeper's show (FAbelShowRules; his bar's show is AbelRules::MakeShow, played by his boss): his entrance with his
// lantern raised, the stagger crits on his coal bring (down on a knee), his shots from the fog while the deck's lanterns
// are dark, his barrage in the wind, and his coal's flare as he falls. AbelKeeperMoves.cpp moves each on frame by frame.

#include "Bosses/AbelKeeper.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Bosses/AbelPoses.h"
#include "Bosses/BossComponent.h"
#include "Combat/EnemyProjectileSubsystem.h"
#include "Creatures/CreatureRankSettings.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"

namespace
{
	/** The model stands on the capsule's foot: its middle is this far over the ground at size 1 (AUnpaidCreature's capsule). */
	constexpr float ShowModelOverMiddle = 90.f;

	/** His lantern's glow as he raises it at his entrance, and out in the fog (a beacon seen across the canyon) (cm). */
	constexpr float IntroGlow = 22.f;
	constexpr float FogGlow = 34.f;
	const FLinearColor ShowFlareColor(1.f, 0.94f, 0.82f);

	/** His coal's flare as he falls: how long it swells and how big (cm). */
	constexpr float DeathFlareSeconds = 0.5f;
	constexpr float DeathFlareGlow = 36.f;

	/** Where the pump's muzzle socket is (SM_AbelPump). */
	const FName ShowMuzzleSocket(TEXT("Muzzle"));

	UEnemyProjectileSubsystem* PelletsOf(const UObject* Context)
	{
		UWorld* World = Context ? Context->GetWorld() : nullptr;
		return World ? World->GetSubsystem<UEnemyProjectileSubsystem>() : nullptr;
	}
}

// ---------------------------------------------------------------------------
// His entrance
// ---------------------------------------------------------------------------

void AAbelKeeper::BeginIntro()
{
	if (IsDead() || !Boss->IsFighting())
	{
		return;
	}
	// He turns to the player and raises his lantern as his bar sweeps in; its light swells (AbelKeeperMoves.cpp).
	BeginMove(EAbelMove::Intro);
	HoldStill(false);
	if (UEnemyProjectileSubsystem* Shots = PelletsOf(this))
	{
		Shots->ShowCharge(this, AbelPoses::FlareGlobe() - FVector(0.0, 0.0, ShowModelOverMiddle), ShowRules.IntroSeconds * 0.4f, IntroGlow,
			ShowFlareColor);
	}
}

// ---------------------------------------------------------------------------
// His stagger
// ---------------------------------------------------------------------------

bool AAbelKeeper::CanBeStaggered() const
{
	// Not out over the canyon, nor mid-lunge or mid-attack, nor knelt at the end.
	return !IsDead() && !IsOutInFog() && !IsLunging() && GetCreatureState() != ECreatureState::Attack && Move != EAbelMove::Kneel
		&& Move != EAbelMove::Scene;
}

void AAbelKeeper::HandleStaggered(bool bStaggered)
{
	if (bStaggered)
	{
		if (IsDead())
		{
			return;
		}
		// Whatever he was at (his grief, a pull's stun, his walk into the wind), he's down on a knee, his coal still open.
		EndMove();
		BeginMove(EAbelMove::Staggered);
		HoldStill(false);
		UE_LOG(LogLooter, Log, TEXT("%s: staggered, down on a knee, his coal open."), *GetActorNameOrLabel());
		return;
	}
	if (Move == EAbelMove::Staggered)
	{
		EndMove();
		// Up again; in the wind he fights faster, as after a pull.
		if (bWindBlowing)
		{
			ApplyPace(true);
		}
		Rejoin();
	}
}

// ---------------------------------------------------------------------------
// His shots from the fog
// ---------------------------------------------------------------------------

void AAbelKeeper::StartFogShot()
{
	if (Move != EAbelMove::InFog || FogFlareLeft > 0.f || !Boss->GetFightPlayer())
	{
		return;
	}
	// His lantern flares out over the canyon, a beacon: the time to get behind a bier before the buckshot comes.
	FogFlareLeft = ShowRules.FogFlareSeconds;
	if (UEnemyProjectileSubsystem* Shots = PelletsOf(this))
	{
		Shots->ShowCharge(this, AbelPoses::FlareGlobe() - FVector(0.0, 0.0, ShowModelOverMiddle), ShowRules.FogFlareSeconds, FogGlow,
			ShowFlareColor);
	}
	LooterSound::PlayAt(this, LooterSoundCue::AbelFogFlare, GetLanternGlobe());
}

void AAbelKeeper::TickFogShot(float DeltaSeconds)
{
	if (FogFlareLeft <= 0.f)
	{
		return;
	}
	FogFlareLeft = FMath::Max(0.f, FogFlareLeft - DeltaSeconds);
	SetLanternFlare(1.f - FogFlareLeft / FMath::Max(ShowRules.FogFlareSeconds, 0.01f));
	if (FogFlareLeft <= 0.f)
	{
		ReleaseBuckshot();
		SetLanternFlare(0.f);
	}
}

// ---------------------------------------------------------------------------
// His barrage in the wind
// ---------------------------------------------------------------------------

void AAbelKeeper::FireBarrageShot()
{
	const int32 Index = BarrageFired++;
	const APawn* Player = Boss->GetFightPlayer();
	UEnemyProjectileSubsystem* Shots = PelletsOf(this);
	if (!Player || !Shots || IsDead())
	{
		return;
	}
	FVector Muzzle = GetActorTransform().TransformPosition(Buckshot.Muzzle);
	if (Pump && Pump->GetStaticMesh() && Pump->DoesSocketExist(ShowMuzzleSocket))
	{
		Muzzle = Pump->GetSocketLocation(ShowMuzzleSocket);
	}
	// The first at the player, the second where their run takes them, the third back the way they came: running one way
	// all through it doesn't dodge them all.
	FVector AimAt = Player->GetActorLocation();
	const float Flight = static_cast<float>(FVector::Dist(Muzzle, AimAt)) / FMath::Max(Buckshot.Speed, 1.f);
	const FVector Run(Player->GetVelocity().X, Player->GetVelocity().Y, 0.0);
	if (Index == 1)
	{
		AimAt += Run * (Flight * ShowRules.BarrageLead);
	}
	else if (Index >= 2)
	{
		AimAt -= Run * (Flight * ShowRules.BarrageLead * 0.5f);
	}
	FEnemyShot Pellet;
	Pellet.Start = Muzzle;
	Pellet.Speed = Buckshot.Speed;
	Pellet.Range = Buckshot.Range;
	Pellet.Radius = Buckshot.Radius;
	// A share of his bite, as his single shot's.
	Pellet.Damage = AttackDamage * Buckshot.DamageShare;
	Pellet.Color = Buckshot.Color;
	Pellet.Shooter = this;
	Pellet.Instigator = GetController();
	Shots->FireVolley(Pellet, AimAt - Muzzle, ShowRules.BarragePellets, ShowRules.BarrageSpread);
	// The spectral pump's report, a little higher with each shot.
	LooterSound::PlayAt(this, LooterSoundCue::ShotgunFire, Muzzle, 1.f, 0.75f + 0.05f * static_cast<float>(Index));
}

// ---------------------------------------------------------------------------
// His fall
// ---------------------------------------------------------------------------

void AAbelKeeper::FlareCoalAtDeath()
{
	if (UEnemyProjectileSubsystem* Shots = PelletsOf(this))
	{
		Shots->ShowCharge(this, GetActorTransform().InverseTransformPosition(GetCoalLocation()), DeathFlareSeconds, DeathFlareGlow,
			UCreatureRankSettings::Get(GetRank()).Color);
	}
}

#include "Embers/Powers/EmberDoubleOrNothing.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Core/LooterCharacter.h"
#include "Combat/BulletSubsystem.h"
#include "Embers/EmberDefinition.h"
#include "Embers/Powers/EmberVfx.h"
#include "Player/ViewKick.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponFX.h"
#include "CoreGlobals.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"

namespace
{
	const FName OddsName(TEXT("Odds"));
	const FName MissStreakName(TEXT("MissStreak"));

	/** A miss's tracer: a dull, cooling grey with a little red in it. */
	const FLinearColor NothingGrey(0.42f, 0.33f, 0.3f);
	/** The coin: gold, flicked up, spinning, caught again about a hand's height over where it left. */
	const FLinearColor CoinGold(1.f, 0.78f, 0.3f);
	constexpr float CoinSeconds = 0.55f;
	constexpr float CoinGravity = 1500.f;
}

void UEmberDoubleOrNothing::Activate(const FEmberContext& Context)
{
	if (!BeginUse(Context))
	{
		return;
	}
	const UEmberDefinition* Def = Context.Definition;
	Strength = Def && Def->Strength > 0.f ? Def->Strength : DefaultStrength;
	Odds = FMath::Clamp(Def ? Def->Extra(OddsName, DefaultOdds) : DefaultOdds, 0.f, 1.f);
	MissStreak = FMath::Max(FMath::RoundToInt(Def ? Def->Extra(MissStreakName, DefaultMissStreak) : DefaultMissStreak), 0);
	Heads = Tails = TailsRunning = 0;
	bLastHeads = true;
	LastPullFrame = TNumericLimits<uint64>::Max();
	LastPullWeapon.Reset();
	bGambling = true;
	Bind(true);

	LooterSound::Play2D(Body.Get(), LooterSoundCue::Ember::DoubleOrNothingStart);
	ShowStart();
	StartLinger();
	UE_LOG(LogLooter, Log, TEXT("Double or Nothing: x%.1f or nothing at %.0f%% heads, never more than %d miss(es) running"), Strength, Odds * 100.f,
		MissStreak);
}

void UEmberDoubleOrNothing::Tick(const FEmberContext& Context, float DeltaSeconds)
{
	// The coin and the glints run on the linger.
}

void UEmberDoubleOrNothing::End(const FEmberContext& Context, bool bInterrupted)
{
	if (!bGambling)
	{
		return;
	}
	bGambling = false;
	Bind(false);
	UE_LOG(LogLooter, Log, TEXT("Double or Nothing: %s, %d heads and %d tails"), bInterrupted ? TEXT("cut short") : TEXT("over"), Heads, Tails);
}

void UEmberDoubleOrNothing::Bind(bool bOn)
{
	if (UBulletSubsystem* Old = Bullets.Get())
	{
		Old->OnBulletFiring.Remove(FiringHandle);
	}
	FiringHandle.Reset();
	Bullets.Reset();
	UWorld* World = GetBodyWorld();
	UBulletSubsystem* Subsystem = World ? World->GetSubsystem<UBulletSubsystem>() : nullptr;
	if (bOn && Subsystem)
	{
		Bullets = Subsystem;
		FiringHandle = Subsystem->OnBulletFiring.AddUObject(this, &UEmberDoubleOrNothing::NoteFiring);
	}
}

bool UEmberDoubleOrNothing::TossHeads()
{
	// The house leans her way: after MissStreak tails running, it's heads.
	const bool bHeads = (MissStreak > 0 && TailsRunning >= MissStreak) || Random.FRand() < Odds;
	TailsRunning = bHeads ? 0 : TailsRunning + 1;
	return bHeads;
}

void UEmberDoubleOrNothing::NoteFiring(FBulletShot& Shot)
{
	if (!bGambling || !Body.IsValid() || Shot.Shooter.Get() != Body.Get())
	{
		return;
	}
	// One toss a pull: a shotgun's pellets go together, all double or all nothing.
	if (GFrameCounter != LastPullFrame || Shot.Weapon.Get() != LastPullWeapon.Get())
	{
		LastPullFrame = GFrameCounter;
		LastPullWeapon = Shot.Weapon.Get();
		bLastHeads = TossHeads();
		if (bLastHeads)
		{
			++Heads;
			LooterSound::Play2D(Body.Get(), LooterSoundCue::Ember::DoubleOrNothingDouble, 0.85f, Random.FRandRange(0.97f, 1.04f));
			for (int32 Index = 0; Index < 7; ++Index)
			{
				const FVector Out = (Shot.Direction + Random.GetUnitVector() * 0.6).GetSafeNormal();
				Sparks.Spawn(Shot.VisualStart, Out * Random.FRandRange(180.f, 420.f), Random.FRandRange(0.15f, 0.3f), 1.2f, Color, 22.f, 300.f, 4.f);
			}
			// (The VFX pass) heads: a hard star of its colour over the flash, so a doubled shot reads as one.
			if (FWeaponFX* FX = Effects())
			{
				FX->SpawnStarburst(Shot.VisualStart + Shot.Direction * 8.f, 40.f, Color, 22.f, 0.08f, 6);
			}
		}
		else
		{
			++Tails;
			LooterSound::Play2D(Body.Get(), LooterSoundCue::Ember::DoubleOrNothingNothing, 0.8f, Random.FRandRange(0.96f, 1.03f));
			if (FWeaponFX* FX = Effects())
			{
				// A dud's breath of grey at the muzzle.
				FX->SpawnWisp(Shot.VisualStart, Shot.Direction * 60.f + FVector(0.0, 0.0, 20.0), 0.18f, 4.f, 16.f, 0.5f);
			}
		}
	}
	if (bLastHeads)
	{
		Shot.Damage *= Strength;
		Shot.TracerColor = Color;
		Shot.TracerWidth *= 1.5f;
		return;
	}
	// Nothing: thrown wide off the aim, and harmless wherever it goes.
	const FVector Way = Shot.Direction.GetSafeNormal();
	FVector Axis = FVector::CrossProduct(Way, Random.GetUnitVector()).GetSafeNormal();
	if (Axis.IsNearlyZero())
	{
		Axis = FVector::CrossProduct(Way, FVector::UpVector).GetSafeNormal();
	}
	Shot.Direction = Way.RotateAngleAxis(Random.FRandRange(MissMinDegrees, MissMaxDegrees), Axis);
	Shot.bBlank = true;
	Shot.TracerColor = NothingGrey;
	Shot.TracerWidth *= 0.7f;
}

bool UEmberDoubleOrNothing::Linger(float DeltaSeconds)
{
	const float Dt = FMath::Max(DeltaSeconds, 0.f);
	FWeaponFX* FX = Effects();
	if (CoinAge >= 0.f)
	{
		// The coin in the air: a bright spinning sliver, rising and falling back into the hand.
		CoinAge += Dt;
		CoinVelocity.Z -= CoinGravity * Dt;
		CoinAt += CoinVelocity * Dt;
		if (FX)
		{
			const float Face = FMath::Abs(FMath::Sin(CoinAge * 38.f));
			FX->AddStreak(CoinAt - FVector(0.0, 0.0, 1.0 + 1.6 * Face), CoinAt + FVector(0.0, 0.0, 1.0 + 1.6 * Face), 1.2f + 2.6f * Face, CoinGold,
				18.f + 14.f * Face);
		}
		if (CoinAge >= CoinSeconds)
		{
			// Slapped down: a glint where it's caught (the pass: a small gold star; and the bet's ring racing out from his feet).
			for (int32 Index = 0; Index < 8; ++Index)
			{
				Sparks.Spawn(CoinAt, Random.GetUnitVector() * Random.FRandRange(120.f, 260.f), Random.FRandRange(0.15f, 0.3f), 1.1f, CoinGold, 18.f, 200.f, 3.f);
			}
			if (FX)
			{
				FX->SpawnStarburst(CoinAt, 26.f, CoinGold, 18.f, 0.08f, 6);
				if (const ACharacter* Character = Body.Get())
				{
					EmberVfx::Activation(*FX, FeetOf(*Character), Color, 0.6f);
				}
			}
			CoinAge = -1.f;
		}
	}
	Sparks.Tick(Dt, FX);
	return bGambling || CoinAge >= 0.f || !Sparks.IsEmpty();
}

void UEmberDoubleOrNothing::ShowStart()
{
	ACharacter* Character = Body.Get();
	if (!Character)
	{
		return;
	}
	// The coin flicked up from the hand, out in front where it's seen, to come down a little lower.
	CoinAt = HandPoint(*Character, 10.f, 14.f, 60.f);
	CoinVelocity = FVector(0.0, 0.0, CoinGravity * CoinSeconds * 0.55f);
	CoinAge = 0.f;
	for (int32 Index = 0; Index < 12; ++Index)
	{
		Sparks.Spawn(CoinAt, Random.GetUnitVector() * Random.FRandRange(100.f, 260.f), Random.FRandRange(0.3f, 0.55f), 1.2f, Color, 16.f, 60.f, 2.f);
	}
	FViewKick Toss;
	Toss.Pitch = 0.4f;
	Toss.Frequency = 5.f;
	Toss.Damping = 0.7f;
	Kick(Toss);
}

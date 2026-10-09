// AFaunaSwarm's insects on the move, each kind in its own way: a butterfly's fluttering wander and its rests on the
// flowers, a dragonfly's hover and dart, a firefly's drift and flash, a fly's quick loops.

#include "World/FaunaSwarm.h"
#include "Audio/LooterSound.h"

namespace
{
	/** Butterflies: how long a heading lasts (s), the chance a new one is a rest on a flower, and how long that is. */
	constexpr float ButterflyLegMin = 0.8f;
	constexpr float ButterflyLegMax = 2.6f;
	constexpr float ButterflyRestChance = 0.15f;
	constexpr float ButterflyRestMin = 2.f;
	constexpr float ButterflyRestMax = 5.f;

	/** Dragonflies: how long one hovers (s), and how long a dart takes. */
	constexpr float DragonflyHoverMin = 0.6f;
	constexpr float DragonflyHoverMax = 2.6f;
	constexpr float DragonflyDartMin = 0.25f;
	constexpr float DragonflyDartMax = 0.6f;
	/** Between two of its darts' rattles (s). */
	constexpr float DartSoundSeconds = 1.5f;

	/** Fireflies: a flash's length and the dark between (s). */
	constexpr float FlashMin = 0.25f;
	constexpr float FlashMax = 0.6f;
	constexpr float DarkMin = 1.f;
	constexpr float DarkMax = 4.f;

	/** Flies: how often one changes its mind (s), and how far it strays from its zone's middle (cm). */
	constexpr float FlyTurnMin = 0.15f;
	constexpr float FlyTurnMax = 0.5f;
	constexpr float FlyStrayMin = 30.f;
	constexpr float FlyStrayMax = 120.f;

	/** Faces along a velocity, pitched with its climb (at most MaxPitch degrees). */
	void FaceAlong(FFaunaInsect& Insect, const FVector& Velocity, float MaxPitch)
	{
		if (Velocity.SizeSquared2D() > 1.0)
		{
			Insect.Yaw = static_cast<float>(FMath::RadiansToDegrees(FMath::Atan2(Velocity.Y, Velocity.X)));
			const double Ground = Velocity.Size2D();
			Insect.Pitch = FMath::Clamp(static_cast<float>(FMath::RadiansToDegrees(FMath::Atan2(Velocity.Z, Ground))), -MaxPitch, MaxPitch);
		}
	}
}

void AFaunaSwarm::MoveButterfly(FFaunaInsect& Insect, float DeltaSeconds, const FFaunaContext& Context)
{
	const FFaunaZone& Zone = Zones[Insect.Zone];
	if (Insect.Rest > 0.f)
	{
		// On a flower: the wings slowly opening and closing.
		Insect.Rest -= DeltaSeconds;
		Insect.FlapPhase = FMath::Fmod(Insect.FlapPhase + UE_TWO_PI * 0.4f * DeltaSeconds, UE_TWO_PI);
		Insect.Velocity = FVector::ZeroVector;
		if (ThreatNear(Insect.Position, Context) != INDEX_NONE)
		{
			Insect.Rest = 0.f;
		}
		return;
	}
	Insect.Timer -= DeltaSeconds;
	const int32 Threat = ThreatNear(Insect.Position, Context);
	float Pace = Speed;
	if (Threat != INDEX_NONE)
	{
		// Up and away over the player's head.
		FVector Away = Insect.Position - Context.Threats[Threat].Location;
		Away.Z = 0.0;
		Insect.Goal = Insect.Position + Away.GetSafeNormal() * 300.0 + FVector(0.0, 0.0, 120.0);
		Insect.Timer = 1.f;
		Insect.Rest = 0.f;
		Pace = Speed * 2.f;
	}
	else if (Insect.Rest < 0.f)
	{
		// Heading down to a flower; it gives up if it can't settle within a few seconds.
		if (Insect.Timer < -3.f)
		{
			Insect.Rest = 0.f;
		}
	}
	else if (Insect.Timer <= 0.f || FVector::DistSquared(Insect.Position, Insect.Goal) < 400.0)
	{
		Insect.Timer = Random.FRandRange(ButterflyLegMin, ButterflyLegMax);
		Insect.Goal = PointIn(Zone);
		if (Random.FRand() < ButterflyRestChance)
		{
			// Down to a flower head, low in the zone, to rest when it gets there.
			Insect.Goal.Z = Zone.Center.Z + Zone.MinHeight * 0.6f;
			Insect.Rest = -1.f;
		}
	}
	// Fluttering: steered toward its goal, jinking side to side and bobbing with each beat.
	const FVector ToGoal = Insect.Goal - Insect.Position;
	const FVector Toward = ToGoal.GetSafeNormal();
	// Slowing and steadying as it comes in, so it can settle on a flower head.
	const float Close = FMath::Clamp(static_cast<float>(ToGoal.Size()) / 150.f, 0.25f, 1.f);
	const float T = Insect.Age;
	const FVector Jink(FMath::Sin(T * 3.1f + Insect.Phase.X), FMath::Sin(T * 2.7f + Insect.Phase.Y), 0.6f * FMath::Sin(T * 4.3f + Insect.Phase.Z));
	const FVector Desired = Toward * (Pace * Close) + Jink * (Pace * 0.6f * Close * Close);
	Insect.Velocity = FMath::Lerp(Insect.Velocity, Desired, FMath::Min(1.f, DeltaSeconds * 3.f));
	Insect.FlapPhase = FMath::Fmod(Insect.FlapPhase + UE_TWO_PI * Insect.FlapRate * DeltaSeconds, UE_TWO_PI);
	Insect.Position += Insect.Velocity * DeltaSeconds + FVector(0.0, 0.0, 2.5 * FMath::Cos(Insect.FlapPhase) * DeltaSeconds * Insect.FlapRate);
	// Never under its zone's floor (the flowers' heads), never far over its ceiling.
	const double Floor = Zone.Center.Z + Zone.MinHeight * 0.5f;
	Insect.Position.Z = FMath::Clamp(Insect.Position.Z, Floor, Zone.Center.Z + Zone.MaxHeight + 150.0);
	FaceAlong(Insect, Insect.Velocity, 25.f);
	if (Insect.Rest < 0.f && FVector::DistSquared(Insect.Position, Insect.Goal) < 100.0)
	{
		Insect.Rest = Random.FRandRange(ButterflyRestMin, ButterflyRestMax);
		Insect.Pitch = 0.f;
	}
}

void AFaunaSwarm::MoveDragonfly(FFaunaInsect& Insect, float DeltaSeconds, const FFaunaContext& Context)
{
	const FFaunaZone& Zone = Zones[Insect.Zone];
	const int32 Threat = ThreatNear(Insect.Position, Context);
	if (Insect.DartDuration > 0.f)
	{
		// A dart: off like a shot and stopping dead, eased at both ends.
		Insect.DartTime += DeltaSeconds;
		const float Alpha = FMath::Clamp(Insect.DartTime / Insect.DartDuration, 0.f, 1.f);
		const FVector Before = Insect.Position;
		Insect.Position = FMath::Lerp(Insect.DartFrom, Insect.Goal, FMath::InterpEaseInOut(0.f, 1.f, Alpha, 2.5f));
		Insect.Velocity = DeltaSeconds > 0.f ? (Insect.Position - Before) / DeltaSeconds : FVector::ZeroVector;
		if (Alpha >= 1.f)
		{
			Insect.DartDuration = 0.f;
			Insect.Timer = Random.FRandRange(DragonflyHoverMin, DragonflyHoverMax);
			Insect.Velocity = FVector::ZeroVector;
		}
		Insect.Pitch = FMath::FInterpTo(Insect.Pitch, 0.f, DeltaSeconds, 8.f);
		return;
	}
	// Hovering: still but for a tremble, turning a little now and then.
	Insect.Timer -= DeltaSeconds;
	Insect.Roll = Random.FRandRange(-3.f, 3.f);
	if (Insect.Timer > 0.f && Threat == INDEX_NONE)
	{
		Insect.Position += FVector(Random.FRandRange(-1.f, 1.f), Random.FRandRange(-1.f, 1.f), Random.FRandRange(-0.6f, 0.6f)) * (40.f * DeltaSeconds);
		return;
	}
	Insect.DartFrom = Insect.Position;
	if (Threat != INDEX_NONE)
	{
		FVector Away = Insect.Position - Context.Threats[Threat].Location;
		Away.Z = 0.0;
		Insect.Goal = Insect.Position + Away.GetSafeNormal() * 350.0;
		Insect.Goal.Z = Zone.Center.Z + Random.FRandRange(Zone.MinHeight, FMath::Max(Zone.MinHeight, Zone.MaxHeight));
	}
	else
	{
		// Somewhere near over the water, mostly along it.
		const FVector Next = PointIn(Zone);
		Insect.Goal = FMath::Lerp(Insect.Position, Next, Random.FRandRange(0.3f, 1.f));
		Insect.Goal.Z = Next.Z;
	}
	const float Distance = static_cast<float>(FVector::Dist(Insect.DartFrom, Insect.Goal));
	Insect.DartDuration = FMath::Clamp(Distance / FMath::Max(Speed, 1.f), DragonflyDartMin, DragonflyDartMax);
	Insect.DartTime = 0.f;
	// It turns before it goes: dragonflies fly the way they face.
	FaceAlong(Insect, Insect.Goal - Insect.DartFrom, 15.f);
	if (!SoundCue.IsNone() && DartSoundCooldown <= 0.f && IsFaunaShown())
	{
		for (const FFaunaThreat& Player : Context.Threats)
		{
			if (Player.bPlayer && FVector::DistSquared(Player.Location, Insect.Position) < FMath::Square(SoundRange))
			{
				LooterSound::PlayAt(this, SoundCue, Insect.Position);
				DartSoundCooldown = DartSoundSeconds;
				break;
			}
		}
	}
}

void AFaunaSwarm::MoveFirefly(FFaunaInsect& Insect, float DeltaSeconds)
{
	const FFaunaZone& Zone = Zones[Insect.Zone];
	// A slow drift on its own wandering currents, drawn back when it strays from its zone.
	const float T = Insect.Age;
	FVector Drift(FMath::Sin(T * 0.7f + Insect.Phase.X), FMath::Cos(T * 0.6f + Insect.Phase.Y), 0.5f * FMath::Sin(T * 0.9f + Insect.Phase.Z));
	const FVector ToCenter = Zone.Center + FVector(0.0, 0.0, 0.5 * (Zone.MinHeight + Zone.MaxHeight)) - Insect.Position;
	const double Stray = ToCenter.Size2D() / FMath::Max(Zone.Radius, 1.f);
	if (Stray > 0.8)
	{
		Drift += ToCenter.GetSafeNormal2D() * (Stray - 0.8) * 4.0;
	}
	Insect.Velocity = FMath::Lerp(Insect.Velocity, Drift * Speed, FMath::Min(1.f, DeltaSeconds));
	Insect.Position += Insect.Velocity * DeltaSeconds;
	Insect.Position.Z = FMath::Clamp(Insect.Position.Z, Zone.Center.Z + Zone.MinHeight, Zone.Center.Z + Zone.MaxHeight);
	// Its flash: a soft swell and fade, then the dark.
	Insect.Timer -= DeltaSeconds;
	if (Insect.Timer <= 0.f)
	{
		Insect.bLit = !Insect.bLit;
		Insect.Timer = Insect.bLit ? Random.FRandRange(FlashMin, FlashMax) : Random.FRandRange(DarkMin, DarkMax);
		Insect.DartDuration = Insect.Timer;
	}
	Insect.Glow = Insect.bLit ? FMath::Sin(UE_PI * (1.f - Insect.Timer / FMath::Max(Insect.DartDuration, 0.01f))) : 0.f;
}

void AFaunaSwarm::MoveFly(FFaunaInsect& Insect, float DeltaSeconds)
{
	const FFaunaZone& Zone = Zones[Insect.Zone];
	Insect.Timer -= DeltaSeconds;
	if (Insect.Timer <= 0.f)
	{
		// A new mind: a point close round the zone's middle, at any height in it.
		Insect.Timer = Random.FRandRange(FlyTurnMin, FlyTurnMax);
		const float Angle = Random.FRandRange(0.f, UE_TWO_PI);
		const float Stray = FMath::Min(Random.FRandRange(FlyStrayMin, FlyStrayMax), FMath::Max(Zone.Radius, FlyStrayMin));
		Insect.Goal = Zone.Center + FVector(FMath::Cos(Angle) * Stray, FMath::Sin(Angle) * Stray,
			Random.FRandRange(Zone.MinHeight, FMath::Max(Zone.MinHeight, Zone.MaxHeight)));
	}
	const FVector Desired = (Insect.Goal - Insect.Position).GetSafeNormal() * Speed;
	Insect.Velocity = FMath::Lerp(Insect.Velocity, Desired, FMath::Min(1.f, DeltaSeconds * 8.f));
	Insect.Position += Insect.Velocity * DeltaSeconds;
	FaceAlong(Insect, Insect.Velocity, 30.f);
}

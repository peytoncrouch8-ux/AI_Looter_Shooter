// UAmbientAirSubsystem's motion: each kind's drift (motes wandering slowly, pollen bobbing, fluff riding the breeze,
// leaves swaying and tumbling down to lie a moment on the ground, moths circling their lamp), carried by the breeze and
// the gust; done ones go back to the pool; one mote a frame looks at the sun again.

#include "World/AmbientAirSubsystem.h"
#include "World/AmbientAirRules.h"

namespace
{
	/** A leaf lies on the ground this long (s) before it goes. */
	constexpr float LieMin = 1.f;
	constexpr float LieMax = 1.8f;
	/** Past its reach by this share, or this far above or below the view (cm), it's put back in the pool. */
	constexpr float ReachSlack = 1.2f;
	constexpr float HeightSlack = 2000.f;
}

void UAmbientAirSubsystem::Move(const FAirView& View, float DeltaSeconds)
{
	for (FAirParticle& Particle : Particles)
	{
		if (!Particle.bLive)
		{
			continue;
		}
		Particle.Age += DeltaSeconds;
		if (Particle.Leaving > 0.f)
		{
			Particle.Leaving -= DeltaSeconds;
			if (Particle.Leaving <= 0.f)
			{
				Retire(Particle);
				continue;
			}
		}
		MoveOne(Particle, View, DeltaSeconds);
		if (IsDone(Particle, View))
		{
			Retire(Particle);
		}
	}
}

void UAmbientAirSubsystem::MoveOne(FAirParticle& Particle, const FAirView& View, float DeltaSeconds)
{
	const FAirKindLook& Look = AmbientAirRules::Look(Particle.Kind);
	// The breeze and the gust carry it; under a roof the motes feel them much less.
	const bool bIndoors = Particle.Kind == EAirKind::Mote && View.bUnderCover;
	const FVector Wind = View.BreezeDirection * (Look.Breeze * (bIndoors ? 0.2f : 1.f))
		+ View.GustDirection * (Look.GustCarry * View.GustStrength * (bIndoors ? 0.4f : 1.f));
	// How quickly it takes up the air's way: quicker in a gust.
	const float Take = 1.f - FMath::Exp(-(1.5f + 4.f * View.GustStrength) * DeltaSeconds);
	const float T = Particle.Age;

	switch (Particle.Kind)
	{
	case EAirKind::Mote:
	case EAirKind::Pollen:
	case EAirKind::Fluff:
	{
		// A slow wander that changes its mind every few seconds; pollen bobs, fluff bobs slower and sinks a little.
		Particle.Timer -= DeltaSeconds;
		if (Particle.Timer <= 0.f)
		{
			Particle.Timer = Random.FRandRange(1.5f, 3.5f);
			Particle.Wander = Random.GetUnitVector() * (Look.Wander * Random.FRandRange(0.4f, 1.f));
			Particle.Wander.Z *= 0.5;
		}
		FVector Goal = Particle.Wander + Wind;
		if (Particle.Kind == EAirKind::Pollen)
		{
			Goal.Z += 5.f * FMath::Sin(1.7f * T + static_cast<float>(Particle.Phase.X));
		}
		else if (Particle.Kind == EAirKind::Fluff)
		{
			Goal.Z += 7.f * FMath::Sin(0.9f * T + static_cast<float>(Particle.Phase.Y)) - 2.5f;
		}
		Particle.Velocity = FMath::Lerp(Particle.Velocity, Goal, Take);
		Particle.Location += Particle.Velocity * DeltaSeconds;
		// Pollen and fluff never sink into the grass.
		if (Particle.Kind != EAirKind::Mote && Particle.Location.Z < Particle.Floor + 15.f)
		{
			Particle.Location.Z = Particle.Floor + 15.f;
			Particle.Velocity.Z = FMath::Max(Particle.Velocity.Z, 0.0);
		}
		break;
	}
	case EAirKind::Leaf:
	{
		if (Particle.Landed >= 0.f)
		{
			// On the ground a moment, then it goes.
			Particle.Landed += DeltaSeconds;
			if (Particle.Leaving <= 0.f && Particle.Landed >= Particle.Timer)
			{
				Particle.Leaving = 1.f;
			}
			break;
		}
		// Swaying side to side as it falls, dropping fastest through the middle of each sway, tumbling as it goes.
		const float Period = 1.6f + static_cast<float>(Particle.Phase.X) * 0.2f;
		const float Swing = FMath::Sin(T * UE_TWO_PI / Period + static_cast<float>(Particle.Phase.Y));
		const float Fall = 40.f + 30.f * static_cast<float>(Particle.Phase.Z) / UE_TWO_PI;
		const FVector Goal = Wind + Particle.Sway * (55.f * Swing) + FVector(0.0, 0.0, -Fall * (0.75f + 0.35f * FMath::Abs(Swing)));
		Particle.Velocity = FMath::Lerp(Particle.Velocity, Goal, 1.f - FMath::Exp(-3.f * DeltaSeconds));
		Particle.Location += Particle.Velocity * DeltaSeconds;
		const float SpinRate = static_cast<float>(Particle.Spin.Size()) * (1.f + 2.f * View.GustStrength);
		if (SpinRate > 0.f)
		{
			Particle.Rotation = FQuat(Particle.Spin.GetSafeNormal(), SpinRate * DeltaSeconds) * Particle.Rotation;
		}
		if (Particle.Location.Z <= Particle.Floor + 0.5f)
		{
			// Down: flat on the ground, a little askew.
			Particle.Location.Z = Particle.Floor + 0.5f;
			Particle.Velocity = FVector::ZeroVector;
			Particle.Landed = 0.f;
			Particle.Timer = Random.FRandRange(LieMin, LieMax);
			Particle.Rotation = FQuat(FVector::UpVector, Random.FRandRange(0.f, UE_TWO_PI)) * FQuat(FVector::ForwardVector, Random.FRandRange(-0.15f, 0.15f));
		}
		break;
	}
	case EAirKind::Moth:
	{
		if (!Lamps.IsValidIndex(Particle.Anchor))
		{
			Particle.Leaving = FMath::Max(Particle.Leaving, 0.3f);
			break;
		}
		// Round and round the lamp, changing course every moment: further out, closer in (now and then a bump against
		// it), higher, lower, quicker, the other way round.
		Particle.Timer -= DeltaSeconds;
		if (Particle.Timer <= 0.f)
		{
			Particle.Timer = Random.FRandRange(0.4f, 1.6f);
			const bool bBump = Random.FRand() < 0.15f;
			const float Way = Particle.OrbitSpeed < 0.f ? -1.f : 1.f;
			const float Speed = Random.FRandRange(1.8f, 5.f) * (Random.FRand() < 0.2f ? -Way : Way);
			Particle.Wander = FVector(bBump ? 8.f : Random.FRandRange(20.f, 90.f), Random.FRandRange(Look.HeightMin, Look.HeightMax), Speed);
		}
		Particle.Orbit = FMath::FInterpTo(Particle.Orbit, static_cast<float>(Particle.Wander.X), DeltaSeconds, 3.f);
		Particle.Height = FMath::FInterpTo(Particle.Height, static_cast<float>(Particle.Wander.Y), DeltaSeconds, 3.f);
		Particle.OrbitSpeed = FMath::FInterpTo(Particle.OrbitSpeed, static_cast<float>(Particle.Wander.Z), DeltaSeconds, 2.f);
		Particle.OrbitAngle = FMath::Fmod(Particle.OrbitAngle + Particle.OrbitSpeed * DeltaSeconds, UE_TWO_PI);
		const FVector Lamp = Lamps[Particle.Anchor].Location + View.GustDirection * (25.f * View.GustStrength);
		const FVector Flutter(4.f * FMath::Sin(13.f * T + static_cast<float>(Particle.Phase.X)), 4.f * FMath::Sin(11.f * T + static_cast<float>(Particle.Phase.Y)),
			4.f * FMath::Sin(17.f * T + static_cast<float>(Particle.Phase.Z)));
		const FVector Next = Lamp + FVector(FMath::Cos(Particle.OrbitAngle) * Particle.Orbit, FMath::Sin(Particle.OrbitAngle) * Particle.Orbit, Particle.Height) + Flutter;
		Particle.Velocity = DeltaSeconds > 0.f ? (Next - Particle.Location) / DeltaSeconds : FVector::ZeroVector;
		Particle.Location = Next;
		break;
	}
	default:
		break;
	}
}

bool UAmbientAirSubsystem::IsDone(const FAirParticle& Particle, const FAirView& View) const
{
	if (Particle.Age >= Particle.Life || !View.bValid)
	{
		return true;
	}
	const float Reach = AmbientAirRules::Reach(Particle.Kind, View.bUnderCover) * ReachSlack;
	if (FVector::DistSquared2D(Particle.Location, View.Location) > FMath::Square(static_cast<double>(Reach)))
	{
		return true;
	}
	return AmbientAirRules::IsPlacedByProbe(Particle.Kind) && FMath::Abs(Particle.Location.Z - View.Location.Z) > HeightSlack;
}

void UAmbientAirSubsystem::Retire(FAirParticle& Particle)
{
	if (Particle.bLive)
	{
		Particle.bLive = false;
		--Counts[static_cast<int32>(Particle.Kind)];
	}
}

void UAmbientAirSubsystem::RecheckSun()
{
	// One mote a frame looks at the sun again: drifting, it may have crossed into shade (and out of sight) or out of it.
	if (Particles.IsEmpty() || CountOf(EAirKind::Mote) == 0 || (!PlaceProbe && TracesLeft <= 0))
	{
		return;
	}
	for (int32 Offset = 0; Offset < Particles.Num(); ++Offset)
	{
		SunCursor = (SunCursor + 1) % Particles.Num();
		FAirParticle& Particle = Particles[SunCursor];
		if (Particle.bLive && Particle.Kind == EAirKind::Mote)
		{
			FAirPlace Found;
			if (Probe(Particle.Location, false, true, Found))
			{
				Particle.bInSun = Found.bInSun;
			}
			return;
		}
	}
}

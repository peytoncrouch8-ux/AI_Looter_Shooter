#include "Weapons/WeaponFX.h"
#include "Weapons/VfxRules.h"
#include "Weapons/VfxTuning.h"
#include "Components/InstancedStaticMeshComponent.h"

// FWeaponFX's moving and drawing: every particle advanced (falling, dragging, tumbling, landing on its impact's floor)
// and drawn as instanced quads (turned to the camera) and lit chunks. The spawns are in WeaponFX.cpp and its topic files.
// The VFX pass (Saved/Vfx/PLAN.md): rings, fireballs and starbursts on M_FX_Shape's quads (or stand-ins on the glows), the
// puffs' seed and erosion for M_FX_Puff, and drops stretched by their speed into streaks of spray.

namespace
{
	constexpr float FXGravity = 980.f;
	/** A chunk lying on its floor goes after this long (seconds, at random between). */
	constexpr float FXRestMin = 0.9f;
	constexpr float FXRestMax = 2.f;
	/** Under this speed (cm/s), or after this many bounces, a landed chunk lies still. */
	constexpr float FXRestSpeed = 45.f;
	constexpr uint8 FXMostBounces = 3;

	/** Any unit vector at right angles to Direction. */
	FVector FXPerpendicular(const FVector& Direction)
	{
		const FVector Other = FMath::Abs(Direction.Z) < 0.9f ? FVector::UpVector : FVector::ForwardVector;
		return FVector::CrossProduct(Direction, Other).GetSafeNormal();
	}

	/** A quad along Tail -> Head, turned about that line to face the camera. The engine plane is 100 x 100 in XY, facing +Z. */
	FTransform StreakTransform(const FVector& Tail, const FVector& Head, float Width, const FVector& Camera)
	{
		const FVector Axis = Head - Tail;
		const float Length = Axis.Size();
		const FVector Direction = Length > UE_KINDA_SMALL_NUMBER ? Axis / Length : FVector::ForwardVector;
		const FVector Middle = (Tail + Head) * 0.5f;
		const FVector ToCamera = Camera - Middle;
		FVector Facing = ToCamera - Direction * FVector::DotProduct(ToCamera, Direction);
		if (!Facing.Normalize())
		{
			Facing = FXPerpendicular(Direction);
		}
		const FQuat Rotation = FRotationMatrix::MakeFromXZ(Direction, Facing).ToQuat();
		return FTransform(Rotation, Middle, FVector(FMath::Max(Length, 0.1f) / 100.f, Width / 100.f, 1.f));
	}

	/** A round quad facing the camera. */
	FTransform SpriteTransform(const FVector& Location, float Size, float Roll, const FVector& Camera)
	{
		FVector Facing = Camera - Location;
		if (!Facing.Normalize())
		{
			Facing = FVector::UpVector;
		}
		const FQuat Rotation = FRotationMatrix::MakeFromZ(Facing).ToQuat() * FQuat(FVector::UpVector, Roll);
		return FTransform(Rotation, Location, FVector(Size / 100.f, Size / 100.f, 1.f));
	}

	void AddGlowData(TArray<float>& Data, const FLinearColor& Color, float Intensity, float Shape)
	{
		Data.Append({ Color.R, Color.G, Color.B, Intensity, Shape });
	}

	/**
	 * A soft streak's quad is this many times its width: M_FX_Muzzle's streak falls off well inside its quad (no hard edge),
	 * so the light shows about as wide as asked.
	 */
	constexpr float SoftStreakWiden = 2.f;

	/** M_FX_Glow's shapes (its custom data's last float). */
	constexpr float GlowStreak = 0.f;
	constexpr float GlowRound = 1.f;
	constexpr float GlowStar = 2.f;

	/** A drop flying this fast (cm/s) is drawn this many times longer per cm/s, at most SprayMostStretch: a spurt reads as spray. */
	constexpr float SprayStretch = 0.0012f;
	constexpr float SprayMostStretch = 2.2f;

	/** A quad square to Axis (the engine plane faces +Z), Size cm across, turned Roll about the axis. */
	FTransform FlatTransform(const FVector& Location, const FVector& Axis, float Size, float Roll)
	{
		const FQuat Rotation = FRotationMatrix::MakeFromZ(Axis).ToQuat() * FQuat(FVector::UpVector, Roll);
		return FTransform(Rotation, Location, FVector(Size / 100.f, Size / 100.f, 1.f));
	}
}

// ---------------------------------------------------------------------------
// Simulation
// ---------------------------------------------------------------------------

void FWeaponFX::Tick(float DeltaSeconds, const FVector& CameraLocation)
{
	const float Dt = FMath::Clamp(DeltaSeconds, 0.f, 0.1f);
	// Where looks are seen from (DetailAt thins what's thrown far off).
	LastCamera = CameraLocation;
	bHasCamera = true;
	WaterRings.Tick(Dt);
	for (int32 Index = Particles.Num() - 1; Index >= 0; --Index)
	{
		FParticle& Particle = Particles[Index];
		Particle.Age += Dt;
		if (Particle.Age >= Particle.Life)
		{
			Particles.RemoveAtSwap(Index, 1, EAllowShrinking::No);
			continue;
		}
		if (Particle.bResting || Particle.Age < 0.f)
		{
			// Lying where it landed until it shrinks away; or not begun yet (a blast's late puff: SpawnFlame's delay).
			continue;
		}
		Particle.Velocity += FVector(0.f, 0.f, -FXGravity * Particle.Gravity * Dt);
		Particle.Velocity *= FMath::Exp(-Particle.Drag * Dt);
		Particle.Location += Particle.Velocity * Dt;
		if (!Particle.Spin.IsNearlyZero())
		{
			const float Angle = Particle.Spin.Size() * Dt;
			Particle.Rotation = FQuat(Particle.Spin.GetSafeNormal(), Angle) * Particle.Rotation;
		}
		if (Particle.bFloor)
		{
			LandOnFloor(Particle);
		}
	}
	Redraw(CameraLocation);
	Streaks.Reset();
}

void FWeaponFX::LandOnFloor(FParticle& Particle)
{
	const FVector Up(Particle.Floor.X, Particle.Floor.Y, Particle.Floor.Z);
	// A chunk comes to lie with its middle a little over the ground; a spark touches it.
	const float Lift = Particle.Type == EParticle::Spark ? 0.3f : Particle.StartSize * 0.25f;
	const double Height = Particle.Floor.PlaneDot(Particle.Location);
	if (Height >= Lift)
	{
		return;
	}
	// A drop (water, gel) splats into the ground: gone.
	const bool bDrop = Particle.Type == EParticle::Droplet
		|| (Particle.Type == EParticle::Bit && BitSets.IsValidIndex(Particle.BitSet) && BitSets[Particle.BitSet].Shape == EBitShape::Round);
	if (bDrop)
	{
		Particle.Life = Particle.Age;
		return;
	}
	Particle.Location += Up * (Lift - Height);
	const double Into = FVector::DotProduct(Particle.Velocity, Up);
	const bool bSpark = Particle.Type == EParticle::Spark;
	if (Into < 0.0)
	{
		// A bounce: it skids on along the ground and hops back up a little (sparks keep more, skittering on).
		const FVector Across = Particle.Velocity - Up * Into;
		Particle.Velocity = Across * (bSpark ? 0.7f : 0.45f) - Up * (Into * (bSpark ? 0.4f : 0.3f));
		Particle.Spin *= 0.5f;
		++Particle.Bounces;
	}
	if (!bSpark && (Particle.Bounces >= FXMostBounces || Particle.Velocity.SizeSquared() < FMath::Square(FXRestSpeed)))
	{
		// Spent: it lies flat where it fell a moment or two, then shrinks away.
		Particle.bResting = true;
		Particle.Velocity = FVector::ZeroVector;
		Particle.Spin = FVector::ZeroVector;
		FVector Along = Particle.Rotation.GetAxisX();
		Along -= Up * FVector::DotProduct(Along, Up);
		if (!Along.Normalize())
		{
			Along = FXPerpendicular(Up);
		}
		Particle.Rotation = FRotationMatrix::MakeFromZX(Up, Along).ToQuat();
		Particle.Life = FMath::Max(Particle.Life, Particle.Age + Random.FRandRange(FXRestMin, FXRestMax));
	}
}

int32 FWeaponFX::NumResting() const
{
	int32 Count = 0;
	for (const FParticle& Particle : Particles)
	{
		Count += Particle.bResting ? 1 : 0;
	}
	return Count;
}

float FWeaponFX::LowestOverFloor() const
{
	bool bAny = false;
	double Lowest = 0.0;
	for (const FParticle& Particle : Particles)
	{
		// Not a drop that just splatted (gone next update) and not a spark (it only skitters).
		if (Particle.bFloor && Particle.Type != EParticle::Spark && Particle.Age < Particle.Life)
		{
			const double Height = Particle.Floor.PlaneDot(Particle.Location);
			Lowest = bAny ? FMath::Min(Lowest, Height) : Height;
			bAny = true;
		}
	}
	return static_cast<float>(Lowest);
}

// ---------------------------------------------------------------------------
// Drawing
// ---------------------------------------------------------------------------

void FWeaponFX::Redraw(const FVector& Camera)
{
	TArray<FTransform> GlowTransforms, SmokeTransforms, ChipTransforms, DropTransforms, ShapeTransforms, FireTransforms;
	TArray<float> GlowData, SmokeData, ShapeData, FireData;
	TArray<TArray<FTransform>, TInlineAllocator<MaxBitSets>> BitTransforms;
	BitTransforms.SetNum(BitSets.Num());

	// The puffs' floats: M_FX_Puff's six (its seed and how far it has thinned away) or the plain smoke's four.
	UInstancedStaticMeshComponent* SmokeInstances = Smoke.Get();
	const bool bPuffData = SmokeInstances && SmokeInstances->NumCustomDataFloats >= 6;
	auto AddSmokeData = [&SmokeData, bPuffData](const FLinearColor& Color, float Opacity, float Seed, float Erode)
	{
		SmokeData.Append({ Color.R, Color.G, Color.B, Opacity });
		if (bPuffData)
		{
			SmokeData.Append({ Seed, Erode });
		}
	};
	// Rings, fireballs and starbursts: M_FX_Shape's quads when it's in the game, else stand-ins on the glows.
	const bool bShapeQuads = Shapes.IsValid();
	auto AddShape = [&ShapeTransforms, &ShapeData](const FTransform& Transform, const FLinearColor& Color, float Intensity, EVfxShape Shape,
		float Param, float Seed)
	{
		ShapeTransforms.Add(Transform);
		ShapeData.Append({ Color.R, Color.G, Color.B, Intensity, static_cast<float>(Shape), Param, Seed });
	};

	// Sparks and tracers as soft tapered streaks on M_FX_Muzzle (the play-check, 2026-10-22: their flat diamonds on M_FX_Glow
	// covered close targets), never bigger on screen than their cvars allow; on the glows without it.
	const FVfxTuning Vfx = FVfxTuning::Live();
	const bool bSoft = SoftStreaks.IsValid();
	TArray<FTransform> SoftTransforms;
	TArray<float> SoftData;
	auto AddSoft = [&SoftTransforms, &SoftData, &Camera](const FVector& Tail, const FVector& Head, float Width, const FLinearColor& Color,
		float Intensity, float MaxWidth, float MaxLength)
	{
		const float Distance = static_cast<float>(FVector::Dist(Camera, Head));
		FVector From = Tail;
		const FVector Way = Head - Tail;
		const float Length = static_cast<float>(Way.Size());
		const float Kept = VfxRules::StreakWidthAt(Length, Distance, MaxLength);
		if (Length > UE_KINDA_SMALL_NUMBER && Kept < Length)
		{
			From = Head - Way * (Kept / Length);
		}
		SoftTransforms.Add(StreakTransform(From, Head, VfxRules::StreakWidthAt(Width, Distance, MaxWidth) * SoftStreakWiden, Camera));
		SoftData.Append({ Color.R, Color.G, Color.B, Intensity, 0.f, 1.f, 0.f, 0.f });
	};

	for (const FStreak& Streak : Streaks)
	{
		if (bSoft && Streak.Look == EStreakLook::Tracer && !Streak.bRound)
		{
			AddSoft(Streak.Tail, Streak.Head, Streak.Width, Streak.Color, Streak.Intensity * Vfx.TracerGlow, Vfx.TracerWidth, 0.f);
			continue;
		}
		// A round one is a flash's soft disc (the glow material's shape 1), a line the streak's.
		GlowTransforms.Add(Streak.bRound ? SpriteTransform(Streak.Head, Streak.Width, 0.f, Camera)
			: StreakTransform(Streak.Tail, Streak.Head, Streak.Width, Camera));
		AddGlowData(GlowData, Streak.Color, Streak.Intensity, Streak.bRound ? 1.f : 0.f);
	}

	// Chunks shrink away over the end of their life: the last fifth of a short one, the last 0.4 s of one that lay a while.
	auto Shrink = [](const FParticle& Particle)
	{
		const float Window = FMath::Max(FMath::Min(Particle.Life * 0.2f, 0.4f), 0.01f);
		return FMath::Clamp((Particle.Life - Particle.Age) / Window, 0.f, 1.f);
	};

	// The flames, far to near (alpha-blended); without M_FX_Fire they go on the smoke.
	CollectFlames(Camera, FireTransforms, FireData, SmokeTransforms, SmokeData, bPuffData);

	for (const FParticle& Particle : Particles)
	{
		if (Particle.Age < 0.f)
		{
			// Not begun yet (a delayed spawn).
			continue;
		}
		const float T = Particle.Age / Particle.Life;
		switch (Particle.Type)
		{
		case EParticle::Flame:
			// Drawn by CollectFlames.
			break;
		case EParticle::Spark:
		{
			// A short streak trailing behind the spark, fading as it cools.
			const float Length = FMath::Clamp(Particle.Velocity.Size() * 0.025f, 3.f, 40.f);
			const FVector Tail = Particle.Location - Particle.Velocity.GetSafeNormal() * Length;
			if (bSoft)
			{
				AddSoft(Tail, Particle.Location, Particle.StartSize, Particle.Color, Particle.Intensity * FMath::Pow(1.f - T, 1.5f) * Vfx.SparkGlow,
					Vfx.SparkWidth, Vfx.SparkLength);
				break;
			}
			GlowTransforms.Add(StreakTransform(Tail, Particle.Location, Particle.StartSize, Camera));
			AddGlowData(GlowData, Particle.Color, Particle.Intensity * FMath::Pow(1.f - T, 1.5f), 0.f);
			break;
		}
		case EParticle::Streak:
		{
			// A ricochet's tracer: it grows from where the round struck to its full length (EndSize) as it flies off.
			const float Speed = Particle.Velocity.Size();
			const float Length = FMath::Min(Speed * Particle.Age + 2.f, Particle.EndSize);
			const FVector Tail = Particle.Location - Particle.Velocity.GetSafeNormal() * Length;
			if (bSoft)
			{
				AddSoft(Tail, Particle.Location, Particle.StartSize, Particle.Color, Particle.Intensity * FMath::Pow(1.f - T, 1.2f) * Vfx.SparkGlow,
					Vfx.TracerWidth, 0.f);
				break;
			}
			GlowTransforms.Add(StreakTransform(Tail, Particle.Location, Particle.StartSize, Camera));
			AddGlowData(GlowData, Particle.Color, Particle.Intensity * FMath::Pow(1.f - T, 1.2f), 0.f);
			break;
		}
		case EParticle::Flash:
		{
			const float Size = FMath::Lerp(Particle.StartSize, Particle.EndSize, FMath::Sqrt(T));
			GlowTransforms.Add(SpriteTransform(Particle.Location, Size, Particle.Roll, Camera));
			AddGlowData(GlowData, Particle.Color, Particle.Intensity * FMath::Square(1.f - T), 1.f);
			break;
		}
		case EParticle::Smoke:
		{
			const float Size = FMath::Lerp(Particle.StartSize, Particle.EndSize, 1.f - FMath::Square(1.f - T));
			// Thinned as the camera gets into it: a puff drifting past the eye (a slide's dust) would otherwise fill the view.
			const float Distance = static_cast<float>(FVector::Dist(Particle.Location, Camera));
			const float Near = FMath::Clamp((Distance - Size * 0.25f) / 40.f, 0.f, 1.f);
			// VFX-2: and faded as it grows past most of the view (a blast's skirt at the feet): a haze there, and all overdraw.
			const float Fill = 1.f - FMath::Clamp((Size / FMath::Max(Distance, 1.f) - 1.f) / 0.8f, 0.f, 1.f);
			const float Opacity = Particle.Intensity * FMath::Pow(1.f - T, 1.5f) * FMath::Clamp(T * 12.f, 0.f, 1.f) * Near * Fill;
			if (Opacity <= 0.001f)
			{
				// Not seen (not begun, or filling the view): not drawn either.
				break;
			}
			SmokeTransforms.Add(SpriteTransform(Particle.Location, Size, Particle.Roll, Camera));
			// Its own noise (its roll as the seed) and how far it has thinned: M_FX_Puff erodes it into wisps as it goes.
			AddSmokeData(Particle.Color, Opacity, Particle.Roll / UE_TWO_PI, T);
			break;
		}
		case EParticle::Thread:
		{
			// A soft line of smoke along a tracer's path, widening as it thins; faded where it passes near the eye.
			const FVector Tail = Particle.Location - Particle.Extent;
			const FVector Head = Particle.Location + Particle.Extent;
			const float Width = FMath::Lerp(Particle.StartSize, Particle.EndSize, FMath::Sqrt(T));
			SmokeTransforms.Add(StreakTransform(Tail, Head, Width, Camera));
			const float Near = FMath::Clamp((static_cast<float>(FMath::PointDistToSegment(Camera, Tail, Head)) - 30.f) / 90.f, 0.f, 1.f);
			const float Opacity = Particle.Intensity * FMath::Pow(1.f - T, 1.4f) * FMath::Clamp(T * 8.f, 0.f, 1.f) * Near;
			// A thread breaks into strands as it thins (half the erosion: it should hang, not crumble).
			AddSmokeData(Particle.Color, Opacity, Particle.Roll / UE_TWO_PI, T * 0.5f);
			break;
		}
		case EParticle::Ring:
		{
			// Out fast and easing, fading as it goes; square to its axis (on the ground) or turned to the eye.
			const float Radius = FMath::Lerp(Particle.StartSize, Particle.EndSize, VfxRules::RingGrowth(T));
			const float Strength = Particle.Intensity * VfxRules::RingFade(T);
			FVector Axis = Particle.Extent;
			if (Particle.bFacing)
			{
				Axis = Camera - Particle.Location;
				if (!Axis.Normalize())
				{
					Axis = FVector::UpVector;
				}
			}
			if (Radius <= 0.5f || Strength <= 0.f)
			{
				break;
			}
			if (bShapeQuads)
			{
				// The quad reaches the ring's outer edge; the band is its thickness's share of that radius.
				const float Outer = Radius + Particle.Width * 0.5f;
				AddShape(FlatTransform(Particle.Location, Axis, Outer * 2.f, Particle.Roll), Particle.Color, Strength, EVfxShape::Ring,
					FMath::Clamp(Particle.Width / Outer, 0.02f, 1.f), Particle.Roll / UE_TWO_PI);
			}
			else
			{
				// Pieces of streak round it, each reaching a third past its ends (a streak's ends are soft: EmberFX::DrawRing's way).
				FVector U, V;
				Axis.FindBestAxisVectors(U, V);
				const int32 Pieces = VfxRules::RingPieces(Radius);
				const float Step = UE_TWO_PI / static_cast<float>(Pieces);
				auto At = [&](float Angle) { return Particle.Location + (U * FMath::Cos(Angle) + V * FMath::Sin(Angle)) * Radius; };
				for (int32 Piece = 0; Piece < Pieces; ++Piece)
				{
					const float From = Particle.Roll + Step * (static_cast<float>(Piece) - 0.33f);
					GlowTransforms.Add(StreakTransform(At(From), At(From + Step * 1.66f), Particle.Width, Camera));
					AddGlowData(GlowData, Particle.Color, Strength, GlowStreak);
				}
			}
			break;
		}
		case EParticle::Fireball:
		{
			// Blooming fast, cooling from white-hot through its colour to a dull ember edge, then gone into its smoke.
			const float Size = FMath::Lerp(Particle.StartSize, Particle.EndSize, VfxRules::FireballGrowth(T));
			const float Heat = VfxRules::FireballHeat(T);
			// Additive fire must stay near 1 to keep its colour in daylight (the tonemapper turns a bright orange white): the
			// spawns keep strengths of 1-4. It dims as it cools (a cooled fireball at full strength burned out to a white cloud in daylight: 2026-10-22).
			const float Strength = Particle.Intensity * (1.f - T * T) * (0.2f + 0.8f * Heat * Heat);
			if (bShapeQuads)
			{
				AddShape(SpriteTransform(Particle.Location, Size, Particle.Roll, Camera), Particle.Color, Strength, EVfxShape::Fireball, Heat,
					Particle.Roll / UE_TWO_PI);
			}
			else
			{
				GlowTransforms.Add(SpriteTransform(Particle.Location, Size, Particle.Roll, Camera));
				AddGlowData(GlowData, VfxRules::HeatColor(Particle.Color, Heat), Strength * (0.35f + 0.65f * Heat), GlowRound);
			}
			break;
		}
		case EParticle::Star:
		{
			// Snapped open, fading fast: a hit's pop.
			const float Size = FMath::Lerp(Particle.StartSize, Particle.EndSize, FMath::Sqrt(T));
			const float Strength = Particle.Intensity * FMath::Square(1.f - T);
			if (bShapeQuads)
			{
				AddShape(SpriteTransform(Particle.Location, Size, Particle.Roll, Camera), Particle.Color, Strength, EVfxShape::Starburst,
					Particle.Width, Particle.Roll / UE_TWO_PI);
			}
			else
			{
				GlowTransforms.Add(SpriteTransform(Particle.Location, Size, Particle.Roll, Camera));
				AddGlowData(GlowData, Particle.Color, Strength, GlowStar);
			}
			break;
		}
		case EParticle::Chip:
		case EParticle::Droplet:
		{
			// The engine cube and sphere are 100 across.
			const float Size = Particle.StartSize * Shrink(Particle) / 100.f;
			// A fast drop is drawn as a thin streak of spray along its flight (thinner as it stretches), rounding up as it slows.
			const float Stretch = 1.3f + FMath::Min(static_cast<float>(Particle.Velocity.Size()) * SprayStretch, SprayMostStretch);
			const FVector Scale = Particle.Type == EParticle::Droplet ? FVector(Size * Stretch, Size / FMath::Sqrt(Stretch), Size / FMath::Sqrt(Stretch)) : FVector(Size, Size * 0.7f, Size * 0.5f);
			// Droplets stretch along their flight.
			const FQuat Rotation = Particle.Type == EParticle::Droplet && !Particle.Velocity.IsNearlyZero()
				? FRotationMatrix::MakeFromX(Particle.Velocity).ToQuat() : Particle.Rotation;
			(Particle.Type == EParticle::Chip ? ChipTransforms : DropTransforms).Add(FTransform(Rotation, Particle.Location, Scale));
			break;
		}
		case EParticle::Bit:
		{
			if (!BitTransforms.IsValidIndex(Particle.BitSet))
			{
				break;
			}
			// Round bits (gel, water) stretch along their flight like droplets; splinters (wood, straw) are long and thin; the
			// rest are thin plates (chitin, chips of paint and glass). Splinters and plates tumble.
			const float Size = Particle.StartSize * Shrink(Particle) / 100.f;
			const EBitShape Shape = BitSets[Particle.BitSet].Shape;
			const bool bRound = Shape == EBitShape::Round;
			const float Stretch = bRound ? 1.35f + FMath::Min(static_cast<float>(Particle.Velocity.Size()) * SprayStretch, SprayMostStretch) : 1.f;
			const FVector Scale = bRound ? FVector(Size * Stretch, Size / FMath::Sqrt(Stretch), Size * 0.9f / FMath::Sqrt(Stretch))
				: (Shape == EBitShape::Splinter ? FVector(Size * 2.4f, Size * 0.3f, Size * 0.22f) : FVector(Size, Size * 0.8f, Size * 0.28f));
			const FQuat Rotation = bRound && !Particle.Velocity.IsNearlyZero() ? FRotationMatrix::MakeFromX(Particle.Velocity).ToQuat() : Particle.Rotation;
			BitTransforms[Particle.BitSet].Add(FTransform(Rotation, Particle.Location, Scale));
			break;
		}
		}
	}

	// Rings and wakes on the water: flat dashes of the same soft smoke quad (WaterRings.h). They write the plain smoke's four
	// floats; on the puffs each gets a seed below 0 (M_FX_Puff's plain soft disc: no noise breaking a dash up).
	if (bPuffData)
	{
		TArray<float> RingData;
		WaterRings.Draw(Camera, SmokeTransforms, RingData);
		for (int32 Index = 0; Index + 3 < RingData.Num(); Index += 4)
		{
			SmokeData.Append({ RingData[Index], RingData[Index + 1], RingData[Index + 2], RingData[Index + 3], -1.f, 0.f });
		}
	}
	else
	{
		WaterRings.Draw(Camera, SmokeTransforms, SmokeData);
	}

	bool bAnything = !SoftTransforms.IsEmpty() || !GlowTransforms.IsEmpty() || !SmokeTransforms.IsEmpty() || !ChipTransforms.IsEmpty() || !DropTransforms.IsEmpty()
		|| !ShapeTransforms.IsEmpty() || !FireTransforms.IsEmpty();
	for (const TArray<FTransform>& Set : BitTransforms)
	{
		bAnything |= !Set.IsEmpty();
	}
	if (bAnything || bDrawnAnything)
	{
		DrawInstances(Glows.Get(), GlowTransforms, GlowData);
		DrawInstances(SoftStreaks.Get(), SoftTransforms, SoftData);
		DrawInstances(Smoke.Get(), SmokeTransforms, SmokeData);
		DrawInstances(Shapes.Get(), ShapeTransforms, ShapeData);
		DrawInstances(FireQuads.Get(), FireTransforms, FireData);
		DrawInstances(Chips.Get(), ChipTransforms, {});
		DrawInstances(Droplets.Get(), DropTransforms, {});
		for (int32 Index = 0; Index < BitSets.Num(); ++Index)
		{
			DrawInstances(BitSets[Index].Instances.Get(), BitTransforms[Index], {});
		}
	}
	bDrawnAnything = bAnything;
}

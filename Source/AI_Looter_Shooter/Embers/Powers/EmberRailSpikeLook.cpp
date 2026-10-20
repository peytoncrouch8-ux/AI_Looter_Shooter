// UEmberRailSpike's looks (EmberRailSpike.h): the spike in flight, what it leaves along its way, at each body and where it
// stops. Its line runs straight away from the player's eye (first person, or the camera behind), so a streak along it shows
// end-on as nothing (2026-10-20: "it barely shows"): what reads is what lies across the line or keeps a size on screen: its
// head's glow, shock rings and steam rings square to its way, a helix of hot air round it, sparks thrown out sideways, and
// the ground under it, where the dust kicked out both sides and a scar torn open run away from the eye.

#include "Embers/Powers/EmberRailSpike.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/GroundCrack.h"
#include "Player/ViewKick.h"
#include "Weapons/CreatureBlood.h"
#include "Weapons/WeaponFX.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"

namespace
{
	/** The spike: white-hot iron in a blue-white streak, its length and width (cm); a shock's hot white. */
	const FLinearColor SpikeHot(0.9f, 0.95f, 1.f);
	const FLinearColor IronGlow(1.f, 0.45f, 0.12f);
	const FLinearColor ShockWhite(1.f, 0.85f, 0.65f);
	constexpr float SpikeLength = 90.f;
	constexpr float SpikeWidth = 4.5f;
	/** It joins the aim line from the forearm over this far (cm), as a tracer does. */
	constexpr float ConvergeDistance = 600.f;
	/** A breath of steam laid along its way every so far (cm). */
	constexpr float SteamEvery = 70.f;
	/**
	 * The line it leaves glowing along its way, fading and spreading, and the smoke thread left hanging: it flies 25 m in a
	 * few frames, so without them the shot was over before it was seen (2026-10-20 play check).
	 */
	constexpr float TrailSeconds = 0.4f;
	constexpr float TrailWidth = 2.5f;
	constexpr float TrailGlow = 9.f;

	/** Its head's glow (cm across) near the eye, growing by this share of its distance so it keeps a size on screen. */
	constexpr float HeadGlowSize = 16.f;
	constexpr float HeadGlowGrowth = 0.012f;
	/**
	 * The hot-air helix wound round its way: from this far out of the arm (cm), a turn every HelixPitch, drawn in HelixStep
	 * pieces, its radius spreading from HelixRadius to HelixSpread as it fades over HelixSeconds.
	 */
	constexpr float HelixStart = 110.f;
	constexpr float HelixPitch = 90.f;
	constexpr float HelixStep = 10.f;
	constexpr float HelixRadius = 8.f;
	constexpr float HelixSpread = 24.f;
	constexpr float HelixSeconds = 0.5f;
	/** A ring of steam blown out square to its way every RingEvery cm, the first RingStart out. */
	constexpr float RingEvery = 260.f;
	constexpr float RingStart = 200.f;
	/**
	 * The ground under its line, looked up every GroundStep cm: dust kicked out where the line is within DustHeight of it,
	 * a scar torn open where within ScarHeight (a stretch of ScarMinLength at least, ScarFirstSample steps past his feet).
	 */
	constexpr float GroundStep = 100.f;
	constexpr float DustHeight = 320.f;
	constexpr float ScarHeight = 150.f;
	constexpr float ScarMinLength = 200.f;
	constexpr int32 ScarFirstSample = 2;
	constexpr float ScarWidth = 16.f;
	/** The scar stays open this long after the flight, then sinks away (AGroundCrack::CloseSeconds). */
	constexpr float ScarHold = 1.2f;
	/** At most this many rings at once (the oldest gives way). */
	constexpr int32 MaxShocks = 16;

	/** Two axes square to Way (for rings round it). */
	void AxesRound(const FVector& Way, FVector& OutU, FVector& OutV)
	{
		Way.GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector).FindBestAxisVectors(OutU, OutV);
	}
}

FVector UEmberRailSpike::SeenAt(float Distance) const
{
	const FVector Offset = VisualStart - Line.Start;
	return Line.Start + Line.Direction * Distance + Offset * (1.f - FMath::SmoothStep(0.f, ConvergeDistance, Distance));
}

void UEmberRailSpike::AddShock(const FVector& Center, const FVector& Axis, float Life, float From, float To, float Width, float Glow,
	const FLinearColor& Hue)
{
	if (Shocks.Num() >= MaxShocks)
	{
		Shocks.RemoveAt(0);
	}
	FShockRing& Ring = Shocks.AddDefaulted_GetRef();
	Ring.Center = Center;
	Ring.Axis = Axis.GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector);
	Ring.Life = FMath::Max(Life, 0.01f);
	Ring.From = From;
	Ring.To = To;
	Ring.Width = Width;
	Ring.Glow = Glow;
	Ring.Color = Hue;
}

// ---------------------------------------------------------------------------
// Leaving the arm
// ---------------------------------------------------------------------------

void UEmberRailSpike::BeginLook()
{
	Shocks.Reset();
	WakeAge = 0.f;
	WakeTwist = Random.FRandRange(0.f, UE_TWO_PI);
	RingLaid = RingStart - RingEvery;
	GroundLaid = 0;
	// The way it leaves the arm (toward where it joins the aim line).
	const FVector Leaving = (SeenAt(80.f) - VisualStart).GetSafeNormal(UE_SMALL_NUMBER, Line.Direction);
	if (FWeaponFX* FX = Effects())
	{
		// The piston's steam bursting off the forearm, its flash.
		for (int32 Index = 0; Index < 6; ++Index)
		{
			const FVector Out = (-Leaving * 0.4 + Random.GetUnitVector() + FVector(0.0, 0.0, 0.5)).GetSafeNormal();
			FX->SpawnWisp(VisualStart, Out * Random.FRandRange(120.f, 260.f), 0.3f, 6.f, Random.FRandRange(35.f, 60.f), Random.FRandRange(0.6f, 1.f));
		}
		FX->SpawnFlash(VisualStart + Leaving * 10.f, Leaving, 0.6f);
		// A doughnut of steam blown out round it as it leaves: square to its way, it faces the eye.
		FVector U, V;
		AxesRound(Leaving, U, V);
		const FVector RingAt = SeenAt(120.f);
		for (int32 Index = 0; Index < 14; ++Index)
		{
			const float Angle = UE_TWO_PI * static_cast<float>(Index) / 14.f + Random.FRandRange(-0.15f, 0.15f);
			const FVector Out = U * FMath::Cos(Angle) + V * FMath::Sin(Angle);
			FX->SpawnWisp(RingAt + Out * 8.f, Out * Random.FRandRange(150.f, 230.f) + Leaving * 60.f, 0.32f, 8.f, Random.FRandRange(34.f, 48.f),
				Random.FRandRange(0.45f, 0.7f));
		}
	}
	// The shock as it leaves: a quick white ring and a wider, slower orange one round its way a step or two out (round the
	// crosshair in first person; nearer, they filled the view).
	AddShock(SeenAt(160.f), Line.Direction, 0.14f, 4.f, 22.f, 3.f, 18.f, ShockWhite);
	AddShock(SeenAt(240.f), Line.Direction, 0.28f, 8.f, 36.f, 5.f, 8.f, IronGlow);
	// A hard kick back, a turn of the view and a breath wider.
	FViewKick Recoil;
	Recoil.Pitch = 1.6f;
	Recoil.Yaw = Random.FRandRange(-0.35f, 0.35f);
	Recoil.Roll = Random.FRandRange(0.4f, 0.7f) * (Random.FRand() < 0.5f ? -1.f : 1.f);
	Recoil.FieldOfView = 0.045f;
	Recoil.Frequency = 10.f;
	Recoil.Damping = 0.5f;
	Kick(Recoil);

	// The ground under its way, for the dust it kicks up and the scar it tears.
	GroundHeight.Reset();
	GroundPoint.Reset();
	ScarFrom = 0.f;
	ScarTo = 0.f;
	if (AGroundCrack* Old = Scar.Get())
	{
		Old->Close(0.f);
	}
	Scar = nullptr;
	UWorld* World = GetBodyWorld();
	if (!World)
	{
		return;
	}
	// The line starts at the eye: behind the body seen from outside (the camera's), so the ground counts from his feet on.
	const float BodyAlong = Body.IsValid() ? static_cast<float>(FVector::DotProduct(Body->GetActorLocation() - Line.Start, Line.Direction)) : 0.f;
	const int32 PastBody = FMath::Max(FMath::CeilToInt32(FMath::Max(BodyAlong, 0.f) / GroundStep), 1);
	const int32 Samples = FMath::FloorToInt32(Line.Length / GroundStep) + 1;
	for (int32 Index = 0; Index < Samples; ++Index)
	{
		const FVector OnLine = Line.Start + Line.Direction * FMath::Min(static_cast<float>(Index) * GroundStep, Line.Length);
		FVector Ground;
		const bool bGround = Index >= PastBody && FindGround(*World, OnLine, 10.f, DustHeight, Ground, nullptr, Body.Get());
		GroundHeight.Add(bGround ? static_cast<float>(OnLine.Z - Ground.Z) : -1.f);
		GroundPoint.Add(bGround ? Ground : OnLine);
	}
	// The scar: the first stretch out from 2 m past his feet where the line skims within ScarHeight of the ground.
	int32 RunFrom = INDEX_NONE;
	int32 RunTo = INDEX_NONE;
	for (int32 Index = PastBody + ScarFirstSample - 1; Index < GroundHeight.Num(); ++Index)
	{
		if (GroundHeight[Index] >= 0.f && GroundHeight[Index] <= ScarHeight)
		{
			RunFrom = RunFrom == INDEX_NONE ? Index : RunFrom;
			RunTo = Index;
		}
		else if (RunFrom != INDEX_NONE)
		{
			break;
		}
	}
	if (RunFrom != INDEX_NONE && static_cast<float>(RunTo - RunFrom) * GroundStep >= ScarMinLength)
	{
		ScarFrom = static_cast<float>(RunFrom) * GroundStep;
		ScarTo = FMath::Min(static_cast<float>(RunTo) * GroundStep, Line.Length);
		Scar = AGroundCrack::Spawn(World, GroundPoint[RunFrom], GroundPoint[RunTo], ScarWidth, IronGlow, Random.RandRange(0, 1 << 20));
	}
}

// ---------------------------------------------------------------------------
// In flight
// ---------------------------------------------------------------------------

void UEmberRailSpike::LookInFlight(float Was)
{
	FWeaponFX* FX = Effects();
	if (FX)
	{
		// The spike itself, white-hot, its last frame's way behind it; its head a glow that keeps its size on screen as it
		// goes (from behind the streak is end-on), in a wider halo.
		const FVector Head = SeenAt(Flown);
		const FVector Tail = SeenAt(FMath::Max(Flown - SpikeLength, 0.f));
		FX->AddStreak(SeenAt(FMath::Max(Was - SpikeLength, 0.f)), Head, SpikeWidth * 0.6f, IronGlow, 10.f);
		FX->AddStreak(Tail, Head, SpikeWidth, SpikeHot, 34.f);
		const float Seen = HeadGlowSize + HeadGlowGrowth * static_cast<float>(FVector::Dist(Line.Start, Head));
		FX->AddGlow(Head, Seen, SpikeHot, 30.f);
		FX->AddGlow(Head, Seen * 2.4f, IronGlow, 5.f);
		// A breath of steam laid behind it, and every few metres a ring of steam blown out square to its way: from
		// behind, rings down the line.
		while (SteamLaid + SteamEvery <= Flown)
		{
			SteamLaid += SteamEvery;
			FX->SpawnWisp(SeenAt(SteamLaid), Random.GetUnitVector() * 25.f + FVector(0.0, 0.0, 15.0), 0.16f, 5.f, Random.FRandRange(26.f, 40.f),
				Random.FRandRange(0.5f, 0.8f));
		}
		FVector U, V;
		AxesRound(Line.Direction, U, V);
		while (RingLaid + RingEvery <= Flown)
		{
			RingLaid += RingEvery;
			const FVector At = SeenAt(RingLaid);
			for (int32 Index = 0; Index < 8; ++Index)
			{
				const float Angle = UE_TWO_PI * static_cast<float>(Index) / 8.f + Random.FRandRange(-0.2f, 0.2f);
				const FVector Out = U * FMath::Cos(Angle) + V * FMath::Sin(Angle);
				FX->SpawnWisp(At + Out * 6.f, Out * Random.FRandRange(110.f, 160.f) + FVector(0.0, 0.0, 10.0), 0.3f, 10.f,
					Random.FRandRange(26.f, 36.f), Random.FRandRange(0.6f, 0.9f));
			}
		}
	}

	// The ground under it as it passes: dust thrown out both sides (more and faster the lower it skims), grit where close.
	const FVector Side = FVector::CrossProduct(Line.Direction, FVector::UpVector).GetSafeNormal(UE_SMALL_NUMBER, FVector::RightVector);
	FLinearColor Dust = FLinearColor::Black;
	bool bDustKnown = false;
	while (GroundLaid < GroundHeight.Num() && static_cast<float>(GroundLaid) * GroundStep <= Flown)
	{
		const int32 Index = GroundLaid++;
		const float Height = GroundHeight[Index];
		if (!FX || Index < 1 || Height < 0.f || Height > DustHeight)
		{
			continue;
		}
		if (!bDustKnown)
		{
			Dust = EmberFX::DustColor(Body.Get());
			bDustKnown = true;
		}
		const float Near = 1.f - Height / DustHeight;
		const FVector Ground = GroundPoint[Index] + FVector(0.0, 0.0, 8.0);
		for (const float Way : { -1.f, 1.f })
		{
			const FVector Out = (Side * Way + Line.Direction * Random.FRandRange(0.1f, 0.4f) + FVector(0.0, 0.0, 0.25)).GetSafeNormal();
			FX->SpawnDustPuff(Ground + Side * (Way * 15.f), Out * (Random.FRandRange(160.f, 300.f) * (0.4f + 0.6f * Near)), Dust,
				0.12f + 0.3f * Near, 22.f, 60.f + 60.f * Near, Random.FRandRange(0.8f, 1.2f));
			if (Height < 120.f)
			{
				FX->SpawnGrit(GroundPoint[Index], (Side * (Way * 0.6f) + FVector::UpVector).GetSafeNormal() * Random.FRandRange(250.f, 400.f),
					Random.FRandRange(0.8f, 1.4f), Random.FRandRange(0.5f, 0.8f));
			}
		}
	}

	// The scar tears open behind it; once it's done it holds a moment and closes.
	if (AGroundCrack* Crack = Scar.Get(); Crack && ScarTo > ScarFrom)
	{
		Crack->SetOpen(FMath::Clamp((Flown - ScarFrom) / (ScarTo - ScarFrom), 0.f, 1.f));
		if (!bFlying)
		{
			Crack->Close(ScarHold);
		}
	}

	// Its flight done: a smoke thread left hanging along its way.
	if (!bFlying && FX)
	{
		const FVector Joined = SeenAt(FMath::Min(ConvergeDistance, Flown));
		FX->AddThread(VisualStart, Joined, 3.f, 0.22f, 1.4f);
		if (Flown > ConvergeDistance)
		{
			FX->AddThread(Joined, SeenAt(Flown), 3.f, 0.22f, 1.4f);
		}
	}
}

// ---------------------------------------------------------------------------
// Through a body, into the world
// ---------------------------------------------------------------------------

void UEmberRailSpike::LookAtPierce(ACreatureBase& Creature, const FVector& At)
{
	// Sparks burst out sideways round it (square to its way they read from behind), a few thrown on through.
	FVector U, V;
	AxesRound(Line.Direction, U, V);
	for (int32 Index = 0; Index < 14; ++Index)
	{
		const float Angle = UE_TWO_PI * static_cast<float>(Index) / 14.f + Random.FRandRange(-0.2f, 0.2f);
		const FVector Out = (U * FMath::Cos(Angle) + V * FMath::Sin(Angle) + Line.Direction * 0.35 + Random.GetUnitVector() * 0.15).GetSafeNormal();
		Sparks.Spawn(At, Out * Random.FRandRange(450.f, 900.f), Random.FRandRange(0.18f, 0.4f), 1.8f, IronGlow, 26.f, 600.f, 2.f);
	}
	for (int32 Index = 0; Index < 6; ++Index)
	{
		const FVector Out = (Line.Direction + Random.GetUnitVector() * 0.6).GetSafeNormal();
		Sparks.Spawn(At, Out * Random.FRandRange(300.f, 700.f), Random.FRandRange(0.15f, 0.35f), 1.4f, IronGlow, 24.f, 700.f, 2.f);
	}
	AddShock(At, Line.Direction, 0.24f, 15.f, 95.f, 6.f, 20.f, ShockWhite);
	if (FWeaponFX* FX = Effects())
	{
		FX->SpawnFlash(At, Line.Direction, 0.8f);
		FX->SpawnWisp(At, Line.Direction * 80.f + FVector(0.0, 0.0, 20.0), 0.3f, 8.f, 40.f, 0.7f);
		// What it spills, thrown out the far side.
		FVector Middle;
		float Radius = 0.f;
		float HalfHeight = 0.f;
		BodyOf(Creature, Middle, Radius, HalfHeight);
		FX->SpawnCreatureHit(At + Line.Direction * FMath::Max(Radius, 20.f), Line.Direction, Line.Direction, CreatureBlood::Of(&Creature),
			CreatureBlood::TintOf(&Creature), 1.8f, true);
	}
	FViewKick Punch;
	Punch.Pitch = 0.35f;
	Punch.Roll = Random.FRandRange(-0.5f, 0.5f);
	Punch.Frequency = 12.f;
	Punch.Damping = 0.6f;
	Kick(Punch);
}

void UEmberRailSpike::LookAtEmbed()
{
	if (FWeaponFX* FX = Effects())
	{
		FX->SpawnRicochet(Line.End, Line.EmbedNormal, 1.2f);
		const FLinearColor Dust = EmberFX::DustColor(Body.Get());
		for (int32 Index = 0; Index < 6; ++Index)
		{
			const FVector Out = (Line.EmbedNormal + Random.GetUnitVector() * 0.7).GetSafeNormal();
			FX->SpawnDustPuff(Line.End + Out * 10.f, Out * Random.FRandRange(120.f, 260.f), Dust, 0.4f, 14.f, Random.FRandRange(50.f, 80.f),
				Random.FRandRange(0.6f, 1.f));
			FX->SpawnGrit(Line.End, Out * Random.FRandRange(250.f, 450.f), Random.FRandRange(0.8f, 1.6f), Random.FRandRange(0.4f, 0.7f));
		}
	}
	for (int32 Index = 0; Index < 16; ++Index)
	{
		const FVector Out = (Line.EmbedNormal + Random.GetUnitVector()).GetSafeNormal();
		Sparks.Spawn(Line.End, Out * Random.FRandRange(250.f, 600.f), Random.FRandRange(0.2f, 0.5f), 1.4f, IronGlow, 26.f, 800.f, 2.f);
	}
	// A ring spreading over the surface it struck.
	AddShock(Line.End + Line.EmbedNormal * 3.f, Line.EmbedNormal, 0.14f, 6.f, 45.f, 2.5f, 16.f, ShockWhite);
	AddShock(Line.End + Line.EmbedNormal * 3.f, Line.EmbedNormal, 0.3f, 10.f, 85.f, 4.f, 8.f, IronGlow);
}

// ---------------------------------------------------------------------------
// What lingers
// ---------------------------------------------------------------------------

bool UEmberRailSpike::LingerLook(float DeltaSeconds)
{
	const float Dt = FMath::Max(DeltaSeconds, 0.f);
	FWeaponFX* FX = Effects();
	if (TrailAge >= 0.f)
	{
		// The way it came, glowing and spreading as it fades (straight from the forearm to where it joined the aim line).
		TrailAge += Dt;
		const float Fade = 1.f - FMath::Clamp(TrailAge / TrailSeconds, 0.f, 1.f);
		if (FX && Fade > 0.f && Flown > 0.f)
		{
			const FVector Joined = Line.Start + Line.Direction * FMath::Min(ConvergeDistance, Flown);
			const float Width = TrailWidth * (1.f + 1.5f * (1.f - Fade));
			const float Glow = TrailGlow * Fade * Fade;
			FX->AddStreak(VisualStart, Joined, Width, IronGlow, Glow);
			if (Flown > ConvergeDistance)
			{
				FX->AddStreak(Joined, Line.Start + Line.Direction * Flown, Width, IronGlow, Glow);
			}
		}
		if (Fade <= 0.f)
		{
			TrailAge = -1.f;
		}
	}
	if (WakeAge >= 0.f)
	{
		// The helix of hot air round its way, white-hot cooling to orange, spreading as it fades; it winds in from the arm.
		WakeAge += Dt;
		const float T = WakeAge / HelixSeconds;
		if (T >= 1.f)
		{
			WakeAge = -1.f;
		}
		else if (FX && Flown > HelixStart)
		{
			const float Radius = FMath::Lerp(HelixRadius, HelixSpread, 1.f - FMath::Square(1.f - T));
			const float Glow = 16.f * FMath::Square(1.f - T);
			const float Width = 3.5f + 2.f * T;
			const FLinearColor Hue = FMath::Lerp(SpikeHot, IronGlow, FMath::Min(T * 1.6f, 1.f));
			FVector U, V;
			AxesRound(Line.Direction, U, V);
			FVector Previous = FVector::ZeroVector;
			for (float Along = HelixStart; Along <= Flown; Along += HelixStep)
			{
				const float Angle = WakeTwist + UE_TWO_PI * Along / HelixPitch;
				const float In = FMath::Clamp((Along - HelixStart) / 100.f, 0.f, 1.f);
				const FVector Point = SeenAt(Along) + (U * FMath::Cos(Angle) + V * FMath::Sin(Angle)) * (Radius * In);
				if (Along > HelixStart)
				{
					FX->AddStreak(Previous, Point, Width, Hue, Glow);
				}
				Previous = Point;
			}
		}
	}
	if (EmbedAge >= 0.f)
	{
		// Stuck in the world, half its length showing, cooling from white to a dull red, a wisp off it now and then; its
		// butt a glow (seen end-on from where it was driven).
		EmbedAge += Dt;
		const float Cool = FMath::Clamp(EmbedAge / EmbedSeconds, 0.f, 1.f);
		if (FX)
		{
			const FVector Out = Line.End - Line.Direction * (SpikeLength * 0.5f);
			const FLinearColor Hue = FMath::Lerp(SpikeHot, IronGlow, FMath::Min(Cool * 2.f, 1.f));
			FX->AddStreak(Out, Line.End, SpikeWidth * 0.8f, Hue, 26.f * (1.f - Cool) + 2.f);
			FX->AddGlow(Out, 6.f + 10.f * (1.f - Cool), Hue, 18.f * (1.f - Cool) + 2.f);
			if (Random.FRand() < Dt * 3.f)
			{
				FX->SpawnWisp(Out, FVector(0.0, 0.0, 30.0), 0.12f * (1.f - Cool), 4.f, 18.f, 0.8f);
			}
		}
		if (Cool >= 1.f)
		{
			EmbedAge = -1.f;
		}
	}
	if (FallAge >= 0.f)
	{
		// Its reach spent in open air: it tumbles down out of sight, cooling.
		FallAge += Dt;
		if (FX)
		{
			const FVector At = Line.End + FVector(0.0, 0.0, -490.f * FallAge * FallAge);
			const FVector Turn = Line.Direction.RotateAngleAxis(FallAge * 540.f, FVector::CrossProduct(Line.Direction, FVector::UpVector).GetSafeNormal());
			FX->AddStreak(At - Turn * (SpikeLength * 0.5f), At + Turn * (SpikeLength * 0.5f), SpikeWidth * 0.8f, IronGlow, 18.f * FMath::Max(1.f - FallAge, 0.f));
		}
		if (FallAge >= 1.f)
		{
			FallAge = -1.f;
		}
	}
	// The shock rings, spreading fast and fading.
	for (int32 Index = Shocks.Num() - 1; Index >= 0; --Index)
	{
		FShockRing& Ring = Shocks[Index];
		Ring.Age += Dt;
		const float T = Ring.Age / Ring.Life;
		if (T >= 1.f)
		{
			Shocks.RemoveAt(Index);
			continue;
		}
		if (FX)
		{
			const float Radius = FMath::Lerp(Ring.From, Ring.To, 1.f - FMath::Pow(1.f - T, 3.f));
			EmberFX::DrawRing(*FX, Ring.Center, Ring.Axis, Radius, 32, Ring.Width * (1.f + T), Ring.Color, Ring.Glow * FMath::Square(1.f - T));
		}
	}
	Sparks.Tick(Dt, FX);
	return TrailAge >= 0.f || WakeAge >= 0.f || EmbedAge >= 0.f || FallAge >= 0.f || !Sparks.IsEmpty() || !Shocks.IsEmpty();
}

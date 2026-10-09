// ABossSeal's curtain: the fog wall's look. Laid out once as the seal rises, then moved and drawn every frame it stands or sinks.
//
// It has to read at a glance as a barrier of grave-fog you can't pass, not as rain: so it is soft and thick, not streaks.
// All of it is camera-facing quads on three instanced meshes (no assets, three draws, the same few hundred instances the old
// dashes were):
//  - Fog (M_FX_Smoke, translucent and unlit, so the color can run over 1 and glow): faint veils as tall as the wall give it
//    its body; wide banks of fog lie along the ground, the densest part; wisps climb from the foot, curling slowly in a
//    flattened circle and thinning as they rise, most of them dying low and a few reaching the top.
//  - Glow (M_FX_Glow, additive): halos along the foot, specks of ghost-light that climb and wink out, and two cards that
//    gather where the player is nearest the wall.
//  - Seam (opaque, M_StylizedSurface): the glowing strip on the ground, as before.
// The fog within a few metres of the player flares brighter. That, and the camera's distance, are found with arithmetic on
// the pieces' own places: no trace after the one pass over the ground that lays it out.

#include "Bosses/BossSeal.h"
#include "World/WorldQueries.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Math/RotationMatrix.h"

namespace
{
	// How thick each kind lies along the line: one piece this far apart (cm). A ring of 15 m takes about 410 instances all
	// told, which is what the old dashes took.
	constexpr float VeilSpacing = 170.f;
	constexpr float BankSpacing = 90.f;
	constexpr float PlumeSpacing = 70.f;
	constexpr float HaloSpacing = 100.f;
	constexpr float MoteSpacing = 300.f;
	/** The ground is read this often along a span (cm); a piece stands at a height read between two readings. */
	constexpr float GroundStep = 100.f;

	/** The seam along the ground: a flat glowing strip this wide and tall (cm). */
	constexpr float SeamWidth = 8.f;
	constexpr float SeamHeight = 3.f;

	/** The veil: half its height as a share of the wall's (its middle is at the foot, so only the upper half shows), and its lift. */
	constexpr float VeilReach = 1.15f;
	constexpr float VeilLift = 30.f;
	/** A halo's lift off the ground (cm). */
	constexpr float HaloLift = 14.f;

	// How strongly each kind shows. Fog opacity is how much of the view behind it a piece hides at its middle; a glow's
	// strength multiplies its color (the unlit, additive M_FX_Glow).
	constexpr float VeilOpacity = 0.11f;
	constexpr float BankOpacity = 0.55f;
	constexpr float PlumeOpacity = 0.30f;
	constexpr float MaxOpacity = 0.9f;
	constexpr float HaloGlow = 1.f;
	constexpr float MoteGlow = 6.f;
	constexpr float FlareGlow = 1.5f;
	/** How much more the fog nearest the player shows at full flare, as a share of its own opacity and brightness. */
	constexpr float FlareOpacity = 0.8f;
	constexpr float FlareBrightness = 0.6f;
	/** ...and the halos, as a multiple. */
	constexpr float FlareHalos = 3.f;

	/** A piece thins as the camera gets into it: gone this close, whole from this far (cm). */
	constexpr float NearGone = 35.f;
	constexpr float NearFull = 170.f;
	/** The flare cards' own: they sit where the player touches the wall, a short way off the camera. */
	constexpr float FlareNearGone = 15.f;
	constexpr float FlareNearFull = 80.f;

	constexpr int32 SmokeFloats = 4;
	constexpr int32 LightFloats = 5;
	/** M_FX_Glow's shapes: 0 a streak, 1 a round soft glow. */
	constexpr float RoundShape = 1.f;
	constexpr float TwoPi = 6.2831853f;

	/** Replaces a component's instances with these (moved in place when the count is unchanged), and their custom data. */
	void DrawInstances(UInstancedStaticMeshComponent* Component, const TArray<FTransform>& Transforms, const TArray<float>& CustomData)
	{
		if (!Component)
		{
			return;
		}
		if (Component->GetInstanceCount() != Transforms.Num())
		{
			Component->ClearInstances();
			if (Transforms.Num() > 0)
			{
				Component->AddInstances(Transforms, /*bShouldReturnIndices*/ false, /*bWorldSpace*/ true, /*bUpdateNavigation*/ false);
			}
		}
		else if (Transforms.Num() > 0)
		{
			Component->BatchUpdateInstancesTransforms(0, Transforms, /*bWorldSpace*/ true, /*bMarkRenderStateDirty*/ false, /*bTeleport*/ true);
		}
		if (Transforms.Num() > 0 && CustomData.Num() == Transforms.Num() * Component->NumCustomDataFloats)
		{
			Component->SetCustomData(0, Transforms.Num() - 1, CustomData, false);
		}
		Component->MarkRenderStateDirty();
	}

	/** The hue of the seal's light at its hottest: its color, a share of the way to white. */
	FLinearColor HotColor(const FLinearColor& Tint)
	{
		return Tint + (FLinearColor::White - Tint) * 0.4f;
	}

	/** The fog's color at a height: Low is 1 at the foot (pale and hot, over 1 so the unlit smoke glows) and 0 high up (the seal's own cold blue). */
	FLinearColor FogColor(const FLinearColor& Tint, float Low)
	{
		const FLinearColor Hot = HotColor(Tint) * 1.9f;
		const FLinearColor Cool = Tint * 1.15f;
		return Cool + (Hot - Cool) * Low;
	}

	/**
	 * A quad turned to face the camera, its up along the world's (so a piece stretched tall stays tall on screen), then
	 * rolled about its face by Roll. bUpright turns it about the vertical only, for a panel that stands. The engine's
	 * plane is 100 x 100 in XY, facing +Z: X is the quad's up and Y its width.
	 */
	FQuat Billboard(const FVector& Where, const FVector& Viewer, float Roll, bool bUpright)
	{
		FVector Facing = Viewer - Where;
		if (bUpright)
		{
			Facing.Z = 0.0;
		}
		if (!Facing.Normalize())
		{
			Facing = FVector::ForwardVector;
		}
		return FRotationMatrix::MakeFromZX(Facing, FVector::UpVector).ToQuat() * FQuat(FVector::ZAxisVector, Roll);
	}
}

// ---------------------------------------------------------------------------
// Laying it out
// ---------------------------------------------------------------------------

void ABossSeal::BuildCurtain()
{
	FogPieces.Reset();
	LightPieces.Reset();
	SeamPieces.Reset();
	FloorPath.Reset();
	UWorld* World = GetWorld();
	const TArray<FVector> Path = GetPath();
	if (!World || Path.Num() < 2)
	{
		return;
	}
	// The ground under the line, so the curtain stands on uneven ground (world-static only: never grass or volumes).
	const FCollisionQueryParams Ground = LooterWorld::StaticGeometryParams(World, TEXT("BossSealGround"), this);
	auto GroundUnder = [World, &Ground](const FVector& Point) -> FVector
	{
		FHitResult Hit;
		if (World->LineTraceSingleByObjectType(Hit, Point + FVector(0.0, 0.0, 400.0), Point - FVector(0.0, 0.0, 600.0),
			FCollisionObjectQueryParams(ECC_WorldStatic), Ground))
		{
			return Hit.ImpactPoint;
		}
		return Point;
	};

	// How a climbing piece goes: up Reach in Period seconds, round a flattened circle of its own, one way or the other.
	// (Chosen before the call: the order a call's arguments are drawn in isn't fixed, and the look should be.)
	FRandomStream Random(static_cast<int32>(GetActorLocation().X * 0.37 + GetActorLocation().Y * 1.13));
	auto Climbing = [&Random](FPiece& Piece, float Reach, float Period, float Size, float MinCurl, float MaxCurl)
	{
		Piece.Reach = Reach;
		Piece.Period = Period;
		Piece.Phase = Random.FRand();
		Piece.Size = Size;
		Piece.Curl = Random.FRandRange(MinCurl, MaxCurl);
		const float Speed = Random.FRandRange(0.25f, 0.55f);
		Piece.CurlSpeed = Random.FRand() < 0.5f ? -Speed : Speed;
		Piece.CurlPhase = Random.FRandRange(0.f, TwoPi);
	};

	const int32 Spans = IsClosed() ? Path.Num() : Path.Num() - 1;
	FVector LastEnd = FVector::ZeroVector;
	for (int32 Index = 0; Index < Spans; ++Index)
	{
		const FVector& A = Path[Index];
		const FVector& B = Path[(Index + 1) % Path.Num()];
		const FVector Flat(B.X - A.X, B.Y - A.Y, 0.0);
		const double Length = Flat.Size();
		if (Length < 1.0)
		{
			continue;
		}
		const FVector Along = Flat / Length;

		// The ground's height at stations along the span, and the span's two ends standing on it.
		const int32 Steps = FMath::Max(1, FMath::CeilToInt32(static_cast<float>(Length) / GroundStep));
		TArray<float, TInlineAllocator<8>> Floor;
		Floor.Reserve(Steps + 1);
		for (int32 Station = 0; Station <= Steps; ++Station)
		{
			Floor.Add(static_cast<float>(GroundUnder(FMath::Lerp(A, B, static_cast<float>(Station) / static_cast<float>(Steps))).Z));
		}
		const FVector From(A.X, A.Y, Floor[0]);
		const FVector To(B.X, B.Y, Floor[Steps]);
		FloorPath.Add(From);
		LastEnd = To;
		auto FootAt = [&](float Alpha) -> FVector
		{
			const float Scaled = FMath::Clamp(Alpha, 0.f, 1.f) * static_cast<float>(Steps);
			const int32 Lower = FMath::Clamp(FMath::FloorToInt32(Scaled), 0, Steps - 1);
			return FVector(FMath::Lerp(A.X, B.X, Alpha), FMath::Lerp(A.Y, B.Y, Alpha), FMath::Lerp(Floor[Lower], Floor[Lower + 1], Scaled - static_cast<float>(Lower)));
		};
		// One piece every Spacing along the span (at least one); a jitter keeps the spacing from showing.
		auto Spread = [&](float Spacing, bool bJitter, TFunctionRef<void(const FVector&)> Each)
		{
			const int32 Count = FMath::Max(1, FMath::RoundToInt32(static_cast<float>(Length) / Spacing));
			for (int32 Slot = 0; Slot < Count; ++Slot)
			{
				const float Jitter = bJitter ? Random.FRandRange(0.15f, 0.85f) : 0.5f;
				Each(FootAt((static_cast<float>(Slot) + Jitter) / static_cast<float>(Count)));
			}
		};

		// Fog: veils (the body of the wall), banks (the dense foot) and plumes (the wisps that climb).
		Spread(VeilSpacing, false, [&](const FVector& Foot)
		{
			FPiece& Piece = FogPieces.AddDefaulted_GetRef();
			Piece.Kind = EPiece::Veil;
			Piece.Foot = Foot;
			Piece.Along = Along;
			// Wide enough that its neighbors overlap two and a half deep and the wall's body shows no lumps.
			Piece.Size = VeilSpacing * 2.4f;
			Piece.Seed = Random.FRand();
		});
		Spread(BankSpacing, true, [&](const FVector& Foot)
		{
			FPiece& Piece = FogPieces.AddDefaulted_GetRef();
			Piece.Kind = EPiece::Bank;
			Piece.Foot = Foot;
			Piece.Along = Along;
			const float Reach = Random.FRandRange(70.f, 150.f);
			const float Period = Random.FRandRange(7.f, 12.f);
			const float Size = Random.FRandRange(220.f, 330.f);
			Climbing(Piece, Reach, Period, Size, 25.f, 55.f);
			Piece.Seed = Random.FRand();
		});
		Spread(PlumeSpacing, true, [&](const FVector& Foot)
		{
			FPiece& Piece = FogPieces.AddDefaulted_GetRef();
			Piece.Kind = EPiece::Plume;
			Piece.Foot = Foot;
			Piece.Along = Along;
			// Most climb a little and die low; a few reach the top: the fog thins out with height.
			const float Reach = FMath::Lerp(150.f, Height * 0.95f, FMath::Pow(Random.FRand(), 1.7f));
			const float Period = Reach / Random.FRandRange(55.f, 110.f);
			const float Size = Random.FRandRange(120.f, 190.f);
			Climbing(Piece, Reach, Period, Size, 30.f, 70.f);
			Piece.Seed = Random.FRand();
		});

		// Light: halos along the foot, and motes that climb.
		Spread(HaloSpacing, true, [&](const FVector& Foot)
		{
			FPiece& Piece = LightPieces.AddDefaulted_GetRef();
			Piece.Kind = EPiece::Halo;
			Piece.Foot = Foot;
			Piece.Along = Along;
			Piece.Size = Random.FRandRange(150.f, 230.f);
			Piece.Seed = Random.FRand();
		});
		Spread(MoteSpacing, true, [&](const FVector& Foot)
		{
			FPiece& Piece = LightPieces.AddDefaulted_GetRef();
			Piece.Kind = EPiece::Mote;
			Piece.Foot = Foot;
			Piece.Along = Along;
			const float Reach = Random.FRandRange(FMath::Min(250.f, Height), Height);
			const float Period = Reach / Random.FRandRange(90.f, 150.f);
			const float Size = Random.FRandRange(14.f, 26.f);
			Climbing(Piece, Reach, Period, Size, 20.f, 50.f);
			Piece.Seed = Random.FRand();
		});

		// The seam: a flat strip from end to end of the span, along the ground.
		const FVector Strip = To - From;
		const float Span = static_cast<float>(Strip.Size());
		if (Span > 1.f)
		{
			SeamPieces.Add(FTransform(FRotationMatrix::MakeFromX(Strip).ToQuat(), (From + To) * 0.5 + FVector(0.0, 0.0, SeamHeight * 0.5),
				FVector((Span + SeamWidth) / 100.f, SeamWidth / 100.f, SeamHeight / 100.f)));
		}
	}
	if (!IsClosed() && !FloorPath.IsEmpty())
	{
		FloorPath.Add(LastEnd);
	}
}

// ---------------------------------------------------------------------------
// Drawing it
// ---------------------------------------------------------------------------

FVector ABossSeal::Climb(const FPiece& Piece, float& OutTrip, FVector& OutVelocity) const
{
	const float Period = FMath::Max(Piece.Period, 0.5f);
	OutTrip = FMath::Frac(Piece.Phase + Clock / Period);
	// A flattened circle round its column: far along the wall, less through it; wider as it climbs.
	const float Turn = Piece.CurlPhase + Clock * Piece.CurlSpeed;
	const float Sweep = Piece.Curl * (0.35f + 0.65f * OutTrip);
	const float Sine = FMath::Sin(Turn);
	const float Cosine = FMath::Cos(Turn);
	const FVector Across = FVector::CrossProduct(FVector::UpVector, Piece.Along);
	OutVelocity = Piece.Along * (Cosine * Piece.CurlSpeed * Sweep) - Across * (Sine * Piece.CurlSpeed * Sweep * 0.45f)
		+ FVector::UpVector * (Piece.Reach / Period);
	return Piece.Foot + Piece.Along * (Sine * Sweep) + Across * (Cosine * Sweep * 0.45f) + FVector::UpVector * (Piece.Reach * OutTrip);
}

void ABossSeal::DrawCurtain()
{
	// The curtain's top is its height times how far it has risen, so it grows up out of the ground and sinks back; a piece
	// that shows nothing keeps its instance at no size, so the counts never change and the instances are only moved.
	const UWorld* World = GetWorld();
	const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
	FCurtainView View;
	// Before a camera exists, the quads face as if seen from above the middle.
	View.Viewer = GetActorLocation() + FVector(0.0, 0.0, Height);
	if (Controller && Controller->PlayerCameraManager)
	{
		View.Viewer = Controller->PlayerCameraManager->GetCameraLocation();
	}
	if (const APawn* Pawn = Controller ? Controller->GetPawn() : nullptr)
	{
		View.Player = Pawn->GetActorLocation();
		View.bPlayer = true;
	}
	View.Top = Height * FMath::InterpEaseOut(0.f, 1.f, Rise, 2.f);
	View.Strength = FMath::SmoothStep(0.f, 0.35f, Rise);
	DrawFog(View);
	DrawLights(View);

	// The seam shows whenever the curtain does, flattening into the ground as it sinks.
	TArray<FTransform> Pieces = SeamPieces;
	for (FTransform& Piece : Pieces)
	{
		FVector Scale = Piece.GetScale3D();
		Scale.Z *= Rise;
		Piece.SetScale3D(Scale);
	}
	DrawInstances(Seam, Pieces, {});
}

void ABossSeal::DrawFog(const FCurtainView& View)
{
	TArray<FTransform> Transforms;
	TArray<float> Data;
	Transforms.Reserve(FogPieces.Num());
	Data.Reserve(FogPieces.Num() * SmokeFloats);
	const float Grown = View.Top / FMath::Max(Height, 1.f);

	for (const FPiece& Piece : FogPieces)
	{
		FVector Where = Piece.Foot;
		float Wide = 0.f;
		float Tall = 0.f;
		float Opacity = 0.f;
		float Low = 0.5f;
		float Tilt = 0.f;
		bool bUpright = false;
		// The whole wall swells and ebbs a little, each piece in its own time: a slight shimmer.
		const float Breath = 0.88f + 0.12f * FMath::Sin(Clock * 1.1f + Piece.Seed * TwoPi);

		if (Piece.Kind == EPiece::Veil)
		{
			// Centered at the foot and as tall as the wall's height and a bit over: its glow is brightest in the middle, so only
			// the faint upper half shows, thinning to nothing by the top.
			bUpright = true;
			Where = Piece.Foot + FVector(0.0, 0.0, VeilLift);
			Wide = Piece.Size;
			Tall = 2.f * Height * VeilReach * Grown;
			Opacity = VeilOpacity * Breath;
		}
		else
		{
			const bool bBank = Piece.Kind == EPiece::Bank;
			float Trip = 0.f;
			FVector Velocity = FVector::ZeroVector;
			Where = Climb(Piece, Trip, Velocity);
			const float Climbed = Piece.Reach * Trip;
			// Swells as it's born and thins as it goes; a bank is a squat billow, a plume a wisp twice as tall as wide.
			const float Born = FMath::SmoothStep(0.f, bBank ? 0.2f : 0.15f, Trip);
			const float Gone = 1.f - FMath::SmoothStep(bBank ? 0.5f : 0.55f, 1.f, Trip);
			Wide = Piece.Size * (bBank ? FMath::Lerp(0.7f, 1.1f, Trip) : FMath::Lerp(0.55f, 1.f, Trip));
			Tall = Wide * (bBank ? 0.62f : 1.7f);
			// Standing on its foot, not centered on it.
			Where.Z += Tall * (bBank ? 0.25f : 0.3f);
			Low = 1.f - FMath::SmoothStep(0.f, Height * 0.7f, Climbed);
			// Cut off at the curtain's top while it rises or sinks.
			const float Shown = 1.f - FMath::SmoothStep(View.Top - 180.f, View.Top + 120.f, Climbed);
			Opacity = (bBank ? BankOpacity : PlumeOpacity * BossSealFog::Density(Climbed, Height)) * Born * Gone * Breath * Shown;
			if (!bBank)
			{
				// Leans the way it drifts across the view, so it streams instead of standing like a post.
				const FVector Right = FVector::CrossProduct((View.Viewer - Where).GetSafeNormal(), FVector::UpVector).GetSafeNormal();
				Tilt = FMath::Clamp(FMath::Atan2(static_cast<float>(FVector::DotProduct(Velocity, Right)), FMath::Max(static_cast<float>(Velocity.Z), 1.f)), -0.6f, 0.6f);
			}
		}

		// Brighter and denser near the player; thinned as the camera gets into it, and with the curtain's rise.
		const float Flare = View.bPlayer ? BossSealFog::Flare(static_cast<float>(FVector::Dist(Where, View.Player))) : 0.f;
		const float Near = FMath::SmoothStep(NearGone, NearFull, static_cast<float>(FVector::Dist(Where, View.Viewer)));
		Opacity = FMath::Min(Opacity * (1.f + FlareOpacity * Flare), MaxOpacity) * Near * View.Strength;
		if (Opacity < 0.004f || Wide < 1.f || Tall < 1.f)
		{
			Transforms.Add(FTransform(FQuat::Identity, Where, FVector::ZeroVector));
			Data.Append({ 0.f, 0.f, 0.f, 0.f });
			continue;
		}
		const FLinearColor Shade = FogColor(Color, Low) * (1.f + FlareBrightness * Flare);
		Transforms.Add(FTransform(Billboard(Where, View.Viewer, Tilt, bUpright), Where, FVector(Tall / 100.f, Wide / 100.f, 1.f)));
		Data.Append({ Shade.R, Shade.G, Shade.B, Opacity });
	}
	DrawInstances(Fog, Transforms, Data);
}

void ABossSeal::DrawLights(const FCurtainView& View)
{
	TArray<FTransform> Transforms;
	TArray<float> Data;
	// The pieces, and the two cards where the player touches the wall.
	Transforms.Reserve(LightPieces.Num() + 2);
	Data.Reserve((LightPieces.Num() + 2) * LightFloats);
	const FLinearColor Hot = HotColor(Color);

	auto AddLight = [&](const FVector& Where, float Size, float Strength, float NearFrom, float NearTo)
	{
		const float Near = FMath::SmoothStep(NearFrom, NearTo, static_cast<float>(FVector::Dist(Where, View.Viewer)));
		const float Shown = Strength * Near * View.Strength;
		if (Shown < 0.01f || Size < 1.f)
		{
			Transforms.Add(FTransform(FQuat::Identity, Where, FVector::ZeroVector));
			Data.Append({ 0.f, 0.f, 0.f, 0.f, RoundShape });
			return;
		}
		Transforms.Add(FTransform(Billboard(Where, View.Viewer, 0.f, false), Where, FVector(Size / 100.f, Size / 100.f, 1.f)));
		Data.Append({ Hot.R, Hot.G, Hot.B, Shown, RoundShape });
	};

	for (const FPiece& Piece : LightPieces)
	{
		const float Shimmer = 0.8f + 0.2f * FMath::Sin(Clock * 3.1f + Piece.Seed * TwoPi * 1.7f);
		FVector Where = Piece.Foot + FVector(0.0, 0.0, HaloLift);
		float Strength = HaloGlow * Shimmer;
		float Climbed = 0.f;
		if (Piece.Kind == EPiece::Mote)
		{
			float Trip = 0.f;
			FVector Velocity = FVector::ZeroVector;
			Where = Climb(Piece, Trip, Velocity);
			Climbed = Piece.Reach * Trip;
			// Lights as it leaves the foot, winks out at the top of its trip, and is cut off at the curtain's top.
			Strength = MoteGlow * Shimmer * FMath::Sin(static_cast<float>(UE_PI) * Trip) * (1.f - FMath::SmoothStep(View.Top - 120.f, View.Top + 60.f, Climbed));
		}
		const float Flare = View.bPlayer ? BossSealFog::Flare(static_cast<float>(FVector::Dist(Where, View.Player))) : 0.f;
		Strength *= Piece.Kind == EPiece::Halo ? 1.f + FlareHalos * Flare : 1.f + Flare;
		AddLight(Where, Piece.Size, Strength, NearGone, NearFull);
	}

	// Where the player is nearest the wall, a glow gathers at the player's height: stronger the closer, none from four metres off.
	float Touch = 0.f;
	FVector Contact = FVector::ZeroVector;
	if (View.bPlayer && FloorPath.Num() > 1)
	{
		const float Away = BossSealFog::NearestOnPath(FloorPath, IsClosed(), View.Player, Contact);
		Touch = BossSealFog::Flare(Away);
		Contact.Z += FMath::Clamp(static_cast<float>(View.Player.Z - Contact.Z), 40.f, FMath::Max(Height - 150.f, 40.f));
	}
	AddLight(Contact, 330.f, FlareGlow * Touch, FlareNearGone, FlareNearFull);
	AddLight(Contact, 110.f, FlareGlow * 1.6f * Touch, FlareNearGone, FlareNearFull);
	DrawInstances(Glow, Transforms, Data);
}

void ABossSeal::ClearCurtain()
{
	DrawInstances(Fog, TArray<FTransform>(), {});
	DrawInstances(Glow, TArray<FTransform>(), {});
	DrawInstances(Seam, TArray<FTransform>(), {});
}

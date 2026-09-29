#include "World/MinimapSubsystem.h"
#include "AI_Looter_Shooter.h"
#include "Environment/StylizedProp.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/PlatformTime.h"

namespace
{
	enum : uint8 { Void = 0, Ground = 1, Obstacle = 2 };

	/** Seconds of tracing per frame while baking. */
	constexpr double BakeBudget = 0.003;
	/** Height step (cm) between neighboring texels that reads as a cliff edge. */
	constexpr float CliffStep = 70.f;
	/** Height between contour lines (cm). */
	constexpr float ContourInterval = 200.f;

	bool IsGroundShape(EStylizedPropShape Shape)
	{
		return Shape == EStylizedPropShape::Terrain || Shape == EStylizedPropShape::IslandTerrain
			|| Shape == EStylizedPropShape::Hill || Shape == EStylizedPropShape::Cliff;
	}

	// The map's palette: the HUD's dark glass and cyan lines. Land is kept low-contrast so markers stand out.
	const FColor LandLow(32, 82, 96, 225);
	const FColor LandHigh(66, 124, 138, 230);
	const FColor ContourColor(92, 156, 170, 235);
	const FColor ObstacleColor(176, 218, 232, 235);
	const FColor CoastColor(108, 212, 255, 255);
	const FColor CliffColor(150, 206, 224, 240);

	FColor Shade(const FColor& Color, float Amount)
	{
		auto Channel = [Amount](uint8 Value) { return static_cast<uint8>(FMath::Clamp(Value * (1.f + Amount), 0.f, 255.f)); };
		return FColor(Channel(Color.R), Channel(Color.G), Channel(Color.B), Color.A);
	}
}

bool UMinimapSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

FVector2D UMinimapSubsystem::WorldToMapUV(const FBox2D& MapBounds, const FVector& World)
{
	const FVector2D Size = MapBounds.GetSize();
	return FVector2D(
		Size.Y > 0.f ? (World.Y - MapBounds.Min.Y) / Size.Y : 0.5f,
		Size.X > 0.f ? (MapBounds.Max.X - World.X) / Size.X : 0.5f);
}

FVector2D UMinimapSubsystem::ViewOffset(const FVector& WorldDelta, float ViewYaw, float PixelsPerCm)
{
	float Sin, Cos;
	FMath::SinCos(&Sin, &Cos, FMath::DegreesToRadians(ViewYaw));
	const float Ahead = WorldDelta.X * Cos + WorldDelta.Y * Sin;
	const float Right = -WorldDelta.X * Sin + WorldDelta.Y * Cos;
	return FVector2D(Right, -Ahead) * PixelsPerCm;
}

UTexture2D* UMinimapSubsystem::GetMapTexture()
{
	if (!bBaked && !bBaking)
	{
		StartBake();
	}
	return Texture;
}

void UMinimapSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (bBaking)
	{
		TraceRows(BakeBudget);
	}
}

void UMinimapSubsystem::StartBake()
{
	bBaked = true;
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Map the ground you can walk on: the terrain props that make up the islands.
	FBox GroundBox(ForceInit);
	for (TActorIterator<AStylizedProp> It(World); It; ++It)
	{
		if (IsGroundShape(It->Shape))
		{
			GroundBox += It->GetComponentsBoundingBox();
		}
	}
	if (!GroundBox.IsValid)
	{
		return;
	}

	// A square a little larger than the ground, so the coast never touches the edge of the picture.
	const FVector2D Center(GroundBox.GetCenter().X, GroundBox.GetCenter().Y);
	const double Half = FMath::Max(GroundBox.GetExtent().X, GroundBox.GetExtent().Y) * 1.05;
	Bounds = FBox2D(Center - FVector2D(Half), Center + FVector2D(Half));
	TraceTop = GroundBox.Max.Z + 1000.f;
	TraceBottom = GroundBox.Min.Z - 1000.f;

	Heights.Init(TNumericLimits<float>::Lowest(), Resolution * Resolution);
	Kinds.Init(Void, Resolution * Resolution);
	NextRow = 0;
	bBaking = true;
	BakeStartTime = FPlatformTime::Seconds();
}

void UMinimapSubsystem::TraceRows(double TimeBudgetSeconds)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		bBaking = false;
		return;
	}

	const double Deadline = FPlatformTime::Seconds() + TimeBudgetSeconds;
	const FCollisionQueryParams Params(SCENE_QUERY_STAT(MinimapBake), false);
	const FCollisionObjectQueryParams Objects(ECC_WorldStatic);
	const FVector2D Size = Bounds.GetSize();
	while (NextRow < Resolution && FPlatformTime::Seconds() < Deadline)
	{
		const int32 V = NextRow++;
		const double X = Bounds.Max.X - (V + 0.5) / Resolution * Size.X;
		for (int32 U = 0; U < Resolution; ++U)
		{
			const double Y = Bounds.Min.Y + (U + 0.5) / Resolution * Size.Y;
			FHitResult Hit;
			if (World->LineTraceSingleByObjectType(Hit, FVector(X, Y, TraceTop), FVector(X, Y, TraceBottom), Objects, Params))
			{
				const int32 Index = V * Resolution + U;
				Heights[Index] = Hit.ImpactPoint.Z;
				const AStylizedProp* Prop = Cast<AStylizedProp>(Hit.GetActor());
				Kinds[Index] = Prop && !IsGroundShape(Prop->Shape) ? Obstacle : Ground;
			}
		}
	}
	if (NextRow >= Resolution)
	{
		FinishBake();
	}
}

void UMinimapSubsystem::FinishBake()
{
	bBaking = false;
	const int32 N = Resolution;

	float MinZ = TNumericLimits<float>::Max();
	float MaxZ = TNumericLimits<float>::Lowest();
	for (int32 Index = 0; Index < N * N; ++Index)
	{
		if (Kinds[Index] == Ground)
		{
			MinZ = FMath::Min(MinZ, Heights[Index]);
			MaxZ = FMath::Max(MaxZ, Heights[Index]);
		}
	}
	const float Range = FMath::Max(MaxZ - MinZ, 1.f);

	auto KindAt = [this, N](int32 U, int32 V) { return (U < 0 || V < 0 || U >= N || V >= N) ? static_cast<uint8>(Void) : Kinds[V * N + U]; };
	auto HeightAt = [this, N](int32 U, int32 V, float Fallback) { return (U < 0 || V < 0 || U >= N || V >= N || Kinds[V * N + U] == Void) ? Fallback : Heights[V * N + U]; };

	TArray<FColor> Colors;
	Colors.Init(FColor(0, 0, 0, 0), N * N);
	for (int32 V = 0; V < N; ++V)
	{
		for (int32 U = 0; U < N; ++U)
		{
			const int32 Index = V * N + U;
			const uint8 Kind = Kinds[Index];
			if (Kind == Void)
			{
				continue;
			}
			const float Height = Heights[Index];
			const bool bCoast = KindAt(U - 1, V) == Void || KindAt(U + 1, V) == Void || KindAt(U, V - 1) == Void || KindAt(U, V + 1) == Void;
			if (bCoast)
			{
				Colors[Index] = CoastColor;
				continue;
			}
			if (Kind == Obstacle)
			{
				Colors[Index] = ObstacleColor;
				continue;
			}
			float Steepest = 0.f;
			bool bContour = false;
			const int32 Band = FMath::FloorToInt(Height / ContourInterval);
			for (const FIntPoint Step : { FIntPoint(1, 0), FIntPoint(-1, 0), FIntPoint(0, 1), FIntPoint(0, -1) })
			{
				if (KindAt(U + Step.X, V + Step.Y) == Ground)
				{
					const float Neighbor = HeightAt(U + Step.X, V + Step.Y, Height);
					Steepest = FMath::Max(Steepest, FMath::Abs(Neighbor - Height));
					// Draw each contour once, on its uphill side.
					bContour |= FMath::FloorToInt(Neighbor / ContourInterval) < Band;
				}
			}
			if (Steepest > CliffStep)
			{
				Colors[Index] = CliffColor;
				continue;
			}
			if (bContour)
			{
				Colors[Index] = ContourColor;
				continue;
			}
			// Height tint, plus a touch of hillshade lit from the north-west so slopes read.
			const FLinearColor Low(LandLow), High(LandHigh);
			const FColor Base = FLinearColor::LerpUsingHSV(Low, High, (Height - MinZ) / Range).ToFColor(true);
			const float Slope = (Height - HeightAt(U - 1, V - 1, Height)) / 80.f;
			Colors[Index] = Shade(Base, FMath::Clamp(Slope, -0.1f, 0.1f));
		}
	}

	// FColor is laid out B, G, R, A in memory, which is exactly PF_B8G8R8A8.
	const TArrayView<const uint8> Bytes(reinterpret_cast<const uint8*>(Colors.GetData()), Colors.Num() * sizeof(FColor));
	UTexture2D* Baked = UTexture2D::CreateTransient(N, N, PF_B8G8R8A8, NAME_None, Bytes);
	if (Baked)
	{
		Baked->SRGB = true;
		Baked->Filter = TF_Bilinear;
		Baked->AddressX = TA_Clamp;
		Baked->AddressY = TA_Clamp;
		Baked->NeverStream = true;
		Baked->UpdateResource();
		Texture = Baked;
	}
	Heights.Empty();
	Kinds.Empty();
	UE_LOG(LogLooter, Log, TEXT("Minimap baked: %dx%d over %.0f m in %.2f s"), N, N, Bounds.GetSize().X / 100.0, FPlatformTime::Seconds() - BakeStartTime);
}

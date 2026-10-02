#include "World/MinimapSubsystem.h"
#include "AI_Looter_Shooter.h"
#include "World/PlayableArea.h"
#include "World/WorldQueries.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/PlatformTime.h"

namespace
{
	/** Seconds of tracing per frame while baking. */
	constexpr double BakeBudget = 0.003;
	/** The map's texel size the resolution aims for (cm), and the steps it moves in. */
	constexpr double TexelAim = 100.0;
	constexpr int32 ResolutionStep = 64;

	/** Terrain the map is made of. */
	bool IsGroundActor(const AActor* Actor)
	{
		return Actor && Actor->ActorHasTag(MinimapTags::Ground);
	}

	/** Something standing on the ground (rocks, walls, trees). */
	bool IsObstacleActor(const AActor* Actor)
	{
		return Actor && Actor->ActorHasTag(MinimapTags::Obstacle);
	}

	// Tree crowns: a deeper green-teal than the land, with a darker rim so neighbouring crowns stay apart.
	const FColor TreeColor(46, 120, 96, 240);
	const FColor TreeRimColor(26, 78, 66, 240);
	/** Share of a tree mesh's footprint its crown covers on the map. */
	constexpr float CrownShare = 0.8f;
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

int32 UMinimapSubsystem::ResolutionFor(double MapSize)
{
	const int32 Steps = FMath::RoundToInt32(FMath::Max(MapSize, 0.0) / TexelAim / ResolutionStep);
	return FMath::Clamp(Steps * ResolutionStep, 256, 1024);
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

	// Map the ground you can walk on: the terrain that makes up the islands, or the valley and its ridge feet.
	FBox GroundBox(ForceInit);
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (IsGroundActor(*It))
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
	Resolution = ResolutionFor(Bounds.GetSize().X);
	TraceTop = GroundBox.Max.Z + 1000.f;
	TraceBottom = GroundBox.Min.Z - 1000.f;
	TraceParams = LooterWorld::StaticGeometryParams(World, TEXT("MinimapBake"), nullptr, false);
	PlayableArea = APlayableArea::Find(World);

	Heights.Init(TNumericLimits<float>::Lowest(), Resolution * Resolution);
	Kinds.Init(EMinimapTexel::Void, Resolution * Resolution);
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
			if (World->LineTraceSingleByObjectType(Hit, FVector(X, Y, TraceTop), FVector(X, Y, TraceBottom), Objects, TraceParams))
			{
				const int32 Index = V * Resolution + U;
				Heights[Index] = Hit.ImpactPoint.Z;
				Kinds[Index] = IsObstacleActor(Hit.GetActor()) ? EMinimapTexel::Obstacle : EMinimapTexel::Ground;
			}
		}
	}
	if (NextRow >= Resolution)
	{
		FinishBake();
	}
}

void UMinimapSubsystem::PaintTrees(TArray<FColor>& Colors) const
{
	const int32 N = Resolution;
	const FVector2D Size = Bounds.GetSize();
	const double CmPerTexel = FMath::Max(Size.X, 1.0) / N;
	auto Paint = [&](const FVector& Where, double CrownRadius)
	{
		const FVector2D UV = WorldToMapUV(Where) * N;
		const double Radius = FMath::Clamp(CrownRadius / CmPerTexel, 1.5, 12.0);
		const int32 R = FMath::CeilToInt(Radius);
		for (int32 V = FMath::FloorToInt(UV.Y) - R; V <= FMath::FloorToInt(UV.Y) + R; ++V)
		{
			for (int32 U = FMath::FloorToInt(UV.X) - R; U <= FMath::FloorToInt(UV.X) + R; ++U)
			{
				const double Distance = FVector2D::Distance(FVector2D(U + 0.5, V + 0.5), UV);
				if (U < 0 || V < 0 || U >= N || V >= N || Distance > Radius || Colors[V * N + U].A == 0)
				{
					continue;
				}
				Colors[V * N + U] = Distance > Radius - 1.0 ? TreeRimColor : TreeColor;
			}
		}
	};

	int32 Trees = 0;
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		const AActor* Actor = *It;
		if (Actor->ActorHasTag(MinimapTags::Tree))
		{
			const FBox Box = Actor->GetComponentsBoundingBox();
			Paint(Box.GetCenter(), FMath::Max(Box.GetExtent().X, Box.GetExtent().Y) * CrownShare);
			++Trees;
			continue;
		}
		TInlineComponentArray<UInstancedStaticMeshComponent*> Instanced(Actor);
		for (const UInstancedStaticMeshComponent* Component : Instanced)
		{
			const UStaticMesh* Mesh = Component->GetStaticMesh();
			if (!Mesh || !Component->ComponentHasTag(MinimapTags::Tree))
			{
				continue;
			}
			const FVector Extent = Mesh->GetBoundingBox().GetExtent();
			const double Footprint = FMath::Max(Extent.X, Extent.Y) * CrownShare;
			for (int32 Index = 0; Index < Component->GetInstanceCount(); ++Index)
			{
				FTransform Instance;
				Component->GetInstanceTransform(Index, Instance, /*bWorldSpace*/ true);
				Paint(Instance.GetLocation(), Footprint * Instance.GetScale3D().X);
				++Trees;
			}
		}
	}
	UE_LOG(LogLooter, Log, TEXT("Minimap: %d trees"), Trees);
}

void UMinimapSubsystem::FinishBake()
{
	bBaking = false;
	const int32 N = Resolution;

	// With a playable area, which texels lie inside it and which carry its boundary line; the land's tint then spans
	// only the heights inside (the ridges around would squash it), unless nothing inside was found.
	TArray<uint8> Inside;
	TArray<uint8> Line;
	if (const APlayableArea* Area = PlayableArea.Get())
	{
		MinimapPaint::RasterizeBoundary(Area->GetBoundary(), Bounds, N, Inside, Line);
	}
	float MinZ = 0.f;
	float MaxZ = 0.f;
	if (!MinimapPaint::HeightRange(Kinds, Heights, Inside, MinZ, MaxZ) && Inside.Num() > 0)
	{
		MinimapPaint::HeightRange(Kinds, Heights, TConstArrayView<uint8>(), MinZ, MaxZ);
	}

	TArray<FColor> Colors;
	MinimapPaint::Terrain(N, Kinds, Heights, MinZ, MaxZ, Colors);
	if (Colors.Num() != N * N)
	{
		Colors.Init(FColor(0, 0, 0, 0), N * N);
	}
	PaintTrees(Colors);
	// Last, so the trees outside are dimmed too and none covers the line.
	if (Inside.Num() > 0)
	{
		MinimapPaint::ApplyBoundary(Inside, Line, Colors);
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
	UE_LOG(LogLooter, Log, TEXT("Minimap baked: %dx%d over %.0f m in %.2f s%s"), N, N, Bounds.GetSize().X / 100.0,
		FPlatformTime::Seconds() - BakeStartTime, Inside.Num() > 0 ? TEXT(", with the playable area's boundary") : TEXT(""));
}

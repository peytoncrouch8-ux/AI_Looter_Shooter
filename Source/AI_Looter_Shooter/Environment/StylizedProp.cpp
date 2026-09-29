#include "Environment/StylizedProp.h"
#include "Environment/StylizedMeshKit.h"
#include "Environment/StylizedSurface.h"
#include "Components/DynamicMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "DynamicMesh/DynamicMeshAttributeSet.h"
#include "GeometryScript/MeshPrimitiveFunctions.h"
#include "UDynamicMesh.h"

using namespace StylizedMesh;
using StylizedColors::Hex;
using UE::Geometry::FDynamicMesh3;
using UE::Geometry::FDynamicMeshMaterialAttribute;

namespace
{
	/** Everything a shape builder needs, and what it hands back besides the mesh. */
	struct FPropBuild
	{
		UDynamicMesh* Mesh = nullptr;
		const AActor* Owner = nullptr;   // for probing the world (terrain-conforming shapes)
		FTransform Transform;
		FRandomStream Random;
		FLinearColor Primary;
		FLinearColor Secondary;
		float NoiseSeed = 0.f;

		TArray<FStylizedSurface> Surfaces;
		float SmoothAngle = 0.f;
		float CullDistance = 0.f;
		bool bCastShadow = true;

		// Optional light pillar and glow light.
		float BeamHeight = 0.f;
		float BeamRadius = 0.f;
		FLinearColor BeamColor = FLinearColor::White;
		float LightIntensity = 0.f;
		float LightRadius = 0.f;
		FVector LightOffset = FVector::ZeroVector;
		FLinearColor LightColor = FLinearColor::White;
	};

	// Shared palette bits that aren't worth a palette slot of their own.
	FLinearColor Earth()     { return Hex(0x8a6a4a); }
	FLinearColor Moss()      { return Hex(0x86a24c); }
	FLinearColor DarkMetal() { return Hex(0x3a3f47); }
	FLinearColor Bark()      { return Hex(0x6b4a35); }
	FLinearColor Leaves()    { return Hex(0x7fae44); }

	FRotator RandomYaw(FRandomStream& Random)
	{
		return FRotator(0.f, Random.FRandRange(0.f, 360.f), 0.f);
	}

	FVector2D RandomInDisc(FRandomStream& Random, float Radius)
	{
		const float Angle = Random.FRandRange(0.f, UE_TWO_PI);
		const float Distance = FMath::Sqrt(Random.FRand()) * Radius;
		return FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Distance;
	}

	FStylizedSurface GroundSurface(const FLinearColor& Dirt, const FLinearColor& Grass)
	{
		FStylizedSurface Surface;
		Surface.Color = Dirt;
		Surface.TopColor = Grass;
		Surface.TopBlend = 1.f;
		Surface.TopThreshold = 0.8f;
		Surface.Variation = 0.35f;
		return Surface;
	}

	FStylizedSurface RockSurface(const FLinearColor& Stone, const FLinearColor& TopCover, float Threshold = 0.72f, float Strata = 0.f)
	{
		FStylizedSurface Surface;
		Surface.Color = Stone;
		Surface.TopColor = TopCover;
		Surface.TopBlend = 1.f;
		Surface.TopThreshold = Threshold;
		Surface.Strata = Strata;
		Surface.Variation = 0.12f;
		return Surface;
	}

	FStylizedSurface FoliageSurface(const FLinearColor& Leaf, float GradHeight, float Wind, float GradDark = 0.35f, float UpNormal = 0.3f)
	{
		FStylizedSurface Surface;
		Surface.Color = Leaf;
		Surface.GradHeight = GradHeight;
		Surface.GradDark = GradDark;
		Surface.Wind = Wind;
		Surface.Variation = 0.2f;
		Surface.UpNormal = UpNormal;
		return Surface;
	}

	FStylizedSurface GlowSurface(const FLinearColor& Color, float Glow)
	{
		FStylizedSurface Surface = FStylizedSurface::Solid(Color, 0.06f);
		Surface.Glow = Glow;
		return Surface;
	}

	// -----------------------------------------------------------------------
	// Ground
	// -----------------------------------------------------------------------

	/**
	 * A floating island: an irregular, gently rolling top (slot 0) on a cliff skirt that tapers into a
	 * jagged underside (slot 1), with hanging spikes and loose chunks drifting around it. The origin is the
	 * center of the top surface, so placing it puts the walkable top where you click.
	 */
	struct FIslandSettings
	{
		float Radius = 1500.f;
		int32 Rings = 8;
		int32 Segments = 32;
		float Relief = 100.f;
		float Depth = 2000.f;
		float FlatCenter = 0.f;
		int32 Spikes = 6;
		int32 Chunks = 4;
	};

	void BuildIsland(FPropBuild& Build, const FIslandSettings& Settings)
	{
		FRandomStream& Random = Build.Random;
		const float Radius = Settings.Radius;
		const float Phase1 = Random.FRandRange(0.f, 6.f);
		const float Phase2 = Random.FRandRange(0.f, 6.f);
		const float Phase3 = Random.FRandRange(0.f, 6.f);
		const FVector2D Offset1(Random.FRandRange(-50.f, 50.f), Random.FRandRange(-50.f, 50.f));
		const FVector2D Offset2(Random.FRandRange(-50.f, 50.f), Random.FRandRange(-50.f, 50.f));

		auto Outline = [&](float Angle)
		{
			return Radius * (1.f + 0.09f * FMath::Sin(3.f * Angle + Phase1) + 0.05f * FMath::Sin(5.f * Angle + Phase2)
				+ 0.035f * FMath::Sin(9.f * Angle + Phase3));
		};

		auto Height = [&](float X, float Y, float EdgeFraction)
		{
			const FVector2D P(X, Y);
			float H = Settings.Relief * (0.75f * FMath::PerlinNoise2D(P / (Radius * 0.45f) + Offset1)
				+ 0.3f * FMath::PerlinNoise2D(P / (Radius * 0.12f) + Offset2));
			if (Settings.FlatCenter > 0.f)
			{
				// Keep the spawn area level.
				H *= FMath::SmoothStep(Settings.FlatCenter * 0.4f, Settings.FlatCenter, static_cast<float>(P.Size()));
			}
			// Roll the top off toward the rim so the edge reads as a soft lip, not a table edge.
			H -= Settings.Relief * 0.6f * FMath::SmoothStep(0.86f, 1.f, EdgeFraction);
			return H;
		};

		const int32 Segments = Settings.Segments;
		int32 FirstSkirtVertex = 0;

		Build.Mesh->EditMesh([&](FDynamicMesh3& Mesh)
		{
			if (!Mesh.HasAttributes())
			{
				Mesh.EnableAttributes();
			}
			if (!Mesh.Attributes()->HasMaterialID())
			{
				Mesh.Attributes()->EnableMaterialID();
			}
			FDynamicMeshMaterialAttribute* MaterialIDs = Mesh.Attributes()->GetMaterialID();
			auto AddTriangle = [&](int32 A, int32 B, int32 C, int32 MaterialID)
			{
				// Unreal is left-handed: listing corners clockwise when seen from outside makes the face point out.
				const int32 Triangle = Mesh.AppendTriangle(A, C, B);
				if (Triangle >= 0)
				{
					MaterialIDs->SetValue(Triangle, MaterialID);
				}
			};
			auto Stitch = [&](const TArray<int32>& Upper, const TArray<int32>& Lower, int32 MaterialID)
			{
				for (int32 Segment = 0; Segment < Segments; ++Segment)
				{
					const int32 Next = (Segment + 1) % Segments;
					AddTriangle(Upper[Segment], Lower[Segment], Lower[Next], MaterialID);
					AddTriangle(Upper[Segment], Lower[Next], Upper[Next], MaterialID);
				}
			};

			// Top: concentric rings out to the irregular outline.
			const int32 Center = Mesh.AppendVertex(FVector3d(0.0, 0.0, Height(0.f, 0.f, 0.f)));
			TArray<int32> Previous;
			for (int32 Ring = 1; Ring <= Settings.Rings; ++Ring)
			{
				const float Fraction = static_cast<float>(Ring) / Settings.Rings;
				TArray<int32> Current;
				for (int32 Segment = 0; Segment < Segments; ++Segment)
				{
					const float Angle = UE_TWO_PI * Segment / Segments;
					const float Distance = Outline(Angle) * Fraction;
					const float X = Distance * FMath::Cos(Angle);
					const float Y = Distance * FMath::Sin(Angle);
					Current.Add(Mesh.AppendVertex(FVector3d(X, Y, Height(X, Y, Fraction))));
				}
				if (Ring == 1)
				{
					for (int32 Segment = 0; Segment < Segments; ++Segment)
					{
						AddTriangle(Center, Current[Segment], Current[(Segment + 1) % Segments], 0);
					}
				}
				else
				{
					Stitch(Previous, Current, 0);
				}
				Previous = Current;
			}

			// Skirt: a short overhanging cliff lip, then a jagged taper down to a point.
			FirstSkirtVertex = Mesh.MaxVertexID();
			const TArray<int32> Rim = Previous;
			const int32 SkirtRings = 9;
			for (int32 Ring = 1; Ring <= SkirtRings; ++Ring)
			{
				const float Fraction = static_cast<float>(Ring) / SkirtRings;
				TArray<int32> Current;
				for (int32 Segment = 0; Segment < Segments; ++Segment)
				{
					const FVector3d RimPoint = Mesh.GetVertex(Rim[Segment]);
					const float Angle = UE_TWO_PI * Segment / Segments;
					float Shrink = 1.f - FMath::Pow(Fraction, 1.7f) * 0.96f;
					Shrink *= 1.f + 0.2f * FMath::PerlinNoise2D(FVector2D(Angle * 2.5f, Fraction * 3.f) + Offset1) * FMath::Min(1.f, Fraction * 4.f);
					float Drop = Settings.Depth * FMath::Pow(Fraction, 1.25f);
					if (Ring == 1)
					{
						Shrink = 1.015f;
						Drop += Settings.Depth * 0.03f;
					}
					Current.Add(Mesh.AppendVertex(FVector3d(RimPoint.X * Shrink, RimPoint.Y * Shrink, RimPoint.Z - Drop)));
				}
				Stitch(Previous, Current, 1);
				Previous = Current;
			}
			const FVector3d TipPoint(Random.FRandRange(-0.1f, 0.1f) * Radius, Random.FRandRange(-0.1f, 0.1f) * Radius,
				Mesh.GetVertex(Previous[0]).Z - Settings.Depth * 0.15f);
			const int32 Tip = Mesh.AppendVertex(TipPoint);
			for (int32 Segment = 0; Segment < Segments; ++Segment)
			{
				AddTriangle(Previous[Segment], Tip, Previous[(Segment + 1) % Segments], 1);
			}
		});

		// Break up the skirt's smooth rings into rock facets.
		Displace(Build.Mesh, FirstSkirtVertex, Radius * 0.05f, 1.f / (Radius * 0.12f), Build.NoiseSeed);

		// Hanging spikes under the island.
		for (int32 Index = 0; Index < Settings.Spikes; ++Index)
		{
			const float Angle = Random.FRandRange(0.f, UE_TWO_PI);
			const float Along = Random.FRandRange(0.15f, 0.8f);
			const float Fraction = FMath::Pow((1.f - Along) / 0.96f, 1.f / 1.7f);
			const float Z = -Settings.Depth * FMath::Pow(Fraction, 1.25f) + Settings.Depth * 0.05f;
			const FVector Base(Along * Radius * FMath::Cos(Angle), Along * Radius * FMath::Sin(Angle), Z);
			const int32 First = VertexCount(Build.Mesh);
			Cone(Build.Mesh, 1, FTransform(FRotator(180.f + Random.FRandRange(-8.f, 8.f), Random.FRandRange(0.f, 360.f), 0.f), Base),
				Radius * Random.FRandRange(0.04f, 0.08f), 0.f, Settings.Depth * Random.FRandRange(0.15f, 0.4f), 5, 1);
			Displace(Build.Mesh, First, Radius * 0.012f, 1.f / (Radius * 0.05f), Build.NoiseSeed + Index);
		}

		// Loose chunks drifting beside it.
		for (int32 Index = 0; Index < Settings.Chunks; ++Index)
		{
			const float Angle = Random.FRandRange(0.f, UE_TWO_PI);
			const float Distance = Radius * Random.FRandRange(1.1f, 1.35f);
			const float Size = Radius * Random.FRandRange(0.03f, 0.075f);
			const FVector Location(Distance * FMath::Cos(Angle), Distance * FMath::Sin(Angle), -Settings.Depth * Random.FRandRange(0.05f, 0.45f));
			const int32 First = VertexCount(Build.Mesh);
			Cone(Build.Mesh, 1, FTransform(FRotator(180.f, Random.FRandRange(0.f, 360.f), 0.f), Location), Size, Size * 0.1f,
				Size * Random.FRandRange(1.5f, 2.6f), 5, 1);
			Displace(Build.Mesh, First, Size * 0.2f, 1.f / Size, Build.NoiseSeed + 50 + Index);
		}
	}

	/** A small canopy tree (trunk in BarkSlot, crown in LeafSlot) for dressing islands. */
	void AppendSimpleTree(FPropBuild& Build, const FVector& Base, float Height, int32 BarkSlot, int32 LeafSlot)
	{
		FRandomStream& Random = Build.Random;
		const FRotator Lean(Random.FRandRange(-6.f, 6.f), Random.FRandRange(0.f, 360.f), 0.f);
		Cone(Build.Mesh, BarkSlot, FTransform(Lean, Base), Height * 0.05f, Height * 0.028f, Height * 0.75f, 6, 1);
		const FVector Crown = Base + Lean.RotateVector(FVector(0.f, 0.f, Height * 0.78f));
		const int32 First = VertexCount(Build.Mesh);
		const int32 Blobs = Random.RandRange(3, 5);
		for (int32 Blob = 0; Blob < Blobs; ++Blob)
		{
			const FVector Offset(Random.FRandRange(-0.2f, 0.2f) * Height, Random.FRandRange(-0.2f, 0.2f) * Height, Random.FRandRange(-0.05f, 0.15f) * Height);
			StylizedMesh::Blob(Build.Mesh, LeafSlot, FTransform(RandomYaw(Random), Crown + Offset, FVector(1.f, 1.f, 0.85f)),
				Height * Random.FRandRange(0.18f, 0.28f), 2);
		}
		Displace(Build.Mesh, First, Height * 0.04f, 1.f / (Height * 0.25f), Build.NoiseSeed + 7.f);
	}

	void BuildIslandTerrain(FPropBuild& Build)
	{
		FIslandSettings Settings;
		Settings.Radius = 10000.f;
		Settings.Rings = 42;
		Settings.Segments = 132;
		Settings.Relief = 560.f;
		Settings.Depth = 6500.f;
		Settings.FlatCenter = 2600.f;
		Settings.Spikes = 18;
		Settings.Chunks = 10;
		BuildIsland(Build, Settings);

		Build.Surfaces.Add(GroundSurface(Earth(), Build.Secondary));
		Build.Surfaces.Add(RockSurface(Build.Primary, Build.Secondary, 0.74f, 0.5f));
		Build.SmoothAngle = 35.f;
	}

	void BuildFloatingIsland(FPropBuild& Build)
	{
		FRandomStream& Random = Build.Random;
		FIslandSettings Settings;
		Settings.Radius = Random.FRandRange(1200.f, 1600.f);
		Settings.Rings = 7;
		Settings.Segments = 30;
		Settings.Relief = 110.f;
		Settings.Depth = Settings.Radius * Random.FRandRange(1.3f, 2.f);
		Settings.Spikes = Random.RandRange(4, 7);
		Settings.Chunks = Random.RandRange(3, 6);
		BuildIsland(Build, Settings);

		const int32 Trees = Random.RandRange(0, 3);
		for (int32 Tree = 0; Tree < Trees; ++Tree)
		{
			const FVector2D Spot = RandomInDisc(Random, Settings.Radius * 0.55f);
			AppendSimpleTree(Build, FVector(Spot, -60.f), Random.FRandRange(500.f, 800.f), 2, 3);
		}

		Build.Surfaces.Add(GroundSurface(Earth(), Build.Secondary));
		Build.Surfaces.Add(RockSurface(Build.Primary, Build.Secondary, 0.74f, 0.5f));
		Build.Surfaces.Add(FStylizedSurface::Solid(Bark()));
		Build.Surfaces.Add(FoliageSurface(Leaves(), 900.f, 0.f, 0.3f));
		Build.SmoothAngle = 35.f;
	}

	bool IsTerrainShape(EStylizedPropShape Shape)
	{
		return Shape == EStylizedPropShape::IslandTerrain || Shape == EStylizedPropShape::Terrain || Shape == EStylizedPropShape::Hill;
	}

	/**
	 * Height of the terrain under a point, in the prop's local space. Only ground counts (island, terrain
	 * tiles, other hills, level geometry), never rocks or trees standing on it.
	 */
	bool ProbeTerrain(const FPropBuild& Build, const FVector2D& LocalXY, float& OutLocalZ)
	{
		const UWorld* World = Build.Owner ? Build.Owner->GetWorld() : nullptr;
		if (!World)
		{
			return false;
		}
		const FVector WorldPoint = Build.Transform.TransformPosition(FVector(LocalXY, 0.f));
		FCollisionQueryParams Params(SCENE_QUERY_STAT(PropTerrainProbe), true, Build.Owner);
		TArray<FHitResult> Hits;
		World->LineTraceMultiByObjectType(Hits, WorldPoint + FVector(0.f, 0.f, 2500.f), WorldPoint - FVector(0.f, 0.f, 2500.f),
			FCollisionObjectQueryParams(ECC_WorldStatic), Params);
		for (const FHitResult& Hit : Hits)
		{
			const AStylizedProp* Prop = Cast<AStylizedProp>(Hit.GetActor());
			if (!Prop || IsTerrainShape(Prop->Shape))
			{
				OutLocalZ = static_cast<float>(Build.Transform.InverseTransformPosition(Hit.ImpactPoint).Z);
				return true;
			}
		}
		return false;
	}

	void BuildTerrain(FPropBuild& Build)
	{
		// 200m x 200m tile of gently rolling meadow. Mesh collision is one-sided, so the tile gets deep
		// skirts on all four edges: wherever an edge floats above lower ground you meet a wall, not a
		// see-through gap you can walk in under.
		const float Size = 20000.f;
		const int32 Steps = 64;
		UGeometryScriptLibrary_MeshPrimitiveFunctions::AppendRectangleXY(Build.Mesh, Slot(0), FTransform::Identity, Size, Size, Steps, Steps);
		const float Skirt = 1500.f;
		for (const float Side : { -1.f, 1.f })
		{
			// Edge vertices sit at the same X/Y as the tile's border, so the noise below moves them together.
			UGeometryScriptLibrary_MeshPrimitiveFunctions::AppendBox(Build.Mesh, Slot(0), FTransform(FVector(0.f, Side * Size * 0.5f, -Skirt * 0.5f)),
				Size, 20.f, Skirt, Steps, 0, 1, EGeometryScriptPrimitiveOriginMode::Center);
			UGeometryScriptLibrary_MeshPrimitiveFunctions::AppendBox(Build.Mesh, Slot(0), FTransform(FVector(Side * Size * 0.5f, 0.f, -Skirt * 0.5f)),
				20.f, Size, Skirt, 0, Steps, 1, EGeometryScriptPrimitiveOriginMode::Center);
		}
		Displace(Build.Mesh, 0, 300.f, 1.f / 4000.f, Build.NoiseSeed);
		Displace(Build.Mesh, 0, 60.f, 1.f / 900.f, Build.NoiseSeed + 3.f);
		Build.Surfaces.Add(GroundSurface(Build.Primary, Build.Secondary));
		Build.SmoothAngle = 40.f;
	}

	void BuildHill(FPropBuild& Build)
	{
		// A soft, grassy mound built as a closed solid: bell-shaped top, a deep skirt and a bottom cap.
		// (An open sheet has one-sided collision; on a slope its low edge floats and you can walk in under it.)
		FRandomStream& Random = Build.Random;
		const float Radius = Random.FRandRange(1600.f, 2400.f);
		const float Height = Random.FRandRange(260.f, 480.f);
		const float Stretch = Random.FRandRange(1.f, 1.4f);
		const FRotator Yaw = RandomYaw(Random);
		const int32 Rings = 14;
		const int32 Segments = 40;
		const float SkirtDepth = 600.f;

		// The outer half of the mound follows the terrain underneath, so the rim always tucks just under the
		// ground wherever it's placed (on slopes too) and the skirt can never show.
		const FVector2D NoiseOffset(Random.FRandRange(-50.f, 50.f), Random.FRandRange(-50.f, 50.f));
		auto Point = [&](float Fraction, float Angle)
		{
			FVector Local = Yaw.RotateVector(FVector(FMath::Cos(Angle) * Radius * Fraction * Stretch, FMath::Sin(Angle) * Radius * Fraction, 0.f));
			const float Conform = FMath::SmoothStep(0.3f, 1.f, Fraction);
			float Ground = 0.f;
			if (Conform <= 0.f || !ProbeTerrain(Build, FVector2D(Local), Ground))
			{
				Ground = 0.f;
			}
			const float Lumps = FMath::PerlinNoise2D(FVector2D(Local) / 700.f + NoiseOffset) * 45.f * (1.f - Conform);
			Local.Z = Height * FMath::Square(1.f - Fraction * Fraction) + Lumps + Ground * Conform - 40.f * Fraction;
			return Local;
		};

		EditRaw(Build.Mesh, [&](FRawBuilder& Raw)
		{
			const int32 Top = Raw.Vertex(Point(0.f, 0.f));
			TArray<int32> Previous;
			for (int32 Ring = 1; Ring <= Rings; ++Ring)
			{
				TArray<int32> Current;
				for (int32 Segment = 0; Segment < Segments; ++Segment)
				{
					Current.Add(Raw.Vertex(Point(static_cast<float>(Ring) / Rings, UE_TWO_PI * Segment / Segments)));
				}
				for (int32 Segment = 0; Segment < Segments; ++Segment)
				{
					const int32 Next = (Segment + 1) % Segments;
					if (Ring == 1)
					{
						Raw.Triangle(Top, Current[Segment], Current[Next], 0);
					}
					else
					{
						Raw.Quad(Previous[Segment], Current[Segment], Current[Next], Previous[Next], 0);
					}
				}
				Previous = Current;
			}

			// Skirt straight down from the rim, then a cap underneath.
			TArray<int32> Bottom;
			for (int32 Segment = 0; Segment < Segments; ++Segment)
			{
				Bottom.Add(Raw.Vertex(Point(1.f, UE_TWO_PI * Segment / Segments) - FVector(0.f, 0.f, SkirtDepth)));
			}
			float BottomZ = 0.f;
			for (const int32 Vertex : Bottom)
			{
				BottomZ += static_cast<float>(Raw.Position(Vertex).Z) / Segments;
			}
			const int32 BottomCenter = Raw.Vertex(FVector(0.f, 0.f, BottomZ));
			for (int32 Segment = 0; Segment < Segments; ++Segment)
			{
				const int32 Next = (Segment + 1) % Segments;
				Raw.Quad(Previous[Segment], Bottom[Segment], Bottom[Next], Previous[Next], 0);
				Raw.Triangle(BottomCenter, Bottom[Next], Bottom[Segment], 0);
			}
		});
		FStylizedSurface Turf = GroundSurface(Build.Primary, Build.Secondary);
		Turf.TopThreshold = 0.6f; // grass all the way down the slopes; no bald patches
		Build.Surfaces.Add(Turf);
		Build.SmoothAngle = 50.f;
	}
	void BuildCliff(FPropBuild& Build)
	{
		// A cluster of chunky, slightly tilted rock prisms with grassy tops: climbable cover and silhouette.
		FRandomStream& Random = Build.Random;
		const int32 Columns = Random.RandRange(3, 5);
		for (int32 Column = 0; Column < Columns; ++Column)
		{
			const float Radius = Random.FRandRange(260.f, 520.f);
			const float Height = Random.FRandRange(450.f, 1100.f) * (Column == 0 ? 1.2f : 1.f);
			const FVector2D Spot = Column == 0 ? FVector2D::ZeroVector : RandomInDisc(Random, 420.f);
			const FRotator Tilt(Random.FRandRange(-4.f, 4.f), Random.FRandRange(0.f, 360.f), Random.FRandRange(-4.f, 4.f));
			Cone(Build.Mesh, 0, FTransform(Tilt, FVector(Spot, -150.f)), Radius, Radius * Random.FRandRange(0.72f, 0.92f), Height,
				Random.RandRange(5, 7), 3);
		}
		Displace(Build.Mesh, 0, 90.f, 1.f / 260.f, Build.NoiseSeed);
		Build.Surfaces.Add(RockSurface(Build.Primary, Build.Secondary, 0.78f, 0.4f));
	}

	// -----------------------------------------------------------------------
	// Rocks
	// -----------------------------------------------------------------------

	void AppendRockLump(FPropBuild& Build, const FVector& Offset, float Radius, int32 Steps, int32 MaterialID)
	{
		FRandomStream& Random = Build.Random;
		const FVector Scale(Random.FRandRange(1.f, 1.35f), Random.FRandRange(0.8f, 1.15f), Random.FRandRange(0.6f, 0.9f));
		const int32 First = VertexCount(Build.Mesh);
		Blob(Build.Mesh, MaterialID, FTransform(RandomYaw(Random), Offset + FVector(0.f, 0.f, Radius * Scale.Z * 0.35f), Scale), Radius, Steps);
		Displace(Build.Mesh, First, Radius * 0.22f, 2.2f / Radius, Build.NoiseSeed + Offset.X * 0.01f);
		// Flat, sunk bottom so it sits on slopes without floating.
		FlattenBelow(Build.Mesh, First, Offset.Z - Radius * 0.2f);
	}

	void BuildRock(FPropBuild& Build)
	{
		AppendRockLump(Build, FVector::ZeroVector, 70.f, 2, 0);
		Build.Surfaces.Add(RockSurface(Build.Primary, Build.Secondary, 0.62f));
		Build.CullDistance = 25000.f;
	}

	void BuildBoulder(FPropBuild& Build)
	{
		FRandomStream& Random = Build.Random;
		AppendRockLump(Build, FVector::ZeroVector, 240.f, 2, 0);
		const int32 Companions = Random.RandRange(1, 3);
		for (int32 Index = 0; Index < Companions; ++Index)
		{
			const FVector2D Direction = RandomInDisc(Random, 1.f).GetSafeNormal();
			AppendRockLump(Build, FVector(Direction * Random.FRandRange(260.f, 340.f), 0.f), Random.FRandRange(60.f, 120.f), 2, 0);
		}
		Build.Surfaces.Add(RockSurface(Build.Primary, Build.Secondary, 0.62f, 0.2f));
	}

	void BuildRockPillar(FPropBuild& Build)
	{
		FRandomStream& Random = Build.Random;
		const int32 Pillars = Random.RandRange(1, 3);
		for (int32 Pillar = 0; Pillar < Pillars; ++Pillar)
		{
			const float Radius = Random.FRandRange(90.f, 160.f) * (Pillar == 0 ? 1.2f : 0.85f);
			const float Height = Random.FRandRange(800.f, 1500.f) * (Pillar == 0 ? 1.f : 0.6f);
			const FVector2D Spot = Pillar == 0 ? FVector2D::ZeroVector : RandomInDisc(Random, 1.f).GetSafeNormal() * Random.FRandRange(220.f, 320.f);
			const FRotator Lean(Random.FRandRange(-5.f, 5.f), Random.FRandRange(0.f, 360.f), Random.FRandRange(-5.f, 5.f));
			const FTransform Base(Lean, FVector(Spot, -60.f));
			Cone(Build.Mesh, 0, Base, Radius, Radius * 0.8f, Height, 6, 3);
			// Broken, tilted cap block.
			const FVector Top = Base.TransformPosition(FVector(0.f, 0.f, Height));
			Cone(Build.Mesh, 0, FTransform(Lean + FRotator(Random.FRandRange(-12.f, 12.f), 0.f, Random.FRandRange(-12.f, 12.f)), Top - FVector(0.f, 0.f, 20.f)),
				Radius * 0.85f, Radius * 0.7f, Radius * 0.9f, 6, 0);
		}
		Displace(Build.Mesh, 0, 30.f, 1.f / 160.f, Build.NoiseSeed);
		const int32 RubbleCount = Random.RandRange(2, 5);
		for (int32 Rubble = 0; Rubble < RubbleCount; ++Rubble)
		{
			AppendRockLump(Build, FVector(RandomInDisc(Random, 420.f), 0.f), Random.FRandRange(35.f, 80.f), 1, 0);
		}
		Build.Surfaces.Add(RockSurface(Build.Primary, Moss(), 0.8f, 0.45f));
	}

	void BuildCrystal(FPropBuild& Build)
	{
		FRandomStream& Random = Build.Random;
		AppendRockLump(Build, FVector::ZeroVector, 90.f, 1, 0);
		const int32 Shards = Random.RandRange(5, 9);
		for (int32 Shard = 0; Shard < Shards; ++Shard)
		{
			const float Radius = Random.FRandRange(14.f, 34.f) * (Shard == 0 ? 1.4f : 1.f);
			const float Length = Random.FRandRange(90.f, 230.f) * (Shard == 0 ? 1.5f : 1.f);
			const FRotator Direction(Shard == 0 ? 0.f : Random.FRandRange(-38.f, 38.f), Random.FRandRange(0.f, 360.f), Shard == 0 ? 0.f : Random.FRandRange(-38.f, 38.f));
			const FTransform Base(Direction, FVector(RandomInDisc(Random, 50.f), 20.f));
			Cylinder(Build.Mesh, 1, Base, Radius, Length, 6);
			Cone(Build.Mesh, 1, FTransform(Direction, Base.TransformPosition(FVector(0.f, 0.f, Length))), Radius, 0.f, Radius * 1.8f, 6, 0);
		}
		Build.Surfaces.Add(RockSurface(Build.Primary, Moss(), 0.7f));
		Build.Surfaces.Add(GlowSurface(Build.Secondary, 1.6f));
		Build.LightIntensity = 18.f;
		Build.LightRadius = 650.f;
		Build.LightOffset = FVector(0.f, 0.f, 150.f);
		Build.LightColor = Build.Secondary;
	}

	void BuildSteppingStones(FPropBuild& Build)
	{
		FRandomStream& Random = Build.Random;
		const int32 Stones = Random.RandRange(5, 8);
		const float Wiggle = Random.FRandRange(0.4f, 0.8f);
		for (int32 Stone = 0; Stone < Stones; ++Stone)
		{
			const FVector Spot((Stone - Stones * 0.5f) * 105.f + Random.FRandRange(-15.f, 15.f),
				FMath::Sin(Stone * Wiggle) * 70.f + Random.FRandRange(-15.f, 15.f), -6.f);
			const int32 First = VertexCount(Build.Mesh);
			Cylinder(Build.Mesh, 0, FTransform(RandomYaw(Random), Spot, FVector(1.f, Random.FRandRange(0.7f, 1.f), 1.f)),
				Random.FRandRange(34.f, 52.f), Random.FRandRange(12.f, 18.f), Random.RandRange(5, 7));
			Displace(Build.Mesh, First, 5.f, 1.f / 40.f, Build.NoiseSeed + Stone);
		}
		Build.Surfaces.Add(RockSurface(Build.Primary, Moss(), 0.97f));
		Build.CullDistance = 12000.f;
	}

	// -----------------------------------------------------------------------
	// Plants
	// -----------------------------------------------------------------------

	/** Thin, flattened, tilted pyramids: reads as a blade of grass at gameplay distances. */
	void AppendBlades(FPropBuild& Build, int32 Count, float Radius, float MinHeight, float MaxHeight, int32 MaterialID)
	{
		FRandomStream& Random = Build.Random;
		for (int32 Blade = 0; Blade < Count; ++Blade)
		{
			const FVector2D Spot = RandomInDisc(Random, Radius);
			const FRotator Tilt(Random.FRandRange(-20.f, 20.f), Random.FRandRange(0.f, 360.f), Random.FRandRange(-20.f, 20.f));
			Cone(Build.Mesh, MaterialID, FTransform(Tilt, FVector(Spot, -4.f), FVector(1.f, 0.3f, 1.f)), Random.FRandRange(5.f, 8.f), 0.f,
				Random.FRandRange(MinHeight, MaxHeight), 3, 0);
		}
	}

	void BuildGrassPatch(FPropBuild& Build)
	{
		AppendBlades(Build, 520, 340.f, 26.f, 62.f, 0);
		Build.Surfaces.Add(FoliageSurface(Build.Primary, 60.f, 7.f, 0.35f, 0.85f));
		Build.bCastShadow = false;
		Build.CullDistance = 9000.f;
	}

	/**
	 * One curved grass ribbon: three tapering segments that arc toward Lean (quadratic bend), ending in a point.
	 * Facing is the direction the flat side looks; the two-sided foliage material shows both faces.
	 */
	void AppendRibbonBlade(FRawBuilder& Raw, const FVector& Base, float Height, float Width, const FVector2D& Facing, const FVector& Lean, int32 MaterialID)
	{
		const FVector Side(-Facing.Y, Facing.X, 0.f);
		constexpr int32 Segments = 3;
		int32 PreviousLeft = INDEX_NONE;
		int32 PreviousRight = INDEX_NONE;
		for (int32 Index = 0; Index <= Segments; ++Index)
		{
			const float Along = static_cast<float>(Index) / Segments;
			const FVector Spine = Base + FVector(0.f, 0.f, Height * Along) + Lean * (Along * Along);
			if (Index == Segments)
			{
				const int32 Tip = Raw.Vertex(Spine);
				Raw.Triangle(PreviousLeft, PreviousRight, Tip, MaterialID);
				break;
			}
			const float HalfWidth = Width * 0.5f * (1.f - Along * 0.8f);
			const int32 Left = Raw.Vertex(Spine - Side * HalfWidth);
			const int32 Right = Raw.Vertex(Spine + Side * HalfWidth);
			if (Index > 0)
			{
				Raw.Quad(PreviousLeft, PreviousRight, Right, Left, MaterialID);
			}
			PreviousLeft = Left;
			PreviousRight = Right;
		}
	}

	void BuildTallGrass(FPropBuild& Build)
	{
		// Lush, knee-to-waist-high grass: dense clumps of arcing ribbons that splay out from their roots,
		// with an occasional wildflower spike in the accent color.
		FRandomStream& Random = Build.Random;
		const float Radius = 330.f;
		EditRaw(Build.Mesh, [&](FRawBuilder& Raw)
		{
			const int32 Clumps = Random.RandRange(30, 40);
			for (int32 Clump = 0; Clump < Clumps; ++Clump)
			{
				const FVector2D Center = RandomInDisc(Random, Radius);
				const float ClumpHeight = Random.FRandRange(55.f, 110.f);
				const int32 Blades = Random.RandRange(14, 24);
				for (int32 Blade = 0; Blade < Blades; ++Blade)
				{
					const FVector2D Offset = RandomInDisc(Random, 24.f);
					const float Angle = Random.FRandRange(0.f, UE_TWO_PI);
					const FVector2D Facing(FMath::Cos(Angle), FMath::Sin(Angle));
					const FVector2D Outward = Offset.IsNearlyZero() ? Facing : Offset.GetSafeNormal();
					const float Height = ClumpHeight * Random.FRandRange(0.55f, 1.15f);
					// Outer blades lean harder, so each clump opens like a fountain.
					const float Splay = Random.FRandRange(0.12f, 0.3f) + Offset.Size() / 24.f * 0.25f;
					const FVector Lean(Outward * Height * Splay, -Height * 0.1f);
					AppendRibbonBlade(Raw, FVector(Center + Offset, -4.f), Height, Random.FRandRange(4.5f, 8.f), Facing, Lean, 0);
				}
			}
		});

		if (Random.FRand() < 0.55f)
		{
			const int32 Spikes = Random.RandRange(2, 6);
			for (int32 Spike = 0; Spike < Spikes; ++Spike)
			{
				const FVector Base(RandomInDisc(Random, Radius * 0.85f), -4.f);
				const float Height = Random.FRandRange(95.f, 135.f);
				const FRotator Tilt(Random.FRandRange(-6.f, 6.f), Random.FRandRange(0.f, 360.f), Random.FRandRange(-6.f, 6.f));
				Cone(Build.Mesh, 0, FTransform(Tilt, Base, FVector(1.f, 0.6f, 1.f)), 2.2f, 1.f, Height, 3, 0);
				// A tapering column of little blooms over the top third.
				const int32 Blooms = Random.RandRange(6, 9);
				for (int32 Bloom = 0; Bloom < Blooms; ++Bloom)
				{
					const float Along = FMath::Lerp(0.62f, 1.f, static_cast<float>(Bloom) / (Blooms - 1));
					const FVector Center = Base + Tilt.RotateVector(FVector(0.f, 0.f, Height * Along));
					Ball(Build.Mesh, 1, FTransform(RandomYaw(Random), Center, FVector(1.f, 1.f, 0.8f)), FMath::Lerp(6.5f, 2.5f, Along * Along), 3, 5);
				}
			}
		}

		FStylizedSurface Blades = FoliageSurface(Build.Primary, 110.f, 12.f, 0.55f, 0.9f);
		Blades.Variation = 0.24f;
		Blades.bTwoSided = true;
		Build.Surfaces.Add(Blades);
		FStylizedSurface Blooms = FoliageSurface(Build.Secondary, 130.f, 12.f, 0.05f, 0.5f);
		Blooms.Variation = 0.2f;
		Build.Surfaces.Add(Blooms);
		Build.SmoothAngle = 180.f;
		Build.bCastShadow = false;
		Build.CullDistance = 12000.f;
	}

	void BuildFlowerPatch(FPropBuild& Build)
	{
		FRandomStream& Random = Build.Random;
		const float Radius = 380.f;
		AppendBlades(Build, 260, Radius, 22.f, 50.f, 0);
		const int32 Flowers = 170;
		for (int32 Flower = 0; Flower < Flowers; ++Flower)
		{
			const FVector Spot(RandomInDisc(Random, Radius), -4.f);
			const FRotator Tilt(Random.FRandRange(-14.f, 14.f), Random.FRandRange(0.f, 360.f), Random.FRandRange(-14.f, 14.f));
			const float Height = Random.FRandRange(32.f, 62.f);
			Cone(Build.Mesh, 0, FTransform(Tilt, Spot, FVector(1.f, 0.5f, 1.f)), 2.6f, 1.f, Height, 3, 0);
			const FVector Head = Spot + Tilt.RotateVector(FVector(0.f, 0.f, Height));
			Ball(Build.Mesh, 1, FTransform(RandomYaw(Random), Head, FVector(1.f, 1.f, 0.5f)), Random.FRandRange(9.f, 13.f), 4, 6);
		}
		Build.Surfaces.Add(FoliageSurface(Build.Primary, 60.f, 6.f, 0.35f, 0.85f));
		FStylizedSurface Petals = FoliageSurface(Build.Secondary, 60.f, 6.f, 0.1f, 0.5f);
		Petals.Variation = 0.3f;
		Build.Surfaces.Add(Petals);
		Build.bCastShadow = false;
		Build.CullDistance = 14000.f;
	}

	void BuildCanopyTree(FPropBuild& Build)
	{
		FRandomStream& Random = Build.Random;
		const float Height = Random.FRandRange(480.f, 720.f);
		const FRotator Lean(Random.FRandRange(-5.f, 5.f), Random.FRandRange(0.f, 360.f), 0.f);
		// Flared base, then the trunk and a side branch.
		Cone(Build.Mesh, 0, FTransform(Lean, FVector(0.f, 0.f, -10.f)), 44.f, 24.f, 50.f, 7, 0);
		Cone(Build.Mesh, 0, FTransform(Lean), 25.f, 13.f, Height * 0.8f, 7, 2);
		const FRotator Branch = Lean + FRotator(Random.FRandRange(35.f, 50.f), Random.FRandRange(0.f, 360.f), 0.f);
		Cone(Build.Mesh, 0, FTransform(Branch, Lean.RotateVector(FVector(0.f, 0.f, Height * 0.45f))), 11.f, 4.f, Height * 0.35f, 5, 0);

		const FVector Crown = Lean.RotateVector(FVector(0.f, 0.f, Height * 0.85f));
		const float CrownRadius = Height * 0.34f;
		const int32 First = VertexCount(Build.Mesh);
		const int32 Blobs = Random.RandRange(5, 8);
		for (int32 Blob = 0; Blob < Blobs; ++Blob)
		{
			const FVector Offset = FVector(RandomInDisc(Random, CrownRadius * 0.75f), Random.FRandRange(-0.3f, 0.45f) * CrownRadius);
			StylizedMesh::Blob(Build.Mesh, 1, FTransform(RandomYaw(Random), Crown + Offset, FVector(1.f, 1.f, 0.82f)),
				CrownRadius * Random.FRandRange(0.45f, 0.7f), 2);
		}
		Displace(Build.Mesh, First, CrownRadius * 0.14f, 1.8f / CrownRadius, Build.NoiseSeed);

		Build.Surfaces.Add(FStylizedSurface::Solid(Build.Primary, 0.08f));
		Build.Surfaces.Add(FoliageSurface(Build.Secondary, Height * 1.25f, 4.f, 0.45f, 0.3f));
		Build.SmoothAngle = 50.f;
	}

	void BuildBlossomTree(FPropBuild& Build)
	{
		FRandomStream& Random = Build.Random;
		const float Height = Random.FRandRange(420.f, 620.f);
		// A trunk with a kink in it, then wide, layered crowns.
		const FRotator LowerLean(Random.FRandRange(-10.f, 10.f), Random.FRandRange(0.f, 360.f), 0.f);
		Cone(Build.Mesh, 0, FTransform(LowerLean, FVector(0.f, 0.f, -10.f)), 26.f, 18.f, Height * 0.45f, 6, 1);
		const FVector Kink = LowerLean.RotateVector(FVector(0.f, 0.f, Height * 0.43f));
		const FRotator UpperLean(-LowerLean.Pitch * 0.8f, LowerLean.Yaw, 0.f);
		Cone(Build.Mesh, 0, FTransform(UpperLean, Kink), 18.f, 10.f, Height * 0.45f, 6, 1);
		const FVector Top = Kink + UpperLean.RotateVector(FVector(0.f, 0.f, Height * 0.45f));

		const int32 First = VertexCount(Build.Mesh);
		const int32 Tiers = Random.RandRange(2, 3);
		for (int32 Tier = 0; Tier < Tiers; ++Tier)
		{
			const float TierRadius = Height * (0.42f - Tier * 0.1f);
			const FVector TierCenter = Top + FVector(Random.FRandRange(-40.f, 40.f), Random.FRandRange(-40.f, 40.f), Tier * Height * 0.14f - Height * 0.05f);
			const int32 Puffs = Random.RandRange(4, 6);
			for (int32 Puff = 0; Puff < Puffs; ++Puff)
			{
				const FVector Offset(RandomInDisc(Random, TierRadius * 0.6f), 0.f);
				StylizedMesh::Blob(Build.Mesh, 1, FTransform(RandomYaw(Random), TierCenter + Offset, FVector(1.f, 1.f, 0.62f)),
					TierRadius * Random.FRandRange(0.45f, 0.65f), 2);
			}
		}
		Displace(Build.Mesh, First, Height * 0.035f, 1.f / (Height * 0.2f), Build.NoiseSeed);

		Build.Surfaces.Add(FStylizedSurface::Solid(Build.Primary, 0.08f));
		Build.Surfaces.Add(FoliageSurface(Build.Secondary, Height * 1.2f, 3.f, 0.35f, 0.35f));
		Build.SmoothAngle = 50.f;
	}

	void BuildPineTree(FPropBuild& Build)
	{
		FRandomStream& Random = Build.Random;
		const float Height = Random.FRandRange(650.f, 1050.f);
		Cone(Build.Mesh, 0, FTransform(FVector(0.f, 0.f, -10.f)), 22.f, 6.f, Height * 0.9f, 6, 1);
		const int32 First = VertexCount(Build.Mesh);
		const int32 Tiers = Random.RandRange(4, 5);
		for (int32 Tier = 0; Tier < Tiers; ++Tier)
		{
			const float Fraction = static_cast<float>(Tier) / Tiers;
			const float TierRadius = Height * FMath::Lerp(0.27f, 0.09f, Fraction);
			const float Z = Height * FMath::Lerp(0.22f, 0.8f, Fraction);
			Cone(Build.Mesh, 1, FTransform(FRotator(Random.FRandRange(-3.f, 3.f), Random.FRandRange(0.f, 360.f), 0.f), FVector(0.f, 0.f, Z)),
				TierRadius, 0.f, Height * 0.3f, 7, 1);
		}
		Displace(Build.Mesh, First, Height * 0.015f, 1.f / (Height * 0.1f), Build.NoiseSeed);
		Build.Surfaces.Add(FStylizedSurface::Solid(Build.Primary, 0.08f));
		Build.Surfaces.Add(FoliageSurface(Build.Secondary, Height, 2.f, 0.45f, 0.2f));
	}

	void BuildBush(FPropBuild& Build)
	{
		FRandomStream& Random = Build.Random;
		const int32 Blobs = Random.RandRange(3, 6);
		for (int32 Blob = 0; Blob < Blobs; ++Blob)
		{
			const float Radius = Random.FRandRange(45.f, 85.f);
			const FVector Offset(RandomInDisc(Random, 70.f), Radius * 0.45f);
			StylizedMesh::Blob(Build.Mesh, 0, FTransform(RandomYaw(Random), Offset, FVector(1.f, 1.f, 0.8f)), Radius, 2);
		}
		Displace(Build.Mesh, 0, 12.f, 1.f / 60.f, Build.NoiseSeed);
		// A sprinkle of small blossoms on top.
		const int32 Blossoms = Random.RandRange(0, 8);
		for (int32 Blossom = 0; Blossom < Blossoms; ++Blossom)
		{
			const FVector Spot(RandomInDisc(Random, 70.f), Random.FRandRange(70.f, 110.f));
			Ball(Build.Mesh, 1, FTransform(RandomYaw(Random), Spot, FVector(1.f, 1.f, 0.6f)), Random.FRandRange(8.f, 12.f), 4, 6);
		}
		Build.Surfaces.Add(FoliageSurface(Build.Primary, 160.f, 3.f, 0.45f, 0.35f));
		Build.Surfaces.Add(FoliageSurface(Build.Secondary, 160.f, 3.f, 0.05f, 0.4f));
		Build.SmoothAngle = 50.f;
		Build.CullDistance = 20000.f;
	}

	// -----------------------------------------------------------------------
	// Props
	// -----------------------------------------------------------------------

	void AppendCrate(FPropBuild& Build, const FTransform& Transform, float Size)
	{
		auto Part = [&](int32 MaterialID, const FVector& Center, const FVector& Extent)
		{
			Box(Build.Mesh, MaterialID, Transform.TransformPosition(Center), Extent * Transform.GetScale3D(), Transform.Rotator());
		};
		const float Half = Size * 0.5f;
		const float Trim = Size * 0.09f;
		Part(0, FVector(0.f, 0.f, Half), FVector(Size));
		// Painted metal edges.
		for (const float X : { -Half, Half })
		{
			for (const float Y : { -Half, Half })
			{
				Part(1, FVector(X, Y, Half), FVector(Trim, Trim, Size + 2.f));
			}
		}
		for (const float Z : { Trim * 0.5f, Size - Trim * 0.5f })
		{
			for (const float Side : { -Half, Half })
			{
				Part(1, FVector(Side, 0.f, Z), FVector(Trim, Size + 2.f, Trim));
				Part(1, FVector(0.f, Side, Z), FVector(Size + 2.f, Trim, Trim));
			}
		}
		// Emblem plates and a latch.
		Part(1, FVector(Half + 1.f, 0.f, Half), FVector(4.f, Size * 0.38f, Size * 0.38f));
		Part(1, FVector(-Half - 1.f, 0.f, Half), FVector(4.f, Size * 0.38f, Size * 0.38f));
		Part(2, FVector(Half + 4.f, 0.f, Size * 0.78f), FVector(6.f, Size * 0.16f, Size * 0.12f));
	}

	void BuildCrate(FPropBuild& Build)
	{
		FRandomStream& Random = Build.Random;
		const float Size = Random.FRandRange(95.f, 125.f);
		AppendCrate(Build, FTransform(RandomYaw(Random)), Size);
		if (Random.FRand() < 0.35f)
		{
			const float TopSize = Size * Random.FRandRange(0.6f, 0.8f);
			AppendCrate(Build, FTransform(RandomYaw(Random), FVector(Random.FRandRange(-10.f, 10.f), Random.FRandRange(-10.f, 10.f), Size + 1.f)), TopSize);
		}
		Displace(Build.Mesh, 0, 1.5f, 1.f / 50.f, Build.NoiseSeed);
		FStylizedSurface Paint = FStylizedSurface::Solid(Build.Primary, 0.1f);
		Paint.GradHeight = Size * 1.5f;
		Paint.GradDark = 0.3f;
		Build.Surfaces.Add(Paint);
		Build.Surfaces.Add(FStylizedSurface::Solid(Build.Secondary, 0.06f));
		Build.Surfaces.Add(FStylizedSurface::Solid(DarkMetal(), 0.04f));
		Build.CullDistance = 25000.f;
	}

	void BuildBarrel(FPropBuild& Build)
	{
		const int32 Sides = 12;
		// Bulged body: frustum, band of cylinder, frustum.
		Cone(Build.Mesh, 0, FTransform::Identity, 40.f, 46.f, 40.f, Sides, 0);
		Cylinder(Build.Mesh, 0, FTransform(FVector(0.f, 0.f, 40.f)), 46.f, 32.f, Sides);
		Cone(Build.Mesh, 0, FTransform(FVector(0.f, 0.f, 72.f)), 46.f, 40.f, 40.f, Sides, 0);
		Cylinder(Build.Mesh, 1, FTransform(FVector(0.f, 0.f, 16.f)), 44.f, 8.f, Sides);
		Cylinder(Build.Mesh, 1, FTransform(FVector(0.f, 0.f, 88.f)), 44.f, 8.f, Sides);
		Cylinder(Build.Mesh, 1, FTransform(FVector(0.f, 0.f, 110.f)), 36.f, 4.f, Sides);
		Cylinder(Build.Mesh, 1, FTransform(FVector(18.f, 0.f, 112.f)), 7.f, 5.f, 8);
		FStylizedSurface Paint = FStylizedSurface::Solid(Build.Primary, 0.08f);
		Paint.GradHeight = 120.f;
		Paint.GradDark = 0.3f;
		Build.Surfaces.Add(Paint);
		Build.Surfaces.Add(FStylizedSurface::Solid(Build.Secondary, 0.05f));
		Build.SmoothAngle = 40.f;
		Build.CullDistance = 25000.f;
	}

	void BuildStoneWall(FPropBuild& Build)
	{
		FRandomStream& Random = Build.Random;
		const float Length = Random.FRandRange(500.f, 720.f);
		const float Height = Random.FRandRange(170.f, 260.f);
		const float Thickness = 60.f;
		const float RowHeight = 42.f;
		const int32 Rows = FMath::Max(2, FMath::RoundToInt(Height / RowHeight));
		const bool bBrokenLeft = Random.RandBool();

		for (int32 Row = 0; Row < Rows; ++Row)
		{
			float X = -Length * 0.5f - (Row % 2) * 40.f;
			while (X < Length * 0.5f)
			{
				const float BlockLength = Random.FRandRange(70.f, 120.f);
				const float Center = FMath::Min(X + BlockLength * 0.5f, Length * 0.5f - 20.f);
				X += BlockLength;

				// Missing blocks toward the top and at the broken end.
				const float Along = (Center / Length) + 0.5f;
				const float BrokenEnd = bBrokenLeft ? 1.f - Along : Along;
				const float MissChance = FMath::Max(0.f, static_cast<float>(Row) / Rows - 0.4f) * 0.8f + FMath::Max(0.f, BrokenEnd - 0.7f) * 2.f * Row / Rows;
				if (Row > 0 && Random.FRand() < MissChance)
				{
					continue;
				}
				const FVector BlockCenter(Center, Random.FRandRange(-3.f, 3.f), Row * RowHeight + RowHeight * 0.5f);
				const FRotator Wobble(Random.FRandRange(-1.5f, 1.5f), Random.FRandRange(-2.f, 2.f), Random.FRandRange(-1.5f, 1.5f));
				Box(Build.Mesh, 0, BlockCenter, FVector(BlockLength - 5.f, Thickness + Random.FRandRange(-6.f, 6.f), RowHeight - 4.f), Wobble, 1);
			}
		}
		// Fallen blocks at the foot of the wall.
		const int32 FallenCount = Random.RandRange(1, 3);
		for (int32 Fallen = 0; Fallen < FallenCount; ++Fallen)
		{
			const FVector Spot(Random.FRandRange(-0.5f, 0.5f) * Length, (Random.RandBool() ? 1.f : -1.f) * Random.FRandRange(70.f, 140.f), 15.f);
			Box(Build.Mesh, 0, Spot, FVector(Random.FRandRange(60.f, 100.f), 50.f, 38.f), FRotator(Random.FRandRange(-10.f, 10.f), Random.FRandRange(0.f, 360.f), Random.FRandRange(-6.f, 6.f)));
		}
		Displace(Build.Mesh, 0, 4.f, 1.f / 28.f, Build.NoiseSeed);
		Build.Surfaces.Add(RockSurface(Build.Primary, Build.Secondary, 0.8f));
	}

	void BuildRuinPillar(FPropBuild& Build)
	{
		FRandomStream& Random = Build.Random;
		Box(Build.Mesh, 0, FVector(0.f, 0.f, 20.f), FVector(150.f, 150.f, 40.f), RandomYaw(Random));
		const int32 Drums = Random.RandRange(2, 6);
		float Z = 40.f;
		for (int32 Drum = 0; Drum < Drums; ++Drum)
		{
			const bool bLast = Drum == Drums - 1;
			const float DrumHeight = Random.FRandRange(60.f, 80.f);
			const FRotator Tilt = bLast && Drums < 6 ? FRotator(Random.FRandRange(-9.f, 9.f), Random.FRandRange(0.f, 360.f), Random.FRandRange(-9.f, 9.f)) : RandomYaw(Random);
			Cylinder(Build.Mesh, 0, FTransform(Tilt, FVector(Random.FRandRange(-3.f, 3.f), Random.FRandRange(-3.f, 3.f), Z)), 46.f, DrumHeight - 3.f, 10);
			Z += DrumHeight;
		}
		if (Drums >= 5)
		{
			Box(Build.Mesh, 0, FVector(0.f, 0.f, Z + 15.f), FVector(125.f, 125.f, 30.f), RandomYaw(Random));
		}
		else
		{
			// The rest of it lies on the ground.
			const FVector2D Direction = RandomInDisc(Random, 1.f).GetSafeNormal();
			const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(Direction.Y, Direction.X));
			Cylinder(Build.Mesh, 0, FTransform(FRotator(-90.f, Yaw, 0.f), FVector(Direction * 150.f, 44.f)), 44.f, Random.FRandRange(60.f, 130.f), 10);
		}
		Displace(Build.Mesh, 0, 4.f, 1.f / 50.f, Build.NoiseSeed);
		Build.Surfaces.Add(RockSurface(Build.Primary, Build.Secondary, 0.8f));
	}

	// -----------------------------------------------------------------------
	// Sky
	// -----------------------------------------------------------------------

	void BuildFloatingDebris(FPropBuild& Build)
	{
		// Little drifting islets: a grassy cap on a jagged, two-stage underside.
		FRandomStream& Random = Build.Random;
		const int32 Chunks = Random.RandRange(4, 7);
		for (int32 Chunk = 0; Chunk < Chunks; ++Chunk)
		{
			const float Size = Random.FRandRange(90.f, 340.f);
			const FVector Spot(RandomInDisc(Random, 1400.f), Random.FRandRange(-600.f, 900.f));
			const float Yaw = Random.FRandRange(0.f, 360.f);
			const int32 First = VertexCount(Build.Mesh);
			Cylinder(Build.Mesh, 0, FTransform(FRotator(0.f, Yaw, 0.f), Spot - FVector(0.f, 0.f, Size * 0.2f)), Size * 1.05f, Size * 0.22f, Random.RandRange(6, 8));
			Cone(Build.Mesh, 0, FTransform(FRotator(180.f + Random.FRandRange(-8.f, 8.f), Yaw, 0.f), Spot - FVector(0.f, 0.f, Size * 0.18f)),
				Size, Size * 0.5f, Size * 0.7f, 6, 1);
			Cone(Build.Mesh, 0, FTransform(FRotator(180.f + Random.FRandRange(-12.f, 12.f), Yaw + 30.f, 0.f), Spot - FVector(0.f, 0.f, Size * 0.85f)),
				Size * 0.52f, 0.f, Size * Random.FRandRange(0.8f, 1.7f), 5, 1);
			Displace(Build.Mesh, First, Size * 0.2f, 1.4f / Size, Build.NoiseSeed + Chunk);
		}
		Build.Surfaces.Add(RockSurface(Build.Primary, Build.Secondary, 0.7f, 0.5f));
	}
	void BuildCloud(FPropBuild& Build)
	{
		FRandomStream& Random = Build.Random;
		const float Length = Random.FRandRange(1300.f, 2200.f);
		const int32 Puffs = Random.RandRange(6, 11);
		for (int32 Puff = 0; Puff < Puffs; ++Puff)
		{
			const float Along = Random.FRandRange(-0.5f, 0.5f);
			// Bigger puffs in the middle for a classic cumulus profile.
			const float Radius = FMath::Lerp(620.f, 260.f, FMath::Abs(Along) * 2.f) * Random.FRandRange(0.75f, 1.1f);
			const FVector Spot(Along * Length, Random.FRandRange(-260.f, 260.f), Radius * Random.FRandRange(0.05f, 0.35f));
			Ball(Build.Mesh, 0, FTransform(RandomYaw(Random), Spot, FVector(1.f, Random.FRandRange(0.75f, 1.f), 0.8f)), Radius, 8, 12);
		}
		// Flat bottom, like a real cumulus sitting on its condensation level.
		FlattenBelow(Build.Mesh, 0, 0.f);
		FStylizedSurface Vapor = FStylizedSurface::Solid(Build.Primary, 0.03f);
		Vapor.GradHeight = 700.f;
		Vapor.GradDark = 0.2f;
		Vapor.UpNormal = 0.35f;
		Build.Surfaces.Add(Vapor);
		Build.SmoothAngle = 180.f;
		Build.bCastShadow = false;
	}

	void BuildBeacon(FPropBuild& Build)
	{
		FRandomStream& Random = Build.Random;
		Cylinder(Build.Mesh, 0, FTransform(FVector(0.f, 0.f, -20.f)), 190.f, 55.f, 8);
		Cylinder(Build.Mesh, 0, FTransform(FVector(0.f, 0.f, 35.f)), 120.f, 25.f, 8);
		// Standing stones leaning in around the core.
		const int32 Stones = 4;
		for (int32 Stone = 0; Stone < Stones; ++Stone)
		{
			const float Angle = UE_TWO_PI * Stone / Stones + UE_PI / 4.f;
			const FVector Spot(FMath::Cos(Angle) * 150.f, FMath::Sin(Angle) * 150.f, 110.f);
			const float Yaw = FMath::RadiansToDegrees(Angle);
			Box(Build.Mesh, 0, Spot, FVector(46.f, 64.f, Random.FRandRange(200.f, 280.f)), FRotator(-7.f, Yaw, 0.f));
		}
		Displace(Build.Mesh, 0, 4.f, 1.f / 60.f, Build.NoiseSeed);
		// Floating crystal core.
		const FTransform Core(RandomYaw(Random), FVector(0.f, 0.f, 120.f));
		Cone(Build.Mesh, 1, FTransform(FRotator(180.f, 0.f, 0.f), Core.GetLocation()), 34.f, 0.f, 60.f, 6, 0);
		Cylinder(Build.Mesh, 1, Core, 34.f, 90.f, 6);
		Cone(Build.Mesh, 1, FTransform(Core.Rotator(), Core.GetLocation() + FVector(0.f, 0.f, 90.f)), 34.f, 0.f, 70.f, 6, 0);

		Build.Surfaces.Add(RockSurface(Build.Primary, Moss(), 0.85f));
		Build.Surfaces.Add(GlowSurface(Build.Secondary, 3.f));
		Build.BeamHeight = 12000.f;
		Build.BeamRadius = 60.f;
		Build.BeamColor = Build.Secondary;
		Build.LightIntensity = 60.f;
		Build.LightRadius = 1400.f;
		Build.LightOffset = FVector(0.f, 0.f, 200.f);
		Build.LightColor = Build.Secondary;
	}
}

AStylizedProp::AStylizedProp()
{
	PrimaryActorTick.bCanEverTick = false;

	MeshComponent = CreateDefaultSubobject<UDynamicMeshComponent>(TEXT("Mesh"));
	MeshComponent->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	SetRootComponent(MeshComponent);

	BeamComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Beam"));
	BeamComponent->SetupAttachment(MeshComponent);
	BeamComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BeamComponent->SetCastShadow(false);
	BeamComponent->SetVisibility(false);

	GlowLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("GlowLight"));
	GlowLight->SetupAttachment(MeshComponent);
	GlowLight->SetIntensityUnits(ELightUnits::Candelas);
	GlowLight->SetCastShadows(false);
	GlowLight->SetVisibility(false);
}

bool AStylizedProp::IsSoftShape(EStylizedPropShape InShape)
{
	return InShape == EStylizedPropShape::GrassPatch || InShape == EStylizedPropShape::TallGrass || InShape == EStylizedPropShape::FlowerPatch
		|| InShape == EStylizedPropShape::Bush || InShape == EStylizedPropShape::Cloud;
}

void AStylizedProp::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Rebuild();
}

void AStylizedProp::Configure(EStylizedPropShape InShape, int32 InSeed, FLinearColor InPrimary, FLinearColor InSecondary)
{
	Shape = InShape;
	Seed = InSeed;
	PrimaryColor = InPrimary;
	SecondaryColor = InSecondary;
	Rebuild();
}

bool AStylizedProp::IsGroundShape(EStylizedPropShape InShape)
{
	return InShape == EStylizedPropShape::Terrain || InShape == EStylizedPropShape::IslandTerrain
		|| InShape == EStylizedPropShape::Hill || InShape == EStylizedPropShape::Cliff;
}

FStylizedPropLook AStylizedProp::Generate(EStylizedPropShape InShape, int32 InSeed, const FLinearColor& Primary, const FLinearColor& Secondary,
	const FTransform& Placement, const AActor* ProbeActor, UDynamicMesh* OutMesh)
{
	FPropBuild Build;
	Build.Mesh = OutMesh;
	Build.Owner = ProbeActor;
	Build.Transform = Placement;
	Build.Random = FRandomStream(InSeed);
	Build.Primary = Primary;
	Build.Secondary = Secondary;
	Build.NoiseSeed = static_cast<float>(FMath::Abs(InSeed) % 997) * 0.731f;

	switch (InShape)
	{
	case EStylizedPropShape::Terrain:        BuildTerrain(Build); break;
	case EStylizedPropShape::IslandTerrain:  BuildIslandTerrain(Build); break;
	case EStylizedPropShape::Hill:           BuildHill(Build); break;
	case EStylizedPropShape::Cliff:          BuildCliff(Build); break;
	case EStylizedPropShape::Rock:           BuildRock(Build); break;
	case EStylizedPropShape::Boulder:        BuildBoulder(Build); break;
	case EStylizedPropShape::RockPillar:     BuildRockPillar(Build); break;
	case EStylizedPropShape::Crystal:        BuildCrystal(Build); break;
	case EStylizedPropShape::SteppingStones: BuildSteppingStones(Build); break;
	case EStylizedPropShape::GrassPatch:     BuildGrassPatch(Build); break;
	case EStylizedPropShape::TallGrass:      BuildTallGrass(Build); break;
	case EStylizedPropShape::FlowerPatch:    BuildFlowerPatch(Build); break;
	case EStylizedPropShape::CanopyTree:     BuildCanopyTree(Build); break;
	case EStylizedPropShape::BlossomTree:    BuildBlossomTree(Build); break;
	case EStylizedPropShape::PineTree:       BuildPineTree(Build); break;
	case EStylizedPropShape::Bush:           BuildBush(Build); break;
	case EStylizedPropShape::Crate:          BuildCrate(Build); break;
	case EStylizedPropShape::Barrel:         BuildBarrel(Build); break;
	case EStylizedPropShape::StoneWall:      BuildStoneWall(Build); break;
	case EStylizedPropShape::RuinPillar:     BuildRuinPillar(Build); break;
	case EStylizedPropShape::FloatingIsland: BuildFloatingIsland(Build); break;
	case EStylizedPropShape::FloatingDebris: BuildFloatingDebris(Build); break;
	case EStylizedPropShape::Cloud:          BuildCloud(Build); break;
	case EStylizedPropShape::Beacon:         BuildBeacon(Build); break;
	}
	FinishNormals(OutMesh, Build.SmoothAngle);

	FStylizedPropLook Look;
	Look.Surfaces = MoveTemp(Build.Surfaces);
	Look.CullDistance = Build.CullDistance;
	Look.bCastShadow = Build.bCastShadow;
	Look.BeamHeight = Build.BeamHeight;
	Look.BeamRadius = Build.BeamRadius;
	Look.BeamColor = Build.BeamColor;
	Look.LightIntensity = Build.LightIntensity;
	Look.LightRadius = Build.LightRadius;
	Look.LightOffset = Build.LightOffset;
	Look.LightColor = Build.LightColor;
	return Look;
}

void AStylizedProp::Rebuild()
{
	// Build into a scratch mesh so the component (and its render/collision data) only updates once.
	UDynamicMesh* Scratch = NewObject<UDynamicMesh>(this, NAME_None, RF_Transient);
	const FStylizedPropLook Look = Generate(Shape, Seed, PrimaryColor, SecondaryColor, GetActorTransform(), this, Scratch);

	FDynamicMesh3 Result;
	Scratch->ProcessMesh([&Result](const FDynamicMesh3& Built) { Result = Built; });
	MeshComponent->SetMesh(MoveTemp(Result));
	StylizedSurfaces::Apply(MeshComponent, Look.Surfaces);

	if (IsSoft())
	{
		// Walk and shoot straight through it; visibility queries still see it as an overlap.
		MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		MeshComponent->SetCollisionObjectType(ECC_WorldDynamic);
		MeshComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
		MeshComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Overlap);
	}
	else
	{
		MeshComponent->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	}
	MeshComponent->EnableComplexAsSimpleCollision();
	MeshComponent->SetCastShadow(Look.bCastShadow);
	// Ground cover is tagged in the custom stencil so the post process leaves it free of ink lines (outlining
	// every blade of grass reads as scribble, not foliage).
	MeshComponent->SetRenderCustomDepth(IsSoft());
	MeshComponent->SetCustomDepthStencilValue(IsSoft() ? 1 : 0);
	MeshComponent->SetCullDistance(Look.CullDistance);

	const bool bBeam = Look.BeamHeight > 0.f;
	BeamComponent->SetVisibility(bBeam);
	if (bBeam)
	{
		StylizedSurfaces::SetupBeam(BeamComponent, Look.BeamColor, 2.5f, Look.BeamHeight, Look.BeamRadius);
	}

	const bool bLight = Look.LightIntensity > 0.f;
	GlowLight->SetVisibility(bLight);
	if (bLight)
	{
		GlowLight->SetRelativeLocation(Look.LightOffset);
		GlowLight->SetIntensity(Look.LightIntensity);
		GlowLight->SetAttenuationRadius(Look.LightRadius);
		GlowLight->SetLightColor(Look.LightColor);
	}
}

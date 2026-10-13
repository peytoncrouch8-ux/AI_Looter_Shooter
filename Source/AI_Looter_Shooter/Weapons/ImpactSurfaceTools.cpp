#include "Weapons/ImpactSurfaceTools.h"
#include "AI_Looter_Shooter.h"
#include "Weapons/ImpactSurface.h"
#include "Weapons/ImpactSurfaceMap.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialParameters.h"
#include "PhysicsEngine/BodySetup.h"

#if WITH_EDITOR
namespace
{
	/** At most this many samples a side are averaged into one cell (a 4096 px window baked into 512 cells: all 8). */
	constexpr int32 SurfaceBakeMostSteps = 8;
	/** The scatter map's samples a side per cell. */
	constexpr int32 SurfaceBakeScatterSteps = 4;

	/** A texture's full-size source pixels (the PNG as imported), whatever their storage. */
	struct FSurfaceSourcePixels
	{
		int32 Width = 0;
		int32 Height = 0;
		ETextureSourceFormat Format = TSF_Invalid;
		TArray64<uint8> Data;

		/** A texel's channels, R G B A: colour turned linear from sRGB when asked (the macro map), alpha always as stored. */
		FLinearColor Texel(int32 X, int32 Y, bool bSRGB) const
		{
			const int64 Index = static_cast<int64>(FMath::Clamp(Y, 0, Height - 1)) * Width + FMath::Clamp(X, 0, Width - 1);
			switch (Format)
			{
			case TSF_BGRA8:
			{
				const uint8* P = Data.GetData() + Index * 4;
				if (bSRGB)
				{
					return FLinearColor(FLinearColor::sRGBToLinearTable[P[2]], FLinearColor::sRGBToLinearTable[P[1]],
						FLinearColor::sRGBToLinearTable[P[0]], P[3] / 255.f);
				}
				return FLinearColor(P[2] / 255.f, P[1] / 255.f, P[0] / 255.f, P[3] / 255.f);
			}
			case TSF_RGBA16:
			{
				const uint16* P = reinterpret_cast<const uint16*>(Data.GetData()) + Index * 4;
				const FLinearColor Stored(P[0] / 65535.f, P[1] / 65535.f, P[2] / 65535.f, P[3] / 65535.f);
				return bSRGB ? FLinearColor(Linear(Stored.R), Linear(Stored.G), Linear(Stored.B), Stored.A) : Stored;
			}
			case TSF_G8:
			{
				const float Value = bSRGB ? FLinearColor::sRGBToLinearTable[Data[Index]] : Data[Index] / 255.f;
				return FLinearColor(Value, Value, Value, 1.f);
			}
			case TSF_G16:
			{
				const float Stored = reinterpret_cast<const uint16*>(Data.GetData())[Index] / 65535.f;
				const float Value = bSRGB ? Linear(Stored) : Stored;
				return FLinearColor(Value, Value, Value, 1.f);
			}
			default:
				return FLinearColor::Black;
			}
		}

		/**
		 * The mean over the box U0..U1 x V0..V1 of the image (U across, V up from the image's bottom: the generator's own
		 * layout, north up), from at most Steps x Steps samples spread over it.
		 */
		FLinearColor Mean(double U0, double V0, double U1, double V1, int32 Steps, bool bSRGB) const
		{
			const double Left = FMath::Clamp(U0, 0.0, 1.0) * Width;
			const double Right = FMath::Clamp(U1, 0.0, 1.0) * Width;
			// Image rows run down from the top: V's top edge is the box's first row.
			const double Top = (1.0 - FMath::Clamp(V1, 0.0, 1.0)) * Height;
			const double Bottom = (1.0 - FMath::Clamp(V0, 0.0, 1.0)) * Height;
			const int32 Across = FMath::Clamp(FMath::CeilToInt32(Right - Left), 1, Steps);
			const int32 Down = FMath::Clamp(FMath::CeilToInt32(Bottom - Top), 1, Steps);
			FLinearColor Sum(0.f, 0.f, 0.f, 0.f);
			for (int32 Row = 0; Row < Down; ++Row)
			{
				const int32 Y = FMath::FloorToInt32(Top + (Bottom - Top) * (Row + 0.5) / Down);
				for (int32 Column = 0; Column < Across; ++Column)
				{
					const int32 X = FMath::FloorToInt32(Left + (Right - Left) * (Column + 0.5) / Across);
					Sum += Texel(X, Y, bSRGB);
				}
			}
			return Sum / static_cast<float>(Across * Down);
		}

		static float Linear(float Encoded)
		{
			return Encoded <= 0.04045f ? Encoded / 12.92f : FMath::Pow((Encoded + 0.055f) / 1.055f, 2.4f);
		}
	};

	/** The texture's source, read whole; false when it has none or keeps it in a form this doesn't read. */
	bool ReadSurfaceSource(UTexture2D& Texture, FSurfaceSourcePixels& Out)
	{
		FTextureSource& Source = Texture.Source;
		if (!Source.IsValid())
		{
			return false;
		}
		Out.Width = static_cast<int32>(Source.GetSizeX());
		Out.Height = static_cast<int32>(Source.GetSizeY());
		Out.Format = Source.GetFormat();
		if (Out.Width <= 0 || Out.Height <= 0
			|| (Out.Format != TSF_BGRA8 && Out.Format != TSF_RGBA16 && Out.Format != TSF_G8 && Out.Format != TSF_G16))
		{
			return false;
		}
		if (!Source.GetMipData(Out.Data, 0))
		{
			return false;
		}
		return Out.Data.Num() >= static_cast<int64>(Out.Width) * Out.Height * Source.GetBytesPerPixel();
	}
}
#endif

bool UImpactSurfaceTools::BakeTerrainMap(UMaterialInterface* Terrain, FVector2D WorldMin, FVector2D WorldMax, UTexture2D* Scatter,
	FVector2D ScatterMin, FVector2D ScatterMax, int32 Size)
{
#if WITH_EDITOR
	const FVector2D Span = WorldMax - WorldMin;
	if (!Terrain || Size < 4 || Size > 4096 || Span.X <= 0.0 || Span.Y <= 0.0)
	{
		UE_LOG(LogLooter, Warning, TEXT("Impact surfaces: no map baked for %s (a size of %d over %s..%s)."), *GetNameSafe(Terrain), Size,
			*WorldMin.ToString(), *WorldMax.ToString());
		return false;
	}
	UTexture* MacroTexture = nullptr;
	Terrain->GetTextureParameterValue(FHashedMaterialParameterInfo(TEXT("MacroMap")), MacroTexture);
	UTexture2D* Macro = Cast<UTexture2D>(MacroTexture);
	FSurfaceSourcePixels MacroPixels;
	if (!Macro || !ReadSurfaceSource(*Macro, MacroPixels))
	{
		UE_LOG(LogLooter, Warning, TEXT("Impact surfaces: %s has no macro map whose source can be read (%s)."), *Terrain->GetName(),
			*GetNameSafe(MacroTexture));
		return false;
	}
	FSurfaceSourcePixels ScatterPixels;
	const FVector2D ScatterSpan = ScatterMax - ScatterMin;
	const bool bScatter = Scatter && ScatterSpan.X > 0.0 && ScatterSpan.Y > 0.0 && ReadSurfaceSource(*Scatter, ScatterPixels);
	if (Scatter && !bScatter)
	{
		UE_LOG(LogLooter, Warning, TEXT("Impact surfaces: %s's scatter map %s can't be read: the macro map alone decides."), *Terrain->GetName(),
			*Scatter->GetName());
	}
	FLinearColor Cues = UImpactSurfaceMapData::DefaultCues;
	FLinearColor Cues2 = UImpactSurfaceMapData::DefaultCues2;
	Terrain->GetVectorParameterValue(FHashedMaterialParameterInfo(TEXT("GroundCues")), Cues);
	Terrain->GetVectorParameterValue(FHashedMaterialParameterInfo(TEXT("GroundCues2")), Cues2);

	// Rows run north (the macro map's V, X in the world), columns east (U, Y). Each cell is the mean of the macro texels
	// under it (colour linear, as the material reads it) and of the scatter map's densities there.
	TArray<uint8> Kinds;
	Kinds.SetNumUninitialized(Size * Size);
	const int32 MacroSteps = FMath::Clamp(FMath::CeilToInt32(static_cast<float>(MacroPixels.Width) / Size), 1, SurfaceBakeMostSteps);
	for (int32 Row = 0; Row < Size; ++Row)
	{
		const double V0 = static_cast<double>(Row) / Size;
		const double V1 = static_cast<double>(Row + 1) / Size;
		for (int32 Column = 0; Column < Size; ++Column)
		{
			const double U0 = static_cast<double>(Column) / Size;
			const double U1 = static_cast<double>(Column + 1) / Size;
			const FLinearColor Mean = MacroPixels.Mean(U0, V0, U1, V1, MacroSteps, /*bSRGB*/ true);
			FImpactGroundSample Sample;
			Sample.Macro = FLinearColor(Mean.R, Mean.G, Mean.B, 1.f);
			Sample.Select = Mean.A;
			if (bScatter)
			{
				// The same cell in the scatter map's own square (laid out as the macro map: U east, V north).
				const double X0 = WorldMin.X + Span.X * V0;
				const double X1 = WorldMin.X + Span.X * V1;
				const double Y0 = WorldMin.Y + Span.Y * U0;
				const double Y1 = WorldMin.Y + Span.Y * U1;
				const FLinearColor Density = ScatterPixels.Mean((Y0 - ScatterMin.Y) / ScatterSpan.Y, (X0 - ScatterMin.X) / ScatterSpan.X,
					(Y1 - ScatterMin.Y) / ScatterSpan.Y, (X1 - ScatterMin.X) / ScatterSpan.X, SurfaceBakeScatterSteps, /*bSRGB*/ false);
				Sample.Pebbles = Density.A;
			}
			Kinds[Row * Size + Column] = static_cast<uint8>(UImpactSurfaceMapData::ClassifyGround(Sample, Cues, Cues2));
		}
	}

	Terrain->Modify();
	UImpactSurfaceMapData* Map = NewObject<UImpactSurfaceMapData>(Terrain, NAME_None, RF_Transactional);
	Map->WorldMin = WorldMin;
	Map->WorldMax = WorldMax;
	Map->Size = Size;
	Map->Kinds = MoveTemp(Kinds);
	// Adding one replaces the material's older map (one of a class).
	Terrain->AddAssetUserData(Map);
	Terrain->MarkPackageDirty();
	ImpactSurface::ResetCache();
	UE_LOG(LogLooter, Display, TEXT("Impact surfaces: %s baked from %s (%dx%d, %s): %s"), *Terrain->GetName(), *Macro->GetName(),
		MacroPixels.Width, MacroPixels.Height, bScatter ? *Scatter->GetName() : TEXT("no scatter map"), *DescribeTerrainMap(Terrain));
	return true;
#else
	return false;
#endif
}

FString UImpactSurfaceTools::DescribeTerrainMap(UMaterialInterface* Terrain)
{
	const UImpactSurfaceMapData* Map = UImpactSurfaceMapData::FindOn(Terrain);
	if (!Map)
	{
		return FString::Printf(TEXT("%s carries no surface map"), *GetNameSafe(Terrain));
	}
	if (Map->Size <= 0 || Map->Kinds.Num() != Map->Size * Map->Size)
	{
		return FString::Printf(TEXT("%s's surface map is broken (%d cells for a side of %d)"), *GetNameSafe(Terrain), Map->Kinds.Num(), Map->Size);
	}
	int32 Counts[static_cast<int32>(EImpactMaterial::Count)] = {};
	for (const uint8 Kind : Map->Kinds)
	{
		if (Kind < static_cast<uint8>(EImpactMaterial::Count))
		{
			++Counts[Kind];
		}
	}
	FString Text = FString::Printf(TEXT("%d cells a side over %s..%s cm:"), Map->Size, *Map->WorldMin.ToString(), *Map->WorldMax.ToString());
	for (int32 Index = 0; Index < static_cast<int32>(EImpactMaterial::Count); ++Index)
	{
		if (Counts[Index] > 0)
		{
			Text += FString::Printf(TEXT(" %s %.0f%%"), ImpactSurface::Name(static_cast<EImpactMaterial>(Index)),
				100.0 * Counts[Index] / Map->Kinds.Num());
		}
	}
	return Text;
}

bool UImpactSurfaceTools::EnableTrimLookups(UStaticMesh* Mesh)
{
#if WITH_EDITOR
	if (!Mesh)
	{
		return false;
	}
	bool bTrim = false;
	for (const FStaticMaterial& Slot : Mesh->GetStaticMaterials())
	{
		bTrim = bTrim || ImpactSurface::IsTrimSheet(Slot.MaterialInterface);
	}
	UBodySetup* Body = Mesh->GetBodySetup();
	if (!bTrim || (Mesh->bSupportPhysicalMaterialMasks && Body && Body->bSupportUVsAndFaceRemap))
	{
		return false;
	}
	// As the mesh's own property change does (UStaticMesh::PostEditChangeProperty): the collision keeps the mesh's UVs and
	// its face remap, cooked with it, so a hit's face gives its UV in a packaged game too.
	Mesh->Modify();
	Mesh->bSupportPhysicalMaterialMasks = true;
	if (Body)
	{
		Body->Modify();
		Body->bSupportUVsAndFaceRemap = true;
		Body->InvalidatePhysicsData();
		Body->CreatePhysicsMeshes();
	}
	Mesh->MarkPackageDirty();
	return true;
#else
	return false;
#endif
}

bool UImpactSurfaceTools::HasTrimLookups(UStaticMesh* Mesh)
{
	if (!Mesh || !Mesh->bSupportPhysicalMaterialMasks)
	{
		return false;
	}
	for (const FStaticMaterial& Slot : Mesh->GetStaticMaterials())
	{
		if (ImpactSurface::IsTrimSheet(Slot.MaterialInterface))
		{
			return true;
		}
	}
	return false;
}

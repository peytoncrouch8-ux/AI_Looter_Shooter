#include "PropBaker.h"
#include "SurfaceMaterials.h"
#include "StylizedSurface.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/PointLight.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "FileHelpers.h"
#include "GeometryScript/CreateNewAssetUtilityFunctions.h"
#include "GeometryScript/MeshAssetFunctions.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Misc/Crc.h"
#include "PhysicsEngine/BodySetup.h"
#include "UDynamicMesh.h"

DEFINE_LOG_CATEGORY_STATIC(LogPropBaker, Log, All);

namespace
{
	const TCHAR* PropRoot = TEXT("/Game/Environment/Props");
	const TCHAR* MaterialFolder = TEXT("/Game/Environment/Props/Materials");
	const TCHAR* BeamMeshPath = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");

	/** How much a beacon's light pillar glows (the runtime prop used the same). */
	constexpr float BeamGlow = 2.5f;

	using SurfaceMaterials::LoadExisting;

	/** Every parameter the runtime material instance set, so equal surfaces share one material instance. */
	uint32 HashSurface(const FStylizedSurface& Surface)
	{
		const float Values[] = {
			Surface.Color.R, Surface.Color.G, Surface.Color.B, Surface.Color.A,
			Surface.TopColor.R, Surface.TopColor.G, Surface.TopColor.B, Surface.TopColor.A,
			Surface.TopBlend, Surface.TopThreshold, Surface.GradHeight, Surface.GradDark, Surface.Strata,
			Surface.Wind, Surface.Glow, Surface.Variation, Surface.UpNormal,
			Surface.bAdditive ? 1.f : 0.f, Surface.bTwoSided ? 1.f : 0.f };
		return FCrc::MemCrc32(Values, sizeof(Values));
	}

	FString ShapeName(EStylizedPropShape Shape)
	{
		return StaticEnum<EStylizedPropShape>()->GetNameStringByValue(static_cast<int64>(Shape));
	}

	/** The meadow's ground cover (Tools/Unreal/build_meadow.py scatters it): SM_<Kind>_V<n> under PropRoot/<Kind>. */
	struct FGroundCoverKind
	{
		const TCHAR* Kind;
		EStylizedPropShape Shape;
		int32 Variants;
		/**
		 * Its Nanite fallback's triangle percentage (the fallback is what Medium and Low draw). Fixed rather than
		 * automatic, which keeps more of a small mesh: these give each kind the triangles per square meter it had when
		 * the patches were four times the size (about 312, 310 and 1470 per patch), and the meadow draws thousands.
		 */
		float FallbackShare;
	};

	constexpr FGroundCoverKind GroundCoverKinds[] = {
		{ TEXT("GrassPatch"), EStylizedPropShape::GrassPatch, 6, 0.4f },
		{ TEXT("TallGrass"), EStylizedPropShape::TallGrass, 6, 0.36f },
		{ TEXT("WildGrass"), EStylizedPropShape::TallGrass, 4, 0.36f },
		{ TEXT("PoppyField"), EStylizedPropShape::FlowerPatch, 6, 0.76f },
		{ TEXT("Marigolds"), EStylizedPropShape::FlowerPatch, 4, 0.76f },
	};

	/** The seeds the variants were first baked with, so baking again keeps each variant's layout. */
	int32 VariantSeed(int32 Variant)
	{
		return 7919 * Variant;
	}

	/** Everything the generated mesh depends on, so identical props share one asset and baking again finds it. */
	uint32 HashProp(const AStylizedProp& Prop)
	{
		const int32 Settings[] = { static_cast<int32>(Prop.Shape), Prop.Seed };
		const FLinearColor Colors[] = { Prop.PrimaryColor, Prop.SecondaryColor };
		uint32 Hash = FCrc::MemCrc32(Settings, sizeof(Settings));
		Hash = FCrc::MemCrc32(Colors, sizeof(Colors), Hash);
		if (Prop.Shape == EStylizedPropShape::Hill)
		{
			// Hills fit the ground where they stand, so each placement is a mesh of its own.
			const FTransform Placement = Prop.GetActorTransform();
			const FVector Location = Placement.GetLocation();
			const FQuat Rotation = Placement.GetRotation();
			const FVector Scale = Placement.GetScale3D();
			Hash = FCrc::MemCrc32(&Location, sizeof(Location), Hash);
			Hash = FCrc::MemCrc32(&Rotation, sizeof(Rotation), Hash);
			Hash = FCrc::MemCrc32(&Scale, sizeof(Scale), Hash);
		}
		return Hash;
	}
}

FPropBaker::FPropBaker(UWorld* InWorld)
	: World(InWorld)
{
}

int32 FPropBaker::ConvertLevel()
{
	// Hills first: each one fits the ground under it, so it's baked while the world is as it was when the editor made it.
	TArray<AStylizedProp*> Props;
	for (TActorIterator<AStylizedProp> It(World); It; ++It)
	{
		Props.Add(*It);
	}
	Props.Sort([](const AStylizedProp& A, const AStylizedProp& B)
	{
		return A.Shape == EStylizedPropShape::Hill && B.Shape != EStylizedPropShape::Hill;
	});

	for (UPackage* Package : SurfaceMaterials::PrepareParents())
	{
		NewPackages.AddUnique(Package);
	}
	int32 Converted = 0;
	for (AStylizedProp* Prop : Props)
	{
		Converted += ConvertProp(Prop) ? 1 : 0;
	}
	return Converted;
}

bool FPropBaker::ConvertProp(AStylizedProp* Prop)
{
	const FBaked* Baked = FindOrBake(*Prop);
	if (!Baked)
	{
		return false;
	}

	const EStylizedPropShape Shape = Prop->Shape;
	const FTransform Transform = Prop->GetActorTransform();
	const FString Label = Prop->GetActorLabel();
	const FName FolderPath = Prop->GetFolderPath();
	const FString Folder = FolderPath.IsNone() ? FString::Printf(TEXT("Props/%s"), *ShapeName(Shape)) : FolderPath.ToString();
	World->EditorDestroyActor(Prop, true);
	return PlaceProp(*Baked, Shape, Transform, Label, Folder) != nullptr;
}

const FPropBaker::FBaked* FPropBaker::FindOrBake(const AStylizedProp& Prop)
{
	// The returned pointer is only good until the next call (the map may grow).
	const FString Kind = ShapeName(Prop.Shape);
	const FString PackagePath = FString(PropRoot) / Kind / FString::Printf(TEXT("SM_%s_%08X"), *Kind, HashProp(Prop));
	if (const FBaked* Found = BakedMeshes.Find(PackagePath))
	{
		return Found;
	}

	UDynamicMesh* Generated = NewObject<UDynamicMesh>(GetTransientPackage(), NAME_None, RF_Transient);
	FBaked Baked;
	Baked.Look = AStylizedProp::Generate(Prop.Shape, Prop.Seed, Prop.PrimaryColor, Prop.SecondaryColor, Prop.GetActorTransform(), &Prop, Generated);

	Baked.Mesh = LoadExisting<UStaticMesh>(PackagePath);
	if (!Baked.Mesh)
	{
		FGeometryScriptCreateNewStaticMeshAssetOptions Options;
		Options.bEnableRecomputeNormals = false; // the generator sets its own hard and soft edges
		Options.bEnableRecomputeTangents = true;
		Options.bEnableNanite = true;
		Options.NaniteSettings.bEnabled = true;
		if (AStylizedProp::IsGroundShape(Prop.Shape))
		{
			// The ground keeps every triangle in its fallback: Medium and Low draw the fallback, and the collision is cooked
			// from it, so anything set on the ground by a trace (props, the meadow, the player) sits on the drawn surface
			// on every preset. Reduced, it strayed up to 1.9 m from what High and Epic draw.
			Options.NaniteSettings.FallbackTarget = ENaniteFallbackTarget::PercentTriangles;
			Options.NaniteSettings.FallbackPercentTriangles = 1.f;
		}
		// Ground cover and clouds never collide, so they carry no collision data.
		const bool bSoft = AStylizedProp::IsSoftShape(Prop.Shape);
		Options.bEnableCollision = !bSoft;
		Options.CollisionMode = ECollisionTraceFlag::CTF_UseComplexAsSimple;
		EGeometryScriptOutcomePins Outcome = EGeometryScriptOutcomePins::Failure;
		Baked.Mesh = UGeometryScriptLibrary_CreateNewAssetFunctions::CreateNewStaticMeshAssetFromMesh(Generated, PackagePath, Options, Outcome);
		if (!Baked.Mesh || Outcome != EGeometryScriptOutcomePins::Success)
		{
			UE_LOG(LogPropBaker, Error, TEXT("Couldn't create %s."), *PackagePath);
			return nullptr;
		}

		// The runtime prop painted each slot with a material instance of its own; the asset uses shared ones.
		Baked.Mesh->Modify();
		TArray<FStaticMaterial>& Slots = Baked.Mesh->GetStaticMaterials();
		for (int32 Index = 0; Index < Baked.Look.Surfaces.Num(); ++Index)
		{
			UMaterialInterface* Material = FindOrCreateSurfaceMaterial(Baked.Look.Surfaces[Index]);
			if (Slots.IsValidIndex(Index))
			{
				Slots[Index].MaterialInterface = Material;
			}
			else
			{
				Slots.Add(FStaticMaterial(Material, *FString::Printf(TEXT("Surface%d"), Index)));
			}
		}
		UBodySetup* Body = Baked.Mesh->GetBodySetup();
		if (bSoft && Body)
		{
			// Actors placed with the mesh's own collision (dragged into a level by hand) don't collide either.
			Body->DefaultInstance.SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
		}
		Baked.Mesh->PostEditChange();
		NewPackages.Add(Baked.Mesh->GetPackage());
		UE_LOG(LogPropBaker, Log, TEXT("Baked %s: %d surfaces."), *PackagePath, Baked.Look.Surfaces.Num());
	}
	return &BakedMeshes.Add(PackagePath, MoveTemp(Baked));
}

AStaticMeshActor* FPropBaker::PlaceProp(const FBaked& Baked, EStylizedPropShape Shape, const FTransform& Transform, const FString& Label,
	const FString& Folder)
{
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AStaticMeshActor* Actor = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), Transform, Params);
	if (!Actor)
	{
		return nullptr;
	}
	Actor->SetActorLabel(Label);
	Actor->SetFolderPath(*Folder);

	UStaticMeshComponent* Mesh = Actor->GetStaticMeshComponent();
	// A placed static mesh actor takes its collision from the mesh unless told otherwise. Turn that off before the mesh
	// is set: while it's on, setting a profile the mesh already names changes nothing on the component itself.
	Mesh->bUseDefaultCollision = false;
	Mesh->SetStaticMesh(Baked.Mesh);
	const bool bSoft = AStylizedProp::IsSoftShape(Shape);
	// Walk and shoot straight through ground cover and clouds.
	Mesh->SetCollisionProfileName(bSoft ? UCollisionProfile::NoCollision_ProfileName : UCollisionProfile::BlockAll_ProfileName);
	Mesh->SetCastShadow(Baked.Look.bCastShadow);
	// Ground cover is marked in the custom stencil so the post process leaves it free of ink lines.
	Mesh->SetRenderCustomDepth(bSoft);
	Mesh->SetCustomDepthStencilValue(bSoft ? 1 : 0);
	Mesh->SetCullDistance(Baked.Look.CullDistance);

	const FName MinimapTag = AStylizedProp::MinimapTag(Shape);
	if (!MinimapTag.IsNone())
	{
		Actor->Tags.Add(MinimapTag);
	}

	const FStylizedPropLook& Look = Baked.Look;
	if (Look.BeamHeight > 0.f)
	{
		// A soft pillar of light standing on the prop (sky beacons), scaled with it.
		if (AStaticMeshActor* Beam = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), Transform, Params))
		{
			UStaticMeshComponent* BeamMesh = Beam->GetStaticMeshComponent();
			BeamMesh->bUseDefaultCollision = false;
			BeamMesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, BeamMeshPath));
			BeamMesh->SetMaterial(0, FindOrCreateBeamMaterial(Look.BeamColor, Look.BeamHeight));
			BeamMesh->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
			BeamMesh->SetCastShadow(false);
			BeamMesh->bReceivesDecals = false;
			Beam->AttachToActor(Actor, FAttachmentTransformRules::KeepRelativeTransform);
			// The engine cylinder is 100 x 100 with its pivot in the middle.
			Beam->SetActorRelativeTransform(FTransform(FRotator::ZeroRotator, FVector(0.f, 0.f, Look.BeamHeight * 0.5f),
				FVector(Look.BeamRadius / 50.f, Look.BeamRadius / 50.f, Look.BeamHeight / 100.f)));
			Beam->SetActorLabel(Label + TEXT("_Beam"));
			Beam->SetFolderPath(*Folder);
		}
	}
	if (Look.LightIntensity > 0.f)
	{
		if (APointLight* Light = World->SpawnActor<APointLight>(APointLight::StaticClass(), Transform, Params))
		{
			if (UPointLightComponent* LightComponent = Cast<UPointLightComponent>(Light->GetLightComponent()))
			{
				LightComponent->SetMobility(EComponentMobility::Movable);
				LightComponent->SetIntensityUnits(ELightUnits::Candelas);
				LightComponent->SetIntensity(Look.LightIntensity);
				LightComponent->SetAttenuationRadius(Look.LightRadius);
				LightComponent->SetLightColor(Look.LightColor);
				LightComponent->SetCastShadows(false);
			}
			Light->AttachToActor(Actor, FAttachmentTransformRules::KeepRelativeTransform);
			Light->SetActorRelativeTransform(FTransform(Look.LightOffset));
			Light->SetActorLabel(Label + TEXT("_Light"));
			Light->SetFolderPath(*Folder);
		}
	}
	return Actor;
}

int32 FPropBaker::BakeGroundCover()
{
	int32 Baked = 0;
	for (const FGroundCoverKind& Cover : GroundCoverKinds)
	{
		for (int32 Variant = 1; Variant <= Cover.Variants; ++Variant)
		{
			const FString PackagePath = FString(PropRoot) / Cover.Kind / FString::Printf(TEXT("SM_%s_V%d"), Cover.Kind, Variant);
			UStaticMesh* Mesh = LoadExisting<UStaticMesh>(PackagePath);
			if (!Mesh)
			{
				UE_LOG(LogPropBaker, Warning, TEXT("%s doesn't exist; ground cover is only baked again, never created."), *PackagePath);
				continue;
			}

			// The triangles and the fallback change: the slots are painted in the generator's slot order, so the asset
			// keeps its materials and (lack of) collision.
			UDynamicMesh* Generated = NewObject<UDynamicMesh>(GetTransientPackage(), NAME_None, RF_Transient);
			AStylizedProp::Generate(Cover.Shape, VariantSeed(Variant), FLinearColor::White, FLinearColor::White, FTransform::Identity, nullptr, Generated);
			FGeometryScriptCopyMeshToAssetOptions Options;
			Options.bEnableRecomputeNormals = false; // the generator sets its own hard and soft edges
			Options.bEnableRecomputeTangents = true;
			Options.bApplyNaniteSettings = true;
			Options.NewNaniteSettings = Mesh->GetNaniteSettings();
			Options.NewNaniteSettings.FallbackTarget = ENaniteFallbackTarget::PercentTriangles;
			Options.NewNaniteSettings.FallbackPercentTriangles = Cover.FallbackShare;
			EGeometryScriptOutcomePins Outcome = EGeometryScriptOutcomePins::Failure;
			UGeometryScriptLibrary_StaticMeshFunctions::CopyMeshToStaticMesh(Generated, Mesh, Options, FGeometryScriptMeshWriteLOD(), Outcome,
				/*bUseSectionMaterials*/ false);
			if (Outcome != EGeometryScriptOutcomePins::Success)
			{
				UE_LOG(LogPropBaker, Error, TEXT("Couldn't bake %s again."), *PackagePath);
				continue;
			}
			NewPackages.AddUnique(Mesh->GetPackage());
			++Baked;
		}
	}
	return Baked;
}

UMaterialInterface* FPropBaker::FindOrCreateSurfaceMaterial(const FStylizedSurface& Surface)
{
	const TCHAR* Kind = Surface.bAdditive ? TEXT("Glow") : (Surface.bTwoSided ? TEXT("Foliage") : TEXT("Surface"));
	const FString Name = FString::Printf(TEXT("MI_Prop%s_%08X"), Kind, HashSurface(Surface));
	if (UMaterialInterface** Found = Materials.Find(Name))
	{
		return *Found;
	}

	UMaterialInterface* Material = LoadExisting<UMaterialInstanceConstant>(FString(MaterialFolder) / Name);
	if (!Material)
	{
		UMaterialInterface* Parent = SurfaceMaterials::ParentFor(Surface);
		UMaterialInstanceConstant* Instance = Parent ? SurfaceMaterials::Create(MaterialFolder, Name, Parent) : nullptr;
		if (Instance)
		{
			SurfaceMaterials::Apply(Instance, Surface);
			NewPackages.Add(Instance->GetPackage());
		}
		Material = Instance ? static_cast<UMaterialInterface*>(Instance) : Parent;
	}
	Materials.Add(Name, Material);
	return Material;
}

UMaterialInterface* FPropBaker::FindOrCreateBeamMaterial(const FLinearColor& Color, float Height)
{
	const float Values[] = { Color.R, Color.G, Color.B, Color.A, Height };
	const FString Name = FString::Printf(TEXT("MI_PropBeam_%08X"), FCrc::MemCrc32(Values, sizeof(Values)));
	if (UMaterialInterface** Found = Materials.Find(Name))
	{
		return *Found;
	}

	UMaterialInterface* Material = LoadExisting<UMaterialInstanceConstant>(FString(MaterialFolder) / Name);
	if (!Material)
	{
		FStylizedSurface Glow;
		Glow.bAdditive = true;
		UMaterialInterface* Parent = SurfaceMaterials::ParentFor(Glow);
		UMaterialInstanceConstant* Instance = Parent ? SurfaceMaterials::Create(MaterialFolder, Name, Parent) : nullptr;
		if (Instance)
		{
			// The same three settings the runtime beam used; the rest stay at the material's defaults.
			Instance->SetVectorParameterValueEditorOnly(FMaterialParameterInfo(TEXT("Color")), Color);
			Instance->SetScalarParameterValueEditorOnly(FMaterialParameterInfo(TEXT("Glow")), BeamGlow);
			Instance->SetScalarParameterValueEditorOnly(FMaterialParameterInfo(TEXT("GradHeight")), Height);
			Instance->PostEditChange();
			NewPackages.Add(Instance->GetPackage());
		}
		Material = Instance ? static_cast<UMaterialInterface*>(Instance) : Parent;
	}
	Materials.Add(Name, Material);
	return Material;
}

bool FPropBaker::SaveAll(bool bIncludeLevel)
{
	TArray<UPackage*> Packages = NewPackages;
	if (bIncludeLevel)
	{
		Packages.AddUnique(World->GetPackage());
	}
	return UEditorLoadingAndSavingUtils::SavePackages(Packages, false);
}

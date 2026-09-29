#include "PropBaker.h"
#include "Environment/EnvironmentLayout.h"
#include "Environment/EnvironmentPalette.h"
#include "Environment/LevelLayoutData.h"
#include "Environment/StylizedSurface.h"
#include "World/MinimapSubsystem.h"
#include "AssetToolsModule.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/PointLight.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Factories/MaterialInstanceConstantFactoryNew.h"
#include "FileHelpers.h"
#include "GeometryScript/CreateNewAssetUtilityFunctions.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Misc/Crc.h"
#include "Misc/PackageName.h"
#include "UDynamicMesh.h"

DEFINE_LOG_CATEGORY_STATIC(LogPropBaker, Log, All);

namespace
{
	const TCHAR* PropRoot = TEXT("/Game/Environment/Props");
	const TCHAR* MaterialFolder = TEXT("/Game/Environment/Props/Materials");
	const TCHAR* SurfaceMaterialPath = TEXT("/Game/Environment/Materials/M_StylizedSurface.M_StylizedSurface");
	const TCHAR* FoliageMaterialPath = TEXT("/Game/Environment/Materials/M_StylizedFoliage.M_StylizedFoliage");
	const TCHAR* GlowMaterialPath = TEXT("/Game/Environment/Materials/M_StylizedGlow.M_StylizedGlow");
	const TCHAR* BeamMeshPath = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");

	/** Kinds placed more often than this share a few baked variants; rarer ones keep each placement's exact shape. */
	constexpr int32 MaxExactPlacements = 8;

	/** How much a beacon's light pillar glows (the runtime prop used the same). */
	constexpr float BeamGlow = 2.5f;

	int32 VariantCount(int32 Placements)
	{
		return Placements > 30 ? 6 : 4;
	}

	/** Fixed seeds, so baking again makes the same variants. */
	int32 VariantSeed(int32 Variant)
	{
		return 7919 * (Variant + 1);
	}

	template <typename T>
	T* LoadExisting(const FString& PackagePath)
	{
		if (!FPackageName::DoesPackageExist(PackagePath))
		{
			return nullptr;
		}
		return LoadObject<T>(nullptr, *(PackagePath + TEXT(".") + FPackageName::GetShortName(PackagePath)));
	}

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

	UMaterialInstanceConstant* CreateMaterialInstance(const FString& Name, UMaterialInterface* Parent)
	{
		IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get();
		UMaterialInstanceConstantFactoryNew* Factory = NewObject<UMaterialInstanceConstantFactoryNew>();
		Factory->InitialParent = Parent;
		return Cast<UMaterialInstanceConstant>(AssetTools.CreateAsset(Name, MaterialFolder, UMaterialInstanceConstant::StaticClass(), Factory));
	}

	FString ShapeName(EStylizedPropShape Shape)
	{
		return StaticEnum<EStylizedPropShape>()->GetNameStringByValue(static_cast<int64>(Shape));
	}
}

FPropBaker::FPropBaker(UWorld* InWorld)
	: World(InWorld)
{
}

int32 FPropBaker::ConvertLevel()
{
	int32 Placed = 0;

	// Hand-placed props first, ground before the rest: the island terrain among them is what the layout's hills fit to.
	TArray<AStylizedProp*> HandPlaced;
	for (TActorIterator<AStylizedProp> It(World); It; ++It)
	{
		HandPlaced.Add(*It);
	}
	HandPlaced.Sort([](const AStylizedProp& A, const AStylizedProp& B)
	{
		return AStylizedProp::IsGroundShape(A.Shape) && !AStylizedProp::IsGroundShape(B.Shape);
	});
	for (AStylizedProp* Prop : HandPlaced)
	{
		Placed += ConvertHandPlaced(Prop);
	}

	TArray<AEnvironmentLayout*> Layouts;
	for (TActorIterator<AEnvironmentLayout> It(World); It; ++It)
	{
		Layouts.Add(*It);
	}
	for (AEnvironmentLayout* Layout : Layouts)
	{
		Placed += ConvertLayout(Layout);
	}
	return Placed;
}

int32 FPropBaker::ConvertHandPlaced(AStylizedProp* Prop)
{
	// Hand-placed props are one-offs (the island itself), so each keeps its exact shape.
	const FString Kind = ShapeName(Prop->Shape);
	const FString Name = FString::Printf(TEXT("SM_%s_Placed%02d"), *Kind, ++HandPlacedCounts.FindOrAdd(Kind));
	const FBaked* Baked = FindOrBake(FString(PropRoot) / Kind, Name, Prop->Shape, Prop->Seed, Prop->PrimaryColor, Prop->SecondaryColor,
		Prop->GetActorTransform(), Prop);
	if (!Baked)
	{
		return 0;
	}

	const EStylizedPropShape Shape = Prop->Shape;
	const FTransform Transform = Prop->GetActorTransform();
	const FString Label = Prop->GetActorLabel();
	const FName FolderPath = Prop->GetFolderPath();
	const FString Folder = FolderPath.IsNone() ? FString::Printf(TEXT("Props/%s"), *Kind) : FolderPath.ToString();
	World->EditorDestroyActor(Prop, true);
	return PlaceProp(*Baked, Shape, Transform, Label, Folder) ? 1 : 0;
}

int32 FPropBaker::ConvertLayout(AEnvironmentLayout* Layout)
{
	const UEnvironmentPalette* Palette = Layout->Palette;
	const ULevelLayoutData* Data = Layout->LayoutData;
	if (!Palette || !Data)
	{
		UE_LOG(LogPropBaker, Warning, TEXT("%s has no palette or layout data; left as it is."), *Layout->GetActorLabel());
		return 0;
	}

	TMap<FName, int32> Placements;
	for (const FPlacedObjectRecord& Record : Data->Objects)
	{
		++Placements.FindOrAdd(Record.EntryId);
	}

	int32 Placed = 0;
	TMap<FName, int32> Counters;
	for (const FPlacedObjectRecord& Record : Data->Objects)
	{
		const FEnvironmentPaletteEntry* Entry = Palette->FindEntry(Record.EntryId);
		if (!Entry)
		{
			UE_LOG(LogPropBaker, Warning, TEXT("Layout entry '%s' isn't in the palette; skipped."), *Record.EntryId.ToString());
			continue;
		}
		const FString Kind = Record.EntryId.ToString();
		const int32 Number = ++Counters.FindOrAdd(Record.EntryId);
		const FString Label = FString::Printf(TEXT("%s_%02d"), *Kind, Number);
		const FString Folder = FString::Printf(TEXT("Props/%s"), *Kind);

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		// Creatures, dummies and anything else with a class of its own is placed as that actor.
		if (UClass* ActorClass = Entry->ActorClass.LoadSynchronous())
		{
			if (AActor* Actor = World->SpawnActor<AActor>(ActorClass, Record.Transform, Params))
			{
				Actor->SetActorLabel(Label);
				Actor->SetFolderPath(*FString::Printf(TEXT("Creatures/%s"), *Kind));
				++Placed;
			}
			continue;
		}
		if (UStaticMesh* StaticMesh = Entry->StaticMesh.LoadSynchronous())
		{
			if (AStaticMeshActor* Actor = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), Record.Transform, Params))
			{
				Actor->GetStaticMeshComponent()->SetStaticMesh(StaticMesh);
				Actor->SetActorLabel(Label);
				Actor->SetFolderPath(*Folder);
				++Placed;
			}
			continue;
		}

		// Kinds placed many times share a few variants; the rest, and hills (they fit the ground where they stand), keep
		// each placement's exact shape.
		const int32 Count = Placements[Record.EntryId];
		const bool bExact = Entry->Shape == EStylizedPropShape::Hill || Count <= MaxExactPlacements;
		FString Name;
		int32 Seed = Record.Seed;
		if (bExact)
		{
			Name = FString::Printf(TEXT("SM_%s_%02d"), *Kind, Number);
		}
		else
		{
			const int32 Variant = static_cast<int32>(static_cast<uint32>(Record.Seed) % static_cast<uint32>(VariantCount(Count)));
			Name = FString::Printf(TEXT("SM_%s_V%d"), *Kind, Variant + 1);
			Seed = VariantSeed(Variant);
		}

		const FBaked* Baked = FindOrBake(FString(PropRoot) / Kind, Name, Entry->Shape, Seed, Entry->PrimaryColor, Entry->SecondaryColor,
			Record.Transform, Layout);
		if (Baked && PlaceProp(*Baked, Entry->Shape, Record.Transform, Label, Folder))
		{
			++Placed;
		}
	}

	UE_LOG(LogPropBaker, Log, TEXT("%s: placed %d of %d objects; removing the layout actor."), *Layout->GetActorLabel(), Placed, Data->Objects.Num());
	World->EditorDestroyActor(Layout, true);
	return Placed;
}

const FPropBaker::FBaked* FPropBaker::FindOrBake(const FString& Folder, const FString& Name, EStylizedPropShape Shape, int32 Seed,
	const FLinearColor& Primary, const FLinearColor& Secondary, const FTransform& Placement, const AActor* ProbeActor)
{
	// The returned pointer is only good until the next call (the map may grow).
	const FString PackagePath = Folder / Name;
	if (const FBaked* Found = BakedMeshes.Find(PackagePath))
	{
		return Found;
	}

	UDynamicMesh* Generated = NewObject<UDynamicMesh>(GetTransientPackage(), NAME_None, RF_Transient);
	FBaked Baked;
	Baked.Look = AStylizedProp::Generate(Shape, Seed, Primary, Secondary, Placement, ProbeActor, Generated);

	Baked.Mesh = LoadExisting<UStaticMesh>(PackagePath);
	if (!Baked.Mesh)
	{
		FGeometryScriptCreateNewStaticMeshAssetOptions Options;
		Options.bEnableRecomputeNormals = false; // the generator sets its own hard and soft edges
		Options.bEnableRecomputeTangents = true;
		Options.bEnableNanite = true;
		Options.NaniteSettings.bEnabled = true;
		Options.bEnableCollision = true;
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
	Mesh->SetStaticMesh(Baked.Mesh);
	const bool bSoft = AStylizedProp::IsSoftShape(Shape);
	if (bSoft)
	{
		// Walk and shoot straight through it; visibility queries still see it as an overlap.
		Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Mesh->SetCollisionObjectType(ECC_WorldDynamic);
		Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
		Mesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Overlap);
	}
	else
	{
		Mesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	}
	Mesh->SetCastShadow(Baked.Look.bCastShadow);
	// Ground cover is marked in the custom stencil so the post process leaves it free of ink lines.
	Mesh->SetRenderCustomDepth(bSoft);
	Mesh->SetCustomDepthStencilValue(bSoft ? 1 : 0);
	Mesh->SetCullDistance(Baked.Look.CullDistance);

	// What the minimap draws: ground, or something standing on it. Soft cover isn't drawn.
	if (AStylizedProp::IsGroundShape(Shape))
	{
		Actor->Tags.Add(MinimapTags::Ground);
	}
	else if (!bSoft)
	{
		Actor->Tags.Add(MinimapTags::Obstacle);
	}

	const FStylizedPropLook& Look = Baked.Look;
	if (Look.BeamHeight > 0.f)
	{
		// A soft pillar of light standing on the prop (sky beacons), scaled with it.
		if (AStaticMeshActor* Beam = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), Transform, Params))
		{
			UStaticMeshComponent* BeamMesh = Beam->GetStaticMeshComponent();
			BeamMesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, BeamMeshPath));
			BeamMesh->SetMaterial(0, FindOrCreateBeamMaterial(Look.BeamColor, Look.BeamHeight));
			BeamMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
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
		const TCHAR* ParentPath = Surface.bAdditive ? GlowMaterialPath : (Surface.bTwoSided ? FoliageMaterialPath : SurfaceMaterialPath);
		UMaterialInterface* Parent = LoadObject<UMaterialInterface>(nullptr, ParentPath);
		UMaterialInstanceConstant* Instance = Parent ? CreateMaterialInstance(Name, Parent) : nullptr;
		if (Instance)
		{
			auto Vector = [Instance](const TCHAR* Param, const FLinearColor& Value)
			{
				Instance->SetVectorParameterValueEditorOnly(FMaterialParameterInfo(Param), Value);
			};
			auto Scalar = [Instance](const TCHAR* Param, float Value)
			{
				Instance->SetScalarParameterValueEditorOnly(FMaterialParameterInfo(Param), Value);
			};
			Vector(TEXT("Color"), Surface.Color);
			Vector(TEXT("TopColor"), Surface.TopColor);
			Scalar(TEXT("TopBlend"), Surface.TopBlend);
			Scalar(TEXT("TopThreshold"), Surface.TopThreshold);
			Scalar(TEXT("GradHeight"), Surface.GradHeight);
			Scalar(TEXT("GradDark"), Surface.GradDark);
			Scalar(TEXT("Strata"), Surface.Strata);
			Scalar(TEXT("Wind"), Surface.Wind);
			Scalar(TEXT("Glow"), Surface.Glow);
			Scalar(TEXT("Variation"), Surface.Variation);
			Scalar(TEXT("UpNormal"), Surface.UpNormal);
			Instance->PostEditChange();
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
		UMaterialInterface* Parent = LoadObject<UMaterialInterface>(nullptr, GlowMaterialPath);
		UMaterialInstanceConstant* Instance = Parent ? CreateMaterialInstance(Name, Parent) : nullptr;
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

bool FPropBaker::SaveAll()
{
	TArray<UPackage*> Packages = NewPackages;
	Packages.AddUnique(World->GetPackage());
	return UEditorLoadingAndSavingUtils::SavePackages(Packages, false);
}

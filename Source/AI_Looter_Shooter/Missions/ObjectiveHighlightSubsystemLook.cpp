// UObjectiveHighlightSubsystem: the highlight on the lit things' meshes. Each lit mesh wears an instance of
// M_ObjectiveHighlight as its overlay (UMeshComponent::SetOverlayMaterial: the mesh drawn once more, translucent, on top),
// the whole of it, a spot round the thing's point of use, or a character's rim; picking which meshes wear it; taking it off
// again as they were.

#include "Missions/ObjectiveHighlightSubsystem.h"
#include "AI_Looter_Shooter.h"
#include "Interaction/Interactable.h"
#include "Story/StoryCharacter.h"
#include "UI/Style/LooterUIStyle.h"
#include "Weapons/WeaponBase.h"
#include "CollisionQueryParams.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/MeshComponent.h"
#include "Components/SkinnedMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "HAL/IConsoleManager.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

namespace
{
	// M_ObjectiveHighlight's parameters the code sets (Tools/Unreal/build_highlight_material.py makes them).
	const FName HighlightStrengthParam(TEXT("Strength"));
	const FName HighlightColorParam(TEXT("Color"));
	const FName HighlightMaskCenterParam(TEXT("MaskCenter"));
	const FName HighlightMaskRadiusParam(TEXT("MaskRadius"));
	/** 1: a character's look (the rim alone, its body all but clear); 0: a thing's (a soft fill and a rim). */
	const FName HighlightFigureParam(TEXT("Figure"));
	/** Walkable terrain carries it for the minimap: the ground never lights, even as a spot. */
	const FName HighlightGroundTag(TEXT("Ground"));

	/** Nanite draws this mesh now, so it would draw no overlay (Medium has Nanite off: never there). */
	bool NaniteDrawsIt(const UMeshComponent& Mesh)
	{
		static const TConsoleVariableData<int32>* NaniteSetting = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.Nanite"));
		if (!NaniteSetting || NaniteSetting->GetValueOnGameThread() == 0)
		{
			return false;
		}
		if (const UStaticMeshComponent* Static = Cast<UStaticMeshComponent>(&Mesh))
		{
			return !Static->IsForceDisableNanite() && !Static->IsDisallowNanite() && Static->HasValidNaniteData();
		}
		if (const USkinnedMeshComponent* Skinned = Cast<USkinnedMeshComponent>(&Mesh))
		{
			return USkinnedMeshComponent::ShouldRenderNaniteSkinnedMeshes() && !Skinned->IsForceDisableNanite() && !Skinned->IsDisallowNanite()
				&& Skinned->HasValidNaniteData();
		}
		return false;
	}

	void SetNaniteOff(UMeshComponent& Mesh, bool bOff)
	{
		if (UStaticMeshComponent* Static = Cast<UStaticMeshComponent>(&Mesh))
		{
			Static->SetForceDisableNanite(bOff);
		}
		else if (USkinnedMeshComponent* Skinned = Cast<USkinnedMeshComponent>(&Mesh))
		{
			Skinned->SetForceDisableNanite(bOff);
		}
	}

	/**
	 * Mesh can wear the overlay: shown, a real model, never instanced scatter, and solid. An overlay covers every triangle
	 * whatever the surface lets through, so see-through surfaces (glass, a beam, a glow) never wear it, and masked ones only
	 * on a character (bFigure: its body and hat are masked for a ghost's fade); anywhere else masked means cards, leaves
	 * and webs, whose whole quads would light up.
	 */
	bool CanWear(const UMeshComponent& Mesh, bool bFigure)
	{
		if (!Mesh.IsRegistered() || !Mesh.IsVisible() || Mesh.IsA<UInstancedStaticMeshComponent>())
		{
			return false;
		}
		const AActor* Owner = Mesh.GetOwner();
		if (Owner && Owner->IsHidden())
		{
			return false;
		}
		if (const UStaticMeshComponent* Static = Cast<UStaticMeshComponent>(&Mesh))
		{
			if (!Static->GetStaticMesh())
			{
				return false;
			}
		}
		else if (const USkinnedMeshComponent* Skinned = Cast<USkinnedMeshComponent>(&Mesh))
		{
			if (!Skinned->GetSkinnedAsset())
			{
				return false;
			}
		}
		else
		{
			return false;
		}
		for (int32 Slot = 0; Slot < Mesh.GetNumMaterials(); ++Slot)
		{
			const UMaterialInterface* Surface = Mesh.GetMaterial(Slot);
			const EBlendMode Blend = Surface ? Surface->GetBlendMode() : BLEND_Opaque;
			if (Blend == BLEND_Masked ? !bFigure : Blend != BLEND_Opaque)
			{
				return false;
			}
		}
		return true;
	}

	/** The highlight's instance for a fit (each fit has its own: a spot's mask, a figure's look). */
	UMaterialInstanceDynamic*& LookSlot(FObjectiveHighlight& Highlight, EObjectiveHighlightFit Fit)
	{
		return Fit == EObjectiveHighlightFit::Spot ? Highlight.SpotLook
			: Fit == EObjectiveHighlightFit::Figure ? Highlight.FigureLook : Highlight.WholeLook;
	}

	UMaterialInstanceDynamic* LookFor(const FObjectiveHighlight& Highlight, const FObjectiveHighlightMesh& Mesh)
	{
		return Mesh.Fit == EObjectiveHighlightFit::Spot ? Highlight.SpotLook
			: Mesh.Fit == EObjectiveHighlightFit::Figure ? Highlight.FigureLook : Highlight.WholeLook;
	}
}

// ---------------------------------------------------------------------------
// Which meshes
// ---------------------------------------------------------------------------

bool UObjectiveHighlightSubsystem::IsFigure(const AActor& Actor)
{
	// The story's people and Hob, whatever their body is made of: Sexton is a posed static model, Amos a posed skinned one.
	if (Actor.IsA<AStoryCharacter>())
	{
		return true;
	}
	// A gun lying about is a prop, even one drawn skinned.
	if (Actor.IsA<AWeaponBase>())
	{
		return false;
	}
	TInlineComponentArray<USkinnedMeshComponent*> Skinned(&Actor);
	return Skinned.ContainsByPredicate([](const USkinnedMeshComponent* Mesh) { return Mesh && Mesh->GetSkinnedAsset(); });
}

void UObjectiveHighlightSubsystem::PickMeshes(const AActor& Actor, const FVector& Focus, TArray<FObjectiveHighlightMesh>& Out)
{
	Out.Reset();
	TInlineComponentArray<UMeshComponent*> Own(&Actor);
	// A character wears the rim alone (the figure look): a fill dark clothes can't take reads as a solid orange figure.
	const bool bFigure = IsFigure(Actor);
	FBox Whole(ForceInit);
	for (UMeshComponent* Mesh : Own)
	{
		if (!Mesh || Out.Num() >= MaxMeshes || !CanWear(*Mesh, bFigure))
		{
			continue;
		}
		if (Mesh->Bounds.SphereRadius <= LargeMeshRadius)
		{
			Out.Add({ Mesh, bFigure ? EObjectiveHighlightFit::Figure : EObjectiveHighlightFit::Whole });
			Whole += Mesh->Bounds.GetBox();
		}
		else if (Mesh->Bounds.SphereRadius <= MaxSpotMeshRadius
			&& Mesh->Bounds.GetBox().ComputeSquaredDistanceToPoint(Focus) <= FMath::Square(SpotRadius))
		{
			// A building or a rig of its own (the jetty, the lift head): a spot round its point of use, never all of it.
			Out.Add({ Mesh, EObjectiveHighlightFit::Spot });
		}
	}
	// Used at a point away from what it shows (the bell's rope inside the door, under the bell in the belfry), or showing
	// nothing of its own there (Delia's screen door in the farmhouse's mesh, a poster on its wall, a notice board's face, a
	// readable that is the dressing's model): what's at that point lights as a spot round it. Plain markers (a reach-place's
	// target point) have no point of use, so the ground or a rock beside them never lights.
	const IInteractable* Usable = Cast<IInteractable>(&Actor);
	const bool bPointAway = Usable && Usable->GetInteractionLocation().IsSet()
		&& (!Whole.IsValid || Whole.ComputeSquaredDistanceToPoint(Focus) > FMath::Square(SpotRadius));
	const bool bOwnSpot = Out.ContainsByPredicate([](const FObjectiveHighlightMesh& Each) { return Each.Fit == EObjectiveHighlightFit::Spot; });
	UWorld* World = Actor.GetWorld();
	if (!bPointAway || bOwnSpot || !World)
	{
		return;
	}
	FCollisionObjectQueryParams Solid;
	Solid.AddObjectTypesToQuery(ECC_WorldStatic);
	Solid.AddObjectTypesToQuery(ECC_WorldDynamic);
	const FCollisionQueryParams Params(SCENE_QUERY_STAT(ObjectiveHighlightSpot), /*bTraceComplex*/ false, &Actor);
	TArray<FOverlapResult> Overlaps;
	World->OverlapMultiByObjectType(Overlaps, Focus, FQuat::Identity, Solid, FCollisionShape::MakeSphere(SpotSearchRadius), Params);
	for (const FOverlapResult& Overlap : Overlaps)
	{
		UMeshComponent* Mesh = Cast<UMeshComponent>(Overlap.GetComponent());
		const AActor* Owner = Mesh ? Mesh->GetOwner() : nullptr;
		if (!Owner || Out.Num() >= MaxMeshes || Owner->IsA<APawn>() || Owner->ActorHasTag(HighlightGroundTag)
			|| Mesh->Bounds.SphereRadius > MaxSpotMeshRadius || !CanWear(*Mesh, /*bCharacter*/ false)
			|| Out.ContainsByPredicate([Mesh](const FObjectiveHighlightMesh& Each) { return Each.Component.Get() == Mesh; }))
		{
			continue;
		}
		Out.Add({ Mesh, EObjectiveHighlightFit::Spot });
	}
}

// ---------------------------------------------------------------------------
// On and off
// ---------------------------------------------------------------------------

void UObjectiveHighlightSubsystem::Light(FObjectiveHighlight& Highlight)
{
	const AActor* Actor = Highlight.Actor.Get();
	if (!Actor)
	{
		return;
	}
	Highlight.bLit = true;
	PickMeshes(*Actor, Highlight.Focus, Highlight.Meshes);
	// Without its material the rules still run (the tests, Looter.Highlight.List say so); nothing is drawn.
	if (Highlight.Meshes.IsEmpty() || !LoadMaterial())
	{
		return;
	}
	for (const FObjectiveHighlightMesh& Mesh : Highlight.Meshes)
	{
		UMaterialInstanceDynamic*& Look = LookSlot(Highlight, Mesh.Fit);
		if (!Look)
		{
			Look = TakeLook(Mesh.Fit);
		}
	}
	ShowStrength(Highlight);
	for (FObjectiveHighlightMesh& Mesh : Highlight.Meshes)
	{
		ApplyTo(Mesh, LookFor(Highlight, Mesh));
	}
}

void UObjectiveHighlightSubsystem::Unlight(FObjectiveHighlight& Highlight)
{
	for (FObjectiveHighlightMesh& Mesh : Highlight.Meshes)
	{
		UMeshComponent* Component = Mesh.Component.Get();
		if (!Component)
		{
			continue;
		}
		// Only its own: a hit flash showing now is the dummy's to clear.
		const UMaterialInstanceDynamic* Look = LookFor(Highlight, Mesh);
		if (Look && Component->OverlayMaterial.Get() == Look)
		{
			Component->SetOverlayMaterial(nullptr);
		}
		if (Mesh.bForcedFallback)
		{
			SetNaniteOff(*Component, false);
		}
	}
	Highlight.Meshes.Reset();
	ReturnLook(Highlight.WholeLook);
	ReturnLook(Highlight.SpotLook);
	ReturnLook(Highlight.FigureLook);
	Highlight.bLit = false;
}

void UObjectiveHighlightSubsystem::Reclaim()
{
	for (FObjectiveHighlight& Highlight : Highlights)
	{
		for (FObjectiveHighlightMesh& Mesh : Highlight.Meshes)
		{
			ApplyTo(Mesh, LookFor(Highlight, Mesh));
		}
	}
}

void UObjectiveHighlightSubsystem::ApplyTo(FObjectiveHighlightMesh& Mesh, UMaterialInstanceDynamic* Look) const
{
	UMeshComponent* Component = Mesh.Component.Get();
	if (!Component || !Look)
	{
		return;
	}
	// Nanite draws no overlay: while lit, a Nanite mesh shows its fallback (which Medium always shows). Asked every time, as a
	// second highlight sharing the mesh (two doors of one house) may have given Nanite back as it went out.
	if (NaniteDrawsIt(*Component))
	{
		SetNaniteOff(*Component, true);
		Mesh.bForcedFallback = true;
	}
	// On already; or something else shows there now (a dummy's or an egg sac's hit flash, another highlight), which clears
	// its own when it's done: this one comes back then, rather than wiping the flash or bringing a stale one back later.
	if (Component->OverlayMaterial.Get() != nullptr)
	{
		return;
	}
	Component->SetOverlayMaterial(Look);
}

void UObjectiveHighlightSubsystem::ShowStrength(FObjectiveHighlight& Highlight) const
{
	const float Shown = ShownStrength(Highlight.Strength);
	for (UMaterialInstanceDynamic* Look : { Highlight.WholeLook, Highlight.FigureLook })
	{
		if (Look)
		{
			Look->SetScalarParameterValue(HighlightStrengthParam, Shown);
		}
	}
	if (Highlight.SpotLook)
	{
		Highlight.SpotLook->SetScalarParameterValue(HighlightStrengthParam, Shown);
		const FLinearColor Center(static_cast<float>(Highlight.Focus.X), static_cast<float>(Highlight.Focus.Y),
			static_cast<float>(Highlight.Focus.Z), 0.f);
		Highlight.SpotLook->SetVectorParameterValue(HighlightMaskCenterParam, Center);
	}
}

// ---------------------------------------------------------------------------
// The material and its instances
// ---------------------------------------------------------------------------

UMaterialInstanceDynamic* UObjectiveHighlightSubsystem::TakeLook(EObjectiveHighlightFit Fit)
{
	UMaterialInstanceDynamic* Look = FreeLooks.IsEmpty() ? nullptr : FreeLooks.Pop();
	if (!Look)
	{
		UMaterialInterface* Base = LoadMaterial();
		if (!Base)
		{
			return nullptr;
		}
		Look = UMaterialInstanceDynamic::Create(Base, this);
		Looks.Add(Look);
		// The tracker's orange, so the world's highlight and the HUD's line read as one.
		Look->SetVectorParameterValue(HighlightColorParam, LooterUI::Color::Accent());
	}
	// Set every time, as the pool's instances go round all three fits. MaskRadius 0 lights the whole mesh; over 0, a spot
	// that big round MaskCenter. Figure 1 is a character's look (a material older than the figure look ignores it).
	Look->SetScalarParameterValue(HighlightMaskRadiusParam, Fit == EObjectiveHighlightFit::Spot ? SpotRadius : 0.f);
	Look->SetScalarParameterValue(HighlightFigureParam, Fit == EObjectiveHighlightFit::Figure ? 1.f : 0.f);
	Look->SetScalarParameterValue(HighlightStrengthParam, 0.f);
	return Look;
}

void UObjectiveHighlightSubsystem::ReturnLook(UMaterialInstanceDynamic*& Look)
{
	if (Look)
	{
		FreeLooks.Add(Look);
		Look = nullptr;
	}
}

UMaterialInterface* UObjectiveHighlightSubsystem::LoadMaterial()
{
	if (!HighlightMaterial && !bMaterialLookedFor)
	{
		bMaterialLookedFor = true;
		HighlightMaterial = LoadObject<UMaterialInterface>(nullptr, MaterialPath, nullptr, LOAD_NoWarn);
		if (!HighlightMaterial)
		{
			UE_LOG(LogLooter, Warning, TEXT("Objective highlight: %s is missing (run Tools/Unreal/build_highlight_material.py): ")
				TEXT("nothing is drawn."), MaterialPath);
		}
	}
	return HighlightMaterial;
}

bool UObjectiveHighlightSubsystem::HasMaterial()
{
	return LoadMaterial() != nullptr;
}

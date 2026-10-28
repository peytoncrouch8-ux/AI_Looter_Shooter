#include "Player/FirstPersonHandsGrips.h"
#include "AI_Looter_Shooter.h"
#include "Weapons/WeaponModelComponent.h"
#include "Chaos/TriangleMeshImplicitObject.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "HAL/IConsoleManager.h"
#include "PhysicsEngine/BodySetup.h"

// The support hand's cup fitted to the gun's own parts (2026-10-28). The cup table (FirstPersonHandsCup.cpp) was read off
// one model per kind; a gun is assembled from parts, and another body or magazine has its underside elsewhere (the AR's
// Marksman body has no thumbhole loop where the Standard's floor is: the hand hung open under nothing). So the underside
// is found on the parts themselves: rays up from under the gun at the table's spot, through each part's collision mesh
// (every gun part is imported with its own mesh as collision, cooked into the asset, so this works in a packaged game
// too), and rays in from each side for its width. Worked out once per assembled gun (UFirstPersonHandsComponent caches
// it), not per frame. The table's spot along the gun (and the user-tuned framing slides on top) still decide where along
// it the hand goes; only how high and how wide come from the parts, and nothing found keeps the table.

namespace
{
	TAutoConsoleVariable<int32> CVarCupFit(TEXT("Looter.Hands.CupFit"), 1,
		TEXT("1: the first-person support hand's cup sits on the underside found on the gun's own parts (any body or magazine). 0: the kind's table only."));

	/** How far under and over the table's underside the rays look (modeled cm): a deep magazine below, a high shell above. */
	constexpr float ReachDown = 30.f;
	constexpr float ReachUp = 12.f;
	/** The rays start this far under the reach, so one never starts inside a part hanging lower still. */
	constexpr float StartUnder = 30.f;
	/**
	 * The rays up across the gun at the spot: this many, this far apart (modeled cm; 6 cm across about the table's middle).
	 * Only at the spot itself along the gun: a trigger guard or a magazine's edge a little behind or ahead would pull the
	 * hand down off the underside it is under.
	 */
	constexpr int32 ProfileRays = 13;
	constexpr float ProfileStep = 0.5f;
	/** A face this much higher across still counts as one floor (a tube's or a rounded bottom's curve; modeled cm). */
	constexpr float CurveAllowance = 1.f;
	/** Spots tried ahead of and behind the table's when nothing is under it (modeled cm), never nearer the grip than GripClear. */
	constexpr float ShiftReach = 8.f;
	constexpr float GripClear = 5.f;
	/** The sides measured this far over the underside, from this far out (modeled cm). */
	constexpr float SideOver = 1.f;
	constexpr float SideFrom = 15.f;

	TAutoConsoleVariable<float> CVarCupFitMinWidth(TEXT("Looter.Hands.CupFit.MinWidth"), 2.6f,
		TEXT("Modeled cm: the narrowest underside the first-person cup rests on. Narrower (the shotgun's and launcher's 1-2.2 cm trigger guards) is a keel the fingers go round; the palm goes up beside it (the AR's loop floor is 3-3.6)."));

	TAutoConsoleVariable<float> CVarCupFitTableTolerance(TEXT("Looter.Hands.CupFit.TableTolerance"), 1.f,
		TEXT("Modeled cm: a face this near the kind's table underside under the cup keeps the table's cup as tuned (the guns the tables were read off: the sniper rests on its magazine, not the stock beside it)."));
	TAutoConsoleVariable<float> CVarCupFitMaxDrop(TEXT("Looter.Hands.CupFit.MaxDrop"), 3.f,
		TEXT("Modeled cm: the furthest the fitted cup goes under the kind's table underside (the LMG's drum hung it out of the frame; the framing is tuned for the table's height)."));

	/** A part's collision mesh and its mesh's space in the model's. */
	struct FCupProbePart
	{
		const UBodySetup* Body = nullptr;
		FTransform ToModel;
	};

	/** The nearest face a ray meets on the parts (the model's space), and where. */
	bool Ray(const TArray<FCupProbePart>& Parts, const FVector& Start, const FVector& Dir, float Length, FVector& OutHit)
	{
		double Best = Length;
		bool bHit = false;
		for (const FCupProbePart& Part : Parts)
		{
			const FVector LocalStart = Part.ToModel.InverseTransformPosition(Start);
			const FVector LocalDelta = Part.ToModel.InverseTransformPosition(Start + Dir * Length) - LocalStart;
			const double LocalLength = LocalDelta.Size();
			if (LocalLength < UE_KINDA_SMALL_NUMBER)
			{
				continue;
			}
			const FVector LocalDir = LocalDelta / LocalLength;
			for (const Chaos::FTriangleMeshImplicitObjectPtr& Mesh : Part.Body->TriMeshGeometries)
			{
				Chaos::FReal Time = 0.0;
				Chaos::FVec3 Position;
				Chaos::FVec3 Normal;
				int32 Face = INDEX_NONE;
				if (Mesh && Mesh->Raycast(LocalStart, LocalDir, LocalLength, 0.0, Time, Position, Normal, Face))
				{
					const double Along = Time / LocalLength * Length;
					if (Along < Best)
					{
						Best = Along;
						OutHit = Part.ToModel.TransformPosition(FVector(Position));
						bHit = true;
					}
				}
			}
		}
		return bHit;
	}
}

FHandsCupFit FirstPersonHandsGrips::FitCup(const UWeaponModelComponent& Model, EWeaponKind Kind)
{
	FHandsCupFit Fit;
	const FGunHands& Hands = For(Kind);
	const FVector Grip = Model.GetGrip();
	if (!Hands.bCup || !Hands.CupShape.bValid || !Model.IsAssembled() || Grip.IsZero())
	{
		return Fit;
	}
	TArray<FCupProbePart> Parts;
	const FTransform ModelWorld = Model.GetComponentTransform();
	for (const UStaticMeshComponent* Part : Model.GetParts())
	{
		UStaticMesh* Mesh = Part ? Part->GetStaticMesh() : nullptr;
		UBodySetup* Body = Mesh ? Mesh->GetBodySetup() : nullptr;
		if (!Body)
		{
			continue;
		}
		// The parts' components have no collision of their own, so nothing else may have made the cooked meshes yet.
		if (!Body->bCreatedPhysicsMeshes)
		{
			Body->CreatePhysicsMeshes();
		}
		if (!Body->TriMeshGeometries.IsEmpty())
		{
			Parts.Add({ Body, Part->GetComponentTransform().GetRelativeTransform(ModelWorld) });
		}
	}
	if (Parts.IsEmpty())
	{
		return Fit;
	}

	// Modeled cm from the grip, as the table has them, to the model's space and back.
	const float Scale = FMath::Max(Model.GetModelScale(), 0.01f);
	auto InModel = [&](const FVector& Offset) { return Grip + Offset * Scale; };
	auto FromGrip = [&](const FVector& InModelSpace) { return (InModelSpace - Grip) / Scale; };
	const FHandleSpec& Table = Hands.CupShape;
	const float TableX = static_cast<float>(Table.Offset.X);
	const float TableBottom = static_cast<float>(Table.Offset.Z) - Table.HalfAcross;

	// The underside across the gun at a spot along it: rays up side by side (ProfileRays, ProfileStep apart, about the
	// table's middle). The palm rests on the lowest face that is at least a palm's narrow width across (MinWidth): a keel
	// narrower than that (the shotgun's and the launcher's trigger guards under the receiver and the tube, 1-2.2 cm; a
	// thin post) is something the fingers go round, and the palm goes up beside it onto the receiver's or the tube's bottom
	// (the tables were read off those). Out: that face's height and the run of rays on it (its middle across, half width).
	const float MinWidth = FMath::Max(CVarCupFitMinWidth.GetValueOnAnyThread(), ProfileStep);
	const float TableTolerance = FMath::Max(CVarCupFitTableTolerance.GetValueOnAnyThread(), 0.f);
	auto Underside = [&](float Along, float& OutBottom, float& OutMiddle, float& OutHalfWidth, bool* OutNearTable = nullptr) -> bool
	{
		float Heights[ProfileRays];
		bool bHits[ProfileRays];
		for (int32 Index = 0; Index < ProfileRays; ++Index)
		{
			const float Across = static_cast<float>(Table.Offset.Y) + (Index - ProfileRays / 2) * ProfileStep;
			FVector Hit;
			bHits[Index] = Ray(Parts, InModel(FVector(Along, Across, TableBottom - ReachDown - StartUnder)), FVector::UpVector,
				(ReachDown + StartUnder + ReachUp) * Scale, Hit);
			Heights[Index] = bHits[Index] ? static_cast<float>(FromGrip(Hit).Z) : 0.f;
			// Below the reach (something hangs lower still): not a face this cup looks at.
			bHits[Index] = bHits[Index] && Heights[Index] >= TableBottom - ReachDown;
			if (OutNearTable && bHits[Index] && FMath::Abs(Heights[Index] - TableBottom) <= TableTolerance)
			{
				*OutNearTable = true;
			}
		}
		TArray<float> Candidates;
		for (int32 Index = 0; Index < ProfileRays; ++Index)
		{
			if (bHits[Index])
			{
				Candidates.Add(Heights[Index]);
			}
		}
		Candidates.Sort();
		for (const float Candidate : Candidates)
		{
			// The widest run of neighbouring rays that met something no higher than this (a gently curved bottom counts).
			int32 BestStart = 0;
			int32 BestRun = 0;
			for (int32 Index = 0, Run = 0; Index < ProfileRays; ++Index)
			{
				Run = bHits[Index] && Heights[Index] <= Candidate + CurveAllowance ? Run + 1 : 0;
				if (Run > BestRun)
				{
					BestRun = Run;
					BestStart = Index - Run + 1;
				}
			}
			if (BestRun * ProfileStep >= MinWidth)
			{
				OutBottom = Candidate;
				OutMiddle = static_cast<float>(Table.Offset.Y) + (BestStart + (BestRun - 1) * 0.5f - ProfileRays / 2) * ProfileStep;
				OutHalfWidth = BestRun * ProfileStep * 0.5f;
				return true;
			}
		}
		return false;
	};

	float Along = TableX;
	float Bottom = 0.f;
	float Middle = 0.f;
	float HalfWidth = 0.f;
	bool bNearTable = false;
	bool bFound = Underside(Along, Bottom, Middle, HalfWidth, &bNearTable);
	// Something under the spot where the table has its underside: a gun like the one the table was read off, whose cup the
	// user's framing was tuned on. The table stands (the sniper's magazine is narrower than a palm, and the fit alone put the
	// hand on the stock beside it).
	if (bNearTable)
	{
		Fit.bFitted = true;
		Fit.Under = FVector(TableX, Table.Offset.Y, TableBottom);
		return Fit;
	}
	// Nothing a palm can rest on under the table's spot: the nearest spot ahead or behind that has (clear of the trigger
	// hand's grip).
	for (float Shift = 1.f; !bFound && Shift <= ShiftReach; Shift += 1.f)
	{
		for (const float Try : { TableX + Shift, TableX - Shift })
		{
			if (!bFound && Try >= GripClear && Underside(Try, Bottom, Middle, HalfWidth))
			{
				Along = Try;
				bFound = true;
			}
		}
	}
	if (!bFound)
	{
		return Fit;
	}
	Fit.bFitted = true;
	Fit.Under = FVector(Along, Middle, Bottom);
	Fit.HalfWidth = HalfWidth;

	// Its true width just over that face, past the rays' span (the LMG's belt box is wider): rays in from each side, kept
	// when they bracket the run found.
	const float SideZ = Bottom + FMath::Min(SideOver, Table.HalfAcross);
	FVector RightHit;
	FVector LeftHit;
	if (Ray(Parts, InModel(FVector(Along, Middle + SideFrom, SideZ)), -FVector::RightVector, SideFrom * 2.f * Scale, RightHit)
		&& Ray(Parts, InModel(FVector(Along, Middle - SideFrom, SideZ)), FVector::RightVector, SideFrom * 2.f * Scale, LeftHit))
	{
		const float Left = static_cast<float>(FromGrip(LeftHit).Y);
		const float Right = static_cast<float>(FromGrip(RightHit).Y);
		if (Left <= Middle && Right >= Middle && Right - Left >= MinWidth && Right - Left < 14.f)
		{
			Fit.Under.Y = (Left + Right) * 0.5f;
			Fit.HalfWidth = (Right - Left) * 0.5f;
		}
	}
	// Not too far under the table's: the framing (Looter.FPHands.<Kind>.Support*, the user's) is tuned for the table's height,
	// and a deep magazine (the LMG's drum, 4.4 cm under its belt box) took the hand out of the frame. The palm then sits a
	// little up into the magazine's bottom rather than out of view.
	Fit.Under.Z = FMath::Max(static_cast<float>(Fit.Under.Z), TableBottom - FMath::Max(CVarCupFitMaxDrop.GetValueOnAnyThread(), 0.f));
	return Fit;
}

uint32 FirstPersonHandsGrips::CupFitSettings()
{
	return HashCombine(HashCombine(GetTypeHash(CVarCupFitMinWidth.GetValueOnAnyThread()), GetTypeHash(CVarCupFitTableTolerance.GetValueOnAnyThread())),
		GetTypeHash(CVarCupFitMaxDrop.GetValueOnAnyThread()));
}


void FirstPersonHandsGrips::FitCupTo(FGunHands& Hands, const FHandsCupFit& Fit)
{
	if (!Fit.bFitted || !Hands.bCup || CVarCupFit.GetValueOnAnyThread() == 0)
	{
		return;
	}
	// The box and the hand move together: the box's bottom onto the underside found, its middle across onto the
	// underside's (the hand keeps its lean to the right of it), along the gun only when the table's spot had nothing.
	FHandleSpec& Shape = Hands.CupShape;
	const FVector Move(Fit.Under.X - Shape.Offset.X, Fit.Under.Y - Shape.Offset.Y, Fit.Under.Z - (Shape.Offset.Z - Shape.HalfAcross));
	Shape.Offset += Move;
	Hands.CupHold += Move;
	if (Fit.HalfWidth > 0.f)
	{
		Shape.HalfThrough = Fit.HalfWidth;
		Shape.Corner = FMath::Min(Shape.Corner, FMath::Min(Shape.HalfThrough, Shape.HalfAcross));
	}
}

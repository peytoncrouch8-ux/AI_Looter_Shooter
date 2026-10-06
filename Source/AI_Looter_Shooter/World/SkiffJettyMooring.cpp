// ASkiffJetty: the moored skiff (where it lies, its gangplank's swing) and its mooring lines.

#include "World/SkiffJetty.h"
#include "World/MinimapSubsystem.h"
#include "Components/SplineMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMeshSocket.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"

namespace
{
	/** The imported plank's way from its hinge when its mesh can't say (Blender's +X): along the actor's -Y. */
	const FVector DefaultGangplankAlong(0.0, -1.0, 0.0);

	/** A slipped line hangs from its bollard: this far out over the deck's edge toward where the skiff was, this far down (cm). */
	constexpr double HangOut = 55.0;
	constexpr double HangDown = 240.0;

	/** Where a let-go line's end comes to hang: over the edge toward where it was tied, under its bollard. */
	FVector HangingEnd(const FVector& Bollard, const FVector& WasTiedAt)
	{
		return Bollard + (WasTiedAt - Bollard).GetSafeNormal2D() * HangOut - FVector(0.0, 0.0, HangDown);
	}
}

// ---------------------------------------------------------------------------
// The skiff
// ---------------------------------------------------------------------------

void ASkiffJetty::MeasureGangplank(FVector& OutAlong, float& OutLength) const
{
	// The plank lies level from its hinge (its origin) to its free end: the way its middle lies, and how far it reaches.
	OutAlong = DefaultGangplankAlong;
	OutLength = 0.f;
	const FBox Box = GangplankMesh ? GangplankMesh->GetBoundingBox() : FBox(ForceInit);
	if (!Box.IsValid)
	{
		return;
	}
	FVector Middle = Box.GetCenter();
	Middle.Z = 0.0;
	if (Middle.Normalize())
	{
		OutAlong = Middle;
	}
	for (int32 Corner = 0; Corner < 8; ++Corner)
	{
		const FVector Point((Corner & 1) ? Box.Max.X : Box.Min.X, (Corner & 2) ? Box.Max.Y : Box.Min.Y, (Corner & 4) ? Box.Max.Z : Box.Min.Z);
		OutLength = FMath::Max(OutLength, static_cast<float>(FVector::DotProduct(Point, OutAlong)));
	}
}

FQuat ASkiffJetty::GetRaiseTurn() const
{
	FVector Along;
	float Length = 0.f;
	MeasureGangplank(Along, Length);
	// About the hinge's line, until the free end points straight up and closes the opening.
	return FQuat::FindBetweenNormals(Along, FVector::UpVector);
}

FTransform ASkiffJetty::ComputeMooring() const
{
	const UStaticMeshSocket* Land = JettyMesh ? JettyMesh->FindSocket(GangplankLandSocket) : nullptr;
	const UStaticMeshSocket* Hinge = SkiffMesh ? SkiffMesh->FindSocket(SkiffGangplankSocket) : nullptr;
	FVector Along;
	float Length = 0.f;
	MeasureGangplank(Along, Length);
	if (!Land || !Hinge || Length <= GangplankOverlap)
	{
		return FallbackMooring;
	}
	// Both models face the same way, so the skiff lies turned as the jetty is; its plank leaves the hinge along the hinge
	// socket's own turn and reaches the deck's edge with GangplankOverlap to spare. The hinge sits on the sill at the
	// deck's height, so the keel comes out under the deck as the art was made.
	const FVector PlankOut = Hinge->RelativeRotation.Quaternion().RotateVector(Along);
	const FVector Origin = Land->RelativeLocation - PlankOut * (Length - GangplankOverlap) - Hinge->RelativeLocation;
	return FTransform(Origin);
}

void ASkiffJetty::SpawnSkiff()
{
	UWorld* World = GetWorld();
	if (!World || !SkiffMesh || Skiff)
	{
		return;
	}
	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.ObjectFlags |= RF_Transient;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AStaticMeshActor* Boat = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), ComputeMooring() * Jetty->GetComponentTransform(), Params);
	if (!Boat)
	{
		return;
	}
	// Its hull is the root, with the Deck socket the ride stands the player on; it moves (the ride, the plank).
	Boat->SetMobility(EComponentMobility::Movable);
	UStaticMeshComponent* Hull = Boat->GetStaticMeshComponent();
	Hull->SetStaticMesh(SkiffMesh);
	Hull->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	Boat->Tags.Add(MinimapTags::Obstacle);

	UStaticMeshComponent* Plank = NewObject<UStaticMeshComponent>(Boat, TEXT("Gangplank"));
	Plank->SetMobility(EComponentMobility::Movable);
	Plank->SetStaticMesh(GangplankMesh);
	Plank->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	Plank->SetupAttachment(Hull, SkiffGangplankSocket);
	Boat->AddInstanceComponent(Plank);
	Plank->RegisterComponent();

	// Fixed to the jetty while moored, so the crosshair on the plank or the hull finds the jetty, the thing to use.
	Boat->AttachToComponent(Jetty, FAttachmentTransformRules::KeepWorldTransform);
	Skiff = Boat;
	Gangplank = Plank;
	GangplankRaiseTurn = GetRaiseTurn();
	ApplyGangplank();
}

// ---------------------------------------------------------------------------
// The mooring lines
// ---------------------------------------------------------------------------

void ASkiffJetty::MakeLines()
{
	Lines.Reset();
	SlipFrom.Reset();
	if (!MooringLineMesh || !Skiff || !Jetty)
	{
		return;
	}
	// The rope's mesh stretched along each line, as thick as a mooring rope whatever the mesh's own girth.
	const FBox MeshBox = MooringLineMesh->GetBoundingBox();
	const double Girth = MeshBox.IsValid ? FMath::Max(MeshBox.GetSize().X, 1.0) : 100.0;
	const FVector2D Thickness(MooringLineThickness / Girth);
	for (int32 Index = 0; Index < MooringLines.Num(); ++Index)
	{
		USplineMeshComponent* Line = NewObject<USplineMeshComponent>(this, FName(*FString::Printf(TEXT("MooringLine%d"), Index + 1)));
		Line->SetMobility(EComponentMobility::Movable);
		Line->SetStaticMesh(MooringLineMesh);
		if (MooringLineMaterial)
		{
			Line->SetMaterial(0, MooringLineMaterial);
		}
		Line->SetForwardAxis(ESplineMeshAxis::Z, /*bUpdateMesh*/ false);
		Line->SetStartScale(Thickness, false);
		Line->SetEndScale(Thickness, false);
		// Thin rope: nothing walks into it, and its shadow is too slight to pay for.
		Line->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
		Line->SetCastShadow(false);
		Line->SetupAttachment(Jetty);
		AddInstanceComponent(Line);
		Line->RegisterComponent();
		Lines.Add(Line);
	}
	SlipFrom.SetNum(Lines.Num());
	bLinesSlipped = false;
	SlipTime = 0.f;
	UpdateLines(0.f);
}

void ASkiffJetty::PlaceLine(USplineMeshComponent& Line, const FVector& Start, const FVector& End, float Sag) const
{
	// A cubic whose end tangents lean down by 4 x Sag sags by Sag in the middle, like a rope hanging between its ends.
	const FVector Span = End - Start;
	const FVector Lean(0.0, 0.0, 4.0 * Sag);
	Line.SetStartAndEnd(Start, Span - Lean, End, Span + Lean, /*bUpdateMesh*/ true);
}

void ASkiffJetty::UpdateLines(float DeltaSeconds)
{
	if (Lines.IsEmpty() || !Jetty)
	{
		return;
	}
	// The lines are the jetty's, drawn in its own space.
	const FTransform JettyToWorld = Jetty->GetComponentTransform();
	const UStaticMeshComponent* Hull = Skiff ? Skiff->GetStaticMeshComponent() : nullptr;
	if (bLinesSlipped)
	{
		SlipTime += DeltaSeconds;
	}
	const float Fall = FMath::Clamp(SlipTime / LineDropSeconds, 0.f, 1.f);
	for (int32 Index = 0; Index < Lines.Num(); ++Index)
	{
		USplineMeshComponent* Line = Lines[Index];
		const FName BollardSocket = MooringLines.IsValidIndex(Index) ? MooringLines[Index].Bollard : NAME_None;
		const FName EndSocket = MooringLines.IsValidIndex(Index) ? MooringLines[Index].SkiffEnd : NAME_None;
		if (!Line || !Jetty->DoesSocketExist(BollardSocket))
		{
			continue;
		}
		const FVector Start = JettyToWorld.InverseTransformPosition(Jetty->GetSocketLocation(BollardSocket));
		if (!bLinesSlipped)
		{
			// Tied: from the bollard to the rope's end on the skiff, wherever the skiff is.
			const bool bTied = Hull && Hull->DoesSocketExist(EndSocket);
			Line->SetVisibility(bTied);
			if (bTied)
			{
				PlaceLine(*Line, Start, JettyToWorld.InverseTransformPosition(Hull->GetSocketLocation(EndSocket)), MooringLineSag);
			}
			continue;
		}
		// Let go: its free end falls, quicker as it goes, and hangs from the bollard over the deck's edge.
		const FVector Hang = HangingEnd(Start, SlipFrom[Index]);
		PlaceLine(*Line, Start, FMath::Lerp(SlipFrom[Index], Hang, static_cast<double>(Fall * Fall)), MooringLineSag * (1.f - Fall));
	}
}

void ASkiffJetty::SlipLines()
{
	if (bLinesSlipped || !Jetty)
	{
		return;
	}
	// Each line falls from where its end was as it let go.
	const FTransform JettyToWorld = Jetty->GetComponentTransform();
	const UStaticMeshComponent* Hull = Skiff ? Skiff->GetStaticMeshComponent() : nullptr;
	for (int32 Index = 0; Index < SlipFrom.Num(); ++Index)
	{
		const FName BollardSocket = MooringLines.IsValidIndex(Index) ? MooringLines[Index].Bollard : NAME_None;
		const FName EndSocket = MooringLines.IsValidIndex(Index) ? MooringLines[Index].SkiffEnd : NAME_None;
		const FVector Bollard = JettyToWorld.InverseTransformPosition(Jetty->GetSocketLocation(BollardSocket));
		SlipFrom[Index] = Hull && Hull->DoesSocketExist(EndSocket)
			? JettyToWorld.InverseTransformPosition(Hull->GetSocketLocation(EndSocket)) : Bollard;
	}
	bLinesSlipped = true;
	SlipTime = 0.f;
	RefreshTick();
}

// ABargeBoarding: her deck kit, built in code in her own frame (BargeRules). The jumboed Tripo hull carries its own decks,
// rails, coaming, superstructure and their collision, so the kit adds only what it hasn't got: the unseen rail walls along
// her decks' drops (nobody climbs her rail and falls), the forecastle's stairs when the model has none, the vents and their
// hoods, the falling third stack and its foot, the work lamp; and the stand-in's deck plates, coaming and rails when the
// hull isn't there. The cover and the furniture are BargeBoardingProps.cpp; the hall and the office below the deck
// BargeBoardingHall.cpp. Collision is plain boxes (hidden); the looks are the PROPS lane's models (BargeLooks), else
// engine shapes in the kit's iron (BargeLooks::KitIron).

#include "World/BargeBoarding.h"
#include "World/BargeLooks.h"
#include "World/BargeRules.h"
#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"

namespace
{
	/** A stair's looks: a tread for each step, each as deep as its share of the run, its top on the ramp's line at its
	 *  middle (the feet on the ramp stay within half a step of it); a stringer along each side. */
	void AddStairLooks(TArray<FTransform>& Out, const BargeRules::FStairRun& Stair, float Width = BargeRules::StairWidth)
	{
		const FVector Run = Stair.To - Stair.From;
		const double Rise = FMath::Abs(Run.Z);
		const double Going = FVector2D(Run.X, Run.Y).Size();
		const int32 Steps = FMath::Max(1, FMath::CeilToInt32(Rise / BargeRules::StairTreadRise));
		const FRotator Yaw(0.f, static_cast<float>(FMath::RadiansToDegrees(FMath::Atan2(Run.Y, Run.X))), 0.f);
		const double Thick = BargeRules::StairTreadThick;
		for (int32 Step = 0; Step < Steps; ++Step)
		{
			const FVector Middle = Stair.From + Run * ((Step + 0.5) / Steps);
			Out.Add(FTransform(Yaw, Middle - FVector(0.0, 0.0, Thick * 0.5), FVector(Going / Steps / 100.0, Width / 100.0, Thick / 100.0)));
		}
		const FRotator Slope = Run.Rotation();
		const FVector Up = Slope.Quaternion().GetUpVector();
		const FVector Side = FVector(-Run.Y, Run.X, 0.0).GetSafeNormal() * (Width + BargeRules::StairStringerThick) * 0.5;
		const FVector Middle = (Stair.From + Stair.To) * 0.5 - Up * (BargeRules::StairStringerDepth * 0.5 - BargeRules::StairStringerProud);
		for (const double Sign : { -1.0, 1.0 })
		{
			Out.Add(FTransform(Slope, Middle + Side * Sign,
				FVector(Run.Size() / 100.0, BargeRules::StairStringerThick / 100.0, BargeRules::StairStringerDepth / 100.0)));
		}
	}
}

UStaticMeshComponent* ABargeBoarding::MakePiece(const TCHAR* Name, UStaticMesh* Mesh, UMaterialInterface* Look, const FVector& Center,
	const FVector& Size, const FRotator& Turn, bool bWalkable)
{
	UStaticMeshComponent* Piece = CreateDefaultSubobject<UStaticMeshComponent>(Name);
	Piece->SetupAttachment(Kit);
	Piece->SetMobility(EComponentMobility::Movable);
	Piece->SetStaticMesh(Mesh);
	if (Look)
	{
		Piece->SetMaterial(0, Look);
	}
	Piece->SetRelativeLocationAndRotation(Center, Turn);
	Piece->SetRelativeScale3D(Size / 100.0);
	Piece->SetCanEverAffectNavigation(false);
	if (bWalkable)
	{
		// World-static, as the ground is: creatures' ground traces and the spawners' probes find it.
		Piece->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
		Piece->SetCollisionObjectType(ECC_WorldStatic);
	}
	else
	{
		Piece->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	return Piece;
}

UBoxComponent* ABargeBoarding::MakeWall(const TCHAR* Name, const FVector& Center, const FVector& Half, const FRotator& Turn)
{
	// A solid wall or floor (the hall's iron): it stops pawns and shots alike; its look is instanced apart.
	UBoxComponent* Box = CreateDefaultSubobject<UBoxComponent>(Name);
	Box->SetupAttachment(Kit);
	Box->SetMobility(EComponentMobility::Movable);
	Box->SetBoxExtent(Half, false);
	Box->SetRelativeLocationAndRotation(Center, Turn);
	Box->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	Box->SetCollisionObjectType(ECC_WorldStatic);
	Box->SetCanEverAffectNavigation(false);
	Box->SetGenerateOverlapEvents(false);
	Box->SetHiddenInGame(true);
	return Box;
}

UBoxComponent* ABargeBoarding::MakeRamp(const TCHAR* Name, const BargeRules::FStairRun& Stair, float Width)
{
	// Its top face through the run's line (its middle a half thickness under it, square to the slope).
	const FVector Run = Stair.To - Stair.From;
	const FRotator Slope = Run.Rotation();
	const FVector Up = Slope.Quaternion().GetUpVector();
	return MakeWall(Name, (Stair.From + Stair.To) * 0.5 - Up * (BargeRules::StairThick * 0.5),
		FVector(Run.Size() * 0.5, Width * 0.5, BargeRules::StairThick * 0.5), Slope);
}

UInstancedStaticMeshComponent* ABargeBoarding::MakeInstanced(const TCHAR* Name, UStaticMesh* Mesh, UMaterialInterface* Look)
{
	UInstancedStaticMeshComponent* Looks = CreateDefaultSubobject<UInstancedStaticMeshComponent>(Name);
	Looks->SetupAttachment(Kit);
	Looks->SetMobility(EComponentMobility::Movable);
	Looks->SetStaticMesh(Mesh);
	if (Look)
	{
		Looks->SetMaterial(0, Look);
	}
	Looks->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Looks->SetCanEverAffectNavigation(false);
	Looks->bReceivesDecals = false;
	return Looks;
}

void ABargeBoarding::BuildKit()
{
	static UStaticMesh* const CubeMesh = BargeLooks::Find<UStaticMesh>(BargeLooks::Cube);
	static UStaticMesh* const CylinderMesh = BargeLooks::Find<UStaticMesh>(BargeLooks::Cylinder);
	static UStaticMesh* const ConeMesh = BargeLooks::Find<UStaticMesh>(BargeLooks::Cone);
	static UStaticMesh* const SphereMesh = BargeLooks::Find<UStaticMesh>(BargeLooks::Sphere);
	static UStaticMesh* const TripoCoal = BargeLooks::Find<UStaticMesh>(BargeLooks::TripoSoulCoal);
	static UStaticMesh* const TripoHose = BargeLooks::Find<UStaticMesh>(BargeLooks::TripoHose);
	static UStaticMesh* const VentMesh = BargeLooks::Find<UStaticMesh>(BargeLooks::DeckVent);
	static UStaticMesh* const HoodMesh = BargeLooks::Find<UStaticMesh>(BargeLooks::FurnaceVent);
	static UStaticMesh* const FireMesh = BargeLooks::Find<UStaticMesh>(BargeLooks::FireTongue);
	static UMaterialInterface* const KitIron = BargeLooks::FindOr<UMaterialInterface>(BargeLooks::KitIron, BargeLooks::Iron);
	static UMaterialInterface* const KitIronDark = BargeLooks::FindOr<UMaterialInterface>(BargeLooks::KitIronDark, BargeLooks::IronBlack);
	static UMaterialInterface* const IronBlack = BargeLooks::Find<UMaterialInterface>(BargeLooks::IronBlack);
	static UMaterialInterface* const Glow = BargeLooks::Find<UMaterialInterface>(BargeLooks::Glow);
	static UMaterialInterface* const Rubber = BargeLooks::Find<UMaterialInterface>(BargeLooks::Rubber);

	bVentModel = VentMesh != nullptr;
	bFireModel = FireMesh != nullptr;
	RailLook = MakeInstanced(TEXT("RailLook"), CubeMesh, KitIronDark);
	StairLook = MakeInstanced(TEXT("StairLook"), CubeMesh, KitIronDark);
	// The vents: the kit's model (its ember bed on the CinderGlow slot: iron while quiet, its own glow in the tell and the
	// burn), else a plate in the kit's dark iron or the glow; their fire the tongue model, else the engine cone.
	VentCold = MakeInstanced(TEXT("VentCold"), bVentModel ? VentMesh : CubeMesh, bVentModel ? nullptr : KitIronDark);
	VentFrame = MakeInstanced(TEXT("VentFrame"), CubeMesh, KitIronDark);
	VentGlow = MakeInstanced(TEXT("VentGlow"), bVentModel ? VentMesh : CubeMesh, bVentModel ? nullptr : Glow);
	VentFire = MakeInstanced(TEXT("VentFire"), bFireModel ? FireMesh : ConeMesh, bFireModel ? nullptr : Glow);
	if (bVentModel)
	{
		BargeLooks::SetSlot(*VentCold, BargeLooks::GlowSlot, IronBlack);
	}
	// The vents' hoods seen from the hall (the model only: there's no stand-in for them), dark or glowing with their vent.
	HoodCold = MakeInstanced(TEXT("HoodCold"), HoodMesh, nullptr);
	HoodGlow = MakeInstanced(TEXT("HoodGlow"), HoodMesh, nullptr);
	if (HoodMesh)
	{
		BargeLooks::SetSlot(*HoodCold, BargeLooks::GlowSlot, IronBlack);
	}
	HallWallLook = MakeInstanced(TEXT("HallWallLook"), CubeMesh, KitIron);
	CatwalkLook = MakeInstanced(TEXT("CatwalkLook"), CubeMesh, KitIronDark);
	HoseSections = MakeInstanced(TEXT("HoseSections"), TripoHose ? TripoHose : CylinderMesh, TripoHose ? nullptr : Rubber);
	SternFire = MakeInstanced(TEXT("SternFire"), bFireModel ? FireMesh : ConeMesh, bFireModel ? nullptr : Glow);
	CoalRain = MakeInstanced(TEXT("CoalRain"), TripoCoal ? TripoCoal : SphereMesh, TripoCoal ? nullptr : Glow);
	// Her hose and the coal hang in the world's frame (laid world-space each frame), the rest in hers.
	for (UInstancedStaticMeshComponent* InWorld : { HoseSections.Get(), CoalRain.Get() })
	{
		InWorld->SetUsingAbsoluteLocation(true);
		InWorld->SetUsingAbsoluteRotation(true);
		InWorld->SetUsingAbsoluteScale(true);
		InWorld->SetCastShadow(false);
	}
	for (UInstancedStaticMeshComponent* Fire : { VentGlow.Get(), VentFire.Get(), SternFire.Get(), HoodGlow.Get() })
	{
		Fire->SetCastShadow(false);
	}

	BuildDeck();
	BuildHall();
	BuildHatchStair();
	BuildCover();
	BuildDressing();

	// The work lamp on the house's fore face over the well deck: shadowless, warm; out as she dies.
	for (int32 Index = 0; Index < static_cast<int32>(UE_ARRAY_COUNT(BargeRules::DeckLampSpots)); ++Index)
	{
		UPointLightComponent* Lamp = CreateDefaultSubobject<UPointLightComponent>(*FString::Printf(TEXT("DeckLamp%d"), Index));
		Lamp->SetupAttachment(Kit);
		Lamp->SetMobility(EComponentMobility::Movable);
		Lamp->SetRelativeLocation(BargeRules::DeckLampSpots[Index]);
		Lamp->SetIntensityUnits(ELightUnits::Candelas);
		Lamp->SetIntensity(BargeRules::DeckLampCandelas);
		Lamp->SetAttenuationRadius(BargeRules::DeckLampReach);
		Lamp->SetLightColor(BargeRules::DeckLampColor);
		Lamp->SetCastShadows(false);
		DeckLamps.Add(Lamp);
	}
	BuildWorkLamps();
}

void ABargeBoarding::BuildDeck()
{
	static UStaticMesh* const CubeMesh = BargeLooks::Find<UStaticMesh>(BargeLooks::Cube);
	static UStaticMesh* const CylinderMesh = BargeLooks::Find<UStaticMesh>(BargeLooks::Cylinder);
	static UStaticMesh* const StackMesh = BargeLooks::Find<UStaticMesh>(BargeLooks::Stack);
	static UStaticMesh* const StackFootMesh = BargeLooks::Find<UStaticMesh>(BargeLooks::StackFoot);
	static UStaticMesh* const HullMesh = BargeLooks::Find<UStaticMesh>(HullPath);
	static UMaterialInterface* const KitIron = BargeLooks::FindOr<UMaterialInterface>(BargeLooks::KitIron, BargeLooks::Iron);
	static UMaterialInterface* const KitIronDark = BargeLooks::FindOr<UMaterialInterface>(BargeLooks::KitIronDark, BargeLooks::IronBlack);

	const bool bHull = HullMesh != nullptr;
	const FBox2D Hole = BargeRules::HatchHole();
	if (!bHull)
	{
		// The stand-in's decks: a plate per zone at its height, cut round the hatch's hole; the coaming round the hole but on
		// the side the stair starts from.
		const float T = BargeRules::PlateThickness;
		int32 PlateIndex = 0;
		for (const BargeRules::FDeckZone& Zone : BargeRules::DeckZones)
		{
			for (const FBox2D& Piece : BargeRules::CutAround(Zone.Box(), Hole))
			{
				PlateParts.Add(MakePiece(*FString::Printf(TEXT("DeckPlate%d"), PlateIndex++), CubeMesh, KitIron,
					FVector(Piece.GetCenter().X, Piece.GetCenter().Y, Zone.Z - T * 0.5), FVector(Piece.GetSize().X, Piece.GetSize().Y, T)));
			}
		}
		const double Rim = BargeRules::HatchCoamingThick;
		const double Up = BargeRules::HatchCoamingUp;
		const double Deck = BargeRules::HatchDeckZ();
		const FVector2D Open = -BargeRules::HatchWay.GetSafeNormal();
		const struct { FVector2D Facing; FBox2D Box; } Rims[] = {
			{ FVector2D(-1.0, 0.0), FBox2D(FVector2D(Hole.Min.X - Rim, Hole.Min.Y - Rim), FVector2D(Hole.Min.X, Hole.Max.Y + Rim)) },
			{ FVector2D(1.0, 0.0), FBox2D(FVector2D(Hole.Max.X, Hole.Min.Y - Rim), FVector2D(Hole.Max.X + Rim, Hole.Max.Y + Rim)) },
			{ FVector2D(0.0, -1.0), FBox2D(FVector2D(Hole.Min.X, Hole.Min.Y - Rim), FVector2D(Hole.Max.X, Hole.Min.Y)) },
			{ FVector2D(0.0, 1.0), FBox2D(FVector2D(Hole.Min.X, Hole.Max.Y), FVector2D(Hole.Max.X, Hole.Max.Y + Rim)) },
		};
		int32 RimIndex = 0;
		for (const auto& Side : Rims)
		{
			if ((Side.Facing - Open).IsNearlyZero(0.01))
			{
				continue;
			}
			MakePiece(*FString::Printf(TEXT("Coaming%d"), RimIndex++), CubeMesh, KitIronDark, FVector(Side.Box.GetCenter(), Deck + Up * 0.5),
				FVector(Side.Box.GetSize(), Up));
		}
	}

	// The rail walls (BargeRules::RailWallLayout): unseen, pawns only (shots and the camera pass over), just outside each
	// zone's drop over her rail so nobody climbs it and falls, open at the berth's gap where the plank lands and the jump is.
	for (const BargeRules::FRailWall& Layout : BargeRules::RailWallLayout())
	{
		const FVector2D Middle = Layout.Footprint.GetCenter();
		const FVector2D Half = Layout.Footprint.GetExtent();
		UBoxComponent* Wall = CreateDefaultSubobject<UBoxComponent>(Layout.Name);
		Wall->SetupAttachment(Kit);
		Wall->SetMobility(EComponentMobility::Movable);
		Wall->SetBoxExtent(FVector(Half.X, Half.Y, BargeRules::RailWallHeight * 0.5), false);
		Wall->SetRelativeLocation(FVector(Middle.X, Middle.Y, Layout.Z + BargeRules::RailWallHeight * 0.5));
		Wall->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Wall->SetCollisionObjectType(ECC_WorldDynamic);
		Wall->SetCollisionResponseToAllChannels(ECR_Ignore);
		Wall->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
		Wall->SetCanEverAffectNavigation(false);
		Wall->SetGenerateOverlapEvents(false);
		Wall->SetHiddenInGame(true);
		RailWalls.Add(Wall);
	}

	// The forecastle's stairs, when the model hasn't its own (their treads: LayStaticLooks).
	if (BargeRules::bKitForecastleStairs || !bHull)
	{
		for (int32 Index = 0; Index < BargeRules::NumForecastleStairs; ++Index)
		{
			MakeRamp(*FString::Printf(TEXT("ForecastleStair%d"), Index), BargeRules::ForecastleStair(Index), BargeRules::ForecastleStairHalfWidth * 2.f);
		}
	}

	// The kit's third stack against the house's fore face, the one that falls in step 7 (the hull's own two stand on the
	// casing): turned about its foot flange's edge on the fall's side (the pivot), so it lands on its side on the deck. The
	// model stands on its foot (pivot at its base, scaled to the rules' radius and height); the stand-in cylinder is centred.
	FallingStackPivot = CreateDefaultSubobject<USceneComponent>(TEXT("FallingStackPivot"));
	FallingStackPivot->SetupAttachment(Kit);
	FallingStackPivot->SetMobility(EComponentMobility::Movable);
	FallingStackPivot->SetRelativeLocation(BargeRules::StackFallPivot());
	FallingStack = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FallingStack"));
	FallingStack->SetupAttachment(FallingStackPivot);
	FallingStack->SetMobility(EComponentMobility::Movable);
	FallingStack->SetStaticMesh(StackMesh ? StackMesh : CylinderMesh);
	const FVector UnderPivot = BargeRules::FallingStackFoot - BargeRules::StackFallPivot();
	if (StackMesh)
	{
		FallingStack->SetRelativeLocation(UnderPivot);
		FallingStack->SetRelativeScale3D(FVector(BargeRules::StackRadius / BargeLooks::StackModelRadius, BargeRules::StackRadius / BargeLooks::StackModelRadius,
			BargeRules::StackHeight / BargeLooks::StackModelHeight));
	}
	else
	{
		FallingStack->SetMaterial(0, KitIronDark);
		FallingStack->SetRelativeLocation(UnderPivot + FVector(0.0, 0.0, BargeRules::StackHeight * 0.5));
		FallingStack->SetRelativeScale3D(FVector(BargeRules::StackRadius * 2.0 / 100.0, BargeRules::StackRadius * 2.0 / 100.0, BargeRules::StackHeight / 100.0));
	}
	FallingStack->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	FallingStack->SetCollisionObjectType(ECC_WorldStatic);
	FallingStack->SetCanEverAffectNavigation(false);
	// Its doubler plate stays on the deck when it falls (sheared bolts round a soot ring): the model only, a look.
	if (StackFootMesh)
	{
		StackFoot = MakePiece(TEXT("StackFoot"), StackFootMesh, nullptr, BargeRules::FallingStackFoot, FVector(100.0), FRotator::ZeroRotator, false);
	}
}

void ABargeBoarding::LayStaticLooks()
{
	if (!RailLook || !StairLook || !HallWallLook || !CatwalkLook || !VentCold || !VentFrame)
	{
		return;
	}
	// The visible rail: a low iron rail in 5 m spans along each rail wall's middle line, over the stand-in box only (the hull
	// has Tripo's: BargeRules::bDrawRails draws it anyway).
	TArray<FTransform> Rails;
	if (!HasHull() || BargeRules::bDrawRails)
	{
		for (const BargeRules::FRailWall& Layout : BargeRules::RailWallLayout())
		{
			const FBox2D& Box = Layout.Footprint;
			const bool bAlongX = Box.GetSize().X >= Box.GetSize().Y;
			const FVector From = bAlongX ? FVector(Box.Min.X, Box.GetCenter().Y, Layout.Z) : FVector(Box.GetCenter().X, Box.Min.Y, Layout.Z);
			const FVector To = bAlongX ? FVector(Box.Max.X, Box.GetCenter().Y, Layout.Z) : FVector(Box.GetCenter().X, Box.Max.Y, Layout.Z);
			const FVector Run = To - From;
			const int32 Spans = FMath::Max(1, FMath::CeilToInt32(Run.Size() / 500.0));
			for (int32 Span = 0; Span < Spans; ++Span)
			{
				const FVector A = From + Run * (static_cast<double>(Span) / Spans);
				const FVector B = From + Run * (static_cast<double>(Span + 1) / Spans);
				Rails.Add(FTransform(FRotator(0.f, static_cast<float>(Run.Rotation().Yaw), 0.f), (A + B) * 0.5 + FVector(0.0, 0.0, BargeRules::RailHeight - 6.0),
					FVector((B - A).Size() / 100.0, 0.1, 0.12)));
				Rails.Add(FTransform(FQuat::Identity, A + FVector(0.0, 0.0, BargeRules::RailHeight * 0.5), FVector(0.08, 0.08, BargeRules::RailHeight / 100.0)));
			}
		}
	}
	RailLook->ClearInstances();
	RailLook->AddInstances(Rails, /*bShouldReturnIndices*/ false);

	// Every stair the kit lays: the forecastle's (when it lays them), the hatch's, the hall's two.
	TArray<FTransform> Stairs;
	if (BargeRules::bKitForecastleStairs || !HasHull())
	{
		for (int32 Index = 0; Index < BargeRules::NumForecastleStairs; ++Index)
		{
			AddStairLooks(Stairs, BargeRules::ForecastleStair(Index), BargeRules::ForecastleStairHalfWidth * 2.f);
		}
	}
	AddStairLooks(Stairs, BargeRules::HatchStair(), BargeRules::HatchStairWidth);
	for (int32 Index = 0; Index < BargeRules::NumFloorStairs; ++Index)
	{
		AddStairLooks(Stairs, BargeRules::FloorStair(Index));
	}
	StairLook->ClearInstances();
	StairLook->AddInstances(Stairs, false);

	LayHallLooks();
	LayWorkLamps();

	// The vents, all cold (the deck's tick lays their glow and fire), their hoods in the hall, and the stand-in grilles'
	// rims and slats over them (the vent model has its own).
	TArray<FTransform> Vents;
	TArray<FTransform> Frames;
	const double Half = BargeRules::VentHalf;
	const double Rim = BargeRules::VentRimWidth;
	const double PlateTop = BargeRules::VentGrilleUp + BargeRules::VentGrilleThick * 0.5;
	TArray<FTransform> Hoods;
	for (int32 Index = 0; Index < BargeRules::NumVents; ++Index)
	{
		const FVector& Spot = BargeRules::VentSpots[Index];
		Vents.Add(VentPlate(Spot));
		if (HasHood(Index))
		{
			Hoods.Add(VentHood(Spot));
		}
		if (bVentModel)
		{
			// The model has its own coaming and louvres.
			continue;
		}
		const FVector RimAt = Spot + FVector(0.0, 0.0, PlateTop + BargeRules::VentRimUp * 0.5);
		for (const double Sign : { -1.0, 1.0 })
		{
			Frames.Add(FTransform(FQuat::Identity, RimAt + FVector(0.0, Sign * (Half - Rim * 0.5), 0.0), FVector(Half / 50.0, Rim / 100.0, BargeRules::VentRimUp / 100.0)));
			Frames.Add(FTransform(FQuat::Identity, RimAt + FVector(Sign * (Half - Rim * 0.5), 0.0, 0.0),
				FVector(Rim / 100.0, (Half - Rim) / 50.0, BargeRules::VentRimUp / 100.0)));
		}
		const double Inside = (Half - Rim) * 2.0;
		for (int32 Slat = 0; Slat < BargeRules::VentSlats; ++Slat)
		{
			const double X = -Inside * 0.5 + Inside * (Slat + 0.5) / BargeRules::VentSlats;
			Frames.Add(FTransform(FQuat::Identity, Spot + FVector(X, 0.0, PlateTop + BargeRules::VentSlatUp * 0.5),
				FVector(BargeRules::VentSlatWidth / 100.0, Inside / 100.0, BargeRules::VentSlatUp / 100.0)));
		}
	}
	VentCold->ClearInstances();
	VentCold->AddInstances(Vents, false);
	VentFrame->ClearInstances();
	VentFrame->AddInstances(Frames, false);
	VentGlow->ClearInstances();
	VentFire->ClearInstances();
	if (HoodCold && HoodGlow && HoodCold->GetStaticMesh())
	{
		HoodCold->ClearInstances();
		HoodCold->AddInstances(Hoods, false);
		HoodGlow->ClearInstances();
	}
	VentGlowMask = 0u;
	VentFireMask = 0u;
}

FTransform ABargeBoarding::VentPlate(const FVector& Spot) const
{
	// The model stands on the deck at its middle (8 cm proud, nothing under the hull's deck); the stand-in's plate is sunk to
	// a centimetre proud.
	if (bVentModel)
	{
		const double Scale = BargeRules::VentHalf / BargeLooks::DeckVentModelHalf;
		return FTransform(FQuat::Identity, Spot, FVector(Scale, Scale, 1.0));
	}
	return FTransform(FQuat::Identity, Spot + FVector(0.0, 0.0, BargeRules::VentGrilleUp),
		FVector(BargeRules::VentHalf / 50.0, BargeRules::VentHalf / 50.0, BargeRules::VentGrilleThick / 100.0));
}

FTransform ABargeBoarding::VentColumn(const FVector& Spot) const
{
	// The column of furnace fire, as wide as the grille and a man and a half tall, its foot on the vent's ember bed.
	const FVector Foot = Spot + FVector(0.0, 0.0, bVentModel ? BargeLooks::DeckVentFireUp : 0.0);
	const double Across = BargeRules::VentColumnRadius / BargeLooks::FireModelRadius;
	return BargeLooks::FireAt(Foot, FVector(Across, Across, BargeRules::VentColumnHeight / BargeLooks::FireModelHeight), bFireModel);
}

FTransform ABargeBoarding::VentHood(const FVector& Spot)
{
	return FTransform(FQuat::Identity, FVector(Spot.X, Spot.Y, BargeRules::HallCeiling - BargeRules::CeilingThickness));
}

bool ABargeBoarding::HasHood(int32 Vent)
{
	// A hood under the vent only where it stays inside the hall's round (the rest would cut its wall).
	if (Vent < 0 || Vent >= BargeRules::NumVents)
	{
		return false;
	}
	const FVector& Spot = BargeRules::VentSpots[Vent];
	return FVector2D(Spot.X - BargeRules::HallCenter.X, Spot.Y - BargeRules::HallCenter.Y).Size() + BargeLooks::HoodHalfDiagonal <= BargeRules::HallRadius;
}

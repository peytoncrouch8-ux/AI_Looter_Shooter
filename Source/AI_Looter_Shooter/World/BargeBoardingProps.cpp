// ABargeBoarding: the props she's dressed with (the REUSE lane's picks, BargeLooks): the deck's cover (barrels in ones, twos
// and threes, crates, sandbag walls, a pump cart, the forecastle's winch), the stoker's corner on the hall's catwalk (his
// desk, safe and radio), coal by the stokeholds and the furnace, the hall's wall lanterns and their lights, the deck's work
// lamps on their brackets. Each mesh at its own size, laid from its
// bounds (the tests read the real ones), with its own simple collision or an unseen block of its bounds; the kit's block
// stands in where a mesh is missing (cover and furniture only: missing coal and lanterns are left out).

#include "World/BargeBoarding.h"
#include "World/BargeLooks.h"
#include "World/BargeRules.h"
#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "PhysicsEngine/BodySetup.h"

namespace
{
	using BargeRules::ECover;

	bool IsBarrels(ECover Kind)
	{
		return Kind == ECover::Barrel || Kind == ECover::BarrelPair || Kind == ECover::BarrelTrio;
	}

	/** Where a cluster's barrels stand round its middle (its own frame), for barrels this far across: side by side, or three
	 *  in a triangle. */
	TArray<FVector2D> BarrelPlaces(ECover Kind, double Across)
	{
		const double R = (Across + BargeRules::BarrelGap) * 0.5;
		switch (Kind)
		{
		case ECover::BarrelPair: return { FVector2D(0.0, -R), FVector2D(0.0, R) };
		case ECover::BarrelTrio:
		{
			const double Root3 = FMath::Sqrt(3.0);
			return { FVector2D(-R / Root3, -R), FVector2D(-R / Root3, R), FVector2D(2.0 * R / Root3, 0.0) };
		}
		default: return { FVector2D::ZeroVector };
		}
	}

	bool HasSimpleCollision(const UStaticMesh* Mesh)
	{
		const UBodySetup* Body = Mesh ? Mesh->GetBodySetup() : nullptr;
		return Body && Body->AggGeom.GetElementCount() > 0;
	}
}

UStaticMeshComponent* ABargeBoarding::MakeProp(const TCHAR* Name, UStaticMesh* Mesh, UMaterialInterface* Look, const FVector& Foot,
	float Yaw, const FVector& FallbackHalf, bool bSolid)
{
	static UStaticMesh* const CubeMesh = BargeLooks::Find<UStaticMesh>(BargeLooks::Cube);
	const FRotator Turn(0.f, Yaw, 0.f);
	if (!Mesh)
	{
		return MakePiece(Name, CubeMesh, Look, Foot + FVector(0.0, 0.0, FallbackHalf.Z), FallbackHalf * 2.0, Turn, bSolid);
	}
	// Its bounds' bottom on the floor and their middle over the spot, whatever its pivot.
	const FBox Local = Mesh->GetBoundingBox();
	const FVector Base(Local.GetCenter().X, Local.GetCenter().Y, Local.Min.Z);
	UStaticMeshComponent* Prop = CreateDefaultSubobject<UStaticMeshComponent>(Name);
	Prop->SetupAttachment(Kit);
	Prop->SetMobility(EComponentMobility::Movable);
	Prop->SetStaticMesh(Mesh);
	Prop->SetRelativeLocationAndRotation(Foot - Turn.RotateVector(Base), Turn);
	Prop->SetCanEverAffectNavigation(false);
	if (bSolid && HasSimpleCollision(Mesh))
	{
		Prop->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
		Prop->SetCollisionObjectType(ECC_WorldStatic);
	}
	else
	{
		Prop->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		if (bSolid)
		{
			// No collision of its own: an unseen block of its bounds stops boots and bullets.
			MakeWall(*FString::Printf(TEXT("%sBlock"), Name), Foot + FVector(0.0, 0.0, Local.GetExtent().Z), Local.GetExtent(), Turn);
		}
	}
	return Prop;
}

void ABargeBoarding::BuildCover()
{
	static UStaticMesh* const BarrelMesh = BargeLooks::Find<UStaticMesh>(BargeLooks::Barrel);
	static UStaticMesh* const CrateMesh = BargeLooks::Find<UStaticMesh>(BargeLooks::Crate);
	static UStaticMesh* const SandbagsMesh = BargeLooks::Find<UStaticMesh>(BargeLooks::Sandbags);
	static UStaticMesh* const HoseCartMesh = BargeLooks::Find<UStaticMesh>(BargeLooks::HoseCart);
	static UStaticMesh* const WinchMesh = BargeLooks::Find<UStaticMesh>(BargeLooks::Winch);
	static UMaterialInterface* const KitIron = BargeLooks::FindOr<UMaterialInterface>(BargeLooks::KitIron, BargeLooks::Iron);

	for (int32 Index = 0; Index < BargeRules::NumCoverPieces; ++Index)
	{
		const BargeRules::FCoverPiece& Piece = BargeRules::CoverPieces[Index];
		UStaticMesh* const Mesh = IsBarrels(Piece.Kind) ? BarrelMesh : Piece.Kind == ECover::Crate ? CrateMesh
			: Piece.Kind == ECover::Sandbags ? SandbagsMesh : Piece.Kind == ECover::HoseCart ? HoseCartMesh : WinchMesh;
		const FVector Foot(Piece.X, Piece.Y, Piece.Z);
		const FString Base = FString::Printf(TEXT("Cover%d"), Index);
		if (IsBarrels(Piece.Kind))
		{
			// Each barrel of the cluster on its own, a little turned from the one before.
			const FVector One = BargeRules::CoverFallbackHalf(ECover::Barrel);
			const FVector Half = Mesh ? Mesh->GetBoundingBox().GetExtent() : One;
			const FRotator Turn(0.f, Piece.Yaw, 0.f);
			int32 Part = 0;
			for (const FVector2D& Place : BarrelPlaces(Piece.Kind, 2.0 * FMath::Max(Half.X, Half.Y)))
			{
				CoverParts.Add(MakeProp(*FString::Printf(TEXT("%s_%d"), *Base, Part), Mesh, KitIron, Foot + Turn.RotateVector(FVector(Place, 0.0)),
					Piece.Yaw + Part * BargeRules::BarrelTurn, One, true));
				++Part;
			}
			continue;
		}
		const FVector Fallback = BargeRules::CoverFallbackHalf(Piece.Kind);
		CoverParts.Add(MakeProp(*Base, Mesh, KitIron, Foot, Piece.Yaw, Fallback, true));
		// A sandbag wall lower than a crouch gets a second course on it (turned end for end: the bags don't repeat).
		const double Height = Mesh ? Mesh->GetBoundingBox().GetSize().Z : Fallback.Z * 2.0;
		if (Piece.Kind == ECover::Sandbags && Height < BargeRules::SandbagsStackUnder)
		{
			CoverParts.Add(MakeProp(*(Base + TEXT("_Up")), Mesh, KitIron, Foot + FVector(0.0, 0.0, Height), Piece.Yaw + 180.f, Fallback, true));
		}
	}
}

void ABargeBoarding::BuildDressing()
{
	static UStaticMesh* const CubeMesh = BargeLooks::Find<UStaticMesh>(BargeLooks::Cube);
	static UStaticMesh* const DeskMesh = BargeLooks::Find<UStaticMesh>(BargeLooks::Desk);
	static UStaticMesh* const SafeMesh = BargeLooks::Find<UStaticMesh>(BargeLooks::Safe);
	static UStaticMesh* const SackMesh = BargeLooks::Find<UStaticMesh>(BargeLooks::CoalSack);
	static UStaticMesh* const CoalMesh = BargeLooks::Find<UStaticMesh>(BargeLooks::TripoSoulCoal);
	static UStaticMesh* const HallLanternMesh = BargeLooks::Find<UStaticMesh>(BargeLooks::HallLantern);
	static UStaticMesh* const OfficeLanternMesh = BargeLooks::Find<UStaticMesh>(BargeLooks::OfficeLantern);
	static UStaticMesh* const RadioMesh = BargeLooks::Find<UStaticMesh>(BargeLooks::Radio);
	static UMaterialInterface* const Wood = BargeLooks::Find<UMaterialInterface>(BargeLooks::Wood);
	static UMaterialInterface* const KitIronDark = BargeLooks::FindOr<UMaterialInterface>(BargeLooks::KitIronDark, BargeLooks::IronBlack);

	// The stoker's corner on the catwalk. The desk: the bench at its own size against the hall's wall, its long side along
	// the ring; the radio on its corner (Ned's call comes out of it).
	const FVector DeskFallback = BargeRules::DeskFallbackHalf;
	const bool bLongX = DeskMesh && DeskMesh->GetBoundingBox().GetSize().X > DeskMesh->GetBoundingBox().GetSize().Y;
	const float DeskYaw = BargeRules::DeskYaw() + (bLongX ? 90.f : 0.f);
	const FVector DeskExtent = DeskMesh ? DeskMesh->GetBoundingBox().GetExtent() : DeskFallback;
	const double DeskDepth = bLongX ? DeskExtent.Y : DeskExtent.X;
	const FVector DeskFoot = BargeRules::DeskSpot(static_cast<float>(DeskDepth));
	Desk = MakeProp(TEXT("OfficeDesk"), DeskMesh, Wood, DeskFoot, DeskYaw, DeskFallback, true);
	const FVector OnDesk = DeskFoot + FVector(0.0, 0.0, DeskExtent.Z * 2.0);
	// The kit's oxide wireless standing on its feet, its dials into the hall; else a dark iron block.
	const FVector RadioMiddle = BargeRules::RadioSpot(OnDesk);
	if (RadioMesh)
	{
		Radio = MakePiece(TEXT("Radio"), RadioMesh, nullptr, RadioMiddle - FVector(0.0, 0.0, BargeRules::RadioSize.Z * 0.5), FVector(100.0),
			FRotator(0.f, BargeRules::RadioYaw(), 0.f), false);
	}
	else
	{
		Radio = MakePiece(TEXT("Radio"), CubeMesh, KitIronDark, RadioMiddle, BargeRules::RadioSize, FRotator(0.f, BargeRules::RadioYaw(), 0.f), false);
	}
	// The safe against the wall between the desk and the hatch stair's foot.
	MakeProp(TEXT("OfficeSafe"), SafeMesh, KitIronDark, BargeRules::SafeSpot(), BargeRules::OfficeAngle + BargeRules::SafeAngle + 180.f,
		BargeRules::SafeFallbackHalf, true);

	// Coal: two sacks against the wall either side of each stokehold's door, soul coal round the furnace's back (looks only).
	int32 CoalIndex = 0;
	if (SackMesh)
	{
		for (const float Angle : BargeRules::StokeholdAngles)
		{
			const FVector Out = BargeRules::HallPoint(Angle, 1.f) - BargeRules::HallPoint(Angle, 0.f);
			const FVector Across(-Out.Y, Out.X, 0.0);
			for (const double Sign : { -1.0, 1.0 })
			{
				const FVector At = BargeRules::HallPoint(Angle, BargeRules::HallRadius - BargeRules::CoalSackIn)
					+ Across * Sign * (BargeRules::AlcoveHalfWidth() + BargeRules::CoalSackBeside);
				MakeProp(*FString::Printf(TEXT("CoalSack%d"), CoalIndex), SackMesh, nullptr, At, BargeRules::OfficeAngle + Angle + CoalIndex * BargeRules::CoalTurn,
					FVector::ZeroVector, false);
				++CoalIndex;
			}
		}
	}
	if (CoalMesh)
	{
		const float Ring = (BargeRules::FurnaceRadius + BargeRules::GrateInner) * 0.5f;
		for (const float Angle : BargeRules::SoulCoalAngles)
		{
			MakeProp(*FString::Printf(TEXT("SoulCoal%d"), CoalIndex), CoalMesh, nullptr, BargeRules::HallPoint(Angle, Ring), CoalIndex * BargeRules::CoalTurn,
				FVector::ZeroVector, false);
			++CoalIndex;
		}
	}

	// The lanterns, their backs on the hall's wall, each with its light (shadowless, warm): four under the catwalk, one over
	// the stoker's desk. Without the mesh the light alone.
	const auto Lantern = [this](const TCHAR* Name, UStaticMesh* Mesh, const FVector& OnWall, const FVector& Inward)
	{
		const float Yaw = static_cast<float>(Inward.Rotation().Yaw);
		const FVector Half = Mesh ? Mesh->GetBoundingBox().GetExtent() : FVector::ZeroVector;
		const FVector Middle = OnWall + Inward * Half.X;
		if (Mesh)
		{
			MakeProp(Name, Mesh, nullptr, Middle - FVector(0.0, 0.0, Half.Z), Yaw, Half, false)->SetCastShadow(false);
		}
		UPointLightComponent* Light = CreateDefaultSubobject<UPointLightComponent>(*FString::Printf(TEXT("%sLight"), Name));
		Light->SetupAttachment(Kit);
		Light->SetMobility(EComponentMobility::Movable);
		Light->SetRelativeLocation(Middle + Inward * Half.X);
		Light->SetIntensityUnits(ELightUnits::Candelas);
		Light->SetIntensity(BargeRules::LanternCandelas);
		Light->SetAttenuationRadius(BargeRules::LanternReach);
		Light->SetLightColor(BargeRules::LanternColor);
		Light->SetCastShadows(false);
		Lanterns.Add(Light);
	};
	int32 LanternIndex = 0;
	for (const float Angle : BargeRules::HallLanternAngles)
	{
		const FVector OnWall = BargeRules::HallPoint(Angle, BargeRules::HallRadius, BargeRules::HallLanternUp);
		const FVector Inward = (BargeRules::HallPoint(Angle, 0.f) - BargeRules::HallPoint(Angle, 1.f)).GetSafeNormal();
		Lantern(*FString::Printf(TEXT("HallLantern%d"), LanternIndex++), HallLanternMesh, OnWall, Inward);
	}
	const FVector OverDeskIn = (BargeRules::HallPoint(BargeRules::DeskAngle, 0.f) - BargeRules::HallPoint(BargeRules::DeskAngle, 1.f)).GetSafeNormal();
	Lantern(TEXT("OfficeLantern"), OfficeLanternMesh, BargeRules::OfficeLanternSpot(), OverDeskIn);
}

void ABargeBoarding::BuildWorkLamps()
{
	static UStaticMesh* const CubeMesh = BargeLooks::Find<UStaticMesh>(BargeLooks::Cube);
	static UStaticMesh* const LanternMesh = BargeLooks::Find<UStaticMesh>(BargeLooks::HallLantern);
	static UMaterialInterface* const KitIronDark = BargeLooks::FindOr<UMaterialInterface>(BargeLooks::KitIronDark, BargeLooks::IronBlack);

	// The brackets' iron and the lanterns, instanced (nine lamps: two draws); looks only, no shadows.
	WorkLampIron = MakeInstanced(TEXT("WorkLampIron"), CubeMesh, KitIronDark);
	WorkLampLanterns = MakeInstanced(TEXT("WorkLampLanterns"), LanternMesh, nullptr);
	for (UInstancedStaticMeshComponent* Looks : { WorkLampIron.Get(), WorkLampLanterns.Get() })
	{
		Looks->SetCastShadow(false);
	}
	// Each light in its lantern's middle (without the mesh, a hand off its seat).
	const double Middle = LanternMesh ? LanternMesh->GetBoundingBox().GetSize().Z * 0.5 : 10.0;
	for (int32 Index = 0; Index < BargeRules::NumWorkLamps; ++Index)
	{
		const BargeRules::FWorkLamp& Fitting = BargeRules::WorkLamps[Index];
		UPointLightComponent* Lamp = CreateDefaultSubobject<UPointLightComponent>(*FString::Printf(TEXT("WorkLamp%d"), Index));
		Lamp->SetupAttachment(Kit);
		Lamp->SetMobility(EComponentMobility::Movable);
		Lamp->SetRelativeLocation(BargeRules::WorkLampSeat(Fitting) + FVector(0.0, 0.0, BargeRules::WorkLampHangs(Fitting) ? -Middle : Middle));
		Lamp->SetIntensityUnits(ELightUnits::Candelas);
		Lamp->SetIntensity(BargeRules::WorkLampCandelas);
		Lamp->SetAttenuationRadius(BargeRules::WorkLampReach);
		Lamp->SetLightColor(BargeRules::DeckLampColor);
		Lamp->SetCastShadows(false);
		DeckLamps.Add(Lamp);
	}
}

void ABargeBoarding::LayWorkLamps()
{
	if (!WorkLampIron || !WorkLampLanterns)
	{
		return;
	}
	const double T = BargeRules::WorkLampIron;
	const double Arm = BargeRules::WorkLampArm;
	const FVector Sunk(0.0, 0.0, BargeRules::WorkLampSunk);
	// A length of iron (the kit's cube) between two points on her, T square.
	TArray<FTransform> Iron;
	const auto Bar = [&Iron, T](const FVector& From, const FVector& To)
	{
		const FVector Run = To - From;
		Iron.Add(FTransform(FRotationMatrix::MakeFromX(Run.GetSafeNormal()).ToQuat(), (From + To) * 0.5, FVector(Run.Size(), T, T) / 100.0));
	};
	TArray<FTransform> Hung;
	const UStaticMesh* LanternMesh = WorkLampLanterns->GetStaticMesh();
	const FBox Local = LanternMesh ? LanternMesh->GetBoundingBox() : FBox(ForceInit);
	for (const BargeRules::FWorkLamp& Lamp : BargeRules::WorkLamps)
	{
		const FVector Out(Lamp.Out, 0.0);
		const FVector Seat = BargeRules::WorkLampSeat(Lamp);
		const FRotator Turn(0.f, static_cast<float>(Out.Rotation().Yaw), 0.f);
		switch (Lamp.Mount)
		{
		case BargeRules::ELampMount::Post:
		{
			// Up out of the wall's top, then the arm along the post's top.
			const FVector Top = Lamp.Fixed + FVector(0.0, 0.0, BargeRules::WorkLampPostUp);
			Bar(Lamp.Fixed - Sunk, Top);
			Bar(Top - FVector(0.0, 0.0, T * 0.5), Top + Out * Arm - FVector(0.0, 0.0, T * 0.5));
			break;
		}
		case BargeRules::ELampMount::Drop:
			Bar(Lamp.Fixed + Sunk, Seat);
			break;
		default:
		{
			// The foot plate under the lantern, its top the seat (a hand wider than the lantern, sunk into the rim).
			const FVector Plate = LanternMesh ? Local.GetSize() + FVector(6.0) : FVector(30.0);
			Iron.Add(FTransform(Turn, Seat - FVector(0.0, 0.0, BargeRules::WorkLampSunk * 0.5),
				FVector(Plate.X, Plate.Y, BargeRules::WorkLampSunk) / 100.0));
			break;
		}
		}
		if (LanternMesh)
		{
			// Standing on its seat or hung with its top there, facing the way the lamp faces.
			const FVector Base(Local.GetCenter().X, Local.GetCenter().Y, Local.Min.Z);
			const FVector Foot = BargeRules::WorkLampHangs(Lamp) ? Seat - FVector(0.0, 0.0, Local.GetSize().Z) : Seat;
			Hung.Add(FTransform(Turn, Foot - Turn.RotateVector(Base)));
		}
	}
	WorkLampIron->ClearInstances();
	WorkLampIron->AddInstances(Iron, /*bShouldReturnIndices*/ false);
	WorkLampLanterns->ClearInstances();
	WorkLampLanterns->AddInstances(Hung, /*bShouldReturnIndices*/ false);
}

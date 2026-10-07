// ATrain: its running gear: the wheel sets on the bodies' axles, the coupling rods on the drivers' cranks, and turning
// them as the train moves.

#include "World/Train.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"

namespace
{
	/** The bodies' axles are Axle_1 to Axle_<n>, front to back (Train.py); none has more than this. */
	constexpr int32 MostAxles = 8;

	/** The locomotive's coupling rods' sockets, one each side. */
	const FName RodSockets[] = { FName(TEXT("Rod_L")), FName(TEXT("Rod_R")) };
}

FQuat ATrain::WheelSpin(float Distance, float Radius)
{
	// About the axle (Y), positive as it rolls out along +X: the tread's lowest point moves back as fast as the axle goes on.
	return FQuat(FVector::YAxisVector, Radius > KINDA_SMALL_NUMBER ? Distance / Radius : 0.f);
}

void ATrain::LayOutRunningGear()
{
	Axles.Reset();
	RodPins.Reset();
	// An axle stands its tread's radius over the rail head: nearer the drivers' height it takes the drivers.
	const float DriverHeight = RailTop + 0.5f * (DriverRadius + CarriageRadius);
	UStaticMeshComponent* Cars[NumCars] = { HearseCar, PassengerCar, Locomotive };
	for (UStaticMeshComponent* Body : Cars)
	{
		if (!Body->GetStaticMesh())
		{
			continue;
		}
		const FTransform CarFrame = Body->GetRelativeTransform();
		const int32 FirstOfCar = Axles.Num();
		for (int32 Number = 1; Number <= MostAxles; ++Number)
		{
			const FName Socket(*FString::Printf(TEXT("Axle_%d"), Number));
			if (!Body->DoesSocketExist(Socket))
			{
				break;
			}
			const FTransform OnBody = Body->GetSocketTransform(Socket, RTS_Component);
			const FTransform At = OnBody * CarFrame;
			FAxle& Axle = Axles.AddDefaulted_GetRef();
			Axle.Middle = At.GetLocation();
			Axle.Rest = At.GetRotation();
			Axle.bDriver = OnBody.GetLocation().Z >= DriverHeight;
			Axle.Radius = Axle.bDriver ? DriverRadius : CarriageRadius;
		}
		// Each rod's pin rides the crank of the driver nearest it along the train, on the rod's own side.
		for (const FName RodSocket : RodSockets)
		{
			if (!Body->DoesSocketExist(RodSocket))
			{
				continue;
			}
			const FTransform At = Body->GetSocketTransform(RodSocket, RTS_Component) * CarFrame;
			int32 Nearest = INDEX_NONE;
			for (int32 Index = FirstOfCar; Index < Axles.Num(); ++Index)
			{
				if (Axles[Index].bDriver && (Nearest == INDEX_NONE
					|| FMath::Abs(Axles[Index].Middle.X - At.GetLocation().X) < FMath::Abs(Axles[Nearest].Middle.X - At.GetLocation().X)))
				{
					Nearest = Index;
				}
			}
			if (Nearest == INDEX_NONE)
			{
				continue;
			}
			FRodPin& Pin = RodPins.AddDefaulted_GetRef();
			Pin.Axle = FVector(Axles[Nearest].Middle.X, At.GetLocation().Y, Axles[Nearest].Middle.Z);
			Pin.RestOffset = At.GetLocation() - Pin.Axle;
			Pin.Rest = At.GetRotation();
		}
	}

	DriverWheels->ClearInstances();
	CarriageWheels->ClearInstances();
	Rods->ClearInstances();
	for (const FAxle& Axle : Axles)
	{
		(Axle.bDriver ? DriverWheels : CarriageWheels)->AddInstance(FTransform(Axle.Rest, Axle.Middle));
	}
	for (const FRodPin& Pin : RodPins)
	{
		Rods->AddInstance(FTransform(Pin.Rest, Pin.Axle + Pin.RestOffset));
	}
	TurnRunningGear(Travel);
}

void ATrain::TurnRunningGear(float Distance)
{
	TArray<FTransform> Drivers;
	TArray<FTransform> Carriages;
	TArray<FTransform> Pins;
	for (const FAxle& Axle : Axles)
	{
		// Turned about its own axle, from its rest turn.
		(Axle.bDriver ? Drivers : Carriages).Add(FTransform(WheelSpin(Distance, Axle.Radius) * Axle.Rest, Axle.Middle));
	}
	// A rod doesn't turn: its pins go round with the drivers' cranks, and it with them.
	const FQuat DriverSpin = WheelSpin(Distance, DriverRadius);
	for (const FRodPin& Pin : RodPins)
	{
		Pins.Add(FTransform(Pin.Rest, Pin.Axle + DriverSpin.RotateVector(Pin.RestOffset)));
	}
	if (Drivers.Num() == DriverWheels->GetInstanceCount() && !Drivers.IsEmpty())
	{
		DriverWheels->BatchUpdateInstancesTransforms(0, Drivers, /*bWorldSpace*/ false, /*bMarkRenderStateDirty*/ true, /*bTeleport*/ true);
	}
	if (Carriages.Num() == CarriageWheels->GetInstanceCount() && !Carriages.IsEmpty())
	{
		CarriageWheels->BatchUpdateInstancesTransforms(0, Carriages, false, true, true);
	}
	if (Pins.Num() == Rods->GetInstanceCount() && !Pins.IsEmpty())
	{
		Rods->BatchUpdateInstancesTransforms(0, Pins, false, true, true);
	}
}

int32 ATrain::GetNumDriverAxles() const
{
	return Axles.FilterByPredicate([](const FAxle& Axle) { return Axle.bDriver; }).Num();
}

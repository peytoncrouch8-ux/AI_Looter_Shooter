#include "World/PCGGroundFitFilter.h"
#include "World/WorldQueries.h"
#include "Data/PCGBasePointData.h"
#include "Data/PCGSpatialData.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "PCGContext.h"
#include "PCGGraphExecutionStateInterface.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PCGGroundFitFilter)

namespace
{
	/** How far above and below a sample point the ground is looked for (cm). Farther down counts as no ground: a drop. */
	constexpr double SearchReach = 300.0;

	class FPCGGroundFitFilterElement : public IPCGElement
	{
	protected:
		virtual bool ExecuteInternal(FPCGContext* Context) const override;
		// It traces the world, and the answer changes with the world.
		virtual bool CanExecuteOnlyOnMainThread(FPCGContext* Context) const override { return true; }
		virtual bool IsCacheable(const UPCGSettings* InSettings) const override { return false; }
		virtual bool SupportsBasePointDataInputs(FPCGContext* InContext) const override { return true; }
	};

	bool FPCGGroundFitFilterElement::ExecuteInternal(FPCGContext* Context) const
	{
		const UPCGGroundFitFilterSettings* Settings = Context->GetInputSettings<UPCGGroundFitFilterSettings>();
		check(Settings);
		UWorld* World = Context->ExecutionSource.IsValid() ? Context->ExecutionSource->GetExecutionState().GetWorld() : nullptr;
		if (!World)
		{
			PCGE_LOG_C(Error, GraphAndLog, Context, NSLOCTEXT("LooterPCG", "GroundFitNoWorld", "No world to find the ground in."));
			return true;
		}

		const FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(World, TEXT("PCGGroundFit"));
		const FName GroundTag = Settings->GroundTag;
		auto GroundHeight = [World, &Params, GroundTag](const FVector& Point) -> TOptional<double>
		{
			TArray<FHitResult> Hits;
			World->LineTraceMultiByObjectType(Hits, Point + FVector(0.0, 0.0, SearchReach), Point - FVector(0.0, 0.0, SearchReach),
				FCollisionObjectQueryParams(ECC_WorldStatic), Params);
			for (const FHitResult& Hit : Hits)
			{
				const AActor* Actor = Hit.GetActor();
				if (Actor && Actor->ActorHasTag(GroundTag))
				{
					return Hit.ImpactPoint.Z;
				}
			}
			return {};
		};

		for (const FPCGTaggedData& Input : Context->InputData.GetInputsByPin(PCGPinConstants::DefaultInputLabel))
		{
			FPCGTaggedData& Output = Context->OutputData.TaggedData.Add_GetRef(Input);
			const UPCGSpatialData* Spatial = Cast<UPCGSpatialData>(Input.Data);
			const UPCGBasePointData* Source = Spatial ? Spatial->ToBasePointData(Context) : nullptr;
			if (!Source)
			{
				PCGE_LOG_C(Error, GraphAndLog, Context, NSLOCTEXT("LooterPCG", "GroundFitNoPoints", "The input isn't points."));
				continue;
			}

			UPCGBasePointData* Fitted = FPCGContext::NewPointData_AnyThread(Context);
			FPCGInitializeFromDataParams InitializeParams(Source);
			InitializeParams.bInheritSpatialData = false; // some points are dropped
			Fitted->InitializeFromDataWithParams(InitializeParams);
			Fitted->SetNumPoints(Source->GetNumPoints(), /*bInitializeValues*/ false);
			Fitted->AllocateProperties(Source->GetAllocatedProperties() | EPCGPointNativeProperties::Transform);
			Fitted->CopyUnallocatedPropertiesFrom(Source);

			const FConstPCGPointValueRanges Read(Source);
			FPCGPointValueRanges Write(Fitted, /*bAllocate*/ false);
			int32 Kept = 0;
			for (int32 Index = 0; Index < Source->GetNumPoints(); ++Index)
			{
				const TOptional<double> Sink = LooterGroundFit::SinkToFit(Read.TransformRange[Index], Settings->Radius, Settings->MaxSink, GroundHeight);
				if (!Sink)
				{
					continue;
				}
				Write.SetFromValueRanges(Kept, Read, Index);
				Write.TransformRange[Kept].AddToTranslation(FVector(0.0, 0.0, -*Sink));
				++Kept;
			}
			Fitted->SetNumPoints(Kept);
			Output.Data = Fitted;
		}
		return true;
	}
}

FPCGElementPtr UPCGGroundFitFilterSettings::CreateElement() const
{
	return MakeShared<FPCGGroundFitFilterElement>();
}

TOptional<double> LooterGroundFit::SinkToFit(const FTransform& Patch, double Radius, double MaxSink,
	TFunctionRef<TOptional<double>(const FVector&)> GroundHeight)
{
	// Around the edge, where a rigid patch leaves the ground first, and a ring halfway in.
	double Sink = 0.0;
	for (const double Fraction : { 0.95, 0.5 })
	{
		const int32 Samples = Fraction > 0.9 ? 12 : 6;
		for (int32 Sample = 0; Sample < Samples; ++Sample)
		{
			const double Angle = UE_TWO_PI * Sample / Samples;
			const FVector Point = Patch.TransformPosition(FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.0) * Radius * Fraction);
			const TOptional<double> Ground = GroundHeight(Point);
			if (!Ground)
			{
				return {};
			}
			Sink = FMath::Max(Sink, Point.Z - *Ground);
		}
	}
	if (Sink > MaxSink)
	{
		return {};
	}
	return Sink;
}

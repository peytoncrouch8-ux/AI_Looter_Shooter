// ASinkIntake: its hoses. From the choke on it draws the barge's hose itself (Landfall's baked one hidden): in Landfall's
// own shape, bucking, swinging across the floor, then limp; and the second hose let down onto Larkspur High's air shaft.

#include "World/SinkIntake.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Math/RotationMatrix.h"

namespace
{
	/** Points along a hose to lay its sections between: about one a section's length, at least this many, at most that. */
	constexpr int32 FewestPoints = 8;
	constexpr int32 MostPoints = 64;
	/** A section's joints close with this much overlap. */
	constexpr double SectionOverlap = 1.03;
}

FVector ASinkIntake::SwingSide() const
{
	// Square to the hose's run from the drum to the mouth: across the floor, the way its blast throws it.
	const FVector Run = (HoseBottom - HoseTop).GetSafeNormal2D();
	FVector Side(-Run.Y, Run.X, 0.0);
	if (Side.IsNearlyZero())
	{
		Side = GetActorRightVector();
	}
	return Side;
}

FIntakeHoseShape ASinkIntake::GetHoseShape() const
{
	const FIntakeHoseShape Rest = SinkIntakeRules::RestShape(HoseTop, HoseC1, HoseC2, HoseBottom);
	const FVector RestEnd = LimpRest.IsNearlyZero() ? HoseBottom + SwingSide() * 800.0 : LimpRest;
	const FIntakeHoseShape Limp = SinkIntakeRules::LimpShape(HoseTop, RestEnd);
	switch (State)
	{
	case EIntakeState::Swinging:
		if (Clock < BuckSeconds)
		{
			return SinkIntakeRules::BuckShape(Rest, SwingSide(), Clock, BuckSeconds);
		}
		return SinkIntakeRules::SwingShape(Rest, Limp, SwingSide(), static_cast<float>(RestEnd.Z), Clock - BuckSeconds, SwingSeconds);
	case EIntakeState::Limp:
		return Limp;
	default:
		return Rest;
	}
}

void ASinkIntake::LayHose(UInstancedStaticMeshComponent* Into, const FIntakeHoseShape& Shape, float Share) const
{
	if (!Into)
	{
		return;
	}
	TArray<FTransform> Transforms;
	const float Upto = FMath::Clamp(Share, 0.f, 1.f);
	if (Upto > 0.f && !HoseTop.IsNearlyZero())
	{
		// About a section a point along its rough length, each section stretched to its span.
		double Rough = 0.0;
		FVector Last = Shape.At(0.f);
		for (int32 Step = 1; Step <= 24; ++Step)
		{
			const FVector Next = Shape.At(Upto * Step / 24.f);
			Rough += FVector::Dist(Last, Next);
			Last = Next;
		}
		const double SectionLength = FMath::Max(static_cast<double>(HoseNativeLength) * HoseGirthScale, 50.0);
		const int32 Count = FMath::Clamp(FMath::CeilToInt(Rough / SectionLength), FewestPoints, MostPoints);
		FVector From = Shape.At(0.f);
		for (int32 Index = 1; Index <= Count; ++Index)
		{
			const FVector To = Shape.At(Upto * Index / Count);
			const FVector Span = To - From;
			const double Length = Span.Size();
			if (Length > 1.0)
			{
				// The section's axis is its Z, its pivot its middle (the Tripo section's and the engine cylinder's alike).
				Transforms.Add(FTransform(FRotationMatrix::MakeFromZ(Span / Length).ToQuat(), (From + To) * 0.5,
					FVector(HoseGirthScale, HoseGirthScale, Length * SectionOverlap / FMath::Max(static_cast<double>(HoseNativeLength), 1.0))));
			}
			From = To;
		}
	}
	if (Into->GetInstanceCount() != Transforms.Num())
	{
		Into->ClearInstances();
		if (!Transforms.IsEmpty())
		{
			Into->AddInstances(Transforms, /*bShouldReturnIndices*/ false, /*bWorldSpace*/ true);
		}
	}
	else if (!Transforms.IsEmpty())
	{
		Into->BatchUpdateInstancesTransforms(0, Transforms, /*bWorldSpace*/ true, /*bMarkRenderStateDirty*/ true, /*bTeleport*/ true);
	}
}

void ASinkIntake::PlaceHoses()
{
	const bool bGone = State == EIntakeState::Gone;
	// Its own hose only from the choke on: before, Landfall's baked one is the hose.
	LayHose(Sections, GetHoseShape(), !bGone && DrawsHose() ? 1.f : 0.f);
	const bool bSecond = !bGone && !SecondHoseFrom.IsNearlyZero() && !SecondHoseTo.IsNearlyZero();
	LayHose(SecondSections, SinkIntakeRules::SecondShape(SecondHoseFrom, SecondHoseTo), bSecond ? SecondShare : 0.f);
}

void ASinkIntake::RefreshSecondHose()
{
	const bool bWanted = AnyHolds(SecondHoseWhen) && !AnyHolds(SecondHoseGoneWhen) && State != EIntakeState::Gone;
	if (!bWanted)
	{
		SecondShare = 0.f;
		bSecondDropping = false;
		return;
	}
	if (SecondShare >= 1.f || bSecondDropping)
	{
		return;
	}
	// Its story just turned to it with the player there (Main 5 at the rim): let down before their eyes. A load past it
	// finds it hanging already.
	if (HasActorBegunPlay() && GetWorld() && GetWorld()->GetTimeSeconds() > 1.0)
	{
		bSecondDropping = true;
		SecondShare = 0.f;
		LooterSound::PlayAt(this, LooterSoundCue::GrappleFire, SecondHoseFrom, 1.2f, 0.5f);
		LooterSound::PlayAt(this, LooterSoundCue::ShipLurch, SecondHoseFrom, 0.8f, 0.7f);
		SetActorTickEnabled(true);
	}
	else
	{
		SecondShare = 1.f;
	}
}

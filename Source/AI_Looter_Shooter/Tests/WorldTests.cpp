#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "World/PCGGroundFitFilter.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGroundFitTest, "Looter.World.GroundFit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGroundFitTest::RunTest(const FString& Parameters)
{
	using LooterGroundFit::SinkToFit;
	constexpr double Radius = 170.0;
	constexpr double MaxSink = 30.0;

	// Flat ground: a patch lying on it stays where it is.
	auto Flat = [](const FVector&) -> TOptional<double> { return 50.0; };
	const TOptional<double> OnFlat = SinkToFit(FTransform(FVector(0.0, 0.0, 50.0)), Radius, MaxSink, Flat);
	TestTrue(TEXT("Lies on flat ground"), OnFlat.IsSet() && FMath::IsNearlyZero(*OnFlat, 1e-6));

	// A 20 degree slope, with the patch tilted to it (as the meadow's raycast leaves it): it lies on the slope.
	const double Slope = FMath::Tan(FMath::DegreesToRadians(20.0));
	auto Sloped = [Slope](const FVector& Point) -> TOptional<double> { return Point.X * Slope; };
	const FTransform Tilted(FQuat(FVector::YAxisVector, FMath::DegreesToRadians(-20.0)), FVector::ZeroVector);
	const TOptional<double> OnSlope = SinkToFit(Tilted, Radius, MaxSink, Sloped);
	TestTrue(TEXT("Lies on a slope it's tilted to"), OnSlope.IsSet() && FMath::IsNearlyZero(*OnSlope, 0.01));

	// A gentle bump: the edge floats, so the patch goes down until it doesn't.
	auto Bump = [](const FVector& Point) -> TOptional<double> { return -0.0006 * (Point.X * Point.X + Point.Y * Point.Y); };
	const TOptional<double> OnBump = SinkToFit(FTransform::Identity, Radius, MaxSink, Bump);
	TestTrue(TEXT("Pressed into a bump"), OnBump.IsSet());
	TestEqual(TEXT("...by as much as its edge floated"), OnBump.Get(0.0), 0.0006 * FMath::Square(Radius * 0.95), 0.01);

	// Scaled up, the patch's edge is farther out, and floats more over the same bump: past MaxSink it's dropped.
	TestFalse(TEXT("A big patch on the bump would sink too far"),
		SinkToFit(FTransform(FQuat::Identity, FVector::ZeroVector, FVector(1.5)), Radius, MaxSink, Bump).IsSet());

	// An edge: the ground drops away a meter past x = 100 (a cliff top), or isn't there at all (the island's rim).
	auto Step = [](const FVector& Point) -> TOptional<double> { return Point.X > 100.0 ? -100.0 : 0.0; };
	auto Rim = [](const FVector& Point) -> TOptional<double> { return Point.X > 100.0 ? TOptional<double>() : TOptional<double>(0.0); };
	TestFalse(TEXT("Dropped at a cliff top's edge"), SinkToFit(FTransform::Identity, Radius, MaxSink, Step).IsSet());
	TestFalse(TEXT("Dropped at the island's rim"), SinkToFit(FTransform::Identity, Radius, MaxSink, Rim).IsSet());
	TestTrue(TEXT("Kept a patch clear of the edge"), SinkToFit(FTransform(FVector(-100.0, 0.0, 0.0)), Radius, MaxSink, Step).IsSet());
	return true;
}

#endif

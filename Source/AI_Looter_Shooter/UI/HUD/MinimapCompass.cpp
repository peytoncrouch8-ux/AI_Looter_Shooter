#include "UI/HUD/MinimapCompass.h"

const TCHAR* MinimapCompass::Letter(EPoint Point)
{
	switch (Point)
	{
	case EPoint::East:
		return TEXT("E");
	case EPoint::South:
		return TEXT("S");
	case EPoint::West:
		return TEXT("W");
	default:
		return TEXT("N");
	}
}

float MinimapCompass::WorldYaw(EPoint Point)
{
	return 90.f * static_cast<float>(Point);
}

float MinimapCompass::BezelDegrees(EPoint Point, float ViewYaw)
{
	// A direction at world yaw W shows (W - ViewYaw) clockwise from up when the map is turned to put ViewYaw up.
	float Degrees = FMath::Fmod(WorldYaw(Point) - ViewYaw, 360.f);
	if (Degrees < 0.f)
	{
		Degrees += 360.f;
	}
	return Degrees;
}

FVector2D MinimapCompass::BezelOffset(EPoint Point, float ViewYaw, float Radius)
{
	const double Angle = FMath::DegreesToRadians(BezelDegrees(Point, ViewYaw));
	return FVector2D(FMath::Sin(Angle), -FMath::Cos(Angle)) * Radius;
}

FVector2D MinimapCompass::EdgeMidpoint(EPoint Point, const FVector2D& FramePosition, const FVector2D& FrameSize)
{
	const FVector2D Middle = FramePosition + FrameSize * 0.5;
	switch (Point)
	{
	case EPoint::East:
		return FVector2D(FramePosition.X + FrameSize.X, Middle.Y);
	case EPoint::South:
		return FVector2D(Middle.X, FramePosition.Y + FrameSize.Y);
	case EPoint::West:
		return FVector2D(FramePosition.X, Middle.Y);
	default:
		return FVector2D(Middle.X, FramePosition.Y);
	}
}

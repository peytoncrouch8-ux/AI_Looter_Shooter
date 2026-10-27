// The new game's intro: where its cameras go. The Rest's shots in Lvl_RansomsRest's layout (Art/Levels/RansomsRest/
// layout_computed.json placements, views.json), the grave framed from its own spots, and the planet framed from the menu's
// view; all eased, nothing cut mid-shot.

#include "Story/NewGameIntro.h"
#include "AI_Looter_Shooter.h"
#include "Core/LooterMenuGameMode.h"
#include "World/WorldQueries.h"
#include "Engine/DirectionalLight.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"

namespace
{
	/** Height the shots' ground probe starts above and ends below an eye (cm): past any roof or ridge in the valley. */
	constexpr float ProbeSpan = 30000.f;

	/** Points along a move's eye path probed for the ground (both ends among them). */
	constexpr int32 ProbeSteps = 6;

	// --- The planet shot (fractions of the planet's radius, or of the menu camera's distance from it) ---

	/** The first move pulls in to this share of the menu's distance, centring the planet, never nearer than MinFirst radii. */
	constexpr float FirstDistance = 0.85f;
	constexpr float MinFirst = 2.2f;
	/** Then on toward the dusk line: this share of the menu's distance, never nearer than MinLast radii (the limb stays round). */
	constexpr float LastDistance = 0.6f;
	constexpr float MinLast = 1.75f;
	/** How far the dusk point sits into the day side (the sun's share of its direction): the low sun, not the night. */
	constexpr float DuskDaySide = 0.2f;
	/** The end view's direction: this share the menu's, the rest the dusk point's (the planet still reads as a globe). */
	constexpr float LastViewShare = 0.55f;
	/** The end view looks at a point this far from the middle toward the dusk point (radii): the lit limb fills the frame. */
	constexpr float LastLookOut = 0.55f;
	/** The lens narrows to this share by the end: a slow push, not a zoom. */
	constexpr float LastFovShare = 0.9f;
	/** With no planet to frame: a turn to the right (the menu leaves the planet in the right 60%) and a narrowing lens. */
	constexpr float BlindTurnDegrees = 7.f;
	constexpr float BlindFovShare = 0.62f;

	float Clamp01(float Value)
	{
		return FMath::Clamp(Value, 0.f, 1.f);
	}

	FRotator Look(const FVector& From, const FVector& To)
	{
		FRotator View = (To - From).Rotation();
		View.Roll = 0.f;
		return View;
	}

	FIntroMove Move(const TCHAR* Name, float Start, float Duration, const FVector& FromEye, const FVector& ToEye, const FVector& FromLook,
		const FVector& ToLook, float FromFov, float ToFov)
	{
		FIntroMove Shot;
		Shot.Name = Name;
		Shot.Start = Start;
		Shot.Duration = Duration;
		Shot.FromEye = FromEye;
		Shot.ToEye = ToEye;
		Shot.FromLook = FromLook;
		Shot.ToLook = ToLook;
		Shot.FromFov = FromFov;
		Shot.ToFov = ToFov;
		return Shot;
	}
}

float NewGameIntro::Ease(float Alpha)
{
	const float A = Clamp01(Alpha);
	return A * A * A * (A * (A * 6.f - 15.f) + 10.f);
}

FIntroView NewGameIntro::ViewAlong(const FIntroMove& Shot, float Alpha)
{
	const float A = Ease(Alpha);
	FIntroView View;
	View.Location = FMath::Lerp(Shot.FromEye, Shot.ToEye, A);
	View.Rotation = Look(View.Location, FMath::Lerp(Shot.FromLook, Shot.ToLook, A));
	View.Fov = FMath::Lerp(Shot.FromFov, Shot.ToFov, A);
	return View;
}

TArray<FIntroMove> NewGameIntro::RestMoves(bool bWestward)
{
	// Layout cm (X north, Y east; Z the world's): the lookout deck on Ransom's Point (-8600,-10000) at 20.1 m, the chapel
	// (6600,-1900) at 7.5 m, Boot Hill (3200,0), the farmhouse (-4000,-5800), the family plot's graves (-3520,-8750 and
	// -8480) at 3.6 m, the burial deck (1200,-15000) on the Rim; the sun low in the west-south-west (azimuth 247.5).
	TArray<FIntroMove> Moves;
	float At = 0.f;
	if (bWestward)
	{
		// Westward: out over the canyon from Gravewind Point's deck into the low sun, the plains running empty to it.
		Moves.Add(Move(TEXT("Westward"), At, PlanetSeconds, FVector(1200.0, -15700.0, 800.0), FVector(900.0, -18200.0, 1700.0),
			FVector(-11200.0, -46000.0, -2800.0), FVector(-9500.0, -46000.0, -1200.0), 70.f, 62.f));
		At += PlanetSeconds;
	}
	// Over the Rim: from over the canyon behind the Point, rising, the lookout's deck passing below, the whole valley opening.
	Moves.Add(Move(TEXT("OverTheRim"), At, 15.f, FVector(-10200.0, -11600.0, 2900.0), FVector(-9400.0, -9600.0, 7600.0),
		FVector(-6000.0, -6500.0, 900.0), FVector(1500.0, 1500.0, 0.0), 72.f, 66.f));
	// The chapel: a slow push from Boot Hill's side on the dark chapel.
	Moves.Add(Move(TEXT("Chapel"), At + 15.f, 12.f, FVector(2500.0, 700.0, 1550.0), FVector(3800.0, -500.0, 1200.0),
		FVector(6600.0, -1700.0, 1250.0), FVector(6600.0, -1900.0, 1350.0), 60.f, 52.f));
	// Home: past the farmhouse, turning onto the family plot under its dead oak.
	Moves.Add(Move(TEXT("Home"), At + 27.f, 10.f, FVector(-2100.0, -6000.0, 1500.0), FVector(-2500.0, -7400.0, 1050.0),
		FVector(-4000.0, -5800.0, 750.0), FVector(-3550.0, -8600.0, 450.0), 66.f, 60.f));
	// The grave: low toward the fresh one and the frosted one beside it (GraveMove replaces it when the grave is found).
	Moves.Add(Move(TEXT("Grave"), At + 37.f, RestSeconds - 37.f, FVector(-2300.0, -8700.0, 700.0), FVector(-2950.0, -8680.0, 520.0),
		FVector(-3520.0, -8650.0, 430.0), FVector(-3520.0, -8700.0, 420.0), 58.f, 50.f));
	return Moves;
}

FIntroMove NewGameIntro::GraveMove(const FVector& LookAt, const FVector& Foot, float Start, float Duration)
{
	// Along the grave from past its foot toward the headboard, a little to one side so the Keeper's grave shows beside it;
	// down to just over a standing eye at the end, looking at the board's face.
	FVector Along = Foot - LookAt;
	Along.Z = 0.0;
	if (!Along.Normalize())
	{
		Along = FVector::ForwardVector;
	}
	const FVector Side = FVector::CrossProduct(FVector::UpVector, Along);
	const FVector Board = LookAt + FVector(0.0, 0.0, 10.0);
	return Move(TEXT("Grave"), Start, Duration,
		LookAt + Along * 950.0 + Side * 180.0 + FVector(0.0, 0.0, 300.0),
		LookAt + Along * 420.0 + Side * 60.0 + FVector(0.0, 0.0, 150.0),
		Board + FVector(0.0, 0.0, -40.0), Board, 58.f, 50.f);
}

float NewGameIntro::ClearanceLift(const UWorld* World, const FIntroMove& Shot)
{
	// One lift for the whole move, measured before it plays: per-frame lifts would pop over every roof.
	if (!World)
	{
		return 0.f;
	}
	const FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(World, TEXT("IntroClearance"));
	float Lift = 0.f;
	for (int32 Step = 0; Step <= ProbeSteps; ++Step)
	{
		const FVector Eye = FMath::Lerp(Shot.FromEye, Shot.ToEye, static_cast<double>(Step) / ProbeSteps);
		FHitResult Hit;
		if (World->LineTraceSingleByObjectType(Hit, Eye + FVector(0.0, 0.0, ProbeSpan), Eye - FVector(0.0, 0.0, ProbeSpan),
			FCollisionObjectQueryParams(ECC_WorldStatic), Params))
		{
			Lift = FMath::Max(Lift, static_cast<float>(Hit.ImpactPoint.Z + MinClearance - Eye.Z));
		}
	}
	return Lift;
}

FIntroView NewGameIntro::PlanetView(const FIntroView& Base, const FPlanetFrame& Planet, float Time)
{
	const float First = Ease((Time - MenuFadeSeconds) / (PlanetTurnAt - MenuFadeSeconds));
	const float Last = Ease((Time - PlanetTurnAt) / (PlanetSeconds - PlanetTurnAt));
	if (!Planet.bFound || Planet.Radius <= 1.f)
	{
		// Nothing to frame: the menu's own view turns a little toward the planet's side and the lens narrows.
		const float All = Ease((Time - MenuFadeSeconds) / (PlanetSeconds - MenuFadeSeconds));
		FIntroView View = Base;
		View.Rotation.Yaw += BlindTurnDegrees * All;
		View.Fov = Base.Fov * FMath::Lerp(1.f, BlindFovShare, All);
		return View;
	}

	const FVector Center = Planet.Center;
	const double Radius = Planet.Radius;
	FVector Out = Base.Location - Center;
	double Distance = Out.Size();
	if (Distance < Radius * 1.05)
	{
		// The menu's camera inside the planet (a framing we don't know): start from outside, along the view.
		Out = -Base.Rotation.Vector();
		Distance = Radius * 3.0;
	}
	Out.Normalize();

	// The dusk line: the great circle square to the sun; its point nearest the viewer, a little into the day side.
	FVector ToSun = Planet.ToSun.GetSafeNormal();
	if (ToSun.IsNearlyZero())
	{
		ToSun = FPlanetFrame().ToSun;
	}
	FVector Dusk = Out - ToSun * FVector::DotProduct(Out, ToSun);
	if (!Dusk.Normalize())
	{
		// The sun straight behind or ahead: any line square to it will do.
		Dusk = FVector::CrossProduct(ToSun, FVector::UpVector);
		if (!Dusk.Normalize())
		{
			Dusk = FVector::CrossProduct(ToSun, FVector::ForwardVector).GetSafeNormal();
		}
	}
	const FVector DuskPoint = (Dusk * (1.0 - DuskDaySide) + ToSun * DuskDaySide).GetSafeNormal();

	const double FirstDist = FMath::Max(Distance * FirstDistance, Radius * MinFirst);
	const FVector EndDir = (Out * LastViewShare + DuskPoint * (1.0 - LastViewShare)).GetSafeNormal();
	const double EndDist = FMath::Max(Distance * LastDistance, Radius * MinLast);
	const FVector StartLook = Base.Location + Base.Rotation.Vector() * Distance;
	const FVector EndLook = Center + DuskPoint * Radius * LastLookOut;

	FIntroView View;
	FVector Dir = Out;
	double Dist = FMath::Lerp(Distance, FirstDist, static_cast<double>(First));
	FVector Aim = FMath::Lerp(StartLook, Center, static_cast<double>(First));
	View.Fov = Base.Fov;
	if (Time > PlanetTurnAt)
	{
		// Round the planet, not through it: the direction turns on a great circle while the distance closes.
		const FQuat Turn = FQuat::FindBetweenNormals(Out, EndDir);
		Dir = FQuat::Slerp(FQuat::Identity, Turn, Last).RotateVector(Out);
		Dist = FMath::Lerp(FirstDist, EndDist, static_cast<double>(Last));
		Aim = FMath::Lerp(Center, EndLook, static_cast<double>(Last));
		View.Fov = FMath::Lerp(Base.Fov, Base.Fov * LastFovShare, Last);
	}
	View.Location = Center + Dir * Dist;
	View.Rotation = Time <= MenuFadeSeconds ? Base.Rotation : Look(View.Location, Aim);
	return View;
}

FPlanetFrame NewGameIntro::FindPlanet(const UWorld* World)
{
	FPlanetFrame Frame;
	if (!World)
	{
		return Frame;
	}
	// The tagged planet; else the smallest actor named for it (its cloud and air shells are a little bigger, a sky far
	// bigger: the smallest is the ground).
	const AActor* Best = nullptr;
	FVector BestOrigin = FVector::ZeroVector;
	double BestRadius = 0.0;
	bool bBestTagged = false;
	for (TActorIterator<AActor> It(const_cast<UWorld*>(World)); It; ++It)
	{
		const AActor* Actor = *It;
		const bool bTagged = Actor->ActorHasTag(PlanetTag());
		const FString Name = Actor->GetName();
		if (!bTagged && !Name.Contains(TEXT("Teropa")) && !Name.Contains(TEXT("Planet")))
		{
			continue;
		}
		FVector Origin;
		FVector Extent;
		Actor->GetActorBounds(/*bOnlyCollidingComponents*/ false, Origin, Extent);
		const double Radius = Extent.GetMax();
		if (Radius <= 100.0 || (bBestTagged && !bTagged))
		{
			continue;
		}
		if (!Best || (bTagged && !bBestTagged) || Radius < BestRadius)
		{
			Best = Actor;
			BestOrigin = Origin;
			BestRadius = Radius;
			bBestTagged = bTagged;
		}
	}
	if (Best)
	{
		Frame.Center = BestOrigin;
		Frame.Radius = BestRadius;
		Frame.bFound = true;
	}
	for (TActorIterator<ADirectionalLight> It(const_cast<UWorld*>(World)); It; ++It)
	{
		// The light runs along the actor's forward: the sun is behind it.
		Frame.ToSun = -It->GetActorForwardVector();
		break;
	}
	return Frame;
}

bool NewGameIntro::IsPlanetWorld(const UWorld* World)
{
	if (!World || !ALooterMenuGameMode::IsMenuWorld(World))
	{
		return false;
	}
	return World->GetMapName().Contains(TEXT("Lvl_MenuTeropa")) || FindPlanet(World).bFound;
}

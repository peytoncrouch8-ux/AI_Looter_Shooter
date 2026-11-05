// ALamplighterShip: her moving pieces in play: the Great Lantern's lean, the gangway shown and her starboard rail opened for
// it, her deck for the riders, the crew bracing as she's hit.

#include "World/LamplighterShip.h"
#include "AI_Looter_Shooter.h"
#include "Story/StoryCondition.h"
#include "Story/WardenCrewMember.h"
#include "World/LandfallMarker.h"
#include "World/LanternFlame.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "World/LamplighterShipKit.h"

using namespace LamplighterShipKit;

void ALamplighterShip::LeanLantern()
{
	if (!LanternFlame)
	{
		return;
	}
	AActor* Target = LanternTargetActor.Get();
	if (!Target && !LanternTarget.IsNone())
	{
		LanternSearchWait -= 0.5f;
		if (LanternSearchWait <= 0.f)
		{
			// Looked for now and then only: a level walk is a few thousand actors.
			LanternSearchWait = 5.f;
			for (TActorIterator<AActor> It(GetWorld()); It && !Target; ++It)
			{
				if (It->ActorHasTag(LanternTarget))
				{
					Target = *It;
				}
			}
			LanternTargetActor = Target;
		}
	}
	if (Target)
	{
		const FVector Toward = Target->GetActorLocation() - LanternFlame->GetActorLocation();
		LanternFlame->Bearing = FMath::RadiansToDegrees(FMath::Atan2(Toward.Y, Toward.X));
	}
	LanternFlame->Lean();
}

void ALamplighterShip::ShowGangway(bool bShown)
{
	const float SideSign = GangwaySide >= 0.f ? 1.f : -1.f;
	// The rail's length the gangway stands in opens; every other one is shut (a jump moved it, or she's flying again).
	const int32 Spans = static_cast<int32>(UE_ARRAY_COUNT(SideSpans));
	const float At = FMath::Clamp(GangwayAt, static_cast<float>(SideSpans[0].X) + 50.f, static_cast<float>(SideSpans[Spans - 1].Y) - 50.f);
	int32 GangwaySpan = 0;
	for (int32 Span = 0; Span < Spans; ++Span)
	{
		if (At >= SideSpans[Span].X && At <= SideSpans[Span].Y)
		{
			GangwaySpan = Span;
		}
	}
	// The side rails are stretches along her bulwark, each tagged with its side and span (BuildRails).
	const FName Opening = SideRailTag(SideSign, GangwaySpan);
	for (UBoxComponent* Wall : Rails)
	{
		if (Wall && IsSideRail(*Wall))
		{
			const bool bOpening = bShown && Wall->ComponentHasTag(Opening);
			Wall->SetCollisionEnabled(bOpening ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryAndPhysics);
		}
	}
	if (!Gangway)
	{
		return;
	}
	// The plank reaches along its own -Y from its hinge (Skiff.py), so on the starboard side it's turned about; tipped down
	// to the ground from her deck.
	const float Reach = PlankLength * GangwayScale;
	const float Down = FMath::RadiansToDegrees(FMath::Asin(FMath::Clamp(DeckTop / Reach, 0.f, 1.f)));
	const FQuat Facing(FVector::UpVector, SideSign < 0.f ? 0.0 : UE_PI);
	Gangway->SetRelativeLocationAndRotation(FVector(At, SideSign * RailOut, DeckTop), Facing * FQuat(FVector::XAxisVector, FMath::DegreesToRadians(Down)));
	Gangway->SetRelativeScale3D(FVector(GangwayScale));
	Gangway->SetVisibility(bShown);
	if (bShown)
	{
		Gangway->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
		Gangway->SetCollisionObjectType(ECC_WorldStatic);
	}
	else
	{
		Gangway->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void ALamplighterShip::OpenStarboardRail(bool bOpen)
{
	// Main 6's berth (PilotSortie.cpp): the stretches of her starboard rail round the plank's hinge, by their tag (BuildRails
	// tags every stretch reaching within PlankOpenHalf of PlankHingeX; their count and order follow her bulwark line).
	for (UBoxComponent* Wall : Rails)
	{
		if (Wall && Wall->ComponentHasTag(PlankRailTag()))
		{
			Wall->SetCollisionEnabled(bOpen ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryAndPhysics);
		}
	}
}

UPrimitiveComponent* ALamplighterShip::GetDeck() const
{
	return Deck.Get();
}

FVector ALamplighterShip::DeckPoint(const FVector& Local) const
{
	return GetActorTransform().TransformPositionNoScale(FVector(Local.X, Local.Y, DeckTop + Local.Z));
}

void ALamplighterShip::BraceCrew(float Seconds)
{
	for (AWardenCrewMember* Member : Crew)
	{
		if (IsValid(Member))
		{
			Member->Brace(Seconds);
		}
	}
}

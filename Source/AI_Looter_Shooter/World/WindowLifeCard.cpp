// AWindowLifeCard: the card in front of a lit window's glass that shows a shadow crossing or a curtain twitching.

#include "World/WindowLifeCard.h"
#include "AI_Looter_Shooter.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Math/RotationMatrix.h"
#include "Misc/PackageName.h"

namespace
{
	/** The engine's plane: 100 x 100 cm in its XY, facing +Z. */
	const TCHAR* PlanePath = TEXT("/Engine/BasicShapes/Plane.Plane");

	/** The material's Kind: a shadow or a curtain. */
	constexpr float ShadowKind = 0.f;
	constexpr float CurtainKind = 1.f;

	/** A material not made yet is looked for again only this often (s), not on every look round. */
	constexpr double RetrySeconds = 10.0;

	/** Whether an asset is in this checkout, without the warning LoadObject gives for one that isn't. */
	bool IsMade(const TCHAR* ObjectPath)
	{
		return FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(FString(ObjectPath)));
	}
}

const TCHAR* AWindowLifeCard::MaterialPath = TEXT("/Game/Art/Materials/Masters/M_WindowLife.M_WindowLife");
const FName AWindowLifeCard::KindParameter(TEXT("Kind"));
const FName AWindowLifeCard::SlideParameter(TEXT("Slide"));
const FName AWindowLifeCard::RiseParameter(TEXT("Rise"));
const FName AWindowLifeCard::SwingParameter(TEXT("Swing"));
const FName AWindowLifeCard::SideParameter(TEXT("Side"));
const FName AWindowLifeCard::StrengthParameter(TEXT("Strength"));
const FName AWindowLifeCard::ColorParameter(TEXT("Color"));
const FName AWindowLifeCard::WidthParameter(TEXT("CardWidth"));
const FName AWindowLifeCard::HeightParameter(TEXT("CardHeight"));

AWindowLifeCard::AWindowLifeCard()
{
	// Ticks only while an event plays.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	SetCanBeDamaged(false);

	Card = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Card"));
	SetRootComponent(Card);
	Card->SetMobility(EComponentMobility::Movable);
	Card->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	Card->SetGenerateOverlapEvents(false);
	Card->SetCanEverAffectNavigation(false);
	Card->SetCastShadow(false);
	Card->bReceivesDecals = false;
	Card->SetVisibility(false);
}

void AWindowLifeCard::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Advance(DeltaSeconds);
}

bool AWindowLifeCard::Prepare()
{
	if (Look)
	{
		return true;
	}
	// Made on first need, not in the constructor: the material may be built (by its script) after the editor started.
	const UWorld* World = GetWorld();
	const double Now = World ? World->GetRealTimeSeconds() : 0.0;
	if (LastTry >= 0.0 && Now - LastTry < RetrySeconds)
	{
		return false;
	}
	LastTry = Now;
	if (!CardMesh)
	{
		CardMesh = LoadObject<UStaticMesh>(nullptr, PlanePath);
	}
	if (!CardMaterial && IsMade(MaterialPath))
	{
		CardMaterial = LoadObject<UMaterialInterface>(nullptr, MaterialPath);
	}
	if (!CardMesh || !CardMaterial)
	{
		UE_CLOG(!bWarned, LogLooter, Warning, TEXT("Window life: no %s yet (Tools/Unreal/build_window_life_material.py): no shadows or curtains at the windows."),
			MaterialPath);
		bWarned = true;
		return false;
	}
	Card->SetStaticMesh(CardMesh);
	Look = Card->CreateDynamicMaterialInstance(0, CardMaterial);
	if (Look)
	{
		Look->SetScalarParameterValue(WidthParameter, Width);
		Look->SetScalarParameterValue(HeightParameter, Height);
	}
	return Look != nullptr;
}

bool AWindowLifeCard::Play(const FWindowLifeEvent& InEvent, const FTransform& Window)
{
	if ((InEvent.Kind != EWindowLifeKind::Shadow && InEvent.Kind != EWindowLifeKind::Curtain) || !Prepare())
	{
		return false;
	}
	// The plane's X up the window, its face (+Z) out through the glass, just off it: in front of the pane, behind the sash
	// bars and the curtains the house is built with, inside the wall's depth.
	const FVector Out = Window.GetUnitAxis(EAxis::X);
	const FVector Up = Window.GetUnitAxis(EAxis::Z);
	SetActorLocationAndRotation(Window.GetLocation() + Out * OffGlass, FRotationMatrix::MakeFromXZ(Up, Out).Rotator());
	SetActorScale3D(FVector(Height / 100.f, Width / 100.f, 1.f));

	Event = InEvent;
	Age = 0.f;
	Look->SetScalarParameterValue(KindParameter, Event.Kind == EWindowLifeKind::Shadow ? ShadowKind : CurtainKind);
	Look->SetScalarParameterValue(SideParameter, Event.Side);
	Look->SetVectorParameterValue(ColorParameter, Event.Color);
	Pose = WindowLifeRules::PoseAt(Event, 0.f);
	ApplyPose();
	Card->SetVisibility(true);
	bPlaying = true;
	SetActorTickEnabled(true);
	return true;
}

void AWindowLifeCard::Advance(float DeltaSeconds)
{
	if (!bPlaying)
	{
		return;
	}
	Age += FMath::Max(DeltaSeconds, 0.f);
	if (Age > Event.Seconds)
	{
		Stop();
		return;
	}
	Pose = WindowLifeRules::PoseAt(Event, Age);
	ApplyPose();
}

void AWindowLifeCard::Stop()
{
	bPlaying = false;
	Pose = FWindowLifePose();
	ApplyPose();
	Card->SetVisibility(false);
	SetActorTickEnabled(false);
}

void AWindowLifeCard::ApplyPose()
{
	if (!Look)
	{
		return;
	}
	Look->SetScalarParameterValue(StrengthParameter, Pose.Strength);
	Look->SetScalarParameterValue(SlideParameter, Pose.Slide);
	Look->SetScalarParameterValue(RiseParameter, Pose.Rise);
	Look->SetScalarParameterValue(SwingParameter, Pose.Swing);
}

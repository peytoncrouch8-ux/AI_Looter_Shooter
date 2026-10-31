#include "Player/FlaskFXView.h"
#include "Player/FlaskFX.h"
#include "Player/PlayerViewComponent.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

namespace
{
	/** Without a model, how long the flask is taken to be behind its mouth (cm; SoulFlask.py: 16 cm to the cork's top). */
	constexpr float FlaskLength = 16.f;
	/** Where the chest is above the body's middle (cm), and where the lips are from the eye (cm, down and ahead). */
	constexpr float ChestUp = 30.f;
	constexpr float LipsDown = 5.f;
	constexpr float LipsAhead = 3.f;
	/** Third person with the hand's flask hidden: the lips under the pawn's view point (cm). */
	constexpr float HeadToLips = 8.f;
	/** Where the flask hangs, stowed: from the body's middle (cm, to the right and down). */
	constexpr float HipRight = 14.f;
	constexpr float HipDown = 12.f;
}

FlaskFXView::FEye FlaskFXView::EyeOf(const APawn& Drinker)
{
	FEye Eye;
	const UPlayerViewComponent* ViewMode = Drinker.FindComponentByClass<UPlayerViewComponent>();
	Eye.bFirstPerson = ViewMode && ViewMode->IsFirstPerson();
	const UCameraComponent* Camera = UPlayerViewComponent::FindFirstPersonCamera(&Drinker);
	Eye.Rotation = Camera ? Camera->GetComponentRotation() : Drinker.GetViewRotation();
	Eye.Location = Camera ? Camera->GetComponentLocation() : Drinker.GetPawnViewLocation();
	const FRotationMatrix Axes(Eye.Rotation);
	Eye.Forward = Axes.GetUnitAxis(EAxis::X);
	Eye.Right = Axes.GetUnitAxis(EAxis::Y);
	Eye.Up = Axes.GetUnitAxis(EAxis::Z);
	Eye.View.Location = Eye.Location;
	Eye.View.Rotation = Eye.Rotation;
	if (Camera)
	{
		Eye.View.FOV = Camera->FieldOfView;
		Eye.View.FirstPersonFOV = Camera->bEnableFirstPersonFieldOfView ? Camera->FirstPersonFieldOfView : Camera->FieldOfView;
		Eye.View.FirstPersonScale = Camera->bEnableFirstPersonScale ? Camera->FirstPersonScale : 1.f;
		Eye.View.bUseFirstPersonParameters = Camera->bEnableFirstPersonFieldOfView || Camera->bEnableFirstPersonScale;
	}
	const APlayerController* Player = Cast<APlayerController>(Drinker.GetController());
	Eye.Camera = Player && Player->PlayerCameraManager ? Player->PlayerCameraManager->GetCameraLocation() : Eye.Location;
	return Eye;
}

FVector FlaskFXView::ShownAt(const FEye& Eye, const FVector& Point, bool bFirstPersonModel)
{
	return bFirstPersonModel && Eye.View.bUseFirstPersonParameters ? Eye.View.TransformWorldToFirstPerson(Point, false) : Point;
}

const UStaticMeshComponent* FlaskFXView::FindFlask(const APawn& Drinker, bool bFirstPerson)
{
	TInlineComponentArray<UStaticMeshComponent*> Meshes(&Drinker);
	for (const UStaticMeshComponent* Mesh : Meshes)
	{
		const UStaticMesh* Shape = Mesh ? Mesh->GetStaticMesh() : nullptr;
		if (!Shape || !Mesh->IsVisible())
		{
			continue;
		}
		if (!Shape->GetName().StartsWith(TEXT("SM_SoulFlask")) && !Mesh->GetName().Contains(TEXT("Flask")))
		{
			continue;
		}
		// Only the model this eye actually draws: the first-person one in first person, the body's in third. Never the other
		// view's (the 2026-10-30 play-check: in third person the light came off a model the eye doesn't see, far from the
		// body), and never one hidden from its owner.
		const bool bFirstPersonModel = Mesh->FirstPersonPrimitiveType == EFirstPersonPrimitiveType::FirstPerson;
		if (bFirstPersonModel == bFirstPerson && !Mesh->bOwnerNoSee)
		{
			return Mesh;
		}
	}
	return nullptr;
}

bool FlaskFXView::PlaceOf(const APawn& Drinker, const FEye& Eye, const FVector* MouthInEye, FPlace& Out)
{
	if (const UStaticMeshComponent* Model = FindFlask(Drinker, Eye.bFirstPerson))
	{
		const bool bFirstPersonModel = Model->FirstPersonPrimitiveType == EFirstPersonPrimitiveType::FirstPerson;
		const FName Socket(FlaskFX::MouthSocket);
		const FVector Foot = Model->GetComponentLocation();
		FVector Mouth;
		FVector Way;
		if (Model->DoesSocketExist(Socket))
		{
			const FTransform At = Model->GetSocketTransform(Socket);
			Mouth = At.GetLocation();
			Way = At.GetRotation().GetUpVector();
		}
		else
		{
			// The stand-in has no socket: its pivot is its middle; up its own axis.
			Way = Model->GetUpVector();
			Mouth = Foot + Way * FlaskLength * 0.5f;
		}
		Out.Foot = ShownAt(Eye, Foot, bFirstPersonModel);
		Out.Mouth = ShownAt(Eye, Mouth, bFirstPersonModel);
		Out.Out = (ShownAt(Eye, Mouth + Way * 5.0, bFirstPersonModel) - Out.Mouth).GetSafeNormal(UE_SMALL_NUMBER, Way);
		Out.bModel = true;
		return true;
	}
	if (!Eye.bFirstPerson)
	{
		// Third person, the hand's flask not shown: at the head, the flask taken as tipped up at the lips in front of the
		// face. Never the mouth given (it may be the first-person camera's place, which this eye doesn't see).
		const FVector Ahead = Drinker.GetActorForwardVector();
		Out.Mouth = Drinker.GetPawnViewLocation() - FVector(0.0, 0.0, HeadToLips) + Ahead * LipsAhead;
		Out.Foot = Out.Mouth + Ahead * (FlaskLength * 0.6f) + FVector(0.0, 0.0, FlaskLength * 0.4f);
		Out.Out = (Out.Mouth - Out.Foot).GetSafeNormal(UE_SMALL_NUMBER, -Ahead);
		Out.bModel = false;
		return true;
	}
	if (!MouthInEye)
	{
		return false;
	}
	Out.Mouth = FromEye(Eye, *MouthInEye);
	Out.Out = (Eye.Location - Out.Mouth).GetSafeNormal(UE_SMALL_NUMBER, Eye.Up);
	Out.Foot = Out.Mouth - Out.Out * FlaskLength;
	Out.bModel = false;
	return true;
}

FlaskFXView::FPlace FlaskFXView::HipPlaceOf(const APawn& Drinker)
{
	FPlace Hip;
	Hip.Foot = Drinker.GetActorTransform().TransformPosition(FVector(0.0, HipRight, -HipDown));
	Hip.Out = FVector::UpVector;
	Hip.Mouth = Hip.Foot + Hip.Out * (FlaskLength * 0.6f);
	Hip.bModel = false;
	return Hip;
}

FVector FlaskFXView::BodyMiddleOf(const APawn& Drinker)
{
	return Drinker.GetActorLocation() + FVector(0.0, 0.0, ChestUp * 0.5f);
}

FVector FlaskFXView::SinkOf(const APawn& Drinker, const FEye& Eye)
{
	return Eye.bFirstPerson ? Eye.Location - Eye.Up * LipsDown + Eye.Forward * LipsAhead
		: Drinker.GetActorLocation() + FVector(0.0, 0.0, ChestUp);
}

float FlaskFXView::SizeAt(const FEye& Eye, const FVector& At, float Share, float Min, float Max)
{
	return FMath::Clamp(static_cast<float>(FVector::Dist(At, Eye.Camera)) * Share, Min, FMath::Max(Min, Max));
}

FVector FlaskFXView::ToEye(const FEye& Eye, const FVector& World)
{
	return FTransform(Eye.Rotation, Eye.Location).InverseTransformPosition(World);
}

FVector FlaskFXView::FromEye(const FEye& Eye, const FVector& Local)
{
	return FTransform(Eye.Rotation, Eye.Location).TransformPosition(Local);
}

#include "Player/PlayerFlaskComponent.h"
#include "AI_Looter_Shooter.h"
#include "Player/Animation/LooterStancePoseDetail.h"
#include "Player/PlayerFlaskMotion.h"
#include "Player/PlayerSize.h"
#include "Player/PlayerViewComponent.h"
#include "UI/Style/LooterUIStyle.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/PackageName.h"

// UPlayerFlaskComponent's flask as it's seen: SM_SoulFlask (FX's model) in the first-person off hand on the camera, on
// FlaskMotion's poses, and in the body's left hand for third person, where the stance layer lifts it
// (LooterStancePose::DescribeDrink). Its liquid glows bright with charges in it and dim with none.

namespace
{
	/** Until SM_SoulFlask is imported: the engine's cylinder, flask-sized (8 cm across, 16 tall), in the soul-motes' green. */
	const TCHAR* StandInMeshPath = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
	const TCHAR* StandInMaterialPath = TEXT("/Game/Environment/Materials/M_StylizedSurface.M_StylizedSurface");
	const FVector StandInScale(0.08f, 0.08f, 0.16f);
	/** The cylinder's pivot is its middle, 50 of its 100 cm up: its foot is this far below the pivot, its top this far above. */
	constexpr float StandInHalfHeight = 50.f;
	/** The stand-in's own glow (M_StylizedSurface's emissive multiplier), as bright as a soul-mote's trail. */
	constexpr float StandInGlow = 1.6f;

	/** The liquid's glow with no charge in it, against its full glow: dim, but still a faint ember of soul. */
	constexpr float EmptyGlowShare = 0.15f;

	/** The body's left-hand grip socket (the jar's: UHandJarComponent::HandSocket), and a body without it: the hand bone. */
	const TCHAR* BodySocket = TEXT("HandGrip_L");
	const TCHAR* BodyFallbackBone = TEXT("hand_l");
	/** Off the grip point away from the palm (cm): the flask's body (4 cm radius) is thicker than the grip the socket was placed for. */
	constexpr float BodyOffPalm = 2.5f;
}

UStaticMesh* UPlayerFlaskComponent::LoadFlaskMesh()
{
	if (bMeshLooked)
	{
		return FlaskMesh;
	}
	bMeshLooked = true;
	// Only once its package is in this build: no load warning while the model is still being made.
	const FSoftObjectPath Path(FlaskMeshPath());
	if (FPackageName::DoesPackageExist(Path.GetLongPackageName()))
	{
		FlaskMesh = Cast<UStaticMesh>(Path.TryLoad());
	}
	bStandIn = FlaskMesh == nullptr;
	if (bStandIn)
	{
		FlaskMesh = LoadObject<UStaticMesh>(nullptr, StandInMeshPath);
		UE_LOG(LogLooter, Log, TEXT("Flask: %s isn't in the game yet; a plain cylinder stands in."), FlaskMeshPath());
	}
	return FlaskMesh;
}

UStaticMeshComponent* UPlayerFlaskComponent::MakeModel(USceneComponent* Parent, FName Socket, const TCHAR* Name)
{
	AActor* Owner = GetOwner();
	UStaticMesh* Mesh = LoadFlaskMesh();
	if (!Owner || !Parent || !Mesh)
	{
		return nullptr;
	}
	UStaticMeshComponent* Model = NewObject<UStaticMeshComponent>(Owner, Name);
	Model->SetStaticMesh(Mesh);
	Model->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	Model->SetGenerateOverlapEvents(false);
	Model->SetCanEverAffectNavigation(false);
	Model->SetCastShadow(false);
	Model->bReceivesDecals = false;
	Model->SetupAttachment(Parent, Socket);
	Model->SetVisibility(false);
	Model->RegisterComponent();

	if (bStandIn)
	{
		// The soul-motes' own look, so the stand-in reads as the flask's light.
		if (UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, StandInMaterialPath))
		{
			UMaterialInstanceDynamic* Look = Model->CreateDynamicMaterialInstance(0, Base);
			if (Look)
			{
				Look->SetVectorParameterValue(TEXT("Color"), LooterUI::Color::Heal());
				Look->SetScalarParameterValue(GlowParameter(), StandInGlow);
				GlowMaterials.Add(Look);
				BaseGlows.Add(StandInGlow);
			}
		}
		return Model;
	}
	// The liquid's materials: whichever of the model's carry a Glow (FX's README says which). Each gets its own instance
	// on this model, its authored Glow kept as the full glow the charges scale.
	for (int32 Slot = 0; Slot < Model->GetNumMaterials(); ++Slot)
	{
		UMaterialInterface* Material = Model->GetMaterial(Slot);
		float Glow = 0.f;
		if (!Material || !Material->GetScalarParameterValue(FHashedMaterialParameterInfo(GlowParameter()), Glow))
		{
			continue;
		}
		if (UMaterialInstanceDynamic* Look = Model->CreateDynamicMaterialInstance(Slot, Material))
		{
			GlowMaterials.Add(Look);
			BaseGlows.Add(Glow);
		}
	}
	return Model;
}

UStaticMeshComponent* UPlayerFlaskComponent::GetOrMakeHandModel()
{
	if (HandModel)
	{
		return HandModel;
	}
	UCameraComponent* Camera = UPlayerViewComponent::FindFirstPersonCamera(GetOwner());
	HandModel = MakeModel(Camera, NAME_None, TEXT("FlaskInHand"));
	if (HandModel)
	{
		// Drawn small and held out at arm's length (FlaskMotion::ViewScale): full size at the lips it filled the view.
		HandModel->SetRelativeScale3D((bStandIn ? StandInScale : FVector::OneVector) * FlaskMotion::ViewScale);
		// Drawn like the gun in hand and the grenade's jar: first-person rendering at the view model's own field of view,
		// never clipping a wall, only for its owner.
		HandModel->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::FirstPerson);
		HandModel->SetOnlyOwnerSee(true);
		UpdateGlow();
	}
	return HandModel;
}

UStaticMeshComponent* UPlayerFlaskComponent::GetOrMakeBodyModel()
{
	if (BodyModel)
	{
		return BodyModel;
	}
	ACharacter* Body = Cast<ACharacter>(GetOwner());
	USkeletalMeshComponent* Mesh = Body ? Body->GetMesh() : nullptr;
	if (!Mesh || !Mesh->GetSkinnedAsset())
	{
		return nullptr;
	}
	const bool bSocket = Mesh->DoesSocketExist(BodySocket);
	if (!bSocket && !Mesh->DoesSocketExist(BodyFallbackBone))
	{
		return nullptr;
	}
	BodyModel = MakeModel(Mesh, bSocket ? FName(BodySocket) : FName(BodyFallbackBone), TEXT("FlaskInBodyHand"));
	if (!BodyModel)
	{
		return nullptr;
	}
	// Full size whatever the player's size, like the jar in the body's hand; the offset is in the body's (smaller) space,
	// hence the divide. Its axis along the grip's, its middle in the fist.
	BodyModel->SetUsingAbsoluteScale(true);
	BodyModel->SetWorldScale3D(bStandIn ? StandInScale : FVector::OneVector);
	const float FootDown = bStandIn ? 0.f : FlaskMotion::MiddleUp;
	FVector Offset(0.f, BodyOffPalm, -FootDown / LooterPlayerSize::Scale);
	if (!bSocket)
	{
		Offset += LooterStancePoseDetail::LeftPalmFromWrist();
	}
	BodyModel->SetRelativeLocation(Offset);
	UpdateGlow();
	return BodyModel;
}

void UPlayerFlaskComponent::UpdateModels()
{
	const APawn* Owner = Cast<APawn>(GetOwner());
	const UPlayerViewComponent* View = Owner ? Owner->FindComponentByClass<UPlayerViewComponent>() : nullptr;
	const bool bFirstPerson = !View || View->IsFirstPerson();
	bool bShown = false;
	const FThrowPose Pose = FlaskMotion::FlaskPose(Clock, bShown);
	bShown &= bDrinking;

	// The first-person flask is the view's own; in third person the body's hand carries one.
	if (bShown && bFirstPerson && GetPlayer())
	{
		if (UStaticMeshComponent* Model = GetOrMakeHandModel())
		{
			// The stand-in's pivot is its middle: lifted along its own axis, so its foot is where the pose puts the foot.
			const FVector Lift = bStandIn ? Pose.Rotation.RotateVector(FVector(0.f, 0.f, StandInHalfHeight * StandInScale.Z * FlaskMotion::ViewScale)) : FVector::ZeroVector;
			Model->SetRelativeLocationAndRotation(Pose.Offset + Lift, Pose.Rotation);
			if (!Model->IsVisible())
			{
				Model->SetVisibility(true);
			}
		}
	}
	else if (HandModel && HandModel->IsVisible())
	{
		HandModel->SetVisibility(false);
	}

	// The body's flask, drawn as the body it's in is: hidden to its owner in first person (its shadow-only stand-in), seen in
	// third. Shown through the same stretch the first-person one is.
	if (bShown)
	{
		if (UStaticMeshComponent* Model = GetOrMakeBodyModel())
		{
			if (const UPrimitiveComponent* Body = Cast<UPrimitiveComponent>(Model->GetAttachParent()))
			{
				if (Model->bOwnerNoSee != Body->bOwnerNoSee)
				{
					Model->SetOwnerNoSee(Body->bOwnerNoSee);
				}
				if (Model->FirstPersonPrimitiveType != Body->FirstPersonPrimitiveType)
				{
					Model->SetFirstPersonPrimitiveType(Body->FirstPersonPrimitiveType);
				}
			}
			if (!Model->IsVisible())
			{
				Model->SetVisibility(true);
			}
		}
	}
	else if (BodyModel && BodyModel->IsVisible())
	{
		BodyModel->SetVisibility(false);
	}
	UpdateGlow();
}

void UPlayerFlaskComponent::HideModels()
{
	for (UStaticMeshComponent* Model : { HandModel.Get(), BodyModel.Get() })
	{
		if (Model && Model->IsVisible())
		{
			Model->SetVisibility(false);
		}
	}
}

void UPlayerFlaskComponent::UpdateGlow()
{
	if (GlowMaterials.IsEmpty())
	{
		return;
	}
	// The soul in the flask now: the charges, the one being drunk draining as the heal goes in, and the next one's soul.
	const float Draining = bDrinking || bPouring ? 1.f - FFlaskRules::HealShareAt(Clock) : 0.f;
	const float Liquid = Charges > 0 ? 1.f : Draining;
	const float Glow = FMath::Lerp(EmptyGlowShare, 1.f, FMath::Clamp(Liquid, 0.f, 1.f));
	const float Fill = FMath::Clamp((Charges + Draining + GetSoulShare()) / FFlaskRules::MaxCharges, 0.f, 1.f);
	const FHashedMaterialParameterInfo FillInfo(FillParameter());
	for (int32 Index = 0; Index < GlowMaterials.Num(); ++Index)
	{
		UMaterialInstanceDynamic* Look = GlowMaterials[Index];
		if (!Look)
		{
			continue;
		}
		Look->SetScalarParameterValue(GlowParameter(), BaseGlows.IsValidIndex(Index) ? BaseGlows[Index] * Glow : Glow);
		float Unused = 0.f;
		if (Look->GetScalarParameterValue(FillInfo, Unused))
		{
			Look->SetScalarParameterValue(FillParameter(), Fill);
		}
	}
}

FVector UPlayerFlaskComponent::MouthOfModel(const UStaticMeshComponent& Model) const
{
	if (Model.DoesSocketExist(MouthSocket()))
	{
		return Model.GetSocketLocation(MouthSocket());
	}
	// No socket: the mesh's own top (the cylinder's) or the model's measured mouth.
	return Model.GetComponentTransform().TransformPosition(FVector(0.f, 0.f, bStandIn ? StandInHalfHeight : FlaskMotion::MouthUp));
}

FVector UPlayerFlaskComponent::GetMouthLocation() const
{
	if (HandModel && HandModel->IsVisible())
	{
		return MouthOfModel(*HandModel);
	}
	if (BodyModel && BodyModel->IsVisible() && !BodyModel->bOwnerNoSee)
	{
		return MouthOfModel(*BodyModel);
	}
	// Nothing seen (no camera, a test's holder): just under and ahead of the eye, where the lips are.
	const APawn* Owner = Cast<APawn>(GetOwner());
	if (!Owner)
	{
		return FVector::ZeroVector;
	}
	const UCameraComponent* Eye = UPlayerViewComponent::FindFirstPersonCamera(Owner);
	const FTransform View = Eye ? Eye->GetComponentTransform() : FTransform(Owner->GetActorRotation(), Owner->GetPawnViewLocation());
	return View.TransformPosition(FlaskMotion::MouthOf(FlaskMotion::AtLipsPose()));
}

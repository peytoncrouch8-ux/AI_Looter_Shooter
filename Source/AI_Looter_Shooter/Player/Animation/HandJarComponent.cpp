#include "Player/Animation/HandJarComponent.h"
#include "AI_Looter_Shooter.h"
#include "Combat/GraveSaltGrenade.h"
#include "Player/Animation/LooterStancePoseDetail.h"
#include "Player/PlayerSize.h"
#include "Player/PlayerThrowComponent.h"
#include "Player/PlayerThrowMotion.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"

namespace
{
	/**
	 * The tin's middle above its foot, its pivot (cm; Art/Models/Throwables/SaltGrenade.py): the middle sits in the fist.
	 * The offset is in the body's space, which is the player's size, while the tin is full size: hence the divide.
	 */
	constexpr float JarMiddle = 5.f;
	/** Off the grip point away from the palm (cm): the tin is twice as thick as the grip the socket was placed for. */
	constexpr float OffPalm = 1.f;
	/** A body without the mannequin's grip sockets: the hand bone itself, and the palm's spot on it (LeftPalmFromWrist). */
	const TCHAR* FallbackBone = TEXT("hand_l");
}

UHandJarComponent::UHandJarComponent()
{
	// It only has to notice a throw starting and ending: a check a frame, after the throw has ticked.
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;

	SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	SetGenerateOverlapEvents(false);
	SetCanEverAffectNavigation(false);
	bReceivesDecals = false;
	// The jar's axis along the grip's (the thumb's side, so the fuse stands up out of the fist), its middle in the hand.
	SetRelativeLocation(FVector(0.f, OffPalm, -JarMiddle / LooterPlayerSize::Scale));
	// Full size, as the jar that flies off is (the body is scaled to the player's size).
	SetUsingAbsoluteScale(true);
}

void UHandJarComponent::BeginPlay()
{
	Super::BeginPlay();
	if (!GetStaticMesh())
	{
		// Null before the tin's import: then it never shows (the throw goes on all the same).
		SetStaticMesh(AGraveSaltGrenade::FindJarMesh());
	}
	USceneComponent* Body = GetAttachParent();
	if (Body && !Body->DoesSocketExist(GetAttachSocketName()) && Body->DoesSocketExist(FallbackBone))
	{
		UE_LOG(LogLooter, Log, TEXT("%s: the body has no %s socket; the jar sits on %s instead."), *GetName(), *GetAttachSocketName().ToString(), FallbackBone);
		AttachToComponent(Body, FAttachmentTransformRules::KeepRelativeTransform, FallbackBone);
		SetRelativeLocation(GetRelativeLocation() + LooterStancePoseDetail::LeftPalmFromWrist());
	}
	SetVisibility(false);
	Refresh();
}

void UHandJarComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	Refresh();
}

void UHandJarComponent::Refresh()
{
	if (!Throw.IsValid())
	{
		Throw = UPlayerThrowComponent::Find(GetOwner());
	}

	// Drawn as the body it's in is: hidden to its owner in first person (a world-space stand-in there, for the shadow),
	// seen in third. The view switches the body (UPlayerViewComponent::ApplyMode); the jar follows it here.
	if (const UPrimitiveComponent* Body = Cast<UPrimitiveComponent>(GetAttachParent()))
	{
		if (bOwnerNoSee != Body->bOwnerNoSee)
		{
			SetOwnerNoSee(Body->bOwnerNoSee);
		}
		if (FirstPersonPrimitiveType != Body->FirstPersonPrimitiveType)
		{
			SetFirstPersonPrimitiveType(Body->FirstPersonPrimitiveType);
		}
	}

	const bool bInHand = GetStaticMesh() && Throw.IsValid() && Throw->IsThrowing() && ThrowMotion::IsJarInHand(Throw->GetThrowTime());
	if (IsVisible() != bInHand)
	{
		SetVisibility(bInHand);
	}
}

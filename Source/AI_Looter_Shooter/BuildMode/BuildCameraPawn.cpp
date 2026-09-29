#include "BuildMode/BuildCameraPawn.h"
#include "Camera/CameraComponent.h"

ABuildCameraPawn::ABuildCameraPawn()
{
	PrimaryActorTick.bCanEverTick = false;
	SetCanBeDamaged(false);
	SetActorEnableCollision(false);

	bUseControllerRotationPitch = true;
	bUseControllerRotationYaw = true;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->FieldOfView = 90.f;
	SetRootComponent(Camera);
}

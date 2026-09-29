#include "RTSCameraPawn.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"

ARTSCameraPawn::ARTSCameraPawn()
{
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	Arm = CreateDefaultSubobject<USpringArmComponent>(TEXT("Arm"));
	Arm->SetupAttachment(Root);
	Arm->TargetArmLength = 3000.f;
	Arm->bDoCollisionTest = false;
	Arm->bUsePawnControlRotation = false;
	Arm->SetUsingAbsoluteRotation(true);
	Arm->SetWorldRotation(FRotator(-55.f, 0.f, 0.f));
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(Arm);
}

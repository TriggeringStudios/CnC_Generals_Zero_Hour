#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "RTSCameraPawn.generated.h"

class USpringArmComponent;
class UCameraComponent;

/** Top-down RTS camera. Panning and zoom are driven by ARTSPlayerController. */
UCLASS()
class GENERALSRTS_API ARTSCameraPawn : public APawn
{
	GENERATED_BODY()
public:
	ARTSCameraPawn();
	UPROPERTY(VisibleAnywhere) TObjectPtr<USpringArmComponent> Arm;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UCameraComponent> Camera;
};

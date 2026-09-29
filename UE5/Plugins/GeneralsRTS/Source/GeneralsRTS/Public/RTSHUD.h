#pragma once
#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "RTSHUD.generated.h"

UCLASS()
class GENERALSRTS_API ARTSHUD : public AHUD
{
	GENERATED_BODY()
public:
	virtual void DrawHUD() override;
};

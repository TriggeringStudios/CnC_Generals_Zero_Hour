#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Info.h"
#include "RTSTeamInfo.generated.h"

class URTSEconomyComponent;
class URTSSkirmishAI;

/** One per side, human or AI. Owns the money and (for AI sides) the skirmish brain. */
UCLASS()
class GENERALSRTS_API ARTSTeamInfo : public AInfo
{
	GENERATED_BODY()
public:
	ARTSTeamInfo();
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 TeamId = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FLinearColor TeamColor = FLinearColor::Blue;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bIsAI = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 StartingMoney = 1000;
	UPROPERTY(VisibleAnywhere) TObjectPtr<URTSEconomyComponent> Economy;
	UPROPERTY(VisibleAnywhere) TObjectPtr<URTSSkirmishAI> AI;
protected:
	virtual void BeginPlay() override;
};

#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "RTSGameMode.generated.h"

class URTSObjectDefinition;

/**
 * Sets up a skirmish on an empty level: a floor, one human base and one AI base with supply
 * piles around each, and the default units/HQ (built in code so it runs with zero content).
 */
UCLASS()
class GENERALSRTS_API ARTSGameMode : public AGameModeBase
{
	GENERATED_BODY()
public:
	ARTSGameMode();
	UPROPERTY(EditAnywhere) float BaseSeparation = 12000.f;
	UPROPERTY(EditAnywhere) float MapHalfExtent = 30000.f;
	UPROPERTY(EditAnywhere) int32 SupplyPilesPerBase = 8;
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void StartPlay() override;
private:
	void SetupSkirmish();
	UPROPERTY() TObjectPtr<URTSObjectDefinition> HQDefinition;
};

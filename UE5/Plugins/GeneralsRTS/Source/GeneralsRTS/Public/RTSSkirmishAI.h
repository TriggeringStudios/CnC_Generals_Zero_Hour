#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RTSSkirmishAI.generated.h"

/**
 * Computer opponent (AIPlayer/AISkirmishPlayer in the original). Lives on an ARTSTeamInfo and plays
 * exactly like a human: it can only issue FRTSCommands and queue production, no cheating.
 * Loop: keep harvesters mining -> keep the economy topped up -> mass an army -> attack in growing
 * waves -> pull the army home when a building is hit.
 */
UCLASS(ClassGroup=RTS)
class GENERALSRTS_API URTSSkirmishAI : public UActorComponent
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere) int32 DesiredWorkers = 4;
	UPROPERTY(EditAnywhere) int32 WaveSize = 6;
	UPROPERTY(EditAnywhere) int32 WaveGrowth = 3;
	UPROPERTY(EditAnywhere) int32 ThinkIntervalFrames = 30;
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
	void OnSimFrame();
	void Think();
	FDelegateHandle SimHandle;
	bool bWaveActive = false;
	int32 ArmyCycle = 0;
};

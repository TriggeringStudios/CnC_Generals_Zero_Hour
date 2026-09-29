#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RTSObjectDefinition.h"
#include "RTSProductionComponent.generated.h"

/**
 * Build queue on a factory (ProductionUpdate). Cost is charged progressively in the
 * original; here it is charged up front and refunded on cancel (a deliberate simplification
 * to revisit). Ticked by the fixed-step sim, not by frame delta.
 */
UCLASS(ClassGroup=RTS, meta=(BlueprintSpawnableComponent))
class GENERALSRTS_API URTSProductionComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere) TObjectPtr<URTSObjectDefinition> Definition;
	UPROPERTY(EditAnywhere) int32 MaxQueue = 5;
	UPROPERTY(EditAnywhere) FVector RallyPoint = FVector::ZeroVector;

	UFUNCTION(BlueprintCallable) bool QueueUnit(FName ObjectName);
	UFUNCTION(BlueprintCallable) bool CancelLast();
	/** Advance one logic frame (1/RTS_LOGIC_FPS s). */
	void SimTick();
protected:
	virtual void OnUnitFinished(const FRTSProductionEntry& Entry);
private:
	const FRTSProductionEntry* Find(FName ObjectName) const;
	struct FJob { FRTSProductionEntry Entry; int32 FramesLeft; };
	TArray<FJob> Queue;
};

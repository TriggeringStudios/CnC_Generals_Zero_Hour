#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RTSObjectDefinition.h"
#include "RTSProductionComponent.generated.h"

/**
 * Factory build queue (ProductionUpdate in the original). Cost is charged up front and refunded
 * on cancel. SimTick is driven by the owning building on the fixed 30 Hz step.
 */
UCLASS(ClassGroup=RTS, meta=(BlueprintSpawnableComponent))
class GENERALSRTS_API URTSProductionComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere) TObjectPtr<URTSObjectDefinition> Definition;
	UPROPERTY(EditAnywhere) int32 MaxQueue = 5;
	FVector RallyPoint = FVector::ZeroVector;
	bool bHasRally = false;

	UFUNCTION(BlueprintCallable) bool QueueUnit(FName ObjectName);
	UFUNCTION(BlueprintCallable) bool CancelLast();
	void SimTick();

	int32 GetQueueCount() const { return Queue.Num(); }
	int32 CountQueued(ERTSUnitRole Role) const;
	float GetProgress01() const; // front of queue
	FName GetFrontName() const { return Queue.Num() ? Queue[0].Entry.ObjectName : NAME_None; }

	/** Spawns a fully configured unit for a team. Shared by factories and initial base setup. */
	static class ARTSUnit* SpawnUnit(UWorld* World, const FRTSProductionEntry& Entry, int32 TeamId, const FVector& Location);
private:
	struct FJob { FRTSProductionEntry Entry; int32 TotalFrames; int32 FramesLeft; };
	TArray<FJob> Queue;
	void Finish(const FRTSProductionEntry& Entry);
};

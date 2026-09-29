#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "RTSSimSubsystem.generated.h"

class ARTSTeamInfo;
class ARTSResourceNode;
class ARTSBuilding;

DECLARE_MULTICAST_DELEGATE(FRTSSimTickDelegate);

/**
 * Fixed-step simulation clock plus the world's entity registry. Everything that makes
 * gameplay decisions binds OnSimTick; rendering-only work stays in Actor Tick.
 * Queries are linear scans: fine for a prototype, replace with a spatial grid for big armies.
 */
UCLASS()
class GENERALSRTS_API URTSSimSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()
public:
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(URTSSimSubsystem, STATGROUP_Tickables); }

	FRTSSimTickDelegate OnSimTick;
	int32 GetFrame() const { return Frame; }

	void RegisterEntity(AActor* A) { Entities.AddUnique(A); }
	void UnregisterEntity(AActor* A) { Entities.Remove(A); }
	void RegisterNode(ARTSResourceNode* N) { Nodes.AddUnique(N); }
	void RegisterTeam(ARTSTeamInfo* T);
	ARTSTeamInfo* GetTeam(int32 TeamId) const;

	void GetTeamActors(int32 TeamId, TArray<AActor*>& Out) const;
	AActor* FindNearestEnemy(const FVector& From, int32 MyTeam, float MaxRange) const;
	ARTSResourceNode* FindNearestNode(const FVector& From) const;
	ARTSBuilding* FindNearestDepot(const FVector& From, int32 TeamId) const;

private:
	float Accumulator = 0.f;
	int32 Frame = 0;
	TArray<TWeakObjectPtr<AActor>> Entities;
	TArray<TWeakObjectPtr<ARTSResourceNode>> Nodes;
	TMap<int32, TWeakObjectPtr<ARTSTeamInfo>> Teams;
};

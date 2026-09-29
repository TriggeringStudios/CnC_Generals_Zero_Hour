#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "RTSTypes.h"
#include "RTSTargetable.h"
#include "RTSUnit.generated.h"

class USphereComponent;
class ARTSResourceNode;

/**
 * A unit that takes FRTSCommands. Decisions run on the fixed sim step (SimTick); Tick only
 * integrates movement smoothly per rendered frame. Movement is straight-line steering for now;
 * pathfinding/flow fields and crowd separation are the next layer.
 */
UCLASS()
class GENERALSRTS_API ARTSUnit : public APawn, public IRTSTargetable
{
	GENERATED_BODY()
public:
	ARTSUnit();
	UPROPERTY(EditAnywhere) int32 TeamId = 0;
	UPROPERTY(EditAnywhere) FName ObjectName;
	UPROPERTY(EditAnywhere) ERTSUnitRole Role = ERTSUnitRole::Combat;
	UPROPERTY(EditAnywhere) FRTSUnitStats Stats;

	void ApplyStats(const FRTSUnitStats& InStats);
	void IssueCommand(const FRTSCommand& Cmd);
	bool HasOrder() const { return bHasOrder; }
	bool CanAttack() const { return Stats.AttackDamage > 0.f; }
	float GetCarried() const { return Carried; }

	virtual int32 GetTeamId() const override { return TeamId; }
	virtual bool IsAlive() const override { return Health > 0.f; }
	virtual void ReceiveDamage(float Amount, AActor* From) override;
	virtual void SetSelected(bool bInSelected) override { bSelected = bInSelected; }
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual void Tick(float DeltaTime) override;
private:
	enum class EGatherPhase : uint8 { ToNode, Harvesting, ToDepot };

	void SimTick();
	void Begin(const FRTSCommand& Cmd);
	void NextOrder();
	void SteerTo(const FVector& Dest);
	float Dist2D(const FVector& P) const;
	bool EngageTarget(AActor* Target);   // chase/shoot; false if target is gone or we can't attack
	void TickGather();
	void ApplySeparation();

	UPROPERTY() TObjectPtr<USphereComponent> Root;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Mesh;
	FDelegateHandle SimHandle;

	float Health = 100.f;
	bool bSelected = false;
	bool bHasOrder = false;
	FRTSCommand Current;
	TArray<FRTSCommand> Queue;
	FVector MoveDir = FVector::ZeroVector;
	int32 CooldownFrames = 0;
	TWeakObjectPtr<AActor> AutoTarget;
	bool bFiring = false;

	// Path following (see URTSPathSubsystem).
	TArray<FVector> Path;
	int32 PathIdx = 0;
	FVector PathGoal = FVector::ZeroVector;
	int32 LastPathFrame = -1000;

	EGatherPhase Phase = EGatherPhase::ToNode;
	TWeakObjectPtr<ARTSResourceNode> Node;
	float Carried = 0.f;
};

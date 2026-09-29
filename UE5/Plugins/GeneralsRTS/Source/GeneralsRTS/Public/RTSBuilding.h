#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RTSTargetable.h"
#include "RTSBuilding.generated.h"

class URTSProductionComponent;
class UBoxComponent;

/** A structure: can be a supply depot, a factory, or both (Command Center style). */
UCLASS()
class GENERALSRTS_API ARTSBuilding : public AActor, public IRTSTargetable
{
	GENERATED_BODY()
public:
	ARTSBuilding();
	UPROPERTY(EditAnywhere) int32 TeamId = 0;
	UPROPERTY(EditAnywhere) float MaxHealth = 1500.f;
	UPROPERTY(EditAnywhere) bool bIsDepot = true;
	UPROPERTY(EditAnywhere) FVector SpawnOffset = FVector(0.f, 350.f, -50.f);
	UPROPERTY(VisibleAnywhere) TObjectPtr<URTSProductionComponent> Production;

	float Health = 0.f;
	int32 LastDamagedFrame = -100000;
	FVector LastAttackerLocation = FVector::ZeroVector;

	virtual int32 GetTeamId() const override { return TeamId; }
	virtual bool IsAlive() const override { return Health > 0.f; }
	virtual bool IsStructure() const override { return true; }
	virtual void ReceiveDamage(float Amount, AActor* From) override;
	virtual void SetSelected(bool bInSelected) override { bSelected = bInSelected; }
	bool IsSelected() const { return bSelected; }
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual void Tick(float DeltaTime) override;
private:
	void SimTick();
	UPROPERTY() TObjectPtr<UBoxComponent> Box;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Mesh;
	FDelegateHandle SimHandle;
	bool bSelected = false;
};

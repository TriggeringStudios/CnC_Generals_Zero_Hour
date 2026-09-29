#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RTSResourceNode.generated.h"

/** A supply pile that harvesters mine (the supply dock/stash in the original). */
UCLASS()
class GENERALSRTS_API ARTSResourceNode : public AActor
{
	GENERATED_BODY()
public:
	ARTSResourceNode();
	UPROPERTY(EditAnywhere) float Remaining = 5000.f;
	UPROPERTY(EditAnywhere) float Radius = 150.f;
	bool IsDepleted() const { return Remaining <= 0.f; }
	/** Removes up to Amount and returns what was actually taken. */
	float Extract(float Amount);
protected:
	virtual void BeginPlay() override;
private:
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Mesh;
};

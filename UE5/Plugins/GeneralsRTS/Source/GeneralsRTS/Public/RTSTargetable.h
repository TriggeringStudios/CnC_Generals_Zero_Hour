#pragma once
#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "RTSTargetable.generated.h"

UINTERFACE(MinimalAPI, NotBlueprintable)
class URTSTargetable : public UInterface { GENERATED_BODY() };

/** Implemented by units and buildings: anything owned by a team that can be selected/attacked. */
class GENERALSRTS_API IRTSTargetable
{
	GENERATED_BODY()
public:
	virtual int32 GetTeamId() const = 0;
	virtual bool IsAlive() const = 0;
	virtual void ReceiveDamage(float Amount, AActor* From) = 0;
	virtual bool IsStructure() const { return false; }
	virtual void SetSelected(bool bSelected) {}
};

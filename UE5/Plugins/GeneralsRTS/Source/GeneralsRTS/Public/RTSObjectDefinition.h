#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RTSTypes.h"
#include "RTSObjectDefinition.generated.h"

/** One thing a factory can build. Data-driven: new units need no code, only an entry. */
USTRUCT(BlueprintType)
struct FRTSProductionEntry
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere) FName ObjectName;
	UPROPERTY(EditAnywhere) int32 Cost = 100;
	UPROPERTY(EditAnywhere) float BuildTimeSeconds = 5.f;
	UPROPERTY(EditAnywhere) ERTSUnitRole Role = ERTSUnitRole::Combat;
	UPROPERTY(EditAnywhere) TSubclassOf<class ARTSUnit> ActorClass; // null = default ARTSUnit
	UPROPERTY(EditAnywhere) FRTSUnitStats Stats;
};

/** What a factory building can produce. */
UCLASS(BlueprintType)
class GENERALSRTS_API URTSObjectDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere) FName TemplateName;
	UPROPERTY(EditAnywhere) TArray<FRTSProductionEntry> Produces;
};

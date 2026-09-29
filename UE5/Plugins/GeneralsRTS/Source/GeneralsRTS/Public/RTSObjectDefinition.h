#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RTSObjectDefinition.generated.h"

/** Unit production entry, from ProductionUpdate / ProductionExitUpdate in Object INI. */
USTRUCT(BlueprintType)
struct FRTSProductionEntry
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere) FName ObjectName;
	UPROPERTY(EditAnywhere) int32 Cost = 0;
	UPROPERTY(EditAnywhere) float BuildTimeSeconds = 5.f;
	UPROPERTY(EditAnywhere) TArray<FName> RequiredBuildings;
	UPROPERTY(EditAnywhere) FName RequiredUpgrade;
};

/**
 * Data-driven object template (the ThingTemplate in the original).
 * Populated by the INI importer from the user's own game data. Rules live here,
 * not in code, so units can be extended without recompiling.
 */
UCLASS(BlueprintType)
class GENERALSRTS_API URTSObjectDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere) FName TemplateName;
	UPROPERTY(EditAnywhere) float MaxHealth = 100.f;
	UPROPERTY(EditAnywhere) FName ArmorSet;
	UPROPERTY(EditAnywhere) FName LocomotorSet;
	UPROPERTY(EditAnywhere) TArray<FName> WeaponSets;
	UPROPERTY(EditAnywhere) int32 BuildCost = 0;
	UPROPERTY(EditAnywhere) float BuildTimeSeconds = 0.f;
	UPROPERTY(EditAnywhere) float VisionRange = 300.f;
	UPROPERTY(EditAnywhere) TArray<FRTSProductionEntry> Produces;
	UPROPERTY(EditAnywhere) TSoftObjectPtr<UStaticMesh> Mesh; // imported from W3D
	UPROPERTY(EditAnywhere) TMap<FName, FString> RawModules; // unmapped INI modules kept for later
};

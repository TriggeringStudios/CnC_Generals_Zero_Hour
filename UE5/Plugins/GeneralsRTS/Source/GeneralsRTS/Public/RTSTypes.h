#pragma once
#include "CoreMinimal.h"
#include "RTSTypes.generated.h"

// The original sim ran at a fixed 30 Hz. We keep that: gameplay is stepped at RTS_LOGIC_FPS
// by URTSSimSubsystem, and rendering interpolates on top of it.
constexpr int32 RTS_LOGIC_FPS = 30;
constexpr float RTS_LOGIC_DT = 1.f / RTS_LOGIC_FPS;

/** Everything a player or the AI can order. Both go through the same path. */
UENUM(BlueprintType)
enum class ERTSCommandType : uint8 { Move, AttackMove, Attack, Stop, Gather };

UENUM(BlueprintType)
enum class ERTSUnitRole : uint8 { Worker, Combat };

USTRUCT(BlueprintType)
struct FRTSCommand
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadWrite) ERTSCommandType Type = ERTSCommandType::Move;
	UPROPERTY(BlueprintReadWrite) FVector Location = FVector::ZeroVector;
	UPROPERTY(BlueprintReadWrite) TWeakObjectPtr<AActor> Target;
	UPROPERTY(BlueprintReadWrite) bool bQueued = false; // shift-queue
};

USTRUCT(BlueprintType)
struct FRTSUnitStats
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere) float MaxHealth = 100.f;
	UPROPERTY(EditAnywhere) float MoveSpeed = 450.f;
	UPROPERTY(EditAnywhere) float AttackRange = 600.f;
	UPROPERTY(EditAnywhere) float AttackDamage = 0.f; // 0 = cannot attack
	UPROPERTY(EditAnywhere) float AttackCooldownSec = 1.f;
	UPROPERTY(EditAnywhere) float AcquireRange = 1000.f; // auto-target distance
	UPROPERTY(EditAnywhere) bool bCanHarvest = false;
	UPROPERTY(EditAnywhere) float HarvestCapacity = 100.f;
	UPROPERTY(EditAnywhere) float HarvestRate = 40.f; // supplies per second
	UPROPERTY(EditAnywhere) float VisualScale = 1.f;
};

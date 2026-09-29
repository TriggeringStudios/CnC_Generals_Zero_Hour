#pragma once
#include "CoreMinimal.h"
#include "RTSTypes.generated.h"

// Original logic ran at a fixed 30 Hz (LOGICFRAMES_PER_SECOND, GameCommon.h).
// Keep the sim on a fixed step so gameplay matches and lockstep stays possible.
constexpr int32 RTS_LOGIC_FPS = 30;

/** Mirrors GameMessage command types issued by the control bar / mouse (MessageStream). */
UENUM(BlueprintType)
enum class ERTSCommandType : uint8
{
	Move, AttackMove, Attack, Stop, Guard, Scatter,
	Build, Dock, Repair, Garrison, Evacuate, SetRallyPoint,
	QueueUnit, CancelUnit, UseSpecialPower
};

USTRUCT(BlueprintType)
struct FRTSCommand
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadWrite) ERTSCommandType Type = ERTSCommandType::Move;
	UPROPERTY(BlueprintReadWrite) FVector Location = FVector::ZeroVector;
	UPROPERTY(BlueprintReadWrite) TWeakObjectPtr<AActor> Target;
	UPROPERTY(BlueprintReadWrite) FName Argument; // e.g. unit type to queue, special power name
	UPROPERTY(BlueprintReadWrite) bool bQueued = false; // shift-queue
};

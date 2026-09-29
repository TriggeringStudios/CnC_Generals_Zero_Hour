#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RTSEconomyComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMoneyChanged, int32, NewMoney, int32, Delta);

/** Per-player money (Money.cpp in the original). Attach to the PlayerState. */
UCLASS(ClassGroup=RTS, meta=(BlueprintSpawnableComponent))
class GENERALSRTS_API URTSEconomyComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintAssignable) FOnMoneyChanged OnMoneyChanged;
	UFUNCTION(BlueprintCallable) int32 GetMoney() const { return Money; }
	UFUNCTION(BlueprintCallable) void Deposit(int32 Amount);
	UFUNCTION(BlueprintCallable) bool TryWithdraw(int32 Amount);
private:
	UPROPERTY(VisibleAnywhere) int32 Money = 0;
};

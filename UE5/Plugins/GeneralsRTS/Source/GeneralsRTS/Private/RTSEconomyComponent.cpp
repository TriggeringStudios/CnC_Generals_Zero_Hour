#include "RTSEconomyComponent.h"

void URTSEconomyComponent::Deposit(int32 Amount)
{
	if (Amount <= 0) return;
	Money += Amount;
	OnMoneyChanged.Broadcast(Money, Amount);
}

bool URTSEconomyComponent::TryWithdraw(int32 Amount)
{
	if (Amount < 0 || Money < Amount) return false;
	Money -= Amount;
	OnMoneyChanged.Broadcast(Money, -Amount);
	return true;
}

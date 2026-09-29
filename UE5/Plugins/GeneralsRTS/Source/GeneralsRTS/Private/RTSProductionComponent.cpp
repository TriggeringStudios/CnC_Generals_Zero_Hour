#include "RTSProductionComponent.h"
#include "RTSEconomyComponent.h"
#include "RTSTypes.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/Pawn.h"

const FRTSProductionEntry* URTSProductionComponent::Find(FName ObjectName) const
{
	return Definition ? Definition->Produces.FindByPredicate(
		[&](const FRTSProductionEntry& E) { return E.ObjectName == ObjectName; }) : nullptr;
}

static URTSEconomyComponent* GetEconomy(const AActor* Owner)
{
	const APawn* Pawn = Cast<APawn>(Owner);
	const APlayerState* PS = Pawn ? Pawn->GetPlayerState() : nullptr;
	return PS ? PS->FindComponentByClass<URTSEconomyComponent>() : nullptr;
}

bool URTSProductionComponent::QueueUnit(FName ObjectName)
{
	const FRTSProductionEntry* Entry = Find(ObjectName);
	URTSEconomyComponent* Econ = GetEconomy(GetOwner());
	if (!Entry || !Econ || Queue.Num() >= MaxQueue || !Econ->TryWithdraw(Entry->Cost)) return false;
	Queue.Add({*Entry, FMath::CeilToInt(Entry->BuildTimeSeconds * RTS_LOGIC_FPS)});
	return true;
}

bool URTSProductionComponent::CancelLast()
{
	if (Queue.IsEmpty()) return false;
	if (URTSEconomyComponent* Econ = GetEconomy(GetOwner())) Econ->Deposit(Queue.Last().Entry.Cost);
	Queue.Pop();
	return true;
}

void URTSProductionComponent::SimTick()
{
	if (Queue.IsEmpty()) return;
	if (--Queue[0].FramesLeft <= 0)
	{
		const FRTSProductionEntry Done = Queue[0].Entry;
		Queue.RemoveAt(0);
		OnUnitFinished(Done);
	}
}

void URTSProductionComponent::OnUnitFinished(const FRTSProductionEntry& Entry)
{
	// TODO: spawn the unit (Mass entity for infantry, actor for heroes/structures) at the exit
	// point and issue a Move to RallyPoint.
}

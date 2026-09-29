#include "RTSProductionComponent.h"
#include "RTSEconomyComponent.h"
#include "RTSTeamInfo.h"
#include "RTSSimSubsystem.h"
#include "RTSBuilding.h"
#include "RTSUnit.h"

static URTSEconomyComponent* GetEconomy(const ARTSBuilding* B)
{
	URTSSimSubsystem* Sim = B ? B->GetWorld()->GetSubsystem<URTSSimSubsystem>() : nullptr;
	ARTSTeamInfo* Team = Sim ? Sim->GetTeam(B->TeamId) : nullptr;
	return Team ? Team->Economy.Get() : nullptr;
}

bool URTSProductionComponent::QueueUnit(FName ObjectName)
{
	if (!Definition || Queue.Num() >= MaxQueue) return false;
	const FRTSProductionEntry* Entry = Definition->Produces.FindByPredicate(
		[&](const FRTSProductionEntry& E) { return E.ObjectName == ObjectName; });
	URTSEconomyComponent* Econ = GetEconomy(Cast<ARTSBuilding>(GetOwner()));
	if (!Entry || !Econ || !Econ->TryWithdraw(Entry->Cost)) return false;
	const int32 Frames = FMath::Max(1, FMath::CeilToInt(Entry->BuildTimeSeconds * RTS_LOGIC_FPS));
	Queue.Add({*Entry, Frames, Frames});
	return true;
}

bool URTSProductionComponent::CancelLast()
{
	if (Queue.IsEmpty()) return false;
	if (URTSEconomyComponent* Econ = GetEconomy(Cast<ARTSBuilding>(GetOwner())))
		Econ->Deposit(Queue.Last().Entry.Cost);
	Queue.Pop();
	return true;
}

int32 URTSProductionComponent::CountQueued(ERTSUnitRole Role) const
{
	int32 N = 0;
	for (const FJob& J : Queue) N += (J.Entry.Role == Role);
	return N;
}

float URTSProductionComponent::GetProgress01() const
{
	return Queue.IsEmpty() ? 0.f : 1.f - float(Queue[0].FramesLeft) / float(Queue[0].TotalFrames);
}

void URTSProductionComponent::SimTick()
{
	if (Queue.IsEmpty()) return;
	if (--Queue[0].FramesLeft <= 0)
	{
		const FRTSProductionEntry Done = Queue[0].Entry;
		Queue.RemoveAt(0);
		Finish(Done);
	}
}

ARTSUnit* URTSProductionComponent::SpawnUnit(UWorld* World, const FRTSProductionEntry& Entry, int32 TeamId, const FVector& Location)
{
	UClass* Class = Entry.ActorClass ? Entry.ActorClass.Get() : ARTSUnit::StaticClass();
	const FTransform T(FRotator::ZeroRotator, Location);
	ARTSUnit* U = World->SpawnActorDeferred<ARTSUnit>(Class, T, nullptr, nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!U) return nullptr;
	U->TeamId = TeamId;
	U->ObjectName = Entry.ObjectName;
	U->Role = Entry.Role;
	U->ApplyStats(Entry.Stats);
	U->FinishSpawning(T);
	return U;
}

void URTSProductionComponent::Finish(const FRTSProductionEntry& Entry)
{
	ARTSBuilding* B = Cast<ARTSBuilding>(GetOwner());
	if (!B) return;
	ARTSUnit* U = SpawnUnit(GetWorld(), Entry, B->TeamId, B->GetActorLocation() + B->SpawnOffset);
	if (!U) return;

	FRTSCommand Cmd;
	if (bHasRally)
	{
		Cmd.Type = ERTSCommandType::Move;
		Cmd.Location = RallyPoint;
		U->IssueCommand(Cmd);
	}
	else if (Entry.Role == ERTSUnitRole::Worker)
	{
		// Workers start harvesting on their own, like supply trucks leaving a supply center.
		Cmd.Type = ERTSCommandType::Gather;
		U->IssueCommand(Cmd);
	}
}

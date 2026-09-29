#include "RTSSimSubsystem.h"
#include "RTSTypes.h"
#include "RTSTargetable.h"
#include "RTSTeamInfo.h"
#include "RTSResourceNode.h"
#include "RTSBuilding.h"

void URTSSimSubsystem::Tick(float DeltaTime)
{
	Accumulator += DeltaTime;
	int32 Steps = 0;
	while (Accumulator >= RTS_LOGIC_DT && Steps < 4) // cap catch-up so a hitch can't spiral
	{
		Accumulator -= RTS_LOGIC_DT;
		++Frame;
		++Steps;
		OnSimTick.Broadcast();
	}
	if (Steps == 4) Accumulator = 0.f;
	Entities.RemoveAll([](const TWeakObjectPtr<AActor>& P) { return !P.IsValid(); });
	Nodes.RemoveAll([](const TWeakObjectPtr<ARTSResourceNode>& P) { return !P.IsValid(); });
}

void URTSSimSubsystem::RegisterTeam(ARTSTeamInfo* T) { Teams.Add(T->TeamId, T); }

ARTSTeamInfo* URTSSimSubsystem::GetTeam(int32 TeamId) const
{
	const TWeakObjectPtr<ARTSTeamInfo>* T = Teams.Find(TeamId);
	return T ? T->Get() : nullptr;
}

void URTSSimSubsystem::GetTeamActors(int32 TeamId, TArray<AActor*>& Out) const
{
	for (const TWeakObjectPtr<AActor>& P : Entities)
	{
		const IRTSTargetable* T = Cast<IRTSTargetable>(P.Get());
		if (T && T->GetTeamId() == TeamId && T->IsAlive()) Out.Add(P.Get());
	}
}

AActor* URTSSimSubsystem::FindNearestEnemy(const FVector& From, int32 MyTeam, float MaxRange) const
{
	AActor* Best = nullptr;
	float BestSq = FMath::Square(MaxRange);
	for (const TWeakObjectPtr<AActor>& P : Entities)
	{
		const IRTSTargetable* T = Cast<IRTSTargetable>(P.Get());
		if (!T || T->GetTeamId() == MyTeam || !T->IsAlive()) continue;
		const float D = FVector::DistSquared2D(From, P->GetActorLocation());
		if (D < BestSq) { BestSq = D; Best = P.Get(); }
	}
	return Best;
}

ARTSResourceNode* URTSSimSubsystem::FindNearestNode(const FVector& From) const
{
	ARTSResourceNode* Best = nullptr;
	float BestSq = TNumericLimits<float>::Max();
	for (const TWeakObjectPtr<ARTSResourceNode>& N : Nodes)
	{
		if (!N.IsValid() || N->IsDepleted()) continue;
		const float D = FVector::DistSquared2D(From, N->GetActorLocation());
		if (D < BestSq) { BestSq = D; Best = N.Get(); }
	}
	return Best;
}

ARTSBuilding* URTSSimSubsystem::FindNearestDepot(const FVector& From, int32 TeamId) const
{
	ARTSBuilding* Best = nullptr;
	float BestSq = TNumericLimits<float>::Max();
	for (const TWeakObjectPtr<AActor>& P : Entities)
	{
		ARTSBuilding* B = Cast<ARTSBuilding>(P.Get());
		if (!B || !B->bIsDepot || B->TeamId != TeamId || !B->IsAlive()) continue;
		const float D = FVector::DistSquared2D(From, B->GetActorLocation());
		if (D < BestSq) { BestSq = D; Best = B; }
	}
	return Best;
}

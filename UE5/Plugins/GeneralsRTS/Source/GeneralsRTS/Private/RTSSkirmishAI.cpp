#include "RTSSkirmishAI.h"
#include "RTSTeamInfo.h"
#include "RTSEconomyComponent.h"
#include "RTSSimSubsystem.h"
#include "RTSUnit.h"
#include "RTSBuilding.h"
#include "RTSProductionComponent.h"

void URTSSkirmishAI::BeginPlay()
{
	Super::BeginPlay();
	if (URTSSimSubsystem* Sim = GetWorld()->GetSubsystem<URTSSimSubsystem>())
		SimHandle = Sim->OnSimTick.AddUObject(this, &URTSSkirmishAI::OnSimFrame);
}

void URTSSkirmishAI::EndPlay(const EEndPlayReason::Type Reason)
{
	if (UWorld* W = GetWorld())
		if (URTSSimSubsystem* Sim = W->GetSubsystem<URTSSimSubsystem>()) Sim->OnSimTick.Remove(SimHandle);
	Super::EndPlay(Reason);
}

void URTSSkirmishAI::OnSimFrame()
{
	const ARTSTeamInfo* Team = Cast<ARTSTeamInfo>(GetOwner());
	URTSSimSubsystem* Sim = GetWorld()->GetSubsystem<URTSSimSubsystem>();
	// Stagger teams so two AIs don't all think on the same frame.
	if (Team && Team->bIsAI && Sim && ((Sim->GetFrame() + Team->TeamId * 7) % ThinkIntervalFrames) == 0) Think();
}

void URTSSkirmishAI::Think()
{
	ARTSTeamInfo* Team = Cast<ARTSTeamInfo>(GetOwner());
	URTSSimSubsystem* Sim = GetWorld()->GetSubsystem<URTSSimSubsystem>();

	TArray<AActor*> Mine;
	Sim->GetTeamActors(Team->TeamId, Mine);
	TArray<ARTSUnit*> Workers, Army;
	TArray<ARTSBuilding*> Factories, Buildings;
	for (AActor* A : Mine)
	{
		if (ARTSUnit* U = Cast<ARTSUnit>(A)) (U->Role == ERTSUnitRole::Worker ? Workers : Army).Add(U);
		else if (ARTSBuilding* B = Cast<ARTSBuilding>(A))
		{
			Buildings.Add(B);
			if (B->Production && B->Production->Definition) Factories.Add(B);
		}
	}
	if (Buildings.IsEmpty() && Army.IsEmpty() && Workers.IsEmpty()) return;
	const FVector Home = Buildings.Num() ? Buildings[0]->GetActorLocation()
		: (Army.Num() ? Army[0]->GetActorLocation() : Workers[0]->GetActorLocation());

	// 1. Economy: idle workers go mine.
	for (ARTSUnit* W : Workers)
	{
		if (W->HasOrder()) continue;
		FRTSCommand C;
		C.Type = ERTSCommandType::Gather;
		W->IssueCommand(C);
	}

	// 2. Production: workers until we have enough, then rotate through combat units.
	int32 QueuedWorkers = 0;
	for (ARTSBuilding* F : Factories) QueuedWorkers += F->Production->CountQueued(ERTSUnitRole::Worker);
	bool bNeedWorker = Workers.Num() + QueuedWorkers < DesiredWorkers;
	for (ARTSBuilding* F : Factories)
	{
		URTSProductionComponent* P = F->Production;
		if (P->GetQueueCount() >= 2) continue;
		TArray<const FRTSProductionEntry*> Options;
		for (const FRTSProductionEntry& E : P->Definition->Produces)
			if ((E.Role == ERTSUnitRole::Worker) == bNeedWorker) Options.Add(&E);
		if (Options.IsEmpty()) continue;
		if (P->QueueUnit(Options[ArmyCycle % Options.Num()]->ObjectName))
		{
			if (bNeedWorker) bNeedWorker = false; else ++ArmyCycle;
		}
	}

	// 3. Defense: if a building was hit recently, send everyone to the attacker.
	const int32 Frame = Sim->GetFrame();
	for (ARTSBuilding* B : Buildings)
	{
		if (Frame - B->LastDamagedFrame < 3 * RTS_LOGIC_FPS)
		{
			for (ARTSUnit* U : Army)
			{
				FRTSCommand C;
				C.Type = ERTSCommandType::AttackMove;
				C.Location = B->LastAttackerLocation;
				U->IssueCommand(C);
			}
			return;
		}
	}

	// 4. Offense: launch a wave when enough idle army is massed; keep idle survivors pressing.
	TArray<ARTSUnit*> IdleArmy;
	for (ARTSUnit* U : Army) if (!U->HasOrder()) IdleArmy.Add(U);
	if (bWaveActive && Army.Num() <= 2)
	{
		bWaveActive = false; // wave broken; regroup and rebuild
		for (ARTSUnit* U : Army)
		{
			FRTSCommand C;
			C.Type = ERTSCommandType::Move;
			C.Location = Home;
			U->IssueCommand(C);
		}
		return;
	}
	if (!bWaveActive && IdleArmy.Num() >= WaveSize)
	{
		bWaveActive = true;
		WaveSize += WaveGrowth;
	}
	if (bWaveActive)
	{
		for (ARTSUnit* U : IdleArmy)
		{
			AActor* Target = Sim->FindNearestEnemy(U->GetActorLocation(), Team->TeamId, 1.0e9f);
			if (!Target) continue;
			FRTSCommand C;
			C.Type = ERTSCommandType::AttackMove;
			C.Location = Target->GetActorLocation();
			U->IssueCommand(C);
		}
	}
}

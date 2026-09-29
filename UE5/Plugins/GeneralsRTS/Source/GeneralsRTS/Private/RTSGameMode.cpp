#include "RTSGameMode.h"
#include "RTSCameraPawn.h"
#include "RTSPlayerController.h"
#include "RTSHUD.h"
#include "RTSTeamInfo.h"
#include "RTSBuilding.h"
#include "RTSUnit.h"
#include "RTSResourceNode.h"
#include "RTSObjectDefinition.h"
#include "RTSProductionComponent.h"
#include "GameFramework/PlayerStart.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Math/RandomStream.h"

ARTSGameMode::ARTSGameMode()
{
	DefaultPawnClass = ARTSCameraPawn::StaticClass();
	PlayerControllerClass = ARTSPlayerController::StaticClass();
	HUDClass = ARTSHUD::StaticClass();
}

void ARTSGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
	// The camera pawn needs a start spot; put it over the human base.
	GetWorld()->SpawnActor<APlayerStart>(FVector(-BaseSeparation * 0.5f, 0.f, 200.f), FRotator::ZeroRotator);
}

void ARTSGameMode::StartPlay()
{
	Super::StartPlay(); // BeginPlay first so everything spawned below initializes immediately
	SetupSkirmish();
}

void ARTSGameMode::SetupSkirmish()
{
	UWorld* World = GetWorld();

	// Floor.
	if (UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")))
	{
		AStaticMeshActor* Floor = World->SpawnActor<AStaticMeshActor>(FVector(0, 0, -50.f), FRotator::ZeroRotator);
		Floor->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);
		Floor->GetStaticMeshComponent()->SetStaticMesh(Cube);
		Floor->SetActorScale3D(FVector(MapHalfExtent * 2.f / 100.f, MapHalfExtent * 2.f / 100.f, 1.f));
	}

	// Data-driven unit roster. Adding a unit = adding an entry here (or in a data asset).
	HQDefinition = NewObject<URTSObjectDefinition>(this);
	HQDefinition->TemplateName = TEXT("CommandCenter");
	{
		FRTSProductionEntry Harvester;
		Harvester.ObjectName = TEXT("Harvester");
		Harvester.Cost = 200; Harvester.BuildTimeSeconds = 5.f; Harvester.Role = ERTSUnitRole::Worker;
		Harvester.Stats.MaxHealth = 300.f; Harvester.Stats.MoveSpeed = 500.f; Harvester.Stats.bCanHarvest = true;
		Harvester.Stats.HarvestCapacity = 200.f; Harvester.Stats.HarvestRate = 60.f; Harvester.Stats.VisualScale = 1.2f;

		FRTSProductionEntry Rifleman;
		Rifleman.ObjectName = TEXT("Rifleman");
		Rifleman.Cost = 100; Rifleman.BuildTimeSeconds = 4.f;
		Rifleman.Stats.MaxHealth = 120.f; Rifleman.Stats.MoveSpeed = 450.f; Rifleman.Stats.AttackRange = 700.f;
		Rifleman.Stats.AttackDamage = 12.f; Rifleman.Stats.AttackCooldownSec = 1.f; Rifleman.Stats.AcquireRange = 1200.f;

		FRTSProductionEntry Tank;
		Tank.ObjectName = TEXT("Tank");
		Tank.Cost = 400; Tank.BuildTimeSeconds = 10.f;
		Tank.Stats.MaxHealth = 600.f; Tank.Stats.MoveSpeed = 600.f; Tank.Stats.AttackRange = 1100.f;
		Tank.Stats.AttackDamage = 45.f; Tank.Stats.AttackCooldownSec = 1.5f; Tank.Stats.AcquireRange = 1500.f;
		Tank.Stats.VisualScale = 1.6f;

		HQDefinition->Produces = { Harvester, Rifleman, Tank };
	}

	FRandomStream Rand(1337);
	for (int32 Team = 0; Team < 2; ++Team)
	{
		const FVector Base((Team == 0 ? -1.f : 1.f) * BaseSeparation * 0.5f, 0.f, 0.f);

		ARTSTeamInfo* Info = World->SpawnActorDeferred<ARTSTeamInfo>(ARTSTeamInfo::StaticClass(), FTransform::Identity);
		Info->TeamId = Team;
		Info->bIsAI = (Team != 0);
		Info->TeamColor = Team == 0 ? FLinearColor(0.1f, 0.3f, 1.f) : FLinearColor(1.f, 0.1f, 0.1f);
		Info->StartingMoney = 1000;
		Info->FinishSpawning(FTransform::Identity);

		const FTransform HQT(FRotator::ZeroRotator, Base + FVector(0, 0, 100.f));
		ARTSBuilding* HQ = World->SpawnActorDeferred<ARTSBuilding>(ARTSBuilding::StaticClass(), HQT);
		HQ->TeamId = Team;
		HQ->bIsDepot = true;
		HQ->Production->Definition = HQDefinition;
		HQ->FinishSpawning(HQT);

		// Two starting harvesters.
		for (int32 i = 0; i < 2; ++i)
		{
			ARTSUnit* U = URTSProductionComponent::SpawnUnit(World, HQDefinition->Produces[0], Team,
				Base + FVector(-300.f, (i ? 1 : -1) * 150.f, 60.f));
			FRTSCommand C; C.Type = ERTSCommandType::Gather;
			if (U) U->IssueCommand(C);
		}

		// Supply piles in a ring around the base.
		for (int32 i = 0; i < SupplyPilesPerBase; ++i)
		{
			const float Ang = (2.f * PI * i / SupplyPilesPerBase) + Rand.FRandRange(-0.2f, 0.2f);
			const float R = Rand.FRandRange(1800.f, 3200.f);
			World->SpawnActor<ARTSResourceNode>(Base + FVector(FMath::Cos(Ang) * R, FMath::Sin(Ang) * R, 50.f), FRotator::ZeroRotator);
		}
	}
}

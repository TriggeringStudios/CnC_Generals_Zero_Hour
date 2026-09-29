#include "RTSBuilding.h"
#include "RTSProductionComponent.h"
#include "RTSSimSubsystem.h"
#include "RTSTeamInfo.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "DrawDebugHelpers.h"

ARTSBuilding::ARTSBuilding()
{
	PrimaryActorTick.bCanEverTick = true;
	Box = CreateDefaultSubobject<UBoxComponent>(TEXT("Box"));
	Box->SetBoxExtent(FVector(200.f, 200.f, 100.f));
	Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Box->SetCollisionResponseToAllChannels(ECR_Ignore);
	Box->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	SetRootComponent(Box);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Box);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (Cube.Succeeded()) Mesh->SetStaticMesh(Cube.Object);
	Mesh->SetRelativeScale3D(FVector(4.f, 4.f, 2.f));

	Production = CreateDefaultSubobject<URTSProductionComponent>(TEXT("Production"));
	Health = MaxHealth;
}

void ARTSBuilding::BeginPlay()
{
	Super::BeginPlay();
	Health = MaxHealth;
	if (URTSSimSubsystem* Sim = GetWorld()->GetSubsystem<URTSSimSubsystem>())
	{
		Sim->RegisterEntity(this);
		SimHandle = Sim->OnSimTick.AddUObject(this, &ARTSBuilding::SimTick);
		if (ARTSTeamInfo* T = Sim->GetTeam(TeamId))
			if (UMaterialInstanceDynamic* M = Mesh->CreateAndSetMaterialInstanceDynamic(0))
				M->SetVectorParameterValue(TEXT("Color"), T->TeamColor);
	}
}

void ARTSBuilding::EndPlay(const EEndPlayReason::Type Reason)
{
	if (UWorld* W = GetWorld())
		if (URTSSimSubsystem* Sim = W->GetSubsystem<URTSSimSubsystem>())
		{
			Sim->OnSimTick.Remove(SimHandle);
			Sim->UnregisterEntity(this);
		}
	Super::EndPlay(Reason);
}

void ARTSBuilding::SimTick()
{
	if (IsAlive() && Production) Production->SimTick();
}

void ARTSBuilding::ReceiveDamage(float Amount, AActor* From)
{
	if (!IsAlive()) return;
	Health -= Amount;
	if (URTSSimSubsystem* Sim = GetWorld()->GetSubsystem<URTSSimSubsystem>()) LastDamagedFrame = Sim->GetFrame();
	if (From) LastAttackerLocation = From->GetActorLocation();
	if (Health <= 0.f) Destroy();
}

void ARTSBuilding::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
#if ENABLE_DRAW_DEBUG
	const FVector C = GetActorLocation();
	if (bSelected)
		DrawDebugBox(GetWorld(), C, FVector(210.f, 210.f, 110.f), FColor::Green, false, -1.f, 0, 4.f);
	const float Pct = FMath::Clamp(Health / MaxHealth, 0.f, 1.f);
	const FVector L = C + FVector(0.f, -200.f, 260.f), R = C + FVector(0.f, 200.f, 260.f);
	DrawDebugLine(GetWorld(), L, R, FColor::Red, false, -1.f, 0, 10.f);
	DrawDebugLine(GetWorld(), L, FMath::Lerp(L, R, Pct), FColor::Green, false, -1.f, 0, 10.f);
#endif
}

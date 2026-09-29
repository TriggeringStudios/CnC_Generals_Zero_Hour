#include "RTSResourceNode.h"
#include "RTSSimSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

ARTSResourceNode::ARTSResourceNode()
{
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (Cube.Succeeded()) Mesh->SetStaticMesh(Cube.Object);
	Mesh->SetRelativeScale3D(FVector(2.f, 2.f, 1.f));
	Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	Mesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
}

void ARTSResourceNode::BeginPlay()
{
	Super::BeginPlay();
	if (UMaterialInstanceDynamic* M = Mesh->CreateAndSetMaterialInstanceDynamic(0))
		M->SetVectorParameterValue(TEXT("Color"), FLinearColor(1.f, 0.8f, 0.1f));
	if (URTSSimSubsystem* Sim = GetWorld()->GetSubsystem<URTSSimSubsystem>()) Sim->RegisterNode(this);
}

float ARTSResourceNode::Extract(float Amount)
{
	const float Taken = FMath::Min(Amount, Remaining);
	Remaining -= Taken;
	if (Remaining <= 0.f) Destroy();
	return Taken;
}

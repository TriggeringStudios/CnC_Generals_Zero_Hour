#include "RTSTeamInfo.h"
#include "RTSEconomyComponent.h"
#include "RTSSkirmishAI.h"
#include "RTSSimSubsystem.h"

ARTSTeamInfo::ARTSTeamInfo()
{
	Economy = CreateDefaultSubobject<URTSEconomyComponent>(TEXT("Economy"));
	AI = CreateDefaultSubobject<URTSSkirmishAI>(TEXT("SkirmishAI"));
}

void ARTSTeamInfo::BeginPlay()
{
	Super::BeginPlay();
	Economy->Deposit(StartingMoney);
	if (URTSSimSubsystem* Sim = GetWorld()->GetSubsystem<URTSSimSubsystem>()) Sim->RegisterTeam(this);
}

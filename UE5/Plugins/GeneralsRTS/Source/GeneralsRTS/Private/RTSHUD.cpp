#include "RTSHUD.h"
#include "RTSPlayerController.h"
#include "RTSSimSubsystem.h"
#include "RTSTeamInfo.h"
#include "RTSEconomyComponent.h"
#include "RTSBuilding.h"
#include "RTSUnit.h"
#include "RTSProductionComponent.h"

void ARTSHUD::DrawHUD()
{
	Super::DrawHUD();
	ARTSPlayerController* PC = Cast<ARTSPlayerController>(GetOwningPlayerController());
	URTSSimSubsystem* Sim = GetWorld()->GetSubsystem<URTSSimSubsystem>();
	if (!PC || !Sim) return;

	FVector2D Min, Max;
	if (PC->GetDragRect(Min, Max))
		DrawRect(FLinearColor(0.f, 1.f, 0.f, 0.15f), Min.X, Min.Y, Max.X - Min.X, Max.Y - Min.Y);

	float Y = 20.f;
	auto Line = [&](const FString& S, FLinearColor C = FLinearColor::White) { DrawText(S, C, 20.f, Y); Y += 20.f; };

	if (const ARTSTeamInfo* Team = Sim->GetTeam(PC->TeamId))
		Line(FString::Printf(TEXT("Supplies: %d"), Team->Economy->GetMoney()), FLinearColor(1.f, 0.85f, 0.2f));

	int32 Units = 0;
	for (const auto& P : PC->GetSelection())
	{
		if (Cast<ARTSUnit>(P.Get())) ++Units;
		if (const ARTSBuilding* B = Cast<ARTSBuilding>(P.Get()))
		{
			const URTSProductionComponent* Pr = B->Production;
			Line(FString::Printf(TEXT("Factory: %d queued  building %s  %d%%"), Pr->GetQueueCount(),
				*Pr->GetFrontName().ToString(), FMath::RoundToInt(Pr->GetProgress01() * 100.f)));
			if (Pr->Definition)
			{
				const TCHAR* Keys = TEXT("ZXCV");
				for (int32 i = 0; i < 4 && i < Pr->Definition->Produces.Num(); ++i)
					Line(FString::Printf(TEXT("  [%c] %s  $%d"), Keys[i],
						*Pr->Definition->Produces[i].ObjectName.ToString(), Pr->Definition->Produces[i].Cost));
			}
		}
	}
	if (Units) Line(FString::Printf(TEXT("Selected units: %d"), Units));
	if (PC->IsAttackMoveArmed()) Line(TEXT("ATTACK-MOVE: left click a target location"), FLinearColor::Red);

	// Win / lose: a side with no structures has lost.
	auto Structures = [&](int32 Team) {
		TArray<AActor*> A; Sim->GetTeamActors(Team, A);
		int32 N = 0; for (AActor* X : A) N += Cast<ARTSBuilding>(X) != nullptr; return N; };
	const float CX = Canvas->SizeX * 0.5f - 60.f, CY = Canvas->SizeY * 0.4f;
	if (Structures(PC->TeamId) == 0) DrawText(TEXT("DEFEAT"), FLinearColor::Red, CX, CY, nullptr, 3.f);
	else if (Structures(1) == 0) DrawText(TEXT("VICTORY"), FLinearColor::Green, CX, CY, nullptr, 3.f);
}

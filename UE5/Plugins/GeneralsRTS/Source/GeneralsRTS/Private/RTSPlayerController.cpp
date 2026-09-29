#include "RTSPlayerController.h"
#include "RTSCameraPawn.h"
#include "RTSSimSubsystem.h"
#include "RTSTargetable.h"
#include "RTSUnit.h"
#include "RTSBuilding.h"
#include "RTSResourceNode.h"
#include "RTSProductionComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Engine/LocalPlayer.h"

void ARTSPlayerController::BeginPlay()
{
	Super::BeginPlay();
	bShowMouseCursor = true;
	SetInputMode(FInputModeGameAndUI().SetHideCursorDuringCapture(false));
}

void ARTSPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	UpdateCamera(DeltaTime);
	HandleMouse();
	HandleKeys();
}

// ---------------------------------------------------------------- camera

void ARTSPlayerController::UpdateCamera(float DT)
{
	ARTSCameraPawn* Cam = Cast<ARTSCameraPawn>(GetPawn());
	if (!Cam) return;
	FVector2D Dir = FVector2D::ZeroVector;
	if (IsInputKeyDown(EKeys::Up)) Dir.X += 1;
	if (IsInputKeyDown(EKeys::Down)) Dir.X -= 1;
	if (IsInputKeyDown(EKeys::Right)) Dir.Y += 1;
	if (IsInputKeyDown(EKeys::Left)) Dir.Y -= 1;

	float MX, MY;
	int32 VW, VH;
	GetViewportSize(VW, VH);
	if (GetMousePosition(MX, MY) && VW > 0)
	{
		if (MX <= EdgeScrollMargin) Dir.Y -= 1;
		if (MX >= VW - EdgeScrollMargin) Dir.Y += 1;
		if (MY <= EdgeScrollMargin) Dir.X += 1;
		if (MY >= VH - EdgeScrollMargin) Dir.X -= 1;
	}
	const float Zoom = Cam->Arm->TargetArmLength / 3000.f;
	if (!Dir.IsNearlyZero())
		Cam->AddActorWorldOffset(FVector(Dir.X, Dir.Y, 0.f).GetSafeNormal() * PanSpeed * Zoom * DT);

	if (WasInputKeyJustPressed(EKeys::MouseScrollUp))
		Cam->Arm->TargetArmLength = FMath::Clamp(Cam->Arm->TargetArmLength - 400.f, 800.f, 12000.f);
	if (WasInputKeyJustPressed(EKeys::MouseScrollDown))
		Cam->Arm->TargetArmLength = FMath::Clamp(Cam->Arm->TargetArmLength + 400.f, 800.f, 12000.f);
}

// ---------------------------------------------------------------- selection

bool ARTSPlayerController::GetDragRect(FVector2D& Min, FVector2D& Max) const
{
	if (!bDragging || (DragNow - DragStart).Size() < 8.0) return false;
	Min = FVector2D(FMath::Min(DragStart.X, DragNow.X), FMath::Min(DragStart.Y, DragNow.Y));
	Max = FVector2D(FMath::Max(DragStart.X, DragNow.X), FMath::Max(DragStart.Y, DragNow.Y));
	return true;
}

bool ARTSPlayerController::OwnedAndAlive(AActor* A) const
{
	const IRTSTargetable* T = Cast<IRTSTargetable>(A);
	return T && T->GetTeamId() == TeamId && T->IsAlive();
}

void ARTSPlayerController::ClearSelection()
{
	for (auto& P : Selection)
		if (IRTSTargetable* T = Cast<IRTSTargetable>(P.Get())) T->SetSelected(false);
	Selection.Reset();
}

void ARTSPlayerController::AddToSelection(AActor* A)
{
	if (!OwnedAndAlive(A) || Selection.Contains(A)) return;
	Cast<IRTSTargetable>(A)->SetSelected(true);
	Selection.Add(A);
}

bool ARTSPlayerController::GroundUnderCursor(FVector& Out) const
{
	FHitResult H;
	if (!GetHitResultUnderCursor(ECC_Visibility, false, H) || !H.bBlockingHit) return false;
	Out = H.ImpactPoint;
	return true;
}

void ARTSPlayerController::HandleMouse()
{
	float X, Y;
	if (GetMousePosition(X, Y)) DragNow = FVector2D(X, Y);
	if (WasInputKeyJustPressed(EKeys::LeftMouseButton)) { bDragging = true; DragStart = DragNow; }
	if (bDragging && WasInputKeyJustReleased(EKeys::LeftMouseButton)) { bDragging = false; OnLeftReleased(); }
	if (WasInputKeyJustPressed(EKeys::RightMouseButton)) { bAttackMoveArmed = false; OnRightClick(); }
}

void ARTSPlayerController::OnLeftReleased()
{
	const bool bShift = IsInputKeyDown(EKeys::LeftShift);

	if (bAttackMoveArmed)
	{
		bAttackMoveArmed = false;
		FVector P;
		if (GroundUnderCursor(P))
		{
			FRTSCommand C; C.Type = ERTSCommandType::AttackMove; C.Location = P; C.bQueued = bShift;
			IssueToUnits(C, true);
		}
		return;
	}

	if (!bShift) ClearSelection();
	FVector2D Min, Max;
	if (!GetDragRect(Min, Max))
	{
		FHitResult H;
		if (GetHitResultUnderCursor(ECC_Visibility, false, H)) AddToSelection(H.GetActor());
		return;
	}
	// Box select: units only (Generals selects units over structures when dragging).
	URTSSimSubsystem* Sim = GetWorld()->GetSubsystem<URTSSimSubsystem>();
	TArray<AActor*> Mine;
	Sim->GetTeamActors(TeamId, Mine);
	for (AActor* A : Mine)
	{
		FVector2D S;
		if (Cast<ARTSUnit>(A) && ProjectWorldLocationToScreen(A->GetActorLocation(), S) &&
			S.X >= Min.X && S.X <= Max.X && S.Y >= Min.Y && S.Y <= Max.Y)
			AddToSelection(A);
	}
}

// ---------------------------------------------------------------- orders

void ARTSPlayerController::IssueToUnits(const FRTSCommand& Cmd, bool bFormation)
{
	TArray<ARTSUnit*> Units;
	for (auto& P : Selection) if (ARTSUnit* U = Cast<ARTSUnit>(P.Get())) if (U->IsAlive()) Units.Add(U);
	const int32 Cols = FMath::Max(1, FMath::CeilToInt(FMath::Sqrt(float(Units.Num()))));
	for (int32 i = 0; i < Units.Num(); ++i)
	{
		FRTSCommand C = Cmd;
		if (bFormation && Units.Num() > 1)
		{
			const int32 Row = i / Cols, Col = i % Cols;
			C.Location += FVector((Row - (Units.Num() / Cols) * 0.5f) * 130.f, (Col - (Cols - 1) * 0.5f) * 130.f, 0.f);
		}
		Units[i]->IssueCommand(C);
	}
}

void ARTSPlayerController::OnRightClick()
{
	FHitResult H;
	if (!GetHitResultUnderCursor(ECC_Visibility, false, H) || !H.bBlockingHit) return;
	AActor* Hit = H.GetActor();
	const bool bQueued = IsInputKeyDown(EKeys::LeftShift);

	// Selected factories: right click sets the rally point.
	for (auto& P : Selection)
		if (ARTSBuilding* B = Cast<ARTSBuilding>(P.Get()))
		{
			B->Production->RallyPoint = H.ImpactPoint;
			B->Production->bHasRally = true;
		}

	const IRTSTargetable* T = Cast<IRTSTargetable>(Hit);
	FRTSCommand Cmd;
	Cmd.bQueued = bQueued;
	if (T && T->GetTeamId() != TeamId && T->IsAlive())
	{
		// Attack: only units that can fight get the attack order; the rest just move there.
		for (auto& P : Selection)
		{
			ARTSUnit* U = Cast<ARTSUnit>(P.Get());
			if (!U) continue;
			FRTSCommand C = Cmd;
			if (U->CanAttack()) { C.Type = ERTSCommandType::Attack; C.Target = Hit; }
			else { C.Type = ERTSCommandType::Move; C.Location = H.ImpactPoint; }
			U->IssueCommand(C);
		}
	}
	else if (ARTSResourceNode* N = Cast<ARTSResourceNode>(Hit))
	{
		for (auto& P : Selection)
		{
			ARTSUnit* U = Cast<ARTSUnit>(P.Get());
			if (!U) continue;
			FRTSCommand C = Cmd;
			if (U->Stats.bCanHarvest) { C.Type = ERTSCommandType::Gather; C.Target = N; }
			else { C.Type = ERTSCommandType::Move; C.Location = H.ImpactPoint; }
			U->IssueCommand(C);
		}
	}
	else
	{
		Cmd.Type = ERTSCommandType::Move;
		Cmd.Location = H.ImpactPoint;
		IssueToUnits(Cmd, true);
	}
}

// ---------------------------------------------------------------- hotkeys

void ARTSPlayerController::HandleKeys()
{
	const bool bCtrl = IsInputKeyDown(EKeys::LeftControl);

	if (WasInputKeyJustPressed(EKeys::A) && Selection.Num()) bAttackMoveArmed = true;
	if (WasInputKeyJustPressed(EKeys::Escape)) bAttackMoveArmed = false;
	if (WasInputKeyJustPressed(EKeys::S))
	{
		FRTSCommand C; C.Type = ERTSCommandType::Stop;
		IssueToUnits(C, false);
	}

	static const FKey Digits[10] = { EKeys::Zero, EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four,
		EKeys::Five, EKeys::Six, EKeys::Seven, EKeys::Eight, EKeys::Nine };
	for (int32 i = 1; i < 10; ++i)
	{
		if (!WasInputKeyJustPressed(Digits[i])) continue;
		if (bCtrl) Groups[i] = Selection;
		else
		{
			ClearSelection();
			for (auto& P : Groups[i]) AddToSelection(P.Get());
		}
	}

	// Production hotkeys on the selected factory.
	static const FKey ProdKeys[4] = { EKeys::Z, EKeys::X, EKeys::C, EKeys::V };
	for (auto& P : Selection)
	{
		ARTSBuilding* B = Cast<ARTSBuilding>(P.Get());
		if (!B || !B->Production->Definition) continue;
		const TArray<FRTSProductionEntry>& List = B->Production->Definition->Produces;
		for (int32 i = 0; i < 4 && i < List.Num(); ++i)
			if (WasInputKeyJustPressed(ProdKeys[i])) B->Production->QueueUnit(List[i].ObjectName);
		if (WasInputKeyJustPressed(EKeys::B)) B->Production->CancelLast();
	}
}

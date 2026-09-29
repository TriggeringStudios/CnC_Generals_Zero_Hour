#pragma once
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "RTSTypes.h"
#include "RTSPlayerController.generated.h"

/**
 * RTS controls. Left click / drag box: select. Right click: contextual order (attack enemy,
 * harvest supplies, or move). Shift: add to selection / queue orders.
 * Arrow keys or screen edge: pan. Wheel: zoom. A + left click: attack-move. S: stop.
 * Ctrl+1..9: save group, 1..9: recall. Selected factory: Z/X/C/V queue units, B cancels; right click sets rally.
 */
UCLASS()
class GENERALSRTS_API ARTSPlayerController : public APlayerController
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere) int32 TeamId = 0;
	UPROPERTY(EditAnywhere) float PanSpeed = 4000.f;
	UPROPERTY(EditAnywhere) float EdgeScrollMargin = 12.f;

	const TArray<TWeakObjectPtr<AActor>>& GetSelection() const { return Selection; }
	bool GetDragRect(FVector2D& Min, FVector2D& Max) const;
	bool IsAttackMoveArmed() const { return bAttackMoveArmed; }
protected:
	virtual void BeginPlay() override;
	virtual void PlayerTick(float DeltaTime) override;
private:
	void UpdateCamera(float DT);
	void HandleMouse();
	void HandleKeys();
	void OnLeftReleased();
	void OnRightClick();
	void ClearSelection();
	void AddToSelection(AActor* A);
	void IssueToUnits(const FRTSCommand& Cmd, bool bFormation);
	bool OwnedAndAlive(AActor* A) const;
	bool GroundUnderCursor(FVector& Out) const;

	TArray<TWeakObjectPtr<AActor>> Selection;
	TArray<TWeakObjectPtr<AActor>> Groups[10];
	bool bDragging = false;
	bool bAttackMoveArmed = false;
	FVector2D DragStart = FVector2D::ZeroVector, DragNow = FVector2D::ZeroVector;
};

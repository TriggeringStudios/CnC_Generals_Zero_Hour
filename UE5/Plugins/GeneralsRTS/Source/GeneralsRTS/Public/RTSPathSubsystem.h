#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "RTSPathSubsystem.generated.h"

/**
 * Grid A* over an unbounded sparse grid (only blocked cells are stored, so map size doesn't matter).
 * Buildings register footprints; paths are smoothed by line-of-sight so units cut corners naturally.
 * The original used a per-cell grid with layers for bridges and cliffs; this is the flat-ground core,
 * and the place to add terrain slope, water and hierarchical (chunked) search for a huge map.
 */
UCLASS()
class GENERALSRTS_API URTSPathSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
public:
	static constexpr float CellSize = 100.f;
	static constexpr int32 MaxExpansions = 20000;
	static constexpr int32 PathsPerFrame = 8;

	/** Block the XY box around Center (world units), inflated by Clearance so units don't clip corners. */
	void AddObstacle(const FVector& Center, const FVector2D& HalfExtent, float Clearance = 100.f);
	void RemoveObstacle(const FVector& Center, const FVector2D& HalfExtent, float Clearance = 100.f);

	bool IsBlocked(const FIntPoint& Cell) const { return Blocked.Contains(Cell); }
	bool HasLineOfSight(const FVector& From, const FVector& To) const;
	/** Waypoints from Start to Goal (excluding Start). Falls back to the closest reachable point. */
	bool FindPath(const FVector& Start, const FVector& Goal, TArray<FVector>& OutPath) const;

	/** Limits pathfinds per sim frame so a mass move order can't hitch the game. */
	void ResetBudget() { Budget = PathsPerFrame; }
	bool ConsumeBudget() { return Budget-- > 0; }

	static FIntPoint ToCell(const FVector& P) { return FIntPoint(FMath::FloorToInt(P.X / CellSize), FMath::FloorToInt(P.Y / CellSize)); }
	static FVector ToWorld(const FIntPoint& C, float Z) { return FVector((C.X + 0.5f) * CellSize, (C.Y + 0.5f) * CellSize, Z); }

private:
	template <typename F> void ForCells(const FVector& Center, const FVector2D& Half, float Clearance, F&& Fn) const;
	FIntPoint NearestFree(const FIntPoint& Cell) const;
	TMap<FIntPoint, int32> Blocked; // cell -> overlapping obstacle count
	int32 Budget = PathsPerFrame;
};

#include "RTSPathSubsystem.h"
#include "Algo/Reverse.h"

template <typename F>
void URTSPathSubsystem::ForCells(const FVector& C, const FVector2D& H, float Clearance, F&& Fn) const
{
	const FIntPoint Lo = ToCell(FVector(C.X - H.X - Clearance, C.Y - H.Y - Clearance, 0.f));
	const FIntPoint Hi = ToCell(FVector(C.X + H.X + Clearance, C.Y + H.Y + Clearance, 0.f));
	for (int32 X = Lo.X; X <= Hi.X; ++X)
		for (int32 Y = Lo.Y; Y <= Hi.Y; ++Y) Fn(FIntPoint(X, Y));
}

void URTSPathSubsystem::AddObstacle(const FVector& C, const FVector2D& H, float Clearance)
{
	ForCells(C, H, Clearance, [this](const FIntPoint& Cell) { Blocked.FindOrAdd(Cell)++; });
}

void URTSPathSubsystem::RemoveObstacle(const FVector& C, const FVector2D& H, float Clearance)
{
	ForCells(C, H, Clearance, [this](const FIntPoint& Cell) {
		if (int32* N = Blocked.Find(Cell)) { if (--(*N) <= 0) Blocked.Remove(Cell); }
	});
}

bool URTSPathSubsystem::HasLineOfSight(const FVector& From, const FVector& To) const
{
	if (Blocked.IsEmpty()) return true;
	const FVector D = To - From;
	const float Len = FVector::Dist2D(From, To);
	const int32 Steps = FMath::CeilToInt(Len / (CellSize * 0.5f));
	// Skip t=0 so a unit standing in an inflated footprint edge can still leave it.
	for (int32 i = 1; i <= Steps; ++i)
		if (Blocked.Contains(ToCell(From + D * (float(i) / Steps)))) return false;
	return true;
}

FIntPoint URTSPathSubsystem::NearestFree(const FIntPoint& Cell) const
{
	if (!IsBlocked(Cell)) return Cell;
	for (int32 R = 1; R <= 20; ++R)
	{
		FIntPoint Best = Cell;
		int32 BestD = MAX_int32;
		for (int32 X = -R; X <= R; ++X)
			for (int32 Y = -R; Y <= R; ++Y)
			{
				if (FMath::Max(FMath::Abs(X), FMath::Abs(Y)) != R) continue;
				const FIntPoint C(Cell.X + X, Cell.Y + Y);
				const int32 D = X * X + Y * Y;
				if (D < BestD && !IsBlocked(C)) { BestD = D; Best = C; }
			}
		if (BestD != MAX_int32) return Best;
	}
	return Cell;
}

namespace
{
	struct FOpen { FIntPoint Cell; float F; };
	float Octile(const FIntPoint& A, const FIntPoint& B)
	{
		const float DX = FMath::Abs(A.X - B.X), DY = FMath::Abs(A.Y - B.Y);
		return (DX + DY) + (1.41421356f - 2.f) * FMath::Min(DX, DY);
	}
}

bool URTSPathSubsystem::FindPath(const FVector& Start, const FVector& Goal, TArray<FVector>& Out) const
{
	Out.Reset();
	const FIntPoint S = ToCell(Start);
	const FIntPoint G = NearestFree(ToCell(Goal)); // goals inside buildings become the nearest open cell
	if (S == G) { Out.Add(Goal); return true; }

	TArray<FOpen> Open;
	TMap<FIntPoint, float> GScore;
	TMap<FIntPoint, FIntPoint> Parent;
	TSet<FIntPoint> Closed;
	auto Less = [](const FOpen& A, const FOpen& B) { return A.F < B.F; };

	GScore.Add(S, 0.f);
	Open.HeapPush({S, Octile(S, G)}, Less);
	FIntPoint Best = S;
	float BestH = Octile(S, G);
	int32 Expanded = 0;
	bool bFound = false;

	while (Open.Num() && Expanded < MaxExpansions)
	{
		FOpen Cur;
		Open.HeapPop(Cur, Less);
		if (Closed.Contains(Cur.Cell)) continue; // stale duplicate heap entry
		Closed.Add(Cur.Cell);
		++Expanded;
		if (Cur.Cell == G) { bFound = true; Best = G; break; }
		const float H = Octile(Cur.Cell, G);
		if (H < BestH) { BestH = H; Best = Cur.Cell; }

		for (int32 DX = -1; DX <= 1; ++DX)
			for (int32 DY = -1; DY <= 1; ++DY)
			{
				if (!DX && !DY) continue;
				const FIntPoint N(Cur.Cell.X + DX, Cur.Cell.Y + DY);
				if (Closed.Contains(N) || IsBlocked(N)) continue;
				// No cutting corners through diagonal gaps.
				if (DX && DY && (IsBlocked(FIntPoint(Cur.Cell.X + DX, Cur.Cell.Y)) || IsBlocked(FIntPoint(Cur.Cell.X, Cur.Cell.Y + DY)))) continue;
				const float NewG = GScore[Cur.Cell] + ((DX && DY) ? 1.41421356f : 1.f);
				const float* Old = GScore.Find(N);
				if (Old && *Old <= NewG) continue;
				GScore.Add(N, NewG);
				Parent.Add(N, Cur.Cell);
				Open.HeapPush({N, NewG + Octile(N, G)}, Less);
			}
	}

	// Rebuild (goal if reached, else the explored cell closest to the goal).
	TArray<FIntPoint> Cells;
	for (FIntPoint C = Best; C != S; C = Parent[C]) Cells.Add(C);
	Algo::Reverse(Cells);
	if (Cells.IsEmpty()) return false;

	// String-pull: skip waypoints we can already see past.
	const float Z = Start.Z;
	FVector Anchor = Start;
	for (int32 i = 0; i < Cells.Num(); ++i)
	{
		const bool bLast = (i == Cells.Num() - 1);
		if (bLast || !HasLineOfSight(Anchor, ToWorld(Cells[i + 1], Z)))
		{
			Anchor = ToWorld(Cells[i], Z);
			Out.Add(Anchor);
		}
	}
	if (bFound && !IsBlocked(ToCell(Goal))) Out.Last() = FVector(Goal.X, Goal.Y, Z);
	return true;
}

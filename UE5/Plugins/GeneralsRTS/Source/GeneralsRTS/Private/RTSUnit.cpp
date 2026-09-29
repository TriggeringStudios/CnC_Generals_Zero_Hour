#include "RTSUnit.h"
#include "RTSSimSubsystem.h"
#include "RTSTeamInfo.h"
#include "RTSEconomyComponent.h"
#include "RTSResourceNode.h"
#include "RTSBuilding.h"
#include "RTSPathSubsystem.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "DrawDebugHelpers.h"

namespace
{
	constexpr float ArriveRadius = 60.f;
	constexpr float HarvestReach = 120.f;
}

ARTSUnit::ARTSUnit()
{
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USphereComponent>(TEXT("Root"));
	Root->InitSphereRadius(50.f);
	Root->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Root->SetCollisionResponseToAllChannels(ECR_Ignore);
	Root->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	SetRootComponent(Root);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Root);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (Cube.Succeeded()) Mesh->SetStaticMesh(Cube.Object);
	Mesh->SetRelativeScale3D(FVector(0.7f));
}

void ARTSUnit::ApplyStats(const FRTSUnitStats& InStats)
{
	Stats = InStats;
	Health = Stats.MaxHealth;
	Root->SetSphereRadius(50.f * Stats.VisualScale);
	Mesh->SetRelativeScale3D(FVector(0.7f * Stats.VisualScale));
}

void ARTSUnit::BeginPlay()
{
	Super::BeginPlay();
	if (URTSSimSubsystem* Sim = GetWorld()->GetSubsystem<URTSSimSubsystem>())
	{
		Sim->RegisterEntity(this);
		SimHandle = Sim->OnSimTick.AddUObject(this, &ARTSUnit::SimTick);
		if (ARTSTeamInfo* T = Sim->GetTeam(TeamId))
			if (UMaterialInstanceDynamic* M = Mesh->CreateAndSetMaterialInstanceDynamic(0))
				M->SetVectorParameterValue(TEXT("Color"), T->TeamColor);
	}
}

void ARTSUnit::EndPlay(const EEndPlayReason::Type Reason)
{
	if (UWorld* W = GetWorld())
		if (URTSSimSubsystem* Sim = W->GetSubsystem<URTSSimSubsystem>())
		{
			Sim->OnSimTick.Remove(SimHandle);
			Sim->UnregisterEntity(this);
		}
	Super::EndPlay(Reason);
}

// ---------------------------------------------------------------- commands

void ARTSUnit::IssueCommand(const FRTSCommand& Cmd)
{
	if (!IsAlive()) return;
	if (Cmd.Type == ERTSCommandType::Stop)
	{
		Queue.Reset();
		bHasOrder = false;
		AutoTarget.Reset();
		MoveDir = FVector::ZeroVector;
		return;
	}
	if (Cmd.bQueued && bHasOrder) { Queue.Add(Cmd); return; }
	Queue.Reset();
	Begin(Cmd);
}

void ARTSUnit::Begin(const FRTSCommand& Cmd)
{
	Current = Cmd;
	bHasOrder = true;
	AutoTarget.Reset();
	if (Cmd.Type == ERTSCommandType::Gather)
	{
		Node = Cast<ARTSResourceNode>(Cmd.Target.Get());
		Phase = (Carried >= Stats.HarvestCapacity) ? EGatherPhase::ToDepot : EGatherPhase::ToNode;
	}
}

void ARTSUnit::NextOrder()
{
	if (Queue.Num())
	{
		const FRTSCommand N = Queue[0];
		Queue.RemoveAt(0);
		Begin(N);
	}
	else
	{
		bHasOrder = false;
		MoveDir = FVector::ZeroVector;
	}
}

// ---------------------------------------------------------------- sim step

float ARTSUnit::Dist2D(const FVector& P) const { return FVector::Dist2D(GetActorLocation(), P); }

void ARTSUnit::SteerTo(const FVector& Dest)
{
	URTSSimSubsystem* Sim = GetWorld()->GetSubsystem<URTSSimSubsystem>();
	URTSPathSubsystem* Paths = GetWorld()->GetSubsystem<URTSPathSubsystem>();
	const FVector Me = GetActorLocation();
	FVector Aim = Dest;

	if (Paths && !Paths->HasLineOfSight(Me, Dest))
	{
		// Something is in the way: (re)path if we have no path, the goal moved, or we were told to.
		const bool bStale = Path.IsEmpty() || FVector::Dist2D(PathGoal, Dest) > URTSPathSubsystem::CellSize * 2.5f;
		if (bStale && Sim->GetFrame() - LastPathFrame >= 10 && Paths->ConsumeBudget())
		{
			LastPathFrame = Sim->GetFrame();
			PathGoal = Dest;
			PathIdx = 0;
			if (!Paths->FindPath(Me, Dest, Path)) Path.Reset();
		}
		while (PathIdx < Path.Num() - 1 && Dist2D(Path[PathIdx]) < ArriveRadius + 20.f) ++PathIdx;
		if (Path.IsValidIndex(PathIdx)) Aim = Path[PathIdx];
	}
	else
	{
		Path.Reset(); // clear shot: go straight
	}

	FVector D = Aim - Me;
	D.Z = 0.f;
	MoveDir = D.GetSafeNormal();
}

bool ARTSUnit::EngageTarget(AActor* Target)
{
	IRTSTargetable* T = Cast<IRTSTargetable>(Target);
	if (!CanAttack() || !T || !T->IsAlive()) return false;
	const float D = Dist2D(Target->GetActorLocation()) - Target->GetSimpleCollisionRadius();
	if (D > Stats.AttackRange)
	{
		SteerTo(Target->GetActorLocation());
		return true;
	}
	// In range: stop and shoot.
	MoveDir = FVector::ZeroVector;
	bFiring = true;
	FVector Face = Target->GetActorLocation() - GetActorLocation();
	Face.Z = 0.f;
	if (!Face.IsNearlyZero()) SetActorRotation(Face.Rotation());
	if (CooldownFrames <= 0)
	{
		CooldownFrames = FMath::Max(1, FMath::RoundToInt(Stats.AttackCooldownSec * RTS_LOGIC_FPS));
		T->ReceiveDamage(Stats.AttackDamage, this);
#if ENABLE_DRAW_DEBUG
		DrawDebugLine(GetWorld(), GetActorLocation(), Target->GetActorLocation(), FColor::Yellow, false, 0.12f, 0, 4.f);
#endif
	}
	return true;
}

void ARTSUnit::SimTick()
{
	if (!IsAlive()) return;
	URTSSimSubsystem* Sim = GetWorld()->GetSubsystem<URTSSimSubsystem>();
	if (!Sim) return;
	if (CooldownFrames > 0) --CooldownFrames;
	MoveDir = FVector::ZeroVector;
	bFiring = false;

	if (!bHasOrder)
	{
		// Idle: defend yourself by auto-acquiring nearby enemies.
		if (CanAttack())
		{
			if (!AutoTarget.IsValid() || !Cast<IRTSTargetable>(AutoTarget.Get())->IsAlive() ||
				Dist2D(AutoTarget->GetActorLocation()) > Stats.AcquireRange * 1.5f)
				AutoTarget = Sim->FindNearestEnemy(GetActorLocation(), TeamId, Stats.AcquireRange);
			if (AutoTarget.IsValid()) EngageTarget(AutoTarget.Get());
		}
		ApplySeparation();
		return;
	}

	switch (Current.Type)
	{
	case ERTSCommandType::Move:
		if (Dist2D(Current.Location) <= ArriveRadius) NextOrder();
		else SteerTo(Current.Location);
		break;

	case ERTSCommandType::AttackMove:
	{
		AActor* Enemy = CanAttack() ? Sim->FindNearestEnemy(GetActorLocation(), TeamId, Stats.AcquireRange) : nullptr;
		if (Enemy) EngageTarget(Enemy);
		else if (Dist2D(Current.Location) <= ArriveRadius) NextOrder();
		else SteerTo(Current.Location);
		break;
	}
	case ERTSCommandType::Attack:
		if (!EngageTarget(Current.Target.Get())) NextOrder();
		break;

	case ERTSCommandType::Gather:
		TickGather();
		break;

	default:
		NextOrder();
		break;
	}

	ApplySeparation();
}

void ARTSUnit::ApplySeparation()
{
	// Moving units steer around each other; parked units get nudged apart (but not while mining).
	URTSSimSubsystem* Sim = GetWorld()->GetSubsystem<URTSSimSubsystem>();
	const FVector Push = Sim->GetSeparation(this, Root->GetScaledSphereRadius() * 2.2f);
	if (!MoveDir.IsNearlyZero()) MoveDir = (MoveDir + Push * 1.2f).GetSafeNormal();
	else if (!bFiring && !Push.IsNearlyZero() && !(bHasOrder && Current.Type == ERTSCommandType::Gather))
		MoveDir = Push.GetSafeNormal() * 0.4f;
}

void ARTSUnit::TickGather()
{
	URTSSimSubsystem* Sim = GetWorld()->GetSubsystem<URTSSimSubsystem>();
	if (!Stats.bCanHarvest) { NextOrder(); return; }

	switch (Phase)
	{
	case EGatherPhase::ToNode:
	{
		if (!Node.IsValid() || Node->IsDepleted()) Node = Sim->FindNearestNode(GetActorLocation());
		if (!Node.IsValid())
		{
			if (Carried > 0.f) Phase = EGatherPhase::ToDepot; else NextOrder();
			return;
		}
		if (Dist2D(Node->GetActorLocation()) <= Node->Radius + HarvestReach) Phase = EGatherPhase::Harvesting;
		else SteerTo(Node->GetActorLocation());
		break;
	}
	case EGatherPhase::Harvesting:
	{
		if (!Node.IsValid()) { Phase = (Carried > 0.f) ? EGatherPhase::ToDepot : EGatherPhase::ToNode; break; }
		Carried += Node->Extract(FMath::Min(Stats.HarvestRate * RTS_LOGIC_DT, Stats.HarvestCapacity - Carried));
		if (Carried >= Stats.HarvestCapacity - KINDA_SMALL_NUMBER) Phase = EGatherPhase::ToDepot;
		break;
	}
	case EGatherPhase::ToDepot:
	{
		ARTSBuilding* Depot = Sim->FindNearestDepot(GetActorLocation(), TeamId);
		if (!Depot) return; // wait; nothing to deliver to
		if (Dist2D(Depot->GetActorLocation()) <= Depot->GetSimpleCollisionRadius() + HarvestReach)
		{
			if (ARTSTeamInfo* Team = Sim->GetTeam(TeamId)) Team->Economy->Deposit(FMath::RoundToInt(Carried));
			Carried = 0.f;
			Phase = EGatherPhase::ToNode;
		}
		else SteerTo(Depot->GetActorLocation());
		break;
	}
	}
}

void ARTSUnit::ReceiveDamage(float Amount, AActor* From)
{
	if (!IsAlive()) return;
	Health -= Amount;
	if (Health <= 0.f) Destroy();
}

// ---------------------------------------------------------------- per-frame visuals

void ARTSUnit::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (!MoveDir.IsNearlyZero())
	{
		SetActorLocation(GetActorLocation() + MoveDir * Stats.MoveSpeed * DeltaTime);
		SetActorRotation(FMath::RInterpTo(GetActorRotation(), MoveDir.Rotation(), DeltaTime, 12.f));
	}
#if ENABLE_DRAW_DEBUG
	const float R = Root->GetScaledSphereRadius();
	const FVector C = GetActorLocation();
	if (bSelected)
		DrawDebugCircle(GetWorld(), C - FVector(0, 0, R - 5.f), R + 15.f, 20, FColor::Green, false, -1.f, 0, 3.f,
			FVector(1, 0, 0), FVector(0, 1, 0), false);
	if (bSelected || Health < Stats.MaxHealth)
	{
		const float Pct = FMath::Clamp(Health / Stats.MaxHealth, 0.f, 1.f);
		const FVector L = C + FVector(0, -R, R + 40.f), Rt = C + FVector(0, R, R + 40.f);
		DrawDebugLine(GetWorld(), L, Rt, FColor::Red, false, -1.f, 0, 6.f);
		DrawDebugLine(GetWorld(), L, FMath::Lerp(L, Rt, Pct), FColor::Green, false, -1.f, 0, 6.f);
	}
#endif
}

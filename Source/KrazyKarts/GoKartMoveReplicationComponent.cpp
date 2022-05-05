// Fill out your copyright notice in the Description page of Project Settings.


#include "GoKartMoveReplicationComponent.h"
#include "Net/UnrealNetwork.h"

UGoKartMoveReplicationComponent::UGoKartMoveReplicationComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	SetIsReplicated(true);
}

void UGoKartMoveReplicationComponent::BeginPlay()
{
	Super::BeginPlay();

	MovementComponent = GetOwner()->FindComponentByClass<UGoKartMovementComponent>();
}

void UGoKartMoveReplicationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!ensure(MovementComponent != nullptr)) return;

	FGoKartMove Move = MovementComponent->GetLastMove();

	if (GetOwnerRole() == ROLE_AutonomousProxy)
	{
		UnackMoves.Add(Move);
		Server_SendMove(Move);
	}
	else if (GetOwner()->GetRemoteRole() == ROLE_SimulatedProxy)
	{
		UpdateServerState(Move);
	}
	else if (GetOwnerRole() == ROLE_SimulatedProxy)
	{
		ClientTick(DeltaTime);
	}
}

void UGoKartMoveReplicationComponent::ClientTick(float DeltaTime)
{
	// MovementComponent->SimulateMove(ServerState.LastMove);
	ClientTimeSinceUpdate += DeltaTime;

	if (ClientTimeSinceUpdate < KINDA_SMALL_NUMBER) return;

	FVector TargetLocation = ServerState.Transform.GetLocation();
	float LerpRatio = ClientTimeSinceUpdate / ClientTimeBetweenLastUpdates;
	FVector StartLocation = ClientStartLocation;
	FVector NewLocation = FMath::LerpStable(
		StartLocation,
		TargetLocation,
		LerpRatio
	);
	GetOwner()->SetActorLocation(NewLocation);
}

void UGoKartMoveReplicationComponent::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UGoKartMoveReplicationComponent, ServerState);
}

void UGoKartMoveReplicationComponent::OnRep_ServerState()
{
	switch (GetOwnerRole())
	{
	case ROLE_AutonomousProxy:
		AutonomousProxy_OnRep_ServerState();
		break;
	case ROLE_SimulatedProxy:
		SimulatedProxy_OnRep_ServerState();
		break;
	default:
		break;
	}
}

void UGoKartMoveReplicationComponent::AutonomousProxy_OnRep_ServerState()
{
	if (!ensure(MovementComponent != nullptr)) return;
	
	GetOwner()->SetActorTransform(ServerState.Transform);
	MovementComponent->SetVelocity(ServerState.VelocityMetersPerSecond);
	ClearAcknowledgedMoves(ServerState.LastMove);

	for (const FGoKartMove& Move : UnackMoves)
	{
		MovementComponent->SimulateMove(Move);
	}
}

void UGoKartMoveReplicationComponent::SimulatedProxy_OnRep_ServerState()
{
	ClientTimeBetweenLastUpdates = ClientTimeSinceUpdate;
	ClientTimeSinceUpdate = 0.f;
	ClientStartLocation = GetOwner()->GetActorLocation();
}

void UGoKartMoveReplicationComponent::ClearAcknowledgedMoves(const FGoKartMove& LastMove)
{
	UnackMoves.RemoveAll([LastMove](const FGoKartMove& Move) {
		return Move.Time < LastMove.Time;
	});
}

void UGoKartMoveReplicationComponent::UpdateServerState(const FGoKartMove& Move)
{
	ServerState.LastMove = Move;
	ServerState.Transform = GetOwner()->GetActorTransform();
	ServerState.VelocityMetersPerSecond = MovementComponent->GetVelocity();
}

void UGoKartMoveReplicationComponent::Server_SendMove_Implementation(FGoKartMove Move)
{
	if (!ensure(MovementComponent != nullptr)) return;
	MovementComponent->SimulateMove(Move);
	UpdateServerState(Move);
}

bool UGoKartMoveReplicationComponent::Server_SendMove_Validate(FGoKartMove Move)
{
	return true;
}


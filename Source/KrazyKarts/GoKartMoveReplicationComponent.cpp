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
	OwningPawn = Cast<APawn>(GetOwner());
}

void UGoKartMoveReplicationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!ensure(MovementComponent != nullptr)) return;

	if (GetOwnerRole() == ROLE_AutonomousProxy)
	{
		FGoKartMove Move = MovementComponent->CreateMove(DeltaTime);
		MovementComponent->SimulateMove(Move);
		UnackMoves.Add(Move);
		Server_SendMove(Move);
	}
	else if (GetOwnerRole() == ROLE_Authority && OwningPawn->IsLocallyControlled())
	{
		FGoKartMove Move = MovementComponent->CreateMove(DeltaTime);
		Server_SendMove(Move);
	}
	else if (GetOwnerRole() == ROLE_SimulatedProxy)
	{
		MovementComponent->SimulateMove(ServerState.LastMove);
	}
}

void UGoKartMoveReplicationComponent::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UGoKartMoveReplicationComponent, ServerState);
}

void UGoKartMoveReplicationComponent::OnRep_ServerState()
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

void UGoKartMoveReplicationComponent::ClearAcknowledgedMoves(const FGoKartMove& LastMove)
{
	UnackMoves.RemoveAll([LastMove](const FGoKartMove& Move) {
		return Move.Time < LastMove.Time;
	});
}

void UGoKartMoveReplicationComponent::Server_SendMove_Implementation(FGoKartMove Move)
{
	if (!ensure(MovementComponent != nullptr)) return;

	MovementComponent->SimulateMove(Move);

	ServerState.LastMove = Move;
	ServerState.Transform = OwningPawn->GetActorTransform();
	ServerState.VelocityMetersPerSecond = MovementComponent->GetVelocity();
}

bool UGoKartMoveReplicationComponent::Server_SendMove_Validate(FGoKartMove Move)
{
	return true;
}

